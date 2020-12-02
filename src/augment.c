/*-
 * Copyright (c) 2006 Zsuzsu <little_zsuzsu@hotmail.com>
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
 * $Id: waffects.c 877 2006-06-26 02:16:49Z zsuzsu $
 */

/*
 * Augmented weapons, those made my hero level characters to be ueber
 * weapons and focus items (which enhance skills/spells).  These are
 * items that gain experience, and power as the hero uses them.  They
 * are meant to be loot/sac-able so that PK should keep them relatively
 * non ultra-powerful.
 *
 * by Zsuzsu
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "debug.h"
#include "db/db.h"
#include "augment.h"

flag_t augment_type[] = 
{
        { "",			TABLE_BITVAL			},

        { "weapon",		AUGMENT_TYPE_WEAPON,	TRUE	},
        { "focus",		AUGMENT_TYPE_FOCUS,	TRUE	},
        { "relic",		AUGMENT_TYPE_RELIC,	TRUE	},

        { NULL }
};

typedef struct augmentation_data AUGMENTATION_DATA;
struct augmentation_data {
	const char 	*name;
	flag32_t	type;
	int		component_vnum;
	int		gp_cost_per_level;
	int		(*augment_cost)(CHAR_DATA *ch, OBJ_DATA *obj);
	bool		(*augment_buy)(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *component, const char *argument);
};


static int augment_cost_dicenumber (CHAR_DATA *ch, OBJ_DATA *obj);
static bool augment_buy_dicenumber (CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *component, const char *argument);
static int augment_cost_dicesize (CHAR_DATA *ch, OBJ_DATA *obj);
static bool augment_buy_dicesize (CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *component, const char *argument);
static int augment_cost_holy (CHAR_DATA *ch, OBJ_DATA *obj);
static bool augment_buy_holy(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *component, const char *argument);
static int augment_cost_flaming (CHAR_DATA *ch, OBJ_DATA *obj);
static bool augment_buy_flaming(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *component, const char *argument);
static int augment_cost_hatred (CHAR_DATA *ch, OBJ_DATA *obj);
static bool augment_buy_hatred(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *component, const char *argument);

AUGMENTATION_DATA augmentation_table[] = {

	{ "dicenumber",		AUGMENT_TYPE_WEAPON,
	AUGMENT_VNUM_DICENUMBER,
	1000,
	augment_cost_dicenumber,	
	augment_buy_dicenumber
	},

	{ "dicesize",		AUGMENT_TYPE_WEAPON,
	AUGMENT_VNUM_DICESIZE,
	500,
	augment_cost_dicesize,
	augment_buy_dicesize
	},

	{ "holy",		AUGMENT_TYPE_WEAPON,
	AUGMENT_VNUM_HOLY,
	500,
	augment_cost_holy,
	augment_buy_holy
	},

	{ "flaming",		AUGMENT_TYPE_WEAPON,
	AUGMENT_VNUM_FLAMING,
	500,
	augment_cost_flaming,
	augment_buy_flaming
	},

	{ "hatred",		AUGMENT_TYPE_WEAPON,
	0,
	4000,
	augment_cost_hatred,
	augment_buy_hatred
	},

	{ NULL }
};

void show_augment_menu (CHAR_DATA *ch, OBJ_DATA *obj);
void show_augment_focus_menu (CHAR_DATA *ch, OBJ_DATA *obj);
void show_augment_weapon_menu(CHAR_DATA *ch, OBJ_DATA *obj);
void do_consecrate_commands(CHAR_DATA *ch);
void do_consecrate_ritual (CHAR_DATA *ch, OBJ_DATA *obj, const char *argument);
void do_consecrate_buy (CHAR_DATA *ch, OBJ_DATA *obj, const char *argument);

int get_augment_exp_modifier(CHAR_DATA *ch, OBJ_DATA *obj);
int augment_exp_for_level (OBJ_DATA *obj, int target_level);
int get_augment_type (OBJ_DATA *obj);
bool can_augment_obj (CHAR_DATA *ch, OBJ_DATA *obj);
OBJ_AUGMENT_DATA *obj_augment_new(void);

/* more power should mean longer casting times, so you need to buy
 * and failure rates, so you need to buy down the others to 
 * keep them in balance.
 */
