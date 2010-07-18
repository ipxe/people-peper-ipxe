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

#include <ipxe/lpci.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <linux_api.h>
#include <ipxe/linux.h>
#include <ipxe/pci.h>
#include <ipxe/malloc.h>
#include <ipxe/netdevice.h>
#include <ipxe/init.h>

/**
 * @file
 *
 * Implementation of the linux PCI devices
 *
 * Most of the work is accessing /sys/ intefaces exposed by the linux PCI subsystem.
 * See linux/Documentation/filesystems/sysfs-pci.txt for details.
 */

#define UIO_DMA_MEM_SIZE (128 * 1024)

#define UIO_DMA_ID_LEN 10

LIST_HEAD(lpci_devices);

int lpci_ioports_ready = 0;

/** Allocate and initialize an lpci device */
static struct lpci_device *alloc_lpci_device()
{
	struct lpci_device *lpci;

	lpci = zalloc(sizeof(*lpci));

	if (lpci) {
		lpci->pci_config_fd = -1;
		INIT_LIST_HEAD(&lpci->iomems);
		INIT_LIST_HEAD(&lpci->ioports);
	}

	return lpci;
}

/** Open the PCI config available via the /sys/ interface */
static int lpci_open_config(struct lpci_device *lpci)
{
	char path[SYS_PATH_LEN];

	if (lpci->pci_config_fd != -1) {
		DBGC(lpci, "lpci %p config '%s' already open\n", lpci, path);
		return -1;
	}

	snprintf(path, SYS_PATH_LEN, "%sconfig", lpci->sys_path);
	lpci->pci_config_fd = linux_open(path, O_RDWR);

	if (lpci->pci_config_fd == -1) {
		DBGC(lpci, "lpci %p open('%s') failed (%s)\n", lpci, path, linux_strerror(linux_errno));
		return -1;
	}

	return 0;
}

/** Close the PCI config */
static void lpci_close_config(struct lpci_device *lpci)
{
	linux_close(lpci->pci_config_fd);
}

/**
 * Map a specific I/O resource available via the /sys/ interface
 *
 * @v lpci        lpci device
 * @v iores_list  List to add the newly mapped I/O resource to
 * @v rind        Index of the resource necessary to find it in /sys/
 * @v start       System address of the resouce
 * @v len         Length of the I/O resource in bytes
 * @ret rc        0 on success
 */
static int lpci_map_iores(struct lpci_device *lpci, struct list_head *iores_list,
		int rind, unsigned long long start, size_t len)
{
	int fd;
	char path[SYS_PATH_LEN];
	struct lpci_iores * iores;
	int rc = 0;

	iores = malloc(sizeof(*iores));

	if (! iores) {
		return -ENOMEM;
	}

	iores->start = start;
	iores->len = len;

	snprintf(path, SYS_PATH_LEN, "%sresource%d", lpci->sys_path, rind);
	fd = linux_open(path, O_RDWR);

	if (fd == -1) {
		DBGC(lpci, "lpci %p open('%s', O_RDWR) failed (%s)\n", lpci, path, linux_strerror(linux_errno));
		rc = fd;
		goto err_open;
	}
	DBGC(lpci, "lpci %p mmapping iores [0x%016llx, 0x%016llx)\n", lpci, start, start + len);

	iores->mapped = linux_mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (iores->mapped == MAP_FAILED) {
		DBGC(lpci, "lpci %p mmap failed (%s)\n", lpci, linux_strerror(linux_errno));
		rc = -1;
		goto err_mmap;
	}

	list_add(&iores->list, iores_list);

	linux_close(fd);

	DBGC(lpci, "lpci %p mapped iores [0x%016llx, 0x%016llx) to %p\n", lpci, start, start + len, iores->mapped);

	return 0;

err_mmap:
	linux_close(fd);
err_open:
	free(iores);

	return rc;
}

/** Unmap all I/O resources on the list */
static void lpci_unmap_iores(struct list_head *iores_list)
{
	struct lpci_iores * iores;
	struct lpci_iores * tmp;

	list_for_each_entry_safe(iores, tmp, iores_list, list) {
		linux_munmap(iores->mapped, iores->len);
		list_del(&iores->list);
		free(iores);
	}
}

