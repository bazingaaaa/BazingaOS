#include "type.h"
#include "config.h"
#include "const.h"
#include "tty.h"
#include "console.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "fs.h"
#include "global.h"
#include "hd.h"

PRIVATE void init_fs();
PRIVATE void mkfs();
PRIVATE void set_superblock();
PRIVATE void set_imap();
PRIVATE void set_smap();
PRIVATE void set_inode_array();
PRIVATE void set_root_de();
PRIVATE void read_super_block(int dev);
PUBLIC struct super_block* get_super_block(int dev);


struct super_block sb;/*临时存放超级块信息*/


/*
功能：文件系统任务，用于处理和文件系统有关的消息
*/
PUBLIC void task_fs()
{
	init_fs();	
	while(1)
	{
		MESSAGE msg;
		int src;

		send_rec(RECEIVE, ANY, &msg);
		src = msg.source;
		pcaller = &proc_table[src];
		switch(msg.type)
		{
			case OPEN:
				msg.FD = do_open(&msg);
				break;
			case CLOSE:
				msg.RETVAL = do_close(&msg);
				break;
		}
		msg.type = SYSCALL_RET;
		send_rec(SEND, src, &msg);
	}
}


/*
功能：初始化文件系统
*/
PRIVATE void init_fs()
{
	int i;
	struct super_block *sb;

	/*数组初始化*/
	for(i = 0;i < NR_FILE_DESC;i++)
	{
		memset(&fd_table[i], 0, sizeof(struct file_desc));
	}
	for(i = 0;i < NR_INODE;i++)
	{
		memset(&inode_table[i], 0, sizeof(struct inode));
	}
	for(i = 0;i < NR_SUPER_BLOCK;i++)
	{
		super_block[i].sb_dev = NO_DEV;
	}

	/*开启设备*/
	MESSAGE driver_msg;
	driver_msg.type = DEV_OPEN;
	driver_msg.DEVICE = MINOR(ROOT_DEV);/*次设备号*/
	send_rec(BOTH, TASK_HD, &driver_msg);

	/*建立文件系统*/
	mkfs();

	/*读取超级块信息*/
	read_super_block(ROOT_DEV);
	sb = get_super_block(ROOT_DEV);
	assert(sb->magic == MAGIC_V1);

	/*设置根目录节点信息*/
	root_inode = get_inode(ROOT_DEV, ROOT_INODE);
}


/*
功能：建立OS的文件系统组织结构
*/
PRIVATE void mkfs()
{
	/*写入超级块的信息*/
	set_superblock();

	/*写入inode图信息*/
	set_imap();

	/*写入sector图信息*/
	set_smap();

	/*写入inode数组*/
	set_inode_array();

	/*写入根目录文件*/
	set_root_de();
}


/*
功能：读写硬盘扇区
输入：io_type-IO类型，读/写
	  dev-设备号（非主设备号或次设备号）
	  pos-若写入，pos代表写入磁盘的位置，相对于该分区起始，单位：字节
	  bytes-读/写的字节数
	  proc_nr-进行读/写的任务号
	  buf-读/写缓存区
返回值：0-读/写正常
备注：阻塞调用
*/
PUBLIC int rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf)
{
	MESSAGE driver_msg;
	driver_msg.type = io_type;
	driver_msg.DEVICE = MINOR(dev);/*次设备号*/
	driver_msg.POSITION = pos;
	driver_msg.CNT = bytes;
	driver_msg.PROC_NR = proc_nr;
	driver_msg.BUF= buf;
	assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
	send_rec(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);
	return 0;
}