void do_consecrate (CHAR_DATA *ch, const char *argument)
{
	char	arg1[MAX_INPUT_LENGTH];
	char	arg2[MAX_INPUT_LENGTH];
	OBJ_DATA *obj = NULL;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (IS_NPC(ch)) {
		char_puts("Silly bean!", ch);
		return;
	}

	if (arg1[0] == '\0') {
		char_puts("Consecrate what?\n", ch);
		return;
	}

	if ((obj = get_obj_wear(ch, arg1)) == NULL) {
		char_puts("You're not wearing that.\n", ch);
		return;
	}

	if (!mlstr_null(obj->owner) 
	&& !IS_OWNER(ch, obj)) {
		act("This item is bound to another soul.",
			ch, NULL, NULL, TO_CHAR);
		return;
	}

	if (arg2[0] == '\0') {
		show_augment(ch, obj, NULL);
		return;
	}
	
	if (!str_cmp(arg2, "ritual")) {
		do_consecrate_ritual(ch, obj, argument);
		return;
	} 
	else if (!str_prefix(arg2, "ritua")) {
		char_puts("If you wish to perform the ritual, spell"
			" it out.\n",ch);
		return;

	} 
	else if (!str_prefix(arg2, "menu")) {
		show_augment_weapon_menu(ch, obj);
		return;
	} 
	else if (!str_prefix(arg2, "show")) {
		show_augment(ch, obj, NULL);
		return;
	}
	else if (!str_prefix(arg2, "augment")) {
		do_consecrate_buy(ch, obj, argument);
		return;
	}
	else {
		do_consecrate_commands(ch);
		return;
	}
}

void do_consecrate_commands(CHAR_DATA *ch)
{
	char_puts("consecrate syntax: consecrate <obj> <command>\n", ch);
	char_puts("consecrate commands: ritual, show, menu, augment\n", ch);
}

/**
 * initializes the object for augumentation.
 *
 */
void do_consecrate_ritual (CHAR_DATA *ch, OBJ_DATA *obj, const char *argument)
{
	int aug_type = get_augment_type(obj);
	char	arg1[MAX_INPUT_LENGTH];
	OBJ_AUGMENT_DATA *aug = NULL;
	pcskill_t *ps;
	int sn = 0;

	argument = one_argument(argument, arg1, sizeof(arg1));

	if (!IS_SET(ch->in_room->room_flags, 
		ROOM_HEROES_ONLY | ROOM_SOLITARY | ROOM_NOMOB)) {
		char_puts("This place is not sacred enough to perform the ritual.\n", ch);
		return;
	}

	if ((ch->gold + ch->pcdata->bank_g) < AUGMENT_COST_GOLD) {
		char_printf(ch, "You don't have the resources to bind"
			" this item to your soul.\n"
			"You need at least %d gold in the bank.\n",
			AUGMENT_COST_GOLD);
		return;
	}

	if (!can_augment_obj(ch, obj))
		return;

	switch(aug_type) {
	case AUGMENT_TYPE_FOCUS:
		if (arg1[0] == '\0') {
			act("What spell do you wish to channel through $p?",
				ch, obj, NULL, TO_CHAR);
			return;
		}

		ps = (pcskill_t*) skill_vlookup(&ch->pcdata->learned, arg1);
		if (!ps || get_skill(ch, sn = ps->sn) <= 0) {
			char_puts("You can't focus magic you don't know.\n", ch);
			return;
		}

		if (SKILL(sn)->spell_fun == NULL) {
			char_puts("Only spells can be focused.\n", ch);
			return;
		}

		if (obj->pIndexData->item_type == ITEM_WAND
		|| obj->pIndexData->item_type == ITEM_STAFF) {
			if (obj->value[ITEM_WAND_SPELL] != sn) {
				act("Those energies are foreign to $p!",
					ch, obj, NULL, TO_CHAR);
				act("You cross-bind $p, which blazes bright and is {Wgone{x.",
					ch, obj, NULL, TO_CHAR);
				act("$n's $p blazes bright and is {Wgone{x.",
					ch, obj, NULL, TO_ROOM);
				extract_obj(obj, 0);
				WAIT_STATE(ch, PULSE_VIOLENCE);
				return;
			}
		}
		else if (ps->percent < 100) {
			char_puts("You must master that spell first.\n",
				ch);
			return;
		}

		break;

	case AUGMENT_TYPE_WEAPON:
		break;
	}

	/* charge char the gold of the binding cost */
	if (ch->gold >= AUGMENT_COST_GOLD)
		ch->gold -= AUGMENT_COST_GOLD;
	else {
		ch->pcdata->bank_g -= AUGMENT_COST_GOLD - ch->gold;
		ch->gold = 0;
	}

	aug = obj_augment_new();
	aug->consecration_time = current_time;
	aug->last_level = current_time;
	aug->exp_modifier = get_augment_exp_modifier(ch, obj);
	
	switch (aug_type) {
	case AUGMENT_TYPE_FOCUS:
		aug->skill_sn = sn;
		break;

	case AUGMENT_TYPE_WEAPON:
		break;
	}

	obj->augment = aug;
	obj->level = get_wear_level(ch, obj);
	obj->owner = mlstr_dup(ch->short_descr);

	act("You tap into your wealth to modify $p to your purpose.",
		ch, obj, NULL, TO_CHAR);

	act("$p comes alive as you imbune it with your spirit and bind it"
		" to your soul.", ch, obj, NULL, TO_CHAR);
	act("The effort exhausts you completely.", ch, NULL, NULL, TO_CHAR);
	act("$n focuses all $h concentration on $p, and looks exhausted.",
		ch, obj, NULL, TO_ROOM);

	ch->hit = ch->hit * 10/100;
	ch->mana = ch->mana * 10/100;
	ch->move = ch->move * 10/100;

	aug->level = 1;
	WAIT_STATE(ch, PULSE_VIOLENCE *2);
}

