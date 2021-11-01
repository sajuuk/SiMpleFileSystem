obj-m += SiMpleFileSystem.o
SiMpleFileSystem-objs := fs.o superOps.o inodeOps.o fileOps.o dirOps.o 

KDIR ?= /lib/modules/$(shell uname -r)/build

MKFS = mkfs

all: $(MKFS)
	make -C $(KDIR) M=$(shell pwd) modules


$(MKFS): mkfs.c
	$(CC) -std=gnu99 -Wall -o $@ $< -lm -g


clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f *~ $(PWD)/*.ur-safe

.PHONY: all clean
