// MegaFDC (c) 2023-2025 J. Bogin, http://boginjr.com
// IMD (ImageDisk)-compatible disk imager

#include "config.h"

#ifdef BUILD_IMD_IMAGER

IMD imd;

// similar to commands.cpp, but initialize 1 drive here
void InitializeDrives()
{
  ui->print(Progmem::getString(Progmem::uiDeleteLine));
  ui->print("");  
  ui->print(Progmem::getString(Progmem::uiSplash));
  ui->print(Progmem::getString(Progmem::imdSplash));
  ui->print(Progmem::getString(Progmem::uiBuild));
  
  // recommend display output to see current disk geometry/bad sectors/unreadable tracks during serial transfers
  // without an LCD, only a string response is written at the end, when XMODEM finishes and serial becomes available
  if (!g_uiEnabled)
  {
    ui->print(Progmem::getString(Progmem::imdLCDRecommended));  
  }
    
  // check if the floppy controller is responding to a reset command
  ui->print(Progmem::getString(Progmem::uiInitializingFDC));
  fdc->resetController();
  ui->print(" ");
  ui->print(Progmem::getString(Progmem::uiOK));
  ui->print(Progmem::getString(Progmem::uiNewLine2x));
  
  // setup parameters: drive letter, type, tracks, - optional: double stepping, sides, sector and format gaps
  ui->print(Progmem::getString(Progmem::imdDriveLetter));
  BYTE key = toupper(ui->readKey("ABCD"));
  imd.getParams().DriveNumber = key - 65;  
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  
  ui->print(Progmem::getString(Progmem::imdDriveType));
  key = ui->readKey("853");
  imd.getParams().DriveInches = key - 48;
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  
  while(true)
  {
    ui->print(Progmem::getString(Progmem::imdDriveTracks));
    WORD tracks = (WORD)atoi(ui->prompt(3, Progmem::getString(Progmem::uiDecimalInput)));
    if ((tracks > 0) && (tracks <= 100))
    {
      imd.getParams().Tracks = (BYTE)tracks;
      ui->print(Progmem::getString(Progmem::uiNewLine));
      break;
    }
    
    ui->print(Progmem::getString(Progmem::uiDeleteLine));
  }
  
  ui->print(Progmem::getString(Progmem::imdDoubleStepAuto));
  key = toupper(ui->readKey("YNG"));
  imd.getParams().DoubleStepping = (key == 'Y');
  imd.setAutodetectDoubleStep(key == 'G');
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  
  ui->print(Progmem::getString(Progmem::imdDiskSidesAuto));
  key = toupper(ui->readKey("YN"));
  if (key == 'Y')
  {
    imd.getParams().Heads = 1;
  }
  else
  {
    imd.getParams().Heads = 2;
    imd.setAutodetectHeads(true);
  }  
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
    
  ui->print(Progmem::getString(Progmem::imdSectorGap));
  imd.getParams().GapLength = (BYTE)strtoul(ui->prompt(2, Progmem::getString(Progmem::uiHexadecimalInput)), 0, 16);
  if (!imd.getParams().GapLength)
  {
    imd.setAutodetectSectorGap(true);    
    if (!strlen(ui->getPromptBuffer()))
    {
      ui->print("0");    
    }
  }
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  ui->print(Progmem::getString(Progmem::imdFormatGap));
  imd.getParams().Gap3Length = (BYTE)strtoul(ui->prompt(2, Progmem::getString(Progmem::uiHexadecimalInput)), 0, 16);
  if (!imd.getParams().Gap3Length)
  {
    imd.setAutodetectFormatGap(true);    
    if (!strlen(ui->getPromptBuffer()))
    {
      ui->print("0");    
    }
  }
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  // initialize configured drive, use the slowest data rate for now
  imd.getParams().CommRate = 250;
  fdc->setActiveDrive(&imd.getParams());
}

