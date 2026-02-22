// MegaFDC (c) 2023-2026 J. Bogin, http://boginjr.com
// IMD (ImageDisk)-compatible disk imager; file transfer over serial link

#include "config.h"

#ifdef BUILD_IMD_IMAGER

#ifndef SD_CARD_SUPPORT

#define CHECK_STREAM_END    if (packetIdx >= size) return true;

extern IMD imd;

bool rx(DWORD no, BYTE* data, WORD size)
{
  return imd.writeDiskCallback(no, data, size);
}

void IMD::writeDisk()
{
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
  m_cbSkipBadSectorsInFile = (key == 'Y');  
  
  // disk sector-by-sector verify?
  ui->print(Progmem::getString(Progmem::imdXmodemVerify));
  key = toupper(ui->readKey("YN\e"));
  if (key == '\e')
  {
    ui->print(Progmem::getString(Progmem::uiNewLine));
    return;
  }
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  m_cbDoVerify = (key == 'Y');  
  
  // offer XMODEM-1K with 1kB packets, if enough memory
  bool useXMODEM1K = false;
  BYTE* testAlloc = new BYTE[1030];
  if (testAlloc)
  {
    delete[] testAlloc;
    
    ui->print(Progmem::getString(Progmem::imdXmodemUse1k));
    key = toupper(ui->readKey("YN\e"));
    if (key == '\e')
    {
      ui->print(Progmem::getString(Progmem::uiNewLine));
      return;
    }
    ui->print(Progmem::getString(Progmem::uiEchoKey), key);
    useXMODEM1K = (key == 'Y');
  }
  
  ui->print("");
  ui->print(Progmem::getString(Progmem::imdInsertWrite));    
  ui->print(Progmem::getString(Progmem::uiContinueAbort));
  key = ui->readKey("\r\e");
  ui->print(Progmem::getString(Progmem::uiNewLine));
  if (key == '\e')
  {
    return;
  }
  
  FDC::DiskDriveMediaParams backup;
  memcpy(&backup, fdc->getParams(), sizeof(FDC::DiskDriveMediaParams));
  fdc->setAutomaticMotorOff(false);
  
  ui->print("");
  ui->print(Progmem::getString(useXMODEM1K ? Progmem::imdXmodem1k : Progmem::imdXmodem));
  ui->print(Progmem::getString(Progmem::imdXmodemWaitSend));
  ui->disableKeyboard(true);
  ui->setPrintDisabled(false, true);
  
  // wait for file
  m_cbResponseStr[0] = 0;
  m_cbSuccess = false;
  m_cbTotalBadSectorsDisk = 0;
  m_cbTotalBadSectorsFile = 0;
  
  // receive and write
  XModem modem(xmodemRx, xmodemTx, &rx, useXMODEM1K);
  modem.receive();
  cleanup();
  dumpSerialTransfer();
   
  ui->print("");
  ui->setPrintDisabled(false, false);  
  fdc->setAutomaticMotorOff(true);
  fdc->seekDrive(0, 0);
  
  // if the process was canceled there might be garbage from the transfer over serial comm  
  ui->print(Progmem::getString(Progmem::uiDeleteLine));
  ui->print(Progmem::getString(Progmem::uiVT100ClearScreen));  
    
  // print if there was any response in details
  ui->print(Progmem::getString(m_cbSuccess ? Progmem::imdXmodemXferEnd : Progmem::imdXmodemXferFail));
  if (strlen(m_cbResponseStr))
  {
    ui->print(m_cbResponseStr);
  }
  // total bad sectors found
  if (m_cbSuccess)
  {
    ui->print(Progmem::getString(Progmem::imdBadSectorsDisk), m_cbTotalBadSectorsDisk);
    ui->print(Progmem::getString(Progmem::imdBadSectorsFile), m_cbTotalBadSectorsFile);
  }
  
  ui->print(Progmem::getString(Progmem::uiNewLine));
  ui->print(Progmem::getString(Progmem::uiContinue));
  ui->disableKeyboard(false);
  ui->readKey("\r");  
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
}

