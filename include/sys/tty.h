#ifndef _TTY_H_
#define _TTY_H_

#define TTY_IN_BYTES 256
#define TTY_OUT_IN_BYTES 2

typedef struct s_tty
{
	u32 in_buf[TTY_IN_BYTES];
	u32 *p_inbuf_head;
	u32 *p_inbuf_tail;
	int inbuf_count;

	/*下列信息在对tty进行读取时使用*/
	int tty_caller;/*调用tty的任务*/
	int tty_pro_nr;/*需要进行输入的进程*/
	void *tty_read_buf;/*读入数据存放的物理地址*/
	int tty_bytes_left;/*剩余需读取字节数*/
	int tty_trans_bytes;/*已经转移的字节数*/

	struct s_console *p_con;
}TTY;


#endif