// MegaFDC (c) 2023-2025 J. Bogin, http://boginjr.com
// Program memory data

#pragma once
#include "config.h"

// bleh
#define PROGMEM_STR inline static const unsigned char

class Progmem
{
public:
  
  // enum in sync with order of m_stringTable
  enum BYTE
  {
    // basic UI    
    uiDeleteLine = 0,
    uiNewLine,
    uiNewLine2x,
    uiEchoKey,
    uiEnterKey,
    uiVT100ClearScreen,
    uiOK,
    uiFAIL,
    uiEnabled,
    uiDisabled,
    uiYes,
    uiNo,
    uiNotAvailable,
    uiAbort,
    uiContinue,
    uiContinueAbort,
    uiDecimalInput,
    uiHexadecimalInput,
    uiBytes,
    uiOperationPending,
    uiCancelOption,
    uiChooseOption,
    
    // startup related   
    uiSplash,
    uiBuild,
    uiNoKeyboard,
    uiInitializingFDC,
    uiDisabled1,
    uiDisabled2,
    uiFatalError,
    uiSystemHalted,
    uiCtrlAltDel,
    uiLoadingConfig,
    uiLoadingAborted,
    
    // FDC
    errRQMTimeout,
    errRecalibrate,
    errSeek,
    errWriProtect,
    errINTTimeout,
    errOverrun,
    errCRC,
    errNoData,
    errNoAddrMark,
    errBadTrack,
    errChsFmtSTRegsSingle,
    errChsFmtSTRegsMulti,
    errChsFmtSingleSector,
    errChsFmtMultiSector,
    errTrack0Error,
    errTryFormat,
	
    // MegaFDC command line
#ifndef BUILD_IMD_IMAGER        
    
    // generic user commands
    cmdHelp,
    cmdSupportedIndex,
    cmdReset,
    cmdDrivParm,
    cmdPersist,
    cmdFormat,
    cmdVerify,
    cmdImage,
    // filesystem user commands
    cmdFSIndex,
    cmdQuickFormat,
    cmdPath,
    cmdCd,
    cmdMd,
    cmdRd,
    cmdDir,
    cmdType,
    cmdTypeInto,
    cmdDel,    
    cmdXfer,
    // end of user commands
    cmdEndIndex,
    
    // CD.  CD..  CD\ variants
    varCd1,
    varCd2,
    varCd3,
    
    // details for HELP
    helpDetails,
    helpReset1,
    helpReset2,
    helpReset3,
    helpDrivParm1,
    helpDrivParm2,
    helpCurrentDrive,
    helpPersist1,
    helpPersist2,
    helpPersist3,
    helpFormat1,
    helpFormat2,
    helpVerify1,
    helpVerify2,
    helpImage1,
    helpImage2,
    helpImage3,
    helpQuickFormat1,
    helpQuickFormat2,
    helpPath1,
    helpPath2,
    helpPath3,
    helpPath4,
    helpCd1,
    helpCd2,
    helpCd3,
    helpCd4,
    helpDir1,
    helpDir2,
    helpDir3,
    helpDel1,
    helpDel2,
    helpType1,
    helpType2,
    helpMd1,
    helpMd2,
    helpRd1,
    helpRd2,
    helpTypeInto1,
    helpTypeInto2,
    helpTypeInto3,
    helpXfer1,
    helpXfer2,
    
    // startup command: initialize drives
    waitingForDrives,
    countDrives,
    specifyParams1,
    specifyParams2,
    driveInches,
    drive8InchText1,
    drive8InchText2,
    drive8InchText3, 
    drive8InchText4,
    drive8InchText5,
    drive8InchText6,
    drive5InchSides,
    drive5InchSSDD,
    drive5Inch,
    drive5InDoubleStep,
    drive3Inch,
    drive3Inch_288,
    
    specifyingAll,
    customCyls,
    customDoubleStep,
    customChangeline,
    customHeads,
    customSpt,
    customSectorSize,
    customSectorGap,
    customFormatGap3,
    customFormatFiller,
    customEncoding,
    customDataRateSel,
    custom8InchEnc,
    custom5InchFM,
    custom5InchMFM,
    custom3InchFM,
    custom3InchMFM,
    custom3InchMFM_1M,
    customEDModeInfo,
    customEDMode,
    customUseFAT,
    customMediaByte,
    customRootDir,
    customClusterSize,
    
    // commands processor
    processInvalidCmd,
    processInvalidDrv,
    processUnknownDrv,
    processNoFS,
    processInvalidChr1,
    processInvalidChr2,
    processMaxPath,
    
    // DRIVPARM command
    drivParmCaption,
    drivParmTypeText,
    drivParmType8SD,
    drivParmType8DD,
    drivParmType5DD,
    drivParmType5HD,
    drivParmType3DD,
    drivParmType3HD,
    drivParmType3ED,
    drivParmDoubleStep,
    drivParmDiskChange,
    drivParmSRT,
    drivParmHLT,
    drivParmHUT,
    drivParmCyls,
    drivParmHeads,
    drivParmHeadsOne,
    drivParmHeadsTwo,
    drivParmSpt,
    drivParmSectorSize,
    drivParmEncoding,
    drivParmFM,
    drivParmMFM,
    drivParmDataRate,
    drivParmFDCRate,
    drivParmKbps,
    drivParmMbps,
    drivParmSectorGap,
    drivParmFormatGap,
    drivParmFormatFill,
    drivParmEDMode,
    drivParmFS,
    drivParmFAT,
    drivParmCPM,
    drivParmFATMedia,
    drivParmFATRootDir,
    drivParmFATCluster,
    
    // PERSIST
    persistStore,
    persistRemove,
    
    // FORMAT, VERIFY, QFORMAT, IMAGE  
    diskIoFormatVerify,
    diskIoCreateFS,
    diskIoInsertDisk, 
    diskIoTotalBadSect,
    diskIoTotalBadTrk,
    diskIoProgress,    
    diskIoQuickFormat,
    diskIoCreatingFAT,
    diskIoCreatingCPM,
    diskIoFormatOK,
    
    // IMAGE, XFER and xmodem related
    xferReadFile,
    xferSaveFile,
    imageReadDisk,
    imageWriteDisk,
    imageTransferLen,
    imageGeometry,
    xmodemUse1k,
    xmodemPrefix,
    xmodem1kPrefix,
    xmodemWaitSend,
    xmodemWaitRecv,
    xmodemTransferEnd,
    xmodemTransferFail,
    
    // DIR
    dirDirectory,
    dirDirectoryEmpty,
    dirBytesFormat,
    dirBytesFree,
    dirCPMUser,
    dirCPMBytes,
    dirCPMKilobytes,
    dirCPMEmpty,
    dirCPMSummary,
    
    // TYPEINTO
    typeIntoCaption,
    
    // Filesystem related
    fsCurrentDrive,
    fsForbiddenCharFAT,
    fsForbiddenCharCPM,
    fsDiskError,
    fsInternalError,
    fsFileNotFound,
    fsPathNotFound,
    fsDirectoryFull,
    fsFileExists,
    fsInvalidObject,
    fsNoVolumeWorkArea,
    fsMkfsError,
    fsNoFAT,
    fsMemoryError,
    fsInvalidParameter,
    fsCPMFileNotFound,
    fsCPMFileEmpty,
    fsCPMNoFilesFound
       
    // IMD imager
#else
    imdSplash,
    imdLCDRecommended,
    imdDriveLetter,
    imdDriveType,
    imdDriveCyls,
    imdDoubleStepAuto,
    imdDoubleStep,
    imdDiskSidesAuto,
    imdDiskSides,
    imdInterleave,
    imdSectorGap,
    imdFormatGap,
    imdAdvancedDetails,
    imdDefaultsNo,
    imdXlate300kbps,
    imdSpecifyOptions,
    imdSpecifyOptions2,
    imdEscGoBack,
    
    imdInsertGoodDisk,
    imdInsertWriteDisk,
    imdInsertFormat,
    imdInsertErase, 
    imdInsertRead,
    imdInsertWrite,
    imdNoteTestWrite,
    imdNoteNoFS,
    imdMemoryError,

    imdOptionRead,
    imdOptionWrite,
    imdOptionFormat,
    imdOptionErase, 
    imdOptionTests,
    imdOptionXlat,
    imdOptionReset,
    
