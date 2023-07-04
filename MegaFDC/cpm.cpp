// MegaFDC (c) 2023 J. Bogin, http://boginjr.com
// CP/M filesystem for 8" SSSD/FM (250K)
// IBM 3740 single-sided geometry 77x26x128B

#include "config.h"

#define DIRECTORY_SECTOR_START 52   // first two tracks reserved for BIOS and OS
#define DIRECTORY_SECTORS      16   // 2K (16x128) of directory follows right after the reserved space
#define DIRECTORY_ENTRY_SIZE   32   // 32 bytes each directory entry
#define DIRECTORY_ENTRY_UNUSED 0xE5 // 0xF6 was an "unused" filler for double/high density media
#define DIRECTORY_ENTRIES      (DIRECTORY_SECTORS*128)/DIRECTORY_ENTRY_SIZE // 64 files
#define FILE_NAME_LENGTH       11   // 8 for the name, 3 for the extension
#define DATA_SECTOR_START      DIRECTORY_SECTOR_START+DIRECTORY_SECTORS // 2002 sectors total (data end)

// structure of one 32-byte directory entry (CP/M "extent"), addressed from disk buffer
struct CPMDirectoryEntry
{
  BYTE userNumber;                 // 0-31, or 0xE5 if file has been deleted
  BYTE fileName[FILE_NAME_LENGTH]; // 7-bit ASCII 8.3 filename with MSB attributes on last 3 bytes of extension
  BYTE entryIndexLo;               // 0-based index of this entry, only bits 0-4 used
  BYTE ignore;                     // reserved
  BYTE entryIndexHi;               // hibyte of this entry
  BYTE entrySize;                  // size, in 128-bytes, per this entry (there might be more of the same file), max 0x80
  BYTE allocationBlocks[16];       // file blocks in 1K, maximum 16K per entry, files larger than 16K have more entries in dir with same fileName
};

// structure of directory entries kept in memory - the entryIndexes are not sequential on disk, so need to read all
// warning: userNumber and fileName must be in the same struct order in both CPMDirectoryEntry and CPMDir,
// these are used to find all matches of the same file spanning thru multiple directory entries
struct CPMDir
{
  BYTE userNumber;                 // same as above
  BYTE fileName[FILE_NAME_LENGTH]; // ditto
  BYTE entrySize;                  // size in 128-byte counts
  WORD entryIndex;                 // disk index of this entry (only max DIRECTORY_ENTRIES used)
  BYTE* blocks;                    // dynamic array of allocation blocks of this entry (0 to 16 max)
  BYTE blocksCount;
};

CPMDir* cpmDirectory = NULL;           // dynamic array of directory entries, NULL on not initialized/failure/no existing files on disk
BYTE    cpmDirectoryCount = 0;         // max DIRECTORY_ENTRIES
WORD    openFileSectorCount = 0;       // open file size in 128-byte sector count, 0 if no file open
WORD    openFileSector = 0;            // current logical sector address during file I/O
CPMDir* openFileDirEntry = NULL;       // current entry during file I/O, or NULL if it needs to be advanced to next one (all allocation blocks read)
BYTE    openFileDirEntryIndex = 0;     // index of openFileDirEntry in the sorted buffer of entries, max DIRECTORY_ENTRIES
BYTE    openFileDirEntryCurBlock = 0;  // current allocation block of openFileDirEntry; if equal to blocksCount, reset to 0 and openFileDirEntry set to null
BYTE    openFileSuccessfulRecords = 0; // count of successful 128-byte records so far, on 8 (8x128B done), reset to 0 and openFileDirEntryCurBlock incremented

// convert null-terminated command-line file name, with a dot, to 11-char 8.3
// cpmName is exactly 11 bytes, padded with spaces, with no null terminator
void convertCmdLineToCPMFileName(const BYTE* cmdLine, BYTE* cpmName)
{
  if (!cmdLine || !cpmName)
  {
    return;
  }
  
  // fill with spaces
  memset(cpmName, 0x20, 11);
  
  BYTE strIndex = 0;
  BYTE outIndex = 0;
  while ((strIndex < strlen(cmdLine)) && (outIndex < 11))
  {
    if (cmdLine[strIndex] == '.')
    {
      outIndex = 8;
      strIndex++;
      continue;
    }
    
    // 7-bit ASCII
    cpmName[outIndex++] = cmdLine[strIndex++] & 0x7F;
  }
}

