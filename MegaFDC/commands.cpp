// MegaFDC (c) 2023 J. Bogin, http://boginjr.com
// Command prompt with commands

#include "config.h"

// internal forward declarations
void SetupDrives();
void ToUpper(BYTE* str);
void SetDriveParameters(FDC::DiskDriveMediaParams* drive);
void SetDriveParametersAdvanced(FDC::DiskDriveMediaParams* drive);
BYTE VerifySuppliedDrive(const BYTE* drive);
bool VerifySuppliedPath(const BYTE* pathToCheck);

// some commands that do not depend on the internal loop
void CommandHELP(const BYTE* details);
void CommandDRIVPARM(FDC::DiskDriveMediaParams* drive);
void CommandFORMAT(FDC::DiskDriveMediaParams* drive);
void CommandVERIFY(FDC::DiskDriveMediaParams* drive);
void CommandIMAGE(FDC::DiskDriveMediaParams* drive);
void CommandQFORMAT(FDC::DiskDriveMediaParams* drive, bool dontAskConfirm = false);
void CommandXFER(const BYTE* fileName);

// 1 to 4 disk drive configurations
FDC::DiskDriveMediaParams* g_diskDrives = NULL;
BYTE g_numberOfDrives = 0;

// global path buffer
BYTE g_path[MAX_PATH + 1] = {0};

BYTE key = 0;
BYTE promptFullPathSetting = 0xFF; // not set

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////// hic sunt leones ////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// initialize: setup drives manually or load settings from EEPROM
void InitializeDrives()
{
  ui->print("");
  ui->print(Progmem::getString(Progmem::uiSplash));
    
  // check if the floppy controller is responding to a reset command
  ui->print(Progmem::getString(Progmem::initializingFDC));
  fdc->resetController();
  ui->print(" ");
  ui->print(Progmem::getString(Progmem::uiOK));
  ui->print(Progmem::getString(Progmem::uiNewLine2x));
  
  // check if settings can be loaded
  if (!eepromIsConfigurationPresent())
  {
    SetupDrives();
    return;
  }
  
  // auto load the configuration, give a small delay to break the process with Esc
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
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
    SetupDrives();
    return;
  }  
  
  eepromLoadConfiguration();
  
  // initialize first drive A: and begin with the prompt
  ui->print("");
  ui->print(Progmem::getString(Progmem::uiWaitingForDrives));
  fdc->setActiveDrive(&g_diskDrives[0]);  
}

// manual drive setup
void SetupDrives()
{
  // how many drives are in system?
  ui->print(Progmem::getString(Progmem::countDrives));
  key = ui->readKey("1234");
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  g_numberOfDrives = key - 48;
  
  // allocate memory for params struct, clear it out and inquire about each drive
  g_diskDrives = new FDC::DiskDriveMediaParams[g_numberOfDrives];
  
  for (BYTE diskDrive = 0; diskDrive < g_numberOfDrives; diskDrive++)
  {
    FDC::DiskDriveMediaParams* drive = &g_diskDrives[diskDrive];
    memset(drive, 0, sizeof(FDC::DiskDriveMediaParams)); // initialize with all 0s (the struct is POD)
    drive->DriveNumber = diskDrive; // store logical drive number (0 to 3)
    
    ui->print("");
    ui->print(Progmem::getString(Progmem::specifyParams1), diskDrive + 65);
    ui->print(Progmem::getString(Progmem::specifyParams2));
    ui->print(Progmem::getString(Progmem::driveInches));
    
    //determine drive, 8", 5", 3" or Esc - which triggers advanced config
    key = ui->readKey("853\e");
    if (key == '\e')
    {
      SetDriveParametersAdvanced(drive);
    }
    
    // we already got the type, so fill in the parameters and go with the other function
    else
    {
      drive->DriveInches = key - 48;
      ui->print(Progmem::getString(Progmem::uiEchoKey), key);
      SetDriveParameters(drive);
    }
    
    // some short delay before repeating the process for other drives
    delay(ui->getDelayOnScreenFill());
  }
  
  // initialize first drive A: and begin with the prompt
  ui->print("");
  ui->print(Progmem::getString(Progmem::uiWaitingForDrives));
  fdc->setActiveDrive(&g_diskDrives[0]);
}

