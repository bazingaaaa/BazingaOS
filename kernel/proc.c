#include "const.h"
#include "type.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "fs.h"
#include "global.h"
#include "string.h"

/*
功能：系统调用，特权级0时调用，获取时钟tick数
*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}


/*
功能：任务调度，在clock_handler中调用
备注：首先选出剩余tick数最大的任务，如果最大tick数为0，则为每个任务重新分配tick，再去找最大tick数,
	增加了进程的状态判断，p_flags为0时进程才有运行的机会
*/
 PUBLIC void schedule()
 {
 	int	 greatest_ticks = 0;
 	PROCESS *p;

	while (!greatest_ticks) {
		for (p = &FIRST_PROC; p <= &LAST_PROC; p++) {
			if (p->p_flags == 0) {
				if (p->ticks > greatest_ticks) {
					greatest_ticks = p->ticks;
					p_proc_ready = p;
				}
			}
		}

		if (!greatest_ticks)
			for (p = &FIRST_PROC; p <= &LAST_PROC; p++)
				if (p->p_flags == 0)
					p->ticks = p->priority;
	}
}


/*
功能：进程间发送和接受消息
备注：系统调用
*/
PUBLIC void sys_sendrec(int function, int src_dest, MESSAGE *m, PROCESS *p)
{
	assert(k_reenter == 0);/*只能在ring1～3中进行该系统调用*/
	assert((src_dest >= 0 && src_dest < NR_TASKS + NR_PROCS) || /*源或目的 来自 任务或者用户进程*/
		src_dest == ANY || src_dest == INTERRUPT);/*源或目的 来自 中断或任意进程*/

	int ret = 0;
	int caller = proc2pid(p);
	MESSAGE *mla = (MESSAGE*)va2la(caller, m);
	mla->source = caller;

	if(SEND == function)
	{
		ret = msg_send(p, src_dest, m);
		if(0 != ret)
			return ret;

	}
	else if(RECEIVE == function)
	{
		ret = msg_receive(p, src_dest, m);
		if(0 != ret)
			return ret;
	}
	else
	{
		panic("{sys_sendrec} invalid function:%d (SEND:%d RECEIVE:%d)", function, SEND, RECEIVE);
	}
	return 0;
}


/*
功能：虚拟转换为线性地址
备注：本系统的ds段为flat（base为0），且在建立页表时，线性地址和物理地址时一一对应关系
	故本系统中 虚拟地址 = 线性地址 = 物理地址
*/
PUBLIC void* va2la(int pid, void* va)
{
	PROCESS *p = &proc_table[pid];
	u32 la = (u32)va + ldt_seg_linear(p, INDEX_LDT_RW);

	if(pid < NR_TASKS + NR_PROCS)
	{
		assert(la == (u32)va);
	}
	return (void*)la;
}


/*
功能：获取进程ldt中第idx段的基地址
*/
PUBLIC int ldt_seg_linear(PROCESS *p, int idx)
{
	DESCRIPTOR *d = &p->ldts[idx];
	return d->base_high << 24 | d->base_mid << 16 | d->base_low;
}


/*
功能：消息发送
返回值：0-成功
备注：
*/
PUBLIC int msg_send(PROCESS *current, int dst, MESSAGE *m)
{
	PROCESS *sender = current;
	PROCESS *p_dst = proc_table + dst;

	/*禁止向自身发送消息*/
	assert(proc2pid(sender) != dst);

	/*检查是否发生死锁*/
	if(0 != deadlock(proc2pid(sender), dst))
	{
		panic(">>deadlock<<  %s->%s", sender->p_name, p_dst->p_name);
	}

	/*消息检查*/
	if((p_dst->p_flags & RECEIVING) &&/*目的进程接收消息*/
		(p_dst->p_recvfrom == ANY ||
			p_dst->p_recvfrom == proc2pid(sender)))
	{
		assert(p_dst->p_msg);
		assert(m);

		phys_copy(va2la(dst, p_dst->p_msg),
			va2la(proc2pid(sender),m),
			sizeof(MESSAGE));
		p_dst->p_msg = 0;
		p_dst->p_flags &= ~RECEIVING;
		p_dst->p_recvfrom = NO_TASK;
		unblock(p_dst);

		assert(p_dst->p_flags == 0);
		assert(p_dst->p_msg == 0);
		assert(p_dst->p_recvfrom == NO_TASK);
		assert(p_dst->p_sendto == NO_TASK);

		assert(sender->p_flags == 0);
		assert(sender->p_msg == 0);
		assert(sender->p_recvfrom == NO_TASK);
		assert(sender->p_sendto == NO_TASK);
	}
	else/*目的进程没有接收该消息，或在接受其他进程消息*/
	{
		PROCESS *p = p_dst->q_sending;

		sender->p_flags |= SENDING;
		sender->p_sendto = dst;
		sender->p_msg = m;

		if(p_dst->q_sending)
		{
			/*找到发送进程链表尾部*/
			while(p->next_sending)
			{
				p = p->next_sending;
			}
			p->next_sending = sender;
		}
		else
		{
			p_dst->q_sending = sender;
		}
		sender->next_sending = 0;

		/*该函数激活调度，系统中断返回后将激活其他进程*/
		block(sender);

		assert(sender->p_flags == SENDING);
		assert(sender->p_msg != 0);
		assert(sender->p_sendto == dst);
		assert(sender->p_recvfrom == NO_TASK);
	}

	return 0;
}


