// MegaFDC (c) 2023-2026 J. Bogin, http://boginjr.com
// File transfer helpers (XMODEM, SD)

#include "config.h"

WORD xmDataPos;
WORD xmRWPos;
WORD totalSectorsCount;
WORD badSectorsCount;
bool success;

int xmodemRx(int msDelay) 
{ 
  const DWORD start = millis();
  while ((millis()-start) < msDelay)
  { 
    if (Serial.available())
    {
      return (BYTE)Serial.read();
    }
  }

  return -1; 
}

void xmodemTx(const char *data, int size)
{  
  Serial.write((const BYTE*)data, size);
}

// dump serial transfer if not successful
void dumpSerialTransfer()
{
  const BYTE CAN = 0x18;
  
  DWORD delayMs = millis() + 10;
  while (millis() < delayMs)
  {
    if (Serial.read() >= 0)
    {
      delayMs = millis() + 10;
    }
  }
  
  while (Serial.available() > 0)
  {
    Serial.read(); 
  }
  
  Serial.write(&CAN, sizeof(BYTE));
  Serial.write(&CAN, sizeof(BYTE));
  Serial.write(&CAN, sizeof(BYTE));
}

#ifndef BUILD_IMD_IMAGER
FRESULT lastResult;

#ifndef SD_CARD_SUPPORT

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////// disk image over XMODEM transfers //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 2 callbacks to send over disk image files
// data sent over XMODEM in 128 or 1024 byte chunks
bool xmodemImageRxCallback(DWORD no, BYTE* data, WORD size)
{ 
  // break on overflow
  if (xmDataPos + size > SECTOR_BUFFER_SIZE)
  {
    success = false;
    return false;
  }
  
  // copy data to RW buffer
  memcpy(&g_rwBuffer[xmDataPos], data, size);
  xmDataPos += size;  
 
  if (xmDataPos == SECTOR_BUFFER_SIZE)
  {
    // flush buffer to disk
    xmDataPos = xmRWPos = 0;  
  }
  else
  {
    // continue filling up
    return true;
  }
  
  // write operation
  while(xmRWPos != SECTOR_BUFFER_SIZE)
  {
    if (totalSectorsCount >= fdc->getTotalSectorCount())
    {
      // end of disk reached, transfer over
      return false;
    }

    // convert current position to CHS    
    BYTE cyl;
    BYTE head;
    BYTE startSector;
    fdc->convertLogicalSectorToCHS(totalSectorsCount, cyl, head, startSector);
    
    // how many sectors till end of track or SECTOR_BUFFER_SIZE constraint
    const BYTE sectorCount = fdc->getMaximumSectorCountForRW(startSector, SECTOR_BUFFER_SIZE - xmRWPos);
    const BYTE endSector = startSector + sectorCount-1;
    
    ui->print(Progmem::getString(Progmem::diskIoProgress), cyl, head);
    
    // seek if needed, inform about cyl and head
    if ((fdc->getCurrentCylinder() != cyl) || (fdc->getCurrentHead() != head))
    {
      fdc->seekDrive(cyl, head);      
    }
    
    // write    
    fdc->readWriteSectors(true, startSector, endSector, &xmRWPos);
    if (fdc->getLastError())
    {
      // do not retry if disk is write protected or there is no disk in drive
      if (fdc->wasErrorNoDiskInDrive() || fdc->wasErrorDiskProtected())
      {
        success = false;
        return false;
      }
      
      // mark bad sectors range
      badSectorsCount += sectorCount;
    }
    
    // increment RW buffer position
    totalSectorsCount += sectorCount;
    xmRWPos += fdc->getParams()->SectorSizeBytes * sectorCount;
  }
  
  return true;
}

