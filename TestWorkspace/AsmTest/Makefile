all: cpuid

.PHONY: clean

cpuid.o: cpuid.asm
	nasm -f elf64 cpuid.asm 

cpuid: cpuid.o 
	ld -s cpuid.o -o cpuid 
	chmod +x cpuid

clean:
	rm -rf *.o cpuid 
