### Message slot ###
Message slot is a mechanism for inter-process communication – Message Slot. A message slot is a character device file through which processes communicate using multiple message channels. A message slot device can have multiple channels active concurrently, which can be used by different processes.

Once a message slot device file is opened, a process uses ioctl() to specify the id of the message channel it wants to use. It subsequently uses read()/write() to receive/send messages on the channel. In contrast to pipes, a message slot preserves a message until it is overwritten; so the same message can be read multiple times.

A message slot supports a single command, named MSG_SLOT_CHANNEL. This command takes a single unsigned int parameter that specifies a non-zero channel id.

### Example session ###
1. Load (insmod) the message_slot.ko module.
2. Check the system log for the acquired major number.
3. Create a message slot file managed by message_slot.ko using mknod.
4. Change the message slot file’s permissions to make it readable and writable.
5. Invoke message_sender to send a message.
6. Invoke message_reader to receive the message.
7. Execute steps #5 and #6 several times, for different channels, in different sequences.

### Setup.sh script ###
Handles steps 1-3, assuming major number 242. Make sure to change path_to_dev_code variable to the path to your local code. 