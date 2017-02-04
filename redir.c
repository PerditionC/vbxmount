/*
 * VBSMOUNT
 *  A network redirector for mounting VirtualBox Shared Folders in DOS 
 *  Copyright (C) 2013  Eduardo Casino
 *
 * redir.c: Redirector interface
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

#include <dos.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>

#include "globals.h"
#include "dosdefs.h"
#include "redir.h"

#include "miniclib.h"
#include "ucs2.h"


#pragma data_seg("BEGTEXT", "CODE");
#pragma code_seg("BEGTEXT", "CODE");

#define SECTOR_SIZE	32768

typedef void (*redirFunction)(void);


// Stack management
//
#define STACK_SIZE 300
static uint8_t newStack[STACK_SIZE] = {0};
static uint16_t far *fpStackParam;	// Far pointer to top os stack at entry
static uint16_t dosSS;
static uint16_t dosSP;
static uint16_t curSP;


void (__interrupt __far *fpPrevInt2fHandler)();
static union INTPACK far *r;
static redirFunction currFunction;
static __segment dosDS;
__segment myDS;

VolInfo volInfo[MAX_MOUNTS];
uint8_t numMounts = 0;

SDA			far *fpSDA;
SDB			far *fpSDB;
FDB			far *fpFDB;
char		far *fpFcbName1;
char		far *fpFcbName2;
char		far *fpFileName1;
char		far *fpFileName2;
char		far *fpCurrentPath;

static char fcbName[12];

static uint8_t driveIdx;

 // While executing the resident code, we no longer have access to the
 // C library functions. This pragma replicates _chain_intr()
 // ( basically copied from clib )
 //
void _chain_intr_local( void (__interrupt __far *)() );
#pragma aux _chain_intr_local = \
	"mov	cx, ax"		/* get offset and segment of previous int */	\
	"mov	ax, dx"														\
	"mov	sp, bp"														\
	"xchg	cx, 20[bp]"	/* restore cx, & put in offset */				\
	"xchg	ax, 22[bp]"	/* restore ax, & put in segment */				\
	"mov	bx, 28[bp]"	/* restore flags... */							\
	"and	bx, 0xfcff"	/* ...but not Interrupt and Trace Flags */		\
	"push	bx"															\
	"popf"																\
	"pop	gs"			/* restore segment registers */					\
	"pop	fs"															\
	"pop	es"															\
	"pop	ds"															\
	"pop	di"															\
	"pop	si"															\
	"pop	bp"															\
	"pop	bx"			/* skip SP */									\
	"pop	bx"			/* restore general purpose registers */			\
	"pop	dx"															\
	"retf"				/* return to previous interrupt handler */		\
	parm [dx ax]														\
	modify [];


static void SetSftOwner( void );
#pragma aux SetSftOwner = \
	/* Set DOS DS and stack */									\
	"mov curSP, sp"		/* Save current SP				*/		\
	"mov ss, dosSS"												\
	"mov sp, dosSP"												\
	"push ds"													\
	"mov ds, dosDS"												\
																\
	/* Call int2F120C "OPEN DEVICE AND SET SFT OWNER/MODE"	*/	\
	/*   needs DS = DOS DS and a DOS internal stack			*/	\
	/*   seems that it destroys ES and DI					*/	\
	"mov ax, 0x120C"											\
	"int 0x2F"													\
																\
	/* Restore our DS and internal stack */						\
	"pop bx"			/* Restore saved DS				*/		\
	"mov ds, bx"												\
	"mov ss, bx"		/* Restore saved SS, same as DS	*/		\
	"mov sp, curSP"		/* Restore saved SP				*/		\
	modify [ax bx es di];


static uint32_t GetDosTime( void );
#pragma aux GetDosTime = \
	/* Set DOS DS and stack */									\
	"mov curSP, sp"		/* Save current SP				*/		\
	"mov ss, dosSS"												\
	"mov sp, dosSP"												\
	"push ds"													\
	"mov ds, dosDS"												\
																\
	/* Call int2F120D "GET DATE AND TIME"			*/			\
	/*   needs DS = DOS DS and a DOS internal stack */			\
	"mov ax, 0x120D"											\
	"int 0x2F"													\
	"xchg ax, dx"												\
																\
	/* Restore our DS and internal stack */						\
	"pop bx"			/* Restore saved DS				*/		\
	"mov ds, bx"												\
	"mov ss, bx"		/* Restore saved SS, same as DS	*/		\
	"mov sp, curSP"		/* Restore saved SP				*/		\
	value [ax dx]												\
	modify [bx];

	
static inline void Success( void )
{
	r->w.flags &= ~INTR_CF;
	r->w.ax = 0;

	return;
}

static inline void Failure( uint16_t error )
{
	r->w.flags |= INTR_CF;
	r->w.ax = error;

	return;
}

