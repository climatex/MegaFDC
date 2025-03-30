// MegaFDC (c) 2023-2025 J. Bogin, http://boginjr.com
// XMODEM helpers

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
    BYTE track;
    BYTE head;
    BYTE startSector;
    fdc->convertLogicalSectorToCHS(totalSectorsCount, track, head, startSector);
    
    // how many sectors till end of track or SECTOR_BUFFER_SIZE constraint
    const BYTE sectorCount = fdc->getMaximumSectorCountForRW(startSector, SECTOR_BUFFER_SIZE - xmRWPos);
    const BYTE endSector = startSector + sectorCount-1;
    
    ui->print(Progmem::getString(Progmem::diskIoProgress), track, head);
    
    // seek if needed, inform about track and head
    if ((fdc->getCurrentTrack() != track) || (fdc->getCurrentHead() != head))
    {
      fdc->seekDrive(track, head);      
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
            
    BYTE track;
    BYTE head;
    BYTE startSector;
    fdc->convertLogicalSectorToCHS(totalSectorsCount, track, head, startSector);
    
    const BYTE sectorCount = fdc->getMaximumSectorCountForRW(startSector, SECTOR_BUFFER_SIZE - xmRWPos);
    const BYTE endSector = startSector + sectorCount-1;
    
    ui->print(Progmem::getString(Progmem::diskIoProgress), track, head);
    
    if ((fdc->getCurrentTrack() != track) || (fdc->getCurrentHead() != head))
    {
      fdc->seekDrive(track, head);
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
FRESULT lastResult;

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
    FAT_EXECUTE(f_close(getFatFile()));
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
  FAT_EXECUTE(f_close(getFatFile()));  
  
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

#endif
