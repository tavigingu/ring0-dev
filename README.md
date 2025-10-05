Linux Kernel Driver Collection
==============================

A collection of Linux kernel drivers and modules developed for educational purposes to demonstrate various kernel programming concepts.

Overview
--------

This repository contains Linux kernel driver implementations demonstrating:

-   Character device manipulation
-   Interrupt handling (IRQ)
-   Synchronization with spinlocks
-   Kernel threads
-   Tasklets and timers
-   ProcFS and SysFS interfaces
-   IOCTL for kernel-userspace communication
-   Delay functions and I/O port operations

Character Driver
----------------

The main driver (`char-driver/`) is a fully functional **character device driver** that integrates multiple advanced kernel programming concepts.

### Features

#### 🔧 Basic Operations

-   **Open/Close**: Dynamic kernel memory allocation/deallocation (kmalloc/kfree)
-   **Read/Write**: Bidirectional data transfer between user-space and kernel-space
-   **IOCTL**: Custom commands for advanced communication

#### 📁 Communication Interfaces

**ProcFS** (`/proc/char_proc`):

-   Read/write text through the `/proc` filesystem
-   Complete implementation with `proc_ops`

**SysFS** (`/sys/kernel/my_sysfs/sysfs_val`):

-   Expose kernel attributes in sysfs
-   Read/write values through `kobject` and `kobj_attribute`

#### ⚡ Interrupt Handling

-   Handler for IRQ #1 (keyboard interrupt)
-   Top-half/bottom-half implementation using **tasklets**
-   Protection with `spin_lock_irq()` for atomic access to shared resources

#### 🔄 Synchronization and Threading

-   **Spinlock** (`DEFINE_SPINLOCK`) for protection in interrupt context
-   Two kernel threads (`kthread_create` and `kthread_run`) that:
    -   Modify a shared variable in a synchronized manner
    -   Demonstrate correct usage of spinlocks
    -   Run periodically with `msleep(1000)`

#### ⏱️ Kernel Timer

-   Periodic timer (5 seconds) implemented with `timer_setup()` and `mod_timer()`
-   Callback function that automatically reactivates the timer

### Architecture

```
Character Driver
├── Device File: /dev/my_device
├── Proc Entry: /proc/char_proc
├── Sysfs Entry: /sys/kernel/my_sysfs/sysfs_val
├── IRQ Handler (IRQ #1)
│   ├── Top Half: Spinlock synchronization
│   └── Bottom Half: Tasklet execution
├── Kernel Threads (×2)
│   └── Synchronized access to shared variable
└── Kernel Timer (periodic 5s)

```

```

Requirements
------------

-   **Operating System**: Linux (kernel 4.11+)
-   **Build tools**:
    -   `gcc`
    -   `make`
    -   `linux-headers` for current kernel
-   **Permissions**: root/sudo for loading modules

### Install Dependencies (Ubuntu/Debian)

```
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)

```

Build and Installation
----------------------

### Character Driver

```
cd char-driver
make

# Load module
sudo insmod char_driver.ko

# Verify loading
lsmod | grep char_driver
dmesg | tail -20

# Check device creation
ls -l /dev/my_device

```

### Build IOCTL Test Program

```
gcc test_ioctl.c -o test_ioctl

```

### Uninstall

```
sudo rmmod char_driver
make clean

```

Usage
-----

### 1\. Basic Operations (Read/Write)

```
# Write data
echo "Hello Kernel!" > /dev/my_device

# Read data
cat /dev/my_device

```

### 2\. IOCTL Test

```
sudo ./test_ioctl

```

Expected output:

```
Device opened successfully
Value 123 sent to driver via ioctl
Value read from driver via ioctl: 123
Device closed

```

### 3\. ProcFS Interaction

```
# Read
cat /proc/char_proc

# Write
echo "New data" | sudo tee /proc/char_proc

```

### 4\. SysFS Interaction

```
# Read value
cat /sys/kernel/my_sysfs/sysfs_val

# Write value
echo 42 | sudo tee /sys/kernel/my_sysfs/sysfs_val

```

### 5\. Monitor Activity

```
# Follow kernel log in real-time
sudo dmesg -w

# Check threads and synchronization
sudo dmesg | grep "thread function"
sudo dmesg | grep "spinlock"

# Check IRQ
sudo dmesg | grep "Interrupt"
cat /proc/interrupts | grep char_device

```

### Debugging and Information

```
# Device information
ls -l /sys/class/my_class/my_device/

# Major/Minor numbers
cat /proc/devices | grep my_dev

# Reserved I/O ports
cat /proc/ioports

```

Additional Modules
------------------

### Delay Functions

The modules in `delay-functions/` demonstrate different techniques for implementing delays in the kernel:

-   **busy_wait.c**: Busy-wait implementation using `cpu_relax()` and `jiffies`
-   **schedule.c**: Delay with processor yielding

```
cd delay-functions
make
sudo insmod busy_wait.ko  # or schedule.ko
sudo rmmod busy_wait

```

### I/O Ports

The modules in `io-ports/` demonstrate direct I/O port manipulation:

-   **ioport.c**: Direct access using `request_region()`, `inl()`, `outl()`
-   **ioport_map.c**: Access through mapping with `ioport_map()`, `ioread32()`, `iowrite32()`

```
cd io-ports
make
sudo insmod ioport.ko  # or ioport_map.ko
sudo rmmod ioport

```

**⚠️ Warning**: The I/O port modules directly manipulate hardware at specific addresses (0x200-0x240) and may cause system instability if these addresses are already in use.

**Important Notes:**

-   This code is intended for educational purposes
-   Always test in a virtual/safe environment
-   Kernel modules can cause system instability if not used properly
-   Data backup is recommended before testing