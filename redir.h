#ifndef REDIR_H_
#define REDIR_H_
#pragma once
/*
 * VBSMOUNT
 *  A network redirector for mounting VirtualBox Shared Folders in DOS 
 *  Copyright (C) 2013  Eduardo Casino
 *
 * redir.h: Redirector interface
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
 *
 */

#include <dos.h>
#include <stdint.h>
#include "dosdefs.h"

#define VBSMOUNT_MAGIC 'SF'
#define MAX_MOUNTS	16

#pragma pack(1)
typedef struct
{
	uint8_t driveNum;
	uint32_t root;
	uint16_t ucs2label[11];	// blank padded, UCS2, first 11 chars of share name
} VolInfo;

typedef struct
{
	char		signature[9];			// "VBSMOUNT"
	uint16_t	psp;					// Our PSP
	void far	*ourHandler;			// Our handler (points to Int2fRedirector() )
	void far	*previousHandler;		// Handler we chain to and must be restored when uninstalled
	uint16_t	pciPort;
	uint32_t	clientId;
	VolInfo		*pVolInfo;
} Signature;


extern void (__interrupt __far *fpPrevInt2fHandler)();

extern uint8_t		numMounts;
extern VolInfo		volInfo[MAX_MOUNTS];
extern SDA			far *fpSDA;
extern SDB			far *fpSDB;
extern FDB			far *fpFDB;
extern char			far *fpFcbName1;
extern char			far *fpFcbName2;
extern char			far *fpFileName1;
extern char			far *fpFileName2;
extern char			far *fpCurrentPath;

extern __segment myDS;

extern void __interrupt far Int2fRedirector( union INTPACK );
extern uint16_t BeginOfTransientBlockNoLfn( void );
	
#endif /* REDIR_H_ */
