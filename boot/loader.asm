org 0100h;当引导扇区加载该文件时，该文件在内存中的位置由引导扇区决定, 这条命令由链接器处理
;决定了文件的偏移量（相对于CS段） 而标号是相对于文件起始的偏移量
	jmp LABEL_START

%include "load.inc"
%include "pm.inc"


[section .gdt]
; GDT
; 								  段基址,     		 段界限, 	          属性
LABEL_GDT: 			Descriptor 		   0,        		 0,   		        0; 空描述符
LABEL_DESC_FLAT_C: Descriptor 		   0, 			0fffffh, DA_CR + DA_32 + DA_LIMIT_4K; 非一致代码段，可读可写，4k粒度
LABEL_DESC_FLAT_RW: 	Descriptor         0,           0fffffh, DA_32 + DA_DRW + DA_LIMIT_4K ; Normal 描述符
LABEL_DESC_VIDEO: 	Descriptor 	 0B8000h, 		    0ffffh, 	DA_DRW + DA_DPL3; 显存首地址
; GDT 结束

GdtLen equ $ - LABEL_GDT ; GDT长度
GdtPtr dw GdtLen - 1 ; GDT界限
	   dd BaseOfLoaderPhyAddr + LABEL_GDT; GDT基地址

; GDT 选择子
SelectorFlatC  equ LABEL_DESC_FLAT_C - LABEL_GDT
SelectorFlatRW 	equ LABEL_DESC_FLAT_RW - LABEL_GDT
SelectorVideo  equ LABEL_DESC_VIDEO - LABEL_GDT + SA_RPL3

;常量
BaseOfStack 		equ 	0100h
;BaseOfKernel		equ 	08000h
;OffsetOfKernel		equ  	0h

;字符串
;字符串
KernelFileName		db "KERNEL  BIN", 0
MessageLength		equ 9
BootMessage:		db "LOADING  ";此处对齐是为了简化
Message1:			db "Ready.   "
Message2:			db "No KERNEL"

%include "fat12hdr.inc"

;变量
wRootDirSizeForLoop dw RootDirSectors;循环查找的扇区数，循环中会减少
WSectorNo			dw 0;要读取的扇区号
;bOdd				db 0;奇数还是偶数
wDirtNo				dw 0;目录号
wByteNo				dw 0;字节号
bOdd				db 0;是否为奇数，1是 0否


;找到文件名位kernel.bin的文件
LABEL_START:
	mov ax, cs 
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov sp, BaseOfStack

	mov dh, 0
	call DispstrRealMode

;软驱复位
	xor ah, ah
	xor dl, dl
	int 13h

	mov word [WSectorNo], SectorNoOfRootDirectory;存放起始扇区号
READ_ON_SECTOR:;循环读取扇区
	cmp word [wRootDirSizeForLoop], 0
	jz LOADER_NOT_FOUND
	dec	word [wRootDirSizeForLoop]
	;读扇区前准备
	mov ax, BaseOfKernel
	mov es, ax
	mov cl, 1
	mov ax, [WSectorNo];
	mov bx, OffsetOfKernel
	call ReadSector
	mov word [wDirtNo], 10h;每个扇区可以存放16个目录项
COMPARE_EVERY_ENT:
	cmp word [wDirtNo], 0
	jz ONE_SECTOR_FINISH;该扇区已经比较完毕
	mov ax, KernelFileName
	mov si, ax
	mov ax, 10h
	sub ax, word [wDirtNo]
	dec word [wDirtNo]
	mov bx, 20h
	mul bx
	add ax, OffsetOfKernel;求出扇区中第n个目录项的偏移
	mov di, ax
	mov ax, BaseOfKernel
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


;发现kernel.bin之后的操作,此时di已经指向kernel.bin的尾部
;1.取出起始的扇区号
;2.读取扇区号对应的数据
;3.根据扇区号获取对应的FAT项，并获取下一个扇区号
;4.判断扇区号是否为0xFFF,若是则结束，若否则回到2
LOADER_FOUND:;找到了kernel.bin
	mov ax, RootDirSectors
	and di, 0FFE0h;找到对应目录项的起始
	add di, 01Ah
	mov cx, word[es:di];cx存放FAT项偏移（非扇区号）
	push cx;将FAT项偏移存入栈中
	add ax, cx
	add ax, DeltaSecNo;此时ax为扇区号

	;初始化loader在内存中存放的位置
	push ax
	mov ax, BaseOfKernel
	mov es, ax
	mov bx, OffsetOfKernel
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
	add bx, word[BPB_BytesPerSec];需要判断是否超过64k
	jc OVER_ONE_SEG
	jmp NOT_OVER_ONE_SEG	

