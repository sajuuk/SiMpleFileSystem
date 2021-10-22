obj-m += smfs.o
simplefs-objs := fs.o super.o inode.o

KDIR ?= /lib/modules/$(shell uname -r)/build

MKFS = mkfs.simplefs

all: $(MKFS)
	make -C $(KDIR) M=$(shell pwd) modules


$(MKFS): mkfs.c
	$(CC) -std=gnu99 -Wall -o $@ $< -lm


clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f *~ $(PWD)/*.ur-safe

.PHONY: all clean