// internal loop
void ProcessCommands()
{ 
  while(true)
  {
    // commands - max length 12
    // arguments - only 8.3 file name allowed for all, with a dot and a terminating \0
    BYTE command[12 + 1] = {0};
    BYTE arguments[12 + 1] = {0};
    
    // prompt full path - if FAT12 support enabled and setting not set or enabled
    const bool promptFullPath = fdc->getParams()->UseFAT12 && (promptFullPathSetting > 0);    
    
    // drive letter only
    if (!promptFullPath)
    {
      ui->print("%c>", fdc->getParams()->DriveNumber + 65);  
    }
    
    // prompt with path
    else
    {
      
      // if the prompt is longer than a line, and we're on a last, print it on a clean screen
#ifdef UI_ENABLED
      if (g_uiEnabled && ui->isOnLastLine() && (strlen(g_path)+5 >= MAX_COLS))
      {
        delay(ui->getDelayOnScreenFill());
        ui->print("");
      }
#endif
      ui->print("%c:\\", fdc->getParams()->DriveNumber + 65);
      if (strlen(g_path))
      {
        // don't display the trailing backslash
        BYTE* backslash = &g_path[strlen(g_path)-1];
        *backslash = 0;
        ui->print(g_path);
        *backslash = '\\';
        
      }
      ui->print(">");
    }
    
    // wait for command (12+12 characters and a space)
    sscanf(ui->prompt(25), "%12s %12s", command, arguments);
    ToUpper(command);
    ToUpper(arguments);
        
    // empty command
    if (!strlen(command))
    {
      ui->print(Progmem::getString(Progmem::uiEnterKey)); // CR
      continue;
    }
    
    ui->print(Progmem::getString(Progmem::uiNewLine));
    
    // drive change A: to D:
    if ((strlen(command) > 1) && !strlen(arguments) && (command[1] == ':'))
    {
      // check drive
      BYTE chosenDrive = VerifySuppliedDrive(command);
      if (chosenDrive == 0xFF)
      {
        continue;
      }
      
      // no change?
      if (chosenDrive == fdc->getParams()->DriveNumber)
      {
        continue;
      }
      
      // set new drive and clear path
      fdc->setActiveDrive(&g_diskDrives[chosenDrive]);      
      g_path[0] = 0;           
      continue;
    }
    
    // HELP - give supported commands list or details for specified one
    else if (strcmp(command, Progmem::getString(Progmem::cmdHelp)) == 0)
    {
      CommandHELP(arguments);
      continue;
    }
    
    // PERSIST - store settings to EEPROM or clear them
    else if (strcmp(command, Progmem::getString(Progmem::cmdPersist)) == 0)
    {
      ui->print("");
  
      ui->print(Progmem::getString(Progmem::persistStore));
      ui->print(Progmem::getString(Progmem::persistRemove));
      ui->print(Progmem::getString(Progmem::uiCancelOption));
      key = toupper(ui->readKey("PRC"));  
      ui->print(Progmem::getString(Progmem::uiEchoKey), key);
      
      // persist configuration
      if (key == 'P')
      {
        ui->print(Progmem::getString(Progmem::uiNewLine));
        ui->print(Progmem::getString(Progmem::uiOperationPending));
        
        eepromStoreConfiguration();
        
        ui->print(Progmem::getString(Progmem::uiOK));
        ui->print(Progmem::getString(Progmem::uiNewLine));
      }
      
      // remove configuration
      else if (key == 'R')
      {   
        ui->print(Progmem::getString(Progmem::uiNewLine));
        
        eepromClearConfiguration();
        
        ui->print(Progmem::getString(Progmem::uiOK));
        ui->print(Progmem::getString(Progmem::uiNewLine));
      }
      
      ui->print(Progmem::getString(Progmem::uiNewLine));
      continue;
    }
    
    // RESET - whole board, or configuration of one drive
    else if (strcmp(command, Progmem::getString(Progmem::cmdReset)) == 0)
    {
      // check if a drive was supplied to command
      if (strlen(arguments))
      {
        // yes, check it
        BYTE chosenDrive = VerifySuppliedDrive(arguments);
        if (chosenDrive == 0xFF)
        {
          continue;
        }
        
        // reset settings of 1 particular drive only
        FDC::DiskDriveMediaParams* drive = &g_diskDrives[chosenDrive];
        memset(drive, 0, sizeof(FDC::DiskDriveMediaParams));
        drive->DriveNumber = chosenDrive;
        
        ui->print("");
        ui->print(Progmem::getString(Progmem::specifyParams1), drive->DriveNumber + 65);
        ui->print(Progmem::getString(Progmem::specifyParams2));
        ui->print(Progmem::getString(Progmem::driveInches));
        key = ui->readKey("853\e");
        if (key == '\e')
        {
          SetDriveParametersAdvanced(drive);
        }
        else
        {
          drive->DriveInches = key - 48;
          ui->print(Progmem::getString(Progmem::uiEchoKey), key);
          SetDriveParameters(drive);
        }
        
        ui->print("");
        fdc->setActiveDrive(&g_diskDrives[chosenDrive]);
        continue;
      }
     
      // no, reset whole board
      ui->reset();
    }
    
    // DRIVPARM
    else if (strcmp(command, Progmem::getString(Progmem::cmdDrivParm)) == 0)
    {
      if (strlen(arguments))
      {
        BYTE chosenDrive = VerifySuppliedDrive(arguments);
        if (chosenDrive == 0xFF)
        {
          continue;
        }
        
        CommandDRIVPARM(&g_diskDrives[chosenDrive]);
        ui->print(Progmem::getString(Progmem::uiNewLine));
        continue;
      }
      
      // no drive supplied, show parameters for the active one
      CommandDRIVPARM(fdc->getParams());
      ui->print(Progmem::getString(Progmem::uiNewLine));
      continue;
    }
    
    // FORMAT
    else if (strcmp(command, Progmem::getString(Progmem::cmdFormat)) == 0)
    {
      if (strlen(arguments))
      {
        BYTE chosenDrive = VerifySuppliedDrive(arguments);
        if (chosenDrive == 0xFF)
        {
          continue;
        }
        
        CommandFORMAT(&g_diskDrives[chosenDrive]);
        continue;
      }
      
      CommandFORMAT(fdc->getParams());
      continue;
    }
    
    // VERIFY
    else if (strcmp(command, Progmem::getString(Progmem::cmdVerify)) == 0)
    {
      if (strlen(arguments))
      {
        BYTE chosenDrive = VerifySuppliedDrive(arguments);
        if (chosenDrive == 0xFF)
        {
          continue;
        }
        
        CommandVERIFY(&g_diskDrives[chosenDrive]);
        continue;
      }
      
      CommandVERIFY(fdc->getParams());
      continue;
    }
    
    // IMAGE
    else if (strcmp(command, Progmem::getString(Progmem::cmdImage)) == 0)
    {
      if (strlen(arguments))
      {
        BYTE chosenDrive = VerifySuppliedDrive(arguments);
        if (chosenDrive == 0xFF)
        {
          continue;
        }
        
        CommandIMAGE(&g_diskDrives[chosenDrive]);
        continue;
      }
      
      CommandIMAGE(fdc->getParams());
      continue;
    }
    
    // QFORMAT
    else if (strcmp(command, Progmem::getString(Progmem::cmdQuickFormat)) == 0)
    {     
      // not enabled on this drive
      if (!fdc->getParams()->UseFAT12 && !fdc->getParams()->UseCPMFS)
      {
        ui->print(Progmem::getString(Progmem::processNoFS));
        ui->print(Progmem::getString(Progmem::uiNewLine));
        continue;
      }
      
      if (strlen(arguments))
      {
        BYTE chosenDrive = VerifySuppliedDrive(arguments);
        if (chosenDrive == 0xFF)
        {
          continue;
        }
        
        CommandQFORMAT(&g_diskDrives[chosenDrive]);
        continue;
      }
      
      CommandQFORMAT(fdc->getParams());
      continue;
    }
    
    // PATH, toggle between full path in prompt and drive letter only
    else if (strcmp(command, Progmem::getString(Progmem::cmdPath)) == 0)
    {
      // not enabled on this drive
      if (!fdc->getParams()->UseFAT12)
      {
        ui->print(Progmem::getString(Progmem::processNoFS));
        ui->print(Progmem::getString(Progmem::uiNewLine2x));
        continue;
      }
      
      // not set
      if (promptFullPathSetting == 0xFF)
      {
        promptFullPathSetting = promptFullPath ? 0 : 1;
      }
      
      // toggle
      else
      {
        promptFullPathSetting = promptFullPathSetting ? 0 : 1;
      }      
      continue;
    }
    
    /* CD, and allow also CD. CD.. CD\ */
    else if ((strcmp(command, Progmem::getString(Progmem::cmdCd)) == 0) ||
             (strcmp(command, Progmem::getString(Progmem::varCd1)) == 0) ||
             (strcmp(command, Progmem::getString(Progmem::varCd2)) == 0) ||
             (strcmp(command, Progmem::getString(Progmem::varCd3)) == 0))
    {
      // not available
      if (!fdc->getParams()->UseFAT12)
      {
        ui->print(Progmem::getString(Progmem::processNoFS));
        ui->print(Progmem::getString(Progmem::uiNewLine2x));
        continue;
      }
      
      // CD\ and CD \ <- go to the root directory
      if ((strcmp(command, Progmem::getString(Progmem::varCd3)) == 0) ||
         ((strcmp(command, Progmem::getString(Progmem::cmdCd)) == 0) && (strcmp(arguments, "\\") == 0)))
      {
        g_path[0] = 0;
        
        ui->print(Progmem::getString(Progmem::uiNewLine));
        continue;
      }
      
      // CD.. and CD .. <- go one level up
      else if ((strcmp(command, Progmem::getString(Progmem::varCd2)) == 0) ||
              ((strcmp(command, Progmem::getString(Progmem::cmdCd)) == 0) && (strcmp(arguments, "..") == 0)))
      {
        if (strlen(g_path) > 1)
        {
          // cancel out ending backslash and find the second to last          
          g_path[strlen(g_path)-1] = 0;         
          BYTE* prevBackslash = strrchr(g_path, '\\');
          
          // go back to root directory
          if (!prevBackslash)
          {
            g_path[0] = 0;
          }
          else
          {
            *(prevBackslash+1) = 0;
          }
        }
                
        ui->print(Progmem::getString(Progmem::uiNewLine));
        continue;        
      }
      
      // CD (empty argument), CD. and CD . <- display current directory on new line
      else if (((strcmp(command, Progmem::getString(Progmem::cmdCd)) == 0) && (!strlen(arguments))) ||
                (strcmp(command, Progmem::getString(Progmem::varCd1)) == 0) ||
               ((strcmp(command, Progmem::getString(Progmem::cmdCd)) == 0) && (strcmp(arguments, ".") == 0)))
      {
        ui->print("%c:\\", fdc->getParams()->DriveNumber + 65);
        if (strlen(g_path))
        {
          BYTE* backslash = &g_path[strlen(g_path)-1];
          *backslash = 0;
          ui->print(g_path);
          *backslash = '\\';
        }
        ui->print(Progmem::getString(Progmem::uiNewLine2x));
        continue;
      }
           
      // verify path
      if (!VerifySuppliedPath(arguments))
      {
        ui->print(Progmem::getString(Progmem::processInvalidChr2));
        ui->print(Progmem::getString(Progmem::uiNewLine2x));
        continue;
      }
      
      // CD directory, verify we're not too nested deeply
      // (current path + new directory + backslash after + 8.3 filename with dot)
      const BYTE newDirLen = strlen(arguments);      
      if ((strlen(g_path) + newDirLen + 12) > MAX_PATH)
      {
        ui->print(Progmem::getString(Progmem::processMaxPath));
        ui->print(Progmem::getString(Progmem::uiNewLine2x));
        continue;
      }
      
      // try if current path works
      ui->disableKeyboard(true);
      bool result = fatChdir();
      ui->disableKeyboard(false);
      if (!result)
      {
        continue;
      }
                 
      //append path and backslash, try changing directory
      strncat(g_path, arguments, MAX_PATH);
      strncat(g_path, "\\", MAX_PATH);
      
      ui->disableKeyboard(true);
      result = fatChdir();
      ui->disableKeyboard(false);
      
      if (!result)
      {
        // failed, shorten the path
        g_path[strlen(g_path)-1] = 0;         
        BYTE* prevBackslash = strrchr(g_path, '\\');
        
        // go back to root directory
        if (!prevBackslash)
        {
          g_path[0] = 0;
        }
        else
        {
          *(prevBackslash+1) = 0;
        }
      }
      else
      {
        ui->print(Progmem::getString(Progmem::uiNewLine));
      }

      continue;
    }
    
    // DIR
    else if (strcmp(command, Progmem::getString(Progmem::cmdDir)) == 0)
    {
      
      if (!fdc->getParams()->UseFAT12 && !fdc->getParams()->UseCPMFS)
      {
        ui->print(Progmem::getString(Progmem::processNoFS));
        ui->print(Progmem::getString(Progmem::uiNewLine2x));
        continue;
      }
      
      // provided 1 directory from our working path and on FAT12
      if (strlen(arguments) && !fdc->getParams()->UseCPMFS)
      {
        if (!VerifySuppliedPath(arguments))
        {
          ui->print(Progmem::getString(Progmem::processInvalidChr2));
          ui->print(Progmem::getString(Progmem::uiNewLine2x));
          continue;
        }
        
        // verify we're not too nested deeply (1 char extra for backslash)
        const BYTE newDirLen = strlen(arguments);      
        if ((strlen(g_path) + newDirLen + 1) > MAX_PATH)
        {
          ui->print(Progmem::getString(Progmem::processMaxPath));
          ui->print(Progmem::getString(Progmem::uiNewLine2x));
          continue;
        }
        
        // try if current path works
        ui->disableKeyboard(true);
        bool result = fatChdir();
        ui->disableKeyboard(false);
        if (!result)
        {
          continue;
        }
        
        // change working directory for a while (similar to CD above)
        strncat(g_path, arguments, MAX_PATH);
        strncat(g_path, "\\", MAX_PATH);
        
        ui->disableKeyboard(true);
        if (fatChdir())
        {
          ui->print("");
          fatDirCommand();
        }
        ui->disableKeyboard(false);
        
        // get the path back again to original
        g_path[strlen(g_path)-1] = 0;         
        BYTE* prevBackslash = strrchr(g_path, '\\');
        
        if (!prevBackslash)
        {
          g_path[0] = 0;
        }
        else
        {
          *(prevBackslash+1) = 0;
        }
        
        continue;
      }
      
      // show current directory contents
      ui->print("");
      
      ui->disableKeyboard(true);
      if (fdc->getParams()->UseFAT12)
      {
        fatDirCommand();
      }
      else
      {
        cpmDirCommand();
      }
      ui->disableKeyboard(false);
      continue;
    }
    
    // MD, RD
    else if ((strcmp(command, Progmem::getString(Progmem::cmdMd)) == 0) ||
             (strcmp(command, Progmem::getString(Progmem::cmdRd)) == 0))
    {
      
      if (!fdc->getParams()->UseFAT12)
      {
        ui->print(Progmem::getString(Progmem::processNoFS));
        ui->print(Progmem::getString(Progmem::uiNewLine2x));
        continue;
      }
      
      // 1 directory name
      if (strlen(arguments))
      {
        if (!VerifySuppliedPath(arguments))
        {
          ui->print(Progmem::getString(Progmem::processInvalidChr2));
          ui->print(Progmem::getString(Progmem::uiNewLine2x));
          continue;
        }
        
        ui->disableKeyboard(true);
        
        // remove directory
        if (strcmp(command, Progmem::getString(Progmem::cmdRd)) == 0)
        {
          fatRmdir(arguments);  
        }
        
        // make directory
        else
        {
          fatMkdir(arguments);
        }        

        ui->disableKeyboard(false);
        continue;
      }
      
      ui->print(Progmem::getString(Progmem::processInvalidChr2));
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
      continue;
    }
    
    // DEL, TYPE
    else if ((strcmp(command, Progmem::getString(Progmem::cmdDel)) == 0) ||
             (strcmp(command, Progmem::getString(Progmem::cmdType)) == 0))
    {
      
      if (!fdc->getParams()->UseFAT12 && !fdc->getParams()->UseCPMFS)
      {
        ui->print(Progmem::getString(Progmem::processNoFS));
        ui->print(Progmem::getString(Progmem::uiNewLine2x));
        continue;
      }
      
      // 1 file name
      if (strlen(arguments))
      {
        if (!VerifySuppliedPath(arguments))
        {
          ui->print(Progmem::getString(Progmem::processInvalidChr1));
          ui->print(Progmem::getString(Progmem::uiNewLine2x));
          continue;
        }
        
        ui->disableKeyboard(true);
        
        // FAT12
        if (fdc->getParams()->UseFAT12)
        {
          // DEL
          if (strcmp(command, Progmem::getString(Progmem::cmdDel)) == 0)
          {
            fatDeleteFile(arguments);
          }
          
          // TYPE, clear screen and turn off screen on fill delay
          else
          {
            const WORD screenDelay = ui->getDelayOnScreenFill();
            ui->setDelayOnScreenFill(0);
            
            ui->print("");                        
            fatDumpFile(arguments);            
            ui->setDelayOnScreenFill(screenDelay);
          }          
        }
        
        // CP/M, analog to above
        else
        {
          if (strcmp(command, Progmem::getString(Progmem::cmdDel)) == 0)
          {
            cpmDeleteFile(arguments);
          }
          
          else
          {
            const WORD screenDelay = ui->getDelayOnScreenFill();
            ui->setDelayOnScreenFill(0);
            
            ui->print("");                        
            cpmDumpFile(arguments);            
            ui->setDelayOnScreenFill(screenDelay);
          }
        }        
        
        ui->disableKeyboard(false);
        continue;
      }
      
      ui->print(Progmem::getString(Progmem::processInvalidChr1));
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
      continue;
    }
    
    // TYPEINTO
    else if (strcmp(command, Progmem::getString(Progmem::cmdTypeInto)) == 0)
    {
      
      if (!fdc->getParams()->UseFAT12)
      {
        ui->print(Progmem::getString(Progmem::processNoFS));
        ui->print(Progmem::getString(Progmem::uiNewLine2x));
        continue;
      }
      
      // 1 file name
      if (strlen(arguments))
      {
        if (!VerifySuppliedPath(arguments))
        {
          ui->print(Progmem::getString(Progmem::processInvalidChr1));
          ui->print(Progmem::getString(Progmem::uiNewLine2x));
          continue;
        }
        
        ui->disableKeyboard(true);
        fatWriteTextFile(arguments);
        ui->disableKeyboard(false);
        continue;
      }
      
      ui->print(Progmem::getString(Progmem::processInvalidChr1));
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
      continue;
    }
    
    // XFER
    else if (strcmp(command, Progmem::getString(Progmem::cmdXfer)) == 0)
    {
      
      if (!fdc->getParams()->UseFAT12 && !fdc->getParams()->UseCPMFS)
      {
        ui->print(Progmem::getString(Progmem::processNoFS));
        ui->print(Progmem::getString(Progmem::uiNewLine2x));
        continue;
      }
      
      // 1 file name
      if (strlen(arguments))
      {
        if (!VerifySuppliedPath(arguments))
        {
          ui->print(Progmem::getString(Progmem::processInvalidChr1));
          ui->print(Progmem::getString(Progmem::uiNewLine2x));
          continue;
        }
        
        CommandXFER(arguments);
        continue;
      }
      
      ui->print(Progmem::getString(Progmem::processInvalidChr1));
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
      continue;
    }
        
    // prompt bubbled through - unrecognized command
    ui->print(Progmem::getString(Progmem::processInvalidCmd));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
}

// ask just a few stuff and auto configure the rest of the parameters
void SetDriveParameters(FDC::DiskDriveMediaParams* drive)
{ 
  drive->LowLevelFormatFiller = 0xF6;
  
  // 8", pick one of the geometries
  if (drive->DriveInches == 8)
  {
    // values common to all 8"
    drive->Tracks = 77;
    drive->CommRate = 500;
    
    ui->print("");
    ui->print(Progmem::getString(Progmem::drive8InchText1));
    ui->print(Progmem::getString(Progmem::drive8InchText2));
    ui->print(Progmem::getString(Progmem::drive8InchText3));
    ui->print(Progmem::getString(Progmem::drive8InchText4));
    ui->print(Progmem::getString(Progmem::drive8InchText5));
    ui->print(Progmem::getString(Progmem::drive8InchText6));
    ui->print(Progmem::getString(Progmem::uiChooseOption));
    key = ui->readKey("1234");
    ui->print(Progmem::getString(Progmem::uiEchoKey), key);
    
    switch(key)
    {
    // 26x128B FM, CP/M
    case '1':
      drive->FM = true;
      drive->UseCPMFS = true;
      drive->Heads = 1;
      drive->SectorSizeBytes = 128;
      drive->SectorsPerTrack = 26;
      drive->GapLength = 7;
      drive->Gap3Length = 0x1B;
      drive->LowLevelFormatFiller = 0xE5; // F6 on DD, E5 on SD
      break;
    // 9x512B MFM, FAT12
    case '2':
      drive->UseFAT12 = true;
      drive->Heads = 1;
      drive->SectorSizeBytes = 512;      
      drive->SectorsPerTrack = 9;
      drive->GapLength = 0x1B;
      drive->Gap3Length = 0x54;
      drive->FATMediaDescriptor = 0xF9;
      drive->FATRootDirEntries = 80;
      drive->FATClusterSizeBytes = 1024;
      break;
    // 16x512B MFM, FAT12
    case '3':
    case '4':
      drive->UseFAT12 = true;
      drive->Heads = (key == '3') ? 1 : 2;
      drive->SectorSizeBytes = 512;
      drive->SectorsPerTrack = 16;
      drive->GapLength = 0x18;   //gaps a little lower to fit in 16 sectors
      drive->Gap3Length = 0x50;
      drive->FATMediaDescriptor = 0xF9;
      drive->FATRootDirEntries = 128;
      drive->FATClusterSizeBytes = 512;
      break;   
    }
  }
  
  // 5.25"
  else if (drive->DriveInches == 5)
  {
    drive->UseFAT12 = true;
    drive->SectorSizeBytes = 512;
    
    // except 1.2M
    drive->Tracks = 40;
    drive->GapLength = 0x2A;
    drive->Gap3Length = 0x50;
    drive->CommRate = 250;
    
    // ask if 1 or 2 sides
    ui->print(Progmem::getString(Progmem::drive5InchSides));
    key = ui->readKey("12");
    drive->Heads = (key == '1') ? 1 : 2;
    ui->print(Progmem::getString(Progmem::uiEchoKey), key);
    
    // one side: 160K, 180K formats (8 and 9 spt)
    if (drive->Heads == 1)
    {
      drive->Tracks = 40;
      drive->CommRate = 250;
      drive->FATRootDirEntries = 64;
      drive->FATClusterSizeBytes = 512;
      
      ui->print(Progmem::getString(Progmem::drive5InchSSDD));
      key = ui->readKey("68");
      drive->SectorsPerTrack = (key == '6') ? 8 : 9;
      drive->FATMediaDescriptor = (key == '6') ? 0xFE : 0xFC;
      ui->print(Progmem::getString(Progmem::uiEchoKey), key);      
    }
    
    // two sides: 320K (8spt), 360K (9spt), 1.2M (15spt HD)
    else
    {     
      ui->print(Progmem::getString(Progmem::drive5Inch));
      key = ui->readKey("261");
      ui->print(Progmem::getString(Progmem::uiEchoKey), key);
      
      switch(key)
      {
      case '2':
        drive->SectorsPerTrack = 8;              
        drive->FATMediaDescriptor = 0xFF;
        drive->FATRootDirEntries = 112;
        drive->FATClusterSizeBytes = 1024;
        break;
      case '6':
        drive->SectorsPerTrack = 9;
        drive->FATMediaDescriptor = 0xFD;
        drive->FATRootDirEntries = 112;
        drive->FATClusterSizeBytes = 1024;
        break;
      case '1':
        drive->DiskChangeLineSupport = true;
        drive->SectorsPerTrack = 15;
        drive->CommRate = 500;
        drive->Tracks = 80;
        drive->GapLength = 0x1B;
        drive->Gap3Length = 0x54;
        drive->FATMediaDescriptor = 0xF9;
        drive->FATRootDirEntries = 224;
        drive->FATClusterSizeBytes = 512;
        break;
      }  
    }
    
    // turn on double stepping and switch transfer rate to 300 if the DD disk is in an HD drive
    if (drive->Tracks == 40)
    {
      ui->print(Progmem::getString(Progmem::drive5InDoubleStep));
      key = toupper(ui->readKey("YN"));
      ui->print(Progmem::getString(Progmem::uiEchoKey), key);
      
      if (key == 'Y')
      {
        drive->CommRate = 300;
        drive->DoubleStepping = true;
      }
    }
  }
  
  // 3.5"
  else
  {
    drive->UseFAT12 = true;
    drive->DiskChangeLineSupport = true;
    drive->SectorSizeBytes = 512;
    drive->Tracks = 80;
    drive->Heads = 2;
    
    // 720K (9spt DD), 1.44MB (18spt HD)
    // offer 2.88MB (36spt ED) only if supported - 1Mbps datarate with perpendicular recording
    const BYTE edSupport = (fdc->getSpecialFeatures() & SUPPORT_1MBPS) && 
                           (fdc->getSpecialFeatures() & SUPPORT_PERPENDICULAR);
                           
    ui->print(Progmem::getString(edSupport ? Progmem::drive3Inch_288 : Progmem::drive3Inch));
    key = toupper(ui->readKey(edSupport ? "712" : "71"));
    ui->print(Progmem::getString(Progmem::uiEchoKey), key);
    
    switch(key)
    {
    case '7':
      drive->SectorsPerTrack = 9;
      drive->CommRate = 250;
      drive->GapLength = 0x1B;
      drive->Gap3Length = 0x50;
      drive->FATMediaDescriptor = 0xF9;
      drive->FATRootDirEntries = 112;
      drive->FATClusterSizeBytes = 1024;
      break;
    case '1':
      drive->SectorsPerTrack = 18;
      drive->CommRate = 500;
      drive->GapLength = 0x1B;
      drive->Gap3Length = 0x6C;
      drive->FATMediaDescriptor = 0xF0;
      drive->FATRootDirEntries = 224;
      drive->FATClusterSizeBytes = 512;
      break;
    case '2':
      drive->SectorsPerTrack = 36;
      drive->CommRate = 1000;
      drive->GapLength = 0x1B;
      drive->Gap3Length = 0x53;
      drive->FATMediaDescriptor = 0xF0;
      drive->FATRootDirEntries = 240;
      drive->FATClusterSizeBytes = 1024;
      drive->PerpendicularRecording = true;
      break;
    }
  }
}

// configure all parameters of DiskDriveMediaParams
void SetDriveParametersAdvanced(FDC::DiskDriveMediaParams* drive)
{ 
  // advanced mode, redraw the screen into a different mode and reask for the drive
  ui->print("");
  ui->print(Progmem::getString(Progmem::specifyingAll), drive->DriveNumber + 65);
  
  // drive type in again (escape was pressed)
  ui->print(Progmem::getString(Progmem::driveInches));
  key = ui->readKey("853");
  drive->DriveInches = key - 48;
  ui->print(Progmem::getString(Progmem::uiEchoKey), key); 
  
  // double stepping yes/no
  ui->print(Progmem::getString(Progmem::customDoubleStep));
  key = toupper(ui->readKey("YN"));
  drive->DoubleStepping = (key == 'Y');
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  
  // changeline support yes/no
  ui->print(Progmem::getString(Progmem::customChangeline));
  key = toupper(ui->readKey("YN"));
  drive->DiskChangeLineSupport = (key == 'Y');
  ui->print(Progmem::getString(Progmem::uiEchoKey), key); 
  
  // get number of tracks (valid: 1-100)
  while(true)
  {
    ui->print(Progmem::getString(Progmem::customTracks));
    WORD tracks = (WORD)atoi(ui->prompt(3, Progmem::getString(Progmem::uiDecimalInput))); // input max 3 characters
    if ((tracks > 0) && (tracks <= 100))
    {
      drive->Tracks = (BYTE)tracks;
      ui->print(Progmem::getString(Progmem::uiNewLine));
      break;
    }
    
    ui->print(Progmem::getString(Progmem::uiDeleteLine));
  }
  
  // heads, 1 or 2
  ui->print(Progmem::getString(Progmem::customHeads));
  key = ui->readKey("12");
  drive->Heads = key - 48;
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  
  // sectors per track (valid: 1-63)
  while(true)
  {
    ui->print(Progmem::getString(Progmem::customSpt));
    BYTE spt = (BYTE)atoi(ui->prompt(2, Progmem::getString(Progmem::uiDecimalInput)));
    if ((spt > 0) && (spt < 64))
    {
      drive->SectorsPerTrack = spt;
      ui->print(Progmem::getString(Progmem::uiNewLine));
      break;
    }
    
    ui->print(Progmem::getString(Progmem::uiDeleteLine));
  }
  
  // sector size
  ui->print(Progmem::getString(Progmem::customSectorSize));
  key = toupper(ui->readKey("125K"));
  switch(key)
  {
  case '1':
    drive->SectorSizeBytes = 128;
    break;
  case '2':
    drive->SectorSizeBytes = 256;
    break;
  case '5':
    drive->SectorSizeBytes = 512;
    break;
  case 'K':
    drive->SectorSizeBytes = 1024;
    break;
  }
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
   
  // sector gap (hex)
  ui->print(Progmem::getString(Progmem::customSectorGap));
  drive->GapLength = (BYTE)strtoul(ui->prompt(2, Progmem::getString(Progmem::uiHexadecimalInput)), 0, 16);
  if (!drive->GapLength && !strlen(ui->getPromptBuffer())) ui->print("0");
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  // format gap (hex)
  ui->print(Progmem::getString(Progmem::customFormatGap3));
  drive->Gap3Length = (BYTE)strtoul(ui->prompt(2, Progmem::getString(Progmem::uiHexadecimalInput)), 0, 16);
  if (!drive->Gap3Length && !strlen(ui->getPromptBuffer())) ui->print("0");
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  // format fill BYTE (hex)
  ui->print(Progmem::getString(Progmem::customFormatFiller));
  drive->LowLevelFormatFiller = (BYTE)strtoul(ui->prompt(2, Progmem::getString(Progmem::uiHexadecimalInput)), 0, 16);
  if (!drive->LowLevelFormatFiller && !strlen(ui->getPromptBuffer())) ui->print("0");
  ui->print(Progmem::getString(Progmem::uiNewLine));
    
  // comm rate - 8" always 500K (data rate 500K MFM, 250K FM)
  if (drive->DriveInches == 8)
  {
    ui->print(Progmem::getString(Progmem::customDataRateSel));
    ui->print(Progmem::getString(Progmem::uiNewLine));
    ui->print(Progmem::getString(Progmem::custom8InchEnc));
    key = ui->readKey("25");
    drive->FM = (key == '2');
    drive->CommRate = 500;
    ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  }
  
  // 5.25", 3.5"
  else
  {
    // MFM or FM encoding
    ui->print(Progmem::getString(Progmem::customEncoding));
    key = toupper(ui->readKey("MF"));
    drive->FM = (key == 'F');
    ui->print(Progmem::getString(Progmem::uiEchoKey), key);
    
    ui->print(Progmem::getString(Progmem::customDataRateSel));
    ui->print(Progmem::getString(Progmem::uiNewLine));
    
    // 5.25"
    if (drive->DriveInches == 5)
    {
      if (drive->FM)
      {
        ui->print(Progmem::getString(Progmem::custom5InchFM));
        key = ui->readKey("152");
        switch(key)
        {
        case '1':
          drive->CommRate = 250;
          break;
        case '5':
          drive->CommRate = 300;
          break;
        case '2':
          drive->CommRate = 500;
          break;
        }
        ui->print(Progmem::getString(Progmem::uiEchoKey), key);
      }
      else
      {
        ui->print(Progmem::getString(Progmem::custom5InchMFM));
        key = ui->readKey("235");
        switch(key)
        {
        case '2':
          drive->CommRate = 250;
          break;
        case '3':
          drive->CommRate = 300;
          break;
        case '5':
          drive->CommRate = 500;
          break;
        }
        ui->print(Progmem::getString(Progmem::uiEchoKey), key);
      }
    }
    
    // 3.5"
    else
    {
      if (drive->FM)
      {
        ui->print(Progmem::getString(Progmem::custom3InchFM));
        key = ui->readKey("12");
        switch(key)
        {
        case '1':
          drive->CommRate = 250;
          break;
        case '2':
          drive->CommRate = 500;
          break;
        }
        ui->print(Progmem::getString(Progmem::uiEchoKey), key);
      }
      else
      {
        // detect if 1Mbps is supported
        const BYTE features = fdc->getSpecialFeatures();
        ui->print(Progmem::getString((features & SUPPORT_1MBPS) ? Progmem::custom3InchMFM_1M : Progmem::custom3InchMFM));
        key = toupper(ui->readKey((features & SUPPORT_1MBPS) ? "25M" : "25"));
        switch(key)
        {
        case '2':
          drive->CommRate = 250;
          break;
        case '5':
          drive->CommRate = 500;
          break;
        case 'M':
          drive->CommRate = 1000;
          break;
        }
        ui->print(Progmem::getString(Progmem::uiEchoKey), key);
        
        // detect if perpendicular recording is supported (500k/1Mbps MFM)
        if ((features & SUPPORT_PERPENDICULAR) && (drive->CommRate >= 500))
        {
          ui->print(Progmem::getString(Progmem::customEDModeInfo));          
          ui->print(Progmem::getString(Progmem::customEDMode));
          key = toupper(ui->readKey("YN"));
          drive->PerpendicularRecording = (key == 'Y');
          ui->print(Progmem::getString(Progmem::uiEchoKey), key);
        }
      }
    }
  }
  
  if (drive->SectorSizeBytes != 512)
  {
    // sector size not 512 for FAT12    
    return;
  }
  
  // ask if to support a FAT filesystem
  ui->print(Progmem::getString(Progmem::customUseFAT));
  key = toupper(ui->readKey("YN"));
  drive->UseFAT12 = (key == 'Y');
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  if (!drive->UseFAT12)
  {
    return;
  }
  
  // FAT media descriptor BYTE (hex)
  ui->print(Progmem::getString(Progmem::customMediaByte));
  drive->FATMediaDescriptor = (BYTE)strtoul(ui->prompt(2, Progmem::getString(Progmem::uiHexadecimalInput)), 0, 16);
  if (!drive->FATMediaDescriptor && !strlen(ui->getPromptBuffer())) ui->print("0");
  ui->print(Progmem::getString(Progmem::uiNewLine));

  // root directory entries (valid in 16 byte increments)
  while(true)
  {
    ui->print(Progmem::getString(Progmem::customRootDir));
    WORD rootDir = (WORD)atoi(ui->prompt(3, Progmem::getString(Progmem::uiDecimalInput)));
    if ((rootDir > 0) && (rootDir < 256) && ((rootDir % 16) == 0))
    {
      drive->FATRootDirEntries = (BYTE)rootDir;
      ui->print(Progmem::getString(Progmem::uiNewLine));
      break;
    }
    
    ui->print(Progmem::getString(Progmem::uiDeleteLine));
  }
  
  // FAT cluster size 512 or 1024 bytes (1 or 2 sectors per cluster)
  ui->print(Progmem::getString(Progmem::customClusterSize));
  key = toupper(ui->readKey("51"));
  drive->FATClusterSizeBytes = (key == '5') ? 512 : 1024;
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
    
  ui->print(Progmem::getString(Progmem::uiNewLine));
}

// verify drive letter in command, returns 0-3 for a valid drive, (BYTE)-1 invalid
BYTE VerifySuppliedDrive(const BYTE* drive)
{
  if (!drive)
  {
    return 0xFF;
  }
  
  // check formatting
  BYTE strLen = strlen(drive);    
  if ((strLen == 0) || (strLen > 3) || ((strLen >= 2) && (drive[1] != ':')) || ((strLen == 3) && (drive[2] != '\\')))
  {
    ui->print(Progmem::getString(Progmem::processInvalidDrv));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
    return 0xFF;
  }
  
  // sanity check, allowed drives A: to D:
  BYTE chosenDrive = drive[0];
  if ((chosenDrive < 65) || (chosenDrive > 68))
  {
    ui->print(Progmem::getString(Progmem::processInvalidDrv));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
    return 0xFF;
  }
  
  // to 0-based drive number, check if it had been configured in the startup
  chosenDrive -= 65;
  if (chosenDrive >= g_numberOfDrives)
  {
    ui->print(Progmem::getString(Progmem::processUnknownDrv));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
    return 0xFF;
  }
  
  return chosenDrive;
}

// verify directory name / file name validity (does not check if it exists)
bool VerifySuppliedPath(const BYTE* pathToCheck)
{
  if (fdc->getParams()->UseFAT12)
  {
    return !strpbrk(pathToCheck, Progmem::getString(Progmem::fsForbiddenCharFAT));
  }
  else if (fdc->getParams()->UseCPMFS)
  {
    return !strpbrk(pathToCheck, Progmem::getString(Progmem::fsForbiddenCharCPM));
  }
  
  return false;  
}

// uppercase string
void ToUpper(BYTE* str)
{
  if (!str)
  {
    return;
  }
  
  for (WORD index = 0; index < strlen(str); index++)
  {
    str[index] = toupper(str[index]);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////// command handlers ///////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CommandHELP(const BYTE* details)
{
  ui->print("");
  
  // details for RESET
  if (strcmp(details, Progmem::getString(Progmem::cmdReset)) == 0)
  {
    ui->print(Progmem::getString(Progmem::helpReset1));
    ui->print(Progmem::getString(Progmem::helpReset2));
    ui->print(Progmem::getString(Progmem::helpReset3));
    return;
  }
  
  // DRIVPARM
  else if (strcmp(details, Progmem::getString(Progmem::cmdDrivParm)) == 0)
  {
    ui->print(Progmem::getString(Progmem::helpDrivParm1));
    ui->print(Progmem::getString(Progmem::helpDrivParm2));
    ui->print(Progmem::getString(Progmem::helpCurrentDrive));
    return;
  }
  
  // PERSIST
  else if (strcmp(details, Progmem::getString(Progmem::cmdPersist)) == 0)
  {
    ui->print(Progmem::getString(Progmem::helpPersist1));
    ui->print(Progmem::getString(Progmem::helpPersist2));
    ui->print(Progmem::getString(Progmem::helpPersist3));
    return;
  }
  
  // FORMAT
  else if (strcmp(details, Progmem::getString(Progmem::cmdFormat)) == 0)
  {
    ui->print(Progmem::getString(Progmem::helpFormat1));
    ui->print(Progmem::getString(Progmem::helpFormat2));
    ui->print(Progmem::getString(Progmem::helpCurrentDrive));
    return;
  }
  
  // VERIFY
  else if (strcmp(details, Progmem::getString(Progmem::cmdVerify)) == 0)
  {
    ui->print(Progmem::getString(Progmem::helpVerify1));
    ui->print(Progmem::getString(Progmem::helpVerify2));
    ui->print(Progmem::getString(Progmem::helpCurrentDrive));
    return;
  }
  
  // IMAGE
  else if (strcmp(details, Progmem::getString(Progmem::cmdImage)) == 0)
  {
    ui->print(Progmem::getString(Progmem::helpImage1));
    ui->print(Progmem::getString(Progmem::helpImage2));
    ui->print(Progmem::getString(Progmem::helpImage3));
    ui->print(Progmem::getString(Progmem::helpCurrentDrive));
    return;
  }
  
  // QFORMAT
  else if (strcmp(details, Progmem::getString(Progmem::cmdQuickFormat)) == 0)
  {
    ui->print(Progmem::getString(Progmem::helpQuickFormat1));
    ui->print(Progmem::getString(Progmem::helpQuickFormat2));
    ui->print(Progmem::getString(Progmem::helpCurrentDrive));
    return;
  }
  
  // PATH
  else if (strcmp(details, Progmem::getString(Progmem::cmdPath)) == 0)
  {
    ui->print(Progmem::getString(Progmem::helpPath1));
    ui->print(Progmem::getString(Progmem::helpPath2));
    ui->print(Progmem::getString(Progmem::helpPath3));
    ui->print(Progmem::getString(Progmem::helpPath4));
    return;
  }
  
  // CD
  else if (strcmp(details, Progmem::getString(Progmem::cmdCd)) == 0)
  {
    ui->print(Progmem::getString(Progmem::helpCd1));
    ui->print(Progmem::getString(Progmem::helpCd2));
    ui->print(Progmem::getString(Progmem::helpCd3));
    ui->print(Progmem::getString(Progmem::helpCd4));
    return;
  }
  
  // DIR
  else if (strcmp(details, Progmem::getString(Progmem::cmdDir)) == 0)
  {
    ui->print(Progmem::getString(Progmem::helpDir1));
    ui->print(Progmem::getString(Progmem::helpDir2));
    ui->print(Progmem::getString(Progmem::helpDir3));
    return;
  }
  
  // DEL
  else if (strcmp(details, Progmem::getString(Progmem::cmdDel)) == 0)
  {
    ui->print(Progmem::getString(Progmem::helpDel1));
    ui->print(Progmem::getString(Progmem::helpDel2));
    return;
  }
  
  // TYPE
  else if (strcmp(details, Progmem::getString(Progmem::cmdType)) == 0)
  {
    ui->print(Progmem::getString(Progmem::helpType1));
    ui->print(Progmem::getString(Progmem::helpType2));
    return;
  }
  
  // TYPEINTO
  else if (strcmp(details, Progmem::getString(Progmem::cmdTypeInto)) == 0)
  {
    ui->print(Progmem::getString(Progmem::helpTypeInto1));
    ui->print(Progmem::getString(Progmem::helpTypeInto2));
    ui->print(Progmem::getString(Progmem::helpTypeInto3));
    return;
  }
  
  // MD
  else if (strcmp(details, Progmem::getString(Progmem::cmdMd)) == 0)
  {
    ui->print(Progmem::getString(Progmem::helpMd1));
    ui->print(Progmem::getString(Progmem::helpMd2));
    return;
  }
  
  // RD
  else if (strcmp(details, Progmem::getString(Progmem::cmdRd)) == 0)
  {
    ui->print(Progmem::getString(Progmem::helpRd1));
    ui->print(Progmem::getString(Progmem::helpRd2));
    return;
  }
  
  // XFER
  else if (strcmp(details, Progmem::getString(Progmem::cmdXfer)) == 0)
  {
    ui->print(Progmem::getString(Progmem::helpXfer1));
    ui->print(Progmem::getString(Progmem::helpXfer2));
    return;
  }
  
  // show all commands
  // determine supported commands (generic, no filesystem enabled, FAT filesystem, CP/M)
  BYTE startCommand = Progmem::cmdSupportedIndex;
  BYTE endCommand = fdc->getParams()->UseFAT12 ? Progmem::cmdEndIndex : Progmem::cmdFSIndex;
  
  ui->print(Progmem::getString(startCommand));
  startCommand++;
  
  for (; startCommand < endCommand; startCommand++)
  {
    const BYTE* command = Progmem::getString(startCommand);
    if (!strlen(command))
    {
      continue;
    }
    
    // print command and separator
    ui->print(command);
    if (startCommand+1 != endCommand)
    {
      ui->print(", ");
    }
  }
  
  // CP/M: allow QFORMAT, DIR, DEL, TYPE, XFER
  if (!fdc->getParams()->UseFAT12 && fdc->getParams()->UseCPMFS)
  {
    const BYTE separator[] = ", ";
    ui->print(separator);
    ui->print(Progmem::getString(Progmem::cmdQuickFormat));
    ui->print(separator);
    ui->print(Progmem::getString(Progmem::cmdDir));
    ui->print(separator);
    ui->print(Progmem::getString(Progmem::cmdDel));
    ui->print(separator);
    ui->print(Progmem::getString(Progmem::cmdType));
    ui->print(separator);
    ui->print(Progmem::getString(Progmem::cmdXfer));
  }
  
  // for details, use HELP with command
  ui->print(Progmem::getString(Progmem::helpDetails));
}

// list drive parameters
void CommandDRIVPARM(FDC::DiskDriveMediaParams* drive)
{
  ui->print("");
  ui->print(Progmem::getString(Progmem::drivParmCaption), drive->DriveNumber + 65);
  
  // media type
  ui->print(Progmem::getString(Progmem::drivParmTypeText));
  if (drive->DriveInches == 8)
  {
    ui->print(Progmem::getString(drive->FM ? Progmem::drivParmType8SD : Progmem::drivParmType8DD));
  }
  else if (drive->DriveInches == 5)
  {
    if (!drive->FM)
    {
      ui->print(Progmem::getString((drive->CommRate == 500) ? Progmem::drivParmType5HD : Progmem::drivParmType5DD));  
    }
    else
    {
      ui->print(Progmem::getString(Progmem::drivParmType5DD));
    }    
  }
  else
  {
    if (!drive->FM)
    {
      switch(drive->CommRate)
      {
      case 1000:
        ui->print(Progmem::getString(Progmem::drivParmType3ED));
        break;
      case 500:
        ui->print(Progmem::getString(Progmem::drivParmType3HD));
        break;
      default:
        ui->print(Progmem::getString(Progmem::drivParmType3DD));
        break;
      }
    }
    else
    {
      ui->print(Progmem::getString(Progmem::drivParmType3DD));
    }
  }
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  
  // double stepping
  ui->print(Progmem::getString(Progmem::drivParmDoubleStep));
  ui->print(Progmem::getString(drive->DoubleStepping ? Progmem::uiEnabled : Progmem::uiDisabled));
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  
  // supports disk change
  ui->print(Progmem::getString(Progmem::drivParmDiskChange));
  ui->print(Progmem::getString(drive->DiskChangeLineSupport ? Progmem::uiYes : Progmem::uiNo));
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  
  // step rate, head load and unload times
  ui->print(Progmem::getString(Progmem::drivParmSRT), drive->SRT);
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  ui->print(Progmem::getString(Progmem::drivParmHLT), drive->HLT);
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  ui->print(Progmem::getString(Progmem::drivParmHUT), drive->HUT);
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  
  // CHS and sector size
  ui->print(Progmem::getString(Progmem::drivParmTracks), drive->Tracks);
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  ui->print(Progmem::getString(Progmem::drivParmHeads));
  ui->print(Progmem::getString((drive->Heads == 1) ? Progmem::drivParmHeadsOne : Progmem::drivParmHeadsTwo));
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  ui->print(Progmem::getString(Progmem::drivParmSpt), drive->SectorsPerTrack);
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  ui->print(Progmem::getString(Progmem::drivParmSectorSize), drive->SectorSizeBytes);
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;  
  
  // data encoding and media data rate
  ui->print(Progmem::getString(Progmem::drivParmEncoding));
  ui->print(Progmem::getString(drive->FM ? Progmem::drivParmFM : Progmem::drivParmMFM));
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  if (!drive->FM && (drive->CommRate == 1000))
  {
    ui->print(Progmem::getString(Progmem::drivParmDataRate), 1);
    ui->print(Progmem::getString(Progmem::drivParmMbps));
  }
  else
  {
    ui->print(Progmem::getString(Progmem::drivParmDataRate), ((drive->FM) ? drive->CommRate/2 : drive->CommRate));
    ui->print(Progmem::getString(Progmem::drivParmKbps));
  }
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  
  // transfer rate between controller and MCU
  if (drive->CommRate == 1000)
  {
    ui->print(Progmem::getString(Progmem::drivParmFDCRate), 1);
    ui->print(Progmem::getString(Progmem::drivParmMbps));
  }
  else
  {
    ui->print(Progmem::getString(Progmem::drivParmFDCRate), drive->CommRate);
    ui->print(Progmem::getString(Progmem::drivParmKbps));
  }
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  
  // sector and format gaps, format filler
  ui->print(Progmem::getString(Progmem::drivParmSectorGap), drive->GapLength);
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  ui->print(Progmem::getString(Progmem::drivParmFormatGap), drive->Gap3Length);
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  ui->print(Progmem::getString(Progmem::drivParmFormatFill), drive->LowLevelFormatFiller);
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  
  // regular or perpendicular recording mode
  ui->print(Progmem::getString(Progmem::drivParmEDMode));
  ui->print(Progmem::getString(drive->PerpendicularRecording ? Progmem::uiEnabled : Progmem::uiDisabled));
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
    
  // filesystem configuration
  ui->print(Progmem::getString(Progmem::drivParmFS));
  if (drive->UseFAT12)
  {
    ui->print(Progmem::getString(Progmem::drivParmFAT));
  }
  else if (drive->UseCPMFS)
  {
    ui->print(Progmem::getString(Progmem::drivParmCPM));
  }
  else
  {
    ui->print(Progmem::getString(Progmem::uiNotAvailable));
  }
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  
  // FAT specific configuration - media descriptor, root dir entries, cluster size
  if (!drive->UseFAT12)
  {
    return;
  }
    
  ui->print(Progmem::getString(Progmem::drivParmFATMedia), drive->FATMediaDescriptor);
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  ui->print(Progmem::getString(Progmem::drivParmFATRootDir), drive->FATRootDirEntries);
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
  ui->print(Progmem::getString(Progmem::drivParmFATCluster), drive->FATClusterSizeBytes);
  ui->print(Progmem::getString(Progmem::uiNewLine)); NEXT_LINE_PAUSE;
}

// low-level disk format
void CommandFORMAT(FDC::DiskDriveMediaParams* drive)
{
  // save currently active drive
  BYTE oldDriveNumber = fdc->getParams()->DriveNumber;
  BYTE chosenDrive = drive->DriveNumber;
  
  ui->print("");
  ui->print(Progmem::getString(Progmem::diskIoInsertDisk), chosenDrive + 65);
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  bool makeFilesystem = false;
  bool withVerify = false;
  
  // if filesystem enabled for drive, ask whether to write it after    
  if (drive->UseFAT12 || drive->UseCPMFS)
  {
    ui->print(Progmem::getString(Progmem::diskIoCreateFS));
    key = toupper(ui->readKey("YN"));
    ui->print(Progmem::getString(Progmem::uiEchoKey), key);
    makeFilesystem = key == 'Y';
    
    // if writing filesystem, turn verify on and do not ask for input
    if (makeFilesystem)
    {
      withVerify = true;
      
      // inform about verify turned on
      ui->print(Progmem::getString(Progmem::diskIoFormatVerify));
      ui->print(Progmem::getString(Progmem::uiEchoKey), 'Y');
    }
  }
  
  // format with verify?
  if (!withVerify)
  {
    ui->print(Progmem::getString(Progmem::diskIoFormatVerify));
    key = toupper(ui->readKey("YN"));
    ui->print(Progmem::getString(Progmem::uiEchoKey), key);
    withVerify = key == 'Y';
  }  
  
  // ask to proceed
  ui->print(Progmem::getString(Progmem::uiContinueAbort));
  key = ui->readKey("\r\e");
  ui->print(Progmem::getString(Progmem::uiNewLine));
  if (key == '\e')
  {
    ui->print(Progmem::getString(Progmem::uiNewLine));
    return;
  }
  
  ui->disableKeyboard(true);
  
  // set new drive
  if (oldDriveNumber != chosenDrive)
  {
    fdc->setActiveDrive(&g_diskDrives[chosenDrive]);
  }
  
  const WORD trackBytes = fdc->getParams()->SectorSizeBytes * fdc->getParams()->SectorsPerTrack;
  WORD badTracks = 0;
  BYTE head = 0;
  BYTE track = 0; 
  
  while(track < fdc->getParams()->Tracks)
  {
    ui->print(Progmem::getString(Progmem::diskIoProgress), track, head);
    fdc->seekDrive(track, head);    
    fdc->formatTrack();
    
    const bool formatError = fdc->getLastError();
    const bool formatBreakError = formatError ? fdc->wasErrorNoDiskInDrive() || fdc->wasErrorDiskProtected() : false;
    if (formatBreakError)
    {
      break;
    }
    else if (formatError)
    {
      badTracks++;
    }
    
    // format OK, now verify
    else if (withVerify)
    {
      const WORD successfulBytesRead = fdc->verifyTrack();     
      if (fdc->wasErrorNoDiskInDrive())
      {
        break;
      }
      
      // generic IO error
      else if (fdc->getLastError() || (successfulBytesRead != trackBytes))
      {
        // failed on track zero? break now
        if (track == 0)
        {
          break;
        }
        
        // increment number of tracks that had at least 1 bad sector       
        badTracks++;
      }
    }
       
    // the other head
    head++;    
    if (head < fdc->getParams()->Heads)
    {
      continue;
    }
    
    head = 0;
    track++;
  }
  
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  // if track 0 bad, do not write filesystem
  bool finishedOk = !fdc->getLastError() && (fdc->getParams()->Tracks == track); 
  bool track0Bad = !finishedOk && (track == 0) && !fdc->wasErrorNoDiskInDrive() && !fdc->wasErrorDiskProtected();  
  if (track0Bad)
  {
    ui->print(Progmem::getString(Progmem::diskIoTrack0Error));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
  
  // track 0 looks to be good, try writing the filesystem by calling QFORMAT routine with "do not ask" argument
  else if (makeFilesystem && finishedOk)
  {
    CommandQFORMAT(drive, true);
    ui->print(Progmem::getString(Progmem::uiNewLine));
  }
  
  // print total bad track count, if logged
  if (badTracks)
  {
    ui->print(Progmem::getString(Progmem::diskIoTotalBadTrk), badTracks);
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
  else if (finishedOk)
  {
    ui->print(Progmem::getString(Progmem::diskIoFormatOK));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
  
  // seek to 0,0
  fdc->seekDrive(0, 0);
  
  // restore active drive and return
  if (oldDriveNumber != chosenDrive)
  {
    fdc->setActiveDrive(&g_diskDrives[oldDriveNumber]);
  }
  
  ui->disableKeyboard(false);
}

// similar pattern to FORMAT
void CommandVERIFY(FDC::DiskDriveMediaParams* drive)
{
  BYTE oldDriveNumber = fdc->getParams()->DriveNumber;
  BYTE chosenDrive = drive->DriveNumber;
    
  ui->print("");
  ui->print(Progmem::getString(Progmem::diskIoInsertDisk), chosenDrive + 65);
  ui->print(Progmem::getString(Progmem::uiNewLine)); 
  
  ui->print(Progmem::getString(Progmem::uiContinueAbort));
  key = ui->readKey("\r\e");
  ui->print(Progmem::getString(Progmem::uiNewLine));
  if (key == '\e')
  {
    ui->print(Progmem::getString(Progmem::uiNewLine));
    return;
  }
  
  ui->disableKeyboard(true);
  
  if (oldDriveNumber != chosenDrive)
  {
    fdc->setActiveDrive(&g_diskDrives[chosenDrive]);  
  }
  
  const WORD trackBytes = fdc->getParams()->SectorSizeBytes * fdc->getParams()->SectorsPerTrack;
  WORD badTracks = 0;
  BYTE head = 0;
  BYTE track = 0;
  
  while(track < fdc->getParams()->Tracks)
  {
    ui->print(Progmem::getString(Progmem::diskIoProgress), track, head);
    fdc->seekDrive(track, head);    
    
    const WORD successfulBytesRead = fdc->verifyTrack();
    if (fdc->wasErrorNoDiskInDrive())
    {
      break;
    }
    
    else if (fdc->getLastError() || (successfulBytesRead != trackBytes))
    {     
      if (track == 0)
      {
        break;
      }
             
      badTracks++;
    }
       
    head++;    
    if (head < fdc->getParams()->Heads)
    {
      continue;
    }
    
    head = 0;       
    track++;
  }
  
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  bool finishedOk = !fdc->getLastError() && (fdc->getParams()->Tracks == track); 
  bool track0Bad = !finishedOk && (track == 0) && !fdc->wasErrorNoDiskInDrive() && !fdc->wasErrorDiskProtected();  
  if (track0Bad)
  {
    ui->print(Progmem::getString(Progmem::diskIoTrack0Error));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
  
  if (badTracks)
  {
    ui->print(Progmem::getString(Progmem::diskIoTotalBadTrk), badTracks);
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
  else if (finishedOk)
  {
    ui->print(Progmem::getString(Progmem::uiOK));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
  
  fdc->seekDrive(0, 0);
  if (oldDriveNumber != chosenDrive)
  {
    fdc->setActiveDrive(&g_diskDrives[oldDriveNumber]);
  }
  
  ui->disableKeyboard(false);
}

void CommandIMAGE(FDC::DiskDriveMediaParams* drive)
{
  BYTE oldDriveNumber = fdc->getParams()->DriveNumber;
  BYTE chosenDrive = drive->DriveNumber;
  
  // activate chosen drive now
  if (oldDriveNumber != chosenDrive)
  {
    fdc->setActiveDrive(&g_diskDrives[chosenDrive]);
  }
  
  // options: read from disk to image, write disk from image, cancel 
  ui->print("");  
  ui->print(Progmem::getString(Progmem::imageTransferLen), (DWORD)fdc->getTotalSectorCount() * fdc->getParams()->SectorSizeBytes);
  ui->print(Progmem::getString(Progmem::imageGeometry), fdc->getParams()->Tracks, fdc->getParams()->Heads,
                                                        fdc->getParams()->SectorsPerTrack, fdc->getParams()->SectorSizeBytes);
  ui->print(Progmem::getString(Progmem::imageReadDisk), chosenDrive + 65);
  ui->print(Progmem::getString(Progmem::imageWriteDisk), chosenDrive + 65);
  ui->print(Progmem::getString(Progmem::uiCancelOption));
  BYTE operation = toupper(ui->readKey("RWC"));
  ui->print(Progmem::getString(Progmem::uiEchoKey), operation);
  
  // cancel, reset to previous drive
  if (operation == 'C')
  {
    if (oldDriveNumber != chosenDrive)
    {
      fdc->setActiveDrive(&g_diskDrives[oldDriveNumber]);
    }
    
    return;    
  }
  
  // offer XMODEM-1K with 1kB packets
  bool useXMODEM1K = false;
  
  if (SECTOR_BUFFER_SIZE >= 1024)
  {
    // try if enough memory
    bool allocOk = false;
    BYTE* testAlloc = new BYTE[1030];
    
    if (testAlloc)
    {
      delete[] testAlloc;
      allocOk = true;
    }
    
    if (allocOk && // for XMODEM with 1K packets, one complete track must fit without remainder of the configured buffer size
       (((fdc->getParams()->SectorsPerTrack * fdc->getParams()->SectorSizeBytes) % SECTOR_BUFFER_SIZE) == 0))
    {
      ui->print(Progmem::getString(Progmem::xmodemUse1k));
      key = toupper(ui->readKey("YN"));
      ui->print(Progmem::getString(Progmem::uiEchoKey), key);
      
      useXMODEM1K = (key == 'Y');
    }
  }  
  
  // read into image file 
  if (operation == 'R')
  {
    xmodemReadDiskIntoImageFile(useXMODEM1K);
  }
  
  // write from image file  
  else
  {
    xmodemWriteDiskFromImageFile(useXMODEM1K);
  }
  
  // reset to previous drive
  if (oldDriveNumber != chosenDrive)
  {
    fdc->setActiveDrive(&g_diskDrives[oldDriveNumber]);
  }
}

void CommandQFORMAT(FDC::DiskDriveMediaParams* drive, bool dontAskConfirm)
{
  BYTE oldDriveNumber = fdc->getParams()->DriveNumber;
  BYTE chosenDrive = drive->DriveNumber;
    
  if (!dontAskConfirm)
  {
    ui->print(Progmem::getString(Progmem::diskIoQuickFormat), chosenDrive + 65);
    key = toupper(ui->readKey("YN"));
    ui->print(Progmem::getString(Progmem::uiEchoKey), key);
    
    if (key == 'N')
    {
      return;
    }
  }
  
  // set new active drive
  if (oldDriveNumber != chosenDrive)
  {
    fdc->setActiveDrive(&g_diskDrives[chosenDrive]);
  }
  
  ui->disableKeyboard(true);
  
  // FAT12
  if (drive->UseFAT12)
  {
    ui->print(Progmem::getString(Progmem::diskIoCreatingFAT));
    fatQuickFormat();
  }
  
  // CP/M
  else
  {
    ui->print(Progmem::getString(Progmem::diskIoCreatingCPM));
    cpmQuickFormat();
  }
  ui->disableKeyboard(false);
  
  // restore back
  if (oldDriveNumber != chosenDrive)
  {
    fdc->setActiveDrive(&g_diskDrives[oldDriveNumber]);
  }
}

// transfer files over serial link
// CP/M disks: read only, for now
void CommandXFER(const BYTE* fileName)
{
  ui->print("");
  const bool fat = fdc->getParams()->UseFAT12;

  ui->print(Progmem::getString(Progmem::xferReadFile));
  if (fat)
  {
    ui->print(Progmem::getString(Progmem::xferSaveFile));
  }
  ui->print(Progmem::getString(Progmem::uiCancelOption));
  key = toupper(ui->readKey(fat ? "RWC" : "RC"));
  ui->print(Progmem::getString(Progmem::uiEchoKey), key);
  
  // read and send file over XMODEM (FAT12, CP/M)
  if (key == 'R')
  {
    xmodemSendFile(fileName);
  }
  
  // receive file over XMODEM and write (FAT12)
  else if (key == 'W')
  {   
    xmodemReceiveFile(fileName);
  }
}
