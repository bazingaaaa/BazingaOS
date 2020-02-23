#include "const.h"
#include "type.h"
#include "config.h"
#include "elf.h"
#include "string.h"

#define SELFMAG 4
#define SHF_ALLOC 2

const char ELFMAG[] = {0x7f, 0x45, 0x4c, 0x46};

/* 
功能：将整型以16进制转换为字符串，开头为0x
备注：数字前面的 0 不被显示出来, 比如 0000B800 被显示成 B800
*/
PUBLIC char *itoa(char * str, int num)
{
	char *p = str;
	int flag = 0;
	int i;

	*p++ = '0';
	*p++ = 'x';

	if(0 == num)
	{
		*p++ = '0';
	}
	else
	{
		for(i = 28;i >= 0;i = i - 4)
		{
			int tmp = num >> i & 0xf;
			if(1 == flag || tmp != 0)/*为了跨越前面的0*/
			{
				*p = tmp + '0';
				if(tmp > 9)
				{
					*p += 7;
				}
				p++;
			}
		}
	}
	*p = 0;
	return str;
}


/*
功能：按16位整型输出到屏幕上
备注：输出的位置由disp_pos确定
*/
PUBLIC void disp_int(int input)
{
	char output[40];
	itoa(output, input);
	disp_str(output);
}


/*
功能：延时函数
*/
PUBLIC void delay(int time)
{
	int i, j, k;

	for(i = 0;i < time;i++)
	{
		for(j = 0;j < 10;j++)
		{
			for(k = 0;k < 100000;k++)
			{}
		}
	}
}


/*
功能：获取启动参数
*/
PUBLIC void get_boot_param(struct boot_params* pbp)
{
	int *p = (int*)BOOT_PARAM_ADDR;
	assert(p[BI_MAG] == BOOT_PARAM_MAGIC);

	pbp->mem_size = p[BI_MEM_SIZE];
	pbp->kernel_file = (unsigned char*)p[BI_KERNEL_FILE];

	/*检查kernelfile的magicno*/
	assert(memcmp(pbp->kernel_file, ELFMAG, SELFMAG) == 0);
}


/*
功能：获取内核内存分布图
返回值：0-成功获取
*/
PUBLIC int get_kernel_map(unsigned int * b, unsigned int * l)
{
	struct boot_params bp;
	get_boot_param(&bp);

	Elf32_Ehdr* elf_header = (Elf32_Ehdr*)(bp.kernel_file);

	/* the kernel file should be in ELF format */
	if (memcmp(elf_header->e_ident, ELFMAG, SELFMAG) != 0)
		return -1;

	*b = ~0;
	unsigned int t = 0;
	int i;
	// for (i = 0; i < elf_header->e_shnum; i++) {
	// 	Elf32_Shdr* section_header =
	// 		(Elf32_Shdr*)(bp.kernel_file +
	// 			      elf_header->e_shoff +
	// 			      i * elf_header->e_shentsize);
	// 	if (section_header->sh_flags & SHF_ALLOC) {
	// 		int bottom = section_header->sh_addr;
	// 		int top = section_header->sh_addr +
	// 			section_header->sh_size;

	// 		if (*b > bottom)
	// 			*b = bottom;
	// 		if (t < top)
	// 			t = top;
	// 	}
	// }
	for (i = 0; i < elf_header->e_shnum; i++) {
		Elf32_Phdr* program_header =
			(Elf32_Phdr*)(bp.kernel_file +
				      elf_header->e_phoff +
				      i * elf_header->e_phentsize);
		if (program_header->p_type == PT_LOAD) {
			int bottom = program_header->p_vaddr;
			int top = program_header->p_vaddr +
				program_header->p_memsz;

			if (*b > bottom)
				*b = bottom;
			if (t < top)
				t = top;
		}
	}
	assert(*b < t);
	*l = t - *b - 1;

	return 0;
}
