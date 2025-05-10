import sys
import os

with open(sys.argv[1], "rb") as vbrfile, open(sys.argv[2], "rb+") as imagefile:
    imagefile.seek(1)
    code_start = imagefile.read(1)[0] + 2
    imagefile.seek(code_start)
    vbrfile.seek(code_start)
    imagefile.write(vbrfile.read(510 - code_start))
    