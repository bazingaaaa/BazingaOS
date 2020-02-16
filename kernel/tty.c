#include "const.h"
#include "type.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "fs.h"
#include "global.h"
#include "keyboard.h"
#include "string.h"

PRIVATE void tty_dev_read(TTY *p_tty);
PRIVATE void tty_dev_write(TTY *p_tty);
PRIVATE void init_tty();
PRIVATE void put_key(TTY *p_tty, u32 key);
PRIVATE void tty_do_write(TTY *p_tty, MESSAGE *msg);
PRIVATE void tty_do_read(TTY *p_tty, MESSAGE *msg);


/*
功能：终端驱动
*/
PUBLIC void task_tty()
{
	MESSAGE msg;

	/*初始化键盘*/
	init_keyboard();

	/*初始化终端*/
	init_tty();

	/*设置当前屏幕*/
	set_cur_con(0);

	while(1)
	{
		TTY *p_tty;
		for(p_tty = tty_table;p_tty < tty_table + NR_TTYS;p_tty++)
		{
			do
			{
				/*从键盘驱动读出字符进入终端缓存区*/
				tty_dev_read(p_tty);

				/*将终端缓存区中的字符输出*/
				tty_dev_write(p_tty);
			}while(p_tty->inbuf_count > 0);
		}

		send_rec(RECEIVE, ANY, &msg);

		int src = msg.source;
		/*确定需要处理该消息的终端*/
		p_tty = &tty_table[msg.DEVICE];

		switch(msg.type)
		{
			case DEV_OPEN:
				reset_msg(&msg);
				msg.type = SYSCALL_RET;
				send_rec(SEND, src, &msg);
				break;
			case DEV_READ:
				tty_do_read(p_tty, &msg);
				break;
			case DEV_WRITE:
				tty_do_write(p_tty, &msg);
				break;
			case HARD_INT:/*由时钟中断触发，避免TTY阻塞在等待消息处，
							而不能及时获取键盘缓存区中的内容*/
				key_pressed = 0;
				break;
			default:
				panic("tty unknown msg type");
				break;
		}

	}	
}


