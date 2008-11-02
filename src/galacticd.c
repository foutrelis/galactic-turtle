/* galacticd.c - Server and game logic. */

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>
#include <getopt.h>
#include <errno.h>
#include <regex.h>
#include "common.h"
#include "galacticd.h"
#include "scoreboard.h"
#include "QRBG/QRBG_wrapper.h"

#define DFLPORT 8000
#define LISTENQ 10

static int really_random = 0;

static int random_int() {
	return really_random ? abs(QRBG_get_int()) : random();
}

static void send_to_all_players(game_t *g, char *msg) {
	int i;
	
	for (i = 0; i < g->cplayers; i++) {
		if (g->player_list[i]) {
			write(g->player_list[i]->fd, msg, strlen(msg));
		}
	}
}

static void generate_topology(player_t *p) {
	int x, y, k = p->new_game_planets;
	
	for (y = 0; y < BOARD_SIZE; y++) {
		for (x = 0; x < BOARD_SIZE; x++) {
			p->new_game_board->nodes[y][x].name = '.';
		}
	}
	
	while (k) {
		x = random_int() % BOARD_SIZE;
		y = random_int() % BOARD_SIZE;
		if (p->new_game_board->nodes[y][x].name == '.') {
			p->new_game_board->nodes[y][x].name = (char) 'A' + k - 1;
			p->new_game_board->nodes[y][x].x = x;
			p->new_game_board->nodes[y][x].y = y;
			p->new_game_board->nodes[y][x].owner = NULL;
			p->new_game_board->nodes[y][x].ships = 20;
			p->new_game_board->nodes[y][x].prod = 10;
			p->new_game_board->nodes[y][x].attack = 40;
			k--;
		}
	}
}

static void draw_topology(char *r, board_t *b) {
	int y, x;
	
	for (y = 0; y < BOARD_SIZE; y++) {
		for (x = 0; x < BOARD_SIZE; x++) {
			strncat(r, &b->nodes[y][x].name, 1);
			strcat(r, " ");
		}
		strcat(r, "|\r\n");
	}
}

static void draw_game_screen(game_t *g) {
	int y, x;
	char r[4096], line_buffer[128];
	
	memset(r, 0, sizeof(r));
	
	for (y = 0; y < BOARD_SIZE; y++) {
		for (x = 0; x < BOARD_SIZE; x++) {
			strncat(r, &g->board.nodes[y][x].name, 1);
			strcat(r, " ");
		}
		strcat(r, "| ");
		if (!y) {
			strcat(r, "Planet  Ships  Prod  Attack%  Owner\r\n");
		} else if (y <= g->planets) {
			memset(line_buffer, 0, sizeof(line_buffer));
			if (g->planet_list[y-1]->owner) {
				sprintf(line_buffer, "%-6c  %-5d  %-4d  %-7d  %s\r\n",
					                 'A'+y-1,
					                 g->planet_list[y-1]->ships,
					                 g->planet_list[y-1]->prod,
					                 g->planet_list[y-1]->attack,
					                 g->planet_list[y-1]->owner);
			} else {
				sprintf(line_buffer, "%c\r\n", 'A'+y-1);
			}
			strcat(r, line_buffer);
		} else {
			strcat(r, "\r\n");
		}
	}
	for (x = 0; x < BOARD_SIZE; x++) {
		strcat(r, "--");
	}
	if (g->cturn <= g->turns) {
		sprintf(line_buffer, "+-[Galactic Turtle, Turn #%2d/%2d]-\r\n",
		                     g->cturn, g->turns);
	} else {
		sprintf(line_buffer, "+-[Galactic Turtle, Game Over :o ]-\r\n");
	}
	strcat(r, line_buffer);
	
	send_to_all_players(g, r);
}

static void reset_player_list(game_t *g) {
	int i, j;
	player_t *new_player_list[MAX_PLAYERS];
	
	memset(&new_player_list, 0, sizeof(new_player_list));
	
	for (i = j = 0; i < g->players; i++) {
		if (g->player_list[i]) {
			new_player_list[j++] = g->player_list[i];
		}
	}
	
	for (i = j = 0; i < g->players; i++) {
		g->player_list[i] = new_player_list[i];
	}
}

static void show_menu_to_player(player_t *p) {
	char response[256];
	
	sprintf(response, "\r\n"
	                  "=================================\r\n"
	                  "Welcome to Galactic Turtle! (.-.)\r\n"
	                  "=================================\r\n\r\n"
	                  "1. Create new game\r\n"
	                  "2. Game list\r\n"
	                  "3. Join a game\r\n"
	                  "4. Highscore list\r\n"
	                  "5. Exit\r\n\r\n"
	                  "Selection: ");
	write(p->fd, response, strlen(response));
}

