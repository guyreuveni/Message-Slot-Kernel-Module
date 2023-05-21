#include "message_slot.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(int argc, char const *argv[])
{
    int fd, msg_length;
    char buffer[BUF_LEN];


    if (argc != 3){
        /*Invalid ammount of arguments*/
        perror("Invalid ammount of arguments");
        exit(1);
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        /*open failed*/
        perror("Open failed");
        exit(1);
    }
    

    if (ioctl(fd, MSG_SLOT_CHANNEL, atoi(argv[2])) < 0){
        /*ioctl failed*/
        perror("ioctl failed");
        exit(1);
    }

    msg_length = read(fd, buffer, BUF_LEN);

    if (msg_length < 0){
        /*read failed*/
        perror("read failed");
        exit(1);
    }

    if (close(fd) < 0){
        /*close failed*/
        perror("close failed");
        exit(1);
    }

    if (write(STDOUT_FILENO, buffer, msg_length) < 0){
        /*write failed*/
        perror("write failed");
        exit(1);
    }
    
    /*Exiting with zero in case of success*/
    exit(0);
}