/*
 * show the augments on this item.
 */
void show_augment (CHAR_DATA *ch, OBJ_DATA *obj, BUFFER *output)
{
	bool free_output = FALSE;
	int i = 0;

	if (!is_augmented(obj)) {
		act("$p is not consecrated.",
			ch, obj, NULL, TO_CHAR);
		return;
	}

	if (!mlstr_null(obj->owner) 
	&& !IS_OWNER(ch, obj)
	&& !IS_IMMORTAL(ch)) {
		act("This item is bound to another soul.",
			ch, NULL, NULL, TO_CHAR);
		return;
	}

	if (!output) {
		output = buf_new(-1);
		free_output = TRUE;
	}

	buf_printf(output,
		"%s is consecrated!\n",
		mlstr_mval(obj->short_descr));

	buf_printf(output,
		"   Level:       {c%d{x\n"
		"   Exp:         {c%d{x\n"
		"   Tnl:         {c%d{x\n"
		"   Points:      {c%d{x\n",
		obj->augment->level,
		obj->augment->exp,
		augment_tnl (obj),
		obj->augment->pts);

	switch (get_augment_type(obj)) {
	case AUGMENT_TYPE_WEAPON:
		buf_printf(output,
			"   DiceNumber:  {c%d{x\n"
			"   DiceSize:    {c%d{x\n"
			"   Flags:       [{c%s{x]\n"
			"   Hatred:      [{c%s{x]\n",
			obj->augment->dicenum,
			obj->augment->dicesize,
			flag_string(weapon_type2, obj->augment->weapon_flags),
			(obj->augment->racial_hatred != RACE_NEUTRAL) 
				? race_name(obj->augment->racial_hatred) 
				: "none"
			);

		break;

	case AUGMENT_TYPE_FOCUS:
		buf_printf(output,
			"   Spell:       '{c%s{x'\n"
			"   Mastery:     {c%d{x\n"
			"   Focus:       {c%d{x\n"
			"   Mana:        {c%d{x\n"
			"   Power:       {c%d{x\n",
		 	skill_name(obj->augment->skill_sn),
			obj->augment->mastery_level,
			obj->augment->focus_level,
			obj->augment->mana_level,
			obj->augment->power_level);

		break;
	}

	if (IS_IMMORTAL(ch) && ch->level > (ML - 5)) {
		buf_printf(output,
			"   {DExpModifier: {c%d{x (IMM only info)\n",
			obj->augment->exp_modifier);
		for (i = 2; i <= 20; i++) {
			buf_printf(output, "   {DLevel %2d: {c%7d\n",
				i,
				augment_exp_for_level(obj, i));
		}
		buf_printf(output, "{x");
	}

	if (free_output) {
		send_to_char(buf_string(output), ch);
		buf_free(output);
	}
}

/**
 * buy augmentations to your item
 */
