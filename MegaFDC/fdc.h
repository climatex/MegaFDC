// MegaFDC (c) 2023 J. Bogin, http://boginjr.com
// DP8473 floppy disk controller interface
// Software compatible with a NEC uPD765 or any other PC FDC

#pragma once
#include "config.h"

// compile-time clock delay
#define DELAY_CYCLES(n) __builtin_avr_delay_cycles(n);
#define DELAY_MS(n)     DELAY_CYCLES(16000*n);

// FDC address registers
#define DCR 2 // drive control register (W only)
#define MSR 4 // main status register (R only)
#define DTR 5 // data register (R/W)
#define DRR 7 // data rate register (W) / disk change info, bit 0x80 (R)

// inlined functions to query values from the FDC
inline BYTE readRegister(BYTE reg) __attribute__((always_inline));
inline void writeRegister(BYTE reg, BYTE value) __attribute__((always_inline));

// read 1 byte from chosen register
BYTE readRegister(BYTE reg)
{
  PORTC = (PORTC & 0xF8) | reg;   // set 3 bits of address lines
  DDRA = 0;                       // set data lines as input
  PORTC ^= 0x10;                  // toggle /RD
  DELAY_CYCLES(2);                // give it time to populate
  const BYTE value = PINA;        // read 8 bits of data lines
  PORTC ^= 0x10;                  // toggle /RD
   
  return value;
}

// write 1 byte to chosen register
void writeRegister(BYTE reg, BYTE value)
{ 
  PORTC = (PORTC & 0xF8) | reg;   // set 3 bits address lines
  DDRA = 0xFF;                    // set data lines as output
  PORTC ^= 0x20;                  // toggle /WR
  PORTA = value;                  // write 8 bits to data lines
  DELAY_CYCLES(2);                // delay two cycles
  PORTC ^= 0x20;                  // toggle /WR
}

class FDC
{
public:
  static FDC* get()
  {
    static FDC fdc;
    return &fdc;
  }
  
  // POD struct about disk, drive and logical media in it, 26 bytes
  struct DiskDriveMediaParams
  {
    BYTE DriveNumber;
    BYTE DriveInches;
    BYTE Tracks;
    BYTE Heads;
    WORD SectorSizeBytes;
    BYTE SectorsPerTrack;
    BYTE GapLength;
    BYTE Gap3Length;
    BYTE LowLevelFormatFiller;
    BYTE SRT;
    WORD HLT;
    WORD HUT;    
    WORD CommRate;
    bool FM;
    bool DoubleStepping;
    bool DiskChangeLineSupport;
    bool UseFAT12;
    bool UseCPMFS;
    BYTE FATMediaDescriptor;
    BYTE FATRootDirEntries;
    WORD FATClusterSizeBytes;
  };
  
  // inquire status
  bool isIdle() { return m_idle; }
  bool isMotorOn() { return m_motorOn; }
  bool isDiskChanged();
  bool wasDiskChangeInquired() { return m_diskChangeInquired; }
  DiskDriveMediaParams* getParams() { return m_params; }
  
  // errors signaled when all retry attempts were exhausted; no disk/write protected: only 1 attempt
  bool getLastError() { return m_lastError; }
  bool wasErrorNoDiskInDrive() { return m_noDiskInDrive; }
  bool wasErrorDiskProtected() { return m_diskWriteProtected; }
  
  // CHS and sizes
  BYTE getCurrentTrack() { return m_currentTrack; }
  BYTE getCurrentHead() { return m_currentHead; }
  BYTE getCurrentSector() { return m_currentSector; }
  void convertLogicalSectorToCHS(WORD logicalSector, BYTE& track, BYTE& head, BYTE& sector);
  WORD getTotalSectorCount();
  BYTE getMaximumSectorCountForRW(BYTE startSector, WORD operationBytes = 0);
  
  // floppy drive operations
  void motorOff();
  void resetController();
  void recalibrateDrive();
  void seekDrive(BYTE track, BYTE head);
  WORD readWriteSectors(bool writeOperation, BYTE startSector, BYTE endSector, WORD* dataPosition = NULL);
  void formatTrack();
  WORD verifyTrack();
  bool verifyTrack0(bool beforeWriteOperation = false);
  void setActiveDrive(DiskDriveMediaParams* newParams);
  void setAutomaticMotorOff(bool enabled = true);
  
private:  
  FDC();
  
  void motorOn(bool withDelay = true); // automatic
  void setInterrupt(BYTE operation = 1);
  void waitForINT();
  bool waitForDATA();
  BYTE getData();
  void sendData(BYTE data);  
  void sendCommand(BYTE command);
  BYTE convertSectorSize(WORD sectorSize);
  bool processIOResult(BYTE st0, BYTE st1, BYTE st2, BYTE endSectorNo);
  void fatalError(BYTE message);
  
  DiskDriveMediaParams* m_params;
  BYTE m_currentTrack;
  BYTE m_currentHead;
  BYTE m_currentSector;
  bool m_idle;
  bool m_initialized;
  bool m_motorOn;
  bool m_noDiskInDrive;
  bool m_diskWriteProtected;
  bool m_diskChangeInquired;
  bool m_lastError;
};