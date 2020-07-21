org 0x100000

	jmp $
;	mov ax, cs
;	mov ds, ax 
;	mov es, ax 
;	mov ss, ax 
;	mov ebp, 0x200000
;
;	mov ax, 1301h 
;	mov bx, 000ah
;	mov dx, 051fh
;	mov cx, mesaage_length 
;	mov bp, mesaage 
;	int 10h 
;
;	jmp $
;
;
;
;mesaage: db 'this is kernel test'
;mesaage_length equ $ - mesaage
