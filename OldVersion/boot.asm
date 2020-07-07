org 0x7c00

BaseOfStack equ 0x7c00

BaseOfLoader equ 0x1000
OffsetOfLoader equ 0x0
 
RootDirSectors equ 14
SectorNumOfRootDirStart equ 19
SectorNumOfFAT1Start equ 1
SectorBanlance equ 17

    jmp short BootStartInitialize     ; 指令执行从BootStartInitialize开始
    nop
    BS_OEMName	db	'jarsboot'
    BPB_BytesPerSec	dw	512
    BPB_SecPerClus	db	1   ;数据区的每簇占多少扇区
    BPB_RsvdSecCnt	dw	1
    BPB_NumFATs	db	2
    BPB_RootEntCnt	dw	224  ; 根目录总的文件数目
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
    ; 段寄存器的初始化
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax  ; 数据段、es、栈段都是这段程序的段地址
    mov sp, BaseOfStack ; 0x7c00减去2后为0x7bfe,是一个很大的栈空间

;========== 先调用bios int10h的6h号中断来清空屏幕中的所有内容
    mov ax, 0600h
	mov bx, 0700h
	mov cx, 0
	mov dx, 0184fh ;第24行，79列 
; =========06H
;屏幕初始化或上卷
;AL = 上卷行数
;AL = 0全屏幕为空白
;BH = 卷入行属性
;CH = 左上角行号
;CL = 左上角列号
;DH = 右下角行号
;DL = 右下角列号
    int 10h     ;上卷0行，但是设置从（0，0）到（24，79）的每一个字节的属性为07h

;========== 再调用int10h 2号设置光标
    mov ax, 0200h
    mov bx, 0000h ; bh=显示的页码，就是内存b800h的8个页中的第0页
    mov dx, 0000h ; dh行坐标，dl列坐标
    int 10h

;========== 显示一个字符串 int 10h 13h
    mov ax, 1301h  
    mov bx, 000ah ; bh = 页码，bl字符属性
    mov dx, 001eh ; 行列号
    mov cx, StartBootMessageLength
    mov bp, StartBootMessage    ; es:bp 是要显示的字符串的地址
    ; es就是这段程序的段地址，因为之前的代码没有修改es的值
    int 10h

;========= 根据前62个字节的数据，将磁盘初始化
    xor ah, ah
    xor dl, dl ; bios int 13h的0号初始化磁盘、dl驱动器号
    int 13h

; 然后是一个大循环来读取fat根目录中有没有loader.bin的文件信息
    mov word [ReadCurrentSectorNumber], SectorNumOfRootDirStart
Search_Loader_In_RootDir:
    cmp word [ReadSectorsOfRootDir], 0 ; 循环14次
    je NoLoaderFound
    dec word [ReadSectorsOfRootDir]
    mov ax, 0
    mov es, ax
    mov bx, 8000h
    mov cx, 1
    mov ax, [ReadCurrentSectorNumber]
    call ReadOneSector
    ; 一个扇区16个文件
    mov dx, 10h
    mov di, 8000h ; 缓冲区偏移地址
    mov si, LoaderFileName
    cld
Search_In_16_File:
    cmp dx, 0
    je Search_In_Next_Sector
    dec dx
    mov cx, 11
Compare_File_Name:
    cmp cx, 0
    je ShowLoaderFoundMessage 
    dec cx
    lodsb
    cmp al, [es:di]
    je Equal_Compare_Next_Char
    jmp Search_In_Next_File

Equal_Compare_Next_Char:
    inc di
    jmp Compare_File_Name
Search_In_Next_File:
    and di, 0ffe0h
    add di, 20h
    mov si, LoaderFileName
    jmp Search_In_16_File

Search_In_Next_Sector:
    add word [ReadCurrentSectorNumber], 1
    jmp Search_Loader_In_RootDir

NoLoaderFound:
    ; 这个函数只有在14个扇区读取完都没有发现loader的情况下才会执行
	; 这个函数的主要功能是调用bios的int 10h的13h号中断在屏幕上显示没有找到的字符串信息
    mov bx, 008ch
    mov dx, 011dh
    mov cx, NoLoaderFoundMessageLength
    mov ax, ds
    mov es, ax
    mov bp, NoLoaderFoundMessage
    mov ax, 1301h
    int 10h
    jmp $

