// MegaFDC (c) 2023-2025 J. Bogin, http://boginjr.com
// FatFs interface for filesystem support

#pragma once

// verify FAT operation macros
#define FAT_EXECUTE(fn)       if (fatResult((fn)) != FR_OK) return;
#define FAT_EXECUTE_0(fn)     if (fatResult((fn)) != FR_OK) return 0;
#define FAT_EXECUTE_DIR(fn)   if (fatResult((fn)) != FR_OK) { f_closedir(&dir); return; }
#define FAT_EXECUTE_FILE(fn)  if (fatResult((fn)) != FR_OK) { f_close(getFatFile()); return; }

// public functions
FRESULT fatResult(FRESULT result);
FATFS* getFat();
FIL* getFatFile();

bool fatMount();
bool fatChdir();
void fatDirCommand();
void fatQuickFormat();
void fatMkdir(const BYTE* dirName);
void fatRmdir(const BYTE* dirName);
void fatDeleteFile(const BYTE* fileName);
void fatDumpFile(const BYTE* fileName);
void fatWriteTextFile(const BYTE* fileName);
