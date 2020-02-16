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
#include "keymap.h"


PRIVATE KB_INPUT kb_in;

PRIVATE int code_with_E0;
PRIVATE int shift_l;
PRIVATE int shift_r;
PRIVATE int alt_l;
PRIVATE int alt_r;
PRIVATE int ctrl_l;
PRIVATE int ctrl_r;

/*leds*/
PRIVATE int caps_lock;
PRIVATE int num_lock;
PRIVATE int scroll_lock;

PRIVATE int column;


PRIVATE u8 get_byte_from_kbuf();
PRIVATE void set_leds();
PRIVATE void kb_ack();
PRIVATE void kb_wait();


/*
功能：键盘处理句柄
备注：键盘接受缓冲区中只存放KB_IN_BYTE字节的数据，超出部分舍去，运行在ring0
*/
PUBLIC void keyboard_handler(int irq)
{
	u8 scan_code = in_byte(0x60);

	if(kb_in.count < KB_IN_BYTE)
	{
		*(kb_in.p_head) = scan_code;
		kb_in.p_head++;
		if(kb_in.p_head == kb_in.buf + KB_IN_BYTE)
		{
			kb_in.p_head = kb_in.buf;
		}
		kb_in.count++;
	}
	key_pressed = 1;
}


/*
功能：初始化键盘
*/
PUBLIC void init_keyboard()
{
	/*初始化键盘缓存区*/
	kb_in.count = 0;
	kb_in.p_head = kb_in.buf;
	kb_in.p_tail = kb_in.buf;

	/*初始化各个按键状态*/
	code_with_E0 = 0;
	shift_l	= shift_r = 0;
	alt_l	= alt_r   = 0;
	ctrl_l	= ctrl_r  = 0;

	/*初始化键盘指示灯*/
	caps_lock = 0;
	num_lock = 1;
	scroll_lock = 0;

	set_leds();

	/*使能键盘中断请求*/
	put_irq_handler(KEYBOARD_INT_NO, keyboard_handler);
	enable_irq(KEYBOARD_INT_NO);
}



