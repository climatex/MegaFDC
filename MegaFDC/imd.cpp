// MegaFDC (c) 2023-2026 J. Bogin, http://boginjr.com
// IMD mode

#include "config.h"

#ifdef BUILD_IMD_IMAGER

IMD imd;

// 1 disk drive configuration
FDC::DiskDriveMediaParams* g_diskDrives = NULL;
BYTE g_numberOfDrives = 0;

// forward decls
void SetupDrive();

// similar to commands.cpp, but initialize 1 drive here
void InitializeDrives()
{
  ui->print(Progmem::getString(Progmem::uiDeleteLine));
  ui->print("");  
  ui->print(Progmem::getString(Progmem::uiSplash));
  ui->print(Progmem::getString(Progmem::imdSplash));
  ui->print(Progmem::getString(Progmem::uiBuild));
  
#ifdef SD_CARD_SUPPORT
  ui->print(Progmem::getString(Progmem::uiSdCardBuild));
#else // serial file transfers

  // recommend display output to see current disk geometry/bad sectors/unreadable tracks during serial transfers
  // without an LCD, only a string response is written at the end, when XMODEM finishes and serial becomes available
  if (!g_uiEnabled)
  {
    ui->print(Progmem::getString(Progmem::imdLCDRecommended));  
  }
#endif
  
  // no keyboard present
  if (g_uiEnabled == UI_LCD)
  {
    ui->print(Progmem::getString(Progmem::uiNoKeyboard));
  }
    
  // check if the floppy controller is responding to a reset command
  ui->print(Progmem::getString(Progmem::uiInitializingFDC));
  fdc->resetController();
  ui->print(" ");
  ui->print(Progmem::getString(Progmem::uiOK));
  ui->print(Progmem::getString(Progmem::uiNewLine2x));
  
  // check if settings can be loaded
  if (!eepromIsConfigurationPresent())
  {
    SetupDrive();
    return;
  }
  
  // auto load the configuration, give a small delay to break the process with Esc
  BYTE key = 0;
  bool autoLoad = true;
  ui->print(Progmem::getString(Progmem::uiLoadingConfig));
  ui->print(Progmem::getString(Progmem::uiAbort));
  
  DWORD timeBefore = millis();
  while (millis() - timeBefore < 2500)
  {
    key = ui->readKey("\e", false); // without wait
    if (key == '\e')
    {
      autoLoad = false;
      break;
    }
  }
  
  ui->print(Progmem::getString(Progmem::uiDeleteLine));
  if (!autoLoad)
  {
    ui->print(Progmem::getString(Progmem::uiLoadingAborted));
    ui->print(Progmem::getString(Progmem::uiNewLine));
    
    // aborted; before respecifying, give option to remove them from EEPROM
    ui->print(Progmem::getString(Progmem::uiSetupAskRemove));
    key = toupper(ui->readKey("YN"));
    ui->print(Progmem::getString(Progmem::uiEchoKey), key);
    if (key == 'Y')
    {
      eepromClearConfiguration();
    }
    ui->print(Progmem::getString(Progmem::uiNewLine));
    
    SetupDrive();
    return;
  }  
  
  // initialize configured drive, call internal setters
  eepromLoadConfiguration();
  FDC::DiskDriveMediaParams* drive = &g_diskDrives[0];
  
  imd.setAutodetectHeads(drive->Heads == 2);      // number of heads stored 1 would force single sided operation, 2: autodetect  
  imd.setAutodetectSectorGap(!drive->GapLength);  // autodetect if gap size was stored 0
  imd.setAutodetectFormatGap(!drive->Gap3Length); // dtto for format gap
  imd.setAutodetectDoubleStep(drive->UseFAT12);   // FAT12 filesystem unused in IMD mode, flag reused as autodetect doublestepping
  
  // use the slowest data rate for now
  drive->CommRate = 250;
  fdc->setActiveDrive(drive);  
}