static void show_game_list_to_player(player_t *p, game_node_t *game_list) {
	game_node_t *tmp = game_list;
	char response[256], mini_buffer[10];
	
	memset(response, 0, sizeof(response));
	
	strcpy(response, "\r\n"
	                 "Game ID  Players  Planets  Turns  Open\r\n"
	                 "=======  =======  =======  =====  ====\r\n");
	write(p->fd, response, strlen(response));
	
	while (tmp != NULL) {
		sprintf(mini_buffer, "%d/%d", tmp->game.cplayers, tmp->game.players);
		sprintf(response, "%-7d  %-7s  %-7d  %'-5d  %-4s\r\n",
		                   tmp->game.id,
		                   mini_buffer,
		                   tmp->game.planets,
		                   tmp->game.turns,
		                   tmp->game.open ? "Yes" : "No");
		write(p->fd, response, strlen(response));
		
		tmp = tmp->next;
	}
}

static void show_scoreboard_to_player(player_t *p) {
	char response[256];
	
	memset(response, 0, sizeof(response));
	
	strcpy(response, "\r\n"
	                 "Player           Best Score  Last Score\r\n"
	                 "======           ==========  ==========\r\n");
	write(p->fd, response, strlen(response));
	
	scoreboard_list(p);
}

static int add_game_to_list(game_node_t **game_list, player_t *p) {
	game_node_t *tmp = *game_list;
	int i, x, y, game_id = 1;
	
	if (!*game_list) {
		tmp = *game_list = malloc(sizeof(game_node_t));
	} else {
		while (tmp != NULL && tmp->next != NULL) {
			tmp = tmp->next;
		}
		game_id = tmp->game.id + 1;
		tmp->next = malloc(sizeof(game_node_t));
		tmp = tmp->next;
	}
	
	tmp->game.players = p->new_game_players;
	tmp->game.planets = p->new_game_planets;
	tmp->game.turns = p->new_game_turns;
	tmp->game.id = game_id;
	tmp->game.open = 1;
	tmp->game.cplayers = 0;
	tmp->game.cturn = 1;
	memset(&tmp->game.player_list, 0, sizeof(tmp->game.player_list));
	memset(&tmp->game.planet_list, 0, sizeof(tmp->game.planet_list));
	memcpy(&tmp->game.board, p->new_game_board, sizeof(board_t));
	for (y = 0; y < BOARD_SIZE; y++) {
		for (x = 0; x < BOARD_SIZE; x++) {
			if (tmp->game.board.nodes[x][y].name != '.') {
				i = (int) tmp->game.board.nodes[x][y].name - 'A';
				tmp->game.planet_list[i] = &tmp->game.board.nodes[x][y];
			}
		}
	}
	tmp->next = NULL;
	
	return game_id;
}

static game_t *find_game_by_id(int game_id, game_node_t *game_list) {
	game_node_t *tmp = game_list;
	
	while (tmp != NULL && tmp->game.id != game_id) {
		tmp = tmp->next;
	}
	
	return &tmp->game;
}

static void add_player_to_game(game_t *g, player_t *p) {
	int i;
	
	for (i = 0; i < g->players; i++) {
		if (!g->player_list[i]) {
			g->player_list[i] = p;
			break;
		}
	}
}

static int nickname_available(game_t *g, char *nickname) {
	int i;
	
	for (i = 0; i < g->cplayers; i++) {
		if (g->player_list[i] && !strcasecmp(g->player_list[i]->nickname, nickname)) {
			return 0;
		}
	}
	return 1;
}

static int nickname_valid(char *nickname) {
	char regex[] = "^[A-z0-9 ]+$";
	static int initialized = 0;
	static regex_t *regex_comp;
	
	if (!initialized) {
		if (!(regex_comp = malloc(sizeof(regex_t)))) {
			exit_with("malloc error", 1);
		}
		if (regcomp(regex_comp, regex, REG_EXTENDED | REG_NOSUB)) {
			exit_with("regex compilation error", 0);
		}
		initialized = 1;
	}
	
	return regexec(regex_comp, nickname, 0, NULL, 0) ? 0 : 1;
}

static void assign_planets_to_players(game_t *g) {
	int i, j;
	
	for (i = 0; i < g->players; i++) {
		while (g->planet_list[(j = random_int() % g->planets)]->owner);
		g->planet_list[j]->owner = g->player_list[i]->nickname;
	}
}

static void check_if_game_is_full(game_t *g) {
	if (g->cplayers == g->players) {
		g->open = 0;
	}
}

