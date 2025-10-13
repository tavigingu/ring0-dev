#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <net/netlink.h>
#include <net/sock.h>

#define USB_VENDOR_ID 0xaaaa
#define USB_PRODUCT_ID 0x8816
#define NETLINK_USER 31 //protocl number 0-31

static struct sock *nl_sk = NULL;

static struct usb_class_driver usb_cd = {
    .name = "tgusb%d",
    .minor_base = 0,
};

//send message to user space
static void send_msg_to_user(char* msg, int pid)
{
    struct sk_buff *skb_out;

    //header specific netlink
    struct nlmsghdr *nlh;
    int msg_size = strlen(msg);
    int res;

    if(!nl_sk) {
        printk(KERN_ERR "Netlinnk socket not init\n");
        return;
    }

    skb_out = nlmsg_new(msg_size, GFP_KERNEL);
    if(!skb_out) {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }

    nlh = nlmsg_put(skb_out, 0,0, NLMSG_DONE, msg_size, 0);
    if(!nlh) {
        printk(KERN_ERR "Failed to put netlink mesasge\n");
        kfree_skb(skb_out);
        return;
    }

    NETLINK_CB(skb_out).dst_group = 0; //unicast

    strncpy(nlmsg_data(nlh), msg, msg_size);

    //send message
    res = nlmsg_unicast(nl_sk, skb_out, pid);
    if(res < 0) {
        printk(KERN_ERR "Erro sending netlink message: %d\n", res);
    }
    else {
        printk(KERN_INFO "Netlink message net to pid %d: %s\n", pid, msg);
    }
}

//callback receiving messages from user space
static void nl_recv_msg(struct sk_buff* skb)
{
    struct nlmsghdr *nlh;
    int pid;
    char* msg;
    char response[256];

    nlh = (struct nlmsghdr*)skb->data;
    pid = nlh->nlmsg_pid; //pid of the process that sent message
    msg = (char*)nlmsg_data(nlh);

    printk(KERN_INFO "Netlink received from PID %d: %s\n", pid, msg);

    snprintf(response, sizeof(response), "Kernel received: %s", msg);
    send_msg_to_user(response, pid);

}

static int netlink_init(void)
{
    struct netlink_kernel_cfg cfg = {
        .input = nl_recv_msg //callback for recv messages
    };

    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if(!nl_sk) {
        printk(KERN_ERR "Error creating Netlink socket\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "Netlink socket created successfully\n");

    return 0;

}

static void netlink_exit(void)
{
    if(nl_sk) {
        netlink_kernel_release(nl_sk);
        printk(KERN_INFO "Netlink socker release\n");
    }
}

static int usb_driver_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct usb_host_interface *interface_desc;
    struct usb_endpoint_descriptor *endpoint __maybe_unused;
    int ret;
    char msg[256];

    interface_desc = interface->cur_altsetting;

    printk(KERN_INFO "USB info %d now probed: (%04X:%04X)\n",interface_desc->desc.bInterfaceNumber, id->idVendor, id->idProduct);
    printk(KERN_INFO "ID->bNumEndpoints : %02X\n", interface_desc->desc.bNumEndpoints);
    printk(KERN_INFO "ID->bInterfaceClass: %02X\n", interface_desc->desc.bInterfaceClass);

    ret = usb_register_dev(interface, &usb_cd);
    if(ret) {
        printk(KERN_INFO "Not able to get the minor number");
    } else {
        printk(KERN_INFO "Minor number = %d\n", interface->minor);
    }

    snprintf(msg, sizeof(msg), "USB_DEVICE_CONNECTED:%04X:%04X:MINOR=%d",id->idVendor, id->idProduct, interface->minor);
    printk(KERN_INFO "USB device connected - ready to notify via Netlink\n");

    return ret;
}

static void usb_driver_disconnect(struct usb_interface *interface)
{
    printk(KERN_INFO "Disconnected and realease the MINOR number %d\n", interface->minor);
    usb_deregister_dev(interface, &usb_cd);
}

static struct usb_device_id my_usb_driver_table[] = {
    {
        USB_DEVICE(USB_VENDOR_ID, USB_PRODUCT_ID)
    },
    {}
};

MODULE_DEVICE_TABLE(usb, my_usb_driver_table);

static struct usb_driver my_usb_driver = {
    .name = "TG USB Driver",
    .probe = usb_driver_probe,
    .disconnect = usb_driver_disconnect,
    .id_table = my_usb_driver_table,
};

static int __init usb_init(void)
{   
    printk(KERN_INFO "Register the usb driver with the usb subsystem\n");
    netlink_init();

    return usb_register_driver(&my_usb_driver, THIS_MODULE, KBUILD_MODNAME);
}

static void __exit usb_exit(void)
{
    printk(KERN_INFO "Deregister the usb driver with the usb subsystem\n");
    netlink_exit();
    usb_deregister(&my_usb_driver);
}

module_init(usb_init);
module_exit(usb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tavi Gingu");
MODULE_DESCRIPTION("USB Driver");