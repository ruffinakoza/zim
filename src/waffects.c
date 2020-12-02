/*-
 * Copyright (c) 2005 Zsuzsu <little_zsuzsu@hotmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
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
 * $Id: waffects.c 1019 2007-02-15 00:52:41Z zsuzsu $
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "const.h"
#include "db/db.h"
#include "waffects.h"

static void show_waffects (CHAR_DATA *ch, BUFFER *output, 
			AREA_DATA *area, bool show_all);
static bool can_see_waffect (CHAR_DATA *ch, WORLD_AFFECT_DATA *waff);
void affect_notify_area (WORLD_AFFECT_DATA *paf);
void affect_notify_world (WORLD_AFFECT_DATA *paf);
void fwrite_waff (FILE *fp, WORLD_AFFECT_DATA *waff);
WORLD_AFFECT_DATA * load_waff(FILE *fp);

/*
 * global
 */
WORLD_AFFECT_DATA *	world_affect_list;

/* functions */
bool is_affected_area (int sn, AREA_DATA *area)
{
	WORLD_AFFECT_DATA *paf;
	for (paf = world_affect_list; paf; paf = paf->next)
		if (paf->type == sn 
		&& paf->active
		&& area == paf->area)
			return TRUE;
	return FALSE;
}

bool is_affected_world (int sn)
{
	return is_affected_area(sn, NULL);
}

bool is_waffected (CHAR_DATA *ch, int sn)
{
	return ch_waffected(ch, sn) != NULL;
}

/*
 * return the affect on this character
 * returns NULL if there is no affect
 *
 * notice, affects do not compound and area affects override
 * world affects.
 */
WORLD_AFFECT_DATA * ch_waffected (CHAR_DATA *ch, int sn)
{
	WORLD_AFFECT_DATA *waf;

	if (!ch || !ch->in_room || !ch->in_room->area)
		return NULL;

	for (waf = world_affect_list; waf; waf = waf->next) {
		if (waf->type == sn 
		&& waf->active
		&& ch->in_room->area == waf->area
		&& ch->level >= waf->min_level
		&& ch->level <= waf->max_level)
			break;
	}

	if (!waf)
		for (waf = world_affect_list; waf; waf = waf->next) {
			if (waf->type == sn 
			&& waf->active
			&& ch->level >= waf->min_level
			&& ch->level <= waf->max_level)
				break;
		}

	return waf;
}

WORLD_AFFECT_DATA * affect_find_area (int sn, AREA_DATA *area)
{
	WORLD_AFFECT_DATA *waf;
	for (waf = world_affect_list; waf; waf = waf->next)
		if (waf->type == sn && area == waf->area)
			return waf;
	return NULL;
}

WORLD_AFFECT_DATA * affect_find_world (int sn)
{
	return affect_find_area(sn, NULL);
}

void affect_to_area (WORLD_AFFECT_DATA *paf) 
{
	affect_to_world(paf);
}

void affect_to_world (WORLD_AFFECT_DATA *paf)
{
	WORLD_AFFECT_DATA *waf_new;

	waf_new = waff_new();
	*waf_new = *paf;
	waf_new->next = world_affect_list;
	world_affect_list = waf_new;

	if (paf->active == TRUE)
		affect_notify_world(paf);
}

void affect_notify_area (WORLD_AFFECT_DATA *paf)
{
	affect_notify_world(paf);
}

void affect_notify_world (WORLD_AFFECT_DATA *paf)
{
	DESCRIPTOR_DATA *d;

	if (IS_SET(paf->notify, WAFF_NOTIFY_OFF)) {
		for (d = descriptor_list; d; d = d->next) {
			if (d->connected == CON_PLAYING
			&& d->character && d->character->in_room
			&& (!paf->area || d->character->in_room->area == paf->area)
			&& ((d->character->level >= paf->min_level
			&& d->character->level <= paf->max_level)
			|| IS_IMMORTAL(d->character))) {
				act_puts(flag_string(
					(paf->active) ? waff_mortal_on
					: waff_mortal_off, paf->type),
				NULL, NULL, d->character, TO_VICT, POS_DEAD);
			}
		}
	}
}

