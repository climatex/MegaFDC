// MegaFDC (c) 2023 J. Bogin, http://boginjr.com
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
    uiWaitingForDrives,
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
    initializingFDC,
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
    
    specifyingAll,
    customTracks,
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
    drivParmDoubleStep,
    drivParmDiskChange,
    drivParmSRT,
    drivParmHLT,
    drivParmHUT,
    drivParmTracks,
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
    diskIoTrack0Error,
    diskIoTryFormat,
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
  
  // translate physical sector to CP/M sector, 1-based
  static const unsigned char cpmSkewSector(unsigned char sector)
  {
    return pgm_read_byte(&(m_cpmSkewTable[sector]));
  }

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
  PROGMEM_STR m_uiSplash[]           PROGMEM = "MegaFDC (c) 2023 J. Bogin\r\n\r\n";
  PROGMEM_STR m_uiWaitingForDrives[] PROGMEM = "Starting MS-BOSS...\r\n\r\n"; // :-)
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
  PROGMEM_STR m_initializingFDC[]    PROGMEM = "Initializing controller...";
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
  PROGMEM_STR m_drive3Inch[]         PROGMEM = "Disk (7)20K (1).44M (2).88M: ";

// init drives command with non standard geometry - asks everything. colon right parenthesis
  PROGMEM_STR m_specifyingAll[]      PROGMEM = "Specifying all parameters for %c:\r\n";
  PROGMEM_STR m_customTracks[]       PROGMEM = "Tracks (1-100): ";
  PROGMEM_STR m_customDoubleStep[]   PROGMEM = "Drive double stepping? Y/N: ";
  PROGMEM_STR m_customChangeline[]   PROGMEM = "DiskChange signal? Y/N: ";
  PROGMEM_STR m_customHeads[]        PROGMEM = "Heads (1 or 2): ";
  PROGMEM_STR m_customSpt[]          PROGMEM = "Sectors (1-63): ";
  PROGMEM_STR m_customSectorSize[]   PROGMEM = "Each (1)28 (2)56 (5)12 1(K)B: ";
  PROGMEM_STR m_customSectorGap[]    PROGMEM = "Sector gap (hex): ";
  PROGMEM_STR m_customFormatGap3[]   PROGMEM = "Format gap (hex): ";
  PROGMEM_STR m_customFormatFiller[] PROGMEM = "Format fill byte (hex): ";
  PROGMEM_STR m_customEncoding[]     PROGMEM = "Data encoding (M)FM / (F)M: ";
  PROGMEM_STR m_customDataRateSel[]  PROGMEM = "Select data rate:";
  PROGMEM_STR m_custom8InchEnc[]     PROGMEM = "(2)50kbps FM / (5)00kbps MFM: ";
  PROGMEM_STR m_custom5InchFM[]      PROGMEM = "(1)25 1(5)0 (2)50 kbps FM: ";
  PROGMEM_STR m_custom5InchMFM[]     PROGMEM = "(2)50 (3)00 (5)00 kbps MFM: ";
  PROGMEM_STR m_custom3InchFM[]      PROGMEM = "(1)25 (2)50 (5)00 kbps FM: ";
  PROGMEM_STR m_custom3InchMFM[]     PROGMEM = "(2)50K (5)00K 1(M)bps MFM: ";
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
  PROGMEM_STR m_drivParmDoubleStep[] PROGMEM = "Drive double-stepping  ";
  PROGMEM_STR m_drivParmDiskChange[] PROGMEM = "Supports DISKCHG line  ";
  PROGMEM_STR m_drivParmSRT[]        PROGMEM = "Drive step rate time   %u ms";
  PROGMEM_STR m_drivParmHLT[]        PROGMEM = "Drive head load time   %u ms";
  PROGMEM_STR m_drivParmHUT[]        PROGMEM = "Drive head unload time %u ms";
  PROGMEM_STR m_drivParmTracks[]     PROGMEM = "Tracks (cylinders)     %u";
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
  PROGMEM_STR m_diskIoProgress[]     PROGMEM = "\rTrack: %02d Head: %d ";
  PROGMEM_STR m_diskIoTrack0Error[]  PROGMEM = "Bad Track0 or wrong drive setup";
  PROGMEM_STR m_diskIoTryFormat[]    PROGMEM = "Try low-level format first";
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
  
