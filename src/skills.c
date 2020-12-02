/*
 * $Id: skills.c 978 2006-12-08 07:38:16Z zsuzsu $
 */

/***************************************************************************
 *     ANATOLIA 2.1 is copyright 1996-1997 Serdar BULUT, Ibrahim CANPUNAR  *
 *     ANATOLIA has been brought to you by ANATOLIA consortium		   *
 *	 Serdar BULUT {Chronos} 	bulut@rorqual.cc.metu.edu.tr	   *
 *	 Ibrahim Canpunar  {Asena}	canpunar@rorqual.cc.metu.edu.tr    *
 *	 Murat BICER  {KIO}		mbicer@rorqual.cc.metu.edu.tr	   *
 *	 D.Baris ACAR {Powerman}	dbacar@rorqual.cc.metu.edu.tr	   *
 *     By using this code, you have agreed to follow the terms of the	   *
 *     ANATOLIA license, in the file Anatolia/anatolia.licence		   *
 ***************************************************************************/

/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,	   *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *									   *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael	   *
 *  Chastain, Michael Quan, and Mitchell Tse.				   *
 *									   *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc	   *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.						   *
 *									   *
 *  Much time and thought has gone into this software and you are	   *
 *  benefitting.  We hope that you share your changes too.  What goes	   *
 *  around, comes around.						   *
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
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

#if !defined(WIN32)
#	include <unistd.h>
#endif
#include <ctype.h>

#include "merc.h"
#include "debug.h"
#include "update.h"
#include "quest.h"
#include "obj_prog.h"
#include "fight.h"
#include "healer.h"
#include "stats.h"

/* command procedures needed */
DECLARE_DO_FUN(do_exits		);
DECLARE_DO_FUN(do_look		);
DECLARE_DO_FUN(do_help		);
DECLARE_DO_FUN(do_affects	);
DECLARE_DO_FUN(do_murder	);
DECLARE_DO_FUN(do_say		);
DECLARE_DO_FUN(do_alist		);
DECLARE_DO_FUN(do_yell		);


/*
 * deprecated
 */
