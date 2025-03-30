// MegaFDC (c) 2023-2025 J. Bogin, http://boginjr.com
// IMD (ImageDisk)-compatible file disk imager

#pragma once

void InitializeDrives();
void ProcessCommands();

#define CHECK_STREAM_END    if (packetIdx >= size) return true;
#define SECTORS_TABLE_COUNT 128

class IMD
{
public:
  IMD();
  virtual ~IMD() {}
  
  FDC::DiskDriveMediaParams& getParams() { return m_params; }
  
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
  
  bool readDiskCallback(DWORD packetNo, BYTE* data, WORD size);
  bool writeDiskCallback(DWORD packetNo, BYTE* data, WORD size);
  void cleanupCallback();
  
private:
  bool autodetectCommRate();
  bool autodetectHeads();
  bool autodetectDoubleStep();
  WORD* autodetectSectorsPerTrack(BYTE& observedSPT, BYTE& maximumSPT);
  bool autodetectInterleave();
  void autodetectGaps(BYTE& sectorGap, BYTE& formatGap);
  bool tryAskIfCannotAutodetect(bool requiredDoubleStep, bool requiredHeads);
  void printGeometryInfo(BYTE track, BYTE head, BYTE interleave);
  
  FDC::DiskDriveMediaParams m_params;
  
  bool m_autoDoubleStep;
  bool m_autoHeads;
  bool m_autoSectorGap;
  bool m_autoFormatGap;
  bool m_xlat300and250;
  
  BYTE m_formatInterleave;
  WORD m_lastGoodCommRate;
  BYTE m_lastGoodUseFM;
  
  // XMODEM callbacks
  bool m_cbSuccess;
  bool m_cbDoVerify;
  bool m_cbModeSpecified;
  bool m_cbTrackSpecified;
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
  BYTE m_cbTrack;
  BYTE m_cbHead;
  BYTE m_cbSpt;
  BYTE m_cbSecSize;
  BYTE m_cbInterleave;
  BYTE m_cbSectorDataType;
  BYTE m_cbSectorIdx; 
  BYTE m_cbStartingSectorIdx;
  WORD m_cbSecSizeBytes;  
  WORD m_cbLastPos;
  WORD m_cbLastPos2;
  WORD m_cbTotalBadSectorsDisk;
  WORD m_cbTotalBadSectorsFile;
  BYTE m_cbUnreadableTracks;
  BYTE* m_cbSectorNumberingMap;
  BYTE* m_cbSectorTrackMap;
  BYTE* m_cbSectorHeadMap;
  WORD* m_cbSectorsTable;
  BYTE m_cbResponseStr[70];
};