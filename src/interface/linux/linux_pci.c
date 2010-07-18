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

#include <errno.h>
#include <linux_api.h>
#include <ipxe/pci.h>
#include <ipxe/lpci.h>

/** @file
 *
 * iPXE PCI I/O API for linux
 *
 */

/** Find a corresponding lpci device and get its PCI config fd */
static int pci_to_config_fd(struct pci_device *pci)
{
	struct lpci_device *lpci;
	
	for_each_lpci(lpci) {
		if (pci->bus == lpci->bus && pci->devfn == lpci->devfn)
			return lpci->pci_config_fd;
	}

	return -1;
}

/** Read from the PCI config space */
int lpci_read(struct pci_device *pci, unsigned int where, void *value, size_t size)
{
	int config_fd = pci_to_config_fd(pci);

	if (linux_lseek(config_fd, where, SEEK_SET) == -1)
		return -1;

	if (linux_read(config_fd, value, size) != (ssize_t)size)
		return -1;

	return 0;
}

/** Write to the PCI config space */
int lpci_write(struct pci_device *pci, unsigned int where, unsigned long value, size_t size)
{
	int config_fd = pci_to_config_fd(pci);

	if (linux_lseek(config_fd, where, SEEK_SET) == -1)
		return -1;

	if (linux_write(config_fd, &value, size) != (ssize_t)size)
		return -1;

	return 0;
}

/**
 * Determine maximum PCI bus number within system
 *
 * @ret max_bus  Maximum bus number
 */
static int lpci_pci_max_bus(void)
{
	struct lpci_device *lpci;
	int max_bus = 0;

	for_each_lpci(lpci) {
		if (lpci->bus > max_bus)
			max_bus = lpci->bus;
	}

	return max_bus;
}

PROVIDE_PCIAPI(linux, pci_max_bus, lpci_pci_max_bus);
PROVIDE_PCIAPI_INLINE(linux, pci_read_config_byte);
PROVIDE_PCIAPI_INLINE(linux, pci_read_config_word);
PROVIDE_PCIAPI_INLINE(linux, pci_read_config_dword);
PROVIDE_PCIAPI_INLINE(linux, pci_write_config_byte);
PROVIDE_PCIAPI_INLINE(linux, pci_write_config_word);
PROVIDE_PCIAPI_INLINE(linux, pci_write_config_dword);
