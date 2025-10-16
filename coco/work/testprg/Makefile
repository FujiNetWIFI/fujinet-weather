CC=cmoc
CP=cp

all: wgtest.dsk

wgtest.dsk: wgtest.c
	$(CC) -owgtest.bin wgtest.c
	$(CC) -I../../../_libs/4.7.9-coco -c -o wctest.o wctest.c
	$(CC) -o wctest.bin wctest.o  -L ../../../_libs/4.7.9-coco -lfujinet-coco-4.7.9.lib
	$(RM) wgtest.dsk
	decb dskini wgtest.dsk
	writecocofile wgtest.dsk wgtest.bin
	writecocofile wgtest.dsk wctest.bin
#	$(CP) wgtest.dsk ~/Workspace/tnfs

clean:
	$(RM) wgtest.bin wctest.bin wgtest.dsk


