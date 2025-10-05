#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,10)
#include <linux/sched/signal.h>
#endif

int delay = HZ;

static struct task_struct *thread_wait;

static int thread_func(void* wait)
{
    unsigned long jif1, jif2;
    
    allow_signal(SIGKILL);
    while(! kthread_should_stop())
    {
        jif1 = jiffies;
        jif2 = jif1 + delay;

        while(time_before(jiffies,jif2))
            cpu_relax();

        jif2 = jiffies;

        printk(KERN_INFO "Jiffies starts = %lu\t and jiffies end = %lu\n", jif1, jif2);
        if(signal_pending(current))
            break;
    }

    printk(KERN_INFO "Thread is stopped\n");
    thread_wait = NULL;

    return 0;
}

static int __init my_init(void)
{
    printk(KERN_INFO "Creating thread..\n");
    thread_wait = kthread_run(thread_func, NULL, "mythread");
    
    return 0;
}

static void __exit my_exit(void)
{
    printk("Removing the module...\n");

    if(thread_wait != NULL) {
        kthread_stop(thread_wait);
        printk(KERN_INFO "Stopping the thread!\n");
    }
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tavi Gingu");
MODULE_DESCRIPTION("Schedule sample code");