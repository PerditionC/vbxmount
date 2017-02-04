/*
 * VBSMOUNT
 *  A network redirector for mounting VirtualBox Shared Folders in DOS 
 *  Copyright (C) 2013  Eduardo Casino
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
 * Acknowledgements:
 *    "Undocumented DOS 2nd ed." by Andrew Schulman et al.
 *    Ken Kato's VMBack info and Command Line Tools
 *                    (http://sites.google.com/site/chitchatvmback/)
 *    VMware's Open Virtual Machine Tools
 *                    (http://open-vm-tools.sourceforge.net/)
 *    Tom Tilli's <aitotat@gmail.com> TSR example in Watcom C for the 
 *                    Vintage Computer Forum (www.vintage-computer.com)
 *
 */ 
#include <i86.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <stdio.h>
#include <ctype.h>

#include "globals.h"
#include "dosdefs.h"
#include "kitten.h"
#include "messages.h"
#include "vmmdev.h"
#include "shflsvc.h"
#include "vboxaux.h"
#include "vboxshfl.h"
#include "redir.h"

PUBLIC nl_catd cat;
PUBLIC Verbosity verbosity = NORMAL;
static int uninstall = 0;

static struct SREGS segRegs;
static char *myName = VBSMOUNT;

static SysVars far *fpSysVars;
static CDS far *fpCurrDir;

static uint8_t testOnly = 0;

static uint8_t far *fpNumMounts;
static VolInfo far *fpVolInfo;
static uint16_t	far *fpPort;
static uint32_t	far *fpClientId;
static void		(__interrupt __far * far * fpfpPrevInt2fHandler)();

// CPU identification routine
// (Info from http://www.ukcpu.net/Programming/Hardware/x86/CPUID/x86-ID.asp)
//
// in 808x and 8018x, flags 12-15 are always set.
// in 286, flags 12-15 are always clear in real mode.
// in 32-bit processors, bit 15 is always clear in real mode.
//                       bits 12-14 have the last value loaded into them.
//
static uint8_t RunningIn386OrHigher( void );
#pragma aux RunningIn386OrHigher =									\
	"pushf"				/* Save current flags */					\
	"xor ax, ax"													\
	"push ax"														\
	"popf"				/* Load all flags cleared */				\
	"pushf"															\
	"pop ax"			/* Load flags back to ax */					\
	"and ax, 0xf000"	/* If 86/186, flags 12-15 will be set */	\
	"cmp ax, 0xf000"												\
	"je return"														\
	"mov ax, 0xf000"												\
	"push ax"														\
	"popf"				/* Load flags 12-15 set */					\
	"pushf"															\
	"pop ax"			/* Load flags back to ax */					\
	"and ax, 0xf000"	/* If 286, flags 12-15 will be cleared */	\
	"jz return"														\
	"mov al, 0x01"													\
	"return:"														\
	"popf"				/* Restore flags */							\
	value [al];

	
//	00h not installed, OK to install
//	01h not installed, not OK to install 
//	FFh some redirector is installed
//
static uint8_t InstallationCheck( void );
#pragma aux InstallationCheck =							\
	"mov ax, 1100h"	/* Installation check */			\
	"int 2fh"											\
	value [al];

	
