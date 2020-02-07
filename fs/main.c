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
PRIVATE int rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf);
PRIVATE void set_superblock();
PRIVATE void set_imap();
PRIVATE void set_smap();
PRIVATE void set_inode_array();
PRIVATE void set_root_de();


/*
功能：文件系统任务，用于处理和文件系统有关的消息
*/
PUBLIC void task_fs()
{
	printl("TASK FS begins.\n");
	init_fs();	
	spin("FS");
}


/*
功能：初始化文件系统
*/
PRIVATE void init_fs()
{
	/*开启设备*/
	MESSAGE driver_msg;
	driver_msg.type = DEV_OPEN;
	driver_msg.DEVICE = MINOR(ROOT_DEV);/*次设备号*/
	send_rec(BOTH, TASK_HD, &driver_msg);

	/*建立文件系统*/
	mkfs();
}


/*
功能：建立OS的文件系统组织结构
*/
PRIVATE void mkfs()
{
	/*写入超级块的信息*/
	set_superblock();
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
PRIVATE int rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf)
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
	struct super_block sb;
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
		fsbuf[i] = 0xFE;
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
*/
PRIVATE void set_imap()
{

}


/*
功能：写入sector图信息
*/
PRIVATE void set_smap()
{

}


/*
功能：写入inode数组
*/
PRIVATE void set_inode_array()
{

}


/*
功能：写入根目录文件
*/
PRIVATE void set_root_de()
{

}