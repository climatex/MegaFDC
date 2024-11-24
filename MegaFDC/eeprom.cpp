// MegaFDC (c) 2023-2024 J. Bogin, http://boginjr.com
// EEPROM helper functions to load and save drives configuration

#include "config.h"

#ifndef BUILD_IMD_IMAGER

// initialized EEPROM data structure
// offset (dec.), length in bytes, description:

// +00, 01, checksum byte
// +01, 01, number of drives configured
// +02, 27, A: drive configuration
// +29, 27, B: drive configuration
// +56, 27, C: drive configuration
// +83, 27, D: drive configuration
// +110 to 4K: 0s

// simple 8-bit checksum
BYTE eepromComputeChecksum()
{
  BYTE checksum = 0;
  for (WORD index = 0; index < EEPROM.length(); index++)
  {
    checksum += (BYTE)EEPROM.read(index);
  }
  
  return checksum;
}

// overwrite checksum byte and set number of drives stored to 0
void eepromClearConfiguration()
{
  EEPROM.update(0, 0xFF);
  EEPROM.update(1, 0);
}

// store number of drives, up to 4 structs of configured drives and a checksum byte
void eepromStoreConfiguration()
{
  WORD eepromOffset = 0;
  
  // update number of drives
  EEPROM.update(eepromOffset++, 0);
  EEPROM.update(eepromOffset++, g_numberOfDrives);
  
  // A: to D:
  for (BYTE driveNo = 0; driveNo < g_numberOfDrives; driveNo++)
  {
    // access struct per byte
    const BYTE* drive = (const BYTE*)(&g_diskDrives[driveNo]);
    
    for (BYTE bufIndex = 0; bufIndex < sizeof(FDC::DiskDriveMediaParams); bufIndex++)
    {
      EEPROM.update(eepromOffset++, drive[bufIndex]);
    }
  }
  
  // the rest is zeros
  while (eepromOffset < EEPROM.length())
  {
    EEPROM.update(eepromOffset++, 0);
  }
    
  // fill in the first byte so that the EEPROM checksum equals 0
  BYTE checksum = eepromComputeChecksum();
  if (checksum != 0)
  {
    checksum = 0x100 - checksum;
    EEPROM.update(0, checksum);
  }
}

bool eepromIsConfigurationPresent()
{
  // check drive count validity
  const BYTE driveCount = EEPROM.read(1);  
  if ((driveCount == 0) || (driveCount > 4))
  {
    return false;
  }
   
  // checksum must be zero
  if (eepromComputeChecksum() != 0)
  {
    return false;
  }
  
  return true;
}

// load configuration
bool eepromLoadConfiguration()
{
  if (!eepromIsConfigurationPresent())
  {
    return false;   
  }
  
  // proceed
  BYTE eepromOffset = 1;
  
  if (g_diskDrives)
  {
    delete[] g_diskDrives;
  }
  
  g_numberOfDrives = EEPROM.read(eepromOffset++);
  g_diskDrives = new FDC::DiskDriveMediaParams[g_numberOfDrives];
  
  for (BYTE driveNo = 0; driveNo < g_numberOfDrives; driveNo++)
  {
    BYTE* drive = (BYTE*)(&g_diskDrives[driveNo]);
    
    for (BYTE bufIndex = 0; bufIndex < sizeof(FDC::DiskDriveMediaParams); bufIndex++)
    {
      drive[bufIndex] = EEPROM.read(eepromOffset++);
    }
  }

  return true;
}

#endif