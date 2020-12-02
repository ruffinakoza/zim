/*
 * $Id: martial_art.c 1016 2007-02-07 02:43:47Z zsuzsu $
 */

/***************************************************************************
 *     ANATOLIA 2.1 is copyright 1996-1997 Serdar BULUT, Ibrahim CANPUNAR  *	
 *     ANATOLIA has been brought to you by ANATOLIA consortium		   *
 *	 Serdar BULUT {Chronos}		bulut@rorqual.cc.metu.edu.tr       *
 *	 Ibrahim Canpunar  {Asena}	canpunar@rorqual.cc.metu.edu.tr    *	
 *	 Murat BICER  {KIO}		mbicer@rorqual.cc.metu.edu.tr	   *	
 *	 D.Baris ACAR {Powerman}	dbacar@rorqual.cc.metu.edu.tr	   *	
 *     By using this code, you have agreed to follow the terms of the      *
 *     ANATOLIA license, in the file Anatolia/anatolia.licence             *	
 ***************************************************************************/

/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*	ROM 2.4 is copyright 1993-1995 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@pacinfo.com)				   *
*	    Gabrielle Taylor (gtaylor@pacinfo.com)			   *
*	    Brian Moore (rom@rom.efn.org)				   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include "merc.h"
#include "debug.h"
#include "fight.h"
#include "stats.h"

#ifdef SUNOS
#	include <stdarg.h>
#	include "compat/compat.h"
#endif

DECLARE_DO_FUN(do_yell		);
DECLARE_DO_FUN(do_sleep		);
DECLARE_DO_FUN(do_sit		);
DECLARE_DO_FUN(do_bash_door	);

static inline bool	check_yell	(CHAR_DATA *ch, CHAR_DATA *victim,
					 bool fighting);
void			one_hit		(CHAR_DATA *ch, CHAR_DATA *victim,
					 int dt, int loc); 
void			set_fighting	(CHAR_DATA *ch, CHAR_DATA *victim);

/*
 * Disarm a creature.
 * Caller must check for successful attack.
 */
void disarm(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj)
{
	OBJ_DATA *obj2;
	int skill;
	int modifier = 0;

	if (IS_OBJ_STAT(obj, ITEM_NOREMOVE)) {
		act("$S weapon won't budge!", ch, NULL, victim, TO_CHAR);
		act("$n tries to disarm you, but your weapon won't budge!",
		    ch, NULL, victim, TO_VICT);
		act("$n tries to disarm $N, but fails.",
		    ch, NULL, victim, TO_NOTVICT);
		return;
	}

	if ((skill = get_skill(victim, gsn_grip))
	   || (skill = get_skill(victim, gsn_double_grip)
		&& get_eq_char(victim, WEAR_SHIELD) == NULL 
		&& get_eq_char(ch, WEAR_HOLD) == NULL
		&& get_eq_char(victim, WEAR_SECOND_WIELD) == NULL)) {


		modifier += (get_curr_stat(victim, STAT_STR) 
				+ (get_skill(victim, gsn_double_grip) ? 3 : 0)
			  - get_curr_stat(ch, STAT_STR)) * 5;

		if (modifier < 0
		&& get_skill(victim, gsn_double_grip))
			modifier /= 3;

		skill += modifier;

		if (number_percent() < skill) {
			act("$N grips and prevent you to disarm $M!",
			    ch, NULL, victim, TO_CHAR);
			act("$n tries to disarm you, but your grip is too strong!",
			    ch, NULL, victim, TO_VICT);
			act("$n tries to disarm $N, but fails.",
			    ch, NULL, victim, TO_NOTVICT);
			if (skill == get_skill(victim, gsn_grip))
				check_improve(victim, gsn_grip, TRUE, 1);
			else
				check_improve(victim, gsn_double_grip, TRUE, 1);
			return;
		}
		else
			if (skill == get_skill(victim, gsn_grip))
				check_improve(victim,gsn_grip,FALSE,1);
			else
				check_improve(victim, gsn_double_grip, FALSE, 1);
	}

	act_puts("$n DISARMS you and sends your weapon flying!", 
		 ch, NULL, victim, TO_VICT, POS_FIGHTING);
	act_puts("You disarm $N!", ch,NULL, victim, TO_CHAR, POS_FIGHTING);
	act_puts("$n disarms $N!", ch, NULL, victim, TO_NOTVICT, POS_FIGHTING);

	obj_from_char(obj);
	if (IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_INVENTORY))
		obj_to_char(obj, victim);
	else {
		obj_to_room(obj, victim->in_room);
		if (IS_NPC(victim)
		&&  victim->wait == 0
		&&  can_see_obj(victim,obj))
			get_obj(victim, obj, NULL);
	}

	if ((obj2 = get_eq_char(victim, WEAR_SECOND_WIELD)) != NULL) {
		act_puts("You wield your second weapon as your first!.",
			 ch, NULL, victim, TO_VICT, POS_FIGHTING);
		act_puts("$N wields $s second weapon as first!",
			 ch, NULL, victim, TO_CHAR, POS_FIGHTING);
		act_puts("$N wields $s second weapon as first!",
			 ch, NULL, victim, TO_NOTVICT, POS_FIGHTING);
		unequip_char(victim, obj2);
		equip_char(victim, obj2, WEAR_WIELD);
	}
}

void do_berserk(CHAR_DATA *ch, const char *argument)
{
	int chance, hp_percent;
	int cost;

	cost = move_cost(ch, gsn_berserk, 0, 0);

	if ((chance = get_skill(ch, gsn_berserk)) == 0) {
		char_puts("You turn red in the face, but nothing happens.\n",
			  ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_BERSERK)
	||  is_affected(ch, gsn_berserk)
	||  is_affected(ch, gsn_frenzy)) {
		char_puts("You get a little madder.\n", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CALM)) {
		char_puts("You're feeling too mellow to berserk.\n", ch);
		return;
	}

	if (ch->mana < 50) {
		char_puts("You can't get up enough energy.\n", ch);
		return;
	}

	/* modifiers */

	/* fighting */
	if (ch->position == POS_FIGHTING)
		chance += 10;

	/* damage -- below 50% of hp helps, above hurts */
	hp_percent = 100 * ch->hit/ch->max_hit;
	chance += 25 - hp_percent/2;


	if (number_percent() < chance) {
		AFFECT_DATA af;

		WAIT_STATE(ch,PULSE_VIOLENCE);
		ch->mana -= 50;
		ch->move -= cost;

		/* heal a little damage */
		ch->hit += LEVEL(ch) * 2;
		ch->hit = UMIN(ch->hit,ch->max_hit);

		char_puts("Your pulse races as you are consumned by rage!\n",
			  ch);
		act_puts("$n gets a wild look in $s eyes.",
			 ch, NULL, NULL, TO_ROOM, POS_FIGHTING);
		check_improve(ch, gsn_berserk, TRUE, 2);

		af.where	= TO_AFFECTS;
		af.type		= gsn_berserk;
		af.level	= ch->level;
		af.duration	= number_fuzzy(ch->level / 8);
		af.modifier	= UMAX(1,LEVEL(ch)/5);
		af.bitvector 	= AFF_BERSERK;

		af.location	= APPLY_HITROLL;
		affect_to_char(ch,&af);

		af.location	= APPLY_DAMROLL;
		affect_to_char(ch,&af);

		af.modifier	= UMAX(10,10 * (LEVEL(ch)/5));
		af.location	= APPLY_AC;
		affect_to_char(ch,&af);
	}
	else {
		WAIT_STATE(ch,2 * PULSE_VIOLENCE);
		ch->mana -= 25;
		ch->move -= cost / 2;

		char_puts("Your pulse speeds up, but nothing happens.\n",
			  ch);
		check_improve(ch, gsn_berserk, FALSE, 2);
	}
}

void do_bash(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int chance, wait;
	bool fighting;
	int damage_bash;
	int prot_chance = IS_NPC(ch) ? 90 : 60;

	if (MOUNTED(ch)) {
		char_puts("You can't bash while riding!\n", ch);
		return;
	}
	
	argument = one_argument(argument, arg, sizeof(arg));
 
	if ((chance = get_skill(ch, gsn_bash)) == 0) {
		char_puts("Bashing? What's that?\n", ch);
		return;
	}

	chance = chance * 2 / 3;
 
	if (arg[0] != '\0' && !str_cmp(arg, "door")) {
		do_bash_door(ch, argument);
		return;
	}

	if (arg[0] == '\0') {
		victim = ch->fighting;
		if (victim == NULL) {
			char_puts("But you aren't fighting anyone!\n", ch);
			return;
		}
	}
	else
		victim = get_char_room(ch, arg);

	if (!victim || victim->in_room != ch->in_room) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("They aren't here.\n", ch);
		return;
	}


	WAIT_STATE(ch, SKILL(gsn_bash)->beats);

	if (victim->position < POS_FIGHTING) {
		act("You'll have to let $M get back up first.",
			  ch, NULL, victim, TO_CHAR);
		return;
	} 

	if (victim == ch) {
		char_puts("You try to bash your brains out, but fail.\n",
			     ch);
		return;
	}

	if (MOUNTED(victim)) {
		char_puts("You can't bash a riding one!\n", ch);
		return;
	}

	if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim) {
		act("But $N is your friend!", ch, NULL, victim, TO_CHAR);
		return;
	}
		
	if (is_affected(victim, gsn_protective_shield)) {
		if (number_percent() < prot_chance) {
			act_puts("Your bash seems to slide around $N.",
				 ch, NULL, victim, TO_CHAR, POS_FIGHTING);
			act_puts("$n's bash slides off your protective shield.",
				 ch, NULL, victim, TO_VICT, POS_FIGHTING);
			act_puts("$n's bash seems to slide around $N.",
				 ch, NULL, victim, TO_NOTVICT, POS_FIGHTING);
			return;
		}
	}
	else {
		chance -= (get_curr_stat(victim, STAT_DEX) - 20) * 2;
	}

	if (is_safe(ch, victim))
		return;

	/* modifiers */

	/* size  and weight */
	chance += URANGE(-10, 
			((ch->carry_weight - victim->carry_weight) 
			/ 500), 
			10);

	if (ch->size > victim->size)
		chance -= (ch->size - victim->size) * 5;
	else
		chance -= (victim->size - ch->size) * 7; 

	/* stats */
	chance += (get_curr_stat(ch, STAT_STR) - get_curr_stat(victim, STAT_STR)) * 3;

	if (IS_AFFECTED(ch, AFF_FLYING))
		chance -= 10;

	/* speed */
	if (IS_NPC(ch) && IS_SET(ch->pIndexData->off_flags, OFF_FAST))
		chance += 10;
	if (IS_NPC(victim) && IS_SET(victim->pIndexData->off_flags, OFF_FAST))
		chance -= 20;

	/* level */
	chance += (LEVEL(ch) - LEVEL(victim)) * 2;

	RESET_WAIT_STATE(ch);
	fighting = (ch->fighting != NULL);

	if (!IS_NPC(ch) && !IS_NPC(victim))
		DEBUG(DEBUG_SKILL_BASH,
			"%s[%d] bashing %s[%d]: %d%%",
			ch->name,
			LEVEL(ch),
			victim->name,
			LEVEL(victim),
			chance);

	chance = URANGE(20, chance, 80);

	/* now the attack */
	if (number_percent() < chance) {
		act("$n sends you sprawling with a powerful bash!",
		    ch, NULL, victim, TO_VICT);
		act("You slam into $N, and send $M flying!",
		    ch, NULL, victim, TO_CHAR);
		act("$n sends $N sprawling with a powerful bash.",
		    ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, gsn_bash, TRUE, 1);

		wait = 2;

		switch(number_bits(2)) {
			case 0: 
			case 1:
				wait = 1; break;
			case 2: 
			case 3:
				wait = 2; break;
		}

		WAIT_STATE(victim, wait * PULSE_VIOLENCE);
		WAIT_STATE(ch, SKILL(gsn_bash)->beats);
		victim->position = POS_RESTING;
		damage_bash = number_range(ch->damroll/3, ch->damroll / 2);
		damage_bash += (ch->size - victim->size)
				* LEVEL(ch) / 10;
		damage_bash += chance/10;

		if (!IS_NPC(ch) && !IS_NPC(victim))
			DEBUG(DEBUG_SKILL_BASH,
				"bashed: %s[%d]%ds vs %s[%d]%ds: %d dam (%d)",
				ch->name,
				LEVEL(ch),
				ch->size,
				victim->name,
				LEVEL(victim),
				victim->size,
				damage_bash,
				ch->damroll);

		damage_bash = UMAX(10, damage_bash);
		damage(ch, victim, damage_bash, gsn_bash, DAM_BASH, TRUE);
	}
	else {
		damage(ch, victim, 0, gsn_bash, DAM_BASH, TRUE);
		act_puts("You fall flat on your face!",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		act("$n falls flat on $s face.",
		    ch, NULL, victim, TO_NOTVICT);
		act("You evade $n's bash, causing $m to fall flat on $s face.",
		    ch, NULL, victim, TO_VICT);
		check_improve(ch, gsn_bash, FALSE, 1);
		ch->position = POS_RESTING;
		WAIT_STATE(ch, SKILL(gsn_bash)->beats * 3/2); 
	}

	if (check_yell(ch, victim, fighting)) 
		doprintf(do_yell, victim,
			 "Help! %s is bashing me!", PERS(ch, victim));
}

