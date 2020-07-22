sector_of_root_dir				equ	14
sector_num_of_root_dir_start	equ 19
sector_num_of_FAT1_start		equ 1
sector_balance					equ 17

; FAT12 describe table 
    BS_OEMName		db	'jarsboot'
    BPB_BytesPerSec	dw	512
    BPB_SecPerClus	db	1   ;数据区的没簇占多少扇区
    BPB_RsvdSecCnt	dw	1
    BPB_NumFATs		db	2
    BPB_RootEntCnt	dw	224  ; 根目录总的文件数目，这个用来确定根目录区占用的扇区数量
    BPB_TotSec16	dw	2880
    BPB_Media		db	0xf0  ; 可移动磁盘的类型F0，不可移动F8
    BPB_FATSz16		dw	9   ; 每个fat表占用的扇区数
    BPB_SecPerTrk	dw	18  ; 每个磁道的扇区数
    BPB_NumHeads	dw	2
    BPB_HiddSec		dd	0
    BPB_TotSec32	dd	0
    BS_DrvNum		db	0
    BS_Reserved1	db	0
    BS_BootSig		db	0x29
    BS_VolID		dd	0
    BS_VolLab		db	'boot loader'  ; 用完11个字节
    BS_FileSysType	db	'FAT12   ' ; 注意这里的空格，要用完8个字节

