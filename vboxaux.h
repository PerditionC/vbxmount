#ifndef _VBOXAUX_H_
#define _VBOXAUX_H_
#pragma once
/*
 * VBSMOUNT
 *  A network redirector for mounting VirtualBox Shared Folders in DOS 
 *  Copyright (C) 2013  Eduardo Casino
 *
 * vboxaux.h: VM functions to be called BEFORE going resident
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
#include "shflsvc.h"

extern int VBoxAuxInitVirtual( uint16_t *pPciPort );
extern int VBoxAuxShflGetMappings( uint16_t pciPort, uint32_t clientId, SHFLMAPPING *pMappings, uint32_t *pNumMappings );
extern int VBoxAuxShflGetMapName( uint16_t pciPort, uint32_t clientId, uint32_t root, SHFLSTRING *pShfName, uint32_t ptrSize );
extern int VBoxAuxBeginSession( uint16_t pciPort, uint32_t *pClientId );
extern void VBoxAuxEndSession( uint16_t pciPort, uint32_t clientId );

#endif /* _VBOXAUX_H_ */
