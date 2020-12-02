/*
 *  $Id: fight.c 1021 2007-02-15 01:16:06Z zsuzsu $
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
 G*  In order to use any part of this Merc Diku Mud, you must comply with   *
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
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#if !defined (WIN32)
#	include <unistd.h>
#endif

#include "merc.h"
#include "debug.h"
#include "quest.h"
#include "fight.h"
#include "rating.h"
#include "update.h"
#include "mob_prog.h"
#include "obj_prog.h"
#include "waffects.h"
#include "augment.h"
#include "stats.h"

DECLARE_DO_FUN(do_crush		);
DECLARE_DO_FUN(do_emote		);
DECLARE_DO_FUN(do_dismount	);
DECLARE_DO_FUN(do_bash		);
DECLARE_DO_FUN(do_berserk	);
DECLARE_DO_FUN(do_disarm	);
DECLARE_DO_FUN(do_kick		);
DECLARE_DO_FUN(do_dirt		);
DECLARE_DO_FUN(do_trip		);
DECLARE_DO_FUN(do_tail		);
DECLARE_DO_FUN(do_look_in	);
DECLARE_DO_FUN(do_get		);
DECLARE_DO_FUN(do_sacrifice	);
DECLARE_DO_FUN(do_visible	);
DECLARE_DO_FUN(do_recall	);
DECLARE_DO_FUN(do_flee		);
DECLARE_DO_FUN(do_clan		);
DECLARE_DO_FUN(do_enslave	);
DECLARE_DO_FUN(do_death_announce);

DECLARE_SPEC_FUN(spec_breath_any        );
DECLARE_SPEC_FUN(spec_breath_acid       );
DECLARE_SPEC_FUN(spec_breath_fire       );
DECLARE_SPEC_FUN(spec_breath_frost      );
DECLARE_SPEC_FUN(spec_breath_gas        );
DECLARE_SPEC_FUN(spec_breath_lightning  );
DECLARE_SPEC_FUN(spec_cast_cleric       );
DECLARE_SPEC_FUN(spec_cast_judge        );
DECLARE_SPEC_FUN(spec_cast_mage         );

/*
 * Local functions.
 */
void	check_assist		(CHAR_DATA *ch, CHAR_DATA *victim);
bool	check_dodge		(CHAR_DATA *ch, CHAR_DATA *victim);
bool	check_parry		(CHAR_DATA *ch, CHAR_DATA *victim, int loc);
bool    check_haft_block        (CHAR_DATA *ch, CHAR_DATA *victim, int loc);
bool 	check_tumble		(CHAR_DATA *ch, CHAR_DATA *victim);
bool	check_shield_block	(CHAR_DATA *ch, CHAR_DATA *victim, int loc);
bool	check_blink		(CHAR_DATA *ch, CHAR_DATA *victim);
bool    check_space_between_beats      (CHAR_DATA *ch, CHAR_DATA *victim);
bool	check_hand_block	(CHAR_DATA *ch, CHAR_DATA *victim);
void	dam_message		(CHAR_DATA *ch, CHAR_DATA *victim, int dam,
				 int dt, bool immune, int dam_type);
void	death_cry		(CHAR_DATA *ch);
void	death_cry_org		(CHAR_DATA *ch, int part);
void	group_gain		(CHAR_DATA *ch, CHAR_DATA *victim);
int	xp_compute		(CHAR_DATA *gch, CHAR_DATA *victim,
				 int total_levels, int members, int morale);
bool	is_safe 		(CHAR_DATA *ch, CHAR_DATA *victim);

OBJ_DATA *make_corpse		(CHAR_DATA *ch);
void	strip_char 		(CHAR_DATA *ch);
void	one_hit 		(CHAR_DATA *ch, CHAR_DATA *victim, int dt,
				 int loc);
void	mob_hit 		(CHAR_DATA *ch, CHAR_DATA *victim, int dt);
void	set_fighting		(CHAR_DATA *ch, CHAR_DATA *victim);
void	disarm			(CHAR_DATA *ch, CHAR_DATA *victim,
				 int disarm_second);
int	critical_strike		(CHAR_DATA *ch, CHAR_DATA *victim, int dam);
void	check_eq_damage		(CHAR_DATA *ch, CHAR_DATA *victim, int loc);
void	check_shield_damage	(CHAR_DATA *ch, CHAR_DATA *victim, int loc);
void	check_weapon_damage	(CHAR_DATA *ch, CHAR_DATA *victim, int loc);
int 	check_forest		(CHAR_DATA *ch);
void	handle_witch_curse	(CHAR_DATA *ch, CHAR_DATA *victim);
bool	handle_enslavement	(CHAR_DATA *ch, CHAR_DATA *victim);
bool	handle_dishonor		(CHAR_DATA *ch, CHAR_DATA *victim);
void	raw_duel_defeat		(CHAR_DATA *ch, CHAR_DATA *victim);

#define FOREST_ATTACK 1
#define FOREST_DEFENCE 2
#define FOREST_NONE 0

void	handle_death		(CHAR_DATA *ch, CHAR_DATA *victim);

/*
 * Gets all money from the corpse.
 */
void get_gold_corpse(CHAR_DATA *ch, OBJ_DATA *corpse)
{
	OBJ_DATA *tmp, *tmp_next;

	for (tmp = corpse->contains; tmp; tmp = tmp_next) {
		tmp_next = tmp->next_content;
		if (tmp->pIndexData->item_type == ITEM_MONEY) {
			get_obj(ch, tmp, corpse);
		}
	}
}

int check_forest(CHAR_DATA* ch)
{
	AFFECT_DATA* paf;

	if (ch->in_room->sector_type != SECT_FOREST
	&& ch->in_room->sector_type != SECT_HILLS
	&& ch->in_room->sector_type != SECT_MOUNTAIN) 
		return FOREST_NONE;
	for (paf = ch->affected; paf; paf = paf->next) {
		if (paf->type == gsn_forest_fighting) {
			if (paf->location == APPLY_AC) 
				return FOREST_DEFENCE;
			else 
				return FOREST_ATTACK;
		}
	}
	return FOREST_NONE;
}



/*
 * Control the fights going on.
 * Called periodically by update_handler.
 */
void violence_update(void)
{
	CHAR_DATA *ch;
	CHAR_DATA *ch_next;
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;

	for (ch = char_list; ch; ch = ch_next) {
		ch_next = ch->next;

		/* decrement the wait */
		if (ch->desc == NULL)
			ch->wait = UMAX(0, ch->wait - PULSE_VIOLENCE);

		if ((victim = ch->fighting) == NULL || ch->in_room == NULL)
			continue;

		if (IS_AWAKE(ch) && ch->in_room == victim->in_room)
			multi_hit(ch, victim, TYPE_UNDEFINED);
		else
			stop_fighting(ch, FALSE);

		if ((victim = ch->fighting) == NULL)
			continue;

		if (!IS_NPC(victim))
			ch->last_fought = victim;

		SET_FIGHT_TIME(ch);

		if (victim->in_room != ch->in_room)
			continue;

		for (obj = ch->carrying; obj; obj = obj_next) {
			obj_next = obj->next_content;
			if (ch->fighting == NULL)
				break;
			oprog_call(OPROG_FIGHT, obj, ch, NULL);
		}

		if ((victim = ch->fighting) == NULL
		||  victim->in_room != ch->in_room)
			continue;

		/*
		 * Fun for the whole family!
		 */
		check_assist(ch, victim);
		if (IS_NPC(ch)) {
			if (HAS_TRIGGER(ch, TRIG_FIGHT))
				mp_percent_trigger(ch, victim, NULL, NULL,
						   TRIG_FIGHT);
			if (HAS_TRIGGER(ch, TRIG_HPCNT))
				mp_hprct_trigger(ch, victim);
		}
	}
}

/* for auto assisting */
void check_assist(CHAR_DATA *ch,CHAR_DATA *victim)
{
	CHAR_DATA *rch, *rch_next;

	if (!victim || IS_DELETED(victim) 
	|| !ch || IS_DELETED(ch))
		return;

	for (rch = ch->in_room->people; rch != NULL; rch = rch_next) {
		rch_next = rch->next_in_room;

		if (IS_AWAKE(rch) && rch->fighting == NULL) {
		    /* quick check for ASSIST_PLAYER */
		    if (!IS_NPC(ch) && IS_NPC(rch)
		    &&  IS_SET(rch->pIndexData->off_flags, ASSIST_PLAYERS)
		    &&	rch->level + 6 > victim->level) {
			do_emote(rch, "screams and attacks!");
			multi_hit(rch,victim,TYPE_UNDEFINED);
			continue;
		    }

		    /* PCs next */
		    if (!IS_NPC(rch) || IS_AFFECTED(rch, AFF_CHARM)) {
			if (((!IS_NPC(rch) &&
			      IS_SET(rch->conf_flags, PLR_CONF_AUTOASSIST)) ||
			     IS_AFFECTED(rch, AFF_CHARM))
			&&  is_same_group(ch,rch)
			&&  !is_safe_nomessage(rch, victim)
			&&  !is_affected(rch, gsn_rnet_trap))
			    multi_hit (rch,victim,TYPE_UNDEFINED);
				continue;
		    }

		    if (!IS_NPC(ch) && RIDDEN(rch) == ch)
		    {
			multi_hit(rch,victim,TYPE_UNDEFINED);
			continue;
		    }

		    /* now check the NPC cases */

		    if (IS_NPC(ch)) {
			if ((IS_NPC(rch) && IS_SET(rch->pIndexData->off_flags,ASSIST_ALL))
			||   (IS_NPC(rch) && rch->race == ch->race
			   && IS_SET(rch->pIndexData->off_flags,ASSIST_RACE))
			||   (IS_NPC(rch) && IS_SET(rch->pIndexData->off_flags,ASSIST_ALIGN)
			   &&	((IS_GOOD(rch)	  && IS_GOOD(ch))
			     ||  (IS_EVIL(rch)	  && IS_EVIL(ch))
			     ||  (IS_NEUTRAL(rch) && IS_NEUTRAL(ch))))
			||   (rch->pIndexData == ch->pIndexData
			   && IS_SET(rch->pIndexData->off_flags,ASSIST_VNUM))) {
			    CHAR_DATA *vch;
			    CHAR_DATA *target;
			    int number;

			    if (number_bits(1) == 0)
				continue;

			    target = NULL;
			    number = 0;

			    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
			    {
				if (can_see(rch,vch)
				&&  is_same_group(vch,victim)
				&&  number_range(0,number) == 0)
				{
				    target = vch;
				    number++;
				}
			    }

			    if (target != NULL)
			    {
				do_emote(rch,"screams and attacks!");
				multi_hit(rch,target,TYPE_UNDEFINED);
			    }
			}
		    }
		}
	}
}


/*
 * Do one group of attacks.
 */
void multi_hit(CHAR_DATA *ch, CHAR_DATA *victim, int dt)
{
	int     chance;
	int     staff = 0;
	int     dualstaff = 0;

	if (IS_DELETED(victim))
		return;

	/* no attacks for stunnies -- just a check */
	if (ch->position < POS_RESTING)
		return;

#if 0
	/* become CRIMINAL in Law rooms */
	if (!IS_NPC(ch) && !IS_NPC(victim)
	&& IS_SET(ch->in_room->room_flags, ROOM_LAW)
	&& !IS_SET(victim->state_flags, STATE_WANTED)
	&& !IS_SET(ch->state_flags, STATE_WANTED)) {
		char_puts("This room is under supervision of the law! "
			  "Now you're {RCRIMINAL{x!\n", ch);
		SET_BIT(ch->state_flags, STATE_WANTED);		
	}
#endif

	/* ridden's adjustment */
	if (RIDDEN(victim) && !IS_NPC(victim->mount)) {
		if (victim->mount->fighting == NULL
		|| victim->mount->fighting == ch)
			victim = victim->mount;
		else
			do_dismount(victim->mount, str_empty);
	}

	if (IS_AFFECTED(ch,AFF_WEAK_STUN)) {
		act_puts("You are too stunned to respond $N's attack.",
			 ch, NULL, victim, TO_CHAR, POS_FIGHTING);
		act_puts("$n is too stunned to respond your attack.",
			 ch, NULL, victim, TO_VICT, POS_FIGHTING);
		REMOVE_BIT(ch->affected_by, AFF_WEAK_STUN);
		return;
	}

	if (IS_AFFECTED(ch,AFF_STUN)) {
		act_puts("You are too stunned to respond $N's attack.",
			 ch, NULL, victim, TO_CHAR, POS_FIGHTING);
		act_puts("$n is too stunned to respond your attack.",
			 ch, NULL, victim, TO_VICT, POS_FIGHTING);
		act_puts("$n seems to be stunned.",
			 ch, NULL, victim, TO_NOTVICT, POS_FIGHTING);
		REMOVE_BIT(ch->affected_by, AFF_STUN);
		affect_bit_strip(ch, TO_AFFECTS, AFF_STUN);
		SET_BIT(ch->affected_by, AFF_WEAK_STUN);
		return;
	}

	if (IS_NPC(ch)) {
		mob_hit(ch, victim, dt);
		return;
	}

	one_hit(ch, victim, dt, WEAR_WIELD);

	if (ch->fighting != victim)
		return;

	if ((chance = get_skill(ch, gsn_area_attack))
	&&  number_percent() < chance) {
		int count = 0, max_count;
		CHAR_DATA *vch, *vch_next;

		check_improve(ch, gsn_area_attack, TRUE, 6);

		if (LEVEL(ch) < 70)
			max_count = 1;
		else if (LEVEL(ch) < 80)
			max_count = 2;
		else if (LEVEL(ch) < 90)
			max_count = 3;
		else
			max_count = 4;

		for (vch = ch->in_room->people; vch; vch = vch_next) {
			vch_next = vch->next_in_room;
			if (vch != victim && vch->fighting == ch) {
				one_hit(ch, vch, dt, WEAR_WIELD);
				if (++count == max_count)
					break;
			}
		}
	}

	if (IS_AFFECTED(ch, AFF_HASTE))
		one_hit(ch, victim, dt, WEAR_WIELD);

	if (ch->fighting != victim || dt == gsn_backstab || dt == gsn_cleave
	|| dt == gsn_ambush || dt == gsn_dual_backstab || dt == gsn_circle
	|| dt == gsn_assassinate || dt == gsn_vampiric_bite || dt == gsn_knife)
		return;


	if (get_eq_char(ch, WEAR_WIELD) 
	 && get_eq_char(ch, WEAR_WIELD)->value[ITEM_WEAPON_TYPE] == WEAPON_STAFF)
		staff = 4;

	if (get_eq_char(ch, WEAR_SECOND_WIELD)
	 && get_eq_char(ch, WEAR_SECOND_WIELD)->value[ITEM_WEAPON_TYPE] == WEAPON_STAFF)
		dualstaff = 4;

	chance = get_skill(ch, gsn_second_attack) / 2;
	chance += staff;

	if (number_percent() < chance) {
		one_hit(ch, victim, dt, WEAR_WIELD);
		check_improve(ch, gsn_second_attack, TRUE, 5);
		if (ch->fighting != victim)
			return;
	}

	chance = get_skill(ch,gsn_third_attack)/3;
	chance += staff;
	if (number_percent() < chance) {
		one_hit(ch, victim, dt, WEAR_WIELD);
		check_improve(ch, gsn_third_attack, TRUE, 6);
		if (ch->fighting != victim)
			return;
	}


	chance = get_skill(ch,gsn_fourth_attack)/6;
	chance += staff;
	if (number_percent() < chance) {
		one_hit(ch, victim, dt, WEAR_WIELD);
		check_improve(ch, gsn_fourth_attack, TRUE, 7);
		if (ch->fighting != victim)
			return;
	}

	chance = get_skill(ch,gsn_fifth_attack)/8;
	chance += staff;
	if (number_percent() < chance) {
		one_hit(ch, victim, dt, WEAR_WIELD);
		check_improve(ch,gsn_fifth_attack,TRUE,8);
		if (ch->fighting != victim)
		    return;
	}

	if (check_forest(ch) == FOREST_ATTACK) {
		chance = get_skill(ch, gsn_forest_fighting);
		while (number_percent() < chance) {
			one_hit(ch, victim, dt, WEAR_WIELD);
			check_improve (ch, gsn_forest_fighting, TRUE, 8);
			if (ch->fighting != victim)
				return;
			chance /= 3;
		}
	}
		

	chance = get_skill(ch, gsn_second_weapon) / 2;
	chance += dualstaff;
	if (number_percent() < chance)
		if (get_eq_char(ch, WEAR_SECOND_WIELD)) {
			one_hit(ch, victim, dt, WEAR_SECOND_WIELD);
			check_improve(ch, gsn_second_weapon, TRUE, 2);
			if (ch->fighting != victim)
				return;
		}

	chance = get_skill(ch,gsn_secondary_attack) / 8;
	chance += dualstaff;
	if (number_percent() < chance)
		if (get_eq_char(ch, WEAR_SECOND_WIELD)) {
			one_hit(ch, victim, dt, WEAR_SECOND_WIELD);
			check_improve(ch, gsn_secondary_attack, TRUE, 2);
			if (ch->fighting != victim)
				return;
		}
}

/* procedure for all mobile attacks */
void mob_hit(CHAR_DATA *ch, CHAR_DATA *victim, int dt)
{
	CHAR_DATA *vch, *vch_next;
	flag64_t act = ch->pIndexData->act;
	flag64_t off = ch->pIndexData->off_flags;

	/* no attack by ridden mobiles except spec_casts */
	if (RIDDEN(ch)) {
		if (ch->fighting != victim) {
			stop_fighting(ch, FALSE);
			set_fighting(ch, victim);
		}
		return;
	}

	one_hit(ch, victim, dt, WEAR_WIELD);

	if (ch->fighting != victim)
		return;

	/* Area attack -- BALLS nasty! */

	if (IS_SET(off, OFF_AREA_ATTACK)) {
		for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
			vch_next = vch->next_in_room;
			if ((vch != victim && vch->fighting == ch))
				one_hit(ch, vch, dt, WEAR_WIELD);
		}
	}

	if (IS_AFFECTED(ch, AFF_HASTE) || IS_SET(off, OFF_FAST))
		one_hit(ch, victim, dt, WEAR_WIELD);

	if (ch->fighting != victim || dt == gsn_backstab || dt == gsn_circle ||
		dt == gsn_dual_backstab || dt == gsn_cleave || dt == gsn_ambush
			|| dt == gsn_vampiric_bite || dt == gsn_knife)
		return;

	if (number_percent() < get_skill(ch, gsn_second_attack) / 2) {
		one_hit(ch, victim, dt, WEAR_WIELD);
		if (ch->fighting != victim)
			return;
	}

	if (number_percent() < get_skill(ch, gsn_third_attack) / 4) {
		one_hit(ch, victim, dt, WEAR_WIELD);
		if (ch->fighting != victim)
			return;
	}

	if (number_percent() < get_skill(ch, gsn_fourth_attack) / 6) {
		one_hit(ch, victim, dt, WEAR_WIELD);
		if (ch->fighting != victim)
			return;
	}

	/* PC waits */

	if (ch->wait > 0)
		return;

#if 0
	switch (number_range(0, 2)) {
	case 1:
		if (IS_SET(act, ACT_MAGE)) {
			mob_cast_mage(ch, victim);
			return;
		}
		break;
	case 2:
		if (IS_SET(act, ACT_CLERIC)) {
			mob_cast_cleric(ch, victim);
			return;
		}
		break;
	}
#endif

	/* now for the skills */

	switch (number_range(0, 7)) {
	case 0:
		if (IS_SET(off, OFF_BASH))
			do_bash(ch, str_empty);
		break;

	case 1:
		if (IS_SET(off, OFF_BERSERK)
		&&  !IS_AFFECTED(ch, AFF_BERSERK))
			do_berserk(ch, str_empty);
		break;


	case 2:
		if (IS_SET(off, OFF_DISARM)
		||  IS_SET(act, ACT_WARRIOR | ACT_THIEF)) {
			if (number_range(0, 1)
			&&  get_eq_char(victim, WEAR_SECOND_WIELD))
				do_disarm(ch, "second");
			else if (get_eq_char(victim, WEAR_WIELD))
				do_disarm(ch, str_empty);
		}
		break;

	case 3:
		if (IS_SET(off, OFF_KICK))
			do_kick(ch, str_empty);
		break;

	case 4:
		if (IS_SET(off, OFF_DIRT_KICK))
			do_dirt(ch, str_empty);
		break;

	case 5:
		if (IS_SET(off, OFF_TAIL))
			do_tail(ch, str_empty);
		break;

	case 6:
		if (IS_SET(off, OFF_TRIP))
			do_trip(ch, str_empty);
		break;
	case 7:
		if (IS_SET(off, OFF_CRUSH))
			do_crush(ch, str_empty);
		break;
	}
}

