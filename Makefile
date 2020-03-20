PWD := $(shell pwd)
CROSS := /home/hypso/Desktop/toolchain/bin/arm-linux-gnueabihf-
KERNEL := /home/hypso/Desktop/linux-4.19

obj-m += timestamp_module.o
timestamp_module-objs := c/timestamp_module.o c/src/hyptimer.o

all:
	make ARCH=arm CROSS_COMPILE=$(CROSS) -C $(KERNEL) SUBDIRS=$(PWD) modules
clean:
	make -C $(KERNEL) SUBDIRS=$(PWD) clean
