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



static int getDeviceByMinor(int minor){

    devicesCurr = devicesHead;


    // Look for the minor.
    while (devicesCurr -> next != NULL) {
        if (devicesCurr->minor == minor) {
            break;
        }
        devicesCurr = devicesCurr->next;
    }

    // First time we see this minor number, or first node.
    if (devicesCurr -> minor != minor){
        return -EINVAL;
    }
    // We now have the relevant minor node.
    messageSlot = devicesCurr -> slot;
    return SUCCESS;
}



static int updateCurrentChannel(int channelId){
    Channel* node;

    // First channel made.
    if (messageSlot -> size == 0){
        return -EINVAL;
    }
        // Not the first channel.
    else {
        // Stays on same channel.
        if (messageSlot -> curr -> id == channelId){
            return SUCCESS;
        }
        node = messageSlot -> head;
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
            messageSlot->curr = node;
        }
       else {
            // No node with this channelId.
            return -EINVAL;
        }
    }

    return SUCCESS;
}


//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode *inode,
                       struct file *file) {

    int minor;
    Devices* node;
    Devices * newNode;
    devicesCurr = devicesHead;
    minor = iminor(inode);

    node = devicesHead;

    // Look for the minor.
    while (devicesCurr -> next != NULL) {
        if (devicesCurr->minor == minor) {
            break;
        }
        devicesCurr = devicesCurr->next;
    }

    // First time we see this minor number, or first node.
    if (devicesCurr -> minor != minor){
        newNode = (Devices *) kmalloc(sizeof(Devices), GFP_KERNEL);
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

    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release(struct inode *inode,
                          struct file *file) {
    return SUCCESS;
}

//---------------------------------------------------------------
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t*  offset) {

    int i;
    char* theMessage;


    if(getDeviceByMinor(iminor(file_inode(file))) < 0) {
        return -EINVAL;
    }

    if(updateCurrentChannel((int)(file -> private_data)) < 0) {
        return -EINVAL;
    }


    // No channel was set.
    if (messageSlot -> size == 0){
        return -EINVAL;
    }
    // Invalid length.
    if (!length || length > BUF_SIZE){
        return -EMSGSIZE;
    }

    theMessage = messageSlot -> curr -> theMessage;
    strcpy(messageSlot -> curr -> theMessage,"");
    for( i = 0; i < length; ++i )
    {
        get_user(theMessage[i], &buffer[i]);
    }
    messageSlot -> curr -> messageLen = i;

    return i;
}

//---------------------------------------------------------------
static ssize_t device_read(struct file *file, char __user* buffer,size_t length, loff_t* offset ) {
    int i, minor, channelId;
    int messageLen;
    char* theMessage;


    minor = iminor(file_inode(file));
    channelId = (int)(file -> private_data);
    if(getDeviceByMinor(minor) < 0) { // error
        return -EINVAL;
    }

    if(updateCurrentChannel(channelId) < 0) { // error
        return -EINVAL;
    }

    // No channel was set.
    if (messageSlot -> size == 0){
        return -EINVAL;
    }

    messageLen = messageSlot -> curr -> messageLen;
    theMessage = messageSlot -> curr -> theMessage;

    // No message.
    if (!messageLen){
        return -EWOULDBLOCK;
    }
    // Buffer too small.
    if (length < messageSlot -> curr-> messageLen){
        return -ENOSPC;
    }

    for( i = 0; i < messageLen; ++i )
    {
       put_user(theMessage[i], &buffer[i]);
    }

    // return the number of input characters used
    return i;
}


//----------------------------------------------------------------
static long device_ioctl(struct file *file,
                         unsigned int ioctlCommandId,
                         unsigned long channelId) {

    Channel* node;
    Channel* newNode;
    if ((MSG_SLOT_CHANNEL != ioctlCommandId) || !channelId) {
        return -EINVAL;
    }
    file -> private_data = (void*)channelId;
    // First channel made.
    if (messageSlot -> size == 0){
        messageSlot -> head -> id = (int) channelId;
        messageSlot -> head -> messageLen = 0;
        messageSlot -> head -> next = NULL;
        messageSlot -> size ++;
        return SUCCESS;
    }
    // Not the first channel.
    else {
        // Stays on same channel.
        if (messageSlot -> curr -> id == channelId){
            return SUCCESS;
        }
        node = messageSlot -> head;
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
            messageSlot->curr = node;
        }
        if(node -> id != channelId) {
            // No node with this channelId.
            newNode = (Channel *) kmalloc(sizeof(Channel), GFP_KERNEL);
            if (newNode == NULL){
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
    freeAllDevices(device -> next);
    if(device -> slot != NULL) {
        freeAllChannels(device->slot->head);
    }
    kfree(device);
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
