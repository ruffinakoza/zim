/*
 * $Id: mob_cmds.c 933 2006-11-19 22:37:00Z zsuzsu $
 */

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

/***************************************************************************
 *                                                                         *
 *  Based on MERC 2.2 MOBprograms by N'Atas-ha.                            *
 *  Written and adapted to ROM 2.4 by                                      *
 *          Markku Nylander (markku.nylander@uta.fi)                       *
 *                                                                         *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "mob_cmds.h"
#include "mob_prog.h"
#include "fight.h"

DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_say);

/*
 * helper functions
 */
void transfer_helper(CHAR_DATA * ch, ROOM_INDEX_DATA * location);

/*
 * Command table.
 */
/* *INDENT-OFF* */
const	struct	mob_cmd_type	mob_cmd_table	[] =
{
	{	"asound", 	do_mpasound	},
	{	"gecho",	do_mpgecho	},
	{	"zecho",	do_mpzecho	},
	{	"kill",		do_mpkill	},
	{	"attack",	do_mpattack	},
	{	"assist",	do_mpassist	},
	{	"junk",		do_mpjunk	},
	{	"echo",		do_mpecho	},
	{	"echoaround",	do_mpechoaround	},
	{	"echoat",	do_mpechoat	},
	{	"mload",	do_mpmload	},
	{	"oload",	do_mpoload	},
	{	"purge",	do_mppurge	},
	{	"goto",		do_mpgoto	},
	{	"at",		do_mpat		},
	{	"transfer",	do_mptransfer	},
	{	"gtransfer",	do_mpgtransfer	},
	{	"otransfer",	do_mpotransfer	},
	{	"force",	do_mpforce	},
	{	"gforce",	do_mpgforce	},
	{	"vforce",	do_mpvforce	},
	{	"damage",	do_mpdamage	},
	{	"remember",	do_mpremember	},
	{	"forget",	do_mpforget	},
	{	"delay",	do_mpdelay	},
	{	"cancel",	do_mpcancel	},
	{	"call",		do_mpcall	},
	{	"flee",		do_mpflee	},
	{	"remove",	do_mpremove	},
	{	"religion",	do_mpreligion	},
	{	"slay",		do_slay		},
	{	"affstrip",	do_mpaffstrip	},
        {	"affbestow",	do_mpaffbestow	},
	{	"award",	do_award	},
	{	str_empty,		0		}
};

/* *INDENT-ON* */
void do_mob(CHAR_DATA * ch, const char *argument)
{
	/*
	 * Security check!
	 */
	if (ch->desc && ch->level < MAX_LEVEL)
		return;
	mob_interpret(ch, argument);
}

/*
 * Mob command interpreter. Implemented separately for security and speed
 * reasons. A trivial hack of interpret()
 */
void mob_interpret(CHAR_DATA * ch, const char *argument)
{
	char command[MAX_INPUT_LENGTH];
	int cmd;

	argument = one_argument(argument, command, sizeof(command));

	/*
	 * Look for command in command table.
	 */
	for (cmd = 0; mob_cmd_table[cmd].name[0] != '\0'; cmd++) {
		if (command[0] == mob_cmd_table[cmd].name[0]
		    && !str_prefix(command, mob_cmd_table[cmd].name)) {
			(*mob_cmd_table[cmd].do_fun) (ch, argument);
			tail_chain();
			return;
		}
	}
	builder_printf("mob_interpret: invalid cmd from mob %d: '%s'",
		       IS_NPC(ch) ? ch->pIndexData->vnum : 0, command);
}

/* 
 * Displays MOBprogram triggers of a mobile
 *
 * Syntax: mpstat [name]
 */
void do_mpstat(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_STRING_LENGTH];
	MPTRIG *mptrig;
	CHAR_DATA *victim;
	int i;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Mpstat whom?\n", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		char_puts("No such creature.\n", ch);
		return;
	}

	if (!IS_NPC(victim)) {
		char_puts("That is not a mobile.\n", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		char_puts("No such creature visible.\n", ch);
		return;
	}

	char_printf(ch, "Mobile #%-6d [%s]\n",
		    victim->pIndexData->vnum, mlstr_mval(victim->short_descr));

	char_printf(ch, "Delay   %-6d [%s]\n",
		    victim->mprog_delay,
		    victim->mprog_target == NULL ?
		    "No target" : victim->mprog_target->name);

	if (!victim->pIndexData->mptrig_types) {
		char_puts("[No programs set]\n", ch);
		return;
	}

	for (i = 0, mptrig = victim->pIndexData->mptrig_list; mptrig != NULL;
	     mptrig = mptrig->next)
		char_printf(ch,
			    "[%2d] Trigger [%-8s] Program [%4d] Phrase [%s]\n",
			    ++i, flag_string(mptrig_types, mptrig->type),
			    mptrig->vnum, mptrig->phrase);
}