static void PrintUsageAndExit( int err )
{
	VERB_FPRINTF( NORMAL, stderr, MSG_MY_NAME, myName, VERSION_MAJOR, VERSION_MINOR );
	VERB_FPRINTF( NORMAL, stderr, MSG_COPYRIGHT );
	VERB_FPUTS( QUIET, catgets( cat, 0, 1, MSG_HELP_1 ), stderr );
	VERB_FPRINTF( QUIET, stderr, catgets( cat, 0, 2, MSG_HELP_2 ), myName );
	VERB_FPRINTF( QUIET, stderr, catgets( cat, 0, 3, MSG_HELP_3 ), myName );
	VERB_FPUTS( QUIET, catgets( cat, 0, 4, MSG_HELP_4 ), stderr );
	VERB_FPUTS( QUIET, catgets( cat, 0, 5, MSG_HELP_5 ), stderr );
	VERB_FPUTS( QUIET, catgets( cat, 0, 6, MSG_HELP_6 ), stderr );
	VERB_FPUTS( QUIET, catgets( cat, 0, 7, MSG_HELP_7 ), stderr );
	VERB_FPUTS( QUIET, catgets( cat, 0, 8, MSG_HELP_8 ), stderr );
	VERB_FPUTS( QUIET, catgets( cat, 0, 9, MSG_HELP_9 ), stderr );
	VERB_FPUTS( QUIET, catgets( cat, 0,19, MSG_HELP_19), stderr );
	exit( err );
	
}

static void GetFarPointersToResidentData( void )
{

	// From redir.c
	fpfpPrevInt2fHandler	= segRegs.cs:>&fpPrevInt2fHandler;
	fpNumMounts = segRegs.cs:>&numMounts;
	fpVolInfo = segRegs.cs:>&volInfo;
	// From vbxcall.c
	fpPort			= segRegs.cs:>&port;
	// From vbxshf.c
	fpClientId		= segRegs.cs:>&clientId;
	
	return;
	
}

static int GetSysVars(void)
{
	union REGS r;
	struct SREGS s;

	r.h.ah = 0x52;
	segread( &s );
	s.es = r.x.bx = 0;
	
	intdosx(&r, &r, &s);
	
	if ( !s.es && !r.x.bx )
		return 1;

	fpSysVars = (SysVars far *) MK_FP( s.es, r.x.bx - SYSVARS_DECR );
	
	return 0;

}

static int AlreadyInstalled( uint16_t *pPciPort, uint32_t *pClientId )
{
	int driveNum;
	
	for ( driveNum = 4; driveNum < fpSysVars->lastDrive; ++driveNum )
	{
		fpCurrDir = &fpSysVars->currDir[driveNum];
		if ( fpCurrDir->u.Net.parameter == VBSMOUNT_MAGIC )
		{
			break;
		}
	}
	
	// I'm going to be a bit paranoid here
	//
	if ( driveNum < fpSysVars->lastDrive )
	{
		if ( (uint32_t) fpCurrDir->u.Net.redirIFSRecordPtr != 0ui32
				&& (uint32_t) fpCurrDir->u.Net.redirIFSRecordPtr != 0xffffffffui32 )
		{
			if ( ! _fstrncmp( ((Signature far *) fpCurrDir->u.Net.redirIFSRecordPtr)->signature, segRegs.ds:>myName, 9 ) )
			{
				*pPciPort	= ((Signature far *) fpCurrDir->u.Net.redirIFSRecordPtr)->pciPort;
				*pClientId	= ((Signature far *) fpCurrDir->u.Net.redirIFSRecordPtr)->clientId;
				
				return 1;
			}
		}
	}

	return 0;
}

static uint16_t savedSS, savedSP;

// As we made the return address of the TSR PSP to point
// to TerminateTsr, execution of UninstallDriver() continues here
// 
#pragma aux TerminateTsr aborts;
void __declspec( naked ) TerminateTsr( void )
{
	_asm {
	
		// restore stack
		//
		mov		ax, seg savedSS
		mov		ds, ax
		mov		ss, savedSS
		mov		sp, savedSP

		// and registers
		//
		pop		di
		pop		es
		pop		ds
		
		// DOS 2+ internal - SET CURRENT PROCESS ID (SET PSP ADDRESS)
		// AH = 50h
		// BX = segment of PSP for new process
		//	
		mov		bx, _psp				// restore our PSP
		mov		ah, 0x50
		int		0x21
		
		// DOS 2+ - EXIT - TERMINATE WITH RETURN CODE
		// AH = 4Ch
		// AL = return code
		//
		mov		al, ERR_SUCCESS			// terminate successfully
		mov		ah, 0x4c
		int		0x21
	}

}

