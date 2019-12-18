
#include <sys/types.h>
#include <sys/errno.h>

#undef errno
extern int errno;

void
_fini(void)
{
}

void
_exit(__unused int status)
{
  while (1) {
    ;
  }
}

caddr_t
_sbrk(__unused int incr)
{
  errno = ENOSYS;
  return (caddr_t) -1;
}

int
_read(__unused int fd, __unused char* buf, __unused int size)
{
  errno = ENOSYS;
  return -1;
}

int
_write(__unused int fd, __unused char* buf, __unused int length)
{
  errno = ENOSYS;
  return -1;
}

int
_close(__unused int fd)
{
  errno = ENOSYS;
  return -1;
}

int
_fstat(__unused int fd, __unused void* stat)
{
  errno = ENOSYS;
  return -1;
}

int
_isatty(__unused int fd)
{
  errno = ENOSYS;
  return 0;
}

int
_lseek(__unused int fd, __unused int offset, __unused int whence)
{
  errno = ENOSYS;
  return -1;
}

