#undef __KERNEL__
#define __KERNEL__

#undef __MODULE__
#define __MODULE__

#include <linux/module.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "message_slot.h"

MODULE_LICENSE("GPL");
//==============================================CHANNEL LINKED LIST========================================
struct channel_node {
	unsigned int channelID;
	char* message;
	int length;
	struct channel_node *next;
} ;

//returns null if allocation failed
struct channel_node* create_channel_node(unsigned int channelNum) {
	struct channel_node *node;
	char* message;
	node = kmalloc(sizeof(struct channel_node), GFP_KERNEL);
	if (!node) {
		return NULL;
	}
	message = kmalloc(sizeof(MSG_LEN*sizeof(char)), GFP_KERNEL);
	if (!message) {
		kfree(node);
		return NULL;
	}
	node->channelID = channelNum;
	node->message = message;
	node->length = 0;
	return node;
}

//returns NULL is node doesn't exist, otherwise returns pointer to node
struct channel_node* search_for_channel(unsigned int channelNum, struct channel_node* head) {
	struct channel_node *curr;
	if (!head) { //list empty
		return NULL;
	}
	else { //list not empty
		curr = head;
		while(curr->next) { //not reached tail
			if (curr->channelID == channelNum) {
				return curr;
			}
			curr = curr->next;
		}
		if (curr->channelID == channelNum) { //special case for tail
			return curr;
		}
		return NULL;
	}
}

//returns NULL if allocation failed, is called after search
struct channel_node* add_channel(unsigned int channelNum, struct channel_node* head) {
	struct channel_node *curr, *new_node;
	curr = search_for_channel(channelNum, head);
	if (curr) { //if channel already exists, return a pointer to it
		return curr;
	}
	new_node = create_channel_node(channelNum);
	if (!new_node) {
		return NULL;
	}
	if (!head) {
		head = new_node;
	}
	else {
		curr = head;
		while (curr->next) { //get to tail
			curr = curr->next;
		}
		curr->next = new_node;
	}
	return new_node;
}

struct channel_node* free_channel_node(struct channel_node* node) {
	struct channel_node* next = node->next;
	kfree(node->message);
	kfree(node);
	return next;
}

void free_channel_list(struct channel_node* head) {
	struct channel_node* curr = head;
	while(curr) {
		curr = free_channel_node(curr);
	}
}

//============================================== SLOT LINKED LIST ==========================================

struct slot_node {
	int minor;
	struct channel_node *channels;
	struct slot_node* next;
} ;

struct slot_node* create_slot_node(int minor) {
	struct slot_node *node = kmalloc(sizeof(struct slot_node), GFP_KERNEL);
	if (!node) {
		return NULL;
	}
	node->minor = minor;
	node->channels = NULL;
	node->next = NULL;
	return node;
}

struct slot_node* search_for_slot(int minor, struct slot_node* head) {
	struct slot_node *curr;
	if (!head) { //list empty
		return NULL;
	}
	else { //list not empty
		curr = head;
		while(curr->next) { //not reached tail
			if (curr->minor == minor) {
				return curr;
			}
			curr = curr->next;
		}
		if (curr->minor == minor) { //special case for tail
			return curr;
		}
		return NULL;
	}
}

struct slot_node* add_slot(int minor, struct slot_node* head) {
	struct slot_node *curr;
	struct slot_node *new_node;
	curr = search_for_slot(minor, head);
	if (curr) { //If already exists in list, return the pointer from the list
		return curr;
	}
	new_node = create_slot_node(minor);
	if (!new_node) {
		return NULL;
	}
	if (!head) {
		head = new_node;
	}
	else {
		curr = head;
		while (curr->next) {
			curr = curr->next;
		}
		curr->next = new_node;
	}
	return new_node;
}

struct slot_node* free_slot_node(struct slot_node* node) {
	struct slot_node* next = node->next;
	free_channel_list(node->channels);
	kfree(node);
	return next;
}

void free_slot_list(struct slot_node* head) {
	struct slot_node* curr = head;
	while (curr) {
		curr = free_slot_node(curr);
	}
}


//================================================== DRIVER CODE ===========================================================

typedef struct driver_info {
	struct slot_node *slots_head;
} driver_info;

//START DEVICE CODE
static struct driver_info* dinfo;

//================================================= DEVICE FUNCTIONS =========================================================

//sets current minor and adds slot node to slot list
static int device_open(struct inode* inodep, struct file* filep) {
	struct slot_node *node;
	if (!inodep || !filep) { //check args
		return -EINVAL;
	}
	node = add_slot(iminor(inodep), dinfo->slots_head); //adds slot node, if already exists, returns a pointer to it 
	if (!node) { //memory allocation problem
		return -EIO;
	}
	if(!dinfo->slots_head) { //if list head hasn't been in
		dinfo->slots_head = node;
	}
	return SUCCESS;
}