/*
 * Prints the argument to all active players in the game
 *
 * Syntax: mob gecho [string]
 */
void do_mpgecho(CHAR_DATA * ch, const char *argument)
{
	DESCRIPTOR_DATA *d;

	if (argument[0] == '\0') {
		builder_printf("MPgecho: invalid object from vnum %d.",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0);
		return;
	}

	for (d = descriptor_list; d; d = d->next) {
		if (d->connected == CON_PLAYING) {
			if (IS_IMMORTAL(d->character))
				char_puts("Mob echo> ", d->character);
			char_puts(argument, d->character);
			char_puts("\n", d->character);
		}
	}
}

/*
 * Prints the argument to all players in the same area as the mob
 *
 * Syntax: mob zecho [string]
 */
void do_mpzecho(CHAR_DATA * ch, const char *argument)
{
	DESCRIPTOR_DATA *d;

	if (argument[0] == '\0') {
		builder_printf("MPzecho: invalid object from vnum %d.",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0);
		return;
	}

	if (ch->in_room == NULL)
		return;

	for (d = descriptor_list; d; d = d->next) {
		if (d->connected == CON_PLAYING
		    && d->character->in_room != NULL
		    && d->character->in_room->area == ch->in_room->area) {
			if (IS_IMMORTAL(d->character))
				char_puts("Mob echo> ", d->character);
			char_puts(argument, d->character);
			char_puts("\n", d->character);
		}
	}
}

/*
 * Prints the argument to all the rooms aroud the mobile
 *
 * Syntax: mob asound [string]
 */
void do_mpasound(CHAR_DATA * ch, const char *argument)
{

	ROOM_INDEX_DATA *was_in_room;
	int door;

	if (argument[0] == '\0')
		return;

	was_in_room = ch->in_room;
	for (door = 0; door < 6; door++) {
		EXIT_DATA *pexit;

		if ((pexit = was_in_room->exit[door]) != NULL
		    && pexit->to_room.r != NULL
		    && pexit->to_room.r != was_in_room) {
			ch->in_room = pexit->to_room.r;
			act(argument, ch, NULL, NULL, TO_ROOM | ACT_NOTRIG);
		}
	}
	ch->in_room = was_in_room;
	return;

}

/*
 * Lets the mobile kill any player or mobile without murder
 *
 * Syntax: mob kill [victim]
 */
void do_mpkill(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0')
		return;

	if ((victim = get_char_room(ch, arg)) == NULL)
		return;

	if (victim == ch || IS_NPC(victim) || ch->position == POS_FIGHTING)
		return;

	if (IS_SET(victim->state_flags, STATE_GHOST))
		return;

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
		builder_printf
		    ("MPkill: charmed mob attacking master from vnum %d: '%s'",
		     IS_NPC(ch) ? ch->pIndexData->vnum : 0, victim->name);
		return;
	}

	multi_hit(ch, victim, TYPE_UNDEFINED);
}

/*
 * Lets the mobile add a spell effect to a player
 * Written by: Tainar
 *
 * Syntax: mob affbestow [victim] [spell]
 */