void do_consecrate_buy (CHAR_DATA *ch, OBJ_DATA *obj, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	int cost = 1;
	AUGMENTATION_DATA *aug = NULL;
	OBJ_INDEX_DATA *pObjIndex = NULL;
	OBJ_DATA *component = NULL;
	int gp_cost = 0;

	argument = one_argument(argument, arg1, sizeof(arg1));

	if (!obj->augment) {
		act("$p isn't consecrated.",
			ch, obj, NULL, TO_CHAR);
		return;
	}

	if (arg1[0] == '\0') {
		show_augment_menu(ch, obj);
	}

	for (aug = augmentation_table; aug->name; aug++) {
		if (!is_name(arg1, aug->name))
			continue;

		if (get_augment_type(obj) != aug->type) {
			act("You can't augment $p in that way.",
				ch, obj, NULL, TO_CHAR);
			return;
		}
		cost = aug->augment_cost(ch, obj);
		if (cost > obj->augment->pts) {
			act("$p doesn't have enough augmentation points.",
				ch, obj, NULL, TO_CHAR);
			return;
		}

		gp_cost = (obj->augment->level > 0 ? obj->augment->level : 1) 
			* aug->gp_cost_per_level;
		if ((ch->gold + ch->pcdata->bank_g) < gp_cost) {
			char_printf(ch, "You don't have the resources to"
				" afford the research necessary for that"
				" augmentation.\n"
				"You need at least {Y%d gold{x in the bank.\n",
				gp_cost);
			return;
		}

		/* required augmentation component present? */
		if (aug->component_vnum != 0
		&& !(component = has_item(ch, aug->component_vnum, -1, FALSE))) {
			pObjIndex = get_obj_index(aug->component_vnum);
			if (pObjIndex == NULL) {
				BUG("augment component [%d] does not exist.",
					aug->component_vnum);
				return;
			}
			char_printf(ch, "Without %s, that would be impossible.",
				mlstr_mval(pObjIndex->short_descr));
			break;
		}

		/* okay, go ahead a buy */
		if (!aug->augment_buy(ch, obj, component, argument))
			continue;

		if (component) {
			obj_from_char(component);
			extract_obj(component, 0);
		}
			
		/* gp cost */
		char_printf(ch, "Your toil cost {Y%d{x gold.\n",
			gp_cost);

		if (ch->gold >= gp_cost)
			ch->gold -= gp_cost;
		else {
			ch->pcdata->bank_g -= gp_cost - ch->gold;
			ch->gold = 0;
		}

		/* pts */
		obj->augment->pts -= cost;
		break;
	}
}

/*
 * show the augment menu for whatever type of item this is.
 */
void show_augment_menu (CHAR_DATA *ch, OBJ_DATA *obj)
{
	switch (get_augment_type(obj)) {
	case AUGMENT_TYPE_WEAPON:
		show_augment_weapon_menu(ch, obj);
		break;

	case AUGMENT_TYPE_FOCUS:
		show_augment_focus_menu(ch, obj);
		break;
	}
}

/*
 * menu of possible weapon augmentations for this weapon
 */
void show_augment_weapon_menu(CHAR_DATA *ch, OBJ_DATA *obj)
{
	BUFFER *output;
	AUGMENTATION_DATA *aug;
	output = buf_new(-1);

	buf_printf(output,
		"Research into the following weapon augmentations are possible:\n");

	for (aug = augmentation_table; aug->name; aug++) {
		if (aug->type == AUGMENT_TYPE_WEAPON
		&& aug->augment_cost(ch, obj) > 0) {
			buf_printf(output,
				"   {c%-20s{x {c%2d{xpts\t{Y%6d{xgp\n",
				aug->name,
				aug->augment_cost(ch, obj),
				((obj->augment->level >0) 
					? obj->augment->level 
					: 1) 
					* aug->gp_cost_per_level);
		}
	}

	send_to_char(buf_string(output), ch);
	buf_free(output);
}

/*
 * menu of possible focus augmentations for this focus item
 */
void show_augment_focus_menu(CHAR_DATA *ch, OBJ_DATA *obj)
{
	BUFFER *output;
	AUGMENTATION_DATA *aug;
	output = buf_new(-1);

	buf_printf(output,
		"Research into the following focus augmentations are possible:\n");

	for (aug = augmentation_table; aug->name; aug++) {
		if (aug->type == AUGMENT_TYPE_FOCUS
		&& aug->augment_cost(ch, obj) > 0) {
			buf_printf(output,
				"%s costs %d\n",
				aug->name,
				aug->augment_cost(ch, obj));
		}
	}

	send_to_char(buf_string(output), ch);
	buf_free(output);
}

/**
 * certain items can be augmented in certain ways
 */
