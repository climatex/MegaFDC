// MegaFDC (c) 2023-2026 J. Bogin, http://boginjr.com
// IMD (ImageDisk)-compatible disk imager; file transfers between an SD card

#include "config.h"

#ifdef BUILD_IMD_IMAGER

#ifdef SD_CARD_SUPPORT

int res;

#define SD_READ(bytes)  PORTC ^= 0x80; \
                        res = file->read(&g_rwBuffer[0], (bytes)); \
                        PORTC ^= 0x80; \
                        if (res == -1) { strcpy(responseStr, Progmem::getString(Progmem::sdCardError)); return false; } \
                        else if (res < (bytes)) { strcpy(responseStr, Progmem::getString(Progmem::sdFileEnd)); return false; } \                        
                       
#define SD_WRITE(bytes) PORTC ^= 0x80; \
                        res = file->write((const BYTE*)&g_rwBuffer[0], (bytes)); \
                        PORTC ^= 0x80; \
                        if (res == 0) { strcpy(responseStr, Progmem::getString(Progmem::sdCardError)); return false; }

#define SD_FLUSH()      PORTC ^= 0x80; \
                        res = file->sync(); \
                        PORTC ^= 0x80; \
                        if (res == 0) { strcpy(responseStr, Progmem::getString(Progmem::sdCardError)); return false; }
                        
void IMD::writeDisk()
{
  if (!SdDetectMemoryCard())
  {
    return;
  }
  
  // double step cannot be autodetected from image file
  if (!tryAskIfCannotAutodetect(true, false))
  {
    return;
  }
  
  // write sectors marked bad?
  ui->print(Progmem::getString(Progmem::imdXmodemSkipBad));
  BYTE key = toupper(ui->readKey("YN\e"));
  if (key == '\e')
  {
    ui->print(Progmem::getString(Progmem::uiNewLine));
    return;
  }
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  const bool skipBadSectorsInFile = (key == 'Y');  
  
  // disk sector-by-sector verify?
  ui->print(Progmem::getString(Progmem::imdXmodemVerify));
  key = toupper(ui->readKey("YN\e"));
  if (key == '\e')
  {
    ui->print(Progmem::getString(Progmem::uiNewLine));
    return;
  }
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  const bool doVerify = (key == 'Y');
  
  // file picker, filter *.imd
  File* file = SdPickFile(false, ".imd");
  if (!file)
  {
    return false;
  }
  
  ui->print("");
  ui->print(Progmem::getString(Progmem::imdInsertWrite));    
  ui->print(Progmem::getString(Progmem::uiContinueAbort));
  key = ui->readKey("\r\e");
  ui->print(Progmem::getString(Progmem::uiNewLine));
  if (key == '\e')
  {
    file->close();
    return;
  }
  
  FDC::DiskDriveMediaParams backup;
  memcpy(&backup, fdc->getParams(), sizeof(FDC::DiskDriveMediaParams));
  fdc->setAutomaticMotorOff(false);
  ui->disableKeyboard(true);
  ui->print("");
  
  BYTE responseStr[70] = {0};
  WORD totalBadSectorsDisk = 0;
  WORD totalBadSectorsFile = 0;
  bool success = writeDiskInternal(skipBadSectorsInFile, doVerify, file, responseStr, totalBadSectorsDisk, totalBadSectorsFile);
   
  file->close();
  ui->print("");
  cleanup();  
  fdc->setAutomaticMotorOff(true);
  fdc->seekDrive(0, 0);
  ui->disableKeyboard(false);
      
  // print if there was any response in details
  ui->print(Progmem::getString(success ? Progmem::imdXmodemXferEnd : Progmem::imdXmodemXferFail));
  if (strlen(responseStr))
  {
    ui->print(responseStr);
  }
  // total bad sectors found
  if (success)
  {
    ui->print(Progmem::getString(Progmem::imdBadSectorsDisk), totalBadSectorsDisk);
    ui->print(Progmem::getString(Progmem::imdBadSectorsFile), totalBadSectorsFile);
  }
  
  ui->print(Progmem::getString(Progmem::uiNewLine));
  ui->print(Progmem::getString(Progmem::uiContinue));
  ui->readKey("\r");  
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
}

