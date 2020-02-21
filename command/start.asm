extern main
extern exit

bits 32
[section .text]

global _start

_start:
	push ax
	push cx

	call main
	push ax

	call exit
	hlt


