#include "const.h"
#include "type.h"
#include "protect.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "fs.h"
#include "global.h"
#include "proto.h"

/*异常中断处理*/
void	divide_error();
void	single_step_exception();
void	nmi();
void	breakpoint_exception();
void	overflow();
void	bounds_check();
void	inval_opcode();
void	copr_not_available();
void	double_fault();
void	copr_seg_overrun();
void	inval_tss();
void	segment_not_present();
void	stack_exception();
void	general_protection();
void	page_fault();
void	copr_error();


/*外部中断，对应8259A的IRQ0～15*/
void 	hwint00();
void 	hwint01();
void 	hwint02();
void 	hwint03();
void 	hwint04();
void 	hwint05();
void 	hwint06();
void 	hwint07();
void 	hwint08();
void 	hwint09();
void 	hwint10();
void 	hwint11();
void 	hwint12();
void 	hwint13();
void 	hwint14();
void 	hwint15();

void 	sys_call();


u32 seg2addr(u16 seg);

/*
功能：初始化中断描述符
备注：中断描述符表全局唯一，可以通过传入通道号的方式进行初始化
*/
PRIVATE void init_idt_desc(u8 vector, u8 desc_type, int_handler handler,u8 privilege)
{
	GATE *pGate = &idt[vector];
	u32 base = (u32)handler;

	pGate->offset_low = base & 0xFFFF;
	pGate->selector = SELECTOR_KERNEL_CS;
	pGate->dcount = 0;
	pGate->attr = (privilege << 5) | desc_type;
	pGate->offset_high = base >> 16 & 0xFFFF;
}


/*
功能：初始化段描述符
*/
PUBLIC void init_descriptor(DESCRIPTOR *pDes, u32 base, u32 limit, u16 attr)
{
	pDes->limit_low = limit & 0xffff;
	pDes->base_low = base & 0xffff;
	pDes->base_mid = base>>16 & 0xff;/*16～23*/
	pDes->attrl = attr & 0xff;
	pDes->limit_high_attr2 = (attr>>8 & 0xf0) | (limit>>16 & 0xf);
	pDes->base_high = base>>24 & 0xff;
}



