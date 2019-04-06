#include <assert.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "message_slot.h"

void printError(char *errorLocation) {
	printf("message_reader: error in %s\n", errorLocation);
}

int main(int argc, char **argv) {
	if(argc < 4) {
		printError("num of args");
		return -1;
	}

	int flag, bytes_written;
	char *fpath = argv[1];
	int channelID = atoi(argv[2]);
	char *message = argv[3];
	int message_length = strlen(message);

	int fd = open(fpath, O_WRONLY);
	if (fd < 0) {
		printError("open");
		return -1;
	}
	//Set channel
	flag = ioctl(fd, MSG_SLOT_CHANNEL, channelID);
	if (flag) {
		printError("ioctl");
		return -1;
	}

	//Write to message slot channel
	bytes_written = write(fd, message, message_length);
	if (bytes_written < 0) {
		printError("write");
		return -1;
	}
	flag = close(fd);
	if (flag) {
		printError("close");
		return -1;
	}
	printf("Written %i bytes to device\n", bytes_written);
	return 0;
}