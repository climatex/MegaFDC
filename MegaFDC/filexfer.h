// MegaFDC (c) 2023-2026 J. Bogin, http://boginjr.com
// File transfer helpers (XMODEM, SD)

#pragma once

// public forward declarations
bool xmodemReadDiskIntoImageFile(bool useXMODEM_1K);
bool xmodemWriteDiskFromImageFile(bool useXMODEM_1K);
bool xmodemSendFile(const BYTE* existingFileName);
bool xmodemReceiveFile(const BYTE* newFileName);

// equiv. SD
bool sdReadDiskIntoImageFile();
bool sdWriteDiskFromImageFile();
bool fileFloppyToSd(const BYTE* existingFileName);
bool fileSdToFloppy(const BYTE* newFileName);

// make these helpers public when building with IMD
#ifdef BUILD_IMD_IMAGER

int xmodemRx(int msDelay);
void xmodemTx(const char *data, int size);
void dumpSerialTransfer();

#endif



