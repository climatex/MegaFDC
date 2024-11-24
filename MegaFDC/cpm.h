// MegaFDC (c) 2023-2024 J. Bogin, http://boginjr.com
// CP/M filesystem for 8" SSSD/FM (250K)
// IBM 3740 single-sided geometry 77x26x128B

#pragma once

bool cpmOpenFile(const BYTE* cmdLine, BYTE userNumber);
bool cpmReadFileRecord(BYTE& size);
void cpmCloseFile();
bool cpmDeleteFile(const BYTE* cmdLine);
void cpmDumpFile(const BYTE* fileName);

void cpmDirCommand();
void cpmQuickFormat();