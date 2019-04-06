#!/bin/bash 

path_to_dev_code=/home/charDev #path to local code
device_file_name=mydev
mod_name=chardev

#Remove and clean old module
rmmod $mod_name.ko
cd $path_to_dev_code
make clean
rm /dev/$device_file_name

#Build and insert new module
cd $path_to_dev_code
make
insmod $mod_name.ko
dmesg
mknod /dev/mydev c 242 0
chmod o+rw /dev/mydev