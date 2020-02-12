#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#define ADDR_REG 0x3d4
#define DATA_REG 0x3d5

#define CUR_POS_LOW 0xf
#define CUR_POS_HIGH 0xe

#define START_ADDR_LOW 0xd
#define START_ADDR_HIGH 0xc

#define V_MEM_BASE 0xb8000
#define V_MEM_SIZE 0x8000
#define DEFAULT_CHAR_COLOR 0x07

#define SCR_DOWN 1
#define SCR_UP 2

#define SCREEN_SIZE (80 * 25)/*一页共25行*/
#define SCREEN_WIDTH 80/*一行80个字符*/

typedef struct s_console
{
	u32 orig_start_addr;
	u32 cur_start_addr;
	u32 v_mem_size;
	u32 v_mem_base;
	u32 cur_cursor_pos;
}CONSOLE;

#endif