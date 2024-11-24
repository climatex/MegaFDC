// MegaFDC (c) 2023-2024 J. Bogin, http://boginjr.com
// Floppy disk controller interface
// Software compatible with a NEC uPD765 or any other PC FDC

#include "config.h"

// global disk I/O buffer
volatile BYTE g_rwBuffer[SECTOR_BUFFER_SIZE];
volatile BYTE intType = 0;
volatile BYTE intFired = 0;
volatile WORD dataPos = 0;

// turn off floppy motor timer, if motor on and FDC idle for more than 2 seconds
ISR(TIMER5_COMPA_vect)
{
  static BYTE timerCount = 0;
  static FDC* fdc = FDC::get();
  
  if (fdc->isIdle())
  {  
    if (fdc->isMotorOn())
    { 
      if (timerCount > 2)
      {
        fdc->motorOff();
        timerCount = 0;
      }
      else
      {
        timerCount++;
      }
    }
  }
  
  // not idle
  else
  {
    timerCount = 0;
  }
}

// singleton
FDC::FDC()
{   
  // means: floppy controller successfully reset and drive can be seeked to track 0
  m_initialized = false;
  
  m_lastError = false;
  m_motorOn = false;
  m_noDiskInDrive = false;
  m_diskWriteProtected = false;
  m_diskChangeInquired = false;
  m_idle = true;
  m_silentOnTrivialError = false;
  m_controlMark = false;
  
  // no special features support determined yet
  m_specialFeatures = 0;
   
  // current physical CHS values
  m_currentTrack = 0;
  m_currentHead = 0;
  m_currentSector = 1;  
   
  // initialize FDC by doing a hardware reset + also set port directions to default:
  // data lines (PORTA) set as output, address lines + RESET/RD/WR/TC (PORTC) set as output
  cli();
  DDRC = 0xFF;
  DDRA = 0xFF;
  PORTA = 0;
  
  // TG43 (on PD7) off by default
  PORTD |= 0x80;

  // pull /CS, /RD, /WR high, RESET high for some good 30us
  PORTC = 0x78;  
  DELAY_CYCLES(500);
  
  // flip RESET low - reset over; /CS low (chip selected)
  PORTC ^= 0x48;
   
  // setup Mega2560 Timer5 to automatically turn off floppy motor
  // freq. = 16MHz / CMR * prescaler
  // 1Hz = 16000000/15624*1024
  TCCR5A = 0;
  TCCR5B = 0;
  TCNT5 = 0;

  // CMR
  OCR5A = 15624; 
  
  // 1024 prescaler with CTC
  TCCR5B |= (1 << WGM52) | (1 << CS52) | (1 << CS50);
  
  // enable timer (interrupts still disabled for now)
  TIMSK5 |= (1 << OCIE5A);
  
  // attach FDC INT signal (pin 2, PORTE4)
  pinMode(2, INPUT);
  setInterrupt();
    
  // enable interrupts
  sei();
}

// get data from DTR
BYTE FDC::getData()
{  
  //read data from FDC DTR with handshaking and timeout
  DWORD timeout = IO_TIMEOUT;
  
  while (--timeout)
  {
    // inspect main status register, receive byte only if RQM=1 and direction is FDC->AVR
    if ((readRegister(MSR) & 0xC0) == 0xC0)
    {
      return readRegister(DTR);
    }
  }
  
  // no picture, no sound
  m_idle = true;
  fatalError(Progmem::errRQMTimeout);
  
  return 0; // won't return - just to keep compiler happy
}

void FDC::sendData(BYTE data)
{  
  //send data to FDC DTR with handshaking and timeout
  DWORD timeout = IO_TIMEOUT;
  
  while (--timeout)  
  {
    // send byte only if RQM=1 and direction is AVR->FDC
    if ((readRegister(MSR) & 0xC0) == 0x80)
    {
      writeRegister(DTR, data);
      return;
    }
  }
  
  // same as above
  m_idle = true;
  fatalError(Progmem::errRQMTimeout);
}

// waits until interrupt from FDC
void FDC::waitForINT()
{
  DWORD timeout = IO_TIMEOUT;

  while (--timeout)
  {
    if (intFired)
    {
      // interrupt as a command result needs to be "sensed" with cmd 0x8
      intFired = 0;
      m_lastError = false;
      return;        
    }
  }
  
  // the FDC is deaf as a post
  m_idle = true;
  fatalError(Progmem::errRQMTimeout);
}

// similar to waitForINT; allows continuing after timeout
bool FDC::waitForDATA()
{
  DWORD timeout = IO_TIMEOUT;
  
  while(--timeout)
  {
    if (intFired)
    {   
      intFired = 0; //reset flag
      return true;
    }
  }
    
  return false;
}

void FDC::sendCommand(BYTE command)
{
  // calls sendData for main commands, erases MFM flag if FM on
  if (m_params && m_params->FM)
  {
    sendData(command & 0xBF);
  }
  else
  {
    sendData(command);
  }
}

