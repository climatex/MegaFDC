// MegaFDC (c) 2023-2025 J. Bogin, http://boginjr.com
// User interface

#include "config.h"

#ifdef UI_ENABLED
    
  U8G2_ST7920_128X64_F_8080 display(U8G2_R0, DISP_D0, DISP_D1, DISP_D2, DISP_D3, DISP_D4, DISP_D5, DISP_D6, DISP_D7, DISP_E, U8X8_PIN_NONE, DISP_RS, DISP_RST);
  PS2KeyAdvanced keyboard;
  BYTE g_uiEnabled = UI_LCDKBD; 
  
#else
  BYTE g_uiEnabled = UI_DISABLED;
#endif

// reset by null pointer function call
void (*resetBoard)() = NULL;

// singleton, needs to be constructed during setup()
Ui::Ui()
{
  // initialize serial communication
  pinMode(SWITCH_9600BPS_RATE, INPUT_PULLUP);
  const bool bps115200 = (digitalRead(SWITCH_9600BPS_RATE) == HIGH);  
  Serial.begin(bps115200 ? 115200 : 9600);  
   
  // setup display and keyboard I/O
#ifdef UI_ENABLED
  m_delayOnScreenFill = 1250; // ms
  m_cursorFlipFlop = false;  
  
  // ui->print() allowed over serial and display
  m_printDisabled = false;
  m_printOverSerialDisabled = false;
  
  // first line: [0,CHAR_height-1]
  m_currentColumn = 0;
  m_cursorX = 0;
  m_cursorY = CHAR_HEIGHT - 1;
  m_printLength = 0;
  
  // UI can be optionally switched off by pulling this pin low during startup
  pinMode(SWITCH_DISABLE_UI, INPUT_PULLUP);
  if (digitalRead(SWITCH_DISABLE_UI) == LOW)
  {
    g_uiEnabled = UI_DISABLED;    
  }
  
  // if compiled with support, but disabled by switch, just use this to display "LCD and keyboard unused"
  display.begin();
  display.setFont(UI_FONT);
    
  // UI_LCD or UI_LCDKBD
  if (g_uiEnabled)
  {   
    // use serial input on no keyboard present
    if (!detectKeyboard())
    {
      g_uiEnabled = UI_LCD;
    }
  }
  
  // UI_DISABLED
  else
  {
    display.drawStr(0, 25, Progmem::getString(Progmem::uiDisabled1));
    display.drawStr(0, 45, Progmem::getString(Progmem::uiDisabled2));
    display.sendBuffer();
  }
#endif
}

// reset board
void Ui::reset()
{
  // clear the screen
  print("");
  
  // de-selects the FDC lines and pulls its hardware RESET line high
  // this helps the controller to reinitialize after a sofware reset
  PORTC = 0x78;
  
  // software reset by null pointer function call
  resetBoard();
}

// detect keyboard during setup
bool Ui::detectKeyboard()
{ 
#ifdef UI_ENABLED
 
  BYTE attempts = 5;
  while (attempts)
  {
    // re-initialize
    pinMode(KEYB_CLK, OUTPUT);
    digitalWrite(KEYB_CLK, HIGH);    
    DELAY_MS(10);
    
    keyboard.begin(KEYB_DATA, KEYB_CLK);    
    keyboard.resetKey(); // send 0xFF
    
    const DWORD timeBefore = millis();
    while (millis() - timeBefore < 100)
    {
      if (keyboard.read())
      {
        // got a response, set NumLock
        keyboard.setLock(PS2_LOCK_NUM);
        return true;
      }
    }
        
    detachInterrupt(digitalPinToInterrupt(KEYB_CLK));
    attempts--;
  }
   
#endif
  return false;
}