void affect_remove_area (WORLD_AFFECT_DATA *paf) {
	affect_remove_world(paf);
}

void affect_remove_world (WORLD_AFFECT_DATA *paf)
{
	WORLD_AFFECT_DATA *prev;

	if (world_affect_list == NULL) {
		BUG("affect_remove_world: tried to remove affect"
		    " from NULL affected world");
		waff_free(paf);
		return;
	}

	if (paf == world_affect_list) {
		world_affect_list = paf->next;
	}
	else {
		for (prev = world_affect_list; prev; prev = prev->next) {
			if (prev->next == paf) {
				prev->next = paf->next;
				break;
			}
		}

		if (prev == NULL) {
			BUG("affect_remove_world: tried to remove affect"
			    " from world but couldn't find affect.");
			waff_free(paf);
			return;
		}
	}

	paf->active = FALSE;
	affect_notify_world(paf);

	waff_free(paf);
}

void affect_strip_world (int sn)
{
	WORLD_AFFECT_DATA *paf;
	WORLD_AFFECT_DATA *paf_next;

	for (paf = world_affect_list; paf; paf = paf_next) {
		paf_next = paf->next;
		if (paf->type == sn)
			affect_remove_world(paf);
	}
}

void affect_strip_area (int sn, AREA_DATA *area)
{
	WORLD_AFFECT_DATA *paf;
	WORLD_AFFECT_DATA *paf_next;

	for (paf = world_affect_list; paf; paf = paf_next) {
		paf_next = paf->next;
		if (paf->type == sn && area == paf->area)
			affect_remove_world(paf);
	}
}


/*
 * add a new affect, or augment an old one.
 */
void affect_join_world (WORLD_AFFECT_DATA *paf)
{
	WORLD_AFFECT_DATA *paf_old;

	for (paf_old = world_affect_list; paf_old; paf_old = paf_old->next) {
		if (paf_old->type == paf->type && paf_old->area == paf->area) {
		    paf->modifier += paf_old->modifier;
		    paf->duration += paf_old->duration;
		    affect_remove_world(paf_old);
		    break;
		}
	}

	affect_to_world(paf);
}


/*
 * World Set - to apply waffects
 *
 * wset add <affect> duration <n> modifier <n> chance <n> area <n> notify 
 * wset del <affect> <notify>
 */