void do_mpaffbestow(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	AFFECT_DATA af;
	int sn;


	log_printf("mob affbestow: %s", argument);

	argument = one_argument(argument, arg, sizeof(arg));
	argument = one_argument(argument, arg2, sizeof(arg2));
	sn = 0;
	if (arg[0] == '\0')
		return;

	if (!strcmp(arg, "self"))
		victim = ch;
	else {
		if ((victim = get_char_room(ch, arg)) == NULL) {
			return;
		}
	}

	sn = sn_lookup(arg2);
	if (sn == -1)
		return;

	if (!strcmp(arg2, "armor")) {
		af.where = TO_AFFECTS;
		af.type = sn;
		af.level = ch->level;
		af.duration = 7 + ch->level / 6;
		af.modifier = -1 * UMAX(20, 10 + ch->level / 4);
		af.location = APPLY_AC;
		af.bitvector = 0;
		affect_to_char(victim, &af);
		return;
	}

	if (!strcmp(arg2, "curse")) {
		af.where = TO_AFFECTS;
		af.type = sn;
		af.level = ch->level;
		af.duration = 7 + ch->level / 6;
		af.location = APPLY_HITROLL;
		af.modifier = -1 * (ch->level / 8);
		af.bitvector = AFF_CURSE;
		affect_to_char(victim, &af);

		af.location = APPLY_SAVING_SPELL;
		af.modifier = ch->level / 8;
		affect_to_char(victim, &af);

		char_puts("You feel unclean.\n", victim);
		if (ch != victim)
			act("$N looks very uncomfortable.", ch, NULL, victim,
			    TO_CHAR);
	}

	if (!strcmp(arg2, "frenzy")) {
		af.where = TO_AFFECTS;
		af.type = sn;
		af.level = ch->level;
		af.duration = ch->level / 3;
		af.modifier = ch->level / 6;
		af.bitvector = 0;

		af.location = APPLY_HITROLL;
		affect_to_char(victim, &af);

		af.location = APPLY_DAMROLL;
		affect_to_char(victim, &af);

		af.modifier = 10 * (ch->level / 12);
		af.location = APPLY_AC;
		affect_to_char(victim, &af);

		char_puts("You are filled with holy wrath!\n", victim);
		act("$n gets a wild look in $s eyes!", victim, NULL, NULL,
		    TO_ROOM);
	}

	if (!strcmp(arg2, "detect fade")) {
		af.where = TO_AFFECTS;
		af.type = sn;
		af.level = ch->level;
		af.duration = 5 + ch->level / 3;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = AFF_DETECT_FADE;

		affect_to_char(victim, &af);
		char_puts("You vision takes on a {ggreenish{x hue.\n", victim);
		act("$n's eyes flash {ggreen{x for a moment.",
		    ch, NULL, NULL, TO_ROOM);
	}
	return;
}

/*
 * Lets the mobile strip the spell affect off a player
 * Written by: Tainar
 *
 * Syntax: mob affstrip [victim] [spell]
 */
void do_mpaffstrip(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	AFFECT_DATA *paf;
	int sn;

	argument = one_argument(argument, arg, sizeof(arg));
	argument = one_argument(argument, arg2, sizeof(arg2));
	sn = 0;

	if (arg[0] == '\0')
		return;

	if (!strcmp(arg, "self"))
		victim = ch;
	else {
		if ((victim = get_char_room(ch, arg)) == NULL) {
			return;
		}
	}

	if (strcmp(arg2, "") && strcmp(arg2, "all")) {
		sn = sn_lookup(arg2);
		if (sn == -1)
			return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
		builder_printf
		    ("MPaffstrip: charmed mob attacking master from vnum %d: '%s'",
		     IS_NPC(ch) ? ch->pIndexData->vnum : 0, victim->name);
		return;
	}

	/* strip the affects */
	if (sn == 0) {
		for (paf = victim->affected; paf; paf = paf->next) {
			if (paf && paf->duration > 0) {
				affect_strip(victim, paf->type);
			}
		}
	}
	/* strip a specific affect */
	else if (sn > 0) {
		char buf[MAX_STRING_LENGTH];

		affect_strip(victim, sn);

		sprintf(buf,
			"A wave of intense magic washes the effect of %s from you.\n\r",
			SKILL(sn)->name);
		send_to_char(buf, victim);
	}

	return;
}

void do_mpattack(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0')
		return;

	if ((victim = get_char_room(ch, arg)) == NULL)
		return;

	if (victim == ch)
		return;

	multi_hit(ch, victim, TYPE_UNDEFINED);
}

/*
 * Lets the mobile assist another mob or player
 *
 * Syntax: mob assist [character]
 */
void do_mpassist(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0')
		return;

	if ((victim = get_char_room(ch, arg)) == NULL)
		return;

	if (victim == ch || ch->fighting != NULL || victim->fighting == NULL ||
	    victim->fighting->in_room != ch->in_room)
		return;

	multi_hit(ch, victim->fighting, TYPE_UNDEFINED);
}


/*
 * Lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy 
 * items using all.xxxxx or just plain all of them 
 *
 * Syntax: mob junk [item]
 */

