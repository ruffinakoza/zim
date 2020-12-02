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
 * $Id: magic3.c 933 2006-11-19 22:37:00Z zsuzsu $
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "fight.h"
#include "healer.h"

/* command procedures needed */
DECLARE_DO_FUN(do_look		);
DECLARE_DO_FUN(do_yell		);
DECLARE_DO_FUN(do_say		);
DECLARE_DO_FUN(do_murder        );

void spell_poison_bolt(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
        CHAR_DATA *victim = (CHAR_DATA *) vo;
        int dam;
        
        dam = dice(level, 3) + 13;

	if (saves_spell(level,victim,DAM_POISON))
	{
	    poison_effect(victim,level/2,dam/4,TARGET_CHAR);
	    damage(ch,victim,dam/2,sn,DAM_POISON,TRUE);
	}
	else
	{
	    poison_effect(victim,level,dam,TARGET_CHAR);
	    damage(ch,victim,dam,sn,DAM_POISON,TRUE);
	}
}

void spell_paranoia(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
        CHAR_DATA *victim = (CHAR_DATA *) vo;
        AFFECT_DATA af;

        if(victim != ch)
        {
                act("You cannot bestow this ability upon another.",ch, NULL, victim, TO_CHAR);
                return;
        }
        if (is_affected(ch, gsn_paranoia))
        {
                char_puts("You already think someone's out to get you.\n", ch);
                return;
        }
        af.where         = TO_AFFECTS;
        af.type          = sn;
        af.level         = level;
        af.duration      = level/6;
        af.modifier      = 0;
        af.location      = APPLY_NONE;
        af.bitvector     = 0;
        affect_to_char(victim, &af);
        char_puts("You feel someone's out to get you.\n", victim);
        act("$n looks around suspiciously.",victim, NULL, NULL, TO_ROOM);
        return;
}

void spell_pollen(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
        CHAR_DATA *victim = (CHAR_DATA *) vo;
        AFFECT_DATA af;

	if (ch->in_room != NULL 
	    && ch->in_room->sector_type != SECT_FOREST
	    && ch->in_room->sector_type != SECT_FIELD
	    && ch->in_room->sector_type != SECT_HILLS
	    && ch->in_room->sector_type != SECT_MOUNTAIN
	    && ch->in_room->sector_type != SECT_DESERT) {

                act("The Forest cannot heed your call from here.",
			ch, NULL, victim, TO_CHAR);
                return;
	}
	

        if (victim == ch)
        {
                act("The Forest refuses to subject you to that.",ch, NULL, victim, TO_CHAR);
                return;
        }

        if (IS_NPC(victim))
        {
                act("Perhaps pollen should be reserved for someone more worthy.",
			ch, NULL, NULL, TO_CHAR);
                return;
        }

        if (IS_IMMORTAL(victim))
        {
                act("Pestering gods is not wise.",
			ch, NULL, NULL, TO_CHAR);
                return;
        }

        if (is_affected(victim, sn))
        {
                char_puts("More pollen won't do much.\n", ch);
                return;
        }

	if (in_PK(ch, victim) && saves_spell(level,victim,DAM_OTHER)) {
                act("$N resists the pollen.",
			ch, NULL, victim, TO_CHAR);
                act("Your eyes water a little.",
			victim, NULL, NULL, TO_CHAR);
		return;
	}

        af.where         = TO_AFFECTS;
        af.type          = sn;
        af.level         = level;
        af.duration      = level/5+2;
        af.modifier      = 0;
        af.location      = APPLY_NONE;
        af.bitvector     = 0;
        affect_to_char(victim, &af);
        char_puts("For some reason your {ga{yll{ge{Gr{gg{yi{Ge{gs{x flare up.\n", victim);
        act("$n looks to be suffering from {ga{yll{ge{Gr{gg{yi{Ge{gs{x.",
		victim, NULL, NULL, TO_ROOM);

	if ((ch->level > victim->level
	|| in_PK(ch, victim)) && number_percent() < 75) {
		af.where         = TO_AFFECTS;
		af.type          = gsn_pollen_blind;
		af.level         = level;
		af.duration      = 0;
		af.modifier      = 10;
		af.location      = APPLY_AC;
		af.bitvector     = AFF_BLIND;
		affect_to_char(victim, &af);

		char_puts("Your eyes {Rs{rw{Rell{x shut!\n", victim);
		act("$n's eyes {Rs{rw{Rell{x shut!", victim, NULL, NULL, TO_ROOM);
	}
}

