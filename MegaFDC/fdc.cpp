// MegaFDC (c) 2023 J. Bogin, http://boginjr.com
// DP8473 floppy disk controller interface
// Software compatible with a NEC uPD765 or any other PC FDC

#include "config.h"

// compile-time clock delay
#define DELAY_CYCLES(n) __builtin_avr_delay_cycles(n);
#define DELAY_MS(n)     DELAY_CYCLES(16000*n);

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
   
  // current CHS values
  m_currentTrack = 0;
  m_currentHead = 0;
  m_currentSector = 1;  
   
  // initialize FDC by doing a hardware reset + also set port directions to default:
  // data lines (PORTA) set as output, address lines + RESET/RD/WR/TC (PORTC) set as output
  cli();
  DDRC = 0xFF;
  DDRA = 0xFF;
  PORTA = 0;
  
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
  WORD timeout = IO_TIMEOUT;
  
  while (timeout)
  {
    // inspect main status register, receive byte only if RQM=1 and direction is FDC->AVR
    if ((readRegister(MSR) & 0xC0) == 0xC0)
    {
      return readRegister(DTR);
    }
    
    timeout--;
    DELAY_MS(1);
  }
  
  // no picture, no sound
  m_idle = true;
  fatalError(Progmem::errRQMTimeout);
  
  return 0; // won't return - just to keep compiler happy
}

void FDC::sendData(BYTE data)
{  
  //send data to FDC DTR with handshaking and timeout
  WORD timeout = IO_TIMEOUT;
  
  while (timeout) 
  {
    // send byte only if RQM=1 and direction is AVR->FDC
    if ((readRegister(MSR) & 0xC0) == 0x80)
    {
      writeRegister(DTR, data);
      return;
    }
    
    timeout--;
    DELAY_MS(1);
  }
  
  // same as above
  m_idle = true;
  fatalError(Progmem::errRQMTimeout);
}

// waits until interrupt from FDC
void FDC::waitForINT()
{
  WORD timeout = IO_TIMEOUT;

  while (timeout)
  {
    if (intFired)
    {
      // interrupt as a command result needs to be "sensed" with cmd 0x8
      intFired = 0;
      m_lastError = false;
      return;        
    }

    timeout--;
    DELAY_MS(1);
  }
  
  // the FDC is deaf as a post
  m_idle = true;
  fatalError(Progmem::errRQMTimeout);
}

