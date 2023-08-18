// MegaFDC (c) 2023 J. Bogin, http://boginjr.com
// Build and wiring configuration

#pragma once

// conditional build; comment out line to disable display and keyboard support
#define UI_ENABLED

/*

  Default pin assignments to a Mega2560 rev3 board
  FDC board schematic see http://boginjr.com/electronics/lv/mega-fdc
  
  optional AT/PS2 keyboard, pinout configurable
  Vcc, GND              (Vcc, GND)
  CLOCK (KEYB_CLK)      (PORTE5: D03) reconfigure note: must be interrupt capable
  DATA (KEYB_DATA)      (PORTG5: D04)
   
  optional 128x64 parallel display (ST7920, others can be used too). Pinout configurable
  Vcc, GND              (Vcc, GND)
  BLK                   (GND)
  BLA                   (Vcc, break to turn off backlight)
  PSB                   (Vcc, for parallel mode)  
  V0 contrast adjust    (10K trimpot taper, trimpot ends Vcc, GND)
  D0-7                  (PORTL0-7: D49, D48, D47, D46, D45, D44, D43, D42)
  E, RS, RST            (PORTG0-2: D41, D40, D39)  
  
  optional TG43 (/REDWC) line handling: reduced write current past track 43 for 8" drives
  TG43                  (PORTD7: D38)  
  
  optional switch/jumper to communicate only via serial interface even when built with UI_ENABLED
  1st contact           (PORTE3: D05)
  2nd contact           (GND)
  
  optional switch/jumper to switch the communication rate from 115200 bps to 9600 bps
  1st contact           (PORTG5: D06)
  2nd contact           (GND)
   
  Floppy disk controller (pinout fixed)
  Vcc, GND              (Vcc, GND)
  INT                   (PORTE4: D02)
  D0-D7                 (PORTA0-7: D22, D23, D24, D25, D26, D27, D28, D29)
  A0-A2                 (PORTC0-2: D37, D36, D35)
  /CS, /RD, /WR, RESET: (PORTC3-6: D34, D33, D32, D31)

*/

// configurable Dxx pins
#define DISP_D0                49   // display parallel data lines 0 to 7
#define DISP_D1                48
#define DISP_D2                47
#define DISP_D3                46
#define DISP_D4                45
#define DISP_D5                44
#define DISP_D6                43
#define DISP_D7                42
#define DISP_E                 41   // display enable
#define DISP_RS                40   // register select
#define DISP_RST               39   // display reset
#define KEYB_CLK               3    // CLK must be hardware interrupt pin, 2 is wired to the FDC
#define KEYB_DATA              4
#define SWITCH_DISABLE_UI      5    // ground this pin to talk via serial even if built with UI_ENABLED
#define SWITCH_9600BPS_RATE    6    // ground this pin to slow down the serial comm from 115200 to 9600

// just in case
#undef BYTE
#undef WORD
#undef DWORD
#define BYTE uint8_t
#define WORD uint16_t
#define DWORD uint32_t

// internal includes and used libraries
#include <Arduino.h>
#include <avr/sfr_defs.h>
#include <string.h>
#include <EEPROM.h>
#include "src/FatFs/ff.h"
#include "src/XModem/XModem.h"

#ifdef UI_ENABLED
  #include "src/PS2KeyAdvanced/PS2KeyAdvanced.h"
  #include "src/U8g2/U8g2lib.h"
#endif

// UI defines
//#define CLEAR_SCREEN_SERIAL  1                  // uncomment for ui->print("") function (which clears the LCD) to also clear the terminal over serial comm
#define UI_FONT                u8g2_font_4x6_mr   // 128 x 64 display with a 4x6 monospace font
#define CHAR_WIDTH             4
#define CHAR_HEIGHT            7                  // height+1 for one extra empty line between lines + blinking cursor
#define MAX_COLS               32                 // maximum characters per line with chosen font
#define MAX_LINES              9                  // maximum lines
#define MAX_CHARS              MAX_COLS*MAX_LINES // maximum characters shown at once on said display; MAX_CHARS+1 size of print buffer
#define MAX_COMMAND_PROMPT_LEN 50                 // maximum number of characters allowed when prompted for input, MAX+1 size of prompt buffer, min. 25
#define MAX_PROGMEM_STRING_LEN 50                 // maximum number of characters for each string in PROGMEM, MAX+1 size of buffer for pgm_read_ptr()

// FDC defines
#define SECTOR_BUFFER_SIZE     3072               // disk sector I/O buffer, needs to be divisible by 128
                                                  // min. 512B for FAT, and 1024 for 8" drive support and XMODEM-1K
                                                  // 1.44M disk image transfer thru XMODEM-1K vs bufsize: 3072 (3:45), 1024 (5:20), 512 (XMODEM-128, 10min)
#define IO_TIMEOUT             5000               // how many 1ms iterations to wait for an FDC interrupt or RQM response
#define DISK_OPERATION_RETRIES 5                  // number of retries per disk operation (at least 5)

// filesystem defines
#define MAX_PATH               48                 // max path, MAX_PATH+1 size of path buffer

// our common includes
#include "progmem.h"
#include "eeprom.h"
#include "isr.h"
#include "ui.h"
#include "fdc.h"
#include "commands.h"
#include "xmodem.h"
#include "cpm.h"
#include "filesystem.h"

// public globals
// UI and FDC singletons - one UI and floppy controller per system
extern volatile BYTE  g_rwBuffer[SECTOR_BUFFER_SIZE];
extern          BYTE  g_path[MAX_PATH + 1];
extern          bool  g_uiEnabled; 
extern          BYTE  g_numberOfDrives;
extern          FDC::DiskDriveMediaParams* g_diskDrives;
extern          Ui*   ui;
extern          FDC*  fdc;