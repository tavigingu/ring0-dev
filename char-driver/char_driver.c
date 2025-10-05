#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h> //kmalloc
#include <linux/uaccess.h> //copy to/from user
#include <linux/ioctl.h>
#include <linux/proc_fs.h> 
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>

#define IRQ_NO 1 //keyboard interrupt line
#define mem_size 1024
#define TIMEOUT 5000 //millisecond

unsigned int i = 0;
static struct timer_list char_timer;
//timer callback function which is called when timer expires
static void timer_callback(struct timer_list *timer)
{
    //printk(KERN_INFO "in timer callback function[%d]\n", i++);
    //re-enabled the timer which will make this as periodic timer
    mod_timer(&char_timer, msecs_to_jiffies(TIMEOUT));
}

//intrerrupt handler for IRQ 1
void tasklet_func(struct tasklet_struct* t);
//struct tasklet_struct *tasklet;
DECLARE_TASKLET(tasklet, tasklet_func); // static method
DEFINE_SPINLOCK(char_spinlock);
unsigned long spinlock_variable = 0;

volatile int char_value = 0;

//define ioctl code
#define WR_DATA _IOW('a', 'a', int32_t*)
#define RD_DATA _IOR('a', 'b', int32_t*)
uint32_t val = 0;

unsigned int sysfs_val = 0;
struct kobject *kobj_ref;

dev_t dev = 0;
static struct class* dev_class;
static struct cdev my_cdev;
static struct proc_dir_entry *proc_entry;

uint8_t *kernel_buffer;
char char_array[] = "hello from  the kernel deriver\n";
static int len = 1;

static int __init char_driver_init(void);
static void __exit char_driver_exit(void);

static struct task_struct *char_thread;
static struct task_struct *char_thread2;

/*******************Driver functions***************************/
static int char_open(struct inode* inode, struct file* file);
static int char_release(struct inode* inode, struct file* file);
static ssize_t char_read(struct file* filp, char __user *buf, size_t len, loff_t *offset);
static ssize_t char_write(struct file *filp, const char __user *buf, size_t len, loff_t *offset);
static long char_ioctl(struct file* file, unsigned int cmd, unsigned long arg);
int thread_function(void *p);
int thread_function2(void *p);
static irqreturn_t irq_handler(int irq, void* dev_id);

/*******************Sysfs functions****************************/
static ssize_t sysfs_show(struct kobject *kobj, struct kobj_attribute *attr, char* buf);
static ssize_t sysfs_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

struct kobj_attribute sysfs_attr = __ATTR(sysfs_val, 0660, sysfs_show, sysfs_store);

/*******************Proc entry functions***********************/
static int open_proc(struct inode* inode, struct file* file);
static int release_proc(struct inode* inode, struct file* file);
static ssize_t read_proc(struct file* filp, char __user *buffer, size_t length, loff_t* offset);
static ssize_t write_proc(struct file* filp, const char* buffer, size_t length, loff_t* offset);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = char_read,
    .write = char_write,
    .open = char_open,
    .unlocked_ioctl = char_ioctl,
    .release = char_release,
};

static struct proc_ops proc_ops = {
    .proc_open = open_proc,
    .proc_read = read_proc,
    .proc_write = write_proc,
    .proc_release = release_proc,
};

int thread_function(void *p) 
{
    //int i=0;
    while(!kthread_should_stop()){
        if(!spin_is_locked(&char_spinlock)) {
            printk(KERN_INFO "Spin is not locked in thread function1\n");
        }
        spin_lock(&char_spinlock);
        if(spin_is_locked(&char_spinlock)) {
            printk(KERN_INFO "spinlock is locked in thread function1\n");
        }
        spinlock_variable ++;
        printk(KERN_INFO "I'm in thread function 1 %lu\n", spinlock_variable);
        spin_unlock(&char_spinlock);
        msleep(1000);
    }

    return 0;
}