    imdTestFDC,
    imdTestDrive,
    imdTestSkipped,
    imdTestController,
    imdTestHD, 
    imdTestED,
    imdTest8inch,    
    imdTestMFM128,
    imdTestMedia,
    imdTestDriveSeek,
    imdTestRPM,
    imdRPM,
    imdDiskReadFail,
    imdSyncSectorFail,
    
    imdGeoCylsHdStep,
    imdSingleStepping,
    imdDoubleStepping,
    imdInterleaveGaps,
    imdGeoGapSizesAuto,
    imdGeoGapSizesUser,
    imdGeoChsSpt,
    imdGeoRateGap3,
    imdProgress,
    
    imdUseSameParams,
    imdFormatParams,
    imdFormatSpt,
    imdFormatStartSec1,
    imdFormatStartSec2,
    imdFormatEncoding,
    imdFormatRate,
    imdFormatRatesMFM,
    imdFormatRatesFM,
    imdFormatSecSize1,
    imdFormatSecSize2,
    imdFormatSecSize3, 
    imdBadSectorsDisk,
    imdBadSectorsFile,
    
    imdXmodem,
    imdXmodem1k,
    imdXmodemUse1k,
    imdXmodemSkipBad,
    imdXmodemVerify,
    imdXmodemWaitSend,
    imdXmodemWaitRecv,
    imdXmodemXferEnd,
    imdXmodemXferFail,
    imdXmodemErrPacket,
    imdXmodemErrHeader,
    imdXmodemErrMode,
    imdXmodemErrCyls,
    imdXmodemErrHead,
    imdXmodemErrSpt,
    imdXmodemErrSsize,
    imdXmodemLowRAM,
    imdXmodemErrData,
    
    imdWriteHeader,
    imdWriteComment,
    imdWriteDone,
    imdWriteEnterEsc,
    imdTrackUnreadable,
    imdUnreadableTrks,
    imdRunPython
#endif
  };
  
  // retrieve string from progmem, buffer valid until next call
  static const unsigned char* getString(unsigned char stringIndex)
  {   
    strncpy_P(m_strBuffer, pgm_read_ptr(&(m_stringTable[stringIndex])), MAX_PROGMEM_STRING_LEN);
    return (const unsigned char*)&m_strBuffer[0];
  }
  