DO_FUN(do_wset)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char helpname[] = "'WIZ WSET'";
	WORLD_AFFECT_DATA waff;
	AREA_DATA *area	= NULL;
	bool is_add = FALSE;
	int  type	= 0,
	     modifier	= 0,
	     duration	= -1,
	     chance	= 100,
	     min_level	= 0,
	     max_level	= LEVEL_HERO,
	     visible_level = LEVEL_IMMORTAL,
	     level	= ch->level,
	     count	= 0,
	     repeat	= WAFF_REPEAT_NONE,
	     interval	= 0,
	     start_hour = 0;
	bool notify	= FALSE,
	     active	= TRUE;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0' || arg2[0] == '\0')  {
		do_help(ch, helpname);
		return;
	}

	if (!str_prefix(arg1, "add"))
		is_add = TRUE;

	else if (!str_prefix(arg1, "remove")
	|| !str_prefix(arg1, "delete"))
		is_add = FALSE;

	else {
		do_wset(ch, str_empty);
		return;
	}

	if ((type = flag_value(waff_types, arg2)) <= 0) {
		char_printf(ch, "choose an affect:\n");
		show_flags(ch, waff_types);
		return;
	}

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	while (arg1[0] != '\0') {
		if (arg2[0] == '\0')
			argument = one_argument(argument, arg2, sizeof(arg2));

		if (!str_prefix(arg1, "modifier")
		|| !str_prefix(arg1, "modify")) {
			if (arg2[0] == '\0') {
				do_help(ch, helpname);
				return;
			}
			if (!str_cmp(arg2, "on"))
				modifier = 1;
			else if (!str_cmp(arg2, "off"))
				modifier = -1;
			else
				modifier = atoi(arg2);
		}
		else if (!str_prefix(arg1, "duration")) {
			if (arg2[0] == '\0') {
				do_help(ch, helpname);
				return;
			}
			if (!str_prefix(arg2, "permanent"))
				duration = -1;
			else
				duration = atoi(arg2);
		}
		else if (!str_prefix(arg1, "chance")) {
			if (arg2[0] == '\0') {
				do_help(ch, helpname);
				return;
			}
			chance = atoi(arg2);
		}
		else if (!str_prefix(arg1, "visible_level")) {
			if (arg2[0] == '\0') {
				do_help(ch, helpname);
				return;
			}
			visible_level = atoi(arg2);
		}
		else if (!str_prefix(arg1, "min_level")) {
			if (arg2[0] == '\0') {
				do_help(ch, helpname);
				return;
			}
			min_level = atoi(arg2);
		}
		else if (!str_prefix(arg1, "max_level")) {
			if (arg2[0] == '\0') {
				do_help(ch, helpname);
				return;
			}
			max_level = atoi(arg2);
		}
		else if (!str_prefix(arg1, "level")) {
			if (arg2[0] == '\0') {
				do_help(ch, helpname);
				return;
			}
			level = atoi(arg2);
		}
		else if (!str_prefix(arg1, "area")) {
			if (arg2[0] == '\0') {
				do_help(ch, helpname);
				return;
			}
			if (arg2[0] == '\0' && (!is_number(arg2)
			|| (area = area_lookup(atoi(arg2))) == NULL)) {
				area = ch->in_room->area;
				strnzcpy(arg1, sizeof(arg1), arg2);
				arg2[0] = '\0';
				continue;
			}
		}
		else if (!str_prefix(arg1, "notify")) {
			if (arg2[0] == '\0'
			|| !str_prefix(arg2, "both")
			|| !str_cmp(arg2, "all")
			|| !str_cmp(arg2, "true")) {
				notify = WAFF_NOTIFY_ON | WAFF_NOTIFY_OFF;
			}
			else if (!str_cmp(arg2, "on")) {
				notify = WAFF_NOTIFY_ON;
			}
			else if (!str_cmp(arg2, "off")) {
				notify = WAFF_NOTIFY_OFF;
			}
			else if (!str_prefix(arg2, "none")) {
				notify = WAFF_NOTIFY_OFF;
			}
		}
		else if (!str_prefix(arg1, "starthour")) {
			if (arg2[0] == '\0') {
				do_help(ch, helpname);
				return;
			}
			start_hour = atoi(arg2);
			active = FALSE;
			if (start_hour < 0 || start_hour > 23) {
				char_printf(ch, 
					"starthour must be between 0 and 23.\n");
				return;
			}
		}
		else if (!str_prefix(arg1, "repeat")) {
			if (is_number(arg2)) {
				repeat = atoi(arg2);
			}
			else {
				repeat = WAFF_REPEAT_PERM;
				strnzcpy(arg1, sizeof(arg1), arg2);
				arg2[0] = '\0';
				continue;
			}
		}
		else if (!str_prefix(arg1, "interval")) {
			if (arg2[0] == '\0' || !is_number(arg2)) {
				do_help(ch, helpname);
				return;
			}
			interval = atoi(arg2);
		}


		arg2[0] = '\0';
		argument = one_argument(argument, arg1, sizeof(arg1));
	}

	if (!is_add) {
		WORLD_AFFECT_DATA *paff;

		while (!count && (paff = affect_find_area(type, area)) != NULL) {
			if (paff->level <= ch->level
			|| ch->level == ML
			|| !str_cmp(ch->name, paff->player_name)) {
				count++;
				affect_remove_world(paff);
			}
			else {
				char_puts("Sorry, you're not high enough"
					  " level to do that.\n", ch);
				break;
			}

		}
		if (count > 0) {
			wiznet("$N has removed $t mode from the world.", 
				ch, flag_string(waff_types, type), 
				WIZ_ANNOUNCE, 0, 0);
			char_printf(ch, "Removed %s.", 
				flag_string(waff_types, type));
		}
		else {
			char_puts("No affects removed.", ch);
		}
	}
	else {
		if (level > ch->level) {
			char_printf(ch, "The level on this world affect"
				" is too high.  Your max is %d.",
				ch->level);
			return;
		}
		waff.player		= ch;
		waff.player_name	= ch->name;
		waff.level		= level;
		waff.type		= type;
		waff.interval		= interval;
		waff.duration		= duration;
		waff.modifier		= modifier;
		waff.chance		= chance;
		waff.min_level		= min_level;
		waff.max_level		= max_level;
		waff.visible_level	= visible_level;
		waff.notify		= notify;
		waff.area		= area;
		waff.active		= active;
		waff.start_hour		= start_hour;
		waff.repeat		= repeat;

		/* if there's a start hour don't start immediately */
		if (active)
			waff.timer = duration;
		else
			waff.timer = -1;

		affect_to_world(&waff);
		char_printf(ch, "Added %s.", flag_string(waff_types, type));
		wiznet("$N has added $t mode to the world.", 
			ch, flag_string(waff_types, type), 
			WIZ_ANNOUNCE, 0, 0);
	}
}