void SetupDrive()
{  
  // setup parameters: drive letter, type, tracks, - optional: double stepping, sides, sector and format gaps
  g_numberOfDrives = 1;
  g_diskDrives = new FDC::DiskDriveMediaParams[g_numberOfDrives];
  FDC::DiskDriveMediaParams* drive = &g_diskDrives[0];
  memset(drive, 0, sizeof(FDC::DiskDriveMediaParams)); // initialize with all 0s (the struct is POD)
  
  // use the slowest datarate for now
  drive->CommRate = 250;
  
  ui->print(Progmem::getString(Progmem::imdDriveLetter));
  BYTE key = toupper(ui->readKey("ABCD"));
  drive->DriveNumber = key - 65;  
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  
  ui->print(Progmem::getString(Progmem::imdDriveType));
  key = ui->readKey("853");
  drive->DriveInches = key - 48;
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  
  while(true)
  {
    ui->print(Progmem::getString(Progmem::imdDriveCyls));
    WORD cylinders = (WORD)atoi(ui->prompt(3, Progmem::getString(Progmem::uiDecimalInput)));
    if ((cylinders > 0) && (cylinders <= 100))
    {
      drive->Cylinders = (BYTE)cylinders;
      ui->print(Progmem::getString(Progmem::uiNewLine));
      break;
    }
    
    ui->print(Progmem::getString(Progmem::uiDeleteLine));
  }
  
  // autodetect doublestepping?
  ui->print(Progmem::getString(Progmem::imdDoubleStepAuto));
  key = toupper(ui->readKey("YNG"));
  const bool guess = (key == 'G');
  drive->DoubleStepping = (key == 'Y');
  imd.setAutodetectDoubleStep(guess);
  drive->UseFAT12 = guess; // reuse "FAT12" flag as autodetect doublestep
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  
  // autodetect number of heads?
  ui->print(Progmem::getString(Progmem::imdDiskSidesAuto));
  key = toupper(ui->readKey("YN"));
  if (key == 'Y')
  {
    drive->Heads = 1;
  }
  else
  {
    drive->Heads = 2;
    imd.setAutodetectHeads(true);
  }  
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
    
  ui->print(Progmem::getString(Progmem::imdSectorGap));
  drive->GapLength = (BYTE)strtoul(ui->prompt(2, Progmem::getString(Progmem::uiHexadecimalInput)), 0, 16);
  if (!drive->GapLength)
  {
    imd.setAutodetectSectorGap(true);    
    if (!strlen(ui->getPromptBuffer()))
    {
      ui->print("0");    
    }
  }
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  ui->print(Progmem::getString(Progmem::imdFormatGap));
  drive->Gap3Length = (BYTE)strtoul(ui->prompt(2, Progmem::getString(Progmem::uiHexadecimalInput)), 0, 16);
  if (!drive->Gap3Length)
  {
    imd.setAutodetectFormatGap(true);    
    if (!strlen(ui->getPromptBuffer()))
    {
      ui->print("0");    
    }
  }
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  // save settings?
  ui->print(Progmem::getString(Progmem::uiSetupAskSave));
  key = toupper(ui->readKey("YN"));
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  if (key == 'Y')
  {
    ui->print(Progmem::getString(Progmem::uiOperationPending));
        
    eepromStoreConfiguration();
        
    ui->print(Progmem::getString(Progmem::uiOK));
    ui->print(Progmem::getString(Progmem::uiNewLine));
  }
  
  // initialize configured drive
  fdc->setActiveDrive(drive);
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
  
  // format interleave sequential by default, start sector at 1
  m_formatInterleave = 1;
  m_formatStartSector = 1;
  
  m_cbInterleave = 1;
  m_cbSectorsTable = NULL;
  
#ifndef SD_CARD_SUPPORT 
  // IMD serial file transfer
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
  cleanup();
#endif
}