static void prompt_player_for_move(player_t *p) {
	char buffer[32];
	
	sprintf(buffer, "%s> ", p->nickname);
	write(p->fd, buffer, strlen(buffer));
}

static void prompt_players_for_move(game_t *g) {
	int i;
	
	for (i = 0; i < g->cplayers; i++) {
		prompt_player_for_move(g->player_list[i]);
	}
}

static void check_if_game_is_ready_to_start(game_t *g) {
	int i;
	
	if (g->rplayers == g->players) {
		assign_planets_to_players(g);
		draw_game_screen(g);
		prompt_players_for_move(g);
		for (i = 0; i < g->cplayers; i++) {
			g->player_list[i]->state = IN_GAME_2;
		}
		g->rplayers = 0;
	}
}

static int player_exists_in_game(game_t *g, char *nickname) {
	int i;
	
	for (i = 0; i < g->cplayers; i++) {
		if (!strcmp(g->player_list[i]->nickname, nickname)) {
			return 1;
		}
	}
	
	return 0;
}

static void cleanup_orphaned_planets(game_t *g, int show_message) {
	int i, j;
	char buffer[128];
	
	for (i = 0; i < g->planets; i++) {
		if (g->planet_list[i]->owner && !player_exists_in_game(g, g->planet_list[i]->owner)) {
			if (show_message) {
				sprintf(buffer, "%s has disconnected.\r\n", g->planet_list[i]->owner);
				send_to_all_players(g, buffer);
			}
			for (j = i+1; j < g->planets; j++) {
				if (g->planet_list[j]->owner == g->planet_list[i]->owner) {
					g->planet_list[j]->owner = NULL;
				}
			}
			free(g->planet_list[i]->owner);
			g->planet_list[i]->owner = NULL;
		}
	}
}

static int calc_score(game_t *g, char *nickname) {
	int i, s;
	
	for (i = s = 0; i < g->planets; i++) {
		if (g->planet_list[i]->owner == nickname) {
			s += g->planet_list[i]->ships * g->planet_list[i]->attack;
		}
	}
	
	return s;
}

static player_t *decide_winner(game_t *g) {
	int i, s, m = 0, winner = 0;
	
	for (i = 0; i < g->cplayers; i++) {
		if ((s = calc_score(g, g->player_list[i]->nickname)) > m) {
			m = s;
			winner = i;
		}
	}
	
	return g->player_list[winner];
}

static void end_game(game_t *g) {
	int i, s;
	player_t *winner;
	char buffer[4096], line_buffer[128];
	
	cleanup_orphaned_planets(g, 0);
	
	/* If all players disconnected during a round there's nothing to do */
	if (!g->cplayers) {
		return;
	}
	
	for (i = 0; i < g->cplayers; i++) {
		g->player_list[i]->state = END_GAME_1;
	}
	
	strcpy(buffer, "Player               Score\r\n");
	strcat(buffer, "======               =====\r\n");
	for (i = 0; i < g->cplayers; i++) {
		s = calc_score(g, g->player_list[i]->nickname);
		scoreboard_add(g->player_list[i]->nickname, s);
		sprintf(line_buffer, "%-20s %d\r\n", g->player_list[i]->nickname, s);
		strcat(buffer, line_buffer);
	}
	strcat(buffer, "\r\n");
	
	winner = decide_winner(g);
	sprintf(line_buffer, "%s wins the game. Press enter to go back to the menu.",
	                winner->nickname);
	strcat(buffer, line_buffer);
	
	send_to_all_players(g, buffer);
}

/* Returns 1 with probability p% */
static int do_it_faggot(int p) {
	if (random_int() % 100 < p) {
		return 1;
	}
	return 0;
}

