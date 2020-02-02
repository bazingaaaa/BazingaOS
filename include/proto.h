PUBLIC void disp_str(char* str);
PUBLIC void disp_color_str(char* str, int text_color);
PUBLIC void out_byte(u16 port, u8 val);
PUBLIC u8 in_byte(u16 port);
PUBLIC void init_8259A();
PUBLIC void init_idt();


PUBLIC void port_read(u16 port, void* buf, int n);
PUBLIC void disable_irq(int irq);
PUBLIC void enbale_irq(int irq);
PUBLIC void put_irq_handler(int irq, irq_handler handler);

PUBLIC void testA();
PUBLIC void testB();
PUBLIC void testC();

PUBLIC void spurious_irq(int irq);
PUBLIC void clock_handler(int iqr);
PUBLIC void milli_delay(int milli_sec);

PUBLIC void init_clock();
PUBLIC void init_keyboard();
PUBLIC void keyboard_read(TTY *p_tty);


/*proc.c*/
PUBLIC int sys_get_ticks();
PUBLIC void* va2la(int pid, void* va);
PUBLIC void schedule();
PUBLIC int ldt_seg_linear(PROCESS *p, int idx);
PUBLIC void sys_sendrec(int function, int src_dest, MESSAGE *m, PROCESS *p);
PUBLIC int msg_send(PROCESS *p, int dst, MESSAGE *m);
PUBLIC int msg_receive(PROCESS *p, int dst, MESSAGE *m);
PUBLIC int deadlock(int src, int dest);
PUBLIC void reset_msg(MESSAGE *m);
PUBLIC void block(PROCESS *p);
PUBLIC void unblock(PROCESS *p);
PUBLIC int send_rec(int function, int src_dest, MESSAGE *m);
PUBLIC void inform_int(int task_nr);


/*syscall*/
PUBLIC int sendrec(int function, int src_dest, MESSAGE *m);
PUBLIC int printx(char* buf);

/*tty.c*/
PUBLIC void task_tty();
PUBLIC void in_process(u32 key, TTY *p_tty);
PUBLIC void sys_write(char* buf, int len, PROCESS *p_proc);
PUBLIC void sys_printx(int unused1, int unused2, char* s, PROCESS *proc);



/*console.c*/
PUBLIC void out_char(u8 ch, CONSOLE *p_con);
PUBLIC void set_cur_con(int con_no);
PUBLIC int is_cur_console(CONSOLE *p_con);
PUBLIC void out_char(u8 ch, CONSOLE *p_con);
PUBLIC void set_cur_con(int con_no);
PUBLIC void init_screen(TTY *p_tty);
PUBLIC void scroll_screen(CONSOLE *p_con, int direction);

/*vsprintf.c*/
PUBLIC int vsprintf(char* buf, const char* fmt, var_list args);

/*printf.c*/
PUBLIC void printf(char *fmt, ...);
#define	printl	printf


/*misc.h*/
PUBLIC void assertion_failure(char *exp, char *file, char *base_file, int line);

/*main.c*/
PUBLIC void panic(char *fmt, ...);
PUBLIC void spin(char *func_name);

/*systask.c*/
PUBLIC void task_sys();


/*hd.c*/
PUBLIC void task_hd();

/*fs/main.c*/
PUBLIC void task_fs();

