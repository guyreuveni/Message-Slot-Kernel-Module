#include "message_slot.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[])
{
    /*TODO: add error handling in all functions*/
    int fd;
    char* msg;


    if (argc != 4){
        /*TODO: handle error*/
        exit(1);
    }

    /*TODO: make sure you need to open write only*/
    fd = open(argv[1], O_WRONLY);

    ioctl(fd, MSG_SLOT_CHANNEL, atoi(argv[2]));

    msg = argv[3];

    write(fd, msg, strlen(msg));

    close(fd);

    /*TODO: it says in the instructions to exit the program
    with exit value 0. does returning 0 is good?*/
    return 0;
}