static void advance_turn(game_t *g) {
	int i, r;
	double defense;
	move_t *m = g->moves_list[g->cturn - 1];
	char buffer[128];
	int diff;
	
	send_to_all_players(g, "\r\n");
	
	cleanup_orphaned_planets(g, 1);
	
	/* Random events */	
	for (i = 0; i < g->planets; i++) {
		if (g->planet_list[i]->owner && do_it_faggot(10)) {
			r = (random_int() % 50) + 1;
			diff = ceil((g->planet_list[i]->prod * r) / 100);
			if (diff) {
				if (do_it_faggot(50)) {
					g->planet_list[i]->prod -= diff;
					switch (random_int() % 3) {
						case 0:
							sprintf(buffer, "Due to lazy workers, ship productivity of"
								            " planet %c decreases %d%%.\r\n", 'A'+i, r);
							break;
						case 1:
							sprintf(buffer, "An accident takes place and ship productivity of"
								            " planet %c decreases %d%%.\r\n", 'A'+i, r);
							break;
						case 2:
							sprintf(buffer, "Workers go on strike. Ship productivity of"
								            " planet %c decreases %d%%.\r\n", 'A'+i, r);
							break;
						default:
							break;
					}
					send_to_all_players(g, buffer);
				} else {
					g->planet_list[i]->prod += diff;
					switch (random_int() % 3) {
						case 0:
							sprintf(buffer, "Thanks to better economy, ship productivity of"
								            " planet %c increases %d%%.\r\n", 'A'+i, r);
							break;
						case 1:
							sprintf(buffer, "New equipment arrives. Ship productivity of"
								            " planet %c increases %d%%.\r\n", 'A'+i, r);
							break;
						case 2:
							sprintf(buffer, "More people are hired and ship productivity of"
								            " planet %c increases %d%%.\r\n", 'A'+i, r);
							break;
						default:
							break;
					}
					send_to_all_players(g, buffer);
				}
			}
		}
		
		if (g->planet_list[i]->owner && do_it_faggot(10)) {
			r = (random_int() % 50) + 1;
			diff = ceil((g->planet_list[i]->attack * r) / 100);
			if (diff) {
				if (do_it_faggot(50)) {
					g->planet_list[i]->attack -= diff;
					switch (random_int() % 3) {
						case 0:
							sprintf(buffer, "Due to poor quality ammunition, attack ratio of"
								            " planet %c decreases %d%%.\r\n", 'A'+i, r);
							break;
						case 1:
							sprintf(buffer, "Ammunition delivery is late, attack ratio of"
								            " planet %c decreases %d%%.\r\n", 'A'+i, r);
							break;
						case 2:
							sprintf(buffer, "Weapon systems maintenance, attack ratio of"
								            " planet %c decreases %d%%.\r\n", 'A'+i, r);
							break;
						default:
							break;
					}
					send_to_all_players(g, buffer);
				} else {
					g->planet_list[i]->attack += diff;
					switch (random_int() % 3) {
						case 0:
							sprintf(buffer, "Thanks to new technology ships, attack ratio of"
								            " planet %c increases %d%%.\r\n", 'A'+i, r);
							break;
						case 1:
							sprintf(buffer, "An ammunition delivery raises the attack ratio of"
								            " planet %c by %d%%.\r\n", 'A'+i, r);
							break;
						case 2:
							sprintf(buffer, "New weapon system developed, attack ratio of"
								            " planet %c increases %d%%.\r\n", 'A'+i, r);
							break;
						default:
							break;
					}
					send_to_all_players(g, buffer);
				}
			}
		}
		
		if (g->planet_list[i]->owner && do_it_faggot(1)) {
			do {
				r = random_int() % g->cplayers;
			} while (g->planet_list[i]->owner == g->player_list[r]->nickname);
			g->planet_list[i]->owner = g->player_list[r]->nickname;
			sprintf(buffer, "The people of planet %c decide to join %s.\r\n", 'A'+i, g->player_list[r]->nickname);
			send_to_all_players(g, buffer);
		}
		
		/* Produce new ships */
		if (g->planet_list[i]->owner) {
			g->planet_list[i]->ships += g->planet_list[i]->prod;
		}
	}
	
	g->cturn++;
	
	while (m) {
		if (m->owner->in_game != g->id) {
			m = m->next;
			continue;
		}
		if (m->target->owner && m->owner->nickname == m->target->owner) {
			m->target->ships += m->ships;
			sprintf(buffer, "Reinforcements (%d ships) arrive at planet %c.\r\n", m->ships, m->target->name);
			send_to_all_players(g, buffer);
		} else {
			defense = m->target->attack + (random_int() % 16);
			while (m->ships && m->target->ships) {
				if ((random_int() % 101) > m->attack) {
					m->ships--;
				}
				if (!m->ships) {
					break;
				}
				if ((random_int() % 101) > defense) {
					m->target->ships--;
				}
			}
			
			if (m->target->owner) {           /* this is not a neutral planet */
				if (m->ships) {
					sprintf(buffer, "%s attacks planet %c and wins with %d"
						            " ships remaining.\r\n",
						            m->owner->nickname, m->target->name, m->ships);
					m->target->owner = m->owner->nickname;
					m->target->ships = m->ships;
				} else {
					sprintf(buffer, "%s attacks planet %c but loses. %s is"
						            " left with %d ships.\r\n", 
						            m->owner->nickname, m->target->name,
						            m->target->owner, m->target->ships);
				}
			} else {                       /* this is a neutral planet */
				if (m->ships) {
					sprintf(buffer, "%s conquers planet %c with %d"
						            " ships remaining.\r\n",
						            m->owner->nickname, m->target->name, m->ships);
					m->target->owner = m->owner->nickname;
					m->target->ships = m->ships;
					m->target->prod = 10;
				} else {
					sprintf(buffer, "%s tries to conquer planet %c but fails.\r\n", 
						            m->owner->nickname, m->target->name);
				}
			}
			send_to_all_players(g, buffer);
		}
		m = m->next;
	}
	
	for (i = 0; i < g->cplayers; i++) {
		g->player_list[i]->state = IN_GAME_2;
	}
	g->rplayers = 0;
	
	draw_game_screen(g);
	if (g->cturn > g->turns || g->cplayers < 2) {
		end_game(g);
	} else {
		prompt_players_for_move(g);
	}
}