OVER_ONE_SEG:
	push ax
	mov ax, es
	add ax, 01000h
	mov es, ax
	pop ax

NOT_OVER_ONE_SEG:
	jmp LOAD_ONE_SEC
	
LOAD_FINISH:;加载完毕
	mov dh, 1
	call DispstrRealMode

	;得到内存数,只能在实模式下完成，转换到保护模式下中断响应(IDT)发生改变
	mov ebx, 0
	mov di, _MemChkBuf;实模式下ds = cs，而_MemChkBuf 代表的是在代码段内的偏移，代码段和数据段地址相同（必须相同），故可直接
					  ;通过段内偏移进行访问

.loop:
	mov eax, 0E820h
	mov ecx, 20;/*代表结构体大小*/
	mov edx, 0534D4150h
	int 15h
	jc LABEL_MEM_CHK_FAIL
	add di, 20
	inc dword[_dwMCRNumber]
	cmp ebx, 0
	jne .loop;不等于条件转移，即ebx不为0，还未到内存结构体数组尾部
	jmp LABEL_MEM_CHK_OK
LABEL_MEM_CHK_FAIL:
	mov dword[_dwMCRNumber], 0
LABEL_MEM_CHK_OK:;此时已经把所需要的信息读取到_MemChkBuf中

	;加载gdtr
	lgdt [GdtPtr]

	;关中断
	cli

	;打开地址线
	in al, 92h
	or al, 00000010b
	out 92h, al

	;切换到保护模式
	mov eax, cr0
	or eax, 1
	mov cr0, eax

	;跳转的时候一定要注意目的段的属性，必须为可执行段
	jmp dword SelectorFlatC:(BaseOfLoaderPhyAddr + LABEL_PM_START)


LOADER_NOT_FOUND:;未找到kernel.bin 
	mov dh, 2
	call DispstrRealMode
	jmp $


GET_FAT_ENT:
	push es
	push bx
	push ax
	mov ax, BaseOfKernel
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

;调用该函数时，dh传入需要打印的字符序号（0， 1， 2）
DispstrRealMode:
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
	add dh, 3
	int 10h
	ret

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

[SECTION .text]
[BITS 32];该声明必不可少，决定了编译器是以16位还是32位的方式生成机器码
;已经进入保护模式
LABEL_PM_START:
	mov ax, SelectorVideo
	mov gs, ax
	mov ax, SelectorFlatRW
	mov ds, ax
	mov ss, ax
	mov es, ax;之后的代码中用到了stosd，需要对es段进行初始化
	mov esp, TopOfStack


	mov edi, (80 * 0 + 17) * 2 ; 屏幕第 10 行, 第 0 列。
 	mov ah, 0Ch   ; 0000: 黑底    1100: 红字
	mov al, 'P'
	mov [gs:edi], ax

	;输出内存信息
	push szMemChkTitle
	call DispStr
	add esp, 4
	call DispMemSize

	;启动分页
	call SetupPaging

	;装载内核，对内核的代码段和数据段进行装载
	call InitKernel
	
	;保存启动参数（BOOT_PARAMS）
	mov dword [BOOT_PARAM_ADDR], BOOT_PARAM_MAGIC
	mov eax, [dwMemSize]
	mov dword [BOOT_PARAM_ADDR + 4], eax
	mov eax, KERNEL_FILE_SEG
	shl eax, 4
	add eax, KERNEL_FILE_OFF
	mov dword [BOOT_PARAM_ADDR + 8], eax
		
	jmp SelectorFlatC:KernelEntryPointPhyAdr
;END of [SECTION .text]

DispMemSize:
	push	esi
	push	edi
	push	ecx

	mov	esi, MemChkBuf
	mov	ecx, [dwMCRNumber];for(int i=0;i<[MCRNumber];i++)//每次得到一个ARDS