/*
 * list world affects
 */
void do_waff(CHAR_DATA *ch, const char *argument)
{
	BUFFER *output;
	char arg[MAX_INPUT_LENGTH];

	argument = first_arg(argument, arg, sizeof(arg), FALSE);

	output = buf_new(ch->lang);

	if (arg[0] == '\0')
		show_waffects(ch, output, NULL, TRUE);
	else if (!str_prefix(arg, "world"))
		show_waffects(ch, output, NULL, FALSE);
	else if (!str_prefix(arg, "area")) {
		if (!ch->in_room)
			BUG("do_waff: character requested area, but not in room.");
		else
			show_waffects(ch, output, ch->in_room->area, FALSE);
	}

	page_to_char(buf_string(output), ch);

	buf_free(output);
}

static void show_waffects (CHAR_DATA *ch, BUFFER *output, 
			AREA_DATA *area, bool show_all)
{
	WORLD_AFFECT_DATA *waf;

	buf_printf(output, "%s is affected by the following:\n",
		(!area) ? "The World" : area->name);

	for (waf = world_affect_list; waf; waf = waf->next)
		if (show_all 
		|| (!show_all && area == waf->area)) {
			if (can_see_waffect(ch, waf)) {
				if (IS_IMMORTAL(ch))
					buf_printf(output, "{%c '{c%s{z'"
						" modified by {c%d{z"
						"  timer {c%d{z"
						"  chance {c%d{z%%"
						"  levels {c%d{z-{c%d{z"
						"\n         "
						"  duration {c%d{z"
						"  interval {c%d{z"
						"  repeat {c%d{z"
						"  hour {c%d{z"
						"\n         "
						"  notify {c%s{z"
						"  visby {c%d{z"
						"  immlevel {c%d{z"
						"  by {W%s{x\n"
						"%s%s%s",
						(waf->active) ? 'x' : 'D',
						flag_string(waff_types, waf->type),
						waf->modifier,
						waf->timer,
						waf->chance,
						waf->min_level,
						waf->max_level,

						waf->duration,
						waf->interval,
						waf->repeat,
						waf->start_hour,

			IS_SET(waf->notify, WAFF_NOTIFY_ON)
			&& IS_SET(waf->notify,  WAFF_NOTIFY_OFF) ? "on/off"
			: IS_SET(waf->notify, WAFF_NOTIFY_ON) ? "on" 
			: IS_SET(waf->notify, WAFF_NOTIFY_OFF) ? "off"
			: "none", 
						waf->visible_level,
						waf->level,
						waf->player_name,
						(waf->area) ?  "           area {c" : "",
						(waf->area) ? waf->area->name : "",
						(waf->area) ? "{x\n" : "");
				else
					buf_printf(output, "[%s] %s\n",
						(!waf->area) ? "{gworld{x" : "{yarea{x",
						flag_string(waff_mortal_on, waf->type));
			}
		}
}