/*
功能：
*/
PUBLIC void in_process(u32 key, TTY *p_tty)
{
	if(!(key & FLAG_EXT))
	{
		//disp_int(key);
		put_key(p_tty, key);
	}
	else
	{
		//disp_int(key);
		int raw_code = key & MASK_RAW;

		switch(raw_code)
		{
			case ENTER:
				put_key(p_tty, '\n');
				break;
			case BACKSPACE:
				put_key(p_tty, '\b');
				break;	
			case F1:
			case F2:
			case F3:
			case F4:
			case F5:
			case F6:
			case F7:
			case F8:
			case F9:
				set_cur_con(key - F1);/*mac的F1,F2等功能键用法和一般键盘不同，此处把ctrl去掉*/
				break;
			case UP:
				if((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
				{
					scroll_screen(p_tty->p_con, SCR_UP);
				}
				break;
			case DOWN:
				if((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
				{
					scroll_screen(p_tty->p_con, SCR_DOWN);
				}
				break;
		}
	}
}


/*
功能：终端从键盘读取输入字符
*/
PRIVATE void tty_dev_read(TTY *p_tty)
{
	if(!is_cur_console(p_tty->p_con))/*只有当前的终端才读入键盘输入*/
	{
		return;
	}
	keyboard_read(p_tty);
}


/*
功能：将缓存区中内容写入到屏幕
*/
PRIVATE void tty_dev_write(TTY *p_tty)
{
	while(p_tty->inbuf_count > 0)
	{
		u8 ch = *p_tty->p_inbuf_tail;
		p_tty->p_inbuf_tail++;
		if(p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES)
		{
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;

		if(p_tty->tty_bytes_left > 0)/*有读取该tty文件的请求*/
		{
			if(ch >= ' ' && ch <= '~')/*可显示的*/
			{
				out_char(ch, p_tty->p_con);
				phys_copy(p_tty->tty_read_buf + p_tty->tty_trans_bytes, va2la(TASK_TTY, &ch), sizeof(u8));
				p_tty->tty_bytes_left--;
				p_tty->tty_trans_bytes++;
			}
			else if(ch == '\b')/*退格键*/
			{
				out_char(ch, p_tty->p_con);
				p_tty->tty_bytes_left++;
				p_tty->tty_trans_bytes--;
			}
			if(0 == p_tty->tty_bytes_left || ch == '\n')/*读取完毕,或遇到换行符*/
			{
				out_char('\n', p_tty->p_con);
				MESSAGE msg;
				msg.type = RESUME_PROC;
				msg.PROC_NR = p_tty->tty_pro_nr;
				msg.CNT = p_tty->tty_trans_bytes;
				send_rec(SEND, p_tty->tty_caller, &msg);
				p_tty->tty_bytes_left = 0;
			}
		}
	}
}


/*
功能：初始化终端
*/
PRIVATE void init_tty()
{
	TTY *p_tty;
	for(p_tty = tty_table;p_tty < tty_table + NR_TTYS;p_tty++)
	{
		/*初始化缓存区*/
		p_tty->p_inbuf_head = p_tty->in_buf;
		p_tty->p_inbuf_tail = p_tty->in_buf;
		p_tty->inbuf_count = 0;

		/*初始化对应的控制台*/
		init_screen(p_tty);
	}
}


/*
功能：添加key
*/
PRIVATE void put_key(TTY *p_tty, u32 key)
{
	if(p_tty->inbuf_count < TTY_IN_BYTES)
	{
		*p_tty->p_inbuf_head = key;
		p_tty->p_inbuf_head++;
		if(p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES)
		{
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}


/*
功能：把字符缓存输出到对应的终端
*/
PUBLIC void tty_write(TTY *p_tty, char* buf, int len)
{
	int i;

	for(i = 0;i < len;i++)
	{
		out_char(buf[i], p_tty->p_con);
	}
}


/*
功能：进程将缓存区中的内容输出至对应的控制台
备注：系统调用
*/
PUBLIC void sys_write(char* buf, int len, PROCESS *p_proc)
{
	TTY *p_tty = &tty_table[p_proc->nr_tty];
	tty_write(p_tty, buf, len);
}


/*
功能：输出缓存区内容至控制台
备注：新的打印函数，需要区分ring1～3和ring0，并且需要检查打印的字符串中是否有magic no，如果有的话需要进行
	异常处理, 
*/
PUBLIC void sys_printx(int unused1, int unused2, char* s, PROCESS *proc)
{
	const char *p;
	char ch;

	char reenter_err[] = "? k_reenter is incorrect for unknown reason";
	reenter_err[0] = MAG_CH_PANIC;

	if(k_reenter == 0)/*ring1~3*/
	{
		p = va2la(proc2pid(proc), s);
	}
	else if(k_reenter > 0)/*ring0*/
	{
		p = s;
	}
	else/*shouldn't reach here*/
	{
		p = reenter_err;
	}

	/*异常检查,如果magic no为panic或者任务级的assert failure会导致系统陷入死循环*/
	if((*p == MAG_CH_PANIC) || 
		(*p == MAG_CH_ASSERT && p_proc_ready < &proc_table[NR_TASKS]))
	{
		disable_int();

		char *v = (char*)V_MEM_BASE;
		const char *q = p + 1;

		while(v < V_MEM_BASE + V_MEM_SIZE)
		{
			*v++ = *q++;
			*v++ = RED_CHAR;

			if(!*q)
			{
				while((int)(v - V_MEM_BASE) % (SCREEN_WIDTH * 16))
				{
					v++;
					*v++ = GRAY_CHAR;
				}
				q = p + 1;
			}
		}
		__asm__ __volatile__("hlt");
	}

	while((ch = *p++) != 0)
	{
		if(*p == MAG_CH_PANIC || *p == MAG_CH_ASSERT)
		{	
			continue;
		}
		out_char(ch, tty_table[proc->nr_tty].p_con);
	}
}


/*
功能:写入TTY文件
*/
PRIVATE void tty_do_write(TTY *p_tty, MESSAGE *msg)
{
	char buf[TTY_OUT_IN_BYTES];
	int bytes_cnt = msg->CNT;
	int src = msg->source;
	int proc_nr = msg->PROC_NR;
	int bytes_trans = 0;
	int i;

	while(bytes_cnt > 0)
	{
		int bytes = MIN(TTY_OUT_IN_BYTES, bytes_cnt);
		phys_copy(va2la(TASK_TTY, buf), va2la(proc_nr, msg->BUF + bytes_trans), bytes);
		for(i = 0;i < bytes;i++)
		{
			out_char(buf[i], p_tty->p_con);
		}
		bytes_cnt -= bytes;
		bytes_trans += bytes;
	}
	msg->CNT = bytes_trans;
	send_rec(SEND, src, msg);
}


/*
功能：从TTY文件中读出数据
备注：一次只能有一个进程读取tty文件
*/
PRIVATE void tty_do_read(TTY *p_tty, MESSAGE *msg)
{
	p_tty->tty_caller = msg->source;
	p_tty->tty_pro_nr = msg->PROC_NR;
	p_tty->tty_read_buf = va2la(msg->PROC_NR, msg->BUF);
	p_tty->tty_bytes_left = msg->CNT;
	p_tty->tty_trans_bytes = 0;

	msg->type = SUSPEND_PROC;/*将请求终端输出数据的进程挂起*/
	msg->CNT = p_tty->tty_bytes_left;
	send_rec(SEND, p_tty->tty_caller, msg);
}	