.loop:				  ;{
	mov	edx, 5		  ;  for(int j=0;j<5;j++) //每次得到一个ARDS中的成员
	mov	edi, ARDStruct	  ;  {//依次显示BaseAddrLow,BaseAddrHigh,LengthLow,
.1:				  ;             LengthHigh,Type
	push	dword [esi]	  ;
	call	DispInt		  ;    DispInt(MemChkBuf[j*4]); //显示一个成员
	pop	eax		  ;
	stosd			  ;    ARDStruct[j*4] = MemChkBuf[j*4];
	add	esi, 4		  ;
	dec	edx		  ;
	cmp	edx, 0		  ;
	jnz	.1		  ;  }
	call	DispReturn	  ;  printf("\n");
	cmp	dword [dwType], 1 ;  if(Type == AddressRangeMemory)
	jne	.2		  ;  {
	mov	eax, [dwBaseAddrLow];
	add	eax, [dwLengthLow];
	cmp	eax, [dwMemSize]  ;    if(BaseAddrLow + LengthLow > MemSize)
	jb	.2		  ;
	mov	[dwMemSize], eax  ;    MemSize = BaseAddrLow + LengthLow;
.2:				  ;  }
	loop	.loop		  ;}
				  ;
	call	DispReturn	  ;printf("\n");
	push	szRAMSize	  ;
	call	DispStr		  ;printf("RAM size:");
	add	esp, 4		  ;
				  ;
	push	dword [dwMemSize] ;
	call	DispInt		  ;DispInt(MemSize);
	add	esp, 4		  ;

	pop	ecx
	pop	edi
	pop	esi
	ret

;启动分页机制
SetupPaging:
	; 根据内存大小计算应初始化多少PDE以及多少页表
	xor	edx, edx
	mov	eax, [dwMemSize]
	mov	ebx, 400000h	; 400000h = 4M = 4096 * 1024, 一个页表对应的内存大小
	div	ebx
	mov	ecx, eax	; 此时 ecx 为页表的个数，也即 PDE 应该的个数
	test	edx, edx
	jz	.no_remainder
	inc	ecx		; 如果余数不为 0 就需增加一个页表
.no_remainder:
	push ecx;暂存页表个数

	; 为简化处理, 所有线性地址对应相等的物理地址. 并且不考虑内存空洞.

	; 首先初始化页目录
	mov	ax, SelectorFlatRW; 此段首地址为 PageDirBase
	mov	es, ax
	mov	edi, PageDirBase;此段首地址为PageDirBase0
	xor	eax, eax
	mov	eax, PageTblBase | PG_P  | PG_USU | PG_RWW
.1:
	stosd
	add	eax, 4096		; 为了简化, 所有页表在内存中是连续的.
	loop	.1

	; 再初始化所有页表
	pop eax;页表个数
	mov	ebx, 1024		; 每个页表 1024 个 PTE
	mul	ebx
	mov	ecx, eax		; PTE个数 = 页表个数 * 1024
	mov edi, PageTblBase;此段首地址为PageTblBase0
	xor	eax, eax
	mov	eax, PG_P | PG_USU | PG_RWW;物理地址和线性地址是一一对应的关系
.2:
	stosd
	add	eax, 4096		; 每一页指向 4K 的空间
	loop	.2

	mov	eax, PageDirBase
	mov	cr3, eax
	mov	eax, cr0
	or	eax, 80000000h
	mov	cr0, eax
	jmp	short .3
.3:
	nop

	ret
;分页基址启动完毕

;初始化内核
;完成的操作就是 1.解析elf头，找到program header的位置
;			   2.根据ph中的内容将kernel.bin中的指定偏移内容拷贝到相应的虚拟地址上去
InitKernel:
	;首先确定ph的个数
	xor esi, esi
	mov cx, word [BaseOfKernelPhyAddr + 2Ch]
	movzx ecx, cx
	mov esi, dword [BaseOfKernelPhyAddr + 1Ch]
	add esi, BaseOfKernelPhyAddr;此时esi指向ph
load_seg:
	mov eax, [esi + 0]
	cmp eax, 0
	jz no_action
	;有效ph
	push dword [esi + 010h]
	mov eax, [esi + 04h]
	add eax, BaseOfKernelPhyAddr
	push eax
	push dword [esi + 08h]
	call MemCpy
	add esp, 12 

no_action:
	add esi, 020h
	dec ecx
	jnz load_seg

	ret



DispInt:
	mov eax, [esp + 4];取出需要显示的整型
	shr eax, 24;找到eax高八位
	call DispAL

	mov eax, [esp + 4];取出需要显示的整型
	shr eax, 16;找到eax高八位
	call DispAL

	mov eax, [esp + 4];取出需要显示的整型
	shr eax, 8;找到eax高八位
	call DispAL

	mov eax, [esp + 4];取出需要显示的整型
	call DispAL

	mov ah, 07h; 0000b：黑底  0111b：灰字
	mov al, 'h'
	push edi
	mov edi, [dwDispPos]
	mov [gs:edi], ax
	add edi, 4
	mov [dwDispPos], edi
	pop edi

	ret
;;DispInt 结束




DispStr:
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi

	mov esi, [ebp + 8];pszInfo
	mov edi, [dwDispPos]
	mov ah, 0Fh 
