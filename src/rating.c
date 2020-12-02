/*-
 * Copyright (c) 1998 efdi <efdi@iclub.nsu.ru>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: rating.c 849 2006-04-22 13:08:54Z zsuzsu $
 */

#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "rating.h"
#include "waffects.h"

struct rating_data {
	const char *name;
	int pk_kills;
	int pk_deaths;
};

struct rating_data rating_table[RATING_TABLE_SIZE];

/*
 * Updates player's rating.
 * Should be called every death.
 */
void rating_update(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (IS_NPC(ch) || IS_NPC(victim) || ch == victim || IS_IMMORTAL(ch)
	|| is_waffected(victim, WAFF_ARENA)
	||  (victim->in_room &&
	     IS_SET(victim->in_room->room_flags, ROOM_BATTLE_ARENA)))
		return;

	rating_add_kill(ch->name, ++ch->pcdata->pk_kills);
	rating_add_death(victim->name, ++victim->pcdata->pk_deaths);
}

/*
 * rating_add - add (or replace) `name/pk_kills' to (in) rating_table
 */
void rating_add_kill(const char* name, int pk_kills)
{
	int i;
	struct rating_data *p = rating_table;

	for (i = 0; i < RATING_TABLE_SIZE; i++) {
		if (rating_table[i].name != NULL
		&&  str_cmp(name, rating_table[i].name) == 0) {
			rating_table[i].pk_kills = pk_kills;
			return;
		}
		if (rating_table[i].pk_kills < p->pk_kills)
			p = rating_table + i;
	}

	if (p->pk_kills < pk_kills) {
		if (p->name != NULL)
			free_string(p->name);
		p->name = str_dup(name);
		p->pk_kills = pk_kills;
	} 	
}

void rating_add_death(const char* name, int pk_deaths)
{
	int i;
	struct rating_data *p = rating_table;

	for (i = 0; i < RATING_TABLE_SIZE; i++) {
		if (rating_table[i].name != NULL
		&&  str_cmp(name, rating_table[i].name) == 0) {
			rating_table[i].pk_deaths = pk_deaths;
			return;
		}
		if (rating_table[i].pk_deaths < p->pk_deaths)
			p = rating_table + i;
	}

	if (p->pk_deaths < pk_deaths) {
		if (p->name != NULL)
			free_string(p->name);
		p->name = str_dup(name);
		p->pk_deaths = pk_deaths;
	} 	
}

static
int
rating_data_cmp(const void *a, const void *b)
{
	return ((struct rating_data*) b)->pk_kills 
		- ((struct rating_data*) a)->pk_kills;
}

void do_rating(CHAR_DATA *ch, const char *argument)
{
	int i;

	qsort(rating_table, RATING_TABLE_SIZE, sizeof(struct rating_data),
	      rating_data_cmp);

	char_puts("Name                    | PC's killed\n", ch);
	char_puts("------------------------+------------\n", ch);
	for (i = 0; i < RATING_TABLE_SIZE; i++) {
		if (rating_table[i].name == NULL)
			continue;
		char_printf(ch, "%-24s| %d\n",
			    rating_table[i].name, rating_table[i].pk_kills);
	}
	if (!ch->pcdata->pk_kills)
		char_puts("\nDo you profess to be SO {Cpeaceful{x?\n"
			     "You have killed no one.\n", ch);
	else {
		if (!strcmp(rating_table[0].name, ch->name))
			char_puts("\nI bet you are {Rawful{x. \n"
				     "You're at the top of this list!", ch);
		char_printf(ch, "\nYou have killed %s{R%d{x player%s.\n",
			    ch->pcdata->pk_kills == 1 ? "ONLY " : str_empty,
			    ch->pcdata->pk_kills, 
			    ch->pcdata->pk_kills == 1 ? str_empty : "s");
	}
}

