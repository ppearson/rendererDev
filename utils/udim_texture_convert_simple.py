from subprocess import call
import os

source = "/home/peter/textures/udim1/scanline/checker_4k_%d.exr"
dest = "/home/peter/textures/udim1/tiledmip/checker_4k_%d.exr"

mkTextBin = "maketx"

for udim in range(1001, 1171):
	localSrc = source % (udim)
	localDst = dest % (udim)

	#print localSrc + "   " + localDst

	args1 = "--tile 32 32 %s -o %s" % (localSrc, localDst)

	#call([mkTextBin, args1])
	os.system(mkTextBin + " " + args1)
