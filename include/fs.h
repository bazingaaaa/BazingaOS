#ifndef __FS__
#define __FS__

struct dev_drv_map
{
	int driver_nr;
};


#define MAGIC_V1			0x111


struct super_block
{
	/*设备上可见*/
	u32 magic;
	u32 nr_inodes;
	u32 nr_sects;
	u32 nr_imap_sects;
	u32 nr_smap_sects;
	u32 n_1st_sect;/*数据区起始扇区*/
	u32 nr_inode_sects;/*inode占用的扇区数量*/
	u32 root_inode;
	u32 inode_size;/*INODE_SIZE*/
	u32 inode_iszie_off;
	u32 inode_start_off;
	u32 dir_ent_size;/*DIR_ENT_SIZE*/
	u32 dir_ent_inode_off;
	u32 dir_ent_fname_off;

	/*内存中可见*/
	int sb_dev;
};

#define SUPER_BLOCK_SIZE 56

struct inode
{
	/*设备上可见*/
	u32 i_mode;
	u32 i_size;
	u32 i_start_sect;
	u32 i_nr_sects;
	u8 _unused[16];

	/*内存中可见*/
	int i_dev;/*设备号*/
	int i_cnt;
	int i_num;/*node_nr*/
};


struct file_desc
{
	int fd_mode;/*读/写*/
	int fd_pos;/*当前读/写位置*/
	struct inode *fd_inode;/*指向inode的指针*/
};

#define INODE_SIZE 32

#define MAX_FILENAME_LEN 12

struct dir_entry
{
	int inode_nr;
	char name[MAX_FILENAME_LEN];
};

#define DIR_ENT_SIZE sizeof(struct dir_entry)


/*读/写*/
#define WR_SECT(dev, sect_nr) rw_sector(DEV_WRITE, \
										dev, \
										(sect_nr) * SECTOR_SIZE, \
										SECTOR_SIZE, \
										TASK_FS, \ 
										fsbuf \
										);

#define RD_SECT(dev, sect_nr) rw_sector(DEV_READ, \
										dev, \
										(sect_nr) * SECTOR_SIZE, \
										SECTOR_SIZE, \
										TASK_FS, \
										fsbuf \
										);

#endif