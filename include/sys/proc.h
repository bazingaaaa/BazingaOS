#ifndef _PROC_H_
#define _PROC_H_


typedef struct s_stackframe
{
	u32 gs;
	u32 fs;
	u32 es;
	u32 ds;
	u32 edi;
	u32 esi;
	u32 ebp;
	u32 kernel_esp;
	/*通用寄存器*/
	u32 ebx;
	u32 edx;
	u32 ecx;
	u32 eax;
	u32 retaddr;
	u32 eip;
	u32 cs;
	u32 eflags;
	u32 esp;
	u32 ss;
}STACK_FRAME;


typedef struct s_proc
{
	STACK_FRAME regs;

	u16 ldt_sel;
	DESCRIPTOR ldts[LDT_SIZE];

	int ticks;/*ticks remain*/
	int priority;

	u32 pid;
	char p_name[16];

	int p_flags;/*process flags 只有当p_flags==0时，进程才可以运行，即阻塞与否*/

	MESSAGE *p_msg;/*当该进程发送消息时，用于缓存未成功发出的消息*/
	int p_recvfrom;/*当该进程处于接收消息状态时，存放想要接受的进程索引*/
	int p_sendto;/*当该进程处于发送消息状态时，存放消息发向的目的进程索引*/

	int has_int_msg;/*该进程是否需要接收中断消息*/

	struct proc *q_sending;/*发送消息至当前进程的进程队列*/
	struct proc *next_sending;/*进程队列中的下一个进程*/

	int p_parent; /*父进程的pid*/

	struct file_desc *filp[NR_FILES];
}PROCESS;

typedef struct s_task
{
	task_f initial_eip;
	int stacksize;
	char name[32];
}TASK;


#define proc2pid(x) (x - proc_table)


#define NR_IRQS 16
#define NR_SYS_CALLS 128

#define STACK_SIZE 0x8000/*栈空间不能太小，栈溢出可能导致各种异常*/
#define STACK_SIZE_INIT STACK_SIZE
#define STACK_SIZE_TESTA STACK_SIZE
#define STACK_SIZE_TESTB STACK_SIZE
#define STACK_SIZE_TESTC STACK_SIZE
#define STACK_SIZE_TASKTTY STACK_SIZE
#define STACK_SIZE_TASKSYS STACK_SIZE
#define STACK_SIZE_TASKTHD STACK_SIZE
#define STACK_SIZE_TASKTFS STACK_SIZE
#define STACK_SIZE_TASKMM STACK_SIZE
#define STACK_SIZE_TOTAL (STACK_SIZE_INIT\
						+STACK_SIZE_TESTA\
						+STACK_SIZE_TESTB\
						+STACK_SIZE_TESTC\
						+STACK_SIZE_TASKTTY\
						+STACK_SIZE_TASKSYS\
						+STACK_SIZE_TASKTHD\
						+STACK_SIZE_TASKTFS\
						+STACK_SIZE_TASKMM)

#endif