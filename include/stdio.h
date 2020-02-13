#define	STR_DEFAULT_LEN	1024

#define	O_CREAT		1
#define	O_RDWR		2

#define SEEK_SET	1
#define SEEK_CUR	2
#define SEEK_END	3

#define	MAX_PATH	128


/*打开文件*/
PUBLIC int open(const char* pathname, int flags);


/*关闭文件*/
PUBLIC int close(int fd);


/*读文件*/
PUBLIC int read(int fd, char* buf, int size);


/*写文件*/
PUBLIC int write(int fd, const char* buf, int size);
