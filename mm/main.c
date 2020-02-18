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

PRIVATE int do_fork(MESSAGE *msg);
PRIVATE void init_mm();
PRIVATE int alloc_mem(int pid, int mem_size);


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
功能：完成创建进程相关的复制工作
备注：复制工作包括 1.在进程表中查找空闲槽位存放子进程
				 2.进程表的复制（局部描述符表选择子不能复制）
				 3.解析父进程的内存分布，并为子进程的T/D/S段分配内存空间
				 4.将父进程的内存映像拷贝到子进程的进程空间中
				 5.向fs任务发送fork消息
				 6.向子进程发送消息并唤醒
*/
PRIVATE int do_fork(MESSAGE *msg)
{
	int i;
	int pid;
	PROCESS *pChild;
	int parent_pid = msg->source;
	int child_ldt_sel;

	for(i = 0;i < NR_TASKS + NR_PROCS;i++)
	{
		if(proc_table[i].p_flags == FREE_SLOT)
			break;
	}
	assert(i >= NR_TASKS + NR_NATIVE_PROCS);
	assert(i < NR_TASKS + NR_PROCS);
	pid = i;
	pChild = &proc_table[pid];
	child_ldt_sel = pChild->ldt_sel;
	*pChild = proc_table[parent_pid];
	pChild->ldt_sel = child_ldt_sel;

	/*获取父进程内存映像图(代码段和数据段)*/
	DESCRIPTOR *pTD = &proc_table[parent_pid].ldts[INDEX_LDT_C];
	DESCRIPTOR *pDD = &proc_table[parent_pid].ldts[INDEX_LDT_RW];
	int caller_T_base = reassembly(pTD->base_high, 24, pTD->base_mid, 16, pTD->base_low);
	int caller_T_limit = reassembly(0, 0, (pTD->limit_high_attr2&0xF), 16, pTD->limit_low);
	int caller_T_size = (1 + caller_T_limit) * ((pTD->limit_high_attr2 & 0x80) ? 4096 : 1);
	int caller_D_base = reassembly(pTD->base_high, 24, pDD->base_mid, 16, pDD->base_low);
	int caller_D_limit = reassembly((pDD->limit_high_attr2&0xF), 16, 0, 0, pDD->limit_low);
	int caller_D_size = (1 + caller_D_limit) * ((pDD->limit_high_attr2 & 0x80) ? 4096 : 1);

	printl("caller_T_base:0x%x  caller_T_limit:0x%x caller_T_size:0x%x\n", caller_T_base, caller_T_limit, caller_T_size);
	printl("caller_D_base:0x%x  caller_D_limit:0x%x caller_D_size:0x%x\n", caller_D_base, caller_D_limit, caller_D_size);
	assert(caller_T_base == caller_D_base &&
			caller_T_limit == caller_D_limit &&
			caller_T_size == caller_D_size);

	/*为子进程分配内存，将父进程的内存映像拷贝到子进程的进程空间中*/
	int child_base = alloc_mem(pid, caller_T_size);
	printl("child_base:0x%x size:0x%x\n", child_base, PROC_DEFAULT_IMG_SIZE);
	phys_copy((void*)child_base, (void*)caller_T_base, caller_T_size);

	/*更改子进程ldt*/
	init_descriptor(&pChild->ldts[INDEX_LDT_C], child_base, (PROC_DEFAULT_IMG_SIZE - 1)>>LIMIT_4K_SHIFT, DA_32 | DA_C | PRIVILEGE_USER<<5 | DA_LIMIT_4K);
	init_descriptor(&pChild->ldts[INDEX_LDT_RW], child_base, (PROC_DEFAULT_IMG_SIZE - 1)>>LIMIT_4K_SHIFT, DA_32 | DA_DRW | PRIVILEGE_USER<<5 | DA_LIMIT_4K);
	
	/*向fs任务发送fork信息*/
	MESSAGE fs_msg;
	fs_msg.type = FORK;
	fs_msg.PID = pid;
	send_rec(BOTH, TASK_FS, &fs_msg);

	msg->PID = pid;

	/*唤醒子进程*/
	MESSAGE child_msg;
	child_msg.PID = 0;
	child_msg.RETVAL = 0;
	child_msg.type = SYSCALL_RET;
	send_rec(SEND, pid, &child_msg);

	return 0;
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
PRIVATE int alloc_mem(int pid, int mem_size)
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