inline void FillFcbName( char far *fcbName, char far *fileName )
{
	int i;
	char far *p= _fstrrchr_local( fileName, '\\' ) + 1;
	
	for ( i = 0; *p; ++p )
	{
		if ( *p == '.' )
		{
			while ( i < 8 )
			{
				fcbName[i++] = ' ';
			}
		}
		else
		{
			fcbName[i++] = *p;
		}
	}
	while ( i < 11 )
	{
		fcbName[i++] = ' ';
	}
		
	return;
}

static void InstallationCheck( void )
{
	r->w.ax = (uint16_t) 0x00FF;		// Installed
	
	return;
}

static void RmDir( void )
{

	
	return;
}

static void MkDir( void )
{

	
	return;
}

static void ChDir( void )
{

	return;
}

static void CloseFile( void )
{

	return;
}

static void CommitFile( void )
{
	// Just succeed

	return;
}


static void ReadFile( void )
{
	
	return;
}

static void WriteFile( void )
{

	return;
}

static void LockFile( void )
{
	return;
}

static void UnlockFile( void )
{
	return;
}

static void GetDiskSpace( void )
{

	return;
}

static void GetFileAttrib( void )
{
	
	return;
}

static void SetFileAttrib( void )
{
	
	return;
}

static void OpenOrCreateFile( uint16_t extAction, uint16_t extMode, uint16_t extAttr )
{

	return;
}

static void OpenFile( void )
{
	OpenOrCreateFile( EXTENDED_OPEN, fpSDA->openMode & 3,
			_A_NORMAL | _A_ARCH | _A_RDONLY | _A_HIDDEN | _A_SYSTEM  );

	return;
}

static void CreateFile( void )
{
	OpenOrCreateFile( EXTENDED_CREATE | EXTENDED_REPLACE,
			MODE_READWRITE,	(uint8_t) *fpStackParam );

	return;
}

inline int MatchAttrMask( uint8_t fatAttr, uint8_t mask )
{
	// _A_RDONLY and _A_ARCH should always match
	//
	return	! ( 	((fatAttr & _A_HIDDEN) && ( ! (mask & _A_HIDDEN)))
				||	((fatAttr & _A_SYSTEM) && ( ! (mask & _A_SYSTEM)))
				||	((fatAttr & _A_VOLID ) && ( ! (mask & _A_VOLID )))
				||	((fatAttr & _A_SUBDIR) && ( ! (mask & _A_SUBDIR)))
			);
}	

inline int MatchNameMask( char *fname, char far *mask )
{
	int i;
	
	for ( i = 0; i < 11; ++i )
	{
		if ( (fname[i] != mask[i]) && (mask[i] != '?') )
		{
			return 0;
		}
	}
	return 1;
}

inline int FcbNameHasWildCards( char far *fcbName )
{
	int i;
	
	for ( i = 0; i < 11; ++i )
	{
		if ( fcbName[i] == '?' )
		{
			return 1;
		}
	}
	return 0;
}

static void FindNext( void )
{

	return;
}

static void FindFirst( void )
{
	// Special case: Get Volume ID
	//
	if ( fpSDA->attrMask == _A_VOLID )
	{

		fpSDB->driveNumber = volInfo[driveIdx].driveNum | 0x80;
		fpSDB->dirEntryNum = 0;
		_fmemcpy_local( fpSDB->searchMask, fpFcbName1, 11 );
		
		fpFDB->fileAttr = _A_VOLID;
		fpFDB->fileTime = GetDosTime();
		fpFDB->fileSize = 0;
		cp_ucs2_to_local( fpFDB->fileName, MK_FP( myDS, volInfo[driveIdx].ucs2label ), 11 );

		return;
	}
	
	return;
}

static void MakeFullPath( char far *fullPath, char far *dirName, char far *fcbName )
{
	int i;
	
	while ( *dirName )
	{
		*(fullPath++) = *(dirName++);
	}
	*(fullPath++) = '\\';
	
	for ( i= 0; i < 11; ++i )
	{
		if ( fcbName[i] != ' ' )
		{
			if ( i == 8 )
			{
				*(fullPath++) = '.';
			}
			*(fullPath++) = fcbName[i];
		}
	}
	*fullPath = '\0';

	return;
}

static void DeleteFile( void )
{


	return;
}


static void RenameFile( void )
{


	return;
}

static void SpecialOpen( void )
{
	OpenOrCreateFile( fpSDA->extAction, fpSDA->extMode & 0x7f, fpSDA->extAttr );
	
	return;
}


