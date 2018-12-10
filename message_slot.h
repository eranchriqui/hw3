//
// Created by eran on 12/8/18.
//

#ifndef HW3_MESSAGE_SLOT_H
#define HW3_MESSAGE_SLOT_H

#define MAJOR_NUM 111
#define MSG_SLOT_CHANNEL _IOWR(MAJOR_NUM, 0, unsigned long)


#define DEVICE_RANGE_NAME "char_dev"
#define BUF_SIZE 128
#define DEVICE_FILE_NAME "message_slot_dev"
#define SUCCESS 0
#define FALSE 0
#define TRUE 1

#endif //HW3_MESSAGE_SLOT_H