void FDC::resetController()
{ 
  m_idle = false;
  
  // reset interrupt handler
  cli();
  detachInterrupt(digitalPinToInterrupt(2));
  intType = 0;
  setInterrupt();
  sei();
  
  // prepare drive select and motor on bits
  BYTE driveSelect = 0;
  BYTE motorOn = 0;
  if (m_params)
  {
    driveSelect = m_params->DriveNumber & 0x03;
    motorOn = 1 << (m_params->DriveNumber + 4);
  }
  
  // software controller reset by bit 2=0 in DCR (keep motor on if retrying)
  // interrupts off, all drives unselected
  writeRegister(DCR, m_motorOn ? motorOn : 0);  
  DELAY_CYCLES(500);
  
  // interrupts on, reset off, drive select (motor on conditions same as above)
  writeRegister(DCR, m_motorOn ? motorOn | driveSelect | 0x0C : driveSelect | 0x0C);
  waitForINT();
    
  // call Sense interrupt status command (0x8) after a reset 3 times (drive polling)
  for (int count = 0; count < 3; count++)
  {
    sendCommand(8);
    getData(); // ST0, unused
    getData(); // current track number, unused
  }
  
  // determine special support - try out controller capabilities: CONFIGURE and PERPENDICULAR commands
  // the former to configure FIFO enabled to offer 1Mbps mode, the latter to offer 2.88MB drives
  // both commands have no result phase - if we get one, it means the FDC did not understand the command  
  // start with 0x13 - Configure
  sendCommand(0x13);
  
  // wait for RQM response after sending
  while (!(readRegister(MSR) & 0x80)) {};
  
  // direction is now FDC->AVR instead of AVR->FDC? this means the FDC wants to tell us something
  if ((readRegister(MSR) & 0xC0) == 0xC0)
  {
    // and we know what it is: 0x80 Invalid command in ST0, read it and trash it
    getData();
  }
  
  // no result phase - command understood and can be specified
  else
  {
    sendData(0); // unused
    sendData(3); // configure: implied seek off, FIFO on, with 4 bytes threshold
    sendData(0); // starting track for write precompensation - unused, default
    
    m_specialFeatures |= SUPPORT_1MBPS;
  }
  
  // 0x12 Perpendicular mode, same check as above
  sendCommand(0x12);
  while (!(readRegister(MSR) & 0x80)) {};
  if ((readRegister(MSR) & 0xC0) == 0xC0)
  {
    getData();
  }
  
  else
  {
    sendData(0x80); // command understood, all 4 drives initialized as normal for now
    m_specialFeatures |= SUPPORT_PERPENDICULAR;
  }
  
  // if setActiveDrive was not called yet, return
  if (!m_params)
  {
    return;
  }
  
  // set FDC communication rate and stepper motor timings
  setCommunicationRate();
  
  // recalibrate command required to initialize disk operations
  m_idle = true;
}

void FDC::setCommunicationRate()
{
  // set FDC communication rate in kbit/s, this also adjusts drive timings
  if (m_params->CommRate == 300)
  {
    writeRegister(DRR, 1);
  }
  else if (m_params->CommRate == 250)
  {
    writeRegister(DRR, 2);
  }
  else if (m_params->CommRate == 1000)
  {
    writeRegister(DRR, 3);
  }
  else if (m_params->CommRate == 500)
  {
    writeRegister(DRR, 0);
  }
  
  // step rate time (SRT): 8": 10ms, 5.25": 8ms, 3.5": 4ms
  if (m_params->DriveInches == 8)
  {
     // the highest for 1Mbps is 8ms but I've never seen an 8" operate at that rate
    m_params->SRT = (m_params->CommRate < 1000) ? 10 : 8;
  }  
  else if (m_params->DriveInches == 5)
  {
    m_params->SRT = 8;
  }
  else if (m_params->DriveInches == 3)
  {
    m_params->SRT = 4;
  }
  
  // head load time (HLT): 16ms (250k, 500k, 1M rates), 16.67ms (300kbps)
  // head unload time (HUT): 224ms (250k, 500k rates), 240ms (300kbps), 127ms (1Mbps)
  switch(m_params->CommRate)
  {
  case 250:
  case 500:
    m_params->HLT = 16;
    m_params->HUT = 224;
    break;
  case 300:
    m_params->HLT = 17;
    m_params->HUT = 240;
    break;
  case 1000:
    m_params->HLT = 16;
    m_params->HUT = 127;
  }
  
  // prepare data for FDC command 0x3 - Specify - values are data rate dependent
  // first byte is SRT (upper nibble) | HUT (lower nibble)
  // second byte is HLT (bits 7-1) | non-DMA mode flag (bit 0)
  // non-DMA mode: 1 (no DMA in classic Arduino)
  // see https://www.isdaman.com/alsos/hardware/fdc/floppy.htm
  BYTE srtHut = 0;
  BYTE hltNonDMA = 0;
  
  if (m_params->CommRate == 250)
  {
    srtHut = (((32 - m_params->SRT) / 2) << 4) | (m_params->HUT / 32);
    hltNonDMA = ((m_params->HLT / 4) << 1) | 1;
  }
  else if (m_params->CommRate == 300)
  {
    srtHut = (((2672 - ((WORD)m_params->SRT * 100)) / 167) << 4) | ((m_params->HUT * 3) / 80);
    hltNonDMA = (((m_params->HLT * 3) / 10) << 1) | 1;
  }
  else if (m_params->CommRate == 500)
  {
    srtHut = ((16 - m_params->SRT) << 4) | (m_params->HUT / 16);
    hltNonDMA = ((m_params->HLT / 2) << 1) | 1;
  }
  else if (m_params->CommRate == 1000)
  {
    srtHut = ((16 - (m_params->SRT * 2)) << 4) | (m_params->HUT / 8);
    hltNonDMA = (m_params->HLT << 1) | 1;
  }
  
  // 0x3 Specify
  sendCommand(3);
  sendData(srtHut);
  sendData(hltNonDMA);
}

