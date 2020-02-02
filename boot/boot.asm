org  7c00h	
	jmp short LABEL_START;start to boot
	nop;不可缺少

%include "fat12hdr.inc"
%include "load.inc"

BaseOfStack			equ 07c00h;堆栈基地址（向下生长）
;BaseOfLoader		equ 0900h;LOADER.BIN加载的位置，段地址
;OffsetOfLoader		equ 0100h;LOADER.BIN加载的位置，偏移

;变量
wRootDirSizeForLoop dw RootDirSectors;循环查找的扇区数，循环中会减少
WSectorNo			dw 0;要读取的扇区号
;bOdd				db 0;奇数还是偶数
wDirtNo				dw 0;目录号
wByteNo				dw 0;字节号
bOdd				db 0;是否为奇数，1是 0否

;字符串
LoaderFileName		db "LOADER  BIN", 0
MessageLength		equ 9
BootMessage:		db "Booting  ";此处对齐是为了简化
Message1:			db "Ready.   "
Message2:			db "No LOADER"


LABEL_START:
;初始化各个段
	mov ax, cs 
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov sp, BaseOfStack

;软驱复位
	xor ah, ah
	xor dl, dl
	int 13h

;显示启动中
	call CLR_SCREEN
	mov dh, 0
	call Dispstr

;循环读取根目录对应的扇区，起始号为SectorNoOfRootDirectory 
;循环次数为RootDirSectors， 每个扇区内目录项的个数为512/32 = 16 
;BaseOfLoader:OffsetOfLoader 为存放读取数据的缓存区
	mov word [WSectorNo], SectorNoOfRootDirectory;存放起始扇区号
READ_ON_SECTOR:;循环读取扇区
	cmp word [wRootDirSizeForLoop], 0
	jz LOADER_NOT_FOUND
	dec	word [wRootDirSizeForLoop]
	;读扇区前准备
	mov ax, BaseOfLoader
	mov es, ax
	mov cl, 1
	mov ax, [WSectorNo];
	mov bx, OffsetOfLoader
	call ReadSector
	mov word [wDirtNo], 10h;每个扇区可以存放16个目录项
COMPARE_EVERY_ENT:
	cmp word [wDirtNo], 0
	jz ONE_SECTOR_FINISH;该扇区已经比较完毕
	mov ax, LoaderFileName
	mov si, ax
	mov ax, 10h
	sub ax, word [wDirtNo]
	dec word [wDirtNo]
	mov bx, 20h
	mul bx
	add ax, OffsetOfLoader;求出扇区中第n个目录项的偏移
	mov di, ax
	mov ax, BaseOfLoader
	mov es, ax
	cld;对si和di的方向进行调整
	mov word [wByteNo], 11;文件名长11个字节
COMPARE_EVERY_BYTE:
	cmp word [wByteNo], 0
	jz LOADER_FOUND;11个字节都遍历完了
	lodsb 
	cmp al, byte [es:di]
	jnz COMPARE_EVERY_ENT;不相同则跳转,接在cmp之后
	inc di
	dec word [wByteNo]
	jmp COMPARE_EVERY_BYTE


ONE_SECTOR_FINISH:
	inc word [WSectorNo]
	jmp READ_ON_SECTOR


;发现loader.bin之后的操作,此时di已经指向loader.bin的尾部
;1.取出起始的扇区号
;2.读取扇区号对应的数据
;3.根据扇区号获取对应的FAT项，并获取下一个扇区号
;4.判断扇区号是否为0xFFF,若是则结束，若否则回到2
LOADER_FOUND:;找到了loader.bin
	mov ax, RootDirSectors
	and di, 0FFE0h;找到对应目录项的起始
	add di, 01Ah
	mov cx, word[es:di];cx存放FAT项偏移（非扇区号）
	push cx;将FAT项偏移存入栈中
	add ax, cx
	add ax, DeltaSecNo;此时ax为扇区号

	;初始化loader在内存中存放的位置
	push ax
	mov ax, BaseOfLoader
	mov es, ax
	mov bx, OffsetOfLoader
	pop ax;此时ax应为需要读取的扇区号