/**
 * ac reduces the amount of damage contributed by damroll
 */
int dam_damroll_vs_ac(CHAR_DATA *ch, CHAR_DATA *victim, 
		OBJ_DATA *wield, int bonus) {
	int dam = 0;
	int sk = 0;
	int sn = 0;
	int ac = 0;
	int dam_type = (wield) 
		? attack_table[wield->value[ITEM_WEAPON_ATTACK_TYPE]].damage
		: attack_table[ch->dam_type].damage;

	switch(dam_type) {
	case DAM_PIERCE:ac = GET_AC(victim,AC_PIERCE);	break;
	case DAM_BASH:  ac = GET_AC(victim,AC_BASH); 	break;
	case DAM_SLASH: ac = GET_AC(victim,AC_SLASH);	break;
	default:	ac = GET_AC(victim,AC_EXOTIC); 	break;
	}

	sn = get_weapon_sn(wield);
	sk = get_weapon_skill(ch, sn);

	ac = UMIN(0, ac);

	dam = (GET_DAMROLL(ch) * UMAX(100, sk)/100) + bonus + ac;

	DEBUG(DEBUG_DAM_AC_REDUCT,
		"%s[dr: %d/sk:%d] vs %s[ac:%d] = %d dam_mod",
		CHAR_NAME(ch),
		GET_DAMROLL(ch),
		sk,
		CHAR_NAME(victim),
		ac,
		dam);

	return dam;
}

int get_dam_type(CHAR_DATA *ch, OBJ_DATA *wield, int *dt)
{
	int dam_type;

	if (*dt == TYPE_UNDEFINED) {
		*dt = TYPE_HIT;
		if (wield &&  wield->pIndexData->item_type == ITEM_WEAPON)
			*dt += wield->value[ITEM_WEAPON_ATTACK_TYPE];
		else
			*dt += ch->dam_type;
	}

	if (*dt < TYPE_HIT)
		if (wield)
			dam_type = attack_table[wield->value[ITEM_WEAPON_ATTACK_TYPE]].damage;
		else
			dam_type = attack_table[ch->dam_type].damage;
	else
		dam_type = attack_table[*dt - TYPE_HIT].damage;

	if (dam_type == TYPE_UNDEFINED)
		dam_type = DAM_BASH;

	return dam_type;
}

#define DAM_BASE  		0
#define DAM_SHARP 		1
#define DAM_NOSHIELD		2
#define DAM_ENHANCED		3
#define DAM_MASTER_HAND		4
#define DAM_MASTERING_SWORD	5
#define DAM_DOUBLE_GRIP		6
#define DAM_SLEEPING		7
#define DAM_NOT_FIGHTING	8
#define DAM_BACKSTAB		9
#define DAM_DUAL_BACKSTAB	10
#define DAM_DEATHBLOW		11	
#define DAM_CLEAVE		12
#define DAM_AMBUSH		13
#define DAM_ASSASSINATE		14
#define DAM_CHARGE		15
#define DAM_CIRCLE		16
#define DAM_KNIFE		17
#define DAM_DRvAC_REAL		18
#define DAM_DRvAC_MOD		19

#define DAM_HIT_MAX		20

/* sum the damage up */
int damage_total(int* dam) {
	int sum = 0;
	int i;
	for (i = 0; i < DAM_HIT_MAX; i++)
		if (i != DAM_DRvAC_REAL)
			sum += dam[i];
	return sum;
}
/*
 * Hit one guy once.
 */
void one_hit(CHAR_DATA *ch, CHAR_DATA *victim, int dt, int loc)
{
	OBJ_DATA *wield;
	OBJ_DATA *dual = NULL;
	int dam2;
	int dam[DAM_HIT_MAX];
	bool bonus[DAM_HIT_MAX];
	bool modify_base = TRUE;
	int diceroll;
	int sn = 0, sk = 0, sk2 = 0;
	int dam_type;
	bool counter;
	bool result;
	int sercount;
	int dam_flags;
	class_t *cl;	
	bool deathblow = FALSE;
	int chance = 0;
	int i;

	for (i = 0; i < DAM_HIT_MAX; i++) {
		dam[i] = 0;
		bonus[i] = TRUE;
	}

	sn = -1;
	counter = FALSE;

	if (IS_DELETED(victim) || IS_DELETED(ch))
		return;

	/* just in case */
	if (victim == ch || ch == NULL || victim == NULL)
		return;

	/*
	 * Can't beat a dead char!
	 * Guard against weird room-leavings.
	 */
	if (victim->position == POS_DEAD || ch->in_room != victim->in_room)
		return;

	/*
	 * Figure out the type of damage message.
	 */
	wield = get_eq_char(ch, loc);
	dam_flags = DAMF_SHOW;
	if (loc == WEAR_SECOND_WIELD)
		dam_flags |= DAMF_SECOND;
	dam_type = get_dam_type(ch, wield, &dt);

	dual = get_eq_char(ch, WEAR_SECOND_WIELD);
	
	/*
	 * don't double backstab if the second weapon isn't peircing
	 */
	if (dt == gsn_dual_backstab 
	&& !IS_NPC(ch)
	&& (!dual 
	|| attack_table[dual->value[ITEM_WEAPON_ATTACK_TYPE]].damage != DAM_PIERCE)) {
		return;
	}

	/* get the weapon skill */
	sn = get_weapon_sn(wield);
	sk = 20 + get_weapon_skill(ch, sn);

	if (get_skill(victim, gsn_armor_use) > 70) {
		check_improve(victim, gsn_armor_use, TRUE, 8);
	}

	if (!can_see(ch, victim)) {
		 if ((sk2 = get_skill(ch, gsn_blind_fighting))
		 &&  number_percent() < sk2)
			check_improve(ch,gsn_blind_fighting,TRUE,16);
	}

	/*
	* The moment of excitement!
	*/
	while ((diceroll = number_bits(5)) >= 20)
	;

	chance = tohit_chance(ch, victim, dt, loc);

	if (diceroll == 0
	|| (diceroll != 19 && diceroll < chance)) {
		/* Miss. */
		damage(ch, victim, 0, dt, dam_type, dam_flags);
		tail_chain();
		return;
	}

	/* special skill hits first */

	/* vampiric bite *************************************************/
	/* doesn't use weapon for damage */
	if (dt == gsn_vampiric_bite) {
		sk = get_skill(ch, gsn_vampiric_bite);
		dam[DAM_BASE] = 2 * LEVEL(ch) * sk / 100;
		if (IS_NPC(victim))
			dam[DAM_BASE] = (LEVEL(ch)/11 + 1) * dam[DAM_BASE] + LEVEL(ch);
		else
			dam[DAM_BASE] = (LEVEL(ch)/20 + 1) * dam[DAM_BASE] + LEVEL(ch);

		DEBUG(DEBUG_DAM_ONEHIT,
			"%s vbite base %d w/weapon %d",
			CHAR_NAME(ch),
			dam[DAM_BASE],
			(wield) ? dice(wield->value[ITEM_WEAPON_DICE_NUM], 
				wield->value[ITEM_WEAPON_DICE_SIZE]) : 0
			);
		bonus[DAM_ENHANCED] = bonus[DAM_DEATHBLOW] 
		= bonus[DAM_DOUBLE_GRIP] = FALSE;
		modify_base = FALSE;
	}

	/* dual backstab *************************************************/
	else if ((dt == gsn_dual_backstab) && dual) {
		sk2 = 20 + get_weapon_skill(ch, get_weapon_sn(dual));
		dam[DAM_BASE] 
			= dice(dual->value[ITEM_WEAPON_DICE_NUM],
			dual->value[ITEM_WEAPON_DICE_SIZE]) 
			* sk2 / 100;
		bonus[DAM_ENHANCED] = bonus[DAM_DEATHBLOW] 
		= bonus[DAM_NOSHIELD] = bonus[DAM_DOUBLE_GRIP] = FALSE;
		modify_base = FALSE;
	}

	/* normal weapon hit *********************************************/
	else if (wield != NULL) {
		if (sn != -1)
			check_improve(ch, sn, TRUE, 5);
		dam[DAM_BASE] 
			= dice(wield->value[ITEM_WEAPON_DICE_NUM],
			wield->value[ITEM_WEAPON_DICE_SIZE]) 
			* sk / 100;
		/* make sure putting a nerfed weapon in a mob's 
		 * hands doesn't nerf it */
		if (IS_NPC(ch)) {
			dam2 = dice(ch->damage[DICE_NUMBER], 
				ch->damage[DICE_TYPE]);
			dam[DAM_BASE] = 
				(dam[DAM_BASE] > dam2) 
				? dam[DAM_BASE] : dam2;
		}
	}

	/* NPCs w/o weapons **********************************************/
	else if (IS_NPC(ch)) {
		dam[DAM_BASE] = dice(ch->damage[DICE_NUMBER], 
			ch->damage[DICE_TYPE]);
	}
	/* PC Hand damage *************************************************/
	else {
		dam[DAM_BASE] = number_range(1 + 4 * sk / 100,
			LEVEL(ch) * sk / 100);
		if ((sk2 = get_skill(ch, gsn_master_hand))
		&& number_percent() < sk2) {
			check_improve(ch, gsn_master_hand, TRUE, 6);
			dam[DAM_MASTER_HAND] = dice(LEVEL(ch) / 20, LEVEL(ch) * sk2 / 100);
		}
		bonus[DAM_NOSHIELD] = bonus[DAM_DOUBLE_GRIP] = FALSE;
	}

	if (modify_base)
		dam[DAM_BASE] = dam[DAM_BASE] * MELEE_BASE_DAMAGE_MODIFIER /100;

	/***********
	 * bonuses *
	 ***********/

	/* no shield = more ***********************************************/
	if (bonus[DAM_NOSHIELD]) {
		if (get_eq_char(ch, WEAR_SHIELD) == NULL)
			dam[DAM_NOSHIELD] 
				= dam[DAM_BASE] * 1/20;
	}

	/* sharpness! *****************************************************/
	if (bonus[DAM_SHARP] && wield 
	&& IS_WEAPON_STAT(wield, WEAPON_SHARP)) {
		int percent;

		if ((percent = number_percent()) <= (sk / 8))
			dam[DAM_SHARP] 
				= dam[DAM_BASE] 
				+ (dam[DAM_BASE] * 2 * percent / 100);
	}

	/* enhanced damage ************************************************/
	if (bonus[DAM_ENHANCED]
	&& (sk2 = get_skill(ch, gsn_enhanced_damage))
	&&  (diceroll = number_percent()) <= sk2) {
		dam[DAM_ENHANCED] = dam[DAM_BASE] * diceroll * sk2 / 10000;
		check_improve(ch, gsn_enhanced_damage, TRUE, 6);
		cl = class_lookup(ch->class);

		if (!(cl == NULL)
                && ( str_cmp(cl->name, "ninja")
                ||  str_cmp(cl->name, "thief")))
			dam[DAM_ENHANCED] /= 2;
	}

	/* bonus for double-gripping a one handed weapon *******************/
	if (bonus[DAM_DOUBLE_GRIP] && wield
	&& !IS_WEAPON_STAT(wield, WEAPON_TWO_HANDS)
	&& get_skill(ch, gsn_double_grip) > 0) {
		check_improve(ch, gsn_double_grip, TRUE, 100); /*rare*/
		dam[DAM_DOUBLE_GRIP] = dam[DAM_BASE] * 10/100;
	}

	/* mastering sword *************************************************/
	if (bonus[DAM_MASTERING_SWORD]
	&& (sn == gsn_katana
	|| sn == gsn_longsword
	|| sn == gsn_shortsword 
	|| sn == gsn_bastardsword)
	&&  (sk2 = get_skill(ch, gsn_mastering_sword))
	&&  number_percent() <= sk2) {
		OBJ_DATA *katana;
		check_improve(ch, gsn_mastering_sword, TRUE, 6);
		dam[DAM_MASTERING_SWORD] = dam[DAM_BASE] * 20/100;

		if (((katana = get_eq_char(ch,WEAR_WIELD)) ||
		     (katana = get_eq_char(ch, WEAR_SECOND_WIELD)))
		&&  IS_WEAPON_STAT(katana, WEAPON_KATANA_QUEST)
		&&  strstr(mlstr_mval(katana->ed->description), ch->name)) {

			if ((katana->cost = ++katana->cost % 250) == 0) {
				AFFECT_DATA *paf = NULL,
					    *af_toHit = NULL,
					    *af_toDam = NULL;
				int old_toHit = 0;
				int old_toDam = 0;

				paf = katana->affected;
				while (paf != NULL) {
					if (paf->type == gsn_katana)
						switch (paf->location) {
							case APPLY_HITROLL:
								af_toHit = paf;
								break;
							case APPLY_DAMROLL:
								af_toDam = paf;
								break;
							default:
								DEBUG(DEBUG_BUG,
									"bad affect on %s's katana location %d type: %d mod: %d",
									ch->name,
									paf->location,
									paf->type,
									paf->modifier);
						}
					paf = paf->next;
				}

				if (af_toHit && af_toDam) {
					old_toHit = af_toHit->modifier;
					old_toDam = af_toDam->modifier;

					af_toHit->modifier = UMIN(af_toHit->modifier+1,
							     ch->level / 3);
					af_toDam->modifier = UMIN(af_toDam->modifier+1,
							     ch->level / 3);

					ch->hitroll += af_toHit->modifier - old_toHit;
					ch->damroll += af_toDam->modifier - old_toDam;

					act("$n's katana glows {Bblue{x.\n",
					    ch, NULL, NULL, TO_ROOM);
					char_puts("Your katana glows {Bblue{x.\n",ch);
				}
				else {
					bug("quest katana - couldn't find toHit or toDam",0);
				}
			}
		}
	}

	/* sleeping victim **************************************************/
	if (bonus[DAM_SLEEPING] && !IS_AWAKE(victim))
		dam[DAM_SLEEPING] = damage_total(dam);
	
	/* not fighting *****************************************************/
	else if (bonus[DAM_NOT_FIGHTING] && victim->position < POS_FIGHTING)
		dam[DAM_NOT_FIGHTING] = damage_total(dam) /2;

	/* counter **********************************************************/
	sercount = number_percent();
	if (dt == gsn_backstab || dt == gsn_vampiric_bite)
		sercount += 40;
	if (!IS_IMMORTAL(ch) && IS_PUMPED(ch))
		sercount += 10;
	sercount *= 2;
	if (victim->fighting == NULL && !IS_NPC(victim)
	&&  !is_safe_nomessage(victim, ch)
	&&  !is_safe_nomessage(ch,victim)
	&&  (victim->position == POS_SITTING ||
	     victim->position == POS_STANDING)
	&&  dt != gsn_assassinate
	&&  (sercount <= get_skill(victim, gsn_counter))) {
		counter = TRUE;
		check_improve(victim,gsn_counter,TRUE,1);
		act("$N turns your attack against you!",
		    ch, NULL, victim, TO_CHAR);
		act("You turn $n's attack against $m!",
		    ch, NULL, victim, TO_VICT);
		act("$N turns $n's attack against $m!",
		    ch, NULL, victim, TO_NOTVICT);
		ch->fighting = victim;
	}
	else if (!victim->fighting)
		check_improve(victim, gsn_counter, FALSE, 1);

	/*************************************
	 * special weaponed attacks
	 *************************************/

	/* backstab ********************************************************/
	if (dt == gsn_backstab
	&& (IS_NPC(ch) || wield)) { /* is NPC don't need a wielded weapon*/
		if (IS_NPC(victim))
			dam[DAM_BACKSTAB] = damage_total(dam)
				* (LEVEL(ch) / 10 + 1);
		else
			dam[DAM_BACKSTAB] = damage_total(dam)
				* (LEVEL(ch) / 15 + 1);
	}

	/* dual backstab **************************************************/
	else if (dt == gsn_dual_backstab) {
		if (IS_NPC(victim))
			dam[DAM_DUAL_BACKSTAB] = damage_total(dam)
				* (LEVEL(ch) / 20);
		else
			dam[DAM_DUAL_BACKSTAB] = damage_total(dam)
				* (LEVEL(ch) / 30);

	}

	else if (dt == gsn_circle && wield) {
		dam[DAM_CIRCLE] = (LEVEL(ch)/30 + 1) * damage_total(dam);
	}

	else if (dt == gsn_knife && wield) {
		dam[DAM_KNIFE] = (LEVEL(ch)/25 + 1) * damage_total(dam);
	}

	/* ranger */
	else if (dt == gsn_ambush) {
		if (IS_NPC(victim))
			dam[DAM_AMBUSH] = (LEVEL(ch)/20) * damage_total(dam);
		else
			dam[DAM_AMBUSH] = (LEVEL(ch)/30) * damage_total(dam);
	}

	/* AP */
	else if (dt == gsn_cleave && wield) {
		if (IS_NPC(victim)
		&& number_percent() <
				(URANGE(4, 5+LEVEL(ch)-LEVEL(victim), 10)
				+ (wield->value[ITEM_WEAPON_TYPE]==WEAPON_AXE) 
				? 2:0 
				+ (get_curr_stat(ch,STAT_STR)-60)/8)
		&&  !counter && !IS_IMMORTAL(victim)) {
			act_puts("Your cleave chops $N IN HALF!",
				 ch, NULL, victim, TO_CHAR, POS_RESTING);
			act_puts("$n's cleave chops you IN HALF!",
				 ch, NULL, victim, TO_VICT, POS_RESTING);
			act_puts("$n's cleave chops $N IN HALF!",
				 ch, NULL, victim, TO_NOTVICT, POS_RESTING);
			WAIT_STATE(ch, 2);
			handle_death(ch, victim);
			return;
		}
		else
			dam[DAM_CLEAVE] = damage_total(dam);
	}

	/* ninja */
	else if (dt == gsn_assassinate) {
		if (IS_NPC(victim) 
		&& number_percent() <= URANGE(10, 20+(LEVEL(ch) - LEVEL(victim))*4, 50)
		&& !counter && !IS_IMMORTAL(victim)) {
			act_puts("You {R+++ASSASSINATE+++{x $N!",
				 ch, NULL, victim, TO_CHAR, POS_RESTING);
			act_puts("$n {R+++ASSASSINATES+++{x $N!",
				 ch, NULL, victim, TO_NOTVICT, POS_RESTING);
			act_puts("$n {R+++ASSASSINATES+++{x you!",
				 ch, NULL, victim, TO_VICT, POS_DEAD);
			check_improve(ch, gsn_assassinate, TRUE, 1);
			handle_death(ch, victim);
			return;
		}
		else {
			check_improve(ch, gsn_assassinate, FALSE, 1);
			dam[DAM_ASSASSINATE] = 2 * damage_total(dam);
		}
	}

	/* knight **********************************************************/
	else if (dt == gsn_charge) {
		dam[DAM_CHARGE] = damage_total(dam) * LEVEL(ch) / 12;
	}

	/* death blow ******************************************************/
	if (bonus[DAM_DEATHBLOW]
	&& (sk2 = get_skill(ch, gsn_deathblow)) > 1) {
		if (number_percent() <  (sk2/8)) {
			dam[DAM_DEATHBLOW] = dam[DAM_BASE] * 2/10;
			deathblow = TRUE;
			check_improve(ch, gsn_deathblow, TRUE, 10);
		}
		else
			check_improve(ch, gsn_deathblow, FALSE, 10);
	}

	dam[DAM_DRvAC_REAL] = dam_damroll_vs_ac(ch, victim, wield, 0);
	dam[DAM_DRvAC_MOD] = URANGE(-1/10 * damage_total(dam),
		dam[DAM_DRvAC_REAL],
		damage_total(dam) / 2);

	/* 
	 * damage from the hit *********************************************
	 */
	if (counter) {
		result = damage(ch, ch, 
			2*damage_total(dam), 
			dt, dam_type, DAMF_SHOW);
		multi_hit(victim, ch, TYPE_UNDEFINED);
	}
	else
		result = damage(ch, victim, damage_total(dam), dt, dam_type, dam_flags);

	DEBUG(DEBUG_MELEE, "%s[%d] %s %s[%d]: "
		"actual: %d tot: %d base: %d"
		" sharp: %d"
		" nosh: %d"
		" enha: %d"
		" mhan: %d"
		" mswd: %d"
		" slep: %d"
		" nfit: %d"
		" bs: %d"
		" dubs: %d"
		" dthb: %d"
		" clev: %d"
		" ambu: %d"
		" assa: %d"
		" chrg: %d"
		" circ: %d"
		" knif: %d"
		" dracR: %d"
		" dracM: %d",
		CHAR_NAME(ch),
		LEVEL(ch),
		(sn) ? skill_name(sn) : "melee",
		CHAR_NAME(victim),
		LEVEL(victim),
		result,
		damage_total(dam),
		dam[DAM_BASE],
		dam[DAM_SHARP],
		dam[DAM_NOSHIELD],
		dam[DAM_ENHANCED],
		dam[DAM_MASTER_HAND],
		dam[DAM_MASTERING_SWORD],
		dam[DAM_SLEEPING],
		dam[DAM_NOT_FIGHTING],
		dam[DAM_BACKSTAB],
		dam[DAM_DUAL_BACKSTAB],
		dam[DAM_DEATHBLOW],
		dam[DAM_CLEAVE],
		dam[DAM_AMBUSH],
		dam[DAM_ASSASSINATE],
		dam[DAM_CHARGE],
		dam[DAM_CIRCLE],
		dam[DAM_KNIFE],
		dam[DAM_DRvAC_REAL],
		dam[DAM_DRvAC_MOD]);

	/*
	 * extra damage
	 */

	if (result && deathblow) {
		act("You really got your weight behind that one!",
		    ch, NULL, NULL, TO_CHAR);
		act("$n's blow hits with extra force!",
		    ch, NULL, NULL, TO_ROOM);
	}


	/* vampiric bite gives hp to ch from victim */
	if (result && dt == gsn_vampiric_bite) {
		int hit_ga = UMIN((damage_total(dam) / 2), victim->max_hit);

		ch->hit += hit_ga;
		ch->hit  = UMIN(ch->hit, ch->max_hit);
		update_pos(ch);
		char_puts("Your health increases as you suck "
			  "your victim's blood.\n", ch);
	}

	/* but do we have a funky weapon? */
	if (result && wield != NULL && ch->fighting == victim) {
		int extra_dam;

		if (victim->hit > 0 && IS_WEAPON_STAT(wield, WEAPON_VORPAL)) {
			int chance;

			chance = get_skill(ch, get_weapon_sn(wield)) +
				 get_curr_stat(ch, STAT_STR);

			if (chance > number_range(1, 200000)
			&&  !IS_IMMORTAL(victim)) {
				act("$p makes an huge arc in the air, "
				    "chopping $n's head OFF!",
				     victim, wield, NULL, TO_ROOM);
				act("$p whistles in the air, "
				    "chopping your head OFF!",
				    victim, wield, NULL, TO_CHAR);
				handle_death(ch, victim);
				return;
			}
		}

		if (victim->hit > 0 && IS_WEAPON_STAT(wield, WEAPON_POISON)) {
			int level;
			AFFECT_DATA *poison, af;

			if ((poison = affect_find(wield->affected,
							gsn_poison)) == NULL)
				level = wield->level;
			else
				level = poison->level;

			if (!saves_spell(level / 2,victim, DAM_POISON)) {
				char_puts("You feel poison coursing "
					  "through your veins.", victim);
				act("$n is poisoned by the venom on $p.",
				    victim, wield, NULL, TO_ROOM);

				af.where     = TO_AFFECTS;
				af.type      = gsn_poison;
				af.level     = level * 3/4;
				af.duration  = level / 2;
				af.location  = APPLY_STR;
				af.modifier  = -1;
				af.bitvector = AFF_POISON;
				affect_join(victim, &af);
			}

			/* weaken the poison if it's temporary */
			if (poison != NULL) {
				poison->level = UMAX(0,poison->level - 2);
				poison->duration = UMAX(0,poison->duration - 1);
				if (poison->level == 0
				||  poison->duration == 0)
					act("The poison on $p has worn off.",
					    ch, wield, NULL, TO_CHAR);
			}
		}

		if (victim->hit > 0 && IS_WEAPON_STAT(wield, WEAPON_HOLY)
		&&  IS_GOOD(ch) && IS_EVIL(victim)) {
			extra_dam = number_range(dam[DAM_BASE] * 1/5+1, 
				dam[DAM_BASE] * 2 / 5+1);
			if (damage(ch, victim, extra_dam, 0, DAM_HOLY, DAMF_NONE)) {
				act("$n's flesh was burned with the holy aura of $p.",
					victim, wield, NULL, TO_ROOM);
				act_puts("Your flesh was burned with the holy aura of $p.",
					victim, wield, NULL, TO_CHAR, POS_DEAD);
			}
		}

		if (victim->hit > 0 && IS_WEAPON_STAT(wield, WEAPON_VAMPIRIC)) {
			int suck_heal = 0;

			extra_dam = number_range(dam[DAM_BASE]/7+1, 
				dam[DAM_BASE] /5+1);
			if (IS_NPC(victim))
				suck_heal = extra_dam /5;
			else
				suck_heal = extra_dam /2;

			if (damage(ch, victim, extra_dam, 0, DAM_NEGATIVE, DAMF_NONE)) {
				act("$p draws life from $n.",
				    victim, wield, NULL, TO_ROOM);
				act("You feel $p drawing your life away.",
				    victim, wield, NULL, TO_CHAR);
				if (ch->hit + suck_heal < ch->hit * 12/10)
					ch->hit += suck_heal;
				else
					suck_heal = 0;
				DEBUG(DEBUG_VAMPIRIC,
					"vampiric: %s[%d] %dhp(+%d)"
					" vs %s[%d] %dhp(-%d) with %s",
					ch->name,
					LEVEL(ch),
					ch->hit,
					suck_heal,
					victim->name,
					LEVEL(victim),
					victim->hit,
					extra_dam,
					wield->name);
			}
		}

		if (victim->hit > 0 && IS_WEAPON_STAT(wield, WEAPON_FLAMING)) {
			extra_dam = number_range(1,wield->level / 4 + 1);
			act("$n is burned by $p.", victim, wield, NULL, TO_ROOM);
			act("$p sears your flesh.",
			    victim, wield, NULL, TO_CHAR);
			fire_effect((void *) victim, wield->level/2, extra_dam,
				    TARGET_CHAR);
			damage(ch, victim, extra_dam, 0, DAM_FIRE, DAMF_NONE);
		}

		if (victim->hit > 0 && IS_WEAPON_STAT(wield, WEAPON_FROST)) {
			extra_dam = number_range(1,wield->level / 6 + 2);
			act("$p freezes $n.", victim, wield, NULL, TO_ROOM);
			act("The cold touch of $p surrounds you with ice.",
			    victim, wield, NULL, TO_CHAR);
			cold_effect(victim, wield->level/2, extra_dam, TARGET_CHAR);
			damage(ch, victim, extra_dam, 0, DAM_COLD, DAMF_NONE);
		}

		if (victim->hit > 0 && IS_WEAPON_STAT(wield, WEAPON_SHOCKING)) {
			extra_dam = number_range(1, wield->level/5 + 2);
			act("$n is struck by lightning from $p.",
			    victim, wield, NULL, TO_ROOM);
			act("You are shocked by $p.",
			    victim, wield, NULL, TO_CHAR);
			shock_effect(victim, wield->level/2, extra_dam, TARGET_CHAR);
			damage(ch, victim, extra_dam, 0, DAM_LIGHTNING, DAMF_NONE);
		}
	}

	if (is_not_my_augment(ch, wield)
	&& !IS_IMMORTAL(ch))
		wield = augment_unauthorized_use(ch, wield);

	tail_chain();
}