void spell_minor_sanctuary (int sn, int level, CHAR_DATA *ch, void *vo, int target) 
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (!IS_IMMORTAL(ch) 
	&& !IS_NPC(ch)
	&& ch->class != CLASS_CLERIC && ch != victim) {
		char_puts("You are not powerful enough to"
			" offer another sanctuary.\n",
			ch);
		return;
	}

	if (is_affected(victim, gsn_minor_sanctuary)
		|| is_affected(victim, gsn_sanctuary)) {
		if (victim == ch)
			char_puts("You are already enveloped in godly grace.\n",ch);
		else
        		act("$n already looks to be enveloped in godly grace.",
				victim, NULL, NULL, TO_ROOM);
		return;
	}

	if (is_affected(victim, gsn_minor_black_shroud)
		|| is_affected(victim, gsn_black_shroud)) {
		if (victim == ch)
			char_puts("Your demonic aura repels your god's grace.\n",ch);
		else
        		act("$n looks to be enveloped in a demonic aura.",
				victim, NULL, NULL, TO_ROOM);
		return;
	}

	if (is_affected(victim, gsn_enhanced_armor)) {
		if (victim == ch)
			char_puts("You already benefit from enhanced armor.\n",ch);
		else
        		act("$n seems to already benefit from some mystical shield.",
				victim, NULL, NULL, TO_ROOM);
		return;
	}

	/*
	if (is_affected(victim, gsn_resistance)) {
		if (victim == ch)
			char_puts("You already benefit from resistance.\n",ch);
		else
        		act("$n seems to already benefit from some mystical shield.",
				victim, NULL, NULL, TO_ROOM);
	}
	*/

	af.where	= TO_AFFECTS;
	af.type		= gsn_minor_sanctuary;
	af.level	= level;
	af.duration	= level / 6;
	af.modifier	= 0;
	af.location	= APPLY_NONE;
	af.bitvector	= AFF_MINOR_SANCTUARY;
	affect_to_char(victim, &af);
	act("$n is surrounded by a thin {Wwhite{x aura.", victim, NULL, NULL, TO_ROOM);
	char_puts("You are surrounded by a thin {Wwhite{x aura.\n", victim);
}

void spell_minor_black_shroud (int sn, int level, CHAR_DATA *ch, void *vo, int target) 
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (!IS_IMMORTAL(ch) 
	&& !IS_NPC(ch)
	&& ch->class != CLASS_CLERIC && ch != victim) {
		char_puts("You are not powerful enough to"
			" extend your shroud to another.\n",
			ch);
		return;
	}

        if (!IS_EVIL(ch)) {
		char_puts("The gods are infuriated!.\n", ch);
		damage(ch, ch, dice(level, IS_GOOD(ch) ? 2 : 1),
			TYPE_HIT, DAM_HOLY, TRUE);
		return;
	}

	if (!IS_EVIL(victim)) {
		act("Your god does not seems to like $N",
			ch, NULL, victim, TO_CHAR);
		return;
	}


	if (is_affected(victim, gsn_minor_sanctuary)
		|| is_affected(victim, gsn_sanctuary)) {
		if (victim == ch)
			char_puts("You are already enveloped in godly grace!\n",ch);
		else
        		act("$n already looks to be enveloped in godly grace.",
				victim, NULL, NULL, TO_ROOM);
		return;
	}

	if (is_affected(victim, gsn_minor_black_shroud)
		|| is_affected(victim, gsn_black_shroud)) {
		if (victim == ch)
			char_puts("You are already enveloped in a demonic aura.\n",ch);
		else
        		act("$n looks to be enveloped in a demonic aura.",
				victim, NULL, NULL, TO_ROOM);
		return;
	}

	if (is_affected(victim, gsn_enhanced_armor)) {
		if (victim == ch)
			char_puts("You already benefit from enhanced armor.",ch);
		else
        		act("$n seems to already benefit from some mystical shield.",
				victim, NULL, NULL, TO_ROOM);
		return;
	}

	if (is_affected(victim, gsn_resistance)) {
		if (victim == ch)
			char_puts("You already benefit from resistance.",ch);
		else
        		act("$n seems to already benefit from some mystical shield.",
				victim, NULL, NULL, TO_ROOM);
		return;
	}

	af.where	= TO_AFFECTS;
	af.type		= sn;
	af.level	= level;
	af.duration	= level / 6;
	af.modifier	= 0;
	af.location	= APPLY_NONE;
	af.bitvector	= AFF_MINOR_BLACK_SHROUD;
	affect_to_char(victim, &af);
	act("$n is surrounded by a thin {Dblack{x aura.", victim, NULL, NULL, TO_ROOM);
	char_puts("You are surrounded by a thin {Dblack{x aura.\n", victim);
}

