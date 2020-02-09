/*
功能：打开文件具体操作
*/
PUBLIC int do_open(MESSAGE *msg)
{

}


/*
功能：关闭文件操作
*/
PUBLIC int do_close(MESSAGE *msg)
{

}


/*
功能：打开文件
*/
PUBLIC int open(const char* pathname, int flags)
{

}


/*
功能：在指定设备上分配imap图位
输入：dev-次设备号
返回值：返回inode号(0为无效inode号)
*/
PRIVATE int alloc_imap_bit(int dev)
{
	return 0;
}	


/*
功能：在指定设备上分配smap图位
输入：dev-次设备号
	  nr_sects_to_alloc-需要分配的扇区数
*/
PRIVATE int alloc_smap_bit(int dev, int nr_sects_to_alloc)
{
	return 0;
}
