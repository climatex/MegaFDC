// MegaFDC (c) 2023-2024 J. Bogin, http://boginjr.com
// FatFs interface for filesystem support

#include "config.h"

#ifndef BUILD_IMD_IMAGER

// get pointers to FATFS and FIL structs
FATFS* getFat()
{
  static FATFS fat = {0};
  return &fat;
}

FIL* getFatFile()
{
  static FIL file = {0};
  return &file;
}

// create empty directory
void fatMkdir(const BYTE* dirName)
{
  if (!fatMount())
  {
    return;
  }
  
  // overflow checks in CD command, as absolute paths in names are not allowed  
  BYTE addPath[MAX_PATH+1] = {0};
  strcat(addPath, g_path);
  strcat(addPath, dirName);
  
  FAT_EXECUTE(f_mkdir(addPath));
}

// remove empty directory
void fatRmdir(const BYTE* dirName)
{ 
  if (!fatMount())
  {
    return;
  }
  
  BYTE addPath[MAX_PATH+1] = {0};
  strcat(addPath, g_path);
  strcat(addPath, dirName);
  
  FAT_EXECUTE(f_rmdir(addPath));
  ui->print(Progmem::getString(Progmem::uiNewLine));
}

// unlink file
void fatDeleteFile(const BYTE* fileName)
{  
  if (!fatMount())
  {
    return;
  }
  
  BYTE addPath[MAX_PATH+1] = {0};
  strcat(addPath, g_path);
  strcat(addPath, fileName);
  
  FAT_EXECUTE(f_unlink(addPath));
  ui->print(Progmem::getString(Progmem::uiNewLine));
}

// dumps file to screen
void fatDumpFile(const BYTE* fileName)
{  
  if (!fatMount())
  {
    return;
  }
  
  BYTE addPath[MAX_PATH+1] = {0};
  strcat(addPath, g_path);
  strcat(addPath, fileName);
  
  WORD count = 1;
  FAT_EXECUTE(f_open(getFatFile(), addPath, FA_READ));
  
  while (count)
  {
    // to ui buffer, 0s skipped
    FAT_EXECUTE_FILE(f_read(getFatFile(), ui->getPrintBuffer(), MAX_CHARS, &count));
    if (count)
    {
      ui->setPrintLength(count);
      ui->print(ui->getPrintBuffer());  
    }    
  }
  
  FAT_EXECUTE(f_close(getFatFile()));
  ui->print(Progmem::getString(Progmem::uiNewLine));
}

// writes text file from screen
void fatWriteTextFile(const BYTE* fileName)
{
  if (!fatMount())
  {
    return;
  }
  
  BYTE addPath[MAX_PATH+1] = {0};
  strcat(addPath, g_path);
  strcat(addPath, fileName);
  
  FAT_EXECUTE(f_open(getFatFile(), addPath, FA_WRITE | FA_CREATE_NEW));
  
  // wait for ENTER keypress
  ui->print(Progmem::getString(Progmem::typeIntoCaption));
  ui->print(Progmem::getString(Progmem::uiContinue));
  ui->disableKeyboard(false);
  ui->readKey(Progmem::getString(Progmem::uiEnterKey));
  ui->print("");
  
  bool emptyLine = false;  
  for (;;)
  {
    // all valid keys allowed
    const BYTE* promptBuffer = ui->prompt();
    WORD length = strlen(promptBuffer);
    
    // quit?
    if (!length && emptyLine)
    {
      break;
    }    
    emptyLine = length == 0;
    
    // write line
    ui->disableKeyboard(true);
    
    WORD dummy;
    FAT_EXECUTE_FILE(f_write(getFatFile(), promptBuffer, length, &dummy));
    FAT_EXECUTE_FILE(f_write(getFatFile(), Progmem::getString(Progmem::uiNewLine), 2, &dummy));
    
    ui->disableKeyboard(false);
    ui->print(Progmem::getString(Progmem::uiNewLine));
  }
  
  FAT_EXECUTE(f_close(getFatFile()));
  ui->print(Progmem::getString(Progmem::uiNewLine));
}

