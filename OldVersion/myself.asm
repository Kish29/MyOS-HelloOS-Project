org 0x7c00 ;注意，org伪指令告诉编译器，这段代码的起始地址是0x7c00，
;如果不设置话，编译期默认这段程序的起始地址是0 
;获取你自己定义的数据地址（比如字符串数据）就是获取的相对于这个程序开始的地址
;比如一个message相对于这个程序的偏移地址是0x25
;当你 mov ax, message的时候，不加org指令的情况下，编译期会把这个指令翻译成
;==== mov ax, 0x25 ===========
;而当你加上org 0x7c00的时候，编译器就翻译成了
;==== mov ax, 0x7c25 =========
;而我们的这个boot程序要放到内存0x7c00处执行，
;注意：！！！这里的0x7c00是rip寄存器的值，不是cs的值，在bochs运行的时候加上断点会看到
;cs的值其实是0，意思就是这个程序是在0x0000:0x7c00处执行的，所以这个org 0x7c00必须加上
;所以如果不设置话，如果你调用bios 10h的13h中断，es:bp = 0:0x25是找不到字符串的




BaseOfStack equ 0x7c00
BaseOfLoader equ 0x1000
OffsetOfLoader equ 0x00

SectorsOfRootDir equ 14 ; 已知根目录总的文件数是224，那么224 × 32=7168就是根目录占用的总字节大小
; 224 x 32 / 512 =  14 就是根目录占用的扇区数量
SectorNumOfRootDirStart equ 19
SectorNumOfFAT1Start equ 1
SectorBanlance equ 17

    jmp short BootStartInitialize     ; 指令执行从BootStartInitialize开始
    nop
    BS_OEMName	db	'jarsboot'
    BPB_BytesPerSec	dw	512
    BPB_SecPerClus	db	1   ;数据区的没簇占多少扇区
    BPB_RsvdSecCnt	dw	1
    BPB_NumFATs	db	2
    BPB_RootEntCnt	dw	224  ; 根目录总的文件数目，这个用来确定根目录区占用的扇区数量
    ; 因为根目录占用14个扇区，每个扇区512个字节，而且目录项每个占用32Bytes
    ; 那么一个扇区就有512/32=16个文件，14个扇区就有224个文件
    BPB_TotSec16	dw	2880
    BPB_Media	db	0xf0  ; 可移动磁盘的类型F0，不可移动F8
    BPB_FATSz16	dw	9   ; 每个fat表占用的扇区数
    BPB_SecPerTrk	dw	18  ; 每个磁道的扇区数
    BPB_NumHeads	dw	2
    BPB_HiddSec	dd	0
    BPB_TotSec32	dd	0
    BS_DrvNum	db	0
    BS_Reserved1	db	0
    BS_BootSig	db	0x29
    BS_VolID	dd	0
    BS_VolLab	db	'boot loader'
    BS_FileSysType	db	'FAT12   '

BootStartInitialize:
; initialize
    mov ax, cs
    mov es, ax
    mov ds, ax
    mov ss, ax
    mov sp, BaseOfStack 


; clear sreen
    mov ax, 0600h ; 06h号中断屏幕上卷
    mov bx, 0200h ; bh=上卷后屏幕中的属性
    mov cx, 0 ; 左上角行列坐标
    mov dx, 184fh ; 右下角行列坐标
    int 10h
; set cursor
    mov ax, 0200h ; 02h中断设置光标位置
    mov bx, 0h ; bh=显示页码
    mov dx, 0h ; dx=行列号
    int 10h
; display message
    mov ax, ds 
	mov es, ax 
	mov bx, 000dh
	mov bp, StartBootMessage  ; 13h中断显示字符串 es:bp是字符串地址
    mov cx, StartBootMessageLength 
	mov dx, 0020h ; 显示字符串的位置，这里是0行32列
	mov ax, 1301h ; al=01显示后光标移到字符串后面
	int 10h
; initial floppy
	xor ax, ax 
	xor dx, dx 
	int 13h

; 这里开始每次读取1个根目录扇区的数据到缓冲区0：0x8000中，一共读取14次
Read_Sector_In_Root_Dir_For_Searching_Loader:
	cmp byte [ReadSectorsOfRootDirNum], 0
	je Loader_Have_Not_Found
	dec byte [ReadSectorsOfRootDirNum]
	; 由于我们要调用int 13h的2号中断来读取扇区的数据
	; 但是这里我知道逻辑扇区，所以要转换
	mov ax, 0
	mov es, ax 
	mov ax, [ReadCurrentSectorInRootDir] 
	mov bx, 8000h ; 缓冲区地址
	mov cx, 1
	call Read_One_Sector_From_LBA_To_CHS
	; 将一个扇区的数据读到缓冲区中后，开始从缓冲区0:8000h处查找loader
	
	mov di, 8000h
	cld ; 设置正向 
	mov dx, 10h ; 每个扇区查找16次，因为一个扇区16个文件 

Go_on_read_next_file:
	cmp dx, 0
	je Go_On_Read_Next_Sector  
	dec dx 
	mov si, LoaderFileName ; 接下来用lodsb来传输11次字节数据
	mov cx, 11 ; 比较11个字节的数据

