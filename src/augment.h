/*-
 * Copyright (c) 2006 Zsuzsu <little_zsuzsu@hotmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
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
 * $Id: waffects.h 851 2006-04-22 13:34:00Z zsuzsu $
 */

/*
 * Consecrate is the act of augmenting items with gained experience.
 */

#ifndef _AUGMENT_H_
#define _AUGMENT_H_

#define AUGMENT_MIN_LEVEL	LEVEL_HERO
#define AUGMENT_COST_GOLD	5000
#define AUGMENT_BASE_EXP	1000
#define AUGMENT_WAND_EXP_MOD	75
#define AUGMENT_STAFF_EXP_MOD	50

#define AUGMENT_TYPE_NONE	0
#define AUGMENT_TYPE_WEAPON	1
#define AUGMENT_TYPE_FOCUS	2
#define AUGMENT_TYPE_RELIC	3

/* cost in points */
#define AUGMENT_COST_WEAPON_DICENUMBER		5
#define AUGMENT_COST_WEAPON_DICESIZE		1
#define AUGMENT_COST_WEAPON_HATRED		10
#define AUGMENT_COST_WEAPON_POISON		3
#define AUGMENT_COST_WEAPON_SHARP		4
#define AUGMENT_COST_WEAPON_FROST		4
#define AUGMENT_COST_WEAPON_VORPAL		4
#define AUGMENT_COST_WEAPON_SHOCKING		5
#define AUGMENT_COST_WEAPON_HOLY		6
#define AUGMENT_COST_WEAPON_FLAMING		8
#define AUGMENT_COST_WEAPON_VAMPIRIC		8

#define AUGMENT_COST_FOCUS_MASTERY		2
#define AUGMENT_COST_FOCUS_FOCUS		2
#define AUGMENT_COST_FOCUS_MANA			2
#define AUGMENT_COST_FOCUS_POWER		4

/* components necessary for augmentation */
#define AUGMENT_VNUM_DICENUMBER		131
#define AUGMENT_VNUM_DICESIZE		0
#define AUGMENT_VNUM_FLAMING		132
#define AUGMENT_VNUM_FROST		133
#define AUGMENT_VNUM_SHOCKING		134
#define AUGMENT_VNUM_VORPAL		135
#define AUGMENT_VNUM_HOLY		136
#define AUGMENT_VNUM_VAMPIRIC		137
#define AUGMENT_VNUM_SHARP		138

/*
 * augmentation of items
 */
typedef struct obj_augment_data	OBJ_AUGMENT_DATA;

/* this should probably be a union, but it's unlikely your typical
 * mud coder can grok that.
 */
struct obj_augment_data
{
	time_t		consecration_time;
	time_t		last_level;
	int		level;		/* exp level of the item*/
	int		exp_modifier;
	int		exp;
	int		pts;		/* available augment points*/
	int		skill_sn;	/* for focus items */
	int		mastery_level;	/* + to percentage */
	int		focus_level;	/* increased success */
	int		mana_level;	/* reduce mana cost */
	int		power_level;	/* increase level of cast */
	int		dicesize;	/* increase in dicesize */
	int		dicenum;	/* increase in dicenum */
	flag32_t	weapon_flags;	/* weapon flags added */
	int		racial_hatred;	/* extra damage to this race */
};

bool is_augmented (OBJ_DATA *obj);
void do_consecrate (CHAR_DATA *ch, const char *argument);
void obj_augment_free(OBJ_AUGMENT_DATA *poa);
bool fread_augment (OBJ_DATA *obj, FILE *fp, char *word);
void fwrite_augment (OBJ_DATA *obj, FILE *fp);
OBJ_DATA * get_augment_obj(CHAR_DATA *ch);
bool gain_obj_exp (CHAR_DATA *ch, OBJ_DATA *obj, int gain);
int augment_tnl (OBJ_DATA *obj);
void show_augment (CHAR_DATA *ch, OBJ_DATA *obj, BUFFER *output);
bool is_not_my_augment (CHAR_DATA *ch, OBJ_DATA *obj);
OBJ_DATA * augment_unauthorized_use (CHAR_DATA *ch, OBJ_DATA *wield);

#endif
