// MegaFDC diskio overrides
// (c) 2023 J. Bogin

#include "diskio.h"
#include "..\..\config.h" // we

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
