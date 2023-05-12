#include "message_slot.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
    /*TODO: add error handling in all functions*/
    int fd, msg_length;
    char buffer[BUF_LEN];


    if (argc != 3){
        /*TODO: handle error*/
        exit(1);
    }

    /*TODO: make sure you need to open read only*/
    fd = open(argv[1], O_RDONLY);

    ioctl(fd, MSG_SLOT_CHANNEL, atoi(argv[2]));

    msg_length = read(fd, buffer, BUF_LEN);

    close(fd);

    write(STDOUT_FILENO, buffer, msg_length);


    /*TODO: it says in the instructions to exit the program
    with exit value 0. does returning 0 is good?*/
    return 0;
}