bool IMD::writeDiskInternal(bool skipBadSectorsInFile, bool doVerify, File* file, BYTE* responseStr, WORD& totalBadSectorsDisk, WORD& totalBadSectorsFile)
{
  // first step: check the IMD header of variable length; skip its contents (needs to end with EOF)
  SD_READ(4);
  if (memcmp(&g_rwBuffer[0], "IMD ", 4) != 0)
  {
    strcpy(responseStr, Progmem::getString(Progmem::imdXmodemErrHeader));
    return false;
  }
  
  // keep looking for ASCII EOF marking the end of header
  for (;;)
  {
    PORTC ^= 0x80; // activity LED
    int eof = file->read();
    PORTC ^= 0x80;
    
    if (eof == -1)
    {
      strcpy(responseStr, Progmem::getString(Progmem::imdXmodemErrHeader));
      return false;
    }
    else if (eof == 0x1A) // EOF
    {
      break;
    }
  }
  
  WORD lastPos = 0;
  BYTE mode = (BYTE)-1;
  BYTE spt = (BYTE)-1;
  BYTE secSize = (BYTE)-1;  
  bool geometryChanged = true;
  bool seekIndicated = true;
  
  for (;;)
  {
    if (file->curPosition() == file->fileSize()) // end-of-file
    {
      responseStr[0] = 0;
      return true;
    }
    
    // specify and check for valid track data: IMD mode byte
    SD_READ(1);
    if (g_rwBuffer[0] != mode)
    {
      mode = g_rwBuffer[0];
      geometryChanged = true;
    }
    
    // mode value between 0-5, but since it's the first data byte, also allow EOF to end the transfer gracefully
    if (mode > 5)
    {
      if (mode == 0x1A)
      {
        responseStr[0] = 0;
        return true; // transfer over
      }
      
      else
      {
        strcpy(responseStr, Progmem::getString(Progmem::imdXmodemErrMode));
        return false;
      }
    }
    
    // IMD cylinder byte    
    SD_READ(1);
    const BYTE cylinder = g_rwBuffer[0];
    if (cylinder >= fdc->getParams()->Cylinders)
    {
      strcpy(responseStr, Progmem::getString(Progmem::imdXmodemErrCyls));
      return false;
    }
    
    // IMD head byte
    SD_READ(1);
    const BYTE head = g_rwBuffer[0] & 0x3F;
    if (head > 1)
    {
      strcpy(responseStr, Progmem::getString(Progmem::imdXmodemErrHead));
      return false;
    }
    
    // the head byte is also used to indicate presence of 2 more data maps
    const bool hasSecTrackMap = g_rwBuffer[0] & 0x80;
    const bool hasSecHeadMap = g_rwBuffer[0] & 0x40;
    
    // sectors per track byte
    SD_READ(1);
    if (g_rwBuffer[0] != spt)
    {
      spt = g_rwBuffer[0];
      geometryChanged = true;
    }
    
    // sectors per track greater than 63 are not feasible on a floppy, and as such, not supported here at this moment
    if (spt > 63)
    {
      strcpy(responseStr, Progmem::getString(Progmem::imdXmodemErrSpt));
      return false;
    }
    
    // sector size byte
    SD_READ(1);
    if (g_rwBuffer[0] != secSize)
    {
      secSize = g_rwBuffer[0];
      geometryChanged = true;
    }    
    if (secSize > 6)
    {      
      strcpy(responseStr, Progmem::getString(Progmem::imdXmodemErrSsize));
      return false;
    }
    // the Mega2560 board has 8K SRAM, so no chance of reading or writing large sectors - but we can format and verify those at least
    if (secSize >= 5)
    {
      sprintf(responseStr, Progmem::getString(Progmem::imdXmodemLowRAM), (secSize == 5) ? 4 : 8);
      strcat(responseStr, Progmem::getString(Progmem::imdAdvancedDetails));
      return false;
    }
    const WORD secSizeBytes = fdc->getSectorSizeBytes(secSize);
    
    // take a look at the sectors per track byte, if this is zero, the whole track was unreadable
    // this means that next 5 bytes of another track follow
    if (!spt)
    {
      geometryChanged = true;
      continue;
    }
    
    // data is valid - now up to 3 maps follow, sector numbering map is there always
    m_cbSectorNumberingMap = new BYTE[spt];
    if (!m_cbSectorNumberingMap)
    {
      strcpy(responseStr, Progmem::getString(Progmem::imdMemoryError));      
      return false;
    }    
    SD_READ(spt);
    memcpy(m_cbSectorNumberingMap, &g_rwBuffer[0], spt);
    
    // now we have the sector numbering map
    // if there are more than 2 sectors per track, compute sector interleave factor from this sequence
    lastPos = 0;
    BYTE newInterleave = 1; // assume sequential
    if (spt > 2)
    {
      // find starting sector
      bool found = false;
      WORD startingSector = 0;
      BYTE idx = 0;
      
      for (;;)
      {
        for (idx = 0; idx < spt; idx++)
        {
          if (m_cbSectorNumberingMap[idx] == startingSector)
          {
            found = true;
            break;
          }
        }
        
        if (found)
        {
          break;
        }
        
        startingSector++;
        if (startingSector > 255)
        {
          startingSector = 1;
          idx = 0;
          break; // we tried
        }
      }
      
      if (!found)
      {
        newInterleave = 1;
      }
      
      // find interleave factor
      else
      {
        lastPos = idx + 1;        
        while (lastPos < spt)
        {
          if (m_cbSectorNumberingMap[lastPos++] != startingSector+1)
          {
            newInterleave++;
          }
          else
          {
            break;
          }
        }
        
        lastPos = 0;
      }
    }
    if (newInterleave != m_cbInterleave)
    {
      m_cbInterleave = newInterleave;
      geometryChanged = true;
    }
    
    // these 2 maps are optional
    if (hasSecTrackMap)
    {
      m_cbSectorTrackMap = new BYTE[spt];
      if (!m_cbSectorTrackMap)
      {      
        strcpy(responseStr, Progmem::getString(Progmem::imdMemoryError));
        return false;
      }
      
      SD_READ(spt);
      memcpy(m_cbSectorTrackMap, &g_rwBuffer[0], spt);
    }
    if (hasSecHeadMap)
    {
      m_cbSectorHeadMap = new BYTE[spt];
      if (!m_cbSectorHeadMap)
      {      
        strcpy(responseStr, Progmem::getString(Progmem::imdMemoryError));
        return false;
      }
      
      SD_READ(spt);
      memcpy(m_cbSectorHeadMap, &g_rwBuffer[0], spt);
    }

    // sector data would now follow, but we need to format first before writing
    // check if single-sided operation was setup but the current image head number is indicating 2nd head
    bool skipOperation = ((fdc->getParams()->Heads == 1) && (head > 0));
    
    // seek to physical cylinder and head
    if (!skipOperation && ((fdc->getCurrentCylinder() != cylinder) || (fdc->getCurrentHead() != head)))
    {
      fdc->seekDrive(cylinder, head);
      seekIndicated = true;
    }
    
    // if mode, spt, secSize or interleave changed, update
    if (geometryChanged)
    {
      if (!skipOperation)
      {
        switch(mode)
        {
        case 0:
          fdc->getParams()->CommRate = 500;
          fdc->getParams()->FM = true;
          break;
        case 1:
          fdc->getParams()->CommRate = 300;
          fdc->getParams()->FM = true;
          break;
        case 2:
          fdc->getParams()->CommRate = 250;
          fdc->getParams()->FM = true;
          break;
        case 3:
          fdc->getParams()->CommRate = 500;
          fdc->getParams()->FM = false;
          break;
        case 4:
          fdc->getParams()->CommRate = 300;
          fdc->getParams()->FM = false;
          break;
        case 5:
          fdc->getParams()->CommRate = 250;
          fdc->getParams()->FM = false;
        }
        
        // if translate 300<->250kbps is on and the file rate is 250kbps, set 300kbps on FDC
        // writing a 250kbps 300RPM disk in a 360RPM drive, so compensate for +16,67%
        if (m_xlat300and250 && (fdc->getParams()->CommRate == 250))
        {
          fdc->getParams()->CommRate = 300;
        }
        
        fdc->setCommunicationRate();
        fdc->getParams()->LowLevelFormatFiller = fdc->getParams()->FM ? 0xE5 : 0xF6;
        
        fdc->getParams()->SectorsPerTrack = spt;
        fdc->getParams()->SectorSizeBytes = secSizeBytes;
        
        BYTE autoSectorGap = 0;
        BYTE autoFormatGap = 0;
        autodetectGaps(autoSectorGap, autoFormatGap);
        if (m_autoSectorGap)
        {
          fdc->getParams()->GapLength = autoSectorGap;
        }
        if (m_autoFormatGap)
        {
          fdc->getParams()->Gap3Length = autoFormatGap;
        }
        
        printGeometryInfo(cylinder, head, m_cbInterleave);
      }
    }
    
    if (!skipOperation && seekIndicated)
    { 
      // print progress on LCD
      ui->print(Progmem::getString(Progmem::imdProgress), cylinder, head);
      
      // prepare CHSV table for formatting, we might have custom values for the cylinder and head
      memset(&g_rwBuffer[0], 0, spt * 4);
      BYTE sector = 0;
      lastPos = 0;
      while (sector < spt)
      {
        const BYTE logicalSector = m_cbSectorNumberingMap[sector];
        const BYTE logicalCylinder = hasSecTrackMap ? m_cbSectorTrackMap[sector] : cylinder;
        const BYTE logicalHead = hasSecHeadMap ? m_cbSectorHeadMap[sector] : head;
        
        g_rwBuffer[lastPos++] = logicalCylinder;
        g_rwBuffer[lastPos++] = logicalHead;
        g_rwBuffer[lastPos++] = logicalSector;
        g_rwBuffer[lastPos++] = secSize;
        
        sector++;
      }
      lastPos = 0;
      
      // format
      fdc->formatTrack(true);    
      
      // 2 errors that stop the datastream
      if (fdc->getLastError())
      {
        if (fdc->wasErrorNoDiskInDrive())
        {
          strcpy(responseStr, Progmem::getString(Progmem::errINTTimeout));
          return false;
        }
        else if (fdc->wasErrorDiskProtected())
        {
          strcpy(responseStr, Progmem::getString(Progmem::errWriProtect));
          return false;
        }
      }
    }
    
    // after formatting
    geometryChanged = false;
    seekIndicated = false;
    
    // write each sector
    BYTE sectorIdx = 0;
    while (sectorIdx < spt)
    {
      // determine sector data record type
      SD_READ(1);
      BYTE sectorDataType = g_rwBuffer[0];
      if (sectorDataType > 8)
      {
        strcpy(responseStr, Progmem::getString(Progmem::imdXmodemErrData));
        return false;
      }
         
      switch(sectorDataType)
      {
      case 1:
      case 3:
      case 5:
      case 7:
      {
        // normal data
        SD_READ(secSizeBytes);
      }
      break;
      case 2:
      case 4:
      case 6:
      case 8:
      {
        // compressed data (same byte repeated secSizeBytes)
        SD_READ(1);
        const BYTE compressedData = g_rwBuffer[0];
        memset(&g_rwBuffer[0], compressedData, secSizeBytes);
      }
      break;
      }
      
      // if the sector contains a "data error" flag 
      if (sectorDataType > 4)
      {
        totalBadSectorsFile++;
        
        if (skipBadSectorsInFile)
        {
          // if not to write sectors marked bad, set sector type to unavailable
          sectorDataType = 0;  
        }      
      }
      
      if (!skipOperation && sectorDataType) // write only if there were data available
      {
        const BYTE logicalSector = m_cbSectorNumberingMap[sectorIdx];
        // these 2 may differ from the physical position that was used for fdc->seekDrive() !
        const BYTE logicalCylinder = hasSecTrackMap ? m_cbSectorTrackMap[sectorIdx] : cylinder;
        const BYTE logicalHead = hasSecHeadMap ? m_cbSectorHeadMap[sectorIdx] : head;    
        
        // normal data with DAM, compressed data with DAM, deleted data with read error, compressed deleted with read error
        const bool deletedDataMark = (sectorDataType == 3) || (sectorDataType == 4) || (sectorDataType == 7) || (sectorDataType == 8);
        
        // finally, write
        bool result = fdc->readWriteSectors(true, logicalSector, logicalSector, NULL, deletedDataMark, &logicalCylinder, &logicalHead);
        
        // 2 errors that stop the datastream
        if (fdc->getLastError())
        {
          if (fdc->wasErrorNoDiskInDrive())
          {
            strcpy(responseStr, Progmem::getString(Progmem::errINTTimeout));
            return false;
          }
          else if (fdc->wasErrorDiskProtected())
          {
            strcpy(responseStr, Progmem::getString(Progmem::errWriProtect));
            return false;
          }
          
          // since we're working sector by sector, if this write operation failed and verify was not specified, log one bad sector
          if (!doVerify)
          {
            totalBadSectorsDisk++;
          }
        }
        
        // write OK, now verify
        else if (doVerify)
        {
          result = fdc->verify(logicalSector, false, &logicalCylinder, &logicalHead);
          
          if (fdc->getLastError())
          {
            if (fdc->wasErrorNoDiskInDrive())
            {
              strcpy(responseStr, Progmem::getString(Progmem::errINTTimeout));
              return false;
            }
            
            totalBadSectorsDisk++;
          }
        }
      }
      
      // so far so good    
      responseStr[0] = 0;  
      sectorIdx++;    
    }
    
    // reached the end of the track?
    // next track might differ in SPT, so the tables will be of different size and contents
    if (m_cbSectorNumberingMap)
    {
      delete[] m_cbSectorNumberingMap;
      m_cbSectorNumberingMap = NULL;
    }
    if (m_cbSectorTrackMap)
    {
      delete[] m_cbSectorTrackMap;
      m_cbSectorTrackMap = NULL;
    }
    if (m_cbSectorHeadMap)
    {
      delete[] m_cbSectorHeadMap;
      m_cbSectorHeadMap = NULL;
    }    
  }
  
  return false; // doesn't reach here
}