bool IMD::autodetectCommRate()
{ 
  // autodetect what FDC communication rate to use, from the current physical cyl/head
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
  const BYTE saveCylinder = fdc->getCurrentCylinder();
  const BYTE saveHead = fdc->getCurrentHead();
  
  // go to track 0 and assume single step first    
  fdc->getParams()->DoubleStepping = false;
  fdc->recalibrateDrive();
  fdc->seekDrive(0, saveHead);
  
  // find sector 1 on track 0 within some 2.5 sec.
  BYTE currCylinder = (BYTE)-1;
  BYTE currSector = (BYTE)-1;
  DWORD timeStart = millis();
  while ((currSector != 1) && ((millis() - timeStart) < 2500))
  {
    if (!fdc->readSectorID(&currCylinder, NULL, &currSector))
    {
      if (fdc->wasErrorNoDiskInDrive())
      {
        fdc->seekDrive(saveCylinder, saveHead);
        return false;
      }
    }
  }
  if ((currSector != 1) || (currCylinder != 0))
  {
    fdc->seekDrive(saveCylinder, saveHead);
    return false;
  }
  
  // now advance to cylinder 2 and do the same
  fdc->seekDrive(2, saveHead);
  currSector = (BYTE)-1;
  currCylinder = (BYTE)-1;
  timeStart = millis();
  while ((currSector != 1) && ((millis() - timeStart) < 2500))
  {
    if (!fdc->readSectorID(&currCylinder, NULL, &currSector))
    {
      if (fdc->wasErrorNoDiskInDrive())
      {
        fdc->seekDrive(saveCylinder, saveHead);
        return false;
      }
    }
  }
  
  if ((currSector == 1) && (currCylinder == 2))
  {
    // auto-detected single stepping
    fdc->seekDrive(saveCylinder, saveHead);
    return true;
  }
  else if ((currSector == 1) && (currCylinder == 1))
  {
    // auto-detected double stepping
    fdc->getParams()->DoubleStepping = true;
    fdc->recalibrateDrive();
    fdc->seekDrive(saveCylinder, saveHead);
    return true;
  }
  
  fdc->recalibrateDrive();
  fdc->seekDrive(saveCylinder, saveHead);
  return false; // we tried, didn't we
}