static void add_move_to_list(move_t **move_list, player_t *owner, board_node_t *target, int ships, int attack) {
	move_t *tmp = *move_list;
	
	if (!*move_list) {
		tmp = *move_list = malloc(sizeof(move_t));
	} else {
		while (tmp != NULL && tmp->next != NULL) {
			if (tmp->owner == owner && tmp->attack == attack && tmp->target == target) {
				tmp->ships += ships;
				return;
			}
			tmp = tmp->next;
		}
		if (tmp->owner == owner && tmp->attack == attack && tmp->target == target) {
			tmp->ships += ships;
			return;
		}
		tmp->next = malloc(sizeof(move_t));
		tmp = tmp->next;
	}
	
	tmp->owner = owner;
	tmp->target = target;
	tmp->ships = ships;
	tmp->attack = attack;
	tmp->next = NULL;
}

static int do_move_parse(char *line, int *from, int *to, int *n) {
	char regex[] = "^([A-z])[[:space:]]+([A-z])[[:space:]]+([1-9][0-9]*)$";
	static int initialized = 0;
	static regex_t *regex_comp;
	static regmatch_t *reg_matches;
	
	if (!initialized) {
		if (!(regex_comp = malloc(sizeof(regex_t)))) {
			exit_with("malloc error", 1);
		}
		if (!(reg_matches = malloc(sizeof(regmatch_t) * 4))) {
			exit_with("malloc error", 1);
		}
		if (regcomp(regex_comp, regex, REG_EXTENDED)) {
			exit_with("regex compilation error", 0);
		}
		initialized = 1;
	}
	
	if(!regexec(regex_comp, line, 4, reg_matches, 0)) {
		line[reg_matches[1].rm_eo] = '\0';
		*from = toupper(line[reg_matches[1].rm_so]) - 'A';
		
		line[reg_matches[2].rm_eo] = '\0';
		*to = toupper(line[reg_matches[2].rm_so]) - 'A';
		
		line[reg_matches[3].rm_eo] = '\0';
		*n = atoi(&line[reg_matches[3].rm_so]);
		
		return 1;
	} else {
		return 0;
	}
}

static int do_move(player_t *p, char *cmd, game_t *g) {
	int from, to, n, diffx, diffy, arrival_turn;
	
	if (!strlen(cmd)) {
		return -1;                      /* Empty command .-. */
	}
	
	if (!do_move_parse(cmd, &from, &to, &n)) {
		return -2;                      /* Invalid command */
	}
	
	if (from < 0 || from > g->planets - 1) {
		return 1;                       /* Invalid source planet */
	} else if (g->planet_list[from]->owner != p->nickname) {
		return 2;                       /* Player doesn't own the planet */
	}
	
	if (to < 0 || to > g->planets - 1 || to == from) {
		return 3;                       /* Invalid target planet */
	}
	
	if (n <= 0 || n > g->planet_list[from]->ships) {
		return 4;                       /* Invalid number of ships */
	}
	
	diffx = abs(g->planet_list[to]->x - g->planet_list[from]->x);
	diffy = abs(g->planet_list[to]->y - g->planet_list[from]->y);
	arrival_turn = floor(sqrt(diffx*diffx + diffy*diffy)) + g->cturn;
	g->planet_list[from]->ships -= n;
	if (arrival_turn <= MAX_TURNS) {
		add_move_to_list(&g->moves_list[arrival_turn - 1], p, g->planet_list[to], n, g->planet_list[from]->attack);
	}
	
	return 0;
}