void do_mpjunk(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0')
		return;

	if (str_cmp(arg, "all") && str_prefix("all.", arg)) {
		if ((obj = get_obj_wear(ch, arg)) != NULL) {
			unequip_char(ch, obj);
			extract_obj(obj, 0);
			return;
		}
		if ((obj = get_obj_carry(ch, arg)) == NULL)
			return;
		extract_obj(obj, 0);
	} else
		for (obj = ch->carrying; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			if (arg[3] == '\0' || is_name(&arg[4], obj->name)) {
				if (obj->wear_loc != WEAR_NONE)
					unequip_char(ch, obj);
				extract_obj(obj, 0);
			}
		}

	return;

}

/*
 * Prints the message to everyone in the room other than the mob and victim
 *
 * Syntax: mob echoaround [victim] [string]
 */

void do_mpechoaround(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0')
		return;

	if ((victim = get_char_room(ch, arg)) == NULL)
		return;

	act(argument, ch, NULL, victim, TO_NOTVICT);
}

/*
 * Prints the message to only the victim
 *
 * Syntax: mob echoat [victim] [string]
 */
void do_mpechoat(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0' || argument[0] == '\0')
		return;

	if ((victim = get_char_room(ch, arg)) == NULL)
		return;

	act(argument, ch, NULL, victim, TO_VICT);
}

/*
 * Prints the message to the room at large
 *
 * Syntax: mpecho [string]
 */
void do_mpecho(CHAR_DATA * ch, const char *argument)
{
	if (argument[0] == '\0')
		return;
	act(argument, ch, NULL, NULL, TO_ROOM);
}

/*
 * Lets the mobile load another mobile.
 *
 * Syntax: mob mload [vnum]
 */
void do_mpmload(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	MOB_INDEX_DATA *pMobIndex;
	CHAR_DATA *victim;
	int vnum;

	one_argument(argument, arg, sizeof(arg));

	if (ch->in_room == NULL || arg[0] == '\0' || !is_number(arg))
		return;

	vnum = atoi(arg);
	if ((pMobIndex = get_mob_index(vnum)) == NULL) {
		builder_printf("MPmload: bad mob index (%d) from mob %d",
			       vnum, IS_NPC(ch) ? ch->pIndexData->vnum : 0);
		return;
	}
	victim = create_mob(pMobIndex);
	char_to_room(victim, ch->in_room);
}

/*
 * Lets the mobile load an object
 *
 * Syntax: mob oload [vnum] {R}
 */
void do_mpoload(CHAR_DATA * ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	OBJ_INDEX_DATA *pObjIndex;
	OBJ_DATA *obj;
	bool fToroom = FALSE, fWear = FALSE;

	argument = one_argument(argument, arg1, sizeof(arg1));
	one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0' || !is_number(arg1)) {
		builder_printf("MPoload: Bad syntax from vnum %d",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0);
		return;
	}

	/*
	 * omitted - load to mobile's inventory
	 * 'R'     - load to room
	 * 'W'     - load to mobile and force wear
	 */
	if (arg2[0] == 'R' || arg2[0] == 'r')
		fToroom = TRUE;
	else if (arg2[0] == 'W' || arg2[0] == 'w')
		fWear = TRUE;

	if ((pObjIndex = get_obj_index(atoi(arg1))) == NULL) {
		builder_printf("MPoload: Bad argument from vnum %d: '%s'",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0, arg1);
		return;
	}

	if (pObjIndex->limit >= 0 && pObjIndex->count >= pObjIndex->limit) {
		builder_printf("Mpoload - mob[%d] limit exceeded [%d]",
			       ch->pIndexData->vnum, pObjIndex->vnum);
		return;
	}

	obj = create_obj(pObjIndex, 0);
	if ((fWear || !fToroom) && CAN_WEAR(obj, ITEM_TAKE)) {
		obj_to_char(obj, ch);
		if (fWear)
			wear_obj(ch, obj, TRUE);
	} else
		obj_to_room(obj, ch->in_room);
}

/*
 * Lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room. The mobile cannot
 * purge itself for safety reasons.
 *
 * syntax mob purge {target}
 */
