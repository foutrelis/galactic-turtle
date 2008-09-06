# Makefile - It's awesome, isn't it? :3
#
#   Copyright (C) 2008 Evangelos Foutras
#
#   This file is part of Galactic Turtle.
#
#   Galactic Turtle is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Galactic Turtle is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Galactic Turtle.  If not, see <http://www.gnu.org/licenses/>. */

CC        = gcc
CXX       = g++
CFLAGS    = -Wall -I./sqlite3 -I./QRBG
CXXFLAGS  = -I./QRBG
LDFLAGS   = -lm -lpthread -ldl

OUT       = galacticd
OBJS      = galacticd.o scoreboard.o common.o QRBG.o QRBG_wrapper.o sqlite3.o

all: $(OUT)

clean:
	rm -f $(OBJS)

distclean: clean
	rm -f $(OUT)

$(OUT): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDFLAGS)

galacticd.o: galacticd.c galacticd.h

scoreboard.o: scoreboard.c scoreboard.h

common.o: common.c common.h

QRBG.o: QRBG/QRBG.cpp QRBG/QRBG.h
	$(CXX) $(CXXFLAGS) -c -o $@ QRBG/QRBG.cpp

QRBG_wrapper.o: QRBG/QRBG_wrapper.cpp QRBG/QRBG_wrapper.h
	$(CXX) $(CXXFLAGS) -c -o $@ QRBG/QRBG_wrapper.cpp

sqlite3.o: sqlite3/sqlite3.c sqlite3/sqlite3.h
	$(CC) -c -o $@ sqlite3/sqlite3.c
