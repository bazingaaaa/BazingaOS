ENTRY_POINT = 0x30400
OFFSET = 0x400
ASM = nasm
CC = gcc
LD = ld


ASMBFLAG = -I boot/include/
ASMKFLAG = -I include/ -I include/sys/ -f elf
CFLAG = -I include/ -I include/sys/ -m32 -c -fno-builtin 
LDFLAG = -s -Ttext $(ENTRY_POINT) -m elf_i386
 

BAZINGABOOT = boot/boot.bin boot/loader.bin
BAZINGAKERNEL = kernel/kernel.bin
LOBJS = lib/close.o lib/fork.o lib/kliba.o lib/klib.o lib/string.o lib/misc.o lib/open.o\
		lib/read.o lib/write.o lib/unlink.o lib/lseek.o lib/wait.o lib/exit.o lib/syscall.o\
		lib/printf.o lib/vsprintf.o lib/getpid.o lib/stat.o lib/exec.o 
OBJS = kernel/kernel.o kernel/start.o  kernel/i8259a.o kernel/protect.o kernel/keyboard.o kernel/tty.o \
		kernel/main.o kernel/global.o kernel/clock.o kernel/proc.o \
		kernel/console.o kernel/systask.o kernel/hd.o fs/main.o \
		fs/misc.o fs/read_write.o fs/link.o fs/open.o fs/lseek.o mm/main.o mm/forkexit.o mm/exec.o $(LOBJS)
	

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

kernel/kernel.o: kernel/kernel.asm
	$(ASM) $(ASMKFLAG) -o $@ $<

kernel/start.o: kernel/start.c
	$(CC) $(CFLAG) -o $@ $<

kernel/i8259a.o: kernel/i8259a.c
	$(CC) $(CFLAG) -o $@ $<

kernel/main.o: kernel/main.c
	$(CC) $(CFLAG) -o $@ $<	

kernel/global.o: kernel/global.c
	$(CC) $(CFLAG) -o $@ $<	

kernel/clock.o: kernel/clock.c
	$(CC) $(CFLAG) -o $@ $<	

kernel/proc.o: kernel/proc.c
	$(CC) $(CFLAG) -o $@ $<	

kernel/keyboard.o: kernel/keyboard.c
	$(CC) $(CFLAG) -o $@ $<	

kernel/tty.o: kernel/tty.c
	$(CC) $(CFLAG) -o $@ $<	

kernel/console.o: kernel/console.c
	$(CC) $(CFLAG) -o $@ $<	

kernel/systask.o: kernel/systask.c 
	$(CC) $(CFLAG) -o $@ $<	

kernel/hd.o: kernel/hd.c 
	$(CC) $(CFLAG) -o $@ $<

kernel/protect.o: kernel/protect.c
	$(CC) $(CFLAG) -o $@ $<


fs/main.o: fs/main.c
	$(CC) $(CFLAG) -o $@ $<
	
fs/misc.o: fs/misc.c
	$(CC) $(CFLAG) -o $@ $<

fs/read_write.o: fs/read_write.c
	$(CC) $(CFLAG) -o $@ $<

fs/disklog.o: fs/disklog.c
	$(CC) $(CFLAG) -o $@ $<

fs/open.o: fs/open.c
	$(CC) $(CFLAG) -o $@ $<

fs/link.o: fs/link.c
	$(CC) $(CFLAG) -o $@ $<

fs/lseek.o: fs/lseek.c
	$(CC) $(CFLAG) -o $@ $<

mm/main.o: mm/main.c
	$(CC) $(CFLAG) -o $@ $<

mm/forkexit.o: mm/forkexit.c
	$(CC) $(CFLAG) -o $@ $<

mm/exec.o: mm/exec.c
	$(CC) $(CFLAG) -o $@ $<

lib/misc.o: lib/misc.c
	$(CC) $(CFLAG) -o $@ $<	

lib/open.o: lib/open.c
	$(CC) $(CFLAG) -o $@ $<	

lib/close.o: lib/close.c
	$(CC) $(CFLAG) -o $@ $<	

lib/kliba.o: lib/kliba.asm
	$(ASM) $(ASMKFLAG) -o $@ $<

lib/klib.o: lib/klib.c
	$(CC) $(CFLAG) -o $@ $<

lib/string.o: lib/string.asm 
	$(ASM) $(ASMKFLAG) -o $@ $<

lib/read.o: lib/read.c
	$(CC) $(CFLAG) -o $@ $<

lib/write.o: lib/write.c
	$(CC) $(CFLAG) -o $@ $<

lib/getpid.o: lib/getpid.c
	$(CC) $(CFLAG) -o $@ $<

lib/syslog.o: lib/syslog.c
	$(CC) $(CFLAG) -o $@ $<

lib/unlink.o: lib/unlink.c
	$(CC) $(CFLAG) -o $@ $<

lib/lseek.o: lib/lseek.c
	$(CC) $(CFLAG) -o $@ $<

lib/fork.o: lib/fork.c
	$(CC) $(CFLAG) -o $@ $<

lib/wait.o: lib/wait.c
	$(CC) $(CFLAG) -o $@ $<

lib/exit.o: lib/exit.c
	$(CC) $(CFLAG) -o $@ $<

lib/syscall.o: lib/syscall.asm
	$(ASM) $(ASMKFLAG) -o $@ $<

lib/printf.o: lib/printf.c
	$(CC) $(CFLAG) -o $@ $<	

lib/vsprintf.o: lib/vsprintf.c
	$(CC) $(CFLAG) -o $@ $<

lib/stat.o: lib/stat.c
	$(CC) $(CFLAG) -o $@ $<

lib/exec.o: lib/exec.c
	$(CC) $(CFLAG) -o $@ $<