void IMD::readDisk()
{
  if (!SdDetectMemoryCard())
  {
    return;
  }
  
  // file picker, filter *.imd
  File* file = SdPickFile(true, ".imd");
  if (!file)
  {
    return false;
  }
  
  ui->print("");
  ui->print(Progmem::getString(Progmem::imdInsertRead));    
  ui->print(Progmem::getString(Progmem::uiContinueAbort));
  BYTE key = ui->readKey("\r\e");
  ui->print(Progmem::getString(Progmem::uiNewLine));
  if (key == '\e')
  {
    file->close();
    return;
  }
  
  ui->print(Progmem::getString(Progmem::uiNewLine));
  FDC::DiskDriveMediaParams backup;  
  memcpy(&backup, fdc->getParams(), sizeof(FDC::DiskDriveMediaParams));
  
  // try autodetecting double stepping (if on Detect) and number of sides (if not forced singlesided)
  // do this on track 0, if it's unreadable, ask to specify
  ui->print(Progmem::getString(Progmem::imdTestMedia));
  ui->print(Progmem::getString(Progmem::uiNewLine));
  fdc->seekDrive(0, 0);
  
  bool askDoubleStep = m_autoDoubleStep;
  bool askHeads = m_autoHeads;
  if (askDoubleStep || askHeads)
  {
    if (autodetectCommRate())
    {
      if (askDoubleStep) askDoubleStep = !autodetectDoubleStep();
      if (askHeads) askHeads = !autodetectHeads();  
    }
    else if (fdc->getLastError() && fdc->wasErrorNoDiskInDrive()) 
    {
      file->close();
      
      ui->print(Progmem::getString(Progmem::errINTTimeout));
      ui->print(Progmem::getString(Progmem::uiNewLine));
      ui->print(Progmem::getString(Progmem::uiContinue));
      ui->readKey("\r");
      ui->print(Progmem::getString(Progmem::uiNewLine));      
      
      memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
      return;
    }
  }
  
  // could not either of them, or both?
  if (!tryAskIfCannotAutodetect(askDoubleStep, askHeads))
  {
    file->close();
    memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
    return;
  }
  
  // ask for a comment in the file header; use the sector I/O buffer to store each line
  g_rwBuffer[0] = 0;
  strcpy(g_rwBuffer, Progmem::getString(Progmem::imdWriteHeader));  
  strcat(g_rwBuffer, Progmem::getString(Progmem::uiNewLine));
  ui->print("");
  ui->print(Progmem::getString(Progmem::imdWriteComment), MAX_COMMAND_PROMPT_LEN);
  ui->print(Progmem::getString(Progmem::imdWriteDone));
  ui->print(Progmem::getString(Progmem::imdWriteEnterEsc));
  key = ui->readKey("\r\e");
  ui->print(Progmem::getString(Progmem::uiDeleteLine));
  if (key == '\r')
  {
    bool emptyLine = false;
    
    // must incl. EOF and NUL
    while ((strlen(g_rwBuffer) + MAX_COMMAND_PROMPT_LEN + 2) < SECTOR_BUFFER_SIZE)
    {
      const BYTE* promptBuffer = ui->prompt();
      WORD length = strlen(promptBuffer);
      
      // done?
      if (!length && emptyLine)
      {
        break;
      }    
      emptyLine = length == 0;
      
      strcat(g_rwBuffer, promptBuffer);
      strcat(g_rwBuffer, Progmem::getString(Progmem::uiNewLine));
      ui->print(Progmem::getString(Progmem::uiNewLine)); 
    }
  }
  
  // EOF marks the end of header
  g_rwBuffer[strlen(g_rwBuffer)] = 0x1A;
  
  // wait for file
  fdc->setAutomaticMotorOff(false);
  ui->disableKeyboard(true);
  ui->print(""); 
  
  BYTE responseStr[70] = {0};
  WORD totalBadSectorsDisk = 0;
  BYTE unreadableTracks = 0;
  bool success = readDiskInternal(file, responseStr, totalBadSectorsDisk, unreadableTracks);  
   
  file->close();
  ui->print("");
  cleanup();  
  fdc->setAutomaticMotorOff(true);
  ui->disableKeyboard(false);
  fdc->seekDrive(0, 0);  
    
  ui->print(Progmem::getString(success ? Progmem::imdXmodemXferEnd : Progmem::imdXmodemXferFail));
  if (strlen(responseStr))
  {
    ui->print(responseStr);
  }
  
  if (success)
  {
    ui->print(Progmem::getString(Progmem::imdBadSectorsDisk), totalBadSectorsDisk);
    ui->print(Progmem::getString(Progmem::imdUnreadableTrks), unreadableTracks);
  }
  
  ui->print(Progmem::getString(Progmem::uiNewLine));
  ui->print(Progmem::getString(Progmem::uiContinue));
  
  ui->readKey("\r");  
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
}