// vice versa - output string shall have a size of at least 13 bytes (8+3 name, dot, \0)
// cpmName is exactly 11 bytes long, padded with spaces
void convertCPMFileNameToCmdLine(const BYTE* cpmName, BYTE* cmdLine)
{
  if (!cmdLine || !cpmName)
  {
    return;
  }
  
  // fill with zeros
  memset(cmdLine, 0, 13);  
  
  // skip writing dot extension?
  bool extensionWritten = (cpmName[8] & 0x7F) == ' ';
  bool isSpace = false;
  
  BYTE cpmNameIdx = 0;
  BYTE outIndex = 0;
  while (cpmNameIdx < 11)
  {
    isSpace = (cpmName[cpmNameIdx] == ' ');
    if (isSpace || (cpmNameIdx == 8))
    {
      if (!extensionWritten)
      {
        cmdLine[outIndex++] = '.';
        extensionWritten = true;
      }
      
      if (isSpace)
      {
        cpmNameIdx++;
        continue;  
      }      
    }
    
    // 7-bit ASCII, cancel out attributes if in use
    cmdLine[outIndex++] = cpmName[cpmNameIdx++] & 0x7F;
  }
}

// checks for validity of a 11-character CP/M file name (not null terminated)
bool cpmVerifyDirFileName(const BYTE* cpmFileName)
{
  if (!cpmFileName)
  {
    return false;
  }
  
  for (BYTE index = 0; index < FILE_NAME_LENGTH; index++)
  {
    // unprintable character
    if (cpmFileName[index] < 0x20)
    {
      return false;
    }
    
    // 7-bit ASCII (MSB is used for attributes)
    const BYTE sevenBit = cpmFileName[index] & 0x7F;
    
    // found lowercase characters in filename? suspicious - ignore
    if ((sevenBit > 0x60) && (sevenBit < 0x7B))
    {
      return false;
    }
    
    // : ; < = > ?
    else if ((sevenBit > 0x39) && (sevenBit < 0x40))
    {
      return false; 
    }
    
    // . [ ]
    else if ((sevenBit == 0x2E) || (sevenBit == 0x5B) || (sevenBit == 0x5D))
    {
      return false;
    }
  }
  
  return true;
}

// read or write one 128byte sector
// there's usually not enough SECTOR_BUFFER_SIZE to load the whole 26x128B track
// and the sectors are skewed by a factor of 6, so no point of doing multi sector operation
bool cpmReadWriteSector(bool write, WORD logicalAddress)
{
  // logical sector starts at 0, CHS starts at 0/0/1
  BYTE track;
  BYTE head; // 0
  BYTE sector;
  fdc->convertLogicalSectorToCHS(logicalAddress, track, head, sector);
  
  // apply 6:1 interleave from progmem table
  sector = Progmem::cpmSkewSector(sector);
  
  // seek if necessary
  if ((fdc->getCurrentTrack() != track) || (fdc->getCurrentHead() != head))
  {
    fdc->seekDrive(track, head);
  }
  
  // read/write
  fdc->readWriteSectors(write, sector, sector);
  return !fdc->getLastError();
}

// deallocate array
void cpmFreeDirectory()
{ 
  if (!cpmDirectory)
  {
    return;
  }
  
  // deallocate blocks
  for (BYTE index = 0; index < cpmDirectoryCount; index++)
  {
    CPMDir* entry = &cpmDirectory[index];
    if (entry->blocks)
    {
      delete[] entry->blocks;
    }
  }
  
  delete[] cpmDirectory;
  cpmDirectory = NULL;
  cpmDirectoryCount = 0;
}