/*
功能：保护模式相关初始化
*/
void init_prot()
{
	/*初始化8259a中断控制器*/
	init_8259A();

	/*异常处理*/
	init_idt_desc(INT_VECTOR_DIVIDE,	DA_386IGate,
		      divide_error,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_DEBUG,		DA_386IGate,
		      single_step_exception,	PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_NMI,		DA_386IGate,
		      nmi,			PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_BREAKPOINT,	DA_386IGate,
		      breakpoint_exception,	PRIVILEGE_USER);

	init_idt_desc(INT_VECTOR_OVERFLOW,	DA_386IGate,
		      overflow,			PRIVILEGE_USER);

	init_idt_desc(INT_VECTOR_BOUNDS,	DA_386IGate,
		      bounds_check,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_INVAL_OP,	DA_386IGate,
		      inval_opcode,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_COPROC_NOT,	DA_386IGate,
		      copr_not_available,	PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_DOUBLE_FAULT,	DA_386IGate,
		      double_fault,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_COPROC_SEG,	DA_386IGate,
		      copr_seg_overrun,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_INVAL_TSS,	DA_386IGate,
		      inval_tss,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_SEG_NOT,	DA_386IGate,
		      segment_not_present,	PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_STACK_FAULT,	DA_386IGate,
		      stack_exception,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_PROTECTION,	DA_386IGate,
		      general_protection,	PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_PAGE_FAULT,	DA_386IGate,
		      page_fault,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_COPROC_ERR,	DA_386IGate,
		      copr_error,		PRIVILEGE_KRNL);


	/*初始化8259a对应的硬件中断*/
	init_idt_desc(INT_VEC_IRQ0 + 0,		DA_386IGate,
		      hwint00,		PRIVILEGE_KRNL);
	init_idt_desc(INT_VEC_IRQ0 + 1,		DA_386IGate,
		      hwint01,		PRIVILEGE_KRNL);
	init_idt_desc(INT_VEC_IRQ0 + 2,		DA_386IGate,
		      hwint02,		PRIVILEGE_KRNL);
	init_idt_desc(INT_VEC_IRQ0 + 3,		DA_386IGate,
		      hwint03,		PRIVILEGE_KRNL);
	init_idt_desc(INT_VEC_IRQ0 + 4,		DA_386IGate,
		      hwint04,		PRIVILEGE_KRNL);
	init_idt_desc(INT_VEC_IRQ0 + 5,		DA_386IGate,
		      hwint05,		PRIVILEGE_KRNL);
	init_idt_desc(INT_VEC_IRQ0 + 6,		DA_386IGate,
		      hwint06,		PRIVILEGE_KRNL);
	init_idt_desc(INT_VEC_IRQ0 + 7,		DA_386IGate,
		      hwint07,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VEC_IRQ8 + 0,		DA_386IGate,
		      hwint08,		PRIVILEGE_KRNL);
	init_idt_desc(INT_VEC_IRQ8 + 1,		DA_386IGate,
		      hwint09,		PRIVILEGE_KRNL);
	init_idt_desc(INT_VEC_IRQ8 + 2,		DA_386IGate,
		      hwint10,		PRIVILEGE_KRNL);
	init_idt_desc(INT_VEC_IRQ8 + 3,		DA_386IGate,
		      hwint11,		PRIVILEGE_KRNL);
	init_idt_desc(INT_VEC_IRQ8 + 4,		DA_386IGate,
		      hwint12,		PRIVILEGE_KRNL);
	init_idt_desc(INT_VEC_IRQ8 + 5,		DA_386IGate,
		      hwint13,		PRIVILEGE_KRNL);
	init_idt_desc(INT_VEC_IRQ8 + 6,		DA_386IGate,
		      hwint14,		PRIVILEGE_KRNL);
	init_idt_desc(INT_VEC_IRQ8 + 7,		DA_386IGate,
		      hwint15,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_SYS_CALL,	DA_386IGate,
		      sys_call,		PRIVILEGE_USER);

	/*初始化ldt段*/
	int i;
	PROCESS *p_proc = proc_table;
	int ldt_index = INDEX_LDT_FIRST;
	for(i = 0;i < NR_TASKS + NR_PROCS;i++)
	{
		memset(p_proc, 0, sizeof(PROCESS));
		p_proc->ldt_sel = ldt_index<<3;
		init_descriptor(&gdt[ldt_index], vir2phy(seg2addr(SELECTOR_KERNEL_DS), (u32)p_proc->ldts), sizeof(DESCRIPTOR) * LDT_SIZE - 1, DA_LDT);
		ldt_index++;
		p_proc++;
	}

	/*初始化段描述符*/
	/*初始化tss段*/
	memset((char*)&tss, 0, sizeof(tss));
	tss.ss0 = SELECTOR_KERNEL_DS;
	tss.iobase = sizeof(tss);
	init_descriptor(&gdt[INDEX_TSS], vir2phy(seg2addr(SELECTOR_KERNEL_DS), (u32)&tss), sizeof(TSS) - 1, DA_386TSS);/*tss只代表段内地址即逻辑地址*/

}



/*
功能：异常处理句柄
*/
PUBLIC void exception_handler(int vec_no, int err_code, int eip, int cs, int eflags)
{
	int i;
	int text_color = 0x74; /* 灰底红字 */

	char * err_msg[] = {"#DE Divide Error",
			    "#DB RESERVED",
			    "—  NMI Interrupt",
			    "#BP Breakpoint",
			    "#OF Overflow",
			    "#BR BOUND Range Exceeded",
			    "#UD Invalid Opcode (Undefined Opcode)",
			    "#NM Device Not Available (No Math Coprocessor)",
			    "#DF Double Fault",
			    "    Coprocessor Segment Overrun (reserved)",
			    "#TS Invalid TSS",
			    "#NP Segment Not Present",
			    "#SS Stack-Segment Fault",
			    "#GP General Protection",
			    "#PF Page Fault",
			    "—  (Intel reserved. Do not use.)",
			    "#MF x87 FPU Floating-Point Error (Math Fault)",
			    "#AC Alignment Check",
			    "#MC Machine Check",
			    "#XF SIMD Floating-Point Exception"
	};

	/* 通过打印空格的方式清空屏幕的前五行，并把 disp_pos 清零 */
	disp_pos = 0;
	for(i=0;i<80*5;i++){
		disp_str(" ");
	}

	/*显示位置*/
	disp_pos = 0;

	disp_color_str("Exception! --> ", text_color);
	disp_color_str(err_msg[vec_no], text_color);
	disp_color_str("\n\n", text_color);
	disp_color_str("EFLAGS:", text_color);
	disp_int(eflags);
	disp_color_str("CS:", text_color);
	disp_int(cs);
	disp_color_str("EIP:", text_color);
	disp_int(eip);

	if(err_code != 0xFFFFFFFF){
		disp_color_str("Error code:", text_color);
		disp_int(err_code);
	}
}


/*
功能：找出段的基地址
*/
u32 seg2addr(u16 seg)
{
	DESCRIPTOR *pDes = &gdt[seg>>3];
	return pDes->base_low | pDes->base_mid<<16 | pDes->base_high<<24;
}