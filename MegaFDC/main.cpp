// MegaFDC (c) 2023 J. Bogin, http://boginjr.com
// Entry point

#include "config.h"

// user interface
Ui* ui = NULL;

// floppy disk controller
FDC* fdc = NULL;

void setup()
{
  ui = Ui::get();
  fdc = FDC::get();
  
  InitializeDrives();
}

void loop()
{ 
  ProcessCommands();
}