void FDC::recalibrateDrive()
{ 
  if (!m_params)
  {
    return;
  }
    
  // software controller reset is necessary to get rid of FDC errors or if not initialized
  if (!m_initialized || m_lastError)
  {
    resetController();
  }
  
  m_idle = false;
  motorOn();
  setInterrupt();
  
  // more retries recalibrating the drive before calling it quits
  for (BYTE retries = 0; retries < DISK_OPERATION_RETRIES*2; retries++)
  {
    if (retries)
    {     
      resetController();
      m_idle = false;
    }
    
    // 0x7 Recalibrate drive - bring head to track 0
    sendCommand(7);
    sendData(m_params->DriveNumber);
    waitForINT();
   
    // 0x8 Sense interrupt status & retrieve results
    sendCommand(8);
    BYTE st0 = getData(); // status register 0
    BYTE currentTrack = getData();

    // was seek successful?
    if (st0 & 0x20)
    {
      // UC error
      if (st0 & 0x10)
      {
        continue;
      }
      
      else
      {
        // success, we're at track 0
        if (currentTrack == 0)         
        {
          // ready and idle
          m_initialized = true;
          m_idle = true;
          
          m_lastError = false;
          PORTD |= 0x80; // deassert TG43
          return;
        }
      }
    }
  }
  
  // cannot recalibrate drive - problem with FDC or drive, or no drive connected
  m_idle = true;  
  fatalError(Progmem::errRecalibrate);
}

void FDC::motorOn(bool withDelay)
{ 
  if (!m_params)
  {
    return;
  }
  
  if (!m_motorOn)
  {
    const BYTE driveSelect = m_params->DriveNumber & 0x03;
    const BYTE motorOn = 1 << (m_params->DriveNumber + 4);
    
    // drive select, turn motor on and give some spin up delay
    writeRegister(DCR, driveSelect | motorOn | 0x0C);
    if (withDelay)
    {
      DELAY_MS(500);  
    }    
    
    m_motorOn = true;
  }  
}

void FDC::motorOff()
{ 
  if (!m_params)
  {
    return;
  }
  
  if (m_motorOn)
  {
    const BYTE driveSelect = m_params->DriveNumber & 0x03;
    writeRegister(DCR, driveSelect | 0x0C);
    
    m_motorOn = false;  
  }  
}

void FDC::seekDrive(BYTE track, BYTE head)
{ 
  if (!m_params)
  {
    return;
  }
  
  if (!m_initialized || m_lastError)
  {
    recalibrateDrive();
  }
  
  // seek the drive heads carriage to given physical track (cylinder) and select head (disk side)
  m_idle = false;
  motorOn();
  setInterrupt();
  
  for (BYTE retries = 0; retries < DISK_OPERATION_RETRIES; retries++)
  {    
    if (retries)
    {      
      // break from attempting to seek if recal not successful
      recalibrateDrive();
      m_idle = false;
    }
    
    // seek with double stepping?
    if (m_params->DoubleStepping)
    {
      track *= 2;
    }
    
    // 0xF Seek
    sendCommand(0xF);
    sendData((head << 2) | m_params->DriveNumber); // head and drive
    sendData(track);
    waitForINT();
    
    // 0x8 Sense interrupt
    sendCommand(8);
    BYTE st0 = getData();
    BYTE seekedTrack = getData();
    
    // seek successful ?
    if (st0 & 0x20)
    {
      // UC error
      if (st0 & 0x10)
      {
        continue;
      }
      
      else
      {
        // not on our track
        if (track != seekedTrack)
        {
          continue;
        }
        
        // success, store current track and head
        else
        {
          // if double stepping, store the actual media track number
          m_currentTrack = m_params->DoubleStepping ? track/2 : track;
          m_currentHead = head;
          
          m_lastError = false;
          m_idle = true;
          
          // handle optional TG43 line on PD7
          if (m_currentTrack > 42)
          {
            PORTD &= 0x7F; //TG43 on
          }
          else
          {
            PORTD |= 0x80; //TG43 off
          }
          
          return;
        }
      }
    }
  }
  
  m_idle = true;  
  fatalError(Progmem::errSeek);
}

