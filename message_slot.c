// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include "message_slot.h"
#include <linux/ioctl.h>
#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/slab.h>
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/

MODULE_LICENSE("GPL");



static MessageSlot* messageSlot;
static Devices* devicesHead;
static Devices* devicesCurr;


//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode *inode,
                       struct file *file) {

    int minor;
    devicesCurr = devicesHead;
    minor = iminor(inode);

    printk("=====OPEN DEVICE %d=====\n", minor);
    printk("current minor is %d\n", devicesCurr -> minor);
    printk("Invoking device_open(%p) with minor %d\n", file, minor);

    Devices* node = devicesHead;

    // Look for the minor.
    while (devicesCurr -> next != NULL) {
        if (devicesCurr->minor == minor) {
            break;
        }
        devicesCurr = devicesCurr->next;
    }

    // First time we see this minor number, or first node.
    if (devicesCurr -> minor != minor){
        printk("First time we see minor or first node\n");
        Devices * newNode = (Devices *) kmalloc(sizeof(Devices), GFP_KERNEL);
        if (!newNode){
            return -EINVAL;
        }

        devicesCurr -> next = newNode;
        devicesCurr = newNode;

        // Now working on the new node.

        devicesCurr -> slot = (MessageSlot *) kmalloc(sizeof(MessageSlot), GFP_KERNEL);
        if (!devicesCurr -> slot){
            return -EINVAL;
        }

        devicesCurr -> slot -> head = (Channel *) kmalloc(sizeof(Channel), GFP_KERNEL);
        if (!devicesCurr -> slot -> head){
            return -EINVAL;
        }
        devicesCurr -> slot -> curr = devicesCurr -> slot -> head;
        devicesCurr -> slot -> size = 0;
        devicesCurr -> minor = minor;
        devicesCurr -> next = NULL;
    }
    // Either way, we now have the relevant minor node.
    messageSlot = devicesCurr -> slot;

    printk("On minor %d we have %d channels and current channel is %d\n",
            minor, messageSlot -> size, messageSlot -> curr -> id);
    if(messageSlot -> size){
        int i;
        printk("Channels are: \n");
        Channel * b = messageSlot->head;
        while(b!=NULL){
            printk("%d \n",b->id);
            b=b->next;
        }
    }

    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release(struct inode *inode,
                          struct file *file) {
    printk("Invoking device_release(%p,%p)\n", inode, file);
    return SUCCESS;
}

//---------------------------------------------------------------
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t*  offset) {

    printk("Trying to write %s\n", buffer);

    // No channel was set.
    if (messageSlot -> size == 0){
        printk("write if1\n");
        return -EINVAL;
    }
    // Invalid length.
    if (!length || length > BUF_SIZE){
        return -EMSGSIZE;
    }

    int i;
    char* theMessage = messageSlot -> curr -> theMessage;
    printk("Invoking device_write(%p,%d)\n", file, length);
    strcpy(messageSlot -> curr -> theMessage,"");
    for( i = 0; i < length; ++i )
    {
        get_user(theMessage[i], &buffer[i]);
    }
    messageSlot -> curr -> messageLen = i;

    printk("#now on channel %d hold %s == %s\n", messageSlot -> curr -> id, buffer, messageSlot -> curr -> theMessage);
    printk("len of written message: %d\n", messageSlot -> curr -> messageLen);
    return i;
}

//---------------------------------------------------------------
static ssize_t device_read(struct file *file, char __user* buffer,size_t length, loff_t* offset ) {

    // No channel was set.
    if (messageSlot -> size == 0){
        return -EINVAL;
    }
    int i;
    int messageLen = messageSlot -> curr -> messageLen;
    char* theMessage = messageSlot -> curr -> theMessage;
    // No message.
    if (!messageLen){
        printk("read if1\n");
        return -EWOULDBLOCK;
    }
    // Buffer too small.
    if (length < messageSlot -> curr-> messageLen){
        printk("read if2\n");
        return -ENOSPC;
    }


    printk("Invoking device_read(%p,%d)\n", file, length);
    for( i = 0; i < messageLen; ++i )
    {
       put_user(theMessage[i], &buffer[i]);
    }

    printk("Read the message %s with len %d\n", theMessage, messageLen);

    // return the number of input characters used
    return i;
}