Read_loader_name_entry:
	cmp cx, 0
	je ShowLoaderFoundMessage ; 找到了先显示一串字符串
	dec cx 
	lodsb
	cmp al, [es:di]
	je Go_on_compare_next_char
	; 注意这个di加32个字节的算法
	and di, 0ffe0h
	add di, 20h
	jmp short Go_on_read_next_file 
Go_on_compare_next_char:

	inc di
	jmp short Read_loader_name_entry 

Go_On_Read_Next_Sector:
	add word [ReadCurrentSectorInRootDir], 1
	jmp Read_Sector_In_Root_Dir_For_Searching_Loader

Loader_Have_Not_Found:
	mov ax, ds 
	mov es, ax 
	mov bx, 008eh
	mov bp, NoLoaderFoundMessage
	mov cx, NoLoaderFoundMessageLength
	mov dx, 011ch 
	mov ax, 1301h
	int 10h
	jmp $

ShowLoaderFoundMessage:
	mov ax, ds 
	mov es, ax 
	mov bx, 000ch
	mov bp, LoaderFoundMessage 
	mov cx, LoaderFoundMessageLength 
	mov dx, 0121h 
	mov ax, 1301h
	int 10h

Get_First_Info_In_FAT_Tab:
	mov ax, 0
	mov es, ax
	and di, 0ffe0h 
	add di, 1ah
	mov ax, [es:di] 
	push ax 

Set_Memory_Address:
	mov ax, BaseOfLoader 
	mov es, ax 
	mov bx, OffsetOfLoader 
	
	pop ax 

Get_Info_From_FAT_Tab:
	push ax 
	add ax, SectorsOfRootDir 
	add ax, SectorBanlance 
	mov cx, 1
	call Read_One_Sector_From_LBA_To_CHS 
	pop ax 
	call Get_Value_From_FAT_Tab
	cmp ax, 0fffh
	je Loader_has_been_loaded
	add bx, [BPB_BytesPerSec]
	jmp short Get_Info_From_FAT_Tab 

Loader_has_been_loaded:
	jmp BaseOfLoader:OffsetOfLoader 


Get_Value_From_FAT_Tab:
	push es 
	push bx 
	push ax 
	
	mov ax, 0
	mov es, ax 

	pop ax 
	mov byte [Fat_Index_Odd_Or_Even], 0
	mov bx, 3
	mul bx 
	mov bx, 2
	div bx
	cmp dx, 0
	je Fat_Index_Even 
	mov byte [Fat_Index_Odd_Or_Even], 1 

Fat_Index_Even:
	mov bx, [BPB_BytesPerSec]
	xor dx, dx ; 注意这里一定要对dx置零，因为下一条除法指令是16位的，默认dx是高16位，ax是低16位，而上面的运算完成后，dx=1，那么接下来的除法就会出错
	div bx 
	add ax, SectorNumOfFAT1Start 
	push dx 
	mov bx, 8000h
	mov cx, 1
	call Read_One_Sector_From_LBA_To_CHS 
	pop dx 
	add bx, dx 
	mov ax, [es:bx]
	cmp byte [Fat_Index_Odd_Or_Even], 0
	je Index_Even 
	shr ax, 4

Index_Even:
	and ax, 0fffh 
	pop bx 
	pop es 
	ret 

Read_One_Sector_From_LBA_To_CHS:
	push bp 
	mov bp, sp 
	sub sp, 4
	mov byte [bp-2], cl
	mov word [bp-4], bx 
	mov bl, byte [BPB_SecPerTrk]
	div bl
	inc ah 
	mov cl, ah
	mov dh, al 
	mov ch, al 
	shr ch, 1
	and dh, 1 ; 立即数会自动拓展位 
	mov dl, [BS_DrvNum]
	mov bx, [bp-4]
	mov al, [bp-2]
Go_On_Read_Sector:
	mov ah, 02h
	int 13h
	jc Go_On_Read_Sector 
	add sp, 4
	pop bp 
	ret 

; ====== 要用到的变量
ReadCurrentSectorInRootDir: dw SectorNumOfRootDirStart ;注意这里的dw字数据，一定要是字数据，如果是db数据，那么要在在传参时对ah置零
ReadSectorsOfRootDirNum: db SectorsOfRootDir 
LoaderFileName: db 'LOADER  BIN' ; 一共11个字节，最后三个字节是文件拓展名
Fat_Index_Odd_Or_Even: db 0

;==========messages 
StartBootMessage: db 'Start booting...'
StartBootMessageLength equ $ - StartBootMessage
NoLoaderFoundMessage: db 'ERROR: NO LOADER FOUND!'
NoLoaderFoundMessageLength equ $ - NoLoaderFoundMessage 
LoaderFoundMessage: db 'Loader Found!'
LoaderFoundMessageLength equ $ - LoaderFoundMessage 

	times 510 - ($ -$$) db 0
	db 0x55
	db 0xaa ; 后面的字数据是主引导签名(MBR)
