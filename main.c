#include "message_slot.h"
#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char* argv[]) {
    int file_desc;
    int ret_val;
    char userMsg2[BUF_SIZE];
    char userMsg3[BUF_SIZE];
    file_desc = open(argv[1], O_RDWR );
    if( file_desc < 0 )
    {
        printf ("Can't open device file: %s\n",argv[1]);
        exit(-1);
    }

    if(argc < 3){
        printf ("Not enough args\n");
        exit(-1);
    }
/*
    ret_val = ioctl( file_desc, MSG_SLOT_CHANNEL, 3);
    ret_val = read( file_desc, userMsg3, strlen(argv[3]));
    printf("%s\n",userMsg3);
    ret_val = ioctl( file_desc, MSG_SLOT_CHANNEL, 2);
    ret_val = read(  file_desc, userMsg2, strlen(argv[2]));
    printf("%s\n",userMsg2);*/

    ret_val = ioctl( file_desc, MSG_SLOT_CHANNEL, 2);
    ret_val = write( file_desc, argv[2] , strlen(argv[2]));
    ret_val = ioctl( file_desc, MSG_SLOT_CHANNEL, 3);
    ret_val = write( file_desc, argv[3], strlen(argv[3]));
    ret_val = read(  file_desc, userMsg3, strlen(argv[3]));
    printf("%s\n",userMsg3);
    ret_val = ioctl( file_desc, MSG_SLOT_CHANNEL, 2);
    ret_val = read(  file_desc, userMsg2, strlen(argv[2]));
    printf("%s\n",userMsg2);

    close(file_desc);
    return 0;
}