void spell_intoxication (int sn, int level, CHAR_DATA *ch, void *vo, int target) 
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	
	if (!check_trust(ch, victim) 
	&& saves_spell(level,victim,DAM_OTHER)) {
		act("You catch your balance.",
			ch, NULL, victim, TO_VICT);
		act("$n catches $s balance.",
			victim, NULL, NULL, TO_ROOM);
		return;
	}

	if (IS_DRUNK(victim)) {
		if (ch != victim)
        		act("$N is already three sheets to the wind.",
				ch, NULL, victim, TO_CHAR);
		else
			char_puts("You are already three sheets to the wind.", ch);
		return;
	}

	if (!IS_NPC(victim)) {
		victim->pcdata->condition[COND_DRUNK] += 10 + (level / 10);
		if (IS_DRUNK(victim))
			drunk_effect(victim, level, TRUE);
		else {
			act("You catch your balance.",
				ch, NULL, victim, TO_VICT);
			act("$n catches $s balance.",
				victim, NULL, NULL, TO_ROOM);
		}
	}
	else {
		drunk_effect(victim, level, TRUE);
	}
}

void spell_sobriety (int sn, int level, CHAR_DATA *ch, void *vo, int target) 
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	bool was_drunk = FALSE;

	if (!check_trust(ch, victim) 
	&& saves_spell(level,victim,DAM_OTHER)) {
		act("$N refuses to sober up.",
			ch, NULL, victim, TO_CHAR);
		act("You refuse to sober up.",
			ch, NULL, victim, TO_VICT);
		act("$N refuses to sober up.",
			ch, NULL, victim, TO_NOTVICT);
		return;
	}
	
	if (IS_NPC(victim) && !IS_DRUNK(victim)) {
		if (ch != victim)
        		act("$N is already sober.",
				ch, NULL, victim, TO_CHAR);
		else
			char_puts("You are already sober.", ch);
		return;
	}

	if (!IS_NPC(victim)) {
		was_drunk = IS_DRUNK(victim);

		victim->pcdata->condition[COND_DRUNK] = 
			UMAX(0, victim->pcdata->condition[COND_DRUNK] 
					- (10 + (level / 10)));
		if (was_drunk) {
			if (IS_DRUNK(victim)) {
				act("You sober a little but are still tipsy.",
					ch, NULL, victim, TO_VICT);
				act("$N sobers a little but is still tipsy.",
					ch, NULL, victim, TO_ROOM);
			}
			else
				drunk_effect(victim, level, FALSE);
		}
		else {
			if (victim->pcdata->condition[COND_DRUNK] > 0) { 
				act("You sober up.",
					ch, NULL, victim, TO_VICT);
				act("$N sobers up.",
					ch, NULL, victim, TO_ROOM);
			}
			else {
				act("You are stone cold sober.",
					ch, NULL, victim, TO_VICT);
				act("$N looks stone cold sober.",
					ch, NULL, victim, TO_ROOM);
			}
		}
	}
	else {
		drunk_effect(victim, level, FALSE);
	}
}

