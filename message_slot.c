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


struct message_slot {
    char buffers[CHANNELS][BUF_SIZE];
    int index;
};

typedef struct node {
    int open;
    int id;
    struct message_slot data;
    struct node* next;
} node_t;


typedef struct list
{
    void* head;
    size_t size;
} list;

typedef struct MsgSlotNode
{
    int id;
    list* channel_list;
    struct MsgSlotNode* next;
} MsgSlotNode;

typedef struct ChannelNode
{
    int channel_id;
    char buffer[BUF_SIZE];
    ssize_t msg_len;
    struct ChannelNode* next;
} ChannelNode;





static node_t* head = NULL;



// used to prevent concurent access into the same device
static int dev_open_flag = 0;

static struct chardev_info device_info;

// The message the device will give when asked
static char the_message[BUF_LEN];

//Do we need to encrypt?
static int encryption_flag = 0;

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
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read(struct file *file, char __user* buffer,size_t length, loff_t* offset )
{
    // read doesnt really do anything (for now)
    printk( "Invocing device_read(%p,%d) - "
            "operation not supported yet\n"
            "(last written - %s)\n",
            file, length, the_message );
    //invalid argument error
    return -EINVAL;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t*  offset) {

    int i;
    printk("Invoking device_write(%p,%d)\n", file, length);
    for( i = 0; i < length && i < BUF_LEN; ++i )
    {
        get_user(the_message[i], &buffer[i]);
        if( 1 == encryption_flag )
            the_message[i] += 1;
    }

    // return the number of input characters used
    return i;
}


//----------------------------------------------------------------
static long device_ioctl(struct file *file,
                         unsigned int ioctl_command_id,
                         unsigned int channel_id) {


    // Switch according to the ioctl called
    if (MSG_SLOT_CHANNEL == ioctl_command_id) {
        // Get the parameter given to ioctl by the process
        ChannelNode current = head;
        while (current -> next != NULL){
            current = current -> next;
        }
        current -> next = kmalloc(sizeof(ChannelNode), GFP_KERNEL);

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

    printk(KERN_INFO "message_slot: registered major number %d\n", rc);
    return 0;
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