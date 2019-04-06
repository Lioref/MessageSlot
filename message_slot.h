#ifndef MSGSLOT_H
#define MSGSLOT_H

#include <linux/ioctl.h>

#define MAJOR_NUM 242

#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)

#define DEVICE_RANGE_NAME "msg_char_dev"
#define MSG_LEN 128
#define DEVICE_FILE_NAME "msgdev"
#define SUCCESS 0

#endif
