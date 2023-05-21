/*=================MACROS AND INCLUDES=========================*/

#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h> /*For kernel work*/
#include <linux/module.h> /*Specifically, a module work*/
#include <linux/fs.h> /*for files*/
#include <linux/uaccess.h> /*for put/get user*/
#include <linux/slab.h> /*For kmalloc*/


#include "message_slot.h"


MODULE_LICENSE("GPL");


/*===================INITIALIZIONS=============================*/
/*Every message slot file will have a node and every channel will have a node.
The nodes of the message slot files will sit in a binary search tree whose keys are
the minor numbers. Every node message slot file node points to the root of a binary
search tree that contains all the channel nodes that are open in this file such
that the keys are the channel ids.*/

typedef struct channel_node
{
    unsigned int channel_id;
    char buffer[BUF_LEN];
    size_t length;
    struct channel_node* left;
    struct channel_node* right;
} channel_node;

typedef struct msg_slot_file_node 
{   
    unsigned int minor;
    channel_node* root;
    struct msg_slot_file_node* left;
    struct msg_slot_file_node* right;
} msg_slot_file_node;


/*The message slot files tree initialization to be empty*/
static msg_slot_file_node* msg_slot_files_root = NULL;

/*A temp buffer for the writes to be atomic*/

static char tmp_buffer[128];


/*=================== HELPER FUNCTIONS ========================*/

/*Given minor num, creates a new message slot device file node with this minor num
and returns a pointer to it. If an error occurs return NULL*/
static msg_slot_file_node* create_file_node(unsigned int minor_num)
{
    msg_slot_file_node* node;

    node = (msg_slot_file_node*) kmalloc(sizeof(msg_slot_file_node),GFP_KERNEL);
    if (node == NULL){
        /*kmalloc failed. returns NULL*/
        return NULL;
    }

    node -> minor = minor_num;
    node -> root = NULL;
    node -> left = NULL;
    node -> right = NULL;

    return node;
}


/*Given channel id, creates a channel node with this channel id
and returns a pointer to it. If an error occurs return NULL*/
static channel_node* create_channel_node(unsigned int channel_id)
{
    channel_node* node;

    node = (channel_node*) kmalloc(sizeof(channel_node),GFP_KERNEL);
    if (node == NULL){
        /*kmalloc failed. returns NULL*/
        return NULL;
    }

    node -> channel_id = channel_id;
    node -> length = 0;
    node -> left = NULL;
    node -> right = NULL;

    return node;
}



/*Given a minor number of a message slot device file which was already opened, returns
its correspondig tree node.*/
static msg_slot_file_node* find_file_node(unsigned minor_number)
{   
    unsigned int curr_minor;
    msg_slot_file_node* node = msg_slot_files_root;
    
    while (1){
        curr_minor = node -> minor;
        if (curr_minor == minor_number){
            return node;
        }
        if (minor_number < curr_minor){
            node = node -> left;
        }
        else {
            node = node -> right;
        }
    }
}


/*Given a root of channels tree and channel id, returns the node that corrsponds
to the channel with that id. If doesn't exist, create one, adds it to the tree
and returns it. Returns NULL if an error occurs.*/
static channel_node* get_channel_node(channel_node* root, unsigned int channel_id)
{

    channel_node *node, *new_node;
    unsigned int curr_node_id;

    if (root == NULL){
        /*The tree is empty*/
        new_node = create_channel_node(channel_id);
        if (new_node == NULL) {
            /*An error occured in create_channel_node. returns NULL*/
            return NULL;
        }
        return new_node;
    }

    node = root;
    while (1)
    {
        curr_node_id = node -> channel_id;
        if (curr_node_id == channel_id)
        {   
            /*The tree already contains a node with this channel id*/
            return node;
        }

        if (channel_id < curr_node_id)
        {
            if (node -> left == NULL){
                /*Creating a new node for the channel id and inserting it to the tree*/
                new_node = create_channel_node(channel_id);
                if (new_node == NULL) {
                        /*An error occured in create_channel_node. returns NULL*/
                        return NULL;
                    }
                node -> left = new_node;
                return new_node;   
            }
            /*continue searching in the left sub-tree*/
            node = node -> left;
        }
        else
        {
            if (node -> right == NULL){
                /*Creating a new node for the channel id and inserting it to the tree*/
                new_node = create_channel_node(channel_id);
                if (new_node == NULL) {
                        /*An error occured in create_channel_node. returns NULL*/
                        return NULL;
                    }
                node -> right = new_node;
                return new_node;   
            }

            /*continue searching in the right sub-tree*/
            node = node -> right;
        }
    
    }
}


/*Freeing a channels tree*/
static void free_channels_tree(channel_node* root)
{
    if (root != NULL){
        /*Freeing left sub-tree*/
        free_channels_tree(root -> left);
        /*Freeing right sub-tree*/
        free_channels_tree(root -> right);
        /*Freeing root*/
        kfree(root);
        }
}

/*Freeing device files tree*/
static void free_all(msg_slot_file_node* root)
{
    if (root != NULL){
        /*Freeing left sub-tree*/
        free_all(root -> left);
        /*Freeing right sub-tree*/
        free_all(root -> right);
        /*Freeing root*/
        free_channels_tree(root -> root);
        kfree(root);
        }
}

/*=================== DEVICE FUNCTIONS ========================*/