// read directory entries and allocate dynamic array
bool cpmReadDirectory()
{
  // 4 32-byte entries per one 128-byte sector
  const BYTE entryCount = fdc->getParams()->SectorSizeBytes / DIRECTORY_ENTRY_SIZE;
  
  // double-check
  cpmFreeDirectory();
  
  if (!fdc->verifyTrack0())
  {
    ui->print(Progmem::getString(Progmem::uiNewLine));
    return false;
  }
  
  // determine array size first
  WORD sector = DIRECTORY_SECTOR_START;
  while (sector < DATA_SECTOR_START)
  {
    // read 4 entries in a sector
    if (!cpmReadWriteSector(false, sector))
    {     
      ui->print(Progmem::getString(Progmem::fsDiskError));
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
      return false;
    }
    
    for (BYTE entryIndex = 0; entryIndex < entryCount; entryIndex++)
    {
      // sanity check: address the read buffer
      CPMDirectoryEntry* entry = (CPMDirectoryEntry*)(&g_rwBuffer[entryIndex*DIRECTORY_ENTRY_SIZE]);
      
      // the user number is invalid (shall be 0-15 or 0-31), 0xE5 means deleted/unused entry
      if (entry->userNumber > 31)
      {
        continue;
      }
      
      // invalid filename in entry, skip      
      else if (!cpmVerifyDirFileName(entry->fileName))
      {
        continue;
      }
      
      // valid filename      
      cpmDirectoryCount++;
    }
    
    sector++;
  }
  
  // no files on disk
  if (!cpmDirectoryCount)
  {
    ui->print(Progmem::getString(Progmem::dirCPMEmpty));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
    return false;
  }
  
  // allocate array and initialize with zeros (important)
  cpmDirectory = new CPMDir[cpmDirectoryCount];
  if (!cpmDirectory)
  {
    cpmDirectoryCount = 0;
    
    ui->print(Progmem::getString(Progmem::fsMemoryError));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
    return false;
  }
  
  BYTE cpmDirectoryIndex = 0;
  for (; cpmDirectoryIndex < cpmDirectoryCount; cpmDirectoryIndex++)
  {
    memset(&cpmDirectory[cpmDirectoryIndex], 0, sizeof(CPMDir));
  }  
  
  // load data from disk into the array
  cpmDirectoryIndex = 0;
  sector = DIRECTORY_SECTOR_START;
  while (sector < DATA_SECTOR_START)
  {
    if (!cpmReadWriteSector(false, sector))
    {
      cpmFreeDirectory();
      
      ui->print(Progmem::getString(Progmem::fsDiskError));
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
      return false;
    }
    
    for (BYTE entryIndex = 0; entryIndex < entryCount; entryIndex++)
    {
      // disk buffer entry
      CPMDirectoryEntry* entry = (CPMDirectoryEntry*)(&g_rwBuffer[entryIndex*DIRECTORY_ENTRY_SIZE]);
      
      // exact same sanity check as above
      if (entry->userNumber > 31)
      {
        continue;
      }
      else if (!cpmVerifyDirFileName(entry->fileName))
      {
        continue;
      }
      
      // initialize memory entry
      CPMDir* entryInMemory = &cpmDirectory[cpmDirectoryIndex++];
      memcpy(entryInMemory->fileName, entry->fileName, FILE_NAME_LENGTH);
      entryInMemory->userNumber = entry->userNumber;
      entryInMemory->entrySize = entry->entrySize;
      entryInMemory->entryIndex = ((WORD)(entry->entryIndexHi) << 5) | entry->entryIndexLo;
           
      // determine blocks count by going over the allocation bytes
      for (; entryInMemory->blocksCount < sizeof(entry->allocationBlocks); entryInMemory->blocksCount++)
      {
        // in 250K disks, these are byte values each so stop at a zero (bigger disks: each value is a word, 2 bytes)
        // 0 might mean a hole in the file block, but we don't support that so treat it as an end of file
        if (!entry->allocationBlocks[entryInMemory->blocksCount])
        {
          break;
        }
      }
      
      // entry with no block count
      if (!entryInMemory->blocksCount)
      {
        continue;
      }

      // allocate and copy only those indexes as required
      entryInMemory->blocks = new BYTE[entryInMemory->blocksCount];
      if (!entryInMemory->blocks)
      {
        // out of memory
        cpmFreeDirectory();
        
        ui->print(Progmem::getString(Progmem::fsMemoryError));
        ui->print(Progmem::getString(Progmem::uiNewLine2x));
        return false;
      }
      
      memcpy(entryInMemory->blocks, entry->allocationBlocks, entryInMemory->blocksCount);
    }
    
    sector++;
  }
  
  return true;
}

