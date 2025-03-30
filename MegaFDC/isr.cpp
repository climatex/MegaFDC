// MegaFDC (c) 2023-2025 J. Bogin, http://boginjr.com
// Floppy disk controller interrupt service routines

#include "config.h"

// defined in FDC
extern volatile BYTE g_rwBuffer[SECTOR_BUFFER_SIZE]; // shared by xmodem, fatfs, cpm
extern volatile BYTE intFired;
extern volatile WORD dataPos;

// FDC interrupt service routines: acknowledge, read, write, verify, determined by FDC::setInterrupt()
// these are split into 4 separate, to decrease time spent in ISR

// acknowledge interrupt fired; zero: false, nonzero: true
void FDCACK()
{
  intFired = 1;
}

// FDC data stream interrupt for read into global buffer
void FDCREAD()
{
  while (true) // both one-shot or with FIFO enabled
  {
    const BYTE msr = readRegister(MSR);
  
    // handshaking - data ready, write into buffer and advance position
    if ((msr & 0xA0) == 0xA0)
    {
      g_rwBuffer[dataPos++] = readRegister(DTR);
    }
      
    // handshaking end of transfer - bits 7, 6 set, 5 cleared; return and read result phase
    else if ((msr & 0xE0) == 0xC0)
    {
      intFired = 1;
      return;
    }
  }
}

// FDC verify - read register without writing, position counter incremented
void FDCVERIFY()
{
  while (true)
  {
    const BYTE msr = readRegister(MSR);
    if ((msr & 0xA0) == 0xA0)
    {
      readRegister(DTR);
      dataPos++;
    }

    else if ((msr & 0xE0) == 0xC0)
    {
      intFired = 1;
      return;
    } 
  }
}

// FDC data stream interrupt for write from global buffer
void FDCWRITE()
{
  while (true)
  {
    const BYTE msr = readRegister(MSR);
    if ((msr & 0xA0) == 0xA0)
    {
      writeRegister(DTR, g_rwBuffer[dataPos++]);
    }
    
    else if ((msr & 0xE0) == 0xC0)
    {
      intFired = 1;
      return;
    }  
  }  
}