bool IMD::autodetectHeads()
{
  // analog to the one above, except that the sector number is ignored
  // side 0 shall be commrate detected
  const BYTE saveCylinder = fdc->getCurrentCylinder();
  const BYTE saveHead = fdc->getCurrentHead();
  
  // find any sector on current cyl of side 0
  fdc->seekDrive(saveCylinder, 0);
  BYTE currHead = (BYTE)-1;
  DWORD timeStart = millis();
  while ((millis() - timeStart) < 2500)
  {
    if (!fdc->readSectorID(NULL, &currHead))
    {
      if (fdc->wasErrorNoDiskInDrive())
      {
        fdc->seekDrive(saveCylinder, saveHead);
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
    fdc->seekDrive(saveCylinder, saveHead);
    return false;
  }
  
  // now check if currHead on side 1 is also 1, then the disk contains a double sided recording
  fdc->seekDrive(saveCylinder, 1);
  currHead = (BYTE)-1;

  // different data rate on second side?
  FDC::DiskDriveMediaParams backup;
  memcpy(&backup, fdc->getParams(), sizeof(FDC::DiskDriveMediaParams));
  if (autodetectCommRate()) 
  {
    timeStart = millis();
    while ((millis() - timeStart) < 2500)
    {
      if (!fdc->readSectorID(NULL, &currHead))
      {
        if (fdc->wasErrorNoDiskInDrive())
        {
          fdc->seekDrive(saveCylinder, saveHead);
          return false;
        }
      }
      else
      {
        break;
      }
    }  
  }  
  memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
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
  
  fdc->seekDrive(saveCylinder, saveHead);
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
  memset(sectorsTable, 0xFF, SECTORS_TABLE_COUNT*sizeof(WORD));
  
  // create an array of sectors sequence
  // empty/invalid entries: 0xFFFF ("127 cyl, head 1, sector 255")
  BYTE idx = 0;
  BYTE computeSPT = 0;
  bool compute = false;
  bool sectorZeroObserved = false;
  DWORD timeStart = millis();
  
  // 5sec should give at least 25 complete disk revolutions @ 300RPM
  while ((millis() - timeStart) < 5000)
  {
    BYTE cyl = 0;
    BYTE head = 0;        
    BYTE sector = 0;
    if (fdc->readSectorID(&cyl, &head, &sector))
    {
      sectorZeroObserved |= !sector;
      
      if (idx == 0)
      {
        // count number of times until we see the same sector again: that's the observed SPT
        computeSPT = sector;
        compute = true;
      }
      else if (computeSPT == sector)
      {
        compute = false; // stop counting
      }
      if (compute)
      {
        observedSPT++;
      }
      
      // the maximum logical sector value seen
      if (sector > maximumSPT)
      {
        maximumSPT = sector;
      }
      
      // each item is 16-bit
      // upper 8 bits: bit7: head number, bits6-0: logical cylinder number
      // lower 8 bits: logical sector number
      sectorsTable[idx++] = (((WORD)head & 1) << 15) | (((WORD)cyl & 0x7F) << 8) | sector;
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
  
  // account for logical sector number 0
  if (sectorZeroObserved)
  {
    maximumSPT++;
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
    fdc->seekDrive(fdc->getCurrentCylinder(), fdc->getCurrentHead());
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
  
  fdc->getParams()->SectorsPerTrack = sectorsPerTrack; // store valid SPT
  
  BYTE startSector = (BYTE)-1;
  BYTE idx2 = 0;
  idx = 0;
  
  // find the lowest logical sector number and its index in the sectors table  
  while (idx < SECTORS_TABLE_COUNT)
  {
    if (m_cbSectorsTable[idx] != 0xFFFF) // undefined
    {
      if ((BYTE)(m_cbSectorsTable[idx]) < startSector)
      {
        startSector = (BYTE)(m_cbSectorsTable[idx]);
        idx2 = idx;
      }
    }    
    idx++;
  }
  
  // next sector
  idx = idx2 + 1;
  BYTE nextSector = 0;
  while (idx < SECTORS_TABLE_COUNT)
  {
    if (m_cbSectorsTable[idx] == 0xFFFF)
    {
      idx++;
      continue;
    }
    
    nextSector = (BYTE)(m_cbSectorsTable[idx]);
    break;
  }
  
  if (idx < SECTORS_TABLE_COUNT)
  {   
    if ((nextSector > startSector) && ((nextSector - startSector) == 1))
    {
      // confirmed sequential
      m_cbInterleave = 1;
      return true;
    }
    
    // nope, try to compute from thisSector
    BYTE interleave = 1;
    idx2 = idx;
    while (idx2 < SECTORS_TABLE_COUNT)
    {
      if ((BYTE)(m_cbSectorsTable[idx2++]) != startSector+1)
      {
        interleave++;
      }
      else
      {
        break;
      }
    }
    
    // double-check if nextSector is advanced by the same factor,
    // or that it loops around the starting sector
    idx2 = idx+interleave;
    
    if ((idx2 < SECTORS_TABLE_COUNT) && (interleave < sectorsPerTrack))
    {
      const BYTE next = (BYTE)(m_cbSectorsTable[idx2]);
      if ((next == nextSector+1) || (next == startSector))
      {
        m_cbInterleave = interleave;
        return true;
      }
    }
  }
  
  // could not determine interleave, fall back to sequential but store SPT
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
  
  // seek fast to the last (specified) cylinder and slowly back to 0, inspecting /TRK00 signal each step
  // number of steps back until the signal asserts must be equal to Cylinders-1    
  ui->print(Progmem::getString(Progmem::imdTestDriveSeek));
  if (fdc->seekTest(fdc->getParams()->Cylinders-1, 1))
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
  FDC::DiskDriveMediaParams backup;
  memcpy(&backup, fdc->getParams(), sizeof(FDC::DiskDriveMediaParams));
    
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
    memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
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
      
      memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
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
    
    memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
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
    
    memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
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
  memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
  fdc->resetController();
}

void IMD::printGeometryInfo(BYTE cyl, BYTE head, BYTE interleave)
{
  ui->print(""); 
  
  // single/doublestep, max. cylinders and sides
  ui->print(Progmem::getString(fdc->getParams()->DoubleStepping ? Progmem::imdDoubleStepping : Progmem::imdSingleStepping));
  ui->print(Progmem::getString(Progmem::imdGeoCylsHdStep), fdc->getParams()->Cylinders, fdc->getParams()->Heads);
  if (fdc->getParams()->Heads > 1)
  {
    ui->print("s");
  }
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  // sectors per track on current side and cylinder, sector gap length
  ui->print(Progmem::getString(Progmem::imdGeoChsSpt), cyl, head, fdc->getParams()->SectorsPerTrack,
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
  FDC::DiskDriveMediaParams backup;
  memcpy(&backup, fdc->getParams(), sizeof(FDC::DiskDriveMediaParams));
  ui->disableKeyboard(true);
  
  fdc->getParams()->CommRate = 250;
  fdc->getParams()->FM = false;
  fdc->getParams()->SectorSizeBytes = 8192;
  fdc->getParams()->SectorsPerTrack = 1;
  autodetectGaps(fdc->getParams()->GapLength, fdc->getParams()->Gap3Length);  
  fdc->setCommunicationRate();
  printGeometryInfo(0, 0, 1);
  
  BYTE head = 0;
  BYTE cyl = 0;  
  while(cyl < fdc->getParams()->Cylinders)
  {
    ui->print(Progmem::getString(Progmem::imdProgress), cyl, head);
    fdc->seekDrive(cyl, head);    
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
    cyl++;
  }
  
  ui->print(Progmem::getString(fdc->wasErrorNoDiskInDrive() ? Progmem::uiNewLine2x : Progmem::uiNewLine));
  fdc->seekDrive(0, 0);  
  ui->disableKeyboard(false);
  ui->print(Progmem::getString(Progmem::uiContinue));
  ui->readKey("\r");
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
}

void IMD::formatDisk()
{
  FDC::DiskDriveMediaParams backup;
  memcpy(&backup, fdc->getParams(), sizeof(FDC::DiskDriveMediaParams));
  
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
      memcpy(fdc->getParams(), &previousFormatParams, sizeof(FDC::DiskDriveMediaParams));      
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
    memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
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
      memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
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
      memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
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
    memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
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
  
  // starting sector
  ui->print(Progmem::getString(Progmem::imdFormatStartSec1));
  while(true)
  {
    ui->print(Progmem::getString(Progmem::imdFormatStartSec2), 256-spt);
    const BYTE* prompt = ui->prompt(3, Progmem::getString(Progmem::uiDecimalInput));    
    if (strlen(prompt))
    {
      WORD startSector = (WORD)atoi(prompt);
      if (startSector <= 256-spt)
      {
        m_formatStartSector = startSector;
        ui->print(Progmem::getString(Progmem::uiNewLine));
        break;
      }
    }    
    
    ui->print(Progmem::getString(Progmem::uiDeleteLine));
  }
    
  // all questions answered
  memcpy(&previousFormatParams, fdc->getParams(), sizeof(FDC::DiskDriveMediaParams));   
  
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
    memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
    return;
  }
  
  ui->disableKeyboard(true);  
  fdc->setCommunicationRate();  
  printGeometryInfo(0, 0, m_formatInterleave);
  
  WORD sector = m_formatStartSector;
  WORD endSector = fdc->getParams()->SectorsPerTrack;
  if (sector == 0)
  {
    endSector--;
  }
  else if (sector > 1)
  {
    endSector += sector-1;
  }
  
  BYTE head = 0;
  BYTE cyl = 0;
  WORD badSectorCount = 0;
  bool breakError = false;
  
  while(cyl < fdc->getParams()->Cylinders)
  {
    ui->print(Progmem::getString(Progmem::imdProgress), cyl, head);      
    
    fdc->seekDrive(cyl, head);    
    fdc->formatTrack(false, m_formatInterleave, m_formatStartSector);

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
      while (sector <= endSector)
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
    sector = m_formatStartSector;
    
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
    cyl++;
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
  
  memcpy(fdc->getParams(), &backup, sizeof(FDC::DiskDriveMediaParams));
}

void IMD::cleanup()
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
  
  // computed from the .IMD sectors map or read from disk via autodetectInterleave
  m_cbInterleave           = 1;
  
#ifndef SD_CARD_SUPPORT
  
  // all of these flags before the actual sector data follow
  // to know where we left off when the last data packet ended, and the callback returned
  m_cbModeSpecified        = false;
  m_cbCylinderSpecified    = false;
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
  m_cbCylinder             = (BYTE)-1;
  m_cbHead                 = (BYTE)-1;
  m_cbSpt                  = (BYTE)-1;
  m_cbSecSize              = (BYTE)-1;
  
  // optional for non-standard disks
  m_cbHasSecTrackMap       = false;
  m_cbHasSecHeadMap        = false;
  
  m_cbSecSizeBytes         = 0;         // 128 << secSize
  m_cbCurrentSector        = 0;         // last valid logical sector number read from m_cbSectorsTable
  m_cbGeometryChanged      = true;      // if mode, spt, secSize or interleave changed
  m_cbSeekIndicated        = true;      // indicate seek
  m_cbLastPos              = 0;         // position in buffer and maps
  m_cbSectorDataType       = 0;         // 1 byte right before data
  m_cbSectorIdx            = 0;         // array index of the current sector in the numberings maps
  m_cbStartingSectorIdx    = (BYTE)-1;  // index of the first starting sector (0 is valid), default: undefined
  
#endif // SD_CARD_SUPPORT
}

#endif // BUILD_IMD_IMAGER