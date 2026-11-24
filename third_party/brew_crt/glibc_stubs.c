/* =========================================================================

FILE: glibc_stubs.c


DESCRIPTION
  CodeSourcery expects us to provide do-nothing stubs (or subroutines with minimal
  functionality) to allow programs to link with the subroutines in libc.a. 
  http://sourceware.org/newlib/libc.html#Syscalls
  
  This file define those subroutines.
  
PUBLIC CLASSES:  
   N/A

INITIALIZATION AND SEQUENCING REQUIREMENTS:  N/A


           Copyright (c)  2010 QUALCOMM Incorporated.
                     All Rights Reserved.
              QUALCOMM Proprietary and Confidential 
========================================================================= */


#include <unistd.h>
#include <errno.h>
#undef EALREADY

void abort (void) { while(1); }
void __cxa_pure_virtual (void) {}

static char *__env[1] = { 0 };
char **environ = __env;
#undef errno
extern int errno;
struct stat *st;
struct tms *buf;

int _close(int file) 
{
    return -1;
}

int _execve(char *name, char **argv, char **env) 
{
	errno = ENOMEM;
	return -1;
}

int fork(void) 
{
	errno = EAGAIN;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    return -1;
}

int _kill (int pid, int sig)
{
	return -1;
}

int _getpid (int n)
{
	return 1;
}

int _isatty(int file)
{
    return 1;
}

int _link(char *old, char *new) 
{
	errno = EMLINK;
	return -1;
}

int _lseek(int file, int ptr, int dir) 
{
    return -1;
}

int _open(const char *name, int flags, int mode) 
{
    return -1;
}

int _read(int file, char *ptr, int len) 
{
	return -1;
}

caddr_t _sbrk (int incr)
{
    return NULL;
}

int _stat(char *file, struct stat *st) 
{
	return -1;
}

int times(struct tms *buf) 
{
    return -1;
}

int _unlink(char *name) 
{
    errno = ENOENT;
    return -1;
}

int wait(int *status) 
{
    errno = ECHILD;
    return -1;
}

int _write(int file, char *ptr, int len) 
{
	return -1;
}