/*
 * handle_death - called from `damage' if `ch' has killed `victim'
 */
void handle_death(CHAR_DATA *ch, CHAR_DATA *victim)
{
	bool vnpc = IS_NPC(victim);
	CHAR_DATA *gch = NULL;
	ROOM_INDEX_DATA *vroom = victim->in_room;
	bool is_duel = !IS_NPC(victim) 
		&& (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)) 
		&& (IS_SET(victim->in_room->room_flags, ROOM_BATTLE_ARENA)
		    || is_waffected(victim, WAFF_ARENA));
	OBJ_DATA *corpse;
	class_t *cl;
	WORLD_AFFECT_DATA *waff;

	if (!is_duel)
		handle_dishonor(ch, victim);

	if (!is_duel && handle_enslavement(ch, victim)) {
		group_gain(ch, victim);
		return;
	}


	if (!IS_NPC(ch)) {
		if (!is_duel) {
			act("$n is DEAD!!", victim, NULL, NULL, TO_ROOM);
			char_puts("You have been KILLED!!\n\n", victim);
			victim->position = POS_DEAD;
		}
		else {
			act("$n is DEFEATED!", victim, NULL, NULL, TO_ROOM);
			char_puts("You have been DEFEATED!\n\n", victim);
			victim->position = POS_DEAD;
		}
	}

	/* world affect for gold drop bonus */
	if (vnpc 
	&& !IS_SET(victim->affected_by, AFF_CHARM)
	&& !IS_SET(victim->pIndexData->act, ACT_PET)
	&& (waff = ch_waffected(ch, WAFF_GOLD)) != NULL) {
		if (number_percent() <= waff->chance) {
			victim->gold += (victim->pIndexData->wealth /100)
				* (waff->modifier / 100);
		}
	}

	group_gain(ch, victim);

	if (!is_duel)
		align_standing_death_gain(ch, victim);

	/*
	 * Death trigger
	 */
	if (vnpc && HAS_TRIGGER(victim, TRIG_DEATH)) {
		victim->position = POS_STANDING;
		mp_percent_trigger(victim, ch, NULL, NULL, TRIG_DEATH);
	}

	/* if victim has witch cursed someone, cure them. */
	if (!is_duel)
		handle_witch_curse(ch, victim);

	/* remove pk_ok victim was in anyone's list */
	if (!is_duel) {
		for (gch = ch->in_room->people; gch; gch = gch->next_in_room) {
			if (!is_same_group(gch, ch) || IS_NPC(gch))
				continue;
			delete_pk_ok(gch, victim);
		}
	}

	if (vnpc)
		wiznet("$N killed $t.",
			ch, mlstr_mval(victim->short_descr), WIZ_MOBDEATHS, 0, 0);
	else if (is_duel)
		wiznet_death(ch, victim, DEATH_DUEL);
	else 
		wiznet_death(ch, victim, DEATH_KILL);

	if (is_duel) {
		raw_duel_defeat(ch, victim);
		return;
	}
	else
		corpse = raw_kill(ch, victim, is_duel);

	/* RT new auto commands */
	if (!IS_NPC(ch) 
	&& vnpc 
	&& vroom == ch->in_room 
	&& corpse
	&& can_see_obj(ch, corpse)) {

		if (HAS_SKILL(ch, gsn_vampire)) {
			act_puts("$n sucks {rblood{x from $N's corpse!!",
				 ch, NULL,victim,TO_ROOM,POS_RESTING);
			char_puts("You suck {rblood{x "
				  "from the corpse.\n\n", ch);
			gain_condition(ch, COND_BLOODLUST, 3);
		}

		if (IS_SET(ch->conf_flags, PLR_CONF_AUTOLOOK))
			do_look_in(ch, "corpse");
		if (corpse->contains) {
			/* corpse exists and not empty */
			if (IS_SET(ch->conf_flags, PLR_CONF_AUTOLOOT)) {
				get_gold_corpse(ch, corpse);
				do_get(ch, "all corpse");
			}
			else if (IS_SET(ch->conf_flags, PLR_CONF_AUTOGOLD))
				get_gold_corpse(ch, corpse);
		}

		if (IS_SET(ch->conf_flags, PLR_CONF_AUTOSAC))
			sac_obj(ch, corpse);
	}

	if (vnpc || victim->position == POS_STANDING || is_duel)
		return;

	/* Dying penalty: 2/3 way back. */
	if (IS_SET(victim->state_flags, STATE_WANTED)
	&&  victim->level > 1) {
                REMOVE_BIT(victim->state_flags, STATE_WANTED);
                victim->level--;
                victim->pcdata->plevels++;
                victim->exp = exp_for_level(victim, victim->level);
                victim->exp_tl = 0;
	}
	else
		if (victim->exp_tl > 0)
			gain_exp(victim, -victim->exp_tl*2/3);

	/* dispatch the true lifers */
	if (IS_SET(victim->acct_flags, ACCT_TRUE_LIFER)) {
		char buf[MAX_STRING_LENGTH];
		if (victim->level > (LEVEL_HERO/2)) {
			act_puts(
			"{WZsuzsu{x descends from the heavens and whispers a solemn eulogy\n"
			"before carrying your soul in her arms, off to {WValhalla{x!\n",
			victim, NULL, NULL, TO_CHAR, POS_DEAD);
			act_puts(
			"{WZsuzsu{x descends from the heavens and whispers a solemn eulogy\n"
			"before carrying $N's soul in her arms, off to {WValhalla{x!",
			victim, NULL, NULL, TO_ROOM, POS_DEAD);
		}
		else {
			act_puts(
			"{WZsuzsu{x descends from the heavens, looks at the corpse, and sighs.\n",
			victim, NULL, NULL, TO_ALL, POS_DEAD);
		}
		snprintf(buf, sizeof(buf),
			"TrueLifer: %s[%d] (%s %s %s) died to %s.",
			victim->name,
			victim->level,
			clan_name(victim->clan),
			race_name(victim->race),
			class_name(victim),
			(ch!=victim) ? format_short(ch->short_descr, ch->name, NULL)
			     : "Natural Causes");

		wiznet(buf, ch, victim, WIZ_DEATHS, 0, 0);
		
		LOG(buf);
		snprintf(buf, sizeof(buf),
			"{W%s{x was {rkilled{x by %s.{x",
			victim->name,
			(ch!=victim) 
			? format_short(ch->short_descr, ch->name, NULL)
			: "Natural Causes");
		do_death_announce(victim, buf);

		snprintf(buf, sizeof(buf),
			"%s %s[%d] (%s/%s/%s) XP:%d Quests:%d PK:%d MK:%s Mobs:%d died to %s[%d] - %s.",
			compact_date_str(current_time),
			victim->name,
			victim->level,
			clan_name(victim->clan),
			race_name(victim->race),
			class_name(victim),
			victim->exp,
			victim->pcdata->questcount,
			victim->pcdata->pk_kills,
			IS_SET(victim->acct_flags, ACCT_MULTIKILLER) 
				? "true" : "false",
			victim->pcdata->align_killed[0]
			+ victim->pcdata->align_killed[1]
			+ victim->pcdata->align_killed[2]
			+ victim->pcdata->align_killed[3],
			(ch!=victim) ? format_short(ch->short_descr, ch->name, NULL)
			     : "Natural Causes",
			(ch!=victim) ? ch->level : 0,
			(victim->desc) ? victim->desc->host : "unknown.ip");

		if (!append_file(TRUELIFER_FILE, buf)) {
			LOG(buf);
			BUG("Couldn't open true lifer file: %s, death stored in logfile", 
				TRUELIFER_FILE);
		}

		/* purge room of items if nobody there */
		CHAR_DATA *prev = NULL;
		OBJ_DATA *obj = NULL, *obj_next = NULL;
		int count = 0;
		if (corpse && corpse->in_room) {
			for (prev = corpse->in_room->people; prev; 
				prev = prev->next_in_room) {
				if (prev != victim && !IS_NPC(prev))
					count++;
			}
			if (count == 0) {
				for (obj = corpse->in_room->contents; 
				     obj != NULL; obj = obj_next) {
					obj_next = obj->next_content;
					if (!IS_OBJ_STAT(obj, ITEM_NOPURGE)
					&& obj->pIndexData->vnum != OBJ_VNUM_CORPSE_PC
					&& obj->pIndexData->vnum != OBJ_VNUM_CORPSE_NPC
					&& !IS_NULLSTR(obj->past_owner[0])
					&& !str_cmp(victim->name, obj->past_owner[0])
)
						extract_obj(obj, 0);
					if (obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC)
						corpse = obj;
				}
				if (corpse) {
					for (obj = corpse->contains; obj; obj = obj_next) {
						obj_next = obj->next_content;
						extract_obj(obj, 0);
					}
				}
			}
		}

		delete_player(victim, "1 death limit for True Lifers");
		return;
	}

	/* Die too much and is deleted ... :( */
	if ((cl = class_lookup(victim->class))
	&&  !CAN_FLEE(ch, cl)) {
		if (++victim->pcdata->death > cl->death_limit) {
			char msg[MAX_STRING_LENGTH];

			snprintf(msg, sizeof(msg),
				 "%d deaths limit for %s",
				 cl->death_limit, cl->name);
			delete_player(victim, msg);
			return;
		}
	}
	else {
		if ((++victim->pcdata->death % 3) != 0)
			return;
		else if (--victim->perm_stat[STAT_CON] < 3) {
			delete_player(victim, "lack of CON");
			return;
		}
		else
			char_puts("You feel your life power has decreased "
				  "with this death.\n", victim);
	}
}

/*
 * Inflict damage from a hit.
 * dt - damage skill
 * rvalue = how much damage was done.
 */