int get_augment_type (OBJ_DATA *obj)
{
	int type = AUGMENT_TYPE_NONE;

	if (obj->pIndexData->item_type == ITEM_WEAPON
		&& IS_SET(obj->pIndexData->wear_flags, ITEM_WIELD)) {
		type = AUGMENT_TYPE_WEAPON;
	}
	else if (obj->pIndexData->item_type != ITEM_WEAPON
		&& IS_SET(obj->pIndexData->wear_flags, ITEM_HOLD)) {
		type = AUGMENT_TYPE_FOCUS;
	}

	return type;
}

/*
 * it the object augmented?
 */
bool is_augmented (OBJ_DATA *obj)
{
	if (!obj || !obj->augment)
		return FALSE;
	return TRUE;
}

/*
 * is this an augmented object which is mine?
 */
bool is_not_my_augment (CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (obj && obj->augment
	&& !mlstr_null(obj->owner) 
	&& !IS_OWNER(ch, obj))
		return TRUE;

	return FALSE;
}

/*
 * when someone who doesn't own an augmented item uses it, it 
 * gets purged to keep people from passing objects.
 */
OBJ_DATA * augment_unauthorized_use (CHAR_DATA *ch, OBJ_DATA *wield)
{
	act("Dissonance between your soul and $p cause it to {Cvaporize{x!",
		ch, wield, NULL, TO_CHAR);
	act("$n's $p shakes violently and {Cvaporizes{x!",
		ch, wield, NULL, TO_ROOM);
	obj_from_char(wield);
	extract_obj(wield, 0);
	wield = NULL;
	return wield;
}

/*
 * can this item be augmented?
 */
bool can_augment (OBJ_DATA *obj)
{
	if (IS_OBJ_LIMITED(obj->pIndexData))
		return FALSE;

	if (IS_SET(obj->pIndexData->extra_flags, ITEM_QUEST))
		return FALSE;

	if (IS_SET(obj->pIndexData->extra_flags, 
		ITEM_NOSAC | ITEM_NOPURGE | ITEM_HAD_TIMER 
		| ITEM_MELT_DROP | ITEM_INVENTORY)) 
		return FALSE;

	return TRUE;
}

/**
 * find out if the character can augment this type of item
 * and tell them why they can't.
 */
bool can_augment_obj (CHAR_DATA *ch, OBJ_DATA *obj)
{
	int type = get_augment_type(obj);
	pcskill_t *ps = NULL;
	int sn = 0;

	if (ch->level <= AUGMENT_MIN_LEVEL) {
		act("You must become more powerful to extend your will"
				" over items in this realm.",
				ch, NULL, NULL, TO_CHAR);
		return FALSE;
	}

	if (ch->class == CLASS_CLERIC) {
		act("Seek greater power by performing the will of your"
			" god, instead of material possessions.",
			ch, NULL, NULL, TO_CHAR);
		return FALSE;
	}

	/* not the owner */
	if (!mlstr_null(obj->owner) 
	&& !IS_OWNER(ch, obj)) {
		act("This item is already bound to another soul.",
			ch, NULL, NULL, TO_CHAR);
		return FALSE;
	}

	if (obj->augment) {
		act("This item is already bound to your soul.",
			ch, NULL, NULL, TO_CHAR);
		return FALSE;
	}


	/* no artifacts */
	if (IS_OBJ_LIMITED(obj->pIndexData)) {
		act("This item is too ancient to be bound to your young soul.",
			ch, NULL, NULL, TO_CHAR);
		return FALSE;
	}

	/* no quest items */
	if (IS_SET(obj->pIndexData->extra_flags, ITEM_QUEST)) {
		act("Quest items can not be consecrated.",
			ch, NULL, NULL, TO_CHAR);
		return FALSE;
	}

	/* no non-sac items */
	if (IS_SET(obj->pIndexData->extra_flags, 
		ITEM_NOSAC | ITEM_NOPURGE | ITEM_HAD_TIMER 
		| ITEM_MELT_DROP | ITEM_INVENTORY)) {
		act("Something strange about $p makes you think better of"
			" performing the ritual on it.",
			ch, obj, NULL, TO_CHAR);
		return FALSE;
	}

	switch (type) {
	case AUGMENT_TYPE_NONE:
		act("You can't consecrate $p.",
			ch, obj, NULL, TO_CHAR);
		return FALSE;
		break;

	case AUGMENT_TYPE_WEAPON:
		if (ch->class == CLASS_WARLOCK
		|| ch->class == CLASS_WITCH
		|| ch->class == CLASS_NECROMANCER) {
			act("You could never atune your existence to an item"
				" of such brutish intent.",
				ch, NULL, NULL, TO_CHAR);
			return FALSE;
		}

		sn = get_weapon_sn(obj);
		ps = pcskill_lookup(ch, sn);

		if (ps->percent < 100) {
			act("You must first master the techniques of $p"
				" to consecrate it.",
				ch, obj, NULL, TO_CHAR);
			return FALSE;
		}

		if (IS_SET(obj->pIndexData->extra_flags, ITEM_ENCHANTED)) {
			act("Strange magic already binds $p to another source.",
				ch, obj, NULL, TO_CHAR);
			return FALSE;
		}

		break;

	case AUGMENT_TYPE_FOCUS:
		if (ch->class != CLASS_WARLOCK
		&& ch->class != CLASS_WITCH
		&& ch->class != CLASS_NECROMANCER) {
			act("You do not possess the intellectual discipline"
				" to focus your energies in such an object.",
				ch, NULL, NULL, TO_CHAR);
			return FALSE;
		}

		switch (obj->pIndexData->item_type) {
		case ITEM_GEM:
		case ITEM_JEWELRY:
		case ITEM_TREASURE:
		case ITEM_WARP_STONE:
		case ITEM_STAFF:
		case ITEM_WAND:
			break;
		default:
			act("You are unable to focus your powers with this"
				" type of item.",
				ch, NULL, NULL, TO_CHAR);
			return FALSE;

		}
		break;
	}
	
	return TRUE;
}