bool IMD::writeDiskCallback(DWORD packetNo, BYTE* data, WORD size)
{ 
  // of the current data packet, 128B or 1024B
  WORD packetIdx = 0;
  
  // what?
  if (!data || ((size != 128) && (size != 1024)))
  {
    m_cbSuccess = false;
    snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdXmodemErrPacket));
    return false;
  }
  
  // repeat until the packet is exhausted; returns true to ask for next, or returns false on error
  for (;;)
  {    
    // first step: check the IMD header of variable length; skip its contents (needs to end with EOF)
    if (m_cbProcessingHeader)
    {     
      if (packetNo == 1)
      {
        // have the message ready
        m_cbSuccess = false;
        snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdXmodemErrHeader));
        
        // verify it begins with "IMD " otherwise abort
        if (memcmp(data, "IMD ", 4) != 0)
        {
          return false;
        }
        packetIdx += 4;  
      }      
      
      // keep looking for ASCII EOF marking the end of header      
      while (m_cbProcessingHeader)
      {
        if (data[packetIdx] == 0x1A) // EOF
        {          
          m_cbProcessingHeader = false;
          m_cbResponseStr[0] = 0; // header skipped OK as it contains text description only
        }
        packetIdx++;
        
        // always take care if we're not at the end of the 128B/1024B datastream 
        CHECK_STREAM_END;
      }
    }   
    
    // specify and check for valid track data
    if (!m_cbModeSpecified)
    {
      if (m_cbMode != data[packetIdx])
      {
        m_cbMode = data[packetIdx];
        m_cbGeometryChanged = true;
      }    
      
      // mode value between 0-5, but since it's the first data byte, also allow EOF to end the transfer gracefully
      if (m_cbMode > 5)
      {
        if (m_cbMode == 0x1A)
        {
          m_cbSuccess = true;
          m_cbResponseStr[0] = 0;
          return false; // transfer over
        }
        else
        {
          m_cbSuccess = false;
          snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdXmodemErrMode));
          return false;
        }
      }
      
      packetIdx++;
      m_cbModeSpecified = true;
      CHECK_STREAM_END;
    }
    
    if (!m_cbCylinderSpecified)
    {
      m_cbCylinder = data[packetIdx];
      
      // more cylinders than configured?
      if (m_cbCylinder >= fdc->getParams()->Cylinders)
      {
        m_cbSuccess = false;
        snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdXmodemErrCyls));
        return false;
      }
      
      packetIdx++;
      m_cbCylinderSpecified = true;
      CHECK_STREAM_END;
    }
    
    if (!m_cbHeadSpecified)
    {
      // this byte was used to indicate presence of 2 more data maps
      m_cbHasSecTrackMap = data[packetIdx] & 0x80;
      m_cbHasSecHeadMap = data[packetIdx] & 0x40;
      m_cbHead = data[packetIdx] & 0x3F;

      // head (sides) number greater than 1
      if (m_cbHead > 1)
      {
        m_cbSuccess = false;
        snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdXmodemErrHead));
        return false;
      }
      
      packetIdx++;
      m_cbHeadSpecified = true;
      CHECK_STREAM_END;
    }
    
    if (!m_cbSptSpecified)
    {
      if (m_cbSpt != data[packetIdx])
      {
        m_cbSpt = data[packetIdx];
        m_cbGeometryChanged = true;
      }    
      
      // sectors per track greater than 63 are not feasible on a floppy, and as such, not supported here at this moment
      if (m_cbSpt > 63)
      {
        m_cbSuccess = false;
        snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdXmodemErrSpt));
        return false;
      }
      
      packetIdx++;
      m_cbSptSpecified = true;
      CHECK_STREAM_END;
    }
    
    if (!m_cbSecSizeSpecified)
    {
      if (m_cbSecSize != data[packetIdx])
      {
        m_cbSecSize = data[packetIdx];
        m_cbGeometryChanged = true;
      }    
      
      if (m_cbSecSize > 6)
      {
        m_cbSuccess = false;
        snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdXmodemErrSsize));
        return false;
      }
      
      // the Mega2560 board has 8K SRAM, so no chance of reading or writing large sectors - but we can format and verify those at least
      if (m_cbSecSize >= 5)
      {
        m_cbSuccess = false;
        snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdXmodemLowRAM), (m_cbSecSize == 5) ? 4 : 8);
        strcat(m_cbResponseStr, Progmem::getString(Progmem::imdAdvancedDetails));
        return false;
      }
     
      packetIdx++;
      m_cbSecSizeBytes = fdc->getSectorSizeBytes(m_cbSecSize);
      m_cbSecSizeSpecified = true;
      CHECK_STREAM_END;
    }
    
    // take a look at the sectors per track byte, if this is zero, the whole track was unreadable
    // this means that next 5 bytes of another track follow
    if (!m_cbSpt)
    {
      m_cbModeSpecified = false;
      m_cbCylinderSpecified = false;
      m_cbHeadSpecified = false;
      m_cbSptSpecified = false;
      m_cbSecSizeSpecified = false;
      m_cbGeometryChanged = true;
      continue;
    }
    
    // data is valid - now up to 3 maps follow, sector numbering map is there always
    if (!m_cbSecMapSpecified)
    {
      if (!m_cbSectorNumberingMap)
      {
        m_cbLastPos = 0;
        m_cbSectorNumberingMap = new BYTE[m_cbSpt];    
        
        if (!m_cbSectorNumberingMap)
        {
          m_cbSuccess = false;
          snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdMemoryError));
          return false;
        }
      }
      
      while (m_cbLastPos < m_cbSpt)
      {
        m_cbSectorNumberingMap[m_cbLastPos++] = data[packetIdx++];
        CHECK_STREAM_END;
      }
         
      m_cbSecMapSpecified = true;
      m_cbLastPos = 0;
      
      // now we have the sector numbering map
      // if there are more than 2 sectors per track, compute sector interleave factor from this sequence
      BYTE newInterleave = 1; // assume sequential
      if (m_cbSpt > 2)
      {
        // find starting sector
        bool found = false;
        WORD startingSector = 0;
        BYTE idx = 0;
        
        for (;;)
        {
          for (idx = 0; idx < m_cbSpt; idx++)
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
          m_cbLastPos = idx + 1;        
          while (m_cbLastPos < m_cbSpt)
          {
            if (m_cbSectorNumberingMap[m_cbLastPos++] != startingSector+1)
            {
              newInterleave++;
            }
            else
            {
              break;
            }
          }
          
          m_cbLastPos = 0;
        }
      }
      if (newInterleave != m_cbInterleave)
      {
        m_cbInterleave = newInterleave;
        m_cbGeometryChanged = true;
      }      
    }
    
    // these 2 maps are optional
    if (m_cbHasSecTrackMap && !m_cbSecTrackMapSpecified)
    {
      if (!m_cbSectorTrackMap)
      {
        m_cbLastPos = 0;
        m_cbSectorTrackMap = new BYTE[m_cbSpt];    
        
        if (!m_cbSectorTrackMap)
        {
          m_cbSuccess = false;
          snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdMemoryError));
          return false;
        }
      }
      
      while (m_cbLastPos < m_cbSpt)
      {
        m_cbSectorTrackMap[m_cbLastPos++] = data[packetIdx++];
        CHECK_STREAM_END;
      }
      
      m_cbSecTrackMapSpecified = true;
      m_cbLastPos = 0;
    }
    
    if (m_cbHasSecHeadMap && !m_cbSecHeadMapSpecified)
    {
      if (!m_cbSectorHeadMap)
      {
        m_cbLastPos = 0;
        m_cbSectorHeadMap = new BYTE[m_cbSpt];    
        
        if (!m_cbSectorHeadMap)
        {
          m_cbSuccess = false;
          snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdMemoryError));
          return false;
        }
      }
      
      while (m_cbLastPos < m_cbSpt)
      {
        m_cbSectorHeadMap[m_cbLastPos++] = data[packetIdx++];
        CHECK_STREAM_END;
      }
      
      m_cbSecHeadMapSpecified = true;
      m_cbLastPos = 0;
    }
    
    // sector data would now follow, but we need to format first before writing
    // check if single-sided operation was setup but the current image head number is indicating 2nd head
    bool skipOperation = ((fdc->getParams()->Heads == 1) && (m_cbHead > 0));
    
    // seek to physical cylinder and head
    if (!skipOperation && ((fdc->getCurrentCylinder() != m_cbCylinder) || (fdc->getCurrentHead() != m_cbHead)))
    {
      fdc->seekDrive(m_cbCylinder, m_cbHead);
      m_cbSeekIndicated = true;
    }
    
    // if mode, spt, secSize or interleave changed, update
    if (m_cbGeometryChanged)
    {
      if (!skipOperation)
      {
        switch(m_cbMode)
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
        
        fdc->getParams()->SectorsPerTrack = m_cbSpt;
        fdc->getParams()->SectorSizeBytes = m_cbSecSizeBytes;
        
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
        
        printGeometryInfo(m_cbCylinder, m_cbHead, m_cbInterleave);
      }
    }
    
    if (!skipOperation && m_cbSeekIndicated)
    { 
      // print progress on LCD
      ui->print(Progmem::getString(Progmem::imdProgress), m_cbCylinder, m_cbHead);
      
      // prepare CHSV table for formatting, we might have custom values for the cylinder and head
      memset(&g_rwBuffer[0], 0, m_cbSpt * 4);
      BYTE sectorIdx = 0;
      m_cbLastPos = 0;
      while (sectorIdx < m_cbSpt)
      {
        const BYTE logicalSector = m_cbSectorNumberingMap[sectorIdx];
        const BYTE logicalCylinder = m_cbHasSecTrackMap ? m_cbSectorTrackMap[sectorIdx] : m_cbCylinder;
        const BYTE logicalHead = m_cbHasSecHeadMap ? m_cbSectorHeadMap[sectorIdx] : m_cbHead;
        
        g_rwBuffer[m_cbLastPos++] = logicalCylinder;
        g_rwBuffer[m_cbLastPos++] = logicalHead;
        g_rwBuffer[m_cbLastPos++] = logicalSector;
        g_rwBuffer[m_cbLastPos++] = m_cbSecSize;
        
        sectorIdx++;
      }
      m_cbLastPos = 0;
      
      // format
      fdc->formatTrack(true);    
      
      // 2 errors that stop the datastream
      if (fdc->getLastError())
      {
        if (fdc->wasErrorNoDiskInDrive())
        {
          m_cbSuccess = false;
          snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::errINTTimeout));
          return false;
        }
        else if (fdc->wasErrorDiskProtected())
        {
          m_cbSuccess = false;
          snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::errWriProtect));
          return false;
        }
      }
    }
    
    // after formatting
    m_cbGeometryChanged = false;
    m_cbSeekIndicated = false;
    
    // determine sector data record type
    if (!m_cbSecDataTypeSpecified)
    {
      m_cbSectorDataType = data[packetIdx];
            
      // 0 to 8
      if (m_cbSectorDataType > 8)
      {
        m_cbSuccess = false;
        snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdXmodemErrData));
        return false;
      }
      
      packetIdx++;
      m_cbSecDataTypeSpecified = true;      
      CHECK_STREAM_END;
    }
       
    switch(m_cbSectorDataType)
    {
    case 1:
    case 3:
    case 5:
    case 7:
    {
      // 1 whole sector data of secSizeBytes, in packet chunks - copy it to g_rwBuffer at the proper position
      if (m_cbLastPos != m_cbSecSizeBytes)
      {
        const WORD maxCopyCount = size - packetIdx;
        const WORD copyCount = (m_cbLastPos+maxCopyCount > m_cbSecSizeBytes) ? m_cbSecSizeBytes-m_cbLastPos : maxCopyCount;
        memcpy(&g_rwBuffer[m_cbLastPos], &data[packetIdx], copyCount);
        m_cbLastPos += copyCount;
        packetIdx += copyCount;
        CHECK_STREAM_END;
      }
      m_cbLastPos = 0;
    }
    break;
    case 2:
    case 4:
    case 6:
    case 8:
    {
      // compressed data (same byte repeated secSizeBytes)
      memset(&g_rwBuffer[0], data[packetIdx++], m_cbSecSizeBytes);
    }
    break;
    }
    m_cbSecDataTypeSpecified = false; // g_rwBuffer prepared, next read will do another sector record
    
    // if the sector contains a "data error" flag 
    if (m_cbSectorDataType > 4)
    {
      m_cbTotalBadSectorsFile++;
      
      if (m_cbSkipBadSectorsInFile)
      {
        // if not to write sectors marked bad, set sector type to unavailable
        m_cbSectorDataType = 0;  
      }      
    }
    
    if (!skipOperation && m_cbSectorDataType) // write only if there were data available
    {
      const BYTE logicalSector = m_cbSectorNumberingMap[m_cbSectorIdx];
      // these 2 may differ from the physical position that was used for fdc->seekDrive() !
      const BYTE logicalCylinder = m_cbHasSecTrackMap ? m_cbSectorTrackMap[m_cbSectorIdx] : m_cbCylinder;
      const BYTE logicalHead = m_cbHasSecHeadMap ? m_cbSectorHeadMap[m_cbSectorIdx] : m_cbHead;    
      
      // normal data with DAM, compressed data with DAM, deleted data with read error, compressed deleted with read error
      const bool deletedDataMark = (m_cbSectorDataType == 3) || (m_cbSectorDataType == 4) || (m_cbSectorDataType == 7) || (m_cbSectorDataType == 8);
      
      // finally, write
      bool result = fdc->readWriteSectors(true, logicalSector, logicalSector, NULL, deletedDataMark, &logicalCylinder, &logicalHead);
      
      // 2 errors that stop the datastream
      if (fdc->getLastError())
      {
        if (fdc->wasErrorNoDiskInDrive())
        {
          m_cbSuccess = false;
          snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::errINTTimeout));
          return false;
        }
        else if (fdc->wasErrorDiskProtected())
        {
          m_cbSuccess = false;
          snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::errWriProtect));
          return false;
        }
        
        // since we're working sector by sector, if this write operation failed and verify was not specified, log one bad sector
        if (!m_cbDoVerify)
        {
          m_cbTotalBadSectorsDisk++;
        }
      }
      
      // write OK, now verify
      else if (m_cbDoVerify)
      {
        result = fdc->verify(logicalSector, false, &logicalCylinder, &logicalHead);
        
        if (fdc->getLastError())
        {
          if (fdc->wasErrorNoDiskInDrive())
          {
            m_cbSuccess = false;
            snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::errINTTimeout));
            return false;
          }
          
          m_cbTotalBadSectorsDisk++;
        }
      }
    }
    
    // so far so good
    m_cbSuccess = true;
    m_cbResponseStr[0] = 0;  
    m_cbSectorIdx++;  
    
    // reached the end of the track? start specifying next track
    if (m_cbSectorIdx == m_cbSpt)
    {
      m_cbSectorIdx = 0;
      m_cbLastPos = 0;
      
      m_cbModeSpecified = false;
      m_cbCylinderSpecified = false;
      m_cbHeadSpecified = false;
      m_cbSptSpecified = false;
      m_cbSecSizeSpecified = false;
      m_cbSecMapSpecified = false;
      m_cbSecTrackMapSpecified = false;
      m_cbSecHeadMapSpecified = false;
      m_cbSecDataTypeSpecified = false;
      
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
    
    CHECK_STREAM_END;
  }

  // infinite loop; doesn't reach here
  return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool tx(DWORD no, BYTE* data, WORD size)
{
  return imd.readDiskCallback(no, data, size);
}