//----------------------------------------------------------------
static long device_ioctl(struct file *file,
                         unsigned int ioctlCommandId,
                         unsigned long channelId) {

    printk("Trying to ioctl to channel %d\n", channelId);
    if ((MSG_SLOT_CHANNEL != ioctlCommandId) || !channelId) {
        return -EINVAL;
    }
    // First channel made.
    if (messageSlot -> size == 0){
        messageSlot -> head -> id = channelId;
        messageSlot -> head -> messageLen = 0;
        messageSlot -> head -> next = NULL;
        messageSlot -> size ++;
        printk("ioctl if1\n");
        printk("Now on channel %d\n", messageSlot -> curr -> id);
        return SUCCESS;
    }
    // Not the first channel.
    else {
        // Stays on same channel.
        if (messageSlot -> curr -> id == channelId){
            printk("ioctl if2\n");
            printk("Now stays on channel %d\n", messageSlot -> curr -> id);
            return SUCCESS;
        }
        Channel* node = messageSlot -> head;
        // Looking for channelId.
        while (node -> next != NULL){
            // Found the relevant channel node.
            if (node -> id == channelId) {
                break;
            }
            node = node -> next;
        }

        // Repeating the check for the last node too
        if (node -> id == channelId) {
            printk("found the node with channelId %d\n", node->id);
            messageSlot->curr = node;
        }
        if(node -> id != channelId) {
            // No node with this channelId.
            printk("No node with this channelId.\n");
            Channel* newNode = (Channel *) kmalloc(sizeof(Channel), GFP_KERNEL);
            if (newNode == NULL){
                printk("ioctl if3\n");
                return -EINVAL;
            }
            newNode -> id = channelId;
            newNode -> next = NULL;
            newNode -> messageLen = 0;
            node -> next = newNode;
            messageSlot -> curr = newNode;
            messageSlot -> size ++;
        }
    }

    printk("Now on channel %d\n", messageSlot -> curr -> id);

    return SUCCESS;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
        {
                .read           = device_read,
                .write          = device_write,
                .open           = device_open,
                .unlocked_ioctl = device_ioctl,
                .release        = device_release,
        };

//---------------------------------------------------------------
static int __init simple_init(void) {
    int rc = -1;

    // Register driver capabilities. Obtain major num
    rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);

    // Negative values signify an error
    if (rc < 0) {
        printk(KERN_ERR "%s registraion failed for  %d\n", DEVICE_FILE_NAME, MAJOR_NUM );
        return rc;
    }

    devicesHead = (Devices *) kmalloc(sizeof(Devices),GFP_KERNEL);
    if(!devicesHead){
        return -EINVAL;
    }

    devicesHead -> slot = NULL;
    devicesHead -> next = NULL;
    devicesHead -> minor = -1;
    devicesCurr = devicesHead;

    printk("=============DEVICE INIT=============\n");
    printk(KERN_INFO "message_slot: registered major number %d\n", MAJOR_NUM);
    return SUCCESS;
}

static int freeAllChannels (Channel * chnl){
    if(chnl == NULL){
        return 0;
    }
    freeAllChannels(chnl -> next);
    kfree(chnl);
    return 0;
}


static int freeAllDevices (Devices * device){
    if(device == NULL){
        return 0;
    }
    printk("Now freeing device%d \n",device->minor);
    freeAllDevices(device -> next);
    if(device -> slot != NULL) {
        freeAllChannels(device->slot->head);
    }
    kfree(device);
    printk("device%d freed\n",device->minor);
    return 0;

}


//---------------------------------------------------------------
static void __exit

simple_cleanup(void) {
    // Unregister the device
    // Should always succeed
    freeAllDevices(devicesHead);
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