static void player_disconnected(player_t *p, game_node_t *game_list) {
	int i = -1;
	char *nickname_copy = NULL;
	game_t *tmp;
	
	if (p->new_game_board) {
		free(p->new_game_board);
		p->new_game_board = NULL;
	}
	
	if (p->in_game < 0) {
		tmp = find_game_by_id(-p->in_game, game_list);
		tmp->cplayers--;
		tmp->open = 1;
	}
	
	if (p->in_game > 0) {
		tmp = find_game_by_id(p->in_game, game_list);
		p->in_game = 0;
		
		/* Reset player list */
		while (tmp->player_list[++i] != p);
		
		tmp->player_list[i] = NULL;
		tmp->cplayers--;
		if (p->state != IN_GAME_2) {
			tmp->rplayers--;
		}
		reset_player_list(tmp);
		
		/* Duplicate player's nickname and set it as planets' owner */
		for (i = 0; i < tmp->planets; i++) {
			if (tmp->planet_list[i]->owner == p->nickname) {
				if (!nickname_copy) {
					nickname_copy = strdup(p->nickname);
				}
				tmp->planet_list[i]->owner = nickname_copy;
			}
		}
		
		if (!tmp->open && tmp->rplayers == tmp->cplayers) {
			advance_turn(tmp);
		}
	}
}

static int player_menu(player_t *p, char *cmd, game_node_t *game_list) {
	char response[64];
	int selection = atoi(cmd);
	
	memset(response, 0, sizeof(response));
	
	if (selection == 1) {
		sprintf(response, "Number of players [2-%d]: ", MAX_PLAYERS);
		p->state = NEW_GAME_1;
	} else if (selection == 2) {
		show_game_list_to_player(p, game_list);
		show_menu_to_player(p);
	} else if (selection == 3) {
		strcpy(response, "Enter game id: ");
		p->state = JOIN_GAME_1;
	} else if (selection == 4) {
		show_scoreboard_to_player(p);
		show_menu_to_player(p);
	} else if (selection == 5) {
		strcpy(response, "Bye!\r\n");
	} else {
		strcpy(response, "Invalid selection, try again: ");
	}
	
	if (strlen(response)) {
		write(p->fd, response, strlen(response));
	}
	
	return (1 <= selection && selection <= 5) ? selection : -1;
}

static void player_new_game_1(player_t *p, char *cmd) {
	char response[32];
	int selection = atoi(cmd);
	
	memset(response, 0, sizeof(response));
	
	if (2 <= selection && selection <= MAX_PLAYERS) {
		sprintf(response, "Number of planets [%d-%d]: ", selection, MAX_PLANETS);
		p->new_game_players = selection;
		p->state = NEW_GAME_2;
	} else {
		strcpy(response, "Invalid selection, try again: ");
	}
	
	write(p->fd, response, strlen(response));
}

static void player_new_game_2(player_t *p, char *cmd) {
	char response[32];
	int selection = atoi(cmd);
	
	memset(response, 0, sizeof(response));
	
	if (p->new_game_players <= selection && selection <= MAX_PLANETS) {
		sprintf(response, "Number of turns [1-%d]: ", MAX_TURNS);
		p->new_game_planets = selection;
		p->new_game_board = malloc(sizeof(board_t));
		generate_topology(p);
		p->state = NEW_GAME_3;
	} else {
		strcpy(response, "Invalid selection, try again: ");
	}
	
	write(p->fd, response, strlen(response));
}

static void player_new_game_3(player_t *p, char *cmd) {
	char response[4096];
	int selection = atoi(cmd);
	
	memset(response, 0, sizeof(response));
	
	if (1 <= selection && selection <= MAX_TURNS) {
		draw_topology(response, p->new_game_board);
		strcat(response, "\r\nLike it? [y/N]? ");
		p->new_game_turns = selection;
		p->state = NEW_GAME_4;
	} else {
		strcpy(response, "Invalid selection, try again: ");
	}
	
	write(p->fd, response, strlen(response));
}

static void player_new_game_4(player_t *p, char *cmd, game_node_t **game_list) {
	char response[4096];
	int selection = tolower(cmd[0]);
	int game_id;
	
	memset(response, 0, sizeof(response));
	
	if (selection == 'y') {
		game_id = add_game_to_list(game_list, p);
		sprintf(response, "Game created! ID: %d \r\n", game_id);
		free(p->new_game_board);
		p->new_game_board = NULL;
		p->state = MENU;
	} else if (selection == 'n' || !strlen(cmd)) {
		strcpy(response, "OK, here's another one:\r\n");
		generate_topology(p);
		draw_topology(response, p->new_game_board);
		strcat(response, "\r\nLike it? [y/N]? ");
	} else {
		strcpy(response, "Invalid selection, try again: ");
	}
	
	if (strlen(response)) {
		write(p->fd, response, strlen(response));
	}
	
	if (p->state == MENU) {
		show_menu_to_player(p);
	}
}

