extern main
extern exit

bits 32
[section .text]

global _start

_start:
	push eax;此处不能是ax,传参时，参数保存在eax寄存器中，若传入ax会发生截断
	push ecx;同eax

	call main
	push eax

	call exit
	hlt