WORD FDC::readWriteSectors(bool writeOperation, BYTE startSector, BYTE endSector, WORD* dataPosition, bool deleted, BYTE* overrideTrack, BYTE* overrideHead)
{
  // reads/writes chosen sector off current track and head to/from ioBuffer
  // writeOperation false: read, true: write
  // startSector: initial sector number (1-based)
  // endSector: ending sector number (1-based) or getMaximumSectorCountForRW() for maximum possible
  // dataPosition: custom dataPos starting index (optional)
  // deleted: read or write deleted data mark (optional, false by default)
  // overrideTrack, overrideHead: logical sector information differs from what's in the current physical track (non-standard disks)
  // returns: bytes successfully read or written
   
  // sanity checks
  if (!m_params ||
      !startSector ||
      (startSector > endSector) ||
      (endSector > m_params->SectorsPerTrack))
  {
    return 0;
  }
  
  // check for end-of-track and data position buffer overflow
  const WORD sectorCount = endSector-startSector+1;
  if (sectorCount > getMaximumSectorCountForRW(startSector))
  {
    return 0;
  }  
  if (dataPosition && ((*dataPosition)+(sectorCount*m_params->SectorSizeBytes)) > SECTOR_BUFFER_SIZE)
  {
    return 0;
  }
   
  // set beginning sector number
  m_currentSector = startSector;
  
  // needs recalibrate and seek?
  if (!m_initialized || m_lastError)
  {
    recalibrateDrive();
    seekDrive(m_currentTrack, m_currentHead);
  }
  
  // seek to the proper track must be already done
  m_idle = false;
  m_noDiskInDrive = false; // reset this flag, to be determined now
  motorOn();
   
  for (BYTE retries = 0; retries < DISK_OPERATION_RETRIES; retries++)
  {    
    // retrying disk operation - need to reset, recalibrate and reseek drive back
    if (retries)
    {      
      recalibrateDrive();
      seekDrive(m_currentTrack, m_currentHead);
      m_idle = false;
    }
    
    // set buffer position to supplied, or do from buffer index 0
    dataPos = dataPosition ? *dataPosition : 0;
    
    // set ISR to data transfer R/W
    setInterrupt(writeOperation ? INTERRUPT_WRITE : INTERRUPT_READ);
    
    // set longitudinal or perpendicular mode
    setRecordingMode();
    
    // supplied track or head number differs from the physical we're currently at?
    BYTE currentTrack = m_currentTrack;
    BYTE currentHead = m_currentHead;
    if (overrideTrack)
    {
      currentTrack = *overrideTrack;
    }
    if (overrideHead)
    {
      currentHead = *overrideHead;
    }
    
    // 0x46 Read data : 0x45 Write data
    BYTE command = writeOperation ? 0x45 : 0x46;
    if (deleted)
    {
      // 0x4C Read deleted data : 0x49 Write deleted data
      command = writeOperation ? 0x49 : 0x4C;
    }
    sendCommand(command);
    sendData((m_currentHead << 2) | m_params->DriveNumber); // physical head and drive
    sendData(currentTrack); // logical track
    sendData(currentHead); // logical head
    sendData(startSector); // which sector to read
    sendData(convertSectorSize(m_params->SectorSizeBytes));  // sector size 0 to 5, 128B to 4096B
    sendData(endSector); // which ending sector
    sendData(m_params->GapLength); // gap length
    sendData((m_params->SectorSizeBytes == 128) ? 0x80 : 0xFF); // data transfer length
    
    // check if there is a disk in drive (no timeout from FDC)
    if (!waitForDATA())
    {
      m_idle = true;
      m_lastError = true;
      m_noDiskInDrive = true;
      motorOff(); // no disk in drive, no point in keeping it on
      
      ui->print(Progmem::getString(Progmem::errINTTimeout));
      return 0;
    }
    
    // get status registers
    BYTE st0 = getData();
    BYTE st1 = getData();
    BYTE st2 = getData();
    
    // track, head, sector number and size
    getData();
    getData();
    m_currentSector = getData();
    getData();
    
    if (processIOResult(st0, st1, st2, endSector))
    {
      if (writeOperation)
      {
        m_diskChangeInquired = false; // data changed on disk; return disk changed yes when asked once
      }
      
      // no errors during I/O, return total bytes read or written successfully
      return dataPos;
    }
  }
    
  // operation failed, return 0 bytes - last error already in print buffer from processIOResult
  m_idle = true;
  m_lastError = true;
  if (!m_silentOnTrivialError || m_diskWriteProtected)
  {
    ui->print(ui->getPrintBuffer());  
  }
  return 0;
}

BYTE* FDC::getInterleaveTable(BYTE sectorsPerTrack, BYTE interleave)
{
  // compute custom interleave table (1-based indexing)
  BYTE* result = new BYTE[sectorsPerTrack + 1];
  if (!result)
  {
    return result;
  }
  memset(result, 0, sectorsPerTrack+1);

  // fallback to sequential on invalid values
  if (!interleave || (interleave >= sectorsPerTrack))
  {
    interleave = 1;
  }

  BYTE pos = 1;
  BYTE currentSector = 1;
  while (currentSector <= sectorsPerTrack)
  {
    result[pos] = currentSector++;
    pos += interleave;
    if (pos > sectorsPerTrack)
    {
      pos = pos % sectorsPerTrack;
      while ((pos <= sectorsPerTrack) && (result[pos]))
      {
        pos++;
      }
    }
  }

  return result;
}