static bool can_see_waffect (CHAR_DATA *ch, WORLD_AFFECT_DATA *waff)
{
	if (!ch || !ch->in_room || !waff)
		return FALSE;

	if (ch->level < waff->visible_level)
		return FALSE;

	/* mortals only see area affects if they're in the area*/
	if (!IS_IMMORTAL(ch)
	&& waff->active
	&& waff->area
	&& ch->in_room->area != waff->area)
		return FALSE;

	if (!IS_IMMORTAL(ch)
	&& waff->active
	&& (waff->min_level > ch->level
	|| waff->max_level < ch->level))
		return FALSE;

	return TRUE;
}

/*
 * world affect update, called by update.c
 */
void world_affect_update (void)
{
	WORLD_AFFECT_DATA *waf;
	WORLD_AFFECT_DATA *waf_next;
	struct tm *ct = NULL;
	int hour = 0;

	ct = localtime(&current_time);

	hour = ct->tm_hour;

	for (waf = world_affect_list; waf; waf = waf_next) {
		waf_next = waf->next;

		if (waf->active) {
			if (waf->duration < 0)
				continue;

			if (waf->timer == 0) {
				waf->active = FALSE;
				if (waf->repeat < 1) {
					affect_remove_world(waf);
				}
				else {
					affect_notify_world(waf);
					waf->repeat--;
					waf->timer = (waf->interval) 
						? waf->interval : -1;
				}
			}
			else
				waf->timer--;
		}
		else {
			/* if the timer is off, start it in the hour */
			if (waf->timer < 0
			&& waf->start_hour
			&& waf->start_hour == hour) {
				if (waf->interval)
					waf->timer = waf->interval;
				else {
					waf->active = TRUE;
					waf->timer = waf->duration;
					affect_notify_world(waf);
				}

			}
			else if (waf->timer == 0) {
				waf->active = TRUE;
				waf->timer = waf->duration;
				affect_notify_world(waf);
			}
			else
				waf->timer--;
		}
	}
}

/*
 * world affect constructors and deconstructors.
 */
WORLD_AFFECT_DATA *waff_new(void)
{
	WORLD_AFFECT_DATA *waff = NULL;
	waff = calloc(1, sizeof(WORLD_AFFECT_DATA));
	waff->start_hour = -1;

	return waff;
}               
        
WORLD_AFFECT_DATA *waff_dup(const WORLD_AFFECT_DATA *paf)
{               
	WORLD_AFFECT_DATA *naf = waff_new();
	naf->player = paf->player;

	return naf;
}
        
void waff_free(WORLD_AFFECT_DATA *paf)
{       
	paf->next = NULL;
	free(paf);
}

/*
 * tables
 */

flag_t waff_types[] =
{
	{ "",			TABLE_INTVAL,			},

	{ "none",		WAFF_NONE,		FALSE	},
	{ "arena",		WAFF_ARENA,		TRUE	},
	{ "auction",		WAFF_AUCTION,		TRUE	},
	{ "peace",		WAFF_PEACE,		TRUE	},
	{ "exp",		WAFF_EXP,		TRUE	},
	{ "qp",			WAFF_QP,		TRUE	},
	{ "gold",		WAFF_GOLD,		TRUE	},
	{ "pvp_damage",		WAFF_PVP_DAMAGE,	TRUE	},
	{ "pvm_damage",		WAFF_PVM_DAMAGE,	TRUE	},
	{ "inflation",		WAFF_INFLATION,		TRUE	},
	{ "ffa",		WAFF_FFA,		TRUE	},
	{ "pk_range",		WAFF_PK_RANGE,		TRUE	},
	{ NULL }
};

flag_t waff_mortal_on[] =
{
	{ "",			TABLE_INTVAL,			},

	{ "none",
		WAFF_NONE,		FALSE	},
	{ "{WYou feel your death, at the hands of a mortal, might have no meaning.{x",
		WAFF_ARENA,		TRUE	},
	{ "{WThe auctioneer is available, and ready for your business.{x",
		WAFF_AUCTION,		TRUE	},
	{ "{WPeace and tranquility fill the realm bringing an end to violence.{x",
		WAFF_PEACE,		TRUE	},
	{ "{WYou seem blessed with greater insight into your combat experiences.{x",
		WAFF_EXP,		TRUE	},
	{ "{WRumors have it that Questus is desperate for help.{x",
		WAFF_QP,		TRUE	},
	{ "{WYour palm itches.{x",
		WAFF_GOLD,		TRUE	},
	{ "{WYou overhear a shopkeeper grumbling about missing shipments.{x",
		WAFF_INFLATION,		TRUE	},
	{ "{WYour soul feels resilient to mortal attacks.{x",
		WAFF_PVP_DAMAGE,	TRUE	},
	{ "{WLesser souls fear your wrath.{x",
		WAFF_PVM_DAMAGE,	TRUE	},
	{ "The world is rife with {rmurderous intent{x.",
		WAFF_FFA,		TRUE	},
	{ "The world seems very {rdangerous{x all of a sudden!",
		WAFF_PK_RANGE,		TRUE	},
	{ NULL }
};

