/*
 * VBSMOUNT
 *  A network redirector for mounting VirtualBox Shared Folders in DOS 
 *  Copyright (C) 2013  Eduardo Casino
 *
 * vboxaux.c: VM functions to be called BEFORE going resident
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
 
/*
 *  I've yet to find a way to do all this initialisation stuff without duplicating code.
 *  Roughly the same functions go into the resident part, but in a different segment
 *  they assume that CS == DS, so calling them from the non-resident part is a lot
 *  of trouble.
 */

#include <i86.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "messages.h"
#include "pci.h"
#include "outpd.h"
#include "vmmdev.h"
#include "shflsvc.h"


/// FIXME: TEST
static uint16_t far fpUnicodeTbl[128] = {
	0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
	0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
	0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
	0x00FF, 0x00D6, 0x00DC, 0x00F8, 0x00A3, 0x00D8, 0x00D7, 0x0192,
	0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
	0x00BF, 0x00AE, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x25C1, 0x00C2, 0x00C0,
	0x00A9, 0x2563, 0x2551, 0x2557, 0x255D, 0x00A2, 0x00A5, 0x2510,
	0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x00E3, 0x00C3,
	0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x00A4,	
	0x00F0, 0x00D0, 0x00CA, 0x00CB, 0x00C8, 0x20AC, 0x00CD, 0x00CE,
	0x00CF, 0x2518, 0x250C, 0x2588, 0x2584, 0x00A6, 0x00CC, 0x2580,
	0x00D3, 0x00DF, 0x00D4, 0x00D2, 0x00F5, 0x00D5, 0x00B5, 0x00FE,
	0x00DE, 0x00DA, 0x00DB, 0x00D9, 0x00FD, 0x00DD, 0x00AF, 0x00B4,
	0x00AD, 0x00B1, 0x2017, 0x00BE, 0x00B6, 0x00A7, 0x00F7, 0x00B8,
	0x00B0, 0x00A8, 0x00B7, 0x00B9, 0x00B3, 0x00B2, 0x25A0, 0x00A0
};

typedef union {
	VMMDevReportGuestInfo		guestInfo;
	VMMDevReportGuestInfo2		guestInfo2;
	VMMDevReqHostVersion		hostVersion;
	VMMDevHGCMConnect			hgcmConnect;
	VMMDevHGCMDisconnect		hgcmDisconnect;
	VBoxSFQueryMappings			queryMappings;
	VBoxSFQueryMapName			queryMapName;
} Request;

struct SREGS s;
Request request;

inline uint8_t lookupCP( uint16_t far *fpUnicodeTbl, uint16_t cp )
{
	uint8_t i;
	
	for ( i = 0; i < 128 && fpUnicodeTbl[i] != cp; ++i );
	
	return ( i < 128 ? (uint8_t) i + 128 : '\0' );

}
int ucs2_to_local( uint16_t *instr, uint16_t len, uint8_t *outstr, uint16_t buflen )
{
	int i, ret = 0;
	uint16_t ucs2;
	uint8_t c;
	
	if ( len == 0 )
	{
		*outstr = '\0';
		return 0;
	}
	if ( ! buflen > len )
	{
		return -1;
	}
	for ( i = 0; i < len; ++i )
	{
		ucs2 = instr[i];
		
		if ( ucs2 < 0x80 )
		{
			*outstr++ = ( uint8_t ) ucs2;
		}
		else
		{
			c = lookupCP( fpUnicodeTbl, ucs2 );
			if ( c == '\0' )
			{
				c = '_';
				ret = 1;
			}
			*outstr++ = c;
		}
	}
	*outstr = '\0';
	
	return ret;
	
}

inline int VBoxAuxSendRequest( uint16_t pciPort, VMMDevRequestHeader *req, int type, size_t size )
{
	uint32_t phys_req;
	
	req->size		= size;
	req->version	= VMMDEV_REQUEST_HEADER_VERSION;
	req->requestType= type;
	req->rc			= VINF_SUCCESS;

	phys_req = MK_LINEAR( s.ds, req );

	__outpd( pciPort, &phys_req );

	return req->rc;
}

