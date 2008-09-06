/* galacticd.h - Structures used throughout the game. */

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

#define MAX_PLAYERS 10
#define MAX_PLANETS 15
#define MAX_TURNS 99
#define BOARD_SIZE 16
#define MAX_NICK_LEN 15

typedef enum player_state_e {
	MENU,               /* player is asked to pick an option from the menu */
	NEW_GAME_1,         /* player is asked for number of players */
	NEW_GAME_2,         /* player is asked for number of planets */
	NEW_GAME_3,         /* player is asked for number of turns */
	NEW_GAME_4,         /* player is asked to pick a topology */
	JOIN_GAME_1,        /* player is asked to enter a game id */
	JOIN_GAME_2,        /* player is asked to enter a nickname */
	IN_GAME_1,          /* player has joined a game and is waiting to start */
	IN_GAME_2,          /* player is asked for this turn's commands */
	IN_GAME_3,          /* player has entered his commands */
	END_GAME_1          /* the game has just ended */
} player_state_t;

typedef struct board_node_s {
	int x;
	int y;
	char name;
	char *owner;
	int ships, prod, attack;
} board_node_t;

typedef struct board_s {
	board_node_t nodes[BOARD_SIZE][BOARD_SIZE];
} board_t;

typedef struct player_s {
	int fd, in_game;
	player_state_t state;
	char nickname[MAX_NICK_LEN + 1];
	int new_game_players, new_game_planets, new_game_turns;
	board_t *new_game_board;
} player_t;

typedef struct move_s {
	player_t *owner;
	board_node_t *target;
	int ships;
	int attack;
	struct move_s *next;
} move_t;

typedef struct game_s {
	int players, planets, turns, cplayers, cturn, rplayers, id, open;
	board_t board;
	player_t *player_list[MAX_PLAYERS];
	board_node_t *planet_list[MAX_PLANETS];
	move_t *moves_list[MAX_TURNS];
} game_t;

typedef struct game_node_s {
	game_t game;
	struct game_node_s *next;
} game_node_t;
