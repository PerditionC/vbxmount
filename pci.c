/*
 * VBSMOUNT
 *  A network redirector for mounting VirtualBox's Shared Folders in DOS 
 *  Copyright (C) 2013  Eduardo Casino
 *
 * pci.c: PCI helper functions
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
#include <i86.h>

#define PCI_INTERRUPT			0x1a

#define PCI_INSTALLATION_CHECK	0xb101
#define PCI_FIND_DEVICE			0xb102
#define PCI_READ_CONFIG_WORD	0xb109

#define PCI_OFFSET_IOBASE		0x0010
#define	PCI_IO_ADDR_MASK		0xfffe

#define PCI_SUCCESS				0x00


int pci_check( void )
{
	union REGS r;

	r.w.ax = PCI_INSTALLATION_CHECK;

	int86( PCI_INTERRUPT, &r, &r );

	if ( r.h.ah == PCI_SUCCESS )
	{
		return r.h.cl;	// last PCI bus in system
	}
	else
	{
		return -1;		// no PCI BIOS detected
	}
}

int pci_find_device( uint16_t venid, uint16_t devid, uint8_t *bus, uint8_t *dev_func )
{
	union REGS r;
	
	r.w.ax = PCI_FIND_DEVICE;
	r.w.dx = venid;
	r.w.cx = devid;
	r.w.si = 0;
	
	int86( PCI_INTERRUPT, &r, &r );
	
	*bus		= r.h.bh;
	*dev_func	= r.h.bl;
	
	return r.w.cflag;
}


int pci_get_iobase( uint8_t bus, uint8_t dev_func, uint16_t *port )
{
	union REGS r;
	
	r.w.ax = PCI_READ_CONFIG_WORD;
	r.h.bh = bus;
	r.h.bl = dev_func;
	r.x.di = PCI_OFFSET_IOBASE;
	
	int86( PCI_INTERRUPT, &r, &r );
	
	if ( r.w.cflag )
	{
		return -1;
	}
	
	// VirtualBox backdoor device address is in I/O port address space
	// 
	*port = r.x.cx & PCI_IO_ADDR_MASK;
		
	return 0;
}
