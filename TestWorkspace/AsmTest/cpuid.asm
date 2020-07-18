section .data
	msg: db "hello! my name is jar", 0ah
	msg_len equ $ - msg 
	cpuid_msg: times 0x80 db 0

section .text 
	global _start

_start:
	mov eax, 4
	mov ebx, 1 
	mov ecx, msg 
	mov edx, msg_len 
	int 80h 

	mov eax, 0
	cpuid

res:
	and eax, 0x000000ff
	mov esi, eax 
	shr eax, 4
	cmp eax, 9
	ja r1
	add eax, '0'
	jmp r2

r1:
	sub eax, 0xa 
	add eax, 'a'

r2:
	mov dword [cpuid_msg], eax 

	mov eax, esi 
	and eax, 0x0000000f
	cmp eax, 9
	ja r3
	add eax, '0'
	jmp r4

r3:
	sub eax, 0xa 
	add eax, 'a'

r4:
	mov dword [cpuid_msg+4], eax

	mov [cpuid_msg+8], ebx 
	mov [cpuid_msg+0xc], edx 
	mov [cpuid_msg+0x10], ecx 
	mov eax, 0xa
	mov [cpuid_msg+0x14], eax

	mov eax, 4
	mov ebx, 1
	mov ecx, cpuid_msg 
	mov edx, 0x15
	int 80h

	mov eax, 1 
	mov ebx, 0
	int 80h
