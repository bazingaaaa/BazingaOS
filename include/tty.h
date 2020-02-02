#ifndef _TTY_H_
#define _TTY_H_

#define TTY_IN_BYTES 256

typedef struct s_tty
{
	u32 in_buf[TTY_IN_BYTES];
	u32 *p_inbuf_head;
	u32 *p_inbuf_tail;
	int inbuf_count;

	struct s_console *p_con;
}TTY;


#endif