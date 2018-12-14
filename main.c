#include "message_slot.h"
#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char* argv[]) {
    int file_desc;
    int ret_val;
    char userMsg[BUF_SIZE];
    file_desc = open(argv[1], O_RDWR );
    if( file_desc < 0 )
    {
        printf ("Can't open device file: %s\n",argv[1]);
        exit(-1);
    }

    ret_val = ioctl( file_desc, MSG_SLOT_CHANNEL, 3);
    ret_val = read(  file_desc, userMsg, 5 );
    printf("%s\n",userMsg);
    ret_val = ioctl( file_desc, MSG_SLOT_CHANNEL, 2);
    ret_val = read(  file_desc, userMsg, 5 );
    printf("%s\n",userMsg);

    /*ret_val = ioctl( file_desc, MSG_SLOT_CHANNEL, 3);
    ret_val = write( file_desc, "Hello", 5);
    ret_val = ioctl( file_desc, MSG_SLOT_CHANNEL, 2);
    ret_val = write( file_desc, "abcde", 5);
    ret_val = read(  file_desc, userMsg, 5 );
    printf("%s\n",userMsg);
    ret_val = ioctl( file_desc, MSG_SLOT_CHANNEL, 3);
    ret_val = read(  file_desc, userMsg, 5 );
    printf("%s\n",userMsg);*/
    close(file_desc);
    return 0;
}