  // translate PS2/AT keycode to ASCII from xlatable in progmem
  static const unsigned char keybToChar(unsigned char keyb)
  {
    return pgm_read_byte(&(m_keybXlatTable[keyb]));
  }

#ifndef BUILD_IMD_IMAGER
  // translate physical sector to CP/M sector, 1-based
  static const unsigned char cpmSkewSector(unsigned char sector)
  {
    return pgm_read_byte(&(m_cpmSkewTable[sector]));
  }
#else
  // index the IMD gaps table
  static const unsigned char imdGapTable(unsigned char index)
  {
    return pgm_read_byte(&(m_imdGapTable[index]));
  }
#endif

// messages (definition order does not matter here). Length max MAX_PROGMEM_STRING_LEN
// variadics can be loaded only one at a time from PROGMEM,
// as only one buffer is used to transfer these between program space and our RAM
private:

// basic UI
  PROGMEM_STR m_uiDeleteLine[]       PROGMEM = "\r                               \r";
  PROGMEM_STR m_uiNewLine[]          PROGMEM = "\r\n";
  PROGMEM_STR m_uiNewLine2x[]        PROGMEM = "\r\n\r\n";
  PROGMEM_STR m_uiEchoKey[]          PROGMEM = "%c\r\n";
  PROGMEM_STR m_uiEnterKey[]         PROGMEM = "\r";
  PROGMEM_STR m_uiVT100ClearScreen[] PROGMEM = "\e[H\e[2J\r        \r";
  PROGMEM_STR m_uiOK[]               PROGMEM = "OK";
  PROGMEM_STR m_uiFAIL[]             PROGMEM = "FAIL";
  PROGMEM_STR m_uiEnabled[]          PROGMEM = "enabled";
  PROGMEM_STR m_uiDisabled[]         PROGMEM = "disabled";
  PROGMEM_STR m_uiYes[]              PROGMEM = "yes";
  PROGMEM_STR m_uiNo[]               PROGMEM = "no";
  PROGMEM_STR m_uiNotAvailable[]     PROGMEM = "N/A";
  PROGMEM_STR m_uiAbort[]            PROGMEM = "Press Esc now to abort.";
  PROGMEM_STR m_uiContinue[]         PROGMEM = "ENTER to continue...";
  PROGMEM_STR m_uiContinueAbort[]    PROGMEM = "ENTER: continue, Esc: abort...";
  PROGMEM_STR m_uiDecimalInput[]     PROGMEM = "0123456789\r\b";
  PROGMEM_STR m_uiHexadecimalInput[] PROGMEM = "0123456789ABCDEF\r\b";
  PROGMEM_STR m_uiBytes[]            PROGMEM = "bytes";
  PROGMEM_STR m_uiOperationPending[] PROGMEM = "Busy...";
  PROGMEM_STR m_uiCancelOption[]     PROGMEM = "(C)ancel\r\n";
  PROGMEM_STR m_uiChooseOption[]     PROGMEM = "Choose: ";
  
// startup related
  PROGMEM_STR m_uiSplash[]           PROGMEM = "MegaFDC (c) 2023-2025 J. Bogin\r\nRegular build, ";
  PROGMEM_STR m_uiBuild[]            PROGMEM = "27 Jun 2025\r\n";
  PROGMEM_STR m_uiNoKeyboard[]       PROGMEM = "No keyboard, using serial input\r\n";
  PROGMEM_STR m_uiInitializingFDC[]  PROGMEM = "\r\nInitializing controller...";
  PROGMEM_STR m_uiDisabled1[]        PROGMEM = "- LCD and keyboard not enabled -";
  PROGMEM_STR m_uiDisabled2[]        PROGMEM = "Use switch to enable, then reset";
  PROGMEM_STR m_uiFatalError[]       PROGMEM = "Fatal error:";
  PROGMEM_STR m_uiSystemHalted[]     PROGMEM = "\r\nSystem halted, reset required.\r\n";
  PROGMEM_STR m_uiCtrlAltDel[]       PROGMEM = "Press CTRL+ALT+DEL to reset...";
  PROGMEM_STR m_uiLoadingConfig[]    PROGMEM = "Loading parameters from EEPROM.\r\n";
  PROGMEM_STR m_uiLoadingAborted[]   PROGMEM = "Loading aborted";

// FDC errors that abort the operation (first three halt the system)
  PROGMEM_STR m_errRQMTimeout[]      PROGMEM = "\r\nController not responding\r\n";
  PROGMEM_STR m_errRecalibrate[]     PROGMEM = "\r\nDrive %c: not responding\r\n";
  PROGMEM_STR m_errSeek[]            PROGMEM = "\r\nDrive %c: not seeking properly\r\n";
  PROGMEM_STR m_errWriProtect[]      PROGMEM = "\r\nDisk is write protected\r\n";
  PROGMEM_STR m_errINTTimeout[]      PROGMEM = "\r\nNo disk in drive";
// FDC errors next to CHS information - these allow continuing
  PROGMEM_STR m_errOverrun[]         PROGMEM = "Data overrun";
  PROGMEM_STR m_errCRC[]             PROGMEM = "CRC error";
  PROGMEM_STR m_errNoData[]          PROGMEM = "Sector not found";
  PROGMEM_STR m_errNoAddrMark[]      PROGMEM = "No address mark";
  PROGMEM_STR m_errBadTrack[]        PROGMEM = "Bad track";
// formatted FDC errors
  PROGMEM_STR m_chsFmtSTRegsSingle[] PROGMEM = "\rCHS %02d/%d/%02d ST0,1,2 %02x,%02x,%02x";
  PROGMEM_STR m_chsFmtSTRegsMulti[]  PROGMEM = "\rCHS %02d/%d/%02d-%02d:\r\nST0,1,2 %02x,%02x,%02x";
  PROGMEM_STR m_chsFmtSingleSector[] PROGMEM = "\rCHS %02d/%d/%02d ";
  PROGMEM_STR m_chsFmtMultiSector[]  PROGMEM = "\rCHS %02d/%d/%02d-%02d ";
// verify track 0 command
  PROGMEM_STR m_errTrack0Error[]     PROGMEM = "Bad Track0 or wrong drive setup";
  PROGMEM_STR m_errTryFormat[]       PROGMEM = "Try low-level format first";
  
// MegaFDC command line
#ifndef BUILD_IMD_IMAGER
  
// user commands, these are printed out in stringTable[] order
  PROGMEM_STR m_cmdHelp[]            PROGMEM = "HELP"; //outside printed commands list, shown in invalid command message
  PROGMEM_STR m_cmdSupportedIndex[]  PROGMEM = "Commands list: ";
  PROGMEM_STR m_cmdReset[]           PROGMEM = "RESET";
  PROGMEM_STR m_cmdDrivParm[]        PROGMEM = "DRIVPARM";
  PROGMEM_STR m_cmdPersist[]         PROGMEM = "PERSIST";
  PROGMEM_STR m_cmdFormat[]          PROGMEM = "FORMAT";
  PROGMEM_STR m_cmdVerify[]          PROGMEM = "VERIFY";
  PROGMEM_STR m_cmdImage[]           PROGMEM = "IMAGE";  
// filesystem specific commands
  PROGMEM_STR m_cmdFSIndex[]         PROGMEM = "";
  PROGMEM_STR m_cmdQuickFormat[]     PROGMEM = "QFORMAT";
  PROGMEM_STR m_cmdPath[]            PROGMEM = "PATH";
  PROGMEM_STR m_cmdCd[]              PROGMEM = "CD";
  PROGMEM_STR m_cmdMd[]              PROGMEM = "MD";
  PROGMEM_STR m_cmdRd[]              PROGMEM = "RD";
  PROGMEM_STR m_cmdDir[]             PROGMEM = "DIR";
  PROGMEM_STR m_cmdType[]            PROGMEM = "TYPE";
  PROGMEM_STR m_cmdTypeInto[]        PROGMEM = "TYPEINTO";
  PROGMEM_STR m_cmdDel[]             PROGMEM = "DEL"; 
  PROGMEM_STR m_cmdXfer[]            PROGMEM = "XFER";
// user commands end
  PROGMEM_STR m_cmdEndIndex[]        PROGMEM = "";
  
// change directory - support all these variants
  PROGMEM_STR m_varCd1[]             PROGMEM = "CD.";
  PROGMEM_STR m_varCd2[]             PROGMEM = "CD..";
  PROGMEM_STR m_varCd3[]             PROGMEM = "CD\\";
  
// HELP details for commands
  PROGMEM_STR m_helpDetails[]        PROGMEM = "\r\n\r\nFor details, use HELP (command)\r\n\r\n";
  PROGMEM_STR m_helpReset1[]         PROGMEM = "Usage: RESET [drive:]\r\n";
  PROGMEM_STR m_helpReset2[]         PROGMEM = "Re-sets parameters for [drive:]\r\n";
  PROGMEM_STR m_helpReset3[]         PROGMEM = "or resets the whole system.\r\n\r\n";
  PROGMEM_STR m_helpDrivParm1[]      PROGMEM = "Usage: DRIVPARM [drive:]\r\n";
  PROGMEM_STR m_helpDrivParm2[]      PROGMEM = "Shows parameters of [drive:]\r\n";
  PROGMEM_STR m_helpCurrentDrive[]   PROGMEM = "or the current drive.\r\n\r\n";
  PROGMEM_STR m_helpPersist1[]       PROGMEM = "Usage: PERSIST\r\n";
  PROGMEM_STR m_helpPersist2[]       PROGMEM = "Stores drive configuration into\r\n";
  PROGMEM_STR m_helpPersist3[]       PROGMEM = "the EEPROM, or removes it.\r\n\r\n";
  PROGMEM_STR m_helpFormat1[]        PROGMEM = "Usage: FORMAT [drive:]\r\n";
  PROGMEM_STR m_helpFormat2[]        PROGMEM = "Low-level format of [drive:]\r\n";
  PROGMEM_STR m_helpVerify1[]        PROGMEM = "Usage: VERIFY [drive:]\r\n";
  PROGMEM_STR m_helpVerify2[]        PROGMEM = "Verifies all tracks of [drive:]\r\n";
  PROGMEM_STR m_helpImage1[]         PROGMEM = "Usage: IMAGE [drive:]\r\n";
  PROGMEM_STR m_helpImage2[]         PROGMEM = "Creates disk image of [drive:]\r\n";
  PROGMEM_STR m_helpImage3[]         PROGMEM = "or writes it to [drive:]\r\n";
  PROGMEM_STR m_helpQuickFormat1[]   PROGMEM = "Usage: QFORMAT [drive:]\r\n";
  PROGMEM_STR m_helpQuickFormat2[]   PROGMEM = "Creates filesystem on [drive:]\r\n";
  PROGMEM_STR m_helpPath1[]          PROGMEM = "Usage: PATH\r\n";
  PROGMEM_STR m_helpPath2[]          PROGMEM = "Toggles between showing whole\r\n";
  PROGMEM_STR m_helpPath3[]          PROGMEM = "path in the command prompt,\r\n";
  PROGMEM_STR m_helpPath4[]          PROGMEM = "or just the drive letter.\r\n\r\n";
  PROGMEM_STR m_helpCd1[]            PROGMEM = "Usage: CD / CD dir / CD ..\r\n";
  PROGMEM_STR m_helpCd2[]            PROGMEM = "CD shows working directory,\r\n";
  PROGMEM_STR m_helpCd3[]            PROGMEM = "CD dir sets working directory,\r\n";
  PROGMEM_STR m_helpCd4[]            PROGMEM = "CD .. goes one level up.\r\n\r\n";
  PROGMEM_STR m_helpDir1[]           PROGMEM = "Usage: DIR [directory]\r\n";
  PROGMEM_STR m_helpDir2[]           PROGMEM = "Shows contents of [directory]\r\n";
  PROGMEM_STR m_helpDir3[]           PROGMEM = "or of the current directory.\r\n\r\n";
  PROGMEM_STR m_helpDel1[]           PROGMEM = "Usage: DEL filename\r\n";
  PROGMEM_STR m_helpDel2[]           PROGMEM = "Deletes one file.\r\n\r\n";
  PROGMEM_STR m_helpType1[]          PROGMEM = "Usage: TYPE filename\r\n";
  PROGMEM_STR m_helpType2[]          PROGMEM = "Prints out contents of a file.\r\n\r\n";
  PROGMEM_STR m_helpMd1[]            PROGMEM = "Usage: MD directory\r\n";
  PROGMEM_STR m_helpMd2[]            PROGMEM = "Creates one empty directory.\r\n\r\n";
  PROGMEM_STR m_helpRd1[]            PROGMEM = "Usage: RD directory\r\n";
  PROGMEM_STR m_helpRd2[]            PROGMEM = "Removes one empty directory.\r\n\r\n";
  PROGMEM_STR m_helpTypeInto1[]      PROGMEM = "Usage: TYPEINTO filename\r\n";
  PROGMEM_STR m_helpTypeInto2[]      PROGMEM = "Creates a new file and uses\r\n";
  PROGMEM_STR m_helpTypeInto3[]      PROGMEM = "keyboard input for contents.\r\n\r\n";
  PROGMEM_STR m_helpXfer1[]          PROGMEM = "Usage: XFER filename\r\n";
  PROGMEM_STR m_helpXfer2[]          PROGMEM = "File transfer over serial link.\r\n\r\n";
  
// init drives command directly at startup
  PROGMEM_STR m_waitingForDrives[]   PROGMEM = "Starting MS-BOSS...\r\n\r\n"; // :-)  
  PROGMEM_STR m_countDrives[]        PROGMEM = "Disk drives connected (1-4): ";
  PROGMEM_STR m_specifyParams1[]     PROGMEM = "Specify parameters for drive %c:\r\n";
  PROGMEM_STR m_specifyParams2[]     PROGMEM = "Press Esc now if non-standard.\r\n\r\n";
  PROGMEM_STR m_driveInches[]        PROGMEM = "Drive (8)\" (5).25\" (3).5\": ";
  PROGMEM_STR m_drive8InchText1[]    PROGMEM = "CP/M 2.2 compatible SSSD 8\":\r\n";
  PROGMEM_STR m_drive8InchText2[]    PROGMEM = "(1) 77x26x128B, 1 side,  FM\r\n\r\n";
  PROGMEM_STR m_drive8InchText3[]    PROGMEM = "Experimental DOS FAT formats:\r\n";
  PROGMEM_STR m_drive8InchText4[]    PROGMEM = "(2) 77x9x512B,  1 side,  MFM\r\n";
  PROGMEM_STR m_drive8InchText5[]    PROGMEM = "(3) 77x16x512B, 1 side,  MFM\r\n";
  PROGMEM_STR m_drive8InchText6[]    PROGMEM = "(4) 77x16x512B, 2 sides, MFM\r\n\r\n";
  PROGMEM_STR m_drive5InchSides[]    PROGMEM = "(1) or (2) sides? normally 2: ";
  PROGMEM_STR m_drive5InchSSDD[]     PROGMEM = "Disk 1(6)0K 1(8)0K: ";
  PROGMEM_STR m_drive5Inch[]         PROGMEM = "Disk 3(2)0K 3(6)0K (1).2M: ";
  PROGMEM_STR m_drive5InDoubleStep[] PROGMEM = "Drive hi-density capable? Y/N: ";
  PROGMEM_STR m_drive3Inch[]         PROGMEM = "Disk (7)20K (1).44M: ";
  PROGMEM_STR m_drive3Inch_288[]     PROGMEM = "Disk (7)20K (1).44M (2).88M: ";

// init drives command with non standard geometry - asks everything. colon right parenthesis
  PROGMEM_STR m_specifyingAll[]      PROGMEM = "Specifying all parameters for %c:\r\n";
  PROGMEM_STR m_customCyls[]         PROGMEM = "Cylinders (1-100): ";
  PROGMEM_STR m_customDoubleStep[]   PROGMEM = "Drive double stepping? Y/N: ";
  PROGMEM_STR m_customChangeline[]   PROGMEM = "DiskChange signal? Y/N: ";
  PROGMEM_STR m_customHeads[]        PROGMEM = "Heads (1 or 2): ";
  PROGMEM_STR m_customSpt[]          PROGMEM = "Sectors (1-63): ";
  PROGMEM_STR m_customSectorSize[]   PROGMEM = "Each (1)28 (2)56 (5)12 1(K)B: ";
  PROGMEM_STR m_customSectorGap[]    PROGMEM = "Sector gap (hex): ";
  PROGMEM_STR m_customFormatGap3[]   PROGMEM = "Format gap (hex): ";
  PROGMEM_STR m_customFormatFiller[] PROGMEM = "Format fill byte (hex): ";
  PROGMEM_STR m_customEncoding[]     PROGMEM = "Data encoding (M)FM / (F)M: ";
  PROGMEM_STR m_customDataRateSel[]  PROGMEM = "Select media data rate:";
  PROGMEM_STR m_custom8InchEnc[]     PROGMEM = "(2)50kbps FM / (5)00kbps MFM: ";
  PROGMEM_STR m_custom5InchFM[]      PROGMEM = "(1)25 1(5)0 (2)50 kbps FM: ";
  PROGMEM_STR m_custom5InchMFM[]     PROGMEM = "(2)50 (3)00 (5)00 kbps MFM: ";
  PROGMEM_STR m_custom3InchFM[]      PROGMEM = "(1)25 (2)50 kbps FM: ";
  PROGMEM_STR m_custom3InchMFM[]     PROGMEM = "(2)50 (5)00 kbps MFM: ";
  PROGMEM_STR m_custom3InchMFM_1M[]  PROGMEM = "(2)50K (5)00K 1(M)bps MFM: ";
  PROGMEM_STR m_customEDModeInfo[]   PROGMEM = "Press N here for normal floppy\r\n";
  PROGMEM_STR m_customEDMode[]       PROGMEM = "Perpendicular recording? Y/N: ";
  PROGMEM_STR m_customUseFAT[]       PROGMEM = "Use FAT12 filesystem? Y/N: ";
  PROGMEM_STR m_customMediaByte[]    PROGMEM = "FAT media descriptor (hex): ";
  PROGMEM_STR m_customRootDir[]      PROGMEM = "Root dir entries (in 16s): ";
  PROGMEM_STR m_customClusterSize[]  PROGMEM = "(5)12 or (1)024B clusters: ";
  
// process command results
  PROGMEM_STR m_processInvalidCmd[]  PROGMEM = "Invalid command; try HELP";
  PROGMEM_STR m_processInvalidDrv[]  PROGMEM = "Invalid drive specification";
  PROGMEM_STR m_processUnknownDrv[]  PROGMEM = "This drive was not configured";
  PROGMEM_STR m_processNoFS[]        PROGMEM = "Not available on this drive";
  PROGMEM_STR m_processInvalidChr1[] PROGMEM = "Provide 1 valid file name";
  PROGMEM_STR m_processInvalidChr2[] PROGMEM = "Provide 1 valid directory name";
  PROGMEM_STR m_processMaxPath[]     PROGMEM = "Maximum path length reached";
  
// DRIVPARM
  PROGMEM_STR m_drivParmCaption[]    PROGMEM = "Parameters of drive %c:\r\n\r\n";
  PROGMEM_STR m_drivParmTypeText[]   PROGMEM = "Disk media type        ";
  PROGMEM_STR m_drivParmType8SD[]    PROGMEM = "8\" SD";
  PROGMEM_STR m_drivParmType8DD[]    PROGMEM = "8\" DD";
  PROGMEM_STR m_drivParmType5DD[]    PROGMEM = "5.25\" DD";
  PROGMEM_STR m_drivParmType5HD[]    PROGMEM = "5.25\" HD";
  PROGMEM_STR m_drivParmType3DD[]    PROGMEM = "3.5\" DD";
  PROGMEM_STR m_drivParmType3HD[]    PROGMEM = "3.5\" HD";
  PROGMEM_STR m_drivParmType3ED[]    PROGMEM = "3.5\" ED";
  PROGMEM_STR m_drivParmDoubleStep[] PROGMEM = "Drive double-stepping  ";
  PROGMEM_STR m_drivParmDiskChange[] PROGMEM = "Supports DISKCHG line  ";
  PROGMEM_STR m_drivParmSRT[]        PROGMEM = "Drive step rate time   %u ms";
  PROGMEM_STR m_drivParmHLT[]        PROGMEM = "Drive head load time   %u ms";
  PROGMEM_STR m_drivParmHUT[]        PROGMEM = "Drive head unload time %u ms";
  PROGMEM_STR m_drivParmCyls[]       PROGMEM = "Cylinders              %u";
  PROGMEM_STR m_drivParmHeads[]      PROGMEM = "Heads (sides)          ";
  PROGMEM_STR m_drivParmHeadsOne[]   PROGMEM = "one";
  PROGMEM_STR m_drivParmHeadsTwo[]   PROGMEM = "two";
  PROGMEM_STR m_drivParmSpt[]        PROGMEM = "Sectors per track      %u";
  PROGMEM_STR m_drivParmSectorSize[] PROGMEM = "Sector size (bytes)    %u";
  PROGMEM_STR m_drivParmEncoding[]   PROGMEM = "Data encoding          ";
  PROGMEM_STR m_drivParmFM[]         PROGMEM = "FM";
  PROGMEM_STR m_drivParmMFM[]        PROGMEM = "MFM";
  PROGMEM_STR m_drivParmDataRate[]   PROGMEM = "Media data rate        %u";
  PROGMEM_STR m_drivParmFDCRate[]    PROGMEM = "FDC transfer rate      %u";
  PROGMEM_STR m_drivParmKbps[]       PROGMEM = "kbps";
  PROGMEM_STR m_drivParmMbps[]       PROGMEM = "Mbps";
  PROGMEM_STR m_drivParmSectorGap[]  PROGMEM = "Sector gap length      0x%02X";
  PROGMEM_STR m_drivParmFormatGap[]  PROGMEM = "Format gap length      0x%02X";
  PROGMEM_STR m_drivParmFormatFill[] PROGMEM = "Format fill byte       0x%02X";
  PROGMEM_STR m_drivParmEDMode[]     PROGMEM = "Perpendicular/ED mode  ";
  PROGMEM_STR m_drivParmFS[]         PROGMEM = "Filesystem support     ";
  PROGMEM_STR m_drivParmFAT[]        PROGMEM = "FAT12";
  PROGMEM_STR m_drivParmCPM[]        PROGMEM = "CP/M";
  PROGMEM_STR m_drivParmFATMedia[]   PROGMEM = "FAT media descriptor   0x%02X";
  PROGMEM_STR m_drivParmFATRootDir[] PROGMEM = "FAT root dir entries   %u";
  PROGMEM_STR m_drivParmFATCluster[] PROGMEM = "FAT cluster size       %u B";
  
// PERSIST
  PROGMEM_STR m_persistStore[]       PROGMEM = "(P)ersist settings to EEPROM\r\n";
  PROGMEM_STR m_persistRemove[]      PROGMEM = "(R)emove settings from EEPROM\r\n";
    
// FORMAT, VERIFY, QFORMAT, IMAGE
  PROGMEM_STR m_diskIoFormatVerify[] PROGMEM = "Verify during format? Y/N: ";
  PROGMEM_STR m_diskIoCreateFS[]     PROGMEM = "Also create filesystem? Y/N: ";
  PROGMEM_STR m_diskIoInsertDisk[]   PROGMEM = "Insert disk into drive %c:";
  PROGMEM_STR m_diskIoTotalBadSect[] PROGMEM = "Total bad sectors: %u";
  PROGMEM_STR m_diskIoTotalBadTrk[]  PROGMEM = "Tracks with bad sectors: %u";
  PROGMEM_STR m_diskIoProgress[]     PROGMEM = "\rCylinder: %02d Head: %d ";
  PROGMEM_STR m_diskIoQuickFormat[]  PROGMEM = "Quick-format drive %c:? Y/N: ";
  PROGMEM_STR m_diskIoCreatingFAT[]  PROGMEM = "Creating FAT12 filesystem...";
  PROGMEM_STR m_diskIoCreatingCPM[]  PROGMEM = "Creating CP/M filesystem...";
  PROGMEM_STR m_diskIoFormatOK[]     PROGMEM = "Format completed";
  
// IMAGE, XFER, XMODEM stuff
  PROGMEM_STR m_xferReadFile[]       PROGMEM = "(R)ead file and send to XMODEM\r\n";
  PROGMEM_STR m_xferSaveFile[]       PROGMEM = "Receive XMODEM and (W)rite file\r\n";
  PROGMEM_STR m_imageReadDisk[]      PROGMEM = "(R)ead %c: into image file\r\n";
  PROGMEM_STR m_imageWriteDisk[]     PROGMEM = "(W)rite to %c: from image file\r\n";
  PROGMEM_STR m_imageTransferLen[]   PROGMEM = "XMODEM transfer length:\r\n%lu bytes\r\n";
  PROGMEM_STR m_imageGeometry[]      PROGMEM = "(CHS %02ux%ux%02u, %u B sectors)\r\n\r\n";
  PROGMEM_STR m_xmodemUse1k[]        PROGMEM = "Use XMODEM-1K? Y/N: ";
  PROGMEM_STR m_xmodemPrefix[]       PROGMEM = "XMODEM: ";
  PROGMEM_STR m_xmodem1kPrefix[]     PROGMEM = "XMODEM-1K: ";
  PROGMEM_STR m_xmodemWaitSend[]     PROGMEM = "OK to launch Send\r\nTimeout 4 minutes\r\n";
  PROGMEM_STR m_xmodemWaitRecv[]     PROGMEM = "OK to launch Receive\r\nTimeout 4 minutes\r\n";
  PROGMEM_STR m_xmodemTransferEnd[]  PROGMEM = "\rEnd of transfer";
  PROGMEM_STR m_xmodemTransferFail[] PROGMEM = "\rTransfer aborted";
  
// DIR
  PROGMEM_STR m_dirDirectory[]       PROGMEM = " [DIRECTORY]  ";
  PROGMEM_STR m_dirDirectoryEmpty[]  PROGMEM = "No files";
  PROGMEM_STR m_dirBytesFormat[]     PROGMEM = "%7lu ";
  PROGMEM_STR m_dirBytesFree[]       PROGMEM = "bytes free.\r\n";
  PROGMEM_STR m_dirCPMUser[]         PROGMEM = "User: %02u ";
  PROGMEM_STR m_dirCPMBytes[]        PROGMEM = "%3u B ";
  PROGMEM_STR m_dirCPMKilobytes[]    PROGMEM = "%3u K ";
  PROGMEM_STR m_dirCPMEmpty[]        PROGMEM = "No files on disk";
  PROGMEM_STR m_dirCPMSummary[]      PROGMEM = "\r\n%3u K in %u file(s)\r\n%3u K free\r\n";
  
// TYPEINTO
  PROGMEM_STR m_typeIntoCaption[]    PROGMEM = "Type two empty newlines to quit\r\n";
  
// Filesystem specific
  PROGMEM_STR m_fsCurrentDrive[]     PROGMEM = "0:";
  PROGMEM_STR m_fsForbiddenCharFAT[] PROGMEM = "*?\\/\":<>|";
  PROGMEM_STR m_fsForbiddenCharCPM[] PROGMEM = "=:;<>";
  PROGMEM_STR m_fsDiskError[]        PROGMEM = "\rAborted due to disk error";
  PROGMEM_STR m_fsInternalError[]    PROGMEM = "Internal filesystem error";
  PROGMEM_STR m_fsFileNotFound[]     PROGMEM = "Not found in current path";
  PROGMEM_STR m_fsPathNotFound[]     PROGMEM = "Path not found";
  PROGMEM_STR m_fsDirectoryFull[]    PROGMEM = "Dir not empty or file readonly";
  PROGMEM_STR m_fsFileExists[]       PROGMEM = "Name already exists";
  PROGMEM_STR m_fsInvalidObject[]    PROGMEM = "Invalid object";
  PROGMEM_STR m_fsNoVolumeWorkArea[] PROGMEM = "Cannot mount filesystem";
  PROGMEM_STR m_fsMkfsError[]        PROGMEM = "Filesystem creation failed";
  PROGMEM_STR m_fsNoFAT[]            PROGMEM = "Not a FAT12 filesystem";
  PROGMEM_STR m_fsMemoryError[]      PROGMEM = "Not enough memory";
  PROGMEM_STR m_fsInvalidParameter[] PROGMEM = "Invalid parameter";
  PROGMEM_STR m_fsCPMFileNotFound[]  PROGMEM = "File for user %02u not found";
  PROGMEM_STR m_fsCPMFileEmpty[]     PROGMEM = "The file is empty";
  PROGMEM_STR m_fsCPMNoFilesFound[]  PROGMEM = "File not found for all users";
  