OBJ_DATA * get_augment_obj (CHAR_DATA *ch)
{
	OBJ_DATA *obj = NULL;
	for (obj = ch->carrying; obj; obj = obj->next_content) {
		if (obj->augment)
			return obj;
	}
	return NULL;
}

/**
 * items with higher BP value are harder to augment
 * returns a percentage.
 */
int get_augment_exp_modifier(CHAR_DATA *ch, OBJ_DATA *obj)
{
	int mod = 100;
	int bp = 0;
	int bpmax = 0;


	build_points(obj->pIndexData, &bp, &bpmax);

	if (obj->augment->skill_sn > 0) {
		
		mod += bp / 2;
		mod += skill_level(ch, obj->augment->skill_sn) / (LEVEL_HERO / 5);
	}
	else {
		mod += bp;
	}

	switch (obj->pIndexData->item_type) {
	case ITEM_WAND: mod = mod * AUGMENT_WAND_EXP_MOD /100; break;
	case ITEM_STAFF: mod = mod * AUGMENT_STAFF_EXP_MOD /100; break;
	}
	
	mod = UMAX(50, mod);
	return mod;
}

int augment_exp_for_level (OBJ_DATA *obj, int target_level)
{
	int i = 0;
	int tnl = AUGMENT_BASE_EXP;

	if (!obj->augment) return -1;

	tnl = tnl * obj->augment->exp_modifier / 100;

	for (i = 2; i < target_level; i++) {
		tnl += (tnl / 2);
	}

	return tnl;
}

int augment_tnl (OBJ_DATA *obj)
{
	int next_level = 0;
	
	if (!obj->augment) return -1;
	
	next_level = augment_exp_for_level(obj, obj->augment->level+1);

	return next_level - obj->augment->exp;
}

bool gain_obj_exp (CHAR_DATA *ch, OBJ_DATA *obj, int gain)
{
	int tnl = 0;
	char mesg[MAX_STRING_LENGTH];
	if (!obj->augment) return FALSE;
	if (ch->level < AUGMENT_MIN_LEVEL) return FALSE;
	if (mlstr_null(obj->owner) 
	|| !IS_OWNER(ch, obj)) return FALSE;

	tnl = augment_tnl(obj);

	obj->augment->exp += gain;

	if (tnl <= gain) {
		obj->augment->level++;
		obj->augment->pts++;
		act("$p {Craises a level!!{x",
			ch, obj, NULL, TO_CHAR);
		snprintf(mesg, sizeof(mesg),
			"{W$N's %s {Bhas attained level {Y$j!{x",
				mlstr_mval(obj->short_descr));
		wiznet(mesg, ch, (const void *) obj->augment->level, 
			WIZ_LEVELS, 0, 0);
		save_char_obj(ch, FALSE);
	}
	return TRUE;
}

/* constructors/destructors -------------------------------------------*/

OBJ_AUGMENT_DATA *obj_augment_new(void)
{
	OBJ_AUGMENT_DATA *aug;

	aug = calloc(1, sizeof(OBJ_AUGMENT_DATA));
	aug->racial_hatred = RACE_NEUTRAL;
	return aug;

}               
        
