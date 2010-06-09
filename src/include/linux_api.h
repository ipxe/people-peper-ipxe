/*
 * Copyright (C) 2010 Piotr Jaroszyński <p.jaroszynski@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _LINUX_API_H
#define _LINUX_API_H

/** * @file
 *
 * Linux API prototypes.
 * Most of the functions map directly to linux syscalls and are the equivalent
 * of POSIX functions with the linux_ prefix removed.
 */

FILE_LICENCE(GPL2_OR_LATER);

#include <bits/linux_api.h>
#include <bits/linux_api_platform.h>

#include <stdint.h>

typedef int pid_t;

#include <linux/types.h>
#include <linux/posix_types.h>
#include <linux/time.h>
#include <linux/mman.h>
#include <linux/fcntl.h>
#include <linux/ioctl.h>
#include <linux/poll.h>
#include <linux/fs.h>

typedef uint32_t useconds_t;

extern long __asmcall linux_syscall(int number, ...);

extern __asmcall int linux_access(const char *pathname, int mode) asm("access");
extern __asmcall int linux_open(const char *pathname, int flags) asm("open");
extern __asmcall int linux_close(int fd) asm("close");
extern __asmcall ssize_t linux_read(int fd, void *buf, size_t count) asm("read");
extern __asmcall ssize_t linux_write(int fd, const void *buf, size_t count) asm("write");
extern __asmcall off_t linux_lseek(int fd, off_t offset, int whence) asm("lseek");
extern __asmcall int linux_fcntl(int fd, int cmd, ...) asm("fcntl");
extern __asmcall int linux_ioctl(int fd, int request, ...) asm("ioctl");

typedef unsigned long nfds_t;
extern __asmcall int linux_poll(struct pollfd *fds, nfds_t nfds, int timeout) asm("poll");

extern __asmcall int linux_nanosleep(const struct timespec *req, struct timespec *rem) asm("nanosleep");
extern __asmcall int linux_usleep(useconds_t usec) asm("usleep");
extern __asmcall int linux_gettimeofday(struct timeval *tv, struct timezone *tz) asm("gettimeofday");

#define MAP_FAILED ((void *)-1)

extern __asmcall void *linux_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) asm("mmap");
extern __asmcall void *linux_mremap(void *old_address, size_t old_size, size_t new_size, int flags) asm("mremap");
extern __asmcall int linux_munmap(void *addr, size_t length) asm("munmap");

extern __asmcall int linux_iopl(int level) asm("iopl");

typedef __kernel_uid_t uid_t;
extern __asmcall uid_t linux_getuid(void) asm("getuid");

extern __asmcall const char *linux_strerror(int errnum) asm("strerror");

#endif /* _LINUX_API_H */