void do_mppurge(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *obj;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		/* 'purge' */
		CHAR_DATA *vnext;
		OBJ_DATA *obj_next;

		for (victim = ch->in_room->people; victim != NULL;
		     victim = vnext) {
			vnext = victim->next_in_room;
			if (IS_NPC(victim) && victim != ch
			    && !IS_SET(victim->pIndexData->act, ACT_NOPURGE))
				extract_char(victim, 0);
		}

		for (obj = ch->in_room->contents; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			if (!IS_SET(obj->extra_flags, ITEM_NOPURGE))
				extract_obj(obj, 0);
		}

		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		if ((obj = get_obj_here(ch, arg))) {
			extract_obj(obj, 0);
		} else {
			builder_printf("Mppurge: invalid cmd from mob %d: '%s'",
				       IS_NPC(ch) ? ch->pIndexData->vnum : 0,
				       arg);
		}
		return;
	}

	if (!IS_NPC(victim)) {
		builder_printf("Mppurge: Purging a PC from vnum %d: '%s'",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0, arg);
		return;
	}
	extract_char(victim, 0);
}

/*
 * Lets the mobile goto any location it wishes that is not private.
 *
 * Syntax: mob goto [location]
 */
void do_mpgoto(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *location;

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		builder_printf("Mpgoto: No argumnet from vnum %d.",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0);
		return;
	}

	if ((location = find_location(ch, arg)) == NULL) {
		builder_printf("Mpgoto: No such location from vnum %d.",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0);
		return;
	}

	if (ch->fighting != NULL)
		stop_fighting(ch, TRUE);

	char_from_room(ch);
	char_to_room(ch, location);
}

/* 
 * Lets the mobile do a command at another location.
 *
 * Syntax: mob at [location] [commands]
 */
void do_mpat(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *location;
	ROOM_INDEX_DATA *original;
	OBJ_DATA *on;

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0' || argument[0] == '\0') {
		builder_printf("MPat: Bad argument from vnum %d.",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0);
		return;
	}

	if ((location = find_location(ch, arg)) == NULL) {
		builder_printf("MPat: No such location from vnum %d: '%s'",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0, arg);
		return;
	}

	original = ch->in_room;
	on = ch->on;
	char_from_room(ch);
	char_to_room(ch, location);
	if (JUST_KILLED(ch))
		return;

	interpret(ch, argument);

	if (ch->extracted)
		return;

	char_from_room(ch);
	char_to_room(ch, original);
	if (!JUST_KILLED(ch))
		ch->on = on;
}

/*
 * Lets the mobile transfer people.  The 'all' argument transfers
 *  everyone in the current room to the specified location
 *
 * Syntax: mob transfer [target|'all'] [location]
 */
void do_mptransfer(CHAR_DATA * ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *location;
	CHAR_DATA *victim;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0') {
		builder_printf("MPtransfer: Bad syntax from vnum %d.",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0);
		return;
	}

	if (!str_cmp(arg1, "all")) {
		CHAR_DATA *victim_next;

		for (victim = ch->in_room->people; victim != NULL;
		     victim = victim_next) {
			victim_next = victim->next_in_room;
			if (!IS_NPC(victim))
				doprintf(do_mptransfer, ch, "%s %s",
					 victim->name, arg2);
		}
		return;
	}

	/*
	 * Thanks to Grodyn for the optional location parameter.
	 */
	if (arg2[0] == '\0') {
		location = ch->in_room;
	} else {
		if ((location = find_location(ch, arg2)) == NULL) {
			builder_printf
			    ("MPtransfer: No such location from vnum %d: '%s'",
			     IS_NPC(ch) ? ch->pIndexData->vnum : 0, arg2);
			return;
		}

		if (room_is_private(location))
			return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL)
		return;
	if (victim->in_room == NULL)
		return;

	transfer_helper(victim, location);
}

void transfer_helper(CHAR_DATA * ch, ROOM_INDEX_DATA * location)
{
	if (ch->fighting != NULL)
		stop_fighting(ch, TRUE);
	char_from_room(ch);
	char_to_room(ch, location);
	if (!JUST_KILLED(ch))
		do_look(ch, "auto");
}

/*
 * Lets the mobile transfer all chars in same group as the victim.
 *
 * Syntax: mob gtransfer [victim] [location]
 */
void do_mpgtransfer(CHAR_DATA * ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *who, *victim, *victim_next;
	ROOM_INDEX_DATA *location;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));


	if (arg1[0] == '\0') {
		builder_printf("MPtransfer: Bad syntax from vnum %d.",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0);
		return;
	}

	if ((who = get_char_room(ch, arg1)) == NULL)
		return;

	if (arg2 == '\0')
		location = ch->in_room;
	else if ((location = find_location(ch, arg2)) == NULL) {
		builder_printf
		    ("MPtransfer: No such location from vnum %d: '%s'",
		     IS_NPC(ch) ? ch->pIndexData->vnum : 0, arg2);
		return;
	}

	for (victim = ch->in_room->people; victim; victim = victim_next) {
		victim_next = victim->next_in_room;
		if (is_same_group(who, victim)
		    && victim->position >= POS_STANDING) {
			transfer_helper(victim, location);
		}
	}
}