// change working directory - check if what is in the path buffer exists
bool fatChdir()
{
  DIR dir = {0};
  
  if (!fatMount())
  {
    return false;
  }
  
  FAT_EXECUTE_0(f_opendir(&dir, g_path));
  FAT_EXECUTE_0(f_closedir(&dir));
  return true;
}

// list contents of current working directory
void fatDirCommand()
{
  DIR dir = {0};
    
  if (!fatMount())
  {
    return;
  }
  
  BYTE* printBuffer = ui->getPrintBuffer();
  BYTE key = 0;
  WORD entriesCount = 0;
  FAT_EXECUTE(f_opendir(&dir, g_path));
  
  while(true)
  {
    FILINFO info = {0};
    FAT_EXECUTE_DIR(f_readdir(&dir, &info));
    
    // empty entry
    BYTE* name = info.fname;   
    if (!name || !strlen(name))
    {
      break;
    }
    
    printBuffer[0] = 0;
    
    // directory?
    if (info.fattrib & AM_DIR)
    {
      strncat(printBuffer, Progmem::getString(Progmem::dirDirectory), MAX_CHARS);
    }
    
    // file, print size
    else
    {     
      snprintf(printBuffer, MAX_CHARS, Progmem::getString(Progmem::dirBytesFormat), info.fsize);
      strncat(printBuffer, Progmem::getString(Progmem::uiBytes), MAX_CHARS);
      strncat(printBuffer, " ", MAX_CHARS);
    }
    
    // attributes readonly, archive, hidden, system
    strncat(printBuffer, info.fattrib & AM_RDO ? "R" : "-", MAX_CHARS);
    strncat(printBuffer, info.fattrib & AM_ARC ? "A" : "-", MAX_CHARS);
    strncat(printBuffer, info.fattrib & AM_HID ? "H" : "-", MAX_CHARS);
    strncat(printBuffer, info.fattrib & AM_SYS ? "S" : "-", MAX_CHARS);
        
    // name
    strncat(printBuffer, " ", MAX_CHARS);
    strncat(printBuffer, name, MAX_CHARS);
        
    // next entry
    strncat(printBuffer, Progmem::getString(Progmem::uiNewLine), MAX_CHARS);
    entriesCount++;
    
    // print line
    ui->print(printBuffer);
    
    // on a display, wait for screen fill on the last line
    if (g_uiEnabled && ui->isOnLastLine())
    {
      ui->print(Progmem::getString(Progmem::uiContinueAbort));
      ui->disableKeyboard(false);
      key = ui->readKey("\r\e");
      ui->disableKeyboard(true);
      ui->print("");
      
      // Escape
      if (key == '\e')
      {
        break;
      }
    }
  }
  
  // end of listing
  FAT_EXECUTE(f_closedir(&dir));
  if (!entriesCount)
  {
    ui->print(Progmem::getString(Progmem::dirDirectoryEmpty));
    ui->print(Progmem::getString(Progmem::uiNewLine));
  }
  
  // wait for keypress on display
  else if ((g_uiEnabled) && (key != '\e') && ui->isOnLastLine())
  {
    ui->print(Progmem::getString(Progmem::uiContinue));
    ui->disableKeyboard(false);
    ui->readKey(Progmem::getString(Progmem::uiEnterKey));
    ui->disableKeyboard(true);
    ui->print("");
  }
  
  // show free space
  FATFS* dummy;
  DWORD sizeFree = 0;
  FAT_EXECUTE(f_getfree(Progmem::getString(Progmem::fsCurrentDrive), &sizeFree, &dummy));
  sizeFree *= fdc->getParams()->FATClusterSizeBytes;
  
  snprintf(printBuffer, MAX_CHARS, Progmem::getString(Progmem::dirBytesFormat), sizeFree);
  strncat(printBuffer, Progmem::getString(Progmem::dirBytesFree), MAX_CHARS);
  ui->print(printBuffer);  
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  ui->disableKeyboard(false);
}

