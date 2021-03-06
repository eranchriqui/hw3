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
#include "message_slot.h"
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/



typedef struct Channel
{
    int id;
    char theMessage[BUF_SIZE];
    int messageLen;
    struct Channel* next;
} Channel;

typedef struct MessageSlot
{
    Channel* head;
    Channel* curr;
    size_t size;
} MessageSlot;


typedef struct Devices
{
    MessageSlot *slot;
    int minor;
    struct Devices* next;

} Devices;


#endif //HW3_MESSAGE_SLOT_H
