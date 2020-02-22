HD = ../80m.img

ENTRY_POINT = 0x1000
ASM = nasm
CC = gcc
LD = ld


ASMFLAG = -f elf
CFLAG = -I ../include/ -m32 -c -fno-builtin -Wall -fno-stack-protector
LDFLAG = -Ttext $(ENTRY_POINT) -m elf_i386
LIB		= ../lib/bazingacrt.a

.PHONY = everything clean realclean all

OBJS = start.o echo.o pwd.o
BIN = echo pwd

everything:$(BIN) $(OBJS)

all: realclean everything

clean:
	rm -f $(OBJS)

realclean:
	rm -f $(OBJS) $(BIN) 

install: all clean
	cp ../kernel/kernel.bin ./ -v
	tar vcf inst.tar kernel.bin $(BIN) 
	dd if=inst.tar of=$(HD) seek=29360128 bs=1 count=81920 conv=notrunc

start.o: start.asm
	$(ASM) $(ASMFLAG) -o $@ $<

echo.o: echo.c
	$(CC) $(CFLAG) -o $@ $<

echo: start.o echo.o $(LIB)
	$(LD) $(LDFLAG) -o $@ $?

pwd.o: pwd.c
	$(CC) $(CFLAG) -o $@ $<

pwd: start.o pwd.o $(LIB)
	$(LD) $(LDFLAG) -o $@ $?