/*
 * Lets the mobile force someone to do something. Must be mortal level
 * and the all argument only affects those in the room with the mobile.
 *
 * Syntax: mob force [victim] [commands]
 */
void do_mpforce(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0' || argument[0] == '\0') {
		builder_printf("MPforce: Bad syntax from num %d.",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0);
		return;
	}

	if (!str_cmp(arg, "all")) {
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;

		for (vch = char_list; vch; vch = vch_next) {
			vch_next = vch->next;

			if (vch->in_room == ch->in_room
			    && IS_TRUSTED(ch, vch->level)
			    && can_see(ch, vch))
				interpret(vch, argument);
		}
	} else {
		CHAR_DATA *victim;

		if ((victim = get_char_room(ch, arg)) == NULL)
			return;

		if (victim == ch)
			return;

		interpret(victim, argument);
	}

	return;
}

/*
 * Lets the mobile force a group something. Must be mortal level.
 *
 * Syntax: mob gforce [victim] [commands]
 */
void do_mpgforce(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim, *vch, *vch_next;

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0' || argument[0] == '\0') {
		builder_printf("MPgforce: Bad syntax from num %d.",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL)
		return;

	if (victim == ch)
		return;

	for (vch = victim->in_room->people; vch != NULL; vch = vch_next) {
		vch_next = vch->next_in_room;

		if (is_same_group(victim, vch)) {
			interpret(vch, argument);
		}
	}
	return;
}

/*
 * Forces all mobiles of certain vnum to do something (except ch)
 *
 * Syntax: mob vforce [vnum] [commands]
 */
void do_mpvforce(CHAR_DATA * ch, const char *argument)
{
	CHAR_DATA *victim, *victim_next;
	char arg[MAX_INPUT_LENGTH];
	int vnum;

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0' || argument[0] == '\0') {
		builder_printf("MPvforce: Bad syntax from num %d.",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0);
		return;
	}

	if (!is_number(arg)) {
		builder_printf("MPvforce: Non-number argument from num %d:'%s'",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0, arg);
		return;
	}

	vnum = atoi(arg);

	for (victim = npc_list; victim; victim = victim_next) {
		victim_next = victim->next;
		if (victim->pIndexData->vnum == vnum
		    && ch != victim && victim->fighting == NULL)
			interpret(victim, argument);
	}
	return;
}

/*
 * Lets mob cause unconditional damage to someone. Nasty, use with caution.
 * Also, this is silent, you must show your own damage message...
 *
 * Syntax: mob damage [victim] [min] [max] {kill}
 */
void do_mpdamage(CHAR_DATA * ch, const char *argument)
{
	CHAR_DATA *victim = NULL, *victim_next;
	char target[MAX_INPUT_LENGTH],
	    min[MAX_INPUT_LENGTH], max[MAX_INPUT_LENGTH];
	int low, high;
	bool fAll = FALSE, fKill = FALSE;

	argument = one_argument(argument, target, sizeof(target));
	argument = one_argument(argument, min, sizeof(min));
	argument = one_argument(argument, max, sizeof(max));

	if (target[0] == '\0') {
		builder_printf("MPdamage: Bad syntax from vnum %d.",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0);
		return;
	}
	if (!str_cmp(target, "all"))
		fAll = TRUE;
	else if ((victim = get_char_room(ch, target)) == NULL)
		return;

	if (is_number(min))
		low = atoi(min);
	else {
		builder_printf("MPdamage: Bad damage min from vnum %d:'%s'",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0, min);
		return;
	}
	if (is_number(max))
		high = atoi(max);
	else {
		builder_printf("MPdamage: Bad damage max from vnum %d:'%s'",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0, max);
		return;
	}
	one_argument(argument, target, sizeof(target));

	/*
	 * If kill parameter is omitted, this command is "safe" and will not
	 * kill the victim.
	 */

	if (target[0] != '\0')
		fKill = TRUE;
	if (fAll) {
		for (victim = ch->in_room->people; victim; victim = victim_next) {
			victim_next = victim->next_in_room;
			if (victim != ch)
				damage(victim, victim,
				       fKill ?
				       number_range(low,
						    high) : UMIN(victim->hit,
								 number_range
								 (low, high)),
				       TYPE_UNDEFINED, DAM_NONE, DAMF_NOREDUCTION);
		}
	} else
		damage(victim, victim,
		       fKill ?
		       number_range(low, high) : UMIN(victim->hit,
						      number_range(low, high)),
		       TYPE_UNDEFINED, DAM_NONE, DAMF_NOREDUCTION);
	return;
}