/** Map I/O resources of the device available via the /sys/ interface */
static int lpci_map_resources(struct lpci_device *lpci)
{
	int fd;
	char path[SYS_PATH_LEN];
	char line[SYS_PCI_RESOURCE_LINE_WIDTH];
	unsigned long long r_start, r_end, r_flags;
	int r_ind = 0;
	char * end = line;

	strcpy(path, lpci->sys_path);
	strcat(path, "resource");

	fd = linux_open(path, O_RDONLY);

	if (fd == -1) {
		DBGC(lpci, "lpci %p open('%s', O_RDONLY) = %d (%s)\n", lpci, path, fd, linux_strerror(linux_errno));
		return -1;
	}

	while (r_ind <= PCI_STD_RESOURCE_END &&
			linux_read(fd, line, SYS_PCI_RESOURCE_LINE_WIDTH) == SYS_PCI_RESOURCE_LINE_WIDTH) {
		line[SYS_PCI_RESOURCE_LINE_WIDTH - 1] = 0;

		r_start = strtoull(end, &end, 0);
		r_end = strtoull(end, &end, 0);
		r_flags = strtoull(end, &end, 0);
		end = line;

		if (r_flags & IORESOURCE_MEM) {
			lpci_map_iores(lpci, &lpci->iomems, r_ind, r_start, r_end - r_start + 1);
		}
		/* Doesn't work on x86, see pci_mmap_page_range() in linux/arch/x86/pci/i386.c */
#if 0
		if (r_flags & IORESOURCE_IO) {
			lpci_map_iores(lpci, &lpci->ioports, r_ind, r_start, r_end - r_start + 1);
		}
#endif

		++r_ind;
	}

	linux_close(fd);

	return 0;
}

/**
 * Parse a device id into domain, bus, device and function number
 *
 * Accept ids in the following format [domain_hex:]bus_hex:device_hex:function_decimal
 *   hex numbers w/o the '0x' prefix
 *   : can be any string not containing hex digits
 */
static int lpci_parse_dev_id(char *id, uint16_t *domain, uint8_t *bus, uint8_t *devfn)
{
	unsigned long parts[4];
	char * p;
	int i = 0;

	for (p = id ; *p && i < 4 ; ) {
		if (! isxdigit(*p))
			++p;
		else
			parts[i++] = strtoul(p, &p, 16);
	}

	/* parsed all? */
	if (*p)
		return -1;

	/* domain present? */
	if (i == 3) {
		*domain = 0;
	} else if (i == 4) {
		*domain = parts[0];
	} else {
		return -1;
	}

	/* bus < 256 ? */
	if (parts[i - 3] >= 256)
		return -1;

	*bus = parts[i - 3];

	/* device < 32 ? */
	if (parts[i - 2] >= 32)
		return -1;

	/* function < 8 ? */
	if (parts[i - 1] >= 8)
		return -1;

	*devfn = PCI_DEVFN(parts[i - 2], parts[i - 1]);

	return 0;
}

/** Used to save the old DMA pool to restore it on remove */
static struct memory_pool *old_dma_pool;

/** The pool to be used while lpci devices are active */
static struct memory_pool lpci_dma_pool = {
	.free_blocks = LIST_HEAD_INIT(lpci_dma_pool.free_blocks),
};

struct uio_dma_mapping *lpci_dma_mapping;

static int uio_dma_fd = -1;
static struct uio_dma_area *uio_dma_area;

/** Do some basic sanity checks */
static int lpci_precheck(struct lpci_device *lpci)
{
	if (linux_getuid() != 0) {
		printf("You need to be root to use lpci devices\n");
		return -EPERM;
	}

	if (linux_access(lpci->sys_path, O_RDONLY) != 0) {
		printf("Cannot access '%s': (%s)\n", lpci->sys_path, linux_strerror(linux_errno));
		return -ENODEV;
	}

	return 0;
}

