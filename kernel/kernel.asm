%include "sconst.inc"

;导入函数
extern cstart
extern exception_handler
extern spurious_irq
extern kernel_main
extern out_byte
extern delay
extern disp_str
extern delay
extern clock_handler

;导入变量
extern gdt_ptr
extern idt_ptr
extern tss
extern p_proc_ready
extern k_reenter
extern irq_table
extern syscall_table
extern disp_pos

bits 32

[section .data]
clock_int_msg		db	"^", 0

[section .bss]
StackSpace		resb		2 * 1024
StackTop:

[section .text]
global _start
global restart

global divide_error
global single_step_exception
global nmi
global breakpoint_exception
global overflow
global bounds_check
global inval_opcode
global copr_not_available
global double_fault
global copr_seg_overrun
global inval_tss
global segment_not_present
global stack_exception
global general_protection
global page_fault
global copr_error

;8259a对应的16个硬件中断
global hwint00
global hwint01
global hwint02
global hwint03
global hwint04
global hwint05
global hwint06
global hwint07
global hwint08
global hwint09
global hwint10
global hwint11
global hwint12
global hwint13
global hwint14
global hwint15

global sys_call

_start:;此处为整个代码段入口，必须命名为_start以便连接器识别
	mov esp, StackTop

	mov dword [disp_pos], 0;此行必不可少，少了就出BUG

	sgdt [gdt_ptr];读取原gdtPtr的内容
	call cstart
	lgdt [gdt_ptr];加载gdtr
	lidt [idt_ptr];加载idtr

	jmp SELECTOR_KERNEL_CS:csinit

csinit:
	;sti :这块儿不能打开中断，此处IF为0
	xor eax, eax
	mov ax, SELECTOR_TSS
	ltr ax

	jmp kernel_main

;异常处理
divide_error:
	push 0xFFFFFFFF
	push 0
	jmp exception

single_step_exception:
	push 0xffffffff
	push 1
	jmp exception

nmi:
	push 0xffffffff
	push 2
	jmp exception

breakpoint_exception:
	push 0xffffffff
	push 3
	jmp exception

overflow:
	push 0xffffffff
	push 4
	jmp exception

bounds_check:
	push 0xffffffff
	push 5
	jmp exception

inval_opcode:
	push 0xffffffff
	push 6
	jmp exception

copr_not_available:
	push 0xffffffff
	push 7
	jmp exception

double_fault:
	push 8
	jmp exception

copr_seg_overrun:
	push 0xffffffff
	push 9
	jmp exception

inval_tss:
	push 10
	jmp exception

segment_not_present:
	push 11
	jmp exception

stack_exception:
	push 12
	jmp exception

general_protection:
	push 13
	jmp exception

page_fault:
	push 14
	jmp exception

copr_error:
	push 0xffffffff
	push 16
	jmp exception

exception:
	call exception_handler
	add esp, 4 * 2
	hlt

%macro hwint_master 1
  	call save
	
	in al, INT_M_CTLMASK
	or al, 1<<%1;禁止时钟中断
	out INT_M_CTLMASK, al 

  	mov al, 20h
	out INT_M_CTL, al

  	sti

  	push %1
  	call [irq_table + 4 * %1]
  	pop ecx

	cli

	in al, INT_M_CTLMASK
	and al, ~(1<<%1);开启时钟中断
	out INT_M_CTLMASK, al

	ret
%endmacro

ALIGN 16
hwint00:
	hwint_master 0

ALIGN 16
hwint01:
	hwint_master 1

ALIGN 16
hwint02:
	hwint_master 2

ALIGN 16
hwint03:
	hwint_master 3

ALIGN 16
hwint04:
	hwint_master 4

ALIGN 16
hwint05:
	hwint_master 5

ALIGN 16
hwint06:
	hwint_master 6

ALIGN 16
hwint07:
	hwint_master 7

%macro hwint_slave 1
	call save
	
	in al, INT_S_CTLMASK
	or al, 1<<%1;禁止时钟中断
	out INT_S_CTLMASK, al 

  	mov al, 20h;20h为EOI，即告知8259a中断处理完毕
	out INT_M_CTL, al
	nop
	out INT_S_CTL, al;此处主从片都需要发送EOI
  	sti

  	push %1
  	call [irq_table + 4 * %1]
  	pop ecx

	cli

	in al, INT_S_CTLMASK
	and al, ~(1<<%1);开启时钟中断
	out INT_S_CTLMASK, al

	ret
%endmacro

ALIGN 16
hwint08:
	hwint_slave 8

ALIGN 16
hwint09:
	hwint_slave 9

ALIGN 16
hwint10:
	hwint_slave 10

ALIGN 16
hwint11:
	hwint_slave 11

ALIGN 16
hwint12:
	hwint_slave 12

ALIGN 16
hwint13:
	hwint_slave 13

ALIGN 16
hwint14:
	hwint_slave 14

ALIGN 16
hwint15:
	hwint_slave 15


sys_call:
	call save
	sti

	push esi

	push dword[p_proc_ready]
	push edx
	push ecx
	push ebx
  	call [syscall_table + eax * 4]
  	add esp, 4 * 4

  	pop esi
  	mov [esi + EAXREG - P_STACKBASE], eax
	cli
	
	ret


;输入参数为端口号,告诉8259a清除ISR中中断对应的标志位
eoi:
	mov edx, [esp + 4]
	mov al, 20h

	out dx, al

	nop
	nop
	ret

;特权级跳转
restart:
	mov esp, [p_proc_ready]
	lldt [esp + P_LDT_SEL]
	
	lea eax, [esp + P_STACKTOP]
	mov dword [tss + TSS3_S_SP0], eax

restart_reenter:
	dec dword[k_reenter]
	pop gs
	pop fs
	pop es
	pop ds
	popad
	add esp, 4;retaddr占用4字节
	iretd;中断返回后eflags被置位

save:
	;sub esp, 4 无需在入栈操作，调用save时，save下一条命令已经自动入栈
	;保存寄存器(上下文)
	pushad
	push ds
	push es
	push fs
	push gs

	mov esi, edx;保存edx的位置

	mov dx, ss
	mov ds, dx
	mov es, dx
	mov fs, dx

	mov edx, esi;恢复edx

	mov esi, esp;保存进程表头部位置

	inc dword[k_reenter]
  	cmp dword[k_reenter], 0
  	jne .1;中断重入
  	
;此时堆栈指针位于进程表中，应该改到内核堆栈中，不然会破坏进程表结构，首先切换堆栈再把返回地址入栈
  	mov esp, StackTop;切换到内核栈
  	push restart
   	jmp [esi + RETADR - P_STACKBASE];调用save后的下一条指令
.1:
   	push restart_reenter
   	jmp [esi + RETADR - P_STACKBASE];调用save后的下一条指令