// This call is made after AlreadyInstalled(), so currDir points to the
// first drive CDS manged by the driver
// WARNING: fpfpPrevInt2fHandler needs to be set!!
//
// This function only returns on error. If successful, execution continues
// with TerminateTsr()
//
static void UninstallDriver( void )
{
	Signature far *sig = (Signature far *) fpCurrDir->u.Net.redirIFSRecordPtr;
	uint16_t psp = sig->psp;
		
	if ( sig->ourHandler != *fpfpPrevInt2fHandler )
	{
		// Can't uninstall, another handler was installed
		return;
	}
	
	_dos_setvect( 0x2f, sig->previousHandler );
	fpCurrDir->flags = fpCurrDir->flags & ~(NETWORK|PHYSICAL);			// Invalidate drive
	fpCurrDir->u.Net.parameter = 0x00;
	fpCurrDir->u.Net.redirIFSRecordPtr = (void far *)0xffffffffui32;

	VBoxAuxEndSession( sig->clientId, sig->pciPort );
	
	VERB_FPUTS( QUIET, catgets( cat, 2, 1, MSG_INFO_UNINSTALL ), stderr );

	// Switch PSP and Int 4Ch method of unloading the TSR
	// (Undocumented DOS, Second Edition)
	//
	_asm {
		// save registers
		//
		push	ds
		push	es
		push	di

		// set TSR PSP return address to TerminateTsr
		//
		mov		es, psp
		mov		di, 0x0a		// Position of terminate address of previous program
		mov		ax, offset TerminateTsr
		stosw
		mov		ax, cs
		stosw

		// DOS 2+ internal - SET CURRENT PROCESS ID (SET PSP ADDRESS)
		// AH = 50h
		// BX = segment of PSP for new process
		//
		mov		bx, es			// set current PSP to TSR's
		mov		ah, 0x50
		int		0x21
		
		// save stack frame
		//
		mov		ax, seg savedSS
		mov		ds, ax
		mov		savedSS, ss
		mov		savedSP, sp
		
		// DOS 2+ - EXIT - TERMINATE WITH RETURN CODE
		// AH = 4Ch
		// AL = return code
		//
		mov		ax, 0x4c00		// successfully terminate TSR
		int		0x21
		
		/* execution continues in TerminateTsr() */
	}
	
}

static int GetOptions( char *argString )
{
	int argIndex, i;
	char c;
	int vset = 0;
	
	for ( argIndex = 0; argString[argIndex] != '\0'; ++argIndex )
	{
		switch ( argString[argIndex] )
		{
			case ' ':
			case '\t':
				break;	// Skip spaces
			
			case '/':
			case '-':
				switch ( c = toupper( argString[++argIndex] ) )
				{
					case 'H':
					case '?':
						return 1;
						break;
					case 'Q':
						if ( vset++ )
						{
							return 2;	// /Q, /QQ or /V already set
						}
						if ( toupper( argString[argIndex+1] ) == 'Q' )
						{
							verbosity = SILENT;
							++argIndex;
						}
						else
						{
							verbosity = QUIET;
						}
						break;
					case 'V':
						if ( vset++ )
						{
							return 2;	// /Q or /V already set
						}
						verbosity = VERBOSE;
						break;
					case 'U':
						++uninstall;
						break;
					case 'T':
						++testOnly;
						break;
					default:
						return 2; // Invalid option
				}
				break;
	
			default:
				return 2; // Non switch
		}
	}
	
	return 0;
}