int damage(CHAR_DATA *ch, CHAR_DATA *victim,
	    int dam, int dt, int dam_type, int dam_flags)
{
	bool immune;
	int dam2;
	int loc;
	int sk = 0;
	bool was_asleep = FALSE;
	WORLD_AFFECT_DATA *waff;

	if (IS_DELETED(victim))
		return FALSE;
	
	if (JUST_KILLED(victim))
		return FALSE;

	if (victim != ch) {
		if(victim->position == POS_SLEEPING) {
			was_asleep = TRUE;
		}
		/*
		 * Certain attacks are forbidden.
		 * Most other attacks are returned.
		 */
		if (victim->position > POS_STUNNED) {
			if (victim->fighting == NULL) {
				if ((victim->in_room == ch->in_room 
				|| IS_NPC(victim))
				&& dt != gsn_spellbane)
					set_fighting(victim, ch);
				else {
					   if (IS_AFFECTED(victim, AFF_SLEEP)) {
					       REMOVE_BIT(victim->affected_by, AFF_SLEEP);
               					affect_bit_strip(victim, TO_AFFECTS, AFF_SLEEP);
       					    }
					   victim->position = POS_STANDING;
				}
				if (IS_NPC(victim)
				&&  HAS_TRIGGER(victim, TRIG_KILL))
					mp_percent_trigger(victim, ch, NULL,
							   NULL, TRIG_KILL);
			}
			if (victim->timer <= 4) {
				if ((victim->in_room == ch->in_room 
				|| IS_NPC(victim))
				&& dt != gsn_spellbane)
					victim->position = POS_FIGHTING;
			}
			else{
                                           if (IS_AFFECTED(victim, AFF_SLEEP)) {
                                               REMOVE_BIT(victim->affected_by, AFF_SLEEP);
                                                affect_bit_strip(victim, TO_AFFECTS, AFF_SLEEP);
                                            }
                                           victim->position = POS_STANDING;
                                }

		}

		if (victim->position > POS_STUNNED) {
			if (ch->fighting == NULL) {

				if ((victim->in_room == ch->in_room 
				|| IS_NPC(victim))
				&& dt != gsn_spellbane)
					set_fighting(ch, victim);
				else {
                                           if (IS_AFFECTED(victim, AFF_SLEEP)) {
                                               REMOVE_BIT(victim->affected_by, AFF_SLEEP);
                                                affect_bit_strip(victim, TO_AFFECTS, AFF_SLEEP);
                                            }
                                           victim->position = POS_STANDING;
                                }
			}

			/*
			 * If victim is charmed, ch might attack
			 * victim's master.
			 */
			if (IS_NPC(ch)
			&&  IS_NPC(victim)
			&&  IS_AFFECTED(victim, AFF_CHARM)
			&&  victim->master
			&&  victim->master->in_room == ch->in_room
			&&  !victim->master->fighting
			&&  number_bits(2) == 0) {
				stop_fighting(ch, FALSE);
				multi_hit(ch, victim->master, TYPE_UNDEFINED);
				return FALSE;
			}
		}


		/*
		 * More charm and group stuff.
		 */
		if (victim->master == ch)
			stop_follower(victim);
		else if (!IS_NPC(victim)
		&& victim->master != NULL
		&& IS_AFFECTED(victim, AFF_CHARM))
			stop_follower(victim);


		if (MOUNTED(victim) == ch || RIDDEN(victim) == ch)
			victim->riding = ch->riding = 0;
	}

	/* adjustment for strength */
	if (dt >= TYPE_HIT && ch != victim) {

		if (is_affected(ch, gsn_giant_strength))
			dam += dam * 0.10;

		if (is_affected(ch, gsn_dragon_strength))
			dam += dam * 0.10;
	}

	/*
	 * No one in combat can hide, be invis or camoed.
	 */
	do_visible(ch, str_empty);

	/*
	 * Damage modifiers.
	 * mortal_strike needs to go through magic protection
	 */
	if (!IS_SET(dam_flags, DAMF_NOREDUCTION)) {
		if (IS_AFFECTED(victim, AFF_SANCTUARY))
			dam /= 2;

		if (dt != gsn_mortal_strike
		&& IS_AFFECTED(victim, AFF_BLACK_SHROUD)) 
			dam /= 2;

		if (dt != gsn_mortal_strike
		&& IS_AFFECTED(victim, AFF_ENHANCED_ARMOR))
			dam /= 2;

		if (IS_AFFECTED(victim, AFF_MINOR_SANCTUARY))
			dam -= dam / 4;

		if (dt != gsn_mortal_strike
		&& IS_AFFECTED(victim, AFF_MINOR_BLACK_SHROUD)) 
			dam -= dam / 4;
	 
		if (IS_AFFECTED(victim, AFF_PROTECT_EVIL) && IS_EVIL(ch))
			dam -= dam / 5;

		if (IS_AFFECTED(victim, AFF_PROTECT_GOOD) && IS_GOOD(ch))
			dam -= dam / 5;

		if (is_affected(victim,gsn_resistance))
			dam -= dam / 4;

		if (is_affected(victim, gsn_intoxication))
			dam -= dam / 10;
	}

	immune = FALSE;
	loc = IS_SET(dam_flags, DAMF_SECOND) ? WEAR_SECOND_WIELD : WEAR_WIELD;

	/*
	 * Check for parry, and dodge.
	 */
	if (dt >= TYPE_HIT && ch != victim) {
		/*
		 * some funny stuff
		 */
		if (IS_NPC(victim) && is_affected(victim, gsn_mirror)) {
			act("$n shatters into tiny fragments of glass.",
			    victim, NULL, NULL, TO_ROOM);
			extract_char(victim, 0);
			return FALSE;
		}

		if (!was_asleep) {
			if (check_blink(ch, victim))
		                return FALSE;       	
			if (check_shield_block(ch, victim, loc))
				return FALSE;
			if (check_parry(ch, victim, loc))
				return FALSE;
			if (check_dodge(ch, victim))
				return FALSE;
			if (check_tumble(ch, victim))
				return FALSE;
			if (check_hand_block(ch, victim))
                                return FALSE;
   		        if (check_haft_block(ch, victim, loc))
       	            	 	return FALSE;
       		        /* if (check_space_between_beats(ch, victim))
	       	          	return FALSE;*/

			if ((sk = get_skill(ch, gsn_master_hand))
			&& get_eq_char(ch, WEAR_WIELD) == NULL
			&& number_percent() < sk/5+LEVEL(ch)-LEVEL(victim)) {
				SET_BIT(victim->affected_by,
					AFF_WEAK_STUN);
				act_puts("You hit $N with a stunning "
					 "force!", ch, NULL, victim,
					 TO_CHAR, POS_DEAD);
				act_puts("$n hit you with a stunning "
					 "force!", ch, NULL, victim,
					 TO_VICT, POS_DEAD);
				act_puts("$n hits $N with a stunning "
					 "force!", ch, NULL, victim,
					 TO_NOTVICT, POS_DEAD);
				check_improve(ch, gsn_master_hand,
					      TRUE, 6);
			}
		}
	}

	switch(check_immune(victim, dam_type)) {
	case IS_IMMUNE:
		immune = TRUE;
		dam = 0;
		break;

	case IS_RESISTANT:
		dam -= dam/3;
		break;

	case IS_VULNERABLE:
		dam += dam/3;
		break;
	}

	if (dam > 0 && dt >= TYPE_HIT && ch != victim) {
		if ((dam2 = critical_strike(ch, victim, dam)) != 0)
			dam = dam2;
	}
	/*
	 * lance--negligible damage unless on horseback
	 */
	if (!IS_NPC(ch)
	&&!MOUNTED(ch)
	&& get_eq_char(ch, WEAR_WIELD)
	&& get_eq_char(ch, WEAR_WIELD)->value[ITEM_WEAPON_TYPE] == WEAPON_LANCE) {
		dam = 5;
	}

	/*
	 * bows -- negligible damage in melee
	 */
	if (!IS_NPC(ch)
	&& get_eq_char(ch, WEAR_WIELD)
	&& get_eq_char(ch, WEAR_WIELD)->value[ITEM_WEAPON_TYPE] == WEAPON_BOW
	&& ch->in_room == victim->in_room)
		dam = 5;

	if (IS_SET(dam_flags, DAMF_SHOW))
		dam_message(ch, victim, dam, dt, immune, dam_type);

	if (dam == 0)
		return FALSE;

	if (dt >= TYPE_HIT && ch != victim)
		check_eq_damage(ch, victim, loc);

	/*
	 * world affect to modify pvp damage
	 * (but not the world for the damage)
	 */
	if (dam > 0
	&& (!IS_NPC(ch) || (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)))
	&& !IS_NPC(victim)
	&& (waff = ch_waffected(victim, WAFF_PVP_DAMAGE)) != NULL) {
		if (waff->modifier <= 0) {
			BUG("World Affect of PVP_DAMAGE would cause negative damage."
			    "  Setting to half damage.");
			waff->modifier = 50;
		}

		/* skills that are not affected by this */
		if (dt != gsn_assassinate
		&&  dt != gsn_backstab
		&&  dt != gsn_dual_backstab
		&&  dt != gsn_ambush
		&&  dt != gsn_vampiric_bite)
			dam = dam * waff->modifier / 100;
	}
	/* 
	 * World affect to modify PC damage to NPCs
	 */
	else if (dam > 0
	&& !IS_NPC(ch) && IS_NPC(victim)
	&& (waff = ch_waffected(victim, WAFF_PVM_DAMAGE)) != NULL) {
		if (waff->modifier <= 0) {
			BUG("World Affect of PVM_DAMAGE would cause negative damage."
			    "  Setting to double damage.");
			waff->modifier = 200;
		}

		dam = dam * waff->modifier / 100;
	}

	DEBUG(DEBUG_DAM,
		"%s[%d] vs %s[%d] %s for %d",
		mlstr_mval(ch->short_descr),
		LEVEL(ch),
		mlstr_mval(victim->short_descr),
		LEVEL(victim),
		dt >= TYPE_HIT ? "melee" : skill_name(dt),
		dam);

	/*
	 * Hurt the victim.
	 * Inform the victim of his new state.
	 */
	victim->hit -= dam;

	/* release charmed players from charm */
	if (!IS_NPC(victim)
	&& IS_AFFECTED(victim, AFF_CHARM)) {
		act("The sting of pain breaks you from your charming daze.",
			victim, NULL, NULL, TO_CHAR);
		act("$n shakes $h headhead as clarity returns.",
			victim, NULL, NULL, TO_CHAR);
		REMOVE_BIT(victim->affected_by, AFF_CHARM);
	}

	if (IS_IMMORTAL(victim) && victim->hit < 1)
		victim->hit = 1;

        /*holy cloak stuff*/
        if(dt >= TYPE_HIT && victim != ch 
	&& is_affected(victim, gsn_holy_cloak) && IS_EVIL(ch))
        {
                act("You are burned by $n's holy cloak!", victim, NULL, ch, TO_VICT);
                act("$N is burned by your holy cloak!", victim, NULL, ch, TO_CHAR);
                act("$n is burned by $N's holy cloak!", ch, NULL, victim, TO_NOTVICT);
                damage(victim, ch, dam/5, gsn_holy_cloak, DAM_HOLY, DAMF_SHOW);
        }

	update_pos(victim);

	if(is_affected(victim, gsn_rnet_trap) && victim != ch 
	&& (dam >= victim->max_hit/10 || victim->position == POS_FIGHTING
	|| victim->hit <= 3*victim->max_hit/4)) {
		affect_strip(victim, gsn_rnet_trap);
		char_puts("The net snaps, spilling you out to the ground!\n",
			victim);
		act("The net snaps, spilling $n out to the ground!",
			victim,NULL,NULL,TO_ROOM);
	}

	switch(victim->position) {
	case POS_MORTAL:
		if (dam_type == DAM_HUNGER || dam_type == DAM_THIRST) break;
		act("$n is mortally wounded, and will die soon, if not aided.",
		    victim, NULL, NULL, TO_ROOM);
		char_puts( "You are mortally wounded, and will die soon, if not aided.\n", victim);
		break;

	case POS_INCAP:
		if (dam_type == DAM_HUNGER || dam_type == DAM_THIRST) break;
		act("$n is incapacitated and will slowly die, if not aided.",
		    victim, NULL, NULL, TO_ROOM);
		char_puts( "You are incapacitated and will slowly die, if not aided.\n", victim);
		break;

	case POS_STUNNED:
		if (dam_type == DAM_HUNGER || dam_type == DAM_THIRST) break;
		act("$n is stunned, but will probably recover.",
		    victim, NULL, NULL, TO_ROOM);
		char_puts("You are stunned, but will probably recover.\n",
			     victim);
		break;

	case POS_DEAD:
		break;

	default:
		if (dam_type == DAM_HUNGER || dam_type == DAM_THIRST) break;
		if (dam > victim->max_hit / 4)
			char_puts("That really did HURT!\n", victim);
		if (victim->hit < victim->max_hit / 4)
			char_puts("You sure are BLEEDING!\n", victim);
		break;
	}

	/*
	 * Sleep spells and extremely wounded folks.
	 */
	if (!IS_AWAKE(victim) && victim->fighting)
		victim->fighting = NULL;

	/*
	 * Payoff for killing things.
	 */
	if (victim->position == POS_DEAD) {
		handle_death(ch, victim);
		return dam;
	}

	if (victim == ch)
		return dam;

	/*
	 * Take care of link dead people.
	 */
	if (!IS_NPC(victim)
	&&  victim->desc == NULL
	&&  !IS_SET(victim->comm, COMM_NOFLEE)) {
		if (number_range(0, victim->wait) == 0) {
			do_flee(victim, str_empty);
			return dam;
		}
	}

	/*
	 * Wimp out?
	 */
	if (IS_NPC(victim) && dam > 0 && victim->wait < PULSE_VIOLENCE / 2) {
		flag64_t act = victim->pIndexData->act;
		if ((IS_SET(act, ACT_WIMPY) && number_bits(2) == 0 &&
		     victim->hit < victim->max_hit / 5)
		||  (IS_AFFECTED(victim, AFF_CHARM) &&
		     victim->master != NULL &&
		     victim->master->in_room != victim->in_room)
		||  (IS_AFFECTED(victim, AFF_FEAR) &&
		     !IS_SET(act, ACT_NOTRACK))) {
			do_flee(victim, str_empty);
			victim->last_fought = NULL;
		}
	}

	if (!IS_NPC(victim)
	&&  victim->hit > 0
	&&  (victim->hit <= victim->wimpy || IS_AFFECTED(victim, AFF_FEAR))
	&&  victim->wait < PULSE_VIOLENCE / 2)
		do_flee(victim, str_empty);

	tail_chain();
	return dam;
}

static bool inline
is_safe_raw(CHAR_DATA *ch, CHAR_DATA *victim)
{
	/*
	CHAR_DATA *gch;
	char buf[ MAX_STRING_LENGTH ];
	*/
	char groupflag;

	groupflag = 'n';

	/*
	 * ghosts are safe
	 * this check must be done first to avoid
	 * suicyco muttafuckas who recite 'leather-bound book' (#5743)
	 * without any target specified
	 * extracted NPCs are safe too
	 */
	if (!IS_NPC(victim)) {
		int clan;

		/* ghost cannot attack anyone */
		if (ch != victim
		&&  !IS_NPC(ch)
		&&  IS_SET(ch->state_flags, STATE_GHOST))
			return TRUE;

		/* clan defenders can attack anyone in their clan */
		if (victim->in_room
		&&  (clan = victim->in_room->area->clan)
		&&  victim->clan != clan
		&&  ch->clan == clan)
			return FALSE;

		/* otherwise ghosts are safe */
		if (IS_SET(victim->state_flags, STATE_GHOST))
			return TRUE;
	}
	else if (victim->extracted)
		return TRUE;

	if (victim->fighting == ch
	||  ch == victim
	||  IS_IMMORTAL(ch))
		return FALSE;

	/* handle ROOM_PEACE flags */
	if ((victim->in_room && IS_SET(victim->in_room->room_flags, ROOM_PEACE))
	||  (ch->in_room && IS_SET(ch->in_room->room_flags, ROOM_PEACE)))
		return TRUE;

	/* handle ROOM_SAFE flags */
	if ((victim->in_room && IS_SET(victim->in_room->room_flags, ROOM_SAFE))
	||  (ch->in_room && IS_SET(ch->in_room->room_flags, ROOM_SAFE)))
		return TRUE;

	/* link dead players whose adrenaline is not gushing are safe */
	if (!IS_NPC(victim) && !IS_PUMPED(victim) && victim->desc == NULL)
		return TRUE;

	/* multiplay pkill reporting */
	/*
	if ( !IS_NPC( ch ) && !IS_NPC( victim ) )
	{
		sprintf( buf, "IS_SAFE: Check for %s grouped with", ch->name );
		for ( gch = char_list; gch; gch = gch->next )
		{
			if ( is_same_group( gch, ch ) && gch != ch )
			{
				groupflag = 'y';
				strcat( buf, " " );
				strcat( buf, gch->name );
			}
		}

		if ( groupflag == 'n' )
			strcat( buf, " no one" );

		strcat( buf, " on " );
		strcat( buf, victim->name );
		log( buf );
	}
	*/

        if(IS_NPC(victim) && victim->invis_level > ch->level)
                return TRUE;

	return !in_PK(ch, victim);
}

/*
 * generic safe-checking function wrapper
 *
 * all the checks are done is_safe_raw to properly strip STATE_GHOST
 * flag if victim is not safe. add you checks there
 */
bool is_safe_nomessage(CHAR_DATA *ch, CHAR_DATA *victim)
{
	bool safe;
	CHAR_DATA *mount;

	if (IS_NPC(ch)
	&&  IS_AFFECTED(ch, AFF_CHARM)
	&&  ch->master
	&&  ch->in_room == ch->master->in_room)
		return is_safe_nomessage(ch->master, victim);
	
	if (IS_NPC(victim)
	&&  IS_AFFECTED(victim, AFF_CHARM)
	&&  victim->master)
		return is_safe_nomessage(ch, victim->master);

	if ((mount = RIDDEN(victim)))
		return is_safe_nomessage(ch, mount);

	if ((safe = is_safe_raw(ch, victim)) || IS_NPC(ch))
		return safe;

	if (victim != ch && IS_SET(ch->state_flags, STATE_GHOST)) {
		char_puts("You return to your normal form.\n", ch);
		REMOVE_BIT(ch->state_flags, STATE_GHOST);
                REMOVE_BIT(ch->affected_by, AFF_FLYING);
                REMOVE_BIT(ch->affected_by, AFF_PASS_DOOR);
	}

	return safe;
}

bool is_safe(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (is_safe_nomessage(ch, victim)) {
		act("The gods protect $N.",ch,NULL,victim,TO_CHAR);
		act("The gods protect $N from $n.",ch,NULL,victim,TO_ROOM);
		return TRUE;
	}
	return FALSE;
}

bool is_safe_spell(CHAR_DATA *ch, CHAR_DATA *victim, bool area)
{
#if 0
	if (ch == victim && !area)
		return TRUE;
#endif
	if (area) {
		if (IS_IMMORTAL(victim)
		||  is_same_group(ch, victim)
		||  ch == victim
		||  RIDDEN(ch) == victim
		||  MOUNTED(ch) == victim)
			return TRUE;
	}

	return is_safe(ch, victim);
}

/*
 * Check for parry.
 */
bool check_parry(CHAR_DATA *ch, CHAR_DATA *victim, int loc)
{
	int chance;

	if (!IS_AWAKE(victim))
		return FALSE;

	chance = parry_chance(ch, victim);

	if (chance <= 0)
		return FALSE;

	if (!IS_NPC(victim) && !IS_NPC(ch))
		DEBUG(DEBUG_EVADE,
			"%s[%d] parry %d%% vs %s[%d]",
			victim->name,
			LEVEL(victim),
			chance,
			ch->name,
			LEVEL(ch));

	if (number_percent() >= chance)
		return FALSE;

	act("You parry $n's attack.", ch, NULL, victim, TO_VICT | ACT_VERBOSE);
	act("$N parries your attack.", ch, NULL, victim, TO_CHAR | ACT_VERBOSE);

	check_weapon_damage(ch, victim, loc);

	if (number_percent() > chance) {
		/* size and weight */
		chance += ch->carry_weight / 25;
		chance -= victim->carry_weight / 20;

		if (ch->size < victim->size)
			chance += (ch->size - victim->size) * 25;
		else
			chance += (ch->size - victim->size) * 10;

		/* stats */
		chance += get_curr_stat(ch, STAT_STR)/4;
		chance -= get_curr_stat(victim, STAT_DEX)/3;

		if (IS_AFFECTED(ch, AFF_FLYING))
			chance -= 10;

		/* speed */
		if (IS_NPC(ch) && IS_SET(ch->pIndexData->off_flags, OFF_FAST))
			chance += 10;
		if (IS_NPC(victim) && IS_SET(victim->pIndexData->off_flags,
					     OFF_FAST))
			chance -= 20;

		/* level */
		chance += (LEVEL(ch) - LEVEL(victim)) * 2;

		if (!IS_NPC(victim) && !IS_NPC(ch) && chance > 0)
			DEBUG(DEBUG_EVADE,
				"%s[%d] parry-fall %d%% vs %s[%d]",
				victim->name,
				LEVEL(victim),
				chance,
				ch->name,
				LEVEL(ch));

		/* now the attack */
		if (number_percent() < (chance / 20 )) {
			act("You couldn't manage to keep your position!",
			    ch, NULL, victim, TO_VICT);
			act("You fall down!", ch, NULL, victim, TO_VICT);
			act("$N couldn't manage to hold your attack "
			    "and falls down!",
			    ch, NULL, victim, TO_CHAR);
			act("$n stunning force makes $N fall down.",
			    ch, NULL, victim, TO_NOTVICT);

			WAIT_STATE(victim, SKILL(gsn_bash)->beats);
			victim->position = POS_RESTING;
		}
	}

	check_improve(victim, gsn_parry, TRUE, 6);
	return TRUE;
}

/***********************
* Check for haft block.*
* Coded by Thornan     *
************************/
bool check_haft_block (CHAR_DATA * ch, CHAR_DATA * victim, int loc)
{
	int chance;

	if (!IS_AWAKE (victim))
		return FALSE;


	chance = haft_block_chance(ch, victim);

	if (chance <= 0)
		return FALSE;

	if (!IS_NPC(victim) && !IS_NPC(ch))
		DEBUG(DEBUG_EVADE,
			"%s[%d] haft_block %d%% vs %s[%d]",
			victim->name,
			LEVEL(victim),
			chance,
			ch->name,
			LEVEL(ch));

	if (number_percent () >= chance)
		return FALSE;

	act ("You use the haft of your weapon to block $n's attack.", 
		ch, NULL, victim, TO_VICT);
	act ("$N uses the haft of $S weapon to block your attack.", 
		ch, NULL, victim, TO_CHAR);
	check_improve (victim, gsn_haft_block, TRUE, 6);
	return TRUE;
}


/*
 * Check tumble: Checks to see if the victim has the tumble skill
 */

bool check_tumble(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int chance;
	
	if (!IS_AWAKE(victim))
	  return FALSE;

	chance = tumble_chance(ch, victim);

	if (chance <= 0)
		return FALSE;

	if (!IS_NPC(victim) && !IS_NPC(ch))
		DEBUG(DEBUG_EVADE,
			"%s[%d] tumble %d%% vs %s[%d]",
			victim->name,
			LEVEL(victim),
			chance,
			ch->name,
			LEVEL(ch));

	if (number_percent () <= chance)
	{
		act ("You spot $n's attack coming and tumble gracefully out the way.", 
			ch, NULL, victim, TO_VICT);
		act ("$N spots your attack coming and rolls out of the way.", ch, NULL, 
			victim, TO_CHAR);
		check_improve(victim, gsn_tumble, TRUE, 6);
		return TRUE;
	}
	else
	{
		check_improve(victim, gsn_tumble, FALSE, 6);
		return FALSE;
	}
}

/*
 * check blink
 */
bool check_blink(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int chance;

	if (!IS_SET(victim->conf_flags, PLR_CONF_BLINK))
		return FALSE;

	chance = blink_chance(ch, victim);

	if (chance <= 0)
		return FALSE;

	if (!IS_NPC(victim) && !IS_NPC(ch))
		DEBUG(DEBUG_EVADE,
			"%s[%d] blink %d%% vs %s[%d]",
			victim->name,
			LEVEL(victim),
			chance,
			ch->name,
			LEVEL(ch));

	if (number_percent() >= chance
	||  victim->mana < 10)
		return FALSE;

	victim->mana -= UMAX(victim->level / 10, 1);

	act("You blink out $n's attack.",
	    ch, NULL, victim, TO_VICT | ACT_VERBOSE);
	act("$N blinks out your attack.",
	    ch, NULL, victim, TO_CHAR | ACT_VERBOSE);
	check_improve(victim, gsn_blink, TRUE, 6);
	return TRUE;
}

/*
 * Check for shield block.
 */
