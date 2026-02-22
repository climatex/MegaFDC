// MegaFDC (c) 2023-2026 J. Bogin, http://boginjr.com
// IMD mode

#pragma once

void InitializeDrives();
void ProcessCommands();

#define SECTORS_TABLE_COUNT 128

class IMD
{
public:
  IMD();
  virtual ~IMD() {}
  
  void setAutodetectDoubleStep(bool set) { m_autoDoubleStep = set; }
  void setAutodetectHeads(bool set) { m_autoHeads = set; }
  void setAutodetectSectorGap(bool set) { m_autoSectorGap = set; }
  void setAutodetectFormatGap(bool set) { m_autoFormatGap = set; }  
  void setFormatInterleave(BYTE set) { m_formatInterleave = set; }
  void setDataRateTranslations(bool xlat300_250) { m_xlat300and250 = xlat300_250; }
  
  void readDisk();
  void writeDisk();
  void formatDisk();
  void eraseDisk();
  void testController();
  void testDrive();
   
private:
  bool autodetectCommRate();
  bool autodetectHeads();
  bool autodetectDoubleStep();
  WORD* autodetectSectorsPerTrack(BYTE& observedSPT, BYTE& maximumSPT);
  bool autodetectInterleave();
  void autodetectGaps(BYTE& sectorGap, BYTE& formatGap);
  bool tryAskIfCannotAutodetect(bool requiredDoubleStep, bool requiredHeads);
  void printGeometryInfo(BYTE cyl, BYTE head, BYTE interleave);
  void cleanup();
  
  bool m_autoDoubleStep;
  bool m_autoHeads;
  bool m_autoSectorGap;
  bool m_autoFormatGap;
  bool m_xlat300and250;
  
  BYTE m_formatInterleave;
  BYTE m_formatStartSector;
  WORD m_lastGoodCommRate;
  BYTE m_lastGoodUseFM;
  
  // shared between both XMODEM and SD implementations
  BYTE m_cbInterleave;
  WORD* m_cbSectorsTable;
  BYTE* m_cbSectorNumberingMap;
  BYTE* m_cbSectorTrackMap;
  BYTE* m_cbSectorHeadMap; 
  
#ifdef SD_CARD_SUPPORT

  bool readDiskInternal(File* file, BYTE* responseStr, WORD& totalBadSectorsDisk, BYTE& unreadableTracks);
  bool writeDiskInternal(bool skipBadSectorsInFile, bool doVerify, File* file, BYTE* responseStr, WORD& totalBadSectorsDisk, WORD& totalBadSectorsFile);

#else
  
public:
  
  // XMODEM callbacks  
  bool readDiskCallback(DWORD packetNo, BYTE* data, WORD size);
  bool writeDiskCallback(DWORD packetNo, BYTE* data, WORD size);
  
private:
  
  bool m_cbSuccess;
  bool m_cbDoVerify;
  bool m_cbModeSpecified;
  bool m_cbCylinderSpecified;
  bool m_cbHeadSpecified;
  bool m_cbSptSpecified;
  bool m_cbSecSizeSpecified;
  bool m_cbSecMapSpecified;
  bool m_cbSecTrackMapSpecified;
  bool m_cbSecHeadMapSpecified;
  bool m_cbSecDataTypeSpecified;
  bool m_cbProcessingHeader;
  bool m_cbHasSecTrackMap;
  bool m_cbHasSecHeadMap;
  bool m_cbGeometryChanged;
  bool m_cbSeekIndicated;
  bool m_cbSkipBadSectorsInFile;
  BYTE m_cbMode;
  BYTE m_cbCylinder;
  BYTE m_cbHead;
  BYTE m_cbSpt;
  BYTE m_cbSecSize;  
  BYTE m_cbCurrentSector;
  BYTE m_cbSectorDataType;
  BYTE m_cbSectorIdx; 
  BYTE m_cbStartingSectorIdx;
  WORD m_cbSecSizeBytes;  
  WORD m_cbLastPos;
  WORD m_cbLastPos2;
  WORD m_cbTotalBadSectorsDisk;
  WORD m_cbTotalBadSectorsFile;
  BYTE m_cbUnreadableTracks;
  BYTE m_cbResponseStr[70];

#endif
};