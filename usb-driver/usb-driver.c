#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>

#define USB_VENDOR_ID 0xaaaa
#define USB_PRODUCT_ID 0x8816

static struct usb_class_driver usb_cd = {
    .name = "tgusb%d",
    .minor_base = 0,
};

static int usb_driver_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct usb_host_interface *interface_desc;
    struct usb_endpoint_descriptor *endpoint __maybe_unused;
    int ret;

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
    return usb_register_driver(&my_usb_driver, THIS_MODULE, KBUILD_MODNAME);
}

static void __exit usb_exit(void)
{
    printk(KERN_INFO "Deregister the usb driver with the usb subsystem\n");
    usb_deregister(&my_usb_driver);
}

module_init(usb_init);
module_exit(usb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tavi Gingu");
MODULE_DESCRIPTION("USB Driver");