void obj_augment_free(OBJ_AUGMENT_DATA *poa)
{       
	if (poa == NULL) return;
	free(poa);
}

/* writer/reader ------------------------------------------------------*/
void fwrite_augment (OBJ_DATA *obj, FILE *fp)
{
	fprintf(fp, "AugTime %ld\n", obj->augment->consecration_time);
	fprintf(fp, "AugLastLevel %ld\n", obj->augment->last_level);
	fprintf(fp, "AugExp %d %d %d %d\n",
		obj->augment->level,
		obj->augment->exp_modifier,
		obj->augment->exp,
		obj->augment->pts);

	switch(get_augment_type(obj)) {
	case AUGMENT_TYPE_WEAPON:
		if (obj->augment->dicesize)
			fprintf(fp, "AugDiceSize %d\n", 
				obj->augment->dicesize);
		if (obj->augment->dicenum)
			fprintf(fp, "AugDiceNumber %d\n", 
				obj->augment->dicenum);
		if (obj->augment->weapon_flags)
			fprintf(fp, "AugFlags %s\n", 
				format_flags(obj->augment->weapon_flags));
		if (obj->augment->racial_hatred != RACE_NEUTRAL)
			fprintf(fp, "AugHatred %s\n", 
				race_name(obj->augment->racial_hatred));
		break;
	case AUGMENT_TYPE_FOCUS:
		fprintf(fp, "AugSkill '%s'\n",
			skill_name(obj->augment->skill_sn));
		if (obj->augment->focus_level)
			fprintf(fp, "AugFocus %d\n", 
				obj->augment->focus_level);
		if (obj->augment->mastery_level)
			fprintf(fp, "AugMastery %d\n", 
				obj->augment->mastery_level);
		if (obj->augment->power_level)
			fprintf(fp, "AugPower %d\n", 
				obj->augment->power_level);
		if (obj->augment->mana_level)
			fprintf(fp, "AugMana %d\n", 
				obj->augment->mana_level);
		break;
	}
}

/*
 * read and fill the data
 */
bool fread_augment (OBJ_DATA *obj, FILE *fp, char *word)
{
	bool fMatch = FALSE;

	if (!obj->augment)
		obj->augment = obj_augment_new();

	switch(word[3]) {
	case 'D':
		KEY("AugDiceSize", obj->augment->dicesize, fread_number(fp));
		KEY("AugDiceNumber", obj->augment->dicesize, fread_number(fp));
		break;

	case 'E':
		obj->augment->level = fread_number(fp);
		obj->augment->exp_modifier = fread_number(fp);
		obj->augment->exp = fread_number(fp);
		obj->augment->pts = fread_number(fp);
		fMatch = TRUE;
		break;

	case 'F':
		KEY("AugFlags", obj->augment->weapon_flags, fread_flags(fp));
		KEY("AugFocus", obj->augment->focus_level, fread_number(fp));
		break;

	case 'H':
		if (!str_cmp(word, "AugHatred")) {
			const char *race = fread_string(fp);
			obj->augment->racial_hatred = rn_lookup(race);
			free_string(race);
			if (obj->augment->racial_hatred != RACE_NEUTRAL)
				fMatch = TRUE;
			break;
		}
		break;
		
	case 'L':
		KEY("AugLastLevel", obj->augment->last_level, fread_number(fp));
		break;

	case 'M':
		KEY("AugMana", obj->augment->mana_level, fread_number(fp));
		KEY("AugMastery", obj->augment->mastery_level, fread_number(fp));
		break;

	case 'P':
		KEY("AugPower", obj->augment->power_level, fread_number(fp));
		break;

	case 'S':
		KEY("AugSkill", obj->augment->skill_sn, sn_lookup(fread_word(fp)));
		break;

	case 'T':
		KEY("AugTime", obj->augment->consecration_time, fread_number(fp));
		break;
	}

	if (!fMatch) {
		obj_augment_free(obj->augment);
		obj->augment = NULL;
	}

	return fMatch;
}

/* -----------------------------------------------------------------
 * augments
 * -----------------------------------------------------------------*/

/* dicenumber ******************************************************/

static int augment_cost_dicenumber (CHAR_DATA *ch, OBJ_DATA *obj)
{
	return AUGMENT_COST_WEAPON_DICENUMBER;
}