// prints a null terminated string, supporting printf variadics (str != getPrintBuffer())
// or a string of custom m_printLength, with 0s and invalid characters skipped (str == getPrintBuffer())
void Ui::print(const BYTE* str, ...)
{
  // null pointer, or print has been disabled - do not print anything
  if (!str || m_printDisabled)
  {
    return;
  }
     
  // if the input buffer is not the same as internal, do variadic expansion normally
  if (str != m_printBuffer)
  {
    va_list args;
    va_start(args, str);
    vsnprintf(m_printBuffer, sizeof(m_printBuffer), str, args);
    va_end(args);
  }
    
  // autoset print length, or if using custom, check bounds
  if (!m_printLength)
  {
    m_printLength = strlen(m_printBuffer);
  }  
  else if (m_printLength > MAX_CHARS)
  {
    m_printLength = MAX_CHARS;
  }
   
  // if no interaction with the display, just print over serial 
  if (!g_uiEnabled)
  {
    // disabled print over serial? (during XMODEM transfers)
    if (m_printOverSerialDisabled)
    {
      return;
    }
    
    // clear display (print called with empty string) ?
    if (!m_printLength)
    {
#ifdef CLEAR_SCREEN_SERIAL
      // try VT100 clear screen, erase what stayed behind if the sequence was unsupported, and a CRLF - at least one will work
      Serial.print((const char*)Progmem::getString(Progmem::uiVT100ClearScreen));
#endif
      Serial.print((const char*)Progmem::getString(Progmem::uiNewLine)); // or just a newline
    }
    
    else
    {
      Serial.write(m_printBuffer, m_printLength);
      m_printLength = 0; // reset print length  
    }   
    
    return;
  }
  
#ifdef UI_ENABLED  
  // shorthand: print with an empty string will clear the whole display
  if (!m_printLength)
  {
    // reset to 0
    m_currentColumn = m_cursorX = 0;
    m_cursorY = CHAR_HEIGHT-1;    
    m_printLength = 0;
    
    display.clearBuffer();
    display.sendBuffer();    
    return;
  }
  
  // if cursor is at on state, draw it off (skip timer)
  if (m_cursorFlipFlop)
  {
    blinkCursor(true);
  }
  
  DWORD index = 0;  
  while (index < m_printLength)
  {
    BYTE chr = m_printBuffer[index++];
    
    // backspace
    if (chr == '\b')
    {    
      // check if there is anything to delete
      if (!m_currentColumn && (m_cursorY == CHAR_HEIGHT-1))
      {
        continue;
      }
      
      // on a new line? go up
      if (!m_currentColumn)
      {
        m_currentColumn = MAX_COLS;
        m_cursorX = MAX_COLS*CHAR_WIDTH;
        m_cursorY -= CHAR_HEIGHT;
      }
      
      // do back space and erase 1 character
      m_cursorX -= CHAR_WIDTH;
      m_currentColumn--;
      display.drawGlyph(m_cursorX, m_cursorY, ' ');   
      
      continue;
    }
    
    // detect end-of-line, CR and LF
    else if ((m_currentColumn == MAX_COLS) || (chr == '\r') || (chr == '\n'))
    {       
      // end of line: CR, LF
      if (m_currentColumn == MAX_COLS)
      {
        m_currentColumn = m_cursorX = 0;
        m_cursorY += CHAR_HEIGHT;        
      }
      
      // CR, return cursor home
      else if (chr == '\r')
      {
        m_currentColumn = m_cursorX = 0;        
      }
      
      // LF, line feed only
      else if (chr == '\n')
      {
        m_cursorY += CHAR_HEIGHT;  
      }      
      
      // screen is full?
      if (m_cursorY > MAX_LINES*CHAR_HEIGHT)
      {
        display.sendBuffer();
        
        // halt for a while to be seen?
        delay(m_delayOnScreenFill);
        
        // continue with a clean screen
        // scrolling each line after reaching the bottom would be slow
        display.clearBuffer();
        m_cursorY = CHAR_HEIGHT-1;
      }
      
      // CR and LF do not do anything more
      if ((chr == '\r') || (chr == '\n'))
      {
        continue;
      }
    }
      
    // Escape as a character ignored, also all other + extended ASCII characters,
    // which are unprintable on a tiny display with its limited font
    else if ((chr < 0x20) || (chr > 0x7E))
    {
      continue;
    }
        
    // any other character - display it and advance cursor & column in line 
    display.drawGlyph(m_cursorX, m_cursorY, chr);
    m_currentColumn++;
    m_cursorX += CHAR_WIDTH;        
  }
  
  // update display when done
  display.sendBuffer(); 
  m_printLength = 0; // reset print length
#endif  
}