void cpmDirCommand()
{
  if (!cpmReadDirectory())
  {
    // error already printed
    return;
  }
  
  // reuse the FatFs buffer to sort indexes of CP/M entries (maximum DIRECTORY_ENTRIES is 64)
  // FAT volumes are remounted anyway on drive change
  BYTE* sortedEntryBuf = (BYTE*)getFat()->win;
  BYTE* printBuffer = ui->getPrintBuffer(); // snprintf
  BYTE displayedEntries = 0;
  BYTE totalOccupiedKilos = 0;
  
  // iterate over all directory entries to check for files
  for (BYTE cpmDirectoryIndex = 0; cpmDirectoryIndex < cpmDirectoryCount; cpmDirectoryIndex++)
  {
    CPMDir* entry = &cpmDirectory[cpmDirectoryIndex];

    // did we already go through this one?
    if (entry->userNumber == DIRECTORY_ENTRY_UNUSED)
    {
      continue;
    }
    
    // 0th byte: user number, 1st to 11th: filename with attributes
    BYTE file[FILE_NAME_LENGTH+1];
    memcpy(file, entry, sizeof(file));
    
    //255: uninitialized (current maximum DIRECTORY_ENTRIES is 64, there's space for a little more)
    memset(sortedEntryBuf, 0xFF, DIRECTORY_ENTRIES);
    
    // find all occurences of this particular file over all entries and store indexes where they are spanned
    for (BYTE index = 0; index < cpmDirectoryCount; index++)
    {
      CPMDir* ent = &cpmDirectory[index];
      
      if (memcmp(ent, file, sizeof(file)) == 0)
      {
        // make sorted buffer by entry index
        sortedEntryBuf[ent->entryIndex] = index;
      }
    }    
    
    WORD totalFileSize = 0; // 1: 128 bytes, 2: 256, 0x80: 16K, 0x100: 32K...
    
    // sorted buffer will be used to make sequential reads of file blocks, here just compute the total size
    for (BYTE index = 0; index < DIRECTORY_ENTRIES; index++)
    {
      if (sortedEntryBuf[index] == 0xFF)
      {
        // nothing here
        continue;
      }
      
      CPMDir* ent = &cpmDirectory[sortedEntryBuf[index]];  
      totalFileSize += ent->entrySize;
      
      // mark all of these as done
      ent->userNumber = DIRECTORY_ENTRY_UNUSED;
    }
    
    // null terminated string with a dot between filename and extension
    BYTE niceFileName[13] = {0};
    convertCPMFileNameToCmdLine(&file[1], niceFileName); // file[0] is the user number    
    printBuffer[0] = 0;

    // print user number in file[0] (original is now marked to skip due to multiple entries)
    ui->print(Progmem::getString(Progmem::dirCPMUser), file[0]);
        
    // under 1K, display bytes
    if (totalFileSize < 8)
    {
      snprintf(printBuffer, MAX_CHARS, Progmem::getString(Progmem::dirCPMBytes), totalFileSize*128);  
      
      // file not empty? reserves 1 1K block
      if (totalFileSize)
      {
        totalOccupiedKilos++;  
      }      
    }
    
    // display kilobytes and round
    else
    {
      BYTE kilo = totalFileSize/8;
      if (totalFileSize % 8)
      {
        kilo++;        
      }
      
      snprintf(printBuffer, MAX_CHARS, Progmem::getString(Progmem::dirCPMKilobytes), kilo);  
      totalOccupiedKilos += kilo;
    }
        
    // attributes readonly, archive, system - if supported
    strncat(printBuffer, file[9]  & 0x80 ? "R" : "-", MAX_CHARS);
    strncat(printBuffer, file[11] & 0x80 ? "A" : "-", MAX_CHARS);
    strncat(printBuffer, file[10] & 0x80 ? "S" : "-", MAX_CHARS);
        
    // name
    strncat(printBuffer, " ", MAX_CHARS);
    strncat(printBuffer, niceFileName, MAX_CHARS);
        
    // next entry
    strncat(printBuffer, Progmem::getString(Progmem::uiNewLine), MAX_CHARS);
    displayedEntries++;
    
    // print line
    ui->print(printBuffer);
    
    // on a display, wait for screen fill on the last line
    if (g_uiEnabled && ui->isOnLastLine())
    {
      ui->print(Progmem::getString(Progmem::uiContinueAbort));
      ui->disableKeyboard(false);
      BYTE key = ui->readKey("\r\e");
      ui->disableKeyboard(true);
      ui->print("");
      
      // Escape
      if (key == '\e')
      {
        break;
      }
    }
  }
  
  cpmFreeDirectory();
  
  // no files
  if (!displayedEntries)
  {
    ui->print(Progmem::getString(Progmem::dirCPMEmpty));    
  }
  
  // show quick summary; 241K total available space (250K minus reserved sectors)
  else
  {
    ui->print(Progmem::getString(Progmem::dirCPMSummary),
              totalOccupiedKilos, displayedEntries, 241 - totalOccupiedKilos);
  }
  
  ui->print(Progmem::getString(Progmem::uiNewLine));
}