static void player_join_game_1(player_t *p, char *cmd, game_node_t *game_list) {
	char response[64];
	int selection = atoi(cmd);
	game_t *tmp;
	
	memset(response, 0, sizeof(response));
	
	tmp = find_game_by_id(selection, game_list);
	
	if (tmp && tmp->open) {
		sprintf(response, "Enter a nickname [%d chars max]: ", MAX_NICK_LEN);
		p->in_game = -selection;
		tmp->cplayers++;
		check_if_game_is_full(tmp);
		p->state = JOIN_GAME_2;
	} else if (tmp) {
		strcpy(response, "\r\nThis game is not accepting new players!\r\n");
		p->state = MENU;
	} else {
		strcpy(response, "\r\nNonexistent game!\r\n");
		p->state = MENU;
	}
	
	write(p->fd, response, strlen(response));
	
	if (p->state == MENU) {
		show_menu_to_player(p);
	}
}

static void player_join_game_2(player_t *p, char *cmd, game_node_t *game_list) {
	char response[128];
	game_t *tmp;
	
	memset(response, 0, sizeof(response));
	memset(p->nickname, 0, sizeof(p->nickname));
	
	tmp = find_game_by_id(-p->in_game, game_list);
	
	strncpy(p->nickname, cmd, MAX_NICK_LEN);
	if (!strlen(p->nickname)) {
		strcpy(response, "Your nickname cannot be empty, try again: ");
	} else if (!nickname_available(tmp, p->nickname)) {
		strcpy(response, "The selected nickname is taken, try another one: ");
	} else if (!nickname_valid(p->nickname)) {
		strcpy(response, "Your nickname may only consist of letters (A-Z, a-z),"
		                 " numbers (0-9) and spaces.\r\n"
		                 "Invalid nickname, try again: ");
	} else {
		strcpy(response, "Waiting for other players to join..\r\n");
		add_player_to_game(tmp, p);
		p->in_game *= -1;
		tmp->rplayers++;
		p->state = IN_GAME_1;
	}
	
	write(p->fd, response, strlen(response));
	
	check_if_game_is_ready_to_start(tmp);
}

static void player_in_game_2(player_t *p, char *cmd, game_node_t *game_list) {
	char response[64];
	game_t *tmp;
	
	memset(response, 0, sizeof(response));
	
	tmp = find_game_by_id(p->in_game, game_list);
	
	if (!strcasecmp(cmd, "pass")) {
		if (++tmp->rplayers == tmp->cplayers) {
			advance_turn(tmp);
		} else {
			strcpy(response, "Please wait for other players to enter their commands..\r\n");
			p->state = IN_GAME_3;
		}
	} else {
		switch (do_move(p, cmd, tmp)) {
			case -1:
				strcpy(response, "To end your turn enter 'pass'.\r\n");
				break;
			case -2:
				strcpy(response, "You're doing it wrong.\r\n");
				break;
			case 1:
				strcpy(response, "Wrong source planet buddy.\r\n");
				break;
			case 2:
				strcpy(response, "You don't own that planet.\r\n");
				break;
			case 3:
				strcpy(response, "You can't attack that planet.\r\n");
				break;
			case 4:
				strcpy(response, "You can has ships, not.\r\n");
				break;
			default:
				break;
		}
	}
	
	if (strlen(response)) {
		write(p->fd, response, strlen(response));
	}
	
	if (strcasecmp(cmd, "pass")) {
		prompt_player_for_move(p);
	}
}

static void player_end_game_1(player_t *p, game_node_t *game_list) {
	game_t *tmp;
	
	tmp = find_game_by_id(p->in_game, game_list);
	tmp->cplayers--;
	p->in_game = 0;
	show_menu_to_player(p);
	p->state = MENU;
}