/*
 * condemnation affect makes healing difficult to impossible
 */
void spell_condemn (int sn, int level, CHAR_DATA *ch, void *vo, int target) 
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (is_affected(victim, gsn_condemnation)) {
		act("The mat ashen color of $n's skin catches your notice.",
			victim, NULL, NULL, TO_ROOM);
		act("Your skin can't get any more ashen.",
			victim, NULL, NULL, TO_CHAR);
		return;
	}

	if (saves_spell(level,victim,DAM_HARM)) {
		act("You feel blood drain from your face, and then return.",
			victim, NULL, NULL, TO_CHAR);
		act("$N's skin looks ashen for a moment, but the color returns.",
			victim, NULL, victim, TO_ROOM);
		return;
	}

	af.where	= TO_AFFECTS;
	af.type		= gsn_condemnation;
	af.level	= level;
	af.duration	= level / 7 + number_range(-3, 3);
	af.modifier	= 0;
	af.location	= APPLY_NONE;
	af.bitvector	= 0;
	affect_to_char(victim, &af);

	act("$n's skin takes on an {Dashen{x hue.",
		victim, NULL, NULL, TO_ROOM);
	act("Your skin takes on an {Dashen{x hue.",
		victim, NULL, NULL, TO_CHAR);
}


void spell_dissent (int sn, int level, CHAR_DATA *ch, void *vo, int target) 
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	CHAR_DATA *leader;

	leader = (victim->master == NULL) ? victim->leader : victim->master;

	if (is_affected(ch, gsn_dissent)) {
		act("You cannot cause more rebellion, just yet.", 
			ch, NULL, NULL, TO_CHAR);
		return;
	}
	
	if (leader == NULL) {
		/* it's not a pet or a groupie. */
		act("$N is not following anyone.", 
			ch, NULL, victim, TO_CHAR);
                return;
	}

	if (saves_spell(LEVEL(ch), victim, DAM_CHARM)) {
		act("$N is too loyal to $n.", 
			leader, NULL, victim, TO_NOTVICT);
		act("You sense $N's loyalty waver.", 
			leader, NULL, victim, TO_CHAR);
		act("Your loyalty wavers.", 
			victim, NULL, NULL, TO_CHAR);
		return;    
	}

	/* victim stops following their leader */
	stop_follower(victim);

	if (victim->position == POS_FIGHTING)
		victim->fighting = leader;

	/* add the gsn_rebel affect to the pet as well, 
	 * so the check in do_murder() can see
	 * the pet is meant to attack. (maybe even make
	 * it so that a PC affected by gsn_rebel
	 * can not group until it wears off)
	 */
	af.where        = TO_AFFECTS;
	af.type         = gsn_rebel;
	af.level        = level;
	af.duration     = 3;
	af.bitvector    = 0;
	af.modifier     = 0;
	af.location     = APPLY_NONE;
	affect_to_char(victim, &af);

	act("You have created dissension within $N's ranks.", 
		ch, NULL, leader, TO_CHAR);
	act("You feel rebellious.", 
		ch, NULL, NULL, TO_VICT);

	do_say(victim, "I've had enough of you bossing me around!");

	act("$n grows rebellious and attacks $N.", 
		victim, NULL, leader, TO_NOTVICT);

	act("$N grows rebellious and attacks you!", 
		leader, NULL, victim, TO_CHAR);
	do_murder(victim, leader->name);

	af.where        = TO_AFFECTS;
	af.type         = gsn_dissent;
	af.level        = level;
	af.duration     = 10;
	af.bitvector    = 0;
	af.modifier     = 0;
	af.location     = APPLY_NONE;
	affect_to_char(ch, &af);
}

/*
 * healing spell for ninja and samurai
 */