/*
功能：消息接受
返回值：0-成功
*/
PUBLIC int msg_receive(PROCESS *current, int src, MESSAGE *m)
{
	PROCESS *p_who_wanna_rcv = current;
	PROCESS *p_from = 0;
	PROCESS *prev = 0;
	int copyok = 0;

	/*进程不能接受来自自身的消息*/
	assert(proc2pid(p_who_wanna_rcv) != src);

	/*首先处理中断*/
	if((p_who_wanna_rcv->has_int_msg) &&
		(src == ANY || src == INTERRUPT))
	{
		MESSAGE msg;

		reset_msg(&msg);
		msg.source = INTERRUPT;
		msg.type = HARD_INT;
		assert(m);

		phys_copy(va2la(proc2pid(p_who_wanna_rcv), m),
			&msg, sizeof(MESSAGE));
		p_who_wanna_rcv->has_int_msg = 0;

		assert(p_who_wanna_rcv->p_flags == 0);
		assert(p_who_wanna_rcv->p_sendto == NO_TASK);
		assert(p_who_wanna_rcv->has_int_msg == 0);
		assert(p_who_wanna_rcv->p_msg == 0);

		return 0;
	}

	/*处理非中断类消息,检查源进程*/
	if(src == ANY)
	{	
		if(p_who_wanna_rcv->q_sending)/*接受进程队列非空*/
		{
			p_from = p_who_wanna_rcv->q_sending;
			copyok = 1;

			assert(p_who_wanna_rcv->p_flags == 0);
			assert(p_who_wanna_rcv->p_msg == 0);
			assert(p_who_wanna_rcv->q_sending != 0);
			assert(p_who_wanna_rcv->p_sendto == NO_TASK);
			assert(p_who_wanna_rcv->p_recvfrom == NO_TASK);

			assert(p_from->p_flags == SENDING);
			assert(p_from->p_sendto == proc2pid(p_who_wanna_rcv));
			assert(p_from->p_recvfrom == NO_TASK);
			assert(p_from->p_msg != 0);
		}
	}
	else/*接收指定进程消息*/
	{
		p_from = &proc_table[src];

		if((p_from->p_flags & SENDING) && 
			p_from->p_sendto == proc2pid(p_who_wanna_rcv))
		{
			PROCESS *p = p_who_wanna_rcv->q_sending;

			copyok = 1;

			assert(p);

			/*将p_from从队列中出列*/
			while(p)
			{
				assert(p_from->p_flags & SENDING);

				if(proc2pid(p) == src)
				{	
					p_from = p;/*这句话貌似有点多余*/
					break;
				}
				prev = p;
				p = p->next_sending;
			}

			assert(p_who_wanna_rcv->p_flags == 0);
			assert(p_who_wanna_rcv->p_msg == 0);
			assert(p_who_wanna_rcv->q_sending != 0);
			assert(p_who_wanna_rcv->p_sendto == NO_TASK);
			assert(p_who_wanna_rcv->p_recvfrom == NO_TASK);

			assert(p_from->p_flags == SENDING);
			assert(p_from->p_msg != 0);
			assert(p_from->p_sendto == proc2pid(p_who_wanna_rcv));
			assert(p_from->p_recvfrom == NO_TASK);

		}
	}

	if(copyok)
	{
		if(p_from == p_who_wanna_rcv->q_sending)/*第一个*/
		{
			p_who_wanna_rcv->q_sending = p_from->next_sending;
			p_from->next_sending = 0;
		}
		else
		{
			assert(prev);
			prev->next_sending = p_from->next_sending;
			p_from->next_sending = 0;
		}

		assert(m);
		assert(p_from->p_msg);
		phys_copy(va2la(proc2pid(p_who_wanna_rcv), m),
			va2la(proc2pid(p_from), p_from->p_msg),
			sizeof(MESSAGE));

		p_from->p_msg = 0;
		p_from->p_flags &= ~SENDING;
		p_from->p_sendto = NO_TASK;
		unblock(p_from);
	}
	else	
	{
		p_who_wanna_rcv->p_flags |= RECEIVING;
		p_who_wanna_rcv->p_msg = m;

		if(src == ANY)
		{
			p_who_wanna_rcv->p_recvfrom = ANY;
		}
		else
		{
			p_who_wanna_rcv->p_recvfrom = proc2pid(p_from);
		}

		block(p_who_wanna_rcv);

		assert(p_who_wanna_rcv->p_flags == RECEIVING);
		assert(p_who_wanna_rcv->p_msg != 0);
		assert(p_who_wanna_rcv->p_recvfrom != NO_TASK);
		assert(p_who_wanna_rcv->p_sendto == NO_TASK);
		assert(p_who_wanna_rcv->has_int_msg == 0);
	}

	return 0;
}