// find specified file, get its size and all of its entries in sequential order
// given fileName null terminated and userNumber (0-31)
// processing analog to cpmReadDirectory above
bool cpmOpenFile(const BYTE* cmdLine, BYTE userNumber)
{
  // reset these
  openFileSectorCount = 0;
  openFileSector = 0;
  openFileDirEntry = NULL;
  openFileDirEntryIndex = 0;
  openFileDirEntryCurBlock = 0;
  openFileSuccessfulRecords = 0;
  
  // wait what
  if (userNumber == DIRECTORY_ENTRY_UNUSED)
  {
    return false;
  }
  
  // remount
  if (!cpmReadDirectory())
  {
    return false;
  }
  
  BYTE file[FILE_NAME_LENGTH+1];
  file[0] = userNumber;
  convertCmdLineToCPMFileName(cmdLine, &file[1]);
  
  BYTE* sortedEntryBuf = (BYTE*)getFat()->win;
  
  for (BYTE cpmDirectoryIndex = 0; cpmDirectoryIndex < cpmDirectoryCount; cpmDirectoryIndex++)
  {
    CPMDir* entry = &cpmDirectory[cpmDirectoryIndex];
    if (entry->userNumber != userNumber) // not this user number
    {
      continue;
    }
    
    memset(sortedEntryBuf, 0xFF, DIRECTORY_ENTRIES);     
    bool fileFound = false;
    
    for (BYTE index = 0; index < cpmDirectoryCount; index++)
    {
      CPMDir* ent = &cpmDirectory[index];      
      
      // we are not using the array to display directory contents
      // so get rid of any attributes (incl. user-defined), otherwise memcmp would fail
      for (BYTE chr = 0; chr < FILE_NAME_LENGTH; chr++)
      {
        ent->fileName[chr] &= 0x7F;
      }
      
      if (memcmp(ent, file, sizeof(file)) == 0)
      {
        sortedEntryBuf[ent->entryIndex] = index;
        fileFound = true;
      }
    }
    
    if (!fileFound)
    {
      cpmFreeDirectory();
      
      ui->print(Progmem::getString(Progmem::fsCPMFileNotFound), userNumber);
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
      return false;
    }
       
    for (BYTE index = 0; index < DIRECTORY_ENTRIES; index++)
    {
      if (sortedEntryBuf[index] == 0xFF)
      {
        continue;
      }
      
      CPMDir* ent = &cpmDirectory[sortedEntryBuf[index]];     
      openFileSectorCount += ent->entrySize;
    }
       
    // the file was found, but it is empty? no point on continuing
    if (!openFileSectorCount)
    {
      cpmFreeDirectory();
      
      ui->print(Progmem::getString(Progmem::fsCPMFileEmpty));
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
      return false;
    }
    
    // opened, size valid, sorted buffer ready
    return true;
  }
  
  cpmFreeDirectory();
  
  ui->print(Progmem::getString(Progmem::fsCPMFileNotFound), userNumber);
  ui->print(Progmem::getString(Progmem::uiNewLine2x));
  return false;
}