flag_t waff_mortal_off[] =
{
	{ "",			TABLE_INTVAL,			},

	{ "none",
		WAFF_NONE,		FALSE	},
	{ "{WThe gods will no longer spare you the effects of mortal death.{x",
		WAFF_ARENA,		TRUE	},
	{ "{WThe auctioneer is no longer doing business in the realm.{x",
		WAFF_AUCTION,		TRUE	},
	{ "{WTurmoil and strife return to the world.{x",
		WAFF_PEACE,		TRUE	},
	{ "{WYour enhanced insight into combat experience fades.{x",
		WAFF_EXP,		TRUE	},
	{ "{WRumor is that Questus is no longer hard-up for help.{x",
		WAFF_QP,		TRUE	},
	{ "{WThe itch in your palm subsides.{x",
		WAFF_GOLD,		TRUE	},
	{ "{WYou notice shops seem to have fuller shelves again.{x",
		WAFF_INFLATION,		TRUE	},
	{ "{WYour soul feels more vulnerable to mortal attacks.{x",
		WAFF_PVP_DAMAGE,	TRUE	},
	{ "{WLesser souls steel themselves against your wrath.{x",
		WAFF_PVM_DAMAGE,	TRUE	},
	{ "The world is no longer rife with {rmurderous intent{x.",
		WAFF_FFA,		TRUE	},
	{ "The world seems less {rdangerous{x.",
		WAFF_PK_RANGE,		TRUE	},
	{ NULL }
};

#define END_TAG "#END\n"

void do_save_waffs (CHAR_DATA *ch, const char *argument)
{

	if (!save_waffs())
		char_printf(ch, "World Affects not saved, for some reason.\n");
	else
		char_printf(ch, "World Affects saved.\n");
}

bool save_waffs ()
{
	WORLD_AFFECT_DATA *waff = world_affect_list;
	FILE *fp = NULL;
	int size;

	if ((fp = dfopen(ETC_PATH, TMP_FILE, "w")) == NULL)
		return FALSE;

	fprintf(fp, "#WorldAffectsFor %s\n", strtime(current_time));
	fprintf(fp, "WorldEpoc %ld\n\n", current_time);

	while (waff) {
		fwrite_waff(fp, waff);
		waff = waff->next;
	}

        size = fprintf(fp, END_TAG);
	fclose(fp);

	if (size != strlen(END_TAG))
		return FALSE;

	d2rename(ETC_PATH, TMP_FILE, ETC_PATH, WAFF_STATE);
	return TRUE;
}

void fwrite_waff (FILE *fp, WORLD_AFFECT_DATA *waff)
{
	fprintf(fp, "#WORLD_AFFECT\n");
	fwrite_string(fp, "Player", waff->player_name);
	fprintf(fp, "Level %d\n", waff->level);
	fprintf(fp, "Type %s~\n", flag_string(waff_types, waff->type));
	fprintf(fp, "Modifier %d\n", waff->modifier);
	fprintf(fp, "Duration %d\n", waff->duration);
	fprintf(fp, "Interval %d\n", waff->interval);
	fprintf(fp, "Timer %d\n", waff->timer);
	fprintf(fp, "Chance %d\n", waff->chance);
	fprintf(fp, "MinLevel %d\n", waff->min_level);
	fprintf(fp, "MaxLevel %d\n", waff->max_level);
	fprintf(fp, "VisLevel %d\n", waff->visible_level);
	fprintf(fp, "StartHour %d\n", waff->start_hour);
	fprintf(fp, "Repeat %d\n", waff->repeat);
	fprintf(fp, "Active %d\n", waff->active);
	fprintf(fp, "Notify %s\n", format_flags(waff->notify));
	if (waff->area) {
		fwrite_string(fp, "AreaName", waff->area->name);
		fprintf(fp, "AreaMinVnum %d\n", waff->area->min_vnum);
	}
	fprintf(fp, "End\n\n");
}

