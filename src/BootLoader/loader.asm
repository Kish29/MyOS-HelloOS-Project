org 10000h
	jmp Label_Start 

;%include	"fat12.inc"
;
;BaseOfKernelFile equ 0x00 
;OffsetOfKernelFile equ 0x100000 
;
;BaseTmpOfKernelAddr equ 0x00 
;OffsetTmpOfKernelFile equ 0x7E00 
;
;MemoryStructBufferAddr equ 0x7E00 
;
;[SECTION gdt]
;
;LABEL_GDT:	dd 0, 0
;LABEL_DESC_CODE32:	dd 0x0000FFFF, 0x00CF9A00 
;LABEL_DESC_DATA32:	dd 0x0000FFFF, 0x00CF9200 
;
;
;
;
;
;
;
;
;
;
;[SECTION .s16]
;[BITS 16]
;

Label_Start:

    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ax, 0h
    mov ss, ax
    mov sp, 7c00h
; ====== 显示字符串
    mov ax, 1301h
    mov bx, 000bh
    mov dx, 021fh
    mov cx, StartLoaderMessageLength
    mov bp, StartLoaderMessage
    int 10h
    
; ====== open address A20
;	push ax 
;	in al, 92h
;	or al, 00000010b
;	out al, 92h
;
;	cli 
;	db 0x66 
;	lgdt [GdtPtr]
;	
;	mov eax,	cr0 
;	or eax, 1
;	mov cr0, eax 
;
;	mov ax, SelectorCode32 
;	mov fs, ax 
;	mov eax, cr0 
;	and al, 11111110b
;	mov cr0, eax
;
;	sti 

; ====== reset floppy 
	
	xor ah, ah 
	xor dl, dl
	int 13h


    jmp $
StartLoaderMessage: db 'Start Loader......'
StartLoaderMessageLength equ $ - StartLoaderMessage