int main( int argc, char **argv )
{

	char	argString[128];
	int		tblSize;
	uint16_t pciPort;
	uint32_t clientId;
	uint32_t mapping, numMappings = SHFL_MAX_MAPPINGS;
	SHFLMAPPING mappings[SHFL_MAX_MAPPINGS];
	uint8_t shfNameBuf[sizeof( SHFLSTRING ) + SHFL_MAX_LEN];
	SHFLSTRING *pShfName = (SHFLSTRING *) shfNameBuf;
	int installed;
	int		ret;
	
	segread( &segRegs );
	
	cat = catopen( myName, 0 );
	
	GetFarPointersToResidentData();
			
	ret = GetOptions( getcmd( argString ) );
	
	if ( ret == 1 )							// User requested help
	{
		verbosity = NORMAL;					// Ignore verbosity level
		PrintUsageAndExit( ERR_SUCCESS );
	}
	if ( ret == 2 )							// Invalid option
	{
		PrintUsageAndExit( ERR_BADOPTS );
	}

	VERB_FPRINTF( QUIET, stderr, MSG_MY_NAME, myName, VERSION_MAJOR, VERSION_MINOR );

	VERB_FPRINTF( NORMAL, stdout, MSG_COPYRIGHT );
	
	// Check that CPU is x386 or higher (to avoid nasty things if somebody
	// tries to run this in a REAL machine)
	//
	if ( ! RunningIn386OrHigher() )
	{
		VERB_FPUTS( QUIET, catgets(cat, 1, 3, MSG_ERROR_NOVIRT ), stderr );
		return( ERR_NOVIRT );
	}

	// Check OS version. Only DOS >= 5.0 is supported
	//
	if ( _osmajor < 5 )
	{
		VERB_FPRINTF( QUIET, stderr, catgets( cat, 1, 2, MSG_ERROR_WRONGOS ), _osmajor, _osminor );
		return( ERR_WRONGOS );
	}

	if ( GetSysVars() )
	{
		VERB_FPUTS( QUIET, catgets( cat, 1, 5, MSG_ERROR_LOL ), stderr );
		return( ERR_SYSTEM );
	}
	
	// Needed by UninstallDriver()
	//
	*fpfpPrevInt2fHandler = _dos_getvect( 0x2F );
	
	installed = AlreadyInstalled( &pciPort, &clientId );
	
	if ( ! installed )
	{
		if ( uninstall )
		{
			VERB_FPUTS( QUIET, catgets( cat, 1, 13, MSG_ERROR_NOTINSTALLED ), stderr );
			return( ERR_NOTINST );
		}
		
		if ( 0x01 == InstallationCheck() )
		{
			VERB_FPUTS( QUIET, catgets(cat, 1, 6, MSG_ERROR_REDIR_NOT_ALLOWED ), stderr );
			return( ERR_NOALLWD );
		}
	
		if ( FAIL == VBoxAuxInitVirtual( &pciPort ) )
		{
			VERB_FPUTS( QUIET, catgets(cat, 1, 3, MSG_ERROR_NOVIRT ), stderr );
			return( ERR_NOVIRT );
		}
	
		if ( FAIL == VBoxAuxBeginSession( pciPort, &clientId ) )
		{
			VERB_FPUTS( QUIET, catgets( cat, 1, 8, MSG_ERROR_NOSHF ), stderr );
			return( ERR_NOSHF );	
		}

	}
	else
	{
		if ( uninstall )
		{
		}
	}
	
	// Get list of available shares
	//
	ret = VBoxAuxShflGetMappings( pciPort, clientId, mappings, &numMappings );
	
	if ( ret != VINF_SUCCESS )
	{
		ret = ERR_NOSHF;
		goto error;
	}

	pShfName->u16size	= SHFL_MAX_LEN;

	for ( mapping = 0; mapping < numMappings; ++mapping )
	{
	
		pShfName->u16length	= 0;
		pShfName->ucs2[0]	= 0;

		ret = VBoxAuxShflGetMapName ( pciPort, clientId, mappings[mapping].root, pShfName, sizeof( SHFLSTRING )+ SHFL_MAX_LEN );
			

	}

	if ( ret != ERR_SUCCESS )
	{
		VERB_FPUTS( QUIET, catgets(cat, 1, 8, MSG_ERROR_NOSHF ), stderr );
		goto error;
	}

error:
	VBoxAuxEndSession( pciPort, clientId );
	return ret;
}
