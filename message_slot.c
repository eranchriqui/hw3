// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include "message_slot.h"

MODULE_LICENSE("GPL");


typedef struct MessageSlot
{
    file* desc;
    Channel* head;
    Channel* curr;
    size_t size;
} MessageSlot;

typedef struct Channel
{
    int id;
    char theMessage[BUF_SIZE];
    int messageLen;
    struct Channel* next;
} Channel;

static MessageSlot* messageSlot;
static struct chardev_info device_info;


//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode *inode,
                       struct file *file) {

    unsigned long flags; // for spinlock
    printk("Invoking device_open(%p)\n", file);

    // We don't want to talk to two processes at the same time
    spin_lock_irqsave(&device_info.lock, flags);
    if (1 == dev_open_flag) {
        spin_unlock_irqrestore(&device_info.lock, flags);
        return -EBUSY;
    }

    ++dev_open_flag;
    spin_unlock_irqrestore(&device_info.lock, flags);
    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release(struct inode *inode,
                          struct file *file) {
    unsigned long flags; // for spinlock
    printk("Invoking device_release(%p,%p)\n", inode, file);

    // ready for our next caller
    spin_lock_irqsave(&device_info.lock, flags);
    --dev_open_flag;
    spin_unlock_irqrestore(&device_info.lock, flags);
    return SUCCESS;
}

//---------------------------------------------------------------
static ssize_t device_read(struct file *file, char __user* buffer,size_t length, loff_t* offset )
{
    // No channel was set.
    if (messageSlot -> size == 0){
        return -EINVAL;
    }
    // No message.
    if (!messageSlot -> curr -> messageLen){
        return -EWOULDBLOCK;
    }
    // Buffer too small.
    if (length < messageSlot -> curr-> messageLen){
        return -ENOSPC;
    }

    int i;
    char* theMessage = messageSlot -> curr -> theMessage;
    printk("Invoking device_write(%p,%d)\n", file, length);
    for( i = 0; i < length; ++i )
    {
        get_user(theMessage[i], &buffer[i]);
    }
    messageSlot -> curr -> messageLen = i;


    return i;
}

//---------------------------------------------------------------
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t*  offset) {

    // No channel was set.
    if (messageSlot -> size == 0){
        return -EINVAL;
    }
    // Invalid length.
    if (!length || length > BUF_SIZE){
        return -EMSGSIZE;
    }

    int i;
    char* theMessage = messageSlot -> curr -> theMessage;
    int messageLen = messageSlot -> curr -> messageLen;
    printk("Invoking device_read(%p,%d)\n", file, length);
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
    if (MSG_SLOT_CHANNEL != ioctlCommandId || !channelId) {
        return -EINVAL;
    }
    // First channel made.
    if (messageSlot -> size == 0){
        messageSlot -> head -> id = channelId;
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
        Channel* node = messageSlot -> head;
        // Looking for channelId.
        while (node -> next != NULL){
            // Found the relevant channel node.
            if (node -> id == channelId) {
                messageSlot->curr = node;
                break;
            }
            node = node -> next;
        }
        if(node -> next == NULL) {
            // No node with this channelId.
            Channel* newNode = (Channel *) kmalloc(sizeof(Channel), GFP_KERNEL);
            if (!newNode){
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
    int majorNumber = -1;

    // Register driver capabilities. Obtain major num
    majorNumber = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);

    // Negative values signify an error
    if (majorNumber < 0) {
        printk(KERN_ERR "%s registraion failed for  %d\n", DEVICE_FILE_NAME, MAJOR_NUM );
        return majorNumber;
    }

    messageSlot = (MessageSlot *) kmalloc(sizeof(MessageSlot), GFP_KERNEL);
    if (!messageSlot){
        return -EINVAL;
    }
    messageSlot -> head = (Channel *) kmalloc(sizeof(Channel), GFP_KERNEL);
    if (!messageSlot -> head){
        return -EINVAL;
    }
    messageSlot -> curr = head;
    messageSlot -> size = 0;

    printk(KERN_INFO "message_slot: registered major number %d\n", majorNumber);
    return SUCCESS;
}



//---------------------------------------------------------------
static void __exit

simple_cleanup(void) {
    // Unregister the device
    // Should always succeed
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================