// MegaFDC (c) 2023-2025 J. Bogin, http://boginjr.com
// XMODEM helpers

#pragma once

// public forward declarations
bool xmodemReadDiskIntoImageFile(bool useXMODEM_1K);
bool xmodemWriteDiskFromImageFile(bool useXMODEM_1K);

bool xmodemSendFile(const BYTE* existingFileName);
bool xmodemReceiveFile(const BYTE* newFileName);

// make these helpers public when building with IMD
#ifdef BUILD_IMD_IMAGER

int xmodemRx(int msDelay);
void xmodemTx(const char *data, int size);
void dumpSerialTransfer();

#endif



