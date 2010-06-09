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

FILE_LICENCE(GPL2_OR_LATER);

/** @file
 *
 * Implementation of most of the linux API.
 */

#include <linux_api.h>

#include <stdarg.h>
#include <asm/unistd.h>
#include <string.h>

__asmcall int linux_access(const char *pathname, int mode)
{
	return linux_syscall(__NR_access, pathname, mode);
}

__asmcall int linux_open(const char *pathname, int flags)
{
	return linux_syscall(__NR_open, pathname, flags);
}

__asmcall int linux_close(int fd)
{
	return linux_syscall(__NR_close, fd);
}

__asmcall ssize_t linux_read(int fd, void *buf, size_t count)
{
	return linux_syscall(__NR_read, fd, buf, count);
}

__asmcall ssize_t linux_write(int fd, const void *buf, size_t count)
{
	return linux_syscall(__NR_write, fd, buf, count);
}

__asmcall off_t linux_lseek(int fd, off_t offset, int whence)
{
	return linux_syscall(__NR_lseek, fd, offset, whence);
}

__asmcall int linux_fcntl(int fd, int cmd, ...)
{
	long arg;
	va_list list;

	va_start(list, cmd);
	arg = va_arg(list, long);
	va_end(list);

	return linux_syscall(__NR_fcntl, fd, cmd, arg);
}

__asmcall int linux_ioctl(int fd, int request, ...)
{
	void *arg;
	va_list list;

	va_start(list, request);
	arg = va_arg(list, void *);
	va_end(list);

	return linux_syscall(__NR_ioctl, fd, request, arg);
}

__asmcall int linux_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	return linux_syscall(__NR_poll, fds, nfds, timeout);
}

__asmcall int linux_nanosleep(const struct timespec *req, struct timespec *rem)
{
	return linux_syscall(__NR_nanosleep, req, rem);
}

__asmcall int linux_usleep(useconds_t usec)
{
	struct timespec ts = {
		.tv_sec = (long) (usec / 1000000),
		.tv_nsec = (long) (usec % 1000000) * 1000ul
	};

	return linux_nanosleep(&ts, NULL);
}

__asmcall int linux_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	return linux_syscall(__NR_gettimeofday, tv, tz);
}

__asmcall void *linux_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	return (void *)linux_syscall(__SYSCALL_mmap, addr, length, prot, flags, fd, offset);
}

__asmcall void *linux_mremap(void *old_address, size_t old_size, size_t new_size, int flags)
{
	return (void *)linux_syscall(__NR_mremap, old_address, old_size, new_size, flags);
}

__asmcall int linux_munmap(void *addr, size_t length)
{
	return linux_syscall(__NR_munmap, addr, length);
}

__asmcall int linux_iopl(int level)
{
	return linux_syscall(__NR_iopl, level);
}

__asmcall uid_t linux_getuid(void)
{
	return linux_syscall(__NR_getuid);
}