  // IMD imager
#else
  PROGMEM_STR m_imdSplash[]          PROGMEM = "\rImageDisk build, ";
  PROGMEM_STR m_imdLCDRecommended[]  PROGMEM = "LCD output recommended!\r\n";
  PROGMEM_STR m_imdDriveLetter[]     PROGMEM = "Drive (A): (B): (C): (D): ";
  PROGMEM_STR m_imdDriveType[]       PROGMEM = "Type (8)\" (5).25\" (3).5\": ";
  PROGMEM_STR m_imdDriveCyls[]       PROGMEM = "Maximum cylinders (1-100): ";  
  PROGMEM_STR m_imdDoubleStepAuto[]  PROGMEM = "Double stepping? Y/N/Guess: ";
  PROGMEM_STR m_imdDoubleStep[]      PROGMEM = "Drive double stepping? Y/N: ";
  PROGMEM_STR m_imdDiskSidesAuto[]   PROGMEM = "Force single-sided? Y/N: ";
  PROGMEM_STR m_imdDiskSides[]       PROGMEM = "How many sides? (1) or (2): ";
  PROGMEM_STR m_imdInterleave[]      PROGMEM = "Format interleave (1: none): ";
  PROGMEM_STR m_imdSectorGap[]       PROGMEM = "Sector gap (hex, 0: auto): ";
  PROGMEM_STR m_imdFormatGap[]       PROGMEM = "Format gap (hex, 0: auto): ";
  PROGMEM_STR m_imdAdvancedDetails[] PROGMEM = "More info at http://boginjr.com\r\n";
  PROGMEM_STR m_imdDefaultsNo[]      PROGMEM = "Defaults: No\r\n\r\n";
  PROGMEM_STR m_imdXlate300kbps[]    PROGMEM = "300 <> 250kbps translate Y/N: ";
  PROGMEM_STR m_imdSpecifyOptions[]  PROGMEM = "Can't autodetect these options.\r\n";
  PROGMEM_STR m_imdSpecifyOptions2[] PROGMEM = "Specify them to continue. Or,\r\n";
  PROGMEM_STR m_imdEscGoBack[]       PROGMEM = "\rPress Esc to go back.\r\n\r\n";
  
