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
 * $Id: skills2.c 1022 2008-05-12 22:40:43Z zsuzsu $
 */

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

void steal_from_container (CHAR_DATA *ch, CHAR_DATA *victim, 
		const char *arg1, const char *arg2);

bool steal_success (CHAR_DATA *ch, CHAR_DATA *victim,
		OBJ_DATA *container, OBJ_DATA *obj);

bool steal_failure (CHAR_DATA *ch, CHAR_DATA *victim,
		OBJ_DATA *container, OBJ_DATA *obj);

bool picklock_obj_worn(CHAR_DATA *ch, const char *arg1, const char *arg2);
/*
 * new version of rockseer 'spell', which is now a skill
 * so anti-magic people can use it.
 * by Zsuzsu
 */
void do_meld_into_stone(CHAR_DATA *ch, const char *argument) 
{
	char arg1 [MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	AFFECT_DATA af;
	int sn;

	if ((sn = sn_lookup("meld into stone")) < 0
	||  (get_skill(ch, sn)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	argument = one_argument(argument, arg1, sizeof(arg1));

	if (IS_NPC(ch))
		return;


	if (arg1[0] != '\0'
	&& (victim = get_char_room(ch, arg1)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}
	else
		victim = ch;

	WAIT_STATE(ch, SKILL(sn)->beats);
	 
	if (is_affected(victim, sn))
	{
	  if (victim == ch)
	   	char_puts("Your skin is already covered with stone.\n",
		     ch); 
	  else
	   	act("$N's skin is already covered with stone.",ch,NULL,
	    victim,TO_CHAR);
	  return;
	}

	af.where	= TO_AFFECTS;
	af.type      = sn;
	af.level     = ch->level;
	af.duration  = ch->level / 10;
	af.location  = APPLY_AC;
	af.modifier  = -10 - ch->level;
	af.bitvector = 0;
	affect_to_char(victim, &af);
	act("$n's skin melds into {Dst{wo{Dne{x.",victim,NULL,NULL,TO_ROOM);
	char_puts("Your skin melds into {Dst{wo{Dne{x.\n", victim);
}

/*
 * limited "see-all" for Sylvan inside the forest.
 * by Zsuzsu
 */
void do_bush_sense (CHAR_DATA *ch, const char *argument)
{
	AFFECT_DATA	af;
	int		chance;
	int		sn;

	if ((sn = sn_lookup("bush sense")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	if (is_affected(ch, sn)) {
		char_puts("You are already in tune with the Forest. \n",ch);
		return;
	}

	WAIT_STATE(ch, SKILL(sn)->beats);

	if (number_percent() > chance) {
		char_puts("You try to sense the subtities of the Forest "
			     "but you are too distracted.\n", ch);
		check_improve(ch, sn, FALSE, 2);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = ch->level;
	af.duration  = ch->level;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = 0;
	affect_to_char(ch, &af);
	char_puts("You feel in tune with the {gForest{x.\n", ch);
	check_improve(ch, sn, TRUE, 1);
}

/*
 * Delve: Cleric ability allows the status of a target to be seen.
 * by Zsuzsu
 */
void do_delve (CHAR_DATA *ch, const char *argument)
{
	char arg1 [MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int sn = gsn_delve;
	int chance;
	int percent;

	argument = one_argument(argument, arg1, sizeof(arg1));

	if  ((chance = get_skill(ch, sn)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	if (arg1[0] == '\0') {
		char_puts("Who is your patient?\n", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg1)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(sn)->beats);

	if (number_percent() > chance) {
		char_puts("You are too distracted to diagnose them correctly.\n"
			,ch);
		check_improve(ch, sn, FALSE, 2);
		return;
	}

	act("You lay your hands on $N and $E is briefly encompassed by a blue aura.", ch, NULL, victim, TO_CHAR);
	act("$n lays $s hands on you and you are briefly encompassed by a blue aura.", ch, NULL, victim, TO_VICT);
	act("$n lays $s hands on $N and $E is briefly encompassed by a blue aura.", ch, NULL, victim, TO_NOTVICT);

        if (victim->max_hit > 0)
                percent = (100 * victim->hit) / victim->max_hit;
        else
                percent = -1;
 
        if (percent >= 100)
           char_puts("{CThey are in perfect health{x.\n", ch);
        else if (percent >= 90)
           char_puts("{bThey have a few scratches{x.\n", ch);
        else if (percent >= 75)
           char_puts("{BThey have some small but disgusting cuts{x.\n", ch);
        else if (percent >= 50)
           char_puts("{GThey are covered with bleeding wounds{x.\n", ch);
        else if (percent >= 30)
           char_puts("{YThey are gushing blood{x.\n", ch);
        else if (percent >= 15)
           char_puts("{MThey are writhing in agony{x.\n", ch);
        else if (percent >= 0)
           char_puts("{RThey are convulsing on the ground{x.\n", ch);
        else
           char_puts("{RThey are nearly dead{x.\n", ch);

	if (IS_AFFECTED (victim, AFF_BLIND))
	  char_puts("They are blinded.\n", ch);
	
	if (IS_AFFECTED (victim, AFF_POISON))
	  char_puts("They are poisoned.\n", ch);
	
	if (IS_AFFECTED (victim, AFF_CURSE))
	  char_puts("They have been cursed.\n", ch);

	if (is_affected (victim, sn_lookup ("intoxication")))
	  char_puts("They are intoxicated.\n", ch);

	if (is_affected (victim, sn_lookup ("plague")))
	  char_puts("They have a nasty disease.\n", ch);

	if (is_affected (victim, sn_lookup ("amnesia")))
	  char_puts("They have forgotten many things.\n", ch);

	if (is_affected (victim, sn_lookup ("corruption")))
	  char_puts("They are slowly rotting away.\n", ch);

	if (is_affected (victim, sn_lookup ("sleep")))
	  char_puts("They have been magically put to sleep.\n", ch);

	if (is_affected (victim, sn_lookup ("weaken")))
	  char_puts("They have been magically weakened.\n", ch);

	if (is_affected (victim, sn_lookup ("witch curse")))
	  char_puts("They have been cursed by a Witch.\n", ch);

	if (is_affected (victim, sn_lookup ("garble")))
	  char_puts("Their speech has been garbled.\n", ch);

	if (is_affected (victim, sn_lookup ("confuse")))
	  char_puts("They are confused.\n", ch);

	check_improve(ch, sn, TRUE, 1);
}

/*
 * Ranger Traps, damaging, net trap, blind trap
 * by: Alamor
 */
void do_trap(CHAR_DATA *ch, const void *argument)
{
	char arg[MAX_STRING_LENGTH];
	int chance, sn;
	argument = one_argument(argument, arg, sizeof(arg));

	if (!ch->in_room)
		return;

	if (!(ch->in_room->sector_type == SECT_FOREST
	|| ch->in_room->sector_type == SECT_MOUNTAIN
	|| ch->in_room->sector_type == SECT_HILLS
	|| ch->in_room->sector_type == SECT_DESERT
	|| ch->in_room->sector_type == SECT_FIELD)) {
		char_puts("There is not enough cover to lay a trap here.\n",ch);
		return;
	}

	if (is_affected_room(ch->in_room, gsn_rnet_trap)
	|| is_affected_room(ch->in_room, gsn_rblind_trap)
	|| is_affected_room(ch->in_room, gsn_ranger_trap)) {
		char_puts("This room has already been trapped.\n",ch);
		return;
	}

	if (is_affected(ch, gsn_ranger_trap) && !IS_IMMORTAL(ch)) {
		char_puts("You do not have enough materials to create another trap.\n",ch);
		return;
	}

	if (!str_cmp(arg, "net")) {
		if ((chance = get_skill(ch, gsn_rnet_trap)) == 0) {
			char_puts("You cannot lay that type of trap.\n",ch);
			return;
		}
		sn = gsn_rnet_trap;
	}
	else if (!str_cmp(arg, "blind")) {
		if ((chance = get_skill(ch, gsn_rblind_trap)) == 0) {
			char_puts("You cannot lay that type of trap.\n",ch);
			return;
		}
		sn = gsn_rblind_trap;
	}
	else if (arg[0] == '\0') { /*no argument means basic damage trap*/
		if ((chance = get_skill(ch, gsn_ranger_trap)) == 0) {
			char_puts("Huh?\n",ch);
			return;
		}
		sn = gsn_ranger_trap;
	}
	else {
		char_puts("Leave the experimentation to the guildmasters.\n",ch);
		return;
	}

	WAIT_STATE(ch, SKILL(sn)->beats);
	if (number_percent() < chance * 8 / 10)
	{
		AFFECT_DATA af;
		AFFECT_DATA af2;
		check_improve(ch, gsn_rnet_trap,TRUE,1);
		af.player     = ch;  /*tells affect who set it for
				       output purposes when trap is sprung*/
		af.where      = TO_ROOM_AFFECTS;
		af.type       = sn;
		af.level      = ch->level;
		af.duration   = ch->level / 9;
		af.location   = APPLY_NONE;
		af.modifier   = 0;
		af.bitvector  = RAFF_RANGER_TRAP;
		affect_to_room(ch->in_room, &af);

		char_printf(ch, "You carefully set a %s.",
			skill_lookup(sn)->name);
		act("$n carefully sets a trap on the ground.",
			ch,NULL,NULL,TO_ROOM);

		check_improve(ch,sn, TRUE, 1);
 
		af2.where     = TO_AFFECTS;
		af2.type      = gsn_ranger_trap; /*trap cooldown*/
		af2.level     = ch->level;
		af2.duration  = (10 - ch->level / 20);
		af2.modifier  = 0;
		af2.location  = APPLY_NONE;
		af2.bitvector = 0;
		affect_to_char(ch, &af2);

		return;
	}
	else check_improve(ch,sn,FALSE,1);

	return;
}

/*
 * remove a trap from a room and salvage the parts
 * by Alamor
 */
void do_dismantle(CHAR_DATA *ch, const char *argument)
{
	int chance;
	AFFECT_DATA *paf;
	if ((chance = get_skill(ch, gsn_dismantle)) == 0) {
		char_puts("Huh?\n",ch);
		return;
	}
	if (IS_ROOM_AFFECTED(ch->in_room, RAFF_RANGER_TRAP)){
		if ((paf = affect_find(ch->in_room->affected, gsn_rnet_trap)) != NULL) {}
		else if ((paf = affect_find(ch->in_room->affected, gsn_ranger_trap)) != NULL) {}
		else if ((paf = affect_find(ch->in_room->affected, gsn_rblind_trap)) != NULL) {}
		else { 
			bug("bad paf for ranger trap",0);
			return;
		}

		if (number_percent() < chance + LEVEL(ch) - paf->level) {
			char_printf(ch, "You carefully dismantle the %s"
					" on the ground.",
				skill_name(paf->type));
			act("$n cautiously dismantles a trap on the ground.",
				ch,NULL,NULL,TO_ROOM);
			affect_remove_room(ch->in_room, paf);
			check_improve(ch,gsn_dismantle,TRUE,1);		
		}
		else {
			char_puts("You failed the dismantle.\n",ch);
			check_improve(ch, gsn_dismantle, FALSE,2);
		}
		return;
	}
	else {
		char_puts("There is no trap here to dismantle.\n",ch);
		return;
	}
}

/*
 * bring flying people down to earth so you can trip them.
 * by Alamor
 */
void do_lash(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_STRING_LENGTH];
	int chance;
	OBJ_DATA *wield;
	CHAR_DATA *victim;

	one_argument(argument, arg, sizeof(arg));

	if ((chance = get_skill(ch, gsn_lash)) == 0) {
		char_puts("Huh?\n",ch);
		return;
	}

	if (arg[0] == '\0') {
		victim = ch->fighting;
		if (victim == NULL) {
			char_puts("Lash whom?\n",ch);
			return;
		}
	}
	else {
		victim = get_char_room(ch, arg);
		if(victim == NULL) {
			char_puts("They aren't here.\n",ch);
			return;
		}
	}

	if ((wield = get_eq_char(ch, WEAR_WIELD)) == NULL
	|| wield->value[ITEM_WEAPON_TYPE] != WEAPON_WHIP) {
		char_puts("You must wield a whip to lash someone.\n",ch);
		return;
	}
	
	if (!victim || victim->in_room != ch->in_room) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("They aren't here.\n",ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_FLYING)) {
		char_puts("You would get better leverage if you were on the ground.\n",ch);
		return;
	}

	if (!IS_AFFECTED(victim, AFF_FLYING)) {
		WAIT_STATE(ch, SKILL(gsn_lash)->beats);
		char_puts("But they aren't flying!\n",ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
		act("$N is your beloved master!.",ch,NULL,victim,TO_CHAR);
		return;
	}

	if (is_safe(ch,victim))
		return;

	if (victim == ch) {
		char_puts("Try as you might, you cannot lash yourself.\n",ch);
		return;
	}			

	/* do checks! */

	chance += (get_curr_stat(ch, STAT_DEX) 
		- 2*get_curr_stat(victim, STAT_DEX));

	if ((IS_NPC(ch) && IS_SET(ch->pIndexData->off_flags, OFF_FAST)) 
	|| IS_AFFECTED(ch, AFF_HASTE)
	|| IS_AFFECTED(victim, AFF_SLOW))
		chance += 5;

	if ((IS_NPC(victim) && IS_SET(victim->pIndexData->off_flags, OFF_FAST))
	|| IS_AFFECTED(victim, AFF_HASTE)
	|| IS_AFFECTED(ch, AFF_SLOW))
		chance -= 10;

	chance += (LEVEL(ch) - LEVEL(victim)) * 3 / 2;

	DEBUG(DEBUG_SKILL_LASH,
		"%s[%d] lashes %s[%d]: %d%%",
		ch->name, LEVEL(ch),
		victim->name, LEVEL(ch),
		chance);

	if (number_percent() < chance) {
		act_puts("$n snags your ankle with $s whip and slams you to the ground!",
			 ch, NULL, victim, TO_VICT, POS_DEAD);
		act("You snag $N's ankle with your whip and slam $M to the ground!",
			 ch, NULL, victim, TO_CHAR);
		act("$n snag $N's ankle with $s whip and slams $M to the ground!",
			 ch, NULL, victim, TO_NOTVICT);
		REMOVE_BIT(victim->affected_by, AFF_FLYING);
		damage(ch, victim, number_range(10, 20 + 2*victim->size),
                       gsn_lash, DAM_BASH, TRUE);
		/*WAIT_STATE(victim, SKILL(gsn_lash)->beats*1/2);*/
		WAIT_STATE(ch, SKILL(gsn_lash)->beats);
		check_improve(ch, gsn_lash, TRUE, 1);
	}
	else {
		act("$n fails to grab your ankle with $s whip.",ch,NULL,victim,TO_VICT);
		act("You fail to grab $N's ankle with your whip.",ch,NULL,victim,TO_CHAR);
		act("$n fails to grab $N's ankle with $s whip.",ch,NULL,victim,TO_NOTVICT);
		WAIT_STATE(ch,SKILL(gsn_lash)->beats);
		check_improve(ch, gsn_lash, FALSE,1);
	}
}

/*
 * see through the eyes of the forest (remote where)
 * by Zsuzsu
 */
void do_clairvoyance (CHAR_DATA *ch, const char *argument)
{
	int		chance;
	int		sn;
	bool found = FALSE;
	bool can_see = FALSE;
	bool fPKonly = FALSE;
	CHAR_DATA *victim;
	DESCRIPTOR_DATA *d;
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *doppel;

	if ((sn = sn_lookup("clairvoyance")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}


	if (ch->in_room != NULL
	   && ch->in_room->sector_type != SECT_FOREST
	   && ch->in_room->sector_type != SECT_FIELD
	   && ch->in_room->sector_type != SECT_HILLS
	   && ch->in_room->sector_type != SECT_MOUNTAIN
	   && ch->in_room->sector_type != SECT_DESERT) {
		char_puts("You can't sense the creatures of the Forest from here.\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(sn)->beats);

	if (number_percent() > chance) {
		char_puts("You feel out of touch with the creatures of the Forest.\n", ch);
		check_improve(ch, sn, FALSE, 2);
		return;
	}

	char_puts("The creatures of the Forest lend you their senses:\n", ch);

	one_argument(argument, arg, sizeof(arg));

/*
	if (!check_blind(ch))
		return;
*/

	if (!str_cmp(arg,"pk"))
		fPKonly = TRUE;

		found = FALSE;
		for (d = descriptor_list; d; d = d->next) {
			if (d->connected == CON_PLAYING
			&&  (victim = d->character) != NULL
			&&  !IS_NPC(victim)
			&&  IS_TRUSTED(ch, victim->invis_level)
			&&  (!fPKonly || in_PK(ch, victim))
			&&  victim->in_room != NULL
			&&  victim->in_room->sector_type == SECT_FOREST
			&&  !IS_SET(victim->in_room->room_flags, ROOM_NOWHERE)
			) {

			can_see = ch == victim
				  || HAS_SKILL(victim, gsn_clairvoyance)
				  || IS_SET(ch->conf_flags, PLR_CONF_HOLYLIGHT
				  || (!IS_AFFECTED(victim, AFF_INVIS)
				  && !IS_AFFECTED(victim, AFF_IMP_INVIS)
				  && !IS_AFFECTED(victim, AFF_HIDE)
				  && !IS_AFFECTED(victim, AFF_FADE)));

				found = TRUE;

				if (is_affected(victim, gsn_doppelganger)
				&&  (IS_NPC(ch) || !IS_SET(ch->conf_flags, PLR_CONF_HOLYLIGHT)))
					doppel = victim->doppel;
				else
					doppel = victim;

				char_printf(ch, "%s%-16s {g%-22s {y:{x %s\n",
					(in_PK(ch, doppel) &&
					!IS_IMMORTAL(ch)) ?
					"{r[{RPK{r]{x " : "     ",
					can_see ? PERS(victim, ch) : "Someone",
					victim->in_room->area->name,
					mlstr_mval(victim->in_room->name));
			}
		}
		if (!found)
			char_puts("No interlopers are in the {gForest{x.\n", ch);

	check_improve(ch, sn, TRUE, 1);
}

/*
 * make a slave of your foes
 * by Zsuzsu
 */
void do_enslave(CHAR_DATA *ch, const char *argument)
{
        CHAR_DATA *victim;
        CHAR_DATA *vch;
        char arg[MAX_INPUT_LENGTH];
	int count = 0;

	if (IS_NPC(ch))
		return;

        argument = one_argument(argument, arg, sizeof(arg));

        if (arg[0] == '\0') {
		if (ch->pcdata->enslaver != NULL) {
			act("You are enslaved by $T.",
				ch, NULL, ch->pcdata->enslaver, TO_CHAR);
		}
		else if (get_skill(ch, gsn_enslave) <= 0) {
			act("You are free of the bonds of slavery.",
				ch, NULL, NULL, TO_CHAR);
		}

		if (get_skill(ch, gsn_enslave) > 0) {
			for (vch = char_list; 
			     vch && vch != char_list_lastpc; 
			     vch = vch->next) {
				if (vch->pcdata->enslaver
				&& can_see(ch, vch)) {
					count++;
					if (!str_cmp(vch->pcdata->enslaver, ch->name)) {
						act("{r$N{x is enslaved by {Dyou{x.",
							ch, NULL, vch, TO_CHAR);
					} else {
						act("{r$N{x is enslaved by {D$t{x.",
							ch, 
							vch->pcdata->enslaver,
							vch, TO_CHAR);
					}
				}
			}
			if (count == 0)
				char_puts("There are no slaves in the realm.\n",
					ch);
		}
		return;
	}

	/* Come out of hiding if you didn't just want a list */
	if (IS_AFFECTED(ch, AFF_HIDE | AFF_FADE)) {
		remove_hide_affect(ch, AFF_HIDE | AFF_FADE);
		act_puts("You step out of shadows.",
			ch, NULL, NULL, TO_CHAR, POS_DEAD);
		act("$n steps out of shadows.",
			ch, NULL, NULL, TO_ROOM);
	}

	if (IS_AFFECTED(ch, AFF_IMP_INVIS))  {
		affect_bit_strip(ch, TO_AFFECTS, AFF_IMP_INVIS);
		act_puts("You fade into existence.",
			ch, NULL, NULL, TO_CHAR, POS_DEAD);
		act("$n fades into existence.",
			ch, NULL, NULL, TO_ROOM);
	}       


        if ((victim = get_char_world(ch, arg)) == NULL) {
                char_puts("They aren't here.\n", ch);
                return;
        }

        if (IS_NPC(victim)) {
                char_puts("Just buy a pet!\n", ch);
                return;
        }

        if (IS_IMMORTAL(victim)) {
                char_puts("You are suddenly dizzy as you leave fantasy land behind.\n", ch);
                return;
	}

        if (victim == ch) {
                char_puts("You can't enslave yourself silly!\n", ch);
                return;
        }

	/* free slave */
	if (victim->pcdata->enslaver != NULL) {
		if (!str_cmp(ch->name, victim->pcdata->enslaver)
		|| ch->pcdata->clan_status == CLAN_LEADER
		|| IS_IMMORTAL(ch)) {
			act_puts("$N is unleashed from bondage.", 
				ch, NULL, victim, TO_CHAR, POS_DEAD);
			act_puts("You are unleashed from bondage!", 
				ch, NULL, victim, TO_VICT, POS_DEAD);
			victim->pcdata->enslaver = NULL;
		}
		else {
			char_puts("You cannot free another's slave from bondage.",
				ch);
		}
		return;
	}

	/* do enslavement */
	else {
		if (is_safe(ch, victim))
			return;

		if (victim->hit <= 1
		|| (ch->hit > victim->hit
		&& (ch->hit * 100 / ch->max_hit > 20)
		&& (victim->hit *100 / victim->max_hit) < 5)) {
			victim->pcdata->enslaver = ch->name;
			act("$E has become your {rslave{x!",
				ch, NULL, victim, TO_CHAR);
			act_puts("You have become $n's {rslave{x!\n"
			"Only your death or the death"
			" of your enslaver can free you from bondage.",
				ch, NULL, victim, TO_VICT, POS_DEAD);
			act("$n has {renslaved{x $N!",
				ch, NULL, victim, TO_NOTVICT);
			return;
		}
		else {
			act("$S will is yet too strong to enslave.",
				ch, NULL, victim, TO_CHAR);
			return;
		}
	}
}

/*** Rogue Skills ******************************************************/

/**
 * look inside a container of someone elses
 * by Zsuzsu
 */
void do_peek_inside(CHAR_DATA *ch, const char *argument)
{
	char arg1 [MAX_INPUT_LENGTH];
	char arg2 [MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA  *obj;
	int sn = 0;
	int chance;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0' || arg2[0] == '\0') {
		char_puts("Peek inside who's what?\n", ch);
		return;
	}

	if (IS_SET(ch->affected_by, AFF_CHARM)) {
		char_puts("Best just obey your master.\n", ch);
		return;
	}

	if (IS_SET(ch->affected_by, AFF_FEAR)) {
		char_puts("You're afraid you'll get caught.\n", ch);
		return;
	}

	if (MOUNTED(ch)) {
		  char_puts("Not while mounted.\n", ch);
		  return;
	}

	if ((victim = get_char_room(ch, arg2)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("Don't make things more difficult"
			  " than they already are for you.\n", ch);
		return;
	}

	if (victim->position == POS_FIGHTING) {
		char_puts("You'd better not -- you might get hit.\n", ch);
		return;
	}

	if ((obj = get_obj_list(ch, arg1, victim->carrying)) == NULL) {
		act("$N doesn't have that item.\n",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	if (obj->pIndexData->item_type != ITEM_CONTAINER) {
		act("You can't find a way to open $p to peek inside.",
			ch, obj, victim, TO_CHAR);
		return;
	}

	if (!IS_IMMORTAL(ch) 
	&& IS_SET(obj->value[ITEM_CONTAINER_FLAGS], CONT_LOCKED)) {
		act("$p is locked.",
			ch, obj, victim, TO_CHAR);
		return;
	}

	if (IS_IMMORTAL(victim) && ch->level < victim->level) {
		act("You'll be lucky if you don't get smote for that.",
			ch, NULL, NULL, TO_CHAR);
		act("$n seems a tad curious what's in $p.",
			ch, obj, victim, TO_VICT);
		return;
	}

	chance = (get_skill(ch, sn = gsn_rummage));

	if (chance <= 0) {
		WAIT_STATE(ch, PULSE_VIOLENCE);

		act("$n seems a tad curious what's in $p.",
			ch, obj, victim, TO_VICT);
		act("Leave the skullduggery to the professionals.",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	if (obj->wear_loc == WEAR_NONE) {
		chance = chance * 50 /100;
		chance += (get_curr_stat(ch,STAT_DEX) - 20) * 7;
		chance += UMIN(20, (LEVEL(ch) - LEVEL(victim)) * 2);
		chance += (get_skill(ch, gsn_furtive_riffle)*20/100);
	}
	else {
		chance = (get_curr_stat(ch,STAT_DEX) - 20) * 10;
		chance = chance * (get_skill(ch, sn = gsn_furtive_riffle)/100);
		chance += UMIN(20, (LEVEL(ch) - LEVEL(victim)) * 2);
	}

	if (IS_SET(obj->value[ITEM_CONTAINER_FLAGS], CONT_CLOSED))
		chance -= 10;

	chance += vigilance_modifier(ch, victim);

	WAIT_STATE(ch, SKILL(sn)->beats);

	if (!tax_move(ch, SKILL(sn)->move_cost)) {
		char_puts("You're too tired for that right now.\n", ch);
		return;
	}

	if (chance <= 0) {
		act("Don't think you got the chops to pull that off.",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	if (!IS_IMMORTAL(ch) && number_percent() > chance) {
		if (!IS_AFFECTED(ch, AFF_SLEEP)
		&& number_percent() < 
		(chance-25 + (get_curr_stat(ch, STAT_CHA) -20) * 5)) {
			act("$n seems a tad curious what's in $p.",
				ch, obj, victim, TO_VICT);
			add_pk_ok(victim, ch);
		}
		act("You couldn't sneek a peek into $p.",
			ch, obj, victim, TO_CHAR);
		return;
	}

	if (IS_NPC(victim) || !IS_AWAKE(victim))
		check_improve(ch, sn, TRUE, 20);
	else
		check_improve(ch, sn, TRUE, 3);

	act("You peek inside $p and see:",
		ch, obj, victim, TO_CHAR);

	show_list_to_char(obj->contains, ch, TRUE, TRUE);
}

/**
 * steal coins
 * This is more dependent on skill than dex, and is mostly
 * for races with low dex that want to be thieves.
 * by Zsuzsu
 */
void do_pickpocket (CHAR_DATA *ch, const char *argument)
{
	char		arg1[MAX_INPUT_LENGTH];
	CHAR_DATA 	*victim;
	int		chance = 0;
	int		chance_skill = 0;
	int		chance_dex = 0;
	int		chance_lvl = 0;
	int		chance_vig = 0;
	int		amount_g = 0;
	int		amount_s = 0;
	int		sn = 0;

	argument = one_argument(argument, arg1, sizeof(arg1));

	if (arg1[0] == '\0') {
		char_puts("Pick who's pocket?\n", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg1)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (get_skill(ch, gsn_pickpocket) <= 0) {
		WAIT_STATE(ch, PULSE_VIOLENCE);

		act("$n is eyeing your money-pouch.",
			ch, NULL, victim, TO_VICT);
		act("Leave the skullduggery to the professionals.",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	if (IS_NPC(ch) && IS_SET(ch->affected_by, AFF_CHARM)
	    && (ch->master != NULL)) {
		char_puts("You are too dazed to steal anything.\n", ch);
		return;
	}

	if (IS_SET(ch->affected_by, AFF_FEAR)) {
		char_puts("You're afraid you'll get caught.\n", ch);
		return;
	}

	if (MOUNTED(ch)) {
		  char_puts("Not while mounted.\n", ch);
		  return;
	}

	if (victim->in_room 
	&& (IS_SET(victim->in_room->room_flags, ROOM_BATTLE_ARENA)
	|| IS_SET(victim->in_room->room_flags, ROOM_SAFE))) {
		act("You're such a cad.\n",
			ch, NULL, victim, TO_CHAR);
		act("You find $n's hands in your pocket!\n",
			ch, NULL, victim, TO_VICT);
		return;
	}

	if (!IS_IMMORTAL(ch) && 
	!in_PK(ch, victim) && ch->level > victim->level) {
		act("That mark isn't worth your time.",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	if (!IS_IMMORTAL(victim) && !in_PK(ch, victim)) {
		add_pk_ok(victim, ch);
	}

	if (IS_IMMORTAL(victim) && ch->level < victim->level) {
		act("You'll be lucky if you don't get smote for that.",
			ch, NULL, NULL, TO_CHAR);
		act("$n tried to steal gold from you!",
			ch, NULL, victim, TO_VICT);
		return;
	}

	chance = chance_skill = (get_skill(ch, sn = gsn_pickpocket) * 50/100);

	chance += chance_dex = (get_curr_stat(ch,STAT_DEX) - 18) * 3;

	chance += chance_lvl = LEVEL(ch) - LEVEL(victim);

	chance += (get_eq_char(ch, WEAR_WIELD) == NULL) ? 3 : 0;

	chance += chance_vig = vigilance_modifier(ch, victim);

	DEBUG(DEBUG_SKILL_STEAL,
		"%s pickpockets %s: skill:%d lvl:%+d dex:%+d"
		" vig:%+d = %d",
		mlstr_mval(ch->short_descr),
		mlstr_mval(victim->short_descr),
		chance_skill,
		chance_lvl,
		chance_dex,
		chance_vig,
		chance);

	WAIT_STATE(ch, SKILL(sn)->beats);

	if (!tax_move(ch, SKILL(sn)->move_cost)) {
		char_puts("You're too tired for that right now.\n", ch);
		return;
	}

	if (chance <= 0) {
		act("Don't think you got the chops to pull that off.",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	if (IS_NPC(victim) && victim->pIndexData->pShop != NULL) {
		act_puts("You'd have a better chance at stealing $S wares.\n",
			ch, NULL, victim, TO_CHAR, POS_DEAD);
		return;
	}

	if (number_percent() > UMAX(5, chance)
	||  IS_SET(victim->imm_flags, IMM_STEAL)) {
		check_improve(ch, sn, FALSE, 2);
		steal_failure(ch, victim, NULL, NULL);
		return;
	}

	if (get_skill(ch, gsn_pickpocket) > 90 
	&& number_percent() < 68 + ch->level/3)
		amount_g = victim->gold * number_range(1, 7) / 100;

	if (amount_g <= 0)
		amount_s = victim->silver * number_range(1, 20) / 100;

	if (amount_s <= 0 && amount_g <= 0) {
		act("Hmm.  $N is poorer than you are.",
			ch, NULL, victim, TO_CHAR);
		return;
	}


	ch->gold += amount_g;
	victim->gold -= amount_g;
	ch->silver += amount_s;
	victim->silver -= amount_s;

	char_printf(ch, "Bingo!  You got %d %s coins.\n",
		    amount_s != 0 ? amount_s : amount_g,
		    amount_s != 0 ? "{Wsilver{x" : "{Ygold{x");

	if (!IS_AWAKE(victim))
		check_improve(ch, sn, TRUE, 10);
	else {
		check_improve(ch, sn, TRUE, 1);
		if (IS_AFFECTED(ch, AFF_CURSE)
		|| (ch->level < LEVEL_HERO 
		&& number_percent() < 5))
			act("Your purse feels a bit lighter than usual.",
				victim, NULL, NULL, TO_CHAR);
	}
	wiznet_theft(ch, victim, NULL, NULL, THEFT_STEAL, 
			chance, TRUE, amount_g *100 + amount_s);
}

/**
 * syntax: steal item container person
 *         steal item person
 * (not for coins)
 * by Zsuzsu
 */
void do_steal (CHAR_DATA * ch, const char *argument)
{
	char            arg1[MAX_INPUT_LENGTH];
	char            arg2[MAX_INPUT_LENGTH];
	char            arg3[MAX_INPUT_LENGTH];
	CHAR_DATA      *victim;
	OBJ_DATA       *obj;
	int		chance = 0;
	int		chance_skill = 0;
	int		chance_dex = 0;
	int		chance_lvl = 0;
	int		chance_vig = 0;
	int		chance_olvl = 0;
	int		chance_cost = 0;
	int		sn;
	bool		success = FALSE;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
	argument = one_argument(argument, arg3, sizeof(arg2));

	if (arg1[0] == '\0' || arg2[0] == '\0') {
		char_puts("Steal what from whom?\n", ch);
		return;
	}

	if (IS_NPC(ch) && IS_SET(ch->affected_by, AFF_CHARM)
	    && (ch->master != NULL)) {
		char_puts("You are too dazed to steal anything.\n", ch);
		return;
	}

	if (IS_SET(ch->affected_by, AFF_FEAR)) {
		char_puts("You're afraid you'll get caught.\n", ch);
		return;
	}

	if (MOUNTED(ch)) {
		  char_puts("Not while mounted.\n", ch);
		  return;
	}

	if (arg3[0] != '\0')
		victim = get_char_room(ch, arg3);
	else
		victim = get_char_room(ch, arg2);

	if (victim == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("That would be pretty pointless.\n", ch);
		return;
	}

	if (victim->position == POS_FIGHTING) {
		char_puts("You'd better not -- you might get hit.\n", ch);
		return;
	}
	
	if (victim->in_room 
	&& (IS_SET(victim->in_room->room_flags, ROOM_BATTLE_ARENA)
	|| IS_SET(victim->in_room->room_flags, ROOM_SAFE))) {
		act("You're such a cad.\n",
			ch, NULL, victim, TO_CHAR);
		act("You find $n's hands in your pocket!\n",
			ch, NULL, victim, TO_VICT);
		return;
	}

	if (!IS_IMMORTAL(ch) && 
	!in_PK(ch, victim) && ch->level > victim->level) {
		act("That mark isn't worth your time.",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	if (!IS_IMMORTAL(victim) && !in_PK(ch, victim)) {
		add_pk_ok(victim, ch);
	}

	if (arg3[0] != '\0') {
		steal_from_container(ch, victim, arg2, arg1);
		return;
	}

	if ((obj = get_obj_list(ch, arg1, victim->carrying)) == NULL) {
		act("$N doesn't have that item.\n",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	if (IS_IMMORTAL(victim) && ch->level < victim->level) {
		act("You'll be lucky if you don't get smote for that.",
			ch, NULL, NULL, TO_CHAR);
		act("$n tried to steal $p from you!",
			ch, obj, victim, TO_VICT);
		return;
	}

	if (obj->wear_loc != WEAR_NONE) {
		act("A specialized technique is required for this job.\n",
			ch, obj, victim, TO_CHAR);
		return;
	}

	chance = chance_skill = (get_skill(ch, sn = gsn_steal) * 20/100);
	chance += chance_dex = (get_curr_stat(ch,STAT_DEX) - 20) * 3;
	if (!IS_NPC(victim))
		chance += chance_lvl = UMIN(30, (LEVEL(ch) - LEVEL(victim)) * 3);
	else
		chance += chance_lvl = (LEVEL(ch) - LEVEL(victim))/5;

	chance += (get_eq_char(ch, WEAR_WIELD) == NULL) ? 3 : 0;

	chance += chance_vig = vigilance_modifier(ch, victim);

	/* higher level objects more difficult to steal */
	if (IS_NPC(victim))
		chance += chance_olvl = UMIN(0, (LEVEL(ch) - obj->level))*2;

	/* more expensive objects harder to steal */
	if (IS_NPC(victim))
		chance += URANGE(-100,
			chance_cost = ((LEVEL(ch)*2 - obj->cost/100)/2),
			50);

	DEBUG(DEBUG_SKILL_STEAL,
		"%s steal from %s: skill:%d lvl:%+d dex:%+d"
		" vig:%+d olvl:%+d cost:%+d = %d",
		mlstr_mval(ch->short_descr),
		mlstr_mval(victim->short_descr),
		chance_skill,
		chance_lvl,
		chance_dex,
		chance_vig,
		chance_olvl,
		chance_cost,
		chance);

	WAIT_STATE(ch, SKILL(sn)->beats);

	if (!tax_move(ch, SKILL(sn)->move_cost)) {
		char_puts("You're too tired for that right now.\n", ch);
		return;
	}

	if (chance <= 0) {
		act("Don't think you got the chops to pull that off.",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	chance = UMIN(99, chance);

	if ((!IS_NPC(ch) && number_percent() > chance)
	||  IS_SET(victim->imm_flags, IMM_STEAL)) {
		check_improve(ch, sn, FALSE, 4);
		steal_failure(ch, victim, NULL, obj);
		success = FALSE;
	}
	else {
		if ((success = steal_success(ch, victim, NULL, obj))
		&& chance < 35) {
			check_improve(ch, sn, TRUE, 1);
		}
	}
	wiznet_theft(ch, victim, NULL, obj, THEFT_STEAL, 
			chance, success, obj->cost);
}

/**
 * steal an item from a container
 * by Zsuzsu
 */
void steal_from_container (CHAR_DATA *ch, CHAR_DATA *victim, 
		const char *arg1, const char *arg2)
{
	OBJ_DATA	*container = NULL;
	OBJ_DATA       *obj;
	int		chance = 0;
	int		sn = 0;
	int		clutter = 0;
	int		chance_lvl = 0;
	int		chance_dex = 0;
	int		chance_cha = 0;
	int		chance_skill = 0;
	int		chance_clutter = 0;

	if ((container = get_obj_list(ch, arg1, victim->carrying)) == NULL) {
		act("$N doesn't have that container.\n",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	chance_lvl = chance = UMIN(10, (LEVEL(ch) - LEVEL(victim)) * 2);

	/* stealing from a container */
	if (container->wear_loc == WEAR_NONE) {
		chance += chance_dex = (get_curr_stat(ch,STAT_DEX) - 20) * 10;
		chance = chance_skill 
			= chance * (get_skill(ch, sn = gsn_pilfer) /100);
		chance += (get_eq_char(ch, WEAR_WIELD) == NULL) ? 5 : 0;
	}
	else {
		chance += chance_dex = (get_curr_stat(ch,STAT_DEX) - 20) * 4;
		chance = chance * ((chance_skill 
				= get_skill(ch, sn = gsn_featherhand)) /100);
		chance += (get_eq_char(ch, WEAR_WIELD) == NULL) ? 5 : 0;
	}

	clutter = obj_in_context_count(container, FALSE);

	chance += chance_clutter = URANGE(-10, (clutter - 8) * 2, 30);

	DEBUG(DEBUG_SKILL_STEAL,
		"%s steal in from %s: lvl:%+d dex:%+d skill:%d%% clut:%+d = %d",
		mlstr_mval(ch->short_descr),
		mlstr_mval(victim->short_descr),
		chance_lvl,
		chance_dex,
		chance_skill,
		chance_clutter,
		chance);

	if (chance <= 0) {
		act("You don't have the chops to pull off this heist.",
			ch, NULL, NULL, TO_CHAR);
		return;
	}

	if (container->pIndexData->item_type != ITEM_CONTAINER) {
		act("You can't find a way to open $p.",
			ch, container, victim, TO_CHAR);
		return;
	}

	if (!IS_IMMORTAL(ch) 
	&& IS_SET(container->value[ITEM_CONTAINER_FLAGS], CONT_LOCKED)) {
		act("$p is locked.",
			ch, container, victim, TO_CHAR);
		return;
	}

	if ((obj = get_obj_list(ch, arg2, container->contains)) == NULL) {
		act("$p doesn't contain that item.",
			ch, container, victim, TO_CHAR);
		return;
	}

	if (IS_IMMORTAL(victim) && ch->level < victim->level) {
		act("You'll be lucky if you don't get smote for that.",
			ch, NULL, NULL, TO_CHAR);
		act("$n tried to steal from $p!",
			ch, obj, victim, TO_VICT);
		return;
	}

	chance = UMAX(5, chance);
		
	if ((!IS_NPC(ch) && number_percent() > chance)
	||  IS_SET(victim->imm_flags, IMM_STEAL)) {
		check_improve(ch, sn, FALSE, 4);
		steal_failure(ch, victim, NULL, obj);
		wiznet_theft(ch, victim, container, obj, THEFT_STEAL, 
				chance, FALSE, obj->cost);
		return;
	}

	if (steal_success(ch, victim, container, obj)) {
		wiznet_theft(ch, victim, container, obj, THEFT_STEAL, 
				chance, TRUE, obj->cost);
		if (!IS_AWAKE(victim))
			check_improve(ch, sn, TRUE, 20);
		else
			check_improve(ch, sn, TRUE, 2);

		if (sn == gsn_featherhand 
		&& (obj->pIndexData->vnum == QUEST_VNUM_RUG
		|| IS_AFFECTED(ch, AFF_CURSE)
		|| number_percent() < UMAX(5, 95-clutter*2))) {
			if (IS_AFFECTED(victim, AFF_SLEEP)) {
				REMOVE_BIT(victim->affected_by, AFF_SLEEP);
				affect_bit_strip(victim, TO_AFFECTS, AFF_SLEEP);
			}

			if (steal_failure(ch, victim, container, obj)) {
				chance = 5;
				chance = chance_cha 
					= (get_curr_stat(ch,STAT_CHA) - 20) * 2;
				chance = chance_skill 
					= chance * (get_skill(ch, gsn_guile) /100);
				DEBUG(DEBUG_SKILL_GUILE,
					"%s guile %s: cha:%+d skill:%d%% = %d",
					mlstr_mval(ch->short_descr),
					mlstr_mval(victim->short_descr),
					chance_cha, chance_skill, chance);

				if (number_percent() < chance) {
					act("You flash $N an innocent smile.",
						ch, NULL, victim, TO_CHAR);
					check_improve(ch, gsn_guile, TRUE, 1);
				}
				else {
					check_improve(ch, gsn_guile, FALSE, 3);
					multi_hit(victim, ch, TYPE_UNDEFINED);
				}
			}
		}
	}
}

/**
 * successfully stole the item.
 * Note: ITEM_INVENTORY is for shops
 * by Zsuzsu
 */
bool steal_success (CHAR_DATA *ch, CHAR_DATA *victim,
		OBJ_DATA *container, OBJ_DATA *obj)
{
	OBJ_DATA *obj_inve = NULL;

	if (!can_drop_obj(ch, obj)) {
		char_puts("You can't pry it away.\n", ch);
		return FALSE;
	}

	if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch)) {
		char_puts("You have your hands full.\n", ch);
		return FALSE;
	}

	if (ch->carry_weight + get_obj_weight(obj) > ch_max_carry_weight(ch)) {
		char_puts("You can't carry that much weight.\n", ch);
		return FALSE;
	}

	if (!IS_SET(obj->extra_flags, ITEM_INVENTORY)) {
		if (container)
			obj_from_obj(obj);
		else
			obj_from_char(obj);
		obj_to_char(obj, ch);
		char_puts("You got it!\n", ch);
	} else {
		obj_inve = create_obj(obj->pIndexData, 0);
		clone_obj(obj, obj_inve);
		REMOVE_BIT(obj_inve->extra_flags, ITEM_INVENTORY);
		obj_to_char(obj_inve, ch);
		char_puts("You got one of them!\n", ch);
		obj = obj_inve;
	}

	/* mark the item as stolen */
	if (IS_NPC(victim) && victim->pIndexData->pShop != NULL) {
		obj->stolen_from = victim->pIndexData->vnum;
	}

	oprog_call(OPROG_GET, obj, ch, NULL);
	return TRUE;
}

/*
 * failed to steal an item.
 * CHA determines if they get caught or not.
 * return indicates if they were really caught or not
 * by Zsuzsu
 */
bool steal_failure (CHAR_DATA *ch, CHAR_DATA *victim,
		OBJ_DATA *container, OBJ_DATA *obj)
{
	int chance = 0;
	int chance_cha = 0;
	int chance_skill = 0;

	char_puts("Oops.\n", ch);

	if (IS_AFFECTED(ch, AFF_HIDE | AFF_FADE) && !IS_NPC(ch)) {
		remove_hide_affect(ch, AFF_HIDE | AFF_FADE);
		act_puts("You step out of shadows.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		act("$n steps out of shadows.",
		    ch, NULL, NULL, TO_ROOM);
	}

	if (IS_AFFECTED(victim, AFF_SLEEP)
	|| (!IS_AWAKE(victim) && number_percent() < 20)) {
		switch(number_range(0,3)) {
		case 0:
			act_puts("Your dreams are plagued with a sense of loss.",
				victim, NULL, NULL, TO_CHAR, POS_DEAD);
			break;
		case 1:
			act_puts("Someone is tickling you!",
				victim, NULL, NULL, TO_CHAR, POS_DEAD);
			break;
		case 2:
			act_puts("Nightmares of poverty fill your sleep.",
				victim, NULL, NULL, TO_CHAR, POS_DEAD);
			break;
		case 3:
			act_puts("You dream there's a hole in your pouch.",
				victim, NULL, NULL, TO_CHAR, POS_DEAD);
			break;
		}
		act("$n stirs restlessly in $s sleep.",
			victim, NULL, NULL, TO_ROOM);
		return FALSE;
	}

	if (!IS_AWAKE(victim)) {
		act_puts("In a groggy haze you awake to find $N's hand in your pocket!",
			victim, NULL, ch, TO_CHAR, POS_DEAD);
		act("$N wakes to find your hand in $S pocket -- how embarassing!",
			ch, NULL, victim, TO_CHAR);

		chance = (ch->sex == SEX_FEMALE && victim->sex == SEX_MALE) 
			? 10 : 0;
		chance += chance_cha = (get_curr_stat(ch,STAT_CHA) - 20) * 5;
		chance = chance * (chance_skill 
			= get_skill(ch, gsn_guile)) /100;

		DEBUG(DEBUG_SKILL_GUILE,
			"%s guile %s: cha:%+d skill:%d%% = %d",
			mlstr_mval(ch->short_descr),
			mlstr_mval(victim->short_descr),
			chance_cha, chance_skill, chance);

		/* if caught, there's a chance you can get away on your charms*/
		if (number_percent() < chance) {
			act_puts("$n flashes you an innocent smile.\n"
			    "You fall back asleep.",
				ch, NULL, victim, TO_VICT, POS_DEAD);
			act("You flash $N an innocent smile.\n"
			    "$S slips back into sleep.",
			    	ch, NULL, victim, TO_CHAR);
			check_improve(ch, gsn_guile, TRUE, 1);
			return FALSE;
		}

		victim->position = POS_STANDING;
		act("You leap to your feet.",
			ch, NULL, NULL, TO_CHAR);
		act("$n leaps to $s feet.",
			ch, NULL, NULL, TO_ROOM);
	}

	if (victim->position < POS_STANDING)
		victim->position = POS_STANDING;

	if (obj != NULL && number_percent() < 10)
		act("$n tried to steal $p from you!",
			ch, obj, victim, TO_VICT);
	else
		act("$n tried to steal from you.\n", 
			ch, NULL, victim, TO_VICT);

	act("$n tried to steal from $N.\n",
		ch, NULL, victim, TO_NOTVICT);

	switch (number_range(0, 3)) {
	case 0:
		doprintf(do_yell, victim, "%s is a lousy thief!", 
			PERS(ch, victim));
		break;
	case 1:
		doprintf(do_yell, victim, "%s couldn't rob %s"
					  " way out of a paper bag!",
			PERS(ch, victim),
			(ch->sex == SEX_FEMALE) ? "her" :
			(ch->sex == SEX_MALE) ? "his" :
			"its");
		break;
	case 2:
		doprintf(do_yell, victim, "%s tried to rob me!",
			 PERS(ch, victim));
		break;
	case 3:
		doprintf(do_yell, victim, "Keep your hands out of there, %s!",
			 PERS(ch, victim));
		break;
	}


	if (!IS_NPC(ch) && IS_NPC(victim)) {
		chance = 5;
		chance = chance_cha = (get_curr_stat(ch,STAT_CHA) - 20) * 2;
		chance = chance_skill 
			= chance * (get_skill(ch, gsn_guile) /100);

		DEBUG(DEBUG_SKILL_GUILE,
			"%s guile %s: cha:%+d skill:%d%% = %d",
			mlstr_mval(ch->short_descr),
			mlstr_mval(victim->short_descr),
			chance_cha, chance_skill, chance);

		if (number_percent() < chance) {
			act("You flash $N an innocent smile.",
			    	ch, NULL, victim, TO_CHAR);
			check_improve(ch, gsn_guile, TRUE, 1);
			return FALSE;
		}
		else {
			check_improve(ch, gsn_guile, FALSE, 3);
			multi_hit(victim, ch, TYPE_UNDEFINED);
			return TRUE;
		}
	}

	return TRUE;
}


/**
 * switching something the victim is wearing with something
 * the character has in inventory.
 * by Zsuzsu
 */
void do_switcheroo (CHAR_DATA * ch, const char *argument)
{
	char            arg1[MAX_INPUT_LENGTH];
	char            arg2[MAX_INPUT_LENGTH];
	char            arg3[MAX_INPUT_LENGTH];
	CHAR_DATA      *victim;
	OBJ_DATA       *obj;
	OBJ_DATA       *obj2;
	int		chance = 0;
	int		chance_skill = 0;
	int		chance_dex = 0;
	int		chance_cha = 0;
	int		chance_lvl = 0;
	int		sn;
	int		location = 0;
	bool		success = FALSE;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
	argument = one_argument(argument, arg3, sizeof(arg2));

	if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
		char_puts("Switch what on whom with what?\n", ch);
		return;
	}

	if (IS_NPC(ch) && IS_SET(ch->affected_by, AFF_CHARM)
	    && (ch->master != NULL)) {
		char_puts("You are too dazed to pull a fast one.\n", ch);
		return;
	}

	if (IS_SET(ch->affected_by, AFF_FEAR)) {
		char_puts("You're afraid you'll get caught.\n", ch);
		return;
	}

	if (MOUNTED(ch)) {
		  char_puts("Not while mounted.\n", ch);
		  return;
	}

	victim = get_char_room(ch, arg2);

	if (victim == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("That would be pretty pointless.\n", ch);
		return;
	}

	if (IS_IMMORTAL(victim) && ch->level < victim->level) {
		act("You'll be lucky if you don't get smote for that.",
			ch, NULL, NULL, TO_CHAR);
		return;
	}

	if (IS_AWAKE(victim)) {
		char_puts("This will only work if your victim is snoring.\n",ch);
		return;
	}

	if (victim->in_room 
	&& (IS_SET(victim->in_room->room_flags, ROOM_BATTLE_ARENA)
	|| IS_SET(victim->in_room->room_flags, ROOM_SAFE))) {
		act("You're such a cad.\n",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	if (!IS_IMMORTAL(ch) && 
	!in_PK(ch, victim) && ch->level > victim->level) {
		act("That mark isn't worth your time.",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	if ((obj = get_obj_list(ch, arg1, victim->carrying)) == NULL) {
		act("$N doesn't have that item.\n",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	if ((obj2 = get_obj_list(ch, arg3, ch->carrying)) == NULL) {
		act("$N doesn't have that item.\n",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	if (obj->wear_loc == WEAR_NONE) {
		act("$N has got to be using $p for this technique to work.",
			ch, obj, victim, TO_CHAR);
		return;
	}

	if (obj2->wear_loc != WEAR_NONE) {
		act("You have to stop using $p first.",
			ch, obj2, victim, TO_CHAR);
		return;
	}

	if (obj->wear_flags != obj2->wear_flags) {
		char_puts("Both items need to be worn similarly.\n", ch);
		return;
	}

	if (obj->wear_loc == WEAR_TATTOO
	|| obj->wear_loc == WEAR_CLANMARK) {
		act("$p is not coming off.\n",
			ch, obj, victim, TO_CHAR);
		return;
	}

	if (IS_SET(obj->extra_flags, ITEM_NODROP)
	|| IS_SET(obj->extra_flags, ITEM_NOREMOVE)
	|| IS_SET(obj->extra_flags, ITEM_INVENTORY)
	|| IS_SET(obj->extra_flags, ITEM_VIS_DEATH)
	|| IS_SET(obj->extra_flags, ITEM_NOUNCURSE)) {
		act("$p is not coming off.\n",
			ch, obj, victim, TO_CHAR);
		return;
	}

	if (IS_SET(obj2->extra_flags, ITEM_NODROP)
	|| IS_SET(obj2->extra_flags, ITEM_NOREMOVE)
	|| IS_SET(obj2->extra_flags, ITEM_INVENTORY)
	|| IS_SET(obj2->extra_flags, ITEM_VIS_DEATH)
	|| IS_SET(obj2->extra_flags, ITEM_NOUNCURSE)) {
		act("$p can't be used to replace anything.\n",
			ch, obj2, victim, TO_CHAR);
		return;
	}

	if (!IS_IMMORTAL(victim) && !in_PK(ch, victim)) {
		add_pk_ok(victim, ch);
	}

	chance = chance_dex = (get_curr_stat(ch,STAT_DEX) - 20) * 3;
	chance = chance * (chance_skill 
		= get_skill(ch, sn = gsn_switcheroo))/100;

	if (IS_AFFECTED(victim, AFF_SLEEP))
		chance += 5;
	else
		chance -= 5;

	if (!IS_NPC(victim))
		chance += chance_lvl = UMIN(0, (LEVEL(ch) - LEVEL(victim)) * 2);
	else
		chance += chance_lvl = (LEVEL(ch) - LEVEL(victim))/5;

	chance += (get_eq_char(ch, WEAR_WIELD) != NULL) ? -3 : 0;

	if (IS_DRUNK(victim))
		chance += 10;

	DEBUG(DEBUG_SKILL_STEAL,
		"%s switcheroo %s: skill:%d lvl:%+d dex:%+d"
		" drunk:%+d = %d",
		mlstr_mval(ch->short_descr),
		mlstr_mval(victim->short_descr),
		chance_skill,
		chance_lvl,
		chance_dex,
		IS_DRUNK(victim) ? 10 : 0,
		chance);

	WAIT_STATE(ch, SKILL(sn)->beats);

	if (!tax_move(ch, SKILL(sn)->move_cost)) {
		char_puts("You're too tired for that right now.\n", ch);
		return;
	}

	if (chance <= 0) {
		act("Don't think you got the chops to pull that off.",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	chance = UMIN(99, chance);

	if ((!IS_NPC(ch) && number_percent() > chance)
	||  IS_SET(victim->imm_flags, IMM_STEAL)) {
		check_improve(ch, sn, FALSE, 2);
		if (IS_AFFECTED(victim, AFF_SLEEP)) {
			REMOVE_BIT(victim->affected_by, AFF_SLEEP);
			affect_bit_strip(victim, TO_AFFECTS, AFF_SLEEP);
		}
		steal_failure(ch, victim, NULL, obj);
		success = FALSE;
		wiznet_theft(ch, victim, obj2, obj, THEFT_SWITCH, 
				chance, success, obj->cost);
	}
	else {
		obj_from_char(obj2);
		location = obj->wear_loc;
		if ((success = steal_success(ch, victim, NULL, obj))) {
			check_improve(ch, sn, TRUE, 1);
			obj_to_char(obj2, victim);
			equip_char(victim, obj2, location);

			wiznet_theft(ch, victim, obj2, obj, THEFT_SWITCH, 
					chance, success, obj->cost);

			if (IS_AFFECTED(victim, AFF_SLEEP)) {
				REMOVE_BIT(victim->affected_by, AFF_SLEEP);
				affect_bit_strip(victim, TO_AFFECTS, AFF_SLEEP);
			}
			victim->position = POS_STANDING;

			chance = 2;
			chance = chance_cha 
				= (get_curr_stat(ch,STAT_CHA) - 20) * 1;
			chance = chance_skill 
				= chance * (get_skill(ch, gsn_guile) /100);

			DEBUG(DEBUG_SKILL_GUILE,
				"%s guile %s: cha:%+d skill:%d%% = %d",
				mlstr_mval(ch->short_descr),
				mlstr_mval(victim->short_descr),
				chance_cha, chance_skill, chance);

			if (number_percent() < chance) {
				act("Something's fishy, but you're not sure what.",
					ch, NULL, victim, TO_VICT);
				act("You flash $N an innocent smile.",
					ch, NULL, victim, TO_CHAR);
				check_improve(ch, gsn_guile, TRUE, 1);
			}
			else {
				doprintf(do_yell, victim,
					"%s is a rotten scoundrel!",
					PERS(ch, victim));
				check_improve(ch, gsn_guile, FALSE, 3);
				multi_hit(victim, ch, TYPE_UNDEFINED);
			}
		}
		else {
			obj_to_char(obj2, ch);
		}
	}
}

/**
 * plant coins on someone -- usually to increase their encumberance
 * by Zsuzsu
 */
void do_chumpchange (CHAR_DATA *ch, const char *argument)
{
	char            arg1[MAX_INPUT_LENGTH];
	char            arg2[MAX_INPUT_LENGTH];
	char            arg3[MAX_INPUT_LENGTH];
	CHAR_DATA      *victim;
	int		sn;
	bool		success = FALSE;
	bool		is_gold = FALSE;
	int		coins = 0;
	int		chance_skill = 0;
	int		chance_lvl = 0;
	int		chance_dex = 0;
	int		chance_vig = 0;
	int		chance_weight = 0;
	int		chance = 0;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
	argument = one_argument(argument, arg3, sizeof(arg2));

	if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
		char_puts("Slip what kind of chumpchange"
				" into who's purse?\n", ch);
		return;
	}

	if (IS_NPC(ch) && IS_SET(ch->affected_by, AFF_CHARM)
	    && (ch->master != NULL)) {
		char_puts("You are too dazed to plant anything.\n", ch);
		return;
	}

	if (IS_SET(ch->affected_by, AFF_FEAR)) {
		char_puts("You're afraid you'll get caught.\n", ch);
		return;
	}
	if (MOUNTED(ch)) {
		  char_puts("Not while mounted.\n", ch);
		  return;
	}

	if (!is_number(arg1)) {
		char_puts("How much were you planning on slipping them?\n", ch);
		return;
	}

	coins = atoi(arg1);

	if (!(is_gold = !str_prefix(arg2, "gold"))
	&& str_prefix(arg2, "silver")) {
		char_puts("Did you want to slip some gold or silver?\n", ch);
		return;
	}

	victim = get_char_room(ch, arg3);

	if (victim == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("That would be pretty pointless.\n", ch);
		return;
	}

	if (victim->position == POS_FIGHTING) {
		char_puts("You'd better not -- you might get hit.\n", ch);
		return;
	}
	
	if (victim->in_room 
	&& (IS_SET(victim->in_room->room_flags, ROOM_BATTLE_ARENA)
	|| IS_SET(victim->in_room->room_flags, ROOM_SAFE))) {
		act("You're such a cad.\n",
			ch, NULL, victim, TO_CHAR);
		act("You find $n's hands in your pocket!\n",
			ch, NULL, victim, TO_VICT);
		return;
	}

	/*
	if (!IS_IMMORTAL(ch) && 
	!in_PK(ch, victim) && ch->level > victim->level) {
		act("That mark isn't worth your time.",
			ch, NULL, victim, TO_CHAR);
		return;
	}
	*/

	if (get_skill(ch, sn = gsn_chumpchange) <= 0) {
		act("Stop playing around and just give it to $H.",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	if (is_gold)
		coins = UMIN(coins, ch->gold);
	else
		coins = UMIN(coins, ch->silver);

	if (coins <= 0) {
		act("Maybe you should get more coins first.",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	/*
	if (!IS_IMMORTAL(victim) && !in_PK(ch, victim)) {
		add_pk_ok(victim, ch);
	}
	*/

	if (get_skill(ch, sn) < 100) 
		coins = coins * (get_skill(ch, sn) - number_range(0, 10)) /100;

	if (coins <= 0)
		coins = 1;
	
	chance = chance_skill = (get_skill(ch, sn) * 90/100);
	chance += chance_dex = (get_curr_stat(ch,STAT_DEX) - 20) * 3;

	chance += chance_lvl = (LEVEL(ch) - LEVEL(victim));

	chance += (get_eq_char(ch, WEAR_WIELD) == NULL) ? 5 : 0;

	chance += chance_vig = vigilance_modifier(ch, victim);

	if (is_gold)
		chance_weight = -1 * coins / 5;
	else
		chance_weight = -1 * coins / 100;

	chance += chance_weight;

	DEBUG(DEBUG_SKILL_STEAL,
		"%s chumpchange %s: skill:%d lvl:%+d dex:%+d"
		" vig:%+d weight:%+d = %d",
		mlstr_mval(ch->short_descr),
		mlstr_mval(victim->short_descr),
		chance_skill,
		chance_lvl,
		chance_dex,
		chance_vig,
		chance_weight,
		chance);

	WAIT_STATE(ch, SKILL(sn)->beats);

	if (!tax_move(ch, SKILL(sn)->move_cost)) {
		char_puts("You're too tired for that right now.\n", ch);
		return;
	}

	if (chance <= 0) {
		act("Don't think you got the chops to pull that off.",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	chance = UMIN(99, chance);

	if ((!IS_NPC(ch) && number_percent() > chance)
	||  IS_SET(victim->imm_flags, IMM_STEAL)) {
		check_improve(ch, sn, FALSE, 4);
		steal_failure(ch, victim, NULL, NULL);
		success = FALSE;
	}
	else {
		success = TRUE;
		if (is_gold)
			victim->gold += coins;
		else
			victim->silver += coins;

		char_printf(ch, "How nice of you to donate %d %s to %s.\n",
			coins,
			is_gold ? "gold" : "silver",
			PERS(victim, ch));
	}

	wiznet_theft(ch, victim, NULL, NULL, THEFT_PLANT, 
			chance, success, is_gold ? coins * -1 : coins);
}

/**
 * put an object on someone w/o their knowing.
 * syntax: objToPlant victim container
 * by Zsuzsu
 */
void do_plant (CHAR_DATA *ch, const char *argument)
{
	char            arg1[MAX_INPUT_LENGTH];
	char            arg2[MAX_INPUT_LENGTH];
	char            arg3[MAX_INPUT_LENGTH];
	CHAR_DATA      *victim;
	OBJ_DATA       *obj;
	OBJ_DATA       *container = NULL;
	int		chance = 0;
	int		chance_skill = 0;
	int		chance_dex = 0;
	int		chance_lvl = 0;
	int		chance_vig = 0;
	int		chance_weight = 0;
	int		chance_clutter = 0;
	int		clutter = 0;
	int		sn;
	bool		success = FALSE;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
	argument = one_argument(argument, arg3, sizeof(arg2));

	if (arg1[0] == '\0' || arg2[0] == '\0') {
		char_puts("Plant what on whom?\n", ch);
		return;
	}

	if (IS_NPC(ch) && IS_SET(ch->affected_by, AFF_CHARM)
	    && (ch->master != NULL)) {
		char_puts("You are too dazed to plant anything.\n", ch);
		return;
	}

	if (IS_SET(ch->affected_by, AFF_FEAR)) {
		char_puts("You're afraid you'll get caught.\n", ch);
		return;
	}

	if (MOUNTED(ch)) {
		  char_puts("Not while mounted.\n", ch);
		  return;
	}

	victim = get_char_room(ch, arg2);

	if (victim == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("That would be pretty pointless.\n", ch);
		return;
	}

	if (victim->position == POS_FIGHTING) {
		char_puts("You'd better not -- you might get hit.\n", ch);
		return;
	}
	
	if (victim->in_room 
	&& (IS_SET(victim->in_room->room_flags, ROOM_BATTLE_ARENA)
	|| IS_SET(victim->in_room->room_flags, ROOM_SAFE))) {
		act("You're such a cad.\n",
			ch, NULL, victim, TO_CHAR);
		act("You find $n's hands in your pocket!\n",
			ch, NULL, victim, TO_VICT);
		return;
	}

	if (!IS_IMMORTAL(ch) && 
	!in_PK(ch, victim) && ch->level > victim->level) {
		act("That mark isn't worth your time.",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	if ((obj = get_obj_list(ch, arg1, ch->carrying)) == NULL) {
		act("You don't have that item.",
			ch, NULL, victim, TO_CHAR);
		return;
	}
	if (arg3[0] != '\0') {
		if ((container = get_obj_list(ch, arg3, victim->carrying)) == NULL) {
			act("$N doesn't have that item.",
				ch, NULL, victim, TO_CHAR);
			return;
		}
	}

	if (IS_IMMORTAL(victim) && ch->level < victim->level) {
		act("You'll be lucky if you don't get smote for that.",
			ch, NULL, NULL, TO_CHAR);
		act("$n tried to plant $p on you!",
			ch, obj, victim, TO_VICT);
		return;
	}

	if (obj->wear_loc != WEAR_NONE) {
		act("You must first stop using $p.",
			ch, obj, victim, TO_CHAR);
		return;
	}

	if (container)
		sn = gsn_nestle;
	else
		sn = gsn_plant;

	if (get_skill(ch, sn) <= 0) {
		act("Stop playing around and just give it to $H.",
			ch, obj, victim, TO_CHAR);
		return;
	}

	if (!IS_IMMORTAL(ch) 
	&& container
	&& IS_SET(container->value[ITEM_CONTAINER_FLAGS], CONT_LOCKED)) {
		act("$p is locked.",
			ch, container, victim, TO_CHAR);
		return;
	}

	if (!IS_IMMORTAL(victim) && !in_PK(ch, victim)) {
		add_pk_ok(victim, ch);
	}

	chance = chance_skill = (get_skill(ch, sn = gsn_steal) * 50/100);
	chance += chance_dex = (get_curr_stat(ch,STAT_DEX) - 20) * 5;

	chance += chance_lvl = (LEVEL(ch) - LEVEL(victim))/5;

	chance += (get_eq_char(ch, WEAR_WIELD) == NULL) ? 5 : 0;

	chance += chance_vig = vigilance_modifier(ch, victim);

	chance += chance_weight = -1 * get_obj_weight(obj) /10;

	if (container) {
		clutter = obj_in_context_count(container, FALSE);
		chance += chance_clutter = URANGE(-10, (clutter - 5) * 2, 20);

		if (IS_SET(container->value[ITEM_CONTAINER_FLAGS], CONT_CLOSED))
			chance -= 10;
	}


	DEBUG(DEBUG_SKILL_STEAL,
		"%s plant %s: skill:%d lvl:%+d dex:%+d"
		" vig:%+d weight:%+d clut:%+d = %d",
		mlstr_mval(ch->short_descr),
		mlstr_mval(victim->short_descr),
		chance_skill,
		chance_lvl,
		chance_dex,
		chance_vig,
		chance_weight,
		chance_clutter,
		chance);

	WAIT_STATE(ch, SKILL(sn)->beats);

	if (!tax_move(ch, SKILL(sn)->move_cost)) {
		char_puts("You're too tired for that right now.\n", ch);
		return;
	}

	if (chance <= 0) {
		act("Don't think you got the chops to pull that off.",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	chance = UMIN(99, chance);

	if ((!IS_NPC(ch) && number_percent() > chance)
	||  IS_SET(victim->imm_flags, IMM_STEAL)) {
		check_improve(ch, sn, FALSE, 4);
		steal_failure(ch, victim, NULL, obj);
		success = FALSE;
	}
	else {
		success = TRUE;
		obj_from_char(obj);
		obj_to_char(obj, victim);
		act("You plant $p on $N.",
			ch, obj, victim, TO_CHAR);
		check_improve(ch, sn, TRUE, 3);
	}
	wiznet_theft(ch, victim, container, obj, THEFT_PLANT, 
			chance, success, obj->cost);
}

/*
 * new version of do_pick()
 * by Zsuzsu
 */
void do_picklock(CHAR_DATA *ch, const char *argument)
{
	return;

#ifdef ___NULL_ME_OUT
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *gch;
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	int door = -1;
	int chance;
	int sn = 0;


	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0') {
		act("You pick your nose.",
			ch, NULL, NULL, TO_CHAR);
		act("$n picks $s nose.",
			ch, NULL, NULL, TO_ROOM);
		return;
	}

	if (arg2[0] != '\0') {
		picklock_obj_worn(ch, arg1, arg2);
	}

	obj = get_obj_here(ch, arg1);
	door = find_door(ch, arg1);

	if (obj == NULL && door < 0) {
		char_puts("Hmm, don't see that here.\n", ch);
		return;
	}

	if (obj != NULL && door >= 0
	&& obj->pIndexData->item_type != ITEM_PORTAL
	&& obj->pIndexData->item_type != ITEM_CONTAINER) {
		DEBUG(DEBUG_SKILL_LOCKPICK,
			"%s lockpicking %s, assuming door instead of %s",
			mlstr_mval(ch->short_descr),
			arg1,
			mlstr_mval(obj->short_descr));
		obj = NULL;
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

	if ((obj = get_obj_here(ch, arg1)) != NULL) {
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

		    if (obj->value[ITEM_PORTAL_UNKNOWN4] < 0) {
			char_puts("It can't be unlocked.\n", ch);
			return;
		    }

		    if (IS_SET(obj->value[ITEM_PORTAL_EXIT_FLAGS],
		    	EX_PICKPROOF)) {
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
		if (obj->value[ITEM_CONTAINER_KEY_] < 0)
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

	if ((door = find_door(ch, arg1)) >= 0) {
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
#endif
}

bool picklock_obj_worn(CHAR_DATA *ch, const char *arg1, const char *arg2)
{
/*
	CHAR_DATA *gch;
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	int chance;
	int sn = 0;
*/

	return FALSE;
}
