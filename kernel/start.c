#define GLOBAL_VARS_HERE

#include "const.h"
#include "type.h"
#include "protect.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "fs.h"
#include "global.h"
#include "proto.h"


void cstart()
{
	int text_color = 0x7c;

	disp_color_str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
		 "-----\"cstart\" begins-----\n", text_color);

	/*准备gdt的地址*/
	memcpy((void*)&gdt, (void*)(*((u32*)(&gdt_ptr[2]))), *(u16*)(&gdt_ptr[0]) + 1);
	u16* p_gdt_len = (u16*)(&gdt_ptr[0]);
	u32* p_gdt_addr = (u32*)(&gdt_ptr[2]);
	*p_gdt_len = sizeof(DESCRIPTOR) * GDT_SIZE - 1;
	*p_gdt_addr = (u32*)&gdt;

	/*准备idt的地址*/
	u16* p_idt_len = (u16*)(&idt_ptr[0]);
	u32* p_idt_addr = (u32*)(&idt_ptr[2]);
	*p_idt_len = sizeof(GATE) * IDT_SIZE - 1;
	*p_idt_addr = (u32*)&idt;

	/*保护模式初始化*/
	init_prot();

	disp_color_str("-----\"cstart\" ends-----\n", text_color);
	//disp_int(123456789);
}