/*
功能：清空消息
*/
PUBLIC void reset_msg(MESSAGE *m)
{
	memset((char*)m, 0, sizeof(MESSAGE));
}


/*
功能：进程阻塞，无实际作用
*/
PUBLIC void block(PROCESS *p)
{
	assert(p->p_flags);
	schedule();
}


/*
功能：解除进程阻塞，无实际作用
*/
PUBLIC void unblock(PROCESS *p)
{
	assert(0 == p->p_flags);
}


/*
功能：检测是否有死锁发生
返回值：0-无死锁  1-有死锁
备注：有类似A->B->C->A的结构即发生死锁, 每次发送消息时均检查是否有环发生
*/
PUBLIC int deadlock(int src, int dest)
{
	PROCESS *p = proc_table + dest;
	while(1)
	{
		if(p->p_flags & SENDING)
		{
			if(src == p->p_sendto)/*有环发生*/
			{
				p = proc_table + src;
				printl("=_=%s", p->p_name);
				do{
					assert(p->p_msg);/*有消息未发出*/
					p = &proc_table[p->p_sendto];
					printl("->%s", p->p_name);
				}while(p != proc_table + src);
				printl("=_=");

				return 1;
			}
			p = &proc_table[p->p_sendto];
		}
		else
		{
			break;
		}
	}
	return 0;
}


/*
功能：进程间进行ipc的接口
返回值：0成功
*/
PUBLIC int send_rec(int function, int src_dest, MESSAGE *m)
{
	int ret = 0;

	if(RECEIVE == function)
		memset((char*)m, 0, sizeof(MESSAGE));

	switch(function)
	{
		case BOTH:
			ret = sendrec(SEND, src_dest, m);
			if(0 == ret)
				ret = sendrec(RECEIVE, src_dest, m);
			break;
		case RECEIVE:
		case SEND:
			ret = sendrec(function, src_dest, m);
			break;
		default:
			assert((function == BOTH) ||
				(function == SEND) || (function == RECEIVE));
			break;
	}
	return ret;
}


/*
功能：通知一个任务产生了一个中断
备注：运行在ring0
*/
PUBLIC void inform_int(int task_nr)
{
	PROCESS *p = proc_table + task_nr;

	if(p->p_flags & RECEIVING &&
		(p->p_recvfrom == INTERRUPT || p->p_recvfrom == ANY))
	{
		p->p_msg->source = INTERRUPT;
		p->p_msg->type = HARD_INT;
		p->p_msg = 0;
		p->has_int_msg = 0;
		p->p_flags &= ~RECEIVING;/*清除接收标志*/
		p->p_recvfrom = NO_TASK;

		assert(p->p_flags == 0)
		unblock(p);

		assert(p->p_recvfrom == NO_TASK);
		assert(p->p_flags == 0);
		assert(p->p_sendto == NO_TASK);
	}
	else
	{
		p->has_int_msg = 1;
	}
}

