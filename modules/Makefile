obj-m += testmod.o

# In case building for the running kernel
#KERNELDIR ?= /lib/modules/$(shell uname -r)/build

# We'll use another kernel that we run in a VM
KERNELDIR ?= /var/git/kernels/linux

PWD := $(shell pwd)


all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