ShowLoaderFoundMessage:
	mov ax, ds
	mov es, ax
	mov bp, LoaderFoundMessage
	mov cx, LoaderFoundMessageLength
    mov ax, 1301h
    mov bx, 0009h
	mov dx, 0121h
    int 10h
LoaderFound:
    and di, 0ffe0h
    add di, 1ah
    mov cx, word [es:di]
    push cx
    add cx, RootDirSectors
    add cx, SectorBanlance
    mov ax, BaseOfLoader
    mov es, ax
    mov bx, OffsetOfLoader
    mov ax, cx

Loading_Loader_File:
    mov cx, 1
    call ReadOneSector
    pop ax  ; ax得到loader所在的簇号（FAT12表项号），139行代码处，push cx
    call Get_Loader_Information_In_FAT_Table
    cmp ax, 0fffh
    ; 0fffh表示最后一个簇，以簇号2为例，即这个loader的数据只占用1个簇的大小，如果不止一个簇的大小，ax中的数据会是0003h，即它还占用了第三个簇
    je	Loader_Has_Loaded	; 如果是一个簇的大小，跳转
    push ax ; 这里必须ax的值入栈，见157行的代码
    add ax, RootDirSectors
    add ax, SectorBanlance
    add bx, [BPB_BytesPerSec]
    jmp Loading_Loader_File
Loader_Has_Loaded:
    ; 跳转到loader程序处执行代码
    jmp BaseOfLoader:OffsetOfLoader
Get_Loader_Information_In_FAT_Table:
    push es
    push bx
    push ax
    mov ax, 0
    mov es, ax
    mov bx, 3
    pop ax
    mul bx
    mov bx, 2
    div bx
    mov byte [FAT_Index_Odd_Or_Even], 0 ; 默认为0
    cmp dx, 0
    je Get_Value_According_Index
    mov byte [FAT_Index_Odd_Or_Even], 1
Get_Value_According_Index:
    xor dx, dx ; 一会儿要用到dx保存余数
    mov bx, [BPB_BytesPerSec]
    div bx
    push dx ; 因为ReadOneSector函数要改变dx的值
    mov bx, 8000h
    add ax, SectorNumOfFAT1Start
    mov cl, 1
    call ReadOneSector
    pop dx
    add bx, dx
    mov ax, [es:bx]
    cmp byte [FAT_Index_Odd_Or_Even], 0
    je Even
    shr ax, 4

Even:
    and ax, 0fffh
    pop bx
    pop es
    ret

ReadOneSector:
    push bp
    mov bp, sp
    sub esp, 2
    mov byte [bp-2], cl
    push bx
    mov bl, byte [BPB_SecPerTrk]
    div bl
    inc ah
    mov cl, ah
    mov dh, al
    mov ch, al
    and dh, 1
    shr ch, 1
    mov dl, byte [BS_DrvNum]
    pop bx
Go_On_Read_If_Failed:
    mov byte al, [bp-2]
    mov ah, 02h
    int 13h
    jc Go_On_Read_If_Failed ; 读取成功cf会置0
    add esp, 2
    pop bp
    ret

;========= 定义要用到的tmp variables
ReadSectorsOfRootDir dw RootDirSectors
ReadCurrentSectorNumber dw 0
FAT_Index_Odd_Or_Even db 0

;========= display messages 
StartBootMessage: db 'Start Booting......'
StartBootMessageLength equ $ - StartBootMessage
NoLoaderFoundMessage: db 'ERROR: NO LOADER FOUND!'
NoLoaderFoundMessageLength equ $ - NoLoaderFoundMessage
LoaderFoundMessage: db 'Loader found!'
LoaderFoundMessageLength equ $ - LoaderFoundMessage
LoaderFileName: db 'LOADER  BIN'
	times 510 - ($ - $$) db 0
	db 0x55
	db 0xaa
