#
# VBSMOUNT
#  A network redirector for mounting VirtualBox Shared Folders in DOS 
#  Copyright (C) 2013  Eduardo Casino
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA  02110-1301, USA.
#

CC = wcc
AS = nasm
LD = wlink
UPX = upx
RM = rm -f
CFLAGS  = -bt=dos -ms -q -s -oh -os -DREVERSE_HASH
ASFLAGS = -f obj -Worphan-labels -O9
#LDFLAGS =	SYSTEM dos \
#			OPTION QUIET \
#			OPTION MAP=$(MAPFILE)
LDFLAGS =	SYSTEM dos \
			ORDER \
				clname CODE segment BEGTEXT segment _TEXT segment ENDTEXT \
				clname FAR_DATA clname BEGDATA clname DATA clname BSS \
				clname STACK \
			OPTION QUIET \
			OPTION MAP=$(MAPFILE)
UPXFLAGS = --8086 --best

TARGET = vbsmount.exe
MAPFILE = $(TARGET:.exe=.map)

OBJS =	kitten.obj vboxaux.obj pci.obj main.obj miniclib.obj ucs2.obj vboxshfl.obj redir.obj endtext.obj


all: $(TARGET)

clean:
	$(RM) $(OBJS) $(TARGET) $(MAPFILE)

vbsmount.exe: $(OBJS)
	$(LD) $(LDFLAGS) NAME $@ FILE {$(OBJS)} $(LIBPATH) $(LIBRARY)
	$(UPX) $(UPXFLAGS) $@
	$(RM) $(@:.exe=.img) ; bfi -f=$(@:.exe=.img) .

	
# main.obj and kitten.obj must be compiled with 8086 instructions only to gracefully
#  execute the processor check in real, older machines
#
main.obj: main.c
	$(CC) -0 $(CFLAGS) -fo=$@ $<

kitten.obj: kitten.c
	$(CC) -0 $(CFLAGS) -fo=$@ $<

kitten.obj: kitten.h

main.obj: globals.h kitten.h messages.h vboxaux.h vboxshfl.h dosdefs.h

vbxaux.obj: globals.h messages.h kitten.h outpd.h vmmdev.h shflsvc.h

vboxshfl.obj: globals.h vmmdev.h shflsvc.h outpd.h

%.obj : %.c
	$(CC) -3 $(CFLAGS) -fo=$@ $<

%.obj : %.asm
	$(AS) $(ASFLAGS) -o $@ $<
	