int thread_function2(void *p)
{
    while(!kthread_should_stop()){
        if(!spin_is_locked(&char_spinlock)) {
            printk(KERN_INFO "Spin is not locked in thread function2\n");
        }
        spin_lock(&char_spinlock);
        if(spin_is_locked(&char_spinlock)) {
            printk(KERN_INFO "spinlock is locked in thread function2\n");
        }
        spinlock_variable ++;
        printk(KERN_INFO "I'm in thread function2 %lu\n", spinlock_variable);
        spin_unlock(&char_spinlock);
        msleep(1000);
    }

    return 0;
}

void tasklet_func(struct tasklet_struct *t)
{   
    spin_lock_irq(&char_spinlock);
    spinlock_variable++;
    printk(KERN_INFO "Executine the Tasklet function %lu\n", spinlock_variable);
    spin_unlock_irq(&char_spinlock);
}

static irqreturn_t irq_handler(int irq, void* dev_id)
{
    spin_lock_irq(&char_spinlock); //block access + disable local interrupts so they don't enter the IRQ handler while the lock is held
    spinlock_variable++;
    printk(KERN_INFO "keyboard: Intrerrupt Ocurred %lu\n", spinlock_variable);
    spin_unlock_irq(&char_spinlock);

    tasklet_schedule(&tasklet); //bottom half
    return IRQ_HANDLED;
}

static int open_proc(struct inode* inode, struct file* file)
{
    printk(KERN_INFO "ProcFs file is opened\n");
    return 0;
}

static ssize_t read_proc(struct file* filp, char __user *buffer, size_t length, loff_t* offset)
{
    static int finished = 0;

    if (finished) {
        finished = 0;
        return 0; // EOF
    }

    finished = 1;
    if (copy_to_user(buffer, char_array, sizeof(char_array))) {
        return -EFAULT;
    }

    printk(KERN_INFO "ProcFs file reading...\n");
    return sizeof(char_array);
}

static ssize_t write_proc(struct file* filp, const char* buffer, size_t length, loff_t* offset)
{
     size_t to_copy = min(length, sizeof(char_array) - 1);

    if (copy_from_user(char_array, buffer, to_copy)) {
        return -EFAULT;
    }

    char_array[to_copy] = '\0';
    printk(KERN_INFO "ProcFs file writing...\n");
    return to_copy;
}

static int release_proc(struct inode* inode, struct file* file)
{
     printk(KERN_INFO "ProcFs file is released\n");
    return 0;
}

static int char_open(struct inode* inode, struct file* file)
{
    //creating physical memory
    if((kernel_buffer = kmalloc(mem_size, GFP_KERNEL)) == 0) {
        printk(KERN_INFO "Cannot allocate the memory to the kernel\n");
        return -1;
    }
    printk(KERN_INFO "Device file opened!\n");
    return 0;
}

static int char_release(struct inode* inode, struct file* file)
{
    kfree(kernel_buffer);
    printk(KERN_INFO "Device FILE closed!\n");
    return 0;
}

static ssize_t char_read(struct file* filp, char __user *buf, size_t len, loff_t *offset)
{
    size_t to_copy = min(mem_size, len);

    if(*offset >= mem_size)
        return 0;  //EOF

    int result = 0;
    result = copy_to_user(buf, kernel_buffer, to_copy);

    *offset += to_copy;

    printk(KERN_INFO "Data read: DONE\n");

    return to_copy;
}

static ssize_t char_write(struct file *filp, const char __user * buf, size_t len, loff_t *offset)
{
    copy_from_user(kernel_buffer, buf, len);
    printk(KERN_INFO "Data is written successfully\n");
    return len;
}

static long char_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
    switch(cmd) {
        case WR_DATA: //user-space send an uint32_t
            copy_from_user(&val, (int32_t*)arg, sizeof(val));
            printk(KERN_INFO "val = %d\n", val);
            break;
        case RD_DATA: //kernel send an uint32_t to user_space
            copy_to_user((int32_t*)arg, &val, sizeof(val));
            break;
        default:
            return -EINVAL;
    }

    return 0;
}