  PROGMEM_STR m_imdInsertGoodDisk[]  PROGMEM = "Insert known good disk to read\r\n";
  PROGMEM_STR m_imdInsertWriteDisk[] PROGMEM = "Insert floppy to test writes\r\n";
  PROGMEM_STR m_imdInsertFormat[]    PROGMEM = "Insert floppy disk to format\r\n";
  PROGMEM_STR m_imdInsertErase[]     PROGMEM = "Insert floppy disk to erase\r\n";
  PROGMEM_STR m_imdInsertRead[]      PROGMEM = "Insert floppy disk to read\r\n";
  PROGMEM_STR m_imdInsertWrite[]     PROGMEM = "Insert floppy disk to write\r\n";
  PROGMEM_STR m_imdNoteTestWrite[]   PROGMEM = "NOTE: data will be destroyed\r\n";
  PROGMEM_STR m_imdNoteNoFS[]        PROGMEM = "NOTE: won't create filesystem\r\n";
  PROGMEM_STR m_imdMemoryError[]     PROGMEM = "Memory allocation error\r\n";
  
  PROGMEM_STR m_imdOptionRead[]      PROGMEM = "(R)ead disk into IMD image\r\n";
  PROGMEM_STR m_imdOptionWrite[]     PROGMEM = "(W)rite disk from IMD image\r\n";
  PROGMEM_STR m_imdOptionFormat[]    PROGMEM = "(F)ormat disk\r\n";
  PROGMEM_STR m_imdOptionErase[]     PROGMEM = "(E)rase disk\r\n";
  PROGMEM_STR m_imdOptionTests[]     PROGMEM = "(T)est FDC, drive seek and RPM\r\n";
  PROGMEM_STR m_imdOptionXlat[]      PROGMEM = "(D)ata rate translations\r\n";
  PROGMEM_STR m_imdOptionReset[]     PROGMEM = "(Q)uit and re-enter settings\r\n\r\n";
  
