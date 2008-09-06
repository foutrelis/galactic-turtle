/* scoreboard.h - Function prototypes. */

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

void scoreboard_init();

void scoreboard_list();

void scoreboard_add(char *nickname, int score);
