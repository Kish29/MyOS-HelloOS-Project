#! /bin/bash 

# mount kernel.bin  
mount -t vfat /root/Projects/HelloOS/boot.img /media/root/OSImage -o loop # -o loop 把文件描述成磁盘分区

cp kernel.bin /media/root/OSImage 
sync 

umount /media/root/OSImage 
