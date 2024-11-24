// MegaFDC diskio overrides
// (c) 2023-2024 J. Bogin

#include "..\..\config.h" // we

#ifndef BUILD_IMD_IMAGER

#include "diskio.h"

// disk rwBuffer[0...511] is current 512B FAT sector data (FatFs operates in single sectors)
// minimum SECTOR_BUFFER_SIZE is 1K to support 8" DSDD, so use the upper half for FAT file data window
// FATFS::win still holds directory and FAT information
BYTE* get_file_window()
{
  return &g_rwBuffer[512];
}
#if SECTOR_BUFFER_SIZE < 1024
#error Minimum sector buffer size is 1K
#endif

DSTATUS disk_status(BYTE pdrv)
{ 
  // unused
  return 0;
}

DSTATUS disk_initialize(BYTE pdrv)
{ 
  // unused
  return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buf, DWORD sec, UINT count)
{
  // FATFS operates in single sectors
  if ((count != 1) || (sec > fdc->getTotalSectorCount()))
  {
    return RES_PARERR;
  }
  
  BYTE track;
  BYTE head;
  BYTE sector;
  fdc->convertLogicalSectorToCHS(sec, track, head, sector);
  
  // seek, if necessary
  if ((fdc->getCurrentTrack() != track) || (fdc->getCurrentHead() != head))
  {
    fdc->seekDrive(track, head);
  }
    
  // read 1 sector
  fdc->readWriteSectors(false, sector, sector);
  if (fdc->getLastError())
  {
    if (fdc->wasErrorNoDiskInDrive())
    {
      return RES_NOTRDY;
    }

    return RES_ERROR;
  }
  
  memmove(buf, g_rwBuffer, fdc->getParams()->SectorSizeBytes);
  return RES_OK;
}


// analog to the one above
DRESULT disk_write(BYTE pdrv, BYTE *buf, DWORD sec, UINT count)
{
  if ((count != 1) || (sec > fdc->getTotalSectorCount()))
  {
    return RES_PARERR;
  }
  
  BYTE track;
  BYTE head;
  BYTE sector;
  fdc->convertLogicalSectorToCHS(sec, track, head, sector);
  
  memcpy(g_rwBuffer, buf, fdc->getParams()->SectorSizeBytes);
  
  if ((fdc->getCurrentTrack() != track) || (fdc->getCurrentHead() != head))
  {
    fdc->seekDrive(track, head);
  }
    
  // write
  fdc->readWriteSectors(true, sector, sector);
  if (fdc->getLastError())
  {
    if (fdc->wasErrorNoDiskInDrive())
    {
      return RES_NOTRDY;
    }
    
    else if (fdc->wasErrorDiskProtected())
    {
      return RES_WRPRT;
    }
    
    return RES_ERROR;
  }
  
  return RES_OK;
}


DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
  DRESULT res = RES_ERROR;

  switch (cmd)
  {
  case CTRL_SYNC: 
    res = RES_OK; 
    break;

  case GET_SECTOR_COUNT:
    *((DWORD *) buff) = (DWORD)fdc->getTotalSectorCount();
    res = RES_OK;
    break;

  case GET_SECTOR_SIZE:
    *((DWORD *) buff) = fdc->getParams()->SectorSizeBytes;
    break;

  case GET_BLOCK_SIZE:
    *((DWORD *) buff) = 1;
    break;
  }
  
  return res;
}

#endif
