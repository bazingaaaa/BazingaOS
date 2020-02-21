BaseOfLoader		equ 	09000h;loader.bin加载的位置，段地址
OffsetOfLoader		equ		0100h;loader.bin加载的位置，偏移地址

BaseOfKernel		equ 	07000h;kernel.bin加载地址，段地址
OffsetOfKernel		equ 	0;kernel.bin加载地址，偏移地址

BaseOfLoaderPhyAddr	equ		BaseOfLoader * 10h;loader.bin加载的位置，物理地址
BaseOfKernelPhyAddr	equ 	BaseOfKernel * 10h;kernel.bin加载的位置，物理地址

ROOT_BASE			equ		0x6000

KernelEntryPointPhyAdr	equ 	030400h

PageDirBase			equ 	0100000h;1M
PageTblBase			equ 	0101000h;1M + 4K


KERNEL_FILE_SEG		equ    BaseOfKernel
KERNEL_FILE_OFF		equ	   OffsetOfKernel	 


;loader存放启动参数的物理地址
BOOT_PARAM_ADDR		equ	0x900
BOOT_PARAM_MAGIC	equ	0xB007