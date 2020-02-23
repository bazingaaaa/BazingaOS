#define EI_NIDENT		16

/*
PT_NULL    = 0: 该段没有被使用;
PT_LOAD    = 1: 该段是一个可装载的内存段;
PT_DYNAMIC = 2: 该段描述的是动态链接信息;
*/

#define PT_LOAD 1

typedef unsigned int Elf32_Addr;
typedef unsigned short Elf32_Half;
typedef unsigned int Elf32_Off;
typedef unsigned long Elf32_Word; 
typedef long Elf32_Sword;

typedef struct elf32_ehdr{
	unsigned char e_ident[EI_NIDENT];
	Elf32_Half e_type;
	Elf32_Half e_machine;
	Elf32_Word e_version;
	Elf32_Addr e_entry;
	Elf32_Off e_phoff;
	Elf32_Off e_shoff;
	Elf32_Word e_flags;
	Elf32_Half e_ehsize;
	Elf32_Half e_phentsize;
	Elf32_Half e_phnum;
	Elf32_Half e_shentsize;
	Elf32_Half e_shnum;
	Elf32_Half e_shstrndx;
}Elf32_Ehdr;


typedef struct elf32_shdr {
  Elf32_Word sh_name;  //节的名字，在符号表中的下标
  Elf32_Word sh_type;  //节的类型，描述符号，代码，数据，重定位等
  Elf32_Word sh_flags;  //读写执行标记
  Elf32_Addr sh_addr;  //节在执行时的虚拟地址
  Elf32_Off sh_offset;  //节在文件中的偏移量
  Elf32_Word sh_size;  //节的大小
  Elf32_Word sh_link;  //其它节的索引
  Elf32_Word sh_info;  //节的其它信息
  Elf32_Word sh_addralign;  //节对齐
  Elf32_Word sh_entsize;  //节拥有固定大小项的大小
} Elf32_Shdr;


typedef struct elf32_Phdr{
	Elf32_Word p_type;
	Elf32_Off p_offset;
	Elf32_Addr p_vaddr;
	Elf32_Addr p_paddr;
	Elf32_Word p_filesz;
	Elf32_Word p_memsz;
	Elf32_Word p_flags;
	Elf32_Word p_align;
}Elf32_Phdr;