  PROGMEM_STR m_imdTestFDC[]         PROGMEM = "(F)loppy controller write test\r\n";
  PROGMEM_STR m_imdTestDrive[]       PROGMEM = "(D)rive seek and RPM test\r\n";  
  PROGMEM_STR m_imdTestSkipped[]     PROGMEM = "SKIP";
  PROGMEM_STR m_imdTestController[]  PROGMEM = "Media data rate [geometry]:\r\n\r\n";
  PROGMEM_STR m_imdTestHD[]          PROGMEM = "500kbps MFM [1.2MB HD]:   ";
  PROGMEM_STR m_imdTestED[]          PROGMEM = "  1Mbps MFM [2.88MB ED]:  ";
  PROGMEM_STR m_imdTest8inch[]       PROGMEM = "250kbps  FM [8\" 250K SD]: ";  
  PROGMEM_STR m_imdTestMFM128[]      PROGMEM = "500kbps MFM [128B/sect.]: "; 
  PROGMEM_STR m_imdTestMedia[]       PROGMEM = "Analyzing disk...";
  PROGMEM_STR m_imdTestDriveSeek[]   PROGMEM = "Drive seek test...";
  PROGMEM_STR m_imdTestRPM[]         PROGMEM = "Synchronizing...";
  PROGMEM_STR m_imdRPM[]             PROGMEM = "RPM: %u   \r";
  PROGMEM_STR m_imdDiskReadFail[]    PROGMEM = "\r\nNo sector IDs on disk";
  PROGMEM_STR m_imdSyncSectorFail[]  PROGMEM = "\r\nCannot sync with disk";

  PROGMEM_STR m_imdGeoCylsHdStep[]   PROGMEM = " max %u cyl %u side";
  PROGMEM_STR m_imdSingleStepping[]  PROGMEM = "Singlestep";
  PROGMEM_STR m_imdDoubleStepping[]  PROGMEM = "Doublestep";
  PROGMEM_STR m_imdInterleaveGaps[]  PROGMEM = "Interleave %u:1, gaps ";
  PROGMEM_STR m_imdGeoGapSizesAuto[] PROGMEM = "automatic\r\n";
  PROGMEM_STR m_imdGeoGapSizesUser[] PROGMEM = "custom\r\n";
  PROGMEM_STR m_imdGeoChsSpt[]       PROGMEM = "CHS %02u/%u/%02ux%-4uB sect GAP 0x%02X\r\n";
  PROGMEM_STR m_imdGeoRateGap3[]     PROGMEM = "Datarate %ukbps %3s, GAP3 0x%02X\r\n";
  PROGMEM_STR m_imdProgress[]        PROGMEM = "\rCylinder: %02u Head: %u ";

  PROGMEM_STR m_imdUseSameParams[]   PROGMEM = "Same settings as before? Y/N: ";
  PROGMEM_STR m_imdFormatParams[]    PROGMEM = "Format parameters (Esc quits):\r\n\r\n";
  PROGMEM_STR m_imdFormatSpt[]       PROGMEM = "Sectors per track (1-63): ";
  PROGMEM_STR m_imdFormatStartSec1[] PROGMEM = "Sectors normally start at 1.\r\n";                                               
  PROGMEM_STR m_imdFormatStartSec2[] PROGMEM = "Starting sector (0-%u): ";
  PROGMEM_STR m_imdFormatEncoding[]  PROGMEM = "Data encoding (M)FM / (F)M: ";  
  PROGMEM_STR m_imdFormatRate[]      PROGMEM = "Select media data rate:\r\n";
  PROGMEM_STR m_imdFormatRatesMFM[]  PROGMEM = "(2)50 (3)00 (5)00 kbps MFM: ";
  PROGMEM_STR m_imdFormatRatesFM[]   PROGMEM = "(1)25 1(5)0 (2)50 kbps FM: ";
  PROGMEM_STR m_imdFormatSecSize1[]  PROGMEM = "Select sector size:\r\n";
  PROGMEM_STR m_imdFormatSecSize2[]  PROGMEM = "(1)28B (2)56B (5)12B 1(K)B,\r\n";
  PROGMEM_STR m_imdFormatSecSize3[]  PROGMEM = "(L) 2KB, (M) 4KB, (N) 8KB: ";
  PROGMEM_STR m_imdBadSectorsDisk[]  PROGMEM = "%u bad sector(s) on disk\r\n";
  PROGMEM_STR m_imdBadSectorsFile[]  PROGMEM = "%u bad sector(s) in IMD file\r\n";
  
