#ifndef _PCI_H_
#define _PCI_H_
#pragma once
/*
 * VBSMOUNT
 *  A network redirector for mounting VirtualBox's Shared Folders in DOS 
 *  Copyright (C) 2013  Eduardo Casino
 *
 * pci.h: PCI helper functions
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 */
 
#include <stdint.h>

extern int pci_check( void );
extern int pci_find_device( uint16_t venid, uint16_t devid, uint8_t *bus, uint8_t *dev_func );
extern int pci_get_iobase( uint8_t bus, uint8_t dev_func, uint16_t *port );

#endif /* _PCI_H_ */
