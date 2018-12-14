//
// Created by eran on 12/14/18.
//

#include "message_reader.h"
#include "message_slot.h"
#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int main(int argc, char* argv[]) {
    int file_desc;
    int ret_val;
    char userMsg[BUF_SIZE];
    file_desc = open(argv[1], O_RDWR);
    if (file_desc < 0) {
        printf("Can't open device file: %s\n", argv[1]);
        exit(-1);
    }
    if (argc < 3) {
        printf("Not enough args\n");
        exit(-1);
    }

    ret_val = ioctl(file_desc, MSG_SLOT_CHANNEL, argv[2]);
    if(ret_val < 0){
        exit(ret_val);
    }

    ret_val =  read(file_desc, userMsg, BUF_SIZE);
    if(ret_val < 0){
        exit(ret_val);
    }

    printf("%.*s\n", ret_val,userMsg);

    close(file_desc);
    return 0;
}