obj-m += busy_wait.o
obj-m += schedule.o
ccflags-y := -Wno-error

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

