PUBLIC void disp_str(char* str);
PUBLIC void disp_color_str(char* str, int text_color);
PUBLIC void out_byte(u16 port, u8 val);
PUBLIC u8 in_byte(u16 port);
PUBLIC void init_8259A();
PUBLIC void init_idt();


PUBLIC void port_read(u16 port, void* buf, int n);
PUBLIC void port_write(u16 port, void* buf, int n);
PUBLIC void disable_irq(int irq);
PUBLIC void enbale_irq(int irq);
PUBLIC void put_irq_handler(int irq, irq_handler handler);

PUBLIC void testA();
PUBLIC void testB();
PUBLIC void testC();
PUBLIC void Init();

PUBLIC void spurious_irq(int irq);
PUBLIC void clock_handler(int iqr);
PUBLIC void milli_delay(int milli_sec);

PUBLIC void init_clock();
PUBLIC void init_keyboard();
PUBLIC void keyboard_read(TTY *p_tty);


/*protect.c*/
PUBLIC void init_descriptor(DESCRIPTOR *pDes, u32 base, u32 limit, u16 attr);

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
PUBLIC void sys_printx(int unused1, int unused2, char* s, PROCESS *proc);



/*console.c*/
PUBLIC void out_char(u8 ch, CONSOLE *p_con);
PUBLIC void set_cur_con(int con_no);
PUBLIC int is_cur_console(CONSOLE *p_con);
PUBLIC void out_char(u8 ch, CONSOLE *p_con);
PUBLIC void set_cur_con(int con_no);
PUBLIC void init_screen(TTY *p_tty);
PUBLIC void scroll_screen(CONSOLE *p_con, int direction);


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
PUBLIC struct super_block* get_super_block(int dev);
PUBLIC struct inode* get_inode(int dev, int inode_nr);
PUBLIC void put_inode(struct inode *pNode);
PUBLIC void sync_inode(struct inode *pNode);
PUBLIC int rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf);

/*fs/misc.c*/
PUBLIC int strip_path(char* filename, struct inode **ppinode, const char* pathname);

/*lib/misc.c*/
PUBLIC int strcmp(const char* s1, const char* s2);
PUBLIC int memcmp(const void *s1, const void *s2, int n);


/*mm/main.c*/
PUBLIC void task_mm();


/*lib/klib.c*/
PUBLIC int get_kernel_map(unsigned int * b, unsigned int * l);
PUBLIC void get_boot_param(struct boot_params* pbp);

/*mm/main.c*/
PUBLIC int alloc_mem(int pid, int mem_size);

/*mm/forkexit.c*/
PUBLIC int do_fork(MESSAGE *msg);
PUBLIC void do_exit(MESSAGE *msg);
PUBLIC void do_wait(MESSAGE *msg);