/** Get the UIO-DMA device id from /sys */
static int lpci_get_uio_dma_id(struct lpci_device *lpci)
{
	char path[SYS_PATH_LEN];
	char buf[UIO_DMA_ID_LEN + 1];
	int fd;
	int rc = 0;

	snprintf(path, SYS_PATH_LEN, "%suio_dma_id", lpci->sys_path);
	fd = linux_open(path, O_RDONLY);
	if (fd == -1) {
		DBGC(lpci, "lpci %p open('%s') failed (%s)\n", lpci, path, linux_strerror(linux_errno));
		return -1;
	}

	if (linux_read(fd, buf, UIO_DMA_ID_LEN) != UIO_DMA_ID_LEN) {
		DBGC(lpci, "lpci %p read('%s') failed (%s)\n", lpci, path, linux_strerror(linux_errno));
		rc = -1;
		goto close;
	}

	buf[UIO_DMA_ID_LEN] = '\0';

	lpci->uio_dma_id = strtoul(buf, NULL, 0);

close:
	linux_close(fd);

	return rc;
}

/**
 * Initialize the DMA mappings with UIO-DMA and switch the memory_pool used
 * by DMA allocations.
 */
static int lpci_init_dma(struct lpci_device *lpci)
{
	if (uio_dma_fd != -1) {
		DBG("uio_dma_fd already open\n");
		return -1;
	}

	uio_dma_fd = uio_dma_open();

	if (uio_dma_fd == -1) {
		DBG("lpci uio_dma_open() failed (%s)\n", linux_strerror(linux_errno));
		goto err_open;
	}

	/* Use a 32bit mask as some drivers might be assuming it */
	uio_dma_area = uio_dma_alloc(uio_dma_fd, UIO_DMA_MEM_SIZE, UIO_DMA_CACHE_DISABLE, UIO_DMA_MASK(32), 0);
	if (! uio_dma_area) {
		DBG("lpci uio_dma_alloc() failed (%s)\n", linux_strerror(linux_errno));
		goto err_alloc;
	}

	lpci_dma_mapping = uio_dma_map(uio_dma_fd, uio_dma_area, lpci->uio_dma_id, UIO_DMA_BIDIRECTIONAL);
	if (! lpci_dma_mapping) {
		DBG("lpci uio_dma_map() failed (%s)\n", linux_strerror(linux_errno));
		goto err_map;
	}

	if (lpci_dma_mapping->chunk_count != 1) {
		DBG("lpci uio_dma_map() returned %d chunks, we only support 1\n", lpci_dma_mapping->chunk_count);
		goto err_chunks;
	}

	mpopulate(&lpci_dma_pool, lpci_dma_mapping->addr, UIO_DMA_MEM_SIZE);
	old_dma_pool = set_dma_pool(&lpci_dma_pool);

	return 0;

err_chunks:
	uio_dma_unmap(uio_dma_fd, lpci_dma_mapping);
err_map:
	uio_dma_free(uio_dma_fd, uio_dma_area);
err_alloc:
	uio_dma_close(uio_dma_fd);
	uio_dma_fd = -1;
err_open:
	return -1;
}

/**
 * Clean up the UIO-DMA mappings and restore the old memory_pool for DMA
 * allocations.
 */
static void lpci_clean_dma()
{
	set_dma_pool(old_dma_pool);

	uio_dma_unmap(uio_dma_fd, lpci_dma_mapping);
	uio_dma_free(uio_dma_fd, uio_dma_area);
	uio_dma_close(uio_dma_fd);
}

/** Initialize access to the I/O ports */
static int lpci_init_ioports()
{
	if (lpci_ioports_ready)
		return 0;

	if (linux_iopl(3) != 0) {
		DBG("lpci iopl(3) failed (%s)\n", linux_strerror(linux_errno));
		return -EPERM;
	}
	lpci_ioports_ready = 1;

	return 0;
}

struct linux_driver lpci_driver __linux_driver;

