;FAT12磁盘头
	BS_OEMName		DB 'ForrestY'
	BPB_BytesPerSec	DW	512
	BPB_SecPerClus	DB 1
	BPB_RsvdSecCnt	DW 1
	BPB_NumFATs		DB 2
	BPB_RootEntCnt	DW 224
	BPB_TotSec16	DW 2880
	BPB_Media		DB 0xF0
	BPB_FATSz16		DW 9
	BPB_SecPerTrk	DW 18;每个磁道的扇区数
	BPB_NumHeads	DW 2;磁头数（共两面）
	BPB_HiddSec		DD 0
	BPB_TotSec32	DD 0
	BS_DrvNum		DB 0;中断13的驱动器号
	BS_Reserved1 	DB 1
	BS_BootSig		DB 29h
	BS_VolID		DD 0
	BS_VolLab		DB 'BazingaOS01'
	BS_FileSysType 	DB 'FAT12   '

; -------------------------------------------------------------------------
; 基于 FAT12 头的一些常量定义，如果头信息改变，下面的常量可能也要做相应改变
; -------------------------------------------------------------------------
; BPB_FATSz16
FATSz			equ	9

; 根目录占用空间:
; RootDirSectors = ((BPB_RootEntCnt*32)+(BPB_BytsPerSec–1))/BPB_BytsPerSec
; 但如果按照此公式代码过长，故定义此宏
RootDirSectors		equ	14

; Root Directory 的第一个扇区号	= BPB_RsvdSecCnt + (BPB_NumFATs * FATSz)
SectorNoOfRootDirectory	equ	19

; FAT1 的第一个扇区号	= BPB_RsvdSecCnt
SectorNoOfFAT1		equ	1

; DeltaSectorNo = BPB_RsvdSecCnt + (BPB_NumFATs * FATSz) - 2
; 文件的开始Sector号 = DirEntry中的开始Sector号 + 根目录占用Sector数目
;                      + DeltaSectorNo
DeltaSecNo		equ	17