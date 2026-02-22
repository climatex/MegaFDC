// MegaFDC (c) 2023-2026 J. Bogin, http://boginjr.com
// SD card routines

#include "config.h"

#ifdef SD_CARD_SUPPORT

SdFat sd;
File file;
BYTE path[MAX_PATH+1] = {0};
BYTE addPath[MAX_PATH+1] = {0};

bool SdDetectMemoryCard()
{ 
  sd.end();
  
  static SdSpiConfig cfg(53, USER_SPI_BEGIN); //53: SCS 
  if (!sd.cardBegin(cfg))
  {
    ui->print(Progmem::getString(Progmem::uiNewLine));
    ui->print(Progmem::getString(Progmem::sdCardMissing));
    ui->print(Progmem::getString(Progmem::uiNewLine));
    return false;
  }
  
  if (!sd.card()->sectorCount() || !sd.volumeBegin())
  {
    sd.end();
    
    ui->print(Progmem::getString(Progmem::uiNewLine));
    ui->print(Progmem::getString(Progmem::sdCardError));
    ui->print(Progmem::getString(Progmem::uiNewLine));
    return false;
  }
  
  return true;  
}

File* SdPickFile(bool writingImage, const BYTE* filterExt /* = NULL */)
{ 
  if (!SdDetectMemoryCard())
  {
    return NULL;
  }
  
  // try to mount root directory
  strcpy(path, "/");
  File rootdir = sd.open(path, O_RDONLY);
  if (!rootdir)
  {
    ui->print(Progmem::getString(Progmem::uiNewLine));
    ui->print(Progmem::getString(Progmem::sdCardError));
    ui->print(Progmem::getString(Progmem::uiNewLine));
    return NULL;
  }
  
  // print file filter wildcard / all files
  ui->print("");
  ui->print(Progmem::getString(Progmem::sdDirListing), filterExt ? filterExt : ".*");
  
  bool noFiles = true;
  while(true)
  {
    File file = rootdir.openNextFile(O_RDONLY);
    if (!file)
    {
      break;
    }

    if (file.isHidden())
    {
      file.close();
      continue;
    }
    
    // filter directories and files
    addPath[0] = 0;
    file.getName(addPath, MAX_PATH);  
    const bool isDirectory = file.isDir() || file.isSubDir();
    file.close();
    if (!strlen(addPath))
    {
      continue;
    }    
    if (!isDirectory && filterExt)
    {
      const BYTE* ext = strcasestr(addPath, filterExt);
      if (!ext)
      {
        continue;
      }
      
      // also ending with it?
      const BYTE extPos = ext-&addPath[0];
      if (extPos != strlen(addPath)-4)
      {
        continue;
      }
    }
    
    // do directory listing
    noFiles = false;
    ui->print(isDirectory ? "<DIR> " : "      ");
    ui->print("%s", addPath);
    ui->print(Progmem::getString(Progmem::uiNewLine));
  }
  rootdir.close();
  
  if (noFiles)
  {
    ui->print(Progmem::getString(Progmem::sdDirEmpty));
    ui->print(Progmem::getString(Progmem::uiNewLine));
    
    if (!writingImage)
    {
      ui->print(Progmem::getString(Progmem::uiContinue));
      ui->readKey("\r");  
      ui->print(Progmem::getString(Progmem::uiNewLine));
      return NULL;  
    }    
  }
  
  while(true)
  {
    strcpy(path, "/");
    ui->print(Progmem::getString(writingImage ? Progmem::sdDirPickWrite : Progmem::sdDirPickRead));
    const BYTE* promptBuffer = ui->prompt(MAX_PATH-1, NULL, true); // -1 to account for initial '/' in path buffer
    if (!promptBuffer) // cancelled
    {
      ui->print(Progmem::getString(Progmem::uiNewLine));
      return NULL;
    }
    else if (strlen(promptBuffer) == 0)
    {
      continue;
    }
    
    WORD idx = 0;
    bool emptyString = true;
    while (idx < strlen(promptBuffer))
    {
      if (!isspace(promptBuffer[idx++]))
      {
        emptyString = false;
        break;
      }
    }
    if (emptyString)
    {
      continue;
    }    
    ui->print(Progmem::getString(Progmem::uiNewLine));
    
    // append extension, if not specified
    bool appendExt = false;
    strcat(path, promptBuffer);
    if (filterExt)
    {
      const BYTE* ext = strcasestr(path, filterExt);
      if (!ext)
      {
        appendExt = true;
      }
      else
      {
        const BYTE extPos = ext-&path[0];
        if (extPos != strlen(path)-4)
        {
          appendExt = true;
        }
      }  
      if (appendExt)
      {
        if (strlen(path) > MAX_PATH-4)
        {
          path[MAX_PATH-4] = 0; // make space :)
        }
        strcat(path, filterExt);
      }  
    }    
    
    // check if file exists; if writing, ask to overwrite
    if (!SdDetectMemoryCard())
    {
      return NULL;
    }
    
    File testOpen = sd.open(path, O_RDONLY);
    bool fileExists = false;
    if (testOpen)
    {
      fileExists = true;
      testOpen.close();
    }
    if (!writingImage && !fileExists)
    {
      ui->print(Progmem::getString(Progmem::sdDirInvalid));
      continue;
    }
    else if (writingImage && fileExists)
    {
      ui->print(Progmem::getString(Progmem::sdDirOverwrite));
      BYTE key = toupper(ui->readKey("YN\e"));
      if (key == '\e')
      {
        ui->print(Progmem::getString(Progmem::uiNewLine));
        return NULL;
      }
      
      ui->print(Progmem::getString(Progmem::uiEchoKey), key);
      if (key == 'N')
      {
        continue;
      }
    }
    
    // open the file for read or write
    file = sd.open(path, writingImage ? O_WRITE | O_CREAT | O_TRUNC : O_RDONLY);
    if (!file)
    {
      ui->print(Progmem::getString(Progmem::uiNewLine));
      ui->print(Progmem::getString(Progmem::sdFileError));
      ui->print(Progmem::getString(Progmem::uiNewLine));
      ui->print(Progmem::getString(Progmem::uiContinue));
      ui->readKey("\r");  
      ui->print(Progmem::getString(Progmem::uiNewLine));
      return NULL;
    }

    return &file;
  }
}

#endif