int load_waffs ()
{
	WORLD_AFFECT_DATA *waff = world_affect_list;
	FILE	*fp    = NULL;
	char    *word  = "#END";
	bool    fMatch = FALSE;
	long    old_time = 0L;
	int	count = 0;

	Line_Number = 0;

	if (!dfexist(ETC_PATH, WAFF_STATE)
	||  (fp = dfopen(ETC_PATH, WAFF_STATE, "r")) == NULL)
		return FALSE;

	while (TRUE) {
                word = feof(fp) ? "#END" : fread_word(fp);
		fMatch = FALSE;

		switch (UPPER(word[0])) {
		case '#':
			if (!str_cmp(word, "#WORLD_AFFECT")) {
				waff = load_waff(fp);
				if (waff) {
					if (world_affect_list) {
						waff->next = world_affect_list;
						world_affect_list = waff;
						count++;
					}
					else {
						world_affect_list = waff;
						count++;
					}
				}
				fMatch = TRUE;
			}
			if (!str_cmp(word, "#END")) {
				return count;
			}
			else {
				fMatch = TRUE;
				fread_to_eol(fp);
			}
			break;
		case 'W':
			KEY("WorldEpoc", old_time, fread_number(fp));
			break;
		}
		if (!fMatch) {
			LOG("load_waffs: '%s' no match (%dth byte?) line %d",
				word, ftell(fp), Line_Number);
			fread_to_eol(fp);
		}
	}

	fclose(fp);

	/* TODO - update all durations with current_time vs old_time */
}

WORLD_AFFECT_DATA * load_waff(FILE *fp)
{
	char    *word = "End";
	bool    fMatch = FALSE;
	int	sanity = 0;
	WORLD_AFFECT_DATA *waff = NULL;

	waff = waff_new();

	while (sanity++ < 100) {
                word = feof(fp) ? "End" : fread_word(fp);
		fMatch = FALSE;

		switch (UPPER(word[0])) {
		case '#':
			fMatch = TRUE;
			fread_to_eol(fp);
			break;

		case 'A':
			KEY("Active", waff->active, fread_number(fp));
			break;

		case 'C':
			KEY("Chance", waff->chance, fread_number(fp));
			break;

		case 'D':
			KEY("Duration", waff->duration, fread_number(fp));
			break;

		case 'E':
			if (!str_cmp(word, "End"))
				return waff;
			break;

		case 'I':
			KEY("Interval", waff->interval, fread_number(fp));
			break;

		case 'L':
			KEY("Level", waff->level, fread_number(fp));
			break;

		case 'M':
			KEY("MaxLevel", waff->max_level, fread_number(fp));
			KEY("MinLevel", waff->min_level, fread_number(fp));
			KEY("Modifier", waff->modifier, fread_number(fp));
			break;

		case 'N':
			KEY("Notify", waff->notify, fread_flags(fp));
			break;

		case 'P':
			KEY("Player", waff->player_name, fread_string(fp));
			break;

		case 'R':
			KEY("Repeat", waff->repeat, fread_number(fp));
			break;

		case 'S':
			KEY("StartHour", waff->start_hour, fread_number(fp));
			break;

		case 'T':
			KEY("Timer", waff->timer, fread_number(fp));
			KEY("Type", waff->type, fread_fstring(waff_types, fp));
			break;

		case 'V':
			KEY("VisLevel", waff->visible_level, fread_number(fp));
			break;
		}

		if (!fMatch) {
			LOG("load_waff: %s: no match (%dth byte?)",
				word, ftell(fp));
			fread_to_eol(fp);
		}
	}

	LOG("load_waff: couldn't find End -- so freeing waff");
	waff_free(waff);
	waff = NULL;
	return NULL;
}