// tables
private:

  PROGMEM_STR* const m_stringTable[] PROGMEM = {  
                                                  m_uiDeleteLine, m_uiNewLine, m_uiNewLine2x, m_uiEchoKey, m_uiEnterKey,
                                                  m_uiVT100ClearScreen, m_uiOK, m_uiEnabled, m_uiDisabled, m_uiYes,
                                                  m_uiNo, m_uiNotAvailable, m_uiAbort, m_uiContinue, m_uiContinueAbort,
                                                  m_uiDecimalInput, m_uiHexadecimalInput, m_uiBytes, m_uiOperationPending,
                                                  m_uiCancelOption, m_uiChooseOption,

                                                  m_uiSplash, m_uiWaitingForDrives, m_uiDisabled1, m_uiDisabled2,
                                                  m_uiFatalError, m_uiSystemHalted, m_uiCtrlAltDel, m_uiLoadingConfig,
                                                  m_uiLoadingAborted,
                                                  
                                                  m_errRQMTimeout, m_errRecalibrate, m_errSeek, m_errWriProtect,
                                                  m_errINTTimeout, m_errOverrun, m_errCRC,  m_errNoData,
                                                  m_errNoAddrMark, m_errBadTrack, m_chsFmtSTRegsSingle,
                                                  m_chsFmtSTRegsMulti, m_chsFmtSingleSector, m_chsFmtMultiSector,
                                                  
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
                                                  
                                                  m_initializingFDC, m_countDrives, m_specifyParams1, m_specifyParams2, 
                                                  m_driveInches, m_drive8InchText1, m_drive8InchText2, m_drive8InchText3, 
                                                  m_drive8InchText4, m_drive8InchText5, m_drive8InchText6,
                                                  m_drive5InchSides, m_drive5InchSSDD, m_drive5Inch, 
                                                  m_drive5InDoubleStep, m_drive3Inch,
                                                  
                                                  m_specifyingAll, m_customTracks, m_customDoubleStep, m_customChangeline,
                                                  m_customHeads, m_customSpt, m_customSectorSize, m_customSectorGap,
                                                  m_customFormatGap3, m_customFormatFiller, m_customEncoding,
                                                  m_customDataRateSel, m_custom8InchEnc, m_custom5InchFM, m_custom5InchMFM,
                                                  m_custom3InchFM, m_custom3InchMFM, m_customUseFAT, m_customMediaByte,
                                                  m_customRootDir, m_customClusterSize,
                                                  
                                                  m_processInvalidCmd, m_processInvalidDrv, m_processUnknownDrv,
                                                  m_processNoFS, m_processInvalidChr1, m_processInvalidChr2, m_processMaxPath,
                                                  
                                                  m_drivParmCaption, m_drivParmTypeText, m_drivParmType8SD, m_drivParmType8DD,
                                                  m_drivParmType5DD, m_drivParmType5HD, m_drivParmType3DD, m_drivParmType3HD,
                                                  m_drivParmDoubleStep, m_drivParmDiskChange, m_drivParmSRT, m_drivParmHLT,
                                                  m_drivParmHUT, m_drivParmTracks, m_drivParmHeads, m_drivParmHeadsOne,
                                                  m_drivParmHeadsTwo, m_drivParmSpt,  m_drivParmSectorSize, m_drivParmEncoding,
                                                  m_drivParmFM, m_drivParmMFM, m_drivParmDataRate, m_drivParmFDCRate,
                                                  m_drivParmKbps, m_drivParmMbps, m_drivParmSectorGap, m_drivParmFormatGap,
                                                  m_drivParmFormatFill, m_drivParmFS, m_drivParmFAT, m_drivParmCPM,
                                                  m_drivParmFATMedia, m_drivParmFATRootDir, m_drivParmFATCluster,
                                                  
                                                  m_persistStore, m_persistRemove,
                                                  
                                                  m_diskIoFormatVerify, m_diskIoCreateFS, m_diskIoInsertDisk, 
                                                  m_diskIoTotalBadSect, m_diskIoTotalBadTrk, m_diskIoProgress,
                                                  m_diskIoTrack0Error, m_diskIoTryFormat, m_diskIoQuickFormat,
                                                  m_diskIoCreatingFAT, m_diskIoCreatingCPM, m_diskIoFormatOK,
                                                  
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
                                               
  // 8" SSSD 26-sector skew (interleave) table
  PROGMEM_STR m_cpmSkewTable[]       PROGMEM = {  0, // 1-based
                                                  1, 7, 13, 19, 25, 5, 11, 17, 23, 3, 9,  15, 21,
                                                  2, 8, 14, 20, 26, 6, 12, 18, 24, 4, 10, 16, 22
                                               };

// provides for transfering messages between program space and our address space
// even though we're in class Progmem, this is in RAM, not in PROGMEM itself
private:

  inline static unsigned char m_strBuffer[MAX_PROGMEM_STRING_LEN + 1];
  
};