bool FDC::formatTrack(bool customCHSVTable, BYTE interleave)
{  
  // formats current physical track and head number (without bad sector verify)
  // customCHSVTable: true if g_rwBuffer already contains the prepared 4-byte values (optional)
  // interleave: sector interleave factor, default 1:1 (optional)
  if (!m_params)
  {
    return false;
  }
  
  m_currentSector = 1;
  if (!customCHSVTable)
  {
    BYTE* interleaveTable = getInterleaveTable(m_params->SectorsPerTrack, interleave);
    if (!interleaveTable)
    {
      return false;
    }
    
    // clear and prepare write buffer with 4-byte 'CHSV' format values, for each sector in track
    memset(&g_rwBuffer[0], 0, m_params->SectorsPerTrack * 4);
    dataPos = 0;
    for (BYTE sector = 0; sector < m_params->SectorsPerTrack; sector++)
    {
      g_rwBuffer[dataPos++] = m_currentTrack;                               // C
      g_rwBuffer[dataPos++] = m_currentHead;                                // H
      g_rwBuffer[dataPos++] = interleaveTable[sector+1];                    // S (1-based)
      g_rwBuffer[dataPos++] = convertSectorSize(m_params->SectorSizeBytes); // V (value of sector size, 0 to 6)
    }
    
    delete[] interleaveTable;
  }
  
  if (!m_initialized || m_lastError)
  {
    recalibrateDrive();
    seekDrive(m_currentTrack, m_currentHead);
  }
  
  m_idle = false;
  m_noDiskInDrive = false;
  motorOn();
   
  for (BYTE retries = 0; retries < DISK_OPERATION_RETRIES; retries++)
  {    
    if (retries)
    {      
      recalibrateDrive();
      seekDrive(m_currentTrack, m_currentHead);
      m_idle = false;
    }
    
    // reset buffer position, set ISR to data transfer write
    dataPos = 0;
    setInterrupt(INTERRUPT_WRITE);
    
    // set longitudinal or perpendicular mode
    setRecordingMode();
          
    // 0x4D Format track
    sendCommand(0x4D);
    sendData((m_currentHead << 2) | m_params->DriveNumber); // head and drive
    sendData(convertSectorSize(m_params->SectorSizeBytes)); // sector size 0 to 6, 128B to 8192B
    sendData(m_params->SectorsPerTrack); // EOT, last sector on track
    sendData(m_params->Gap3Length); // GAP3
    sendData(m_params->LowLevelFormatFiller); // filler byte into sectors
       
    if (!waitForDATA())
    {
      m_idle = true;
      m_lastError = true;
      m_noDiskInDrive = true;
      motorOff();
            
      ui->print(Progmem::getString(Progmem::errINTTimeout));
      return false;
    }
    
    BYTE st0 = getData();
    BYTE st1 = getData();
    BYTE st2 = getData();
    
    getData();
    getData();
    getData();
    getData();
    
    if (processIOResult(st0, st1, st2, m_params->SectorsPerTrack))
    {
      m_diskChangeInquired = false; // data changed on disk; return disk changed yes when asked once
      return true; // no errors during I/O
    }
  }
    
  m_idle = true;
  m_lastError = true;
  if (!m_silentOnTrivialError || m_diskWriteProtected)
  {
    ui->print(ui->getPrintBuffer());  
  }
  return false;
}

WORD FDC::verify(BYTE sector, bool wholeTrack, BYTE* overrideTrack, BYTE* overrideHead)
{
  // reads sector(s) without storing the buffer  
  // overrideTrack, overrideHead: logical sector information differs from what's in the current physical track (non-standard disks)
  // FDC::verify() without input arguments verifies whole track
  
  if (!m_params || (sector > m_params->SectorsPerTrack))
  {
    return 0;
  }
    
  m_currentSector = sector;
  
  if (!m_initialized || m_lastError)
  {
    recalibrateDrive();
    seekDrive(m_currentTrack, m_currentHead);
  }
  
  m_idle = false;
  m_noDiskInDrive = false;
  motorOn();
   
  for (BYTE retries = 0; retries < DISK_OPERATION_RETRIES; retries++)
  {    
    if (retries)
    {      
      recalibrateDrive();
      seekDrive(m_currentTrack, m_currentHead);
      m_idle = false;
    }
    
    // reset buffer position, set ISR to data verify
    dataPos = 0;
    setInterrupt(INTERRUPT_VERIFY);
    
    // set longitudinal or perpendicular mode
    setRecordingMode();
    
    // supplied track or head number differs from the physical we're currently at?
    BYTE currentTrack = m_currentTrack;
    BYTE currentHead = m_currentHead;
    if (overrideTrack)
    {
      currentTrack = *overrideTrack;
    }
    if (overrideHead)
    {
      currentHead = *overrideHead;
    }
       
    // 0x46 Read data
    sendCommand(0x46);
    sendData((m_currentHead << 2) | m_params->DriveNumber); // physical head and drive
    sendData(currentTrack); // logical track
    sendData(currentHead); // logical head
    sendData(sector); // starting sector
    sendData(convertSectorSize(m_params->SectorSizeBytes));  // sector size 0 to 6, 128B to 8192B
    sendData(wholeTrack ? m_params->SectorsPerTrack : sector); // ending sector
    sendData(m_params->GapLength); // gap length
    sendData((m_params->SectorSizeBytes == 128) ? 0x80 : 0xFF); // data transfer length
    
    if (!waitForDATA())
    {
      m_idle = true;
      m_lastError = true;
      m_noDiskInDrive = true;
      motorOff();
      
      ui->print(Progmem::getString(Progmem::errINTTimeout));
      return 0;
    }
    
    BYTE st0 = getData();
    BYTE st1 = getData();
    BYTE st2 = getData();
    
    getData();
    getData(); 
    m_currentSector = getData();
    getData();
    
    if (processIOResult(st0, st1, st2, wholeTrack ? m_params->SectorsPerTrack : sector))
    {
      return dataPos;
    }
  }
    
  m_idle = true;
  m_lastError = true;
  if (!m_silentOnTrivialError)
  {
    ui->print(ui->getPrintBuffer());  
  }
  return 0;
}

