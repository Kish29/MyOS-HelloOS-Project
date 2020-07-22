demo=loader.bin boot.bin

all: $(demo) 
loader.bin: loader.asm
	nasm loader.asm -o loader.bin

boot.bin: boot.asm 
	nasm boot.asm -o boot.bin 

.PHONY:clean

clean:
	rm -rf *.bin 