static int VBoxAuxSendHGCMRequest( uint16_t pciPort, VMMDevHGCMRequestHeader *req, int type, size_t size )
{
	int ret;
	int wait_ms;

	req->flags	= 0;
	req->result	= VINF_SUCCESS;
	
	ret = VBoxAuxSendRequest( pciPort, &req->header, type, size );
	
	if ( ret != VINF_SUCCESS && ret != VINF_HGCM_ASYNC_EXECUTE )
	{
		return ret;
	}
	
	if ( ret == VINF_HGCM_ASYNC_EXECUTE )
	{
		for ( wait_ms = VMMDEV_ASYNC_TIMEOUT ; wait_ms ; --wait_ms )
		{
			delay(1);
			if ( req->flags & 1)
			{
				break;
			}
		}
		if ( ! wait_ms )
		{
			req->flags = 0;
			(void) VBoxAuxSendRequest( pciPort, &req->header,
					VMMDEV_REQ_HGCMCANCEL, sizeof( VMMDevHGCMRequestHeader ) );
			return VERR_TIMEOUT;
		}
	}
	
	return VINF_SUCCESS;

}

static int VBoxAuxReportGuestInfo2( uint16_t pciPort )
{
	request.guestInfo2.additionsMajor		= VBOX_VERSION_MAJOR;
	request.guestInfo2.additionsMinor		= VBOX_VERSION_MINOR;
	request.guestInfo2.additionsBuild		= VBOX_VERSION_BUILD;
	request.guestInfo2.additionsRevision	= VBOX_VERSION_REV;
	request.guestInfo2.additionsFeatures	= 0;
	strcpy( request.guestInfo2.szName, STRINGFY(VBOX_VERSION_MAJOR) "." STRINGFY(VBOX_VERSION_MINOR) "." STRINGFY(VBOX_VERSION_BUILD)
			"_" VBSMOUNT "-" STRINGFY(VERSION_MAJOR) "." STRINGFY(VERSION_MINOR) );
				
	return VBoxAuxSendRequest( pciPort, &request.guestInfo2.header, VMMDEV_REQ_REPORTGUESTINFO2, sizeof( VMMDevReportGuestInfo2 ) );
}

static int VBoxAuxReportGuestInfo( uint16_t pciPort )
{
	request.guestInfo.interfaceVersion 	= VMMDEV_VERSION;
	request.guestInfo.osType			= VBOX_OSTYPE_DOS;
	
	return VBoxAuxSendRequest( pciPort, &request.guestInfo.header, VMMDEV_REQ_REPORTGUESTINFO, sizeof( VMMDevReportGuestInfo ) );
}

 
static int VBoxAuxVBoxBackdoorInit( uint16_t *pPciPort )
{
	int ret;
	int maxbus;
	uint8_t bus, dev_func;

	maxbus = pci_check();
	
	if ( maxbus < 0 )	// PCI BIOS not detected
	{
		return FAIL;
	}
	
	ret = pci_find_device( VMMDEV_PCI_VENDOR, VMMDEV_PCI_DEVICE, &bus, &dev_func );
	
	if ( ret )	// VirtualBox backdoor not found
	{
		return FAIL;
	}
	
	ret = pci_get_iobase( bus, dev_func, pPciPort );
	
	if ( ret )	// Can't get IRQ and/or port for backdoor device
	{
		return FAIL;
	}

	ret = VBoxAuxReportGuestInfo2( *pPciPort );
	
	if ( ret == VINF_SUCCESS || ret == VERR_NOT_SUPPORTED || ret == VERR_NOT_IMPLEMENTED )
	{
		ret = VBoxAuxReportGuestInfo( *pPciPort );
	}

	if ( ret != VINF_SUCCESS )
	{
		return FAIL;
	}

	return OK;

}

static int VBoxAuxGetVersion( uint16_t pciPort )
{	
	int ret;
	
	ret = VBoxAuxSendRequest( pciPort, &request.hostVersion.header, VMMDEV_REQ_GETHOSTVERSION, sizeof( VMMDevReqHostVersion ) );
	
	if ( ret == VINF_SUCCESS )
	{
		VERB_FPRINTF( VERBOSE, stdout, catgets( cat, 10, 0, MSG_INFO_VBXVERS ),
			request.hostVersion.major, request.hostVersion.minor, request.hostVersion.build, request.hostVersion.revision );
	
		return OK;
	}
	
	return FAIL;
}