// verify head 0, track 0 readability, useful before beginning filesystem operations
bool FDC::verifyTrack0(bool beforeWriteOperation)
{
  seekDrive(0, 0);
  verify();
  
  if (m_lastError)
  {
    // no disk, error written by FDC
    if (m_noDiskInDrive)
    {
      ui->print(Progmem::getString(Progmem::uiNewLine));
      return false;
    }
    
    // cannot read track 0 - bad disk or improperly config'd drive  
    else
    {
      ui->print(Progmem::getString(Progmem::errTrack0Error));
            
      // try formatting first?
      if (beforeWriteOperation)
      {
        ui->print(Progmem::getString(Progmem::uiNewLine));
        ui->print(Progmem::getString(Progmem::errTryFormat));
      }
      
      ui->print(Progmem::getString(Progmem::uiNewLine));
      return false;
    }
  }
  
  return true;
}

// set ISR type behavior
void FDC::setInterrupt(BYTE operation)
{
  // operation - 1: interrupt acknowledge, 2: read, 3: verify (read without storing), 4: write  
  // no change
  if (intType == operation)
  {
    return;
  }

  // attachInterrupt to FDCACK, FDCREAD, FDCVERIFY, FDCWRITE
  if (intType != 0)
  {
    // changed, do detach before
    detachInterrupt(digitalPinToInterrupt(2));  
  }
  
  switch(operation)
  {
    case 1: // acknowledge
      attachInterrupt(digitalPinToInterrupt(2), FDCACK, RISING);
      break;
    case 2: // read sectors
      attachInterrupt(digitalPinToInterrupt(2), FDCREAD, RISING);
      break;
    case 3: // verify whole track
      attachInterrupt(digitalPinToInterrupt(2), FDCVERIFY, RISING);
      break;
    case 4: // write sectors
      attachInterrupt(digitalPinToInterrupt(2), FDCWRITE, RISING);
      break;    
  }
  
  intType = operation;
}

bool FDC::processIOResult(BYTE st0, BYTE st1, BYTE st2, BYTE endSectorNo)
{ 
  m_idle = true;
  
  // process IO operation result, and if there was an error, print it into the ui->print() buffer
  // however, print it out only after all retries failed
  // if m_silentOnTrivialError, only print fatal errors and no disk in drive/disk write protected
  
  // reset these flags, to be determined here
  m_diskWriteProtected = false;
  m_lastError = false;
  
  // control mark: read/write data found with a deleted address mark
  // OR read/write deleted data found with a regular address mark
  m_controlMark = st2 & 0x40;
  
  // bits 7-6 of st0, if equal to 0, indicate successful operation
  // however, also allow st1 bit 7 set ("End of track error") to pass as OK
  // this is because we do not assert the TC line, which would be used by the DMA controller
  // checking for sector count being inside total sectors per track is done by us
  if (!(st0 & 0xC0) || (st1 & 0x80))
  {
    return true;
  }
  
  m_lastError = true;
  BYTE errorMessage = 0;
    
  // CRC error
  if ((st1 & 0x20) || (st2 & 0x20))
  {
    errorMessage = Progmem::errCRC;
  }
  
  // no address mark
  else if ((st1 & 1) || (st2 & 1))
  {
    errorMessage = Progmem::errNoAddrMark;
  }
  
  // data overrun (ISR processing too slow)
  else if (st1 & 0x10)
  {
    errorMessage = Progmem::errOverrun;
  }
  
  // sector not found
  else if ((st1 & 4) || (st2 & 0x40))
  {
    errorMessage = Progmem::errNoData;
  }
  
  // wrong track number or bad track
  else if (st2 & 0x12)
  {
    errorMessage = Progmem::errBadTrack;
  }
  
  // write protected - do not write CHS information, as this applies for whole media
  else if (st1 & 2)
  {
    m_diskWriteProtected = true;
    strncpy(ui->getPrintBuffer(), Progmem::getString(Progmem::errWriProtect), MAX_CHARS);
  }
  
  // some other error - unknown: return CHS with ST registers in hex
  else
  {   
    // CHS single sector
    if (endSectorNo == m_currentSector)
    {
      snprintf(ui->getPrintBuffer(), MAX_CHARS, 
               Progmem::getString(Progmem::errChsFmtSTRegsSingle),
               m_currentTrack, m_currentHead, m_currentSector, st0, st1, st2);               
    }
    
    // CHS multi sector
    else
    {
      snprintf(ui->getPrintBuffer(), MAX_CHARS, 
               Progmem::getString(Progmem::errChsFmtSTRegsMulti),
               m_currentTrack, m_currentHead, m_currentSector, endSectorNo, st0, st1, st2); 
    }
    
    // append newline
    strcat(ui->getPrintBuffer(), "\r\n");
  }
  
  // document error with CHS information
  if (errorMessage)
  {
    // CHS single sector
    if (endSectorNo == m_currentSector)
    {
      snprintf(ui->getPrintBuffer(), MAX_CHARS, 
               Progmem::getString(Progmem::errChsFmtSingleSector),
               m_currentTrack, m_currentHead, m_currentSector);
    }
    
    // CHS multi sector
    else
    {
      snprintf(ui->getPrintBuffer(), MAX_CHARS, 
               Progmem::getString(Progmem::errChsFmtMultiSector),
               m_currentTrack, m_currentHead, m_currentSector, endSectorNo);
    }
    
    // append the rest of the error message and an extra newline
    strcat(ui->getPrintBuffer(), Progmem::getString(errorMessage));
    strcat(ui->getPrintBuffer(), "\r\n");
  }
  
  // just print it into buffer, do not display yet  
  return false;
}

