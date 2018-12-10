//
// Created by eran on 12/8/18.
//

#ifndef HW3_MESSAGE_SLOT_H
#define HW3_MESSAGE_SLOT_H
#include <linux/ioctl.h>

#define MAJOR_NUM 555
#define MSG_SLOT_CHANNEL _IORW(MAJOR_NUM, 0, unsigned long)


#define DEVICE_RANGE_NAME "char_dev"
#define BUF_SIZE 128
#define CHANNELS 4
#define DEVICE_FILE_NAME "message_slot_dev"
#define SUCCESS 0
#define NULL 0
#define FALSE 0
#define TRUE 1

#endif //HW3_MESSAGE_SLOT_H



// The major device number.
// We don't rely on dynamic registration
// any more. We want ioctls to know this
// number at compile time.

// Set the message of the device driver


#endif