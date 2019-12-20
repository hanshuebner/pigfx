
#include <sys/types.h>
#include <sys/errno.h>

#undef errno
extern int errno;

void *const __dso_handle = 0;

void
__sync_synchronize()
{
}

void
_fini()
{
}

void
_exit(__unused int status)
{
  while (1) {
    ;
  }
}

extern caddr_t pheap_space;
extern unsigned int heap_sz;
static caddr_t heap_end;

void
heap_init()
{
  heap_end = pheap_space + heap_sz;
}

caddr_t
_sbrk(int incr)
{
  caddr_t space = pheap_space;
  pheap_space += incr;
  if (pheap_space >= heap_end) {
    return (caddr_t) -1;
  } else {
    return space;
  }
}

int
_open(__unused const char* pathname, __unused int flags, __unused int mode)
{
  errno = ENOSYS;
  return -1;
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

