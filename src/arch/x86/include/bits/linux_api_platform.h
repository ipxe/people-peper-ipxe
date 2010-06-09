#ifndef _LINUX_API_PLATFORM_H
#define _LINUX_API_PLATFORM_H

#ifndef PLATFORM_linuxlibc

extern int linux_errno;

#else

/* __errno_location is defined by stdlibs */
extern int *__errno_location(void) asm("__errno_location");
#define linux_errno (*__errno_location ())

#endif

#endif /* _LINUX_API_PLATFORM_H */