void do_dirt(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	bool fighting;
	int chance;

	if (MOUNTED(ch)) {
		char_puts("You can't dirt while riding!\n", ch);
		return;
	}

	one_argument(argument, arg, sizeof(arg));

	if ((chance = get_skill(ch, gsn_dirt)) == 0) {
		char_puts("You get your feet dirty.\n", ch);
		return;
	}

	if (arg[0] == '\0') {
		victim = ch->fighting;
		if (victim == NULL) {
			char_puts("But you aren't in combat!\n", ch);
			return;
		}
	}
	else 
		victim = get_char_room(ch, arg);

	if (!victim || victim->in_room != ch->in_room) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("They aren't here.\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_dirt)->beats);

	if (IS_AFFECTED(ch, AFF_FLYING)) {
		 char_puts("While flying?\n", ch);
		 return;
	}

	if (IS_AFFECTED(victim, AFF_BLIND)) {
		act("$e's already been blinded.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (victim == ch) {
		char_puts("Very funny.\n", ch);
		return;
	}

	if (MOUNTED(victim)) {
		char_puts("You can't dirt a riding one!\n", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
		act("But $N is such a good friend!", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (is_safe(ch, victim))
		return;

	/* modifiers */

	/* dexterity */
	chance += get_curr_stat(ch, STAT_DEX);
	chance -= 2 * get_curr_stat(victim, STAT_DEX);

	/* speed  */
	if ((IS_NPC(ch) && IS_SET(ch->pIndexData->off_flags, OFF_FAST))
	||  IS_AFFECTED(ch, AFF_HASTE))
		chance += 10;
	if ((IS_NPC(victim) && IS_SET(victim->pIndexData->off_flags, OFF_FAST))
	||  IS_AFFECTED(victim, AFF_HASTE))
		chance -= 25;

	/* level */
	chance += (LEVEL(ch) - LEVEL(victim)) * 2;

	if (chance % 5 == 0)
		chance += 1;

	/* terrain */

	switch(ch->in_room->sector_type) {
	case(SECT_INSIDE):		chance -= 20;	break;
	case(SECT_CITY):		chance -= 10;	break;
	case(SECT_FIELD):		chance +=  5;	break;
	case(SECT_FOREST):				break;
	case(SECT_HILLS):				break;
	case(SECT_MOUNTAIN):		chance -= 10;	break;
	case(SECT_WATER_SWIM):		chance  =  0;	break;
	case(SECT_WATER_NOSWIM):	chance  =  0;	break;
	case(SECT_AIR):			chance  =  0;  	break;
	case(SECT_DESERT):		chance += 10;   break;
	}

	if (chance == 0) {
		char_puts("There isn't any dirt to kick.\n",ch);
		return;
	}

	fighting = (ch->fighting != NULL);

	/* now the attack */
	if (number_percent() < chance) {
		AFFECT_DATA af;
		act("$n is blinded by the dirt in $s eyes!",
		    victim, NULL, NULL, TO_ROOM);
		char_puts("You can't see a thing!\n", victim);
		check_improve(ch, gsn_dirt, TRUE, 2);

		af.where	= TO_AFFECTS;
		af.type 	= gsn_dirt;
		af.level 	= ch->level;
		af.duration	= 0;
		af.location	= APPLY_HITROLL;
		af.modifier	= -4;
		af.bitvector 	= AFF_BLIND;

		affect_to_char(victim, &af);
		damage(ch, victim, number_range(2, 5),
		       gsn_dirt, DAM_NONE, FALSE);
	}
	else {
		damage(ch, victim, 0, gsn_dirt, DAM_NONE, TRUE);
		check_improve(ch, gsn_dirt, FALSE, 2);
	}

	if (check_yell(ch, victim, fighting))
		doprintf(do_yell, victim,
			 "Help! %s just kicked dirt into my eyes!",
			 PERS(ch,victim));
}

void do_trip(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	bool fighting;
	int chance;

	if (MOUNTED(ch)) {
		char_puts("You can't trip while riding!\n", ch);
		return;
	}

	one_argument(argument, arg, sizeof(arg));

	if ((chance = get_skill(ch,gsn_trip)) == 0) {
		char_puts("Tripping? What's that?\n", ch);
		return;
	}

	chance = chance * 2 /3;

	if (arg[0] == '\0') {
		victim = ch->fighting;
		if (victim == NULL) {
			char_puts("But you aren't fighting anyone!\n",ch);
			return;
 		}
	}
	else 
		victim = get_char_room(ch, arg);

	if (!victim || victim->in_room != ch->in_room) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("They aren't here.\n",ch);
		return;
	}

	if (MOUNTED(victim)) {
		char_puts("You can't trip a rider!\n", ch);
		return;
	}

	if (IS_AFFECTED(victim, AFF_FLYING)) {
		act("$S feet aren't on the ground.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (victim->position < POS_FIGHTING) {
		act("$N is already down.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (victim == ch) {
		act_puts("You fall flat on your face!",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		WAIT_STATE(ch, 2 * SKILL(gsn_trip)->beats);
		act("$n trips over $s own feet!", ch, NULL, NULL, TO_ROOM);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
		act("$N is your beloved master.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (is_safe(ch, victim))
		return;

	/* modifiers */

	/* size */
	if (ch->size < victim->size)
		chance += (victim->size - ch->size) * 10;

	/* dex */
	chance += get_curr_stat(ch, STAT_DEX) -20;
	chance -= (get_curr_stat(victim, STAT_DEX)-20) * 3 / 2;

	if (IS_AFFECTED(ch,AFF_FLYING))
		chance -= 10;

	/* speed */
	if ((IS_NPC(ch) && IS_SET(ch->pIndexData->off_flags, OFF_FAST))
	||  IS_AFFECTED(ch, AFF_HASTE))
		chance += 10;
	if ((IS_NPC(victim) && IS_SET(victim->pIndexData->off_flags, OFF_FAST))
	||  IS_AFFECTED(victim, AFF_HASTE))
		chance -= 20;

	/* level */
	chance += (LEVEL(ch) - LEVEL(victim)) * 2;

	RESET_WAIT_STATE(ch);
	fighting = (ch->fighting != NULL);

	WAIT_STATE(ch, SKILL(gsn_trip)->beats);

	if (!IS_NPC(ch) && !IS_NPC(victim))
		DEBUG(DEBUG_SKILL_TRIP,
			"%s[%d] tripping %s[%d]: %d%%",
			ch->name,
			LEVEL(ch),
			victim->name,
			LEVEL(victim),
			chance);

	chance = URANGE(20, chance, 80);

	/* now the attack */
	if (number_percent() < chance) {
		act("$n trips you and you go down!", ch, NULL, victim, TO_VICT);
		act("You trip $N and $N goes down!",
		    ch, NULL, victim, TO_CHAR);
		act("$n trips $N, sending $M to the ground.",
		    ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, gsn_trip, TRUE, 1);

		WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
		victim->position = POS_RESTING;
		damage(ch, victim, number_range(2, 2 + 2*victim->size),
		       gsn_trip, DAM_BASH, TRUE);
	}
	else {
		check_improve(ch, gsn_trip, FALSE, 1);
		damage(ch, victim, 0, gsn_trip, DAM_NONE, TRUE);
	}

	if (check_yell(ch, victim, fighting))
		doprintf(do_yell, victim,
			 "Help! %s just tripped me!", PERS(ch, victim));
}

bool backstab_ok(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (victim->fighting) {
		if (ch)
			char_puts("You can't backstab a fighting person.\n",
				  ch);
		return FALSE;
	}

	if (victim->hit < victim->max_hit * 6 / 10) {
		if (ch)
			act("$N is hurt and suspicious... "
			    "you couldn't sneak up.",
			    ch, NULL, victim, TO_CHAR);
		return FALSE;
	}

	return TRUE;
}

void backstab(CHAR_DATA *ch, CHAR_DATA *victim, int chance)
{
	if (!IS_AWAKE(victim)
	||  number_percent() < chance) {
		check_improve(ch, gsn_backstab, TRUE, 1);
		if (number_percent() <
				get_skill(ch, gsn_dual_backstab) * 8 / 10) {
			check_improve(ch, gsn_dual_backstab, TRUE, 1);
			one_hit(ch, victim, gsn_backstab, WEAR_WIELD);
			one_hit(ch, victim, gsn_dual_backstab, WEAR_WIELD);
		}
		else {
			check_improve(ch, gsn_dual_backstab, FALSE, 1);
			one_hit(ch, victim, gsn_backstab,WEAR_WIELD);
		}
	}
	else {
		check_improve(ch, gsn_backstab, FALSE, 1);
		damage(ch, victim, 0, gsn_backstab, DAM_NONE, TRUE);
	}

	if (!IS_NPC(victim) && victim->position == POS_FIGHTING) 
		doprintf(do_yell, victim,
			 "Die, %s! You are backstabbing scum!",
			 PERS(ch,victim));
}

void do_backstab(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	int chance;

	one_argument(argument, arg, sizeof(arg));

	if (MOUNTED(ch)) {
		char_puts("You can't backstab while riding!\n", ch);
		return;
	}

	if ((chance = get_skill(ch, gsn_backstab)) == 0) {
		char_puts("You don't know how to backstab.\n",ch);
		return;
	}

	if ((obj = get_eq_char(ch, WEAR_WIELD)) == NULL) {
		char_puts("You need to wield a weapon to backstab.\n", ch);
		return;
	}

	if (attack_table[obj->value[ITEM_WEAPON_ATTACK_TYPE]].damage != DAM_PIERCE) {
		char_puts("You need to wield a piercing weapon "
			  "to backstab.\n", ch);
		return;
	}

	if (arg[0] == '\0') {
		char_puts("Backstab whom?\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_backstab)->beats);

	if ((victim = get_char_room(ch, arg)) == NULL) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("How can you sneak up on yourself?\n", ch);
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (!backstab_ok(ch, victim))
		return;

	backstab(ch, victim, chance);
}

void do_knife(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *knife;
	int chance;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Knife whom?\n", ch);
		return;
	}
	
	if ((chance = get_skill(ch, gsn_knife)) == 0) {
		act("You don't know how to knife.", ch, NULL, NULL, TO_CHAR);
		return;
	}

	if ((knife = get_eq_char(ch, WEAR_WIELD)) == NULL) {
		act("You need a weapon.", ch, NULL, NULL, TO_CHAR);
		return;
	}

	if (knife->value[ITEM_WEAPON_TYPE] != WEAPON_DAGGER) {
		act("Your weapon must be dagger.", ch, NULL, NULL, TO_CHAR);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		return;
	}

	if (victim->fighting != NULL) {
		act("$N is fighting.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if(victim->hit >  victim->max_hit * 2 / 10) {
		if(ch)
			act("$N isn't wounded enough for a finishing blow.",
				ch,NULL,victim, TO_CHAR);
		return;
	}

	if (is_safe(ch, victim))
		return;

	WAIT_STATE(ch, SKILL(gsn_knife)->beats);

	if (number_percent()<chance) {
		one_hit(ch, victim, gsn_knife, WEAR_WIELD);
		check_improve(ch, gsn_knife, TRUE, 1);
	}
	else {
		damage(ch, victim, 0, gsn_knife,  DAM_NONE, TRUE);
		check_improve(ch, gsn_knife, FALSE, 1);
	}
}

void do_cleave(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	int chance;

	if (MOUNTED(ch)) {
		char_puts("You can't cleave while riding!\n", ch);
		return;
	}

	one_argument(argument, arg, sizeof(arg));

	if (ch->master != NULL && IS_NPC(ch))
		return;

	if ((chance = get_skill(ch, gsn_cleave)) == 0) {
		char_puts("You don't know how to cleave.\n",ch);
		return;
	}

	if (arg[0] == '\0') {
		char_puts("Cleave whom?\n", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("How can you sneak up on yourself?\n", ch);
		return;
	}

	if ((obj = get_eq_char(ch, WEAR_WIELD)) == NULL) {
		char_puts("You need to wield a weapon to cleave.\n", ch);
		return;
	}

	if (obj->value[ITEM_WEAPON_TYPE] != WEAPON_AXE 
	&& obj->value[ITEM_WEAPON_TYPE] != WEAPON_BASTARDSWORD
	&& obj->value[ITEM_WEAPON_TYPE] != WEAPON_LONGSWORD
	&& obj->value[ITEM_WEAPON_TYPE] != WEAPON_KATANA) {
		char_puts("You must wield axe or sword to cleave.\n", ch);
		return;
	}

	if (victim->fighting != NULL) {
		char_puts("You can't cleave a fighting person.\n", ch);
		return;
	}

	if ((victim->hit < (0.9 * victim->max_hit))
	&&  (IS_AWAKE(victim))) {
		act("$N is hurt and suspicious ... you can't sneak up.",
		    ch, NULL, victim, TO_CHAR);
		return;
	}

	if (is_safe(ch, victim))
		return;

	WAIT_STATE(ch, SKILL(gsn_cleave)->beats);

	if (!IS_AWAKE(victim)
	||  IS_NPC(ch)
	||  number_percent() < chance) {
		check_improve(ch, gsn_cleave, TRUE, 1);
		one_hit(ch, victim, gsn_cleave,WEAR_WIELD);
	}
	else {
		check_improve(ch, gsn_cleave, FALSE, 1);
		damage(ch, victim, 0, gsn_cleave, DAM_NONE, TRUE);
	}
	if (!IS_NPC(victim) && victim->position==POS_FIGHTING)
		doprintf(do_yell,victim,"Die, %s, you butchering fool!",PERS(ch,victim));
}

void do_ambush(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int chance;

	if (MOUNTED(ch)) {
		char_puts("You can't ambush while riding!\n", ch);
		return;
	}

	one_argument(argument, arg, sizeof(arg));

	if ((chance = get_skill(ch, gsn_ambush)) == 0) {
		char_puts("You don't know how to ambush.\n", ch);
		return;
	}

	if (arg[0] == '\0') {
		char_puts("Ambush whom?\n", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim->hit < victim->max_hit / 2) {
		if (ch)
			act("$N is hurt and suspicious... "
			    "you couldn't sneak up.",
			    ch, NULL, victim, TO_CHAR);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_ambush)->beats);

	if (victim == ch) {
		char_puts("How can you ambush yourself?\n", ch);
		return;
	}

	if (!IS_AFFECTED(ch, AFF_CAMOUFLAGE) || can_see(victim, ch)) {
		char_puts("But they can see you.\n", ch);
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (!IS_AWAKE(victim)
	||  IS_NPC(ch)
	||  number_percent() < chance) {
		check_improve(ch, gsn_ambush, TRUE, 1);
		one_hit(ch, victim, gsn_ambush,WEAR_WIELD);
	}
	else {
		check_improve(ch, gsn_ambush, FALSE, 1);
		damage(ch, victim, 0, gsn_ambush, DAM_NONE, TRUE);
	}
	if (!IS_NPC(victim) && victim->position==POS_FIGHTING)
		doprintf(do_yell,victim,"Help! I've been ambushed by %s!",PERS(ch,victim));
}

void do_rescue(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *fch;

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Rescue whom?\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_rescue)->beats);

	if ((victim = get_char_room(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("What about fleeing instead?\n", ch);
		return;
	}

	if (!IS_NPC(ch) && IS_NPC(victim)) {
		char_puts("Doesn't need your help!\n", ch);
		return;
	}

	if (ch->fighting == victim) {
		char_puts("Too late.\n", ch);
		return;
	}

	if ((fch = victim->fighting) == NULL) {
		char_puts("That person is not fighting right now.\n", ch);
		return;
	}

	if (IS_NPC(ch) && ch->master != NULL && IS_NPC(victim))
		return;

	if (ch->master != NULL && is_safe(ch->master, fch))
		return;

	if (is_safe(ch, fch))
		return;

	if (number_percent() > get_skill(ch,gsn_rescue)) {
		char_puts("You fail the rescue.\n", ch);
		check_improve(ch, gsn_rescue, FALSE, 1);
		return;
	}

	act("You rescue $N!",  ch, NULL, victim, TO_CHAR   );
	act("$n rescues you!", ch, NULL, victim, TO_VICT   );
	act("$n rescues $N!",  ch, NULL, victim, TO_NOTVICT);
	check_improve(ch, gsn_rescue, TRUE, 1);

	stop_fighting(ch, FALSE);
	stop_fighting(fch, FALSE);
	stop_fighting(victim, FALSE);

	set_fighting(ch, fch);
	set_fighting(fch, ch);
}

void do_kick(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	int kick_dam;
	int chance;

	if (MOUNTED(ch)) {
		char_puts("You can't kick while riding!\n", ch);
		return;
	}

	if ((chance = get_skill(ch, gsn_kick)) == 0) {
		char_puts("You better leave the martial arts to fighters.\n",
			  ch);
		return;
	}

	if ((victim = ch->fighting) == NULL) {
		char_puts("You aren't fighting anyone.\n", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_FLYING))
		chance = chance * 110 / 100;

	WAIT_STATE(ch, SKILL(gsn_kick)->beats);
	if (IS_NPC(ch) || number_percent() < chance) {
		kick_dam = number_range(1, LEVEL(ch));
		if (HAS_SKILL(ch, gsn_dishonor)
		&&  (get_eq_char(ch, WEAR_FEET) == NULL)) 
			kick_dam *= 2;
		kick_dam += ch->damroll / 2;
		damage(ch, victim, kick_dam, gsn_kick, DAM_BASH, TRUE);
		check_improve(ch, gsn_kick, TRUE, 1);
	}
	else {
		damage(ch, victim, 0, gsn_kick, DAM_BASH, TRUE);
		check_improve(ch, gsn_kick, FALSE, 1);
	}
}

void do_circle(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	CHAR_DATA *rch;
	int chance;
	OBJ_DATA *obj;

	if (MOUNTED(ch)) {
		char_puts("You can't circle while riding!\n", ch);
		return;
	}

	if ((chance = get_skill(ch, gsn_circle)) == 0) {
		char_puts("You don't know how to circle.\n", ch);
		return;
	}

	if ((victim = ch->fighting) == NULL) {
		char_puts("You aren't fighting anyone.\n", ch);
		return;
	}

	if ((obj = get_eq_char(ch, WEAR_WIELD)) == NULL
	||  attack_table[obj->value[ITEM_WEAPON_ATTACK_TYPE]].damage != DAM_PIERCE) {
		 char_puts("You must wield a piercing weapon to circle stab.\n",
			   ch);
		 return;
	}

	if (is_safe(ch, victim))
		return;

	WAIT_STATE(ch, SKILL(gsn_circle)->beats);

	for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
		if (rch->fighting == ch) {
			char_puts("You can't circle while defending yourself.\n",
				  ch);
			return;
		}
	}

	if (number_percent() < chance) {
		one_hit(ch, victim, gsn_circle, WEAR_WIELD);
		check_improve(ch, gsn_circle, TRUE, 1);
	}
	else {
		damage(ch, victim, 0, gsn_circle, TYPE_UNDEFINED, TRUE);
		check_improve(ch, gsn_circle, FALSE, 1);
	}
}

void do_disarm(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	OBJ_DATA *wield;
	OBJ_DATA *vwield;
	int chance, ch_weapon, vict_weapon;
	int loc = WEAR_WIELD;
	char arg[MAX_INPUT_LENGTH];
	int hth = 0;

	if (ch->master != NULL && IS_NPC(ch))
		return;

	if ((chance = get_skill(ch, gsn_disarm)) == 0) {
		char_puts("You don't know how to disarm opponents.\n", ch);
		return;
	}

	if ((wield = get_eq_char(ch, WEAR_WIELD)) == NULL 
	&&  (hth = get_skill(ch, gsn_hand_to_hand)) == 0) {
		char_puts("You must wield a weapon to disarm.\n", ch);
		return;
	}

	if ((victim = ch->fighting) == NULL) {
		char_puts("You aren't fighting anyone.\n", ch);
		return;
	}

	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] && !str_prefix(arg, "second"))
		loc = WEAR_SECOND_WIELD;

	if ((vwield = get_eq_char(victim, loc)) == NULL) {
		char_puts("Your opponent is not wielding a weapon.\n", ch);
		return;
	}

	/* find weapon skills */
	ch_weapon = get_weapon_skill(ch, get_weapon_sn(wield));
	vict_weapon = get_weapon_skill(victim, get_weapon_sn(vwield));

	/* modifiers */

	/* skill */
	if (wield == NULL)
		chance = chance * hth/150;
	else {
		chance = chance * ch_weapon/100;

		/* whips are easier to disarm with */
		if (wield->value[ITEM_WEAPON_TYPE] == WEAPON_WHIP)
			chance += 20;
	}

	chance += (ch_weapon/2 - vict_weapon) / 2; 

	/* dex vs. strength */
	chance += get_curr_stat(ch,STAT_DEX);
	chance -= 2 * get_curr_stat(victim,STAT_STR);

	/* level */
	chance += (LEVEL(ch) - LEVEL(victim)) * 2;

 
	/* and now the attack */
	WAIT_STATE(ch, SKILL(gsn_disarm)->beats);
	if (number_percent() < chance) {
		disarm(ch, victim, vwield);
		check_improve(ch, gsn_disarm, TRUE, 1);
	}
	else {
		act("You fail to disarm $N.", ch, NULL, victim, TO_CHAR);
		act("$n tries to disarm you, but fails.",
		    ch, NULL, victim, TO_VICT);
		act("$n tries to disarm $N, but fails.",
		    ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, gsn_disarm, FALSE, 1);
	}
}

void do_nerve(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	int chance;

	if (MOUNTED(ch)) {
		char_puts("You can't nerve while riding!\n", ch);
		return;
	}

	one_argument(argument, arg, sizeof(arg));

	if ((chance = get_skill(ch, gsn_nerve)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}
	
	if (arg[0] =='\0') {
		victim = ch->fighting;
		if (victim == NULL) {
			char_puts("You aren't fighting anyone.\n", ch);
			return;
		}
	}
	else if ((victim=get_char_room(ch, arg)) == NULL) {
			char_puts("They aren't here.\n", ch);
			return;
	}


	if (is_affected(victim, gsn_nerve)) {
		char_puts("You cannot weaken that character any more.\n",
			  ch);
		return;
	}

	if (is_safe(ch,victim))
		return;

	WAIT_STATE(ch, SKILL(gsn_nerve)->beats);

	if (IS_NPC(ch)
	||  number_percent() < (chance + ch->level 
			                 + get_curr_stat(ch,STAT_DEX))/2) {
		AFFECT_DATA af;
		af.where	= TO_AFFECTS;
		af.type 	= gsn_nerve;
		af.level 	= ch->level;
		af.duration	= ch->level * PULSE_VIOLENCE/PULSE_TICK;
		af.location	= APPLY_STR;
		af.modifier	= -3;
		af.bitvector	= 0;

		affect_to_char(victim,&af);
		act("You weaken $N with your nerve pressure.",
		    ch, NULL, victim, TO_CHAR);
		act("$n weakens you with $s nerve pressure.",
		    ch, NULL, victim, TO_VICT);
		act("$n weakens $N with $s nerve pressure.",
		    ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, gsn_nerve, TRUE, 1);
	}
	else {
		char_puts("You press the wrong points and fail.\n",ch);
		act("$n tries to weaken you with nerve pressure, but fails.",
		    ch, NULL, victim, TO_VICT);
		act("$n tries to weaken $N with nerve pressure, but fails.",
		    ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, gsn_nerve, FALSE, 1);
	}

	multi_hit(victim,ch,TYPE_UNDEFINED);
}

void do_endure(CHAR_DATA *ch, const char *argument)
{
	AFFECT_DATA af;
	int chance;

	if (IS_NPC(ch)) {
		char_puts("You have no endurance whatsoever.\n", ch);
		return;
	}

	if ((chance = get_skill(ch, gsn_endure)) == 0) {
		char_puts("You lack the concentration.\n", ch);
		return;
	}
		 
	if (is_affected(ch,gsn_endure)) {
		char_puts("You cannot endure more concentration.\n", ch);
		return;
	}
	
	WAIT_STATE(ch, SKILL(gsn_endure)->beats);

	af.where 	= TO_AFFECTS;
	af.type 	= gsn_endure;
	af.level 	= ch->level;
	af.duration	= ch->level / 4;
	af.location	= APPLY_SAVING_SPELL;
	af.modifier	= - chance / 10; 
	af.bitvector	= 0;

	affect_to_char(ch, &af);

	char_puts("You prepare yourself for magical encounters.\n", ch);
	act("$n concentrates for a moment, then resumes $s position.",
	    ch, NULL, NULL, TO_ROOM);
	check_improve(ch, gsn_endure, TRUE, 1);
}
 
void do_tame(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	int chance;

	if ((chance = get_skill(ch, gsn_tame)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("You are beyond taming.\n", ch);
		act("$n tries to tame $mself but fails miserably.",
		    ch, NULL, NULL, TO_ROOM);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_tame)->beats);
	
	if ((victim = get_char_room(ch,arg)) == NULL) {
		char_puts("They're not here.\n", ch);
		return;
	}

	if (IS_NPC(ch)) {
		char_puts("Why don't you tame yourself first?", ch);
		return;
	}

	if (!IS_NPC(victim)) {
		act("$N is beyond taming.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (!IS_AGGRO(victim, NULL)) {
		act("$N is not usually aggressive.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (number_percent() < chance + 15 + 4*(LEVEL(ch) - LEVEL(victim))) {
		SET_BIT(victim->affected_by, AFF_CALM);
		char_puts("You calm down.\n", victim);
		act("You calm $N down.", ch, NULL, victim, TO_CHAR);
		act("$n calms $N down.", ch, NULL, victim, TO_NOTVICT);
		stop_fighting(victim, TRUE);
		check_improve(ch, gsn_tame, TRUE, 1);
	}
	else {
		char_puts("You failed.\n",ch);
		act("$n tries to calm down $N but fails.",
		    ch, NULL, victim, TO_NOTVICT);
		act("$n tries to calm you down but fails.",
		    ch, NULL, victim, TO_VICT);
		check_improve(ch, gsn_tame, FALSE, 1);
	}
}

void do_assassinate(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int chance;

	if (MOUNTED(ch)) {
		char_puts("You can't assassinate while riding!\n", ch);
		return;
	}

	one_argument(argument, arg, sizeof(arg));

	if (ch->master != NULL && IS_NPC(ch))
		return;

	if ((chance = get_skill(ch, gsn_assassinate)) == 0) {
		char_puts("You don't know how to assassinate.\n", ch);
		return;
	}
	
	if (IS_AFFECTED(ch, AFF_CHARM))  {
		char_puts("You don't want to kill your beloved master.\n", ch);
		return;
	} 

	if (arg[0] == '\0') {
		char_puts("Assassinate whom?\n", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("Suicide is against your way.\n", ch);
		return;
	}

	if (IS_IMMORTAL(victim) && !IS_NPC(victim)) {
		char_puts("Your hands pass through.\n", ch);
		return;
	}

	if (victim->fighting != NULL) {
		char_puts("You can't assassinate a fighting person.\n", ch);
		return;
	}
 
	if ((get_eq_char(ch, WEAR_WIELD) != NULL)
	||  (get_eq_char(ch, WEAR_HOLD ) != NULL))  {
		char_puts("You need both hands free to assassinate somebody.\n",
			  ch);
		return;
	}
 
	if (victim->hit < victim->max_hit*0.9) {
		act("$N is hurt and suspicious ... you can't sneak up.",
		    ch, NULL, victim, TO_CHAR);
		return;
	}
	
/*
	if (IS_SET(victim->imm_flags, IMM_WEAPON)) {
		act("$N seems immune to your assassination attempt.",
		    ch, NULL, victim, TO_CHAR);
		act("$N seems immune to $n's assassination attempt.",
		    ch, NULL, victim, TO_ROOM);
		return;
	}
*/

	if (is_safe(ch, victim))
		return;

	WAIT_STATE(ch, SKILL(gsn_assassinate)->beats);

	if (number_percent() < chance
	&&  !IS_CLAN_GUARD(victim)
	&&  !IS_IMMORTAL(victim))
		one_hit(ch, victim, gsn_assassinate,WEAR_WIELD);
	else {
		check_improve(ch, gsn_assassinate, FALSE, 1);
		damage(ch, victim, 0, gsn_assassinate, DAM_NONE, TRUE);
	}
	if (!IS_NPC(victim) && victim->position==POS_FIGHTING) 
		doprintf(do_yell,victim,"Help! %s tries to assasinate me!",PERS(ch,victim));
}

void do_caltrops(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim = ch->fighting;
	int chance;

	if ((chance = get_skill(ch, gsn_caltrops)) == 0) {
		char_puts("Caltrops? Is that a dance step?\n", ch);
		return;
	}

	if (victim == NULL) {
		char_puts("You must be in combat.\n",ch);
		return;
	}

	if (is_safe(ch,victim))
		return;
		
	act("You throw a handful of sharp spikes at the feet of $N.",
	    ch, NULL, victim, TO_CHAR);
	act("$n throws a handful of sharp spikes at your feet!",
	    ch, NULL, victim, TO_VICT);

	WAIT_STATE(ch, SKILL(gsn_caltrops)->beats);

	if (!IS_NPC(ch) && number_percent() >= chance) {
		damage(ch, victim, 0, gsn_caltrops, DAM_PIERCE, TRUE);
		check_improve(ch, gsn_caltrops, FALSE, 1);
		return;
	}

	damage(ch, victim, LEVEL(ch), gsn_caltrops, DAM_PIERCE, TRUE);
	if (JUST_KILLED(victim))
		return;

	if (!is_affected(victim, gsn_caltrops)) {
		AFFECT_DATA tohit, todam, todex;

		tohit.where     = TO_AFFECTS;
		tohit.type      = gsn_caltrops;
		tohit.level     = ch->level;
		tohit.duration  = -1; 
		tohit.location  = APPLY_HITROLL;
		tohit.modifier  = -5;
		tohit.bitvector = 0;
		affect_to_char(victim, &tohit);

		todam.where = TO_AFFECTS;
		todam.type = gsn_caltrops;
		todam.level = ch->level;
		todam.duration = -1;
		todam.location = APPLY_DAMROLL;
		todam.modifier = -5;
		todam.bitvector = 0;
		affect_to_char(victim, &todam);

		todex.type = gsn_caltrops;
		todex.level = ch->level;
		todex.duration = -1;
		todex.location = APPLY_DEX;
		todex.modifier = -5;
		todex.bitvector = 0;
		affect_to_char(victim, &todex);

		act("$N starts limping.", ch, NULL, victim, TO_CHAR);
		act("You start to limp.", ch, NULL, victim, TO_VICT);
		check_improve(ch, gsn_caltrops, TRUE, 1);
	}
}

DECLARE_DO_FUN(do_throw_spear);

void do_throw(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	int chance;
	int prot_chance = IS_NPC(ch) ? 90 : 60;
	int dam = 0;

	if (MOUNTED(ch)) {
		char_puts("You can't throw while riding!\n", ch);
		return;
	}

	argument = one_argument(argument, arg, sizeof(arg));

	if (!str_cmp(arg, "spear")) {
		do_throw_spear(ch, argument);
		return;
	}

	if ((chance = get_skill(ch, gsn_throw)) == 0) {
		char_puts("A clutz like you couldn't throw down a worm.\n",
			  ch);
		return;
	}

	chance = chance * 2/3;

	if (IS_AFFECTED(ch, AFF_FLYING)) {
		char_puts("Your feet should touch the ground to balance\n",
			     ch);
		return;
	}

	if ((victim = ch->fighting) == NULL) {
		char_puts("You aren't fighting anyone.\n", ch);
		return;
	}

	if (victim->in_room != ch->in_room) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim) {
		act("But $N is your friend!", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (is_safe(ch,victim))
		return;

	WAIT_STATE(ch, SKILL(gsn_throw)->beats);

	if (is_affected(victim, gsn_protective_shield)) {
		if (number_percent() < prot_chance) {
			act_puts("You fail to reach $s arm.",ch,NULL,victim, TO_CHAR,
				 POS_FIGHTING);
			act_puts("$n fails to throw you.", ch, NULL, victim, TO_VICT,
				 POS_FIGHTING);
			act_puts("$n fails to throw $N.", ch, NULL, victim, TO_NOTVICT,
				 POS_FIGHTING);
			return;
		}
	}
	else 
		chance += (get_curr_stat(ch, STAT_DEX) 
			- get_curr_stat(victim,STAT_DEX)) * 3/2;

	/* stats */
	chance += get_curr_stat(ch,STAT_STR) 
		- get_curr_stat(victim,STAT_STR);

	if (IS_AFFECTED(victim, AFF_FLYING))
		chance += 10;

	/* speed */
	if (IS_NPC(ch) && IS_SET(ch->pIndexData->off_flags, OFF_FAST))
		chance += 10;
	if (IS_NPC(victim) && IS_SET(victim->pIndexData->off_flags, OFF_FAST))
		chance -= 20;

	/* level */
	chance += (LEVEL(ch) - LEVEL(victim)) * 2;

	if (!IS_NPC(ch) && !IS_NPC(victim))
		DEBUG(DEBUG_SKILL_THROW,
			"%s[%d] throwing %s[%d]: %d%%",
			ch->name,
			LEVEL(ch),
			victim->name,
			LEVEL(victim),
			chance);

	chance = URANGE(20, chance, 80);

	if (number_percent() < chance) {
		act("You throw $N to the ground with stunning force.",
		    ch, NULL, victim, TO_CHAR);
		act("$n throws you to the ground with stunning force.",
		    ch, NULL, victim, TO_VICT);
		act("$n throws $N to the ground with stunning force.",
		    ch, NULL, victim, TO_NOTVICT);
		WAIT_STATE(victim,2 * PULSE_VIOLENCE);

		dam = LEVEL(ch) + get_curr_stat(ch, STAT_STR);

		if (!IS_NPC(ch) && !IS_NPC(victim))
			DEBUG(DEBUG_SKILL_THROW,
				"thrown: %s[%d] vs %s[%d]: %ddam",
				ch->name,
				LEVEL(ch),
				victim->name,
				LEVEL(victim),
				dam);
		damage(ch, victim, dam, gsn_throw, DAM_BASH, TRUE);
		check_improve(ch, gsn_throw, TRUE, 1);
	}
	else {
		act("You fail to grab your opponent.",
		    ch, NULL, NULL, TO_CHAR);
		act("$N tries to throw you, but fails.",
		    victim, NULL, ch, TO_CHAR);
		act("$n tries to grab $N's arm.",
		    ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, gsn_throw, FALSE, 1);
	}
}

void do_strangle(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	AFFECT_DATA af;
	int chance;

	if (MOUNTED(ch)) {
		char_puts("You can't strangle while riding!\n", ch);
		return;
	}

	if ((chance = get_skill(ch, gsn_strangle)) == 0) {
		char_puts("You lack the skill to strangle.\n", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM))  {
		char_puts("You don't want to grap your beloved masters' neck.\n",
			  ch);
		return;
	} 


	if ((victim = get_char_room(ch,argument)) == NULL) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("You do not see that person here.\n", ch);
		return;
	}

	if (ch == victim) {
		char_puts("Even you are not that stupid.\n", ch);
		return;
	}

	if (is_affected(victim, gsn_strangle))
		return;

	if (is_safe(ch, victim))
		return;

	WAIT_STATE(ch, SKILL(gsn_strangle)->beats);

        if (is_affected(victim,gsn_paranoia))  {
                act("$E is way too paranoid for that.", ch, NULL, victim
, TO_CHAR);
                return;
        }

	SET_FIGHT_TIME(victim);
	SET_FIGHT_TIME(ch);
	
	if (number_percent() < chance * 6 /10
	&&  !IS_CLAN_GUARD(victim)
	&&  !IS_IMMORTAL(victim)) {
		act("You grab hold of $N's neck and put $M to sleep.",
		    ch, NULL, victim, TO_CHAR);
		act("$n grabs hold of your neck and puts you to sleep.",
		    ch, NULL, victim, TO_VICT);
		act("$n grabs hold of $N's neck and puts $M to sleep.",
		    ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, gsn_strangle, TRUE, 1);
		
		af.type = gsn_strangle;
		af.where = TO_AFFECTS;
		af.level = ch->level;
		af.duration = ch->level / 20 + 1;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = AFF_SLEEP;
		affect_join (victim,&af);

                af.type = gsn_paranoia;
                af.where        = TO_AFFECTS;
                af.level        = ch->level;
                af.duration     = number_fuzzy(af.duration);
                af.location     = APPLY_NONE;
                af.modifier     = 0;
                af.bitvector    = 0;
                affect_join (victim,&af);

		if (IS_AWAKE(victim))
			victim->position = POS_SLEEPING;
	}
	else {
		damage(ch,victim, 0, gsn_strangle, DAM_NONE, TRUE);
		check_improve(ch, gsn_strangle, FALSE, 1);
		if (!IS_NPC(victim)) doprintf(do_yell, victim,
			"Help! I'm being strangled by %s", PERS(ch,victim));
	}
}

void do_blackjack(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	AFFECT_DATA af;
	int chance;

	if (MOUNTED(ch)) {
		char_puts("You can't blackjack while riding!\n", ch);
		return;
	}

	if ((chance = get_skill(ch, gsn_blackjack)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	if ((victim = get_char_room(ch,argument)) == NULL) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("You do not see that person here.\n", ch);
		return;
	}

	if (ch == victim) {
		char_puts("You idiot?! Blackjack your self?!\n", ch);
		return;
	}

	if (MOUNTED(victim)) {
		act("You can't blackjack a riding person.\n", 
			ch, NULL, NULL, TO_CHAR);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM))  {
		char_puts("You don't want to hit your beloved masters' head with a lead filled jack.\n",
			  ch);
		return;
	} 

	if (IS_AFFECTED(victim,AFF_SLEEP))  {
		act("$E is already asleep.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (is_safe(ch,victim))
		return;

	WAIT_STATE(ch, SKILL(gsn_blackjack)->beats);

	if (is_affected(victim, gsn_paranoia))  {
		act("$E is way too paranoid for that.", ch, NULL, victim, TO_CHAR);
		return;
	}

	SET_FIGHT_TIME(victim);
	SET_FIGHT_TIME(ch);

	chance /= 2;
	chance += URANGE(0, (get_curr_stat(ch, STAT_DEX)-20)*2, 10);
	chance += can_see(victim, ch) ? 0 : 5;
	if (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
		chance -= 40;

	if (number_percent() < chance
	&&  !IS_CLAN_GUARD(victim)
	&&  !IS_IMMORTAL(victim)) {
		act("You hit $N's head with a lead filled sack.",
		    ch, NULL, victim, TO_CHAR);
		act("You feel a sudden pain erupt through your skull!",
		    ch, NULL, victim, TO_VICT);
		act("$n whacks $N at the back of $S head with a heavy looking sack!  *OUCH*",
		    ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, gsn_blackjack, TRUE, 1);
		
		af.type		= gsn_blackjack;
		af.where	= TO_AFFECTS;
		af.level	= ch->level;
		af.duration	= ch->level / 15 + 1;
		af.location	= APPLY_NONE;
		af.modifier	= 0;
		af.bitvector	= AFF_SLEEP;
		affect_join (victim,&af);

		af.type	= gsn_paranoia;
		af.where	= TO_AFFECTS;
		af.level	= ch->level;
                af.duration     = number_fuzzy(af.duration);
		af.location	= APPLY_NONE;
		af.modifier	= 0;
		af.bitvector	= 0;
		affect_join (victim,&af);

		if (IS_AWAKE(victim))
			victim->position = POS_SLEEPING;
	}
	else {
		damage(ch, victim, ch->level/2, gsn_blackjack, DAM_NONE, TRUE);
		check_improve(ch, gsn_blackjack, FALSE, 1);
		if (!IS_NPC(victim)) 
			doprintf(do_yell, victim, 
		 	 "Help! I'm being blackjacked by %s", PERS(ch, victim));
	}
}

void do_bloodthirst(CHAR_DATA *ch, const char *argument)
{
	int chance, hp_percent;
	int cost;

	cost = move_cost(ch, gsn_bloodthirst, 0, 0);

	if ((chance = get_skill(ch, gsn_bloodthirst)) == 0) {
		char_puts("You're not that thirsty.\n", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_BLOODTHIRST)) {
		char_puts("Your thirst for blood continues.\n", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CALM)) {
		char_puts("You're feeling to mellow to be bloodthirsty.\n",
			  ch);
		return;
	}

	if (ch->fighting == NULL) {
		char_puts("You need to be fighting.\n", ch);
		return;
	}

	/* modifiers */

	hp_percent = 100 * ch->hit/ch->max_hit;
	chance += 25 - hp_percent/2;

	if (number_percent() < chance) {
		AFFECT_DATA af;
		ch->move -= cost;

		WAIT_STATE(ch, PULSE_VIOLENCE);
	
		char_puts("You hunger for blood!\n", ch);
		act("$n gets a bloodthirsty look in $s eyes.",
		    ch, NULL, NULL, TO_ROOM);
		check_improve(ch, gsn_bloodthirst, TRUE, 2);

		af.where	= TO_AFFECTS;
		af.type		= gsn_bloodthirst;
		af.level	= ch->level;
		af.duration	= number_range(1, 4);
		af.modifier	= 5 + LEVEL(ch) / 4;
		af.bitvector 	= AFF_BLOODTHIRST;

		af.location	= APPLY_HITROLL;
		affect_to_char(ch, &af);

		af.location	= APPLY_DAMROLL;
		affect_to_char(ch, &af);

		af.modifier	= - UMIN(ch->level - 5, 35);
		af.location	= APPLY_AC;
		affect_to_char(ch, &af);
	}
	else {
		ch->move -= cost / 2;
		WAIT_STATE(ch,3 * PULSE_VIOLENCE);
		char_puts("You feel bloodthirsty for a moment, but it passes.\n",
			  ch);
		check_improve(ch, gsn_bloodthirst, FALSE, 2);
	}
}
/* Spellbane - autoskill now */
#if 0
void do_spellbane(CHAR_DATA *ch, const char *argument)
{
	AFFECT_DATA af;

	if (get_skill(ch, gsn_spellbane) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	if (is_affected(ch, gsn_spellbane)) {
		char_puts("You are already deflecting spells.\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_spellbane)->beats);

	af.where	= TO_AFFECTS;
	af.type 	= gsn_spellbane;
	af.level 	= ch->level;
	af.duration	= -1;
	af.location	= APPLY_SAVING_SPELL;
	af.modifier	= -LEVEL(ch)/4;
	af.bitvector	= 0;

	affect_to_char(ch, &af);

	act("Your hatred of magic surrounds you.", ch, NULL, NULL, TO_CHAR);
	act("$n fills the air with $s hatred of magic.",
	    ch, NULL, NULL, TO_ROOM);
}
#endif

void do_warcry(CHAR_DATA *ch, const char *argument)
{
	AFFECT_DATA af;
	int chance;
	int mana;
	
	if ((chance = get_skill(ch, gsn_warcry)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}
	
	if (is_affected(ch,gsn_bless) || is_affected(ch, gsn_warcry)) {
		char_puts("You are already blessed.\n",ch);
		return;
	}
	
	mana = SKILL(gsn_warcry)->min_mana;
	if (ch->mana < mana) {
		char_puts("You can't concentrate enough right now.\n",ch);
		return;
	}
	
	WAIT_STATE(ch, SKILL(gsn_warcry)->beats);
	
	if (number_percent() > chance) {
		char_puts("You grunt softly.\n", ch);
		act("$n makes some soft grunting noises.",
		    ch, NULL, NULL, TO_ROOM);
		ch->mana -= mana/2;
		check_improve(ch, gsn_warcry, FALSE, 1);
		return;
	}
	
	ch->mana -= mana;
	check_improve(ch, gsn_warcry, TRUE, 1);
 
	af.where	= TO_AFFECTS;
	af.type      = gsn_warcry;
	af.level	 = ch->level;
	af.duration  = 6+ch->level;
	af.location  = APPLY_HITROLL;
	af.modifier  = LEVEL(ch) / 8;
	af.bitvector = 0;
	affect_to_char(ch, &af);
	
	af.location  = APPLY_SAVING_SPELL;
	af.modifier  = 0 - LEVEL(ch) / 8;
	affect_to_char(ch, &af);
	char_puts("You feel righteous as you yell out your warcry.\n", ch);
}

void do_guard(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int chance;
	
	if ((chance = get_skill(ch, gsn_guard)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Guard whom?\n", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (IS_NPC(victim)) {
		act("$N doesn't need any of your help!", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (!str_cmp(arg, "none") || !str_cmp(arg, "self") || victim == ch) {
		if (ch->guarding == NULL) {
			char_puts("You can't guard yourself!\n", ch);
			return;
		}
		act("You stop guarding $N.", ch, NULL, ch->guarding, TO_CHAR);
		act("$n stops guarding you.", ch, NULL, ch->guarding, TO_VICT);
		act("$n stops guarding $N.", ch, NULL, ch->guarding, TO_NOTVICT);
		ch->guarding->guarded_by = NULL;
		ch->guarding             = NULL;
		return;
	}

	if (ch->guarding == victim) {
		act("You're already guarding $N!", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (ch->guarding != NULL) {
		char_puts("But you're already guarding someone else!\n", ch);
		return;
	}

	if (victim->guarded_by != NULL) {
		act("$N is already being guarded by someone.",
		    ch, NULL, victim, TO_CHAR);
		return;
	}

	if (victim->guarding == ch) {
		act("But $N is guarding you!", ch, NULL, victim, TO_CHAR);
		return;
	}

/*	if (!is_same_group(victim, ch)) {
		act("But you aren't in the same group as $N.",
		    ch, NULL, victim, TO_CHAR);
		return;
	}
*/
	if (IS_AFFECTED(ch,AFF_CHARM)) {
		act("You like your master too much to bother guarding $N!",
		    ch, NULL, victim, TO_VICT);
		return;
	}
/*
	if (victim->fighting != NULL) {
		char_puts("Why don't you let them stop fighting first?\n",
			     ch);
		return;
	}
	
	if (ch->fighting != NULL) {
		char_puts("Better finish your own battle before you "
			     "worry about guarding someone else.\n", ch);
		return;
	}
*/
	act("You are now guarding $N.", ch, NULL, victim, TO_CHAR);
	act("You are being guarded by $n.", ch, NULL, victim, TO_VICT);
	act("$n is now guarding $N.", ch, NULL, victim, TO_NOTVICT);

	ch->guarding = victim;
	victim->guarded_by = ch;
}

CHAR_DATA *check_guard(CHAR_DATA *ch, CHAR_DATA *mob)
{
	int chance;

	if (ch->guarded_by == NULL ||
		get_char_room(ch,ch->guarded_by->name) == NULL)
		return ch;
	else {
		chance = (get_skill(ch->guarded_by,gsn_guard) - 
			(1.5 * (ch->level - mob->level)));
		if (number_percent() < chance) {
			act("$n jumps in front of $N!",
			    ch->guarded_by, NULL, ch, TO_NOTVICT);
			act("$n jumps in front of you!",
			    ch->guarded_by, NULL, ch, TO_VICT);
			act("You jump in front of $N!",
			    ch->guarded_by, NULL, ch, TO_CHAR);
			check_improve(ch->guarded_by, gsn_guard, TRUE, 3);
			return ch->guarded_by;
		}
		else {
			check_improve(ch->guarded_by, gsn_guard, FALSE, 3);
			return ch;
		}
	}
}

void do_explode(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim = ch->fighting;
	CHAR_DATA *vch, *vch_next;
	int dam=0,hp_dam,dice_dam,mana;
	int hpch,level= LEVEL(ch);
	char arg[MAX_INPUT_LENGTH];
	int chance;

	if ((chance = get_skill(ch, gsn_explode)) == 0) {
		char_puts("Flame? What is that?\n", ch);
		return;
	}

	if (ch->fighting)
		victim = ch->fighting;
	else {
		one_argument(argument, arg, sizeof(arg));
		if (arg[0] == '\0') { 
			char_puts("You play with the exploding material.\n",
				  ch);
			return;
		}
	}

	if (!victim || victim->in_room != ch->in_room) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("They aren't here.\n", ch);
		return;
	}

	mana = SKILL(gsn_explode)->min_mana;
	if (ch->mana < mana) {
		char_puts("You can't find that much energy to fire\n", ch);
		return;
	}
	ch->mana -= mana;
	
	WAIT_STATE(ch, SKILL(gsn_explode)->beats);

	act("$n burns something.", ch, NULL, victim, TO_NOTVICT);
	act("$n burns a cone of exploding material over you!",
	    ch, NULL, victim, TO_VICT);
	act("Burn them all!.", ch, NULL, NULL, TO_CHAR);

	if (number_percent() >= chance) {
		damage(ch, victim, 0, gsn_explode, DAM_FIRE, TRUE);
		check_improve(ch, gsn_explode, FALSE, 1);
		return;
	}

	hpch = UMAX(10, ch->hit);
	hp_dam  = number_range(hpch/9+1, hpch/5);
	dice_dam = dice(level,20);

	if (!is_safe(ch,victim)) {
		dam = UMAX(hp_dam + dice_dam /10, dice_dam + hp_dam / 10);
		fire_effect(victim->in_room, level, dam/2, TARGET_ROOM);
	}

	for (vch = victim->in_room->people; vch != NULL; vch = vch_next) {
		vch_next = vch->next_in_room;

		if (is_safe_spell(ch,vch,TRUE)
		||  (IS_NPC(vch) && IS_NPC(ch)
		&&   (ch->fighting != vch || vch->fighting != ch)))
			  continue;

		if (vch == victim) { /* full damage */
			fire_effect(vch, level, dam, TARGET_CHAR);
			damage(ch, vch, dam, gsn_explode, DAM_FIRE, TRUE);
		}
		else { /* partial damage */
			fire_effect(vch, level/2, dam/4, TARGET_CHAR);
			damage(ch, vch, dam/2, gsn_explode, DAM_FIRE,TRUE);
		}
	}

	if (number_percent() >= chance) {
		fire_effect(ch, level/4, dam/10, TARGET_CHAR);
		damage(ch, ch, (ch->hit / 10), gsn_explode, DAM_FIRE, TRUE);
	}
}

void do_target(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	int chance;

	if ((chance = get_skill(ch, gsn_target)) == 0) {
		char_puts("You don't know how to change the target "
			  "while fighting a group.\n" ,ch);
		return;
	}

	if (ch->fighting == NULL) {
		char_puts("You aren't fighting yet.\n",ch);
		return;
	}

	if (argument[0] == '\0') {
		char_puts("Change target to whom?\n",ch);
		return;
	}

	if ((victim = get_char_room(ch, argument)) == NULL) {
		char_puts("You don't see that item.\n",ch);
		return;
	}

	/* check victim is fighting with him */

	if (victim->fighting != ch) {
		char_puts ("Target is not fighting with you.\n",ch);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_target)->beats);

	if (number_percent() < chance / 2) {
		check_improve(ch, gsn_target, TRUE, 1);

		ch->fighting = victim;

		act("$n changes $s target to $N!",
		    ch, NULL, victim, TO_NOTVICT);
		act("You change your target to $N!",
		    ch, NULL, victim, TO_CHAR);
		act("$n changes target to you!", ch, NULL, victim, TO_VICT);
		return;
	}

	char_puts("You tried, but you couldn't. "
		  "But for honour try again!.\n", ch);
	check_improve(ch, gsn_target, FALSE, 1);
}

void do_tiger(CHAR_DATA *ch, const char *argument)
{
	int chance, hp_percent;
	int mana;
	int cost;

	cost = move_cost(ch, gsn_tiger_power, 0, 0);

	if ((chance = get_skill(ch, gsn_tiger_power)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}
	act("$n calls forth the tigers spirit to join with him!.", ch, NULL, NULL, TO_ROOM);

	if (IS_AFFECTED(ch, AFF_BERSERK) || is_affected(ch, gsn_berserk)
	||  is_affected(ch, gsn_tiger_power) || is_affected(ch, gsn_frenzy)) {
		char_puts("You get a little madder.\n", ch);
		return;
	}

	if (IS_AFFECTED(ch,AFF_CALM)) {
		char_puts("You're feeling too mellow to call upon the tigers spirit.\n",
			  ch);
		return;
	}

	if (ch->in_room->sector_type != SECT_FIELD
	&&  ch->in_room->sector_type != SECT_FOREST
	&&  ch->in_room->sector_type != SECT_MOUNTAIN
	&&  ch->in_room->sector_type != SECT_HILLS) {
		char_puts("The tigers spirits cannot hear your call.\n", ch);
		return;
	}

	mana = SKILL(gsn_tiger_power)->min_mana;
	if (ch->mana < mana) {
		char_puts("You can't get up enough energy.\n", ch);
		return;
	}

	/* modifiers */

	/* fighting */
	if (ch->position == POS_FIGHTING)
		chance += 10;

	hp_percent = 100 * ch->hit/ch->max_hit;
	chance += 25 - hp_percent/2;

	if (number_percent() < chance) {
		AFFECT_DATA af;


		WAIT_STATE(ch, SKILL(gsn_tiger_power)->beats);
		ch->mana -= mana;
		ch->move -= cost;

		/* heal a little damage */
		ch->hit += LEVEL(ch) * 15;
		ch->hit = UMIN(ch->hit, ch->max_hit);

		char_puts("The spirits of the Tigers join you as you call upon them!\n",
			  ch);
		act("The tigers spirits come across $n, and connect with $n.",
		    ch, NULL, NULL, TO_ROOM);
		check_improve(ch, gsn_tiger_power, TRUE, 15);

		af.where	= TO_AFFECTS;
		af.type		= gsn_tiger_power;
		af.level	= ch->level;
		af.duration	= number_fuzzy(ch->level / 8);
		af.modifier	= UMAX(1, LEVEL(ch)/5);
		af.bitvector 	= AFF_BERSERK;

		af.location	= APPLY_HITROLL;
		affect_to_char(ch,&af);

		af.location	= APPLY_DAMROLL;
		affect_to_char(ch,&af);

		af.modifier	= UMAX(10,10 * (LEVEL(ch)/5));
		af.location	= APPLY_AC;
		affect_to_char(ch,&af);
	}
	else {
		WAIT_STATE(ch, 2 * SKILL(gsn_tiger_power)->beats);
		ch->mana -= mana/15;
		ch->move -= cost / 2;
		char_puts("Your feel the presence of the tigers spirits,"
			" but they do not join with you.\n",
			  ch);
		check_improve(ch, gsn_tiger_power, FALSE, 15);
	}
}

void do_hara(CHAR_DATA *ch, const char *argument)
{
	int chance;
	AFFECT_DATA  af;

	if (MOUNTED(ch)) {
		char_puts("You can't harakiri while riding!\n", ch);
		return;
	}

	if ((chance = get_skill(ch,gsn_hara_kiri)) == 0) {
		char_puts("You try to kill yourself, but you can't resist this ache.\n",ch);
		return;
	}

	if (is_affected(ch, gsn_hara_kiri)) {
		char_puts("If you want to kill yourself go and try to kill Questus.\n",ch);
		return;
	}

	/* fighting */
	if (ch->position == POS_FIGHTING) {
		char_puts("The battle is not yet lost.\n", ch);
		return;
	}

	if (number_percent() < chance) {
		AFFECT_DATA af;

		WAIT_STATE(ch, SKILL(gsn_hara_kiri)->beats);

		ch->hit = 1;
		ch->mana = 1;
		ch->move = 1;

		if (ch->pcdata->condition[COND_HUNGER] < 40) 
			ch->pcdata->condition[COND_HUNGER] = 40; 
		if (ch->pcdata->condition[COND_THIRST] < 40) 
			ch->pcdata->condition[COND_THIRST] = 40; 

		char_puts("You slice into your side and let your blood flow.\n",
			  ch);
		act_puts("$n cuts $s body and takes on a grim aura.",
			 ch, NULL, NULL, TO_ROOM, POS_FIGHTING);
		check_improve(ch, gsn_hara_kiri, TRUE, 2);
		do_sleep(ch, str_empty);
		SET_BIT(ch->state_flags,STATE_HARA_KIRI);

		af.where     = TO_AFFECTS;
		af.type      = gsn_hara_kiri;
		af.level     = ch->level;
		af.duration  = 10;
		af.location  = APPLY_NONE;
		af.modifier  = 0;
		af.bitvector = 0;
		affect_to_char(ch, &af);
	}
	else {
		WAIT_STATE(ch, 2 * SKILL(gsn_hara_kiri)->beats);

		af.where     = TO_AFFECTS;
		af.type      = gsn_hara_kiri;
		af.level     = ch->level;
		af.duration  = 0;
		af.location  = APPLY_NONE;
		af.modifier  = 0;
		af.bitvector = 0;
		affect_to_char(ch, &af);

		char_puts("The cut is not deep enough.\n",ch);
		check_improve(ch, gsn_hara_kiri, FALSE, 2);
	}
}

/*  
 * critical strike
 */
int critical_strike(CHAR_DATA *ch, CHAR_DATA *victim, int dam)
{
	int diceroll;
	AFFECT_DATA baf;
	int chance;

	if (IS_NPC(ch))
	  return dam;	

	if (get_eq_char(ch, WEAR_WIELD) != NULL 
	&&  get_eq_char(ch, WEAR_SECOND_WIELD) != NULL
	&&  number_percent() > ((ch->hit * 100) / ch->max_hit)) 
		return 0;

	if ((chance = get_skill(ch, gsn_critical)) == 0)
		return dam;
	
	diceroll = number_range(0, 100);
	if (LEVEL(victim) > LEVEL(ch))
		diceroll += (LEVEL(victim) - LEVEL(ch));
	else
		diceroll -= (LEVEL(ch) - LEVEL(victim));
 
	if (diceroll <= chance /2)  {  
		check_improve(ch, gsn_critical, TRUE, 2);
		dam += dam * diceroll/200;
	}  

	if (diceroll > chance / 13)
		return dam;

	diceroll = number_percent();
	if (diceroll <= 75) {  
		act_puts("You take $N down with a weird judo move!", 
			 ch, NULL, victim, TO_CHAR, POS_DEAD);
		act("$n takes you down with a weird judo move!", 
		    ch, NULL, victim, TO_VICT);
		act("$n takes $N down with a weird judo move!", 
		    ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, gsn_critical, TRUE, 3);
		WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
		dam += (dam * number_range(2, 5)) / 5;
		return dam;
	}   
	else if (diceroll > 75 && diceroll < 95) {   
		act_puts("You blind $N with your attack!",
			 ch, NULL, victim, TO_CHAR, POS_DEAD);
		act("You are blinded by $n's attack!",
		    ch, NULL, victim, TO_VICT);
		act("$N is blinded by $n's attack!",
		    ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, gsn_critical, TRUE, 4);
		if (!IS_AFFECTED(victim, AFF_BLIND)) {
			baf.where = TO_AFFECTS;
			baf.type = gsn_critical;
			baf.level = ch->level; 
			baf.location = APPLY_HITROLL; 
			baf.modifier = -4;
			baf.duration = number_range(1, 3); 
			baf.bitvector = AFF_BLIND;
			affect_to_char(victim, &baf);
		}  
		dam += dam * (number_range(10, 15) / 10);			
		return dam;
	} 

	act_puts("You cut out $N's {Rheart{x! I bet that hurt!",  
		 ch, NULL, victim, TO_CHAR ,POS_RESTING);
	act_puts("$n cuts out your {Rheart{x! OUCH!!",  
		 ch, NULL, victim, TO_VICT ,POS_RESTING); 
	act_puts("$n cuts out $N's {Rheart{x! I bet that hurt!",  
		 ch, NULL, victim, TO_NOTVICT ,POS_RESTING); 
	check_improve(ch, gsn_critical, TRUE, 5);
	dam += dam * number_range(10, 30) / 12;
	return dam;
}  

void do_shield(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	int chance,ch_weapon,vict_shield;
	OBJ_DATA *shield,*axe;

	if (IS_NPC(ch))
		return;

	if ((victim = ch->fighting) == NULL) {
		char_puts("You aren't fighting anyone.\n", ch);
		return;
	}
		
	if (victim->in_room != ch->in_room) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if ((axe = get_eq_char(ch,WEAR_WIELD)) == NULL) {
		char_puts("You must be wielding a weapon.\n", ch);
		return;
	}

	if ((chance = get_skill(ch, gsn_shield_cleave)) == 0) {
		char_puts("You don't know how to cleave opponents's shield.\n",
			  ch);
		return;
	}

	if ((shield = get_eq_char(victim, WEAR_SHIELD)) == NULL) {
		char_puts("Your opponent must wield a shield.\n", ch);
		return;
	}

	if (check_material(shield,"platinum")
	||  shield->pIndexData->limit != -1)
		return;

	if (axe->value[ITEM_WEAPON_TYPE] == WEAPON_AXE)
		chance *= 1.2;
	else if (axe->value[ITEM_WEAPON_TYPE] != WEAPON_LONGSWORD
		&& axe->value[ITEM_WEAPON_TYPE] != WEAPON_BASTARDSWORD
		&& axe->value[ITEM_WEAPON_TYPE] != WEAPON_KATANA) {
		char_puts("Your weapon must be an axe or a sword.\n", ch);
		return;
	}

	/* find weapon skills */
	ch_weapon = get_weapon_skill(ch, get_weapon_sn(axe));
	vict_shield = get_skill(ch, gsn_shield_block);
	/* modifiers */

	/* skill */
	chance = chance * ch_weapon / 200;
	if (vict_shield)
		chance = chance * 100 / vict_shield;

	/* dex vs. strength */
	chance += get_curr_stat(ch, STAT_DEX);
	chance -= 2 * get_curr_stat(victim, STAT_STR);

	/* level */
/*	chance += (ch->level - victim->level) * 2; */
	chance += LEVEL(ch) - LEVEL(victim);
	chance += axe->level - shield->level;
 
	/* and now the attack */
	SET_BIT(ch->affected_by, AFF_WEAK_STUN);
	WAIT_STATE(ch, SKILL(gsn_shield_cleave)->beats);
	if (number_percent() < chance) {
		act("You cleaved $N's shield into two.",
		    ch, NULL, victim, TO_CHAR);
		act("$n cleaved your shield into two.",
		    ch, NULL, victim, TO_VICT);
		act("$n cleaved $N's shield into two.",
		    ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, gsn_shield_cleave, TRUE, 1);
		extract_obj(get_eq_char(victim, WEAR_SHIELD), 0);
	}
	else {
		act("You fail to cleave $N's shield.",
		    ch, NULL, victim, TO_CHAR);
		act("$n tries to cleave your shield, but fails.",
		    ch, NULL, victim, TO_VICT);
		act("$n tries to cleave $N's shield, but fails.",
		    ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, gsn_shield_cleave, FALSE, 1);
	}
}

void do_weapon(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	OBJ_DATA *wield,*axe;
	int chance,ch_weapon,vict_weapon;

	if (IS_NPC(ch))
		return;

	if ((victim = ch->fighting) == NULL) {
		char_puts("You aren't fighting anyone.\n", ch);
		return;
	}

	if ((axe = get_eq_char(ch, WEAR_WIELD)) == NULL) {
		char_puts("You must be wielding a weapon.\n",ch);
		return;
	}

	if ((chance = get_skill(ch, gsn_weapon_cleave)) == 0) {
		char_puts("You don't know how to cleave opponents's weapon.\n",ch);
		return;
	}

	if ((wield = get_eq_char(victim, WEAR_WIELD)) == NULL) {
		char_puts("Your opponent must wield a weapon.\n", ch);
		return;
	}

	if (IS_OBJ_STAT(wield, ITEM_QUEST))
	{
		char_puts("Your opponents weapon is impervious to your attack!\n", ch);
		return;
	}

	if (check_material(wield,"platinum") || wield->pIndexData->limit != -1)
		return;


	if (axe->value[ITEM_WEAPON_TYPE] == WEAPON_AXE)
		chance *= 1.2;
	else if (axe->value[ITEM_WEAPON_TYPE] != WEAPON_LONGSWORD
		&& axe->value[ITEM_WEAPON_TYPE] != WEAPON_BASTARDSWORD
		&& axe->value[ITEM_WEAPON_TYPE] != WEAPON_KATANA) {
		char_puts("Your weapon must be an axe or a sword.\n",ch);
		return;
	}

	/* find weapon skills */
	ch_weapon = get_weapon_skill(ch, get_weapon_sn(axe));
	vict_weapon = get_weapon_skill(victim, get_weapon_sn(wield));
	/* modifiers */

	/* skill */
	chance = chance * ch_weapon / 200;
	chance = chance * 101 / (vict_weapon+1);

	/* staffs easier to cleave */
	if (wield->value[ITEM_WEAPON_TYPE] == WEAPON_STAFF)
		chance += 10;

	/* dex vs. strength */
	chance += get_curr_stat(ch, STAT_DEX) + get_curr_stat(ch, STAT_STR);
	chance -= get_curr_stat(victim, STAT_STR) +
				2 * get_curr_stat(victim, STAT_DEX);

	chance += LEVEL(ch) - LEVEL(victim);
	chance += axe->level - wield->level;
 
	/* and now the attack */
	SET_BIT(ch->affected_by,AFF_WEAK_STUN);
	WAIT_STATE(ch, SKILL(gsn_weapon_cleave)->beats);
	if (number_percent() < chance) {
		act("You cleaved $N's weapon into two.",
		    ch, NULL, victim, TO_CHAR);
		act("$n cleaved your weapon into two.",
		    ch, NULL, victim, TO_VICT);
		act("$n cleaved $N's weapon into two.",
		    ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, gsn_weapon_cleave, TRUE, 1);
		extract_obj(get_eq_char(victim, WEAR_WIELD), 0);
	}
	else {
		act("You fail to cleave $N's weapon.",
		    ch, NULL, victim, TO_CHAR);
		act("$n tries to cleave your weapon, but fails.",
		    ch, NULL, victim, TO_VICT);
		act("$n tries to cleave $N's weapon, but fails.",
		    ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, gsn_weapon_cleave, FALSE, 1);
	}
}

void do_tail(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int chance, wait;
	int damage_tail;
	int prot_chance = IS_NPC(ch) ? 90 : 60;

	if (MOUNTED(ch)) {
		char_puts("You can't tail while riding!\n", ch);
		return;
	}

	one_argument(argument, arg, sizeof(arg));
 
	if ((chance = get_skill(ch, gsn_tail)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	chance = chance / 2;
 
	if (arg[0] == '\0') {
		victim = ch->fighting;
		if (victim == NULL) {
			char_puts("But you aren't fighting anyone!\n", ch);
			return;
		}
	}
	else
		victim = get_char_room(ch, arg);

	if (!victim || victim->in_room != ch->in_room) {
		char_puts("They aren't here.\n", ch);
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_tail)->beats);

	if (victim->position < POS_FIGHTING) {
		act("You'll have to let $M get back up first.",
		    ch, NULL, victim, TO_CHAR);
		return;
	} 

	if (victim == ch) {
		char_puts("You try to hit yourself by your tail, but failed.\n",
			  ch);
		return;
	}

	if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim) {
		act("But $N is your friend!", ch, NULL, victim, TO_CHAR);
		return;
	}
		
	if (is_affected(victim, gsn_protective_shield)) {
		if (number_percent() < prot_chance) {
			act_puts("Your tail seems to slide around $N.",
				 ch, NULL, victim, TO_CHAR, POS_FIGHTING);
			act_puts("$n's tail slides off your protective shield.",
				 ch, NULL, victim, TO_VICT, POS_FIGHTING);
			act_puts("$n's tail seems to slide around $N.",
				 ch, NULL, victim, TO_NOTVICT, POS_FIGHTING);
			return;
		}
	}
	else {
		chance -= get_curr_stat(victim, STAT_DEX);
	}

	if (is_safe(ch, victim))
		return;

	/* modifiers */

	if (ch->size < victim->size)
		chance += (ch->size - victim->size) * 5;
	else
		chance += (ch->size - victim->size) * 2; 

	/* stats */
	chance += get_curr_stat(ch, STAT_STR) + get_curr_stat(ch, STAT_DEX);

	if (IS_AFFECTED(ch, AFF_FLYING))
		chance -= 10;

	/* speed */
	if (IS_NPC(ch) && IS_SET(ch->pIndexData->off_flags, OFF_FAST))
		chance += 20;
	if (IS_NPC(victim) && IS_SET(victim->pIndexData->off_flags, OFF_FAST))
		chance -= 30;

	/* level */
	chance += (LEVEL(ch) - LEVEL(victim)) * 2;

	RESET_WAIT_STATE(ch);

	/* now the attack */
	if (number_percent() < (chance / 4)) {
		act("$n sends you sprawling with a powerful tail!",
		    ch, NULL, victim, TO_VICT);
		act("You sprawle $N with your tail , and send $M flying!",
		    ch, NULL, victim, TO_CHAR);
		act("$n sends $N sprawling with a powerful tail.",
		    ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, gsn_tail, TRUE, 1);

		wait = 2;

		switch(number_bits(2)) {
		case 0: 
		case 1:
			wait = 1; break;
		case 2:
		case 3: 
			wait = 2; break;
		}

		WAIT_STATE(victim, wait * PULSE_VIOLENCE);
		WAIT_STATE(ch, SKILL(gsn_tail)->beats);
		victim->position = POS_RESTING;
		damage_tail = ch->damroll + 
			(2 * number_range(4,4 + 10* ch->size + chance/10));
		damage(ch, victim, damage_tail, gsn_tail, DAM_BASH, TRUE);
	}
	else {
		damage(ch,victim,0,gsn_tail,DAM_BASH, TRUE);
		act("You lost your position and fall down!",
		    ch, NULL, victim, TO_CHAR);
		act("$n lost $s position and fall down!.",
		    ch, NULL, victim, TO_NOTVICT);
		act("You evade $n's tail, causing $m to fall down.",
		    ch, NULL, victim, TO_VICT);
		check_improve(ch,gsn_tail,FALSE,1);
		ch->position = POS_RESTING;
		WAIT_STATE(ch, SKILL(gsn_tail)->beats * 3/2); 
	}
}

void do_concentrate(CHAR_DATA *ch, const char *argument)
{
	int chance;
	int mana;
	int cost;

	cost = move_cost(ch, gsn_concentrate, STAT_INT, 1);

	if (MOUNTED(ch)) {
		char_puts("You can't concentrate while riding!\n", ch);
		return;
	}

	if ((chance = get_skill(ch,gsn_concentrate)) == 0) {
		char_puts("You try to concentrate on what is going on.\n",
			  ch);
		return;
	}

	if (is_affected(ch,gsn_concentrate)) {
		char_puts("You are already concentrated for the fight.\n",
			  ch);
		return;
	}
		
	mana = SKILL(gsn_concentrate)->min_mana;
	if (ch->mana < mana) {
		char_puts("You can't get up enough energy.\n", ch);
		return;
	}

	/* fighting */
	if (ch->fighting) {
		char_puts("Concentrate on your fighting!\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_concentrate)->beats);
	if (number_percent() < chance) {
		AFFECT_DATA af;

		ch->mana -= mana;
		ch->move -= cost;

		do_sit(ch,str_empty);
		char_puts("You sit down and relax, "
			  "concentrating on the next fight.!\n", ch);
		act_puts("$n concentrates for the next fight.",
			 ch, NULL, NULL, TO_ROOM, POS_FIGHTING);
		check_improve(ch,gsn_concentrate,TRUE,2);

		af.where	= TO_AFFECTS;
		af.type		= gsn_concentrate;
		af.level	= ch->level;
		af.duration	= number_fuzzy(ch->level / 8);
		af.modifier	= UMAX(1, LEVEL(ch)/8);
		af.bitvector 	= 0;

		af.location	= APPLY_HITROLL;
		affect_to_char(ch,&af);

		af.location	= APPLY_DAMROLL;
		affect_to_char(ch,&af);

		af.modifier	= UMAX(1,ch->level/10);
		af.location	= APPLY_AC;
		affect_to_char(ch,&af);
	}
	else {
		ch->mana -= mana/2;
		ch->move -= cost/2;
		char_puts("You try to concentrate for the next fight but fail.\n",
			  ch);
		check_improve(ch, gsn_concentrate, FALSE, 2);
	}
}

void do_bandage(CHAR_DATA *ch, const char *argument)
{
	int heal;
	int chance;

	if ((chance = get_skill(ch,gsn_bandage)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_REGENERATION)) {
		char_puts("You have already using your bandage.\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_bandage)->beats);
	if (number_percent() < chance) {
		AFFECT_DATA af;

		char_puts("You place your bandage to your shoulder!\n", ch);
		act("$n places a bandage to $s shoulder.",
		    ch, NULL, NULL, TO_ROOM);
		check_improve(ch, gsn_bandage, TRUE, 2);

		heal = dice(4, 8) + LEVEL(ch) / 2;
		ch->hit = UMIN(ch->hit + heal, ch->max_hit);
		update_pos(ch);
		char_puts("You feel better!\n", ch);

		af.where	= TO_AFFECTS;
		af.type		= gsn_bandage;
		af.level	= ch->level;
		af.duration	= ch->level / 10;
		af.modifier	= UMIN(15,ch->level/2);
		af.bitvector 	= AFF_REGENERATION;
		af.location	= 0;
		affect_to_char(ch,&af);
	}
	else {
		char_puts("You failed to place your bandage to your shoulder.\n",
			  ch);
		check_improve(ch, gsn_bandage, FALSE, 2);
	}
}

void do_crush(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	int chance, wait;
	int damage_crush;
	int prot_chance = IS_NPC(ch) ? 90 : 60;

	if (MOUNTED(ch)) 
		return;

	if ((chance = get_skill(ch, gsn_crush)) == 0) {
		char_puts("You don't know how to crush.\n", ch);
		return;
	}
 
	if ((victim = ch->fighting) == NULL || victim->in_room != ch->in_room)
		return;

	if (victim->position < POS_FIGHTING)
		return;

	if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
		return;
		
	if (is_affected(victim, gsn_protective_shield)
		&& number_percent() < prot_chance) {
		act_puts("Your crush seems to slide around $N.",
			 ch, NULL, victim, TO_CHAR, POS_FIGHTING);
		act_puts("$n's crush slides off your protective shield.",
			 ch, NULL, victim, TO_VICT, POS_FIGHTING);
		act_puts("$n's crush seems to slide around $N.",
			 ch, NULL, victim, TO_NOTVICT, POS_FIGHTING);
		return;
	}
		
	if (is_safe(ch, victim))
		return;

	/* modifiers */

	/* size  and weight */
	chance += ch->carry_weight / 25;
	chance -= victim->carry_weight / 20;

	if (ch->size < victim->size)
		chance += (ch->size - victim->size) * 25;
	else
		chance += (ch->size - victim->size) * 10; 

	/* stats */
	chance += get_curr_stat(ch, STAT_STR);
	chance -= get_curr_stat(victim, STAT_DEX) * 4/3;

	if (IS_AFFECTED(ch, AFF_FLYING))
		chance -= 10;

	/* speed */
	if (IS_NPC(ch) && IS_SET(ch->pIndexData->off_flags, OFF_FAST))
		chance += 10;
	if (IS_NPC(victim) && IS_SET(victim->pIndexData->off_flags, OFF_FAST))
		chance -= 20;

	/* level */
	chance += (ch->level - victim->level) * 2;

	/* now the attack */
	if (number_percent() < chance) {
		act("$n squeezes you with a powerful crush!",
		    ch, NULL, victim, TO_VICT);
		act("You slam into $N, and crushes $M!",
		    ch, NULL, victim, TO_CHAR);
		act("$n squeezes $N with a powerful crush.",
		    ch, NULL, victim, TO_NOTVICT);

		wait = 3;

		switch(number_bits(2)) {
		case 0: wait = 1; break;
		case 1: wait = 2; break;
		case 2: wait = 4; break;
		case 3: wait = 3; break;
		}

		WAIT_STATE(victim, wait * PULSE_VIOLENCE);
		WAIT_STATE(ch, SKILL(gsn_crush)->beats);
		victim->position = POS_RESTING;
		damage_crush = (ch->damroll / 2) +
				number_range(4, 4 + 4*ch->size + chance/10);
		damage(ch, victim, damage_crush, gsn_crush, DAM_BASH, TRUE);
	}
	else {
		damage(ch, victim, 0, gsn_crush, DAM_BASH, TRUE);
		act_puts("You fall flat on your face!",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		act("$n falls flat on $s face.",
		    ch, NULL, victim, TO_NOTVICT);
		act("You evade $n's crush, causing $m to fall flat on $s face.",
		    ch, NULL, victim, TO_VICT);
		ch->position = POS_RESTING;
		WAIT_STATE(ch, SKILL(gsn_crush)->beats * 3/2); 
	}
}

void do_sense(CHAR_DATA *ch, const char *argument)
{
	int chance;
	int mana;

	if ((chance = get_skill(ch, gsn_sense_life)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	if (is_affected(ch, gsn_sense_life)) {
		char_puts("You can already feel life forms.\n", ch);
		return;
	}

	mana = SKILL(gsn_sense_life)->min_mana;
	if (ch->mana < mana) {
		char_puts("You cannot seem to concentrate enough.\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_sense_life)->beats);

	if (number_percent() < chance) {
		AFFECT_DATA af;
		
		af.where	= TO_AFFECTS;
		af.type 	= gsn_sense_life;
		af.level 	= ch->level;
		af.duration	= ch->level;
		af.location	= APPLY_NONE;
		af.modifier	= 0;
		af.bitvector	= AFF_DETECT_LIFE;
		affect_to_char(ch, &af);

		ch->mana -= mana;

		act("You start to sense life forms in the room!",
		    ch, NULL, NULL, TO_CHAR);
		act("$n looks more sensitive.", ch, NULL, NULL, TO_ROOM);
		check_improve(ch, gsn_sense_life, TRUE, 1);
	}
	else {
		ch->mana -= mana/2;
		char_puts("You failed.\n" ,ch);
		check_improve(ch, gsn_sense_life, FALSE, 1);
	}
}

void do_pure(CHAR_DATA *ch, const char *argument)
{
        int chance;
        int mana;

        if ((chance = get_skill(ch, gsn_pure_sight)) == 0) {
                char_puts("Huh?\n", ch);
                return;
        }

        if (is_affected(ch, gsn_pure_sight)) {
                char_puts("You can already feel evil life forms.\n", ch);
                return;
        }

        mana = SKILL(gsn_pure_sight)->min_mana;
        if (ch->mana < mana) {
                char_puts("You cannot concentrate enough.\n", ch);
                return;
        }

        WAIT_STATE(ch, SKILL(gsn_pure_sight)->beats);

        if (number_percent() < chance) {
                AFFECT_DATA af;

                af.where        = TO_AFFECTS;
                af.type         = gsn_pure_sight;
                af.level        = ch->level;
                af.duration     = number_range(ch->level / 8, ch->level / 4);
                af.location     = APPLY_NONE;
                af.modifier     = 0;
                af.bitvector    = 0;
                affect_to_char(ch, &af);

                ch->mana -= mana;

                act("You start to sense evil life forms in the room!",
                    ch, NULL, NULL, TO_CHAR);
                act("$n eyes glow white.", ch, NULL, NULL, TO_ROOM);
                check_improve(ch, gsn_pure_sight, TRUE, 1);
        }
        else {
                ch->mana -= mana/2;
                char_puts("You failed.\n" ,ch);
                check_improve(ch, gsn_pure_sight, FALSE, 1);
        }
}

void do_poison_smoke(CHAR_DATA *ch, const char *argument) 
{
	int chance;
	int mana;
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	if ((chance = get_skill(ch, gsn_poison_smoke)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	mana = SKILL(gsn_poison_smoke)->min_mana;
	if (ch->mana < mana) {
		char_puts("You can't get up enough energy.\n", ch);
		return;
	}
	ch->mana -= mana;
	WAIT_STATE(ch, SKILL(gsn_poison_smoke)->beats);

	if (number_percent() > chance) {
		char_puts("You failed.\n", ch);
		check_improve(ch, gsn_poison_smoke, FALSE, 1);
		return;
	}

	char_puts("A cloud of poison smoke fills the room.\n", ch);
	act("A cloud of poison smoke fills the room.", ch, NULL, NULL, TO_ROOM);

	check_improve(ch, gsn_poison_smoke, TRUE, 1);

	for (vch = ch->in_room->people; vch; vch = vch_next) {
		vch_next = vch->next_in_room;

		if (is_safe_spell(ch, vch, TRUE))
			continue;

		spell_poison(gsn_poison, LEVEL(ch), ch, vch, TARGET_CHAR);
		if (vch != ch)
			multi_hit(vch, ch, TYPE_UNDEFINED);
	}
}

void do_blindness_dust(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int chance;
	char arg[MAX_INPUT_LENGTH];
	int mana;

	if ((chance = get_skill(ch, gsn_blindness_dust)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	mana = SKILL(gsn_blindness_dust)->min_mana;
	if (ch->mana < mana) {
		char_puts("You can't get up enough energy.\n", ch);
		return;
	}

	ch->mana -= mana;
	WAIT_STATE(ch, SKILL(gsn_blindness_dust)->beats);
	if (number_percent() > chance) {
		char_puts("You failed.\n",ch);
		check_improve(ch,gsn_blindness_dust,FALSE,1);
		return;
	}

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("A cloud of dust fills in the room.\n", ch);
		act("A cloud of dust fills the room.", ch, NULL, NULL, TO_ROOM);

		check_improve(ch, gsn_blindness_dust, TRUE, 1);

		for (vch = ch->in_room->people; vch; vch = vch_next) {
			vch_next = vch->next_in_room;

			if (is_safe_spell(ch, vch, TRUE)) 
				continue;

			spell_blindness(gsn_blindness, LEVEL(ch), ch, vch, TARGET_CHAR);
			if (vch != ch)
				multi_hit(vch, ch, TYPE_UNDEFINED);
		}
	}
	else {
		if ((vch = get_char_room(ch, arg)) == NULL) {
			char_puts("They aren't here.\n", ch);
			return;
		}
		if (is_safe(ch, vch))
			return;
		act("You throw some dust into $N's eyes.", ch, NULL, vch, TO_CHAR);
		act("$n throws some dust into $N's eyes.", ch, NULL, vch, TO_ROOM);
		act("$n throws some dust into your eyes.", ch, NULL, vch, TO_VICT);
		spell_blindness(gsn_blindness, LEVEL(ch), ch, vch, TARGET_CHAR);
		if (vch != ch)
			multi_hit(vch, ch, TYPE_UNDEFINED);
	}
}

bool check_yell(CHAR_DATA *ch, CHAR_DATA *victim, bool fighting)
{
	return (!IS_NPC(ch) && !IS_NPC(victim) &&
		victim->position > POS_STUNNED && !fighting);
}

void do_wild_swing(CHAR_DATA *ch, const char *argument)
{
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;
	OBJ_DATA *wield;
	OBJ_DATA *obj2;
        int chance, sn, dam;	
	sn = gsn_wild_swing;

        if ((chance = get_skill(ch, gsn_wild_swing)) == 0) {
                char_puts("Huh?\n", ch);
                return;
        }

        if((wield = get_eq_char(ch, WEAR_WIELD)) == NULL
	|| !(wield->value[ITEM_WEAPON_TYPE] == WEAPON_POLEARM || 
	     wield->value[ITEM_WEAPON_TYPE] == WEAPON_FLAIL ||
	     wield->value[ITEM_WEAPON_TYPE] == WEAPON_SPEAR ||
	     wield->value[ITEM_WEAPON_TYPE] == WEAPON_LANCE ||
	     wield->value[ITEM_WEAPON_TYPE] == WEAPON_BASTARDSWORD)) {
		char_puts("You must use the right kind of weapon to swing.\n",ch);
		return;
	}
        WAIT_STATE(ch, SKILL(gsn_wild_swing)->beats);

        if (number_percent() > chance) {
          if (IS_OBJ_STAT(wield, ITEM_NOREMOVE) || get_skill(ch, gsn_grip) >= 75
	     || (get_skill(ch, gsn_double_grip) >= 75
		&& get_eq_char(ch, WEAR_SHIELD) != NULL 
		&& get_eq_char(ch, WEAR_SECOND_WIELD) != NULL)) {
		char_puts("You start your swing but trip over your feet and fall to the ground!\n",
		ch);
		act("$n starts to swing but trips over $s own feet!",
		ch,NULL,NULL,TO_ROOM);
		ch->position = POS_RESTING;
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
                return;
          }

	  char_puts("You spin around and lose your grip on your weapon,"
		    " and it flies out of your hands!\n",ch);
          act("$n spins around and loses $s grip on $s weapon,"
		    " and it flies out of $s hands!",
		ch,NULL,NULL,TO_ROOM);
                 
          obj_from_char(wield);
          if (IS_OBJ_STAT(wield, ITEM_NODROP) 
	  || IS_OBJ_STAT(wield,ITEM_INVENTORY))
                obj_to_char(wield, ch);
          else {
                obj_to_room(wield, ch->in_room);
          }

          if ((obj2 = get_eq_char(ch, WEAR_SECOND_WIELD)) != NULL) {
                char_puts("You wield your second weapon as your first!.\n",
                         ch);
                act("$n wields $s second weapon as $s first!",
                         ch, NULL, NULL, TO_ROOM);
                unequip_char(ch, obj2);
                equip_char(ch, obj2, WEAR_WIELD);
          }
	  return;
        }

        char_puts("You spin around wildly, swinging your weapon with you!\n", 
		ch);
        act("$n spins around wildly, swinging $s weapon dangerously!", 
		ch, NULL, NULL, TO_ROOM);

        check_improve(ch, gsn_wild_swing, TRUE, 1);

        for (vch = ch->in_room->people; vch; vch = vch_next) {
                vch_next = vch->next_in_room;
                if (is_safe(ch, vch) || ch == vch || is_same_group(ch, vch))
                        continue;

		dam = dice(wield->value[ITEM_WEAPON_DICE_NUM], 
			wield->value[ITEM_WEAPON_DICE_SIZE]);
		dam += GET_DAMROLL(ch);

		if (IS_WEAPON_STAT(wield, WEAPON_POISON)) {
			int level;
			AFFECT_DATA *poison, af;

                        if ((poison = affect_find(wield->affected,gsn_poison)) == NULL)
                            level = wield->level;
                        else
                            level = poison->level;

                        if (!saves_spell(level,vch,DAM_POISON)) {
                            char_puts("You feel poison coursing through your veins.",
                                  vch);
                            act("$n is poisoned by the venom on $p.",
                                    vch,wield,NULL,TO_ROOM);

                            	af.where     = TO_AFFECTS;
                            	af.type      = gsn_poison;
                            	af.level     = level * 3/4;
                            	af.duration  = level / 2;
                            	af.location  = APPLY_STR;
                            	af.modifier  = -1;
                            	af.bitvector = AFF_POISON;
                            	affect_join(vch, &af);
                        }  

		}

                if (IS_WEAPON_STAT(wield,WEAPON_FLAMING)) {
                        act("$n is burned by $p.",vch,wield,NULL,TO_ROOM);
                        act("$p sears your flesh.",vch,wield,NULL,TO_CHAR);
                        fire_effect((void *) vch,wield->level,dam,TARGET_CHAR);
                }

                if (IS_WEAPON_STAT(wield,WEAPON_FROST)) {
                        act("$p freezes $n.",vch,wield,NULL,TO_ROOM);
                        act("The cold touch of $p surrounds you with ice.",
                                vch,wield,NULL,TO_CHAR);
                        cold_effect(vch,wield->level,dam,TARGET_CHAR);
                }

                if (IS_WEAPON_STAT(wield,WEAPON_SHOCKING)) {
                        act("$n is struck by lightning from $p.",vch,wield,NULL,TO_ROOM);
                        act("You are shocked by $p.",vch,wield,NULL,TO_CHAR);
                        shock_effect(vch,wield->level,dam,TARGET_CHAR);
                }

                damage(ch, vch, dam, sn, DAM_SLASH, DAMF_SHOW);
        }        
}
