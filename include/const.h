#ifndef _CONST_H_
#define _CONST_H_

#define PUBLIC
#define PRIVATE static
#define EXTERN extern

#define ASSERT 
#ifdef ASSERT
void assertion_failure(char *exp, char *file, char *base_file, int line);
#define assert(exp)  if(exp);\
	else assertion_failure(#exp, __FILE__, __BASE_FILE__, __LINE__);
#else
#define assert(exp)
#endif

#define MIN(a, b) (a > b ? b : a)

#define TRUE 1
#define FALSE 0

#define GDT_SIZE 128

#define IDT_SIZE 256
#define LDT_SIZE 256

#define INT_M_CTL 0x20
#define INT_M_CTLMASK 0x21

#define INT_S_CTL 0xA0
#define INT_S_CTLMASK 0xA1

#define CLOCK_INT_NO 0
#define KEYBOARD_INT_NO 1
#define	CASCADE_IRQ	2	/* cascade enable for 2nd AT controller */
#define	AT_WINI_IRQ	14	/* at winchester */

/* 权限 */
#define	PRIVILEGE_KRNL	0
#define	PRIVILEGE_TASK	1
#define	PRIVILEGE_USER	3

/*时钟中断相关*/
#define TIMER0			0x40
#define TIMER_MODE 		0x43
#define RATE_GENERATOR 	0x34

#define TIMER_FREQ 1193182L
#define HZ 100

#define BLACK 0x0 
#define BLUE 0x1
#define GREEN 0x2
#define RED 0x4
#define WHITE 0x7
#define BLINK 0x80
#define BRIGHT 0x08
#define MAKE_COLOR(color1, color2) (color1 | color2)

#define NR_TTYS 3
#define NR_CONSOLES	3	/* consoles */

/*最大任务数，暂定为4*/
#define NR_TASKS 4
#define NR_PROCS 3

#define FIRST_PROC	proc_table[0]
#define LAST_PROC	proc_table[NR_TASKS + NR_PROCS - 1]

/* ipc */
#define SEND		1
#define RECEIVE		2
#define BOTH		3	/* BOTH = (SEND | RECEIVE) */

/* TASK_XX 要和global.c中对应*/
#define INVALID_DRIVER	-20
#define INTERRUPT -10
#define TASK_SYS 0
#define TASK_TTY 1
#define TASK_HD 2
#define TASK_FS 3


#define ANY	(NR_TASKS + NR_PROCS + 10)
#define NO_TASK	(NR_TASKS + NR_PROCS + 20)

/* Process */
#define SENDING   0x02	/* set when proc trying to send */
#define RECEIVING 0x04	/* set when proc trying to recv */
#define WAITING   0x08	/* set when proc waiting for the child to terminate */
#define HANGING   0x10	/* set when proc exits without being waited by parent */
#define FREE_SLOT 0x20	/* set when proc table entry is not used

/* magic chars used by `printx' */
#define MAG_CH_PANIC	'\002'
#define MAG_CH_ASSERT	'\003'

enum msgtype {
	/* 
	 * when hard interrupt occurs, a msg (with type==HARD_INT) will
	 * be sent to some tasks
	 */
	HARD_INT = 1,

	/* SYS task */
	GET_TICKS, GET_PID, GET_RTC_TIME,

	/* FS */
	OPEN, CLOSE, READ, WRITE, LSEEK, STAT, UNLINK,

	/* FS & TTY */
	SUSPEND_PROC, RESUME_PROC,

	/* MM */
	EXEC, WAIT,

	/* FS & MM */
	FORK, EXIT,

	/* TTY, SYS, FS, MM, etc */
	SYSCALL_RET,

	/* message type for drivers */
	DEV_OPEN = 1001,
	DEV_CLOSE,
	DEV_READ,
	DEV_WRITE,
	DEV_IOCTL
};


/* macros for messages */
#define	FD		u.m3.m3i1
#define	PATHNAME	u.m3.m3p1
#define	FLAGS		u.m3.m3i1
#define	NAME_LEN	u.m3.m3i2

#define	CNT		u.m3.m3i2
#define	REQUEST		u.m3.m3i2
#define	PROC_NR		u.m3.m3i3
#define	DEVICE		u.m3.m3i4
#define	POSITION	u.m3.m3l1
#define	BUF		u.m3.m3p2
/* #define	OFFSET		u.m3.m3i2 */
/* #define	WHENCE		u.m3.m3i3 */

/* #define	PID		u.m3.m3i2 */
/* #define	STATUS		u.m3.m3i1 */
#define	RETVAL		u.m3.m3i1
/* #define	STATUS		u.m3.m3i1 */


#define GRAY_CHAR (MAKE_COLOR(BLACK, BLACK) | BRIGHT)
#define RED_CHAR  (MAKE_COLOR(BLUE, RED) | BRIGHT)


#define	DIOCTL_GET_GEO	1

/*硬盘扇区*/
#define SECTOR_SIZE 512
#define SECTOR_SIZE_BIT		(SECTOR_SIZE * 8)
#define SECTOR_SIZE_SHIFT	9


#define MAX_DRIVES			2/*最大硬盘数量*/
#define NR_PART_PER_DRIVE  	4/*主引导扇区分区表对应4个主分区*/
#define NR_SUB_PER_PART		16/*每个扩展分区最多有16个逻辑分区*/
#define NR_SUB_PER_DRIVE   	(NR_SUB_PER_PART * NR_PART_PER_DRIVE)
#define NR_PRIM_PER_DRIVE  	(NR_PART_PER_DRIVE + 1)

#define MAX_PRIM			(MAX_DRIVES * NR_PRIM_PER_DRIVE - 1)
#define MAX_SUBPARTIONS		(NR_SUB_PER_DRIVE * MAX_DRIVES)

/*主设备号*/
#define NO_DEV				0
#define DEV_FLOPPY			1
#define DEV_CDROM			2
#define DEV_HD				3
#define DEV_CHAR_TTY		4
#define DEV_SCSI			5

/*利用主设备号和次设备号形成设备号*/
#define MAJOR_SHIFT			8
#define MAKE_DEV(a, b)		((a << MAJOR_SHIFT) | b)

/*利用设备号获取主设备号和次设备号*/
#define MAJOR(x)			((x >> MAJOR_SHIFT) & 0xff)		
#define	MINOR(x)			(x & 0xff)

/*设备号*/
#define MINOR_hd1a			0x10
#define MINOR_hd2a			(MINOR_hd1a + NR_SUB_PER_PART)

#define ROOT_DEV			MAKE_DEV(DEV_HD, MINOR_BOOT) 			

#define	P_PRIMARY	0
#define	P_EXTENDED	1

#define BAZINGA_PART	0x99	/* Orange'S partition */
#define NO_PART		0x00	/* unused entry */
#define EXT_PART	0x05	/* extended partition */


#define	NR_FILES	64/*进程打开的文件数量最大值*/
#define	NR_FILE_DESC	64	/* FIXME */
#define	NR_INODE	64	/* FIXME */
#define	NR_SUPER_BLOCK	8


/* INODE::i_mode (octal, lower 12 bits reserved) */
#define I_TYPE_MASK     0x0170000
#define I_REGULAR       0x0100000
#define I_BLOCK_SPECIAL 0x0060000
#define I_DIRECTORY     0x0040000
#define I_CHAR_SPECIAL  0x0020000
#define I_NAMED_PIPE	0x0010000

#define	is_special(m)	((((m) & I_TYPE_MASK) == I_BLOCK_SPECIAL) ||	\
			 (((m) & I_TYPE_MASK) == I_CHAR_SPECIAL))

#define	NR_DEFAULT_FILE_SECTS	2048 /* 2048 * 512 = 1MB */

#define	INVALID_INODE		0
#define	ROOT_INODE			1

#endif