/*
 * Lets the mobile to remember a target. The target can be referred to
 * with $q and $Q codes in MOBprograms. See also "mob forget".
 *
 * Syntax: mob remember [victim]
 */
void do_mpremember(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	one_argument(argument, arg, sizeof(arg));
	if (arg[0] != '\0')
		ch->mprog_target = get_char_world(ch, arg);
	else
		builder_printf("MPremember: missing argument from vnum %d.",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0);
}

/*
 * Reverse of "mob remember".
 *
 * Syntax: mob forget
 */
void do_mpforget(CHAR_DATA * ch, const char *argument)
{
	ch->mprog_target = NULL;
}

/*
 * Sets a delay for MOBprogram execution. When the delay time expires,
 * the mobile is checked for a MObprogram with DELAY trigger, and if
 * one is found, it is executed. Delay is counted in PULSE_MOBILE
 *
 * Syntax: mob delay [pulses]
 */
void do_mpdelay(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg, sizeof(arg));
	if (!is_number(arg)) {
		builder_printf("MPdelay: invalid arg from vnum %d: '%s'",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0, arg);
		return;
	}
	ch->mprog_delay = atoi(arg);
}

/*
 * Reverse of "mob delay", deactivates the timer.
 *
 * Syntax: mob cancel
 */
void do_mpcancel(CHAR_DATA * ch, const char *argument)
{
	ch->mprog_delay = -1;
}

/*
 * Lets the mobile to call another MOBprogram withing a MOBprogram.
 * This is a crude way to implement subroutines/functions. Beware of
 * nested loops and unwanted triggerings... Stack usage might be a problem.
 * Characters and objects referred to must be in the same room with the
 * mobile.
 *
 * Syntax: mob call [vnum] [victim|'null'] [object1|'null'] [object2|'null']
 *
 */
void do_mpcall(CHAR_DATA * ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *vch;
	OBJ_DATA *obj1, *obj2;
	int vnum;

	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		builder_printf("MPcall: invalid arg from vnum %d: '%s'",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0, arg);
		return;
	}

	vnum = atoi(arg);

	vch = NULL;
	obj1 = obj2 = NULL;
	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] != '\0')
		vch = get_char_room(ch, arg);
	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] != '\0')
		obj1 = get_obj_here(ch, arg);
	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] != '\0')
		obj2 = get_obj_here(ch, arg);
	program_flow(vnum, ch, vch, (void *) obj1, (void *) obj2);
}

/*
 * Forces the mobile to flee.
 *
 * Syntax: mob flee
 *
 */
void do_mpflee(CHAR_DATA * ch, const char *argument)
{
	ROOM_INDEX_DATA *was_in;
	EXIT_DATA *pexit;
	int door, attempt;

	if (ch->fighting != NULL)
		return;

	if ((was_in = ch->in_room) == NULL)
		return;

	for (attempt = 0; attempt < 6; attempt++) {
		door = number_door();
		if ((pexit = was_in->exit[door]) == 0
		    || pexit->to_room.r == NULL
		    || IS_SET(pexit->exit_info, EX_CLOSED)
		    || (IS_NPC(ch)
			&& IS_SET(pexit->to_room.r->room_flags, ROOM_NOMOB)))
			continue;

		move_char(ch, door, FALSE);
		if (ch->in_room != was_in)
			return;
	}
}

/*
 * Lets the mobile to transfer an object. The object must be in the same
 * room with the mobile.
 *
 * Syntax: mob otransfer [item name] [location]
 */
void do_mpotransfer(CHAR_DATA * ch, const char *argument)
{
	OBJ_DATA *obj;
	ROOM_INDEX_DATA *location;
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		builder_printf("MPotransfer: missing arg from vnum %d: '%s'",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0, arg);
		return;
	}
	one_argument(argument, buf, sizeof(buf));
	if ((location = find_location(ch, buf)) == NULL) {
		builder_printf
		    ("MPotransfer: no such location from vnum %d: '%s'",
		     IS_NPC(ch) ? ch->pIndexData->vnum : 0, buf);
		return;
	}
	if ((obj = get_obj_here(ch, arg)) == NULL)
		return;
	if (obj->carried_by == NULL)
		obj_from_room(obj);
	else {
		if (obj->wear_loc != WEAR_NONE)
			unequip_char(ch, obj);
		obj_from_char(obj);
	}
	obj_to_room(obj, location);
}