/*
功能：写入超级块的信息
*/
PRIVATE void set_superblock()
{
	int i;
	struct part_info geo;/*存放分区位置信息*/
	int bits_per_sects = SECTOR_SIZE * 8;

	/*获取分区位置信息*/
	MESSAGE driver_msg;
	driver_msg.type = DEV_IOCTL;
	driver_msg.REQUEST = DIOCTL_GET_GEO;
	driver_msg.BUF = &geo;
	driver_msg.DEVICE = MINOR(ROOT_DEV);/*次设备号*/
	assert(dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
	send_rec(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);

	/*填写并写入超级块*/
	sb.magic = MAGIC_V1;/*魔数，用于标识文件系统*/
	sb.nr_inodes = bits_per_sects;/*inode数量*/
	sb.nr_sects = geo.size;/*文件系统占用的扇区数量*/
	sb.nr_imap_sects = 1;/*imap占用的扇区数量*/
	sb.nr_smap_sects = sb.nr_sects / bits_per_sects + 1;/*smap占用的扇区数量*/
	sb.nr_inode_sects = sb.nr_inodes * INODE_SIZE / SECTOR_SIZE;/*inode占用的扇区数量*/
	sb.n_1st_sect = 1 + 1 + sb.nr_imap_sects + sb.nr_smap_sects + sb.nr_inode_sects;/*BOOT sect + SB sect + imap sect + smap sect + inode array sect*/
	sb.root_inode = ROOT_INODE;/*根目录inode号*/
	sb.inode_size = INODE_SIZE;

	struct inode x;
	sb.inode_iszie_off = (int)((void*)&x.i_size - (void*)&x);
	sb.inode_start_off = (int)((void*)&x.i_start_sect - (void*)&x);

	struct dir_entry de;
	sb.dir_ent_inode_off = (int)((void*)&de.inode_nr - (void*)&de);;
	sb.dir_ent_fname_off = (int)((void*)&de.name - (void*)&de);
	
	/*对扇区进行初始化（写入0xFE主要是为了验证硬盘写入功能是否正常）*/
	for(i = 0;i < SECTOR_SIZE;i++)
	{
		fsbuf[i] = 0xF0;
	}
	memcpy(fsbuf, &sb, sizeof(struct super_block));

	/*将超级块信息写入到硬盘中*/
	WR_SECT(ROOT_DEV, 1);

	printl("devbase:0x%x00  sb:0x%x00  imap:0x%x00  smap:0x%x00\n  inode:0x%x00  n_1st_sect:0x%x00\n",
			geo.base * 2,
			(geo.base + 1) * 2,
			(geo.base + 2) * 2,
			(geo.base + 3) * 2,
			(geo.base + 3 + sb.nr_smap_sects) * 2,
			(geo.base + 3 + sb.nr_smap_sects + sb.nr_inode_sects) * 2
			);
}


/*
功能：写入inode图信息
备注：inode图的bit0作为保留，bit1位根目录文件，bit2~bit4为三个终端文件
*/
PRIVATE void set_imap()
{
	int i;
	memset(fsbuf, 0, SECTOR_SIZE);
	for(i = 0;i < NR_CONSOLES + 2;i++)
	{
		fsbuf[0] |= (1<<i);
	}
	assert(fsbuf[0] = 0x1F);
	/*将超级块信息写入到硬盘中*/
	WR_SECT(ROOT_DEV, 2);
}


/*
功能：写入sector图信息
备注：NR_DEFAULT_FILE_SECTS为根目录文件预留，对硬盘进行写入时以扇区为单位
*/
PRIVATE void set_smap()
{
	int i;
	int nr_sects = NR_DEFAULT_FILE_SECTS + 1;/*BIT0为保留*/
	int bits_per_byte = 8;

	memset(fsbuf, 0, SECTOR_SIZE);
	/*整字节部分*/
	for(i = 0;i < nr_sects/bits_per_byte;i++)
	{
		fsbuf[i] = 0xFF;
	}
	/*非整字节部分*/
	for(i = 0;i < nr_sects%bits_per_byte;i++)
	{
		fsbuf[nr_sects/bits_per_byte] |= (1<<i);
	}
	/*写入有效部分*/
	WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects);
	/*剩余扇区清0(第一个扇区已经处理了)*/
	memset(fsbuf, 0, SECTOR_SIZE);
	for(i = 1;i < sb.nr_smap_sects;i++)
	{
		WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + i);
	}
}


/*
功能：写入inode数组
*/
PRIVATE void set_inode_array()
{
	int i;
	struct inode *pNode = (struct inode*)fsbuf;
	memset(fsbuf, 0, SECTOR_SIZE);
	
	pNode->i_mode = I_DIRECTORY;
	pNode->i_size = 4 * DIR_ENT_SIZE;
	pNode->i_start_sect = sb.n_1st_sect;
	pNode->i_nr_sects = NR_DEFAULT_FILE_SECTS;
	
	/*TTY0~2*/
	for(i = 0;i < NR_CONSOLES;i++)
	{
		pNode++;
		pNode->i_mode = I_CHAR_SPECIAL;
		pNode->i_size = 0;
		pNode->i_start_sect = MAKE_DEV(DEV_CHAR_TTY, i);
		pNode->i_nr_sects = 0;
	}
	WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + sb.nr_smap_sects);
}


/*
功能：写入根目录文件
*/
PRIVATE void set_root_de()
{
	int i;
	memset(fsbuf, 0, SECTOR_SIZE);
	struct dir_entry *pDe = (struct dir_entry *)fsbuf;
	pDe->inode_nr = ROOT_INODE;
	strcpy(pDe->name, ".");

	/*dev_tty0~2*/
	for(i = 0;i < NR_CONSOLES;i++)
	{
		pDe++;
		pDe->inode_nr = i + 2;
		sprintf(pDe->name, "dev_tty%d", i);
	}
	WR_SECT(ROOT_DEV, sb.n_1st_sect);
}