PUBLIC int VBoxAuxShflGetMappings( uint16_t pciPort, uint32_t clientId, SHFLMAPPING *pMappings, uint32_t *pNumMappings )
{
	int ret;
	
	request.queryMappings.callInfo.u32ClientID			= clientId;
	request.queryMappings.callInfo.u32Function			= SHFL_FN_QUERY_MAPPINGS;
	request.queryMappings.callInfo.cParms				= SHFL_CPARMS_QUERY_MAPPINGS;

	request.queryMappings.flags.type					= VMMDEV_HGCM_PARMTYPE_32BIT;
	request.queryMappings.flags.u.value32				= SHFL_MF_UCS2;

	request.queryMappings.numberOfMappings.type			= VMMDEV_HGCM_PARMTYPE_32BIT;
	request.queryMappings.numberOfMappings.u.value32	= *pNumMappings;

	request.queryMappings.mappings.type					= VMMDEV_HGCM_PARMTYPE_LINADDR;
	request.queryMappings.mappings.u.Pointer.size		= *pNumMappings * sizeof( SHFLMAPPING );
	request.queryMappings.mappings.u.Pointer.linearAddr	= MK_LINEAR( s.ds, pMappings );

	ret = VBoxAuxSendHGCMRequest( pciPort, &request.queryMappings.callInfo.header, VMMDEV_REQ_HGCMCALL, sizeof( VBoxSFQueryMappings ) );
	
	if ( ret == VINF_SUCCESS )
	{
		ret = request.queryMappings.callInfo.header.result;
		
		if ( ret == VINF_SUCCESS || ret == VINF_BUFFER_OVERFLOW )
		{
			*pNumMappings = request.queryMappings.numberOfMappings.u.value32;
		}
	}
	
	return ret;

}

// NOTE: This function returns error if UTF8 is set!!!!!!!!
//
PUBLIC int VBoxAuxShflGetMapName( uint16_t pciPort, uint32_t clientId, uint32_t root, SHFLSTRING *pShfName, uint32_t ptrSize )
{
	int ret;

	request.queryMapName.callInfo.u32ClientID		= clientId;
	request.queryMapName.callInfo.u32Function		= SHFL_FN_QUERY_MAP_NAME;
	request.queryMapName.callInfo.cParms			= SHFL_CPARMS_QUERY_MAP_NAME;
	
	request.queryMapName.root.type					= VMMDEV_HGCM_PARMTYPE_32BIT;
	request.queryMapName.root.u.value32				= root;

	request.queryMapName.name.type					= VMMDEV_HGCM_PARMTYPE_LINADDR;
	request.queryMapName.name.u.Pointer.size		= ptrSize;
	request.queryMapName.name.u.Pointer.linearAddr	= MK_LINEAR( s.ds, pShfName );
	
	ret = VBoxAuxSendHGCMRequest( pciPort, &request.queryMapName.callInfo.header, VMMDEV_REQ_HGCMCALL, sizeof( VBoxSFQueryMapName ) );
	
	if ( ret ==  VINF_SUCCESS )
	{
		ret = request.queryMapName.callInfo.header.result;
	}
	
	return ret;

}

PUBLIC int VBoxAuxInitVirtual( uint16_t *pPciPort )
{
	int ret;
	
	segread( &s );

	ret = VBoxAuxVBoxBackdoorInit( pPciPort );

	if ( ret )
		return ret;
	
	return VBoxAuxGetVersion( *pPciPort );

}

/*
	open connection for shared folder functions
*/
PUBLIC int VBoxAuxBeginSession( uint16_t pciPort, uint32_t *pClientId )
{
	int ret;
	
	request.hgcmConnect.type = VMMDEV_HGCM_SVCLOC_LOCALHOST_EXISTING;

	(void) strcpy( request.hgcmConnect.host, "VBoxSharedFolders" );
	
	ret = VBoxAuxSendHGCMRequest( pciPort, &request.hgcmConnect.header, VMMDEV_REQ_HGCMCONNECT, sizeof( VMMDevHGCMConnect ) );

	if ( ret == VINF_SUCCESS )
	{
		ret = request.hgcmConnect.header.result;
		
		if ( ret == VINF_SUCCESS )
		{
			*pClientId = request.hgcmConnect.u32ClientID;
			return OK;
		}
	}
	
	return FAIL;
	
}


/*
	release shared folder context
*/
PUBLIC void VBoxAuxEndSession( uint16_t pciPort, uint32_t clientId )
{
	request.hgcmDisconnect.u32ClientID	= clientId;

	(void) VBoxAuxSendHGCMRequest( pciPort, &request.hgcmDisconnect.header, VMMDEV_REQ_HGCMDISCONNECT, sizeof( VMMDevHGCMDisconnect ) );
				
	return;
	
}