// similar to waitForINT; allows continuing after timeout
bool FDC::waitForDATA()
{
  WORD timeout = IO_TIMEOUT; 
  
  while(timeout)
  {
    if (intFired)
    {   
      intFired = 0; //reset flag
      return true;
    }
    
    timeout--;
    DELAY_MS(1);
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
  
  cli();
#ifndef FDC_ISR_ASSEMBLY
  detachInterrupt(2);
#endif
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
  
  // if setActiveDrive was not called yet, return
  if (!m_params)
  {
    return;
  }
  
  // set communication (data) rate in kbps
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
  else // 500
  {
    writeRegister(DRR, 0);
  }  

  // execute FDC command 0x3 - Specify
  // values per https://www.isdaman.com/alsos/hardware/fdc/floppy.htm
  sendCommand(3);
  
  // first byte is SRT (upper nibble) | HUT (lower nibble)
  // second byte is HLT (bits 7-1) | non-DMA mode flag (bit 0)
  // non-DMA mode: 1 (no DMA in classic Arduino)
   
  // 8" drives (FDC comm rate always 500K, MFM or FM):
  // 8ms SRT, 256ms head load, 256ms unload time
  if (m_params->DriveInches == 8)
  {
    m_params->SRT = 8; m_params->HLT = 256; m_params->HUT = 256;
    sendData(0x80);
    sendData(1);
  }  
  
  // 5.25" drives (comm rate 250K DD, 500K HD, 300K DD disk in HD drive; always MFM)
  // timings depend on comm rate
  // 6ms SRT, 64ms head load, 256ms unload time DD & HD
  else if (m_params->DriveInches == 5)
  {
    if (m_params->CommRate == 250)
    {
      m_params->SRT = 6; m_params->HLT = 64; m_params->HUT = 256;
      sendData(0xD8);
      sendData(0x21);
    }
    
    // 6.67ms SRT, 63.3ms head load, 266.67ms unload time if DD in HD drive
    else if (m_params->CommRate == 300)
    {
      m_params->SRT = 7; m_params->HLT = 63; m_params->HUT = 267;
      sendData(0xCA);
      sendData(0x27);
    }
    
    else // 500
    {
      m_params->SRT = 6; m_params->HLT = 64; m_params->HUT = 256;
      sendData(0xA0);
      sendData(0x41);
    }
  }
  
  // 3.5" drives (comm rate 250K DD, 500K HD, 1M ED, always MFM)
  // 4ms SRT, 32ms head load, 256ms unload time (except 1Mbps: 128ms HUT)
  else
  {
    if (m_params->CommRate == 250)
    {
      m_params->SRT = 4; m_params->HLT = 32; m_params->HUT = 256;
      sendData(0xE8);
      sendData(0x11);
    }
    
    else if (m_params->CommRate == 1000)
    {
      m_params->SRT = 4; m_params->HLT = 32; m_params->HUT = 128;
      sendData(0x80);
      sendData(0x41);
    }

    else // 500
    {
      m_params->SRT = 4; m_params->HLT = 32; m_params->HUT = 256;
      sendData(0xC0);
      sendData(0x21);
    }
  }
  
  // recalibrate command required to initialize disk operations
  m_idle = true;
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
  
  // seek heads to given track (cylinder) and select head (disk side)
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
          m_currentTrack = track;
          m_currentHead = head;
          
          m_lastError = false;
          m_idle = true;
          return;
        }
      }
    }
  }
  
  m_idle = true;  
  fatalError(Progmem::errSeek);
}

WORD FDC::readWriteSectors(bool writeOperation, BYTE startSector, BYTE endSector, WORD* dataPosition)
{
  // reads/writes chosen sector off current track and head to/from ioBuffer
  // writeOperation false: read, true: write
  // startSector: initial sector number (1-based)
  // endSector: ending sector number (1-based) or getMaximumSectorCountForRW() for maximum possible
  // dataPosition: custom dataPos starting index (optional)
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
    setInterrupt(writeOperation ? 4 : 2);
       
    // 0x46 Read sector : 0x45 Write sector
    sendCommand(writeOperation ? 0x45 : 0x46);    
    sendData((m_currentHead << 2) | m_params->DriveNumber); // head and drive
    sendData(m_currentTrack); // current track
    sendData(m_currentHead); // head again
    sendData(startSector); // which sector to read
    sendData(convertSectorSize(m_params->SectorSizeBytes));  // sector size 0 to 3, 128B to 1024B
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
    getData();
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
  ui->print(ui->getPrintBuffer());
  return 0;
}

void FDC::formatTrack()
{
  if (!m_params)
  {
    return;
  }
  
  // formats current track and head number (without bad sector verify)
  m_currentSector = 1;
  
  // clear and prepare write buffer with 4-byte 'CHSV' format values, for each sector in track
  memset(g_rwBuffer, 0, m_params->SectorsPerTrack * 4);
  dataPos = 0;
  for (BYTE sector = 0; sector < m_params->SectorsPerTrack; sector++)
  {
    g_rwBuffer[dataPos++] = m_currentTrack; // C
    g_rwBuffer[dataPos++] = m_currentHead;  // H
    g_rwBuffer[dataPos++] = sector + 1;     // S (1-based)
    g_rwBuffer[dataPos++] = convertSectorSize(m_params->SectorSizeBytes); // V (value of sector size, 0 to 3)
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
    setInterrupt(4);
          
    // 0x4D Format track
    sendCommand(0x4D);
    sendData((m_currentHead << 2) | m_params->DriveNumber); // head and drive
    sendData(convertSectorSize(m_params->SectorSizeBytes)); // sector size 0 to 3, 128B to 1024B
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
      return;
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
      return; // no errors during I/O
    }
  }
    
  m_idle = true;
  m_lastError = true;
  ui->print(ui->getPrintBuffer());
  return;
}

