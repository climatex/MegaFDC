// MegaFDC (c) 2023-2024 J. Bogin, http://boginjr.com
// User interface

#pragma once
#include "config.h"

// next line pause - without delay on screen fill, since we waited for a key here
#define NEXT_LINE_PAUSE if (g_uiEnabled && ui->isOnLastLine()) { \
                          ui->print(Progmem::getString(Progmem::uiContinue)); \
                          ui->readKey(Progmem::getString(Progmem::uiEnterKey)); \
                          ui->print(""); }

// readkey with wait or without
#define READKEY_CHECK_WAIT if (withWait) continue; else return 0;

class Ui
{
public:
  static Ui* get()
  {
    static Ui ui;
    return &ui;
  }
  
  void reset();
  
  void print(const BYTE* str, ...);
  BYTE readKey(const BYTE* allowedKeys = NULL, bool withWait = true);
  const BYTE* prompt(BYTE maximumPromptLen = 0, const BYTE* allowedKeys = NULL, bool escReturnsNull = false);
  
  bool isOnLastColumn() { return m_currentColumn >= MAX_COLS; }
  bool isOnLastLine() { return m_cursorY >= CHAR_HEIGHT*(MAX_LINES-1); }
  
  WORD getDelayOnScreenFill() { return m_delayOnScreenFill; }
  void setDelayOnScreenFill(WORD delayms) { m_delayOnScreenFill = delayms; }
    
  const BYTE* getPromptBuffer() { return (const BYTE*)&m_promptBuffer[0]; }
  BYTE* getPrintBuffer() { return &m_printBuffer[0]; }
  void setPrintLength(WORD length) { m_printLength = length; } 
  
  void disableKeyboard(bool disabled);
  void setPrintDisabled(bool all, bool overSerial) { m_printDisabled = all; m_printOverSerialDisabled = overSerial; }
  
private:  
  Ui();
  BYTE xlateShift(BYTE key);
  void blinkCursor(bool skipTimer = false);
  
  BYTE m_currentColumn;
  WORD m_cursorX;
  WORD m_cursorY;
  BYTE m_printBuffer[MAX_CHARS + 1];
  BYTE m_promptBuffer[MAX_COMMAND_PROMPT_LEN + 1];
  WORD m_printLength;
  
  WORD m_delayOnScreenFill;
  bool m_printDisabled;
  bool m_printOverSerialDisabled;
  bool m_cursorFlipFlop;
};