  PROGMEM_STR m_imdXmodem[]          PROGMEM = "XMODEM: ";
  PROGMEM_STR m_imdXmodem1k[]        PROGMEM = "XMODEM-1K: ";
  PROGMEM_STR m_imdXmodemUse1k[]     PROGMEM = "Use XMODEM-1K? Y/N: ";
  PROGMEM_STR m_imdXmodemSkipBad[]   PROGMEM = "Skip sectors marked bad? Y/N: ";
  PROGMEM_STR m_imdXmodemVerify[]    PROGMEM = "Rigorous verify? (SLOW!) Y/N: ";
  PROGMEM_STR m_imdXmodemWaitSend[]  PROGMEM = "OK to launch Send\r\nTimeout 4 minutes\r\n";
  PROGMEM_STR m_imdXmodemWaitRecv[]  PROGMEM = "OK to launch Receive\r\nTimeout 4 minutes\r\n";
  PROGMEM_STR m_imdXmodemXferEnd[]   PROGMEM = "\rEnd of transfer\r\n";
  PROGMEM_STR m_imdXmodemXferFail[]  PROGMEM = "\rTransfer aborted\r\n";
  PROGMEM_STR m_imdXmodemErrPacket[] PROGMEM = "Invalid XMODEM data packet\r\n";
  PROGMEM_STR m_imdXmodemErrHeader[] PROGMEM = "Invalid header in file\r\n";
  PROGMEM_STR m_imdXmodemErrMode[]   PROGMEM = "IMD Mode byte must be 0-5\r\n";
  PROGMEM_STR m_imdXmodemErrCyls[]   PROGMEM = "More cyls than configured\r\n";
  PROGMEM_STR m_imdXmodemErrHead[]   PROGMEM = "Heads (sides) must be 0-1\r\n";
  PROGMEM_STR m_imdXmodemErrSpt[]    PROGMEM = "Sectors per track must be <64\r\n";
  PROGMEM_STR m_imdXmodemErrSsize[]  PROGMEM = "Sector size byte must be 0-6\r\n";
  PROGMEM_STR m_imdXmodemLowRAM[]    PROGMEM = "Not enough RAM for %uK sectors\r\n";  
  PROGMEM_STR m_imdXmodemErrData[]   PROGMEM = "Invalid data record type (0-8)\r\n";
  
  PROGMEM_STR m_imdWriteHeader[]     PROGMEM = "IMD file created by MegaFDC, (c) J. Bogin\r\n";
  PROGMEM_STR m_imdWriteComment[]    PROGMEM = "Comment (max %u chars per line)\r\n";
  PROGMEM_STR m_imdWriteDone[]       PROGMEM = "Type 2 empty newlines when done\r\n";
  PROGMEM_STR m_imdWriteEnterEsc[]   PROGMEM = "ENTER: continue, Esc: skip...";
  PROGMEM_STR m_imdTrackUnreadable[] PROGMEM = "Unreadable\r\n";
  PROGMEM_STR m_imdUnreadableTrks[]  PROGMEM = "%u unreadable track(s)\r\n\r\n";
  PROGMEM_STR m_imdRunPython[]       PROGMEM = "Run 'imdtrim.py' before using!\r\n";
  
#endif
  
// tables
private:

  PROGMEM_STR* const m_stringTable[] PROGMEM = {  
                                                  m_uiDeleteLine, m_uiNewLine, m_uiNewLine2x, m_uiEchoKey, m_uiEnterKey,
                                                  m_uiVT100ClearScreen, m_uiOK, m_uiFAIL, m_uiEnabled, m_uiDisabled, m_uiYes,
                                                  m_uiNo, m_uiNotAvailable, m_uiAbort, m_uiContinue, m_uiContinueAbort,
                                                  m_uiDecimalInput, m_uiHexadecimalInput, m_uiBytes, m_uiOperationPending,
                                                  m_uiCancelOption, m_uiChooseOption,

                                                  m_uiSplash, m_uiBuild, m_uiNoKeyboard, m_uiInitializingFDC, m_uiDisabled1,
                                                  m_uiDisabled2, m_uiFatalError, m_uiSystemHalted, m_uiCtrlAltDel, m_uiLoadingConfig,
                                                  m_uiLoadingAborted,
                                                  
                                                  m_errRQMTimeout, m_errRecalibrate, m_errSeek, m_errWriProtect,
                                                  m_errINTTimeout, m_errOverrun, m_errCRC,  m_errNoData,
                                                  m_errNoAddrMark, m_errBadTrack, m_chsFmtSTRegsSingle,
                                                  m_chsFmtSTRegsMulti, m_chsFmtSingleSector, m_chsFmtMultiSector,
                                                  m_errTrack0Error, m_errTryFormat,

#ifndef BUILD_IMD_IMAGER
                                                  m_cmdHelp,                                                              
                                                  m_cmdSupportedIndex,
                                                  m_cmdReset, m_cmdDrivParm, m_cmdPersist, m_cmdFormat, m_cmdVerify, m_cmdImage,
                                                  m_cmdFSIndex,
                                                  m_cmdQuickFormat, m_cmdPath, m_cmdCd, m_cmdMd, m_cmdRd, m_cmdDir,
                                                  m_cmdType, m_cmdTypeInto, m_cmdDel, m_cmdXfer,
                                                  m_cmdEndIndex,
                                                  
                                                  m_varCd1, m_varCd2, m_varCd3,
                                                  
                                                  m_helpDetails, m_helpReset1, m_helpReset2, m_helpReset3,
                                                  m_helpDrivParm1, m_helpDrivParm2, m_helpCurrentDrive, 
                                                  m_helpPersist1, m_helpPersist2, m_helpPersist3, m_helpFormat1,
                                                  m_helpFormat2, m_helpVerify1, m_helpVerify2, m_helpImage1, 
                                                  m_helpImage2, m_helpImage3, m_helpQuickFormat1, m_helpQuickFormat2,
                                                  m_helpPath1, m_helpPath2, m_helpPath3,
                                                  m_helpPath4, m_helpCd1, m_helpCd2, m_helpCd3, m_helpCd4,
                                                  m_helpDir1, m_helpDir2, m_helpDir3, m_helpDel1, m_helpDel2,
                                                  m_helpType1, m_helpType2, m_helpMd1, m_helpMd2, 
                                                  m_helpRd1, m_helpRd2, m_helpTypeInto1, m_helpTypeInto2, 
                                                  m_helpTypeInto3, m_helpXfer1, m_helpXfer2,
                                                  
                                                  m_waitingForDrives, m_countDrives, m_specifyParams1, m_specifyParams2, 
                                                  m_driveInches, m_drive8InchText1, m_drive8InchText2, m_drive8InchText3, 
                                                  m_drive8InchText4, m_drive8InchText5, m_drive8InchText6,
                                                  m_drive5InchSides, m_drive5InchSSDD, m_drive5Inch, 
                                                  m_drive5InDoubleStep, m_drive3Inch, m_drive3Inch_288,
                                                  
                                                  m_specifyingAll, m_customCyls, m_customDoubleStep, m_customChangeline,
                                                  m_customHeads, m_customSpt, m_customSectorSize, m_customSectorGap,
                                                  m_customFormatGap3, m_customFormatFiller, m_customEncoding,
                                                  m_customDataRateSel, m_custom8InchEnc, m_custom5InchFM, m_custom5InchMFM,
                                                  m_custom3InchFM, m_custom3InchMFM, m_custom3InchMFM_1M, m_customEDModeInfo,
                                                  m_customEDMode, m_customUseFAT, m_customMediaByte, m_customRootDir, m_customClusterSize,
                                                  
                                                  m_processInvalidCmd, m_processInvalidDrv, m_processUnknownDrv,
                                                  m_processNoFS, m_processInvalidChr1, m_processInvalidChr2, m_processMaxPath,
                                                  
                                                  m_drivParmCaption, m_drivParmTypeText, m_drivParmType8SD, m_drivParmType8DD,
                                                  m_drivParmType5DD, m_drivParmType5HD, m_drivParmType3DD, m_drivParmType3HD, m_drivParmType3ED,
                                                  m_drivParmDoubleStep, m_drivParmDiskChange, m_drivParmSRT, m_drivParmHLT,
                                                  m_drivParmHUT, m_drivParmCyls, m_drivParmHeads, m_drivParmHeadsOne,
                                                  m_drivParmHeadsTwo, m_drivParmSpt,  m_drivParmSectorSize, m_drivParmEncoding,
                                                  m_drivParmFM, m_drivParmMFM, m_drivParmDataRate, m_drivParmFDCRate,
                                                  m_drivParmKbps, m_drivParmMbps, m_drivParmSectorGap, m_drivParmFormatGap,
                                                  m_drivParmFormatFill, m_drivParmEDMode, m_drivParmFS, m_drivParmFAT, m_drivParmCPM,
                                                  m_drivParmFATMedia, m_drivParmFATRootDir, m_drivParmFATCluster,
                                                  
                                                  m_persistStore, m_persistRemove,
                                                  
                                                  m_diskIoFormatVerify, m_diskIoCreateFS, m_diskIoInsertDisk, 
                                                  m_diskIoTotalBadSect, m_diskIoTotalBadTrk, m_diskIoProgress,
                                                  m_diskIoQuickFormat, m_diskIoCreatingFAT, m_diskIoCreatingCPM, m_diskIoFormatOK,
                                                  
                                                  m_xferReadFile, m_xferSaveFile, m_imageReadDisk, m_imageWriteDisk,
                                                  m_imageTransferLen, m_imageGeometry, m_xmodemUse1k, m_xmodemPrefix,
                                                  m_xmodem1kPrefix, m_xmodemWaitSend, m_xmodemWaitRecv, m_xmodemTransferEnd,
                                                  m_xmodemTransferFail,
                                                  
                                                  m_dirDirectory, m_dirDirectoryEmpty, m_dirBytesFormat, m_dirBytesFree,
                                                  m_dirCPMUser, m_dirCPMBytes, m_dirCPMKilobytes, m_dirCPMEmpty, m_dirCPMSummary,
                                                  
                                                  m_typeIntoCaption,
                                                  
                                                  m_fsCurrentDrive, m_fsForbiddenCharFAT, m_fsForbiddenCharCPM, m_fsDiskError,
                                                  m_fsInternalError, m_fsFileNotFound, m_fsPathNotFound, m_fsDirectoryFull,
                                                  m_fsFileExists, m_fsInvalidObject, m_fsNoVolumeWorkArea, m_fsMkfsError, m_fsNoFAT,
                                                  m_fsMemoryError, m_fsInvalidParameter, m_fsCPMFileNotFound, m_fsCPMFileEmpty,
                                                  m_fsCPMNoFilesFound                                                  
#else
                                                  m_imdSplash, m_imdLCDRecommended, m_imdDriveLetter, m_imdDriveType, m_imdDriveCyls,
                                                  m_imdDoubleStepAuto, m_imdDoubleStep, m_imdDiskSidesAuto, m_imdDiskSides, m_imdInterleave, 
                                                  m_imdSectorGap, m_imdFormatGap, m_imdAdvancedDetails, m_imdDefaultsNo,
                                                  m_imdXlate300kbps, m_imdSpecifyOptions, m_imdSpecifyOptions2, m_imdEscGoBack,
                                                  
                                                  m_imdInsertGoodDisk, m_imdInsertWriteDisk, m_imdInsertFormat, m_imdInsertErase, 
                                                  m_imdInsertRead, m_imdInsertWrite, m_imdNoteTestWrite, m_imdNoteNoFS, m_imdMemoryError,
  
                                                  m_imdOptionRead, m_imdOptionWrite, m_imdOptionFormat, m_imdOptionErase, 
                                                  m_imdOptionTests, m_imdOptionXlat, m_imdOptionReset,
                                                  
                                                  m_imdTestFDC, m_imdTestDrive, m_imdTestSkipped, m_imdTestController, m_imdTestHD, 
                                                  m_imdTestED, m_imdTest8inch, m_imdTestMFM128, m_imdTestMedia, 
                                                  m_imdTestDriveSeek, m_imdTestRPM, m_imdRPM, m_imdDiskReadFail, m_imdSyncSectorFail,
                                                  
                                                  m_imdGeoCylsHdStep, m_imdSingleStepping, m_imdDoubleStepping, m_imdInterleaveGaps,
                                                  m_imdGeoGapSizesAuto, m_imdGeoGapSizesUser, m_imdGeoChsSpt, m_imdGeoRateGap3,
                                                  m_imdProgress,
                                                  
                                                  m_imdUseSameParams, m_imdFormatParams, m_imdFormatSpt, m_imdFormatStartSec1,
                                                  m_imdFormatStartSec2, m_imdFormatEncoding, m_imdFormatRate, m_imdFormatRatesMFM,
                                                  m_imdFormatRatesFM, m_imdFormatSecSize1, m_imdFormatSecSize2, m_imdFormatSecSize3,
                                                  m_imdBadSectorsDisk, m_imdBadSectorsFile,
                                                  
                                                  m_imdXmodem, m_imdXmodem1k, m_imdXmodemUse1k, m_imdXmodemSkipBad, m_imdXmodemVerify, 
                                                  m_imdXmodemWaitSend, m_imdXmodemWaitRecv, m_imdXmodemXferEnd, m_imdXmodemXferFail,                                                  
                                                  m_imdXmodemErrPacket, m_imdXmodemErrHeader, m_imdXmodemErrMode, m_imdXmodemErrCyls, 
                                                  m_imdXmodemErrHead, m_imdXmodemErrSpt, m_imdXmodemErrSsize, m_imdXmodemLowRAM,
                                                  m_imdXmodemErrData,
                                                  
                                                  m_imdWriteHeader, m_imdWriteComment, m_imdWriteDone, m_imdWriteEnterEsc,
                                                  m_imdTrackUnreadable, m_imdUnreadableTrks, m_imdRunPython
#endif
                                               };

                                                  
  // ENTER treated as CR (\r), Escape as \e, scancodes with SHIFT determined separately
  // DEL key returns 0xFF and works as a backspace + checks if Ctrl+Alt pressed beforehand, to trigger a reset
  // for key-unsigned char index reference see PS2_KEY_xxx in PS2KeyAdvanced.h
  PROGMEM_STR m_keybXlatTable[]      PROGMEM = {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, '\e',
                                                  '\b', 0, '\r', ' ', '0', '1', '2', '3', '4',
                                                  '5', '6', '7', '8', '9', '.', '\r', '+', '-',
                                                  '*', '/', '0', '1', '2', '3', '4', '5', '6',
                                                  '7', '8', '9', '\'', ',', '-', '.', '/', 0,
                                                  '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
                                                  'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
                                                  'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
                                                  ';', '\\', '[', ']', '=', 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0 
                                               };
                                           
#ifndef BUILD_IMD_IMAGER
  // 8" SSSD 26-sector skew (interleave) table
  PROGMEM_STR m_cpmSkewTable[]       PROGMEM = {  0, // 1-based
                                                  1, 7, 13, 19, 25, 5, 11, 17, 23, 3, 9,  15, 21,
                                                  2, 8, 14, 20, 26, 6, 12, 18, 24, 4, 10, 16, 22
                                               };
#else
  // known sector and format gaps to IMD
  // each group of four values: sector size N, number of sectors, sector gap, format gap
  // table must end with 0xFF
	PROGMEM_STR m_imdGapTable[]        PROGMEM = {	0x00, 0x1A, 0x07, 0x1B,	// 8" FM
                                                  0x01, 0x0F, 0x0E, 0x2A,
                                                  0x02, 0x08, 0x1B, 0x3A,
                                                  0x03, 0x04, 0x47, 0x8A,
                                                  0x04, 0x02, 0xC8, 0xFF,
                                                  0x05, 0x01, 0xC8, 0xFF,
                                                  0x01, 0x1A, 0x0E, 0x36,	// 8" MFM
                                                  0x02, 0x0F, 0x1B, 0x54,
                                                  0x03, 0x08, 0x35, 0x74,
                                                  0x04, 0x04, 0x99, 0xFF,
                                                  0x05, 0x02, 0xC8, 0xFF,
                                                  0x06, 0x01, 0xC8, 0xFF,
                                                  0x00, 0x12, 0x07, 0x09,	// 5" FM
                                                  0x00, 0x10, 0x10, 0x19,
                                                  0x01, 0x08, 0x18, 0x30,
                                                  0x02, 0x04, 0x46, 0x87,
                                                  0x03, 0x02, 0xC8, 0xFF,
                                                  0x04, 0x01, 0xC8, 0xFF,
                                                  0x01, 0x12, 0x0A, 0x0C,	// 5" MFM
                                                  0x01, 0x10, 0x20, 0x32,
                                                  0x02, 0x08, 0x2A, 0x50,
                                                  0x02, 0x09, 0x18, 0x40,
                                                  0x02, 0x0A, 0x07, 0x0E,
                                                  0x03, 0x04, 0x8D, 0xF0,
                                                  0x04, 0x02, 0xC8, 0xFF,
                                                  0x05, 0x01, 0xC8, 0xFF,
                                                  0x02, 0x12, 0x1B, 0x54,	// 1.4MB 3.5
                                                  0xFF
                                               };
#endif

// provides for transfering messages between program space and our address space
// even though we're in class Progmem, this is in RAM, not in PROGMEM itself
private:

  inline static unsigned char m_strBuffer[MAX_PROGMEM_STRING_LEN + 1];
  
};