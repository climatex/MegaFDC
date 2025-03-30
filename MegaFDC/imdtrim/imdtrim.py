# Trim IMD image to original length, having been received via XMODEM
# (c) J. Bogin, 2024
# Run: python imdtrim.py image.imd

import sys
import io

# Verifies the IMD file has a text header beginning with "IMD " and ending with ASCII EOF
def find_header_end(imd):
  imd.seek(0)
  signature = imd.read(4)
  if (signature != b'IMD '):
    return False
  while True:
    byte = imd.read(1)
    if (not byte):
      return False
    elif (byte[0] == 0x1A):
      return True

def was_end_of_file(imd):
  currPos = imd.tell()
  imd.seek(0, 2)
  endPos = imd.tell()
  imd.seek(currPos, 0)
  return (currPos >= endPos)

# Parse the sequential IMD image each track and sector and stop at an invalid track Mode 0x1A,
# which - in our case - indicates XMODEM end of transfer packet.
# Naturally the easiest way would be to do a reverse file search for 0x1A stopping at the last occurence,
# however, this would break any IMD image having 0x1A as a completely valid last sector record.
def find_last_mode(imd):
  while True:
    mode = imd.read(1)
    if (not mode):
      return False
    elif (mode[0] == 0x1A):
      return True
    elif (mode[0] > 5):
      return False
    imd.read(1)
    head = imd.read(1)
    if (not head):
      return False
    if ((head[0] & 0x3F) > 1):
      return False
    cylmap = ((head[0] & 0x80) > 0)
    headmap = ((head[0] & 0x40) > 0)
    spt = imd.read(1)
    if (not spt):
      return False
    secsize = imd.read(1)
    if (not secsize):
      return False
    if (secsize[0] > 6):
      return False
    if (spt[0] == 0):
      continue
    imd.read(spt[0])
    if (cylmap):
      imd.read(spt[0])
    if (headmap):
      imd.read(spt[0])
    currSector = 0
    while (currSector < spt[0]):
      databyte = imd.read(1)
      if (not databyte):        
        return False
      if (databyte[0] > 8):        
        return False
      if (databyte[0] == 0):
        currSector += 1
        continue
      compressed = ((databyte[0] % 2) < 1)
      if (compressed):
        imd.read(1)
      else:
        imd.read(128 << secsize[0])
      currSector += 1
      
# A reminder to use this script after receiving the file, was part of the description added by MegaFDC.
# These descriptions are viewable in the IMD app upon loading the image, or via TYPE in a command line.
# Now that we set the file size correctly, there's no more need to have this in the description.
def remove_warning(imd):
  imd.seek(43)
  if (imd.read(30) == b"Run 'imdtrim.py' before using!"):
    imd.seek(43)
    imd.write(b"                                ")
  return

def main():
  imd = 0
  try:
    imd = open(sys.argv[1], "r+b")
  except:
    print("Usage: imdtrim.py image.imd")
    return
  if (not find_header_end(imd)):
    print("Invalid IMD file header")
    imd.close()
    return
  if (not find_last_mode(imd)):
    if (not was_end_of_file(imd)):
      print("Invalid data in IMD file")
    else:
      print("Trim likely not required")
      remove_warning(imd) # if it happens to have one
    imd.close()
    return
  newsize = imd.tell() - 1
  imd.truncate(newsize)
  remove_warning(imd)
  imd.close()
  print("Done!")  
  return

if __name__ == "__main__":
  main()
