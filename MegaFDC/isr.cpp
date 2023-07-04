// MegaFDC (c) 2023 J. Bogin, http://boginjr.com
// Floppy disk controller interrupt service routines

#include "config.h"

// defined in FDC
extern volatile BYTE g_rwBuffer[SECTOR_BUFFER_SIZE]; // shared by xmodem, fatfs, cpm
extern volatile BYTE intType; // used locally here
extern volatile BYTE intFired;
extern volatile WORD dataPos;

// FDC interrupt service routines: acknowledge, read, write, verify, determined by FDC::setInterrupt()
// these are split into 4 separate, to decrease time spent in ISR

// NOTE:
// for 1Mbps data rate support (2.88MB drives), FDC_ISR_ASSEMBLY must be defined in config.h
// then, comment out IMPLEMENT_ISR(INT4_vect, EXTERNAL_INT_0) in WInterrupts.c of Arduino core for the Mega 2560
// as including our ISR will cause a redefinition of __vector_5 error of attachInterrupt() API
// just remember to uncomment afterwards, or if building anything else!

#ifndef FDC_ISR_ASSEMBLY

// acknowledge interrupt fired; zero: false, nonzero: true
void FDCACK()
{
  intFired = 1;
}

// FDC data stream interrupt for read into global buffer
void FDCREAD()
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
  }
}

// FDC verify - read register without writing, position counter incremented
void FDCVERIFY()
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
  }
}

// FDC data stream interrupt for write from global buffer
void FDCWRITE()
{
  const BYTE msr = readRegister(MSR);
  if ((msr & 0xA0) == 0xA0)
  {
    writeRegister(DTR, g_rwBuffer[dataPos++]);
  }
  
  else if ((msr & 0xE0) == 0xC0)
  {
    intFired = 1;  
  }
}

// all of the above in naked assembly without Arduino and compiler overhead
// this needs to finish within 6us for a 1Mbps data rate...
// ... just the ISR entry and exit (without code) of an ISR in Arduino, takes around 6us.
// plus the Mega2560 runs at a slower clock rate (16MHz) than the DP8473 FDC (24MHz) :-)
#else

