#include "const.h"
#include "type.h"
#include "protect.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "fs.h"
#include "global.h"
#include "proto.h"

PRIVATE void set_cursor(u16 cur_disp_pos);
PRIVATE void flush(CONSOLE *p_con);


/*
功能：初始化终端对应的屏幕信息
*/
PUBLIC void init_screen(TTY *p_tty)
{
	u32 console_no = p_tty - tty_table;
	u32 v_mem_size = V_MEM_SIZE >> 1;/*in word*/
	u32 mem_size = v_mem_size / NR_TTYS;
	p_tty->p_con = console_table + console_no;

	p_tty->p_con->orig_start_addr =  mem_size * console_no;/*这地方不用考虑基地址*/
	p_tty->p_con->v_mem_size = mem_size;
	p_tty->p_con->v_mem_base = V_MEM_BASE;
	p_tty->p_con->cur_start_addr = p_tty->p_con->orig_start_addr;
	p_tty->p_con->cur_cursor_pos = p_tty->p_con->cur_start_addr;

	if(0 == console_no)
	{
		p_tty->p_con->cur_cursor_pos = disp_pos / 2;
		disp_pos = 0;
	}
	else
	{
		out_char(console_no + '0', p_tty->p_con);
		out_char('#', p_tty->p_con);
	}

	//set_cursor(p_tty->p_con->cur_cursor_pos);
}


/*
功能：设置光标位置
*/
PRIVATE void set_cursor(u16 cur_disp_pos)
{
	disable_int();

    //告诉地址寄存器要接下来要使用14号寄存器
    out_byte(ADDR_REG, CUR_POS_HIGH);

    //向光标位置高位寄存器写入值
    out_byte(DATA_REG, (cur_disp_pos >> 8) & 0xff);

    //告诉地址寄存器要接下来要使用15号寄存器
    out_byte(ADDR_REG, CUR_POS_LOW);

    //向光标位置低位寄存器写入值
    out_byte(DATA_REG, cur_disp_pos & 0xff);

    enable_int();
}


/*
功能：设置屏幕起始位置
*/
PUBLIC void set_screen_start(u32 addr)
{
	disable_int();

	//告诉地址寄存器要接下来要使用14号寄存器
    out_byte(ADDR_REG, START_ADDR_HIGH);

    //向光标位置高位寄存器写入值
    out_byte(DATA_REG, (addr >> 8) & 0xff);

    //告诉地址寄存器要接下来要使用15号寄存器
    out_byte(ADDR_REG, START_ADDR_LOW);

    //向光标位置低位寄存器写入值
    out_byte(DATA_REG, addr & 0xff);

    enable_int();
}


/*
功能：判断是否为当前控制台
*/
PUBLIC int is_cur_console(CONSOLE *p_con)
{
	if(p_con == &console_table[nr_cur_console])
	{
		return 1;
	}
	return 0;
}


/*
功能：在相应的控制台上输出字符
*/
PUBLIC void out_char(u8 ch, CONSOLE *p_con)
{
	u8 *p_mem;

	switch(ch)/*特殊符号均均手动处理*/
	{
		case '\n':
			if(p_con->cur_cursor_pos < p_con->orig_start_addr + p_con->v_mem_size - SCREEN_WIDTH)
			{
				p_con->cur_cursor_pos = p_con->orig_start_addr + ((p_con->cur_cursor_pos - p_con->orig_start_addr)/ SCREEN_WIDTH + 1) * SCREEN_WIDTH;
			}
			break;
		case '\b':
			if(p_con->cur_cursor_pos > p_con->orig_start_addr)
			{
				p_con->cur_cursor_pos--;
				p_mem = (u8*)(p_con->cur_cursor_pos * 2 + p_con->v_mem_base);/*u8类型指针 不能用u32类型指针*/
				*p_mem = ' ';
				*(p_mem + 1) = DEFAULT_CHAR_COLOR;
			}
			break;
		default:
			if(p_con->cur_cursor_pos < p_con->orig_start_addr + p_con->v_mem_size - 1)
			{
				p_mem = (u8*)(p_con->cur_cursor_pos * 2 + p_con->v_mem_base);/*u8类型指针 不能用u32类型指针*/
				*p_mem = ch;
				*(p_mem + 1) = DEFAULT_CHAR_COLOR;
				p_con->cur_cursor_pos++;
			}
			break;
	}
	while(p_con->cur_cursor_pos >= p_con->cur_start_addr + SCREEN_SIZE)
	{
		scroll_screen(p_con, SCR_DOWN); 
	}
	flush(p_con);
}


/*
功能：设置当前控制台
*/
PUBLIC void set_cur_con(int con_no)
{
	if(con_no < 0 || con_no >= NR_TTYS)/*无效控制台号*/
	{
		return;
	}

	nr_cur_console = con_no;
	set_cursor(console_table[con_no].cur_cursor_pos);
	set_screen_start(console_table[con_no].cur_start_addr);

	//out_char(con_no + '0', &console_table[con_no]);
	//out_char('#', &console_table[con_no]);
}


/*
功能：向上或向下滚动屏幕
*/
PUBLIC void scroll_screen(CONSOLE *p_con, int direction)
{
	if(SCR_DOWN != direction && SCR_UP != direction)
	{
		return;
	}
	if(SCR_DOWN == direction)
	{
		if(p_con->cur_start_addr + SCREEN_SIZE < p_con->orig_start_addr + p_con->v_mem_size)
		{
			p_con->cur_start_addr += SCREEN_WIDTH;
		}
	}
	else
	{
		if(p_con->cur_start_addr > p_con->orig_start_addr)
		{
			p_con->cur_start_addr -= SCREEN_WIDTH;
		}
	}
	set_screen_start(p_con->cur_start_addr);
	set_cursor(p_con->cur_cursor_pos);
}


/*
功能：刷新屏幕
*/
PRIVATE void flush(CONSOLE *p_con)
{
	set_cursor(p_con->cur_cursor_pos);
	set_screen_start(p_con->cur_start_addr);
}