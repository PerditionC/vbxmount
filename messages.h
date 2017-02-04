#ifndef _MESSAHES_H
#define _MESSAGES_H
#pragma once
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
 */

#define MSG_MY_NAME				"%s %d.%d -"
#define MSG_COPYRIGHT			" (C) 2013 Eduardo Casino - GNU GPL Ver. 2.0\n"
#define MSG_HELP_1				"\nUsage:\n"
#define MSG_HELP_2				"   %s [/H][/V|/Q|/QQ] [/T]\n"
#define MSG_HELP_3				"   %s [/V|/Q/QQ] /U\n"
#define MSG_HELP_4				"        /H                  - Prints this message and exits\n"
#define MSG_HELP_5				"        /V                  - Verbose: Prints information on system resources\n"
#define MSG_HELP_6				"        /Q                  - Quiet: Omits copyright message\n"
#define MSG_HELP_7				"        /QQ                 - Silent: Does not print any messages at all\n"
#define MSG_HELP_8				"        /T                  - Test only: List shared folders and status\n"
#define MSG_HELP_9				"                              (With /QQ just checks if running on VirtualBox)\n"
#define MSG_HELP_19				"        /U                  - Uninstall\n"
#define MSG_ERROR_WRONGOS		" ERROR: Usupported DOS version %d.%d. Need 5.0 or higher.\n"
#define MSG_ERROR_NOVIRT		" ERROR: Not running on top of VirtualBox.\n"
#define MSG_ERROR_NOSHF			" ERROR: No Shared Folders found.\n"
#define MSG_ERROR_NOMEM			" ERROR: Not enough memory.\n"
#define MSG_ERROR_LOL			" ERROR: Can't get the List-Of-Lists!\n"
#define MSG_ERROR_UNINSTALL		" ERROR: Unable to uninstall.\n"
#define MSG_INFO_VBXVERS		" INFO: Running on VirtualBox %u.%u.%lu r%lu\n"
#define MSG_ERROR_NOTINSTALLED	" ERROR: Driver not installed.\n"
#define MSG_INFO_UNINSTALL		" Successfully removed from memory.\n"
#define MSG_ERROR_REDIR_NOT_ALLOWED " ERROR: Redirectors are not allowed.\n"

#endif /* _MESSAGES_H */