bool check_shield_block(CHAR_DATA *ch, CHAR_DATA *victim, int loc)
{
	int chance;

	if (!IS_AWAKE(victim))
		return FALSE;

	chance = shield_block_chance(ch, victim);

	if (chance <= 0)
		return FALSE;

	if (!IS_NPC(victim) && !IS_NPC(ch))/* && !IS_NPC(ch))*/
		DEBUG(DEBUG_EVADE,
			"%s[%d] shield block %d%% vs %s[%d]",
			victim->name,
			LEVEL(victim),
			chance,
			ch->name,
			LEVEL(ch));

	if (number_percent() < chance) {
		act("Your shield blocks $n's attack.",
		    ch, NULL, victim, TO_VICT | ACT_VERBOSE);
		act("$N deflects your attack with $S shield.",
		    ch, NULL, victim, TO_CHAR | ACT_VERBOSE);
		check_shield_damage(ch, victim, loc);
		check_improve(victim, gsn_shield_block, TRUE, 6);
		return TRUE;
	}
	return FALSE;
}

/* 
 * Check for hand block
 */

bool check_hand_block(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int chance;

	if (!IS_AWAKE(victim))
		return FALSE;

	chance = hand_block_chance(ch, victim);

	if (chance <= 0)
		return FALSE;

	if (!IS_NPC(victim) && !IS_NPC(ch))
		DEBUG(DEBUG_EVADE,
			"%s[%d] hand block %d%% vs %s[%d]",
			victim->name,
			LEVEL(victim),
			chance,
			ch->name,
			LEVEL(ch));

	if (number_percent() < chance) {
		act("Your hand blocks $n's attack.", 
			ch, NULL, victim, TO_VICT|ACT_VERBOSE);
		act("$N blocks your attack with $S hand.",
			ch, NULL, victim, TO_CHAR|ACT_VERBOSE);
		check_improve(victim, gsn_hand_block, TRUE, 6);
		return TRUE;
	}
	return FALSE;
}

/*
 * Check for dodge.
 */
bool check_dodge(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int chance;

	if (!IS_AWAKE(victim))
		return FALSE;

	chance = dodge_chance(ch, victim);

	if (chance <= 0)
		return FALSE;

	if (!IS_NPC(victim) && !IS_NPC(ch))
		DEBUG(DEBUG_EVADE,
			"%s[%d] dodge %d%% vs %s[%d]",
			victim->name,
			LEVEL(victim),
			chance,
			ch->name,
			LEVEL(ch));

	if (number_percent() >= chance)
		return FALSE;

	act("You dodge $n's attack.", ch, NULL, victim, TO_VICT | ACT_VERBOSE);
	act("$N dodges your attack.", ch, NULL, victim, TO_CHAR	| ACT_VERBOSE);

	check_improve(victim, gsn_dodge, TRUE, 6);
	return TRUE;
}

/* commented out -- not used due to bugs */
bool check_dodge_fail(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int chance = 0;
	if (number_percent() < get_skill(victim,gsn_dodge) / 20
	&&  !(IS_AFFECTED(ch, AFF_FLYING) || ch->position < POS_FIGHTING)) {
		/* size */
		if (victim->size < ch->size)
			/* bigger = harder to trip */
			chance += (victim->size - ch->size) * 10;

		/* dex */
		chance += get_curr_stat(victim, STAT_DEX);
		chance -= get_curr_stat(ch, STAT_DEX) * 3 / 2;

		if (IS_AFFECTED(victim, AFF_FLYING))
			chance -= 10;

		/* speed */
		if ((IS_NPC(victim) 
		&& IS_SET(victim->pIndexData->off_flags, OFF_FAST))
		||  IS_AFFECTED(victim, AFF_HASTE))
			chance += 10;
		if ((IS_NPC(ch) && IS_SET(ch->pIndexData->off_flags, OFF_FAST))
		||  IS_AFFECTED(ch, AFF_HASTE))
			chance -= 20;

		/* level */
		chance += (victim->level - ch->level) * 2;

		chance = chance / 20;

		if (!IS_NPC(victim) && !IS_NPC(ch) && chance > 0)
			DEBUG(DEBUG_EVADE,
				"%s[%d] dodge-fall %d%% vs %s[%d]",
				victim->name,
				LEVEL(victim),
				chance,
				ch->name,
				LEVEL(ch));

		/* now the attack */
		if (number_percent() < chance) {
			act("$n lost his postion and fall down!",
			    ch, NULL, victim, TO_VICT);
			act("As $N moves you lose your position fall down!",
			    ch, NULL, victim, TO_CHAR);
			act("As $N dodges $N's attack, $N lost his position "
			    "and falls down.", ch, NULL, victim, TO_NOTVICT);

			WAIT_STATE(ch, SKILL(gsn_trip)->beats);
			ch->position = POS_RESTING;
		}
	}
	check_improve(victim, gsn_dodge, TRUE, 6);
	return TRUE;
}

/*
 * Set position of a victim.
 */
void update_pos(CHAR_DATA *victim)
{
	if (victim->hit > 0) {
		if (victim->position <= POS_STUNNED) {
			if (IS_AFFECTED(victim, AFF_SLEEP)) {
				REMOVE_BIT(victim->affected_by, AFF_SLEEP);
				affect_bit_strip(victim, TO_AFFECTS, AFF_SLEEP);
			}

			victim->position = POS_STANDING;
		}
		return;
	}

	if (IS_NPC(victim) && victim->hit < 1) {
		victim->position = POS_DEAD;
		return;
	}

	if (victim->hit <= -11) {
		victim->position = POS_DEAD;
		return;
	}

	if (victim->hit <= -6)
		victim->position = POS_MORTAL;
	else if (victim->hit <= -3)
		victim->position = POS_INCAP;
	else
		victim->position = POS_STUNNED;
}

/*
 * Start fights.
 */
void set_fighting(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (ch->fighting != NULL) {
		bug("Set_fighting: already fighting", 0);
		return;
	}

	if (IS_AFFECTED(ch, AFF_SLEEP)) {
		REMOVE_BIT(ch->affected_by, AFF_SLEEP);
		affect_bit_strip(ch, TO_AFFECTS, AFF_SLEEP);
	}

	ch->fighting = victim;
	ch->position = POS_FIGHTING;
}

static void STOP_FIGHTING(CHAR_DATA *ch)
{
	ch->fighting = NULL;
	ch->position = IS_NPC(ch) ? ch->default_pos : POS_STANDING;
	update_pos(ch);
}

/*
 * Stop fights.
 */
void stop_fighting(CHAR_DATA *ch, bool fBoth)
{
	CHAR_DATA *fch;

	STOP_FIGHTING(ch);
	if (!fBoth)
		return;

	for (fch = char_list; fch; fch = fch->next) {
		if (fch->fighting == ch)
			STOP_FIGHTING(fch);
	}
}

/*
 * Make a corpse out of a character.
 */
OBJ_DATA * make_corpse(CHAR_DATA *ch)
{
	OBJ_DATA *corpse;
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	int i;

	if (IS_NPC(ch)) {
		corpse	= create_obj_of(get_obj_index(OBJ_VNUM_CORPSE_NPC),
					ch->short_descr);
		corpse->timer	= number_range(3, 6);
		if (ch->gold > 0 || ch->silver > 0) {
			OBJ_DATA *money = create_money(ch->gold, ch->silver);

			if (ch->gold > 500 || ch->silver > 50000)
				DEBUG(DEBUG_GOLD,
					"%s[%d] dropped %d gold %d silver upon death.",
					format_short(ch->short_descr, ch->name, NULL),
					ch->pIndexData->vnum,
					ch->gold,
					ch->silver);
					
			if (IS_SET(ch->form,FORM_INSTANT_DECAY))
				obj_to_room(money, ch->in_room);
			else
				obj_to_obj(money, corpse);

		}
	}
	else {
		corpse	= create_obj_of(get_obj_index(OBJ_VNUM_CORPSE_PC),
					ch->short_descr);
		if (IS_GOOD(ch))
		  i = 0;
		if (IS_EVIL(ch))
		  i = 2;
		else
		  i = 1;
		
                corpse->timer= number_range(7, 10);
		corpse->altar = get_altar(ch);

		if (ch->gold > 0 || ch->silver > 0)
			obj_to_obj(create_money(ch->gold, ch->silver), corpse);
	}

	corpse->owner = mlstr_dup(ch->short_descr);
	corpse->level = ch->level;

	ch->gold = 0;
	ch->silver = 0;

	for (obj = ch->carrying; obj != NULL; obj = obj_next) {
		obj_next = obj->next_content;
		obj_from_char(obj);
		if (obj->pIndexData->item_type == ITEM_POTION)
		    obj->timer = number_range(500,1000);
		if (obj->pIndexData->item_type == ITEM_SCROLL)
		    obj->timer = number_range(1000,2500);
		if (IS_SET(obj->extra_flags,ITEM_ROT_DEATH))  {
		    obj->timer = number_range(5,10);
		    if (obj->pIndexData->item_type == ITEM_POTION)
		       obj->timer += obj->level * 20;
		}
		REMOVE_BIT(obj->extra_flags,ITEM_VIS_DEATH);
		REMOVE_BIT(obj->extra_flags,ITEM_ROT_DEATH);

		if (IS_SET(obj->extra_flags, ITEM_INVENTORY)  ||
		    (obj->pIndexData->limit != -1 &&
			(obj->pIndexData->count > obj->pIndexData->limit)))
		  {
			  if (obj->pIndexData->limit > -1)
				DEBUG(DEBUG_LIMITED,
					"%s[%d] reclaim limited [%d-%d/%d] [%d] %s",
					ch->name, ch->level,
					obj->pIndexData->level,
					obj->pIndexData->count,
					obj->pIndexData->limit,
					obj->pIndexData->vnum,
					obj->pIndexData->name);
		    extract_obj(obj, 0);
		    continue;
		  }
		else if (IS_SET(ch->form,FORM_INSTANT_DECAY))
		  obj_to_room(obj, ch->in_room);

		else
		  obj_to_obj(obj, corpse);
	}

	obj_to_room(corpse, ch->in_room);

	return corpse;
}

/* this needs to parallel make_corpse() */
void strip_char(CHAR_DATA *ch)
{
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	OBJ_DATA *tattoo;
	OBJ_DATA *clanmark;

	BUG("stripping character");
	tattoo = get_eq_char(ch, WEAR_TATTOO);
	clanmark = get_eq_char(ch, WEAR_CLANMARK);

	if (tattoo != NULL)
		obj_from_char(tattoo);
	if (clanmark != NULL)
		obj_from_char(clanmark);

	if (ch->gold > 0 || ch->silver > 0) {
		obj_to_room(create_money(ch->gold, ch->silver), ch->in_room);
		ch->gold = 0;
		ch->silver = 0;
	}

	for (obj = ch->carrying; obj != NULL; obj = obj_next) {
		obj_next = obj->next_content;
		obj_from_char(obj);
		if (obj->pIndexData->item_type == ITEM_POTION)
		    obj->timer = number_range(500,1000);
		if (obj->pIndexData->item_type == ITEM_SCROLL)
		    obj->timer = number_range(1000,2500);
		if (IS_SET(obj->extra_flags,ITEM_ROT_DEATH))  {
		    obj->timer = number_range(5,10);
		    if (obj->pIndexData->item_type == ITEM_POTION)
		       obj->timer += obj->level * 20;
		}
		REMOVE_BIT(obj->extra_flags,ITEM_VIS_DEATH);
		REMOVE_BIT(obj->extra_flags,ITEM_ROT_DEATH);

		if (IS_SET(obj->extra_flags, ITEM_INVENTORY) 
		|| (obj->pIndexData->limit != -1 
		&& (obj->pIndexData->count > obj->pIndexData->limit))) {
			  if (obj->pIndexData->limit > -1)
				DEBUG(DEBUG_LIMITED,
					"%s[%d] reclaim limited [%d-%d/%d] [%d] %s",
					ch->name, ch->level,
					obj->pIndexData->level,
					obj->pIndexData->count,
					obj->pIndexData->limit,
					obj->pIndexData->vnum,
					obj->pIndexData->name);
		    extract_obj(obj, 0);
		    continue;
		}
		BUG("Object to room %s", obj->name);
		obj_to_room(obj, ch->in_room);
	}

	if (tattoo != NULL) {
		obj_to_char(tattoo, ch);
		equip_char(ch, tattoo, WEAR_TATTOO);
	}

	if (clanmark != NULL) {
		obj_to_char(clanmark, ch);
		equip_char(ch, clanmark, WEAR_CLANMARK);
	}
}

void death_cry(CHAR_DATA *ch)
{
  death_cry_org(ch, -1);
}

/*
 * Improved Death_cry contributed by Diavolo.
 */
void death_cry_org(CHAR_DATA *ch, int part)
{
	ROOM_INDEX_DATA *was_in_room;
	char *msg;
	int door;
	int vnum;

	vnum = 0;
	msg = "You hear $n's death cry.";

	if (part == -1)
	  part = number_bits(4);

	switch (part) {
	case  0:
		msg  = "$n hits the ground ... DEAD.";
		break;
	case  1:
		if (ch->material_descr == 0) {
		    msg  = "$n splatters blood on your armor.";
		    break;
		}
		/* FALLTHRU */
	case  2:
		if (IS_SET(ch->parts, PART_GUTS)) {
			msg = "$n spills $s guts all over the floor.";
			vnum = OBJ_VNUM_GUTS;
		}
		break;
	case  3:
		if (IS_SET(ch->parts, PART_HEAD)) {
			msg  = "$n's severed head plops on the ground.";
			vnum = OBJ_VNUM_SEVERED_HEAD;
		}
		break;
	case  4:
		if (IS_SET(ch->parts, PART_HEART)) {
			msg  = "$n's heart is torn from $s chest.";
			vnum = OBJ_VNUM_TORN_HEART;
		}
		break;
	case  5:
		if (IS_SET(ch->parts, PART_ARMS)) {
			msg  = "$n's arm is sliced from $s dead body.";
			vnum = OBJ_VNUM_SLICED_ARM;
		}
		break;
	case  6:
		if (IS_SET(ch->parts, PART_LEGS)) {
			msg  = "$n's leg is sliced from $s dead body.";
			vnum = OBJ_VNUM_SLICED_LEG;
		}
		break;
	case 7:
		if (IS_SET(ch->parts, PART_BRAINS)) {
			msg = "$n's head is shattered, and $s brains splash all over you.";
			vnum = OBJ_VNUM_BRAINS;
		}
		break;
	}

	act(msg, ch, NULL, NULL, TO_ROOM);

	if (vnum) {
		OBJ_DATA *obj;

		obj = create_obj_of(get_obj_index(vnum), ch->short_descr);
		obj->level = ch->level;
		obj->owner = mlstr_dup(ch->short_descr);
		obj->timer = number_range(4, 7);

		if (obj->pIndexData->item_type == ITEM_FOOD) {
			if (IS_SET(ch->form,FORM_POISON))
				obj->value[ITEM_FOOD_POISON] = 1;
			else if (!IS_SET(ch->form,FORM_EDIBLE))
				SET_BIT(obj->extra_flags, ITEM_NOT_EDIBLE);
		}

		obj_to_room(obj, ch->in_room);
	}

	if (IS_NPC(ch))
		msg = "You hear something's death cry.";
	else
		msg = "You hear someone's death cry.";

	if ((was_in_room = ch->in_room)) {
		for (door = 0; door <= 5; door++) {
			EXIT_DATA *pexit;

			if ((pexit = was_in_room->exit[door]) != NULL
			&&   pexit->to_room.r != NULL
			&&   pexit->to_room.r != was_in_room) {
				ch->in_room = pexit->to_room.r;
				act(msg, ch, NULL, NULL, TO_ROOM);
			}
		}
		ch->in_room = was_in_room;
	}
}

/*
 * send defeated character back to altar w/ all affects stripped
 */
void raw_duel_defeat (CHAR_DATA *ch, CHAR_DATA *victim) 
{
	if (IS_NPC(ch))
		remove_mind(ch, victim->name);

	stop_fighting(victim, TRUE);

	RESET_FIGHT_TIME(victim);

	while (victim->affected)
	affect_remove(victim, victim->affected);
		victim->affected_by	= 0;

	victim->position = POS_RESTING;

	victim->hit		= victim->max_hit / 10;
	victim->mana		= victim->max_mana / 10;
	victim->move		= victim->max_move;
	update_pos(victim);

	char_from_room(victim);
	char_to_room(victim, get_altar(victim)->room);
	WAIT_STATE(victim, PULSE_VIOLENCE * 1);
}

OBJ_DATA * raw_kill_org(CHAR_DATA *ch, CHAR_DATA *victim, int part, bool is_duel)
{
	CHAR_DATA *tmp_ch, *tmp_ch_next;
	OBJ_DATA *obj,*obj_next;
	int i;
	OBJ_DATA *tattoo, *clanmark, *corpse;

	for (obj = victim->carrying;obj != NULL;obj = obj_next) {
		obj_next = obj->next_content;
		if (obj->wear_loc != WEAR_NONE
		&&  oprog_call(OPROG_DEATH, obj, victim, NULL)) {
			victim->position = POS_STANDING;
			return NULL;
		}
	}

	/* don't remember killed victims anymore */
	if (IS_NPC(ch))
		remove_mind(ch, victim->name);

	stop_fighting(victim, TRUE);
	if (!is_duel) rating_update(ch, victim);
	quest_handle_death(ch, victim);
	RESET_FIGHT_TIME(victim);
	victim->last_death_time = current_time;
	death_cry_org(victim, part);

	tattoo = get_eq_char(victim, WEAR_TATTOO);
	clanmark = get_eq_char(victim, WEAR_CLANMARK);

	if (tattoo != NULL)
		obj_from_char(tattoo);
	if (clanmark != NULL)
		obj_from_char(clanmark);

	corpse = make_corpse(victim);

	if (victim == ch || is_duel) 
		corpse->killer = NULL;
	else {
		if (IS_NPC(ch) 
		&& ch->master != NULL
		&& ch->in_room == ch->master->in_room)
			corpse->killer = mlstr_dup(ch->master->short_descr);
		else
			corpse->killer = mlstr_dup(ch->short_descr);
	}
	
	if (IS_NPC(victim)) {
		victim->pIndexData->killed++;
		extract_char(victim, 0);
		return corpse;
	}

	while (victim->affected)
	affect_remove(victim, victim->affected);
		victim->affected_by	= 0;

	SET_BIT(victim->state_flags, STATE_GHOST);
	SET_BIT(victim->affected_by, AFF_FLYING);

	if (!is_duel && !IS_SET(victim->acct_flags, ACCT_TRUE_LIFER)) {
		char_puts("You turn into an invincible ghost for a few minutes.\n"
			  "As long as you don't attack anything.\n",
			  victim);
		SET_BIT(victim->affected_by, AFF_PASS_DOOR);
	}

	/*need to do something about pets first*/
	if (is_duel)
		quest_cancel(victim);
	else
		disband_pets(victim);


	extract_char(victim, XC_F_INCOMPLETE);

	for (i = 0; i < 4; i++)
		victim->armor[i] = 100;

	victim->position	= POS_RESTING;
	victim->hit		= victim->max_hit / 10;
	victim->mana		= victim->max_mana / 10;
	victim->move		= victim->max_move;
	update_pos(victim);

	/* RT added to prevent infinite deaths */
	REMOVE_BIT(victim->state_flags, STATE_BOUGHT_PET);

	victim->pcdata->condition[COND_THIRST] = 40;
	victim->pcdata->condition[COND_HUNGER] = 40;
	victim->pcdata->condition[COND_FULL] = 40;
	victim->pcdata->condition[COND_BLOODLUST] = 40;
	victim->pcdata->condition[COND_DESIRE] = 40;

	WAIT_STATE(victim, FIGHT_DELAY_TIME);

	if (tattoo != NULL) {
		obj_to_char(tattoo, victim);
		equip_char(victim, tattoo, WEAR_TATTOO);
	}

	if (clanmark != NULL) {
		obj_to_char(clanmark, victim);
		equip_char(victim, clanmark, WEAR_CLANMARK);
	}

	if (victim->level > 1)
		save_char_obj(victim, FALSE);

	/*
	 * Calm down the tracking mobiles
	 */
	for (tmp_ch = npc_list; tmp_ch; tmp_ch = tmp_ch_next) {
		tmp_ch_next = tmp_ch->next;
		if (tmp_ch->last_fought == victim)
			tmp_ch->last_fought = NULL;
		remove_mind(tmp_ch, victim->name);
		if (tmp_ch->target == victim 
		&&  tmp_ch->pIndexData->vnum == MOB_VNUM_STALKER) {
			doprintf(do_clan, tmp_ch,
				"%s is dead and I can leave the realm.",
				PERS(victim, tmp_ch));
			extract_char(tmp_ch, 0);
		}
	}

	if (!IS_NPC(victim))
		victim->pcdata->saved_stalkers = 0;

	return corpse;
}

