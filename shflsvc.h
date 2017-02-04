#ifndef _SHFLSVC_H_
#define _SHFLSVC_H_
#pragma once
/*
 * VBSMOUNT
 *  A network redirector for mounting VirtualBox Shared Folders in DOS 
 *  Copyright (C) 2013  Eduardo Casino
 *
 * shflsvc.h: Shared Folders service interface
 * Copyright (C) 2006-2011 Oracle Corporation
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
#include "vmmdev.h"

#define SHFL_FN_QUERY_MAPPINGS      1
#define SHFL_FN_QUERY_MAP_NAME      2
#define SHFL_FN_CREATE              3
#define SHFL_FN_CLOSE               4
#define SHFL_FN_READ                5
#define SHFL_FN_WRITE               6
#define SHFL_FN_LOCK                7
#define SHFL_FN_LIST                8
#define SHFL_FN_INFORMATION         9
#define SHFL_FN_REMOVE              11
#define SHFL_FN_MAP_FOLDER_OLD      12
#define SHFL_FN_UNMAP_FOLDER        13
#define SHFL_FN_RENAME              14
#define SHFL_FN_FLUSH               15
#define SHFL_FN_SET_UTF8            16
#define SHFL_FN_MAP_FOLDER          17
#define SHFL_FN_READLINK            18
#define SHFL_FN_SYMLINK             19
#define SHFL_FN_SET_SYMLINKS        20

#define SHFL_MAX_LEN		256
#define SHFL_MAX_MAPPINGS	64

#pragma pack(1)

typedef struct _SHFLSTRING
{
	uint16_t u16size;
	uint16_t u16length;
	uint16_t ucs2[1];
} SHFLSTRING;

typedef struct _SHFLMAPPING
{
	uint32_t u32status;
	uint32_t root;
} SHFLMAPPING;


#define SHFL_MF_UCS2       0x00000000ui32
typedef struct _VBoxSFQueryMappings
{
	VMMDevHGCMCall	callInfo;

	HGCMFunctionParameter32Bit		flags;
	HGCMFunctionParameter32Bit		numberOfMappings;
	HGCMFunctionParameterLinaddr	mappings;		// Points to array of SHFLMAPPING structures
	
} VBoxSFQueryMappings;

#define SHFL_CPARMS_QUERY_MAPPINGS	3

typedef struct _VBoxSFQueryMapName
{
	VMMDevHGCMCall	callInfo;
	
	HGCMFunctionParameter32Bit		root;
	HGCMFunctionParameterLinaddr	name;			// Points to SHFLSTRING buffer

} VBoxSFQueryMapName;

#define SHFL_CPARMS_QUERY_MAP_NAME	2

typedef struct _VBoxSFMapFolder
{
	VMMDevHGCMCall	callInfo;

	HGCMFunctionParameterLinaddr	path;			// Points to SHFLSTRING buffer
	HGCMFunctionParameter32Bit		root;
	HGCMFunctionParameter32Bit		delimiter;
	HGCMFunctionParameter32Bit		fCaseSensitive;
	
} VBoxSFMapFolder;

#define SHFL_CPARMS_MAP_FOLDER	4

#endif	/* _SHFLSVC_H_ */