bool IMD::readDiskInternal(File* file, BYTE* responseStr, WORD& totalBadSectorsDisk, BYTE& unreadableTracks)
{
  // write IMD header and comment to output buffer, until EOF in the I/O buffer
  WORD lastPos = 0;   
  while (g_rwBuffer[lastPos++] != 0x1A) {}
  SD_WRITE(lastPos);
  memset(&g_rwBuffer[0], 0, lastPos); // then clear the buffer
  lastPos = 0;
  
  BYTE currentSector = 0;
  BYTE sectorIdx = 0;
  BYTE startingSectorIdx = (BYTE)-1;
  bool geometryChanged = true;
  
  // process tracks
  for (;;)
  {
    SD_FLUSH();
  
    // 1st track specification byte: mode
    BYTE mode = 0;
    {
      const WORD lastCommRate = fdc->getParams()->CommRate;
      const WORD lastSecSize = fdc->getParams()->SectorSizeBytes;
      const bool lastFM = fdc->getParams()->FM;
      
      // call autodetectCommRate on current track and head, this also sets fdc->sector size in bytes when successful
      fdc->getParams()->SectorSizeBytes = 0; // assume error
      const bool autodetected = autodetectCommRate();
      
      // geometry changed: new mode, sector size, spt or interleave (logical-OR this, as the last two will be checked later)
      geometryChanged |=     (lastCommRate != fdc->getParams()->CommRate) ||
                             (lastFM != fdc->getParams()->FM) ||
                             (lastSecSize != fdc->getParams()->SectorSizeBytes);
      if (autodetected)
      {
        // if we are reading a 300kbps disk and our translation setting is set to 250kbps:
        if (m_xlat300and250 && (fdc->getParams()->CommRate == 300))
        {
          mode = fdc->getParams()->FM ? 2 : 5;
        }  
        
        // normal
        else
        {
          if (fdc->getParams()->CommRate == 500)
          {
            mode = fdc->getParams()->FM ? 0 : 3;
          }
          else if (fdc->getParams()->CommRate == 300)
          {
            mode = fdc->getParams()->FM ? 1 : 4;
          }
          else if (fdc->getParams()->CommRate == 250)
          {
            mode = fdc->getParams()->FM ? 2 : 5;
          }
        }
      }
      
      else
      {
        // abort 
        if (fdc->wasErrorNoDiskInDrive())
        {
          strcpy(responseStr, Progmem::getString(Progmem::errINTTimeout));
          return false;
        }
        
        mode = 0; // no sector IDs on the whole track: treat mode as zero byte
      }
     
      g_rwBuffer[0] = mode;
      SD_WRITE(1);
    }
    
    // 2nd byte: cylinder
    BYTE cylinder = fdc->getCurrentCylinder();
    g_rwBuffer[0] = cylinder;
    SD_WRITE(1);
    
    // 3rd byte: head
    BYTE head = fdc->getCurrentHead();
    // but not just the head, bits 7 and 6 are used to indicate if logical cyl/head IDs don't match the physical cyl/head
    BYTE headExtraBits = head;   
      
    // assume they don't, as only non-standard disks do
    bool hasSecTrackMap = false;
    bool hasSecHeadMap = false;
      
    // for this we need to call the interleave detection function, but only if we know the current track is readable
    // i.e. sector size is set properly by autodetectCommRate
    if (fdc->getParams()->SectorSizeBytes > 0)
    {     
      BYTE oldInterleave = m_cbInterleave; // modified by autodetectInterleave
      BYTE oldSpt = fdc->getParams()->SectorsPerTrack; // modified by autodetectInterleave
      fdc->getParams()->SectorsPerTrack = 0; // set this to zero to see if we have an issue with the call
      
      autodetectInterleave();    
             
      // oh yes indeed we have
      if (!m_cbSectorsTable && (fdc->getParams()->SectorsPerTrack == 0))
      {
        // less bad
        if (fdc->getLastError() && fdc->wasErrorNoDiskInDrive())
        {
          strcpy(responseStr, Progmem::getString(Progmem::errINTTimeout));
          return false;
        }
        
        // oopsie daisy
        // I swear to God I'm not touching anything with 8K of RAM or less in the Year of our Lord 2024
        strcpy(responseStr, Progmem::getString(Progmem::imdMemoryError));
        return false;
      }
      
      // SPT out of range
      else if (fdc->getParams()->SectorsPerTrack > 63)
      {
        strcpy(responseStr, Progmem::getString(Progmem::imdXmodemErrSpt));
        return false;
      }
              
      // skim through     
      for (BYTE idx = 0; idx < SECTORS_TABLE_COUNT; idx++)
      {
        // determined?
        if (hasSecTrackMap && hasSecHeadMap)
        {
          break;
        }
        
        // valid sector ID information?
        if (m_cbSectorsTable[idx] != 0xFFFF)
        {
          if (!hasSecTrackMap)
          {
            // sector ID cyl number differs from physical?
            const BYTE cyl = (m_cbSectorsTable[idx] >> 8) & 0x7F;
            if (cyl != cylinder)
            {
              hasSecTrackMap = true;
            }              
          }
          if (!hasSecHeadMap)
          {
            // sector ID head number differs from physical?
            const BYTE hd = m_cbSectorsTable[idx] >> 15;
            if (hd != head)
            {                
              hasSecHeadMap = true;
            }
          } 
        }          
      }
      
      // last 2 checks if geometry changed are known here
      if ((oldInterleave != m_cbInterleave) || (oldSpt != fdc->getParams()->SectorsPerTrack))
      {
        geometryChanged = true;
      }
    }

    // has custom track numbering in sector IDs
    if (hasSecTrackMap)
    {
      headExtraBits |= 0x80;
    }
    // custom head numbering
    if (hasSecHeadMap)
    {
      headExtraBits |= 0x40;
    }
    
    g_rwBuffer[0] = headExtraBits;
    SD_WRITE(1);

    // 4th byte: sectors per track  
    // initialized by autodetectCommRate()  
    BYTE spt = 0;
    if (fdc->getParams()->SectorSizeBytes > 0)
    {
      // initialized by autodetectInterleave()
      spt = fdc->getParams()->SectorsPerTrack;  
    }
      
    if (spt)
    {
      // as the interleave table almost always never starts from the beginning, find the starting sector
      WORD startingSector = 0;
      
      while (startingSectorIdx == (BYTE)-1) // undefined
      {
        bool found = false;
        for (BYTE idx = 0; idx < SECTORS_TABLE_COUNT; idx++)
        {
          if (m_cbSectorsTable[idx] == 0xFFFF) // entry not defined
          {
            continue;
          }
          
          if ((BYTE)m_cbSectorsTable[idx] == startingSector)
          {
            startingSectorIdx = idx;
            sectorIdx = startingSectorIdx;
            lastPos = 0; // use this as count how many were written in the map
            found = true;
            break;
          }
        }
        
        if (found)
        {
          break;
        }
        
        startingSector++;  // starts from 2 or whatever
        if (startingSector > 255) // cannot sync
        {
          // mark track as unreadable
          spt = 0;
          fdc->getParams()->SectorSizeBytes = 0;            
          break;
        }
      }
    }
    
    g_rwBuffer[0] = spt;
    SD_WRITE(1);
        
    // 5th byte: sector size
    BYTE secSize = 0;
    WORD secSizeBytes = 0;
    if (fdc->getParams()->SectorSizeBytes > 0)
    {
      secSizeBytes = fdc->getParams()->SectorSizeBytes;
      secSize = fdc->convertSectorSize(secSizeBytes);
    }
    
    // more than 2K
    if (secSize >= 5)
    {
      sprintf(responseStr, Progmem::getString(Progmem::imdXmodemLowRAM), (secSize == 5) ? 4 : 8);
      strcat(responseStr, Progmem::getString(Progmem::imdAdvancedDetails));
      return false;
    }
    
    g_rwBuffer[0] = secSize;
    SD_WRITE(1);
    
    // track has no valid sectors, advance
    if (!spt || !secSizeBytes)
    {
      // progress Unreadable
      ui->print(Progmem::getString(Progmem::imdProgress), cylinder, head);
      ui->print(Progmem::getString(Progmem::imdTrackUnreadable));
      unreadableTracks++;
      
      // re-specify
      startingSectorIdx = (BYTE)-1;
      sectorIdx = 0;
      lastPos = 0;
      geometryChanged = true;
      
      // seek to the next
      head++;
      if (head == fdc->getParams()->Heads)
      {
        head = 0;
        cylinder++;
      }
      if (cylinder == fdc->getParams()->Cylinders)
      {
        responseStr[0] = 0;
        return true; // end
      }
      
      fdc->seekDrive(cylinder, head);      
      continue; // transfer continues
    }    

    // now write the sector numbering map
_j1: 
    while ((lastPos < spt) && (sectorIdx < SECTORS_TABLE_COUNT))
    {
      if (m_cbSectorsTable[sectorIdx] == 0xFFFF) // undefined?
      {
        sectorIdx++;
        continue;
      }
      
      currentSector = (BYTE)m_cbSectorsTable[sectorIdx++]; // sector info only
      
      g_rwBuffer[0] = currentSector;
      SD_WRITE(1);
      lastPos++;
    }
    
    // sectors per track count not reached: do we still need to go from the beginning of the table?
    if ((sectorIdx == SECTORS_TABLE_COUNT) && (lastPos < spt))
    {
      bool found = false;
          
      while (currentSector && !found)
      {
        for (sectorIdx = 0; sectorIdx < SECTORS_TABLE_COUNT; sectorIdx++)
        {
          if (((BYTE)m_cbSectorsTable[sectorIdx] == currentSector) &&
               (m_cbSectorsTable[sectorIdx] != 0xFFFF))
          {
            found = true;
            break;
          }
        }
        
        if (!found)
        {
          currentSector++; // possible gap?
        }
      }            
      
      // found from the beginning, get the succeeding sector index
      if (found)
      {
        sectorIdx += 1;
        if (sectorIdx < SECTORS_TABLE_COUNT)
        {
          goto _j1; // valid
        }
      }
      
      // not found or out-of-bounds
      sectorIdx = 0;
      goto _j1;
    }
    
    // reuse startingSectorIdx for the rest of the maps
    sectorIdx = startingSectorIdx;
    lastPos = 0;
    currentSector = 0;
    
    // write sector track map analog to above
    if (hasSecTrackMap)
    {
_j2:
      while ((lastPos < spt) && (sectorIdx < SECTORS_TABLE_COUNT))
      {
        if (m_cbSectorsTable[sectorIdx] == 0xFFFF)
        {
          sectorIdx++;
          continue;
        }
        
        currentSector = (BYTE)m_cbSectorsTable[sectorIdx];
        g_rwBuffer[0] = (m_cbSectorsTable[sectorIdx++] >> 8) & 0x7F; // cyl info only
        SD_WRITE(1);
        lastPos++;
      }
      if ((sectorIdx == SECTORS_TABLE_COUNT) && (lastPos < spt))
      {
        bool found = false;
            
        while (currentSector && !found)
        {
          for (sectorIdx = 0; sectorIdx < SECTORS_TABLE_COUNT; sectorIdx++)
          {
            if (((BYTE)m_cbSectorsTable[sectorIdx] == currentSector) &&
                 (m_cbSectorsTable[sectorIdx] != 0xFFFF))
            {
              found = true;
              break;
            }
          }
          
          if (!found)
          {
            currentSector++;
          }
        }
      
        if (found)
        {
          sectorIdx += 1;
          if (sectorIdx < SECTORS_TABLE_COUNT)
          {
            goto _j2;
          }
        }
        
        sectorIdx = 0;
        goto _j2;
      }
      
      sectorIdx = startingSectorIdx;
      lastPos = 0;
      currentSector = 0;
    }
    
    // write sector head map, dtto
    if (hasSecHeadMap)
    {
_j3:
      while ((lastPos < spt) && (sectorIdx < SECTORS_TABLE_COUNT))
      {
        if (m_cbSectorsTable[sectorIdx] == 0xFFFF)
        {
          sectorIdx++;
          continue;
        }
        
        currentSector = (BYTE)m_cbSectorsTable[sectorIdx];
        g_rwBuffer[0] = m_cbSectorsTable[sectorIdx++] >> 15; // head info only
        SD_WRITE(1);
        lastPos++;        
      }
      if ((sectorIdx == SECTORS_TABLE_COUNT) && (lastPos < spt))
      {
        bool found = false;
            
        while (currentSector && !found)
        {
          for (sectorIdx = 0; sectorIdx < SECTORS_TABLE_COUNT; sectorIdx++)
          {
            if (((BYTE)m_cbSectorsTable[sectorIdx] == currentSector) &&
                (m_cbSectorsTable[sectorIdx] != 0xFFFF))
            {
              found = true;
              break;
            }
          }
          
          if (!found)
          {
            currentSector++;
          }
        }
      
        if (found)
        {
          sectorIdx += 1;
          if (sectorIdx < SECTORS_TABLE_COUNT)
          {
            goto _j3;
          }
        }
        
        sectorIdx = 0;
        goto _j3;
      }
      
      sectorIdx = startingSectorIdx;
      lastPos = 0;
      currentSector = 0;      
    }
    
    // if mode, spt, secSize or interleave changed, update gaps and print info
    if (geometryChanged)
    {
      BYTE autoSectorGap = 0;
      BYTE autoFormatGap = 0;
      autodetectGaps(autoSectorGap, autoFormatGap);
      if (m_autoSectorGap)
      {
        fdc->getParams()->GapLength = autoSectorGap;
      }
      if (m_autoFormatGap)
      {
        fdc->getParams()->Gap3Length = autoFormatGap;
      }
      
      printGeometryInfo(cylinder, head, m_cbInterleave);
      ui->print(Progmem::getString(Progmem::imdProgress), cylinder, head);
      geometryChanged = false; // reset the flag
    }
    
    // now try to read
_j4:
    while ((lastPos < spt) && (sectorIdx < SECTORS_TABLE_COUNT))
    { 
      if (m_cbSectorsTable[sectorIdx] == 0xFFFF) // skip undefined
      {
        sectorIdx++;
        continue;
      }
      
      const BYTE logicalSector = (BYTE)m_cbSectorsTable[sectorIdx];
      currentSector = logicalSector;
      const BYTE logicalCylinder = hasSecTrackMap ? (m_cbSectorsTable[sectorIdx] >> 8) & 0x7F : cylinder;
      const BYTE logicalHead = hasSecHeadMap ? m_cbSectorsTable[sectorIdx] >> 15 : head;
      memset(&g_rwBuffer[0], 0, fdc->getParams()->SectorSizeBytes);
      
      fdc->readWriteSectors(false, logicalSector, logicalSector, NULL, false, &logicalCylinder, &logicalHead);        
      if (fdc->getLastError())
      {
        if (fdc->wasErrorNoDiskInDrive())
        {
          strcpy(responseStr, Progmem::getString(Progmem::errINTTimeout));
          return false;
        }          
        totalBadSectorsDisk++;
      }
      
      // go thru the buffer to determine if it's the same byte (indicate compressed)
      bool compressedData = true;
      BYTE lastData = g_rwBuffer[0];
      for (WORD idx = 1; idx < fdc->getParams()->SectorSizeBytes; idx++)
      {
        if (g_rwBuffer[idx] != lastData)
        {
          compressedData = false;
          break;
        }
        lastData = g_rwBuffer[idx];
      }
      
      // form data record type
      BYTE sectorDataType = 0;
      
      if (!fdc->getLastError())
      {
        sectorDataType = compressedData ? 2 : 1;
        if (fdc->wasControlMark()) // deleted data mark
        {
          sectorDataType = compressedData ? 4 : 3;
        }
      }
      else
      {
        // if the whole buffer remained zeroed after the read, then the sector data is unavailable
        if (compressedData && (g_rwBuffer[0] == 0))
        {
          sectorDataType = 0;
        }
        else
        {
          sectorDataType = compressedData ? 6 : 5;
          if (fdc->wasControlMark())
          {
            sectorDataType = compressedData ? 8 : 7;
          }
        }          
      }
      
      lastData = g_rwBuffer[0];
      g_rwBuffer[0] = sectorDataType;
      SD_WRITE(1);
      g_rwBuffer[0] = lastData;
                
      // data now ready in g_rwBuffer according to data record type
      switch(sectorDataType)
      {
      case 1:
      case 3:
      case 5:
      case 7:
      {
        // normal data
        SD_WRITE(secSizeBytes);
      }
      break;
      case 2:
      case 4:
      case 6:
      case 8:
      {
        // compressed data (same byte repeated secSizeBytes)
        SD_WRITE(1);
      }
      break;
      }
      
      // next sector
      sectorIdx++;
      lastPos++;
    }    
    if ((sectorIdx == SECTORS_TABLE_COUNT) && (lastPos < spt))
    {
      bool found = false;
            
      while (currentSector && !found)
      {
        for (sectorIdx = 0; sectorIdx < SECTORS_TABLE_COUNT; sectorIdx++)
        {
          if (((BYTE)m_cbSectorsTable[sectorIdx] == currentSector) &&
               (m_cbSectorsTable[sectorIdx] != 0xFFFF))
          {
            found = true;
            break;
          }
        }
        
        if (!found)
        {
          currentSector++;
        }
      }
    
      if (found)
      {
        sectorIdx += 1;
        if (sectorIdx < SECTORS_TABLE_COUNT)
        {
          goto _j4;
        }
      }

      sectorIdx = 0;
      goto _j4;
    }
       
    // end of track?
    responseStr[0] = 0;

    // re-specify
    lastPos = 0;
    sectorIdx = 0;
    currentSector = 0;
    startingSectorIdx = (BYTE)-1;
    
    // and seek to next
    head++;
    if (head == fdc->getParams()->Heads)
    {
      head = 0;
      cylinder++;
    }
    if (cylinder == fdc->getParams()->Cylinders)
    {
      return true; // completed
    }

    ui->print(Progmem::getString(Progmem::imdProgress), cylinder, head);
    fdc->seekDrive(cylinder, head);  
  }
  
  return false; // doesn't reach here
}


#endif // SD_CARD_SUPPORT

#endif // BUILD_IMD_IMAGER