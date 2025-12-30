import sys
import os

with open(sys.argv[1], "rb") as srcfile, open(sys.argv[2], "rb+") as destfile:
    srcfile.seek(0, os.SEEK_END)
    srcsize = srcfile.tell()
    srcfile.seek(0, os.SEEK_SET)
    
    destfile.seek(int(sys.argv[3]))
    destfile.write(srcfile.read(srcsize))
    