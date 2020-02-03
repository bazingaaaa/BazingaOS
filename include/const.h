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

/*最大任务数，暂定为4*/
#define NR_TASKS 4
#define NR_PROCS 0

#define FIRST_PROC	proc_table[0]
#define LAST_PROC	proc_table[NR_TASKS + NR_PROCS - 1]

/* ipc */
#define SEND		1
#define RECEIVE		2
#define BOTH		3	/* BOTH = (SEND | RECEIVE) */

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

#define	RETVAL		u.m3.m3i1


#define GRAY_CHAR (MAKE_COLOR(BLACK, BLACK) | BRIGHT)
#define RED_CHAR  (MAKE_COLOR(BLUE, RED) | BRIGHT)

#define MAX_DRIVES			2/*最大硬盘数量*/
#define NR_PART_PER_DRIVE  	4/*主引导扇区分区表对应4个主分区*/
#define NR_SUB_PER_PART		16/*每个扩展分区最多有16个逻辑分区*/
#define NR_SUB_PER_DRIVE   	(NR_SUB_PER_PART * NR_PART_PER_DRIVE)
#define NR_PRIM_PER_DRIVE  	(NR_PART_PER_DRIVE + 1)

#define MAX_PRIM			(MAX_DRIVES * NR_PART_PER_DRIVE - 1)
#define MAX_SUBPARTIONS		(NR_SUB_PER_DRIVE * MAX_DRIVES)

#endif