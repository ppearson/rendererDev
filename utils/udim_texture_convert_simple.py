from subprocess import call
import os

source = "/home/peter/Content/Source/Textures/udims/udim2_%d.png"
dest = "/home/peter/Content/Source/Textures/udims/tiled/udim1_%d.exr"

mkTextBin = "maketx"

for udim in range(1001, 1100):
	localSrc = source % (udim)
	localDst = dest % (udim)

	#print localSrc + "   " + localDst

	args1 = "--tile 32 32 %s -o %s" % (localSrc, localDst)

	#call([mkTextBin, args1])
	os.system(mkTextBin + " " + args1)
