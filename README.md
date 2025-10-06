Linux Kernel Driver Collection
==============================

A collection of Linux kernel drivers and modules developed for educational purposes to demonstrate various kernel programming concepts.


Character Driver
----------------

The main driver (`char-driver/`) is a fully functional **character device driver** that integrates multiple advanced kernel programming concepts.

### Features

#### Basic Operations

-   **Open/Close**: Dynamic kernel memory allocation (kmalloc/kfree)
-   **Read/Write**: Bidirectional data transfer between user-space and kernel-space
-   **IOCTL**: Custom commands for kernel-userspace communication

#### Communication Interfaces

-   **ProcFS** (`/proc/char_proc`): Read/write through proc filesystem
-   **SysFS** (`/sys/kernel/my_sysfs/sysfs_val`): Kernel attributes exposed in sysfs

#### Interrupt Handling

-   IRQ #1 handler (keyboard interrupt)
-   Top-half/bottom-half implementation using **tasklets**
-   Atomic access protection with `spin_lock_irq()`

#### Synchronization and Threading

-   **Spinlock** for protecting shared resources
-   Two kernel threads demonstrating synchronized access to shared variable
-   Threads run periodically with `msleep(1000)`

#### Kernel Timer

-   Periodic timer (5 seconds) with `timer_setup()` and `mod_timer()`

### Data Protection

The `spinlock_variable` is accessed from 4 different contexts:

1.  **Thread 1** - uses `spin_lock()`/`spin_unlock()`
2.  **Thread 2** - uses `spin_lock()`/`spin_unlock()`
3.  **IRQ Handler** - uses `spin_lock_irq()`/`spin_unlock_irq()`
4.  **Tasklet** - uses `spin_lock_irq()`/`spin_unlock_irq()`

This demonstrates proper handling of race conditions in multi-threaded and interrupt-driven environments.

USB Driver
----------

The USB driver (`usb-driver/`) is a basic USB device driver that demonstrates how to integrate with the **Linux USB subsystem**.

### Current Features

- Detects and registers a USB device with specific **Vendor ID / Product ID**
- Allocates a **minor number** and creates a char device node (`/dev/tgusbX`)
- Logs device information when plugged/unplugged (`dmesg`)
- Implements basic `probe` and `disconnect` callbacks

### What Can Be Added / Future Work

1. **File Operations**: Implement `read`/`write` to communicate with the device through `/dev/tgusbX`.
2. **Endpoint Handling**: Support reading/writing data via USB **bulk** or **interrupt endpoints**.
3. **HID Parsing (optional)**: For devices like keyboards or mice, parse HID reports to get keycodes or input events.
4. **User-space Notifications**: Use `poll/select` or async notifications to inform user-space when new data is available.
5. **Support Multiple Devices**: Extend `usb_device_id` table to handle multiple VID/PID devices.
6. **Error Handling & Debugging**: Improve logging with `dev_info`/`dev_err` and handle edge cases like device removal during transfer.

---

Additional Modules
------------------

### Delay Functions (`delay-functions/`)

-   **busy_wait.c**: Busy-wait implementation using `cpu_relax()` and `jiffies`
-   **schedule.c**: Delay with processor yielding

### I/O Ports (`io-ports/`)

-   **ioport.c**: Direct I/O port access using `inl()`/`outl()`
-   **ioport_map.c**: I/O port access via mapping with `ioread32()`/`iowrite32()`

**⚠️ Warning**: I/O port modules manipulate hardware at addresses 0x200-0x240 and may cause system instability.

Requirements
------------

-   Linux kernel 4.11+
-   `gcc`, `make`, `linux-headers`
-   Root/sudo permissions for loading modules

```
sudo apt install build-essential linux-headers-$(uname -r)

```

Build and Installation
----------------------

```
cd char-driver
make
sudo insmod char_driver.ko

# Verify
lsmod | grep char_driver
ls -l /dev/my_device

# Uninstall
sudo rmmod char_driver
make clean

```

### Build IOCTL Test Program

```
gcc test_ioctl.c -o test_ioctl

```

Usage
-----

### Basic Operations

```
# Write/Read
echo "Hello Kernel!" > /dev/my_device
cat /dev/my_device

```

### IOCTL Test

```
sudo ./test_ioctl

```

### ProcFS and SysFS

```
# ProcFS
cat /proc/char_proc
echo "New data" | sudo tee /proc/char_proc

# SysFS
cat /sys/kernel/my_sysfs/sysfs_val
echo 42 | sudo tee /sys/kernel/my_sysfs/sysfs_val

```

### Monitor Activity

```
# Follow kernel log
sudo dmesg -w

# Check synchronization
sudo dmesg | grep "thread function"
sudo dmesg | grep "spinlock"
sudo dmesg | grep "Interrupt"

```

Author
------

**Tavi Gingu**

License
-------

GPL (GNU General Public License)