void IMD::readDisk()
{
  bool useXMODEM1K = false;
  BYTE key = 0;
  BYTE* testAlloc = new BYTE[1030];
  if (testAlloc)
  {
    delete[] testAlloc;
    
    ui->print(Progmem::getString(Progmem::imdXmodemUse1k));
    key = toupper(ui->readKey("YN\e"));
    if (key == '\e')
    {
      ui->print(Progmem::getString(Progmem::uiNewLine));
      return;
    }
    ui->print(Progmem::getString(Progmem::uiEchoKey), key);
    useXMODEM1K = (key == 'Y');
  }
  
  ui->print("");
  ui->print(Progmem::getString(Progmem::imdInsertRead));    
  ui->print(Progmem::getString(Progmem::uiContinueAbort));
  key = ui->readKey("\r\e");
  ui->print(Progmem::getString(Progmem::uiNewLine));
  if (key == '\e')
  {
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
    memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
    return;
  }
  
  // ask for a comment in the file header; use the sector I/O buffer to store each line
  g_rwBuffer[0] = 0;
  strcpy(g_rwBuffer, Progmem::getString(Progmem::imdWriteHeader));
  strcat(g_rwBuffer, Progmem::getString(Progmem::imdRunPython));
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
  ui->print("");
  ui->print(Progmem::getString(useXMODEM1K ? Progmem::imdXmodem1k : Progmem::imdXmodem));
  ui->print(Progmem::getString(Progmem::imdXmodemWaitRecv));
  ui->disableKeyboard(true);
  ui->setPrintDisabled(false, true);
  
  m_cbResponseStr[0] = 0;
  m_cbSuccess = false;
  m_cbTotalBadSectorsDisk = 0;
  m_cbUnreadableTracks = 0;
  
  // read and transmit
  XModem modem(xmodemRx, xmodemTx, &tx, useXMODEM1K);
  modem.transmit();
  cleanup();
  dumpSerialTransfer();
   
  ui->print("");  
  ui->setPrintDisabled(false, false);  
  fdc->setAutomaticMotorOff(true);
  fdc->seekDrive(0, 0);
  
  ui->print(Progmem::getString(Progmem::uiDeleteLine));
  ui->print(Progmem::getString(Progmem::uiVT100ClearScreen));  
    
  ui->print(Progmem::getString(m_cbSuccess ? Progmem::imdXmodemXferEnd : Progmem::imdXmodemXferFail));
  if (strlen(m_cbResponseStr))
  {
    ui->print(m_cbResponseStr);
  }
  
  if (m_cbSuccess)
  {
    ui->print(Progmem::getString(Progmem::imdBadSectorsDisk), m_cbTotalBadSectorsDisk);
    ui->print(Progmem::getString(Progmem::imdUnreadableTrks), m_cbUnreadableTracks);
    
    // warn about the requirement to trim the received file from the EOF filler of the XMODEM packet
    ui->print(Progmem::getString(Progmem::uiNewLine));
    ui->print(Progmem::getString(Progmem::imdRunPython));
  }
  
  ui->print(Progmem::getString(Progmem::uiNewLine));
  ui->print(Progmem::getString(Progmem::uiContinue));
  ui->disableKeyboard(false);
  ui->readKey("\r");  
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
}