void do_pick(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *gch;
	OBJ_DATA *obj;
	int door;
	int chance;

	if ((chance = get_skill(ch, gsn_pick)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Pick what?\n", ch);
		return;
	}

	if (MOUNTED(ch)) {
		  char_puts("You can't pick while mounted.\n", ch);
		  return;
	}

	WAIT_STATE(ch, SKILL(gsn_pick)->beats);

	/* look for guards */
	for (gch = ch->in_room->people; gch; gch = gch->next_in_room) {
		if (IS_NPC(gch)
		&&  IS_AWAKE(gch)
		&&  ch->level + 5 < gch->level) {
			act("$N is standing too close to lock.",
			    ch, NULL, gch, TO_CHAR);
			return;
		}
	}

	if (!IS_NPC(ch) && number_percent() > chance) {
		char_puts("You failed.\n", ch);
		check_improve(ch, gsn_pick, FALSE, 2);
		return;
	}

	if ((obj = get_obj_here(ch, arg)) != NULL) {
		/* portal stuff */
		if (obj->pIndexData->item_type == ITEM_PORTAL) {
		    if (!IS_SET(obj->value[ITEM_PORTAL_EXIT_FLAGS],EX_ISDOOR)) {	
			char_puts("You can't do that.\n", ch);
			return;
		    }

		    if (!IS_SET(obj->value[ITEM_PORTAL_EXIT_FLAGS],EX_CLOSED)) {
			char_puts("It's not closed.\n", ch);
			return;
		    }

		    if (obj->value[ITEM_PORTAL_KEY] < 0) {
			char_puts("It can't be unlocked.\n", ch);
			return;
		    }

		    if (IS_SET(obj->value[ITEM_PORTAL_EXIT_FLAGS],EX_PICKPROOF)) {
			char_puts("You failed.\n", ch);
			return;
		    }

		    REMOVE_BIT(obj->value[ITEM_PORTAL_EXIT_FLAGS],EX_LOCKED);
		    act_puts("You pick the lock on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
		    act("$n picks the lock on $p.", ch, obj, NULL, TO_ROOM);
		    check_improve(ch, gsn_pick, TRUE, 2);
		    return;
		}

		
		/* 'pick object' */
		if (obj->pIndexData->item_type != ITEM_CONTAINER)
		    { char_puts("That's not a container.\n", ch); return; }
		if (!IS_SET(obj->value[ITEM_CONTAINER_FLAGS], CONT_CLOSED))
		    { char_puts("It's not closed.\n", ch); return; }
		if (obj->value[ITEM_CONTAINER_KEY] < 0)
		    { char_puts("It can't be unlocked.\n", ch); return; }
		if (!IS_SET(obj->value[ITEM_CONTAINER_FLAGS], CONT_LOCKED))
		    { char_puts("It's already unlocked.\n", ch); return; }
		if (IS_SET(obj->value[ITEM_CONTAINER_FLAGS], CONT_PICKPROOF))
		    { char_puts("You failed.\n", ch); return; }

		REMOVE_BIT(obj->value[ITEM_CONTAINER_FLAGS], CONT_LOCKED);
		act_puts("You pick the lock on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
		act("$n picks the lock on $p.", ch, obj, NULL, TO_ROOM);
		check_improve(ch, gsn_pick, TRUE, 2);
		return;
	}

	if ((door = find_door(ch, arg)) >= 0) {
		/* 'pick door' */
		ROOM_INDEX_DATA *to_room;
		EXIT_DATA *pexit;
		EXIT_DATA *pexit_rev;

		pexit = ch->in_room->exit[door];
		if (!IS_SET(pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch))
		    { char_puts("It's not closed.\n", ch); return; }
		if (pexit->key < 0 && !IS_IMMORTAL(ch))
		    { char_puts("It can't be picked.\n", ch); return; }
		if (!IS_SET(pexit->exit_info, EX_LOCKED))
		    { char_puts("It's already unlocked.\n", ch); return; }
		if (IS_SET(pexit->exit_info, EX_PICKPROOF) && !IS_IMMORTAL(ch))
		    { char_puts("You failed.\n", ch); return; }

		REMOVE_BIT(pexit->exit_info, EX_LOCKED);
		char_puts("*Click*\n", ch);
		act("$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM);
		check_improve(ch, gsn_pick, TRUE, 2);

		/* pick the other side */
		if ((to_room   = pexit->to_room.r           ) != NULL
		&&   (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
		&&   pexit_rev->to_room.r == ch->in_room)
			REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);
	}
}

void do_sneak(CHAR_DATA *ch, const char *argument)
{
	AFFECT_DATA	af;
	int		chance;

	if ((chance = get_skill(ch, gsn_sneak)) == 0)
		return;

	if (MOUNTED(ch)) {
		  char_puts("You can't sneak while mounted.\n", ch);
		  return;
	}

	char_puts("You attempt to move silently.\n", ch);
	affect_strip(ch, gsn_sneak);

	if (IS_AFFECTED(ch, AFF_SNEAK))
		return;

	if (number_percent() < chance) {
		check_improve(ch, gsn_sneak, TRUE, 3);
		af.where     = TO_AFFECTS;
		af.type      = gsn_sneak;
		af.level     = ch->level; 
		af.duration  = ch->level;
		af.location  = APPLY_NONE;
		af.modifier  = 0;
		af.bitvector = AFF_SNEAK;
		affect_to_char(ch, &af);
	}
	else
		check_improve(ch, gsn_sneak, FALSE, 3);
}

void do_hide(CHAR_DATA *ch, const char *argument)
{
	int		 chance;
	flag32_t		 sector;
	int cost = 0;
	AFFECT_DATA af;

	cost = move_cost(ch, gsn_hide, STAT_DEX, 1);

	if (MOUNTED(ch)) {
		  char_puts("You can't hide while mounted.\n", ch);
		  return;
	}

	if(is_affected(ch, gsn_rnet_trap)) {
		char_puts("You cannot hide while constricted in a net.\n",ch);
		return;
	}
	if (RIDDEN(ch)) {
		  char_puts("You can't hide while being ridden.\n", ch);
		  return;
	}

	/*if (IS_PUMPED(ch)) {
		  char_puts("You can't hide while your adrenaline is flowing!\n", ch);
		  return;
	}*/

	if (IS_AFFECTED(ch, AFF_FAERIE_FIRE) )  {
		char_puts("You cannot hide while glowing.\n", ch);
		return;
	}

	if (ch->in_room != NULL
	    && ch->in_room->sector_type == SECT_FOREST
	    && is_affected(ch, gsn_pollen) )  {
		char_puts("You cannot hide itching so badly.\n", ch);
		return;
	}

	if (ch->move < cost) {
		char_puts("You don't have enough stamina to hide.\n", ch);
		return;
	}

	char_puts("You attempt to hide.\n", ch);
	if ((chance = get_skill(ch, gsn_hide)) == 0) {
	   char_puts("Huh?\n", ch);
		return;
	}

	sector = ch->in_room->sector_type;
	if (sector == SECT_FOREST
	||  sector == SECT_HILLS
	||  sector == SECT_MOUNTAIN)
		chance -= 20;
	else if (sector == SECT_CITY)
		chance += 20;
	
	if (!is_affected(ch, gsn_hide)) {
		af.where     = TO_AFFECTS;
		af.type      = gsn_hide;
		af.level     = LEVEL(ch);
		af.duration  = -1;
		af.modifier  = 0;
		af.location  = APPLY_NONE;
		af.bitvector = AFF_NO_DISPLAY;
		affect_to_char(ch, &af);
	}

	if (number_percent() < chance) {
		SET_BIT(ch->affected_by, AFF_HIDE);

		check_improve(ch, gsn_hide, TRUE, 7);
	}
	else  {
		REMOVE_BIT(ch->affected_by, AFF_HIDE);
		check_improve(ch, gsn_hide, FALSE, 6);
	}

	ch->move -= cost;

}

void do_camouflage(CHAR_DATA *ch, const char *argument)
{
	int		sn;
	int		chance;
	flag32_t		sector;
	int cost = 0;
	AFFECT_DATA af;

	cost = move_cost(ch, gsn_camouflage, STAT_DEX, 1);

	if (MOUNTED(ch)) {
		char_puts("You can't camouflage while mounted.\n", ch);
		return;
	}

	if (RIDDEN(ch)) {
		char_puts("You can't camouflage while being ridden.\n", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_FAERIE_FIRE))  {
		char_puts("You can't camouflage yourself while glowing.\n", ch);
		return;
	}

	if (ch->in_room != NULL
	    && (ch->in_room->sector_type == SECT_FOREST
	    ||  ch->in_room->sector_type == SECT_FIELD
	    ||  ch->in_room->sector_type == SECT_HILLS
	    ||  ch->in_room->sector_type == SECT_MOUNTAIN 
	    ||  ch->in_room->sector_type == SECT_DESERT)
	    && is_affected(ch, gsn_pollen) )  {
		char_puts("You can't camouflage yourself while itching so badly.\n", ch);
		return;
	}

	if ((sn = sn_lookup("camouflage")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("You don't know how to camouflage yourself.\n", ch);
		return;
	}

	if (ch->move < cost) {
		char_puts("You don't have enough stamina to camouflage yourself.\n", ch);
		return;
	}

	sector = ch->in_room->sector_type;
	if (sector != SECT_FOREST
	&&  sector != SECT_HILLS
	&&  sector != SECT_MOUNTAIN) {
		char_puts("There's no cover here.\n", ch);
		act("$n tries to camouflage $mself against the lone leaf on the ground.",
		    ch, NULL, NULL, TO_ROOM);
		return;
	}

	char_puts("You attempt to camouflage yourself.\n", ch);
/*
   Taken out to be equvilant to hide
	WAIT_STATE(ch, SKILL(sn)->beats);
*/
	if (!is_affected(ch, gsn_camouflage)) {
		af.where     = TO_AFFECTS;
		af.type      = gsn_camouflage;
		af.level     = LEVEL(ch);
		af.duration  = -1;
		af.modifier  = 0;
		af.location  = APPLY_NONE;
		af.bitvector = AFF_NO_DISPLAY;
		affect_to_char(ch, &af);
	}

	if (IS_AFFECTED(ch, AFF_CAMOUFLAGE))
		REMOVE_BIT(ch->affected_by, AFF_CAMOUFLAGE);

	if (IS_NPC(ch) || number_percent() < chance) {
		SET_BIT(ch->affected_by, AFF_CAMOUFLAGE);
		check_improve(ch, sn, TRUE, 2);
	}
	else
		check_improve(ch, sn, FALSE, 2);

	ch->move -= cost;
}

void do_track(CHAR_DATA *ch, const char *argument)
{
	ROOM_HISTORY_DATA *rh;
	EXIT_DATA *pexit;
	static char *door[] = { "north","east","south","west","up","down",
		                      "that way" };
	int d;
	int chance;

	if ((chance = get_skill(ch, gsn_track)) == 0) {
		char_puts("There are no train tracks here.\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_track)->beats);
	act("$n checks the ground for tracks.", ch, NULL, NULL, TO_ROOM);

	if (number_percent() < chance) {
		if (IS_NPC(ch)) {
			if (ch->last_fought != NULL
			&&  !IS_SET(ch->pIndexData->act, ACT_NOTRACK))
				add_mind(ch, ch->last_fought->name);
		}

		for (rh = ch->in_room->history; rh != NULL; rh = rh->next)
			if (is_name(argument, rh->name)) {
				check_improve(ch, gsn_track, TRUE, 1);
			if ((d = rh->went) == -1)
				continue;
			char_printf(ch, "%s's tracks lead %s.\n",
				     rh->name, door[d]);
			if ((pexit = ch->in_room->exit[d]) != NULL
			&&  IS_SET(pexit->exit_info, EX_ISDOOR)
			&&  pexit->keyword != NULL)
				doprintf(do_open, ch, "%s", door[d]);
			move_char(ch, rh->went, FALSE);
			return;
		}
	}

	char_puts("You don't see any tracks.\n", ch);
	check_improve(ch, gsn_track, FALSE, 1);
}

void do_vampire(CHAR_DATA *ch, const char *argument)
{
	AFFECT_DATA af;
	int level, duration;
	int chance;
 
	if (is_affected(ch, gsn_vampire)) {
		char_puts("But you are already a vampire. Kill them! Kill them!\n", ch);
		return;
	}

	if ((chance = get_skill(ch, gsn_vampire)) == 0) {
		char_puts("You try to show yourself even more ugly.\n", ch);
		return;
	}

	if (chance < 100) {
		char_puts("Go and ask the questor. He'll help you.\n", ch);
		return;
	}
/*################ # Erebus asked to remove this check so you can change at any time
#	if (weather_info.sunlight == SUN_LIGHT 
#	||  weather_info.sunlight == SUN_RISE) {
#		char_puts("You should wait for the evening or night to transform to a vampire.\n", ch);
#		return;
#	}  
#################
*/

	level = ch->level;
	duration = level / 10 + 5;

	af.type      = gsn_vampire;
	af.level     = level;
	af.duration  = duration;

/* negative immunity */
	af.where = TO_IMMUNE;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = IMM_NEGATIVE;
	affect_to_char(ch, &af);

/* haste */
	af.where     = TO_AFFECTS;
	af.location  = APPLY_DEX;
	af.modifier  = 1 + (level /20);
	af.bitvector = AFF_HASTE;
	affect_to_char(ch, &af);

/* giant strength + infrared */
	af.location  = APPLY_STR;
	af.modifier  = 1 + (level / 20);
	af.bitvector = 0;
	affect_to_char(ch, &af);

/* damroll */
	af.where     = TO_AFFECTS;
	af.location  = APPLY_DAMROLL;
	af.modifier  = ch->level;
	af.bitvector = AFF_BERSERK;
	affect_to_char(ch, &af);

/* flying, infrared */
	af.where     = TO_AFFECTS;
	af.location  = 0;
	af.modifier  = 0;
	af.bitvector = AFF_SNEAK | AFF_FLYING | AFF_INFRARED;
	affect_to_char(ch, &af);

	char_puts("You feel yourself getting greater and greater.\n", ch);
	act("You cannot recognize $n anymore.", ch, NULL, NULL, TO_ROOM);
}

void do_vbite(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int chance;

	one_argument(argument, arg, sizeof(arg));

	if ((chance = get_skill(ch, gsn_vampiric_bite)) == 0) {
		char_puts("You don't know how to bite creatures.\n", ch);
		return;
	}

	if (!is_affected(ch, gsn_vampire)) {
		char_puts("You must transform vampire before biting.\n", ch);
		return;
	}

	if (arg[0] == '\0') {
		char_puts("Bite whom?\n", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim->position != POS_SLEEPING) {
		char_puts("They must be sleeping.\n", ch);
		return;
	}

	if (IS_NPC(ch) && !IS_NPC(victim))
		return;

	if (victim == ch) {
		char_puts("How can you sneak upon yourself?\n", ch);
		return;
	}

	if (victim->fighting != NULL) {
		char_puts("You can't bite a fighting person.\n", ch);
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (victim->hit < (8 * victim->max_hit / 10) ) {
		act_puts("$N is hurt and suspicious ... doesn't worth up.",
			 ch, NULL, victim, TO_CHAR, POS_DEAD);
		return;
	}


	WAIT_STATE(ch, SKILL(gsn_vampiric_bite)->beats);

	if (!IS_AWAKE(victim)
	&&  (IS_NPC(ch) ||
	     number_percent() < ((chance * 7 / 10) +
		(2 * (ch->level - victim->level)) ))) {
		check_improve(ch,gsn_vampiric_bite,TRUE,1);
		one_hit(ch, victim, gsn_vampiric_bite, WEAR_WIELD);
	}
	else {
		check_improve(ch, gsn_vampiric_bite, FALSE, 1);
		damage(ch, victim, 0, gsn_vampiric_bite, DAM_NONE, DAMF_SHOW);
	}
	if (!IS_NPC(victim) && victim->position==POS_FIGHTING) 
		doprintf(do_yell, victim, 
			"Help! %s tried to bite me!", PERS(ch, victim));
}

void do_bash_door(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *gch;
	int chance;
	int damage_bash,door;
	int beats;

	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	EXIT_DATA *pexit_rev;

	one_argument(argument, arg, sizeof(arg));
 
	if ((chance = get_skill(ch, gsn_bash_door)) == 0) {
		char_puts("Bashing? What's that?\n", ch);
		return;
	}
 
	if (MOUNTED(ch)) {
        	char_puts("You can't bash doors while mounted.\n", ch);
		return;
	}

	if (RIDDEN(ch)) {
		char_puts("You can't bash doors while being ridden.\n", ch);
		return;
	}

	if (arg[0] == '\0') {
		char_puts("Bash wich door or direction?\n", ch);
		return;
	}

	if (ch->fighting) {	
		char_puts("Wait until the fight finishes.\n", ch);
		return;
	}

	/* look for guards */
	for (gch = ch->in_room->people; gch; gch = gch->next_in_room)
		if (IS_NPC(gch)
		&&  IS_AWAKE(gch) && ch->level + 5 < gch->level) {
			act_puts("$N is standing too close to door.",
				 ch, NULL, gch, TO_CHAR, POS_DEAD);
			return;
		}

	if ((door = find_door(ch, arg)) < 0)
		return;

	pexit = ch->in_room->exit[door];

	if (!IS_SET(pexit->exit_info, EX_CLOSED)) {
		char_puts("It's already open.\n", ch);
		return;
	}

	if (!IS_SET(pexit->exit_info, EX_LOCKED)) {
		char_puts("Just try to open it.\n", ch);
		return;
	}

	if (IS_SET(pexit->exit_info, EX_NOPASS)) {
		char_puts("A mystical shield protects exit.\n", ch); 
		return;
	}

	if (IS_AFFECTED(ch,AFF_FLYING)) {
		char_puts("You bounce against the door"
			" and glide backward across the room.\n", ch);
		return;
	}

	chance -= 90;

	/* modifiers */

	/* size and weight */
	chance += ch_weight_carried(ch) / 100;
	chance += (ch->size - 2) * 20;

	/* stats */
	chance += get_curr_stat(ch, STAT_STR)/4;

	act_puts("You slam into $d, and try to break $d!",
		 ch, NULL, pexit->keyword, TO_CHAR, POS_DEAD);
	act("$n slams into $d, and tries to break it!",
	    ch, NULL, pexit->keyword, TO_ROOM);

	if (room_dark(ch->in_room))
		chance /= 2;

	beats = SKILL(gsn_bash_door)->beats;

	/* now the attack */
	if (number_percent() < chance) {

		WAIT_STATE(ch, beats);

		if (IS_SET(pexit->exit_info, EX_NOBASH)) {
			char_puts("The door is reinforced against"
				" brute strength.\n", ch);
			return;
		}

		if (pexit->lock_level > LEVEL(ch)) {
			char_puts("This lock looks rather strong,"
			" you might consider a more delicate approach.\n", ch);
			return;
		}

		check_improve(ch, gsn_bash_door, TRUE, 1);

		REMOVE_BIT(pexit->exit_info, EX_LOCKED);
		REMOVE_BIT(pexit->exit_info, EX_CLOSED);
		act("$n bashes the $d and breaks the lock.",
		    ch, NULL, pexit->keyword, TO_ROOM);
		act_puts("You succeeded in opening the door.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);

/* open the other side */
		if ((to_room = pexit->to_room.r) != NULL
		&&  (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
		&&  pexit_rev->to_room.r == ch->in_room) {
			ROOM_INDEX_DATA *in_room;

			REMOVE_BIT(pexit_rev->exit_info, EX_CLOSED);
			REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);

			in_room = ch->in_room;
			ch->in_room = to_room;
			act("$n bashes the $d and breaks the lock.",
			    ch, NULL, pexit->keyword, TO_ROOM);
			ch->in_room = in_room;
		}

		check_improve(ch, gsn_bash_door, TRUE, 1);
	}
	else {
		act_puts("You fall flat on your face!",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		act_puts("$n falls flat on $s face.",
			 ch, NULL, NULL, TO_ROOM, POS_RESTING);
		check_improve(ch, gsn_bash_door, FALSE, 1);
		ch->position = POS_RESTING;
		WAIT_STATE(ch, beats * 3 / 2); 
		damage_bash = ch->damroll +
			      number_range(4,4 + 4* ch->size + chance/5);
		damage(ch, ch, damage_bash, gsn_bash_door, DAM_BASH, DAMF_SHOW);

	}
}

void do_blink(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];

	if (get_skill(ch, gsn_blink) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_printf(ch, "Your current blink status: %s.\n",
			    IS_SET(ch->conf_flags, PLR_CONF_BLINK) ? "ON" : "OFF");
		return;
	}

	if (!str_cmp(arg, "ON")) {
		SET_BIT(ch->conf_flags, PLR_CONF_BLINK);
		char_puts("Now, your current blink status is ON.\n", ch);
		ch->mana -= 10;
		return;
	}

	if (!str_cmp(arg, "OFF")) {
		REMOVE_BIT(ch->conf_flags, PLR_CONF_BLINK);
		char_puts("Now, your current blink status is OFF.\n", ch);
		return;
	}

	char_printf(ch, "What's that? Is %s a status?\n", arg);

}

void do_vanish(CHAR_DATA *ch, const char *argument)
{
	int chance;
	int sn;
	int min_mana;
	ROOM_INDEX_DATA *dest = NULL;

	if ((sn = sn_lookup("vanish")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	if (ch->mana < (min_mana = SKILL(sn)->min_mana)) {
		char_puts("You don't have enough power.\n", ch);
		return;
	}
	ch->mana -= min_mana;
	WAIT_STATE(ch, SKILL(sn)->beats);

	if (number_percent() > chance) {
		char_puts("You failed.\n", ch);
		check_improve(ch, sn, FALSE, 1);
		return;
	}

	if (IS_SET(ch->in_room->room_flags, 
		   ROOM_NORECALL | ROOM_PEACE | ROOM_SAFE | ROOM_NOSUMMON)) {
		char_puts("You failed.\n", ch);
		return;
	}

	if (IS_SET(ch->in_room->area->flags, AREA_NOSUMMON)) {
		char_puts("You failed.\n", ch);
		return;
	}

	act("You throw down a small {Dg{wl{Wo{Wb{De{x.", ch, NULL, NULL, TO_CHAR);
	act("$n throws down a small {Dg{wl{Wo{Wb{De{x.", ch, NULL, NULL, TO_ROOM);
	check_improve(ch, sn, TRUE, 1);

  	if (!IS_NPC(ch) && ch->fighting && number_bits(1) == 1) {
		char_puts("You failed.\n", ch);
		return;
	}

	dest = get_random_room(ch, ch->in_room->area);
	if (dest == NULL) {
		char_puts("Nowhere to go!\n", ch);
		return;
	}
	if (IS_SET(dest->room_flags, 
	ROOM_NORECALL | ROOM_SAFE | ROOM_NOSUMMON)) {
		char_puts("Nowhere to go!\n", ch);
		return;
	}

	stop_fighting(ch, TRUE);
	transfer_char(ch, NULL, dest,
		      "$N is gone!", NULL, "$N appears from nowhere.");
}

/*void do_shadow_meld(CHAR_DATA *ch, const char *argument)
{
	int chance;
	if ((chance=get_skill(ch, gsn_shadow_meld)) == 0)
		return;

	if (MOUNTED(ch)) {
		  char_puts("You can't meld into shadows while mounted.\n", ch);
		  return;
	}

	if (IS_PUMPED(ch)) {
		  char_puts("You can't meld into shadows while your adrenaline is gushing!\n", ch);
		  return;
	}

	if (RIDDEN(ch)) {
		  char_puts("You can't meld into shadows while being ridden.\n", ch);
		  return;
	}

	char_puts("You attempt to meld into shadows.\n", ch);
	if (number_percent()<=chance) {
		SET_BIT(ch->affected_by, AFF_SHADOW_MELD);
		check_improve(ch, gsn_shadow_meld, TRUE, 3);
	}
	else check_improve(ch, gsn_shadow_meld, FALSE, 3);
}*/

void do_fade(CHAR_DATA *ch, const char *argument)
{
	int chance;
	int cost = 0;
	AFFECT_DATA af;

	cost = move_cost(ch, gsn_fade, 0, 0);

	if ((chance=get_skill(ch, gsn_fade)) == 0)
		return;

	if (MOUNTED(ch)) {
		  char_puts("You can't fade while mounted.\n", ch);
		  return;
	}

	if (RIDDEN(ch)) {
		  char_puts("You can't fade while being ridden.\n", ch);
		  return;
	}

	if(is_affected(ch, gsn_rnet_trap)) {
		char_puts("You cannot fade while constricted by the net.\n",ch);
		return;
	}

	if (IS_AFFECTED(ch,AFF_FAERIE_FIRE)) {
		char_puts("You can not fade while glowing.\n",ch);
		return;
	}

	if (ch->in_room != NULL
	    && ch->in_room->sector_type == SECT_FOREST
	    && is_affected(ch, gsn_pollen) )  {
		char_puts("You can not fade while itching so badly.\n",ch);
		return;
	}

	if (ch->move < cost) {
		char_puts("You don't have enough stamina to fade.\n", ch);
		return;
	}

	if (!is_affected(ch, gsn_fade)) {
		af.where     = TO_AFFECTS;
		af.type      = gsn_fade;
		af.level     = LEVEL(ch);
		af.duration  = -1;
		af.modifier  = 0;
		af.location  = APPLY_NONE;
		af.bitvector = AFF_NO_DISPLAY;
		affect_to_char(ch, &af);
	}

	if (IS_PUMPED(ch)) {
		char_puts("You attempt to fade.\n", ch);
			if (number_percent() < (chance/2)) {
				SET_BIT(ch->affected_by, AFF_FADE);
				check_improve(ch, gsn_fade, TRUE, 3);
			}
			else
				check_improve(ch, gsn_fade, FALSE, 3);
	}
        else  if (number_percent() < chance) {
		char_puts("You attempt to fade.\n", ch);
	        SET_BIT(ch->affected_by, AFF_FADE);
                check_improve(ch, gsn_fade, TRUE, 3);
	}
	else
                check_improve(ch, gsn_fade, FALSE, 3);

	ch->move -= cost;
}

void do_vtouch(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	AFFECT_DATA af;
	int chance;
	int sn;

	if ((sn = sn_lookup("vampiric touch")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("You lack the skill to draining touch.\n", ch);
		return;
	}

	if (!is_affected(ch, gsn_vampire)) {
		char_puts("Let it be.\n", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM))  {
		char_puts("You don't want to drain your master.\n", ch);
		return;
	} 

	if ((victim = get_char_room(ch,argument)) == NULL) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (ch == victim) {
		char_puts("Even you are not so stupid.\n", ch);
		return;
	}

	if (is_affected(victim, sn))
		return;

	if (is_safe(ch,victim))
		return;

	WAIT_STATE(ch, SKILL(sn)->beats);

        if (is_affected(victim, gsn_paranoia))  {
                act("$E is way too paranoid for that.", 
			ch, NULL, victim , TO_CHAR);
		return;                                               
        }

	SET_FIGHT_TIME(victim);
	SET_FIGHT_TIME(ch);

	chance = chance * 5/6;
	chance += (LEVEL(ch) - LEVEL(victim)) *3;

	DEBUG(DEBUG_SKILL_VTOUCH,
		"%s[%d] vtouch %d%% %s[%d]",
		ch->name,
		LEVEL(ch),
		chance,
		victim->name,
		LEVEL(victim));

	if (number_percent() < chance
	&&  !IS_CLAN_GUARD(victim)
	&&  !IS_IMMORTAL(victim)) {
		act_puts("You deadly touch $n's neck and put $m to nightmares.",
			 victim, NULL, ch, TO_VICT, POS_DEAD); 
		act_puts("$N deadly touches your neck and puts you "
			 "to nightmares.", victim, NULL, ch, TO_CHAR, POS_DEAD);
		act("$N deadly touches $n's neck and puts $m to nightmares.",
		    victim, NULL, ch, TO_NOTVICT);

		check_improve(ch, sn, TRUE, 1);
		
		af.type = sn;
		af.where = TO_AFFECTS;
		af.level = ch->level;
		af.duration = ch->level / 20 + 1;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = AFF_SLEEP;
		affect_join(victim,&af);

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
	} else {
		damage(ch, victim, 0, sn, DAM_NONE, DAMF_SHOW);
		check_improve(ch, sn, FALSE, 1);
	}
}

void do_push(CHAR_DATA *ch, const char *argument)
{
	char arg1 [MAX_INPUT_LENGTH];
	char arg2 [MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	EXIT_DATA *pexit;
	int percent;
	int door;
	int sn;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0' || arg2[0] == '\0') {
		char_puts("Push whom to what direction?\n", ch);
		return;
	}

	if (MOUNTED(ch)) {
		char_puts("You can't push while mounted.\n", ch);
		return;
	}

	if (RIDDEN(ch)) {
		char_puts("You can't push while being ridden.\n", ch);
		return;
	}

	if (IS_NPC(ch) && IS_SET(ch->affected_by, AFF_CHARM) 
		&& (ch->master != NULL)) {
		char_puts("You are too dazed to push anyone.\n", ch);
		return;
	}

	if ((sn = sn_lookup("push")) < 0)
		return;

	if ((victim = get_char_room(ch, arg1)) == NULL) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (!IS_NPC(victim) && victim->desc == NULL) {
		char_puts("You can't do that.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("That's pointless.\n", ch);
		return;
	}

	if (victim->position == POS_FIGHTING) {
		char_puts("Wait until the fight finishes.\n", ch);
		return;
	}

	if ((door = find_exit(ch, arg2)) < 0)
		return;

	WAIT_STATE(ch, SKILL(sn)->beats);

	if ((pexit = ch->in_room->exit[door])
	&&  IS_SET(pexit->exit_info, EX_ISDOOR)) {
		if (IS_SET(pexit->exit_info, EX_CLOSED)) {
			char_puts("The door is closed.\n", ch); 
			return;
		}
		if (IS_SET(pexit->exit_info, EX_LOCKED)) {
			char_puts("The door is locked.\n", ch); 
			return;
		}
	}


	if (IS_AFFECTED(ch, AFF_DETECT_WEB)) {
		char_puts("You're webbed, and want to do WHAT?!?\n", ch);
		act("$n stupidly tries to push $N while webbed.",
		    ch, NULL, victim, TO_ROOM);
		return; 
	}

	if (IS_AFFECTED(victim, AFF_DETECT_WEB)) {
		act_puts("You attempt to push $N, but the webs hold $m "
			 "in place.", victim, NULL, ch, TO_VICT, POS_DEAD);
		act("$n attempts to push $n, but fails as the webs hold "
		    "$n in place.", victim, NULL, ch, TO_NOTVICT);
		return; 
	}

	if (is_safe(ch,victim))
		return;

	percent  = number_percent() + (IS_AWAKE(victim) ? 10 : -50);
	percent += can_see(victim, ch) ? -10 : 0;

	if (victim->position == POS_FIGHTING
	||  (IS_NPC(victim) && IS_SET(victim->pIndexData->act, ACT_NOTRACK))
	||  (!IS_NPC(ch) && percent > get_skill(ch, sn))
	||  pexit->to_room.r->area != ch->in_room->area) {
		/*
		 * Failure.
		 */

		char_puts("Oops.\n", ch);
		if (!IS_AFFECTED(victim, AFF_SLEEP)) {
			victim->position = victim->position == POS_SLEEPING ? 
					   POS_STANDING : victim->position;
			act("$n tried to push you.",
			    ch, NULL, victim, TO_VICT);
		}
		act("$n tried to push $N.", ch, NULL, victim, TO_NOTVICT);

		if (IS_AWAKE(victim))
			doprintf(do_yell, victim,
				 "Keep your hands out of me, %s!",
				 ch->name);
		if (!IS_NPC(ch) && IS_NPC(victim)) {
			check_improve(ch, sn, FALSE, 2);
			multi_hit(victim, ch, TYPE_UNDEFINED);
		}
		return;
	}

	act_puts("You push $N to $t.",
		 ch, dir_name[door], victim, TO_CHAR | ACT_TRANS, POS_SLEEPING);
	act_puts("$n pushes you to $t.",
		 ch, dir_name[door], victim, TO_VICT | ACT_TRANS, POS_SLEEPING);
	act("$n pushes $N to $t.",
	    ch, dir_name[door], victim, TO_NOTVICT | ACT_TRANS);
	move_char(victim, door, FALSE);

	check_improve(ch, sn, TRUE, 1);
}

void do_crecall(CHAR_DATA *ch, const char *argument)
{
	ROOM_INDEX_DATA *location;
	clan_t *clan;
	CHAR_DATA *gch;
	AFFECT_DATA af;
	int sn;

	if ((sn = sn_lookup("clan recall")) < 0
	||  get_skill(ch, sn) == 0
	||  (clan = clan_lookup(ch->clan)) == NULL) {
		char_puts("Huh?\n", ch);
		return;
	}

	if (is_affected(ch, sn)) {
		act_puts("You can't pray now.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		return;
	}

	if (ch->desc && IS_PUMPED(ch)) {
		act_puts("You are too pumped to pray now.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		return;
	}

	act("$n prays to their clan gods for transportation.",
	    ch, NULL, NULL, TO_ROOM);
	
	if ((location = get_room_index(clan->recall_vnum)) == NULL) {
		char_puts("You are completely lost.\n", ch);
		return;
	}

	if (ch->in_room == location)
		return;

	if (IS_SET(ch->in_room->room_flags, ROOM_NORECALL)
	||  IS_AFFECTED(ch, AFF_CURSE) 
	||  IS_RAFFECTED(ch->in_room, RAFF_CURSE)) {
		char_puts("The gods have forsaken you.\n", ch);
		return;
	}

	ch->move /= 2;
	af.type      = sn;
	af.level     = ch->level;
	af.duration  = SKILL(sn)->beats;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = 0;
	affect_to_char(ch, &af);

	ch->move /= 2;
        for (gch = npc_list; gch; gch = gch->next) {
                if (gch->in_room == ch->in_room
		&&  IS_AFFECTED(gch, AFF_CHARM)
                &&  gch->master == ch
		&& !IS_AFFECTED(gch, AFF_SLEEP)
		&& gch->position >= POS_STANDING) {
			recall(gch, location);
                }
        }
	recall(ch, location);
	if(is_affected(ch, gsn_rnet_trap))
		affect_strip(ch, gsn_rnet_trap);
}

void do_escape(CHAR_DATA *ch, const char *argument)
{
	ROOM_INDEX_DATA *was_in;
	ROOM_INDEX_DATA *now_in;
	EXIT_DATA *pexit;
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	int door;
	int chance;
	int sn;

	if ((victim = ch->fighting) == NULL) {
		if (ch->position == POS_FIGHTING)
			ch->position = POS_STANDING;
		char_puts("You aren't fighting anyone.\n", ch);
		return;
	}

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Escape to what direction?\n", ch);
		return;
	}

	if (MOUNTED(ch)) {
		  char_puts("You can't escape while mounted.\n", ch);
		  return;
	}

	if (RIDDEN(ch)) {
		  char_puts("You can't escape while being ridden.\n", ch);
		  return;
	}

	if ((sn = sn_lookup("escape")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("Try flee. It may fit you better.\n", ch);
		return;
	}

	was_in = ch->in_room;

	if ((door = find_exit(ch, arg)) < 0) {
		char_puts("PANIC! You couldn't escape!\n", ch);
		return;
	}

	if ((pexit = was_in->exit[door]) == 0
	||  pexit->to_room.r == NULL
	||  (IS_SET(pexit->exit_info, EX_CLOSED) &&
	     (!IS_AFFECTED(ch, AFF_PASS_DOOR) ||
	      IS_SET(pexit->exit_info, EX_NOPASS)) &&
	     !IS_TRUSTED(ch, ANGEL))
	||  IS_SET(pexit->exit_info, EX_NOFLEE)
	||  (IS_NPC(ch) &&
	     IS_SET(pexit->to_room.r->room_flags, ROOM_NOMOB))) {
		char_puts("Something prevents you to escape that direction.\n", ch); 
		return;
	}

	WAIT_STATE(ch, SKILL(sn)->beats);

	chance = chance / (IS_AFFECTED(ch, AFF_BLIND) ? 2 : 1);

	if (number_percent() > chance) {
		act_puts("You failed to escape.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		check_improve(ch, sn, FALSE, 1);	
		return;
	}

	check_improve(ch, sn, TRUE, 1);	
	move_char(ch, door, FALSE);
	if ((now_in = ch->in_room) == was_in) {
		char_puts("It's pointless to escape there.\n", ch);
		return;
	}

	ch->in_room = was_in;
	act("$n has escaped!", ch, NULL, NULL, TO_ROOM);
	ch->in_room = now_in;

	if (!IS_NPC(ch)) {
		act_puts("You escaped from combat!",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		if (ch->level < LEVEL_HERO) {
			char_printf(ch, "You lose %d exps.\n", 10);
			gain_exp(ch, -10);
		}
	}
	else
		/* Once fled, the mob will not go after */
		ch->last_fought = NULL;

	stop_fighting(ch, TRUE);
}

void do_layhands(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	AFFECT_DATA af;
	int sn;
	int hp = 0;

	if ((sn = sn_lookup("lay hands")) < 0
	||  get_skill(ch, sn) == 0) {
		char_puts("You cannot channel healing powers through a laying of hands.\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(sn)->beats);

	if ((victim = get_char_room(ch,argument)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (is_affected(ch, sn)) {
		 char_puts("You haven't recovered from your last laying of hands.\n", ch);
		 return;
	}

	ch->mana = ch->mana / 2;

        af.type = sn;
        af.where = TO_AFFECTS;
        af.level = ch->level;
        af.duration = 23;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = 0;
        affect_to_char (ch, &af);

	hp = (victim->max_hit - victim->hit) * 2/3;

	if (LEVEL(victim) > LEVEL(ch))
		hp = hp * (LEVEL(victim) - LEVEL(ch) / 100); 

	heal(ch, NULL, victim, sn, LEVEL(ch), hp);

        if (LEVEL(ch) > 80
	&& IS_AFFECTED(victim, gsn_deafen))
                affect_strip(victim, gsn_deafen);

        if (LEVEL(ch) > 77
	&& IS_AFFECTED(victim, AFF_PLAGUE))
                spell_cure_disease (sn_lookup("cure disease"), ch->level,
                        ch, (void*)victim, TARGET_CHAR);

        if (IS_AFFECTED(victim, AFF_BLIND))
                spell_cure_blindness(sn_lookup("cure blindness"), LEVEL(ch),
                        ch, (void*)victim, TARGET_CHAR);
     
        if (IS_AFFECTED(victim, AFF_POISON))
                spell_cure_poison(sn_lookup("cure poison"), LEVEL(ch),
                        ch, (void*)victim, TARGET_CHAR);

        check_improve(ch, sn, TRUE, 1);
}

int send_arrow(CHAR_DATA *ch, CHAR_DATA *victim,OBJ_DATA *arrow, 
	       int door, int chance ,int bonus) 
{
	EXIT_DATA *pExit;
	ROOM_INDEX_DATA *dest_room;
	AFFECT_DATA *paf;
	int damroll = 0, hitroll = 0, sn;
	AFFECT_DATA af;
	OBJ_DATA *bow = NULL;

	if (arrow->value[ITEM_WEAPON_TYPE] == WEAPON_SPEAR)  
		sn = gsn_spear;
	else 
		sn = gsn_arrow;

	for (paf = arrow->affected; paf != NULL; paf = paf->next) {
		if (paf->location == APPLY_DAMROLL)
			damroll += paf->modifier;
		if (paf->location == APPLY_HITROLL)
			hitroll += paf->modifier;
	}

	if (sn == gsn_arrow) {
		bow = get_eq_char(ch, WEAR_WIELD);
		for (paf = bow->affected; paf; paf = paf->next) {
			if (paf->location == APPLY_DAMROLL)
				damroll += paf->modifier;
			if (paf->location == APPLY_HITROLL)
				hitroll += paf->modifier;
		}
	}

	dest_room = ch->in_room;
	chance += GET_NIMROLL(ch);

	damroll *= 10;

	while (1) {
		chance -= 10;

		if (victim->in_room == dest_room) {
			if (number_percent() < chance) { 
				if (check_obj_dodge(ch, victim, arrow, chance))
					return 0;
				act("$p strikes you!",
				    victim, arrow, NULL, TO_CHAR);
				act_puts("Your $p strikes $N!",
					 ch, arrow, victim, TO_CHAR, POS_DEAD);
				if (ch->in_room == victim->in_room)
					act("$n's $p strikes $N!",
					    ch, arrow, victim, TO_NOTVICT);
				else {
					act("$n's $p strikes $N!",
					    ch, arrow, victim, TO_ROOM);
					act("$p strikes $n!",
					    victim, arrow, NULL, TO_ROOM);
				}
				if (is_safe(ch, victim)
				||  (IS_NPC(victim) &&
				     IS_SET(victim->pIndexData->act, ACT_NOTRACK))) {
					act("$p falls from $n doing no visible damage...",
					    victim, arrow, NULL, TO_ALL);
					act("$p falls from $n doing no visible damage...",
					    ch, arrow, NULL, TO_CHAR);
					obj_to_room(arrow, victim->in_room);
				}
				else {
					int dam;

					dam = dice(arrow->value[ITEM_WEAPON_DICE_NUM],
						   arrow->value[ITEM_WEAPON_DICE_SIZE]);
					if (bow) 
						dam += dice(bow->value[ITEM_WEAPON_DICE_NUM],
							   bow->value[ITEM_WEAPON_DICE_SIZE]);
					dam = number_range(dam, 2 * dam);
					dam += damroll + bonus 
						+ get_curr_stat(ch, STAT_STR)/4;
					if (IS_WEAPON_STAT(arrow,
							   WEAPON_POISON)) {
						int level;
						AFFECT_DATA *poison, af;

		      	 if ((poison = affect_find(arrow->affected,gsn_poison)) == NULL)
		          	level = arrow->level;
		      	 else
		          	level = poison->level;
		      	 if (!saves_spell(level,victim,DAM_POISON))
		      	 {
		            char_puts("You feel poison coursing through your veins.",
		              victim);
		            act("$n is poisoned by the venom on $p.",
				victim,arrow,NULL,TO_ROOM);

		            af.where     = TO_AFFECTS;
		            af.type      = gsn_poison;
		            af.level     = level * 3/4;
		            af.duration  = level / 2;
		            af.location  = APPLY_STR;
		            af.modifier  = -1;
		            af.bitvector = AFF_POISON;
		            affect_join(victim, &af);
		      	 }

		  	}
		  	if (IS_WEAPON_STAT(arrow,WEAPON_FLAMING))
		  	{
		      	 act("$n is burned by $p.",victim,arrow,NULL,TO_ROOM);
		      	 act("$p sears your flesh.",victim,arrow,NULL,TO_CHAR);
		      	 fire_effect((void *) victim,arrow->level,dam,TARGET_CHAR);
		        }
		  	if (IS_WEAPON_STAT(arrow,WEAPON_FROST))
		        {
		            act("$p freezes $n.",victim,arrow,NULL,TO_ROOM);
		            act("The cold touch of $p surrounds you with ice.",
		                victim,arrow,NULL,TO_CHAR);
		            cold_effect(victim,arrow->level,dam,TARGET_CHAR);
		        }
		        if (IS_WEAPON_STAT(arrow,WEAPON_SHOCKING))
		        {
		            act("$n is struck by lightning from $p.",victim,arrow,NULL,TO_ROOM);
		            act("You are shocked by $p.",victim,arrow,NULL,TO_CHAR);
		            shock_effect(victim,arrow->level,dam,TARGET_CHAR);
		        }

			if (dam > victim->max_hit / 10 
				&& number_percent() < 50)
			{
			  af.where     = TO_AFFECTS;
			  af.type      = sn;
			  af.level     = ch->level; 
			  af.duration  = -1;
			  af.location  = APPLY_HITROLL;
			  af.modifier  = - (dam / 20);
			  if (IS_NPC(victim)) af.bitvector = 0;
				else af.bitvector = AFF_CORRUPTION;

			  affect_join(victim, &af);

			  obj_to_char(arrow,victim);
			  equip_char(victim,arrow,WEAR_STUCK_IN);
			}
		        else obj_to_room(arrow,victim->in_room); 

		DEBUG(DEBUG_SKILL_SHOOT,
			"shoot: %s[%d] vs %s[%d] hit: %d%% dam: %d",
			ch->name,
			LEVEL(ch),
			(victim) ? victim->name : "no-one",
			(victim) ? LEVEL(victim) : 0,
			chance,
			dam);
		
			damage(ch, victim, dam, sn, DAM_PIERCE, DAMF_SHOW);
			path_to_track(ch,victim,door);

		    }
		    return TRUE;
		  }
		  else {
		 	  obj_to_room(arrow,victim->in_room);
		          act("$p sticks in the ground at your feet!",victim,arrow,NULL, TO_ALL);
		          return FALSE;
		        }
		 }
		pExit = dest_room->exit[ door ];
		 if (!pExit) break;
		else {
			dest_room = pExit->to_room.r;
			if (dest_room->people) {
			 	act("$p sails into the room from the $T!",
				    dest_room->people, arrow,
				    dir_name[rev_dir[door]],
				    TO_ALL | ACT_TRANS);
			}

		}
	}
	return FALSE;
}

static OBJ_DATA *find_arrow(CHAR_DATA *ch)
{
	OBJ_DATA *arrow;
	OBJ_DATA *obj;

	if ((arrow = get_eq_char(ch, WEAR_HOLD)))
		return arrow;

	for (obj = ch->carrying; obj; obj = obj->next_content) {
		if (obj->wear_loc == WEAR_NONE
		||  obj->pIndexData->item_type != ITEM_CONTAINER
		||  !IS_SET(obj->value[ITEM_CONTAINER_FLAGS], CONT_QUIVER)
		||  !obj->contains
		||  (obj->contains->level > ch->level +15))
			continue;
		return obj->contains;
	}

	return NULL;
}

DO_FUN(do_charge) 
{
 	CHAR_DATA* victim;
	OBJ_DATA* wield;
	int chance, direction;
	EXIT_DATA *pexit;
	ROOM_INDEX_DATA *to_room;

	char arg1[512], arg2[512];


	if (IS_NPC(ch) || !(chance = get_skill(ch, gsn_charge))) {
		char_puts("Huh?\n", ch);
		return;
	}

	argument = one_argument(argument, arg1, sizeof(arg1));
	one_argument(argument, arg2, sizeof(arg2));

	if (arg1 == '\0' || arg2 == '\0') {
		char_puts("Charge whom?\n", ch);
		return;
	}

	if ((wield = get_eq_char(ch, WEAR_WIELD)) == NULL) {
		char_puts("You need a weapon to charge.\n", ch);
		return;
	}

	if (wield->value[ITEM_WEAPON_TYPE] != WEAPON_LANCE 
	&& wield->value[ITEM_WEAPON_TYPE] != WEAPON_SPEAR) {
		char_puts("You need lance or spear to charge.\n", ch);
		return;
	}

	if ((direction = find_exit(ch, arg1)) <0 || direction >= MAX_DIR) {
		char_puts("Charge whom?\n", ch);
		return;
	}

	if ((victim = find_char(ch, arg2, direction, 1)) == NULL) { 
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		return;
	}

	if (ch->in_room == victim->in_room) {
		act("$N is here. Just MURDER $M.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (ch->mount == NULL) {
		char_puts("You have to be riding.\n", ch);
		return;
	}

	if (is_safe(ch, victim))
		return;

	if (victim->hit < victim->max_hit*9/10) {
		act("$N is already bleeding, your honour do not allow you attack $M.", ch, NULL, victim, TO_CHAR);
		return;
	}

	chance = chance * get_skill(ch, gsn_riding)/100;

	if (!move_char_org(ch, direction, FALSE, TRUE))
		return;
	act("$n gallops from $t, charging you!",
	    ch, dir_name[rev_dir[direction]], victim, TO_VICT);
	act("$n gallops from $t, charging $N!",
	    ch, dir_name[rev_dir[direction]], victim, TO_NOTVICT);


	if (number_percent() < chance) {
		one_hit(ch, victim, gsn_charge, WEAR_WIELD);
		WAIT_STATE(victim, SKILL(gsn_charge)->beats * 2);
		WAIT_STATE(ch, SKILL(gsn_charge)->beats);
		check_improve(ch, gsn_charge, TRUE, 1);
	}
	else {
		damage(ch, victim, 0, gsn_charge, DAM_NONE, TRUE);
		check_improve(ch, gsn_charge, FALSE, 1);
		if (number_percent() > get_skill(ch, gsn_riding)) {
			if ((pexit=ch->in_room->exit[direction]) == NULL
			|| (to_room = pexit->to_room.r) == NULL
			|| !can_see_room(ch, to_room)
			|| IS_ROOM_AFFECTED(ch->in_room, RAFF_RANDOMIZER)
			|| IS_SET(pexit->exit_info, EX_CLOSED)) {
				WAIT_STATE(ch, SKILL(gsn_charge)->beats*2);
				return;
			}
			else {
				act("$n cannot hold $s $N.\n",
				    ch, dir_name[direction], ch->mount, TO_NOTVICT);
				act("You cannot hold your $N.",
				    ch, NULL, ch->mount, TO_CHAR);
				move_char(ch, direction, FALSE);
				WAIT_STATE(ch, SKILL(gsn_charge)->beats*5);
				return;
			}
		}
		WAIT_STATE(ch, SKILL(gsn_charge)->beats*2);
	}
}

DO_FUN(do_shoot)
{
	CHAR_DATA *victim;
	OBJ_DATA *wield;
	OBJ_DATA *arrow; 
	char arg1[512],arg2[512];
	bool success;
	int chance, direction;
	int range = (ch->level / 10) + 1;
	
	if (IS_NPC(ch))
		return; /* Mobs can't use bows */

	if (IS_NPC(ch) || (chance = get_skill(ch, gsn_bow)) == 0) {
		char_puts("You don't know how to shoot.\n",ch);
		return;
	}

	argument = one_argument(argument, arg1, sizeof(arg1));
	one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0' || arg2[0] == '\0') {
		char_puts("Shoot what direction and whom?\n", ch);
		return;
	}

	if (ch->fighting) {
		CHAR_DATA *vch;

		for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
			if (vch->fighting == ch)
				break;
		if (vch) {
			char_puts("You cannot concentrate "
				  "on shooting arrows.\n", ch);
			return;
		}
	}

	direction = find_exit(ch, arg1);

	if (direction < 0 || direction >= MAX_DIR) {
		char_puts("Shoot which direction and whom?\n",ch);
		return;
	}
		
	if ((victim = find_char(ch, arg2, direction, range)) == NULL) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("They aren't there.\n", ch);
		return;
	}

	if (!IS_NPC(victim) && victim->desc == NULL) {
		char_puts("You can't do that.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("That's pointless.\n", ch);
		return;
	}

	wield = get_eq_char(ch, WEAR_WIELD);

	if (!wield || wield->pIndexData->item_type != ITEM_WEAPON
	||  wield->value[ITEM_WEAPON_TYPE] != WEAPON_BOW) {
		char_puts("You need a bow to shoot!\n", ch);
		return;    	
	}

	if (get_eq_char(ch, WEAR_SECOND_WIELD)
	||  get_eq_char(ch, WEAR_SHIELD)) {
		char_puts("Your second hand should be free!\n",ch);
		return;    	
	}

	if ((arrow = find_arrow(ch)) == NULL) {
		 char_puts("You need an arrow to shoot!\n", ch);
		 return;    	
	}
		
	if (arrow->pIndexData->item_type != ITEM_WEAPON
	||  arrow->value[ITEM_WEAPON_TYPE] != WEAPON_ARROW) {
		char_puts("That's not the right kind of arrow!\n", ch);
		return;
	}
		
	if (is_safe(ch, victim))
		return;

	WAIT_STATE(ch, SKILL(gsn_bow)->beats);

	chance = (chance - 50) * 2;
	if (victim->position == POS_SLEEPING)
		chance += 40;
	if (victim->position == POS_RESTING)
		chance += 10;
	if (victim->position == POS_FIGHTING)
		chance -= 40;

	act_puts("You shoot $p to $T.",
		 ch, arrow, dir_name[direction], TO_CHAR | ACT_TRANS, POS_DEAD);
	act("$n shoots $p to $T.",
	    ch, arrow, dir_name[direction], TO_ROOM | ACT_TRANS);

	if (arrow->carried_by)
		obj_from_char(arrow);
	else if (arrow->in_obj)
		obj_from_obj(arrow);

	success = send_arrow(ch, victim, arrow, direction, chance,
			     dice(arrow->value[ITEM_WEAPON_DICE_NUM],
			     	arrow->value[ITEM_WEAPON_DICE_SIZE]));
	check_improve(ch, gsn_bow, TRUE, 1);
}

void do_human(CHAR_DATA *ch, const char *argument)
{
	if (!is_affected(ch, gsn_vampire)) {
		char_puts("You are already a human.\n", ch);
		return;
	}

	affect_strip(ch, gsn_vampire);
	char_puts("You return to your original size.\n", ch);
}

void do_throw_spear(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	OBJ_DATA *spear;
	char arg1[512],arg2[512];
	bool success;
	int chance,direction;
	int range = (ch->level / 10) + 1;

	if (IS_NPC(ch) || (chance = get_skill(ch, gsn_spear)) == 0) {
		char_puts("You don't know how to throw a spear.\n",ch);
		return;
	}

	argument = one_argument(argument, arg1, sizeof(arg1));
	one_argument(argument, arg2, sizeof(arg2));

  	if (arg1[0] == '\0' || arg2[0] == '\0') {
		char_puts("Throw spear what direction and whom?\n", ch);
		return;
	}

	if (ch->fighting) {
		CHAR_DATA *vch;

		for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
			if (vch->fighting == ch)
				break;
		if (vch) {
			char_puts("You cannot concentrate on throwing "
				  "spears.\n", ch);
			return;
		}
	}

	direction = find_exit(ch, arg1);
	if (direction < 0 || direction >= MAX_DIR) {
		char_puts("Throw which direction and whom?\n",ch);
		return;
	}
		
	if ((victim = find_char(ch, arg2, direction, range)) == NULL) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("They aren't there.\n", ch);
		return;
	}

	if (!IS_NPC(victim) && victim->desc == NULL) {
		char_puts("You can't do that.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("That's pointless.\n", ch);
		return;
	}

	spear = get_eq_char(ch, WEAR_WIELD);
	if (!spear || spear->pIndexData->item_type != ITEM_WEAPON
	||  spear->value[ITEM_WEAPON_TYPE] != WEAPON_SPEAR) {
		char_puts("You need a spear to throw!\n",ch);
		return;    	
	}

	if (get_eq_char(ch,WEAR_SECOND_WIELD) || get_eq_char(ch,WEAR_SHIELD)) {
		char_puts("Your second hand should be free!\n",ch);
		return;    	
	}

	if (is_safe(ch,victim))
		return;

	WAIT_STATE(ch, SKILL(gsn_spear)->beats);

	chance = (chance - 50) * 2;
	if (victim->position == POS_SLEEPING)
		chance += 40;
	if (victim->position == POS_RESTING)
		chance += 10;
	if (victim->position == POS_FIGHTING)
		chance -= 40;

	act_puts("You throw $p to $T.", 
		 ch, spear, dir_name[direction], TO_CHAR, POS_DEAD);
	act("$n throws $p to $T.",
	    ch, spear, dir_name[direction], TO_ROOM | ACT_TRANS);

	obj_from_char(spear);
	success = send_arrow(ch,victim,spear,direction,chance,
			dice(spear->value[ITEM_WEAPON_DICE_NUM],
				spear->value[ITEM_WEAPON_DICE_SIZE]));
	check_improve(ch, gsn_spear, TRUE, 1);
}

void do_settraps(CHAR_DATA *ch, const char *argument)
{
	int chance;

	if ((chance = get_skill(ch, gsn_settraps)) == 0) {
		char_puts("You don't know how to set traps.\n",ch);
		return;
	}

	if (!ch->in_room)
		return;

	if (IS_SET(ch->in_room->room_flags, ROOM_LAW)) {
		char_puts("A mystical power protects the room.\n",ch);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_settraps)->beats);

	if (IS_NPC(ch) || number_percent() <  chance * 7 / 10) {
	  AFFECT_DATA af,af2;

	  check_improve(ch,gsn_settraps,TRUE,1);

	  if (is_affected_room(ch->in_room, gsn_settraps))
	  {
	char_puts("This room has already trapped.\n",ch);
	return;
	   }

	  if (is_affected(ch,gsn_settraps))
	  {
	char_puts("This skill is used too recently.\n",ch);
	return;
	  }
   
	  af.where     = TO_ROOM_AFFECTS;
	  af.type      = gsn_settraps;
	  af.level     = ch->level;
	  af.duration  = ch->level / 40;
	  af.location  = APPLY_NONE;
	  af.modifier  = 0;
	  af.bitvector = RAFF_THIEF_TRAP;
	  affect_to_room(ch->in_room, &af);

	  af2.where     = TO_AFFECTS;
	  af2.type      = gsn_settraps;
	  af2.level	    = ch->level;
	
	  if (!IS_IMMORTAL(ch) && IS_PUMPED(ch))
	     af2.duration  = 1;
	  else af2.duration = ch->level / 10;

	  af2.modifier  = 0;
	  af2.location  = APPLY_NONE;
	  af2.bitvector = 0;
	  affect_to_char(ch, &af2);
	  char_puts("You set the room with your trap.\n", ch);
	  act("$n set the room with $s trap.",ch,NULL,NULL,TO_ROOM);
	  return;
	}
	else check_improve(ch,gsn_settraps,FALSE,1);

   return;
}

void do_forest(CHAR_DATA* ch, const char* argument)
{
	char arg[MAX_STRING_LENGTH];
	AFFECT_DATA af;
	bool attack;

	if (IS_NPC(ch) || !get_skill(ch, gsn_forest_fighting)) {
		char_puts("Huh?\n", ch);
		return;
	}
	
	one_argument(argument, arg, sizeof(arg));
	if (arg == '\0') {
		char_puts("Usage: forest {{ attack|defense|normal}", ch);
		return;
	}

	if (!str_prefix(arg, "normal")) {
		if (!is_affected(ch, gsn_forest_fighting)) {
			char_puts("You do not use your knowledge of forest "
				  "in fight.\n", ch);
			return;
		}
		else {
			char_puts("You stop using your knowledge of forest in "
				  "fight.\n", ch);
			affect_strip(ch, gsn_forest_fighting);
			return;
		}
	}

	if (!str_prefix(arg, "defense") || !str_prefix(arg, "defence"))
		attack = FALSE;
	else if (!str_prefix(arg, "attack"))
		attack = TRUE;
	else {
		char_puts("Usage: forest {{ attack|defense|normal}", ch);
		return;
	}

	if (is_affected(ch, gsn_forest_fighting))
		affect_strip(ch, gsn_forest_fighting);
	
	af.where 	= TO_AFFECTS;
	af.type  	= gsn_forest_fighting;
	af.level 	= ch->level;
	af.duration	= -1;
	af.bitvector	= 0;

	if (attack) {
		af.modifier	= ch->level/8;
		af.location	= APPLY_HITROLL;
		affect_to_char(ch, &af);
		af.location	= APPLY_DAMROLL;
		act_puts("You feel yourself wild.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		act("$n looks wild.", ch, NULL, NULL, TO_ROOM);
	}
	else {
		af.modifier	= -ch->level;
		af.location	= APPLY_AC;
		act_puts("You feel yourself protected.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		act("$n looks protected.", ch, NULL, NULL, TO_ROOM);
	}

	affect_to_char(ch, &af);
}

static OBJ_DATA *find_shuriken(CHAR_DATA *ch)
{
        OBJ_DATA *shuriken;

        if ((shuriken = get_eq_char(ch, WEAR_HOLD)))
                return shuriken;
        return NULL;
}

int send_shuriken(CHAR_DATA *ch, CHAR_DATA *victim,OBJ_DATA *shuriken,
               int door, int chance)
{
        EXIT_DATA *pExit;
        ROOM_INDEX_DATA *dest_room;
        AFFECT_DATA *paf;
        int hitroll = 0, sn;

        for (paf = shuriken->affected; paf != NULL; paf = paf->next) {
                if (paf->location == APPLY_HITROLL)
                        hitroll += paf->modifier;
        }
	sn = gsn_shuriken;
        dest_room = ch->in_room;
        chance += GET_NIMROLL(ch);
        while (1) {
                chance -= 10;
                if (victim->in_room == dest_room) {
                        if (number_percent() < chance) {
                                if (check_obj_dodge(ch, victim, shuriken, chance))
                                        return 0;
                                act("$p lands painfully in your body!",
                                    victim, shuriken, NULL, TO_CHAR);
                                act_puts("$p lands in $N!",
                                         ch, shuriken, victim, TO_CHAR, POS_DEAD);
                                if (ch->in_room == victim->in_room)
                                        act("$p lands painfully in $N!",
                                            ch, shuriken, victim, TO_NOTVICT);
                                else {
                                        act("$n's $p lands painfully in $N!",
                                            ch, shuriken, victim, TO_ROOM);
                                        act("$p lands painfully in $n!",
                                            victim, shuriken, NULL, TO_ROOM);
                                }
                                if (is_safe(ch, victim)
                                ||  (IS_NPC(victim) &&
                                     IS_SET(victim->pIndexData->act, ACT_NOTRACK))) {
                                        act("$p falls from $n doing no visible damage...",
                                            victim, shuriken, NULL, TO_ALL);
                                        act("$p falls from $n doing no visible damage...",
                                            ch, shuriken, NULL, TO_CHAR);
                                        obj_to_room(shuriken, victim->in_room);
                                }
                                else {
                                        int dam;

					dam = dice(shuriken->value[ITEM_WEAPON_DICE_NUM],
						   shuriken->value[ITEM_WEAPON_DICE_SIZE]);
                                        if (IS_WEAPON_STAT(shuriken,
                                                           WEAPON_POISON)) {
                                                int level;
                                                AFFECT_DATA *poison, af;

                         if ((poison = affect_find(shuriken->affected,gsn_poison)) == NULL)
                                level = shuriken->level;
                         else
                                level = poison->level;
                         if (!saves_spell(level,victim,DAM_POISON))
                         {
                            char_puts("You feel poison coursing through your veins.",
                              victim);
                            act("$n is poisoned by the venom on $p.",
                                victim,shuriken,NULL,TO_ROOM);

                            af.where     = TO_AFFECTS;
                            af.type      = gsn_poison;
                            af.level     = level * 3/4;
                            af.duration  = level / 2;
                            af.location  = APPLY_STR;
                            af.modifier  = -1;
                            af.bitvector = AFF_POISON;
                            affect_join(victim, &af);
                         }
                        }
		  	if (IS_WEAPON_STAT(shuriken,WEAPON_FLAMING))
		  	{
		      	 act("$n is burned by $p.",victim,shuriken,NULL,TO_ROOM);
		      	 act("$p sears your flesh.",victim,shuriken,NULL,TO_CHAR);
		      	 fire_effect((void *) victim,shuriken->level - 5,dam,TARGET_CHAR);
		        }
		  	if (IS_WEAPON_STAT(shuriken,WEAPON_FROST))
		        {
		            act("$p freezes $n.",victim,shuriken,NULL,TO_ROOM);
		            act("The cold touch of $p surrounds you with ice.",
		                victim,shuriken,NULL,TO_CHAR);
		            cold_effect(victim,shuriken->level - 5,dam,TARGET_CHAR);
		        }
		        if (IS_WEAPON_STAT(shuriken,WEAPON_SHOCKING))
		        {
		            act("$n is struck by lightning from $p.",victim,shuriken,NULL,TO_ROOM);
		            act("You are shocked by $p.",victim,shuriken,NULL,TO_CHAR);
		            shock_effect(victim,shuriken->level - 5,dam,TARGET_CHAR);
		        }

                        obj_to_room(shuriken,victim->in_room);

                        damage(ch, victim, dam, sn, DAM_PIERCE, DAMF_SHOW);
                        path_to_track(ch,victim,door);

                    }
                    return TRUE;
                  }
                  else {
                          obj_to_room(shuriken,victim->in_room);
                          act("$p lands in the ground at your feet!",victim,shuriken,NULL, TO_ALL);
                          return FALSE;
                        }
                 }
                pExit = dest_room->exit[ door ];
                 if (!pExit) break;
                else {
                        dest_room = pExit->to_room.r;
                        if (dest_room->people) {
                                act("$p whirs through the room from the $T!",
                                    dest_room->people, shuriken,
                                    dir_name[rev_dir[door]],
                                    TO_ALL | ACT_TRANS);
                        }

                }
        }
        return FALSE;

}

void do_shuriken(CHAR_DATA *ch, const char *argument)
{
        CHAR_DATA *victim;
        OBJ_DATA *shuriken;
        char arg1[512],arg2[512];
        bool success;
        int chance, direction;
        int range = 1;

        if (IS_NPC(ch))
                return; /* Mobs can't use shuriken */

        if (IS_NPC(ch) || (chance = get_skill(ch, gsn_shuriken)) == 0) {
                char_puts("You don't know how to throw shuriken.\n",ch);
                return;
        }

        argument = one_argument(argument, arg1, sizeof(arg1));
        one_argument(argument, arg2, sizeof(arg2));

        if (arg1[0] == '\0' || arg2[0] == '\0') {
                char_puts("Throw what direction and whom?\n", ch);
                return;
        }

        if (ch->fighting) {
                CHAR_DATA *vch;

                for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
                        if (vch->fighting == ch)
                                break;
                if (vch) {
                        char_puts("Your opponent is too close to throw at.\n", ch);
                        return;
                }
                
        }

        direction = find_exit(ch, arg1);

        if (direction < 0 || direction >= MAX_DIR) {
                char_puts("Throw which direction and whom?\n",ch);
                return;
        }

        if ((victim = find_char(ch, arg2, direction, range)) == NULL) {
                WAIT_STATE(ch, MISSING_TARGET_DELAY);
                char_puts("They aren't there.\n", ch);
                return;
        }

        if (!IS_NPC(victim) && victim->desc == NULL) {
                char_puts("You can't do that.\n", ch);
                return;
        }

        if (victim == ch) {
                char_puts("Shurikens, unlike boomerangs, do not come back to the thrower.\n", ch);
                return;
        }

        if (get_eq_char(ch, WEAR_WIELD)) {
                char_puts("You can't throw while wielding a weapon.\n", ch);
                return;
        }

        

        if ((shuriken = find_shuriken(ch)) == NULL) {
                 char_puts("You need a shuriken to throw!\n", ch);
                 return;
        }

        if (shuriken->pIndexData->item_type != ITEM_WEAPON
        ||  shuriken->value[ITEM_WEAPON_TYPE] != WEAPON_SHURIKEN) {
                char_puts("That's not the right kind of shuriken!\n", ch);
                return;
        }

        if (is_safe(ch, victim))
                return;

        WAIT_STATE(ch, SKILL(gsn_shuriken)->beats);

        chance = (chance - 50) * 2;
        if (victim->position == POS_SLEEPING)
                chance += 40;
        if (victim->position == POS_RESTING)
                chance += 10;
        if (victim->position == POS_FIGHTING)
                chance -= 40;

        chance += GET_HITROLL(ch);
	chance += get_curr_stat(ch, STAT_DEX) - get_curr_stat(victim, STAT_DEX);

        act_puts("You throw $p to $T.",
                 ch, shuriken, dir_name[direction], TO_CHAR | ACT_TRANS, POS_DEAD);
        act("$n throws $p to $T.",
            ch, shuriken, dir_name[direction], TO_ROOM | ACT_TRANS);

        if (shuriken->carried_by)
                obj_from_char(shuriken);
        else if (shuriken->in_obj)
                obj_from_obj(shuriken);

        success = send_shuriken(ch, victim, shuriken, direction, chance);
        check_improve(ch, gsn_shuriken, TRUE, 1);
}

static OBJ_DATA *find_dagger(CHAR_DATA *ch)
{
	OBJ_DATA *obj;
		for (obj = ch->carrying; obj; obj = obj->next_content)
		{
			if ((obj->wear_loc == WEAR_NONE) 
			&& (obj->pIndexData->item_type == ITEM_WEAPON)
			&& obj->value[ITEM_WEAPON_TYPE] == WEAPON_DAGGER)
				return obj;
		}
		return NULL;
}

int send_dagger(CHAR_DATA *ch, CHAR_DATA *victim,OBJ_DATA *dagger, int door,
		int chance ,int bonus)
{
        EXIT_DATA *pExit;
        ROOM_INDEX_DATA *dest_room;
        AFFECT_DATA *paf;
        int damroll = 0, hitroll = 0, sn;
        AFFECT_DATA af;
        
        sn = gsn_sling;

        for (paf = dagger->affected; paf != NULL; paf = paf->next) {
                if (paf->location == APPLY_DAMROLL)
                        damroll += paf->modifier;
                if (paf->location == APPLY_HITROLL)
                        hitroll += paf->modifier;
        }

        chance += GET_NIMROLL(ch);
        /*damroll *= 10;*/
        while (1) 
	{
                chance -= 10;                
                if (number_percent() < chance) 
		{
			if (check_obj_dodge(ch, victim, dagger, chance))
                                        return 0;
                        if (is_safe(ch, victim)) 
			{
                                act("$p rebounds harmlessly off of $n...",
                                    victim, dagger, NULL, TO_ALL);
                                act("$p rebounds harmlessly off of $n...",
                                    ch, dagger, NULL, TO_CHAR);
                                obj_to_room(dagger, victim->in_room);
                        }
                        else 
			{
                                int dam;

                                dam = dice(dagger->value[ITEM_WEAPON_DICE_NUM],
                                           dagger->value[ITEM_WEAPON_DICE_SIZE]) / 2;
                                dam += bonus 
					+ get_curr_stat(ch, STAT_STR)/4 
					* LEVEL(ch) / LEVEL_HERO;
                                if (IS_WEAPON_STAT(dagger,
                                                   WEAPON_POISON)) 
				{
                                        int level;
                                        AFFECT_DATA *poison, af;

                         		if ((poison = affect_find(dagger->affected,gsn_poison)) == NULL)
                                		level = dagger->level;
                         		else
                                		level = poison->level;
                         		if (!saves_spell(level,victim,DAM_POISON))
                         		{
                            			char_puts("You feel poison coursing through your veins.",
                              				victim);
                            			act("$n is poisoned by the venom on $p.",
                                			victim,dagger,NULL,TO_ROOM);

                            			af.where     = TO_AFFECTS;
                            			af.type      = gsn_poison;
                           			af.level     = level * 3/4;
                            			af.duration  = level / 2;
                            			af.location  = APPLY_STR;
                            			af.modifier  = -1;
                            			af.bitvector = AFF_POISON;
                            			affect_join(victim, &af);
                         		}

                        	}
                        	if (IS_WEAPON_STAT(dagger,WEAPON_FLAMING))
                        	{
                         		act("$n is burned by $p.",victim,dagger,NULL,TO_ROOM);
                         		act("$p sears your flesh.",victim,dagger,NULL,TO_CHAR);
                         		fire_effect((void *) victim,dagger->level,dam,TARGET_CHAR);
                        	}
                        	if (IS_WEAPON_STAT(dagger,WEAPON_FROST))
                        	{
                            		act("$p freezes $n.",victim,dagger,NULL,TO_ROOM);
                            		act("The cold touch of $p surrounds you with ice.",
                                		victim,dagger,NULL,TO_CHAR);
                            		cold_effect(victim,dagger->level,dam,TARGET_CHAR);
                        	}
                        	if (IS_WEAPON_STAT(dagger,WEAPON_SHOCKING))
                        	{
                            		act("$n is struck by lightning from $p.",victim,dagger,NULL,TO_ROOM);
                            		act("You are shocked by $p.",victim,dagger,NULL,TO_CHAR);
                            		shock_effect(victim,dagger->level,dam,TARGET_CHAR);
                        	}

                        	if (number_percent() < get_skill(ch, gsn_sling) - 50)
                        	{
                          		af.where     = TO_AFFECTS;
                          		af.type      = sn;
                          		af.level     = ch->level;
                          		af.duration  = -1;
                          		af.location  = APPLY_DEX;
                          		af.modifier  = - 1;
                          		af.bitvector = 0;                                

                          		affect_join(victim, &af);

                          		obj_to_char(dagger,victim);
                          		equip_char(victim,dagger,WEAR_STUCK_IN);
					act("You feel a surge of pain as $p buries itself in your leg.",
						victim, dagger, NULL,TO_CHAR);
					act("$n curses in pain as $p buries itself in $s leg.",
						victim, dagger, NULL,TO_ROOM);
                        	}
                        	else {
					obj_to_room(dagger,victim->in_room);
					act("$p strikes a glancing blow to $n, but doesn't lodge.",victim,dagger,NULL,TO_ROOM);
					act("$p strikes you a glancing blow, but doesn't lodge.",victim,dagger,NULL,TO_CHAR);
					dam /= 4;
				}			

                        damage(ch, victim, dam, sn, DAM_PIERCE, DAMF_SHOW);
                        path_to_track(ch,victim,door);

                    	}
                    	return TRUE;
                  }
                  else {
                          obj_to_room(dagger,victim->in_room);
                          act("$p sticks in the ground at your feet!",victim,dagger,NULL, TO_ALL);
                          return FALSE;
                  }
                pExit = dest_room->exit[ door ];
                 if (!pExit) break;
                else {
                        dest_room = pExit->to_room.r;
                        if (dest_room->people) {
                                act("$p darts through the room from the $T!",
                                    dest_room->people, dagger,
                                    dir_name[rev_dir[door]],
                                    TO_ALL | ACT_TRANS);
                        }

                }
        }
                

        return FALSE;
}



void do_sling(CHAR_DATA *ch, const char *argument)
{
        CHAR_DATA *victim;
        OBJ_DATA *dagger;
        char arg1[512],arg2[512];
        bool success;
        int chance, direction;
        int range = 1;

        if (IS_NPC(ch))
                return; /* Mobs can't sling */

        if (IS_NPC(ch) || (chance = get_skill(ch, gsn_sling)) == 0) {
                char_puts("You don't know how to sling daggers.\n",ch);
                return;
        }

        argument = one_argument(argument, arg1, sizeof(arg1));
        one_argument(argument, arg2, sizeof(arg2));

        if (arg1[0] == '\0' || arg2[0] == '\0') {
                char_puts("Throw what direction and whom?\n", ch);
                return;
        }

        if (ch->fighting) {
                CHAR_DATA *vch;

                for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
                        if (vch->fighting == ch)
                                break;
                if (vch) {
                        char_puts("Your opponent is too close.\n", ch);
                        return;
                }
                
        }

        direction = find_exit(ch, arg1);

        if (direction < 0 || direction >= MAX_DIR) {
                char_puts("Throw which direction and whom?\n",ch);
                return;
        }

        if ((victim = find_char(ch, arg2, direction, range)) == NULL) {
                WAIT_STATE(ch, MISSING_TARGET_DELAY);
                char_puts("They aren't there.\n", ch);
                return;
        }

        if (!IS_NPC(victim) && victim->desc == NULL) {
                char_puts("You can't do that.\n", ch);
                return;
        }

        if (victim == ch) {
	        char_puts("Trying to juggle knives?\n", ch);
	        return;
	}        

        if (get_eq_char(ch, WEAR_HOLD) != NULL
	&& get_eq_char(ch, WEAR_SHIELD) != NULL
	&& get_eq_char(ch, WEAR_SECOND_WIELD) != NULL) {
		char_puts("You must have a free hand to sling a dagger.\n", ch);
		return;
	}

	if ((dagger = find_dagger(ch)) == NULL) {
	        char_puts("You need a dagger to sling, quick search your pockets!\n", ch);
	        return;
	}

        if (is_safe(ch, victim))
                return;

        WAIT_STATE(ch, SKILL(gsn_sling)->beats);

        chance = (chance - 50) * 2;
        if (victim->position == POS_SLEEPING)
                chance += 40;
        if (victim->position == POS_RESTING)
                chance += 10;
        if (victim->position == POS_FIGHTING)
                chance -= 40;

	chance += get_curr_stat(ch, STAT_DEX) - get_curr_stat(victim, STAT_DEX);

        act_puts("You sling $p to $T.",
                 ch, dagger, dir_name[direction], TO_CHAR | ACT_TRANS, POS_DEAD);
        act("$n slings $p to $T.",
            ch, dagger, dir_name[direction], TO_ROOM | ACT_TRANS);

        if (dagger->carried_by)
                obj_from_char(dagger);
        else if (dagger->in_obj)
                obj_from_obj(dagger);

        success = send_dagger(ch, victim, dagger, direction, chance, 0);
        check_improve(ch, gsn_sling, TRUE, 1);
}

void do_hometown(CHAR_DATA *ch, const char *argument)
{
	int amount;
	int htn;
	race_t *r;
	class_t *cl;

	if (IS_NPC(ch)) {
		act_puts("You can't change your hometown!",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		return;
	}

	if ((r = race_lookup(ORG_RACE(ch))) == NULL
	||  !r->pcdata
	||  (cl = class_lookup(ch->class)) == NULL)
		return;

	if (!IS_SET(ch->in_room->room_flags, ROOM_REGISTRY)) {
		act_puts("You have to be in the Registry "
			 "to change your hometown.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		return;
	}

	if ((htn = hometown_permanent(ch)) >= 0) {
		act_puts("Your hometown is $t, permanently. "
			 "You can't change your hometown.",
			 ch, hometown_name(htn), NULL,
			 TO_CHAR | ACT_TRANS, POS_DEAD);
		return;
	}

	amount = (ch->level * 250) + 1000;

	if (argument[0] == '\0') {
		act_puts("The change of hometown will cost you $j gold.",
			 ch, (const void*) amount, NULL, TO_CHAR, POS_DEAD);
		char_puts("Choose from: ", ch);
		hometown_print_avail(ch);
		char_puts(".\n", ch);
		return;
	}

	if ((htn = htn_lookup(argument)) < 0) {
		act_puts("That's not a valid hometown.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		return;
	}

	if (htn == ch->hometown) {
		act_puts("But you already live in $t!",
			 ch, hometown_name(htn), NULL,
			 TO_CHAR | ACT_TRANS, POS_DEAD);
		return;
	}

	if (hometown_restrict(HOMETOWN(htn), ch)) {
		act_puts("You are not allowed there.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		return;
	}

	if (ch->pcdata->bank_g < amount) {
		act_puts("You don't have enough money in bank "
			 "to change hometowns!",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		return;
	}

	ch->hometown = htn;
	act_puts("Now your hometown is $t.",
		 ch, hometown_name(ch->hometown),
		 NULL, TO_CHAR | ACT_TRANS, POS_DEAD);
}

void do_bear_call(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *	gch;
	CHAR_DATA *	bear;
	CHAR_DATA *	bear2;
	AFFECT_DATA	af;
	int		i;
	int		chance;
	int		sn;
	int		mana;

	if ((sn = sn_lookup("bear call")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	char_puts("You call for bears help you.\n",ch);
	act("$n shouts a bear call.",ch,NULL,NULL,TO_ROOM);

	if (is_affected(ch, sn)) {
		char_puts("You cannot summon the strength to handle "
			     "more bears right now.\n", ch);
		return;
	}

	for (gch = npc_list; gch; gch = gch->next) {
		if (IS_AFFECTED(gch, AFF_CHARM)
		&&  gch->master == ch
		&&  gch->pIndexData->vnum == MOB_VNUM_BEAR) {
			char_puts("What's wrong with the bear you've got?",
				     ch);
			return;
		}
	}

	if (ch->in_room != NULL
	&&  IS_SET(ch->in_room->room_flags, ROOM_NOMOB)) {
		char_puts("No bears listen you.\n", ch);
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_SAFE | ROOM_PEACE |
					    ROOM_PRIVATE | ROOM_SOLITARY)
	||  (ch->in_room->exit[0] == NULL && ch->in_room->exit[1] == NULL
	&&   ch->in_room->exit[2] == NULL && ch->in_room->exit[3] == NULL
	&&   ch->in_room->exit[4] == NULL && ch->in_room->exit[5] == NULL)
	||  (ch->in_room->sector_type != SECT_FIELD
	&&   ch->in_room->sector_type != SECT_FOREST
	&&   ch->in_room->sector_type != SECT_MOUNTAIN
	&&   ch->in_room->sector_type != SECT_HILLS)) {
		char_puts("No bears come to your rescue.\n", ch);
		return;
	}

	mana = SKILL(sn)->min_mana;
	if (ch->mana < mana) {
		char_puts("You don't have enough mana "
			     "to shout a bear call.\n", ch);
		return;
	}
	ch->mana -= mana;

	if (number_percent() > chance) {
		char_puts("No bears listen you.\n", ch);
		check_improve(ch, sn, FALSE, 1);
		return;
	}

	check_improve(ch, sn, TRUE, 1);
	bear = create_mob(get_mob_index(MOB_VNUM_BEAR));

	for (i=0;i < MAX_STATS; i++)
		bear->perm_stat[i] = UMIN(25,2 * ch->perm_stat[i]);

	bear->max_hit = IS_NPC(ch) ? ch->max_hit : ch->pcdata->perm_hit;
	bear->hit = bear->max_hit;
	bear->max_mana = IS_NPC(ch) ? ch->max_mana : ch->pcdata->perm_mana;
	bear->mana = bear->max_mana;
	bear->alignment = ch->alignment;
	bear->level = UMIN(100, 1 * ch->level-2);
	for (i=0; i < 3; i++)
		bear->armor[i] = interpolate(bear->level, 100, -100);
	bear->armor[3] = interpolate(bear->level, 100, 0);
	bear->sex = ch->sex;
	bear->gold = 0;

	bear2 = create_mob(bear->pIndexData);
	clone_mob(bear, bear2);

	SET_BIT(bear->affected_by, AFF_CHARM);
	SET_BIT(bear2->affected_by, AFF_CHARM);
	bear->master = bear2->master = ch;
	bear->leader = bear2->leader = ch;

	char_puts("Two bears come to your rescue!\n",ch);
	act("Two bears come to $n's rescue!", ch, NULL, NULL, TO_ROOM);

	af.where	= TO_AFFECTS;
	af.type 	= sn;
	af.level	= ch->level;
	af.duration	= SKILL(sn)->beats;
	af.bitvector	= 0;
	af.modifier	= 0;
	af.location	= APPLY_NONE;
	affect_to_char(ch, &af);

	char_to_room(bear, ch->in_room);
	char_to_room(bear2, ch->in_room);
}

void do_lion_call(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *	gch;
	CHAR_DATA *	lion;
	CHAR_DATA *	lion2;
	AFFECT_DATA	af;
	int		i;
	int		chance;
	int		sn;
	int		mana;

	if ((sn = sn_lookup("lion call")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	char_puts("You call for lions help you.\n",ch);
	act("$n shouts a lion call.",ch,NULL,NULL,TO_ROOM);

	if (is_affected(ch, sn)) {
		char_puts("You cannot summon the strength to handle "
			     "more lions right now.\n", ch);
		return;
	}

	for (gch = npc_list; gch; gch = gch->next) {
		if (IS_AFFECTED(gch,AFF_CHARM)
		&&  gch->master == ch
		&&  gch->pIndexData->vnum == MOB_VNUM_LION) {
			char_puts("What's wrong with the lion you've got?", ch);
			return;
		}
	}

	if (ch->in_room != NULL
	&& IS_SET(ch->in_room->room_flags, ROOM_NOMOB)) {
		char_puts("No lions can listen you.\n", ch);
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_SAFE | ROOM_PEACE |
					    ROOM_PRIVATE | ROOM_SOLITARY)
	||  (ch->in_room->exit[0] == NULL && ch->in_room->exit[1] == NULL
	&&   ch->in_room->exit[2] == NULL && ch->in_room->exit[3] == NULL
	&&   ch->in_room->exit[4] == NULL && ch->in_room->exit[5] == NULL)
	||  (ch->in_room->sector_type != SECT_FIELD
	&&   ch->in_room->sector_type != SECT_FOREST
	&&   ch->in_room->sector_type != SECT_MOUNTAIN
	&&   ch->in_room->sector_type != SECT_HILLS)) {
		char_puts("No lions come to your rescue.\n", ch);
		return;
	}

	mana = SKILL(sn)->min_mana;
	if (ch->mana < mana) {
		char_puts("You don't have enough mana "
			     "to shout a lion call.\n", ch);
		return;
	}
	ch->mana -= mana;

	if (number_percent() > chance) {
		check_improve(ch, sn, FALSE, 1);
		char_puts("No lions listen you.\n", ch);
		return;
	}

	check_improve(ch, sn, TRUE, 1);
	lion = create_mob(get_mob_index(MOB_VNUM_LION));

	for (i=0;i < MAX_STATS; i++)
		lion->perm_stat[i] = UMIN(25,2 * ch->perm_stat[i]);

	lion->max_hit = IS_NPC(ch) ? ch->max_hit : ch->pcdata->perm_hit;
	lion->hit = lion->max_hit;
	lion->max_mana = IS_NPC(ch) ? ch->max_mana : ch->pcdata->perm_mana;
	lion->mana = lion->max_mana;
	lion->alignment = ch->alignment;
	lion->level = UMIN(100,1 * ch->level-2);
	for (i=0; i < 3; i++)
		lion->armor[i] = interpolate(lion->level,100,-100);
	lion->armor[3] = interpolate(lion->level,100,0);
	lion->sex = ch->sex;
	lion->gold = 0;

	lion2 = create_mob(lion->pIndexData);
	clone_mob(lion,lion2);

	SET_BIT(lion->affected_by, AFF_CHARM);
	SET_BIT(lion2->affected_by, AFF_CHARM);
	lion->master = lion2->master = ch;
	lion->leader = lion2->leader = ch;

	char_puts("Two lions come to your rescue!\n",ch);
	act("Two lions come to $n's rescue!",ch,NULL,NULL,TO_ROOM);

	af.where	= TO_AFFECTS;
	af.type 	= sn;
	af.level	= ch->level;
	af.duration	= SKILL(sn)->beats;
	af.bitvector	= 0;
	af.modifier	= 0;
	af.location	= APPLY_NONE;
	affect_to_char(ch, &af);

	char_to_room(lion, ch->in_room);
	char_to_room(lion2, ch->in_room);
}

void do_camp(CHAR_DATA *ch, const char *argument)
{
	AFFECT_DATA af;
	int sn;
	int chance;
	int mana;

	if ((sn = sn_lookup("camp")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	if (is_affected(ch, sn)) {
		char_puts("You don't have enough power to handle more "
			     "camp areas.\n", ch);
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_SAFE | ROOM_PEACE |
					    ROOM_PRIVATE | ROOM_SOLITARY)
	||  (ch->in_room->sector_type != SECT_FIELD
	&&   ch->in_room->sector_type != SECT_FOREST
	&&   ch->in_room->sector_type != SECT_MOUNTAIN
	&&   ch->in_room->sector_type != SECT_HILLS)) {
		char_puts("There are not enough leaves to camp here.\n",
			     ch);
		return;
	}

	mana = SKILL(sn)->min_mana;
	if (ch->mana < mana) {
		char_puts("You don't have enough mana to make a camp.\n",
			     ch);
		return;
	}
	ch->mana -= mana;

	if (number_percent() > chance) {
		char_puts("You failed to make your camp.\n", ch);
		check_improve(ch, sn, FALSE, 4);
		return;
	}

	check_improve(ch, sn, TRUE, 4);
	WAIT_STATE(ch, SKILL(sn)->beats);

	char_puts("You succeeded to make your camp.\n", ch);
	act("$n succeeded to make $s camp.", ch, NULL, NULL, TO_ROOM);

	af.where	= TO_AFFECTS;
	af.type 	= sn;
	af.level	= ch->level;
	af.duration	= 12;
	af.bitvector	= 0;
	af.modifier	= 0;
	af.location	= APPLY_NONE;
	affect_to_char(ch, &af);

	af.where	= TO_ROOM_CONST;
	af.type		= sn;
	af.level	= ch->level;
	af.duration	= ch->level / 20;
	af.bitvector	= 0;
	af.modifier	= 2 * ch->level;
	af.location	= APPLY_ROOM_HEAL;
	affect_to_room(ch->in_room, &af);

	af.modifier	= ch->level;
	af.location	= APPLY_ROOM_MANA;
	affect_to_room(ch->in_room, &af);
}

void do_request(CHAR_DATA *ch, const char *argument)
{
	char arg1 [MAX_INPUT_LENGTH];
	char arg2 [MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA  *obj;
	AFFECT_DATA af;

	if (is_affected(ch, gsn_reserved)) {
		char_puts("Wait for a while to request again.\n", ch);
		return;
	}

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (IS_NPC(ch))
		return;

	if (arg1[0] == '\0' || arg2[0] == '\0') {
		char_puts("Request what from whom?\n", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg2)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (!IS_NPC(victim)) {
		char_puts("Why don't you just ask the player?\n", ch);
		return;
	}

	if (!IS_GOOD(ch)) {
		do_say(victim,
		       "I will not give anything to someone so impure.");
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	if (IS_AFFECTED(victim, AFF_CHARM) && victim->master != ch) {
		do_say(victim,
		       "Ask my master, if you want something.");
		return;
	}

	if (((obj = get_obj_carry(victim , arg1)) == NULL
	&&  (obj = get_obj_wear(victim, arg1)) == NULL)
	||  IS_SET(obj->extra_flags, ITEM_INVENTORY)) {
		do_say(victim, "Sorry, I don't have that.");
		return;
	}
	
	if (IS_NEWBIE(ch) && obj->pIndexData->limit >= 0) {
		do_say(victim, "I'm sorry, newbie, I cannot offer you this artifact.");
		return;
	}

	if(obj->level <= victim->level)
	{
		if (obj->level >= ch->level + 16 || obj->level >= ch->level * 2) {
                do_say(victim, "In good time, my child");
                return;
		}
        }
	else
	{
		if(victim->level >= ch->level + 16 || victim->level >= ch->level * 2){
		do_say(victim, "In good time, my child");
		return;
		}
	}

	if (!IS_GOOD(victim)) {
		do_say(victim, "I'm not about to give you anything!");
		do_murder(victim, ch->name);
		return;
	}

	if (obj->wear_loc != WEAR_NONE)
		unequip_char(victim, obj);

	if (!can_drop_obj(ch, obj)) {
		do_say(victim, "Sorry, I can't let go of it.  It's cursed.");
		return;
	}

	if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch)) {
		char_puts("Your hands are full.\n", ch);
		return;
	}

	if (ch->carry_weight + get_obj_weight(obj) > ch_max_carry_weight(ch)) {
		char_puts("You can't carry that much weight.\n", ch);
		return;
	}

	if (!can_see_obj(ch, obj)) {
		act("You don't see that.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (IS_SET(obj->extra_flags, ITEM_QUIT_DROP)) {
		do_say(victim, "Sorry, I must keep it myself.");
		return;
	}

	obj_from_char(obj);
	obj_to_char(obj, ch);
	act("$n requests $p from $N.", ch, obj, victim, TO_NOTVICT);
	act("You request $p from $N.",	 ch, obj, victim, TO_CHAR);
	act("$n requests $p from you.", ch, obj, victim, TO_VICT);

	oprog_call(OPROG_GIVE, obj, ch, victim);

	act("You feel grateful for the trust of $N.", ch, NULL, victim,
	    TO_CHAR);
	char_puts("and for the goodness you have seen in the world.\n",ch);

	af.type = gsn_reserved;
	af.where = TO_AFFECTS;
	af.level = ch->level;
	af.duration = 2;
	af.location = APPLY_NONE;
	af.modifier = 0;
	af.bitvector = 0;
	affect_to_char(ch, &af);
}

void do_demand(CHAR_DATA *ch, const char *argument)
{
	char arg1 [MAX_INPUT_LENGTH];
	char arg2 [MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA  *obj;
	int chance;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (IS_NPC(ch))
		return;

	if (arg1[0] == '\0' || arg2[0] == '\0') {
		char_puts("Demand what from whom?\n", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg2)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (IS_SET(ch->state_flags, STATE_GHOST)) {
		char_puts("You're still a ghost.\n", ch);
		return;
	}

	if (!IS_NPC(victim)) {
		char_puts("Why don't you just want that directly "
			     "from the player?\n", ch);
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	chance = IS_EVIL(victim) ? 10 : IS_GOOD(victim) ? -5 : 0;
	chance += get_curr_stat(ch,STAT_CHA);
	chance += ch->level - victim->level;

	chance = (get_skill(ch, gsn_demand))*chance/100;

	if (number_percent() > chance) {
		do_say(victim, "I'm not about to give you anything!");
		check_improve(ch, gsn_demand, FALSE, 1);
		do_murder(victim, ch->name);
		return;
	}

	check_improve(ch, gsn_demand, TRUE, 1);

	if (((obj = get_obj_carry(victim , arg1)) == NULL
	&&   (obj = get_obj_wear(victim, arg1)) == NULL)
	||  IS_SET(obj->extra_flags, ITEM_INVENTORY)) {
		do_say(victim, "Sorry, I don't have that.");
		return;
	}

	if (IS_SET(obj->extra_flags, ITEM_QUIT_DROP)) {
		do_say(victim, "Forgive me, my master, I can't give it to anyone.");
		return;
	}


	if (obj->wear_loc != WEAR_NONE)
		unequip_char(victim, obj);

	if (!can_drop_obj(ch, obj)) {
		do_say(victim, "It's cursed so, I can't let go of it. "
			       "Forgive me, my master");
		return;
	}

	if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch)) {
		char_puts("Your hands are full.\n", ch);
		return;
	}

	if (ch->carry_weight + get_obj_weight(obj) > ch_max_carry_weight(ch)) {
		char_puts("You can't carry that much weight.\n", ch);
		return;
	}

	if (!can_see_obj(ch, obj)) {
		act("You don't see that.", ch, NULL, victim, TO_CHAR);
		return;
	}

	obj_from_char(obj);
	obj_to_char(obj, ch);
	act("$n demands $p from $N.", ch, obj, victim, TO_NOTVICT);
	act("You demand $p from $N.",	ch, obj, victim, TO_CHAR  );
	act("$n demands $p from you.", ch, obj, victim, TO_VICT  );

	oprog_call(OPROG_GIVE, obj, ch, victim);
	char_puts("Your power makes all around the world shivering.\n",ch);
}

void do_control(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int chance;
	int sn;
	race_t *r;

	argument = one_argument(argument, arg, sizeof(arg));

	if ((sn = sn_lookup("control animal")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	if (arg[0] == '\0') {
		char_puts("Charm what?\n", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if ((r = race_lookup(ORG_RACE(victim))) && r->pcdata) {
		char_puts("You should try this on monsters?\n", ch);
		return;
	}

	if (count_charmed(ch))
		return;

	if (is_safe(ch, victim))
		return;

	WAIT_STATE(ch, SKILL(sn)->beats);

	chance += get_curr_stat(ch,STAT_CHA);
	chance += (ch->level - victim->level) * 3;
	chance +=
	(get_curr_stat(ch,STAT_INT) - get_curr_stat(victim,STAT_INT));

	if (IS_AFFECTED(victim, AFF_CHARM)
	||  IS_AFFECTED(ch, AFF_CHARM)
	||  number_percent() > chance
	||  ch->level < (victim->level + 2)
	||  IS_SET(victim->imm_flags,IMM_CHARM)
	||  (IS_NPC(victim) && victim->pIndexData->pShop != NULL)) {
		check_improve(ch, sn, FALSE, 2);
		do_say(victim,"I'm not about to follow you!");
		do_murder(victim, ch->name);
		return;
	}

	check_improve(ch, sn, TRUE, 2);

	if (victim->master)
		stop_follower(victim);
	SET_BIT(victim->affected_by, AFF_CHARM);
	victim->master = victim->leader = ch;

	act("Isn't $n just so nice?", ch, NULL, victim, TO_VICT);
	if (ch != victim)
		act("$N looks at you with adoring eyes.",
		    ch, NULL, victim, TO_NOTVICT);
}

void do_make_arrow(CHAR_DATA *ch, const char *argument)
{
	OBJ_DATA *arrow;
	AFFECT_DATA af;
	OBJ_INDEX_DATA *pObjIndex;
	int count, mana, wait;
	char arg[MAX_INPUT_LENGTH];
	int chance;
	int color_chance = 100;
	int vnum = -1;
	int sn;
	int color = -1;

	if (IS_NPC(ch))
		return;

	if ((sn = sn_lookup("make arrow")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("You don't know how to make arrows.\n", ch);
		return;
	}

	if (ch->in_room->sector_type != SECT_FIELD
	&&  ch->in_room->sector_type != SECT_FOREST
	&&  ch->in_room->sector_type != SECT_HILLS) {
		char_puts("You couldn't find enough wood.\n", ch);
		return;
	}

	mana = SKILL(sn)->min_mana;
	wait = SKILL(sn)->beats;

	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0')
		vnum = OBJ_VNUM_WOODEN_ARROW;
	else if (!str_prefix(arg, "green")) {
		color = sn_lookup("green arrow");
		vnum = OBJ_VNUM_GREEN_ARROW;
	}
	else if (!str_prefix(arg, "red")) {
		color = sn_lookup("red arrow");
		vnum = OBJ_VNUM_RED_ARROW;
	}
	else if (!str_prefix(arg, "white")) {
		color = sn_lookup("white arrow");
		vnum = OBJ_VNUM_WHITE_ARROW;
	}
	else if (!str_prefix(arg, "blue")) {
		color = sn_lookup("blue arrow");
		vnum = OBJ_VNUM_BLUE_ARROW;
	}

	if (vnum < 0) {
		char_puts("You don't know how to make "
			  "that kind of arrow.\n", ch);
		return;
	}

	if (color > 0) {
		color_chance = get_skill(ch, color);
		mana += SKILL(color)->min_mana;
		wait += SKILL(color)->beats;
	}

	if (ch->mana < mana) {
		char_puts("You don't have enough energy "
			  "to make that kind of arrows.\n", ch);
		return;
	}

	ch->mana -= mana;
	WAIT_STATE(ch, wait);

	char_puts("You start to make arrows!\n",ch);
	act("$n starts to make arrows!", ch, NULL, NULL, TO_ROOM);
	pObjIndex = get_obj_index(vnum);
	for(count = 0; count < ch->level / 5; count++) {
		if (number_percent() > chance * color_chance / 100) {
			char_puts("You failed to make the arrow, "
				  "and broke it.\n", ch);
			check_improve(ch, sn, FALSE, 3);
			if (color > 0)
				check_improve(ch, color, FALSE, 3);
			continue;
		}

		check_improve(ch, sn, TRUE, 3);
		if (color > 0)
			check_improve(ch, color, TRUE, 3);

		arrow = create_obj(pObjIndex, 0);
		arrow->level = ch->level;
		autoweapon(arrow, 100);

		arrow->owner = mlstr_dup(ch->short_descr);

		af.where	 = TO_OBJECT;
		af.type		 = sn;
		af.level	 = ch->level;
		af.duration	 = -1;
		af.location	 = APPLY_HITROLL;
		af.modifier	 = ch->level / 10;
		af.bitvector 	 = 0;
		affect_to_obj(arrow, &af);

		af.where	= TO_OBJECT;
		af.type		= sn;
		af.level	= ch->level;
		af.duration	= -1;
		af.location	= APPLY_DAMROLL;
		af.modifier	= ch->level / 10;
		af.bitvector	= 0;
		affect_to_obj(arrow, &af);

		obj_to_char(arrow, ch);
		act_puts("You successfully make $p.",
			 ch, arrow, NULL, TO_CHAR, POS_DEAD);
	}
}

void do_make_bow(CHAR_DATA *ch, const char *argument)
{
	OBJ_DATA *	bow;
	AFFECT_DATA	af;
	int		mana;
	int		sn;
	int		chance;

	if (IS_NPC(ch))
		return;

	if ((sn = sn_lookup("make bow")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("You don't know how to make bows.\n", ch);
		return;
	}

	if (ch->in_room->sector_type != SECT_FIELD
	&&  ch->in_room->sector_type != SECT_FOREST
	&&  ch->in_room->sector_type != SECT_HILLS) {
		char_puts("You couldn't find enough wood.\n", ch);
		return;
	}

	mana = SKILL(sn)->min_mana;
	if (ch->mana < mana) {
		char_puts("You don't have enough energy to make a bow.\n",
			     ch);
		return;
	}
	ch->mana -= mana;
	WAIT_STATE(ch, SKILL(sn)->beats);

	if (number_percent() > chance) {
		char_puts("You failed to make the bow, and broke it.\n",
			     ch);
		check_improve(ch, sn, FALSE, 1);
		return;
	}
	check_improve(ch, sn, TRUE, 1);

	bow = create_obj(get_obj_index(OBJ_VNUM_RANGER_BOW), 0);
	bow->level = ch->level;
	autoweapon(bow, 100);

	af.where	= TO_OBJECT;
	af.type		= sn;
	af.level	= ch->level;
	af.duration	= -1;
	af.location	= APPLY_HITROLL;
	af.modifier	= ch->level / 10;
	af.bitvector 	= 0;
	affect_to_obj(bow, &af);

	af.where	= TO_OBJECT;
	af.type		= sn;
	af.level	= ch->level;
	af.duration	= -1;
	af.location	= APPLY_DAMROLL;
	af.modifier	= ch->level / 10;
	af.bitvector 	= 0;
	affect_to_obj(bow, &af);

	obj_to_char(bow, ch);
	act_puts("You successfully make $p.", ch, bow, NULL, TO_CHAR, POS_DEAD);
}

void do_make_katana(CHAR_DATA *ch, const char *argument)
{
	OBJ_DATA *katana;
	AFFECT_DATA af;
	OBJ_DATA *part;
	char arg[MAX_INPUT_LENGTH];
	int chance;
	int mana;

	one_argument(argument, arg, sizeof(arg));
	
	if ((chance = get_skill(ch, gsn_make_katana)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}
	
	if (is_affected(ch, gsn_make_katana)) {
		char_puts("But you've already got one katana!\n", ch);
		return;
	}
	
	mana = SKILL(gsn_make_katana)->min_mana;
	if (ch->mana < mana) {
		char_puts("You feel too weak to concentrate on a katana.\n",
			  ch);
		return;
	}
	
	if (arg[0] == '\0') {
		char_puts("Make a katana from what?\n", ch);
		return;
	}
	
	if ((part = get_obj_carry(ch, arg)) == NULL) {
		char_puts("You do not have chunk of iron.\n", ch);
		return;
	}

	if (part->pIndexData->vnum != OBJ_VNUM_CHUNK_IRON) {
		char_puts("You do not have the right material.\n", ch);
		return;
	}

	if (number_percent() < chance / 3 * 2) {
		char_puts("You failed and destroyed it.\n", ch);
		extract_obj(part, 0);
		return;
	} 

	WAIT_STATE(ch, SKILL(gsn_make_katana)->beats);

	if (number_percent() < chance) {
		af.where	= TO_AFFECTS;
		af.type		= gsn_katana;
		af.level	= ch->level;
		af.duration	= ch->level;
		af.modifier	= 0;
		af.bitvector 	= 0;      
		af.location	= 0;
		affect_to_char(ch,&af);
	
		katana = create_obj(get_obj_index(OBJ_VNUM_KATANA_SWORD), 0);
		katana->level = ch->level;
		autoweapon(katana, 100);
		katana->owner = mlstr_dup(ch->short_descr);
		katana->cost  = 0;
		ch->mana -= mana;

		af.where	= TO_OBJECT;
		af.type 	= gsn_katana;
		af.level	= ch->level;
		af.duration	= -1;
		af.location	= APPLY_DAMROLL;
		af.modifier	= ch->level / 10;
		af.bitvector	= 0;
		affect_to_obj(katana, &af);

		af.location	= APPLY_HITROLL;
		affect_to_obj(katana, &af);

		katana->ed = ed_new2(katana->pIndexData->ed, ch->name);
			
		obj_to_char(katana, ch);
		check_improve(ch, gsn_make_katana, TRUE, 1);
			
		act("You make a katana from $p!",ch,part,NULL,TO_CHAR);
		act("$n makes a katana from $p!",ch,part,NULL,TO_ROOM);
			
		extract_obj(part, 0);
		return;
	}
	else {
		char_puts("You destroyed it.\n", ch);
		extract_obj(part, 0);
		ch->mana -= mana/2;
		check_improve(ch, gsn_make_katana, FALSE, 1);
	}
}

/*Added by Osya*/
void do_homepoint(CHAR_DATA *ch, const char *argument)
{
        AFFECT_DATA af;
        int sn;
        int chance;
        char arg[MAX_INPUT_LENGTH];

        if ((sn = sn_lookup("homepoint")) < 0
        ||  (chance = get_skill(ch, sn)) == 0) {
                char_puts("Huh?\n", ch);
                return;
        }

        if (is_affected(ch, sn)) {
                char_puts("You fatigue for searching new home.\n", ch) ;
                return;
        }

        if (IS_SET(ch->in_room->room_flags, ROOM_SAFE | ROOM_PEACE |
                                            ROOM_PRIVATE | ROOM_SOLITARY)
        ||  (ch->in_room->sector_type != SECT_FIELD
        &&   ch->in_room->sector_type != SECT_FOREST
        &&   ch->in_room->sector_type != SECT_MOUNTAIN
        &&   ch->in_room->sector_type != SECT_HILLS)) {
                char_puts("You are cannot set home here.\n",
                             ch);
                return;
        }

        if (ch->mana < ch->max_mana) {
                char_puts("You don't have strength to make a new home.\n",
                             ch);
                return;
        }

        ch->mana = 0 ;

        if (number_percent() > chance) {
                char_puts("You failed to make your homepoint.\n", ch);
                check_improve(ch, sn, FALSE, 4);
                return;
        }

        check_improve(ch, sn, TRUE, 4);
        WAIT_STATE(ch, SKILL(sn)->beats);

        char_puts("You succeeded to make your homepoint.\n", ch);
        act("$n succeeded to make $s homepoint. ", ch, NULL, NULL, TO_ROOM);

        af.where        = TO_AFFECTS;
        af.type         = sn;
        af.level        = ch->level;
        af.duration     = 100;
        af.bitvector    = 0;
        af.modifier     = 0;
        af.location     = APPLY_NONE;
        affect_to_char(ch, &af);

        argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] && !str_prefix(arg, "motherland"))
		ch->pcdata->homepoint = NULL;
        else 
		ch->pcdata->homepoint = ch->in_room; 
}

void do_make_shuriken(CHAR_DATA *ch, const char *argument)
{
        OBJ_DATA *shuriken;
        int mana;
        int chance;
        int sn;
        int count;

        if (IS_NPC(ch))
                return;

        if ((sn = sn_lookup("make shuriken")) < 0
        ||  (chance = get_skill(ch, sn)) == 0) {
                char_puts("You do not know the art of creating shurikens.\n", ch);
                return;
        }

        mana = SKILL(sn)->min_mana;

        if (ch->mana < mana) {
                char_puts("You don't have enough energy "
                          "to make a shuriken.\n", ch);
                return;
        }

        ch->mana -= mana;
        WAIT_STATE(ch, SKILL(sn)->beats);

        char_puts("You settle down to create shurikens.\n",ch);
        act("$n begins creating shurikens.", ch, NULL, NULL, TO_ROOM);
        for(count = 0; count < ch->level / 15; count++) {
                if (number_percent() > chance) {
                        char_puts("You cut the shuriken too small and it broke.\n", ch);
                        check_improve(ch, sn, FALSE, 3);
                        continue;
                }

                check_improve(ch, sn, TRUE, 3);

                shuriken = create_obj(get_obj_index(OBJ_VNUM_SHURIKEN), 0);
                shuriken->level = ch->level;
		autoweapon(shuriken, 100);
                obj_to_char(shuriken, ch);
                act_puts("You successfully create $p.",
                         ch, shuriken, NULL, TO_CHAR, POS_DEAD);
        }
}

void do_make(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("What should you make?\n",ch);
		return;
	}

	if (!str_prefix(arg, "arrow"))
		do_make_arrow(ch, argument);
	else if (!str_prefix(arg, "bow"))
		do_make_bow(ch, argument);
	else if(!str_prefix(arg, "shuriken"))
		do_make_shuriken(ch, argument);
	else if(!str_prefix(arg, "katana"))
		do_make_katana(ch, argument);
	else
		do_make(ch, str_empty);
}

void do_detect_hidden(CHAR_DATA *ch, const char *argument)
{
	AFFECT_DATA	af;
	int		chance;
	int		sn;

	if ((sn = sn_lookup("detect hide")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_DETECT_HIDDEN)) {
		char_puts("You are already as alert as you can be. \n",ch);
		return;
	}

	if (number_percent() > chance) {
		char_puts("You peer intently at the shadows "
			     "but they are unrevealing.\n", ch);
		check_improve(ch, sn, FALSE, 1);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = ch->level;
	af.duration  = ch->level;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = AFF_DETECT_HIDDEN;
	affect_to_char(ch, &af);
	char_puts("Your awareness improves.\n", ch);
	check_improve(ch, sn, TRUE, 1);
}

void do_resistance(CHAR_DATA *ch, const char *argument)
{
	int chance;
	int mana;
	pcskill_t* ps;
	
	if (IS_NPC(ch)) 
		return;
	if ((ps = pcskill_lookup(ch, gsn_resistance)) == NULL
	||  skill_level(ch, gsn_resistance) > ch->level)
		chance = 0;
	else
		chance = ps->percent;

	if (chance == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	if (is_affected(ch, gsn_resistance)) {
		char_puts("You are as resistant as you will get.\n", ch);
		return;
	}

	mana = SKILL(gsn_resistance)->min_mana;
	if (ch->mana < mana) {
		char_puts("You cannot muster up the energy.\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_resistance)->beats);

	if (number_percent() < chance) {
		AFFECT_DATA af;
		
		af.where	= TO_AFFECTS;
		af.type 	= gsn_resistance;
		af.level 	= ch->level;
		af.duration	= ch->level / 6;
		af.location	= APPLY_NONE;
		af.modifier	= 0;
		af.bitvector	= 0;

		affect_to_char(ch, &af);
		ch->mana -= mana;

		act("You feel tough!", ch, NULL, NULL, TO_CHAR);
		act("$n looks tougher.", ch, NULL, NULL, TO_ROOM);
		check_improve(ch, gsn_resistance, TRUE, 1);
	}
	else {
		ch->mana -= mana/2;

		char_puts("You flex your muscles, "
			  "but you don't feel tougher.\n", ch);
		act("$n flexes $s muscles, trying to look tough.",
		    ch, NULL, NULL, TO_ROOM);
		check_improve(ch, gsn_resistance, FALSE, 1);
	}
}

void do_trophy(CHAR_DATA *ch, const char *argument)
{
	int trophy_vnum;
	OBJ_DATA *trophy;
	AFFECT_DATA af;
	OBJ_DATA *part;
	char arg[MAX_INPUT_LENGTH];
	int level;
	int chance;
	int mana;
	
	if ((chance = get_skill(ch, gsn_trophy)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}
	
	if (is_affected(ch, gsn_trophy)) {
		char_puts("But you've already got one trophy!\n", ch);
		return;
	}
	
	mana = SKILL(gsn_trophy)->min_mana;
	if (ch->mana < mana) {
		char_puts("You feel too weak to concentrate on a trophy.\n",
			  ch);
		return;
	}
	
	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Make a trophy of what?\n", ch);
		return;
	}
	
	if ((part = get_obj_carry(ch, arg)) == NULL) {
		char_puts("You do not have that body part.\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_trophy)->beats);

	if (part->pIndexData->vnum == OBJ_VNUM_SLICED_ARM
	||  part->pIndexData->vnum == OBJ_VNUM_SLICED_LEG
	||  part->pIndexData->vnum == OBJ_VNUM_SEVERED_HEAD
	||  part->pIndexData->vnum == OBJ_VNUM_TORN_HEART
	||  part->pIndexData->vnum == OBJ_VNUM_GUTS)
		trophy_vnum = OBJ_VNUM_BATTLE_PONCHO;
	else if (part->pIndexData->vnum == OBJ_VNUM_BRAINS) {
		char_puts("Why don't you just eat those instead?\n", ch);
		return;
	}
	else {
		char_puts("You can't make a trophy out of that!\n", ch);
		return;
	}

	if (mlstr_null(part->owner)) {
		char_puts("Invalid body part.\n", ch);
		return;
	}

	if (!IS_NPC(ch) && number_percent() < chance) {
		af.where  = TO_AFFECTS;
		af.type	= gsn_trophy;
		af.level	= ch->level;
		af.duration	= ch->level/2;
		af.modifier	= 0;
		af.bitvector 	= 0;
		
		af.location	= 0;
		affect_to_char(ch,&af);
	
		if (trophy_vnum != 0) {
			level = UMIN(part->level + 5, MAX_LEVEL);
 
			trophy = create_obj_of(get_obj_index(trophy_vnum),
					       part->owner);
			trophy->level = ch->level;
			trophy->timer = ch->level * 2;
			trophy->cost  = 0;
			ch->mana -= mana;
			af.where	= TO_OBJECT;
			af.type 	= gsn_trophy;
			af.level	= level;
			af.duration	= -1;
			af.location	= APPLY_DAMROLL;
			af.modifier	= ch->level / 10 + number_range(2,4);
			af.bitvector	= 0;
			affect_to_obj(trophy, &af);

			af.location	= APPLY_HITROLL;
			af.modifier	= ch->level / 10 + number_range(2,4);
			affect_to_obj(trophy, &af);

			af.location	= APPLY_INT;
			af.modifier	= level>20?-2:-1;
			affect_to_obj(trophy, &af);

			af.location	= APPLY_STR;
			af.modifier	= level>20?2:1;
			affect_to_obj(trophy, &af);

			trophy->value[ITEM_ARMOR_AC_PIERCE] = ch->level / 3;
			trophy->value[ITEM_ARMOR_AC_BASH] = ch->level / 3;
			trophy->value[ITEM_ARMOR_AC_SLASH] = ch->level / 3;
			trophy->value[ITEM_ARMOR_AC_EXOTIC] = ch->level / 3;

			obj_to_char(trophy, ch);
			  check_improve(ch, gsn_trophy, TRUE, 1);
			
			act("You make a poncho from $p!",
			    ch, part, NULL, TO_CHAR);
			act("$n makes a poncho from $p!",
			    ch, part, NULL, TO_ROOM);
			
			extract_obj(part, 0);
			return;
		}
	}
	else {
		char_puts("You destroyed it.\n", ch);
		extract_obj(part, 0);
		ch->mana -= mana/2;
		check_improve(ch, gsn_trophy, FALSE, 1);
	}
}
	
void do_truesight(CHAR_DATA *ch, const char *argument)
{
	int chance;

	if ((chance = get_skill(ch, gsn_truesight)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	if (is_affected(ch, gsn_truesight)) {
		char_puts("Your eyes are as sharp as they can get.\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_truesight)->beats);

	if (number_percent() < chance) {
		AFFECT_DATA af;
		
		af.where    = TO_AFFECTS;
		af.type     = gsn_truesight;
		af.level    = ch->level;
		af.duration = ch->level/2 + 5;
		af.location = APPLY_NONE;
		af.modifier = 0;
			
		af.bitvector = AFF_DETECT_INVIS;
		affect_to_char(ch, &af);

/*		af.bitvector = AFF_DETECT_IMP_INVIS;
		affect_to_char(ch,&af);
*/
		
		af.bitvector = AFF_DETECT_MAGIC;
		affect_to_char(ch,&af);

		act("You look around sharply!",ch,NULL,NULL,TO_CHAR);
		act("$n looks more enlightened.",ch,NULL,NULL,TO_ROOM);
		check_improve(ch,gsn_truesight,TRUE,1);
	}
	else {
		char_puts("You look about sharply, but you don't see "
			     "anything new.\n" ,ch);
		act("$n looks around sharply but doesn't seem enlightened.",
			ch,NULL,NULL,TO_ROOM);
		check_improve(ch, gsn_truesight, FALSE, 1);
	}
}