/*
 * Lets the mobile to strip an object or all objects from the victim.
 * Useful for removing e.g. quest objects from a character.
 *
 * Syntax: mob remove [victim] [object vnum|'all']
 */
void do_mpremove(CHAR_DATA * ch, const char *argument)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj, *obj_next;
	int vnum = 0;
	bool fAll = FALSE;
	char arg[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg, sizeof(arg));
	if ((victim = get_char_room(ch, arg)) == NULL)
		return;

	one_argument(argument, arg, sizeof(arg));
	if (!str_cmp(arg, "all"))
		fAll = TRUE;
	else if (!is_number(arg)) {
		builder_printf("MPremove: invalid object from vnum %d: '%s'",
			       IS_NPC(ch) ? ch->pIndexData->vnum : 0, arg);
		return;
	} else
		vnum = atoi(arg);

	for (obj = victim->carrying; obj; obj = obj_next) {
		obj_next = obj->next_content;
		if (fAll || obj->pIndexData->vnum == vnum) {
			unequip_char(ch, obj);
			obj_from_char(obj);
			extract_obj(obj, 0);
		}
	}
}


int lookup_religion_leader(const char *name)
{
	int value;

	for (value = 0; value < MAX_RELIGION; value++) {
		if (LOWER(name[0]) == LOWER(religion_table[value].leader[0])
		    && !str_prefix(name, religion_table[value].leader))
			return value;
	}

	return 0;
}

void do_mpreligion(CHAR_DATA * ch, const char *argument)
{
	CHAR_DATA *victim;
	char name[MAX_STRING_LENGTH];
	int chosen = 0;
	bool correct = TRUE;

	argument = one_argument(argument, name, sizeof(name));
	if ((victim = get_char_room(ch, name)) == NULL)
		return;

	if ((chosen = lookup_religion_leader(argument)) == 0)
		return;

	if (victim->religion > 0 && victim->religion < MAX_RELIGION) {
		doprintf(do_say, ch, "You are already in the way of %s",
			 religion_table[victim->religion].leader);
		return;
	}

	switch (chosen) {
	case RELIGION_ATUM_RA:
		if (!IS_GOOD(victim) || victim->ethos != ETHOS_LAWFUL)
			correct = FALSE;
		break;
	case RELIGION_ZEUS:
		if (!IS_GOOD(victim) || victim->ethos != ETHOS_NEUTRAL)
			correct = FALSE;
		break;
	case RELIGION_SIEBELE:
		if (!IS_NEUTRAL(victim) || victim->ethos != ETHOS_NEUTRAL)
			correct = FALSE;
		break;
	case RELIGION_AHURAMAZDA:
		if (!IS_GOOD(victim) || victim->ethos != ETHOS_CHAOTIC)
			correct = FALSE;
		break;
	case RELIGION_EHRUMEN:
		if (!IS_EVIL(victim) || victim->ethos != ETHOS_CHAOTIC)
			correct = FALSE;
		break;
	case RELIGION_DEIMOS:
		if (!IS_EVIL(victim) || victim->ethos != ETHOS_LAWFUL)
			correct = FALSE;
		break;
	case RELIGION_PHOBOS:
		if (!IS_EVIL(victim) || victim->ethos != ETHOS_NEUTRAL)
			correct = FALSE;
		break;
	case RELIGION_ODIN:
		if (!IS_NEUTRAL(victim) || victim->ethos != ETHOS_LAWFUL)
			correct = FALSE;
		break;
	case RELIGION_TESHUB:
		if (!IS_NEUTRAL(victim) || victim->ethos != ETHOS_CHAOTIC)
			correct = FALSE;
		break;
	}

	if (!correct) {
		if (victim->ethos == ETHOS_NONE && !CHAR_CREATE_ETHOS) 
			do_say(ch,
			       "That god is not taking on believers right now, choose another.");
		else
			do_say(ch,
				"That religion doesn't match your ethos and alignment.");
		return;
	}

	victim->religion = chosen;
	doprintf(do_say, ch,
		 "From now on and forever, you are in the way of %s",
		 religion_table[victim->religion].leader);
}
