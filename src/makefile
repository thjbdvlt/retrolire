# retrolire -- commande line bibliography manager.
# Copyright (C) 2024,2025  thjbdvlt
#
# retrolire is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# retrolire is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with retrolire.  If not, see <https://www.gnu.org/licenses/>.

CC := gcc

CFLAGS := -I /usr/include/postgresql \
	      -D_GNU_SOURCE -I/usr/include/ncursesw -lncursesw \
		  -L /usr/lib/ -lpq \
		  -Wall -Wextra -Wconversion \
		  -Wno-unused-variable -Wno-unused-parameter -lncurses \
		  -I.. -I. -Icommands

TINYFLAGS := -s -Os -fno-ident -fno-asynchronous-unwind-tables
CFLAGS += $(TINYFLAGS)

bin := ../bin/retrolire

CFILES := $(wildcard *.c) $(wildcard commands/*.c)
OBJS := $(CFILES:.c=.o)
headers := $(wildcard *.h) ../config.h

$(bin): $(OBJS) $(headers)
	gcc $^ $(CFLAGS) -o $@

../config.h: ../config.def.h
	cp -n $< $@

clean:
	rm -f *.o *.gch $(bin)
