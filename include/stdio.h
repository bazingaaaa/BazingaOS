#ifndef _BAZINGA_STDIO_H_
#define _BAZINGA_STDIO_H_

/* the assert macro */
#define ASSERT
#ifdef ASSERT
void assertion_failure(char *exp, char *file, char *base_file, int line);
#define assert(exp)  if (exp) ; \
        else assertion_failure(#exp, __FILE__, __BASE_FILE__, __LINE__)
#else
#define assert(exp)
#endif

#include "type.h"

/* EXTERN */
#define	EXTERN	extern	/* EXTERN is defined as extern except in global.c */

#define	STR_DEFAULT_LEN	1024

#define	O_CREAT		1
#define	O_RDWR		2

#define SEEK_SET	1
#define SEEK_CUR	2
#define SEEK_END	3

#define	MAX_PATH	128


/*vsprintf.c*/
int vsprintf(char* buf, const char* fmt, var_list args);
int sprintf(char* buf, const char* fmt, ...);

/*printf.c*/
void printf(char *fmt, ...);
void printl(char *fmt, ...);


/*打开文件*/
int open(const char* pathname, int flags);


/*关闭文件*/
int close(int fd);


/*读文件*/
int read(int fd, char* buf, int size);


/*写文件*/
int write(int fd, const char* buf, int size);


/*删除文件*/
int unlink(const char* pathname);


/*操作文件指针*/
off_t lseek(int fd, off_t offset, int whence);


/*创建进程*/
int fork();


/*等待子进程结束*/
int wait(int *status);


/*进程退出*/
void exit(int status);

#endif
