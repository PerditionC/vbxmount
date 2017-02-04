#ifndef _VMMDEV_H_
#define _VMMDEV_H_
#pragma once
/*
 * VBSMOUNT
 *  A network redirector for mounting VirtualBox Shared Folders in DOS 
 *  Copyright (C) 2013  Eduardo Casino
 *
 * vmmdev.h: Virtual Device for Guest <-> VMM/Host communication
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
#include <i86.h>
#include <stdint.h>
#include "pci.h"

#define VMMDEV_PCI_VENDOR 0x80ee
#define VMMDEV_PCI_DEVICE 0xcafe

#define VMMDEV_REQ_GETHOSTVERSION		4
#define VMMDEV_REQ_REPORTGUESTINFO		50
#define VMMDEV_REQ_REPORTGUESTINFO2		58
#define VMMDEV_REQ_HGCMCONNECT			60
#define VMMDEV_REQ_HGCMDISCONNECT		61
#define VMMDEV_REQ_HGCMCALL				62
#define VMMDEV_REQ_HGCMCANCEL			64

#define VINF_SUCCESS					0
#define VERR_GENERAL_FAILURE			(-1)
#define VERR_NOT_IMPLEMENTED			(-12)
#define VERR_NOT_SUPPORTED				(-37)
#define VERR_TIMEOUT					(-40)
#define VINF_BUFFER_OVERFLOW			41
#define VERR_HGCM_SERVICE_NOT_FOUND		(-2900)
#define VINF_HGCM_CLIENT_REJECTED		2901
#define VERR_HGCM_INVALID_CMD_ADDRESS	(-2902)
#define VINF_HGCM_ASYNC_EXECUTE			2903
#define VERR_HGCM_INTERNAL				(-2904)
#define VERR_HGCM_INVALID_CLIENT_ID		(-2905)

#define VMMDEV_ASYNC_TIMEOUT			500

#define VBOX_OSTYPE_DOS					0x00010000ui32
#define VMMDEV_VERSION					0x00010004ui32
#define VMMDEV_VERSION_MAJOR			(VMMDEV_VERSION >> 16)
#define VMMDEV_VERSION_MINOR			(VMMDEV_VERSION & 0xffff)

#define VMMDEV_REQUEST_HEADER_VERSION	0x00010001ui32

#define VBOX_VERSION_MAJOR			4
#define VBOX_VERSION_MINOR			0
#define VBOX_VERSION_BUILD			0
#define VBOX_VERSION_REV			0

#define VMMDEV_HGCM_PARMTYPE_32BIT		1
#define VMMDEV_HGCM_PARMTYPE_64BIT		2
#define VMMDEV_HGCM_PARMTYPE_LINADDR	4

#pragma pack(1)

typedef struct
{
	uint32_t type;
	union
	{
		uint32_t   value32;
		uint64_t   value64;
		struct
		{
			uint32_t size;
			uint32_t linearAddr;
		} Pointer;
	} u;
} HGCMFunctionParameter;

typedef HGCMFunctionParameter	HGCMFunctionParameter32Bit;
typedef HGCMFunctionParameter	HGCMFunctionParameter64Bit;
typedef HGCMFunctionParameter	HGCMFunctionParameterLinaddr;

typedef struct {
	uint32_t size;
	uint32_t version;
	uint32_t requestType;
	int32_t rc;
	uint32_t reserved1;
	uint32_t reserved2;
} VMMDevRequestHeader;

typedef struct {
	VMMDevRequestHeader header;
	uint16_t major;
	uint16_t minor;
	uint32_t build;
	uint32_t revision;
	uint32_t features;
} VMMDevReqHostVersion;

typedef struct {
	VMMDevRequestHeader header;
	uint32_t interfaceVersion;
	uint32_t osType;
} VMMDevReportGuestInfo;

typedef struct {
	VMMDevRequestHeader header;
	uint16_t additionsMajor;
	uint16_t additionsMinor;
	uint32_t additionsBuild;
	uint32_t additionsRevision;
	uint32_t additionsFeatures;
	char szName[128];
} VMMDevReportGuestInfo2;

#define VBOX_HGCM_REQ_DONE    (1 << 0)
 
typedef struct {
	VMMDevRequestHeader header;
	uint32_t flags;
	int32_t result;
} VMMDevHGCMRequestHeader;

typedef struct {
	VMMDevHGCMRequestHeader header;
	uint32_t u32ClientID;
	uint32_t u32Function;
	uint32_t cParms;
} VMMDevHGCMCall;

#define VMMDEV_HGCM_SVCLOC_LOCALHOST_EXISTING 2

#define VMMDEV_HGCM_NAME_SIZE 128

typedef struct {
	VMMDevHGCMRequestHeader header;
	uint32_t type;
	char host[VMMDEV_HGCM_NAME_SIZE];
	uint32_t u32ClientID;
} VMMDevHGCMConnect;

typedef struct {
	VMMDevHGCMRequestHeader header;
	uint32_t u32ClientID;
} VMMDevHGCMDisconnect;


#define VMMDEV_MIN_BLOCK_SIZE		2048		// Really?
#define VMMDEV_DEF_BLOCK_SIZE		4096
#define VMMDEV_MAX_BLOCK_SIZE		65535		// Maximum data chunk size

#endif	/* _VMMDEV_H_ */