BYTE Ui::readKey(const BYTE* allowedKeys, bool withWait)
{
  // read key (without echo); withWait false: just check if that key is available
  static bool shiftState = false;
  
  // support ctrl+alt+del :-) from physical keyboard
  static bool ctrlState = false;
  static bool altState = false;
  
  // check for allowed keys; null pointer means all (supported) are allowed
  bool checkAllowed = (allowedKeys != NULL);
  
  while(true)
  {
    // blink the cursor on LCD during keyboard wait
    blinkCursor();
    
    // read keys through serial?
    if (g_uiEnabled != UI_LCDKBD)
    { 
      int read = Serial.read();
      if (read == -1)
      {
        READKEY_CHECK_WAIT;
      }
      
      // filter out invalid characters
      else if (read == 0x7f)
      {
        read = 0x08; // treat DEL as backspace if it came thru serial
      }
      else if (read > 0x7e)
      {  
        READKEY_CHECK_WAIT;
      }
      // allow CR (ENTER), Escape, and backspace
      else if ((read < 0x20) && (read != 0x0D) && (read != 0x1B) && (read != 0x08))
      { 
        READKEY_CHECK_WAIT;
      }
      
      BYTE chr = (BYTE)read;
      
      // no check for certain keys? return here
      if (!checkAllowed)
      {
        return chr;
      }
      
      // go thru the allowed characters string, re-read keyboard if key not in there
      for (WORD index = 0; index < strlen(allowedKeys); index++)
      {
        if (toupper(chr) == toupper(allowedKeys[index]))
        {
          return chr;
        }
      }
      
      READKEY_CHECK_WAIT;
    }
    
#ifndef UI_ENABLED
  }
#else
    
    // read key information from physical keyboard
    WORD key = keyboard.read();   
    if (key == 0)
    {
      READKEY_CHECK_WAIT;
    }

    // detect shift states depending on keypress or release
    ctrlState = key & PS2_CTRL;
    altState = (key & PS2_ALT) || (key & PS2_ALT_GR);
    shiftState = key & PS2_SHIFT;
        
    // process keys, keyrelease of other ignored
    if (key & PS2_BREAK)
    {
      READKEY_CHECK_WAIT;
    }
    
    // translate key to ASCII character, zero if no match - repeat key read
    // 256byte table in progmem
    BYTE xlate = Progmem::keybToChar((BYTE)key);
    if (xlate == 0)
    {
      READKEY_CHECK_WAIT;
    }
    
    // special case: Delete keypress with ctrl and alt already on - call reset
    if (xlate == 0xFF)
    {
      if (ctrlState && altState && !shiftState)
      {
        reset();
      }
      
      //otherwise just treat it as a backspace (no cursor keys support)
      xlate = '\b';
    }
    
    // Caps lock
    if (key & PS2_CAPS)
    {
      xlate = toupper(xlate);
    }
    
    // apply shift to character
    if (shiftState)
    {
      xlate = xlateShift(xlate);
    }
    
    // no check for certain keys? return here
    if (!checkAllowed)
    {
      return xlate;
    }
    
    // go thru the allowed characters string, re-read keyboard if key not in there
    for (WORD index = 0; index < strlen(allowedKeys); index++)
    {
      if (toupper(xlate) == toupper(allowedKeys[index]))
      {
        return xlate;
      }
    }
    
    READKEY_CHECK_WAIT;
  }
#endif
}

