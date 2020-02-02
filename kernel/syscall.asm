%include "sconst.inc"

_NR_sendrecv		equ		0
_NR_printx			equ		1
INT_VECTOR_SYS_CALL equ		0x90

global sendrec
global printx


bits 32
[section .text]

sendrec:
	mov eax, _NR_sendrecv
	mov ebx, [esp + 4];/*function*/
	mov ecx, [esp + 8];/*src_dest*/
	mov edx, [esp + 12];/*message*/
	int INT_VECTOR_SYS_CALL
	ret

printx:
	mov eax, _NR_printx
	mov ebx, [esp + 4];buf
	int INT_VECTOR_SYS_CALL
	ret 

	
