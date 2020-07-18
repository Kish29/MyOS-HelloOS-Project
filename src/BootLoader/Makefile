demo=loader.bin boot.bin kernel.bin 

all: $(demo) 
loader.bin: loader.asm
	nasm loader.asm -o loader.bin

boot.bin: boot.asm 
	nasm boot.asm -o boot.bin 

kernel.bin: kernel.asm 
	nasm kernel.asm -o kernel.bin 

.PHONY:clean

clean:
	rm -rf *.bin 
