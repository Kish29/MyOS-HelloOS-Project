#! /bin/bash 

# dd if=boot.bin of=../../boot.img bs=512 count=1 conv=notrunc

# mount boot.img 
mount -t vfat /root/Projects/HelloOS/boot.img /media/root/OSImage -o loop # -o loop 把文件描述成磁盘分区

cp loader.bin kernel.bin /media/root/OSImage 
sync 

umount /media/root/OSImage 