static bool augment_buy_dicenumber (CHAR_DATA *ch, OBJ_DATA *obj,
	OBJ_DATA *component, const char *argument)
{
	act("You shave off a section of the $p drastically improving its deadly balance.",
		ch, obj, NULL, TO_CHAR);
	act("$n shaves off a section of the $p, and seems pleased with the result.",
		ch, obj, NULL, TO_ROOM);
	obj->value[ITEM_WEAPON_DICE_NUM]++;
	obj->augment->dicenum++;

	if (component)
		act("$p crumbles to dust.",
			ch, component, NULL, TO_ALL);

	return TRUE;
}

/* dicesize *******************************************************/
static int augment_cost_dicesize (CHAR_DATA *ch, OBJ_DATA *obj)
{
	return AUGMENT_COST_WEAPON_DICESIZE;
}

static bool augment_buy_dicesize (CHAR_DATA *ch, OBJ_DATA *obj,
	OBJ_DATA *component, const char *argument)
{
	act("You hone out some imperfections in $p.",
		ch, obj, NULL, TO_CHAR);
	act("$n hones out some imperfections in $p.",
		ch, obj, NULL, TO_ROOM);
	obj->value[ITEM_WEAPON_DICE_SIZE]++;
	obj->augment->dicesize++;

	if (component)
		act("$p crumbles to dust.",
			ch, component, NULL, TO_ALL);

	return TRUE;
}


/* holy ***********************************************************/
static int augment_cost_holy (CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (IS_WEAPON_STAT(obj, WEAPON_HOLY))
		return 0;

	return AUGMENT_COST_WEAPON_HOLY;
}

static bool augment_buy_holy (CHAR_DATA *ch, OBJ_DATA *obj,
	OBJ_DATA *component, const char *argument)
{
	if (IS_WEAPON_STAT(obj, WEAPON_VAMPIRIC)
	|| IS_WEAPON_STAT(obj, ITEM_ANTI_GOOD)) {
		act("$p rejects the holy blessing.",
			ch, obj, NULL, TO_CHAR);
		return FALSE;
	}

	SET_BIT(obj->value[ITEM_WEAPON_FLAGS], WEAPON_HOLY);
	SET_BIT(obj->augment->weapon_flags, WEAPON_HOLY);

	act("$p bursts with holy light!",
		ch, obj, NULL, TO_ALL);

	if (component)
		act("$p turns to light and ascends to heaven.",
			ch, component, NULL, TO_ALL);

	return TRUE;
}

/* flaming ********************************************************/
static int augment_cost_flaming (CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (IS_WEAPON_STAT(obj, WEAPON_FLAMING))
		return 0;

	return AUGMENT_COST_WEAPON_FLAMING;
}

static bool augment_buy_flaming (CHAR_DATA *ch, OBJ_DATA *obj, 
	OBJ_DATA *component, const char *argument)
{
	if (IS_WEAPON_STAT(obj, WEAPON_FROST)
	|| IS_WEAPON_STAT(obj, WEAPON_SHOCKING)) {
		act("$p seems bound to another elemental force already.",
			ch, obj, NULL, TO_CHAR);
		return FALSE;
	}

	SET_BIT(obj->value[ITEM_WEAPON_FLAGS], WEAPON_FLAMING);
	SET_BIT(obj->augment->weapon_flags, WEAPON_FLAMING);
	act("$p ignites in flames!",
		ch, obj, NULL, TO_ALL);

	if (component)
		act("$p vanishes in a puff of smoke.",
			ch, component, NULL, TO_ALL);

	return TRUE;
}

/* hatred *********************************************************/
static int augment_cost_hatred (CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (obj->augment->racial_hatred == RACE_NEUTRAL)
		return 0;

	return AUGMENT_COST_WEAPON_HATRED;
}

static bool augment_buy_hatred (CHAR_DATA *ch, OBJ_DATA *obj, 
	OBJ_DATA *component, const char *argument)
{
	int race = RACE_NEUTRAL;

	if (obj->augment->racial_hatred != RACE_NEUTRAL) {
		act("$p is already filled with rage!",
			ch, obj, NULL, TO_CHAR);
		return FALSE;
	}

	race = rn_lookup(argument);

	if (race < 0) {
                char_puts("Available races are:", ch);

                for (race = 1; race < races.nused; race++) {
                        if ((race % 3) == 0)
                                char_puts("\n", ch);
                        char_printf(ch, " %-15s", RACE(race)->name);
                }

                char_puts("\n", ch);
		
		return FALSE;
	}

	obj->augment->racial_hatred = race;

	act("$p pulses with animosity!",
		ch, obj, NULL, TO_ALL);

	if (component)
		act("$p disolves into dust.",
			ch, component, NULL, TO_ALL);

	return TRUE;
}