LOAD_ONE_SEC:
	;每读一个扇区打印一个点
	push ax
	push bx
	mov ah, 0Eh
	mov al, '.'
	mov bl, 0Fh
	int 10h 
	pop bx 
	pop ax
	
	mov cl, 1
	call ReadSector
	pop ax;此处应为FAT项偏移 
	call GET_FAT_ENT
	cmp ax, 0FFFh
	jz LOAD_FINISH
	push ax
	add ax, RootDirSectors
	add ax, DeltaSecNo
	add bx, word[BPB_BytesPerSec]
	jmp LOAD_ONE_SEC
	
LOAD_FINISH:
	mov dh, 1
	call Dispstr
	jmp BaseOfLoader:OffsetOfLoader


LOADER_NOT_FOUND:;未找到loader.bin 
	mov dh, 2
	call Dispstr
	jmp $


;ax中存放扇区号，cl中存放一次读取的扇区数,读出的数据存放在es:bx指向缓冲区
ReadSector:
	push bp
	mov bp, sp 
	sub esp, 2

	mov byte [bp-2], cl;在栈上保存一次需要读取的扇区数
	push bx
	mov bl, [BPB_SecPerTrk]
	div bl
	inc ah
	mov cl, ah
	mov dh, al;先暂存商Q
	shr al, 1;商
	mov ch, al;添加柱面号
	and dh, 1
	pop bx;数据缓存区
	mov dl, [BS_DrvNum]
.GoOnReading:
	mov ah, 2
	mov al, byte [bp-2];一次去读的扇区数
	int 13h;产生中断
	jc .GoOnReading

	add esp, 2;恢复堆栈
	pop bp;恢复堆栈

	ret


;调用该函数时，dh传入需要打印的字符序号（0， 1， 2）
Dispstr:
	xor bx, bx
	mov ax, cs
	mov es, ax
	mov ax, MessageLength
	mov bl, dh
	mul bl
	add ax, BootMessage
	mov bp, ax
	mov ah, 13h 
	mov al, 1
	mov cx, MessageLength
	;mov bh, 1
	mov bl, 00000111b
	mov dl, 0;起始为0列
	int 10h
	ret

;清除屏幕
CLR_SCREEN:
	push ax
	push bx
	push cx
	push dx

	mov ax, 0600h
	mov bx, 0700h 
	mov cx, 0
	mov dx, 0184fh
	int 10h

	pop dx
	pop cx
	pop bx
	pop ax

	ret


;根据扇区号获取FAT项，并在ax中返回FAT项中的扇区号
;根据扇区号找到FAT项在FAT表中的偏移，加上FAT表的
;起始，找到FAT项对应的扇区号，读取对应的扇区内容，
;并取出FAT项中的下一个扇区号，注意FAT项可能跨越扇区
;ax存放输入的FAT项偏移，返回值也存放在ax中，为FAT项偏移
;将读取的FAT表的内容存放在BaseOfLoader之前4K的内存中
;可能改变的寄存器：es
GET_FAT_ENT:
	push es
	push bx
	push ax
	mov ax, BaseOfLoader
	sub ax, 0100h;4K字节的缓存区
	mov es, ax
	pop ax;恢复ax
	mov bx, 3
	mul bx
	mov bx, 2
	div bx
	mov byte[bOdd], dl;保存奇偶性，奇数的情况需要右移4位
	;此刻ax代表FAT项偏移的字节数（对应于FAT表的起始）,需要找到
	;偏移的扇区数
	xor dx, dx
	mov bx, word[BPB_BytesPerSec]
	div bx 
	push dx;需要保存偏移量
	add ax, SectorNoOfFAT1
	mov bx, 0
	mov cl, 2
	call ReadSector

	pop dx
	add bx, dx
	mov ax, [es:bx]
	cmp byte[bOdd], 0;
	jz FAT_EVEN
	shr ax, 4
FAT_EVEN:
	and ax, 0FFFh

	pop bx
	pop es
	ret



DUMMY:	times 510 - ($ - $$) db 0
end:	DW 0xAA55;0xAA55作为引导扇区的结束标志符
	
