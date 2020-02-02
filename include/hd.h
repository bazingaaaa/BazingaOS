#ifndef _HD_H_
#define _HD_H_


#define MAKE_DEVICE_REG(lba, drv, lba_highest) (lba << 6 | drv << 4 | (lba_highest & 0xf) | 0xA0)


#define REG_DATA 0x1F0
#define REG_FEATURE 0x1F1
#define REG_SECTOR_COUNT 0x1F2
#define REG_LBA_LOW 0x1F3
#define REG_LBA_MID 0x1F4
#define REG_LBA_HIGH 0x1F5
#define REG_DEV 0x1F6
#define REG_CMD 0x1F7
#define REG_STATUS REG_CMD

#define REG_DEV_CTRL	0x3F6		/*	Device Control			O		*/



#define	STATUS_BSY	0x80
#define	STATUS_DRDY	0x40
#define	STATUS_DFSE	0x20
#define	STATUS_DSC	0x10
#define	STATUS_DRQ	0x08
#define	STATUS_CORR	0x04
#define	STATUS_IDX	0x02
#define	STATUS_ERR	0x01

#define	HD_TIMEOUT		10000	/* in millisec */

#define ATA_IDENTIFY		0xEC
#define ATA_READ		0x20
#define ATA_WRITE		0x30

#define SECTOR_SIZE 512

typedef struct 
{
	u8 features;
	u8 count;
	u8 lba_low;
	u8 lba_mid;
	u8 lba_high;
	u8 device;
	u8 command;	
}hd_cmd;



#endif