// clears the screen (serial: newlines), informs and goes into infinite loop
void FDC::fatalError(BYTE message)
{
  // make sure printing and keyboard are enabled, motor timer on
  ui->setPrintDisabled(false, false);
  ui->disableKeyboard(false);
  setAutomaticMotorOff(true);
  
  ui->print("");
  ui->print(Progmem::getString(Progmem::uiFatalError));
  
  // if the message was drive not responding/failed to seek, inform which one
  if ((message == Progmem::errRecalibrate) || (message == Progmem::errSeek))
  {
    ui->print(Progmem::getString(message), m_params->DriveNumber + 65);
  }
  else
  {
    ui->print(Progmem::getString(message));  
  }
  
  ui->print(Progmem::getString(Progmem::uiSystemHalted));
  
  // Ctrl+Alt+Delete
  if (g_uiEnabled)
  {
    ui->print(Progmem::getString(Progmem::uiCtrlAltDel));
  }
  
  // creative infinite loop - wait for a key with no keys allowed
  // this keeps interrupts enabled to turn FDD motor off etc
  ui->readKey("");
}

// switch to other drive, set new params pointer and perform a quick seek test
void FDC::setActiveDrive(DiskDriveMediaParams* newParams)
{
  if (!newParams)
  {
    return;
  }
  
  // turn interrupts off, set new parameters, set previous flags default
  cli();
  motorOff();
  
  // reset interrupt handler
  detachInterrupt(digitalPinToInterrupt(2));
  intType = 0;

  m_params = newParams;
  m_currentTrack = 0;
  m_currentHead = 0;
  m_currentSector = 1;  

  m_initialized = false;
  m_lastError = false;
  m_noDiskInDrive = false;
  m_diskWriteProtected = false;
  m_diskChangeInquired = false;
  m_idle = false;
  m_silentOnTrivialError = false;
  m_controlMark = false;
  sei();  
  
  // do a quick seek test  
  if (!seekTest(10, 2))
  {
    fatalError(Progmem::errSeek);
  }  
}

bool FDC::seekTest(BYTE toTrack, BYTE step)
{ 
  // seek "toTrack" in one go, then down to 0 in decrements of "step"
  // make sure we're recalibrated and on track 0
  setInterrupt();
  resetController();
  recalibrateDrive();

  int trackToSeek = toTrack;
  BYTE onTrackZero = 0;
  while (trackToSeek >= 0)
  {
    seekDrive(trackToSeek, 0);
    
    // call 0x4 Sense drive status to inspect TRK00 signal each try
    // make sure the drive is selected and motor on, beforehand
    m_idle = false;
    motorOn();
    
    sendCommand(4);
    sendData(m_params->DriveNumber);
    onTrackZero = getData() & 0x10;
        
    // inspect TRK00 signal
    if (((trackToSeek == 0) && !onTrackZero) ||
        ((trackToSeek > 0) && onTrackZero))
    {
      m_idle = true;  
      return false;
    }
    
    trackToSeek -= step;
  }
  
  m_idle = true;
  return (m_currentTrack == 0) && onTrackZero;
}

// determine if disk has been changed in the drive
bool FDC::isDiskChanged()
{
  // no changeline support, disk always "changed"
  if (!m_params || !m_params->DiskChangeLineSupport)
  {
    return true;
  }
  
  // inquired for the first time (or after setActiveDrive/format/write operation)
  // return disk changed yes *once* without checking
  if (!m_diskChangeInquired)
  {
    m_diskChangeInquired = true;
    return true;
  }
   
  bool diskChanged = false;
  
  // inquire the disk change flag from DRR when drive selected and motor on
  motorOn(false);
  diskChanged = readRegister(DRR) & 0x80;
  
  // cancel out the flag inside DRR by seeking back and forth; this flag is readonly
  seekDrive(++m_currentTrack, m_currentHead);
  seekDrive(--m_currentTrack, m_currentHead);  
    
  return diskChanged;
}

