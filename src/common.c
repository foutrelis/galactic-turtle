/* common.c - Some helper functions. */

/* Copyright (C) 2008 Evangelos Foutras

   This file is part of Galactic Turtle.

   Galactic Turtle is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Galactic Turtle is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Galactic Turtle.  If not, see <http://www.gnu.org/licenses/>. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

void exit_with(const char *r, int use_perror) {
	if (use_perror) {
		perror(r);
	} else {
		fprintf(stderr, "%s\n", r);
	}
	exit(1);
}

char *trim_string(char *cline) {
	char *tmp;
	
	/* Left trim */
	while(isspace(*cline)) {
		cline++;
	}
	
	/* Right trim */
	tmp = strchr(cline, '\0') - 1;
	while (tmp >= cline && isspace(*tmp)) {
		*tmp-- = '\0';
	}
	
	return cline;
}
