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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

FILE_LICENCE(GPL2_OR_LATER);

#include <ipxe/io.h>
#include <ipxe/linux/linux_io.h>
#include <ipxe/lpci.h>
#include <linux_api.h>
#include <ipxe/linux/uio-dma.h>

/** @file
 *
 * iPXE I/O API for linux
 *
 */

static unsigned long linux_phys_to_bus(unsigned long phys_addr)
{
	return uio_dma_addr(lpci_dma_mapping, (uint8_t *)phys_addr, 1);
}


static unsigned long linux_bus_to_phys(unsigned long bus_addr)
{
	return (unsigned long)uio_maddr(lpci_dma_mapping, bus_addr, 1);
}

static void *linux_ioremap(unsigned long bus_addr, size_t len)
{
	struct lpci_device *lpci;
	struct lpci_iores *iomem;
	void *res;

	/* Scan all the I/O memory mappings */
	for_each_lpci(lpci) {
		list_for_each_entry(iomem, &lpci->iomems, list) {
			if (bus_addr >= iomem->start && bus_addr + len <= iomem->start + iomem->len) {
				res = iomem->mapped + (bus_addr - iomem->start);
				DBG("linux_io: found a mapping for [0x%016lx, 0x%016lx) -> [%p, %p)\n",
				    bus_addr, bus_addr + len, res, res + len);
				return res;
			}
		}
	}
	DBG("linux_io: didn't find a mapping for [0x%016lx, 0x%016lx)\n", bus_addr, bus_addr + len);
	return NULL;
}

static void linux_iodelay(void)
{
	linux_usleep(1);
}

static void linux_get_memmap ( struct memory_map *memmap )
{
	memmap->count = 0;
}

PROVIDE_IOAPI(linux, phys_to_bus, linux_phys_to_bus);
PROVIDE_IOAPI(linux, bus_to_phys, linux_bus_to_phys);
PROVIDE_IOAPI(linux, ioremap, linux_ioremap);
PROVIDE_IOAPI_INLINE(linux, iounmap);
PROVIDE_IOAPI_INLINE(linux, io_to_bus);
PROVIDE_IOAPI_INLINE(linux, readb);
PROVIDE_IOAPI_INLINE(linux, readw);
PROVIDE_IOAPI_INLINE(linux, readl);
PROVIDE_IOAPI_INLINE(linux, readq);
PROVIDE_IOAPI_INLINE(linux, writeb);
PROVIDE_IOAPI_INLINE(linux, writew);
PROVIDE_IOAPI_INLINE(linux, writel);
PROVIDE_IOAPI_INLINE(linux, writeq);
PROVIDE_IOAPI_INLINE(linux, inb);
PROVIDE_IOAPI_INLINE(linux, inw);
PROVIDE_IOAPI_INLINE(linux, inl);
PROVIDE_IOAPI_INLINE(linux, outb);
PROVIDE_IOAPI_INLINE(linux, outw);
PROVIDE_IOAPI_INLINE(linux, outl);
PROVIDE_IOAPI_INLINE(linux, insb);
PROVIDE_IOAPI_INLINE(linux, insw);
PROVIDE_IOAPI_INLINE(linux, insl);
PROVIDE_IOAPI_INLINE(linux, outsb);
PROVIDE_IOAPI_INLINE(linux, outsw);
PROVIDE_IOAPI_INLINE(linux, outsl);
PROVIDE_IOAPI(linux, iodelay, linux_iodelay);
PROVIDE_IOAPI_INLINE(linux, mb);
PROVIDE_IOAPI(linux, get_memmap, linux_get_memmap);