void group_gain(CHAR_DATA *ch, CHAR_DATA *victim)
{
	CHAR_DATA *lch;
	CHAR_DATA *gch;
	int xp;
	int members;
	int group_levels;
	int morale = 0;
	OBJ_DATA *obj = NULL;

	/* no exp for PKs or suicide */
	if (!IS_NPC(victim) || victim == ch)
		return;

	/* no exp for killing summoned or bought pets */
	if (IS_SET(victim->pIndexData->act, ACT_PET)
	||  victim->pIndexData->vnum < 100
	||  victim->master
	||  victim->leader)
		return;

	lch = ch->leader ? ch->leader : ch;

	members = 0;
	group_levels = 0;
	morale = 0;
	for (gch = ch->in_room->people; gch; gch = gch->next_in_room) {
		if (is_same_group(gch, ch)) {
			if (IS_NPC(gch)) {
				/* don't count WARLOCK | WITCH charmies */
				if (gch->master
				&& IS_SET(gch->pIndexData->act, ACT_SUMMONED)
				&& !IS_SET(gch->pIndexData->act, ACT_PET)
				&& is_same_group(gch->master, ch))
					continue;

				if (gch->master 
				&& !IS_SET(gch->pIndexData->act, ACT_PET)
				&& !IS_SET(gch->pIndexData->act, ACT_SUMMONED)
				&& (gch->master->class == CLASS_WARLOCK
				   || gch->master->class == CLASS_WITCH)
				&& is_same_group(gch->master, ch))
					continue;
			}
			else {
				if (ABS(gch->level - lch->level) <= 8)
					members++;
			}
			group_levels += gch->level;
		}
	}

	for (gch = ch->in_room->people; gch; gch = gch->next_in_room) {
		if (!is_same_group(gch, ch) || IS_NPC(gch))
			continue;

		if (gch->level - lch->level > 8) {
			char_puts("You are too high for this group.\n", gch);
			continue;
		}

		if (gch->level - lch->level < -8) {
			char_puts("You are too low for this group.\n", gch);
			continue;
		}

		morale = (get_curr_stat(lch, STAT_CHA) 
			+ get_curr_stat(gch, STAT_CHA)) / 2;

		if (gch == lch && members > 1)
			morale+= 5;

		xp = xp_compute(gch, victim, group_levels, members, morale);

		if (gch->level < LEVEL_HERO) {
			if(IS_SET(ch->state_flags, STATE_NOEXP)) {
				char_puts("The gods forbid you to"
					" to gain experience!\n", ch);
				xp = 0;
			}
			else if (is_affected(gch, gsn_dishonor)) {
				char_puts("One without honor cannot"
					" advance in the way of the"
					" samurai.\n", ch);
				continue;
			}
			else {
				char_printf(gch, "You receive %d"
						" experience points.\n", xp);
				gain_exp(gch, xp);
			}
		}
		/* augmentation exp for the weapon or focus item */
		else if ((!IS_IMMORTAL(ch) || IS_TRUSTED_IMP(ch))
		&& (obj = get_augment_obj(ch)) != NULL) {
			if(IS_SET(ch->state_flags, STATE_NOEXP)) {
				char_puts("The gods forbid you to"
					" to gain experience!\n", ch);
				xp = 0;
			}
			else if (is_affected(gch, gsn_dishonor)) {
				char_puts("One without honor cannot"
					" advance one's weapon.\n",
					ch);
				continue;
			}
			else {
				char_printf(ch, 
					"%s gains %d experience points.\n",
					mlstr_mval(obj->short_descr),
					xp);
				gain_obj_exp(ch, obj, xp);
			}
		}

		if (IS_NEWBIE(gch) && gch->level == MAX_NEWBIE_LEVEL) {
			char_puts("Sorry, you have reached the maximum level for a newbie.\n"
				  "If you feel you have learned enough about our realm please\n"
				  "Delete this character and create an non-newbie character to\n"
				  "experience everything this realm has to offer.\n", ch);
		}

	}
}

/*
 * Compute xp for a kill.
 * Also adjust alignment of killer.
 * Edit this function to change xp computations.
 */
int xp_compute(CHAR_DATA *gch, CHAR_DATA *victim, int total_levels, int members, 
		int morale)
{
	int xp;
	int base_exp;
	int level_range = victim->level - gch->level;
	WORLD_AFFECT_DATA *waff = NULL;
	int bonus_align = 0,
	    bonus_morale = 0,
	    bonus_waff = 0,
	    bonus_group = 0,
	    bonus_act = 0,
	    group_split = 0,
	    raw_exp = 0,
	    i, killed_sum = 0;

/* base exp */
	switch (level_range) {
	case -9:	base_exp =   1; 	break;
	case -8:	base_exp =  10; 	break;
	case -7:	base_exp =  20; 	break;
	case -6:	base_exp =  30; 	break;
	case -5:	base_exp =  40; 	break;
	case -4:	base_exp =  50; 	break;
	case -3:	base_exp =  60; 	break;
	case -2:	base_exp =  75; 	break;
	case -1:	base_exp =  87; 	break;
	case  0:	base_exp = 100; 	break;
	case  1:	base_exp = 115; 	break;
	case  2:	base_exp = 130; 	break;
	case  3:	base_exp = 140; 	break;
	case  4:	base_exp = 150; 	break;
	default:
		/* the cap is for occations where for some
		 * reason a hard mob is made easy
		 */
		if (level_range > 10)
			base_exp = 180 + 10 * (level_range - 4);
		else if (level_range > 4)
			base_exp = 150 + 20 * (level_range - 4);
		else
			base_exp = 0;

		if (level_range > 15 * (gch->level /30+1))
			BUG("%s[%d]+%d defeated %s[%d] for %d exp!",
				gch->name,
				gch->level,
				members,
				victim->name,
				victim->level,
				base_exp);

	}

	bonus_act = 0;
	if (IS_NPC(victim) && level_range >= 1) {
		if (IS_SET(victim->pIndexData->affected_by, AFF_SANCTUARY)
		|| IS_SET(victim->pIndexData->affected_by, AFF_BLACK_SHROUD))
			bonus_act += 30;
		else if (IS_SET(victim->pIndexData->affected_by, AFF_MINOR_SANCTUARY)
			|| IS_SET(victim->pIndexData->affected_by, AFF_MINOR_BLACK_SHROUD))
			bonus_act += 15;
		if (IS_SET(victim->pIndexData->act, ACT_WARRIOR))
			bonus_act += 15;
		if (victim->pIndexData->spec_fun == spec_cast_mage
		|| victim->pIndexData->spec_fun == spec_cast_cleric)
			bonus_act += 20;
		else if (victim->pIndexData->spec_fun == spec_breath_any
		|| victim->pIndexData->spec_fun == spec_breath_acid
		|| victim->pIndexData->spec_fun == spec_breath_fire
		|| victim->pIndexData->spec_fun == spec_breath_frost
		|| victim->pIndexData->spec_fun == spec_breath_lightning
		|| victim->pIndexData->spec_fun == spec_breath_gas)
			bonus_act += 35;
	}

/* calculate exp multiplier */
	if (IS_NPC(victim) && IS_SET(victim->pIndexData->act, ACT_NOALIGN))
		xp = base_exp;
	else if ((IS_EVIL(gch) && IS_GOOD(victim))
	||  (IS_EVIL(victim) && IS_GOOD(gch)))
		xp = base_exp * 8/5;
	else if (IS_GOOD(gch) && IS_GOOD(victim))
		xp = 0;
	else if (!IS_NEUTRAL(gch) && IS_NEUTRAL(victim))
		xp = base_exp * 1.0;
	else if (IS_NEUTRAL(gch) && !IS_NEUTRAL(victim))
		xp = base_exp * 1.3;
	else
		xp = base_exp;

	bonus_align = xp - base_exp;

	xp += bonus_act;

	/* morale */
	if (morale < 50)
		xp += bonus_morale = xp * (morale - 50) * 3 / 100;
	else
		xp += bonus_morale = xp * (morale - 50) * 2 / 100;

	/* adjust for grouping */
	xp = xp * gch->level/total_levels;
	group_split = xp;
	if (members == 2 || members == 3 || members == 4) {
		xp *= members;
	}
	bonus_group = xp - group_split;

	/*
	 * WAFF_EXP - modifies experience gain
	 */
	bonus_waff = xp;
	if ((waff = ch_waffected(gch, WAFF_EXP)) != NULL) {
		if (number_percent() <= waff->chance)
			xp = xp * waff->modifier / 100;
	}
	bonus_waff = xp - bonus_waff;

	raw_exp = xp;

	/* randomize the rewards */
	xp = number_range(xp * 3/4, xp * 5/4);

#if 0
	xp += (xp * (gch->max_hit - gch->hit)) / (gch->max_hit * 5);
#endif

	DEBUG(DEBUG_EXP,
		"%s[%d]+%d(%d) killed %s[%d] for %dexp"
		" (raw %d - base %d align %d act %d morale %d split %d"
		" group %d waff %d)",
		gch->name,
		gch->level,
		members -1,
		total_levels,
		victim->name,
		victim->level,
		xp,
		raw_exp,
		base_exp,
		bonus_align,
		bonus_act,
		bonus_morale,
		group_split,
		bonus_group,
		bonus_waff);

	/*
	 * Record of kills by Alignment
	 */

	if (IS_GOOD(victim))
		gch->pcdata->align_killed[ALIGN_INDEX_GOOD]++;
	else if (IS_EVIL(victim))
		gch->pcdata->align_killed[ALIGN_INDEX_EVIL]++;
	else if (IS_NEUTRAL(victim))
		gch->pcdata->align_killed[ALIGN_INDEX_NEUTRAL]++;
	else
		gch->pcdata->align_killed[ALIGN_INDEX_NONE]++;

	for (i=0; i< ALIGN_INDEX_MAX; i++)
		killed_sum += gch->pcdata->align_killed[i];

	if ((killed_sum % 100) == 0)
		char_printf(gch, 
		"You have taken %d angelic, %d balanced and %d demonic souls up to now.\n",
			    gch->pcdata->align_killed[ALIGN_INDEX_GOOD],
			    gch->pcdata->align_killed[ALIGN_INDEX_NEUTRAL],
			    gch->pcdata->align_killed[ALIGN_INDEX_EVIL]);

	if (IS_GOOD(gch) && IS_EVIL(victim)
	&& gch->pcdata->align_killed[ALIGN_INDEX_EVIL] == 1000
	&& gch->perm_stat[STAT_CHA] < get_max_train(gch, STAT_CHA)) {
		char_puts("Word has spread of your good deeds.\n"
			  "Your charisma has increased by {Yone{x!\n",gch);
		gch->perm_stat[STAT_CHA] += 1;
	}
	else if (IS_GOOD(gch) && IS_GOOD(victim)
	&& gch->pcdata->align_killed[ALIGN_INDEX_GOOD] == 25 
	&& gch->perm_stat[STAT_CHA] > 3) {
		char_puts("Word has spread of your evil deeds.\n"
			  "Your charisma has decreased by {rone{x!\n",gch);
		gch->perm_stat[STAT_CHA] -= 1;
	}

	return xp;
}

void dam_message(CHAR_DATA *ch, CHAR_DATA *victim,
		 int dam, int dt, bool immune, int dam_type)
{
	const char *vs;
	const char *vp;
	const char *msg_char;
	const char *msg_vict = NULL;
	const char *msg_notvict;
	const char *attack = str_empty;

	if (dam == 0) {
		vs = "miss";
		vp = "misses";
	}
	else if (dam <= 4) {
		vs = "{cscratch{x";
		vp = "{cscratches{x";
	}
	else if (dam <= 8) {
		vs = "{cgraze{x";
		vp = "{cgrazes{x";
	}
	else if (dam <= 12) {
		vs = "{chit{x";
		vp = "{chits{x";
	}
	else if (dam <= 16) {
		vs = "{cinjure{x";
		vp = "{cinjures{x";
	}
	else if (dam <= 20) {
		vs = "{cwound{x";
		vp = "{cwounds{x";
	}
	else if (dam <= 24) {
		vs = "{cmaul{x";
		vp = "{cmauls{x";
	}
	else if (dam <= 28) {
		vs = "{cdecimate{x";
		vp = "{cdecimates{x";
	}
	else if (dam <= 32) {
		vs = "{cdevastate{x";
		vp = "{cdevastates{x";
	}
	else if (dam <= 36) {
		vs = "{cmaim{x";
		vp = "{cmaims{x";
	}
	else if (dam <= 42) {
		vs = "{MMUTILATE{x";
		vp = "{MMUTILATES{x";
	}
	else if (dam <= 52) {
		vs = "{MDISEMBOWEL{x";
		vp = "{MDISEMBOWELS{x";
	}
	else if (dam <= 65) {
		vs = "{MDISMEMBER{x";
		vp = "{MDISMEMBERS{x";
	}
	else if (dam <= 80) {
		vs = "{MMASSACRE{x";
		vp = "{MMASSACRES{x";
	}
	else if (dam <= 100) {
		vs = "{MMANGLE{x";
		vp = "{MMANGLES{x";
	}
	else if (dam <= 130) {
		vs = "{y** DEMOLISH **{x";
		vp = "{y** DEMOLISHES **{x";
	}
	else if (dam <= 175) {
		vs = "{y*** DEVASTATE ***{x";
		vp = "{y*** DEVASTATES ***{x";
	}
	else if (dam <= 250) {
		vs = "{y== OBLITERATE =={x";
		vp = "{y== OBLITERATES =={x";
	}
	else if (dam <= 325) {
		vs = "{y=== ATOMIZE ==={x";
		vp = "{y=== ATOMIZES ==={x";
	}
	else if (dam <= 400) {
		vs = "{R>> ANNIHILATE <<{x";
		vp = "{R>> ANNIHILATES <<{x";
	}
	else if (dam <= 500) {
		vs = "{R>>> ERADICATE <<<{x";
		vp = "{R>>> ERADICATES <<<{x";
	}
	else if (dam <= 650) {
		vs = "{R-=> ELECTRONIZE <=-{x";
		vp = "{R-=> ELECTRONIZES <=-{x";
	}
	else if (dam <= 800) {
		vs = "{R-==> SKELETONIZE <==-{x";
		vp = "{R-==> SKELETONIZES <==-{x";
	}
	else if (dam <= 1000) {
		vs = "{R## NUKE ##{x";
		vp = "{R## NUKES ##{x";
	}
	else if (dam <= 1250) {
		vs = "{R### TERMINATE ###{x";
		vp = "{R### TERMINATES ###{x";
	}
	else if (dam <= 1500) {
		vs = "{R[*] TEAR UP [*]{x";
		vp = "{R[*] TEARS UP [*]{x";
	}
	else {
		vs = "{*{R[*] POWER HIT [*]{x";
		vp = "{*{R[*] POWER HITS [*]{x";
	}

	if (dt == TYPE_HIT || dt == TYPE_HUNGER) {
		if (ch == victim) {
			switch (dam_type) {
			case DAM_HUNGER:
				msg_notvict = "$n's hunger $u $mself!";
				msg_char = "Your hunger $u yourself!";
				break;

			case DAM_THIRST:
				msg_notvict = "$n's thirst $u $mself!";
				msg_char = "Your thirst $u yourself!";
				break;

			case DAM_LIGHT_V:
				msg_notvict = "The light of the room $u $n!";
				msg_char = "The light of the room $u you!";
				break;

			case DAM_RANGER_TRAP:
				msg_notvict = "The hidden spear $u $n!";
				msg_char = "The hidden spear $u you!";
				break;

			case DAM_TRAP_ROOM:
				msg_notvict = "The trap at room $u $n!";
				msg_char = "The trap at room $u you!";
				break;

			default:
				msg_notvict = "$n $u $mself!";
				msg_char = "You $u yourself!";
				break;
			}
		}
		else {
			msg_notvict = "$n $u $N.";
			msg_char = "You $u $N.";
			msg_vict = "$n $u you.";
		}
	}
	else {
		skill_t *sk;

/* XXX */
#define MAX_DAMAGE_MESSAGE 40
		if ((sk = skill_lookup(dt)))
			attack	= sk->noun_damage;
		else if (dt >= TYPE_HIT && dt <= TYPE_HIT + MAX_DAMAGE_MESSAGE)
			attack	= attack_table[dt - TYPE_HIT].noun;
		else {
			bug("Dam_message: bad dt %d.", dt);
			dt = TYPE_HIT;
			attack = attack_table[0].name;
		}

		if (immune) {
			if (ch == victim) {
				msg_notvict = "$n is unaffected by $s own $U.";
				msg_char = "Luckily, you are immune to that.";
			}
			else {
				msg_notvict = "$N is unaffected by $n's $U!";
				msg_char = "$N is unaffected by your $U!";
				msg_vict = "$n's $U is powerless against you.";
			}
		}
		else {
			vs = vp;

			if (ch == victim) {
				msg_notvict = "$n's $U $u $m.";
				msg_char = "Your $U $u you.";
			}
			else {
				msg_notvict = "$n's $U $u $N.";
				msg_char = "Your $U $u $N.";
				msg_vict = "$n's $U $u you.";
			}
		}
	}

	if (ch == victim) {
		act_puts3(msg_notvict, ch, vp, NULL, attack,
			  TO_ROOM | ACT_TRANS, POS_RESTING);
		act_puts3(msg_char, ch, vs, NULL, attack,
			  TO_CHAR | ACT_TRANS, POS_RESTING);
	}
	else {
		act_puts3(msg_notvict, ch, vp, victim, attack,
			  TO_NOTVICT | ACT_TRANS, POS_RESTING);
		act_puts3(msg_char, ch, vs, victim, attack,
			  TO_CHAR | ACT_TRANS, POS_RESTING);
		act_puts3(msg_vict, ch, vp, victim, attack,
			  TO_VICT | ACT_TRANS, POS_RESTING);
	}
}