ISR(INT4_vect, ISR_NAKED)
{
  asm volatile (
  "                push r0                \n"  // R0 used to save status register after ISR, store old R0 on stack
  "                in r0,%[sreg]          \n"
  "                push r17               \n"  // R17 used to read MSR/DTR value
  "                push r16               \n"  // R16 used for generic 8-bit work
  "                lds r16,intType        \n"  // determine interrupt type set inside setInterrupt()
  "                cpi r16,5              \n"  // just a quick sanity check here
  "                brsh quit              \n"  // >=5: invalid
  "                cpi r16,2              \n"
  "                brsh readwrite         \n"  // 2, 3, 4: read, verify, write
  "                cpi r16,0              \n"  // 0: invalid
  "                breq quit              \n"
    
  "fired:                                 \n"  // 1: acknowledge interrupt has been fired and quit
  "                ser r16                \n"  // intFired = 0xFF;
  "                sts (intFired),r16     \n"

  "quit:                                  \n"
  "                pop r16                \n"  // restore R16
  "                pop r17                \n"  // restore R17
  "                out %[sreg],r0         \n"  // restore SREG
  "                pop r0                 \n"  // restore R0
  "                reti                   \n"  // return from ISR
  
  "readwrite:                             \n"  // read (2), verify (3), write (4)
  "                push r16               \n"  // read MSR first (R16 on stack again, to determine interrupt type)
  "                clr r16                \n"
  "                out %[ddra],r16        \n"  // DDRA = 0; set data lines as input
  "                ldi r16,0x24           \n"
  "                out %[portc],r16       \n"  // PORTC = 0x24; set address lines to MSR, /RD down
  "                nop                    \n"  // delay for FDC to populate data lines
  "                nop                    \n"
  "                in r17,%[pina]         \n"  // 8-bits of MSR from PINA in R17
  "                ldi r16,0x34           \n"
  "                out %[portc],r16       \n"  // PORTC = 0x34; /RD up
  "                pop r16                \n"  // what was the interrupt type again?
  "                cpi r16,4              \n"
  "                breq check_write       \n"  // it was FDCWRITE
  "                mov r16,r17            \n"  // no, FDCREAD or FDCVERIFY, check handshaking, MSR & 0xA0
  "                andi r16,0xA0          \n"
  "                cpi r16,0xA0           \n"  // data ready?
  "                breq read_dtr          \n"  // yes, read DTR
  
  "check_fired:                           \n"  // no, data not ready, check if end of transfer
  "                andi r17,0xE0          \n"
  "                cpi r17,0xC0           \n"  // bits 7,6 set and 5 cleared?
  "                brne quit              \n"  // no, quit
  "                rjmp fired             \n"  // yes, signal interrupt done
  
  "check_write:                           \n"  // same handshake/end of transfer check as above, with different jump
  "                mov r16,r17            \n"
  "                andi r16,0xA0          \n"
  "                cpi r16,0xA0           \n"
  "                breq advance_buf       \n"  // prepare to write DTR, but advance the buffer pos first
  "                rjmp check_fired       \n" 

  "read_dtr:                              \n"  // read DTR after handshake
  "                ldi r16,0x25           \n"  // PORTC = 0x25; set address lines to DTR, /RD down
  "                out %[portc],r16       \n"
  "                nop                    \n"  // wait for the FDC to populate its lines
  "                nop                    \n"
  "                in r17,%[pina]         \n"  // 8-bits of DTR from PINA in R17
  "                ldi r16,0x35           \n"
  "                out %[portc],r16       \n"  // PORTC = 0x35; /RD up
  
  "advance_buf:                           \n"  // advance buffer position after reading, or before writing
  "                push r30               \n"  // save R18 and R19, and Z index registers R30, R31
  "                push r31               \n"  // R31:R30 used to address g_rwBuffer[], R19:R18 for the dataPos value
  "                push r18               \n"
  "                push r19               \n"
  "                ldi r30,lo8(g_rwBuffer)\n"  // load Z index with 16-bit address of g_rwBuffer[] lo byte
  "                ldi r31,hi8(g_rwBuffer)\n"  // Z index hi byte
  "                lds r18,(dataPos)      \n"  // load 16-bit value of dataPos, lo byte
  "                lds r19,(dataPos+1)    \n"  // hi byte
  "                lds r16,intType        \n"
  "                cpi r16,3              \n"  // was intType 3 - verify?
  "                breq after_buf_io      \n"  // yes - just advance the datapos, skip buffer I/O
  "                add r30,r18            \n"  // no - add current dataPos value to buffer address
  "                adc r31,r19            \n"  // with carry, if we cross the 8bit boundary
  "                cpi r16,4              \n"  // write operation?
  "                breq write_dtr         \n"  // yes
  "                st Z,r17               \n"  // no, g_rwBuffer[dataPos] = DTR;

  "after_buf_io:                          \n"
  "                ldi r16,1              \n" 
  "                add r18,r16            \n"  // dataPos++, increment with carry (WORD dataPos)
  "                adc r19,r1             \n"  // R1 is a 0-register, carry
  "                sts (dataPos),r18      \n"
  "                sts (dataPos+1),r19    \n"  // store from registers to memory
  "                pop r19                \n"  // restore clobbered registers
  "                pop r18                \n"
  "                pop r31                \n"
  "                pop r30                \n"
  "                rjmp quit              \n"  // done
    
  "write_dtr:                             \n"  
  "                ld r17,Z               \n"  // DTR = g_rwBuffer[dataPos];
  "                ser r16                \n"  // DDRA = 0xFF, set data lines as output
  "                out %[ddra],r16        \n"
  "                ldi r16,0x15           \n"  // PORTC = 0x15; set address lines to DTR, /WR down
  "                out %[portc],r16       \n"
  "                out %[porta],r17       \n"  // write DTR
  "                nop                    \n"  // give it time
  "                nop                    \n"
  "                ldi r16,0x35           \n"
  "                out %[portc],r16       \n"  // PORTC = 0x35; /WR up
  "                rjmp after_buf_io      \n"  // increment dataPos, restore registers and quit
  
  ::
  [sreg] "I" (_SFR_IO_ADDR(SREG)),             // let the inline assembler parse input constants used for ports at compile time,
  [ddra] "I" (_SFR_IO_ADDR(DDRA)),             // as these have offsets around them
  [pina] "I" (_SFR_IO_ADDR(PINA)),
  [porta] "I" (_SFR_IO_ADDR(PORTA)),
  [portc] "I" (_SFR_IO_ADDR(PORTC))
  );
}

#endif