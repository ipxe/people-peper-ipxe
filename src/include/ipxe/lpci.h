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

#ifndef _IPXE_LPCI_H
#define _IPXE_LPCI_H

FILE_LICENCE(GPL2_OR_LATER);

#include <ipxe/list.h>

#define SYS_PATH_LEN 64 /* enough to contain any path under /sys we need */
#define SYS_PCI_DEVICES_PATH "/sys/bus/pci/devices/"
#define SYS_PCI_RESOURCE_LINE_WIDTH (18 * 3 + 1 * 3)

#define IORESOURCE_TYPE_BITS  0x00001f00 /* Resource type */
#define IORESOURCE_IO         0x00000100
#define IORESOURCE_MEM        0x00000200
#define IORESOURCE_IRQ        0x00000400
#define IORESOURCE_DMA        0x00000800
#define IORESOURCE_BUS        0x00001000

#define PCI_STD_RESOURCE_END 5

/** A linux PCI device
 *
 * It is used under the hood to provide I/O and PCI API on linux
 */
struct lpci_device {
	/** Domain number */
	uint16_t domain;
	/** Bus number */
	uint8_t bus;
	/** Device and function number */
	uint8_t devfn;
	/** Base path of the device under /sys/ */
	char sys_path[SYS_PATH_LEN];
	/** File descriptor of an opened PCI config file under /sys/ */
	int pci_config_fd;
	/** List node */
	struct list_head list;
	/** List of mmaped I/O memory */
	struct list_head iomems;
	/** List of mmaped I/O ports */
	struct list_head ioports;
	/** UIO-DMA device id */
	uint32_t uio_dma_id;
	/** Settings list */
	struct list_head *settings;
};

/** List of all linux PCI devices */
extern struct list_head lpci_devices;

/** Iterate over all lpci devices */
#define for_each_lpci( lpci ) \
	list_for_each_entry ( (lpci), &lpci_devices, list )

/** An mmaped() I/O resource */
struct lpci_iores {
	unsigned long long start;
	size_t len;
	void * mapped;
	struct list_head list;
};

/** Are ioports ready to be used? */
extern int lpci_ioports_ready;

/** lpci DMA mapping */
extern struct uio_dma_mapping * lpci_dma_mapping;

#endif /* _IPXE_LPCI_H */