int main(int argc, char *argv[]) {
	int i, r, maxi, maxfd, lport = 0, listenfd, connfd, sockfd, nready, opt;
	int is_daemon = 0;
	int one = 1;
	socklen_t len;
	player_t client[FD_SETSIZE];
	ssize_t n;
	fd_set rset, allset;
	char *c, buffer[1024];
	struct sockaddr_in cliaddr, servaddr;
	game_node_t *game_list = NULL;
	int option_index = 0;
	char *version;
	struct option long_options[] = {
		{"really-random", 0, 0, 'r'},
		{"version", 0, 0, 'v'}
	};
	
	/* Parse command-line options */
	while ((opt = getopt_long(argc, argv, "vdp:", long_options, &option_index)) != -1) {
		switch (opt) {
			case 'v':
#ifdef VERSION
				version = VERSION;
#else
				version = "(Unknown Version)";
#endif
				printf("Galactic Turtle %s\n", version);
				exit(0);
				break;
			case 'p':
				lport = atoi(optarg);
				break;
			case 'd':
				is_daemon = 1;
				break;
			case 'r':
				really_random = 1;
				break;
			default:
			case '?':
				fprintf(stderr, "Usage: %s [-d] [-p port] [--really-random]\n", argv[0]);
				exit(1);
		}
	}
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (listenfd < 0) {
		exit_with("socket error", 1);
	}
	
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void *) &one, sizeof(int)) < 0) {
		exit_with("setsockopt error", 1);
	}
	
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(lport ? lport : DFLPORT);
	
	if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		exit_with("bind error", 1);
	}
	
	if (listen(listenfd, LISTENQ) < 0) {
		exit_with("listen error", 1);
	}
	
	maxfd = listenfd;    /* lazy pointer to the biggest fd (for select) */
	maxi = -1;           /* lazy pointer to the biggest index in client[] */
	
	for (i = 0; i < FD_SETSIZE ; i++) {
		client[i].fd = -1;    /* -1 indicates available entry */
	}
	
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
	if (really_random) {
		QRBG_init();                     /* Initiate QRBG service */
	} else {
		srandom(time(NULL) + getpid());  /* I can has random numbers? kthxbai */
	}
	scoreboard_init();                   /* Initiate our scoreboard database */
	
	if (is_daemon) {
		daemon(0,0);
	}
	
	while (1) {
		rset = allset;
		
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
		
		if (nready < 0) {
			exit_with("select error", 1);
		}
		
		if (FD_ISSET(listenfd, &rset)) {    /* we have a new connection */
			len = sizeof(cliaddr);
			
			connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len);
			
			if (connfd < 0) {
				exit_with("accept error", 1);
			}
			
			for (i=0; i<FD_SETSIZE; i++) { /* find empty slot in client array */
				if (client[i].fd < 0) {
					client[i].fd = connfd;    /* record client's connected fd */
					client[i].in_game = 0;
					client[i].new_game_board = NULL;
					client[i].state = MENU;
					show_menu_to_player(&client[i]);   /* show menu to player */
					FD_SET(connfd, &allset);
					if (connfd > maxfd) {
						maxfd = connfd;
					}
					if (i > maxi) {
						maxi = i;
					}
					break;
				}
			}
			
			if (i == FD_SETSIZE) {    /* no slots available */
				write(connfd, "Too many clients. Try again later.\r\n", 40);
				close(connfd);
			}
			
			/* if no other descriptors are ready (according to select) then
			   skip the following loop */
			if (--nready <= 0) {
				continue;
			}
		}
		
		for (i=0; i<=maxi; i++) {                /* check clients for data */
			if ((sockfd = client[i].fd) < 0) {   /* empty slot, let's skip it */
				continue;
			}
			
			if (FD_ISSET(sockfd, &rset)) {
				memset(buffer, 0, sizeof(buffer));
				n = read(sockfd, buffer, 1024);
				if (n == -1) {                   /* read error */
					/* do nothing */
				} else if (n == 0) {             /* client disconnected */
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i].fd = -1;
					player_disconnected(&client[i], game_list);
				} else {
					if ((c = strchr(buffer, '\r')) > 0 || (c = strchr(buffer, '\n')) > 0) {
						*c = '\0';
					}
					c = trim_string(buffer);
					switch (client[i].state) {
						case MENU:
							r = player_menu(&client[i], c, game_list);
							if (r == 5) {
								close(sockfd);
								FD_CLR(sockfd, &allset);
								client[i].fd = -1;
								player_disconnected(&client[i], game_list);
							}
							break;
						case NEW_GAME_1:
							player_new_game_1(&client[i], c);
							break;
						case NEW_GAME_2:
							player_new_game_2(&client[i], c);
							break;
						case NEW_GAME_3:
							player_new_game_3(&client[i], c);
							break;
						case NEW_GAME_4:
							player_new_game_4(&client[i], c, &game_list);
							break;
						case JOIN_GAME_1:
							player_join_game_1(&client[i], c, game_list);
							break;
						case JOIN_GAME_2:
							player_join_game_2(&client[i], c, game_list);
							break;
						case IN_GAME_2:
							player_in_game_2(&client[i], c, game_list);
							break;
						case END_GAME_1:
							player_end_game_1(&client[i], game_list);
							break;
						default:
							break;
					}
				}
				
				/* no more descriptors require processing? call select again */
				if (--nready <= 0) {
					break;
				}
			}
		}
	}
	
	return 0;
}
