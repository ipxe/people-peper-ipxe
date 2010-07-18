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

#ifndef _IPXE_LINUX_PCI_H
#define _IPXE_LINUX_PCI_H

FILE_LICENCE(GPL2_OR_LATER);

/** @file
 *
 * iPXE PCI I/O API for Linux
 *
 */

#ifdef PCIAPI_LINUX
#define PCIAPI_PREFIX_linux
#else
#define PCIAPI_PREFIX_linux __linux_
#endif

struct pci_device;

extern int lpci_read(struct pci_device *pci, unsigned int where, void *value, size_t size);
extern int lpci_write(struct pci_device *pci, unsigned int where, unsigned long value, size_t size);

/**
 * Read byte from PCI configuration space via linux
 *
 * @v pci	PCI device
 * @v where	Location within PCI configuration space
 * @v value	Value read
 * @ret rc	Return status code
 */
static inline __always_inline int
PCIAPI_INLINE(linux, pci_read_config_byte)(struct pci_device *pci, unsigned int where, uint8_t *value)
{
	return lpci_read(pci, where, value, sizeof(*value));
}

/**
 * Read word from PCI configuration space via linux
 *
 * @v pci	PCI device
 * @v where	Location within PCI configuration space
 * @v value	Value read
 * @ret rc	Return status code
 */
static inline __always_inline int
PCIAPI_INLINE(linux, pci_read_config_word)(struct pci_device *pci, unsigned int where, uint16_t *value)
{
	return lpci_read(pci, where, value, sizeof(*value));
}

/**
 * Read dword from PCI configuration space via linux
 *
 * @v pci	PCI device
 * @v where	Location within PCI configuration space
 * @v value	Value read
 * @ret rc	Return status code
 */
static inline __always_inline int
PCIAPI_INLINE(linux, pci_read_config_dword)(struct pci_device *pci, unsigned int where, uint32_t *value)
{
	return lpci_read(pci, where, value, sizeof(*value));
}

/**
 * Write byte to PCI configuration space via linux
 *
 * @v pci	PCI device
 * @v where	Location within PCI configuration space
 * @v value	Value to be written
 * @ret rc	Return status code
 */
static inline __always_inline int
PCIAPI_INLINE(linux, pci_write_config_byte)(struct pci_device *pci, unsigned int where, uint8_t value)
{
	return lpci_write(pci, where, value, sizeof(value));
}

/**
 * Write word to PCI configuration space via linux
 *
 * @v pci	PCI device
 * @v where	Location within PCI configuration space
 * @v value	Value to be written
 * @ret rc	Return status code
 */
static inline __always_inline int
PCIAPI_INLINE(linux, pci_write_config_word)(struct pci_device *pci, unsigned int where, uint16_t value)
{
	return lpci_write(pci, where, value, sizeof(value));
}

/**
 * Write dword to PCI configuration space via linux
 *
 * @v pci	PCI device
 * @v where	Location within PCI configuration space
 * @v value	Value to be written
 * @ret rc	Return status code
 */
static inline __always_inline int
PCIAPI_INLINE(linux, pci_write_config_dword)(struct pci_device *pci, unsigned int where, uint32_t value)
{
	return lpci_write(pci, where, value, sizeof(value));
}

#endif /* _IPXE_LINUX_PCI_H */