void ProcessCommands()
{
  // main menu
  for(;;)
  {
    ui->print("");
    ui->print(Progmem::getString(Progmem::imdOptionRead));
    ui->print(Progmem::getString(Progmem::imdOptionWrite));
    ui->print(Progmem::getString(Progmem::imdOptionFormat));
    ui->print(Progmem::getString(Progmem::imdOptionErase));
    ui->print(Progmem::getString(Progmem::imdOptionTests));
    ui->print(Progmem::getString(Progmem::imdOptionXlat));
    ui->print(Progmem::getString(Progmem::imdOptionReset));
    ui->print(Progmem::getString(Progmem::uiChooseOption));
    BYTE mainmenu = toupper(ui->readKey("RWFETDQ"));
    ui->print(Progmem::getString(Progmem::uiEchoKey), mainmenu);
    
    if (mainmenu == 'R')
    {
      imd.readDisk();
    }
       
    else if (mainmenu == 'W')
    {
      imd.writeDisk();
    }
    
    else if (mainmenu == 'F')
    {
      imd.formatDisk();
    }
    
    else if (mainmenu == 'E')
    {
      imd.eraseDisk();
    }
    
    else if (mainmenu == 'T')
    {
      for (;;)
      {
        ui->print("");
        ui->print(Progmem::getString(Progmem::imdTestFDC));
        ui->print(Progmem::getString(Progmem::imdTestDrive));
        ui->print(Progmem::getString(Progmem::uiCancelOption));
        ui->print(Progmem::getString(Progmem::uiNewLine));
        ui->print(Progmem::getString(Progmem::uiChooseOption));
        BYTE tests = toupper(ui->readKey("FDC"));
        ui->print(Progmem::getString(Progmem::uiEchoKey), tests);
        
        if (tests == 'F')
        {
          imd.testController();
        }
        
        else if (tests == 'D')
        {
          imd.testDrive();
        }
        
        else if (tests == 'C')
        {
          break;
        }  
      }      
    }
    
    else if (mainmenu == 'D')
    {
      ui->print("");
      ui->print(Progmem::getString(Progmem::imdAdvancedDetails));
      ui->print(Progmem::getString(Progmem::imdDefaultsNo));
      ui->print(Progmem::getString(Progmem::imdXlate300kbps));
      BYTE key = toupper(ui->readKey("YN"));
      imd.setDataRateTranslations(key == 'Y');
      ui->print(Progmem::getString(Progmem::uiEchoKey), key);      
    }
    
    else if (mainmenu == 'Q')
    {
      ui->reset();
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMD::IMD()
{
  m_autoDoubleStep = false;
  m_autoHeads = false;
  m_autoSectorGap = false;
  m_autoFormatGap = false;
  m_lastGoodUseFM = false;
  m_xlat300and250 = false;
  m_lastGoodCommRate = 0;
  
  // format interleave sequential by default
  m_formatInterleave = 1;
  
  // IMD file transfer
  m_cbSuccess = false;
  m_cbDoVerify = false;
  m_cbSkipBadSectorsInFile = false;
  m_cbResponseStr[0] = 0;
  m_cbTotalBadSectorsDisk = 0;
  m_cbUnreadableTracks = 0;
  m_cbTotalBadSectorsFile = 0;
  m_cbSectorNumberingMap = NULL;
  m_cbSectorTrackMap = NULL;
  m_cbSectorHeadMap = NULL;
  m_cbSectorsTable = NULL;
  cleanupCallback();
}

bool IMD::autodetectCommRate()
{ 
  // autodetect what FDC communication rate to use, from the current physical track/head
  // on success, also initializes fdc->SectorSizeBytes
  const WORD commRates[] = {500, 250, 300};
  const bool useFM[] = {false, true};
  
  for (BYTE fmIdx = 0; fmIdx < sizeof(useFM) / sizeof(useFM[0]); fmIdx++)
  {
    for (BYTE rateIdx = 0; rateIdx < sizeof(commRates) / sizeof(commRates[0]); )
    {     
      // try the last good one?
      if (m_lastGoodCommRate)
      {
        fdc->getParams()->FM = m_lastGoodUseFM;
        fdc->getParams()->CommRate = m_lastGoodCommRate;
        m_lastGoodCommRate = 0; // next time, go with commRates[]
      }
      else
      {
        fdc->getParams()->FM = useFM[fmIdx];
        fdc->getParams()->CommRate = commRates[rateIdx++];
      }      
      fdc->setCommunicationRate();
           
      // success? store the last good combination here
      BYTE sectorSizeN = 0;
      if (fdc->readSectorID(NULL, NULL, NULL, &sectorSizeN))
      {
        m_lastGoodCommRate = fdc->getParams()->CommRate;
        m_lastGoodUseFM = fdc->getParams()->FM;        
        fdc->getParams()->SectorSizeBytes = fdc->getSectorSizeBytes(sectorSizeN);       
        return true;
      }     
     
      // pointless to try another combination
      if (fdc->wasErrorNoDiskInDrive())
      {
        return false;          
      }
    }
  }
  
  return false;
}

bool IMD::autodetectDoubleStep()
{
  // comm rate must be already set or detected (disk must be in)
  // do detection on the current side (head)
  // returns true and sets DoubleStepping accordingly or false if couldn't detect
  const BYTE saveTrack = fdc->getCurrentTrack();
  const BYTE saveHead = fdc->getCurrentHead();
  
  // go to track 0 and assume single step first    
  fdc->getParams()->DoubleStepping = false;
  fdc->recalibrateDrive();
  fdc->seekDrive(0, saveHead);
  
  // find sector 1 on track 0 within some 2.5 sec.
  BYTE currTrack = (BYTE)-1;
  BYTE currSector = (BYTE)-1;
  DWORD timeStart = millis();
  while ((currSector != 1) && ((millis() - timeStart) < 2500))
  {
    if (!fdc->readSectorID(&currTrack, NULL, &currSector))
    {
      if (fdc->wasErrorNoDiskInDrive())
      {
        fdc->seekDrive(saveTrack, saveHead);
        return false;
      }
    }
  }
  if ((currSector != 1) || (currTrack != 0))
  {
    fdc->seekDrive(saveTrack, saveHead);
    return false;
  }
  
  // now advance to track 2 and do the same
  fdc->seekDrive(2, saveHead);
  currSector = (BYTE)-1;
  currTrack = (BYTE)-1;
  timeStart = millis();
  while ((currSector != 1) && ((millis() - timeStart) < 2500))
  {
    if (!fdc->readSectorID(&currTrack, NULL, &currSector))
    {
      if (fdc->wasErrorNoDiskInDrive())
      {
        fdc->seekDrive(saveTrack, saveHead);
        return false;
      }
    }
  }
  
  if ((currSector == 1) && (currTrack == 2))
  {
    // auto-detected single stepping
    fdc->seekDrive(saveTrack, saveHead);
    return true;
  }
  else if ((currSector == 1) && (currTrack == 1))
  {
    // auto-detected double stepping
    fdc->getParams()->DoubleStepping = true;
    fdc->recalibrateDrive();
    fdc->seekDrive(saveTrack, saveHead);
    return true;
  }
  
  fdc->recalibrateDrive();
  fdc->seekDrive(saveTrack, saveHead);
  return false; // we tried, didn't we
}

bool IMD::autodetectHeads()
{
  // analog to the one above, except that the sector number is ignored
  // side 0 shall be commrate detected
  const BYTE saveTrack = fdc->getCurrentTrack();
  const BYTE saveHead = fdc->getCurrentHead();
  
  // find any sector on current track of side 0
  fdc->seekDrive(saveTrack, 0);
  BYTE currHead = (BYTE)-1;
  DWORD timeStart = millis();
  while ((millis() - timeStart) < 2500)
  {
    if (!fdc->readSectorID(NULL, &currHead))
    {
      if (fdc->wasErrorNoDiskInDrive())
      {
        fdc->seekDrive(saveTrack, saveHead);
        return false;
      }
    }
    else
    {
      break;
    }
  }  
  // rather don't autodetect if this one ain't right
  if (currHead != 0)
  {
    fdc->seekDrive(saveTrack, saveHead);
    return false;
  }
  
  // now check if currHead on side 1 is also 1, then the disk contains a double sided recording
  fdc->seekDrive(saveTrack, 1);
  currHead = (BYTE)-1;

  // different data rate on second side?
  FDC::DiskDriveMediaParams backup(m_params);
  if (autodetectCommRate()) 
  {
    timeStart = millis();
    while ((millis() - timeStart) < 2500)
    {
      if (!fdc->readSectorID(NULL, &currHead))
      {
        if (fdc->wasErrorNoDiskInDrive())
        {
          fdc->seekDrive(saveTrack, saveHead);
          return false;
        }
      }
      else
      {
        break;
      }
    }  
  }  
  m_params = backup;
  m_lastGoodCommRate = fdc->getParams()->CommRate;
  m_lastGoodUseFM = fdc->getParams()->FM;  
  // restore from backup for a quicker response
    
  if (currHead == 1)
  {
    fdc->getParams()->Heads = 2;
  }
  else
  {
    // if it is whatever else, or could not read at all
    fdc->getParams()->Heads = 1;
  }
  
  fdc->seekDrive(saveTrack, saveHead);
  return true;  
}

WORD* IMD::autodetectSectorsPerTrack(BYTE& observedSPT, BYTE& maximumSPT)
{
  // from the current physical track, return a table of sectors and their logical IDs
  observedSPT = 0;
  maximumSPT = 0;  
  
  WORD* sectorsTable = new WORD[SECTORS_TABLE_COUNT];
  if (!sectorsTable)
  {
    return NULL;
  }
  memset(sectorsTable, 0, SECTORS_TABLE_COUNT*sizeof(WORD));
  
  // create an array of sectors sequence
  // sectors are 1-based, so 0s are empty (invalid) entries that didn't make it in disk rotations
  BYTE idx = 0;
  BYTE computeSPT = 0;  
  DWORD timeStart = millis();
  
  // 5sec should give at least 25 complete disk revolutions @ 300RPM
  while ((millis() - timeStart) < 5000)
  {
    BYTE track = 0;
    BYTE head = 0;        
    BYTE sector = 0;
    if (fdc->readSectorID(&track, &head, &sector) && sector)
    {
      if (idx == 0)
      {
        // count number of times until we see the same sector again: that's the observed SPT
        computeSPT = sector;      
      }
      else if (computeSPT == sector)
      {
        computeSPT = 0; // stop counting
      }
      if (computeSPT)
      {
        observedSPT++;
      }
      
      // the maximum logical sector value seen
      if (sector > maximumSPT)
      {
        maximumSPT = sector;
      }
      
      // each item is 16-bit, upper 8 bits: logical cylinder number, lower 8 bits: log. head (bit 7) and sector (bits 6-0)
      sectorsTable[idx++] = ((WORD)track << 8) | (head << 7) | (sector & 0x7F);
      if (idx == SECTORS_TABLE_COUNT)
      {
        break;
      }
    }
    else
    {
      if (fdc->wasErrorNoDiskInDrive())
      {
        delete[] sectorsTable;
        return NULL;
      }
    }
  }
  
  // failsafe
  if (observedSPT > maximumSPT)
  {
    observedSPT = maximumSPT;
  }
  
  return sectorsTable;
}

bool IMD::autodetectInterleave()
{
  // first autodetect SPT by scanning the track into a sectors table,
  // then compute interleave factor for the r/w callbacks (m_cbInterleave)
  if (m_cbSectorsTable)
  {
    delete[] m_cbSectorsTable;
    m_cbSectorsTable = NULL;
  }
  
  // up to 3 attempts if there are gaps due to bad sector presence
  WORD* attempts[3] = {NULL};
  BYTE sectorsPerTrack = 0;
  BYTE idx;
  BYTE idxToUse = (BYTE)-1; // unsure, yet
  
  for (idx = 0; idx < 3; idx++)
  {
    BYTE observedSPT = 0;
    BYTE maximumNumber = 0;
    attempts[idx] = autodetectSectorsPerTrack(observedSPT, maximumNumber);
    
    if (!attempts[idx])
    {
      idxToUse = (BYTE)-1; // indicate failure, due to no disk in drive or memory error
      break;
    }
        
    // no gaps in the sectors being scanned, done
    if ((observedSPT == maximumNumber) && (observedSPT > 0))
    {
      sectorsPerTrack = observedSPT;
      idxToUse = idx;
      break;
    }
    
    // gaps present, try to re-seek and retry
    if (observedSPT > sectorsPerTrack)
    {
      sectorsPerTrack = observedSPT;
      idxToUse = idx;
    }
    
    fdc->recalibrateDrive();
    fdc->seekDrive(fdc->getCurrentTrack(), fdc->getCurrentHead());
  } 
  
  // determine which attempt at scanning yielded the most observed SPT and use that
  // deallocate the other tables, if present
  for (idx = 0; idx < 3; idx++)
  {
    if (attempts[idx])
    {
      if (idx == idxToUse)
      {
        m_cbSectorsTable = attempts[idx];
        continue;
      }
      
      delete[] attempts[idx];
    }
  }
  
  // no valid sector IDs, no disk, memory error
  if (!m_cbSectorsTable || !sectorsPerTrack)
  {
    return false;
  }
    
  // if SPT is below 3, it is sequential
  else if (sectorsPerTrack < 3)
  {
    fdc->getParams()->SectorsPerTrack = sectorsPerTrack;
    m_cbInterleave = 1; 
    return true;
  }
  
  // sanity checks
  BYTE thisSector = 0;
  BYTE nextSector = 0;
  idx = 0;
  while (idx < SECTORS_TABLE_COUNT-1)
  {
    thisSector = ((BYTE)m_cbSectorsTable[idx]) & 0x7F;
    nextSector = ((BYTE)m_cbSectorsTable[idx+1]) & 0x7F;
    // zero in table?
    if (!thisSector || !nextSector)
    {
      idx++;
    }    
    // neither of these is the last sector
    else if ((thisSector != sectorsPerTrack) && (nextSector != sectorsPerTrack))
    {
      break;
    }
    idx++;
  }
  
  if (thisSector && nextSector && (idx < SECTORS_TABLE_COUNT-1))
  {
    if ((nextSector - thisSector) == 1)
    {
      // confirmed sequential
      fdc->getParams()->SectorsPerTrack = sectorsPerTrack;
      m_cbInterleave = 1;
      return true;
    }
    
    // nope, try to compute from thisSector
    BYTE idx2 = idx;
    BYTE interleave = 0;
    while (idx2 < SECTORS_TABLE_COUNT)
    {
      if (((BYTE)m_cbSectorsTable[idx2++] & 0x7F) != thisSector+1)
      {
        interleave++;
      }
      else
      {
        break;
      }
    }
    // double-check if nextSector is advanced by the same factor
    if (((BYTE)m_cbSectorsTable[idx+1+interleave] & 0x7F) == nextSector+1)
    {
      fdc->getParams()->SectorsPerTrack = sectorsPerTrack;
      m_cbInterleave = interleave;
      return true;
    }
  }
  
  // could not determine interleave, fall back to sequential but store SPT
  fdc->getParams()->SectorsPerTrack = sectorsPerTrack;
  m_cbInterleave = 1;
  return false;
}

void IMD::autodetectGaps(BYTE& sectorGap, BYTE& formatGap)
{
  const BYTE sectorSizeN = fdc->convertSectorSize(fdc->getParams()->SectorSizeBytes);

  WORD gap = 0;
  WORD gap3 = 0;

  // exists pre-defined?
  BYTE idx = 0;
  while (Progmem::imdGapTable(idx) != 0xFF)
  {
    if ((Progmem::imdGapTable(idx) == sectorSizeN) &&
        (Progmem::imdGapTable(idx+1) == fdc->getParams()->SectorsPerTrack))
    {
      gap = Progmem::imdGapTable(idx+2);
      gap3 = Progmem::imdGapTable(idx+3);
      break;
    }
    idx += 4;
  }

  // if not, calculate the same exact way IMD does it...
  // credits go to Dave Dunfield for this one!
  if ((gap == 0) || (gap3 == 0))
  {
    if (fdc->getParams()->FM)
    {
      gap3 = (((fdc->getParams()->CommRate == 500) ? 4962 : 2948) / fdc->getParams()->SectorsPerTrack) - 33;
    }
    else
    {
      gap3 = (((fdc->getParams()->CommRate == 500) ? 9926 : 5898) / fdc->getParams()->SectorsPerTrack) - 62;
    }
    if ((gap3 <= fdc->getParams()->SectorSizeBytes) || ((gap3 -= fdc->getParams()->SectorSizeBytes) < 13))
    {
      gap3 = 13;
    }
    
    // min. 7, 13; max 200, 255
    gap = (gap3 * 2) / 3;
    if ((gap -= (gap / 5)) < 7)
    {
      gap = 7;
    }
    if (gap > 200)
    {
      gap = 200;
    }
    if (gap3 > 255)
    {
      gap3 = 255;
    }
  }

  sectorGap = (BYTE)gap;
  formatGap = (BYTE)gap3;
}

bool IMD::tryAskIfCannotAutodetect(bool requiredDoubleStep, bool requiredHeads)
{
  // returns: true if all required were specified, false on Escape keypress
  
  if((m_autoDoubleStep && requiredDoubleStep) ||
     (m_autoHeads && requiredHeads))     
  {
    ui->print("");
    ui->print(Progmem::getString(Progmem::imdSpecifyOptions));
    ui->print(Progmem::getString(Progmem::imdSpecifyOptions2));
    ui->print(Progmem::getString(Progmem::imdEscGoBack)); 
    
    if (m_autoDoubleStep && requiredDoubleStep)
    {
      ui->print(Progmem::getString(Progmem::imdDoubleStep));
      BYTE key = toupper(ui->readKey("YN\e"));
      if (key == '\e')
      {
        ui->print(Progmem::getString(Progmem::uiNewLine));
        return false;
      }
      
      ui->print(Progmem::getString(Progmem::uiEchoKey), key);
      fdc->getParams()->DoubleStepping = (key == 'Y');
    }
        
    if (m_autoHeads && requiredHeads)
    {
      ui->print(Progmem::getString(Progmem::imdDiskSides));
      BYTE key = toupper(ui->readKey("12\e"));
      if (key == '\e')
      {
        ui->print(Progmem::getString(Progmem::uiNewLine));
        return false;
      }
      
      ui->print(Progmem::getString(Progmem::uiEchoKey), key);
      fdc->getParams()->Heads = key - 48;
    }
  }  
  
  return true;
}

void IMD::testDrive()
{
  ui->print("");
  ui->print(Progmem::getString(Progmem::imdInsertGoodDisk));
  ui->print(Progmem::getString(Progmem::uiContinueAbort));
  BYTE key = ui->readKey("\r\e");
  ui->print(Progmem::getString(Progmem::uiNewLine));
  if (key == '\e')
  {
    return;
  }
  ui->print("");
  
  // auto-detect FDC rate to adjust drive timings
  ui->print(Progmem::getString(Progmem::imdTestMedia));  
  if (!autodetectCommRate())
  {
    ui->print(Progmem::getString(fdc->wasErrorNoDiskInDrive() ? Progmem::errINTTimeout : Progmem::imdDiskReadFail));
    
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
    ui->print(Progmem::getString(Progmem::uiContinue));
    ui->readKey("\r");
    ui->print(Progmem::getString(Progmem::uiNewLine));
    return;
  }
  ui->print(Progmem::getString(Progmem::uiNewLine));
   
  // single or double step must be determined before the seek test
  if (m_autoDoubleStep && !autodetectDoubleStep())
  {
    if (!tryAskIfCannotAutodetect(true, false))
    {
      return;
    }
    ui->print("");
  }
  
  // seek fast to the last (specified) track and slowly back to 0, inspecting /TRK00 signal each step
  // number of steps back until the signal asserts must be equal to Tracks-1    
  ui->print(Progmem::getString(Progmem::imdTestDriveSeek));
  if (fdc->seekTest(fdc->getParams()->Tracks-1, 1))
  {
    ui->print(" ");
    ui->print(Progmem::getString(Progmem::uiOK));
  }
  else
  {
    ui->print(" ");
    ui->print(Progmem::getString(Progmem::uiFAIL));
  }
  fdc->seekDrive(0, 0);
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  // disk RPM test, try syncing with the 1st sector within 10s
  ui->print(Progmem::getString(Progmem::imdTestRPM));
  
  fdc->setAutomaticMotorOff(false);
  BYTE sector = 0;
  BYTE count = 0;
  DWORD lastTest = millis();  
  while (count || ((millis() - lastTest) < 10000))
  {
    if (!fdc->readSectorID(NULL, NULL, &sector))      
    {
      fdc->setAutomaticMotorOff();
      ui->print(Progmem::getString(fdc->wasErrorNoDiskInDrive() ? Progmem::errINTTimeout : Progmem::imdDiskReadFail));
      
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
      ui->print(Progmem::getString(Progmem::uiContinue));
      ui->readKey("\r");
      ui->print(Progmem::getString(Progmem::uiNewLine));
      return;
    }
       
    // in sync?
    if (sector == 1)
    {
      const DWORD now = millis();
      const WORD calValue = 60000-420; // subtract index signal width in a minute
      const WORD rpm = ((DWORD)count * calValue) / (now - lastTest);
      
      // quit
      key = ui->readKey("\e", false);
      if (key == '\e')
      {
        break;
      }      
      // begin testing
      else if (!count)
      {
        lastTest = now;
        ui->print(Progmem::getString(Progmem::imdEscGoBack));
      }      
      // re-average every 20 spins
      else if (count > 20)
      {
        lastTest = now;
        count = 0;        
      }      
      else
      {
        ui->print(Progmem::getString(Progmem::imdRPM), rpm);
      }

      count++;
    }
  }
  fdc->setAutomaticMotorOff();
  
  if (count)
  {
    ui->print(Progmem::getString(Progmem::uiNewLine));
    return;
  }
  
  ui->print(Progmem::getString(Progmem::imdSyncSectorFail));
  ui->print(Progmem::getString(Progmem::uiNewLine2x));
  ui->print(Progmem::getString(Progmem::uiContinue));
  ui->readKey("\r"); 
  ui->print(Progmem::getString(Progmem::uiNewLine));
}

void IMD::testController()
{ 
  ui->print("");
  ui->print(Progmem::getString(Progmem::imdInsertWriteDisk));
  ui->print(Progmem::getString(Progmem::imdNoteTestWrite));
  ui->print(Progmem::getString(Progmem::uiContinueAbort));
  BYTE key = ui->readKey("\r\e");
  ui->print(Progmem::getString(Progmem::uiNewLine));
  if (key == '\e')
  {
    return;
  }  
  
  // prepare to write
  ui->print("");
  fdc->seekDrive(0, 0);  
  ui->print(Progmem::getString(Progmem::imdTestController));  
  fdc->setSilentOnTrivialError(true);
  
  // make a backup of the original setup
  FDC::DiskDriveMediaParams backup(m_params);
    
  // format 15 SPT @ 500kbps on track 0, write sector 1 and read it
  fdc->getParams()->CommRate = 500;
  fdc->getParams()->FM = 0;
  fdc->setCommunicationRate();
  fdc->getParams()->SectorsPerTrack = 15;
  fdc->getParams()->SectorSizeBytes = 512;
  fdc->getParams()->LowLevelFormatFiller = 0xF6;
  autodetectGaps(fdc->getParams()->GapLength, fdc->getParams()->Gap3Length);
  
  ui->disableKeyboard(true);
  
  ui->print(Progmem::getString(Progmem::imdTestHD));
  bool result = false;
  if (fdc->formatTrack())
  {
    memset(&g_rwBuffer[0], 0xA1, fdc->getParams()->SectorSizeBytes);
    if (fdc->readWriteSectors(true, 1, 1))
    {
      memset(&g_rwBuffer[0], 0, fdc->getParams()->SectorSizeBytes);
      if (fdc->readWriteSectors(false, 1, 1) && (g_rwBuffer[fdc->getParams()->SectorSizeBytes-1] == 0xA1))
      {
        result = true;
      }
    }
  }
  // if the error was due to write protection or no disk in drive, don't go further
  if (!result && (fdc->wasErrorNoDiskInDrive() || fdc->wasErrorDiskProtected()))
  {
    ui->disableKeyboard(false);
    ui->print(Progmem::getString(fdc->wasErrorNoDiskInDrive() ? Progmem::uiNewLine2x : Progmem::uiNewLine));
    ui->print(Progmem::getString(Progmem::uiContinue));
    ui->readKey("\r");
    ui->print(Progmem::getString(Progmem::uiNewLine));
    fdc->setSilentOnTrivialError(false);
    
    // restore back what was configured
    m_params = backup;
    fdc->resetController();    
    return;
  }
  ui->print(Progmem::getString(result ? Progmem::uiOK : Progmem::uiFAIL));
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  // check if 1Mbps data rate, perpendicular recording mode and FIFO buffer are supported
  ui->print(Progmem::getString(Progmem::imdTestED));
  if ((fdc->getSpecialFeatures() & SUPPORT_1MBPS) && 
      (fdc->getSpecialFeatures() & SUPPORT_PERPENDICULAR))
  {
    // format 36 SPT @ 1Mbps (cannot autodetect this)
    fdc->getParams()->CommRate = 1000;
    fdc->getParams()->FM = 0;
    fdc->setCommunicationRate();
    fdc->getParams()->SectorsPerTrack = 36;
    fdc->getParams()->SectorSizeBytes = 512;
    fdc->getParams()->PerpendicularRecording = true;
    fdc->getParams()->GapLength = 0x1B;
    fdc->getParams()->Gap3Length = 0x53;
    
    result = false;
    if (fdc->formatTrack())
    {
      memset(&g_rwBuffer[0], 0xB2, fdc->getParams()->SectorSizeBytes);
      if (fdc->readWriteSectors(true, 1, 1))
      {
        memset(&g_rwBuffer[0], 0, fdc->getParams()->SectorSizeBytes);
        if (fdc->readWriteSectors(false, 1, 1) && (g_rwBuffer[fdc->getParams()->SectorSizeBytes-1] == 0xB2))
        {
          result = true;
        }
      }
    }
    
    // cancel this out here so I don't forget :)
    fdc->getParams()->PerpendicularRecording = false;
    
    if (!result && (fdc->wasErrorNoDiskInDrive() || fdc->wasErrorDiskProtected()))
    {
      ui->disableKeyboard(false);
      ui->print(Progmem::getString(fdc->wasErrorNoDiskInDrive() ? Progmem::uiNewLine2x : Progmem::uiNewLine));
      ui->print(Progmem::getString(Progmem::uiContinue));
      ui->readKey("\r");
      ui->print(Progmem::getString(Progmem::uiNewLine));
      fdc->setSilentOnTrivialError(false);
      
      m_params = backup;
      fdc->resetController();
      return;
    }
    ui->print(Progmem::getString(result ? Progmem::uiOK : Progmem::uiFAIL));
    ui->print(Progmem::getString(Progmem::uiNewLine));
  }
  else
  {
    ui->print(Progmem::getString(Progmem::imdTestSkipped));
    ui->print(Progmem::getString(Progmem::uiNewLine));
  }
  
  // 26 SPT @ 500kbps (data rate is 250kbps) FM
  fdc->getParams()->CommRate = 500;
  fdc->getParams()->FM = 1;
  fdc->setCommunicationRate();
  fdc->getParams()->SectorsPerTrack = 26;
  fdc->getParams()->SectorSizeBytes = 128;
  autodetectGaps(fdc->getParams()->GapLength, fdc->getParams()->Gap3Length);
  
  ui->print(Progmem::getString(Progmem::imdTest8inch));  
  result = false;
  if (fdc->formatTrack())
  {
    memset(&g_rwBuffer[0], 0xC3, fdc->getParams()->SectorSizeBytes);
    if (fdc->readWriteSectors(true, 1, 1))
    {
      memset(&g_rwBuffer[0], 0, fdc->getParams()->SectorSizeBytes);
      if (fdc->readWriteSectors(false, 1, 1) && (g_rwBuffer[fdc->getParams()->SectorSizeBytes-1] == 0xC3))
      {
        result = true;
      }
    }
  }
  if (!result && (fdc->wasErrorNoDiskInDrive() || fdc->wasErrorDiskProtected()))
  {
    ui->disableKeyboard(false);
    ui->print(Progmem::getString(fdc->wasErrorNoDiskInDrive() ? Progmem::uiNewLine2x : Progmem::uiNewLine));
    ui->print(Progmem::getString(Progmem::uiContinue));
    ui->readKey("\r");
    ui->print(Progmem::getString(Progmem::uiNewLine));
    fdc->setSilentOnTrivialError(false);
    
    m_params = backup;
    fdc->resetController();
    return;
  }
  ui->print(Progmem::getString(result ? Progmem::uiOK : Progmem::uiFAIL));
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  // same as above, just MFM  
  fdc->getParams()->FM = 0;
  fdc->setCommunicationRate();
  
  ui->print(Progmem::getString(Progmem::imdTestMFM128));  
  result = false;
  if (fdc->formatTrack())
  {
    memset(&g_rwBuffer[0], 0xD4, fdc->getParams()->SectorSizeBytes);
    if (fdc->readWriteSectors(true, 1, 1))
    {
      memset(&g_rwBuffer[0], 0, fdc->getParams()->SectorSizeBytes);
      if (fdc->readWriteSectors(false, 1, 1) && (g_rwBuffer[fdc->getParams()->SectorSizeBytes-1] == 0xD4))
      {
        result = true;
      }
    }
  }
  if (!result && (fdc->wasErrorNoDiskInDrive() || fdc->wasErrorDiskProtected()))
  {
    ui->disableKeyboard(false);
    ui->print(Progmem::getString(fdc->wasErrorNoDiskInDrive() ? Progmem::uiNewLine2x : Progmem::uiNewLine));
    ui->print(Progmem::getString(Progmem::uiContinue));
    ui->readKey("\r");
    ui->print(Progmem::getString(Progmem::uiNewLine));
    fdc->setSilentOnTrivialError(false);
    
    m_params = backup;
    fdc->resetController();
    return;
  }
  ui->print(Progmem::getString(result ? Progmem::uiOK : Progmem::uiFAIL));
    
  // done
  ui->disableKeyboard(false);
  ui->print(Progmem::getString(Progmem::uiNewLine2x));
  ui->print(Progmem::getString(Progmem::uiContinue));
  ui->readKey("\r");
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  fdc->setSilentOnTrivialError(false);
  m_params = backup;
  fdc->resetController();
}

void IMD::printGeometryInfo(BYTE track, BYTE head, BYTE interleave)
{
  ui->print(""); 
  
  // single/doublestep, max. tracks and sides
  ui->print(Progmem::getString(fdc->getParams()->DoubleStepping ? Progmem::imdDoubleStepping : Progmem::imdSingleStepping));
  ui->print(Progmem::getString(Progmem::imdGeoCylsHdStep), fdc->getParams()->Tracks, fdc->getParams()->Heads);
  if (fdc->getParams()->Heads > 1)
  {
    ui->print("s");
  }
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  // sectors per track on current side and track, sector gap length
  ui->print(Progmem::getString(Progmem::imdGeoChsSpt), track, head, fdc->getParams()->SectorsPerTrack,
            fdc->getParams()->SectorSizeBytes, fdc->getParams()->GapLength);
  
  // media data rate, encoding, format gap length
  ui->print(Progmem::getString(Progmem::imdGeoRateGap3), 
            fdc->getParams()->FM ? fdc->getParams()->CommRate / 2 : fdc->getParams()->CommRate, // media data rate halved in FM
            fdc->getParams()->FM ? "FM" : "MFM",
            fdc->getParams()->Gap3Length);
  
  // interleave and if gap sizes autodetected or provided
  ui->print(Progmem::getString(Progmem::imdInterleaveGaps), interleave);
  if (m_autoSectorGap || m_autoFormatGap)
  {
    ui->print(Progmem::getString(Progmem::imdGeoGapSizesAuto));
  }
  else
  {
    ui->print(Progmem::getString(Progmem::imdGeoGapSizesUser));
  }
  
  ui->print(Progmem::getString(Progmem::uiNewLine));
}

void IMD::eraseDisk()
{
  // double step and number of sides cannot be autodetected here
  if (!tryAskIfCannotAutodetect(true, true))
  {
    return;
  }
  
  ui->print("");
  ui->print(Progmem::getString(Progmem::imdInsertErase));
  ui->print(Progmem::getString(Progmem::uiContinueAbort));
  BYTE key = ui->readKey("\r\e");
  ui->print(Progmem::getString(Progmem::uiNewLine));
  if (key == '\e')
  {
    return;
  }
  
  // format with an 8K sector @ 250 kbps to overflow tracks on disk
  // fixed 1:1 interleave
  FDC::DiskDriveMediaParams backup(m_params);
  ui->disableKeyboard(true);
  
  fdc->getParams()->CommRate = 250;
  fdc->getParams()->FM = false;
  fdc->getParams()->SectorSizeBytes = 8192;
  fdc->getParams()->SectorsPerTrack = 1;
  autodetectGaps(fdc->getParams()->GapLength, fdc->getParams()->Gap3Length);  
  fdc->setCommunicationRate();
  printGeometryInfo(0, 0, 1);
  
  BYTE head = 0;
  BYTE track = 0;  
  while(track < fdc->getParams()->Tracks)
  {
    ui->print(Progmem::getString(Progmem::imdProgress), track, head);
    fdc->seekDrive(track, head);    
    fdc->formatTrack();
    
    const bool formatBreakError = fdc->getLastError() ? fdc->wasErrorNoDiskInDrive() || fdc->wasErrorDiskProtected() : false;
    if (formatBreakError)
    {
      break;
    }  
       
    // the other head
    head++;    
    if (head < fdc->getParams()->Heads)
    {
      continue;
    }
    
    head = 0;
    track++;
  }
  
  ui->print(Progmem::getString(fdc->wasErrorNoDiskInDrive() ? Progmem::uiNewLine2x : Progmem::uiNewLine));
  fdc->seekDrive(0, 0);  
  ui->disableKeyboard(false);
  ui->print(Progmem::getString(Progmem::uiContinue));
  ui->readKey("\r");
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  m_params = backup;
}

void IMD::formatDisk()
{
  FDC::DiskDriveMediaParams backup(m_params);
  
  // skip through all this if wished and had gone through before
  static FDC::DiskDriveMediaParams previousFormatParams;
  if (previousFormatParams.DriveInches) // initialized
  {
    ui->print("");
    ui->print(Progmem::getString(Progmem::imdUseSameParams));
    BYTE choose = toupper(ui->readKey("YN"));
    ui->print(Progmem::getString(Progmem::uiEchoKey), choose);
    if (choose == 'Y')
    {
      m_params = previousFormatParams;
      goto beginFormat; 
    }
  }
  
  // as we don't check the previous format of the disk
  if (!tryAskIfCannotAutodetect(true, true))
  {
    return;
  }
  
  // format parameters
  ui->print("");
  ui->print(Progmem::getString(Progmem::imdFormatParams));
  
  BYTE spt = 0;
  BYTE* allowedKeys = &g_rwBuffer[0];
  allowedKeys[0] = 0;
  strcpy(allowedKeys, Progmem::getString(Progmem::uiDecimalInput));
  strcat(allowedKeys, "\e");
  
  // sectors per track
  while ((spt == 0) || (spt > 63))
  {
    ui->print(Progmem::getString(Progmem::uiDeleteLine));
    ui->print(Progmem::getString(Progmem::imdFormatSpt));        
    
    const BYTE* prompt = ui->prompt(2, allowedKeys, true);
    if (!prompt)
    {
      ui->print(Progmem::getString(Progmem::uiNewLine));
      return;
    }    
    spt = (BYTE)atoi(prompt);
  }
  fdc->getParams()->SectorsPerTrack = spt;
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  // data encoding
  ui->print(Progmem::getString(Progmem::imdFormatEncoding));
  BYTE key = toupper(ui->readKey("MF\e"));
  if (key == '\e')
  {
    ui->print(Progmem::getString(Progmem::uiNewLine));
    m_params = backup;
    return;
  }  
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  fdc->getParams()->FM = (key == 'F');
  
  // data rate
  ui->print(Progmem::getString(Progmem::imdFormatRate));
  if (fdc->getParams()->FM)
  {
    ui->print(Progmem::getString(Progmem::imdFormatRatesFM));
    key = toupper(ui->readKey("152\e"));
    if (key == '\e')
    {
      ui->print(Progmem::getString(Progmem::uiNewLine));
      m_params = backup;
      return;
    }
    
    // displayed media data rates for FM are half the FDC communication rates
    switch(key)
    {
    case '1':
      fdc->getParams()->CommRate = 250;
      break;
    case '5':
      fdc->getParams()->CommRate = 300;
      break;
    case '2':
      fdc->getParams()->CommRate = 500;
    }
  }
  else
  {
    ui->print(Progmem::getString(Progmem::imdFormatRatesMFM));
    key = toupper(ui->readKey("235\e"));
    if (key == '\e')
    {
      ui->print(Progmem::getString(Progmem::uiNewLine));
      m_params = backup;
      return;
    }
    
    switch(key)
    {
    case '2':
      fdc->getParams()->CommRate = 250;
      break;
    case '3':
      fdc->getParams()->CommRate = 300;
      break;
    case '5':
      fdc->getParams()->CommRate = 500;
    }
  }
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  
  // sector size
  ui->print(Progmem::getString(Progmem::imdFormatSecSize1));
  ui->print(Progmem::getString(Progmem::imdFormatSecSize2));
  ui->print(Progmem::getString(Progmem::imdFormatSecSize3));
  key = toupper(ui->readKey("125KLMN\e"));
  if (key == '\e')
  {
    ui->print(Progmem::getString(Progmem::uiNewLine));
    m_params = backup;
    return;
  }
  switch(key)
  {
  case '1':
    fdc->getParams()->SectorSizeBytes = 128;
    break;
  case '2':
    fdc->getParams()->SectorSizeBytes = 256;
    break;
  case '5':
    fdc->getParams()->SectorSizeBytes = 512;
    break;
  case 'K':
    fdc->getParams()->SectorSizeBytes = 1024;
    break;
  case 'L':
    fdc->getParams()->SectorSizeBytes = 2048;
    break;
  case 'M':
    fdc->getParams()->SectorSizeBytes = 4096;
    break;
  case 'N':
    fdc->getParams()->SectorSizeBytes = 8192;
  }
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  
  // use default format filler, 0xF6 for MFM disks, 0xE5 for FM (SD)
  fdc->getParams()->LowLevelFormatFiller = fdc->getParams()->FM ? 0xE5 : 0xF6;
  
  // use supplied gaps or autodetect and overwrite
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
  
  // format interleave
  while(true)
  {
    ui->print(Progmem::getString(Progmem::imdInterleave));
    BYTE interleave = (WORD)atoi(ui->prompt(2, Progmem::getString(Progmem::uiDecimalInput)));
    if (interleave > 0)
    {
      m_formatInterleave = interleave;
      ui->print(Progmem::getString(Progmem::uiNewLine));
      break;
    }
    
    ui->print(Progmem::getString(Progmem::uiDeleteLine));
  }
    
  // all questions answered
  previousFormatParams = m_params;  
  
  // begin format
beginFormat:
  ui->print(Progmem::getString(Progmem::uiNewLine));
  ui->print(Progmem::getString(Progmem::imdInsertFormat));
  ui->print(Progmem::getString(Progmem::imdNoteNoFS));  
  ui->print(Progmem::getString(Progmem::uiContinueAbort));
  key = ui->readKey("\r\e");
  ui->print(Progmem::getString(Progmem::uiNewLine));
  if (key == '\e')
  {
    m_params = backup;
    return;
  }
  
  ui->disableKeyboard(true);  
  fdc->setCommunicationRate();  
  printGeometryInfo(0, 0, m_formatInterleave);
  
  BYTE head = 0;
  BYTE track = 0;
  BYTE sector = 1;
  WORD badSectorCount = 0;
  bool breakError = false;
  
  while(track < fdc->getParams()->Tracks)
  {
    ui->print(Progmem::getString(Progmem::imdProgress), track, head);      
    
    fdc->seekDrive(track, head);    
    fdc->formatTrack(false, m_formatInterleave);

    bool formatError = fdc->getLastError();
    breakError = formatError ? fdc->wasErrorNoDiskInDrive() || fdc->wasErrorDiskProtected() : false;    
    if (breakError)
    {
      break;
    }
    else if (formatError)
    {
      // the whole track failed to format before verifying individual sectors?
      badSectorCount += fdc->getParams()->SectorsPerTrack;
    }
    
    // format OK, now verify
    else
    {
      while (sector <= fdc->getParams()->SectorsPerTrack)
      {
        const WORD successfulBytesRead = fdc->verify(sector, false);                
        
        if (fdc->wasErrorNoDiskInDrive())
        {
          breakError = true;
          break;
        }        
        // generic IO error
        else if (fdc->getLastError() || (successfulBytesRead != fdc->getParams()->SectorSizeBytes))
        {
          badSectorCount++;
        }
        
        sector++;
      }
    }
    sector = 1;
    
    // break while verify?
    if (breakError)
    {
      break;
    }
      
    head++;    
    if (head < fdc->getParams()->Heads)
    {
      continue;
    }
    
    head = 0;
    track++;
  }
  
  ui->print(Progmem::getString(fdc->wasErrorNoDiskInDrive() ? Progmem::uiNewLine2x : Progmem::uiNewLine));
  fdc->seekDrive(0, 0);
  ui->disableKeyboard(false);
  
  // we're done
  if (!breakError)
  {
    ui->print(Progmem::getString(Progmem::imdBadSectorsDisk), badSectorCount);
  }
  ui->print(Progmem::getString(Progmem::uiContinue));
  ui->readKey("\r");
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  m_params = backup;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void IMD::cleanupCallback()
{
  // cleans up and reinitializes default values for variables shared between the two (read and write) XMODEM callback functions
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
  if (m_cbSectorsTable)
  {
    delete[] m_cbSectorsTable;
    m_cbSectorsTable = NULL;
  }
  
  // all of these flags before the actual sector data follow
  // to know where we left off when the last data packet ended, and the callback returned
  m_cbModeSpecified        = false;
  m_cbTrackSpecified       = false;
  m_cbHeadSpecified        = false;
  m_cbSptSpecified         = false;
  m_cbSecSizeSpecified     = false;
  m_cbSecMapSpecified      = false;
  m_cbSecTrackMapSpecified = false;
  m_cbSecHeadMapSpecified  = false;
  m_cbSecDataTypeSpecified = false;
  m_cbProcessingHeader     = true;
  
  // even zeros are valid so we need to know what was specified and what not yet
  m_cbMode                 = (BYTE)-1;
  m_cbTrack                = (BYTE)-1;
  m_cbHead                 = (BYTE)-1;
  m_cbSpt                  = (BYTE)-1;
  m_cbSecSize              = (BYTE)-1;
  
  // optional for non-standard disks
  m_cbHasSecTrackMap       = false;
  m_cbHasSecHeadMap        = false;
  
  m_cbSecSizeBytes         = 0;         // 128 << secSize
  m_cbInterleave           = 1;         // computed from the .IMD sectors map or read from disk via autodetectInterleave
  m_cbGeometryChanged      = true;      // if mode, spt, secSize or interleave changed
  m_cbSeekIndicated        = true;      // indicate seek
  m_cbLastPos              = 0;         // position in buffer and maps
  m_cbSectorDataType       = 0;         // 1 byte right before data
  m_cbSectorIdx            = 0;         // array index of the current sector in the numberings maps
  m_cbStartingSectorIdx    = (BYTE)-1;  // index of the first starting sector (0 is valid), default: undefined
}

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
  
  FDC::DiskDriveMediaParams backup(m_params);  
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
  cleanupCallback();
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
  
  m_params = backup;
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
    
    if (!m_cbTrackSpecified)
    {
      m_cbTrack = data[packetIdx];
      
      // more tracks than configured?
      if (m_cbTrack >= fdc->getParams()->Tracks)
      {
        m_cbSuccess = false;
        snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdXmodemErrTrack));
        return false;
      }
      
      packetIdx++;
      m_cbTrackSpecified = true;
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
      m_cbTrackSpecified = false;
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
        // find starting sector; might not start from one
        BYTE startingSector = 1;
        BYTE idx = 0;
        
        for (;;)
        {
          for (idx = 0; idx < m_cbSpt; idx++)
          {
            if (m_cbSectorNumberingMap[idx] == startingSector)
            {
              break;
            }
          }
          
          if (m_cbSectorNumberingMap[idx] != startingSector)
          {
            startingSector++;
            if (startingSector > m_cbSpt)
            {
              startingSector = 1;
              idx = 0;
              break; // we tried
            }  
          }
          else
          {
            break; // found
          }
        }
        
        // find interleave factor
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
    
    // seek to physical track and head
    if (!skipOperation && ((fdc->getCurrentTrack() != m_cbTrack) || (fdc->getCurrentHead() != m_cbHead)))
    {
      fdc->seekDrive(m_cbTrack, m_cbHead);
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
        
        printGeometryInfo(m_cbTrack, m_cbHead, m_cbInterleave);
      }
    }
    
    if (!skipOperation && m_cbSeekIndicated)
    { 
      // print progress on LCD
      ui->print(Progmem::getString(Progmem::imdProgress), m_cbTrack, m_cbHead);
      
      // prepare CHSV table for formatting, we might have custom values for the track and head
      memset(&g_rwBuffer[0], 0, m_cbSpt * 4);
      BYTE sectorIdx = 0;
      m_cbLastPos = 0;
      while (sectorIdx < m_cbSpt)
      {
        const BYTE logicalSector = m_cbSectorNumberingMap[sectorIdx];
        const BYTE logicalTrack = m_cbHasSecTrackMap ? m_cbSectorTrackMap[sectorIdx] : m_cbTrack;
        const BYTE logicalHead = m_cbHasSecHeadMap ? m_cbSectorHeadMap[sectorIdx] : m_cbHead;
        
        g_rwBuffer[m_cbLastPos++] = logicalTrack;
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
      const BYTE logicalTrack = m_cbHasSecTrackMap ? m_cbSectorTrackMap[m_cbSectorIdx] : m_cbTrack;
      const BYTE logicalHead = m_cbHasSecHeadMap ? m_cbSectorHeadMap[m_cbSectorIdx] : m_cbHead;    
      
      // normal data with DAM, compressed data with DAM, deleted data with read error, compressed deleted with read error
      const bool deletedDataMark = (m_cbSectorDataType == 3) || (m_cbSectorDataType == 4) || (m_cbSectorDataType == 7) || (m_cbSectorDataType == 8);
      
      // finally, write
      bool result = fdc->readWriteSectors(true, logicalSector, logicalSector, NULL, deletedDataMark, &logicalTrack, &logicalHead);
      
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
        result = fdc->verify(logicalSector, false, &logicalTrack, &logicalHead);
        
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
      m_cbTrackSpecified = false;
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
  FDC::DiskDriveMediaParams backup(m_params);  
  
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
      
      m_params = backup;
      return;
    }
  }
  
  // could not either of them, or both?
  if (!tryAskIfCannotAutodetect(askDoubleStep, askHeads))
  {
    m_params = backup;
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
  cleanupCallback();
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
    ui->print(Progmem::getString(Progmem::imdRunPython));
  }
  
  ui->print(Progmem::getString(Progmem::uiNewLine));
  ui->print(Progmem::getString(Progmem::uiContinue));
  ui->disableKeyboard(false);
  ui->readKey("\r");  
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  m_params = backup;
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
  if (m_cbTrack == fdc->getParams()->Tracks)
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
    
    if (!m_cbTrackSpecified)
    {
      m_cbTrack = fdc->getCurrentTrack();
      data[packetIdx++] = m_cbTrack;
      
      m_cbTrackSpecified = true;
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
          if (m_cbSectorsTable[idx])
          {
            if (!m_cbHasSecTrackMap)
            {
              // sector ID track number differs from physical?
              const BYTE track = (BYTE)(m_cbSectorsTable[idx] >> 8);
              if (track != m_cbTrack)
              {
                m_cbHasSecTrackMap = true;
              }              
            }
            if (!m_cbHasSecHeadMap)
            {
              // sector ID head number differs from physical?
              const BYTE head = ((BYTE)m_cbSectorsTable[idx]) >> 7;
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
      ui->print(Progmem::getString(Progmem::imdProgress), m_cbTrack, m_cbHead);
      ui->print(Progmem::getString(Progmem::imdTrackUnreadable));
      m_cbUnreadableTracks++;
      
      // re-specify
      m_cbModeSpecified = false;
      m_cbTrackSpecified = false;
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
        m_cbTrack++;
      }
      if (m_cbTrack == fdc->getParams()->Tracks)
      {
        m_cbSuccess = true;
        m_cbResponseStr[0] = 0;
        return true; // flush the buffer
      }
      
      fdc->seekDrive(m_cbTrack, m_cbHead);      
      continue; // transfer continues
    }
    
    // next in the list is the mandatory sector map
    if (!m_cbSecMapSpecified)
    {
      // as the interleave table almost always never starts from the beginning, find the starting sector
      // but even the starting sector number might not start from 1...
      BYTE startingSector = 1;
      
      while (m_cbStartingSectorIdx == (BYTE)-1) // undefined
      {
        for (BYTE idx = 0; idx < SECTORS_TABLE_COUNT; idx++)
        {
          if (!m_cbSectorsTable[idx]) // entry not defined
          {
            continue;
          }
          
          // last 6 bytes (1-63)
          if ((((BYTE)m_cbSectorsTable[idx]) & 0x3F) == startingSector)
          {
            m_cbStartingSectorIdx = idx;
            m_cbSectorIdx = m_cbStartingSectorIdx;
            m_cbLastPos = 0; // use this as count how many were written in the map
            break;
          }
        }
        
        startingSector++;  // starts from 2 or whatever
        if (startingSector > SECTORS_TABLE_COUNT) // cannot sync
        {
          m_cbSuccess = false;
          snprintf(m_cbResponseStr, sizeof(m_cbResponseStr), Progmem::getString(Progmem::imdSyncSectorFail));
          return false;
        }
      }
      
      // now write the sector numbering map
      while ((m_cbLastPos < m_cbSpt) && (m_cbSectorIdx < SECTORS_TABLE_COUNT))
      {
        data[packetIdx++] = (((BYTE)m_cbSectorsTable[m_cbSectorIdx++]) & 0x3F); // sector info only
        m_cbLastPos++;
        CHECK_STREAM_END;
      }
      
      // we still need to go from the beginning of the table?
      if ((m_cbSectorIdx == SECTORS_TABLE_COUNT) && (m_cbLastPos < m_cbSpt))
      {
        m_cbSectorIdx = 0;
        continue;
      }
      
      // reuse m_cbStartingSectorIdx for the rest of the maps
      m_cbSectorIdx = m_cbStartingSectorIdx;
      m_cbLastPos = 0;
      m_cbSecMapSpecified = true;
    }
    
    if (m_cbHasSecTrackMap && !m_cbSecTrackMapSpecified)
    {
      // write sector track numbering analog to above
      while ((m_cbLastPos < m_cbSpt) && (m_cbSectorIdx < SECTORS_TABLE_COUNT))
      {
        data[packetIdx++] = (BYTE)(m_cbSectorsTable[m_cbSectorIdx++] >> 8); // track info only
        m_cbLastPos++;
        CHECK_STREAM_END;
      }
      if ((m_cbSectorIdx == SECTORS_TABLE_COUNT) && (m_cbLastPos < m_cbSpt))
      {
        m_cbSectorIdx = 0;
        continue;
      }
      
      m_cbSectorIdx = m_cbStartingSectorIdx;
      m_cbLastPos = 0;      
      m_cbSecTrackMapSpecified = true;
    }
    
    if (m_cbHasSecHeadMap && !m_cbSecHeadMapSpecified)
    {
      while ((m_cbLastPos < m_cbSpt) && (m_cbSectorIdx < SECTORS_TABLE_COUNT))
      {
        data[packetIdx++] = (((BYTE)m_cbSectorsTable[m_cbSectorIdx++]) >> 7); // head info only
        m_cbLastPos++;
        CHECK_STREAM_END;
      }
      if ((m_cbSectorIdx == SECTORS_TABLE_COUNT) && (m_cbLastPos < m_cbSpt))
      {
        m_cbSectorIdx = 0;
        continue;
      }
      
      m_cbSectorIdx = m_cbStartingSectorIdx;
      m_cbLastPos = 0;      
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
      
      printGeometryInfo(m_cbTrack, m_cbHead, m_cbInterleave);
      ui->print(Progmem::getString(Progmem::imdProgress), m_cbTrack, m_cbHead);
      m_cbGeometryChanged = false; // reset the flag
    }
    
    // now try to read    
    while ((m_cbLastPos < m_cbSpt) && (m_cbSectorIdx < SECTORS_TABLE_COUNT))
    {     
      static WORD rwBufferPos = 0;
      
      if (!m_cbSecDataTypeSpecified) // determine data record type
      {
        rwBufferPos = 0;
        
        const BYTE logicalSector = (((BYTE)m_cbSectorsTable[m_cbSectorIdx]) & 0x3F);
        const BYTE logicalTrack = m_cbHasSecTrackMap ? (BYTE)(m_cbSectorsTable[m_cbSectorIdx] >> 8) : m_cbTrack;
        const BYTE logicalHead = m_cbHasSecHeadMap ? (((BYTE)m_cbSectorsTable[m_cbSectorIdx]) >> 7) : m_cbHead;
        memset(&g_rwBuffer[0], 0, fdc->getParams()->SectorSizeBytes);
        
        fdc->readWriteSectors(false, logicalSector, logicalSector, NULL, false, &logicalTrack, &logicalHead);        
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
      m_cbSectorIdx = 0;
      continue;
    }
       
    // end of track?
    m_cbSuccess = true;
    m_cbResponseStr[0] = 0;

    // re-specify
    m_cbModeSpecified = false;
    m_cbTrackSpecified = false;
    m_cbHeadSpecified = false;
    m_cbSptSpecified = false;
    m_cbSecSizeSpecified = false;
    m_cbSecMapSpecified = false;
    m_cbSecTrackMapSpecified = false;
    m_cbSecHeadMapSpecified = false;
    m_cbSecDataTypeSpecified = false;
    m_cbLastPos = 0;
    m_cbSectorIdx = 0;
    m_cbStartingSectorIdx = (BYTE)-1;
    
    // and seek to next
    m_cbHead++;
    if (m_cbHead == fdc->getParams()->Heads)
    {
      m_cbHead = 0;
      m_cbTrack++;
    }
    if (m_cbTrack == fdc->getParams()->Tracks)
    {
      m_cbSuccess = true;
      m_cbResponseStr[0] = 0;
      return true; // flush the buffer
    }

    ui->print(Progmem::getString(Progmem::imdProgress), m_cbTrack, m_cbHead);
    fdc->seekDrive(m_cbTrack, m_cbHead);  
  }
  
  return false;
}

#endif