// compute CHS values from supplied logical address
void FDC::convertLogicalSectorToCHS(WORD logicalSector, BYTE& track, BYTE& head, BYTE& sector)
{
  if (!m_params)
  {
    return;
  }
  
  track = (logicalSector / m_params->SectorsPerTrack) / m_params->Heads;
  head = (logicalSector / m_params->SectorsPerTrack) % m_params->Heads;
  sector = (logicalSector % m_params->SectorsPerTrack) + 1;
}

// get number of sectors total
WORD FDC::getTotalSectorCount()
{
  if (!m_params)
  {
    return 0;
  }
    
  return m_params->Tracks * m_params->SectorsPerTrack * m_params->Heads;
}

// get the maximum sector count for a read/write operation
// depending on sector buffer size or bytes of operation on same track left
BYTE FDC::getMaximumSectorCountForRW(BYTE startSector, WORD operationBytes)
{
  if (!m_params || !startSector || (startSector > m_params->SectorsPerTrack))
  {
    return 0;
  }
  
  const WORD maxBytes = operationBytes ? operationBytes : SECTOR_BUFFER_SIZE;  
  BYTE endSector = startSector - 1;
  endSector += maxBytes / m_params->SectorSizeBytes;
  
  if (endSector > m_params->SectorsPerTrack)
  {
    endSector = m_params->SectorsPerTrack;
  }
  
  return endSector - startSector + 1;
}

// translate sector size in bytes to FDC sector size "N"
BYTE FDC::convertSectorSize(WORD sectorSize)
{
  WORD sizeN = 0;
  while ((128 << sizeN) != sectorSize)
  {
    sizeN++;
  }
  
  return (BYTE)sizeN;
}

// convert sector size "N" to bytes
WORD FDC::getSectorSizeBytes(BYTE sectorSizeN)
{
  return 128 << (WORD)sectorSizeN;
}

// allows to enable or disable auto floppy motor off timer
void FDC::setAutomaticMotorOff(bool enabled)
{
  static bool previous = true;
  if (previous == enabled)
  {
    return;
  }
  
  cli();
  if (enabled)
  {
    TIMSK5 |= (1 << OCIE5A);
  }
  else
  {
    TIMSK5 &= ~(1 << OCIE5A);
  }
  sei();
  
  previous = enabled;
}

// set normal (longitudinal) or perpendicular recording mode
void FDC::setRecordingMode()
{
  // no command support?
  if (!m_params || !(m_specialFeatures & SUPPORT_PERPENDICULAR))
  {
    return;
  }
  
  // set overwrite flag on and normal recording mode by default for all drives
  BYTE data = 0x80;
    
  // perpendicular recording support turned on by the FDC by adjusting GAP2 and WGATE
  // setting valid for 500k (GAP, WGATE 01) and 1Mbps (GAP, WGATE 11) rates
  if (m_params->PerpendicularRecording && (m_params->CommRate >= 500))
  {
    data |= (m_params->CommRate == 500) ? 1 : 3;
    data |= (m_params->DriveNumber + 1) << 2; // affected drive number
  }
  
  // 0x12 Perpendicular mode
  sendCommand(0x12);
  sendData(data);
}

bool FDC::readSectorID(BYTE* track, BYTE* head, BYTE* sector, BYTE* sectorSizeN)
{
  // read the ID of whatever sector that is currently passing through the R/W head using the current comm rate
  // on success, return its parameters
  // on failure, this particular call does not write error information or swing the R/W head back and forth
  // - no recalibration is called or signaled at the end, except when no disk was in drive
  if (!m_params)
  {
    return false;
  }
  
  if (!m_initialized || m_lastError)
  {
    recalibrateDrive();
    seekDrive(m_currentTrack, m_currentHead);
  }
       
  m_noDiskInDrive = false;
  motorOn(false);
  setInterrupt();
  
  for (BYTE retries = 0; retries < DISK_OPERATION_RETRIES; retries++)
  {
    m_idle = false;    
    
    // 0x4A Read ID
    sendCommand(0x4A);
    sendData((m_currentHead << 2) | m_params->DriveNumber); // head and drive
    if (!waitForDATA())
    {
      resetController();
      m_idle = true;
      m_lastError = true;
      m_noDiskInDrive = true;
      
      return false;
    }
    
    const BYTE st0 = getData();
    const BYTE st1 = getData();
    getData();
    
    const BYTE _track = getData();
    const BYTE _head  = getData();
    m_currentSector   = getData();
    const BYTE _ssize = getData();
        
    // on success, return values if pointers were provided
    if (!(st0 & 0xC0) || (st1 & 0x80))
    {
      if (track)
      {
        *track = _track;
      }    
      if (head)
      {
        *head = _head;
      }
      if (sector)
      {
        *sector = m_currentSector;
      }
      if (sectorSizeN)
      {
        *sectorSizeN = _ssize;
      }
      
      m_idle = true;
      return true;
    }
  }
  
  m_idle = true;
  return false;
}