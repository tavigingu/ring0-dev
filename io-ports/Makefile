obj-m += ioport_map.o
obj-m += ioport.o
ccflags-y := -Wno-error

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