static int device_release(struct inode* inodep, struct file* filep) {
	if (!inodep || !filep) { //check args
		return -EINVAL;
	}
	return SUCCESS;
}

static ssize_t device_read(struct file* filep, char __user* bufferp, size_t length, loff_t* offsetp) {
	int minor, i;
	struct channel_node* chan;
	struct slot_node* slot;
	if (!bufferp || !filep || !filep->private_data || (int) filep->private_data <= 0) { //check args
		return -EINVAL;
	}
	minor = iminor(file_inode(filep));
	if (minor < 0) { //shouldn't happen
		return -EIO;
	}
	slot = search_for_slot(minor, dinfo->slots_head);
	if (!slot) { //shouldn't happen, was created in device_open
		return -EIO;
	}
	chan = search_for_channel((int) filep->private_data , slot->channels);
	if (!chan) { //write hasn't been called before this read
		return -EWOULDBLOCK;
	}

	if (length < chan->length) { //buffer is too small for message
		return -ENOSPC;
	}
	for (i=0 ; i < chan->length ; i++ ) {
		if (put_user(chan->message[i] , &bufferp[i]) < 0) { //error in put user
			return -EIO;
		}
	}
	return i;
}

static ssize_t device_write(struct file* filep, const char __user* bufferp, size_t length, loff_t* offsetp) {
	int minor, i;
	char *new_message, *tmp;
	struct channel_node* chan;
	struct slot_node* slot;

	if (!bufferp || !filep || !filep->private_data ||  (int) filep->private_data <= 0) { //check args
		return -EINVAL;
	}
	else if (length <= 0 || length > MSG_LEN) { //invalid length to write
		return -EMSGSIZE;
	}
	else { //Everything is in order, prepare to write
		minor = iminor(file_inode(filep));
		if (minor < 0) { //shouldn't happen
			return -EIO;
		}
		slot = search_for_slot(minor, dinfo->slots_head);
		if (!slot) { //shouldn't happen, was created in device_open
			return -EIO;
		}
		chan = add_channel((int) filep->private_data, slot->channels); //if channel already exists already returns pointer
		if (!chan) { //memory alloc problem
			return -EIO;
		}
		if (!slot->channels) {
			slot->channels = chan;
		}
		new_message = kmalloc(MSG_LEN*sizeof(char), GFP_KERNEL);
		if (!new_message) { //memory alloc prob
			return -EIO;
		}
		for (i=0 ; i < length ; i++) {
			if (get_user(new_message[i] , &bufferp[i]) < 0) { //error in get_user
				kfree(new_message); 
				return -EIO;
			}
		}
		//write was successful, change pointers and release the old one
		tmp = chan->message;
		chan->message = new_message;
		kfree(tmp);

		chan->length = i; //save message length after writing
	}
	return i; //returns number of bytes written
}

//only saves slot, does not deal with initialization of linked list etc...
static long device_ioctl(struct file* filep, unsigned int ioctl_command_id, unsigned long ioctl_param) {
	if ((MSG_SLOT_CHANNEL == ioctl_command_id) && (ioctl_param > 0) && (filep)) { //check args
		filep->private_data = (void *) ioctl_param; //save the channel in private data
		return SUCCESS;
	}
	return -EINVAL; //args are invalid
}

//DEVICE SETUP
struct file_operations fops = {
	.open = device_open,
	.read = device_read,
	.write = device_write,
	.unlocked_ioctl = device_ioctl,
	.release = device_release,
} ; 


//=============================================== INIT AND CLEANUP ==========================================
static int __init simple_init(void) {
	int rc = -1;
	rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &fops);
	if (rc < 0) {
		printk(KERN_ERR "%s registration failed for %d\n", DEVICE_FILE_NAME, MAJOR_NUM);
		return rc;
	}
	dinfo = kmalloc(sizeof(struct driver_info), GFP_KERNEL);
	if (!dinfo) {
		printk(KERN_ERR "initialization of module failed\n");
		return -EIO;
	}
	dinfo->slots_head = NULL;
	printk(KERN_INFO "message slot: registration major number %d\n", MAJOR_NUM);
	return SUCCESS;
}

static void __exit simple_cleanup(void) {
	free_slot_list(dinfo->slots_head); //frees inner structs as well
	kfree(dinfo);
	unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//==========================================================================================================

module_init(simple_init);
module_exit(simple_cleanup);

//================================================ END OF FILE ===============================================