bool IMD::readDiskCallback(DWORD packetNo, BYTE* data, WORD size)
{
  WORD packetIdx = 0;
  
  if (!data || ((size != 128) && (size != 1024)))
  {
    m_cbSuccess = false;
    snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdXmodemErrPacket));
    return false;
  }
  
  // fill the output buffer with ASCII EOF (end-of-file) padding so it's known where the transfer ended
  // as XMODEM sends fixed 128B or 1024B packets
  memset(data, 0x1A, size);
  
  // end of transfer
  if (m_cbCylinder == fdc->getParams()->Cylinders)
  {
    return false;
  }
  
  for(;;)
  {
    // write IMD header and comment to output buffer, until EOF in the I/O buffer
    if (m_cbProcessingHeader)
    {       
      while (g_rwBuffer[m_cbLastPos] != 0x1A)
      {
        data[packetIdx++] = g_rwBuffer[m_cbLastPos++];
        CHECK_STREAM_END;
      }
      
      // get over the EOF too (to make the IMD file viewable with TYPE), then clear the buffer
      data[packetIdx++] = g_rwBuffer[m_cbLastPos++];
      memset(&g_rwBuffer[0], 0, m_cbLastPos);
      
      m_cbProcessingHeader = false;
      m_cbLastPos = 0;
      CHECK_STREAM_END;
    }
    
    // 1st track specification byte: mode
    if (!m_cbModeSpecified)
    {
      const WORD lastCommRate = fdc->getParams()->CommRate;
      const WORD lastSecSize = fdc->getParams()->SectorSizeBytes;
      const bool lastFM = fdc->getParams()->FM;
      
      // call autodetectCommRate on current track and head, this also sets fdc->sector size in bytes when successful
      fdc->getParams()->SectorSizeBytes = 0; // assume error
      const bool autodetected = autodetectCommRate();
      
      // geometry changed: new mode, sector size, spt or interleave (logical-OR this, as the last two will be checked later)
      m_cbGeometryChanged |= (lastCommRate != fdc->getParams()->CommRate) ||
                             (lastFM != fdc->getParams()->FM) ||
                             (lastSecSize != fdc->getParams()->SectorSizeBytes);
      if (autodetected)
      {
        // if we are reading a 300kbps disk and our translation setting is set to 250kbps:
        if (m_xlat300and250 && (fdc->getParams()->CommRate == 300))
        {
          m_cbMode = fdc->getParams()->FM ? 2 : 5;
        }  
        
        // normal
        else
        {
          if (fdc->getParams()->CommRate == 500)
          {
            m_cbMode = fdc->getParams()->FM ? 0 : 3;
          }
          else if (fdc->getParams()->CommRate == 300)
          {
            m_cbMode = fdc->getParams()->FM ? 1 : 4;
          }
          else if (fdc->getParams()->CommRate == 250)
          {
            m_cbMode = fdc->getParams()->FM ? 2 : 5;
          }
        }
      }
      
      else
      {
        // abort 
        if (fdc->wasErrorNoDiskInDrive())
        {
          m_cbSuccess = false;
          snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::errINTTimeout));
          return false;
        }
        
        m_cbMode = 0; // no sector IDs on the whole track: treat mode as zero byte
      }
     
      data[packetIdx++] = m_cbMode;
      m_cbModeSpecified = true;
      CHECK_STREAM_END;
    }
    
    if (!m_cbCylinderSpecified)
    {
      m_cbCylinder = fdc->getCurrentCylinder();
      data[packetIdx++] = m_cbCylinder;
      
      m_cbCylinderSpecified = true;
      CHECK_STREAM_END;
    }
    
    if (!m_cbHeadSpecified)
    {
      // but not just the head, bits 7 and 6 are used to indicate if logical cyl/head IDs don't match the physical cyl/head
      BYTE currHead = fdc->getCurrentHead();
      m_cbHead = currHead;
      
      // assume they don't, as only non-standard disks do
      m_cbHasSecTrackMap = false;
      m_cbHasSecHeadMap = false;
      
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
            m_cbSuccess = false;
            snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::errINTTimeout));
            return false;
          }
          
          // oopsie daisy
          // I swear to God I'm not touching anything with 8K of RAM or less in the Year of our Lord 2024
          m_cbSuccess = false;
          snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdMemoryError));
          return false;
        }
        
        // SPT out of range
        else if (fdc->getParams()->SectorsPerTrack > 63)
        {
          m_cbSuccess = false;
          snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdXmodemErrSpt));
          return false;
        }
                
        // skim through     
        for (BYTE idx = 0; idx < SECTORS_TABLE_COUNT; idx++)
        {
          // determined?
          if (m_cbHasSecTrackMap && m_cbHasSecHeadMap)
          {
            break;
          }
          
          // valid sector ID information?
          if (m_cbSectorsTable[idx] != 0xFFFF)
          {
            if (!m_cbHasSecTrackMap)
            {
              // sector ID cyl number differs from physical?
              const BYTE cylinder = (m_cbSectorsTable[idx] >> 8) & 0x7F;
              if (cylinder != m_cbCylinder)
              {
                m_cbHasSecTrackMap = true;
              }              
            }
            if (!m_cbHasSecHeadMap)
            {
              // sector ID head number differs from physical?
              const BYTE head = m_cbSectorsTable[idx] >> 15;
              if (head != m_cbHead)
              {                
                m_cbHasSecHeadMap = true;
              }
            } 
          }          
        }
        
        // last 2 checks if geometry changed are known here
        if ((oldInterleave != m_cbInterleave) || (oldSpt != fdc->getParams()->SectorsPerTrack))
        {
          m_cbGeometryChanged = true;
        }
      }

      // has custom track numbering in sector IDs
      if (m_cbHasSecTrackMap)
      {
        currHead |= 0x80;
      }
      // custom head numbering
      if (m_cbHasSecHeadMap)
      {
        currHead |= 0x40;
      }      
      
      data[packetIdx++] = currHead;      
      m_cbHeadSpecified = true;
      CHECK_STREAM_END;
    }
    
    if (!m_cbSptSpecified)
    {
      // initialized by autodetectCommRate()  
      if (fdc->getParams()->SectorSizeBytes > 0)
      {
        // initialized by autodetectInterleave()
        m_cbSpt = fdc->getParams()->SectorsPerTrack;  
      }
      else
      {
        m_cbSpt = 0;
      }
      
      if (m_cbSpt)
      {
        // as the interleave table almost always never starts from the beginning, find the starting sector
        WORD startingSector = 0;
        
        while (m_cbStartingSectorIdx == (BYTE)-1) // undefined
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
              m_cbStartingSectorIdx = idx;
              m_cbSectorIdx = m_cbStartingSectorIdx;
              m_cbLastPos = 0; // use this as count how many were written in the map
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
            m_cbSpt = 0;
            fdc->getParams()->SectorSizeBytes = 0;            
            break;
          }
        }
      }
          
      data[packetIdx++] = m_cbSpt; 
      m_cbSptSpecified = true;
      CHECK_STREAM_END;
    }
    
    if (!m_cbSecSizeSpecified)
    {
      if (fdc->getParams()->SectorSizeBytes > 0)
      {
        m_cbSecSizeBytes = fdc->getParams()->SectorSizeBytes;
        m_cbSecSize = fdc->convertSectorSize(m_cbSecSizeBytes);
      }
      else
      {
        m_cbSecSize = 0;
        m_cbSecSizeBytes = 0;
      }
      
      // more than 2K
      if (m_cbSecSize >= 5)
      {
        m_cbSuccess = false;
        snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdXmodemLowRAM), (m_cbSecSize == 5) ? 4 : 8);
        strcat(m_cbResponseStr, Progmem::getString(Progmem::imdAdvancedDetails));
        return false;
      }
      
      data[packetIdx++] = m_cbSecSize; 
      m_cbSecSizeSpecified = true;
      CHECK_STREAM_END;
    }
    
    // track has no valid sectors, advance
    if (!m_cbSpt || !m_cbSecSizeBytes)
    {
      // progress Unreadable
      ui->print(Progmem::getString(Progmem::imdProgress), m_cbCylinder, m_cbHead);
      ui->print(Progmem::getString(Progmem::imdTrackUnreadable));
      m_cbUnreadableTracks++;
      
      // re-specify
      m_cbModeSpecified = false;
      m_cbCylinderSpecified = false;
      m_cbHeadSpecified = false;
      m_cbSptSpecified = false;
      m_cbSecSizeSpecified = false;
      m_cbSecMapSpecified = false;
      m_cbSecTrackMapSpecified = false;
      m_cbSecHeadMapSpecified = false;
      m_cbStartingSectorIdx = (BYTE)-1;
      m_cbSectorIdx = 0;
      m_cbLastPos = 0;
      m_cbGeometryChanged = true;
      
      // seek to the next
      m_cbHead++;
      if (m_cbHead == fdc->getParams()->Heads)
      {
        m_cbHead = 0;
        m_cbCylinder++;
      }
      if (m_cbCylinder == fdc->getParams()->Cylinders)
      {
        m_cbSuccess = true;
        m_cbResponseStr[0] = 0;
        return true; // flush the buffer
      }
      
      fdc->seekDrive(m_cbCylinder, m_cbHead);      
      continue; // transfer continues
    }
    
    // next in the list is the mandatory sector map
    if (!m_cbSecMapSpecified)
    {
      // now write the sector numbering map
      while ((m_cbLastPos < m_cbSpt) && (m_cbSectorIdx < SECTORS_TABLE_COUNT))
      {
        if (m_cbSectorsTable[m_cbSectorIdx] == 0xFFFF) // undefined?
        {
          m_cbSectorIdx++;
          continue;
        }
        
        m_cbCurrentSector = (BYTE)m_cbSectorsTable[m_cbSectorIdx++]; // sector info only
        
        data[packetIdx++] = m_cbCurrentSector;
        m_cbLastPos++;
        CHECK_STREAM_END;
      }
      
      // sectors per track count not reached: do we still need to go from the beginning of the table?
      if ((m_cbSectorIdx == SECTORS_TABLE_COUNT) && (m_cbLastPos < m_cbSpt))
      {
        bool found = false;
            
        while (m_cbCurrentSector && !found)
        {
          for (m_cbSectorIdx = 0; m_cbSectorIdx < SECTORS_TABLE_COUNT; m_cbSectorIdx++)
          {
            if (((BYTE)m_cbSectorsTable[m_cbSectorIdx] == m_cbCurrentSector) &&
                 (m_cbSectorsTable[m_cbSectorIdx] != 0xFFFF))
            {
              found = true;
              break;
            }
          }
          
          if (!found)
          {
            m_cbCurrentSector++; // possible gap?
          }
        }            
        
        // found from the beginning, get the succeeding sector index
        if (found)
        {
          m_cbSectorIdx += 1;
          if (m_cbSectorIdx < SECTORS_TABLE_COUNT)
          {
            continue; // valid
          }
        }
        
        // not found or out-of-bounds
        m_cbSectorIdx = 0;
        continue;
      }
      
      // reuse m_cbStartingSectorIdx for the rest of the maps
      m_cbSectorIdx = m_cbStartingSectorIdx;
      m_cbLastPos = 0;
      m_cbCurrentSector = 0;
      m_cbSecMapSpecified = true;
    }
    
    if (m_cbHasSecTrackMap && !m_cbSecTrackMapSpecified)
    {
      // write sector track numbering analog to above
      while ((m_cbLastPos < m_cbSpt) && (m_cbSectorIdx < SECTORS_TABLE_COUNT))
      {
        if (m_cbSectorsTable[m_cbSectorIdx] == 0xFFFF)
        {
          m_cbSectorIdx++;
          continue;
        }
        
        m_cbCurrentSector = (BYTE)m_cbSectorsTable[m_cbSectorIdx];
        data[packetIdx++] = (m_cbSectorsTable[m_cbSectorIdx++] >> 8) & 0x7F; // cyl info only
        m_cbLastPos++;
        CHECK_STREAM_END;
      }
      if ((m_cbSectorIdx == SECTORS_TABLE_COUNT) && (m_cbLastPos < m_cbSpt))
      {
        bool found = false;
            
        while (m_cbCurrentSector && !found)
        {
          for (m_cbSectorIdx = 0; m_cbSectorIdx < SECTORS_TABLE_COUNT; m_cbSectorIdx++)
          {
            if (((BYTE)m_cbSectorsTable[m_cbSectorIdx] == m_cbCurrentSector) &&
                 (m_cbSectorsTable[m_cbSectorIdx] != 0xFFFF))
            {
              found = true;
              break;
            }
          }
          
          if (!found)
          {
            m_cbCurrentSector++;
          }
        }
      
        if (found)
        {
          m_cbSectorIdx += 1;
          if (m_cbSectorIdx < SECTORS_TABLE_COUNT)
          {
            continue;
          }
        }
        
        m_cbSectorIdx = 0;
        continue;
      }
      
      m_cbSectorIdx = m_cbStartingSectorIdx;
      m_cbLastPos = 0;
      m_cbCurrentSector = 0;
      m_cbSecTrackMapSpecified = true;
    }
    
    if (m_cbHasSecHeadMap && !m_cbSecHeadMapSpecified)
    {
      while ((m_cbLastPos < m_cbSpt) && (m_cbSectorIdx < SECTORS_TABLE_COUNT))
      {
        if (m_cbSectorsTable[m_cbSectorIdx] == 0xFFFF)
        {
          m_cbSectorIdx++;
          continue;
        }
        
        m_cbCurrentSector = (BYTE)m_cbSectorsTable[m_cbSectorIdx];
        data[packetIdx++] = m_cbSectorsTable[m_cbSectorIdx++] >> 15; // head info only
        m_cbLastPos++;
        CHECK_STREAM_END;
      }
      if ((m_cbSectorIdx == SECTORS_TABLE_COUNT) && (m_cbLastPos < m_cbSpt))
      {
        bool found = false;
            
        while (m_cbCurrentSector && !found)
        {
          for (m_cbSectorIdx = 0; m_cbSectorIdx < SECTORS_TABLE_COUNT; m_cbSectorIdx++)
          {
            if (((BYTE)m_cbSectorsTable[m_cbSectorIdx] == m_cbCurrentSector) &&
                (m_cbSectorsTable[m_cbSectorIdx] != 0xFFFF))
            {
              found = true;
              break;
            }
          }
          
          if (!found)
          {
            m_cbCurrentSector++;
          }
        }
      
        if (found)
        {
          m_cbSectorIdx += 1;
          if (m_cbSectorIdx < SECTORS_TABLE_COUNT)
          {
            continue;
          }
        }
        
        m_cbSectorIdx = 0;
        continue;
      }
      
      m_cbSectorIdx = m_cbStartingSectorIdx;
      m_cbLastPos = 0;
      m_cbCurrentSector = 0;      
      m_cbSecHeadMapSpecified = true;
    }
    
    // if mode, spt, secSize or interleave changed, update gaps and print info
    if (m_cbGeometryChanged)
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
      
      printGeometryInfo(m_cbCylinder, m_cbHead, m_cbInterleave);
      ui->print(Progmem::getString(Progmem::imdProgress), m_cbCylinder, m_cbHead);
      m_cbGeometryChanged = false; // reset the flag
    }
    
    // now try to read    
    while ((m_cbLastPos < m_cbSpt) && (m_cbSectorIdx < SECTORS_TABLE_COUNT))
    {     
      static WORD rwBufferPos = 0;
      
      if (!m_cbSecDataTypeSpecified) // determine data record type
      {
        rwBufferPos = 0;
        
        if (m_cbSectorsTable[m_cbSectorIdx] == 0xFFFF) // skip undefined
        {
          m_cbSectorIdx++;
          continue;
        }
        
        const BYTE logicalSector = (BYTE)m_cbSectorsTable[m_cbSectorIdx];
        m_cbCurrentSector = logicalSector;
        const BYTE logicalCylinder = m_cbHasSecTrackMap ? (m_cbSectorsTable[m_cbSectorIdx] >> 8) & 0x7F : m_cbCylinder;
        const BYTE logicalHead = m_cbHasSecHeadMap ? m_cbSectorsTable[m_cbSectorIdx] >> 15 : m_cbHead;
        memset(&g_rwBuffer[0], 0, fdc->getParams()->SectorSizeBytes);
        
        fdc->readWriteSectors(false, logicalSector, logicalSector, NULL, false, &logicalCylinder, &logicalHead);        
        if (fdc->getLastError())
        {
          if (fdc->wasErrorNoDiskInDrive())
          {
            m_cbSuccess = false;
            snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::errINTTimeout));
            return false;
          }          
          m_cbTotalBadSectorsDisk++;
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
        
        // form data record type, 1 to 8
        if (!fdc->getLastError())
        {
          m_cbSectorDataType = compressedData ? 2 : 1;
          if (fdc->wasControlMark()) // deleted data mark
          {
            m_cbSectorDataType = compressedData ? 4 : 3;
          }
        }
        else
        {
          // if the whole buffer remained zeroed after the read, then the sector data is unavailable
          if (compressedData && (g_rwBuffer[0] == 0))
          {
            m_cbSectorDataType = 0;
          }
          else
          {
            m_cbSectorDataType = compressedData ? 6 : 5;
            if (fdc->wasControlMark())
            {
              m_cbSectorDataType = compressedData ? 8 : 7;
            }
          }          
        }        
        
        data[packetIdx++] = m_cbSectorDataType; 
        m_cbSecDataTypeSpecified = true;
        CHECK_STREAM_END;
      }
                
      // data now ready in g_rwBuffer according to data record type
      switch(m_cbSectorDataType)
      {
      case 1:
      case 3:
      case 5:
      case 7:
      {
        while (rwBufferPos != m_cbSecSizeBytes)
        {
          const WORD maxCopyCount = size - packetIdx;
          const WORD copyCount = (rwBufferPos+maxCopyCount > m_cbSecSizeBytes) ? m_cbSecSizeBytes-rwBufferPos : maxCopyCount;
          memcpy(&data[packetIdx], &g_rwBuffer[rwBufferPos], copyCount);
          rwBufferPos += copyCount;
          packetIdx += copyCount;
          CHECK_STREAM_END;
        }
        rwBufferPos = 0;
      }
      break;
      case 2:
      case 4:
      case 6:
      case 8:
      {
        // compressed data (same byte repeated secSizeBytes)
        data[packetIdx++] = g_rwBuffer[0];
        m_cbSectorDataType = 0; // go to next sector
        CHECK_STREAM_END;
      }
      break;
      }
      
      // next sector
      m_cbSectorIdx++;
      m_cbLastPos++;
      m_cbSecDataTypeSpecified = false;
    }    
    if ((m_cbSectorIdx == SECTORS_TABLE_COUNT) && (m_cbLastPos < m_cbSpt))
    {
      bool found = false;
            
      while (m_cbCurrentSector && !found)
      {
        for (m_cbSectorIdx = 0; m_cbSectorIdx < SECTORS_TABLE_COUNT; m_cbSectorIdx++)
        {
          if (((BYTE)m_cbSectorsTable[m_cbSectorIdx] == m_cbCurrentSector) &&
               (m_cbSectorsTable[m_cbSectorIdx] != 0xFFFF))
          {
            found = true;
            break;
          }
        }
        
        if (!found)
        {
          m_cbCurrentSector++;
        }
      }
    
      if (found)
      {
        m_cbSectorIdx += 1;
        if (m_cbSectorIdx < SECTORS_TABLE_COUNT)
        {
          continue;
        }
      }

      m_cbSectorIdx = 0;
      continue;
    }
       
    // end of track?
    m_cbSuccess = true;
    m_cbResponseStr[0] = 0;

    // re-specify
    m_cbModeSpecified = false;
    m_cbCylinderSpecified = false;
    m_cbHeadSpecified = false;
    m_cbSptSpecified = false;
    m_cbSecSizeSpecified = false;
    m_cbSecMapSpecified = false;
    m_cbSecTrackMapSpecified = false;
    m_cbSecHeadMapSpecified = false;
    m_cbSecDataTypeSpecified = false;
    m_cbLastPos = 0;
    m_cbSectorIdx = 0;
    m_cbCurrentSector = 0;
    m_cbStartingSectorIdx = (BYTE)-1;
    
    // and seek to next
    m_cbHead++;
    if (m_cbHead == fdc->getParams()->Heads)
    {
      m_cbHead = 0;
      m_cbCylinder++;
    }
    if (m_cbCylinder == fdc->getParams()->Cylinders)
    {
      m_cbSuccess = true;
      m_cbResponseStr[0] = 0;
      return true; // flush the buffer
    }

    ui->print(Progmem::getString(Progmem::imdProgress), m_cbCylinder, m_cbHead);
    fdc->seekDrive(m_cbCylinder, m_cbHead);  
  }
  
  return false;
}

#endif // SD_CARD_SUPPORT

#endif // BUILD_IMD_IMAGER