// alias to cpmFreeDirectory
void cpmCloseFile()
{
  cpmFreeDirectory();
}

// read one CP/M file record (128 bytes), just right for XMODEM that stemmed from this...
// returns false on error, or true if success (and true with size set to 0 if end of file)
// on success, current file record is in g_rwBuffer[0..127]
bool cpmReadFileRecord(BYTE& size)
{
  // error: nothing opened
  if (!openFileSectorCount || !cpmDirectory)
  {
    size = 0;
    return false;
  }

  // end
  if (openFileSector == openFileSectorCount)
  {
    cpmFreeDirectory();
    
    size = 0;
    return true;
  }
  
  // get next sequential directory entry if at the end of all allocation blocks (or doing first time read)
  const BYTE* sortedEntryBuf = (const BYTE*)getFat()->win;    
  while(!openFileDirEntry && (openFileDirEntryIndex < DIRECTORY_ENTRIES))
  {
    if (sortedEntryBuf[openFileDirEntryIndex] == 0xFF)
    {
      openFileDirEntryIndex++;
      continue;
    }
    
    openFileDirEntry = &cpmDirectory[sortedEntryBuf[openFileDirEntryIndex]];
    openFileDirEntryIndex++;
    
    // also check for allocation blocks count
    if (openFileDirEntry && openFileDirEntry->blocksCount)
    {
      break;
    }    
  }
  
  // this should not happen, but we're at the end (openFileSector should reach sector count before this)
  if (openFileDirEntryIndex == DIRECTORY_ENTRIES)
  {
    cpmFreeDirectory();
    
    size = 0;
    return true;
  }
  
  // translate the 1K CP/M block to sector address and read
  // file blocks are numbered starting the directory extents
  WORD address = DIRECTORY_SECTOR_START + ((WORD)openFileDirEntry->blocks[openFileDirEntryCurBlock] * 8);
  address += openFileSuccessfulRecords;
  if (!cpmReadWriteSector(false, address))
  {
    // cannot read one single sector after multiple retries, abort
    cpmFreeDirectory();
    
    ui->print(Progmem::getString(Progmem::fsDiskError));
    ui->print(Progmem::getString(Progmem::uiNewLine2x));
    
    size = 0;
    return false;
  }
  
  openFileSector++;
  openFileSuccessfulRecords++;
  
  // file bigger than 1K (8x128B reached), reset records per block counter and advance to next allocation block
  if (openFileSuccessfulRecords == 8)
  {
    openFileSuccessfulRecords = 0;
    openFileDirEntryCurBlock++;
    
    // block count reached for given entry
    // if openFileSector < openFileSectorCount, file is bigger than 16K and a next entry follows
    if (openFileDirEntryCurBlock == openFileDirEntry->blocksCount)
    {
      openFileDirEntryCurBlock = 0;
      openFileDirEntry = NULL; // search for next one
    }
  }
  
  // fdc->getParams()->SectorSizeBytes, 128 here
  size = 128;
  return true;  
}

