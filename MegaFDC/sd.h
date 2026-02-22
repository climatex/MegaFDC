// MegaFDC (c) 2023-2026 J. Bogin, http://boginjr.com
// SD card routines

bool  SdDetectMemoryCard();
File* SdPickFile(bool writingImage, const BYTE* filterExt = NULL);