static int device_open(struct inode* inode, struct file* file)
{   
    msg_slot_file_node* node, *new_node;
    unsigned int curr_node_minor_num;
    unsigned int minor_num = iminor(inode);
    if (msg_slot_files_root == NULL){
        /*This is the first message slot device file opened*/
        msg_slot_files_root = create_file_node(minor_num);
        if (msg_slot_files_root == NULL){
            /*memory allocation to a new node failed*/
            return -ENOMEM;
        }
        return SUCCESS;
    }

    node = msg_slot_files_root;
    while(1){
        curr_node_minor_num = node -> minor;
        if (minor_num == curr_node_minor_num){
            /*This message slot device file was already opened*/
            return SUCCESS;
        }
        if (minor_num < curr_node_minor_num){
            if (node -> left == NULL){
                /*Creating a new node for the file and inserting it as the left son
                of the current node*/
                new_node = create_file_node(minor_num);
                if (new_node == NULL){
                    /*memory allocation to a new node failed*/
                    return -ENOMEM;
                }
                node -> left = new_node;
                return SUCCESS;
            }
            node = node -> left;
        }
        else {
            if (node -> right == NULL){
                /*Creating a new node for the file and inserting it as the right son
                of the current node*/
                new_node = create_file_node(minor_num);
                if (new_node == NULL){
                    /*memory allocation to a new node failed*/
                    return -ENOMEM;
                }
                node -> right = new_node;
                return SUCCESS;
            }
            node = node -> right;
        }
    }

}

/*----------------------------------------------------------------------------------*/
static long device_ioctl(   struct file* file,
                            unsigned int ioctl_command_id,
                            unsigned long ioctl_param)
{
    unsigned int minor_num, new_channel_id, curr_channel_id;
    msg_slot_file_node* f_node;
    channel_node* c_node;
    
    if (ioctl_command_id != MSG_SLOT_CHANNEL || ioctl_param == 0){
        /*Invaid arguments*/
        return -EINVAL;
    }
    
    /*we can assume that the channel id provided is less than 2^32*/
    new_channel_id = (unsigned int) ioctl_param;
    if (file -> private_data != NULL) {
        curr_channel_id =((channel_node*)(file -> private_data)) -> channel_id;
        if (new_channel_id == curr_channel_id){
            /*The struct file already points to the right channel*/
            return SUCCESS;
        }
    }
    
    minor_num = iminor(file -> f_inode);
    f_node = find_file_node(minor_num);
    c_node = get_channel_node(f_node -> root, new_channel_id);
    if (c_node == NULL){
        /*memory allocation to a new node failed*/
        return -ENOMEM;
    }

    if (f_node -> root == NULL)
    {
        /*This is the first channel opened in this file*/
        f_node -> root = c_node;
    }
    

    file -> private_data = c_node;
    return SUCCESS;
}

/*----------------------------------------------------------------------------------*/

static ssize_t device_read( struct  file* file,
                            char    __user* buffer,
                            size_t  length,
                            loff_t* offset)
{
    channel_node* node;
    size_t cur_msg_len;
    int i, ret_value;
    char* msg;

    if (file -> private_data == NULL){
        /*No channel has been set on the file descriptor*/
        return -EINVAL;
    }

    node = (channel_node*) file -> private_data;
    cur_msg_len = node -> length;
    if (cur_msg_len == 0){
        /*No message exists on the channel*/
        return -EWOULDBLOCK;
    }
    
    if (length < cur_msg_len){
        /*The provided buffer length is too small to hold the last message written
        on the channel*/
        return -ENOSPC;
    }

    msg = node -> buffer;

    /*Copying the message into the user's buufer*/
    for (i = 0; i < cur_msg_len; ++i){
        ret_value = put_user(msg[i], &buffer[i]);
        if (ret_value != 0){
            /*put_user failed*/
            return ret_value;
        }
    }

    return (ssize_t)cur_msg_len;
}


/*----------------------------------------------------------------------------------*/

static ssize_t device_write(    struct file*    file,
                                const char      __user* buffer,
                                size_t          length,
                                loff_t*         offset)
{   
    channel_node* node;
    int i, ret_value;

    if (file -> private_data == NULL){
        /*No channel has been set on the file descriptor*/
        return -EINVAL;
    }

    if (length == 0 || length > 128){
        /*Message length is invalid*/
        return -EMSGSIZE;
    }

    /*Copying message from user land to kernel space first*/
    for (i = 0; i < length; ++i){
        ret_value = get_user(tmp_buffer[i], &buffer[i]);
        if (ret_value != 0){
            /*get_user failed*/
            return ret_value;
        }
    }
    
    node = (channel_node*)file -> private_data;
    /*Only after completing copying from user space successfully, copying to the
    channel buffer*/
    for (i = 0; i < length; ++i){
        (node -> buffer)[i] = tmp_buffer[i];
    }

    /*Updating the length field in node*/
    node -> length = length;

    return (ssize_t)length;

}


/*=================== DEVICE SETUP ===========================*/

/*Assigning the "file_opertations struct" with the device implementation
to the functions*/

struct file_operations Fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .unlocked_ioctl = device_ioctl
};


/*Intializing the device driver module - registering as a character device driver*/

static int __init init_driver(void)
{
    int rc;

    /*Registering as a charcter device driver*/
    rc = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);

    if (rc < 0){
        /*Failed registering the driver*/
        printk(KERN_ERR "registraion faild for %s device\n", DEVICE_NAME);
        return rc;
    }
    
    return SUCCESS;    
}


/*Unloader for device driver*/

static void __exit close_driver(void)
{
    /*Unregister the device*/
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);

    /*Freeing memory*/
    free_all(msg_slot_files_root);
}

/*Defining the macros for loading and unloading of the module*/
module_init(init_driver);
module_exit(close_driver);

/*=============================== THE END =========================================*/



