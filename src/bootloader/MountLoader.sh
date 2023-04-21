#! /bin/bash 

dd if=boot.bin of=../../boot.img bs=512 count=1 conv=notrunc

# mount boot.img 
# -o loop describe the file as a valid disk partition.
mount -t vfat /root/Projects/HelloOS/boot.img /media/root/OSImage -o loop 

cp loader.bin /media/root/OSImage 
sync 

umount /media/root/OSImage 
