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
	if(argc < 3) {
		printError("num of args");
		return -1;
	}

	int flag, bytes_read;
	char *fpath = argv[1];
	int channelID = atoi(argv[2]);
	
	char *buffer;
	buffer = malloc(MSG_LEN*sizeof(char));
	if (!buffer) {
		printError("malloc");
		return -1;
	}

	//open file
	int fd = open(fpath, O_RDONLY);
	if (fd < 0) {
		printError("open");
		return -1;
	}
	//set channel
	flag = ioctl(fd, MSG_SLOT_CHANNEL, channelID);
	if (flag) {
		printError("ioctl");
		return -1;
	}

	//Read from message slot channel
	bytes_read = read(fd, buffer, MSG_LEN);
	if (bytes_read < 0) {
		printError("read");
		return -1;
	}
	flag = close(fd);
	if (flag) {
		printError("close");
		return -1;
	}
	printf("%s\n", buffer);
	printf("Successfully read %i bytes from device\n" , bytes_read);
	free(buffer);
	return 0;
}