void spell_sakura (int sn, int level, CHAR_DATA *ch, void *vo, int target) 
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (is_affected(ch, gsn_sakura)) {
		char_puts("Your clouded mind is not ready yet.\n", ch);
		return;
	}

        af.type		= gsn_sakura;
        af.where	= TO_AFFECTS;
        af.level	= LEVEL(ch);
        af.duration	= 23;
        af.location	= APPLY_NONE;
        af.modifier	= 0;
        af.bitvector	= 0;
        affect_to_char (ch, &af);

	act("The fragerance of {Rch{re{Rrry{x blossoms fills the room.",
		ch, NULL, NULL, TO_ALL);

	heal(ch, NULL, victim, gsn_sakura, LEVEL(ch), victim->max_hit /3);
        update_pos(victim);

        if (IS_AFFECTED(victim, AFF_BLIND))
                spell_cure_blindness(sn_lookup("cure blindness"), LEVEL(ch) + 10,
                        ch, (void*)ch, TARGET_CHAR);
     
        if (IS_AFFECTED(victim, AFF_POISON))
                spell_cure_poison(sn_lookup("cure poison"), LEVEL(ch) + 10,
                        ch, (void*)victim, TARGET_CHAR);


}

void spell_summon_zombie(int sn, int level, CHAR_DATA *ch, void *vo , int target)
{
        CHAR_DATA *gch;
        CHAR_DATA *undead;
        AFFECT_DATA af;
        int u_level, i;

        if (is_affected(ch,sn)) {
		char_puts("You lack the power to command a zombie's will.\n",ch);
		return;
        }

        char_puts("You attempt to summon a zombie.\n",ch);
        act("$n attempts to summon a zombie.",ch,NULL,NULL,TO_ROOM);

        for (gch = npc_list; gch; gch = gch->next) {
                if (IS_AFFECTED(gch, AFF_CHARM)
                &&  gch->master == ch
                &&  gch->pIndexData->vnum == MOB_VNUM_ZOMBIE) {
                        char_puts("Two zombies are more than you can control!\n",
                                  ch);
                        return;
                }
        }

        undead = create_mob(get_mob_index(MOB_VNUM_ZOMBIE));

	u_level = ch->level;

	for (i = 0; i < MAX_STATS; i++)
		undead->perm_stat[i] = UMIN(25, 15+u_level/10);

	undead->max_hit = dice(20,u_level*2)+u_level*20;
	undead->hit = undead->max_hit;
	undead->max_mana = dice(u_level,10)+100;
	undead->mana = undead->max_mana;
	undead->alignment = -1000;
	undead->level = u_level;

	for (i = 0; i < 3; i++)
		undead->armor[i] = interpolate(undead->level,100,-100);
	undead->armor[3] = interpolate(undead->level, 50, -200);
	undead->sex = ch->sex;
	undead->gold = 0;
	undead->damage[DICE_NUMBER] = 11;
	undead->damage[DICE_TYPE]   = 5;
	undead->damage[DICE_BONUS]  = u_level/2 +10;

	undead->master = ch;
	undead->leader = ch;

	af.where        = TO_AFFECTS;
	af.type         = sn;
	af.level        = level;
	af.duration     = number_range(13, 24);
	af.bitvector    = 0;
	af.modifier     = 0;
	af.location     = APPLY_NONE;
	affect_to_char(ch, &af);

	char_to_room(undead,ch->in_room);
	
	act("$N rises from a crack in the ground and moans.",
		ch, NULL, undead, TO_ALL);
}

