/* scoreboard.c - Interacts with our awesome sqlite database that keeps
   the scores. */

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
#include <strings.h>
#include "sqlite3.h"
#include "galacticd.h"

#define SCOREBOARD_DB "scoreboard.db"

static sqlite3 *db;

static int scoreboard_list_callback(void *pp, int argc, char **argv, char **column_names) {
	char buffer[128];
	player_t *p = (player_t *) pp;
	
	sprintf(buffer, "%-15s  %-10s  %-10s\n", argv[0], argv[1], argv[2]);
	write(p->fd, buffer, strlen(buffer));
	
	return 0;
}

void scoreboard_init() {
	char *errmsg;
	char *q = "CREATE TABLE IF NOT EXISTS `scores`("
	          "`nickname` varchar(64) NOT NULL PRIMARY KEY,"
	          "`best` integer NOT NULL,"
	          "`last` integer NOT NULL)";
	
	if (sqlite3_open(SCOREBOARD_DB, &db) != SQLITE_OK) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		exit(1);
	}
	if (sqlite3_exec(db, q, NULL, NULL, &errmsg) != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", errmsg);
		sqlite3_free(errmsg);
		exit(1);
	}
}

void scoreboard_list(player_t *p) {
	char *errmsg;
	char *q = "SELECT `nickname`, `best`, `last` FROM scores ORDER BY `best` DESC;";
	
	if (sqlite3_exec(db, q, scoreboard_list_callback, p, &errmsg) != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", errmsg);
		sqlite3_free(errmsg);
		exit(1);
	}
}

void scoreboard_add(char *nickname, int score) {
	char *insert_q = "INSERT INTO `scores` (`nickname`, `best`, `last`) "
	                 "VALUES (?, ?, ?);";
	char *update_q = "UPDATE `scores` "
	                 "SET `last` = ?,"
	                 "`best` = MAX(`best`, ?)"
	                 "WHERE `nickname` = ?;";
	static int initialized = 0;
	static sqlite3_stmt *insert_stmp, *update_stmp;
	static const char *insert_stmp_tail, *update_stmp_tail;
	int insert_rc, update_rc;
	
	if (!initialized) {
		if (sqlite3_prepare_v2(db, insert_q, 128, &insert_stmp, &insert_stmp_tail) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
			exit(1);
		}
		if (sqlite3_prepare_v2(db, update_q, 128, &update_stmp, &update_stmp_tail) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
			exit(1);
		}
		initialized = 1;
	}
	
	sqlite3_reset(insert_stmp);
	sqlite3_reset(update_stmp);
	
	sqlite3_bind_text(insert_stmp, 1, nickname, -1, NULL);
	sqlite3_bind_int(insert_stmp, 2, score);
	sqlite3_bind_int(insert_stmp, 3, score);
	insert_rc = sqlite3_step(insert_stmp);
	
	if (insert_rc == SQLITE_CONSTRAINT) {
		sqlite3_bind_int(update_stmp, 1, score);
		sqlite3_bind_int(update_stmp, 2, score);
		sqlite3_bind_text(update_stmp, 3, nickname, -1, NULL);
		update_rc = sqlite3_step(update_stmp);
		
		if (update_rc != SQLITE_DONE) {
			fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
			return;
		}
	} else if (insert_rc != SQLITE_DONE) {
		fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
		return;
	}
}