/*
功能：终端任务读取
备注：任务在ring1
*/
PUBLIC void keyboard_read(TTY *p_tty)
{
	u8 scan_code;
	int bMake;
	u32 *keyrow;
	u32 key = 0;

	/*memset(output, 0, 2);*/
	if(kb_in.count > 0)
	{
		int is_pausebreak = 1;
		code_with_E0 = 0;

		scan_code = get_byte_from_kbuf();

		if(0xE1 == scan_code)
		{
			int i;
			u8 pausebrk_scode[] = {
				0xE1, 0x1D, 0x45,
				0xE1, 0x9D, 0xC5
			};
			for(i = 0;i < 6;i++)
			{
				if(get_byte_from_kbuf() != pausebrk_scode[i])
				{
					is_pausebreak = 0;
					break;
				}
			}
			if(is_pausebreak)
			{
				key = PAUSEBREAK;
			}
		}
		else if(0xE0 == scan_code)
		{
			scan_code = get_byte_from_kbuf();

			if(0x2A == scan_code)
			{
				if(0xE0 == get_byte_from_kbuf())
				{
					if(0x37 == get_byte_from_kbuf())
					{
						key = PRINTSCREEN;
						bMake = 1;
					}
				}
			}
			if(0xB7 == scan_code)
			{
				if(0xE0 == get_byte_from_kbuf())
				{
					if(0xAA == get_byte_from_kbuf())
					{
						key = PRINTSCREEN;
						bMake = 0;
					}
				}
			}
			if(0 == key)
			{
				code_with_E0 = 1;
			}
		}
		if((PAUSEBREAK != key) && (PRINTSCREEN != key))
		{
			
			bMake = (scan_code & FLAG_BREAK) ? FALSE : TRUE;
			keyrow = &keymap[(scan_code & 0x7F) * MAP_COLS];

			column = 0;
			if(shift_l || shift_r)
			{
				column = 1;
			}
			if(code_with_E0)
			{
				column = 2;
				code_with_E0 = 0;
			}

			key = keyrow[column];

			//disp_int(scan_code);
			switch(key)
			{
				case SHIFT_L:
					shift_l = bMake;
					break;
				case SHIFT_R:
					shift_r = bMake;
					break;
				case CTRL_L:
					ctrl_l = bMake;
					break;
				case CTRL_R:
					ctrl_r = bMake;
					break;
				case ALT_L:
					alt_l = bMake;
					break;
				case ALT_R:
					alt_r = bMake;
					break;
				case CAPS_LOCK:
					caps_lock = 1;
					set_leds();
				case NUM_LOCK:
					num_lock = 1;
					set_leds();
				case SCROLL_LOCK:
					scroll_lock = 1;
					set_leds();
				default:
					break;
			}

			if(bMake)
			{
				int pad = 0;

				/* 首先处理小键盘 */
				if ((key >= PAD_SLASH) && (key <= PAD_9)) {
					pad = 1;
					switch(key) {
					case PAD_SLASH:
						key = '/';
						break;
					case PAD_STAR:
						key = '*';
						break;
					case PAD_MINUS:
						key = '-';
						break;
					case PAD_PLUS:
						key = '+';
						break;
					case PAD_ENTER:
						key = ENTER;
						break;
					default:
						if (num_lock &&
						    (key >= PAD_0) &&
						    (key <= PAD_9)) {
							key = key - PAD_0 + '0';
						}
						else if (num_lock &&
							 (key == PAD_DOT)) {
							key = '.';
						}
						else{
							switch(key) {
							case PAD_HOME:
								key = HOME;
								break;
							case PAD_END:
								key = END;
								break;
							case PAD_PAGEUP:
								key = PAGEUP;
								break;
							case PAD_PAGEDOWN:
								key = PAGEDOWN;
								break;
							case PAD_INS:
								key = INSERT;
								break;
							case PAD_UP:
								key = UP;
								break;
							case PAD_DOWN:
								key = DOWN;
								break;
							case PAD_LEFT:
								key = LEFT;
								break;
							case PAD_RIGHT:
								key = RIGHT;
								break;
							case PAD_DOT:
								key = DELETE;
								break;
							default:
								break;
							}
						}
						break;
					}
				}

				key |= shift_l ? FLAG_SHIFT_L : 0;
				key |= shift_r ? FLAG_SHIFT_R : 0;
				key |= ctrl_l ? FLAG_CTRL_L : 0;
				key |= ctrl_r ? FLAG_CTRL_R : 0;
				key |= alt_l ? FLAG_ALT_L : 0;
				key |= alt_r ? FLAG_ALT_R : 0;
				key |= pad ? FLAG_PAD : 0;
				//disp_int(key);
				in_process(key, p_tty);
			}
		}
	}
}


PRIVATE u8 get_byte_from_kbuf()
{
	u8 scan_code;

	while(kb_in.count <= 0)
	{}
	
	disable_int();/*对kb_in.buf进行访问保护*/
	scan_code = *(kb_in.p_tail);
	kb_in.p_tail++;
	if(kb_in.p_tail == kb_in.buf + KB_IN_BYTE)
	{
		kb_in.p_tail = kb_in.buf;
	}
	kb_in.count--;
	enable_int();
		
	return scan_code;
}


PRIVATE void kb_wait()
{
	u8 kb_stat;

	do{
		kb_stat = in_byte(KB_CMD);
	}while(kb_stat & 0x02);
}


PRIVATE void kb_ack()
{
	u8 kb_read;

	do{
		kb_read = in_byte(KB_DATA);
	}while(KB_ACK != kb_read);
}


PRIVATE void set_leds()
{
	u8 leds = caps_lock << 2 | num_lock << 1 | scroll_lock;

	kb_wait();
	out_byte(KB_DATA, LED_CODE);
	kb_ack();

	kb_wait();
	out_byte(KB_DATA, leds);
	kb_ack();
}