void spell_charm_monster(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	char buf[MAX_INPUT_LENGTH];
	AFFECT_DATA af;
	int ladj = 0;		/* level adjustment (for opposite sex) */

	if (count_charmed(ch))
		return;

	if (victim == ch) {
		char_puts("You really think that much of yourself?\n", ch);
		return;
	}

	if (!IS_NPC(victim) && !IS_NPC(ch)) {
		char_puts("Well, they might be ugly, but a monster?\n", ch);
		return;
	}

	ladj += (ch->sex != victim->sex) ? 2 : 0;

	if (IS_AFFECTED(victim, AFF_CHARM)) {
		act("$N is already charmed.",
		   ch, NULL, victim, TO_CHAR);
		return;
	}

	if (IS_AFFECTED(ch, AFF_CHARM)) {
		char_puts("You have no will to exert over others\n", ch);
		return;
	}

	if (level+ladj < victim->level) {
		act("You find $n's charms amusing.\n",
		   ch, NULL, victim, TO_VICT);
		act("$N is much too powerful to succumb to your will.",
		   ch, NULL, victim, TO_CHAR);
		act("$N seems impervious to $n's charms.\n",
		   ch, NULL, victim, TO_NOTVICT);
		return;
	}

	level = IS_NPC(victim) ? level : level-8;

	if (is_safe(ch, victim)) {
		act("$N seems impervious to your charms at this moment.",
		   ch, NULL, victim, TO_CHAR);
		return;
	}

	if (check_dispel(level-8, victim, gsn_dissent)) {
		act("$n doesn't seem so disagreable anymore.",victim,NULL,NULL,TO_ROOM);
	}

	if (is_affected(victim, gsn_dissent)) {
		ladj -= level -5;
	}

	if (IS_SET(victim->imm_flags, IMM_CHARM)
	||  (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
	||  saves_spell((level+ladj), victim, DAM_CHARM)) {
		act("You find $n's charms amusing.\n",
		   ch, NULL, victim, TO_VICT);
		act("$N is not impressed by your charms.",
		   ch, NULL, victim, TO_CHAR);
		act("$N resists $n's charms.\n",
		   ch, NULL, victim, TO_NOTVICT);
		return;
	}

	if (is_affected(victim, gsn_dissent)) {
		affect_strip(victim,gsn_dissent);
	}

	if (victim->master)
		stop_follower(victim);
	add_follower(victim, ch);

	if (ch->leader == victim) 
		ch->leader = NULL;

	victim->leader = ch;

	af.where	= TO_AFFECTS;
	af.type		= sn;
	af.level	= level;
	af.duration	= number_fuzzy(level / 3);
	af.location	= 0;
	af.modifier	= 0;
	af.bitvector	= AFF_CHARM;
	affect_to_char(victim, &af);

	act("Isn't $n just so nice?", ch, NULL, victim, TO_VICT);

	if (ch != victim)
		act("$N looks at you with adoring eyes.",
		    ch, NULL, victim, TO_CHAR);

	if (!IS_NPC(victim))
		REMOVE_BIT(victim->conf_flags, PLR_CONF_AUTOASSIST);

	if (IS_NPC(victim) && !IS_NPC(ch)) {
		victim->last_fought=ch;
		if (number_percent() < (4 + (victim->level - ch->level)) * 10)
		 	add_mind(victim, ch->name);
		else if (victim->in_mind == NULL) {
			snprintf(buf, sizeof(buf), "%d", victim->in_room->vnum);
			victim->in_mind = str_dup(buf);
		}
	}
}

/**
 * range damage spell that is good alignment based
 */
void spell_ray_of_hope (int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam, align;

	if (!IS_GOOD(ch))
	{
		victim = ch;
		char_puts("The energy explodes inside you!\n",ch);
	}

	if (victim != ch)
	{
		act("$n raises $s hand, and a blinding ray of hope shoots forth!",
		    ch,NULL,NULL,TO_ROOM);
		char_puts(
		   "You raise your hand and a blinding ray of hope shoots forth!\n",
		   ch);
	}

	if (IS_GOOD(victim))
	{
		act("$n seems unharmed by the light.",victim,NULL,victim,TO_ROOM);
		char_puts("The light seems powerless to affect you.\n",victim);
		return;
	}

	dam = dice(level, 10);
	if (saves_spell(level, victim,DAM_HOLY))
		dam /= 2;

	align = victim->alignment;
	align -= 350;

	if (align < -1000)
		align = -1000 + (align + 1000) / 3;

	dam = (dam * align * align) / 1000000;

	spell_blindness(gsn_blindness, 3 * level / 4, ch,
			(void *) victim, TARGET_CHAR);
	damage(ch, victim, dam, sn, DAM_HOLY ,TRUE);
}

/**
 * range damage spell that is neutral alignment based
 */
void spell_ray_of_truth (int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam, align;

	if (!IS_NEUTRAL(ch))
	{
		victim = ch;
		char_puts("The energy explodes inside you!\n",ch);
	}

	if (victim != ch)
	{
		act("$n raises $s hand, and a blinding ray of light shoots forth!",
		    ch,NULL,NULL,TO_ROOM);
		char_puts(
		   "You raise your hand and a blinding ray of light shoots forth!\n",
		   ch);
	}

	if (IS_NEUTRAL(victim))
	{
		act("$n seems unharmed by the light.",victim,NULL,victim,TO_ROOM);
		char_puts("The light seems powerless to affect you.\n",victim);
		return;
	}

	dam = dice(level, 8);
	if (saves_spell(level, victim,DAM_LIGHT))
		dam /= 2;

	align = victim->alignment;

	dam = (dam * align * align) / 1000000;

	spell_blindness(gsn_blindness, 3 * level / 4, ch,
			(void *) victim, TARGET_CHAR);
	damage(ch, victim, dam, sn, DAM_LIGHT ,TRUE);
}

/**
 * range damage spell that is evil alignment based
 */
void spell_ray_of_terror (int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam, align;

	if (!IS_EVIL(ch))
	{
		victim = ch;
		char_puts("The energy explodes inside you!\n",ch);
	}

	if (victim != ch)
	{
		act("$n raises $s hand, and a {Dblack ray{x of {Dterror{x shoots forth!",
		    ch,NULL,NULL,TO_ROOM);
		char_puts(
		   "You raise your hand and a {Dblack ray{x of {Dterror{x shoots forth!\n",
		   ch);
	}

	if (IS_EVIL(victim))
	{
		act("$n seems immune to the terror.",victim,NULL,victim,TO_ROOM);
		char_puts("The terror harmlessly washes over you.\n",victim);
		return;
	}

	dam = dice(level, 10);
	if (saves_spell(level, victim,DAM_NEGATIVE))
		dam /= 2;

	align = victim->alignment;
	align += 350;

	if (align > 1000)
		align = 1000 + (align + -1000) / 3;


	dam = (dam * align * align) / 1000000;

	if (dam < 0) {
		BUG("%s castin RoT on %s (align %d) damage is %d?!",
		ch->name, victim->name, victim->alignment, dam);
		dam = dice(level, 10);
	}

	spell_blindness(gsn_blindness, 3 * level / 4, ch,
			(void *) victim, TARGET_CHAR);
	damage(ch, victim, dam, sn, DAM_NEGATIVE ,TRUE);
}

/*
 * necro spell to heal minons through corpses
 */
void spell_consume_dead(int sn,int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim;
	CHAR_DATA *undead = NULL, *gch;
	/*AFFECT_DATA af;*/
	OBJ_DATA *obj, *obj2, *next;
	bool had_items = FALSE;

	/* deal with the object case first */
	if (target == TARGET_OBJ) {
		obj = (OBJ_DATA *) vo;

		if (!(obj->pIndexData->item_type == ITEM_CORPSE_NPC 
		|| obj->pIndexData->item_type == ITEM_CORPSE_PC)) {
			char_puts("Your minons can only devour corpses!\n", ch);
			return;
		}

		/*
		if (is_affected(ch, sn)) {
			char_puts("Your minons must wait to feed.\n", ch);
			return;
		}
		*/

		if (!ch->in_room) {
			char_puts("Nowhere is no place to feed.\n", ch);
			return;
		}

		for (gch = ch->in_room->people; gch; gch = gch->next) {
			if (IS_AFFECTED(gch, AFF_CHARM)
			&&  gch->master == ch
			&& gch->position >= POS_STANDING
			&& IS_UNDEAD(gch)) {
				/* pick most unhealthy */
				if (!undead 
				|| ((undead->max_hit - undead->hit) / (undead->max_hit+1)*100)
				< ((gch->max_hit - gch->hit) / (gch->max_hit+1)*100))
					undead = gch;
			}
		}

		if (!undead) {
			char_puts("You have no minons here to feed.\n", ch);
			return;
		}

		/* can't animate PC corpses in ROOM_BATTLE_ARENA */
		if (obj->pIndexData->item_type == ITEM_CORPSE_PC
		&&  obj->in_room
		&&  IS_SET(obj->in_room->room_flags, ROOM_BATTLE_ARENA		    
		&&  !IS_OWNER(ch, obj))){
			char_puts("You cannot do that here.\n", ch);
			return;
		}

		if (IS_SET(ch->in_room->room_flags,
			   ROOM_SAFE | ROOM_PEACE | ROOM_PRIVATE |
			   ROOM_SOLITARY)) {
			char_puts("You can't animate here.\n", ch);
			return;
		}

		if (obj->pIndexData->item_type == ITEM_CORPSE_PC
		&& obj->contains != NULL) {
			char_puts("You need to strip the body first.\n", ch);
			return;
		}

		/* drop all objects on the floor */
		for (obj2 = obj->contains; obj2; obj2 = next) {
			next = obj2->next_content;
			obj_from_obj(obj2);
			obj_to_room(obj2, ch->in_room);
			/*obj_to_char(obj2, undead);*/
			had_items = TRUE;
		}

		/*
		af.where     = TO_AFFECTS;
		af.type      = sn;
		af.level     = ch->level;
		af.duration  = 0;
		af.modifier  = 0;
		af.bitvector = 0;
		af.location  = APPLY_NONE;
		affect_to_char(ch, &af);
		*/

		act_puts("With mystic power, you command $N to {rfeed{x!",
			 ch, NULL, undead, TO_CHAR, POS_DEAD);

		act_puts("With mystic power, $n commands $N to {rfeed{x!",
			 ch, NULL, undead, TO_ROOM, POS_DEAD);

		act_puts("$n {rravages{x $p and looks . . . {Dbetter{x.",
			 undead, obj, NULL, TO_ROOM, POS_DEAD);

		if (had_items)
			act("$p's worldy belongings are scattered on the ground.",
				ch, obj, NULL, TO_ALL);

		undead->hit += obj->level * level / 5;

		extract_obj(obj, 0);

		return;
	}

	victim = (CHAR_DATA *) vo;

	if (ch == victim) {
		char_puts("Definately a bad idea.\n", ch);
		return;
	}
	else if (victim) {
		act_puts("That corpse is moving around too much"
			" for your minions to chomp on.\n",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		return;
	}
	else {
		act_puts("Huh?\n",
			 ch, NULL, undead, TO_CHAR, POS_DEAD);
	}

	return;
}

void spell_prismic_beam(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam, damtype;

	switch(number_range(0,5)) {
	case 0:
		damtype = DAM_ACID;
		act("$n raises $s hand, and a {Yyellow beam{x shoots forth!",
		    ch,NULL,NULL,TO_ROOM);
		char_puts(
		   "You raise your hand and a {Yyellow beam{x shoots forth!\n",
		   ch);
		break;
	case 1:
		damtype = DAM_FIRE;
		act("$n raises $s hand, and a {rred beam{x shoots forth!",
		    ch,NULL,NULL,TO_ROOM);
		char_puts(
		   "You raise your hand and a {rred beam{x shoots forth!\n",
		   ch);
		break;
	case 2:
		damtype = DAM_COLD;
		act("$n raises $s hand, and a {Bblue beam{x shoots forth!",
		    ch,NULL,NULL,TO_ROOM);
		char_puts(
		   "You raise your hand and a {Bblue beam{x shoots forth!\n",
		   ch);
		break;
	case 3:
		damtype = DAM_LIGHTNING;
		act("$n raises $s hand, and a {Wwhite beam{x shoots forth!",
		    ch,NULL,NULL,TO_ROOM);
		char_puts(
		   "You raise your hand and a {Wwhite beam{x shoots forth!\n",
		   ch);
		break;
	case 4:
	default:
		damtype = DAM_ENERGY;
		act("$n raises $s hand, and a {mviolet beam{x shoots forth!",
		    ch,NULL,NULL,TO_ROOM);
		char_puts(
		   "You raise your hand and a {mviolet beam{x shoots forth!\n",
		   ch);
		break;
	}

	dam = number_range(dice(level,14), dice(level, 18));
	if (saves_spell(level, victim, damtype))
		dam /= 2;
	damage(ch, victim, dam, sn,damtype,TRUE);

	return;
}
