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
 * $Id: waffects.c 917 2006-10-13 22:06:33Z zsuzsu $
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "stats.h"
#include "debug.h"

/**
 * these are all for stats based on 100, instead of 25.
 * There is no upper bounds on the stats.
 */

/*
 * keeping track of stats for every kill, that way you can't
 * just dawn EQ at the end of the last batter for the boost
 */
int ch_stat_avg(CHAR_DATA *ch, int stat)
{
	if (IS_NPC(ch)) return get_curr_stat(ch, stat);

	if (ch->pcdata->stat_avg[stat][STAT_AVG_NUM] <= 0)
		return get_curr_stat(ch, stat);

	return ch->pcdata->stat_avg[stat][STAT_AVG_TOTAL] /
		ch->pcdata->stat_avg[stat][STAT_AVG_NUM];
}

/*
 * char's damroll is affected by strength
 * and perm_dam.
 */
int ch_damroll (CHAR_DATA *ch)
{
	int todam = 0;
	if (IS_NPC(ch))
		todam = ch->damroll;
	else {
		todam = ch->damroll + ch->pcdata->perm_dam;
		todam = todam * get_stat_str_todam_mod(ch)/100;
	}
	return todam;
}

int ch_hitroll (CHAR_DATA *ch)
{
	int tohit = 0;

	if (IS_NPC(ch))
		tohit = ch->hitroll;
	else {
		tohit = ch->hitroll;
	}
	return tohit;
}

/*
 * in tenth of pounds.
 */
int ch_max_carry_weight(CHAR_DATA *ch)
{
	int tenth_lbs = get_stat_str_max_carry_weight(ch);

	return tenth_lbs;
}

int get_stat_str_tohit_mod (CHAR_DATA *ch, OBJ_DATA *wield)
{
	return 100;
}


/*
 * mod the damroll.  Str 50 will be averge (100% damage).
 */
int get_stat_str_todam_mod (CHAR_DATA *ch)
{
	int str = get_curr_stat(ch, STAT_STR);

	return UMAX(0, 100 + (2*(str - 50)));
}

/*
 * in tenth of pounds.
 */
int get_stat_str_max_carry_weight (CHAR_DATA *ch)
{
	int weight = 0;

	if (IS_NPC(ch))
		weight = get_curr_stat(ch, STAT_STR) * 10 + ch->level * 25;
	else if (IS_IMMORTAL(ch))
		weight = 1000000;
	else {
		weight = ch->pcdata->trend_stat[STAT_STR] / 4
			+ get_curr_stat(ch, STAT_STR) * 50;
	}

	return weight;
}

/*
 * return the cost multiplier for costs of items from shops.
 */
int get_stat_cha_cost_mod (CHAR_DATA *ch)
{
	int mod = 100;

	mod -= (get_curr_stat(ch, STAT_CHA) - 50) *3;

	return UMAX(50, mod);
}

int get_stat_cha_sell_mod (CHAR_DATA *ch)
{
	int mod = 100;

	mod -= (get_curr_stat(ch, STAT_CHA) - 50) *4;

	return URANGE(20, mod, 130);
}

/* in pounds
 */
int get_stat_str_max_wield_weight (CHAR_DATA *ch, int hand)
{
	int str = get_curr_stat(ch, STAT_STR);
	int max = 0;

	switch (hand) {
	case HAND_PRIMARY:	max = str*10 / 2; break;
	case HAND_SECONDARY:	max = str*10 / 3; break;
	case HAND_BOTH:		max = str*10 * 3/4; break;
	default:
		BUG("unknown hand specified in max_wield_weight");
	}

	return max;
}

bool get_stat_str_can_wield (CHAR_DATA *ch, OBJ_DATA *wield, int hand)
{
	DEBUG(DEBUG_BUG, "str_can_wield: %s %d(%d) >= %s[%d] (%d)",
		ch->name,
		get_curr_stat(ch, STAT_STR),
		get_stat_str_max_wield_weight(ch, hand),
		wield->name,
		wield->pIndexData->vnum,
		get_obj_weight(wield));
	return get_stat_str_max_wield_weight(ch, hand) >= get_obj_weight(wield);
}

bool get_stat_str_can_wear (CHAR_DATA *ch, OBJ_DATA *obj)
{
	return TRUE;
}

int ch_mana_cost_mod (CHAR_DATA *ch)
{
	return get_stat_wis_mana_cost_mod(ch);
}

int get_stat_wis_mana_cost_mod (CHAR_DATA *ch)
{
	int mana_cost = 100;
	int wis = get_curr_stat(ch, STAT_WIS);
	
	if (wis > 50)
	   mana_cost = mana_cost - (wis - 50)*110/100;
	else
	   mana_cost = mana_cost + (50 - wis)*3;

	return UMAX(30, mana_cost);
}

int ch_learn_rate (CHAR_DATA *ch)
{
	return get_stat_int_learn_rate(ch);
}

int get_stat_int_learn_rate (CHAR_DATA *ch)
{
	int intel = get_curr_stat(ch, STAT_INT);

	return ((intel - (100 - intel)/5 ) * 3/4);
}

int ch_practice_per_level (CHAR_DATA *ch)
{
	return get_stat_wis_practice_per_level(ch);
}

int get_stat_wis_practice_per_level (CHAR_DATA *ch)
{
	int wis = get_curr_stat(ch, STAT_WIS);
	int prac = 0;

	if (wis >= 110) prac = 6;
	else if (wis >= 95) prac = 5;
	else if (wis >= 80) prac = 4;
	else if (wis >= 60) prac = 3;
	else if (wis >= 40) prac = 2;
	else if (wis >= 20) prac = 1;
	else wis = 0;

	return prac;
}

int get_stat_dex_defense (CHAR_DATA *ch)
{
	if (IS_NPC(ch))
		return -4 * ch->level;
	else
		return -1 * ch->pcdata->perm_nim;
}