/** Handle a request for an lpci device */
static int lpci_probe(struct linux_device *device, struct linux_device_request *request)
{
	struct linux_setting *dev_setting;
	uint16_t domain = 0;
	uint8_t bus = 0;
	uint8_t devfn = 0;
	struct lpci_device *lpci;
	int rc;

	/* Look for the mandatory dev setting */
	dev_setting = linux_find_setting("dev", &request->settings);

	if (! dev_setting) {
		printf("lpci missing mandatory dev setting\n");
		return -EINVAL;
	}

	dev_setting->applied = 1;

	if (lpci_parse_dev_id(dev_setting->value, &domain, &bus, &devfn)) {
		printf("lpci missing mandatory dev setting\n");
		return -ENODEV;
	}

	lpci = alloc_lpci_device();
	if (! lpci)
		return -ENOMEM;

	lpci->settings = &request->settings;
	lpci->domain = domain;
	lpci->bus = bus;
	lpci->devfn = devfn;
	sprintf(lpci->sys_path, SYS_PCI_DEVICES_PATH "%04x:%02x:%02x.%d/",
			domain, bus, PCI_SLOT(devfn), PCI_FUNC(devfn));

	linux_set_drvdata(device, lpci);

	rc = lpci_precheck(lpci);
	if (rc) {
		goto err_precheck;
	}

	rc = lpci_get_uio_dma_id(lpci);
	if (rc) {
		printf("lpci failed to get UIO-DMA device id. Is the uio-dma-pci driver bound to the device?\n");
		goto err_uio_dma_id;
	}

	rc = lpci_open_config(lpci);
	if (rc) {
		printf("lpci failed to open PCI config\n");
		goto err_config;
	}

	rc = lpci_map_resources(lpci);
	if (rc) {
		printf("lpci failed to map I/O resources\n");
		goto err_map;
	}

	rc = lpci_init_ioports();
	if (rc) {
		printf("lpci ioprts not ready\n");
		goto err_init_ioports;
	}

	rc = lpci_init_dma(lpci);
	if (rc) {
		printf("lpci DMA memory not ready\n");
		goto err_init_dma;
	}

	list_add(&lpci->list, &lpci_devices);

	DBGC(lpci, "lpci %p added device [%04x:%02x:%02x.%d]\n", lpci, domain, bus, PCI_SLOT(devfn), PCI_FUNC(devfn));

	/*
	 * We can handle only one device at the same time.
	 * To support multiple devices time malloc_dma() and phys_to_bus()
	 * conversion would have to know for which device they are being done.
	 */
	lpci_driver.can_probe = 0;

	return 0;

err_init_dma:
err_init_ioports:
err_map:
	lpci_unmap_iores(&lpci->iomems);
	lpci_unmap_iores(&lpci->ioports);
	lpci_close_config(lpci);
err_uio_dma_id:
err_precheck:
err_config:
	free(lpci);

	return rc;
}

/** Remove the lpci device */
static void lpci_remove(struct linux_device *device)
{
	struct lpci_device *lpci = linux_get_drvdata(device);

	lpci_clean_dma();

	lpci_unmap_iores(&lpci->iomems);
	lpci_unmap_iores(&lpci->ioports);
	lpci_close_config(lpci);
	list_del(&lpci->list);

	free(lpci);
}

/** lpci linux_driver */
struct linux_driver lpci_driver __linux_driver = {
	.name = "lpci",
	.probe = lpci_probe,
	.remove = lpci_remove,
	.can_probe = 1,
};

/** Apply lpci settings to corresponding net devices */
void lpci_apply_settings(void)
{
	struct lpci_device *lpci;
	struct net_device *ndev;

	/* Look for the net_device corresponding to each lpci device */
	for_each_lpci(lpci) {
		for_each_netdev(ndev) {
			if (ndev->dev->desc.bus_type == BUS_TYPE_PCI &&
					ndev->dev->desc.location == (unsigned int)PCI_BUSDEVFN(lpci->bus, lpci->devfn)) {
				/* Apply the lpci settings to the net device */
				linux_apply_settings(lpci->settings, &ndev->settings.settings);
			}
		}
	}
}

struct startup_fn lpci_apply_settings_startup_fn __startup_fn(STARTUP_LATE) = {
	.startup = lpci_apply_settings,
};