static int IsCallForUs( uint8_t function )
{

	SFT far *fpSFT = (SFT far *)MK_FP( r->w.es, r->w.di );

	if ( function == 0x00 )
	{
		return 1;
	}
	
	if ( function == 0x21 || (function >= 0x06 && function <= 0x0B) )
	{
		for ( driveIdx = 0; driveIdx < numMounts; ++driveIdx )
		{
			if ( (fpSFT->devInfoWord & 0x3F) == volInfo[driveIdx].driveNum )
			{
				return 1;
			}
		}
		return 0;
	}
	
	if ( function == 0x1C )		// FindNext
	{
		for ( driveIdx = 0; driveIdx < numMounts; ++driveIdx )
		{
	
			if ( (fpSDB->driveNumber & 0x40) &&
					((fpSDB->driveNumber & 0x1F) == volInfo[driveIdx].driveNum ) )
			{
				return 1;
			}
		}
		return 0;
	}
	
	// This is not mentioned in "Undocumented DOS". GetDiskSpace can be called for
	// a drive other than current and CDS is passed in es:di
	//
	if ( function == 0x0C )	// GetDiskSpace
	{
		CDS far *fpCDS = (CDS far *)MK_FP( r->w.es, r->w.di );
		
		if ( fpCDS->u.Net.parameter == VBSMOUNT_MAGIC )
		{
			return 1;
		}
	}

	if ( fpSDA->currentCDS->u.Net.parameter == VBSMOUNT_MAGIC )
	{
		FillFcbName( fpFcbName1, fpFileName1 );
		return 1;
	}
	else
	{
		return 0;
	}
}

static redirFunction dispatchTable[] = {
	InstallationCheck,	/* 0x00 */
	RmDir,				/* 0x01 */
	NULL,				/* 0x02 */
	MkDir,				/* 0x03 */
	NULL,				/* 0x04 */
	ChDir,				/* 0x05 */
	CloseFile,			/* 0x06 */
	CommitFile,			/* 0x07		Does nothing */
	ReadFile,			/* 0x08 */
	WriteFile,			/* 0x09 */
	LockFile,			/* 0x0A		Does nothing */
	UnlockFile,			/* 0x0B		Does nothing */
	GetDiskSpace,		/* 0x0C */
	NULL,				/* 0x0D */
	SetFileAttrib,		/* 0x0E */
	GetFileAttrib,		/* 0x0F */
	NULL,				/* 0x10 */
	RenameFile,			/* 0x11 */
	NULL,				/* 0x12 */
	DeleteFile,			/* 0x13 */
	NULL,				/* 0x14 */
	NULL,				/* 0x15 */
	OpenFile,			/* 0x16 */
	CreateFile,			/* 0x17 */
	NULL,				/* 0x18 */
	NULL,				/* 0x19 */
	NULL,				/* 0x1A */
	FindFirst,			/* 0x1B */
	FindNext,			/* 0x1C */
	NULL,				/* 0x1D */
	NULL,				/* 0x1E */
	NULL,				/* 0x1F */
	NULL,				/* 0x20 */
	NULL,				/* 0x21		SeekFromdEnd() Unsupported */
	NULL,				/* 0x22 */
	NULL,				/* 0x23 */
	NULL,				/* 0x24 */
	NULL,				/* 0x25 */
	NULL,				/* 0x26 */
	NULL,				/* 0x27 */
	NULL,				/* 0x28 */
	NULL,				/* 0x29 */
	NULL,				/* 0x2A */
	NULL,				/* 0x2B */
	NULL,				/* 0x2C */
	NULL,				/* 0x2D */
	SpecialOpen			/* 0x2E */
};

#define MAX_FUNCTION	0x2E

void __interrupt Int2fRedirector( union INTPACK regset )
{
	static uint16_t dosBP;

	_asm
	{
		sti
		push cs
		pop ds
	}
	
	if ( regset.h.ah != 0x11 || regset.h.al > MAX_FUNCTION )
	{
		goto chain;
	}

	r = &regset;
		
	currFunction = dispatchTable[regset.h.al];

	if ( NULL == currFunction || !IsCallForUs( regset.h.al ) )
	{
		goto chain;
	}
	
	
	dosDS = r->w.ds;	// We'll use this in some inline assembly functions because
						// OpenWatcom does not support struct references in ASM
	
	// Save ss:sp and switch to our internal stack.
	// Save bp so we can get at parameters at the top of the stack.
	// Also get current DS
	//
	_asm
	{
		mov dosSS, ss
		mov dosSP, sp
		mov dosBP, bp
		mov ax, ds
		mov myDS, ax
		mov ss, ax
		mov sp, (offset newStack) + STACK_SIZE - 2
	};
		
	fpStackParam = (uint16_t far *)MK_FP( dosSS, dosBP + sizeof( union INTPACK ) ); 

	Success();
	
	currFunction();

	_asm
	{
		mov ss, dosSS
		mov sp, dosSP
	};

	return;

chain:	
	_chain_intr_local( fpPrevInt2fHandler );
}

/**
 * This function must the the last one in the BEGTEXT segment!
 *
 * BeginOfTransientBlock() must locate after all other resident functions and global variables.
 * This function must be defined in the last source file if there are more than
 * one source file containing resident functions.
 */
uint16_t BeginOfTransientBlockNoLfn( void )
{
	return (uint16_t) BeginOfTransientBlockNoLfn;	// Force some code
}
