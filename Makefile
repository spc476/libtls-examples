#########################################################################
#
# Copyright 2000 by Sean Conner.  All Rights Reserved.
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# Comments, questions and criticisms can be sent to: sean@conman.org
#
########################################################################

CC      = gcc -std=c99 -Wall -Wextra -pedantic
CFLAGS  = -g -I$(HOME)/JAIL/include -D_GNU_SOURCE
LDFLAGS = -g -L$(HOME)/JAIL/lib -Wl,-rpath,$(HOME)/JAIL/lib
LDLIBS  = -ltls

.PHONY: all clean

all : get get2 get3

clean :
	$(RM) $(shell find . -name '*~')
	$(RM) $(shell find . -name '*.bin')
	$(RM) get get2 get3