WORD FDC::verifyTrack()
{
  if (!m_params)
  {
    return;
  }
  
  // reads whole track without storing the buffer
  m_currentSector = 1;
  
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
    setInterrupt(3);
       
    // 0x46 Read sector
    sendCommand(0x46);
    sendData((m_currentHead << 2) | m_params->DriveNumber); // head and drive
    sendData(m_currentTrack); // current track
    sendData(m_currentHead); // head again
    sendData(1); // starting at sector 1
    sendData(convertSectorSize(m_params->SectorSizeBytes));  // sector size 0 to 3, 128B to 1024B
    sendData(m_params->SectorsPerTrack); // read whole track
    sendData(m_params->GapLength); // gap length
    sendData((m_params->SectorSizeBytes == 128) ? 0x80 : 0xFF); // data transfer length
    
    if (!waitForDATA())
    {
      m_idle = true;
      m_lastError = true;
      m_noDiskInDrive = true;
      motorOff();
      
      ui->print(Progmem::getString(Progmem::errINTTimeout));
      return;
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
      return dataPos;
    }
  }
    
  m_idle = true;
  m_lastError = true;
  ui->print(ui->getPrintBuffer());
  return 0;
}

// verify head 0, track 0 readability, useful before beginning filesystem operations
bool FDC::verifyTrack0(bool beforeWriteOperation)
{
  seekDrive(0, 0);
  verifyTrack();
  
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
      ui->print(Progmem::getString(Progmem::diskIoTrack0Error));
            
      // try formatting first?
      if (beforeWriteOperation)
      {
        ui->print(Progmem::getString(Progmem::uiNewLine));
        ui->print(Progmem::getString(Progmem::diskIoTryFormat));
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

// the classic way: attachInterrupt to FDCACK, FDCREAD, FDCVERIFY, FDCWRITE
#ifndef FDC_ISR_ASSEMBLY  
  // changed, do detach before
  if (intType != 0)
  {
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
#else
  // one ISR in assembly: enable INT4
  cli();
  EICRB = (EICRB & ~((1 << ISC40) | (1 << ISC41))) | (RISING << ISC40);
  EIMSK |= (1 << INT4);
  sei();
#endif  
  
  intType = operation;
}

bool FDC::processIOResult(BYTE st0, BYTE st1, BYTE st2, BYTE endSectorNo)
{ 
  m_idle = true;
  
  // process IO operation result, and if there was an error, print it into the ui->print() buffer
  // however, print it out only after all retries failed
  
  // reset these two flags, to be determined here
  m_diskWriteProtected = false;
  m_lastError = false;
  
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

// switch to other drive - set new params pointer
void FDC::setActiveDrive(DiskDriveMediaParams* newParams)
{
  if (!newParams)
  {
    return;
  }
  
  // turn interrupts off, set new parameters, set previous flags default and reset the controller
  cli();
  motorOff();
  
#ifndef FDC_ISR_ASSEMBLY
  detachInterrupt(2);
#endif
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

  setInterrupt();
  sei();

  recalibrateDrive();
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
  
  // double stepping?
  if (m_params->DoubleStepping && (track % 2))
  {
    track++;
  }
}

// get number of sectors total
WORD FDC::getTotalSectorCount()
{
  if (!m_params)
  {
    return 0;
  }
  
  WORD tracks = m_params->Tracks;
  if (m_params->DoubleStepping)
  {
    tracks /= 2;
  }
  
  return tracks * m_params->SectorsPerTrack * m_params->Heads;
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

// translate sector sizes in bytes to indexed 0-3 for the FDC internally
BYTE FDC::convertSectorSize(WORD sectorSize)
{
  switch(sectorSize)
  {
  case 128:
    return 0;
  case 256:
    return 1;
  case 512:
  default:
    return 2;
  case 1024:
    return 3;
  }
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