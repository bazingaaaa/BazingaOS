#include "type.h"
#include "config.h"
#include "const.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "fs.h"
#include "global.h"
#include "string.h"


PRIVATE void init_mm();


int memory_size;/*内存大小*/


/*
功能：内存管理任务
*/
PUBLIC void task_mm()
{
	MESSAGE msg;
	int src;

	init_mm();

	while(1)
	{	
		send_rec(RECEIVE, ANY, &msg);
		src = msg.source;
		int reply = 1;

		switch(msg.type)
		{
			case FORK:
				msg.RETVAL = do_fork(&msg);
				break;
			case EXIT:
				reply = 0;
				do_exit(&msg);
				break;
			case WAIT:
				reply = 0;
				do_wait(&msg);
				break;
			default:
				panic("mm unknown msg type");
				break;
		}
		if(reply)
		{
			msg.type = SYSCALL_RET;
			send_rec(SEND, src, &msg);
		}
	}
}


/*
功能：初始化mm任务
*/
PRIVATE void init_mm()
{
	struct boot_params bp;
	get_boot_param(&bp);
	memory_size = bp.mem_size;
	printl("memory_size:0x%x\n", bp.kernel_file[0]);
}


/*
功能：为进程分配内存
输入：pid-进程pid
	 mem_size-需要分配的内存大小
返回值：>0 分配所得的内存基址
*/
PUBLIC int alloc_mem(int pid, int mem_size)
{
	assert(pid >= NR_TASKS + NR_NATIVE_PROCS);

	if(mem_size > PROC_DEFAULT_IMG_SIZE)/*大小超过限制*/
	{
		panic("alloc mem_size too large!");
	}
	int base = PROC_ALLOC_BASE + (pid - (NR_TASKS + NR_NATIVE_PROCS)) * PROC_DEFAULT_IMG_SIZE;
	if(base + mem_size > memory_size)
	{
		panic("mem overflow");
	}
	return base;
}