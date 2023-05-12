/*TODO: understand if the following is needed*/
#ifndef MSG_SLOT_H
#define MSG_SLOT_H

#include <linux/ioctl.h>

/*We are asked to hard code the major number of the device to 235*/
#define MAJOR_NUM 235

/*TODO: make sure this name is ok*/
#define DEVICE_NAME "message_slot"
#define SUCCESS 0

/*Setting the message of the device driver for setting channel*/
/*TODO: make sure if need to write unsigned int or unsigned long*/
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)

#define BUF_LEN 128


#endif