// transmit callback, analog to the one above
bool xmodemImageTxCallback(DWORD no, BYTE* data, WORD size)
{
  bool endOfDisk = false;
  
  // read from disk until buffer full
  while(xmRWPos != SECTOR_BUFFER_SIZE)
  {
    if (totalSectorsCount >= fdc->getTotalSectorCount())
    {
      endOfDisk = true;
      break;
    }
            
    BYTE cyl;
    BYTE head;
    BYTE startSector;
    fdc->convertLogicalSectorToCHS(totalSectorsCount, cyl, head, startSector);
    
    const BYTE sectorCount = fdc->getMaximumSectorCountForRW(startSector, SECTOR_BUFFER_SIZE - xmRWPos);
    const BYTE endSector = startSector + sectorCount-1;
    
    ui->print(Progmem::getString(Progmem::diskIoProgress), cyl, head);
    
    if ((fdc->getCurrentCylinder() != cyl) || (fdc->getCurrentHead() != head))
    {
      fdc->seekDrive(cyl, head);
    }
       
    fdc->readWriteSectors(false, startSector, endSector, &xmRWPos);
    if (fdc->getLastError())
    {
      // write protect check skipped here as we are reading
      if (fdc->wasErrorNoDiskInDrive())
      {
        success = false;
        return false;
      }
      
      // clear out offending bad sectors with 0s inside the disk R/W buffer
      memset(&g_rwBuffer[xmRWPos], 0, sectorCount * fdc->getParams()->SectorSizeBytes);
      badSectorsCount += sectorCount;
    }
    
    totalSectorsCount += sectorCount;
    xmRWPos += fdc->getParams()->SectorSizeBytes * sectorCount;
  }
  
  // last sector reached?
  if (endOfDisk)
  {
    if (xmDataPos >= xmRWPos)
    {
      // buffer finished, transfer over
      return false;
    }
    
    // buffer not yet finished, transfer the remainder
    memcpy(data, &g_rwBuffer[xmDataPos], size);
    xmDataPos += size;    
    return true;
  }
  
  // not the end of disk, continuing read - copy rw buffer to data
  memcpy(data, &g_rwBuffer[xmDataPos], size);
  xmDataPos += size;
  if (xmDataPos == xmRWPos) // buffer full, read disk at next call
  {
    xmDataPos = xmRWPos = 0;  
  }
  
  return true;
}