// deletes all entries of given file for all users
bool cpmDeleteFile(const BYTE* cmdLine)
{
  const BYTE entryCount = fdc->getParams()->SectorSizeBytes / DIRECTORY_ENTRY_SIZE; 
  
  // this routine does not require directory entries in memory
  cpmFreeDirectory();
  
  if (!fdc->verifyTrack0())
  {
    ui->print(Progmem::getString(Progmem::uiNewLine));
    return false;
  }
  
  bool deleteResult = false; // deleted at least 1 entry
  BYTE cpmFileName[FILE_NAME_LENGTH];
  convertCmdLineToCPMFileName(cmdLine, cpmFileName);
  
  WORD sector = DIRECTORY_SECTOR_START;
  while (sector < DATA_SECTOR_START)
  {
    // read
    if (!cpmReadWriteSector(false, sector))
    {
      ui->print(Progmem::getString(Progmem::fsDiskError));
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
      return false;
    }
    
    for (BYTE entryIndex = 0; entryIndex < entryCount; entryIndex++)
    {
      CPMDirectoryEntry* entry = (CPMDirectoryEntry*)(&g_rwBuffer[entryIndex*DIRECTORY_ENTRY_SIZE]);
      if (entry->userNumber == DIRECTORY_ENTRY_UNUSED)
      {
        continue;
      }      
      if (!cpmVerifyDirFileName(entry->fileName))
      {
        continue;
      }
      
      // erase any file attributes
      for (BYTE chr = 0; chr < FILE_NAME_LENGTH; chr++)
      {
        entry->fileName[chr] &= 0x7F;
      }
      
      if (memcmp(entry->fileName, cpmFileName, sizeof(cpmFileName)) != 0)
      {
        continue;
      }
      
      // found, write
      entry->userNumber = DIRECTORY_ENTRY_UNUSED;      
      if (!cpmReadWriteSector(true, sector))
      {
        ui->print(Progmem::getString(Progmem::fsDiskError));
        ui->print(Progmem::getString(Progmem::uiNewLine2x));
        return false;
      }
      
      // continue in deletion
      // - there might be identical filenames for files bigger than 16K
      // or identical filenames for different user numbers
      deleteResult = true;
    }
    
    sector++;
  }
  
  // not found anything
  if (!deleteResult)
  {
    ui->print(Progmem::getString(Progmem::fsCPMNoFilesFound));
    ui->print(Progmem::getString(Progmem::uiNewLine));
  }
  ui->print(Progmem::getString(Progmem::uiNewLine));
  
  return deleteResult;
}

// prints file of user 0 to screen
void cpmDumpFile(const BYTE* fileName)
{  
  if (!cpmOpenFile(fileName, 0))
  {
    return;
  }
  
  // read in 128-byte records
  BYTE recordSize;
  cpmReadFileRecord(recordSize);
  
  while (recordSize)
  {
    // MAX_CHARS can be configured below 128
    const BYTE printLength = min(MAX_CHARS, recordSize);
    BYTE printCount = 0;
    
    while (printCount < recordSize)
    {
      memcpy(ui->getPrintBuffer(), &g_rwBuffer[printCount], printLength);
      
      ui->setPrintLength(printLength);
      ui->print(ui->getPrintBuffer()); 
      
      printCount += printLength;
    }
    
    cpmReadFileRecord(recordSize);
  }
  
  // file is auto closed on end
  ui->print(Progmem::getString(Progmem::uiNewLine));
}

// destroys all directory entries
void cpmQuickFormat()
{
  // verify reading of track 0, inform "try formatting" if it fails
  if (!fdc->verifyTrack0(true))
  {
    ui->print(Progmem::getString(Progmem::uiNewLine));
    return;
  }
  
  // prepare write buffer with an "unused" filler
  memset(&g_rwBuffer[0], DIRECTORY_ENTRY_UNUSED, fdc->getParams()->SectorSizeBytes);
  
  // if the disk is bootable and this behavior shall be preserved, set this from 0 to DIRECTORY_SECTOR_START
  // to save CP/M on the reserved sectors - but normally, a "quick format" gets rid of the old "boot sector" on FAT12 too
  WORD sector = 0;
  while (sector < DATA_SECTOR_START)
  {
    if (!cpmReadWriteSector(true, sector))
    {     
      ui->print(Progmem::getString(Progmem::fsDiskError));
      ui->print(Progmem::getString(Progmem::uiNewLine2x));
      return false;
    }
    
    sector++;
  }
    
  ui->print(Progmem::getString(Progmem::uiOK));
  ui->print(Progmem::getString(Progmem::uiNewLine));
  return true;
}

