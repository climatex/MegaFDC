// MegaFDC (c) 2023 J. Bogin, http://boginjr.com
// XMODEM helpers

#pragma once

// public forward declarations
bool xmodemReadDiskIntoImageFile(bool useXMODEM_1K);
bool xmodemWriteDiskFromImageFile(bool useXMODEM_1K);

bool xmodemSendFile(const BYTE* existingFileName);
bool xmodemReceiveFile(const BYTE* newFileName);