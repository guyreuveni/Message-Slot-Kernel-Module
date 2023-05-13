#include "message_slot.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char const *argv[])
{
    int fd;

    if (argc != 4){
        /*Invalid ammount of arguments*/
        perror(strerror(EINVAL));
        exit(1);
    }

    fd = open(argv[1], O_WRONLY);
    if (fd < 0)
    {
        /*open failed*/
        perror(strerror(errno));
        exit(1);
    }

    if (ioctl(fd, MSG_SLOT_CHANNEL, atoi(argv[2])) < 0){
        /*ioctl failed*/
        perror(strerror(errno));
        exit(1);
    }

    if (write(fd, argv[3], strlen(argv[3])) < 0){
        /*write failed*/
        perror(strerror(errno));
        exit(1);
    }

    if (close(fd) < 0){
        /*close failed*/
        perror(strerror(errno));
        exit(1);
    }

    /*Exiting with zero in case of success*/
    exit(0);
}