// prompt for string with a maximum length if set; allowed keys (if not null) shall contain at least \r\b
const BYTE* Ui::prompt(BYTE maximumPromptLen, const BYTE* allowedKeys, bool escReturnsNull)
{ 
  // buffer overflow check
  if (!maximumPromptLen || (maximumPromptLen > (sizeof(m_promptBuffer)-1)))
  {
    maximumPromptLen = sizeof(m_promptBuffer)-1;
  }
  
#ifdef UI_ENABLED
  // if on last line and on display, limit the end of the prompt
  if (g_uiEnabled && isOnLastLine())
  { 
    const WORD remaining = MAX_COLS-1-m_currentColumn;
    if (maximumPromptLen > remaining)
    {
      maximumPromptLen = remaining;
    }
  }
#endif
  
  // prompt for a command
  BYTE index = 0;
  memset(m_promptBuffer, 0, sizeof(m_promptBuffer));
  
  while(true)
  {
    BYTE chr = readKey(allowedKeys);
       
    // ENTER or CR on terminal: confirm prompt
    if (chr == '\r')
    {
      break;
    }
      
    // ESC, if allowed
    else if (chr == '\e')
    {
      // return as if prompt canceled
      if (escReturnsNull)
      {
        return NULL;
      }
      
      // default: cancel out current command and make a newline (like in DOS)      
      while (index > 0)
      {
        m_promptBuffer[--index] = 0;
      }
            
      // echo backslash with a newline
      print("\\\r\n");      
      continue;
    }
    
    // backspace - shorten the string by one, do not erase display outside prompt
    else if (chr == '\b')
    {
      if (index > 0)
      {
        // make sure the backspace is destructive on a serial terminal (it already is on a display)
        print(!g_uiEnabled ? "\b \b" : "\b");        
        m_promptBuffer[--index] = 0;
      }
      
      continue;
    }
    
    // any other key and the buffer is full?
    else if (index == maximumPromptLen)
    {
      continue;
    }
    
    // echo and store prompt buffer
    print("%c", chr);   
    m_promptBuffer[index++] = chr;    
  }
  
  return (const BYTE*)&m_promptBuffer;
}

void Ui::blinkCursor(bool skipTimer)
{
#ifdef UI_ENABLED
  if (g_uiEnabled)
  {
    // 10Hz cursor blink during keyboard wait
    static DWORD prevTime = millis();
    
    if (skipTimer || (millis() > prevTime+100))
    {
      m_cursorFlipFlop = !m_cursorFlipFlop;    
      display.setDrawColor(m_cursorFlipFlop ? 1 : 2);
      
      // normal behavior
      if (m_currentColumn < MAX_COLS)
      {
        display.drawLine(m_cursorX, m_cursorY, m_cursorX+CHAR_WIDTH-2, m_cursorY);  
      }
      
      // blink the cursor on the next line, if the display is not full
      else if (m_cursorY < MAX_LINES*CHAR_HEIGHT)
      {
        display.drawLine(0, m_cursorY+CHAR_HEIGHT, CHAR_WIDTH-2, m_cursorY+CHAR_HEIGHT);
      }
      
      display.setDrawColor(m_cursorFlipFlop ? 2 : 1);    
      display.sendBuffer();    
      prevTime = millis();
    }  
  }
#endif
}

BYTE Ui::xlateShift(BYTE key)
{
  if ((key >= 'a') && (key <= 'z'))
  {
    return toupper(key);
  }
  
  switch(key)
  {
  case '`':
    return '~';
  case '1':
    return '!';
  case '2':
    return '@';
  case '3':
    return '#';
  case '4':
    return '$';
  case '5':
    return '%';
  case '6':
    return '^';
  case '7':
    return '&';
  case '8':
    return '*';
  case '9':
    return '(';
  case '0':
    return ')';
  case '-':
    return '_';
  case '=':
    return '+';
  case '[':
    return '{';
  case ']':
    return '}';
  case ';':
    return ':';
  case ',':
    return '<';
  case '.':
    return '>';
  case '\'':
    return '"';
  case '\\':
    return '|';
  case '/':
    return '?';
  default:
    return key;
  }
}

// disable physical keyboard in order not to trigger INTs during data transfers - can lead to data overruns
void Ui::disableKeyboard(bool disable)
{
#ifdef UI_ENABLED

  // remember key locks
  static BYTE lock = PS2_LOCK_NUM;
  if (disable)
  {
    lock = keyboard.getLock();
    pinMode(KEYB_CLK, OUTPUT);
    digitalWrite(KEYB_CLK, LOW);
  }
  else
  {
    pinMode(KEYB_CLK, INPUT_PULLUP);
    keyboard.resetBuffersStates();
    keyboard.setLock(lock); // restore locks
  }

#endif
}