bool xmodemReadDiskIntoImageFile(bool useXMODEM_1K)
{
  // initialize
  totalSectorsCount = badSectorsCount = xmRWPos = xmDataPos = 0;
  success = true;
  
  // detect disk presence
  if (!fdc->verifyTrack0())
  {
    return false;
  }
  
  // set auto motor off disabled during waits on serial
  fdc->setAutomaticMotorOff(false);
  
  ui->print("");
  ui->print(Progmem::getString(useXMODEM_1K ? Progmem::xmodem1kPrefix : Progmem::xmodemPrefix));
  ui->print(Progmem::getString(Progmem::xmodemWaitRecv));
  
  // disable printing over serial and keyboard input
  ui->disableKeyboard(true);
  ui->setPrintDisabled(false, true);
  
  XModem modem(xmodemRx, xmodemTx, xmodemImageTxCallback, useXMODEM_1K);
  bool result = modem.transmit() && success;
  dumpSerialTransfer();
    
  fdc->seekDrive(0, 0);
  ui->setPrintDisabled(false, false);
  
  // some remains of a dumped transfer can be present on the terminal, clear it
  ui->print(Progmem::getString(Progmem::uiVT100ClearScreen));
  ui->print(Progmem::getString(Progmem::uiDeleteLine));
  
  if (!result)
  { 
    ui->print(Progmem::getString(Progmem::xmodemTransferFail));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
  
  // show bad sectors if any
  else
  {
    if (badSectorsCount)
    {
      ui->print(Progmem::getString(Progmem::diskIoTotalBadSect), badSectorsCount);
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
    }
    else
    {
      ui->print(Progmem::getString(Progmem::xmodemTransferEnd));
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
    }
  }
  
  // reenable motor timer
  ui->disableKeyboard(false);
  fdc->setAutomaticMotorOff(true);
  
  return result;
}

// analog to one above
bool xmodemWriteDiskFromImageFile(bool useXMODEM_1K)
{
  totalSectorsCount = badSectorsCount = xmRWPos = xmDataPos = 0;
  success = true;
  
  if (!fdc->verifyTrack0(true))
  {
    return false;
  }
  
  fdc->setAutomaticMotorOff(false);
  
  ui->print("");
  ui->print(Progmem::getString(useXMODEM_1K ? Progmem::xmodem1kPrefix : Progmem::xmodemPrefix));
  ui->print(Progmem::getString(Progmem::xmodemWaitSend));
  
  ui->disableKeyboard(true);
  ui->setPrintDisabled(false, true);
  XModem modem(xmodemRx, xmodemTx, xmodemImageRxCallback, useXMODEM_1K);
  bool result = modem.receive() && success;
    
  // if transfer is over and there's any remainder in buffer, flush it
  if (result && (xmDataPos < SECTOR_BUFFER_SIZE))
  {
    BYTE dummy = 0;
    xmDataPos = SECTOR_BUFFER_SIZE;
    xmodemImageRxCallback(0, &dummy, 0);
  }  
  
  dumpSerialTransfer();  
  ui->setPrintDisabled(false, false);
  fdc->seekDrive(0, 0);
  
  ui->print(Progmem::getString(Progmem::uiVT100ClearScreen));
  ui->print(Progmem::getString(Progmem::uiDeleteLine));
  
  if (!result)
  {  
    ui->print(Progmem::getString(Progmem::xmodemTransferFail));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
  
  else
  {
    if (badSectorsCount)
    {
      ui->print(Progmem::getString(Progmem::diskIoTotalBadSect), badSectorsCount);
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
    }
    else
    {
      ui->print(Progmem::getString(Progmem::xmodemTransferEnd));
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
    }
  }
  
  ui->disableKeyboard(false);
  fdc->setAutomaticMotorOff(true);
  
  return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// files over XMODEM transfers /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// always 128 byte packets here

bool xmodemFileTxCallback(DWORD no, BYTE* data, WORD size)
{
  // FAT12
  if (fdc->getParams()->UseFAT12)
  {
    lastResult = f_read(getFatFile(), data, size, &xmRWPos);
    if (lastResult != FR_OK)
    {
      // filesystem error, quit
      success = false;
      return false;
    }
    
    // transfer over
    else if (!xmRWPos)
    {
      return false;
    }
    
    // fill buffer with ASCII end-of-file as we don't know the exact transfer length
    while (xmRWPos < size)
    {
      data[xmRWPos] = 26;
      xmRWPos++;
    }
    
    // continue TX
    return true;
  }
  
  // CP/M, file records are 128 bytes
  else if (fdc->getParams()->UseCPMFS)
  {
    BYTE outputSize;
    if (cpmReadFileRecord(outputSize))
    {
      if (outputSize)
      {
        // success, continue TX
        memcpy(data, &g_rwBuffer[0], size);
        return true;
      }
      
      else
      {
        // success, end of TX
        return false;
      }
    }
    
    // disk read failure
    success = false;
    return false;
  }
  
  return false;
}

bool xmodemFileRxCallback(DWORD no, BYTE* data, WORD size)
{
  // FAT12
  if (!fdc->getParams()->UseFAT12)
  {
    return false;
  }
  
  lastResult = f_write(getFatFile(), data, size, &xmRWPos);
  if (lastResult != FR_OK)
  {
    success = false;
    return false;
  }
  
  // continue RX; callback won't be fired on transfer over
  return true;
}

// analog to sending images, but works with file access
bool xmodemSendFile(const BYTE* existingFileName)
{
  xmRWPos = 0;
  lastResult = FR_OK;
  success = true;
  
  const bool fat = fdc->getParams()->UseFAT12;  
  
  if (fat)
  {
    if (!fatMount())
    {
      return false;
    }  
    
    BYTE addPath[MAX_PATH+1] = {0};
    strcat(addPath, g_path);
    strcat(addPath, existingFileName);
    
    FAT_EXECUTE_0(f_open(getFatFile(), addPath, FA_READ));
  }

  // CP/M, open file name for user 0
  else
  {
    if (!cpmOpenFile(existingFileName, 0))
    {
      return false;
    }
  }
  
  ui->print("");
  ui->print(Progmem::getString(Progmem::xmodemPrefix));
  ui->print(Progmem::getString(Progmem::xmodemWaitRecv));
  
  ui->disableKeyboard(true);  
  ui->setPrintDisabled(false, true);
  fdc->setAutomaticMotorOff(false);
  
  XModem modem(xmodemRx, xmodemTx, xmodemFileTxCallback);
  bool result = modem.transmit() && success;
  if (fat)
  {
    f_close(getFatFile());
  }
  else
  {
    cpmCloseFile();
  }    
  
  ui->disableKeyboard(false);
  fdc->setAutomaticMotorOff(true);
  
  dumpSerialTransfer(); 
  ui->setPrintDisabled(false, false);
  
  ui->print(Progmem::getString(Progmem::uiVT100ClearScreen));
  ui->print(Progmem::getString(Progmem::uiDeleteLine));
  
  if (!result)
  {  
    if (fat)
    {
      fatResult(lastResult);  
    }
    else
    {
      ui->print(Progmem::getString(Progmem::fsDiskError));
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
    }
    
    ui->print(Progmem::getString(Progmem::xmodemTransferFail));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
  
  else
  {
    ui->print(Progmem::getString(Progmem::xmodemTransferEnd));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
    
  return result;
}

bool xmodemReceiveFile(const BYTE* newFileName)
{
  xmRWPos = 0;
  lastResult = FR_OK;
  success = true;
  
  if (!fatMount())
  {
    return false;
  }
  
  BYTE addPath[MAX_PATH+1] = {0};
  strcat(addPath, g_path);
  strcat(addPath, newFileName);
  
  // attempt to delete the file if it already exists (CREATE_NEW would fail otherwise)
  f_unlink(addPath);
  
  FAT_EXECUTE_0(f_open(getFatFile(), addPath, FA_WRITE | FA_CREATE_NEW));
  
  ui->print("");
  ui->print(Progmem::getString(Progmem::xmodemPrefix));
  ui->print(Progmem::getString(Progmem::xmodemWaitSend));
  
  ui->disableKeyboard(true);  
  ui->setPrintDisabled(false, true);
  fdc->setAutomaticMotorOff(false);
  
  XModem modem(xmodemRx, xmodemTx, xmodemFileRxCallback);
  bool result = modem.receive() && success;
  f_close(getFatFile());
  
  ui->disableKeyboard(false);
    
  dumpSerialTransfer();  
  ui->setPrintDisabled(false, false);
  fdc->setAutomaticMotorOff(true);
  
  ui->print(Progmem::getString(Progmem::uiVT100ClearScreen));
  ui->print(Progmem::getString(Progmem::uiDeleteLine));
  
  if (!result)
  {
    fatResult(lastResult);
    
    ui->print(Progmem::getString(Progmem::xmodemTransferFail));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
  
  else
  {
    ui->print(Progmem::getString(Progmem::xmodemTransferEnd));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
    
  return result;
}

#else // SD_CARD_SUPPORT
  
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// disk images from/to an SD card ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool sdReadDiskIntoImageFile()
{ 
  totalSectorsCount = badSectorsCount = 0;  
  
  // detect disk presence
  if (!fdc->verifyTrack0())
  {
    return false;
  }
  
  // detect memory card and pick file to write
  if (!SdDetectMemoryCard())
  {
    return false;
  }
  
  File* file = SdPickFile(true);
  if (!file)
  {
    return false;
  }
  
  // set keyb and auto motor off disabled
  ui->disableKeyboard(true);
  fdc->setAutomaticMotorOff(false);  
  ui->print("");
  
  success = true;      
  while(true)
  {
    if (totalSectorsCount >= fdc->getTotalSectorCount())
    {
      break; // end of disk
    }
            
    BYTE cyl;
    BYTE head;
    BYTE startSector;
    fdc->convertLogicalSectorToCHS(totalSectorsCount, cyl, head, startSector);
    
    const BYTE sectorCount = fdc->getMaximumSectorCountForRW(startSector);
    const BYTE endSector = startSector + sectorCount-1;
    
    ui->print(Progmem::getString(Progmem::diskIoProgress), cyl, head);
    
    if ((fdc->getCurrentCylinder() != cyl) || (fdc->getCurrentHead() != head))
    {
      fdc->seekDrive(cyl, head);
    }
       
    fdc->readWriteSectors(false, startSector, endSector);
    if (fdc->getLastError())
    {
      // write protect check skipped here as we are reading
      if (fdc->wasErrorNoDiskInDrive())
      {
        success = false;
        break;
      }
      
      // clear out offending bad sectors with 0s inside the disk R/W buffer
      memset(&g_rwBuffer[0], 0, sectorCount * fdc->getParams()->SectorSizeBytes);
      badSectorsCount += sectorCount;
    }
    
    // write to SD card
    PORTC ^= 0x80; // toggle activity LED
    const size_t result = file->write((const BYTE*)&g_rwBuffer[0], sectorCount * fdc->getParams()->SectorSizeBytes);
    const bool flushed = file->sync();
    PORTC ^= 0x80;
    if (!result || !flushed)          
    {
      ui->print(Progmem::getString(Progmem::uiNewLine));
      ui->print(Progmem::getString(Progmem::sdCardError));
      ui->print(Progmem::getString(Progmem::uiNewLine));
      success = false;
      break; 
    }
    
    totalSectorsCount += sectorCount;
  }
    
  ui->print(Progmem::getString(Progmem::uiDeleteLine));
  file->close();
  fdc->seekDrive(0, 0);
  
  if (!success)
  { 
    ui->print(Progmem::getString(Progmem::xmodemTransferFail));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
  
  // show bad sectors if any
  else
  {
    if (badSectorsCount)
    {
      ui->print(Progmem::getString(Progmem::diskIoTotalBadSect), badSectorsCount);
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
    }
    else
    {
      ui->print(Progmem::getString(Progmem::xmodemTransferEnd));
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
    }
  }
  
  // reenable motor timer
  ui->disableKeyboard(false);
  fdc->setAutomaticMotorOff(true);
  
  return success;
}

bool sdWriteDiskFromImageFile()
{
  totalSectorsCount = badSectorsCount = 0;
  
  if (!fdc->verifyTrack0(true))
  {
    return false;
  }
  
  if (!SdDetectMemoryCard())
  {
    return false;
  }
  
  File* file = SdPickFile(false);
  if (!file)
  {
    return false;
  }

  ui->disableKeyboard(true);
  fdc->setAutomaticMotorOff(false);  
  ui->print("");
  
  success = true;      
  while(true)
  {
    if (totalSectorsCount >= fdc->getTotalSectorCount())
    {
      break;
    }
 
    BYTE cyl;
    BYTE head;
    BYTE startSector;
    fdc->convertLogicalSectorToCHS(totalSectorsCount, cyl, head, startSector);
    
    const BYTE sectorCount = fdc->getMaximumSectorCountForRW(startSector);
    const BYTE endSector = startSector + sectorCount-1;
    const WORD length = sectorCount * fdc->getParams()->SectorSizeBytes;
    
    // read from SD card
    PORTC ^= 0x80; // toggle activity LED
    int result = file->read((BYTE*)&g_rwBuffer[0], length);
    PORTC ^= 0x80;
    if (result != length)
    {
      ui->print(Progmem::getString(Progmem::uiNewLine));
      ui->print(Progmem::getString((result == -1) ? Progmem::sdCardError : Progmem::sdFileEnd));
      ui->print(Progmem::getString(Progmem::uiNewLine));
      success = false;
      break; 
    }
    
    ui->print(Progmem::getString(Progmem::diskIoProgress), cyl, head);    
    if ((fdc->getCurrentCylinder() != cyl) || (fdc->getCurrentHead() != head))
    {
      fdc->seekDrive(cyl, head);      
    }
    
    // write    
    fdc->readWriteSectors(true, startSector, endSector);
    if (fdc->getLastError())
    {
      // do not retry if disk is write protected or there is no disk in drive
      if (fdc->wasErrorNoDiskInDrive() || fdc->wasErrorDiskProtected())
      {
        success = false;
        break;
      }
      
      badSectorsCount += sectorCount;
    }
    
    totalSectorsCount += sectorCount;
  }
  
  ui->print(Progmem::getString(Progmem::uiDeleteLine));
  file->close();
  fdc->seekDrive(0, 0);
   
  if (!success)
  {  
    ui->print(Progmem::getString(Progmem::xmodemTransferFail));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
  
  else
  {
    if (badSectorsCount)
    {
      ui->print(Progmem::getString(Progmem::diskIoTotalBadSect), badSectorsCount);
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
    }
    else
    {
      ui->print(Progmem::getString(Progmem::xmodemTransferEnd));
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
    }
  }
  
  ui->disableKeyboard(false);
  fdc->setAutomaticMotorOff(true);
  
  return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////// files from/to an SD card //////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool fileFloppyToSd(const BYTE* existingFileName)
{
  BYTE buf[512];
  lastResult = FR_OK;
  const bool fat = fdc->getParams()->UseFAT12;  
  
  if (fat)
  {
    if (!fatMount())
    {
      return false;
    }  
    
    buf[0] = 0;
    strcat(buf, g_path);
    strcat(buf, existingFileName);
    
    FAT_EXECUTE_0(f_open(getFatFile(), buf, FA_READ));
  }

  else
  {
    if (!cpmOpenFile(existingFileName, 0))
    {
      return false;
    }
  }
  
  if (!SdDetectMemoryCard())
  {
    if (fat) f_close(getFatFile()); else cpmCloseFile();
    return false;
  }
  
  File* file = SdPickFile(true);
  if (!file)
  {
    if (fat) f_close(getFatFile()); else cpmCloseFile();
    return false;
  }

  ui->disableKeyboard(true);
  fdc->setAutomaticMotorOff(false);
  ui->print("");  
  
  success = true;
  while (true)
  {
    xmRWPos = 0;
    
    if (fat)
    {
      lastResult = f_read(getFatFile(), &buf[0], sizeof(buf), &xmRWPos);
      if (lastResult != FR_OK)
      {
        success = false;
        break;
      }
      
      else if (!xmRWPos)
      {
        break;
      }
    }
    
    else
    {
      BYTE outputSize;
      if (cpmReadFileRecord(outputSize))
      {
        if (outputSize)
        {
          memcpy(&buf[0], &g_rwBuffer[0], outputSize);
          xmRWPos = outputSize;          
        }
        
        else
        {
          break;
        }
      }
      
      else
      {
        // disk read failure
        success = false;
        break;
      }
    }
    
    // write to SD card
    PORTC ^= 0x80; // toggle activity LED
    const size_t result = file->write((const BYTE*)&buf[0], xmRWPos);
    const bool flushed = file->sync();
    PORTC ^= 0x80;
    if (!result || !flushed)          
    {
      ui->print(Progmem::getString(Progmem::uiNewLine));
      ui->print(Progmem::getString(Progmem::sdCardError));
      ui->print(Progmem::getString(Progmem::uiNewLine));
      break; 
    }
  }
  
  if (fat)
  {
    f_close(getFatFile());
  }
  else
  {
    cpmCloseFile();
  }    
  
  ui->disableKeyboard(false);
  fdc->setAutomaticMotorOff(true);
  file->close();
  ui->print(Progmem::getString(Progmem::uiDeleteLine));
  
  if (!success)
  {  
    if (fat)
    {
      fatResult(lastResult);  
    }
    else
    {
      ui->print(Progmem::getString(Progmem::fsDiskError));
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
    }
    
    ui->print(Progmem::getString(Progmem::xmodemTransferFail));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
  
  else
  {
    ui->print(Progmem::getString(Progmem::xmodemTransferEnd));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
    
  return success;
}

bool fileSdToFloppy(const BYTE* newFileName)
{
  BYTE buf[512];
  lastResult = FR_OK;
  
  if (!fatMount())
  {
    return false;
  }
  
  if (!SdDetectMemoryCard())
  {
    return false;
  }
  
  File* file = SdPickFile(false);
  if (!file)
  {
    return false;
  }
  
  buf[0] = 0;
  strcat(buf, g_path);
  strcat(buf, newFileName);
  
  f_unlink(buf);  
  if (fatResult(f_open(getFatFile(), buf, FA_WRITE | FA_CREATE_NEW)) != FR_OK)
  {
    file->close();
    return false;
  }
    
  ui->disableKeyboard(true);
  fdc->setAutomaticMotorOff(false);
  ui->print("");
  
  success = true;
  while(true)
  {
    xmRWPos = 0;
    
    // read from SD card
    PORTC ^= 0x80; // toggle activity LED
    int result = file->read((BYTE*)&buf[0], sizeof(buf));
    PORTC ^= 0x80;
    if (result == -1)
    {
      ui->print(Progmem::getString(Progmem::uiNewLine));
      ui->print(Progmem::getString(Progmem::sdCardError));
      ui->print(Progmem::getString(Progmem::uiNewLine));
      break; 
    }
    
    lastResult = f_write(getFatFile(), &buf[0], result, &xmRWPos);
    if (lastResult != FR_OK)
    {
      success = false;
      break;
    }
  
    // end of file
    if (result < sizeof(buf))
    {
      break;
    }
  }  
  
  file->close();
  f_close(getFatFile());
  ui->print(Progmem::getString(Progmem::uiDeleteLine));
  
  ui->disableKeyboard(false);    
  fdc->setAutomaticMotorOff(true);
  
  if (!success)
  {
    fatResult(lastResult);
    
    ui->print(Progmem::getString(Progmem::xmodemTransferFail));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
  
  else
  {
    ui->print(Progmem::getString(Progmem::xmodemTransferEnd));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
    
  return success;
}

#endif // SD_CARD_SUPPORT

#endif // BUILD_IMD_IMAGER
