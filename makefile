ENTRY_POINT = 0x30400
OFFSET = 0x400
ASM = nasm
CC = gcc
LD = ld


ASMBFLAG = -I boot/include/
ASMKFLAG = -I include/ -f elf
CFLAG = -I include/ -m32 -c -fno-builtin 
LDFLAG = -s -Ttext $(ENTRY_POINT) -m elf_i386
 

BAZINGABOOT = boot/boot.bin boot/loader.bin
BAZINGAKERNEL = kernel/kernel.bin
OBJS = kernel/kernel.o kernel/start.o lib/kliba.o lib/klib.o lib/string.o kernel/i8259a.o kernel/protect.o \
		kernel/main.o kernel/global.o kernel/clock.o kernel/syscall.o kernel/proc.o kernel/keyboard.o kernel/tty.o \
		kernel/console.o kernel/printf.o kernel/vsprintf.o lib/misc.o kernel/systask.o kernel/hd.o fs/main.o lib/open.o \
		fs/misc.o
	


.PHONY = everything clean realclean all buildimg

everything:$(BAZINGABOOT) $(BAZINGAKERNEL)

all: realclean everything

clean:
	rm -f $(OBJS)

realclean:
	rm -f $(OBJS) $(BAZINGABOOT) $(BAZINGAKERNEL)

buildimg:
	dd if=boot/boot.bin of=a.img bs=512 count=1 conv=notrunc
	mount -o loop a.img /mnt/floppy/
	cp -fv boot/loader.bin /mnt/floppy/
	cp -fv kernel/kernel.bin /mnt/floppy/
	umount /mnt/floppy/


boot/boot.bin:boot/boot.asm boot/include/fat12hdr.inc boot/include/load.inc
	$(ASM) $(ASMBFLAG) -o $@ $<

boot/loader.bin:boot/loader.asm boot/include/pm.inc boot/include/load.inc
	$(ASM) $(ASMBFLAG) -o $@ $<

$(BAZINGAKERNEL): $(OBJS)
	$(LD) $(LDFLAG) -o $(BAZINGAKERNEL) $(OBJS)

kernel/syscall.o: kernel/syscall.asm include/sconst.inc
	$(ASM) $(ASMKFLAG) -o $@ $<

kernel/kernel.o: kernel/kernel.asm include/sconst.inc
	$(ASM) $(ASMKFLAG) -o $@ $<

kernel/start.o: kernel/start.c include/const.h include/protect.h include/type.h include/global.h include/proto.h
	$(CC) $(CFLAG) -o $@ $<

kernel/i8259a.o: kernel/i8259a.c include/protect.h include/const.h include/type.h include/proto.h
	$(CC) $(CFLAG) -o $@ $<

kernel/main.o: kernel/main.c include/proto.h
	$(CC) $(CFLAG) -o $@ $<	

kernel/global.o: kernel/global.c include/proto.h include/const.h include/type.h include/protect.h
	$(CC) $(CFLAG) -o $@ $<	

kernel/clock.o: kernel/clock.c include/proto.h include/const.h
	$(CC) $(CFLAG) -o $@ $<	

kernel/proc.o: kernel/proc.c include/proto.h
	$(CC) $(CFLAG) -o $@ $<	

kernel/keyboard.o: kernel/keyboard.c include/proto.h
	$(CC) $(CFLAG) -o $@ $<	

kernel/tty.o: kernel/tty.c include/proto.h
	$(CC) $(CFLAG) -o $@ $<	

kernel/console.o: kernel/console.c include/proto.h
	$(CC) $(CFLAG) -o $@ $<	

kernel/printf.o: kernel/printf.c include/proto.h
	$(CC) $(CFLAG) -o $@ $<	

kernel/vsprintf.o: kernel/vsprintf.c include/proto.h
	$(CC) $(CFLAG) -o $@ $<

kernel/systask.o: kernel/systask.c 
	$(CC) $(CFLAG) -o $@ $<	

kernel/hd.o: kernel/hd.c 
	$(CC) $(CFLAG) -o $@ $<	

lib/misc.o: lib/misc.c include/proto.h
	$(CC) $(CFLAG) -o $@ $<	

lib/open.o: lib/open.c
	$(CC) $(CFLAG) -o $@ $<	

lib/kliba.o: lib/kliba.asm
	$(ASM) $(ASMKFLAG) -o $@ $<

lib/klib.o: lib/klib.c include/const.h
	$(CC) $(CFLAG) -o $@ $<

kernel/protect.o: kernel/protect.c include/const.h include/type.h include/protect.h include/global.h include/proto.h
	$(CC) $(CFLAG) -o $@ $<

lib/string.o: lib/string.asm 
	$(ASM) $(ASMKFLAG) -o $@ $<	

fs/main.o: fs/main.c
	$(CC) $(CFLAG) -o $@ $<
	
fs/misc.o: fs/misc.c
	$(CC) $(CFLAG) -o $@ $<