static ssize_t sysfs_show(struct kobject *kobj, struct kobj_attribute *attr, char* buf)
{
    printk(KERN_INFO "Reading - sysfs show function...\n");
    return sprintf(buf, "%d", sysfs_val);
}

static ssize_t sysfs_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    printk(KERN_INFO "Writing - sysfs store function...\n");
    sscanf(buf, "%d", &sysfs_val);
    return count;
}

static int __init char_driver_init(void)
{
    if((alloc_chrdev_region(&dev, 0,1,"my_dev")) < 0){
        printk(KERN_INFO "Cannot allocate the major number");
        return -1;
    }

    printk(KERN_INFO "MAJOR = %d MINOR = %d\n", MAJOR(dev), MINOR(dev));

    cdev_init(&my_cdev, &fops);

    if((cdev_add(&my_cdev, dev, 1)) < 0) {
        printk(KERN_INFO "Cannot add the device to the system");
        unregister_chrdev_region(dev,1);
        return -1;
    }

    if((dev_class = class_create("my_class")) == NULL) {
        printk(KERN_INFO "Cannot create the struct class");
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev,1);
        return -1;
    }

    if((device_create(dev_class, NULL, dev, NULL, "my_device"))== NULL) {
        printk(KERN_INFO "Cannot create device");
        class_destroy(dev_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev,1);
        return -1;
    }

    proc_entry = proc_create("char_proc", 0666, NULL, &proc_ops);

     if (request_irq(IRQ_NO, irq_handler, IRQF_SHARED, "char_device", (void*)(irq_handler)))
    {
        printk(KERN_INFO "char_device: cannot register IRQ\n");
        return -1;
    }

    //creating the directory in /sys/kernel
    kobj_ref = kobject_create_and_add("my_sysfs", kernel_kobj);
    //creating the sysfs file
    if(sysfs_create_file(kobj_ref, &sysfs_attr.attr)){
        printk(KERN_INFO "Unable to create the sysfs file..\n");
        kobject_put(kobj_ref);
        sysfs_remove_file(kernel_kobj, &sysfs_attr.attr);
    } 

    // tasklet = kmalloc(sizeof(struct tasklet_struct), GFP_KERNEL);
    // if(tasklet == NULL) {
    //     printk(KERN_INFO "Cannot allocate memory\n");
    //     goto irq;
    // }

    // tasklet_init(tasklet, tasklet_func,0); //dynamic method

    char_thread = kthread_create(thread_function, NULL, "char thread 1"); 
    if(char_thread) {
        wake_up_process(char_thread);
    } else {
        printk(KERN_INFO "Unable to create the thread");
        return -1;
    }

    //2nd method to start thread
    char_thread2 = kthread_run(thread_function2, NULL, "char thread 2"); //wrapper: kthread_create() + wake_up_process().
    if(char_thread2){
        printk(KERN_INFO "Successfully created the kernel thread...\n");
    } else {
        printk(KERN_INFO "Unable to create the thread..\n");
    }

    //setup your timer to call timer callback function
    timer_setup(&char_timer, timer_callback,0);
    //setup the timer interval to base on TIMEOUT macro
    mod_timer(&char_timer, jiffies + msecs_to_jiffies(TIMEOUT));

    printk(KERN_INFO "Device driver inserted\n");
    return 0; 
}


void __exit char_driver_exit(void)
{   
    kthread_stop(char_thread);
    kthread_stop(char_thread2);
    free_irq(IRQ_NO, (void*)(irq_handler));
    device_destroy(dev_class, dev);
    sysfs_remove_file(kernel_kobj, &sysfs_attr.attr);
    del_timer(&char_timer);
    class_destroy(dev_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev, 1);
    proc_remove(proc_entry);
    printk(KERN_INFO "Device driver is removed succcessfully\n");
}

module_init(char_driver_init);
module_exit(char_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tavi Gingu");
MODULE_DESCRIPTION("Character device driver");