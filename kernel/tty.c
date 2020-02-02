#include "const.h"
#include "type.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "global.h"
#include "keyboard.h"

PRIVATE void tty_do_read(TTY *p_tty);
PRIVATE void tty_do_write(TTY *p_tty);
PRIVATE void init_tty();
PRIVATE void put_key(TTY *p_tty, u32 key);


/*
功能：
*/
PUBLIC void task_tty()
{
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
			
			tty_do_read(p_tty);

			tty_do_write(p_tty);
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
功能：
*/
PRIVATE void tty_do_read(TTY *p_tty)
{
	if(!is_cur_console(p_tty->p_con))/*只有当前的终端才读入键盘输入*/
	{
		return;
	}
	keyboard_read(p_tty);
}


/*
功能：
*/
PRIVATE void tty_do_write(TTY *p_tty)
{
	if(p_tty->inbuf_count > 0)
	{
		u8 ch = *p_tty->p_inbuf_tail;
		p_tty->p_inbuf_tail++;
		if(p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES)
		{
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;

		out_char(ch, p_tty->p_con);
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