.1:
	lodsb;把si指向的字节装入al
	test al, al
	jz .2;ZF=1跳转 al为0则跳转
	cmp al, 0Ah;判断是不是回车
	jnz .3;ZF=0跳转 即al不是回车跳转
	push eax
	mov eax, edi
	mov bl, 160
	div bl
	and eax, 0ffh 
	inc eax
	mov bl, 160
	mul bl
	mov edi, eax
	pop eax
	jmp .1
.3:
	mov [gs:edi], ax
	add edi, 2
	jmp .1
.2:
	mov [dwDispPos], edi

	pop edi
	pop esi
	pop ebx
	pop ebp
	ret
;;DispStr 结束


DispReturn:
	push szReturn
	call DispStr
	add esp, 4

	ret
;;DispReturn结束


;; 显示 AL 中的数字
DispAL:
	push	ecx
	push	edx
	push	edi

	mov	edi, [dwDispPos]

	mov	ah, 0Fh			; 0000b: 黑底    1111b: 白字
	mov	dl, al
	shr	al, 4
	mov	ecx, 2
.begin:
	and	al, 01111b
	cmp	al, 9
	ja	.1
	add	al, '0'
	jmp	.2
.1:
	sub	al, 0Ah
	add	al, 'A'
.2:
	mov	[gs:edi], ax
	add	edi, 2

	mov	al, dl
	loop	.begin
	;add	edi, 2

	mov	[dwDispPos], edi

	pop	edi
	pop	edx
	pop	ecx

	ret
;; DispAL 结束


;内存拷贝
MemCpy:
	push ebp;用来保存堆栈指针的位置
	mov ebp, esp

	;把可能被更改的寄存器入栈
	push esi
	push edi
	push ecx

	mov ecx, [ebp + 16];拷贝的长度 counter
	mov esi, [ebp + 12];source
	mov edi, [ebp + 8];destination

.1:
	cmp ecx, 0
	jz .2;如果为0则跳转

	mov al, [ds:esi]
	inc esi

	mov byte [es:edi], al
	inc edi

	dec ecx
	jmp .1
.2:
	mov eax, [ebp + 8];返回值

	pop ecx
	pop edi
	pop esi
	mov esp, ebp
	pop ebp

	ret
;MemCpy结束


[section .data1]
ALIGN 32
LABEL_DATA:
;实模式下
;字符串
_szRAMSize          db  "RAM size:", 0
_szMemChkTitle:			db	"BaseAddrL BaseAddrH LengthLow LengthHigh   Type", 0Ah, 0	; 进入保护模式后显示此字符串

;变量

_szReturn           db  0Ah, 0
_dwDispPos:         dd  (80 * 6 + 0) * 2    ; 屏幕第 6 行, 第 0 列
_MemChkBuf: times   256 db  0;存放内存检查结果的缓存区
_dwMCRNumber		dd 		0
_ARDStruct:         ; Address Range Descriptor Structure
    _dwBaseAddrLow:     dd  0
    _dwBaseAddrHigh:    dd  0
    _dwLengthLow:       dd  0
    _dwLengthHigh:      dd  0
    _dwType:        dd  0
_dwMemSize:	dd 0;内存大小


;32位保护模式下的地址
szRAMSize       equ BaseOfLoaderPhyAddr + _szRAMSize
szMemChkTitle 	equ BaseOfLoaderPhyAddr + _szMemChkTitle
szReturn		equ	BaseOfLoaderPhyAddr + _szReturn
dwDispPos       equ BaseOfLoaderPhyAddr + _dwDispPos
MemChkBuf 		equ 	BaseOfLoaderPhyAddr + _MemChkBuf
dwMCRNumber 	equ		BaseOfLoaderPhyAddr + _dwMCRNumber
ARDStruct			equ BaseOfLoaderPhyAddr + _ARDStruct; Address Range Descriptor Structure
    dwBaseAddrLow   equ BaseOfLoaderPhyAddr + _dwBaseAddrLow
    dwBaseAddrHigh  equ BaseOfLoaderPhyAddr + _dwBaseAddrHigh
    dwLengthLow 	equ BaseOfLoaderPhyAddr + _dwLengthLow
    dwLengthHigh 	equ BaseOfLoaderPhyAddr + _dwLengthHigh
    dwType        	equ BaseOfLoaderPhyAddr + _dwType
dwMemSize 			equ BaseOfLoaderPhyAddr + _dwMemSize

; 堆栈就在数据段的末尾
StackSpace:	times	1024	db	0
TopOfStack	equ	BaseOfLoaderPhyAddr + $	; 栈顶
; SECTION .data1 之结束 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^