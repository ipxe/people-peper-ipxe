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

#ifndef _IPXE_LINUX_IO_H
#define _IPXE_LINUX_IO_H

FILE_LICENCE(GPL2_OR_LATER);

#include <ipxe/linux/uio-dma.h>
#include <ipxe/lpci.h>

/** @file
 *
 * iPXE I/O API for Linux
 */

#ifdef IOAPI_LINUX
#define IOAPI_PREFIX_linux
#else
#define IOAPI_PREFIX_linux __linux_
#endif

static inline __always_inline void
IOAPI_INLINE(linux, iounmap)(volatile const void *io_addr __unused)
{
	/* Nothing to do */
}

static inline __always_inline unsigned long
IOAPI_INLINE(linux, io_to_bus)(volatile const void *io_addr)
{
	return ((unsigned long) io_addr);
}

static inline __always_inline uint8_t
IOAPI_INLINE(linux, readb)(volatile uint8_t *io_addr)
{
	return *io_addr;
}

static inline __always_inline uint16_t
IOAPI_INLINE(linux, readw)(volatile uint16_t *io_addr)
{
	return *io_addr;
}

static inline __always_inline uint32_t
IOAPI_INLINE(linux, readl)(volatile uint32_t *io_addr)
{
	return *io_addr;
}

static inline __always_inline uint64_t
IOAPI_INLINE(linux, readq)(volatile uint64_t *io_addr)
{
	return *io_addr;
}

static inline __always_inline void
IOAPI_INLINE(linux, writeb)(uint8_t data, volatile uint8_t *io_addr)
{
	*io_addr = data;
}

static inline __always_inline void
IOAPI_INLINE(linux, writew)(uint16_t data, volatile uint16_t *io_addr)
{
	*io_addr = data;
}

static inline __always_inline void
IOAPI_INLINE(linux, writel)(uint32_t data, volatile uint32_t *io_addr)
{
	*io_addr = data;
}

static inline __always_inline void
IOAPI_INLINE(linux, writeq)(uint64_t data, volatile uint64_t *io_addr)
{
	*io_addr = data;
}

/* in/out from x86. Aborting if I/O ports weren't initialized */
#define LINUX_INX( _insn_suffix, _type, _reg_prefix )			      \
static inline __always_inline _type					      \
IOAPI_INLINE ( linux, in ## _insn_suffix ) ( volatile _type *io_addr ) {      \
	if (! lpci_ioports_ready)					      \
		return (_type)0xffffffff;				      \
	_type data;							      \
	__asm__ __volatile__ ( "in" #_insn_suffix " %w1, %" _reg_prefix "0"   \
			       : "=a" ( data ) : "Nd" ( io_addr ) );	      \
	return data;							      \
}									      \
static inline __always_inline void					      \
IOAPI_INLINE ( linux, ins ## _insn_suffix ) ( volatile _type *io_addr,	      \
					    _type *data,		      \
					    unsigned int count ) {	      \
	if (! lpci_ioports_ready)					      \
		return;							      \
	unsigned int discard_D;						      \
	__asm__ __volatile__ ( "rep ins" #_insn_suffix			      \
			       : "=D" ( discard_D )			      \
			       : "d" ( io_addr ), "c" ( count ),	      \
				 "0" ( data ) );			      \
}
LINUX_INX(b, uint8_t, "b");
LINUX_INX(w, uint16_t, "w");
LINUX_INX(l, uint32_t, "k");

#define LINUX_OUTX( _insn_suffix, _type, _reg_prefix )			      \
static inline __always_inline void					      \
IOAPI_INLINE ( linux, out ## _insn_suffix ) ( _type data,		      \
					    volatile _type *io_addr ) {	      \
	if (! lpci_ioports_ready)					      \
		return;							      \
	__asm__ __volatile__ ( "out" #_insn_suffix " %" _reg_prefix "0, %w1"  \
			       : : "a" ( data ), "Nd" ( io_addr ) );	      \
}									      \
static inline __always_inline void					      \
IOAPI_INLINE ( linux, outs ## _insn_suffix ) ( volatile _type *io_addr,	      \
					     const _type *data,		      \
					     unsigned int count ) {	      \
	unsigned int discard_S;						      \
	if (! lpci_ioports_ready)					      \
		return;							      \
	__asm__ __volatile__ ( "rep outs" #_insn_suffix			      \
			       : "=S" ( discard_S )			      \
			       : "d" ( io_addr ), "c" ( count ),	      \
				 "0" ( data ) );			      \
}
LINUX_OUTX(b, uint8_t, "b");
LINUX_OUTX(w, uint16_t, "w");
LINUX_OUTX(l, uint32_t, "k");

static inline __always_inline void IOAPI_INLINE(linux, mb)(void)
{
	/* gcc's built-in for a memory barrier */
	__sync_synchronize();
}

#endif /* _IPXE_LINUX_IO_H */
