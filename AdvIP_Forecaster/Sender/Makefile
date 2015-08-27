.PHONY: default clean

obj-m := packetfilter.o

KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -rf *.ko *.o *.mod.* .H* .tm* .*cmd Module.symvers