/*
功能：从硬盘中读取超级块
输入：dev-设备号
*/
PRIVATE void read_super_block(int dev)
{
	int i;
	MESSAGE driver_msg;
	driver_msg.type = DEV_READ;
	driver_msg.DEVICE = MINOR(dev);/*次设备号*/
	driver_msg.POSITION = SECTOR_SIZE * 1;
	driver_msg.CNT = SECTOR_SIZE;
	driver_msg.PROC_NR = TASK_FS;
	driver_msg.BUF= fsbuf;
	assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
	send_rec(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);

	for(i = 0;i < NR_SUPER_BLOCK;i++)
	{
		if(NO_DEV == super_block[i].sb_dev)
		{
			break;
		}
	}
	if(i >= NR_SUPER_BLOCK)
		panic("super_block slots used up\n");

	assert(i == 0);/*目前只有一个超级块*/

	super_block[i] = *(struct super_block*)fsbuf;
	super_block[i].sb_dev = dev;
}


/*
功能：获取超级块信息
*/
PUBLIC struct super_block* get_super_block(int dev)
{
	int i;
	for(i = 0;i < NR_SUPER_BLOCK;i++)
	{
		if(super_block[i].sb_dev = dev)
		{
			return &super_block[i];
		}
	}
	panic("super block of dev %d is not found\n", dev);

	return 0;
}


/*
功能：获取节点
备注：从缓存中（inode_table）获取节点，若没找到则从硬盘中进行读取
*/
PUBLIC struct inode* get_inode(int dev, int inode_nr)
{
	struct inode *p;
	struct inode *q = 0;

	/*查询是由位于缓存中,若存在直接将其取出，否则占用一个空闲槽位，整个缓存区都需要遍历到*/
	for(p = inode_table;p < inode_table + NR_INODE;p++)
	{
		if(p->i_cnt)
		{
			if(p->i_dev == dev && p->i_num == inode_nr)/*找到指定节点*/
			{
				return p;
			}
		}
		else/*空闲槽位*/
		{	
			if(!q)/*还未找到该节点，临时占用空闲槽位*/
			{
				q = p;
			}
		}
	}
	if(!q)
	{
		panic("invalid inode\n");
	}
	/*inode信息初始化*/
	q->i_dev = dev;
	q->i_num = inode_nr;
	q->i_cnt = 1;

	/*从硬盘中读取inode信息,DIR_ENT_SIZE可以被SECTOR_SIZE整除*/
	struct super_block *sb = get_super_block(dev);
	int sect_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects + (inode_nr - 1) * DIR_ENT_SIZE / SECTOR_SIZE;/*读取inode所在的扇区*/
	RD_SECT(dev, sect_nr);
	struct inode *pNode = (struct inode*)fsbuf + (inode_nr - 1) % (SECTOR_SIZE / DIR_ENT_SIZE);/*该inode位于读取扇区的第几个*/

	/*写入读取inode信息*/
	q->i_mode = pNode->i_mode;
	q->i_size = pNode->i_size;
	q->i_start_sect = pNode->i_start_sect;
	q->i_nr_sects = pNode->i_nr_sects;
	return q;
}


/*
功能：释放inode
*/
PUBLIC void put_inode(struct inode *pNode)
{
	assert(pNode->i_cnt > 0);
	pNode->i_cnt--;
}


/*
功能：inode同步
备注：将更改过inode写入硬盘，保持缓存与硬盘的同步
*/
PUBLIC void sync_inode(struct inode *pNode)
{
	int inode_nr = pNode->i_num;
	int dev = pNode->i_dev;

	/*从硬盘中读取inode信息,DIR_ENT_SIZE可以被SECTOR_SIZE整除*/
	struct super_block *sb = get_super_block(dev);
	int sect_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects + (inode_nr- 1) * DIR_ENT_SIZE / SECTOR_SIZE;/*读取inode所在的扇区*/
	RD_SECT(dev, sect_nr);
	struct inode *pDst = (struct inode*)fsbuf + (inode_nr - 1) % (SECTOR_SIZE / DIR_ENT_SIZE);/*该inode位于读取扇区的第几个*/

	/*写入读取inode信息*/
	pDst->i_mode = pNode->i_mode;
	pDst->i_size = pNode->i_size;
	pDst->i_start_sect = pNode->i_start_sect;
	pDst->i_nr_sects = pNode->i_nr_sects;

	WR_SECT(dev, sect_nr);
}