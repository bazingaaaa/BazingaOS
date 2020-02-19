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



PRIVATE void cleanup(PROCESS *p);
PRIVATE int free_mem(PROCESS *p);


/*
功能：完成创建进程相关的复制工作
备注：复制工作包括 1.在进程表中查找空闲槽位存放子进程
				 2.进程表的复制（局部描述符表选择子不能复制）
				 3.解析父进程的内存分布，并为子进程的T/D/S段分配内存空间
				 4.将父进程的内存映像拷贝到子进程的进程空间中
				 5.向fs任务发送fork消息
				 6.向子进程发送消息并唤醒
*/
PUBLIC int do_fork(MESSAGE *msg)
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

	/*设置子进程名称*/
	pChild->p_parent = parent_pid;
	sprintf(pChild->p_name, "%s_%d", proc_table[parent_pid].p_name, pid);

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
功能：进程退出操作
备注：1.向fs任务发送进程退出消息
	 2.释放内存
	 3.检查父进程状态
	 4.检查子进程状态
	 暂不考虑INIT进程退出
*/
PUBLIC void do_exit(MESSAGE *msg)
{
	int i;
	int pid = msg->source;
	MESSAGE msg2fs;

	msg2fs.type = EXIT;
	msg2fs.PID = pid;
	send_rec(BOTH, TASK_FS, &msg2fs);

	/*设置退出状态*/
	proc_table[pid].exit_status = msg->STATUS;

	/*释放内存*/
	free_mem(&proc_table[pid]);

	int parent = proc_table[pid].p_parent;
	/*检查父进程状态*/
	if(proc_table[parent].p_flags & WAITING)
	{
		proc_table[parent].p_flags &= ~WAITING;/*去除WAITING状态*/
		cleanup(&proc_table[pid]);
	}
	else
	{
		proc_table[pid].p_flags |= HANGING;/*等待父进程回收*/
	}

	/*检查子进程状态*/
	for(i = 0;i < NR_TASKS + NR_PROCS;i++)
	{
		if(proc_table[i].p_parent == pid)/*找到子进程*/
		{	
			proc_table[i].p_parent = INIT;/*将子进程过继给INIT进程*/
			if((proc_table[i].p_flags & HANGING) && 
				(proc_table[INIT].p_flags & WAITING))/*正好子进程处于HANGING，INIT进程处于WAITING态*/
			{
				proc_table[INIT].p_flags &= ~WAITING;
				cleanup(&proc_table[i]);
			}
		}
	}
}


/*
功能：等待进程结束操作
备注：1.检查是否有处于HANGING状态的子进程
	 2.若有则回收该子进程的进程表项，并返回子进程的退出状态
	 3.若无则检查是否有子进程
	 4.若有则置WAITING状态，阻塞该进程，若无则直接返回
*/
PUBLIC void do_wait(MESSAGE *msg)
{
	int i;
	int hasChild = 0;/*是否有子进程标志*/
	int pid = msg->source;
	PROCESS *p = &proc_table[pid];

	/*检查子进程状态*/
	for(i = 0;i < NR_TASKS + NR_PROCS;i++)
	{
		if(proc_table[i].p_parent != pid)
		{
			continue;
		}
		hasChild = 1;
		if(proc_table[i].p_flags & HANGING)
		{
			cleanup(&proc_table[i]);
			return;
		}
	}

	if(hasChild)/*有子进程*/
	{
		p->p_flags |= WAITING;
	}
	else/*无子进程*/
	{
		MESSAGE reply;
		reply.type = SYSCALL_RET;
		reply.PID = NO_TASK;
		reply.STATUS = -1;
		send_rec(SEND, pid, &reply);
	}
}


/*
功能：清除进程表项,解除父进程阻塞
*/
PRIVATE void cleanup(PROCESS *p)
{
	MESSAGE msg;

	msg.type = SYSCALL_RET;
	msg.PID = proc2pid(p);
	msg.STATUS = p->exit_status;
	send_rec(SEND, p->p_parent, &msg);

	p->p_flags = FREE_SLOT;
}


/*
功能：释放进程占用的内存
*/
PRIVATE int free_mem(PROCESS *p)
{
	return 0;
}