// create FAT filesystem (disk must be formatted)
void fatQuickFormat()
{
  // verify reading of track 0, inform "try formatting" if it fails
  if (!fdc->verifyTrack0(true))
  {
    ui->print(Progmem::getString(Progmem::uiNewLine));
    return;
  }
  
  MKFS_PARM param = {0};
  param.fmt = FM_FAT | FM_SFD; // FAT with no partitioning
  param.n_fat = 2;             // 2 allocation tables
  param.align = 1;             // block alignment, not used for FAT12
  
  // fill in our config
  param.heads = fdc->getParams()->Heads;
  param.spt = fdc->getParams()->SectorsPerTrack;
  param.au_size = fdc->getParams()->FATClusterSizeBytes;
  param.n_root = fdc->getParams()->FATRootDirEntries;
  param.media = fdc->getParams()->FATMediaDescriptor;
          
  FAT_EXECUTE(f_mkfs(Progmem::getString(Progmem::fsCurrentDrive), &param, getFat()->win, fdc->getParams()->SectorSizeBytes));
  
  ui->print(Progmem::getString(Progmem::uiOK));
  ui->print(Progmem::getString(Progmem::uiNewLine));
}

// (re)mount FAT
bool fatMount()
{
  // retrieve disk status - in this order
  //const bool diskChangeInquired = fdc->wasDiskChangeInquired();
  const bool diskErrorWP = fdc->wasErrorDiskProtected();
  const bool diskChanged = fdc->isDiskChanged();
   
  // verify track 0 can be read
  if (!fdc->verifyTrack0())
  {
    ui->print(Progmem::getString(Progmem::uiNewLine));
    return false;
  }
    
  // (re)mount volume if disk changed
  if (diskChanged || diskErrorWP)
  {    
    FAT_EXECUTE(f_mount(NULL, Progmem::getString(Progmem::fsCurrentDrive), 0));
    FAT_EXECUTE(f_mount(getFat(), Progmem::getString(Progmem::fsCurrentDrive), 0));
    
    // check if the working path is still valid by doing a quick blip (do not print errors here)
    DIR dir = {0};
    const bool pathExists = (f_opendir(&dir, g_path) == FR_OK);
    
    // keep using it
    if (pathExists)
    {
      f_closedir(&dir);
    }
    
    // get back to A:\> (or B, C, D)
    else
    {
      g_path[0] = 0;
    }
  }
  
  return true;
}

FRESULT fatResult(FRESULT result)
{
  // certain error messages are handled by FDC already
  BYTE errorMessage = 0;
  
  switch (result)
  {
  case FR_DISK_ERR:
    errorMessage = Progmem::fsDiskError;
    break;
  case FR_INT_ERR:
    errorMessage = Progmem::fsInternalError;
    break;
  case FR_NO_FILE:
    errorMessage = Progmem::fsFileNotFound;
    break;
  case FR_NO_PATH:
    errorMessage = Progmem::fsPathNotFound;
    break;
  case FR_INVALID_NAME:
    errorMessage = Progmem::processInvalidChr1;
    break;
  case FR_DENIED:
    errorMessage = Progmem::fsDirectoryFull;
    break;
  case FR_EXIST:
    errorMessage = Progmem::fsFileExists;
    break;
  case FR_INVALID_OBJECT:
    errorMessage = Progmem::fsInvalidObject;
    break;
  case FR_NOT_ENABLED:
    errorMessage = Progmem::fsNoVolumeWorkArea;
    break;
  case FR_NO_FILESYSTEM:
    errorMessage = Progmem::fsNoFAT;
    break;
  case FR_MKFS_ABORTED:
    errorMessage = Progmem::fsMkfsError;
    break;
  case FR_NOT_ENOUGH_CORE:
    errorMessage = Progmem::fsMemoryError;
    break;
  case FR_INVALID_PARAMETER:
    errorMessage = Progmem::fsInvalidParameter;
    break;
  }
  
  if (errorMessage)
  {
    ui->print(Progmem::getString(errorMessage));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
  }
  
  return result;
}

#endif