void do_kill(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int chance;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Kill whom?\n", ch);
		return;
	}

	WAIT_STATE(ch, 1 * PULSE_VIOLENCE);

	if ((victim = get_char_room(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (ch->position == POS_FIGHTING) {
		if (victim == ch->fighting)
			char_puts("You do the best you can!\n", ch);
		else if (victim->fighting != ch)
			char_puts("One battle at a time, please.\n",ch);
		else {
			char_puts("First dispatch the foe infront of you.\n",ch);
		}
		return;
	}

	if (!IS_NPC(victim)) {
		char_puts("You must MURDER a player.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("You hit yourself.  Ouch!\n", ch);
		multi_hit(ch, ch, TYPE_UNDEFINED);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
		act("$N is your beloved master.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (is_safe(ch, victim))
		return;

	if ((chance = get_skill(ch, gsn_mortal_strike))
	&&  (get_eq_char(ch, WEAR_WIELD) || HAS_SKILL(ch, gsn_master_hand))
	&&  ch->level > (victim->level - 5)) {
		chance /= 30;
		chance += 1 + (ch->level - victim->level) / 2;
		if (number_percent() < chance) {
			act_puts("Your flash strike instantly slays $N!",
				 ch, NULL, victim, TO_CHAR, POS_RESTING);
			act_puts("$n flash strike instantly slays $N!",
				 ch, NULL, victim, TO_NOTVICT,
				 POS_RESTING);
			act_puts("$n flash strike instantly slays you!",
				 ch, NULL, victim, TO_VICT, POS_DEAD);
			damage(ch, victim, (victim->hit + 1),
			       gsn_mortal_strike, DAM_NONE, DAMF_SHOW);
			check_improve(ch, gsn_mortal_strike, TRUE, 1);
			return;
		} else
			check_improve(ch, gsn_mortal_strike, FALSE, 3);
	}

	multi_hit(ch, victim, TYPE_UNDEFINED);
}

void do_murde(CHAR_DATA *ch, const char *argument)
{
	char_puts("If you want to MURDER, spell it out.\n", ch);
	return;
}

void do_murder(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int chance;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Murder whom?\n", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM)
	||  (IS_NPC(ch) && (IS_SET(ch->pIndexData->act, ACT_PET) && !is_affected(ch, gsn_rebel))))
		return;

	if ((victim = get_char_room(ch, arg)) == NULL) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("Suicide is a mortal sin.\n", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
		act("$N is your beloved master.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (ch->position == POS_FIGHTING) {
		char_puts("You do the best you can!\n", ch);
		return;
	}

	if (is_safe(ch, victim))
		return;

	WAIT_STATE(ch, 1 * PULSE_VIOLENCE);

	if ((chance = get_skill(ch, gsn_mortal_strike))
	&&  get_eq_char(ch, WEAR_WIELD)
	&&  LEVEL(ch) > (LEVEL(victim) - 5)) {
		chance /= 30;
		chance += 1 + (LEVEL(ch) - LEVEL(victim)) / 2;

		DEBUG(DEBUG_SKILL_MORTAL_STRIKE,
			"mortal_strike: %s[%d] vs %s[%d] %d%%",
			ch->name, LEVEL(ch), 
			victim->name, LEVEL(victim),
			chance);

		if (number_percent() < chance) {
			act_puts("Your flash strike mortally wounds $N!",
				 ch, NULL, victim, TO_CHAR, POS_RESTING);
			act_puts("$n flash strike mortally wounds $N!",
				 ch, NULL, victim, TO_NOTVICT,
				 POS_RESTING);
			act_puts("$n flash strike mortally wounds you!",
				 ch, NULL, victim, TO_VICT, POS_DEAD);
			damage(ch, victim, (victim->hit/2),
			       gsn_mortal_strike, DAM_NONE, DAMF_SHOW);
			check_improve(ch, gsn_mortal_strike, TRUE, 1);
			return;
		} else
			check_improve(ch, gsn_mortal_strike, FALSE, 3);
	}

	multi_hit(ch, victim, TYPE_UNDEFINED);
}

void do_flee(CHAR_DATA *ch, const char *argument)
{
	ROOM_INDEX_DATA *was_in;
	ROOM_INDEX_DATA *now_in;
	CHAR_DATA *victim;
	int attempt;
	class_t *cl;

	if (RIDDEN(ch)) {
		char_puts("You should ask to your rider!\n", ch);
		return;
	}

	if (MOUNTED(ch))
		do_dismount(ch, str_empty);

	if ((victim = ch->fighting) == NULL) {
		if (ch->position == POS_FIGHTING)
			ch->position = POS_STANDING;
		char_puts("You aren't fighting anyone.\n", ch);
		return;
	}

	if ((cl = class_lookup(ch->class))
	&&  !CAN_FLEE(ch, cl)) {
		 char_puts("Your honour doesn't let you flee, "
			   "try dishonoring yourself.\n", ch);
		 return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE/2);

	was_in = ch->in_room;
	for (attempt = 0; attempt < 6; attempt++) {
		EXIT_DATA *pexit;
		int door;

		door = number_door();
		if ((pexit = was_in->exit[door]) == 0
		     || pexit->to_room.r == NULL
		     || (IS_SET(pexit->exit_info, EX_CLOSED)
		         && (!IS_AFFECTED(ch, AFF_PASS_DOOR)
		             || IS_SET(pexit->exit_info,EX_NOPASS))
		             && !IS_TRUSTED(ch,ANGEL))
		         || (IS_SET(pexit->exit_info , EX_NOFLEE))
		         || (IS_NPC(ch)
		             && IS_SET(pexit->to_room.r->room_flags, ROOM_NOMOB)))
			continue;

		move_char(ch, door, FALSE);
		if ((now_in = ch->in_room) == was_in)
		    continue;

		ch->in_room = was_in;
		act("$n has fled!", ch, NULL, NULL, TO_ROOM);
		ch->in_room = now_in;

		if (!IS_NPC(ch)) {
			act_puts("You fled from combat!",
				 ch, NULL, NULL, TO_CHAR, POS_DEAD);
			if (ch->level < LEVEL_HERO) {
				char_printf(ch, "You lose %d exps.\n", 10);
				gain_exp(ch, -10);
			}
		} else
			ch->last_fought = NULL;

		stop_fighting(ch, TRUE);
		return;
	}

	char_puts("PANIC! You couldn't escape!\n", ch);
	return;
}

void do_sla(CHAR_DATA *ch, const char *argument)
{
	char_puts("If you want to SLAY, spell it out.\n", ch);
	return;
}

/*:======================================================================:
 *| John Strange                                Triad Mud                |
 *| gambit@wvinter.net                          triad.telmaron.com 7777  |
 *:======================================================================:
 */
void do_slay (CHAR_DATA *ch, const char *argument )
{
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg, sizeof(arg));
	one_argument(argument, arg2, sizeof(arg2));

	if ( arg[0] == '\0' ) {
		send_to_char( "Syntax: [Char] [Type]\n", ch );
		send_to_char( "Types: Skin, Slit, Immolate,"
			" Demon, Shatter, Slit, Deheart, Pounce.\n", ch);
		return;
	}

	if (( victim = get_char_room(ch, arg)) == NULL) {
		send_to_char( "They aren't here.\n\r", ch);
		return;
	}

	if (ch == victim) {
		send_to_char( "Suicide is a mortal sin.\n\r", ch );
		return;
	}

	if (IS_IMMORTAL(victim)) {
		char_puts("You failed.\n", ch);
		return;
	}

	if (!str_prefix(arg2, "skin")) {
		act( "You rip the flesh from $N and send $S soul to the"
			" fiery depths of hell.", ch, NULL, victim, TO_CHAR );
		act( "Your flesh has been torn from your bones and your"
			" bodyless soul now watches your bones incenerate"
			" in the fires of hell.", ch, NULL, victim, TO_VICT );
		act( "$n rips the flesh off of $N, releasing $S soul into"
			" the fiery depths of hell.", 
			ch, NULL, victim, TO_NOTVICT );
	}

	else if (!str_prefix(arg2, "deheart")) {
		act( "You rip through $N's chest and pull out $S beating"
			" heart in your hand.", ch, NULL, victim, TO_CHAR );
		act( "You feel a sharp pain as $n rips into your chest"
			" and pulls our your beating heart in $s hand.", 
			ch, NULL, victim, TO_VICT );
		act( "Specks of blood hit your face as $n rips through"
			" $N's chest pulling out $S's beating heart.", 
			ch, NULL, victim, TO_NOTVICT );
	}

	else if (!str_prefix(arg2, "immolate")) {
	act( "Your fireball turns $N into a blazing inferno.",
		ch, NULL, victim, TO_CHAR);
	act( "$n releases a searing fireball in your direction.", 
		ch, NULL, victim, TO_VICT);
	act( "$n points at $N, who bursts into a flaming inferno.",
		ch, NULL, victim, TO_NOTVICT);
	}

	else if (!str_prefix(arg2, "shatter")) {
		act( "You freeze $N with a glance and shatter $S"
			" frozen corpse into tiny shards.", 
			ch, NULL, victim, TO_CHAR    );
		act( "$n freezes you with a glance and shatters"
			" your frozen body into tiny shards.", 
			ch, NULL, victim, TO_VICT    );
		act( "$n freezes $N with a glance and shatters"
			" $S frozen body into tiny shards.",  
			ch, NULL, victim, TO_NOTVICT );
	}

	else if (!str_prefix(arg2, "demon")) {
		act( "You gesture, and a slavering demon appears."
			"  With a horrible grin, the foul creature"
			" turns on $N, who screams in panic before"
			" being eaten alive.",
			ch, NULL, victim, TO_CHAR );
		act( "$n gestures, and a slavering demon appears."
			"  The foul creature turns on you with a"
			" horrible grin.  You scream in panic before"
			" being eaten alive.", 
			ch, NULL, victim, TO_VICT );
		act( "$n gestures, and a slavering demon appears."
			"  With a horrible grin, the foul creature"
			" turns on $N, who screams in panic before"
			" being eaten alive.", 
			ch, NULL, victim, TO_NOTVICT );
	}

	else if (!str_prefix( arg2, "pounce")) {
		act( "Leaping upon $N with bared fangs, you tear open"
			" $S throat and toss the corpse to the ground...",
			ch, NULL, victim, TO_CHAR );
		act( "In a heartbeat, $n rips $s fangs through your throat!"
			"  Your blood sprays and pours to the ground as"
			" your life ends...",
			ch, NULL, victim, TO_VICT );
		act( "Leaping suddenly, $n sinks $s fangs into $N's throat."
			"  As blood sprays and gushes to the ground, $n"
			" tosses $N's dying body over $s shoulder.", 
			ch, NULL, victim, TO_NOTVICT );
	}
	else if (!str_prefix(arg2, "slit")) {
		act( "You calmly slit $N's throat.", 
			ch, NULL, victim, TO_CHAR );
		act( "$n reaches out with a clawed finger and"
			" calmly slits your throat.", 
			ch, NULL, victim, TO_VICT );
		act( "A claw extends from $n's hand as $e"
			" calmly slits $N's throat.",
			ch, NULL, victim, TO_NOTVICT );
	}
	else {
		act("You slay $M in cold blood!", ch, NULL, victim, TO_CHAR);
		act("$n slays you in cold blood!", ch, NULL, victim, TO_VICT);
		act("$n slays $N in cold blood!", ch, NULL, victim, TO_NOTVICT);
	}

	wiznet_death(ch, victim, DEATH_SLAY);
	raw_kill(ch, victim, TRUE);
}


/*
 * Check for obj dodge.
 */
bool check_obj_dodge(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj, int bonus)
{
	int chance;

	if (!IS_AWAKE(victim) || MOUNTED(victim))
		return FALSE;

/*
	if (!IS_NPC(victim)) {
		if (victim->pcdata->clan_status) {
			act("You catch $p that had been shot to you.",
			    ch, obj, victim, TO_VICT);
			act("$N catches $p that had been shot to $M.",
			    ch, obj, victim, TO_CHAR);
			act("$n catches $p that had been shot to $m.",
			    victim, obj, ch, TO_NOTVICT);
			obj_to_char(obj, victim);
		}
		return TRUE;
	}
*/

	if (IS_NPC(victim))
		 chance  = UMIN(30, victim->level);
	else {
		chance  = get_skill(victim, gsn_dodge) / 2;
		/* chance for high dex. */
		chance += 2 * (get_curr_stat(victim, STAT_DEX) - 20);
	}

	chance -= (bonus - 90);
	chance /= 2;
	if (number_percent() >= chance)
		return FALSE;
	if(obj->value[ITEM_WEAPON_TYPE] == WEAPON_DAGGER)
	{
	act("You dodge $p that had been thrown at you.",
	    ch, obj, victim, TO_VICT);
	act("$N dodges $p that had been thrown at $M.",
	    ch, obj, victim, TO_CHAR);
	act("$n dodges $p that had been thrown at $m.",
	    victim, obj, ch, TO_NOTVICT);
	}
	else
	{
	act("You dodge $p that had been shot to you.",
	    ch, obj, victim, TO_VICT);
	act("$N dodges $p that had been shot to $M.",
	    ch, obj, victim, TO_CHAR);
	act("$n dodges $p that had been shot to $m.",
	    victim, obj, ch, TO_NOTVICT);
	}
	obj_to_room(obj, victim->in_room);
	check_improve(victim, gsn_dodge, TRUE, 6);

	return TRUE;
}

void do_dishonor(CHAR_DATA *ch, const char *argument)
{
	ROOM_INDEX_DATA *was_in;
	ROOM_INDEX_DATA *now_in;
	CHAR_DATA *gch;
	int attempt, level = 0, chlevel = 0;
	int sn_dishonor;
	int chance;
	bool dishonored = FALSE;
	AFFECT_DATA af;

	if (RIDDEN(ch)) {
		char_puts("You should ask to your rider!\n", ch);
		return;
	}

	if ((sn_dishonor = sn_lookup("dishonor")) < 0
	||  !HAS_SKILL(ch, sn_dishonor)) {
		char_puts("Which honor?\n", ch);
		return;
	}

	if (ch->fighting == NULL) {
		if (ch->position == POS_FIGHTING)
			ch->position = POS_STANDING;
		char_puts("You aren't fighting anyone.\n", ch);
		return;
	}

	for (gch = char_list; gch; gch = gch->next) {
		if ((is_same_group(gch, ch->fighting) 
		&& gch->in_room == ch->fighting->in_room)
		|| gch->fighting == ch)
			level += IS_NPC(gch)	? gch->level * 2/3
						: gch->level;
		if (is_same_group(gch, ch) && gch->in_room == ch->in_room)
			chlevel += IS_NPC(gch)	? gch->level * 3/4
						: gch->level;
	}

	if ((ch->fighting->level - ch->level) < 5 + (ch->level / 20) 
	&& chlevel+2 > level) {
		dishonored = TRUE;
	}

	WAIT_STATE(ch, SKILL(gsn_dishonor)->beats);

	was_in = ch->in_room;
	chance = get_skill(ch, sn_dishonor);
	for (attempt = 0; attempt < 6; attempt++) {
		EXIT_DATA *pexit;
		int door;

		if (number_percent() >= chance)
			continue;

		door = number_door();
		if ((pexit = was_in->exit[door]) == 0
		||  pexit->to_room.r == NULL
		||  (IS_SET(pexit->exit_info, EX_CLOSED) &&
		     (!IS_AFFECTED(ch, AFF_PASS_DOOR) ||
		      IS_SET(pexit->exit_info,EX_NOPASS)) &&
		     !IS_TRUSTED(ch,ANGEL))
		|| IS_SET(pexit->exit_info, EX_NOFLEE)
		|| (IS_NPC(ch) &&
		    IS_SET(pexit->to_room.r->room_flags, ROOM_NOMOB)))
			continue;

		move_char(ch, door, FALSE);
		if ((now_in = ch->in_room) == was_in)
			continue;

		ch->in_room = was_in;
		act("$n has dishonored $mself!",
		    ch, NULL, NULL, TO_ROOM);
		ch->in_room = now_in;

		if (!IS_NPC(ch)) {
			if (dishonored) {
				char_puts("You have dishonored yourself"
					" by fleeing like a coward. "
					" You are no samurai!\n",
					ch);

				if (!is_affected(ch, gsn_dishonor)) {
					af.where	= TO_AFFECTS;
					af.type		= gsn_dishonor;
					af.level	= ch->level;
					af.duration	= -1;
					af.bitvector	= 0;

					af.location	= APPLY_LEVEL;
					af.modifier	= -2;
					affect_to_char(ch, &af);

					af.location	= APPLY_CHA;
					af.modifier	= -3;
					affect_to_char(ch, &af);

					if (ch->level < LEVEL_HERO) {
						char_printf(ch, "You lose %d exps.\n",
							    (ch->level /50 + 1) * 1000);
						gain_exp(ch, (ch->level /50+1) * -1000);
					}

					if ((!IS_NPC(ch->fighting) && number_percent() > 50)
					|| (IS_NPC(ch->fighting) && number_percent() > 90)) {
						char_printf(ch, 
							"You suffer a {rserious{x loss of face.\n");
						ch->perm_stat[STAT_CHA]--;
					}
				}
				else {
					char_puts("Maybe you should"
						" reconsider your dedication"
						" to the bushido!\n",ch);
				}

				DEBUG(DEBUG_HONOR,
					"%s[%d](%d) dishonoring "
					"against level [%d](%d): dishonor",
					ch->name,
					ch->level,
					chlevel,
					ch->fighting->level,
					level);
			}
			else {
				char_puts("You tactically retreat from"
					" unsurmountable odds.\n",
					ch);
				if (ch->level < LEVEL_HERO) {
					char_printf(ch, "You lose %d exps.\n",
						    ch->level);
					gain_exp(ch, -(ch->level));
				}
				DEBUG(DEBUG_HONOR,
					"%s[%d](%d) dishonoring "
					"against level [%d](%d): retreat",
					ch->name,
					ch->level,
					chlevel,
					ch->fighting->level,
					level);
			}

		}
		else
			ch->last_fought = NULL;

		stop_fighting(ch, TRUE);

		if (MOUNTED(ch))
			do_dismount(ch,str_empty);

		check_improve(ch, sn_dishonor, TRUE, 1);
		return;
	}

	DEBUG(DEBUG_HONOR,
		"%s[%d](%d) dishonoring "
		"against level [%d](%d): failed",
		ch->name,
		ch->level,
		chlevel,
		ch->fighting->level,
		level);

	char_puts("PANIC! You couldn't escape!\n", ch);
	check_improve(ch, sn_dishonor, FALSE, 1);
}

void do_surrender(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *mob;

	if (!IS_NPC(ch)) {
		char_puts("Huh?\n", ch);
		return;
	}

	if ((mob = ch->fighting) == NULL) {
		char_puts("But you're not fighting!\n", ch);
		return;
	}
	act("You surrender to $N!", ch, NULL, mob, TO_CHAR);
	act("$n surrenders to you!", ch, NULL, mob, TO_VICT);
	act("$n tries to surrender to $N!", ch, NULL, mob, TO_NOTVICT);
	stop_fighting(ch, TRUE);
 
	if (!IS_NPC(ch) && IS_NPC(mob) 
	&&  (!HAS_TRIGGER(mob, TRIG_SURR) ||
	     !mp_percent_trigger(mob, ch, NULL, NULL, TRIG_SURR))) {
		act("$N seems to ignore your cowardly act!",
		    ch, NULL, mob, TO_CHAR);
		multi_hit(mob, ch, TYPE_UNDEFINED);
	}
}

/*
 * handle_witch_curse - if the witch has died, the curse ends
 * by Zsuzsu
 */
void handle_witch_curse (CHAR_DATA *ch, CHAR_DATA *victim)
{
	AFFECT_DATA *pAf = NULL;
	DESCRIPTOR_DATA *d;

	if (victim->cursed_by_witch) {
		act("You sense $N has succumb to {Ddeath{x.",
			victim->cursed_by_witch,
			NULL, victim, TO_CHAR);
		victim->cursed_by_witch = NULL;
	}

	for (d = descriptor_list; d; d = d->next) {
		CHAR_DATA *fch = d->character;

		if (fch != NULL
		&&  fch != victim
		&&  d->connected == CON_PLAYING
		&&  !IS_NPC(fch)
		&&  fch->cursed_by_witch == victim) {
			fch->cursed_by_witch = NULL;
			
			act("You feel $N's {Dcurse{x is lifted!",
				fch, NULL, victim, TO_CHAR);
			act("You sense $N has wrest $Mself free of your curse.",
				victim, NULL, fch, TO_CHAR);
			pAf = affect_find(fch->affected, gsn_witch_curse);
			affect_remove(fch, pAf);
		}
	}
}

/*
 * enslave someone instead of killing them if ch has autoenslave on.
 * and free slaves of the victim
 *
 * returns TRUE if enslavement occured
 *
 * by Zsuzsu
 */
bool handle_enslavement (CHAR_DATA *ch, CHAR_DATA *victim)
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *enslaver = NULL;

	if (IS_NPC(victim))
		return FALSE;

	enslaver = ch;
	/* if the pet did the kill */
	if (IS_NPC(enslaver) 
	&& enslaver->master != NULL
	&& !IS_SET(enslaver->master->state_flags, STATE_GHOST)
	&& enslaver->in_room == enslaver->master->in_room)
		enslaver = enslaver->master;

	if (!IS_NPC(enslaver)
	&& enslaver != victim
	&& IS_SET(enslaver->conf_flags, PLR_CONF_AUTO_CLAN_SKILL)
	&& get_skill(enslaver, gsn_enslave) > 0) {

		if (victim->pcdata->enslaver == NULL) {

			victim->hit = 0;
			victim->mana = 0;
			victim->move = 0;
			victim->position = POS_STUNNED;
			doprintf(do_enslave, enslaver, "%s", victim->name);

			/*SET_BIT(ch->affected_by, AFF_STUN);*/
			/*SET_BIT(victim->state_flags, STATE_GHOST);*/
			victim->last_death_time = current_time;

			stop_fighting(victim, TRUE);

			/* drop all objects on the floor */
			strip_char(victim);
			act_puts("You are stripped of all your belongings.",
				victim, NULL, NULL, TO_CHAR, POS_DEAD);
			act("$n's belongings are stripped from $m and strewn on the ground.",
				victim, NULL, NULL, TO_ROOM);	


			/* pets abandon you so you can't fight back
			 * as quickly */
			act_puts("Your pets seem to want to any part of you.",
				victim, NULL, NULL, TO_CHAR, POS_DEAD);	
			disband_pets(victim);

			/* remove all affects */
			while (victim->affected)
				affect_remove(victim, victim->affected);
			victim->affected_by     = 0;

			wiznet_death(ch, victim, DEATH_ENSLAVED);

			rating_update(enslaver, victim);

			return TRUE;
		}

		else if (!str_cmp(enslaver->name, victim->pcdata->enslaver)) {
			/* attempting not to double credit people for enslavement kills*/
			if (current_time - victim->last_death_time >= GHOST_DELAY_TIME / 3)
				rating_update(enslaver, victim);
		}
		else {
			char_printf(ch, "You just killed %s's property!", 
				victim->pcdata->enslaver);
		}
	}

	/* free slaves */

	/* free victim if the attacker was an enslaver */
	if (get_skill(enslaver, gsn_enslave) > 0
	&& victim->pcdata->enslaver != NULL) {
		enslaver = get_char_world_unrestricted(victim->pcdata->enslaver);
		if (enslaver)
			doprintf(do_enslave, enslaver, "%s", victim->name);
		else {
			victim->pcdata->enslaver = NULL;
			act_puts("You are unleashed from bondage!",
				victim, NULL, NULL, TO_CHAR, POS_DEAD);
		}
	}

	/* free slaves of the victim */
	/* can't just do this for those with the skill, because they
	 * might be out of scion */
	for (d = descriptor_list; d; d = d->next) {
		CHAR_DATA *fch = d->character;

		if (fch != NULL
		&&  fch != victim
		&&  d->connected == CON_PLAYING
		&&  !IS_NPC(fch)
		&&  !str_cmp(victim->name, fch->pcdata->enslaver)) {

			if (get_skill(victim, gsn_enslave) > 0)
				doprintf(do_enslave, victim, "%s", fch->name);
			else {
				fch->pcdata->enslaver = NULL;
				act_puts("You are unleashed from bondage!",
					fch, NULL, NULL, TO_CHAR, POS_DEAD);
			}
		}
	}

	return FALSE;
}

/*
 * handle_dishonor - checks to see if a samurai
 * 	has regained honor through solo combat
 * 	against superior odds (or PC peer).
 *
 * 	returns TRUE if honor was regained
 * 		FALSE for all other cases
 *
 * by Zsuzsu
 */
bool handle_dishonor (CHAR_DATA *ch, CHAR_DATA *victim)
{
	CHAR_DATA *gch;
	int op_level = 0;
	AFFECT_DATA *paf;

	if (IS_NPC(ch) || !HAS_SKILL(ch, gsn_dishonor))
		return FALSE;

	if (!is_affected(ch, gsn_dishonor))
		return FALSE;

	for (gch = char_list; gch; gch = gch->next) {
		if (gch != ch 
		&& is_same_group(ch, gch)
		&& ch->in_room == gch->in_room)
			return FALSE;

		if (is_same_group(victim, gch)
		&& ch->in_room == gch->in_room)
			op_level += IS_NPC(gch) 
				? gch->level -1 : gch->level;
	}

	if (op_level >= ch->level) {
		while ((paf = affect_find(ch->affected, gsn_dishonor)))
			affect_remove(ch, paf);
		char_puts("You have regained your honor"
			" this day!\n", ch);
		if (op_level > ch->level+1 && number_percent() > 93) {
			char_printf(ch, 
				"You {Ysave{x face.\n");
			ch->perm_stat[STAT_CHA]++;
		}
		return TRUE;
	}
	return FALSE;
}

/* 
 * determine the chance of any one character parrying 
 * the attacks of another character
 *
 * this has been seperated from check_parry() mostly
 * so it can be called seperately to determine
 * battle odds of one character to another
 */
int parry_chance (CHAR_DATA *ch, CHAR_DATA *victim)
{
	OBJ_DATA *ch_weapons[2] = {get_eq_char(ch, WEAR_WIELD), 
				get_eq_char(ch, WEAR_SECOND_WIELD)};
	OBJ_DATA *v_weapon;
	int chance;
	int i;

	if (IS_NPC(victim)) {
		chance = 40;
		if (IS_SET(victim->pIndexData->act, ACT_WARRIOR))
			chance += 60;

		if (IS_SET(victim->pIndexData->act, ACT_THIEF))
			chance += 30;

		if (IS_SET(victim->pIndexData->act, ACT_MAGE))
			chance -= 20;
	}
	else {
		if (get_eq_char(victim, WEAR_WIELD) == NULL)
			return 0;

		chance = get_skill(victim, gsn_parry);
	}

	if (chance <= 0)
	  return 0;

	chance = chance / 4;

	if (check_forest(victim) == FOREST_DEFENCE
	&&  (number_percent() < get_skill(victim, gsn_forest_fighting))) {
		chance = chance * 120 / 100;
		check_improve (victim, gsn_forest_fighting, TRUE, 7);
	}
	
	if (get_eq_char(ch, WEAR_WIELD)
	&& get_eq_char(ch, WEAR_WIELD)->value[ITEM_WEAPON_TYPE] == WEAPON_FLAIL)
		chance /= 2;

	/* weapon modifications */
	for (i = 0;i < 2;i++) {
		if (ch_weapons[i] == NULL) continue;

		switch (ch_weapons[i]->value[ITEM_WEAPON_TYPE]) {
			case WEAPON_POLEARM:
				chance += 4 / (i+1);
				break;
			case WEAPON_DAGGER:
				chance -= 4 / (i+1);
				break;
		}
	}

	if ((v_weapon = get_eq_char(victim, WEAR_WIELD)) != NULL) {
		switch (v_weapon->value[ITEM_WEAPON_TYPE]) {
			case WEAPON_SHORTSWORD:
				chance += 10;
				break;
			case WEAPON_DAGGER:
				chance -= 10;
				break;
			case WEAPON_AXE:
				chance += 10;
				break;
		}
	}

	chance += (LEVEL(victim) - LEVEL(ch)) / 2;

	/* stat modifications */
	if (!IS_NPC(victim)) {
		chance += (get_curr_stat(victim, STAT_STR) -50) / 5; 
		chance += (get_curr_stat(victim, STAT_DEX) -50) / 5; 
	}

	/* affect modifications */
	if (is_affected(victim, gsn_giant_strength))
		chance += 1;
	if (is_affected(ch, gsn_giant_strength))
		chance -= 1;

	if (is_affected(victim, gsn_dragon_strength))
		chance += 1;
	if (is_affected(ch, gsn_dragon_strength))
		chance -= 1;

	if (IS_AFFECTED(victim, AFF_HASTE))
		chance += 1;
	if (IS_AFFECTED(ch, AFF_HASTE))
		chance -= 1;

	chance = URANGE(0, chance, 50);

	return chance;
}

int dodge_chance (CHAR_DATA *ch, CHAR_DATA *victim)
{
	OBJ_DATA *ch_weapons[2] = {get_eq_char(ch, WEAR_WIELD), 
				get_eq_char(ch, WEAR_SECOND_WIELD)};
	OBJ_DATA *v_weapon;
	int chance;
	int i;

	if (MOUNTED(victim))
		return 0;

	if (IS_NPC(victim)) {
		if (victim->pIndexData->dodge < 1) {
			chance = 60;
			if (IS_SET(victim->pIndexData->act, ACT_WARRIOR))
				chance += 40;

			if (IS_SET(victim->pIndexData->act, ACT_THIEF))
				chance += 60;

			if (IS_SET(victim->pIndexData->act, ACT_MAGE))
				chance -= 40;

			if (IS_SET(victim->pIndexData->off_flags, OFF_FAST))
				chance += 20;
		}
		else
			chance = victim->pIndexData->dodge;
	}
	else {
		chance = get_skill(victim, gsn_dodge);

	}

	chance = chance / 4;

	if (chance <= 0)
		return 0;

	if (!IS_NPC(victim)) {
		chance += (get_curr_stat(ch, STAT_DEX)-55) / 4;

		if (ENCUMBERANCE(victim) > 35)
			chance -= (ENCUMBERANCE(victim) - 35)/2;

		if (ENCUMBERANCE(victim) > 75)
			chance -= (ENCUMBERANCE(victim) - 75);

		if (ENCUMBERANCE(victim) > 90)
			chance = 0;

		if (ENCUMBERANCE(victim) < 15)
			chance += (15 - ENCUMBERANCE(victim)) / 2;
	}


	if (chance <= 0)
		return 0;

	chance += (LEVEL(victim) - LEVEL(ch)) / 2;

	if (IS_AFFECTED(victim, AFF_HASTE))
		chance += 2;

	if (IS_AFFECTED(ch, AFF_HASTE))
		chance -= 2;

	if (check_forest(victim) == FOREST_DEFENCE 
	  && (get_skill(victim, gsn_forest_fighting) > number_percent())) {
		chance = chance * 120 / 100;
		check_improve (victim, gsn_forest_fighting, TRUE, 7);
	}


	/* weapon modifications */
	for (i = 0;i < 2;i++) {
		if (ch_weapons[i] == NULL) continue;

		switch (ch_weapons[i]->value[ITEM_WEAPON_TYPE]) {
			case WEAPON_POLEARM:
				chance += 4 / (i+1);
				break;
			case WEAPON_DAGGER:
				chance -= 4 / (i+1);
				break;
		}
	}

	v_weapon = get_eq_char(victim, WEAR_WIELD);
	
	if (v_weapon != NULL) {
		switch (v_weapon->value[ITEM_WEAPON_TYPE]) {
			case WEAPON_AXE:
				chance -= 2;
				break;
			case WEAPON_DAGGER:
				chance += 2;
				break;
		}
	}

	chance = URANGE(0, chance, 50);

	return chance;
}

int shield_block_chance (CHAR_DATA *ch, CHAR_DATA *victim)
{
	OBJ_DATA *ch_weapons[2] = {get_eq_char(ch, WEAR_WIELD), 
				get_eq_char(ch, WEAR_SECOND_WIELD)};
	int chance = 0;
	int i;

	if (!IS_NPC(victim)
	&& get_eq_char(victim, WEAR_SHIELD) == NULL)
		return 0;

	if (IS_NPC(victim)) {
		if (get_eq_char(victim, WEAR_SHIELD))
			chance = 50;
		if (IS_SET(victim->pIndexData->act, ACT_WARRIOR))
			chance += 50;
	}
	else {
		chance = get_skill(victim, gsn_shield_block);
		if (chance <= 1)
			return 0;
		chance += (get_curr_stat(victim, STAT_STR) - 50)/ 2;

	}	

	chance = chance / 4;

	if (chance <= 0)
		return 0;
	
	if (check_forest(victim) == FOREST_DEFENCE 
	&& (number_percent() < get_skill(victim, gsn_forest_fighting))) {
		chance = chance * 120 / 100;
		check_improve (victim, gsn_forest_fighting, TRUE, 7);
	}

	if (MOUNTED(victim))
		chance = chance * 120 / 100;

	chance += (LEVEL(victim) - LEVEL(ch)) / 4;

	/* weapon modifications */
	if (get_eq_char(ch, WEAR_WIELD)
	 && get_eq_char(ch, WEAR_WIELD)->value[ITEM_WEAPON_TYPE] == WEAPON_WHIP)
		chance /= 2;

	for (i = 0;i < 2;i++) {
		if (ch_weapons[i] == NULL) continue;

		switch (ch_weapons[i]->value[ITEM_WEAPON_TYPE]) {
			case WEAPON_POLEARM:
				chance += 4 / (i+1);
				break;
			case WEAPON_DAGGER:
				chance -= 4 / (i+1);
				break;
		}
	}

	if (is_affected(victim, gsn_giant_strength))
		chance += 2;
	if (is_affected(ch, gsn_giant_strength))
		chance -= 2;

	if (is_affected(victim, gsn_dragon_strength))
		chance += 2;
	if (is_affected(ch, gsn_dragon_strength))
		chance -= 2;

	chance = URANGE(0, chance, 50);

	return chance;
}

int hand_block_chance (CHAR_DATA *ch, CHAR_DATA *victim)
{
	OBJ_DATA *ch_weapons[2] = {get_eq_char(ch, WEAR_WIELD), 
				get_eq_char(ch, WEAR_SECOND_WIELD)};
	int chance;
	int i;
	
	if (IS_NPC(victim) 
	|| (get_eq_char(victim, WEAR_WIELD) && get_eq_char(victim, WEAR_SHIELD))
	|| (get_eq_char(victim, WEAR_WIELD) && get_eq_char(victim, WEAR_HOLD))
	|| (get_eq_char(victim, WEAR_WIELD) && get_eq_char(victim, WEAR_SECOND_WIELD))
	|| ((chance=get_skill(victim, gsn_hand_block)/5)<=0)) 
		return 0;

	chance += (LEVEL(victim) - LEVEL(ch));

	if (!IS_NPC(ch) && !IS_NPC(victim))
		chance += (get_curr_stat(victim, STAT_DEX) 
			- get_curr_stat(ch, STAT_DEX)) / 2;
	else if (!IS_NPC(victim))
		chance += (get_curr_stat(victim, STAT_DEX) -60) / 4;


	/* weapon modifications */
	for (i = 0;i < 2;i++) {
		if (ch_weapons[i] == NULL) continue;

		switch (ch_weapons[i]->value[ITEM_WEAPON_TYPE]) {
			case WEAPON_POLEARM:
			case WEAPON_STAFF:
			case WEAPON_DAGGER:
				chance += 4 / (i+1);
				break;
		}
	}

	chance = URANGE(0, chance, 40);

	return chance;
}

int haft_block_chance (CHAR_DATA *ch, CHAR_DATA *victim)
{
	OBJ_DATA *ch_weapons[2] = {get_eq_char(ch, WEAR_WIELD), 
				get_eq_char(ch, WEAR_SECOND_WIELD)};
	OBJ_DATA *v_weapon;
	int chance;
	int i;

	if (IS_NPC (victim)) {
		if (!IS_SET(victim->pIndexData->act, ACT_WARRIOR))
			chance = 0;
		else
			chance = UMAX(5, LEVEL(victim) - 45);
	}
	else {
		chance = get_skill (victim, gsn_haft_block) /10;
	}

	if (chance <= 0)
		return 0;

	if (get_eq_char (victim, WEAR_WIELD) == NULL)
		return 0;

	if (IS_AFFECTED (victim, AFF_BLIND)) {
		if (get_skill (victim, gsn_blind_fighting) == 0)
			chance /= 2;
		else {
			chance /= 2;
			chance += ((get_skill (victim, gsn_blind_fighting)) / 4);
		}
	}

	chance += (LEVEL(victim) - LEVEL(ch)) / 3;

	/* weapon modifications */
	for (i = 0;i < 2;i++) {
		if (ch_weapons[i] == NULL) continue;

		switch (ch_weapons[i]->value[ITEM_WEAPON_TYPE]) {
			case WEAPON_DAGGER:
				chance -= 6 / (i+1);
				break;
		}
	}

	if ((v_weapon = get_eq_char(victim, WEAR_WIELD)) == NULL) {
		switch (v_weapon->value[ITEM_WEAPON_TYPE]) {
			case WEAPON_POLEARM:
			case WEAPON_SPEAR:
			case WEAPON_STAFF:
				chance += 4;
				break;
			case WEAPON_DAGGER:
				chance = 0;
				break;
		}
	}

	chance = URANGE(0, chance, 30);

	return chance;
}

int tumble_chance (CHAR_DATA *ch, CHAR_DATA *victim)
{
	int chance;
	OBJ_DATA *ch_weapons[2] = {get_eq_char(ch, WEAR_WIELD), 
				get_eq_char(ch, WEAR_SECOND_WIELD)};
	OBJ_DATA *v_weapon;
	int i;



	if (IS_NPC(victim))
	  return 0;

	if (get_eq_char (victim, WEAR_SHIELD) != NULL)
		return 0;

	chance = get_skill (victim, gsn_tumble) / 4;

	if (chance <= 0)
	  return 0;

	chance = chance + (get_curr_stat (victim, STAT_DEX) / 2);
	chance = chance - (get_curr_stat (ch, STAT_DEX) / 2);

	if (IS_AFFECTED (victim, AFF_BLIND))
	{
		if (get_skill (victim, gsn_blind_fighting) == 0)
		  chance /= 2;
		else
		{
			chance /= 2;
			chance += ((get_skill(victim, gsn_blind_fighting)) / 7);
		}
	}

	chance += (LEVEL(victim) - LEVEL(ch)) / 2;

	if (ENCUMBERANCE(victim) > 35)
		chance -= (ENCUMBERANCE(victim) - 35) / 2;

	if (ENCUMBERANCE(victim) > 75)
		chance -= (ENCUMBERANCE(victim) - 85) / 2;

	else if (ENCUMBERANCE(victim) <= 15)
		chance += ENCUMBERANCE(victim) / 3;

	/* weapon modifications */
	for (i = 0;i < 2;i++) {
		if (ch_weapons[i] == NULL) continue;

		switch (ch_weapons[i]->value[ITEM_WEAPON_TYPE]) {
			case WEAPON_POLEARM:
				chance += 4 / (i+1);
				break;
			case WEAPON_DAGGER:
				chance -= 4 / (i+1);
				break;
		}
	}

	v_weapon = get_eq_char(victim, WEAR_WIELD);

	chance = URANGE(0, chance, 80);

	return chance;
}

int blink_chance (CHAR_DATA *ch, CHAR_DATA *victim)
{
	int chance;

	if (IS_NPC(victim)) {
		if (IS_SET(victim->pIndexData->act, ACT_MAGE)
		&& LEVEL(victim) > 10)
			chance = 30;
		else
			chance = 0;
	}
	else
		chance	= get_skill(victim, gsn_blink) / 2;

	if (chance <= 0)
		return 0;

	chance += LEVEL(victim) - LEVEL(ch);

	chance = URANGE(0, chance, 80);

	return chance;
}

/*
 * assess the chance someone will hit someone else with a certain attack
 */
int tohit_chance(CHAR_DATA *ch, CHAR_DATA *victim, int dt, int loc)
{
	OBJ_DATA *wield;
	OBJ_DATA *dual = NULL;
	int victim_ac;
	int thac0;
	int thac0_00;
	int thac0_32;
	int sn, sk, sk2;
	int dam_type;
	bool counter;

	sn = -1;
	counter = FALSE;

	/* just in case */
	if (victim == ch || ch == NULL || victim == NULL)
		return -1; 

	/*
	 * Figure out the type of damage message.
	 */
	wield = get_eq_char(ch, loc);
	dam_type = get_dam_type(ch, wield, &dt);

	dual = get_eq_char(ch, WEAR_SECOND_WIELD);
	
	/*
	 * don't double backstab if the second weapon isn't peircing
	 */
	if (dt == gsn_dual_backstab 
	   && !IS_NPC(ch)
	   && (!dual || attack_table[dual->value[ITEM_WEAPON_ATTACK_TYPE]].damage != DAM_PIERCE)) {
		return -1;
	}

	/* get the weapon skill */
	sn = get_weapon_sn(wield);
	sk = 20 + get_weapon_skill(ch, sn);

	/*
	 * Calculate to-hit-armor-class-0 versus armor.
	 */
	if (IS_NPC(ch)) {
		flag64_t act = ch->pIndexData->act;

		thac0_00 = 20;
		thac0_32 = -4;	 /* as good as a thief */
		if (IS_SET(act, ACT_WARRIOR))
			thac0_32 = -10;
		else if (IS_SET(act, ACT_THIEF))
			thac0_32 = -4;
		else if (IS_SET(act, ACT_CLERIC))
			thac0_32 = 2;
		else if (IS_SET(act, ACT_MAGE))
			thac0_32 = 6;
	}
	else {
		class_t *cl;

		if ((cl = class_lookup(ch->class)) == NULL)
			return -1;

		thac0_00 = cl->thac0_00;
		thac0_32 = cl->thac0_32;
	}

	thac0  = interpolate(LEVEL(ch), thac0_00, thac0_32);

	if (thac0 < 0)
		thac0 = thac0/2;

	if (thac0 < -5)
		thac0 = -5 + (thac0 + 5) / 2;

	thac0 -= GET_HITROLL(ch) * sk / 100;
	thac0 += 5 * (100 - sk) / 100;

	if (dt == gsn_backstab)
		thac0 -= 10 * (100 - get_skill(ch, gsn_backstab));
	else if (dt == gsn_dual_backstab)
		thac0 -= 10 * (100 - get_skill(ch, gsn_dual_backstab));
	else if (dt == gsn_cleave)
		thac0 -= 10 * (100 - get_skill(ch, gsn_cleave));
	else if (dt == gsn_ambush)
		thac0 -= 10 * (100 - get_skill(ch, gsn_ambush));
	else if (dt == gsn_vampiric_bite)
		thac0 -= 10 * (100 - get_skill(ch, gsn_vampiric_bite));
	else if (dt == gsn_charge)
		thac0 -= 10 * (100 - get_skill(ch, gsn_charge));

	switch(dam_type) {
	case DAM_PIERCE:victim_ac = GET_AC(victim,AC_PIERCE)/10; break;
	case DAM_BASH:  victim_ac = GET_AC(victim,AC_BASH)/10; 	 break;
	case DAM_SLASH: victim_ac = GET_AC(victim,AC_SLASH)/10;	 break;
	default:	victim_ac = GET_AC(victim,AC_EXOTIC)/10; break;
	}

	if (victim_ac < -15)
		victim_ac = (victim_ac + 15) / 5 - 15;

	if (get_skill(victim, gsn_armor_use) > 70) {
		victim_ac -= (victim->level) / 2;
	}

	if (!can_see(ch, victim)) {
		 if ((sk2 = get_skill(ch, gsn_blind_fighting) == 0)
		 ||  number_percent() >= sk2)
			victim_ac -= 4;
	}

	if (victim->position < POS_FIGHTING)
		victim_ac += 4;

	if (victim->position < POS_RESTING)
		victim_ac += 6;

	return thac0 - victim_ac;
}

