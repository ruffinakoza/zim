/*
 * $Id: magic.c 1009 2007-01-22 02:20:20Z zsuzsu $
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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "debug.h"
#include "update.h"
#include "fight.h"
#include "magic.h"
#include "healer.h"
#include "quest.h"

DECLARE_DO_FUN(do_yell		);
DECLARE_DO_FUN(do_look		);
DECLARE_DO_FUN(do_stand		);

extern int gsn_anathema;

/*
 * get the casting type for the character
 * to see if they are a communer, channeler
 * or regular magic castrer
 */
int cast_type (CHAR_DATA *ch)
{
	if (IS_NPC(ch)) {
		if (IS_SET(ch->pIndexData->act, ACT_CLERIC))
			return CAST_COMMUNE;
		else
			return CAST_CAST;
	}

	switch (ch->class) {
		case CLASS_CLERIC:
		case CLASS_PALADIN:
		case CLASS_RANGER:
			return CAST_COMMUNE;
		case CLASS_NINJA:
		case CLASS_SAMURAI:
			return CAST_CHANNEL;
		default:
			return CAST_CAST;
	}
}

/*
 * for casting different rooms 
 * returned value is the range 
 */
int allowed_other(CHAR_DATA *ch, int sn)
{
	if (IS_SET(SKILL(sn)->flags, SKILL_RANGE))
		return ch->level / 20 + 1;
	return 0;
}

/*
 * spellbane - check if 'bch' deflects the spell of 'ch'
 */
bool spellbane(CHAR_DATA *bch, CHAR_DATA *ch, int bane_bonus, int bane_damage)
{
	int bane_chance = get_skill(bch, gsn_spellbane);

	if (bane_chance <= 0
	|| !clan_item_ok(ch->clan))
		return FALSE;

	if (IS_IMMORTAL(ch) && bch == ch)
		return FALSE;

	if (!IS_NPC(bch)) {
		switch (bch->class) {
			case CLASS_WARRIOR:
			case CLASS_THIEF:
				bane_chance = bane_chance * 4/10;
				break;
			case CLASS_SAMURAI:
			case CLASS_NINJA:
			case CLASS_RANGER:
				bane_chance = bane_chance * 3/10;
				break;
			case CLASS_CLERIC:
			case CLASS_PALADIN:
				bane_chance = bane_chance * 2/10;
				break;
			default:
				DEBUG(DEBUG_SKILL_SPELLBANE,
					"spellbane: unknown class for %s: %d",
					bch->name,
					bch->class);
		}
	}

	if (ch->position == POS_SLEEPING) {
		bane_chance = bane_chance * 1/2;
	}

	DEBUG(DEBUG_SKILL_SPELLBANE,
		"spellbane %s[%d] vs %s[%d]: %d%%", 
		bch->name,
		LEVEL(bch),
		ch->name,
		LEVEL(ch),
		bane_chance);

	if (HAS_SKILL(bch, gsn_spellbane)
	&&  number_percent() < bane_chance) {
		if (bch == ch) {
                        act_puts("The gods are furious with you!",
				bch, NULL, NULL, TO_CHAR, POS_DEAD);
			act("The magic bounces off $n, but $e's struck something else!",
				bch, NULL, NULL, TO_ROOM);
			damage(bch, ch, 5 * LEVEL(bch),
				gsn_spellbane, DAM_OTHER, TRUE);
		}
		else {
			act_puts("$N deflects your spell!",
				ch, NULL, bch, TO_CHAR, POS_DEAD);
			act("You deflect $n's spell!",
				ch, NULL, bch, TO_VICT);
			act("$N deflects $n's spell!",
				ch, NULL, bch, TO_NOTVICT);
			if (!is_safe(bch, ch))
				damage(bch, ch, LEVEL(ch) + 1,
					gsn_spellbane, DAM_OTHER, TRUE);
			check_improve(bch, gsn_spellbane, TRUE, 1);
		}
		return TRUE;
	}
	else
		check_improve(bch, gsn_spellbane, FALSE, 1);

	return FALSE;
}

/*
 * The kludgy global is for spells who want more stuff from command line.
 */
const char *target_name;

/*
 * All forms of casting spells (commune, cast, channel) call
 * this function after the cast_type is set
 */
void cast_common(CHAR_DATA *ch, const char *argument, int cast_type)
{
	char arg1[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	void *vo;
	int mana;
	int sn = -1;
	int target;
	int door, range;
	bool cast_far = FALSE;
	bool offensive = FALSE;
	int slevel;
	int chance = 0;
	skill_t *spell;
	class_t *cl;
	int percent_drunk_fail = 0;

	CHAR_DATA *bch;		/* char to check spellbane on */
	int bane_bonus;		/* spellbane chance */

	if (IS_NPC(ch)) {
		if (IS_SET(ch->pIndexData->act, ACT_CLERIC)) {
			cast_type = CAST_COMMUNE;
		}
	}
	else if (PICK_RELIGION_TO_COMMUNE
	&& ch->level > MIN_PK_LEVEL -2 
	&& cast_type == CAST_COMMUNE
	&& ch->class != CLASS_RANGER
	&& (ch->religion == RELIGION_NONE
	|| ch->religion == RELIGION_ATHEIST)) {
		char_puts("You need to worship a god to empower your prayers.\n",ch);
		return;
	}

	if ((cl = class_lookup(ch->class)) == NULL)
		return;

	if (HAS_SKILL(ch, gsn_spellbane) && cast_type == CAST_CAST) {
		char_puts("You are Barbarian, not a filthy magician.\n",
			  ch);
		return;
	}

	if (is_affected(ch, gsn_shielding)) {
		char_puts("You reach for the True Source and feel something "
			  "stopping you.\n", ch);
		return;
	}

	if (is_affected(ch, gsn_garble) && number_percent() < 50) {
		char_puts("You can't get the right intonations.\n", ch);
		return;
	}

	if (is_affected(ch, gsn_deafen)) {
		char_puts("You can't get the right intonations.\n", ch);
		return;
	}

     /*   if (IS_SET(ch->state_flags, STATE_MANACLE))  {
                char_puts("You can't finish the spell with your hands bound!\n", ch);
                return;
        }*/


	target_name = one_argument(argument, arg1, sizeof(arg1));
	if (arg1[0] == '\0') {
		switch (cast_type) {
			case CAST_COMMUNE:
				char_puts("Commune which what where?\n", ch);
				break;
			case CAST_CHANNEL:
				char_puts("Channel which what where?\n", ch);
				break;
			default:
				char_puts("Cast which what where?\n", ch);
		}
		return;
	}

	if (IS_NPC(ch)) {
		if (!str_cmp(arg1, "nowait")) {
			target_name = one_argument(target_name,
						   arg1, sizeof(arg1));
			if (ch->wait)
				ch->wait = 0;
		}
		else if (ch->wait) 
			return;
		sn = sn_lookup(arg1);
	}
	else {
		pcskill_t *ps;
		ps = (pcskill_t*) skill_vlookup(&ch->pcdata->learned, arg1);
		if (ps)
			sn = ps->sn;
	}

	if ((chance = get_skill(ch, sn)) == 0) {
		switch (cast_type) {
			case CAST_COMMUNE:
				char_puts("You don't know any prayers by that name.\n", ch);
				break;
			case CAST_CHANNEL:
				char_puts("You don't know any ki by that name.\n", ch);
				break;
			default:
				char_puts("You don't know any spells of that name.\n", ch);
		}
		return;
	}
	if (IS_NPC(ch))
		chance = 97;
	spell = SKILL(sn);
	
/* ### Cast vampire lock out wished removed ### 
#Removed by thorel 	 
#
#
#
#	if (HAS_SKILL(ch, gsn_vampire)
#	&&  !is_affected(ch, gsn_vampire)
#	&&  !IS_SET(spell->flags, SKILL_CLAN)) {
#		char_puts("You must transform to vampire before casting!\n",
#			  ch);
#		return;
#	}
#############################################
*/

	if (spell->spell_fun == NULL) {
		switch (cast_type) {
			case CAST_COMMUNE:
				char_puts("That's not a prayer.\n", ch);
				break;
			case CAST_CHANNEL:
				char_puts("That's not ki.\n", ch);
				break;
			default:
				char_puts("That's not a spell.\n", ch);
		}
		return;
	}

	if (ch->position < spell->minimum_position) {
		char_puts("You can't concentrate enough.\n", ch);
		return;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_NOMAGIC)) {
		switch (cast_type) {
			case CAST_COMMUNE:
				char_puts("Your prayer fizzles out and fails.\n", ch);
				act("$n's prayer fizzles out and fails.",
				    ch, NULL, NULL, TO_ROOM);
				break;
			case CAST_CHANNEL:
				char_puts("Your ki fizzles out and fails.\n", ch);
				act("$n's ki fizzles out and fails.",
				    ch, NULL, NULL, TO_ROOM);
				break;
			default:
				char_puts("Your spell fizzles out and fails.\n", ch);
				act("$n's spell fizzles out and fails.",
				    ch, NULL, NULL, TO_ROOM);
		}
		return;
	}

	if (!IS_NPC(ch)) {
		mana = mana_cost(ch, sn);

		if (ch->mana < mana) {
			char_puts("You don't have enough mana.\n", ch);
			return;
		}
	}
	else
		mana = 0;

	/*
	 * Locate targets.
	 */
	victim		= NULL;
	obj		= NULL;
	vo		= NULL;
	target		= TARGET_NONE;
	bane_bonus	= 0;
	bch		= NULL;

	switch (spell->target) {
	default:
		bug("Do_cast: bad target for sn %d.", sn);
		return;

	case TAR_IGNORE:
		bch = ch;
		break;

	case TAR_CHAR_OFFENSIVE:
		if (target_name[0] == '\0') {
			if ((victim = ch->fighting) == NULL) {
				switch (cast_type) {
					case CAST_COMMUNE:
						char_puts("Commune the prayer on whom?\n", ch);
						break;
					case CAST_CHANNEL:
						char_puts("Channel your ki on whom?\n", ch);
						break;
					default:
						char_puts("Cast the spell on whom?\n", ch);
				}
				return;
			}
		}
		else if ((range = allowed_other(ch, sn)) > 0) {
			if ((victim = get_char_spell(ch, target_name,
						     &door, range)) == NULL) {
				WAIT_STATE(ch, MISSING_TARGET_DELAY);
				return;
			}

			if (victim->in_room != ch->in_room) {
				cast_far = TRUE;
				if (IS_NPC(victim)) {
					if (room_is_private(ch->in_room)) {
						WAIT_STATE(ch, spell->beats);
						switch (cast_type) {
							case CAST_COMMUNE:
								char_puts("You can't commune this prayer from private room right now.\n", ch);
								break;
							case CAST_CHANNEL:
								char_puts("You can't channel your ki from private room right now.\n", ch);
								break;
							default:
								char_puts("You can't cast this spell from private room right now.\n", ch);
						}
						return;
					}

					if (IS_SET(victim->pIndexData->act,
						   ACT_NOTRACK)) {
						WAIT_STATE(ch, spell->beats);
						switch (cast_type) {
							case CAST_COMMUNE:
								act_puts("You can't commune this prayer on $N at this distance.", ch, NULL, victim, TO_CHAR, POS_DEAD);
								break;
							case CAST_CHANNEL:
								act_puts("You can't channel your ki to $N at this distance.", ch, NULL, victim, TO_CHAR, POS_DEAD);
								break;
							default:
								act_puts("You can't cast this spell to $N at this distance.", ch, NULL, victim, TO_CHAR, POS_DEAD);
						}
						return;
					}	
				}
			}
		}
		else if ((victim = get_char_room(ch, target_name)) == NULL) {
			WAIT_STATE(ch, MISSING_TARGET_DELAY);
			char_puts("They aren't here.\n", ch);
			return;
		}
		else if (victim 
			&& IS_NEWBIE(ch)
			&& ((!IS_NPC(victim) && !is_same_group(ch, victim))
			|| (IS_NPC(victim) && !is_same_group(ch, victim) 
				&& IS_AFFECTED(victim, AFF_CHARM)))) {
			WAIT_STATE(ch, MISSING_TARGET_DELAY);
			char_puts("Since you are a newbie, you can't cast on others.\n",ch);
			return;
		}

		target = TARGET_CHAR;
		bch = victim;
		bane_bonus = (LEVEL(bch) - LEVEL(ch));
		break;

	case TAR_CHAR_DEFENSIVE:
		if (target_name[0] == '\0')
			victim = ch;
		else if ((victim = get_char_room(ch, target_name)) == NULL) {
			char_puts("They aren't here.\n", ch);
			return;
		}
		else if (victim 
			&& IS_NEWBIE(ch)
			&& ((!IS_NPC(victim) && !is_same_group(ch, victim))
			|| (IS_NPC(victim) && !is_same_group(ch, victim) 
				&& IS_AFFECTED(victim, AFF_CHARM)))) {
			WAIT_STATE(ch, MISSING_TARGET_DELAY);
			char_puts("Since you are a newbie, you can't cast on others.\n",ch);
			return;
		}


		target = TARGET_CHAR;
		bch = victim;
		break;

	case TAR_CHAR_SELF:
		if (target_name[0] == '\0')
			victim = ch;
		else if ((victim = get_char_room(ch, target_name)) == NULL
		     ||  (!IS_NPC(ch) && victim != ch)) {
			switch (cast_type) {
				case CAST_COMMUNE:
					char_puts("You cannot commune this prayer "
						  "on another.\n", ch);
					break;
				case CAST_CHANNEL:
					char_puts("You cannot channel this ki "
						  "on another.\n", ch);
					break;
				default:
					char_puts("You cannot cast this spell "
						  "on another.\n", ch);
			}
			return;
		}

		target = TARGET_CHAR;
		bch = victim;
		break;

	case TAR_OBJ_INV:
		if (target_name[0] == '\0') {
			switch (cast_type) {
				case CAST_COMMUNE:
					char_puts("What should the prayer be communed upon?\n",
						  ch);
					break;
				case CAST_CHANNEL:
					char_puts("What should the ki be channeled upon?\n",
						  ch);
					break;
				default:
					char_puts("What should the spell be cast upon?\n",
						  ch);
			}
			return;
		}

		if ((obj = get_obj_carry(ch, target_name)) == NULL) {
			char_puts("You are not carrying that.\n", ch);
			return;
		}

		target = TARGET_OBJ;
		bch = ch;
		break;

	case TAR_OBJ_CHAR_OFF:
		if (target_name[0] == '\0') {
			if ((victim = ch->fighting) == NULL) {
				WAIT_STATE(ch, MISSING_TARGET_DELAY);
				switch (cast_type) {
					case CAST_COMMUNE:
						char_puts("Commune the blessing on whom or what?\n",
						  ch);
						break;
					case CAST_CHANNEL:
						char_puts("Channel your ki on whom or what?\n",
						  ch);
						break;
					default:
						char_puts("Cast the spell on whom or what?\n",
						  ch);
				}
				return;
			}
			target = TARGET_CHAR;
		}
		else if ((victim = get_char_room(ch, target_name)))
			target = TARGET_CHAR;
		else if ((obj = get_obj_here(ch, target_name)))
			target = TARGET_OBJ;
		else {
			WAIT_STATE(ch, spell->beats);
			char_puts("You don't see that here.\n",ch);
			return;
		}
		if (victim 
			&& IS_NEWBIE(ch)
			&& ((!IS_NPC(victim) && !is_same_group(ch, victim))
			|| (IS_NPC(victim) && !is_same_group(ch, victim) 
				&& IS_AFFECTED(victim, AFF_CHARM)))) {
			WAIT_STATE(ch, MISSING_TARGET_DELAY);
			char_puts("Since you are a newbie, you can't cast on others.\n",ch);
			return;
		}

		bch = victim;
		break;

	case TAR_OBJ_CHAR_DEF:
		if (target_name[0] == '\0') {
			victim = ch;
			target = TARGET_CHAR;
		}
		else if ((victim = get_char_room(ch, target_name)))
			target = TARGET_CHAR;
		else if ((obj = get_obj_carry(ch, target_name)))
			target = TARGET_OBJ;
		else {
			char_puts("You don't see that here.\n",ch);
			return;
		}
		if (victim 
			&& IS_NEWBIE(ch)
			&& ((!IS_NPC(victim) && !is_same_group(ch, victim))
			|| (IS_NPC(victim) && !is_same_group(ch, victim) 
				&& IS_AFFECTED(victim, AFF_CHARM)))) {
			WAIT_STATE(ch, MISSING_TARGET_DELAY);
			char_puts("Since you are a newbie, you can't cast on others.\n",ch);
			return;
		}

		bch = victim;
		break;
	}

	if (str_cmp(spell->name, "ventriloquate"))
		say_spell(ch, sn);

	switch (target) {
	case TARGET_CHAR:
		vo = (void*) victim;

		switch (spell->target) {
		case TAR_CHAR_DEFENSIVE:
		case TAR_OBJ_CHAR_DEF:
			if (victim 
			&& IS_NEWBIE(ch)
			&& ((!IS_NPC(victim) && !is_same_group(ch, victim))
			|| (IS_NPC(victim) && !is_same_group(ch, victim) 
				&& IS_AFFECTED(victim, AFF_CHARM)))) {
				WAIT_STATE(ch, MISSING_TARGET_DELAY);
				char_puts("Since you are a newbie, you can't cast on others.\n",ch);
				return;
			}

			if (IS_SET(spell->flags, SKILL_QUESTIONABLE)
			&&  !check_trust(ch, victim)) {
				char_puts("They do not trust you enough "
					  "for that.\n", ch);
				return;
			}
			break;

		case TAR_CHAR_OFFENSIVE:
		case TAR_OBJ_CHAR_OFF:
			offensive = TRUE;
			if (victim 
			&& IS_NEWBIE(ch)
			&& ((!IS_NPC(victim) && !is_same_group(ch, victim))
			|| (IS_NPC(victim) && !is_same_group(ch, victim) 
				&& IS_AFFECTED(victim, AFF_CHARM)))) {
				WAIT_STATE(ch, MISSING_TARGET_DELAY);
				char_puts("Since you are a newbie, you can't cast on others.\n",ch);
				return;
			}

			if (IS_SET(spell->flags, SKILL_QUESTIONABLE))
				offensive = !check_trust(ch, victim);

			if (offensive && is_safe(ch, victim))
				return;
		}
		break;

	case TARGET_OBJ:
		vo = (void*) obj;
		break;
	}

	WAIT_STATE(ch, spell->beats);

	if (cast_type == CAST_CAST 
	&& bch 
	&& spellbane(bch, ch, bane_bonus, 3 * bch->level))
		return;

	/* do the yelling */
	switch (target) {
	case TARGET_CHAR:
		switch (spell->target) {
		case TAR_CHAR_OFFENSIVE:
		case TAR_OBJ_CHAR_OFF:

			if (!IS_NPC(ch)
			&&  !IS_NPC(victim)
			&&  victim != ch
			&&  ch->fighting != victim
			&&  victim->fighting != ch
			&&  !is_same_group(ch, victim)) {
				doprintf(do_yell, victim,
					 "Die, %s, you sorcerous dog!",
					 PERS(ch, victim));
			}
			break;
		}
	}

	/* drunks have harder time casting */
	switch (cast_type) {
		case CAST_COMMUNE:
			percent_drunk_fail = 10;
			break;
		case CAST_CHANNEL:
			percent_drunk_fail = 15;
			break;
		case CAST_CAST:
		default:
			percent_drunk_fail = 20;
	}
	switch (ch->race) {
		case RACE_DWARF:
		case RACE_DUERGAR:
			percent_drunk_fail -= 5;
	}

	if (number_percent() > chance
	|| (IS_DRUNK(ch) && number_percent() < percent_drunk_fail)) {
		char_puts("You lost your concentration.\n", ch);
		if (!IS_DRUNK(ch)) 
			check_improve(ch, sn, FALSE, 1);
		ch->mana -= mana / 2;
		if (cast_far) cast_far = FALSE;
	}
	else {
		if (IS_SET(cl->flags, CLASS_MAGIC))
			slevel = LEVEL(ch) - UMAX(0, (LEVEL(ch) / 20));
		else
			slevel = LEVEL(ch) - UMAX(5, (LEVEL(ch) / 10));

		if ((chance = get_skill(ch, gsn_spell_craft))) {
			if (number_percent() < chance) {
				slevel = LEVEL(ch); 
				check_improve(ch, gsn_spell_craft, TRUE, 2);
			}
			else 
				check_improve(ch, gsn_spell_craft, FALSE, 2);
		}

		if (IS_SET(spell->group, GROUP_MALADICTIONS)
		&&  (chance = get_skill(ch, gsn_improved_maladiction))) {
			if (number_percent() < chance) {
				slevel = LEVEL(ch);
				slevel += chance/20;
				check_improve(ch, gsn_improved_maladiction,
					      TRUE, 4);
			}
			else
				check_improve(ch, gsn_improved_maladiction,
					      FALSE, 4);
		}

		if (IS_SET(spell->group, GROUP_BENEDICTIONS)
		&&  (chance = get_skill(ch, gsn_improved_benediction))) {
			if (number_percent() < chance) {
				slevel = LEVEL(ch);
				slevel += chance/10;
				check_improve(ch, gsn_improved_benediction,
					      TRUE, 4);
			}
			else 
				check_improve(ch, gsn_improved_benediction,
					      FALSE, 4);
		}

		if (IS_SET(spell->group, GROUP_CURATIVE)
		&&  (chance = get_skill(ch, gsn_improved_curative))) {
			if (number_percent() < chance) {
				slevel = LEVEL(ch);
				slevel += chance/10;
				check_improve(ch, gsn_improved_curative,
					      TRUE, 4);
			}
			else 
				check_improve(ch, gsn_improved_curative,
					      FALSE, 4);
		}

		if (IS_SET(spell->group, GROUP_HEALING)
		&&  (chance = get_skill(ch, gsn_improved_healing))) {
			if (number_percent() < chance) {
				slevel = LEVEL(ch);
				slevel += chance/10;
				check_improve(ch, gsn_improved_healing,
					      TRUE, 4);
			}
			else 
				check_improve(ch, gsn_improved_healing,
					      FALSE, 4);
		}

		if (!IS_NPC(ch)
		&& IS_SET(cl->flags, CLASS_MAGIC))
			slevel = UMAX(1, slevel/2 
				+  (slevel/2) 
				* get_curr_stat(ch, STAT_INT) / 60);
		else
			slevel = UMAX(1, slevel * 2/3 
				+  (slevel/3) 
				* get_curr_stat(ch, STAT_INT) / 55);

		/* bonus for being true to your alignment */
		if (align_standing_important(ch))
			slevel += align_standing_bonus(ch);

		if ((chance = get_skill(ch, gsn_mastering_spell))
		&&  number_percent() < chance) {
			slevel += number_range(1,1); 
			check_improve(ch, gsn_mastering_spell, TRUE, 6);
		}

		if (IS_AFFECTED(ch, AFF_CURSE)) {
			AFFECT_DATA* paf;
			for (paf = ch->affected; paf; paf = paf->next) 
				if (paf->type == gsn_anathema
				&& paf->location == APPLY_LEVEL)
					slevel += paf->modifier * 3;
		}

		if (slevel < 1)
			slevel = 1;

		ch->mana -= mana;

		slevel = IS_NPC(ch) ? ch->level : slevel;

		DEBUG(DEBUG_MAGIC_CAST_LEVEL, 
			"cast: %s[%d] spell '%s' lvl %d",
			CHAR_NAME(ch), 
			LEVEL(ch), 
			spell->name,
			slevel);

		spell->spell_fun(sn, slevel, ch, vo, target);
		check_improve(ch, sn, TRUE, 1);
	}
		
	if (cast_far && door != -1)
		path_to_track(ch, victim, door);
	else if (offensive && victim != ch && victim->master != ch) {
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;

		for (vch = ch->in_room->people; vch; vch = vch_next) {
			vch_next = vch->next_in_room;
			if (victim == vch && victim->fighting == NULL) {
				if (victim->position != POS_SLEEPING)
					multi_hit(victim, ch, TYPE_UNDEFINED);
				break;
			}
		}
	}
}

void do_cast(CHAR_DATA *ch, const char *argument) {
	switch (cast_type(ch)) {
		case CAST_COMMUNE:
			char_puts("You don't know how to cast spells, maybe try to 'commune' your prayers.", ch);
			return;
			break;
		case CAST_CHANNEL:
			char_puts("You don't know how to cast spells, maybe try to 'channel' your ki.", ch);
			return;
		default:
			cast_common(ch, argument, CAST_CAST);
	}
}
void do_commune(CHAR_DATA *ch, const char *argument) {
	switch (cast_type(ch)) {
		case CAST_COMMUNE:
			cast_common(ch, argument, CAST_COMMUNE);
			break;
		case CAST_CHANNEL:
			char_puts("You don't know how to commune prayers, maybe try to 'channel' your ki.", ch);
			return;
		default:
			char_puts("You don't know how to commune prayers, maybe try to 'cast' your spells.", ch);
			return;
	}
}
void do_channel(CHAR_DATA *ch, const char *argument) {
	switch (cast_type(ch)) {
		case CAST_COMMUNE:
			char_puts("You don't know how to channel your ki, maybe try to 'commune' your prayers.", ch);
			break;
		case CAST_CHANNEL:
			cast_common(ch, argument, CAST_CHANNEL);
			return;
		default:
			char_puts("You don't know how to channel your ki, maybe try to 'cast' your spells.", ch);
			return;
	}
}

/*
 * Cast spells at targets using a magical object.
 */
void obj_cast_spell(int sn, int level,
		    CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj)
{
	void *vo = NULL;
	int target = TARGET_NONE;
	skill_t *spell;
	CHAR_DATA *bch = NULL;
	int bane_bonus = 0;
	int bane_damage = 0;
	bool offensive = FALSE;

	if (sn <= 0
	||  (spell = skill_lookup(sn)) == NULL
	||  spell->spell_fun == NULL)
		return;

	/* cases the object has many spells on it, and the character
	 * or victim got killed by the previous one */
	if ((ch && !ch->name) || (victim && !victim->name))
		return;

	switch (spell->target) {
	default:
		bug("Obj_cast_spell: bad target for sn %d.", sn);
		return;

	case TAR_IGNORE:
		bch = ch;
		bane_damage = 10*bch->level;
		break;

	case TAR_CHAR_OFFENSIVE:
		if (victim == NULL)
			victim = ch->fighting;
		if (victim == NULL) {
			char_puts("You can't do that.\n", ch);
			return;
		}

		target = TARGET_CHAR;
		bch = victim;
		bane_damage = 10*bch->level;
		/*XXX- why would object be null?*/
		bane_bonus = (LEVEL(bch) 
			- ((obj) ? obj->pIndexData->level : (LEVEL(ch))));
		break;

	case TAR_CHAR_DEFENSIVE:
	case TAR_CHAR_SELF:
		if (victim == NULL)
			victim = ch;
		target = TARGET_CHAR;
		bch = victim;
		bane_damage = 10*bch->level;
		break;

	case TAR_OBJ_INV:
		if (obj == NULL) {
			char_puts("You can't do that.\n", ch);
			return;
		}
		target = TARGET_OBJ;
		bch = ch;
		bane_damage = 3*bch->level;
		break;

	case TAR_OBJ_CHAR_OFF:
		if (!victim && !obj) {
			if (ch->fighting)
				victim = ch->fighting;
			else {
				char_puts("You can't do that.\n", ch);
				return;
			}
		}

		if (victim) {
			target = TARGET_CHAR;
			bch = victim;
			bane_damage = 3*bch->level;
		}
		else
			target = TARGET_OBJ;
		break;

	case TAR_OBJ_CHAR_DEF:
		if (!victim && !obj) {
			victim = ch;
			target = TARGET_CHAR;
		}
		else if (victim) {
			target = TARGET_CHAR;
		}
		else 
			target = TARGET_OBJ;

		if (victim) {
			bch = victim;
			bane_damage = 3*bch->level;
		}
		break;
	}

	switch (target) {
	case TARGET_NONE:
		vo = NULL;
		break;

	case TARGET_OBJ:
		if (obj->extracted)
			return;
		vo = (void*) obj;
		break;

	case TARGET_CHAR:
		vo = (void *) victim;

		switch (spell->target) {
		case TAR_CHAR_DEFENSIVE:
		case TAR_OBJ_CHAR_DEF:
			if (IS_SET(spell->flags, SKILL_QUESTIONABLE)
			&&  !check_trust(ch, victim)) {
				char_puts("They do not trust you enough "
					  "for this spell.\n", ch);
				return;
			}
			break;

		case TAR_CHAR_OFFENSIVE:
		case TAR_OBJ_CHAR_OFF:
			offensive = TRUE;
			if (IS_SET(spell->flags, SKILL_QUESTIONABLE))
				offensive = !check_trust(ch, victim);

			if (offensive) {
				if (is_safe(ch, victim)) {
					char_puts("Something isn't right...\n",
						  ch);
					return;
				}
			}
			break;
		}
	}

	if (bch && spellbane(bch, ch, bane_bonus, bane_damage))
		return;

	target_name = str_empty;
	spell->spell_fun(sn, level, ch, vo, target);

	if (offensive && victim != ch && victim->master != ch) {
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;

		for (vch = ch->in_room->people; vch; vch = vch_next) {
			vch_next = vch->next_in_room;

			if (victim == vch)
				doprintf(do_yell, victim,
					 "Help! %s is attacking me!",
					 PERS(ch, victim));

			if (victim == vch && victim->fighting == NULL) {
				multi_hit(victim, ch, TYPE_UNDEFINED);
				break;
			}
		}
	}
}

/*
 * Spell functions.
 */
void spell_acid_blast(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level, 14);
	if (saves_spell(level, victim, DAM_ACID))
		dam /= 2;
	damage(ch, victim, dam, sn,DAM_ACID,TRUE);
	return;
}

void spell_armor(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (is_affected(victim, sn))
	{
		if (victim == ch)
		  char_puts("You are already armored.\n",ch);
		else
		  act("$N is already armored.",ch,NULL,victim,TO_CHAR);
		return;
	}
	af.where	 = TO_AFFECTS;
	af.type      = sn;
	af.level	 = level;
	af.duration  = 7 + level / 6;
	af.modifier  = -1 * UMAX(20,10 + level / 4); /* af.modifier  = -20;*/
	af.location  = APPLY_AC;
	af.bitvector = 0;
	affect_to_char(victim, &af);
	char_puts("You feel someone protecting you.\n", victim);
	act("$n is protected by mystical armor.",victim,NULL,NULL,TO_ROOM);
	return;
}

void spell_bone_armor(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
        CHAR_DATA *victim = (CHAR_DATA *) vo;
        AFFECT_DATA af;

        if (is_affected(victim, sn))
        {
                if (victim == ch)
                  char_puts("You are already armored by bones.\n",ch);
                else
                  act("$N is already armored with bones.",ch,NULL,victim,TO_CHAR);
                return;
        }
        af.where         = TO_AFFECTS;
        af.type      = sn;
        af.level         = level;
        af.duration  = 7 + level / 6;
        af.modifier  = -1 * UMAX(20,10 + level / 3); /* af.modifier  = -20;*/
        af.location  = APPLY_AC;
        af.bitvector = 0;
        affect_to_char(victim, &af);
        char_puts("You feel bones wrapping around you.\n", victim);
        act("$n is wrapped in an armor of bone.",victim,NULL,NULL,TO_ROOM);
}



void spell_bless(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	AFFECT_DATA af;

	/* deal with the object case first */
	if (target == TARGET_OBJ)
	{
		obj = (OBJ_DATA *) vo;
		if (IS_OBJ_STAT(obj,ITEM_BLESS))
		{
		    act("$p is already blessed.",ch,obj,NULL,TO_CHAR);
		    return;
		}

		if (IS_OBJ_STAT(obj,ITEM_EVIL))
		{
		    AFFECT_DATA *paf;

		    paf = affect_find(obj->affected,gsn_curse);
		    if (!saves_dispel(level,paf != NULL ? paf->level : obj->level,0))
		    {
			if (paf != NULL)
			    affect_remove_obj(obj,paf);
			act("$p glows a pale blue.",ch,obj,NULL,TO_ALL);
			REMOVE_BIT(obj->extra_flags,ITEM_EVIL);
			return;
		    }
		    else
		    {
			act("The evil of $p is too powerful for you to overcome.",
			    ch,obj,NULL,TO_CHAR);
			return;
		    }
		}

		af.where    = TO_OBJECT;
        	af.type      = sn;
        	af.level     = level;
		af.duration  = (6 + level / 2);
        	af.bitvector = ITEM_BLESS;

		switch (obj->pIndexData->item_type) {
			case ITEM_WEAPON: 
				af.modifier  = 1 * UMIN(2,level / 10); 
				af.location  = APPLY_HITROLL;
				break;
			case ITEM_ARMOR: 
			default:
				af.modifier  = -1 * UMIN(2,level / 10); 
				af.location  = APPLY_AC;
				break;
		}
		affect_to_obj(obj,&af);

		act("$p glows with a holy aura.",ch,obj,NULL,TO_ALL);
		return;
	}

	/* character target */
	victim = (CHAR_DATA *) vo;


	if (is_affected(victim, sn))
	{
		if (victim == ch)
		  char_puts("You are already blessed.\n",ch);
		else
		  act("$N already has divine favor.",ch,NULL,victim,TO_CHAR);
		return;
	}

	if (is_affected(victim, gsn_warcry))
	{
		act("$N is already receiving a blessing.", ch, NULL, victim, TO_CHAR);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level	 = level;
	af.duration  = (6 + level / 2);
	af.location  = APPLY_HITROLL;
	af.modifier  = level / 8;
	af.bitvector = 0;
	affect_to_char(victim, &af);

	af.location  = APPLY_SAVING_SPELL;
	af.modifier  = 0 - level / 8;
	affect_to_char(victim, &af);
	char_puts("You feel righteous.\n", victim);
	if (ch != victim) {
		act("$N is granted the favor of your god.",ch,NULL,victim,TO_CHAR);
		act("$N is granted the favor of $n's god.",ch,NULL,victim,TO_NOTVICT);
	}
	else
		act("$N is granted the favor of $s god.",ch,NULL,victim,TO_NOTVICT);
}



void spell_blindness(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (IS_AFFECTED(victim, AFF_BLIND)) {
		act("$N is already blind.",ch,NULL,victim,TO_CHAR);
		return;
	}
	if (saves_spell(level,victim,DAM_OTHER))  {
		act("$N resists the blindness.",ch,NULL,victim,TO_CHAR);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.location  = APPLY_HITROLL;
	af.modifier  = -4;
	af.duration  = 3+level / 15;
	af.bitvector = AFF_BLIND;
	affect_to_char(victim, &af);
	char_puts("You are blinded!\n", victim);
	act("$n appears to be blinded.",victim,NULL,NULL,TO_ROOM);
	return;
}


/*
 * Delve: Cleric spell to allow the status of a target to be seen.
 */
void spell_delving(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int percent;

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
}
	 
void spell_burning_hands(int sn,int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level , 2) + 7;
	if (saves_spell(level, victim,DAM_FIRE))
		dam /= 2;
	damage(ch, victim, dam, sn, DAM_FIRE,TRUE);
	return;
}

void spell_call_lightning(int sn, int level,CHAR_DATA *ch,void *vo, int target)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int dam;

	if (!IS_OUTSIDE(ch)) {
		char_puts("You must be out of doors.\n", ch);
		return;
	}

	if (weather_info.sky < SKY_RAINING) {
		char_puts("You need bad weather.\n", ch);
		return;
	}

	dam = dice(level, 9);

	char_puts("Gods' lightning strikes your foes!\n", ch);
	act("$n calls lightning to strike $s foes!", ch, NULL, NULL, TO_ROOM);

	for (vch = char_list; vch; vch = vch_next) {
		vch_next	= vch->next;

		if (vch->in_room == NULL)
			continue;

		if (vch->in_room == ch->in_room) {
			if (is_safe_spell(ch, vch, TRUE))
				continue;
			damage(ch, vch,
			       saves_spell(level, vch, DAM_LIGHTNING) ?
			       dam / 2 : dam,
			       sn,DAM_LIGHTNING,TRUE);
			continue;
		}

		if (vch->in_room->area == ch->in_room->area
		&&  IS_OUTSIDE(vch)
		&&  IS_AWAKE(vch))
		    char_puts("Lightning flashes in the sky.\n", vch);
	}
}

/* RT calm spell stops all fighting in the room */
void spell_calm(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *vch;
	int mlevel = 0;
	int count = 0;
	int high_level = 0;
	int chance;
	AFFECT_DATA af;

	/* get sum of all mobile levels in the room */
	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	{
		if (vch->position == POS_FIGHTING)
		{
		    count++;
		    if (IS_NPC(vch))
		    	mlevel += vch->level;
		    else
		    	mlevel += vch->level/2;

		    high_level = UMAX(high_level,vch->level);
		}
	}

	/* compute chance of stopping combat */
	chance = 4 * level - high_level + 2 * count;

	if (IS_IMMORTAL(ch)) /* always works */
	  mlevel = 0;

	act("You extend the serenity of your mind.",
		ch, NULL, NULL, TO_CHAR);
	
	if (ch->fighting || ch->position == POS_FIGHTING)
		stop_fighting(ch, FALSE);

	if (number_range(0, chance) >= mlevel) { /* hard to stop large fights */
		for (vch = ch->in_room->people; vch; vch = vch->next_in_room) {

			if (ch == vch) continue;

			if (IS_NPC(vch)
			&&  (IS_SET(vch->imm_flags, IMM_MAGIC) ||
			     IS_SET(vch->pIndexData->act, ACT_UNDEAD))) {
				act("$n fails to sooth you.",
				   ch, NULL, vch, TO_VICT);
				act("$N ignores your soothing words.",
				   ch, NULL, vch, TO_CHAR);
				continue;
			}

			if (IS_AFFECTED(vch, AFF_CALM)) {
				act("$N can be calmed no more.",
				   ch, NULL, vch, TO_CHAR);
				continue;
			}

			if (IS_AFFECTED(vch, AFF_BERSERK)
			||  is_affected(vch, gsn_frenzy)) {
				act("$n fails to sooth your rage.",
				   ch, NULL, vch, TO_VICT);
				act("$N's rage is too strong to be soothed.",
				   ch, NULL, vch, TO_CHAR);
				continue;
			}

			if (vch->fighting || vch->position == POS_FIGHTING)
				stop_fighting(vch, FALSE);

			af.where = TO_AFFECTS;
			af.type = sn;
			af.level = level;
			af.duration = level/4;
			af.location = APPLY_HITROLL;
			if (!IS_NPC(vch))
				af.modifier = -5;
			else
				af.modifier = -2;
			af.bitvector = AFF_CALM;
			affect_to_char(vch, &af);

			af.location = APPLY_DAMROLL;
			affect_to_char(vch, &af);

			char_puts("A wave of calm passes over you.\n", vch);
			act("$N is pacified by your soothing.",
			   ch, NULL, vch, TO_CHAR);
		}
	}
	else {
		char_puts("Your soothing words fall on deaf ears.\n",
			ch);
	}
}

void spell_cancellation(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	bool found = FALSE;

	level += 2;

	/* unlike dispel magic, the victim gets NO save */

	/* begin running through the spells */
	found = dispel_affects(level, victim);

	if (!found) {
		char_puts("You cancelled nothing.\n",ch);
		act("$N seems unaffected by $n's efforts.\n",
		   ch, NULL, victim, TO_NOTVICT);
	}
}

void spell_cause_light(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	damage(ch, (CHAR_DATA *) vo, dice(1, 8) + level / 3, sn,DAM_HARM,TRUE);
}

void spell_cause_critical(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
	damage(ch, (CHAR_DATA *) vo, dice(3, 8) + level - 6, sn,DAM_HARM,TRUE);
}

void spell_cause_serious(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
	damage(ch, (CHAR_DATA *) vo, dice(2, 8) + level / 2, sn,DAM_HARM,TRUE);
}

void spell_chain_lightning(int sn,int level,CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	CHAR_DATA *vch,*last_vict,*next_vict;
	bool found;
	int dam;

	/* first strike */

	act("A lightning bolt leaps from $n's hand and arcs to $N.",
		ch,NULL,victim,TO_ROOM);
	act("A lightning bolt leaps from your hand and arcs to $N.",
		ch,NULL,victim,TO_CHAR);
	act("A lightning bolt leaps from $n's hand and hits you!",
		ch,NULL,victim,TO_VICT);

	dam = dice(level,6);
	if (saves_spell(level,victim,DAM_LIGHTNING))
		dam /= 3;
	damage(ch,victim,dam,sn,DAM_LIGHTNING,TRUE);

	last_vict = victim;
	level -= 4;   /* decrement damage */

	/* new targets */
	while (level > 0) {
		found = FALSE;
		for (vch = ch->in_room->people; vch; vch = next_vict) {
			next_vict = vch->next_in_room;

			if (vch == last_vict)
				continue;

			if (is_safe_spell(ch, vch, TRUE)) {
				act("The bolt passes around $n's body.",
				    ch, NULL, NULL, TO_ROOM);
				act("The bolt passes around your body.",
				    ch, NULL, NULL, TO_CHAR);
				continue;
			}

			found = TRUE;
			last_vict = vch;
			act("The bolt arcs to $n!", vch, NULL, NULL, TO_ROOM);
			act("The bolt hits you!", vch, NULL, NULL, TO_CHAR);
			dam = dice(level,6);

			if (saves_spell(level, vch, DAM_LIGHTNING))
				dam /= 3;
			damage(ch, vch, dam, sn, DAM_LIGHTNING, TRUE);
			level -= 4;  /* decrement damage */
		}   /* end target searching loop */

		if (found)
			continue;

/* no target found, hit the caster */
		if (ch == NULL)
			return;

		if (last_vict == ch) { /* no double hits */
			act("The bolt seems to have fizzled out.",
			    ch, NULL, NULL, TO_ROOM);
			act("The bolt grounds out through your body.",
			    ch, NULL, NULL, TO_CHAR);
			return;
		}

		last_vict = ch;
		act("The bolt arcs to $n...whoops!", ch, NULL, NULL, TO_ROOM);
		char_puts("You are struck by your own lightning!\n", ch);
		dam = dice(level,6);
		if (saves_spell(level, ch, DAM_LIGHTNING))
			dam /= 3;
		damage(ch, ch, dam, sn, DAM_LIGHTNING, TRUE);
		level -= 4;  /* decrement damage */
	} /* now go back and find more targets */
}

void spell_healing_light(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	AFFECT_DATA af,af2;

	if (is_affected_room(ch->in_room, sn))
	{
		char_puts("This room has already been healed by light.\n",ch);
		return;
	}

	af.where     = TO_ROOM_CONST;
	af.type      = sn;
	af.level     = level;
	af.duration  = level / 25;
	af.location  = APPLY_ROOM_HEAL;
	af.modifier  = level;
	af.bitvector = 0;
	affect_to_room(ch->in_room, &af);

	af2.where     = TO_AFFECTS;
	af2.type      = sn;
	af2.level	 = level;
	af2.duration  = level / 10;
	af2.modifier  = 0;
	af2.location  = APPLY_NONE;
	af2.bitvector = 0;
	affect_to_char(ch, &af2);
	char_puts("The room starts to be filled with healing light.\n", ch);
	act("The room starts to be filled with $n's healing light.",ch,NULL,NULL,TO_ROOM);
}

void spell_charm_person(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	char buf[MAX_INPUT_LENGTH];
	AFFECT_DATA af;
	int ladj = 0;		/* level adjustment (for opposite sex) */

	if (count_charmed(ch))
		return;

	if (victim == ch) {
		char_puts("You like yourself even better!\n", ch);
		return;
	}

	if (!IS_NPC(victim) && !IS_NPC(ch)) {
		if (get_curr_stat(ch, STAT_CHA) > 20)
			ladj += get_curr_stat(ch, STAT_CHA) - 20;
		else if (get_curr_stat(ch, STAT_CHA) < 18)
			ladj += get_curr_stat(ch, STAT_CHA) - 18;
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

	if  ((victim->in_room &&
	     IS_SET(victim->in_room->room_flags, ROOM_BATTLE_ARENA))) {
		act("You feel this isn't the right environment to work your charms.",
		   ch, NULL, victim, TO_CHAR);
		return;
	}

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
	af.duration	= number_fuzzy(level / 5);
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

void spell_chill_touch(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int dam;

	dam = number_range(1, level);
	if (!saves_spell(level, victim, DAM_COLD)) {
		act("$n turns blue and shivers.", victim, NULL, NULL, TO_ROOM);
		af.where     = TO_AFFECTS;
		af.type      = sn;
		af.level     = level;
		af.duration  = 6;
		af.location  = APPLY_STR;
		af.modifier  = -1;
		af.bitvector = 0;
		affect_join(victim, &af);
	}
	else
		dam /= 2;

	damage(ch, victim, dam, sn, DAM_COLD, TRUE);
}

void spell_colour_spray(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level,3) + 13;
	if (saves_spell(level, victim, DAM_LIGHT))
		dam /= 2;
	else
		spell_blindness(sn_lookup("blindness"),
				level/2, ch, (void *) victim, TARGET_CHAR);

	damage(ch, victim, dam, sn, DAM_LIGHT, TRUE);
}

void spell_continual_light(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
	int fuel = 0;
	OBJ_DATA *light = (OBJ_DATA *) vo;

	/*if (target_name[0] != '\0')*/  /* do a glow on some object */

	if (light)  /* do a glow on some object */
	{
		/*light = get_obj_carry(ch,target_name);*/

		if (light == NULL)
		{
		    char_puts("You don't see that here.\n",ch);
		    return;
		}
		if (light->pIndexData->item_type != ITEM_LIGHT 
		&& IS_OBJ_STAT(light,ITEM_GLOW)) {
		    act("$p is already glowing.",ch,light,NULL,TO_CHAR);
		    return;
		}

		if (light->value[ITEM_LIGHT_DURATION] 
		< light->pIndexData->value[ITEM_LIGHT_DURATION]) {
			fuel = light->pIndexData->value[ITEM_LIGHT_DURATION] 
				- light->value[ITEM_LIGHT_DURATION];
			light->value[ITEM_LIGHT_DURATION] 
				+= UMAX(fuel / 2, fuel * level / 100);
			act("$p brims with rekindled light.", 
				ch, light, NULL, TO_ALL);
		}
		else
			act("$p glows with a white light.",ch,light,NULL,TO_ALL);

		SET_BIT(light->extra_flags,ITEM_GLOW);

		return;
	}

	light = create_obj(get_obj_index(OBJ_VNUM_LIGHT_BALL), 0);
	obj_to_room(light, ch->in_room);
	act("$n twiddles $s thumbs and $p appears.",   ch, light, NULL, TO_ROOM);
	act("You twiddle your thumbs and $p appears.", ch, light, NULL, TO_CHAR);
	return;
}



void spell_control_weather(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
	if (!str_cmp(target_name, "better"))
		weather_info.change += dice(level / 3, 4);
	else if (!str_cmp(target_name, "worse"))
		weather_info.change -= dice(level / 3, 4);
	else  {
		char_puts ("Do you want it to get better or worse?\n", ch);
		return;
	}

	char_puts("The weather bends to your will.\n", ch);
	return;
}



void spell_create_food(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	OBJ_DATA *mushroom;

	mushroom = create_obj(get_obj_index(OBJ_VNUM_MUSHROOM), 0);
	mushroom->value[ITEM_FOOD_HOURS] = level / 2;
	mushroom->value[ITEM_FOOD_FULL] = level;
	obj_to_room(mushroom, ch->in_room);
	act("$p suddenly appears.", ch, mushroom, NULL, TO_ROOM);
	act("$p suddenly appears.", ch, mushroom, NULL, TO_CHAR);
	return;
}

void spell_create_rose(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	OBJ_DATA *rose = create_obj(get_obj_index(OBJ_VNUM_ROSE), 0);
	act("$n has created $p.", ch, rose, NULL, TO_ROOM);
	act("You create $p.", ch, rose, NULL, TO_CHAR);
	obj_to_char(rose, ch);
}

void spell_create_spring(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
	OBJ_DATA *spring;

	spring = create_obj(get_obj_index(OBJ_VNUM_SPRING), 0);
	spring->timer = level;
	obj_to_room(spring, ch->in_room);
	act("$p flows from the ground.", ch, spring, NULL, TO_ROOM);
	act("$p flows from the ground.", ch, spring, NULL, TO_CHAR);
	return;
}

void spell_create_water(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	int water;

	if (obj->pIndexData->item_type != ITEM_DRINK_CON) {
		char_puts("It is unable to hold water.\n", ch);
		return;
	}

	if (obj->value[ITEM_DRINK_TYPE] != LIQ_WATER 
	&& obj->value[ITEM_DRINK_REMAINING] != 0) {
		char_puts("It contains some other liquid.\n", ch);
		return;
	}

	water = UMIN(level * (weather_info.sky >= SKY_RAINING ? 4 : 2),
		     obj->value[ITEM_DRINK_TOTAL] 
		     - obj->value[ITEM_DRINK_REMAINING]);

	if (water > 0) {
		obj->value[ITEM_DRINK_TYPE] = LIQ_WATER;
		obj->value[ITEM_DRINK_REMAINING] += water;

		if (!is_name("water", obj->name)) {
			const char *p = obj->name;
			obj->name = str_printf("%s %s", obj->name, "water");
			free_string(p);
		}

		act("$p is filled.", ch, obj, NULL, TO_CHAR);
	}
}



void spell_cure_blindness(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int fbsn = sn_lookup("fire breath");

	if ((!is_affected(victim, gsn_blindness)) 
	&& (!is_affected(victim, fbsn))
	&& (!is_affected(victim, gsn_rblind_trap))) {
		if (victim == ch)
			char_puts("You aren't blind.\n",ch);
		else
			act("$N doesn't appear to be blinded.",
				ch,NULL,victim,TO_CHAR);
		return;
	}

	if (is_affected(victim, gsn_blindness) 
	&& check_dispel(level,victim,gsn_blindness)) {
		char_puts("Your vision returns!\n", victim);
		act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM);
	}
	else if(is_affected(victim, fbsn)
	&& check_dispel(level,victim,fbsn)) {
		char_puts("Your eyes heal and your vision returns!\n", victim);
		act("$n can see again.",victim,NULL,NULL,TO_ROOM);
	}
	else if(is_affected(victim, gsn_rblind_trap) 
	&& check_dispel(level,victim,gsn_rblind_trap))
	{
		char_puts("Your vision returns!\n",victim);
		act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM);
	}
	else
		char_puts("Your attempt to restore vision fails.\n",ch);
}


void spell_cure_disease(int sn, int level, CHAR_DATA *ch,void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;

	if (!is_affected(victim, gsn_plague))
	{
		if (victim == ch)
		  char_puts("You aren't ill.\n",ch);
		else
		  act("$N doesn't appear to be diseased.",ch,NULL,victim,TO_CHAR);
		return;
	}

	if (check_dispel(level,victim,gsn_plague))
	{
		char_puts("Your sores vanish.\n",victim);
		act("$n looks relieved as $s sores vanish.",victim,NULL,NULL,TO_ROOM);
	}
	else
		char_puts("You are unable to route out the disease.\n",ch);
}


/* RT added to cure plague */

void spell_cure_poison(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;

	if (!is_affected(victim, gsn_poison))
	{
		if (victim == ch)
		  char_puts("You aren't poisoned.\n",ch);
		else
		  act("$N doesn't appear to be poisoned.",ch,NULL,victim,TO_CHAR);
		return;
	}

	if (check_dispel(level,victim,gsn_poison))
	{
		char_puts("A warm feeling runs through your body.\n",victim);
		act("$n no longer looks ill.",victim,NULL,NULL,TO_ROOM);
	}
	else {
		char_puts("The poison resists your efforts.\n",ch);
		act("$n's efforts have no affect on the poison in your body.\n",
		   ch, NULL, victim, TO_VICT);
		act("$n fails to overcome $N's poison.\n",
		   ch, NULL, victim,TO_NOTVICT);
	}
}

/*
 * Health spells
 */
void spell_cure_light(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;

	heal(ch, NULL, victim, sn, level, dice(1,8) + level / 4 + 5);
	return;
}

void spell_cure_serious(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;

	heal(ch, NULL, victim, sn, level, dice(2,8) + level / 3 + 10);
	return;
}

void spell_cure_critical(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;

	heal(ch, NULL, victim, sn, level, dice(3,8) + level / 2);
	return;
}

void spell_heal(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	heal(ch, NULL, victim, sn, level, 100 + level / 10);
	return;
}

void spell_superior_heal(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	heal(ch, NULL, victim, sn, level, 170 + level + dice(1,20));
	return;
}

void spell_master_healing(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	heal(ch, NULL, victim, sn, level, 300 + level + dice(1,40));
	return;
}

void spell_assist(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (is_affected(ch, sn)) {
		char_puts("This power is used too recently.\n",ch);
		return;
	}

	af.where	 = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = 15 + level / 50;
	af.location  = 0;
	af.modifier  = 0;
	af.bitvector = 0;
	affect_to_char(ch, &af);

	heal(ch, NULL, victim, sn, level, 100 + level * 3);
	return;
}  
	
void spell_aid(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (is_affected(ch, sn)) {
		char_puts("This power is used too recently.\n",ch);
		return;
	}

	af.where	 = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = level / 50;
	af.location  = 0;
	af.modifier  = 0;
	af.bitvector = 0;
	affect_to_char(ch, &af);

	heal(ch, NULL, victim, sn, level, level * 5);
	return;
}  

void spell_group_heal(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *gch;
	int heal_sn, refresh_sn;

	heal_sn = sn_lookup("master healing");
	refresh_sn = sn_lookup("refresh");

	if (heal_sn < 0 || refresh_sn < 0)
		return;

	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room) {
		if (is_same_group(ch, gch)) {
			spell_heal(heal_sn, level, ch, gch, TARGET_CHAR);
			spell_refresh(refresh_sn, level, ch, gch, TARGET_CHAR);
		}
	}
}

void spell_restoring_light(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int nsn, mana_add = 0;

	if (IS_AFFECTED(victim,AFF_BLIND))
	{
	 nsn = sn_lookup("cure blindness");
	 spell_cure_blindness(nsn,level,ch,(void *)victim,TARGET_CHAR);
	}
	if (IS_AFFECTED(victim,AFF_CURSE))
	{
	 nsn = sn_lookup("remove curse");
	 spell_remove_curse(nsn,level,ch,(void *)victim,TARGET_CHAR);
	}
	if (IS_AFFECTED(victim,AFF_POISON))
	{
	 spell_cure_poison(gsn_cure_poison,level,ch,(void *)victim,TARGET_CHAR);
	}
	if (IS_AFFECTED(victim,AFF_PLAGUE))
	{
	 nsn = sn_lookup("cure disease");
	 spell_cure_disease(nsn,level,ch,(void *)victim,TARGET_CHAR);
	}

	if (victim->hit != victim->max_hit)
	{
		 mana_add = UMIN((victim->max_hit - victim->hit), ch->mana);
		 ch->mana -= mana_add;
	}
 	heal(ch, NULL, victim, sn, level, mana_add);
	return;
}

void spell_curse(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	AFFECT_DATA af;

	/* deal with the object case first */
	if (target == TARGET_OBJ)
	{
		obj = (OBJ_DATA *) vo;
		if (IS_OBJ_STAT(obj,ITEM_EVIL))
		{
		    act("$p is already filled with evil.",ch,obj,NULL,TO_CHAR);
		    return;
		}

		if (IS_OBJ_STAT(obj,ITEM_BLESS))
		{
		    AFFECT_DATA *paf;

		    paf = affect_find(obj->affected,sn_lookup("bless"));
		    if (!saves_dispel(level,paf != NULL ? paf->level : obj->level,0))
		    {
			if (paf != NULL)
			    affect_remove_obj(obj,paf);
			act("$p glows with a red aura.",ch,obj,NULL,TO_ALL);
			REMOVE_BIT(obj->extra_flags,ITEM_BLESS);
			return;
		    }
		    else
		    {
			act("The holy aura of $p is too powerful for you to overcome.",
			    ch,obj,NULL,TO_CHAR);
			return;
		    }
		}

                af.where        = TO_OBJECT;
                af.type         = sn;
                af.level        = level;
                af.duration     = -1;
		af.duration	= (6 + level / 2);
                af.bitvector    = ITEM_EVIL;

		switch (obj->pIndexData->item_type) {
			case ITEM_WEAPON:
				af.modifier	= -1 * UMIN(10,level / 10); 
				af.location     = APPLY_HITROLL;
				break;
			case ITEM_ARMOR:
			default:
				af.modifier	= 1 * UMIN(10,level / 10); 
				af.location     = APPLY_AC;
				break;
		}
                affect_to_obj(obj,&af);    

		act("$p glows with a malevolent aura.",ch,obj,NULL,TO_ALL);
		return;
	}

	/* character curses */
	victim = (CHAR_DATA *) vo;

	if (IS_AFFECTED(victim,AFF_CURSE)) {
		act("$N is already suffering a curse.",ch,NULL,victim,TO_CHAR);
		return;
	}
	if (saves_spell(level,victim,DAM_NEGATIVE)) {
		act("$N resists your curse.",ch,NULL,victim,TO_CHAR);
		act("You resist $n's curse.",ch,NULL,victim,TO_VICT);
		act("$n's curse isn't strong enough to take hold of $N.",
		   ch,NULL,victim,TO_NOTVICT);
		return;
	}
	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = (8 + level / 10);
	af.location  = APPLY_HITROLL;
	af.modifier  = -1 * (level / 8);
	af.bitvector = AFF_CURSE;
	affect_to_char(victim, &af);

	af.location  = APPLY_SAVING_SPELL;
	af.modifier  = level / 8;
	affect_to_char(victim, &af);

	char_puts("You feel unclean.\n", victim);
	act("$n looks very uncomfortable.",victim,NULL,NULL, TO_ROOM);
}


void spell_anathema(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	AFFECT_DATA af;
	CHAR_DATA* victim = (CHAR_DATA*) vo;
	int strength = 0;

	if (IS_GOOD(victim))
		strength = IS_EVIL(ch) ? 2 : (IS_GOOD(ch) ? 0 : 1);
	else if (IS_EVIL(victim))
		strength = IS_GOOD(ch) ? 2 : (IS_EVIL(ch) ? 0 : 1);
	else 
		strength = (IS_GOOD(ch) || IS_EVIL(ch)) ? 1:0;

	if (!strength) {
		act_puts("Oh, no. Your god seems to like $N.",
			 ch, NULL, victim, TO_CHAR, POS_DEAD);
		return;
	}

	if (is_affected(victim, sn)) {
		act("$N's is already cursed.",ch,NULL,victim,TO_CHAR);
		return;
	}

	level += (strength - 1) * 3;

	if (saves_spell(level, victim, DAM_HOLY)) {
		char_puts("Your nasty curse is resisted.\n", ch);
		act("You resist $n's nasty curse.",ch,NULL,victim,TO_VICT);
		act("$n's nasty curse isn't strong enough to take hold of $N.",
		   ch,NULL,victim,TO_NOTVICT);
		return;
	}

	af.where 	= TO_AFFECTS;
	af.type  	= sn;
	af.level 	= level;
	af.duration	= (8 + level/10);
	af.location	= APPLY_HITROLL;
	af.modifier	= - level/5 * strength;
	af.bitvector	= AFF_CURSE;

	affect_to_char(victim, &af);
	
	af.location	= APPLY_SAVING_SPELL;
	af.modifier	= level/5 * strength;

	affect_to_char(victim, &af);

	af.location	= APPLY_LEVEL;
	af.modifier	= -strength;

	affect_to_char(victim, &af);
	
	act("$n looks terribly uncomfortable.", victim, NULL, NULL, TO_ROOM);
	char_puts("You feel bathed in squalor.\n", victim);
}

/* RT replacement demonfire spell */

void spell_demonfire(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	if (!IS_NPC(ch) && !IS_EVIL(ch))
	{
		victim = ch;
		char_puts("The demons turn upon you!\n",ch);
	}

	if (victim != ch)
	{
		act("$n calls forth the demons of Hell upon $N!",
		    ch,NULL,victim,TO_ROOM);
		act("$n has assailed you with the demons of Hell!",
		    ch,NULL,victim,TO_VICT);
		char_puts("You conjure forth the demons of hell!\n",ch);
	}
	dam = dice(level, 12);
	if (saves_spell(level, victim,DAM_NEGATIVE))
		dam /= 2;
	spell_curse(gsn_curse, 3 * level / 4, ch, (void *) victim, TARGET_CHAR);
	damage(ch, victim, dam, sn, DAM_NEGATIVE ,TRUE);
}

/* added by chronos */
void spell_bluefire(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	if (!IS_NEUTRAL(ch))
	{
		victim = ch;
		char_puts("Your blue fire turn upon you!\n",ch);
	}

	if (victim != ch)
	{
		act("$n calls forth the blue fire of earth $N!",
		    ch,NULL,victim,TO_ROOM);
		act("$n has assailed you with the neutrals of earth!",
		    ch,NULL,victim,TO_VICT);
		char_puts("You conjure forth the blue fire!\n",ch);
	}

	dam = dice(level, 12);
	if (saves_spell(level, victim,DAM_FIRE))
		dam /= 2;
	damage(ch, victim, dam, sn, DAM_FIRE ,TRUE);
}


void spell_detect_evil(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (IS_AFFECTED(victim, AFF_DETECT_EVIL))
	{
		if (victim == ch)
		  char_puts("You can already sense evil.\n",ch);
		else
		  act("$N can already detect evil.",ch,NULL,victim,TO_CHAR);
		return;
	}
	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level	 = level;
	af.duration  = (5 + level / 3);
	af.modifier  = 0;
	af.location  = APPLY_NONE;
	af.bitvector = AFF_DETECT_EVIL;
	affect_to_char(victim, &af);
	char_puts("Your vision takes on a {rredish{x hue.\n", victim);
	act("$n's eyes flash {rred{x for a moment.",victim,NULL,NULL,TO_ROOM);
	return;
}


void spell_detect_good(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (IS_AFFECTED(victim, AFF_DETECT_GOOD))
	{
		if (victim == ch)
		  char_puts("You can already sense good.\n",ch);
		else
		  act("$N can already detect good.",ch,NULL,victim,TO_CHAR);
		return;
	}
	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = (5 + level / 3);
	af.modifier  = 0;
	af.location  = APPLY_NONE;
	af.bitvector = AFF_DETECT_GOOD;
	affect_to_char(victim, &af);
	char_puts("Your vision takes on a {Yg{yo{Yld{ye{yn{x hue.\n", victim);
	act("$n's eyes flash {Ygold{x for a moment.",victim,NULL,NULL,TO_ROOM);
}

void spell_detect_hidden(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (IS_AFFECTED(victim, AFF_DETECT_HIDDEN)) {
		if (victim == ch)
			char_puts("You are already as alert as you can be.\n",
				  ch);
		else
			act("$N can already sense hidden lifeforms.",
			    ch, NULL, victim, TO_CHAR);
		return;
	}

	af.where	= TO_AFFECTS;
	af.type		= sn;
	af.level	= level;
	af.duration	= 5 + level / 3;
	af.location	= APPLY_NONE;
	af.modifier	= 0;
	af.bitvector	= AFF_DETECT_HIDDEN;
	affect_to_char(victim, &af);
	char_puts("Your awareness improves.\n", victim);
	act("$n looks more aware.",victim,NULL,NULL,TO_ROOM);
}

void spell_detect_fade(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (IS_AFFECTED(victim, AFF_DETECT_FADE)) {
		if (victim == ch)
			char_puts("You are already as alert as you can be.\n",
				  ch);
		else
			act("$N can already sense faded lifeforms.",
			    ch, NULL, victim, TO_CHAR);
		return;
	}

	af.where	= TO_AFFECTS;
	af.type		= sn;
	af.level	= level;
	af.duration	= 5 + level / 3;
	af.location	= APPLY_NONE;
	af.modifier	= 0;
	af.bitvector	= AFF_DETECT_FADE;
	affect_to_char(victim, &af);
	char_puts("You vision takes on a {ggreenish{x hue.\n", victim);
	act("$n's eyes flash {ggreen{x for a moment.",
	ch, NULL, NULL, TO_ROOM);
}

void spell_detect_invis(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	AFFECT_DATA *paf;

	if (IS_AFFECTED(victim, AFF_DETECT_INVIS))
	{
		char_puts("The {bbluish{x hue of your vision intensifies.\n", victim);
		act("$n's eyes flash a deeper {bblue{x for a moment.", victim, NULL, NULL, TO_ROOM);
		paf = affect_find(ch->affected, sn);
		if (paf == NULL) {
			char_puts("Something is wrong with your vision.\n", victim);
			return;
		}
		else {
			paf->level = level+2;
			paf->duration  = (5 + level / 15);
			return;
		}
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = (5 + level / 15);
	af.modifier  = 0;
	af.location  = APPLY_NONE;
	af.bitvector = AFF_DETECT_INVIS;
	affect_to_char(victim, &af);
	char_puts("Your vision takes on a {bbluish{x hue.\n", victim);
	act("$n's eyes flash {bblue{x for a moment.", victim, NULL, NULL, TO_ROOM);
}



void spell_detect_magic(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (IS_AFFECTED(victim, AFF_DETECT_MAGIC))
	{
		if (victim == ch)
		  char_puts("You can already sense magical auras.\n",ch);
		else
		  act("$N can already detect magic.",ch,NULL,victim,TO_CHAR);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level	 = level;
	af.duration  = (5 + level / 3);
	af.modifier  = 0;
	af.location  = APPLY_NONE;
	af.bitvector = AFF_DETECT_MAGIC;
	affect_to_char(victim, &af);
	char_puts("Your vision takes on a {Mpinkish{x hue.\n", victim);
	act("$n's eyes flash {Mpink{x for a moment.",
		    victim, NULL, NULL, TO_ROOM);
}



void spell_detect_poison(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;

	if (obj->pIndexData->item_type == ITEM_DRINK_CON 
	|| obj->pIndexData->item_type == ITEM_FOOD) {
		if (obj->value[ITEM_FOOD_POISON] != 0)
		    char_puts("You smell poisonous fumes.\n", ch);
		else
		    char_puts("It looks delicious.\n", ch);
	}
	else {
		char_puts("It doesn't look poisoned.\n", ch);
	}

	return;
}



void spell_dispel_evil(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	if (!IS_NPC(ch) && IS_EVIL(ch))
		victim = ch;

	if (IS_GOOD(victim))
	{
		act("Gods protects $N.", ch, NULL, victim, TO_ROOM);
		return;
	}

	if (IS_NEUTRAL(victim))
	{
		act("$N does not seem to be affected.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (victim->hit > (ch->level * 4))
	  dam = dice(level, 5);
	else
	  dam = UMAX(victim->hit, dice(level,5));
	if (saves_spell(level, victim,DAM_HOLY))
		dam /= 2;
	damage(ch, victim, dam, sn, DAM_HOLY ,TRUE);
}

void spell_dispel_good(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	if (!IS_NPC(ch) && IS_GOOD(ch))
		victim = ch;

	if (IS_EVIL(victim))
	{
		act("$N is protected by $S evil.", ch, NULL, victim, TO_ROOM);
		return;
	}

	if (IS_NEUTRAL(victim))
	{
		act("$N does not seem to be affected.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (victim->hit > (ch->level * 5))
	  dam = dice(level, 5);
	else
	  dam = UMAX(victim->hit, dice(level,5));
	if (saves_spell(level, victim,DAM_NEGATIVE))
		dam /= 2;
	damage(ch, victim, dam, sn, DAM_NEGATIVE ,TRUE);
}

/* modified for enhanced use */

void spell_dispel_magic(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	bool found = FALSE;

	if (saves_spell(level, victim,DAM_OTHER))
	{
		char_puts("You feel a brief tingling sensation.\n",victim);
		act("$N resists your dispel attempt.", 
		ch, NULL, victim, TO_CHAR);
		act("$n's mystical efforts seem too weak to affect $N.", 
		ch, NULL, victim, TO_ROOM);
		return;
	}

	/* begin running through the spells */
	found = dispel_affects(level, victim);

	if (!found)
	    char_puts("Your dispel attempt had no effect.\n",ch);
}

void spell_earthquake(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	char_puts("The earth trembles beneath your feet!\n", ch);
	act("$n makes the earth tremble and shiver.", ch, NULL, NULL, TO_ROOM);

	for (vch = char_list; vch; vch = vch_next) {
		vch_next	= vch->next;

		if (vch->in_room == NULL)
			continue;

		if (vch->in_room == ch->in_room) {
			if (is_safe_spell(ch, vch, TRUE))
				continue;

			if (IS_AFFECTED(vch,AFF_FLYING))
				damage(ch, vch, 0, sn, DAM_BASH, TRUE);
			else
				damage(ch, vch, level + dice(2, 8), sn,
				       DAM_BASH,TRUE);
			continue;
		}

		if (vch->in_room->area == ch->in_room->area)
			char_puts("The earth trembles and shivers.\n", vch);
	}
}

void spell_enchant_armor(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	AFFECT_DATA *paf;
	int result, fail;
	int ac_bonus, added;
	bool ac_found = FALSE;

	if (obj->pIndexData->item_type != ITEM_ARMOR)
	{
		char_puts("That isn't armor.\n",ch);
		return;
	}

	if (obj->wear_loc != -1)
	{
		char_puts("The item must be carried to be enchanted.\n",ch);
		return;
	}

	if (is_quest_item(obj->pIndexData)) {
		char_puts("That item is beyond your ability to enchant.\n",ch);
		return;
	}

	/* this means they have no bonus */
	ac_bonus = 0;
	fail = 25;	/* base 25% chance of failure */

	/* find the bonuses */

	if (!IS_SET(obj->extra_flags, ITEM_ENCHANTED))
		for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
		{
		    if (paf->location == APPLY_AC)
		    {
		    	ac_bonus = paf->modifier;
			ac_found = TRUE;
		    	fail += 5 * (ac_bonus * ac_bonus);
 	    }

		    else  /* things get a little harder */
		    	fail += 20;
		}
 
	for (paf = obj->affected; paf != NULL; paf = paf->next)
	{
		if (paf->location == APPLY_AC)
  	{
		    ac_bonus = paf->modifier;
		    ac_found = TRUE;
		    fail += 5 * (ac_bonus * ac_bonus);
		}

		else /* things get a little harder */
		    fail += 20;
	}

	/* apply other modifiers */
	fail -= level;

	if (IS_OBJ_STAT(obj,ITEM_BLESS))
		fail -= 15;
	if (IS_OBJ_STAT(obj,ITEM_GLOW))
		fail -= 5;

	fail = URANGE(5,fail,85);

	result = number_percent();

	/* the moment of truth */
	if (result < (fail / 5))  /* item destroyed */
	{
		act("$p flares blindingly... and evaporates!",ch,obj,NULL,TO_CHAR);
		act("$p flares blindingly... and evaporates!",ch,obj,NULL,TO_ROOM);
		extract_obj(obj, 0);
		return;
	}

	if (result < (fail / 3)) /* item disenchanted */
	{
		AFFECT_DATA *paf_next;

		act("$p glows brightly, then fades...oops.",ch,obj,NULL,TO_CHAR);
		act("$p glows brightly, then fades.",ch,obj,NULL,TO_ROOM);

		/* remove all affects */
		for (paf = obj->affected; paf != NULL; paf = paf_next)
		{
		    paf_next = paf->next;
		    aff_free(paf);
		}
		obj->affected = NULL;

		SET_BIT(obj->extra_flags, ITEM_ENCHANTED);
		return;
	}

	if (result <= fail)  /* failed, no bad result */
	{
		char_puts("Nothing seemed to happen.\n",ch);
		return;
	}

	/* okay, move all the old flags into new vectors if we have to */
	if (!IS_SET(obj->extra_flags, ITEM_ENCHANTED)) {
		AFFECT_DATA *af_new;
		SET_BIT(obj->extra_flags, ITEM_ENCHANTED);

		for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next) 
		{
		    af_new = aff_new();
		
		    af_new->next = obj->affected;
		    obj->affected = af_new;

		    af_new->where	= paf->where;
		    af_new->type 	= UMAX(0,paf->type);
		    af_new->level	= paf->level;
		    af_new->duration	= paf->duration;
		    af_new->location	= paf->location;
		    af_new->modifier	= paf->modifier;
		    af_new->bitvector	= paf->bitvector;
		}
	}

	if (result <= (90 - level/5))  /* success! */
	{
		act("$p shimmers with a gold aura.",ch,obj,NULL,TO_CHAR);
		act("$p shimmers with a gold aura.",ch,obj,NULL,TO_ROOM);
		SET_BIT(obj->extra_flags, ITEM_MAGIC);
		added = -1;
	}
	
	else  /* exceptional enchant */
	{
		act("$p glows a brillant gold!",ch,obj,NULL,TO_CHAR);
		act("$p glows a brillant gold!",ch,obj,NULL,TO_ROOM);
		SET_BIT(obj->extra_flags,ITEM_MAGIC);
		SET_BIT(obj->extra_flags,ITEM_GLOW);
		added = -2;
	}
			
	/* now add the enchantments */

	if (obj->level < LEVEL_HERO)
		obj->level = UMIN(LEVEL_HERO - 1,obj->level + 1);

	if (ac_found)
	{
		for (paf = obj->affected; paf != NULL; paf = paf->next)
		{
		    if (paf->location == APPLY_AC)
		    {
			paf->type = sn;
			paf->modifier += added;
			paf->level = UMAX(paf->level,level);
		    }
		}
	}
	else /* add a new affect */
	{
 	paf = aff_new();

		paf->where	= TO_OBJECT;
		paf->type	= sn;
		paf->level	= level;
		paf->duration	= -1;
		paf->location	= APPLY_AC;
		paf->modifier	=  added;
		paf->bitvector  = 0;
		paf->next	= obj->affected;
		obj->affected	= paf;
	}

}

void spell_enchant_weapon(int sn,int level,CHAR_DATA *ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	AFFECT_DATA *paf;
	int result, fail;
	int hit_bonus, dam_bonus, added;
	bool hit_found = FALSE, dam_found = FALSE;

	if (obj->pIndexData->item_type != ITEM_WEAPON)
	{
		char_puts("That isn't a weapon.\n",ch);
		return;
	}

	if (obj->wear_loc != -1)
	{
		char_puts("The item must be carried to be enchanted.\n",ch);
		return;
	}

	if (is_quest_item(obj->pIndexData)) {
		char_puts("That item is beyond your ability to enchant.\n",ch);
		return;
	}

	/* this means they have no bonus */
	hit_bonus = 0;
	dam_bonus = 0;
	fail = 25;	/* base 25% chance of failure */

	/* find the bonuses */

	if (!IS_SET(obj->extra_flags, ITEM_ENCHANTED))
		for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
		{
	        if (paf->location == APPLY_HITROLL)
	        {
		    	hit_bonus = paf->modifier;
			hit_found = TRUE;
		    	fail += 2 * (hit_bonus * hit_bonus);
 	    }

		    else if (paf->location == APPLY_DAMROLL)
		    {
		    	dam_bonus = paf->modifier;
			dam_found = TRUE;
		    	fail += 2 * (dam_bonus * dam_bonus);
		    }

		    else  /* things get a little harder */
		    	fail += 25;
		}
 
	for (paf = obj->affected; paf != NULL; paf = paf->next)
	{
		if (paf->location == APPLY_HITROLL)
  	{
		    hit_bonus = paf->modifier;
		    hit_found = TRUE;
		    fail += 2 * (hit_bonus * hit_bonus);
		}

		else if (paf->location == APPLY_DAMROLL)
  	{
		    dam_bonus = paf->modifier;
		    dam_found = TRUE;
		    fail += 2 * (dam_bonus * dam_bonus);
		}

		else /* things get a little harder */
		    fail += 25;
	}

	/* apply other modifiers */
	fail -= 3 * level/2;

	if (IS_OBJ_STAT(obj,ITEM_BLESS))
		fail -= 15;
	if (IS_OBJ_STAT(obj,ITEM_GLOW))
		fail -= 5;

	fail = URANGE(5,fail,95);

	result = number_percent();

	/* the moment of truth */
	if (result < (fail / 5))  /* item destroyed */
	{
		act("$p shivers violently and explodes!",ch,obj,NULL,TO_CHAR);
		act("$p shivers violently and explodeds!",ch,obj,NULL,TO_ROOM);
		extract_obj(obj, 0);
		return;
	}

	if (result < (fail / 2)) /* item disenchanted */
	{
		AFFECT_DATA *paf_next;

		act("$p glows brightly, then fades...oops.",ch,obj,NULL,TO_CHAR);
		act("$p glows brightly, then fades.",ch,obj,NULL,TO_ROOM);

		/* remove all affects */
		for (paf = obj->affected; paf != NULL; paf = paf_next)
		{
		    paf_next = paf->next; 
		    aff_free(paf);
		}
		obj->affected = NULL;

		SET_BIT(obj->extra_flags, ITEM_ENCHANTED);
		return;
	}

	if (result <= fail)  /* failed, no bad result */
	{
		char_puts("Nothing seemed to happen.\n",ch);
		return;
	}

	/* okay, move all the old flags into new vectors if we have to */
	if (!IS_SET(obj->extra_flags, ITEM_ENCHANTED)) {
		AFFECT_DATA *af_new;
		SET_BIT(obj->extra_flags, ITEM_ENCHANTED);

		for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next) 
		{
		    af_new = aff_new();
		
		    af_new->next = obj->affected;
		    obj->affected = af_new;

		    af_new->where	= paf->where;
		    af_new->type 	= UMAX(0,paf->type);
		    af_new->level	= paf->level;
		    af_new->duration	= paf->duration;
		    af_new->location	= paf->location;
		    af_new->modifier	= paf->modifier;
		    af_new->bitvector	= paf->bitvector;
		}
	}

	if (result <= (100 - level/5))  /* success! */
	{
		act("$p glows blue.",ch,obj,NULL,TO_CHAR);
		act("$p glows blue.",ch,obj,NULL,TO_ROOM);
		if (HAS_SKILL(ch, gsn_spellbane))
			SET_BIT(obj->extra_flags,ITEM_HUM);
		else
			SET_BIT(obj->extra_flags, ITEM_MAGIC);
		added = 1;
	}
	
	else  /* exceptional enchant */
	{
		act("$p glows a brillant blue!",ch,obj,NULL,TO_CHAR);
		act("$p glows a brillant blue!",ch,obj,NULL,TO_ROOM);
		if (HAS_SKILL(ch, gsn_spellbane))
			SET_BIT(obj->extra_flags,ITEM_HUM);
		else
			SET_BIT(obj->extra_flags,ITEM_MAGIC);
		SET_BIT(obj->extra_flags,ITEM_GLOW);
		added = 2;
	}
			
	/* now add the enchantments */ 

	if (obj->level < LEVEL_HERO - 1)
		obj->level = UMIN(LEVEL_HERO - 1,obj->level + 1);

	if (dam_found)
	{
		for (paf = obj->affected; paf != NULL; paf = paf->next)
		{
		    if (paf->location == APPLY_DAMROLL)
		    {
			paf->type = sn;
			paf->modifier += added;
			paf->level = UMAX(paf->level,level);
			if (paf->modifier > 4)
			    SET_BIT(obj->extra_flags,ITEM_HUM);
		    }
		}
	}
	else /* add a new affect */
	{
		paf = aff_new();

		paf->where	= TO_OBJECT;
		paf->type	= sn;
		paf->level	= level;
		paf->duration	= -1;
		paf->location	= APPLY_DAMROLL;
		paf->modifier	=  added;
		paf->bitvector  = 0;
		paf->next	= obj->affected;
		obj->affected	= paf;
	}

	if (hit_found)
	{
	    for (paf = obj->affected; paf != NULL; paf = paf->next)
		{
	        if (paf->location == APPLY_HITROLL)
	        {
			paf->type = sn;
	            paf->modifier += added;
	            paf->level = UMAX(paf->level,level);
	            if (paf->modifier > 4)
	                SET_BIT(obj->extra_flags,ITEM_HUM);
	        }
		}
	}
	else /* add a new affect */
	{
	    paf = aff_new();
 
	    paf->type       = sn;
	    paf->level      = level;
  	    paf->duration   = -1;
	    paf->location   = APPLY_HITROLL;
	    paf->modifier   =  added;
	    paf->bitvector  = 0;
	    paf->next       = obj->affected;
	    obj->affected   = paf;
	}

}



/*
 * Drain XP, MANA, HP.
 * Caster gains HP.
 */
void spell_energy_drain(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	int dam;

	if (ch == victim) {
		char_puts("Draining your own energy causes an feedback loop!\n",victim);
		damage(ch, victim, ch->level *2, sn, DAM_OTHER, TRUE);
		return;
	}

	if (saves_spell(level, victim,DAM_NEGATIVE))
	{
		char_puts("You feel a momentary chill.\n",victim);
		act("$N resists your drain.",ch, NULL, victim, TO_CHAR);
		act("$n shakes off a momentary chill.",victim, NULL, NULL, TO_ROOM);
		return;
	}


	if (victim->level <= 2)
	{
		dam		 = ch->hit + 1;
	}
	else
	{
		gain_exp(victim, 0 - number_range(level/5, 3 * level / 5));
		victim->mana	/= 2;
		victim->move	/= 2;
		dam		 = dice(1, level);
		ch->hit		+= dam;
	}

	if (number_percent() < 15) {
		af.where 	= TO_AFFECTS;
		af.type		= sn;
		af.level	= level/2;
		af.duration	= (6 + level/12);
		af.location	= APPLY_LEVEL;
		af.modifier	= -1;
		af.bitvector	= 0;

		affect_join(victim, &af);

	}


	char_puts("You feel your life slipping away!\n",victim);
	char_puts("Wow....what a rush!\n",ch);
	damage(ch, victim, dam, sn, DAM_NEGATIVE, TRUE);
}

void spell_hellfire(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	damage(ch, victim, dice(level, 7), sn, DAM_FIRE, TRUE);
}

void spell_iceball(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int dam;
	int movedam;

	dam = dice(level , 12);
	movedam     = number_range(ch->level, 2 * ch->level);

	for (vch = ch->in_room->people; vch; vch = vch_next) {
		vch_next = vch->next_in_room;

		if (is_safe_spell(ch, vch, TRUE))
			continue;

		if (saves_spell(level, vch, DAM_COLD))
			dam /= 2;
		vch->move -= UMIN(vch->move, movedam);
		damage(ch, vch, dam, sn, DAM_COLD, TRUE);
	}
}

void spell_fireball(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level, 12);
	if (saves_spell(level, victim, DAM_FIRE))
		dam /= 2;
	damage(ch, victim, dam, sn, DAM_FIRE ,TRUE);
	fire_effect(victim, level, dam/2, TARGET_CHAR);
}

void spell_fireproof(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	AFFECT_DATA af;

	if (IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
	{
		act("$p is already protected from burning.",ch,obj,NULL,TO_CHAR);
		return;
	}

	af.where     = TO_OBJECT;
	af.type      = sn;
	af.level     = level;
	af.duration  = number_fuzzy(URANGE(72, level * 6, 720));
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = ITEM_BURN_PROOF;

	affect_to_obj(obj,&af);

	act("You protect $p from fire.",ch,obj,NULL,TO_CHAR);
	act("$p is surrounded by a protective aura.",ch,obj,NULL,TO_ROOM);
}

void spell_flamestrike(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level, 10);
	if (saves_spell(level, victim,DAM_FIRE))
		dam /= 2;
	damage(ch, victim, dam, sn, DAM_FIRE ,TRUE);
}

void spell_faerie_fire(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (IS_AFFECTED(victim, AFF_FAERIE_FIRE)) {
		act("You couldn't make $M glow any brighter.",
			ch, NULL, victim, TO_CHAR);
		return;
	}

	if (saves_spell(level+5, victim, DAM_OTHER)) {
		act("The air around $N doesn't ignite.",
			ch, NULL, victim, TO_CHAR);
		act("Wicks of {Mpink{x flame flair up and die on your shoulder.",
			ch, NULL, victim, TO_VICT);
		act("Wicks of {Mpink{x flame flair up and die on $N's shoulder.",
			ch, NULL, victim, TO_NOTVICT);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level	 = level;
	af.duration  = 10 + level / 5;
	af.location  = APPLY_AC;
	af.modifier  = 2 * level;
	af.bitvector = AFF_FAERIE_FIRE;
	affect_to_char(victim, &af);

	affect_bit_strip(victim, TO_AFFECTS, AFF_INVIS | AFF_IMP_INVIS);
	remove_hide_affect(victim, AFF_HIDE | AFF_FADE | AFF_CAMOUFLAGE);
	REMOVE_BIT(victim->affected_by, AFF_HIDE | AFF_FADE |
				     AFF_CAMOUFLAGE | AFF_INVIS |
				     AFF_IMP_INVIS);

	char_puts("You are surrounded by a {Mpink{x flames.\n", victim);
	act("$n is surrounded by a {Mpink{x flames.", victim, NULL, NULL, TO_ROOM);
}

void spell_faerie_fog(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *ich;

	act("$n conjures a cloud of {mpurple{x smoke.", ch, NULL, NULL, TO_ROOM);
	char_puts("You conjure a cloud of {mpurple{x smoke.\n", ch);

	for (ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room) {
		if (ich->invis_level > 0)
			continue;

		if (ich == ch || saves_spell(level, ich, DAM_OTHER))
			continue;

		affect_bit_strip(ich, TO_AFFECTS, AFF_INVIS | AFF_IMP_INVIS);
		remove_hide_affect(ich, AFF_HIDE | AFF_FADE | AFF_CAMOUFLAGE);
		REMOVE_BIT(ich->affected_by, AFF_HIDE | AFF_FADE |
					     AFF_CAMOUFLAGE | AFF_INVIS |
					     AFF_IMP_INVIS);

		act("$n is revealed!", ich, NULL, NULL, TO_ROOM);
		char_puts("You are revealed!\n", ich);
	}
}

void spell_floating_disc(int sn, int level,CHAR_DATA *ch,void *vo, int target)
{
	OBJ_DATA *disc, *floating;

	floating = get_eq_char(ch,WEAR_FLOAT);
	if (floating != NULL && IS_OBJ_STAT(floating,ITEM_NOREMOVE))
	{
		act("You can't remove $p.",ch,floating,NULL,TO_CHAR);
		return;
	}

	disc = create_obj(get_obj_index(OBJ_VNUM_DISC), 0);
	disc->value[ITEM_CONTAINER_WEIGHT] = ch->level * 10; /* 10 pounds per level capacity */
	disc->value[ITEM_CONTAINER_PER_ITEM_WEIGHT] = ch->level * 5; /* 5 pounds per level max per item */
	disc->timer		= ch->level * 2 - number_range(0,level / 2); 

	act("$n has created a floating black disc.",ch,NULL,NULL,TO_ROOM);
	char_puts("You create a floating disc.\n",ch);
	obj_to_char(disc,ch);
	wear_obj(ch,disc,TRUE);
	return;
}

void spell_fly(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (IS_AFFECTED(victim, AFF_FLYING))
	{
		if (victim == ch)
		  char_puts("You are already airborne.\n",ch);
		else
		  act("$N doesn't need your help to fly.",ch,NULL,victim,TO_CHAR);
		return;
	}
	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level	 = level;
	af.duration  = level + 3;
	af.location  = 0;
	af.modifier  = 0;
	af.bitvector = AFF_FLYING;
	affect_to_char(victim, &af);
	char_puts("Your feet rise off the ground.\n", victim);
	act("$n's feet rise off the ground.", victim, NULL, NULL, TO_ROOM);
}

/* RT clerical berserking spell */
void spell_frenzy(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (is_affected(victim,sn) || is_affected(victim, gsn_berserk))
	{
		if (victim == ch)
		  char_puts("You are already in a frenzy.\n",ch);
		else
		  act("$N is already in a frenzy.",ch,NULL,victim,TO_CHAR);
		return;
	}

	if (is_affected(victim,sn_lookup("calm")))
	{
		if (victim == ch)
		  char_puts("Why don't you just relax for a while?\n",ch);
		else
		  act("$N doesn't look like $e wants to fight anymore.",
		      ch,NULL,victim,TO_CHAR);
		return;
	}

	if ((IS_GOOD(ch) && !IS_GOOD(victim)) ||
		(IS_NEUTRAL(ch) && !IS_NEUTRAL(victim)) ||
		(IS_EVIL(ch) && !IS_EVIL(victim))
	  )
	{
		act("Your god doesn't seem to like $N",ch,NULL,victim,TO_CHAR);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type 	 = sn;
	af.level	 = level;
	af.duration	 = level / 3;
	af.modifier  = level / 6;
	af.bitvector = 0;

	af.location  = APPLY_HITROLL;
	affect_to_char(victim,&af);

	af.location  = APPLY_DAMROLL;
	affect_to_char(victim,&af);

	af.modifier  = 10 * (level / 12);
	af.location  = APPLY_AC;
	affect_to_char(victim,&af);

	char_puts("You are filled with holy wrath!\n",victim);
	act("$n gets a wild look in $s eyes!",victim,NULL,NULL,TO_ROOM);
}

static inline void
gate(CHAR_DATA *ch, CHAR_DATA *victim)
{
	transfer_char(ch, NULL, victim->in_room,
		      "$N steps through a {Gg{gat{Ge{x and {Dvanishes{x.",
		      "You step through a {Gg{gat{Ge{x and {Dvanish{x.\n",
		      "$N has arrived through a {Gg{gat{Ge{x.");
}

void spell_gate(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim;
	CHAR_DATA *gch = NULL;
	ROOM_INDEX_DATA *origin;
	AFFECT_DATA af;

	if (!IS_IMMORTAL(ch) && is_affected(ch, sn)) {
		char_puts("You must wait for your soul to catch up with you from last time.\n", ch);
		return;
	}

	if ((victim = get_char_world(ch, target_name)) == NULL
	||  victim->level >= level + 3
	||  saves_spell(level, victim, DAM_OTHER)
	||  !can_gate(ch, victim)) {
		char_puts("A {Ws{wh{Wi{wmm{We{wr{Wi{wn{Wg {Gg{gat{Ge{x opens, but quickly collapses.\n", ch);
		return;
	}

	af.where		= TO_AFFECTS;
	af.type               = sn;
	af.level              = level; 
	af.duration           = number_fuzzy(2);
	af.bitvector          = 0;
	af.modifier           = 0;
	af.location           = APPLY_NONE;
	affect_to_char(ch, &af);  

	origin = ch->in_room;
	gate(ch, victim);
        for (gch = npc_list; gch; gch = gch->next) {
                if (gch->in_room == origin
                &&  gch->master == ch
		&& IS_AFFECTED(gch, AFF_CHARM)
                && !IS_AFFECTED(gch, AFF_SLEEP)
                && gch->position >= POS_STANDING) {
			gate(gch, victim);
                }
        }
}

void spell_giant_strength(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (is_affected(victim, sn))
	{
		if (victim == ch)
		  char_puts("You are already as strong as you can get!\n",ch);
		else
		  act("$N can't get any stronger.",ch,NULL,victim,TO_CHAR);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level	 = level;
	af.duration  = (10 + level / 3);
	af.location  = APPLY_STR;
	af.modifier  = UMAX(2,level / 10);
	af.bitvector = 0;
	affect_to_char(victim, &af);
	char_puts("Your muscles surge with heightened power!\n", victim);
	act("$n's muscles surge with heightened power.",victim,NULL,NULL,TO_ROOM);
	return;
}



void spell_harm(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = UMAX( 20, victim->hit - dice(1,4));
	if (saves_spell(level, victim,DAM_HARM))
		dam = UMIN(50, dam / 2);
	dam = UMIN(100, dam);
	damage(ch, victim, dam, sn, DAM_HARM ,TRUE);
}

/* RT haste spell */

void spell_haste(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
 
	if (is_affected(victim, sn)
	||  IS_AFFECTED(victim,AFF_HASTE)
	||  (IS_NPC(victim) && IS_SET(victim->pIndexData->off_flags, OFF_FAST))) {
		if (victim == ch)
			char_puts("You can't move any faster!\n",ch);
		else
			act("$N is already moving as fast as $E can.",
			    ch, NULL, victim, TO_CHAR);
		return;
	}

	if (IS_AFFECTED(victim,AFF_SLOW))
	{
		if (!check_dispel(level,victim,sn_lookup("slow")))
		{
		    if (victim != ch)
			char_puts("Spell failed.\n",ch);
		    char_puts("You feel momentarily faster.\n",victim);
		    return;
		}
		act("$n is moving less slowly.",victim,NULL,NULL,TO_ROOM);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	if (victim == ch)
	  af.duration  = level/2;
	else
	  af.duration  = level/4;
	af.location  = APPLY_DEX;
	af.modifier  = UMAX(2,level / 12);
	af.bitvector = AFF_HASTE;
	affect_to_char(victim, &af);
	char_puts("You feel yourself moving more quickly.\n", victim);
	act("$n is moving more quickly.",victim,NULL,NULL,TO_ROOM);
}




void spell_heat_metal(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	OBJ_DATA *obj_lose, *obj_next;
	int dam = 0;
	bool fail = TRUE;

   if (!saves_spell(level + 2,victim,DAM_FIRE)
   &&  !IS_SET(victim->imm_flags,IMM_FIRE))
   {
		for (obj_lose = victim->carrying;
		      obj_lose != NULL;
		      obj_lose = obj_next)
		{
		    obj_next = obj_lose->next_content;
		    if (number_range(1,2 * level) > obj_lose->level
		    &&   !saves_spell(level,victim,DAM_FIRE)
		    &&   is_metal(obj_lose)
		    &&   !IS_OBJ_STAT(obj_lose,ITEM_BURN_PROOF))
		    {
			switch (obj_lose->pIndexData->item_type)
			{
			case ITEM_ARMOR:
			if (obj_lose->wear_loc != -1) /* remove the item */
			{
			    if (can_drop_obj(victim,obj_lose)
			    &&  (obj_lose->weight / 10) <
				number_range(1,2 * get_curr_stat(victim,STAT_DEX))
			    &&  remove_obj(victim, obj_lose->wear_loc, TRUE))
			    {
				act("$n yelps and throws $p to the ground!",
				    victim,obj_lose,NULL,TO_ROOM);
				act("You remove and drop $p before it burns you.",
				    victim,obj_lose,NULL,TO_CHAR);
				dam += (number_range(1,obj_lose->level) / 3);
				obj_from_char(obj_lose);
				obj_to_room(obj_lose, victim->in_room);
				fail = FALSE;
			    }
			    else /* stuck on the body! ouch! */
			    {
				act("Your skin is seared by $p!",
				    victim,obj_lose,NULL,TO_CHAR);
				dam += (number_range(1,obj_lose->level));
				fail = FALSE;
			    }

			}
			else /* drop it if we can */
			{
			    if (can_drop_obj(victim,obj_lose))
			    {
				act("$n yelps and throws $p to the ground!",
				    victim,obj_lose,NULL,TO_ROOM);
				act("You and drop $p before it burns you.",
				    victim,obj_lose,NULL,TO_CHAR);
				dam += (number_range(1,obj_lose->level) / 6);
				obj_from_char(obj_lose);
				obj_to_room(obj_lose, victim->in_room);
				fail = FALSE;
			    }
			    else /* cannot drop */
			    {
				act("Your skin is seared by $p!",
				    victim,obj_lose,NULL,TO_CHAR);
				dam += (number_range(1,obj_lose->level) / 2);
				fail = FALSE;
			    }
			}
			break;
			case ITEM_WEAPON:
			if (obj_lose->wear_loc != -1) /* try to drop it */
			{
			    if (IS_WEAPON_STAT(obj_lose,WEAPON_FLAMING))
				continue;

			    if (can_drop_obj(victim,obj_lose)
			    &&  remove_obj(victim,obj_lose->wear_loc,TRUE))
			    {
				act("$n is burned by $p, and throws it to the ground.",
				    victim,obj_lose,NULL,TO_ROOM);
				char_puts(
				    "You throw your red-hot weapon to the ground!\n",
				    victim);
				dam += 1;
				obj_from_char(obj_lose);
				obj_to_room(obj_lose,victim->in_room);
				fail = FALSE;
			    }
			    else /* YOWCH! */
			    {
				char_puts("Your weapon sears your flesh!\n",
				    victim);
				dam += number_range(1,obj_lose->level);
				fail = FALSE;
			    }
			}
			else /* drop it if we can */
			{
			    if (can_drop_obj(victim,obj_lose))
			    {
				act("$n throws a burning hot $p to the ground!",
				    victim,obj_lose,NULL,TO_ROOM);
				act("You and drop $p before it burns you.",
				    victim,obj_lose,NULL,TO_CHAR);
				dam += (number_range(1,obj_lose->level) / 6);
				obj_from_char(obj_lose);
				obj_to_room(obj_lose, victim->in_room);
				fail = FALSE;
			    }
			    else /* cannot drop */
			    {
				act("Your skin is seared by $p!",
				    victim,obj_lose,NULL,TO_CHAR);
				dam += (number_range(1,obj_lose->level) / 2);
				fail = FALSE;
			    }
			}
			break;
			}
		    }
		}
	}
	if (fail)
	{
		char_puts("Your spell had no effect.\n", ch);
		char_puts("You feel momentarily warmer.\n",victim);
	}
	else /* damage! */
	{
		if (saves_spell(level,victim,DAM_FIRE))
		    dam = 2 * dam / 3;
		damage(ch,victim,dam,sn,DAM_FIRE,TRUE);
	}
}

/* RT really nasty high-level attack spell */
void spell_holy_word(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int dam;
	int sn_bless, sn_curse;

	if ((sn_bless = sn_lookup("bless")) < 0
	||  (sn_curse = sn_lookup("curse")) < 0)
		return;

	act("$n utters a word of divine power!", ch, NULL, NULL, TO_ROOM);
	char_puts("You utter a word of divine power.\n", ch);

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
		vch_next = vch->next_in_room;

		if (((IS_GOOD(ch) && IS_GOOD(vch)) ||
		    (IS_EVIL(ch) && IS_EVIL(vch)) ||
		    (IS_NEUTRAL(ch) && IS_NEUTRAL(vch)))
		    && vch->invis_level <= HERO) {
			char_puts("You feel full more powerful.\n", vch);
			spell_frenzy(gsn_frenzy, level, ch, vch, TARGET_CHAR);
			spell_bless(sn_bless, level, ch, vch, TARGET_CHAR);
			continue;
		}

		if (is_safe_spell(ch, vch, TRUE))
			continue;

		if ((IS_GOOD(ch) && IS_EVIL(vch))
		||  (IS_EVIL(ch) && IS_GOOD(vch))) {
			spell_curse(sn_curse, level, ch, vch, TARGET_CHAR);
			char_puts("You are struck down!\n",vch);
			dam = dice(level, 6);
			damage(ch, vch, dam, sn, DAM_ENERGY, TRUE);
			continue;
		}

		if (IS_NEUTRAL(ch)) {
			spell_curse(sn_curse, level/2, ch, vch, TARGET_CHAR);
			char_puts("You are struck down!\n", vch);
			dam = dice(level, 4);
			damage(ch, vch, dam, sn, DAM_ENERGY, TRUE);
		}
	}

	char_puts("You feel drained.\n", ch);
	gain_exp(ch, -1 * number_range(1,10) * 5);
	ch->move /= (4/3);
	ch->hit /= (4/3);
}

void spell_identify(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	BUFFER *output;

	output = buf_new(-1);
	format_obj(output, obj);
	if (!IS_SET(obj->extra_flags, ITEM_ENCHANTED))
		format_obj_affects(output, obj->pIndexData->affected,
				   FOA_F_NODURATION | FOA_F_NOAFFECTS);
	format_obj_affects(output, obj->affected, FOA_F_NOAFFECTS);
	page_to_char(buf_string(output), ch);
	buf_free(output);
}

void spell_infravision(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (IS_AFFECTED(victim, AFF_INFRARED)) {
		if (victim == ch)
			char_puts("You can already see in the dark.\n", ch);
		else
			act("$N already has infravision.\n",
			    ch, NULL, victim,TO_CHAR);
		return;
	}
	act("$n's eyes glow {Rred{x.\n", ch, NULL, NULL, TO_ROOM);

	af.where	= TO_AFFECTS;
	af.type		= sn;
	af.level	= level;
	af.duration	= 2 * level;
	af.location	= APPLY_NONE;
	af.modifier	= 0;
	af.bitvector	= AFF_INFRARED;
	affect_to_char(victim, &af);
	char_puts("Your eyes glow {Rred{x.\n", victim);
}



void spell_invisibility(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	AFFECT_DATA af;

	/* object invisibility */
	if (target == TARGET_OBJ)
	{
		obj = (OBJ_DATA *) vo;

		if (IS_OBJ_STAT(obj,ITEM_INVIS))
		{
		    act("$p is already invisible.",ch,obj,NULL,TO_CHAR);
		    return;
		}

		af.where	= TO_OBJECT;
		af.type		= sn;
		af.level	= level;
		af.duration	= level / 4 + 12;
		af.location	= APPLY_NONE;
		af.modifier	= 0;
		af.bitvector	= ITEM_INVIS;
		affect_to_obj(obj,&af);

		act("$p fades out of sight.",ch,obj,NULL,TO_ALL);
		return;
	}

	/* character invisibility */
	victim = (CHAR_DATA *) vo;

	if (IS_AFFECTED(victim, AFF_FAERIE_FIRE))  {
		if (ch == victim)
			char_puts("The glow of faerie fire makes"
				" invisibility impossible.\n", ch);
		else
		    	act("The glow around $N makes"
				" invisibility impossible.",
				ch,NULL,victim,TO_CHAR);
		return;
	}

	if (IS_AFFECTED(victim, AFF_INVIS)) {
		if (ch == victim)
			char_puts("You're already invisible.\n", ch);
		else
		    	act("$N is already invisible.",ch,NULL,victim,TO_CHAR);
		return;
	}

	act("$n fades out of existence.", victim, NULL, NULL, TO_ROOM);

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = (level / 8 + 10);
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = AFF_INVIS;
	affect_to_char(victim, &af);
	char_puts("You fade out of existence.\n", victim);
}



void spell_know_alignment(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	char *msg;

		 if (IS_GOOD(victim)) msg = "$N has a pure and good aura.";
	else if (IS_NEUTRAL(victim)) msg = "$N act as no align.";
	else msg = "$N is the embodiment of pure evil!.";

	act(msg, ch, NULL, victim, TO_CHAR);

	if (!IS_NPC(victim)) {
		switch (victim->ethos) {
		case ETHOS_LAWFUL:
			msg = "$N upholds the laws."; break;
		case ETHOS_NEUTRAL:
			msg = "$N seems ambivalent to society."; break;
		case ETHOS_CHAOTIC:
			msg = "$N seems very chaotic."; break;
		case ETHOS_NONE:
		default:
			msg = "Let $N's reputation, regarding the law, speak for itself."; break;
		}
		act(msg, ch, NULL, victim, TO_CHAR);
	}
	return;
}



void spell_lightning_bolt(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level,4) + 12;
	if (saves_spell(level, victim,DAM_LIGHTNING))
		dam /= 2;
	damage(ch, victim, dam, sn, DAM_LIGHTNING ,TRUE);
}

void spell_locate_object(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	BUFFER *buffer = NULL;
	OBJ_DATA *obj;
	OBJ_DATA *in_obj;
	int number = 0, max_found;

	number = 0;
	max_found = IS_IMMORTAL(ch) ? 100 : 2 * level;

	for (obj = object_list; obj != NULL; obj = obj->next) {
		if (!can_see_obj(ch, obj) || !is_name(target_name, obj->name)
		||  IS_OBJ_STAT(obj,ITEM_NOLOCATE)
		||  number_percent() > 2 * level
		||  ch->level < obj->level)
			continue;

		if (buffer == NULL)
			buffer = buf_new(-1);

		for (in_obj = obj; 
		     in_obj->in_obj != NULL; 
		     in_obj = in_obj->in_obj)
			;

		/* don't show immortal's items*/
		if (in_obj->carried_by != NULL
		&& IS_IMMORTAL(in_obj->carried_by)
		&& !IS_IMMORTAL(ch))
			continue;

		number++;

		if (in_obj->carried_by != NULL
		&&  can_see(ch,in_obj->carried_by))
		    buf_printf(buffer, "One is carried by %s\n",
			fix_short(PERS(in_obj->carried_by, ch)));
		else
		{
		    if (IS_IMMORTAL(ch) && in_obj->in_room != NULL)
			buf_printf(buffer, "One is in %s [Room %d]\n",
				mlstr_cval(in_obj->in_room->name, ch),
				in_obj->in_room->vnum);
		    else
			buf_printf(buffer, "One is in %s\n",
			    in_obj->in_room == NULL ?
			    "somewhere" :
			    mlstr_cval(in_obj->in_room->name, ch));
		}

		if (number >= max_found)
			break;
	}

	if (number < 1) {
		char_puts("Nothing like that in heaven or earth.\n", ch);
	}

	if (buffer != NULL)
		page_to_char(buf_string(buffer),ch);

	buf_free(buffer);
}

void spell_magic_missile(int sn, int level, CHAR_DATA *ch,void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;

	static const int dam_each[] =
	{
		 0,
		 3,  3,  4,  4,  5,	 6,  6,  6,  6,  6,
		 7,  7,  7,  7,  7,	 8,  8,  8,  8,  8,
		 9,  9,  9,  9,  9,	10, 10, 10, 10, 10,
		11, 11, 11, 11, 11,	12, 12, 12, 12, 12,
		13, 13, 13, 13, 13,	14, 14, 14, 14, 14
	};

	int dam;

	if (is_affected(victim, gsn_shield))  {
		const char *text = ch->level > 4 ? "missiles" : "missile";

		act("Your magic $t fizzle out near your victim.",
		    ch, text, victim, TO_CHAR);
		act("Your shield blocks $N's magic $t.",
		    victim, text, ch, TO_CHAR);
		return;
	}

	level = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
	level = UMAX(0, level);
	if (ch->level > 50)
		dam = level / 4;
	else
		dam = number_range(dam_each[level] / 2, dam_each[level] * 2);

	DEBUG(DEBUG_BUG, "magic_missile: %s(%d) dam %d",
		ch->name, LEVEL(ch), dam);

	if (saves_spell(level, victim, DAM_ENERGY))
		dam /= 2;
	damage(ch, victim, dam, sn, DAM_ENERGY ,TRUE);

	if (ch->level > 4)  {
		dam = number_range(dam_each[level] / 2, dam_each[level] * 2);
		if (saves_spell(level, victim, DAM_ENERGY))
			dam /= 2;
		damage(ch, victim, dam, sn, DAM_ENERGY ,TRUE);
	}
	if (ch->level > 8)  {
		dam = number_range(dam_each[level] / 2, dam_each[level] * 2);
		if (saves_spell(level, victim,DAM_ENERGY))
			dam /= 2;
		damage(ch, victim, dam, sn, DAM_ENERGY ,TRUE);
	}
	if (ch->level > 12)  {
		dam = number_range(dam_each[level] / 2, dam_each[level] * 2);
		if (saves_spell(level, victim,DAM_ENERGY))
			dam /= 2;
		damage(ch, victim, dam, sn, DAM_ENERGY ,TRUE);
	}
	if (ch->level > 16)  {
		dam = number_range(dam_each[level] / 2, dam_each[level] * 2);
		if (saves_spell(level, victim,DAM_ENERGY))
			dam /= 2;
		damage(ch, victim, dam, sn, DAM_ENERGY ,TRUE);
	}
}

void spell_mass_healing(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *gch;
	int heal_num, refresh_num;

	heal_num = sn_lookup("heal");
	refresh_num = sn_lookup("refresh");

	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
		if ((IS_NPC(ch) && IS_NPC(gch))
		||  (!IS_NPC(ch) && !IS_NPC(gch))) {
			spell_heal(heal_num, level, ch, (void *) gch,
				   TARGET_CHAR);
			spell_refresh(refresh_num, level, ch, (void *) gch,
				      TARGET_CHAR);
		}
}

void spell_mass_invis(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	AFFECT_DATA af;
	CHAR_DATA *gch;
	int count = 0;

	for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
	{
		if (!is_same_group(gch, ch) || IS_AFFECTED(gch, AFF_INVIS))
		    continue;
		act("$n slowly fades out of existence.", gch, NULL, NULL, TO_ROOM);
		char_puts("You slowly fade out of existence.\n", gch);

		af.where     = TO_AFFECTS;
		af.type      = sn;
		af.level     = 3 * level/5;
		af.duration  = 5 + (level /5);
		af.location  = APPLY_NONE;
		af.modifier  = 0;
		af.bitvector = AFF_INVIS;
		affect_to_char(gch, &af);
		count++;
	}
	if (count == 0)
		char_puts("Everyone in your group is already invisible.\n", ch);

}

void spell_pass_door(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (IS_AFFECTED(victim, AFF_PASS_DOOR))
	{
		if (victim == ch)
		  char_puts("You are already out of phase.\n",ch);
		else
		  act("$N is already shifted out of phase.",ch,NULL,victim,TO_CHAR);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = number_fuzzy(level / 4);
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = AFF_PASS_DOOR;
	affect_to_char(victim, &af);
	act("$n turns {ctrans{Clucent{x.", victim, NULL, NULL, TO_ROOM);
	char_puts("You turn {ctrans{Clucent{x.\n", victim);
}

/* RT plague spell, very nasty */

void spell_plague(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (saves_spell(level,victim,DAM_DISEASE) ||
		(IS_NPC(victim) && IS_SET(victim->pIndexData->act, ACT_UNDEAD)))
	{
		char_puts("You feel under the weather, but it passes.\n",victim);
		act("$N looks under the weather, but it passes.\n",
		   ch, NULL, victim, TO_NOTVICT);
		if (ch != victim)
		  act("$N shakes off the disease.",ch,NULL,victim,TO_CHAR);
		return;
	}

	if (is_affected(victim, sn)) {
		char_puts("The sores on your body well up.\n",victim);
		act("$n's sores well up, but can't get much worse.\n",
			victim, NULL, ch, TO_ROOM);
		return;
	}

	af.where	= TO_AFFECTS;
	af.type 	= sn;
	af.level	= level * 3/4;
	af.duration	= (10 + level / 10);
	af.location	= APPLY_STR;
	af.modifier	= -1 * UMAX(1,3 + level / 15); 
	af.bitvector	= AFF_PLAGUE;
	affect_join(victim,&af);

	char_puts("You scream in agony as plague sores erupt from your skin.\n",victim);
	act("$n screams in agony as plague sores erupt from $s skin.",
		victim,NULL,NULL,TO_ROOM);
}

void spell_poison(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	AFFECT_DATA af;
	AFFECT_DATA *oldaff;


	if (target == TARGET_OBJ)
	{
		obj = (OBJ_DATA *) vo;

		if (obj->pIndexData->item_type == ITEM_FOOD || obj->pIndexData->item_type == ITEM_DRINK_CON)
		{
		    if (IS_OBJ_STAT(obj,ITEM_BLESS) || IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
		    {
			act("Your spell fails to corrupt $p.",ch,obj,NULL,TO_CHAR);
			return;
		    }
		    obj->value[ITEM_FOOD_POISON] = 1;
		    act("$p is infused with poisonous vapors.",ch,obj,NULL,TO_ALL);
		    return;
		}

		if (obj->pIndexData->item_type == ITEM_WEAPON)
		{
		    if (IS_WEAPON_STAT(obj,WEAPON_FLAMING)
		    ||  IS_WEAPON_STAT(obj,WEAPON_FROST)
		    ||  IS_WEAPON_STAT(obj,WEAPON_VAMPIRIC)
		    ||  IS_WEAPON_STAT(obj,WEAPON_SHARP)
		    ||  IS_WEAPON_STAT(obj,WEAPON_VORPAL)
		    ||  IS_WEAPON_STAT(obj,WEAPON_SHOCKING)
		    ||  IS_WEAPON_STAT(obj,WEAPON_HOLY)
		    ||  IS_OBJ_STAT(obj,ITEM_BLESS) || IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
		    {
			act("You can't seem to envenom $p.",ch,obj,NULL,TO_CHAR);
			return;
		    }

		    if (IS_WEAPON_STAT(obj,WEAPON_POISON))
		    {
			act("$p is already envenomed.",ch,obj,NULL,TO_CHAR);
			return;
		    }

		    af.where	 = TO_WEAPON;
		    af.type	 = sn;
		    af.level	 = level / 2;
		    af.duration	 = level/8;
		    af.location	 = 0;
		    af.modifier	 = 0;
		    af.bitvector = WEAPON_POISON;
		    affect_to_obj(obj,&af);

		    act("$p is coated with deadly venom.",ch,obj,NULL,TO_ALL);
		    return;
		}

		act("You can't poison $p.",ch,obj,NULL,TO_CHAR);
		return;
	}

	victim = (CHAR_DATA *) vo;

	if (saves_spell(level, victim,DAM_POISON))
	{
		act("$n turns slightly green, but it passes.",
		   victim,NULL,NULL,TO_ROOM);
		char_puts("You feel momentarily ill, but it passes.\n",victim);
		return;
	}

	oldaff = affect_find(victim->affected, sn);

	if (oldaff) {
		if (get_curr_stat(victim, STAT_STR) -2 < victim->perm_stat[STAT_STR]* 2/ 3) {
			act("$n's blood is already saturated with poison.",
				victim, NULL, NULL, TO_ROOM);
			char_puts("Fortunately, you can't get any sicker.\n", victim);
			return;
		}
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = (10 + level / 10);
	af.location  = APPLY_STR;
	af.modifier  = -2;
	af.bitvector = AFF_POISON;
	affect_join(victim, &af);
	char_puts("You feel very sick.\n", victim);
	act("$n looks very ill.",victim,NULL,NULL,TO_ROOM);
}



void spell_protection_evil(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (IS_AFFECTED(victim, AFF_PROTECT_EVIL) 
	||   IS_AFFECTED(victim, AFF_PROTECT_GOOD))
	{
		if (victim == ch)
		  char_puts("You are already protected.\n",ch);
		else
		  act("$N is already protected.",ch,NULL,victim,TO_CHAR);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = (10 + level / 5);
	af.location  = APPLY_SAVING_SPELL;
	af.modifier  = -(1 + level/10);
	af.bitvector = AFF_PROTECT_EVIL;
	affect_to_char(victim, &af);
	char_puts("You feel holy and pure.\n", victim);
	act("$n is surrounded by a {Choly{x aura.\n", 
	   victim, NULL, NULL, TO_ROOM);
}

void spell_protection_good(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (IS_AFFECTED(victim, AFF_PROTECT_GOOD)
	||   IS_AFFECTED(victim, AFF_PROTECT_EVIL))
	{
		if (victim == ch)
		  char_puts("You are already protected.\n",ch);
		else
		  act("$N is already protected.",ch,NULL,victim,TO_CHAR);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = (10 + level / 5);
	af.location  = APPLY_SAVING_SPELL;
	af.modifier  = -(1+level/10);
	af.bitvector = AFF_PROTECT_GOOD;
	affect_to_char(victim, &af);
	char_puts("You feel aligned with darkness.\n", victim);
	act("$n is surrounded by a {rdemonic{x aura.\n", 
	   victim, NULL, NULL, TO_ROOM);
}

void spell_recharge(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	int chance, percent;

	if (obj->pIndexData->item_type != ITEM_WAND && obj->pIndexData->item_type != ITEM_STAFF)
	{
		char_puts("That item does not carry charges.\n",ch);
		return;
	}

	if (obj->value[ITEM_WAND_CHARGES_REMAINING] >= 3 * (level+15) / 2)
	{
		char_puts("Your skills are not great enough for that.\n",ch);
		return;
	}

	if (obj->value[ITEM_WAND_CHARGES_TOTAL] == 0)
	{
		char_puts("That item has already been recharged once.\n",ch);
		return;
	}

	chance = 40 + 2 * level;

	chance -= obj->value[ITEM_WAND_LEVEL]; /* harder to do high-level spells */
	chance -= (obj->value[ITEM_WAND_CHARGES_TOTAL] 
		- obj->value[ITEM_WAND_CHARGES_REMAINING]) *
		      (obj->value[ITEM_WAND_CHARGES_TOTAL] 
		      - obj->value[ITEM_WAND_CHARGES_REMAINING]);

	chance = UMAX(level/2,chance);

	percent = number_percent();

	if (percent < chance / 2)
	{
		act("$p glows softly.",ch,obj,NULL,TO_CHAR);
		act("$p glows softly.",ch,obj,NULL,TO_ROOM);
		obj->value[ITEM_WAND_CHARGES_REMAINING] 
			= UMAX(obj->value[ITEM_WAND_CHARGES_TOTAL],
				obj->value[ITEM_WAND_CHARGES_REMAINING]);
		obj->value[ITEM_WAND_CHARGES_TOTAL] = 0;
		return;
	}

	else if (percent <= chance)
	{
		int chargeback,chargemax;

		act("$p glows softly.",ch,obj,NULL,TO_CHAR);
		act("$p glows softly.",ch,obj,NULL,TO_CHAR);

		chargemax = obj->value[ITEM_WAND_CHARGES_TOTAL] 
			- obj->value[ITEM_WAND_CHARGES_REMAINING];

		if (chargemax > 0)
		    chargeback = UMAX(1,chargemax * percent / 100);
		else
		    chargeback = 0;

		obj->value[ITEM_WAND_CHARGES_REMAINING] += chargeback;
		obj->value[ITEM_WAND_CHARGES_TOTAL] = 0;
		return;
	}

	else if (percent <= UMIN(95, 3 * chance / 2))
	{
		char_puts("Nothing seems to happen.\n",ch);
		if (obj->value[ITEM_WAND_CHARGES_TOTAL] > 1)
		    obj->value[ITEM_WAND_CHARGES_TOTAL]--;
		return;
	}

	else /* whoops! */
	{
		act("$p glows brightly and explodes!",ch,obj,NULL,TO_CHAR);
		act("$p glows brightly and explodes!",ch,obj,NULL,TO_ROOM);
		extract_obj(obj, 0);
	}
}

void spell_refresh(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	victim->move = UMIN(victim->move + level, victim->max_move);
	if (victim->max_move == victim->move) {
		char_puts("You feel fully refreshed!\n",victim);
		act("$n looks fully refreshed.\n",victim,NULL,ch,TO_ROOM);
	}
	else {
		char_puts("You feel less tired.\n", victim);
		act("$n looks less tired.\n",victim,NULL,ch,TO_ROOM);
	}
	return;
}

void spell_remove_curse(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	bool found = FALSE;

	/* do object cases first */
	if (target == TARGET_OBJ)
	{
		obj = (OBJ_DATA *) vo;

		if (IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_NOREMOVE))
		{
		    if (!IS_OBJ_STAT(obj,ITEM_NOUNCURSE)
		    &&  !saves_dispel(level + 2,obj->level,0))
		    {
			REMOVE_BIT(obj->extra_flags,ITEM_NODROP);
			REMOVE_BIT(obj->extra_flags,ITEM_NOREMOVE);
			act("$p glows blue.",ch,obj,NULL,TO_ALL);
			return;
		    }

		    act("The curse on $p is beyond your power.",ch,obj,NULL,TO_CHAR);
		    act("$n doesn't appear powerful enough to extract the curse from $p.", ch, obj, NULL, TO_ROOM);
		    return;
		}
		else  {
		  char_puts("Nothing happens...\n", ch);
		  return;
		}
	}

	/* characters */
	victim = (CHAR_DATA *) vo;

	if (check_dispel(level,victim,gsn_curse))
	{
		char_puts("You feel better.\n",victim);
		act("$n looks more relaxed.",victim,NULL,NULL,TO_ROOM);
	}
	else
	if (check_dispel(level,victim,gsn_wrath))
	{
		char_puts("You feel divine wrath lifted from you.\n",victim);
		act("$n looks more at ease.",victim,NULL,NULL,TO_ROOM);
	}

   for (obj = victim->carrying; (obj != NULL && !found); obj = obj->next_content)
   {
		if ((IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_NOREMOVE))
		&&  !IS_OBJ_STAT(obj,ITEM_NOUNCURSE))
		{   /* attempt to remove curse */
		    if (!saves_dispel(level,obj->level,0))
		    {
			found = TRUE;
			REMOVE_BIT(obj->extra_flags,ITEM_NODROP);
			REMOVE_BIT(obj->extra_flags,ITEM_NOREMOVE);
			act("Your $p glows blue.",victim,obj,NULL,TO_CHAR);
			act("$n's $p glows blue.",victim,obj,NULL,TO_ROOM);
		    }
		 }
	}
}

void spell_sanctuary(int sn, int level, CHAR_DATA *ch, void *vo, int target)
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

	if (IS_AFFECTED(victim, AFF_SANCTUARY)) {
		if (victim == ch)
			char_puts("You are already in sanctuary.\n", ch);
		else
			act("$N is already in sanctuary.",
			    ch, NULL, victim, TO_CHAR);
		return;
	}

	if (IS_AFFECTED(victim, AFF_BLACK_SHROUD)
		|| IS_AFFECTED(victim, AFF_MINOR_BLACK_SHROUD)
		|| is_affected(victim, gsn_black_shroud)
		|| is_affected(victim, gsn_minor_black_shroud)) {
		if (victim == ch)
	 		char_puts("But you are surrounded by black shroud.\n",
				  ch);
		else
			act("But $N is surrounded by black shroud.\n",
			    ch, NULL, victim, TO_CHAR);
		return;
	}

	if (is_affected(victim, gsn_enhanced_armor)) {
		if (victim == ch)
			char_puts("You are already benefits from enhanced armor.\n", ch);
		else
			act("$N is already benefiting from enhanced armor.",
			    ch, NULL, victim, TO_CHAR);
		return;
	}
	/*
	 * replace resistance with sanctuary
	 */
	if (ch != victim && is_affected(victim, gsn_resistance)) {
		affect_strip(victim,gsn_resistance);
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = level / 6;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = AFF_SANCTUARY;
	affect_to_char(victim, &af);

	if (is_affected(victim, gsn_minor_sanctuary)) {
		affect_strip(victim,gsn_minor_sanctuary);
		act("The {Wwhite{x aura around $n intensifies.", victim, NULL, NULL, TO_ROOM);
		char_puts("Your {Wwhite{x aura intensifies.\n", victim);
	}
	else {
		act("$n is surrounded by a {Wwhite{x aura.", victim, NULL, NULL, TO_ROOM);
		char_puts("You are surrounded by a {Wwhite{x aura.\n", victim);
	}
}

void spell_black_shroud(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA*) vo;
	AFFECT_DATA af;

	if (!IS_IMMORTAL(ch) 
	&& !IS_NPC(ch)
	&& ch->class != CLASS_CLERIC && ch != victim) {
		char_puts("You are not powerful enough to"
			" extend your shroud to another.\n",
			ch);
		return;
	}

	if (IS_AFFECTED(victim, AFF_SANCTUARY)
	|| IS_AFFECTED(victim, AFF_MINOR_SANCTUARY)) {
		if (victim==ch)
			char_puts("But you are in sanctuary.\n", ch);
		else
			act("But $N is in sanctuary.", ch, NULL, victim,TO_CHAR);
		return;
	}

	if (is_affected(victim, gsn_enhanced_armor)) {
		if (victim == ch)
			char_puts("You are already benefits from enhanced armor.\n", ch);
		else
			act("$N is already benefiting from enhanced armor.",
			    ch, NULL, victim, TO_CHAR);
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

	if (is_affected(victim, sn)) {
		if (victim == ch)
			char_puts("You are already protected.\n", ch);
		else
			act("$N is already protected.\n",
			    ch, NULL, victim, TO_CHAR);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = level/6;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = AFF_BLACK_SHROUD;
	affect_to_char(victim, &af);

	if (is_affected(victim, gsn_minor_black_shroud)) {
		affect_strip(victim,gsn_minor_black_shroud);
		act("The {Dblack{x aura around $n intensifies.", victim, NULL, NULL, TO_ROOM);
		char_puts("Your {Dblack{x aura intensifies.\n", victim);
	}
	else {
		act ("$n is surrounded by {Dblack{x aura.", victim, NULL, NULL, TO_ROOM);
		char_puts("You are surrounded by {Dblack{x aura.\n", victim);
	}
}

void spell_shield(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (is_affected(victim, sn))
	{
		if (victim == ch)
		  char_puts("You are already shielded from harm.\n",ch);
		else
		  act("$N is already protected by a shield.",ch,NULL,victim,TO_CHAR);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = (8 + level / 3);
	af.location  = APPLY_AC;
	af.modifier  = -1 * UMAX(20,10 + level / 3); /* af.modifier  = -20;*/
	af.bitvector = 0;
	affect_to_char(victim, &af);
	act("$n is surrounded by a force shield.", victim, NULL, NULL, TO_ROOM);
	char_puts("You are surrounded by a force shield.\n", victim);
	return;
}

void spell_shocking_grasp(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;

	static const int dam_each[] =
	{
		 6,
		 8,  10,  12,  14,  16,	 18, 20, 25, 29, 33,
		36, 39, 39, 39, 40,	40, 41, 41, 42, 42,
		43, 43, 44, 44, 45,	45, 46, 46, 47, 47,
		48, 48, 49, 49, 50,	50, 51, 51, 52, 52,
		53, 53, 54, 54, 55,	55, 56, 56, 57, 57
	};

	int dam;

	level	= UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
	level	= UMAX(0, level);
		if (ch->level > 50)
	dam 	= level / 2 ;
		else
	dam		= number_range(dam_each[level] / 2, dam_each[level] * 2);
	if (saves_spell(level, victim,DAM_LIGHTNING))
		dam /= 2;
	damage(ch, victim, dam, sn, DAM_LIGHTNING ,TRUE);
}

void spell_sleep(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (IS_AFFECTED(victim, AFF_SLEEP)
	||  (IS_NPC(victim) && IS_SET(victim->pIndexData->act, ACT_UNDEAD))
	||  level < victim->level
	||  saves_spell(level, victim, DAM_CHARM))
		return;

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = 1 + level/10;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = AFF_SLEEP;
	affect_join(victim, &af);

	if (IS_AWAKE(victim))
	{
		char_puts("You feel very sleepy ..... zzzzzz.\n", victim);
		act("$n goes to sleep.", victim, NULL, NULL, TO_ROOM);
		victim->position = POS_SLEEPING;
	}
	return;
}

void spell_slow(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (is_affected(victim, sn) || IS_AFFECTED(victim,AFF_SLOW))
	{
		if (victim == ch)
		  char_puts("You can't move any slower!\n",ch);
		else
		  act("$N can't get any slower than that.",
		      ch,NULL,victim,TO_CHAR);
		return;
	}

	if (saves_spell(level,victim,DAM_OTHER)
	||  IS_SET(victim->imm_flags,IMM_MAGIC))
	{
		char_puts("You feel momentarily lethargic.\n",victim);
		act("$n shrugs off a lethargic feeling.\n",
		   victim, NULL, NULL, TO_ROOM);
		return;
	}

	if (IS_AFFECTED(victim,AFF_HASTE))
	{
		if (!check_dispel(level,victim,sn_lookup("haste")))
		{
		    act("$n slows a little, but then speed right back up again.\n",
		       victim, NULL, NULL, TO_ROOM);
		    char_puts("You feel momentarily slower.\n",victim);
		    return;
		}

		act("$n is moving less quickly.",victim,NULL,NULL,TO_ROOM);
		return;
	}


	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = (4 + level / 12);
	af.location  = APPLY_DEX;
	af.modifier  = - UMAX(2,level / 12);
	af.bitvector = AFF_SLOW;
	affect_to_char(victim, &af);
	char_puts("You feel yourself slowing d o w n...\n", victim);
	act("$n starts to move in slow motion.",victim,NULL,NULL,TO_ROOM);
}


void spell_stone_skin(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (is_affected(ch, sn))
	{
		if (victim == ch)
		  char_puts("Your skin is already as hard as a rock.\n",ch);
		else
		  act("$N is already as hard as can be.",ch,NULL,victim,TO_CHAR);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = (10 + level / 5);
	af.location  = APPLY_AC;
	af.modifier  = -1 * UMAX(40,20 + level / 2);  /*af.modifier=-40;*/ 
	af.bitvector = 0;
	affect_to_char(victim, &af);
	act("$n's skin turns to stone.", victim, NULL, NULL, TO_ROOM);
	char_puts("Your skin turns to stone.\n", victim);
	return;
}

void spell_summon(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	bool failed = FALSE;
	CHAR_DATA *victim;
	AFFECT_DATA af;
	int bane_bonus = 0;

	if ((victim = get_char_world(ch, target_name)) == NULL
	||  victim->in_room == NULL) {
		act("You can't seem to locate that life force.", 
		ch, NULL, NULL, TO_CHAR);
		return;
	}

	if (victim == ch
	||  victim->fighting != NULL
	||  !can_see_room(ch, victim->in_room)
        ||  IS_AFFECTED(victim, AFF_CURSE)
        ||  IS_RAFFECTED(victim->in_room, RAFF_CURSE) 
	||  IS_SET(ch->in_room->room_flags, ROOM_SAFE | ROOM_NORECALL |
					    ROOM_PEACE | ROOM_NOSUMMON)
	||  IS_SET(victim->in_room->room_flags, ROOM_SAFE | ROOM_NORECALL |
						ROOM_PEACE | ROOM_NOSUMMON)
	||  IS_SET(ch->in_room->area->flags, AREA_CLOSED | AREA_NOSUMMON)
	||  IS_SET(victim->in_room->area->flags, AREA_CLOSED | AREA_NOSUMMON)
	||  room_is_private(ch->in_room)
	||  IS_SET(victim->imm_flags, IMM_SUMMON)
	||  (victim->in_room->exit[0] == NULL &&
	     victim->in_room->exit[1] == NULL &&
	     victim->in_room->exit[2] == NULL &&
	     victim->in_room->exit[3] == NULL &&
	     victim->in_room->exit[4] == NULL &&
	     victim->in_room->exit[5] == NULL)) {
		failed = TRUE;
	}

	else if (IS_NPC(victim)) {
		/* pets */
		if (IS_AFFECTED(victim, AFF_CHARM)
		&& victim->master != NULL) {
			if (!in_PK(ch, victim->master) 
			&& IS_SET(victim->master->conf_flags, PLR_CONF_NOSUMMON)) {
				act("$N's master does not wish $M to be summoned.", 
				    ch, NULL, victim, TO_CHAR);
				return;
			}
		}

		/* nonpets */
		if (victim->level >= level + 3
		|| victim->pIndexData->pShop != NULL
		||  saves_spell(level, victim, DAM_OTHER)
		||  IS_AGGRO(victim, NULL)
		||  IS_SET(ch->in_room->room_flags, ROOM_NOMOB)) {
			failed = TRUE;
		}
	}
	else {
		if (!in_PK(ch, victim) 
		&&    IS_SET(victim->conf_flags, PLR_CONF_NOSUMMON)) {
			act("$N does not wish to be summoned.", 
			    ch, NULL, victim, TO_CHAR);
			return;
		}

		if (victim->level > LEVEL_HERO
		||  !guild_ok(victim, ch->in_room))
			failed = TRUE;
	}

	if (!failed 
	&& !IS_NPC(victim)
	&& in_PK(ch, victim)
	&& !check_trust(ch, victim)) {
		if (is_affected(ch, sn)) {
			act("Your power is still too weak to summon $N against $S will.",
				ch, NULL, victim, TO_CHAR);
			return;
		}

		if (saves_spell(level, victim, DAM_OTHER))
			failed = TRUE;
	}

	if (failed) {
		act("The etheral plane refuses your attempt.", 
		ch, NULL, victim, TO_CHAR);
		return;
	}

	if (IS_NPC(victim) && victim->in_mind == NULL) {
		char buf[MAX_INPUT_LENGTH];
		snprintf(buf, sizeof(buf), "%d", victim->in_room->vnum);
		victim->in_mind = str_dup(buf);
	}

	if (!IS_NPC(victim) && cast_type(ch) == CAST_CAST) {
		bane_bonus = LEVEL(victim) - LEVEL(ch);
	}

	if (!IS_NPC(victim) && in_PK(ch, victim) 
	&& !check_trust(ch, victim)) {
		af.where     = TO_AFFECTS;
		af.type      = sn;
		af.level     = level;
		af.duration  = number_fuzzy(3);
		af.location  = APPLY_NONE;
		af.modifier  = 0;
		af.bitvector = 0;
		affect_to_char(ch, &af);
	}

	transfer_char(victim, ch, ch->in_room,
		"$N disappears suddenly.",
		"$n has summoned you!",
		"$N arrives suddenly.");
}

void spell_teleport(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	ROOM_INDEX_DATA *dest = NULL;

	if (victim->in_room == NULL
	||  IS_SET(victim->in_room->room_flags, ROOM_NORECALL)
	||  (victim != ch && IS_SET(victim->imm_flags,IMM_SUMMON))
/*	||  !can_gate(ch, victim)*/
	||  (!IS_NPC(ch) && victim->fighting != NULL)
        || IS_SET(victim->in_room->room_flags, ROOM_NOSUMMON)
        ||  IS_SET(victim->in_room->area->flags, AREA_CLOSED | AREA_NOSUMMON)
        ||  room_is_private(victim->in_room)
	||  (victim != ch
	&&  (saves_spell(level - 5, victim,DAM_OTHER)))) {
		char_puts("You failed.\n", ch);
		return;
	}

	dest = get_random_room(victim, NULL);
	if (dest == NULL) {
		char_puts("You failed.\n", ch);
		return;
	}
	transfer_char(victim, ch, dest,
		      "$N vanishes!",
		      "You have been teleported!", 
		      "$N slowly fades into existence.");
}

void spell_bamf(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;

	if (victim->in_room == NULL
	||  saves_spell(level, victim, DAM_OTHER)
	||  IS_SET(victim->in_room->room_flags, ROOM_PEACE | ROOM_SAFE)) {
		send_to_char("You failed.\n",ch);
		return;
	}

	transfer_char(victim, ch,
		      get_random_room(victim, victim->in_room->area),
		      "$N vanishes!",
		      "You have been teleported.",
		      "$N slowly fades into existence.");
}

void spell_ventriloquate(int sn, int level, CHAR_DATA *ch,void *vo, int target)
{
	char speaker[MAX_INPUT_LENGTH];
	CHAR_DATA *vch;

	target_name = one_argument(target_name, speaker, sizeof(speaker));

	for (vch = ch->in_room->people; vch; vch = vch->next_in_room) {
		if (is_name(speaker, vch->name))
			continue;

		if (saves_spell(level, vch, DAM_OTHER)) {
			act("Someone makes $t say '{G$T{x'",
			    vch, speaker, target_name, TO_CHAR);
		}
		else {
			act("$t says '{G$T{x'",
			    vch, speaker, target_name, TO_CHAR);
		}
	}
}

void spell_weaken(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (is_affected(victim, sn)) {
		act("$N couldn't get any weaker.",
			ch, NULL, victim, TO_CHAR);
		act("Your muscles resist more decay.",
			ch, NULL, victim, TO_VICT);
		act("$N's withered muscles resist more decay.",
			ch, NULL, victim, TO_NOTVICT);
		return;
	}
	if (saves_spell(level, victim,DAM_OTHER)) {
		act("$n's muscles wither and then regain their strength .",
			victim, NULL, NULL, TO_ROOM);
		act("Your muscles wither and then regain their strength .",
			ch, NULL, victim, TO_VICT);
		act("$N's muscles wither and then regain their strength .",
			ch, NULL, victim, TO_NOTVICT);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = (4 + level / 12);
	af.location  = APPLY_STR;
	af.modifier  = -1 * (2 + level / 12);
	af.bitvector = AFF_WEAKEN;
	affect_to_char(victim, &af);
	char_puts("You feel your strength slip away.\n", victim);
	act("$n looks tired and weak.",victim,NULL,NULL,TO_ROOM);
	return;
}

void spell_word_of_recall(int sn, int level, CHAR_DATA *ch,void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	ROOM_INDEX_DATA *location, *origin;
	class_t *vcl;
	CHAR_DATA *gch;
	AFFECT_DATA af;

	if (IS_NPC(victim))
		return;
	if (victim->fighting
	&&  (vcl = class_lookup(victim->class))
	&&  !CAN_FLEE(victim, vcl)) {
		if (victim == ch)
			char_puts("Your honour doesn't let you recall!.\n", ch);
		else
			char_printf(ch, "You can't commune this blessing on an "
					"honourable warrior fighting %s!\n",
				    vcl->name);
		return;
	}

/*	if (victim->desc && IS_PUMPED(victim)) {
		act_puts("You are too pumped to pray now.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		return;
	}*/

	act("$n prays for transportation!", ch, NULL, NULL, TO_ROOM);

	if (IS_SET(victim->in_room->room_flags, ROOM_NORECALL)
	||  IS_AFFECTED(victim, AFF_CURSE)
	||  IS_RAFFECTED(victim->in_room, RAFF_CURSE)) {
		char_puts("The weight of an anchor is on your soul.\n", victim);
		return;
	}

	if (is_affected(ch, sn)) {
		if (victim == ch)
			char_puts("Your god ignores pleas that come too often.\n", victim);
		else
			char_puts("Your god ignores pleas that come too often.\n", ch);
		return;
	}

	if (victim->fighting) {
		if (victim == ch)
			gain_exp(victim, 0 - (victim->level + 25));
		stop_fighting(victim, TRUE);
	}

        victim->move /= 2;
	location = get_recall(victim);
	origin = victim->in_room;
        recall(victim, location);
        for (gch = npc_list; gch; gch = gch->next) {
                if (gch->in_room == origin
		&& IS_AFFECTED(gch, AFF_CHARM)
                &&  gch->master == ch
                && !IS_AFFECTED(gch, AFF_SLEEP)
                && gch->position >= POS_STANDING) {
                        recall(gch, location);
                }
        }
	if (is_affected(victim, gsn_rnet_trap))
		affect_strip(victim, gsn_rnet_trap);

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = number_fuzzy(7);
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = 0;
	affect_to_char(victim, &af);

}

/*
 * Draconian spells.
 */
void spell_acid_breath(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam,hp_dam,dice_dam,hpch;

	act("$n spits acid at $N.",ch,NULL,victim,TO_NOTVICT);
	act("$n spits a stream of corrosive acid at you.",ch,NULL,victim,TO_VICT);
	act("You spit acid at $N.",ch,NULL,victim,TO_CHAR);

	if (IS_NPC(victim)) {
		hpch = UMAX(12,ch->hit);
		hp_dam = number_range(hpch/11 + 1, hpch/6);
		dice_dam = dice(level,16);

		dam = UMAX(hp_dam + dice_dam/10,dice_dam + hp_dam/10);
	}
	else {
		dam = dice(level, 10 + (level / 9));
	}

	DEBUG(DEBUG_DAM_BREATH,
		"%s[%d] acid_breath %s[%d] for d% dam (unsaved)",
		mlstr_mval(ch->short_descr),
		ch->level,
		mlstr_mval(victim->short_descr),
		victim->level,
		dam);
	
	if (saves_spell(level,victim,DAM_ACID))
	{
		acid_effect(victim,level/2,dam/4,TARGET_CHAR);
		damage(ch,victim,dam/2,sn,DAM_ACID,TRUE);
	}
	else
	{
		acid_effect(victim,level,dam,TARGET_CHAR);
		damage(ch,victim,dam,sn,DAM_ACID,TRUE);
	}
}



void spell_fire_breath(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	CHAR_DATA *vch, *vch_next;
	int dam, npc_dam, pc_dam, hp_dam, dice_dam;
	int hpch;

	act("$n breathes forth a cone of fire.",ch,NULL,victim,TO_NOTVICT);
	act("$n breathes a cone of hot fire over you!",ch,NULL,victim,TO_VICT);
	act("You breath forth a cone of fire.",ch,NULL,NULL,TO_CHAR);

	hpch = UMAX(10, ch->hit);
	hp_dam  = number_range(hpch/9+1, hpch/5);
	dice_dam = dice(level,20);

	npc_dam = UMAX(hp_dam + dice_dam /10, dice_dam + hp_dam / 10);
	fire_effect(victim->in_room,level,npc_dam/2,TARGET_ROOM);

	pc_dam = dice(level, 10 + (level / 9));

	for (vch = victim->in_room->people; vch != NULL; vch = vch_next)
	{
		vch_next = vch->next_in_room;

		if (is_safe_spell(ch,vch,TRUE)
		||  (IS_NPC(vch) && IS_NPC(ch) 
		&&  (ch->fighting != vch /*|| vch->fighting != ch */)))
		    continue;

		dam = IS_NPC(vch) ? npc_dam : pc_dam;

		if (vch == victim) /* full damage */
		{
			DEBUG(DEBUG_DAM_BREATH,
				"%s[%d] fire_breath %s[%d] for d% dam (unsaved)",
				mlstr_mval(ch->short_descr),
				ch->level,
				mlstr_mval(vch->short_descr),
				vch->level,
				dam);

		    if (saves_spell(level,vch,DAM_FIRE)) {
			fire_effect(vch,level/2,dam/4,TARGET_CHAR);
			damage(ch,vch,dam/2,sn,DAM_FIRE,TRUE);
		    }
		    else
		    {
			fire_effect(vch,level,dam,TARGET_CHAR);
			damage(ch,vch,dam,sn,DAM_FIRE,TRUE);
		    }
		}
		else /* partial damage */
		{
			DEBUG(DEBUG_DAM_BREATH,
				"%s[%d] fire_breath %s[%d] for d% dam (unsaved)",
				mlstr_mval(ch->short_descr),
				ch->level,
				mlstr_mval(vch->short_descr),
				vch->level,
				dam);

		    if (saves_spell(level - 2,vch,DAM_FIRE))
		    {
			fire_effect(vch,level/4,dam/8,TARGET_CHAR);
			damage(ch,vch,dam/4,sn,DAM_FIRE,TRUE);
		    }
		    else
		    {
			fire_effect(vch,level/2,dam/4,TARGET_CHAR);
			damage(ch,vch,dam/2,sn,DAM_FIRE,TRUE);
		    }
		}
	}
}

void spell_frost_breath(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	CHAR_DATA *vch, *vch_next;
	int dam, npc_dam, pc_dam, hp_dam, dice_dam, hpch;

	act("$n breathes out a freezing cone of frost!",ch,NULL,victim,TO_NOTVICT);
	act("$n breathes a freezing cone of frost over you!",
		ch,NULL,victim,TO_VICT);
	act("You breath out a cone of frost.",ch,NULL,NULL,TO_CHAR);

	hpch = UMAX(12,ch->hit);
	hp_dam = number_range(hpch/11 + 1, hpch/6);
	dice_dam = dice(level,18);

	npc_dam = UMAX(hp_dam + dice_dam/10,dice_dam + hp_dam/10);
	cold_effect(victim->in_room,level,npc_dam/2,TARGET_ROOM); 

	pc_dam = dice(level, 8 + (level / 9));

	for (vch = victim->in_room->people; vch != NULL; vch = vch_next)
	{
		vch_next = vch->next_in_room;

		if (is_safe_spell(ch,vch,TRUE)
		||  (IS_NPC(vch) && IS_NPC(ch) 
		&&   (ch->fighting != vch /*|| vch->fighting != ch*/)))
		    continue;

		dam = IS_NPC(vch) ? npc_dam : pc_dam;

		if (vch == victim) /* full damage */
		{
			DEBUG(DEBUG_DAM_BREATH,
				"%s[%d] frost_breath %s[%d] for d% dam (unsaved)",
				mlstr_mval(ch->short_descr),
				ch->level,
				mlstr_mval(vch->short_descr),
				vch->level,
				dam);

		    if (saves_spell(level,vch,DAM_COLD))
		    {
			cold_effect(vch,level/2,dam/4,TARGET_CHAR);
			damage(ch,vch,dam/2,sn,DAM_COLD,TRUE);
		    }
		    else
		    {
			cold_effect(vch,level,dam,TARGET_CHAR);
			damage(ch,vch,dam,sn,DAM_COLD,TRUE);
		    }
		}
		else
		{
			DEBUG(DEBUG_DAM_BREATH,
				"%s[%d] frost_breath %s[%d] for d% dam (unsaved)",
				mlstr_mval(ch->short_descr),
				ch->level,
				mlstr_mval(vch->short_descr),
				vch->level,
				dam);

		    if (saves_spell(level - 2,vch,DAM_COLD))
		    {
			cold_effect(vch,level/4,dam/8,TARGET_CHAR);
			damage(ch,vch,dam/4,sn,DAM_COLD,TRUE);
		    }
		    else
		    {
			cold_effect(vch,level/2,dam/4,TARGET_CHAR);
			damage(ch,vch,dam/2,sn,DAM_COLD,TRUE);
		    }
		}
	}
}

	
void spell_gas_breath(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int dam, npc_dam, pc_dam, hp_dam, dice_dam, hpch;

	act("$n breathes out a cloud of poisonous gas!",ch,NULL,NULL,TO_ROOM);
	act("You breath out a cloud of poisonous gas.",ch,NULL,NULL,TO_CHAR);

	hpch = UMAX(16,ch->hit);
	hp_dam = number_range(hpch/15+1,8);
	dice_dam = dice(level,13);

	npc_dam = UMAX(hp_dam + dice_dam/10,dice_dam + hp_dam/10);
	poison_effect(ch->in_room,level,npc_dam,TARGET_ROOM);

	pc_dam = dice(level, 8 + (level / 20));

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	{
		vch_next = vch->next_in_room;

		if (is_safe_spell(ch,vch,TRUE)
		||  (IS_NPC(ch) && IS_NPC(vch) 
		&&   (ch->fighting == vch || vch->fighting == ch)))
		    continue;

		dam = IS_NPC(vch) ? npc_dam : pc_dam;

		DEBUG(DEBUG_DAM_BREATH,
			"%s[%d] gas_breath %s[%d] for d% dam (unsaved)",
			mlstr_mval(ch->short_descr),
			ch->level,
			mlstr_mval(vch->short_descr),
			vch->level,
			dam);

		if (saves_spell(level,vch,DAM_POISON))
		{
		    poison_effect(vch,level/2,dam/4,TARGET_CHAR);
		    damage(ch,vch,dam/2,sn,DAM_POISON,TRUE);
		}
		else
		{
		    poison_effect(vch,level,dam,TARGET_CHAR);
		    damage(ch,vch,dam,sn,DAM_POISON,TRUE);
		}
	}
}

void spell_lightning_breath(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam, npc_dam, pc_dam, hp_dam, dice_dam, hpch;

	act("$n breathes a bolt of lightning at $N.",ch,NULL,victim,TO_NOTVICT);
	act("$n breathes a bolt of lightning at you!",ch,NULL,victim,TO_VICT);
	act("You breathe a bolt of lightning at $N.",ch,NULL,victim,TO_CHAR);

	hpch = UMAX(10,ch->hit);
	hp_dam = number_range(hpch/9+1,hpch/5);
	dice_dam = dice(level,20);

	npc_dam = UMAX(hp_dam + dice_dam/10,dice_dam + hp_dam/10);
	pc_dam = dice(level, 10 + (level / 9));

	dam = IS_NPC(victim) ? npc_dam : pc_dam;

	DEBUG(DEBUG_DAM_BREATH,
		"%s[%d] lightning_breath %s[%d] for d% dam (unsaved)",
		mlstr_mval(ch->short_descr),
		ch->level,
		mlstr_mval(victim->short_descr),
		victim->level,
		dam);

	if (saves_spell(level,victim,DAM_LIGHTNING))
	{
		shock_effect(victim,level/2,dam/4,TARGET_CHAR);
		damage(ch,victim,dam/2,sn,DAM_LIGHTNING,TRUE);
	}
	else
	{
		shock_effect(victim,level,dam,TARGET_CHAR);
		damage(ch,victim,dam,sn,DAM_LIGHTNING,TRUE); 
	}
}

/*
 * Spells for mega1.are from Glop/Erkenbrand.
 */
void spell_general_purpose(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
 
	dam = number_range(25, 100);
	if (saves_spell(level, victim, DAM_PIERCE))
	    dam /= 2;
	damage(ch, victim, dam, sn, DAM_PIERCE ,TRUE);
}

void spell_high_explosive(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
 
	dam = number_range(30, 120);
	if (saves_spell(level, victim, DAM_PIERCE))
	    dam /= 2;
	damage(ch, victim, dam, sn, DAM_PIERCE ,TRUE);
}

void spell_find_object(int sn, int level, CHAR_DATA *ch, void *vo, int target) 
{
	BUFFER *buffer = NULL;
	OBJ_DATA *obj;
	OBJ_DATA *in_obj;
	int number = 0, max_found;

	number = 0;
	max_found = IS_IMMORTAL(ch) ? 200 : 2 * level;


	for (obj = object_list; obj != NULL; obj = obj->next) {
		if (!can_see_obj(ch, obj) || !is_name(target_name, obj->name)
			|| number_percent() > 2 * level
			||   ch->level < obj->level)
		    continue;

		/* don't show quest items */
		if (!IS_IMMORTAL(ch) && IS_SET(obj->pIndexData->extra_flags, ITEM_NOFIND))
			continue;

		if (buffer == NULL)
			buffer = buf_new(-1);
		number++;

		for (in_obj = obj; in_obj->in_obj != NULL;
						in_obj = in_obj->in_obj)
			;

		/* don't show immortal's items*/
		if (in_obj->carried_by != NULL
		&& IS_IMMORTAL(in_obj->carried_by)
		&& !IS_IMMORTAL(ch))
			continue;

		if (in_obj->carried_by != NULL
		&&  can_see(ch,in_obj->carried_by))
			buf_printf(buffer, "One is carried by %s\n",
				fix_short(PERS(in_obj->carried_by, ch)));
		else {
			if (IS_IMMORTAL(ch) && in_obj->in_room != NULL)
				buf_printf(buffer, "One is in %s [Room %d]\n",
					mlstr_cval(in_obj->in_room->name, ch),
					in_obj->in_room->vnum);
			else
				buf_printf(buffer, "One is in %s\n",
					in_obj->in_room == NULL ?
					"somewhere" :
					mlstr_cval(in_obj->in_room->name, ch));
		}

		if (number >= max_found)
			break;
	}

	if (buffer == NULL)
		char_puts("Nothing like that in heaven or earth.\n", ch);
	else {
		page_to_char(buf_string(buffer),ch);
		buf_free(buffer);
	}
}

void spell_lightning_shield(int sn, int level, CHAR_DATA *ch, void *vo, int target) 
{
	AFFECT_DATA af,af2;

	if (is_affected_room(ch->in_room, sn))
	{
		char_puts("This room has already shielded.\n",ch);
		return;
	}

	if (is_affected(ch,sn))
	{
		char_puts("This spell is used too recently.\n",ch);
		return;
	}
   
	af.where     = TO_ROOM_AFFECTS;
	af.type      = sn;
	af.level     = ch->level;
	af.duration  = level / 40;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = RAFF_LSHIELD;
	affect_to_room(ch->in_room, &af);

	af2.where     = TO_AFFECTS;
	af2.type      = sn;
	af2.level	 = ch->level;
	af2.duration  = level / 10;
	af2.modifier  = 0;
	af2.location  = APPLY_NONE;
	af2.bitvector = 0;
	affect_to_char(ch, &af2);

	ch->in_room->owner = str_qdup(ch->name);
	char_puts("The room starts to be filled with lightning.\n", ch);
	act("The room starts to be filled with $n's lightning.",ch,NULL,NULL,TO_ROOM);
	return;
}

void spell_shocking_trap(int sn, int level, CHAR_DATA *ch, void *vo, int target) 
{
	AFFECT_DATA af,af2;

	if (is_affected_room(ch->in_room, sn))
	{
		char_puts("This room has already trapped with shocks waves.\n",ch);
		return;
	}

	if (is_affected(ch,sn))
	{
		char_puts("This spell is used too recently.\n",ch);
		return;
	}
   
	af.where     = TO_ROOM_AFFECTS;
	af.type      = sn;
	af.level     = ch->level;
	af.duration  = level / 40;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = RAFF_SHOCKING;
	affect_to_room(ch->in_room, &af);

	af2.where     = TO_AFFECTS;
	af2.type      = sn;
	af2.level	 = level;
	af2.duration  = ch->level / 10;
	af2.modifier  = 0;
	af2.location  = APPLY_NONE;
	af2.bitvector = 0;
	affect_to_char(ch, &af2);
	char_puts("The room starts to be filled with shock waves.\n", ch);
	act("The room starts to be filled with $n's shock waves.",ch,NULL,NULL,TO_ROOM);
	return;
}

void spell_acid_arrow(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level, 9);
	if (saves_spell(level, victim, DAM_ACID))
		dam /= 2;
	damage(ch, victim, dam, sn,DAM_ACID,TRUE);
}


/* energy spells */
void spell_etheral_fist(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;
	int chance = 100;

	dam = dice(level, 8);

	if (IS_AFFECTED(victim, AFF_PASS_DOOR)) {
		act("A fist of black, otherworldly ether passes through $N, harmlessly."
			,ch,NULL,victim,TO_NOTVICT);
		act("A fist of black, otherworldly ether passes through you, harmlessly."
			,ch,NULL,victim,TO_VICT);
		act("A fist of black, otherworldly ether passes through $N, harmlessly."
			,ch,NULL,victim,TO_CHAR);
		return;
	}

	if (saves_spell(level, victim, DAM_ENERGY)) {
		dam /= 2;
		chance /= 2;
	}

	act("A fist of black, otherworldly ether rams into $N."
			,ch,NULL,victim,TO_ROOM);
	act("A fist of black, otherworldly ether rams into you!"
			,ch,NULL,victim,TO_VICT); 
	damage(ch, victim, dam, sn,DAM_ENERGY,TRUE);
	
	/* modifiers */
	/* stats */

	/*level*/
	chance += (LEVEL(ch) - LEVEL(victim)) * 2;

	if (number_percent() < chance ) {
		WAIT_STATE(victim, 2*PULSE_VIOLENCE);
		victim->position = POS_RESTING;
		act("The etheral fist knocks $n to the ground!",
			victim, NULL, NULL, TO_ROOM);
		act("The etheral fist knocks you to the ground!",
			ch, NULL, victim, TO_VICT); 
	}	

}

void spell_spectral_furor(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level, 8);
	if (saves_spell(level, victim, DAM_ENERGY))
		dam /= 2;
	act("The fabric of the cosmos strains in fury about $N!",
			ch,NULL,victim,TO_NOTVICT);
	damage(ch, victim, dam, sn,DAM_ENERGY,TRUE);
}

void spell_disruption(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level, 9);
	if (saves_spell(level, victim, DAM_ENERGY))
		dam /= 2;
	act("A weird energy encompasses $N, causing you to question $S continued existence.",
			ch,NULL,victim,TO_NOTVICT);
	damage(ch, victim, dam, sn,DAM_ENERGY,TRUE);
}


void spell_sonic_resonance(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level, 7);
	if (saves_spell(level, victim, DAM_ENERGY))
		dam /= 2;
	act("A cone of harmonic energy enshrouds $N causing $S to resonate.",
			ch,NULL,victim,TO_NOTVICT);
	damage(ch, victim, dam, sn,DAM_ENERGY,TRUE);
}

/* mental */
void spell_mind_wrack(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level, 7);
	if (saves_spell(level, victim, DAM_MENTAL))
		dam /= 2;
	act("$n stares intently at $N, causing $N to seem very lethargic.",
			ch,NULL,victim,TO_NOTVICT);
	damage(ch, victim, dam, sn,DAM_MENTAL,TRUE);
}

void spell_mind_wrench(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level, 9);
	if (saves_spell(level, victim, DAM_MENTAL))
		dam /= 2;
	act("$n stares intently at $N, causing $N to seem very hyperactive.",
			ch,NULL,victim,TO_NOTVICT);
	damage(ch, victim, dam, sn,DAM_MENTAL,TRUE);
}

/* acid */
void spell_sulfurus_spray(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level, 7);
	if (saves_spell(level, victim, DAM_ACID))
		dam /= 2;
	act("A stinking spray of sulfurous liquid rains down on $N." ,
			ch,NULL,victim,TO_NOTVICT);
	damage(ch, victim, dam, sn,DAM_ACID,TRUE);
}

void spell_caustic_font(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level, 9);
	if (saves_spell(level, victim, DAM_ACID))
		dam /= 2;
	act("A fountain of caustic liquid forms below $N. The smell of $S degenerating tissues is revolting! ",
			ch,NULL,victim,TO_NOTVICT);
	damage(ch, victim, dam, sn,DAM_ACID,TRUE);
}

void spell_acetum_primus(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level, 8);
	if (saves_spell(level, victim, DAM_ACID))
		dam /= 2;
	act("A cloak of primal acid enshrouds $N, sparks form as it consumes all it touches. ",
			ch,NULL,victim,TO_NOTVICT);
	damage(ch, victim, dam, sn,DAM_ACID,TRUE);
}

/*  Electrical  */

void spell_galvanic_whip(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level, 7);
	if (saves_spell(level, victim, DAM_LIGHTNING))
		dam /= 2;
	act("$n conjures a whip of bound ether, which lashes ferociously at $N.",
			ch,NULL,victim,TO_NOTVICT);
	damage(ch, victim, dam, sn,DAM_LIGHTNING,TRUE);
}

void spell_magnetic_trust(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level, 8);
	if (saves_spell(level, victim, DAM_LIGHTNING))
		dam /= 2;
	act("An unseen energy moves nearby, causing your hair to stand on end!",
			ch,NULL,victim,TO_NOTVICT);
	damage(ch, victim, dam, sn,DAM_LIGHTNING,TRUE);
}

void spell_quantum_spike(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	dam = dice(level, 9);
	if (saves_spell(level, victim, DAM_LIGHTNING))
		dam /= 2;
	act("$N seems to dissolve into tiny unconnected particles, then is painfully reassembled.",
			ch,NULL,victim,TO_NOTVICT);
	damage(ch, victim, dam, sn,DAM_LIGHTNING,TRUE);
}

/* negative */
void spell_hand_of_undead(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

	if (saves_spell(level, victim, DAM_NEGATIVE)) {
		char_puts("You feel a momentary chill.\n",victim);
		act("$N resists the hand of undead.",ch, NULL, victim, TO_CHAR);
		act("$n shakes off a momentary chill.",victim, NULL, NULL, TO_ROOM);
		return;
	}

	if (IS_NPC(victim) && IS_SET(victim->pIndexData->act, ACT_UNDEAD)) {
		 char_puts("Your victim is unaffected by hand of undead.\n",ch);
		 return;
	}

	if (victim->level <= 2)
		dam		 = ch->hit + 1;
	else {
		dam = dice(level, 10);
		victim->mana	/= 2;
		victim->move	/= 2;
		ch->hit		+= dam / 2;
	}

	char_puts("You feel your life slipping away!\n",victim);
	act("$N is grasped by an incomprehensible hand of undead!",
			ch,NULL,victim,TO_NOTVICT);
	damage(ch, victim, dam, sn,DAM_NEGATIVE,TRUE);
}

static inline void
astral_walk(CHAR_DATA *ch, CHAR_DATA *victim)
{
	transfer_char(ch, victim, victim->in_room,
		      "$N disappears in a flash of {Ylight{x!",
		      "You travel via astral planes and go to $n.",
		      "$N appears in a flash of {Ylight{x!");
}

/* travel via astral plains */
void spell_astral_walk(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim;
	CHAR_DATA *gch = NULL;
	ROOM_INDEX_DATA *origin;
	AFFECT_DATA af;

	if (!IS_IMMORTAL(ch) && is_affected(ch, sn)) {
		char_puts("You'll need to wait for your stomach to settle first.\n",ch);
		return;
	}

	if ((victim = get_char_world(ch, target_name)) == NULL
	||  victim->level >= level + 3
	||  saves_spell(level, victim, DAM_OTHER)
	||  !can_gate(ch, victim)) {
		char_puts("You failed.\n", ch);
		return;
	}

	af.where		= TO_AFFECTS;
	af.type               = sn;
	af.level              = level; 
	af.duration           = number_fuzzy(1);
	af.bitvector          = 0;
	af.modifier           = 0;
	af.location           = APPLY_NONE;
	affect_to_char(ch, &af);  

	origin = ch->in_room;
	astral_walk(ch, victim);
        for (gch = npc_list; gch; gch = gch->next) {
                if (gch->in_room == origin
		&& IS_AFFECTED(gch, AFF_CHARM)
                &&  gch->master == ch
                && !IS_AFFECTED(gch, AFF_SLEEP)
                && gch->position >= POS_STANDING) {

			astral_walk(gch, victim);
                }
        }
}


/* vampire version astral walk */
void spell_mist_walk(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim;
	AFFECT_DATA af;

	if (!IS_IMMORTAL(ch) && is_affected(ch, sn)) {
		char_puts("You cannot take mist form yet.\n", ch);
		return;
	}

	if ((victim = get_char_world(ch, target_name)) == NULL
	||  victim->level >= level - 5
	||  saves_spell(level, victim, DAM_OTHER)
	||  !can_gate(ch, victim)) {
		char_puts("You failed.\n", ch);
		return;
	}

	af.where		= TO_AFFECTS;
	af.type               = sn;
	af.level              = level; 
	af.duration           = number_fuzzy(5);
	af.bitvector          = 0;
	af.modifier           = 0;
	af.location           = APPLY_NONE;
	affect_to_char(ch, &af);  

	transfer_char(ch, NULL, victim->in_room,
		      "$N dissolves into a cloud of {Mgl{mo{Mw{mi{Mng{x mist, then vanishes!",
		      "You dissolve into a cloud of {Mgl{mo{Mw{mi{Mng{x mist, then flow to your target.",
		      "A cloud of {Mgl{mo{Mw{mi{Mng{x mist engulfs you, then withdraws to unveil $N!");
}

/*  Cleric version of astra_walk  */
void spell_solar_flight(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim;
	AFFECT_DATA af;

	if (!IS_IMMORTAL(ch) && is_affected(ch, sn)) {
		char_puts("You cannot summon the solar winds yet.\n", ch);
		return;
	}

	if  (time_info.hour > 18 || time_info.hour < 8) {
		 char_puts("You need sunlight for solar flight.\n",ch);
		 return;
	}

	if ((victim = get_char_world(ch, target_name)) == NULL
	||  victim->level >= level + 1
	||  saves_spell(level, victim, DAM_OTHER)
	||  !can_gate(ch, victim)) {
		char_puts("You failed.\n", ch);
		return;
	}

	af.where		= TO_AFFECTS;
	af.type               = sn;
	af.level              = level; 
	af.duration           = number_fuzzy(3);
	af.bitvector          = 0;
	af.modifier           = 0;
	af.location           = APPLY_NONE;
	affect_to_char(ch, &af);  

	transfer_char(ch, NULL, victim->in_room,
		      "$N disappears in a blinding flash of {Ylight{x!",
		      "You dissolve in a blinding flash of {Ylight{x!",
		      "$N appears in a blinding flash of {Ylight{x!");
}

/* travel via astral plains */
void spell_helical_flow(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim;
	AFFECT_DATA af;

	if (!IS_IMMORTAL(ch) && is_affected(ch, sn)) {
		char_puts("You cannot summon the solar winds yet.\n", ch);
		return;
	}

	if ((victim = get_char_world(ch, target_name)) == NULL
	||  victim->level >= level + 3
	||  saves_spell(level, victim, DAM_OTHER)
	||  !can_gate(ch, victim)) {
		char_puts("You failed.\n", ch);
		return;
	}

	af.where		= TO_AFFECTS;
	af.type               = sn;
	af.level              = level; 
	af.duration           = number_fuzzy(3);
	af.bitvector          = 0;
	af.modifier           = 0;
	af.location           = APPLY_NONE;
	affect_to_char(ch, &af);  

	transfer_char(ch, NULL, victim->in_room,
		      "$N coils into an ascending column of colour, vanishing into thin air.",
		      "You coil into an ascending column of colour, vanishing into thin air.",
		      "A coil of colours descends from above, revealing $N as it dissipates.");
}

void spell_corruption(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (IS_AFFECTED(victim, AFF_CORRUPTION)) {
		act("$N is already corrupting.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (IS_IMMORTAL(victim)
	||  saves_spell(level, victim, DAM_NEGATIVE)
	||  (IS_NPC(victim) && IS_SET(victim->pIndexData->act, ACT_UNDEAD))) {
		if (ch == victim)
			act_puts("Your skin itches, but the feeling passes.",
				 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		else
			act_puts("$N seems to be unaffected.",
				 ch, NULL, victim, TO_CHAR, POS_DEAD);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type 	 = sn;
	af.level	 = level * 3/4;
	af.duration  = (10 + level / 5);
	af.location  = APPLY_NONE;
	af.modifier  = 0; 
	af.bitvector = AFF_CORRUPTION;
	affect_join(victim,&af);

	act("You scream in agony as you start to decay into dust.",
	    victim, NULL, NULL, TO_CHAR);
	act("$n screams in agony as $n start to decay into dust.",
	    victim, NULL, NULL, TO_ROOM);
}

void spell_hurricane(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int dam,hp_dam,dice_dam,hpch;

	act("$n prays the gods of the storm for help.",ch,NULL,NULL,TO_NOTVICT);
	act("You pray the gods of the storm to help you.",ch,NULL,NULL,TO_CHAR);

	hpch = UMAX(16,ch->hit);
	hp_dam = number_range(hpch/15+1,8);
	dice_dam = dice(level,12);

	dam = UMAX(hp_dam + dice_dam/10,dice_dam + hp_dam/10);

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	{
		vch_next = vch->next_in_room;

		if (is_safe_spell(ch,vch,TRUE)
		||  (IS_NPC(ch) && IS_NPC(vch) 
		&&   (ch->fighting == vch || vch->fighting == ch)))
		    continue;

		if (!IS_AFFECTED(vch,AFF_FLYING)) dam /= 2;

		if (vch->size == SIZE_TINY)  dam *= 1.5;
		else if (vch->size == SIZE_SMALL)  dam *= 1.3;
		else if (vch->size == SIZE_MEDIUM)  dam *= 1;
		else if (vch->size == SIZE_LARGE)  dam *= 0.9;
		else if (vch->size == SIZE_HUGE)  dam *= 0.7;
		else dam *= 0.5;

		if (saves_spell(level,vch,DAM_OTHER))
		    damage(ch,vch,dam/2,sn,DAM_OTHER,TRUE);
		else
		    damage(ch,vch,dam,sn,DAM_OTHER,TRUE);
	}
}


void spell_detect_undead(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

	if (IS_AFFECTED(victim, AFF_DETECT_UNDEAD)) {
		if (victim == ch)
			char_puts("You can already sense undead.\n", ch);
		else
			act("$N can already detect undead.",
			    ch, NULL, victim, TO_CHAR);
		return;
	}

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level	 = level;
	af.duration  = (5 + level / 3);
	af.modifier  = 0;
	af.location  = APPLY_NONE;
	af.bitvector = AFF_DETECT_UNDEAD;
	affect_to_char(victim, &af);
	char_puts("You see death all around you.\n", victim);
 	act("Something in $n's eyes deaden to the world.\n"
		,victim, NULL, NULL, TO_ROOM);
}

int dispel_affects (int level, CHAR_DATA *victim) {
	int found = FALSE;

	if (IS_CLAN_GUARD(victim)) {
	    act("The magic bound to $n is much too powerful.",victim,NULL,NULL,TO_ROOM);
	    return FALSE;
	}

	if (check_dispel(level,victim,sn_lookup("armor"))) {
		found = TRUE;
	    act("$n no longer looks as armored.",victim,NULL,NULL,TO_ROOM);
	}
 
	if (check_dispel(level,victim,sn_lookup("enhanced armor"))) {
		found = TRUE;
	    act("$n's armor no longer looks enhanced.",victim,NULL,NULL,TO_ROOM);
	}
 
	if (check_dispel(level,victim,sn_lookup("bless"))) {
	    found = TRUE;
	    act("Divine blessings abandon $n.",victim,NULL,NULL,TO_ROOM);
	}
 
	if (check_dispel(level,victim,sn_lookup("blindness")))
	{
	    found = TRUE;
	    act("$n is no longer blind.",victim,NULL,NULL,TO_ROOM);
	}

	if (check_dispel(level,victim,sn_lookup("calm")))
	{
		found = TRUE;
		act("$n no longer looks so peaceful...",victim,NULL,NULL,TO_ROOM);
	}
 
	if (check_dispel(level,victim,sn_lookup("change sex")))
	{
	    found = TRUE;
		act("$n looks more like $mself again.",victim,NULL,NULL,TO_ROOM);
	}

	if (check_dispel(level,victim,sn_lookup("charm person")))
	{
		found = TRUE;
		act("$n regains $s free will.",victim,NULL,NULL,TO_ROOM);
	}

	if (check_dispel(level,victim,sn_lookup("chill touch")))
	{
		found = TRUE;
		act("$n looks warmer.",victim,NULL,NULL,TO_ROOM);
	}

	if (check_dispel(level,victim,sn_lookup("curse"))) {
		found = TRUE;
		act("$n look exorcized of curses.",victim,NULL,NULL,TO_ROOM);
	}

	if (check_dispel(level,victim,sn_lookup("detect evil")))
		found = TRUE;

	if (check_dispel(level,victim,sn_lookup("detect good")))
		found = TRUE;

	if (check_dispel(level,victim,sn_lookup("detect hidden")))
		found = TRUE;

	if (check_dispel(level,victim,sn_lookup("detect invis")))
		found = TRUE;

	if (check_dispel(level,victim,sn_lookup("detect hidden")))
		found = TRUE;

	if (check_dispel(level,victim,sn_lookup("detect magic")))
		found = TRUE;

	if (check_dispel(level,victim,sn_lookup("faerie fire")))
	{
		act("$n's outline fades.",victim,NULL,NULL,TO_ROOM);
		found = TRUE;
	}

	if (check_dispel(level,victim,sn_lookup("fly")))
	{
		act("$n falls to the ground!",victim,NULL,NULL,TO_ROOM);
		found = TRUE;
	}

	if (check_dispel(level,victim,gsn_frenzy))
	{
		act("$n no longer looks so wild.",victim,NULL,NULL,TO_ROOM);;
		found = TRUE;
	}

	if (check_dispel(level,victim,sn_lookup("giant strength")))
	{
		act("$n no longer looks so mighty.",victim,NULL,NULL,TO_ROOM);
		found = TRUE;
	}

	if (check_dispel(level,victim,sn_lookup("haste")))
	{
		act("$n is no longer moving so quickly.",victim,NULL,NULL,TO_ROOM);
		found = TRUE;
	}

	if (check_dispel(level,victim,sn_lookup("infravision")))
		found = TRUE;

	if (check_dispel(level,victim,sn_lookup("invis")))
	{
		act("$n fades into existance.",victim,NULL,NULL,TO_ROOM);
		found = TRUE;
	}

	if (check_dispel(level,victim,sn_lookup("mass invis")))
	{
		act("$n fades into existance.",victim,NULL,NULL,TO_ROOM);
		found = TRUE;
	}

	if (check_dispel(level,victim,sn_lookup("pass door"))) {
		found = TRUE;
		act("$n solidifies.",victim,NULL,NULL,TO_ROOM);
	}

	if (check_dispel(level,victim,sn_lookup("protection evil"))) {
		found = TRUE;
		act("$n has lost $s ward against evil.",victim,NULL,NULL,TO_ROOM);
	}

	if (check_dispel(level,victim,sn_lookup("protection good"))) {
		found = TRUE;
		act("$n has lost $m ward against good.",victim,NULL,NULL,TO_ROOM);
	}

	if (check_dispel(level,victim,sn_lookup("sanctuary"))) {
		act("The white aura around $n's body vanishes.",
		    victim,NULL,NULL,TO_ROOM);
		found = TRUE;
	}
/*Zz - added b/c loaded mobs might just have flag*/
        if (IS_AFFECTED(victim,AFF_SANCTUARY)
                && !saves_dispel(level, victim->level,-1)
                && !is_affected(victim,sn_lookup("sanctuary")))
        {
                REMOVE_BIT(victim->affected_by,AFF_SANCTUARY);
            act("The white aura around $n's body vanishes.",
                victim,NULL,NULL,TO_ROOM);
            found = TRUE;
        }

	if (check_dispel(level,victim,sn_lookup("minor sanctuary"))) {
		act("The thin white aura around $n's body vanishes.",
		    victim,NULL,NULL,TO_ROOM);
		found = TRUE;
	}
/*Zz - added b/c loaded mobs might just have flag*/
        if (IS_AFFECTED(victim,AFF_MINOR_SANCTUARY)
                && !saves_dispel(level, victim->level,-1)
                && !is_affected(victim,sn_lookup("minor_sanctuary")))
        {
                REMOVE_BIT(victim->affected_by,AFF_MINOR_SANCTUARY);
            act("The thin white aura around $n's body vanishes.",
                victim,NULL,NULL,TO_ROOM);
            found = TRUE;
        }

	if (check_dispel(level, victim, sn_lookup("black shroud"))) { 
		act("The black aura around $n's body vanishes.",
		    victim, NULL, NULL, TO_ROOM);
		found = TRUE;
	}

/*Zz - added b/c loaded mobs might just have flag*/
        if (IS_AFFECTED(victim,AFF_BLACK_SHROUD)
                && !saves_dispel(level, victim->level,-1)
                && !is_affected(victim,sn_lookup("black shroud")))
        {
                REMOVE_BIT(victim->affected_by,AFF_BLACK_SHROUD);
            act("The black aura around $n's body vanishes.",
                victim,NULL,NULL,TO_ROOM);
            found = TRUE;
        }

	if (check_dispel(level, victim, sn_lookup("minor black shroud"))) { 
		act("The thin black aura around $n's body vanishes.",
		    victim, NULL, NULL, TO_ROOM);
		found = TRUE;
	}

/*Zz - added b/c loaded mobs might just have flag*/
        if (IS_AFFECTED(victim,AFF_MINOR_BLACK_SHROUD)
                && !saves_dispel(level, victim->level,-1)
                && !is_affected(victim,sn_lookup("minor black shroud")))
        {
                REMOVE_BIT(victim->affected_by,AFF_MINOR_BLACK_SHROUD);
            act("The thin black aura around $n's body vanishes.",
                victim,NULL,NULL,TO_ROOM);
            found = TRUE;
        }

	if (check_dispel(level, victim, sn_lookup("shield"))) {
		act("The shield protecting $n vanishes.",
		    victim, NULL, NULL, TO_ROOM);
		found = TRUE;
	}

	if (check_dispel(level,victim,sn_lookup("sleep"))) {
		found = TRUE;
		act("$n is less sleepy.",victim,NULL,NULL,TO_ROOM);
	}

	if (check_dispel(level,victim,sn_lookup("slow")))
	{
		act("$n is no longer moving so slowly.",victim,NULL,NULL,TO_ROOM);
		found = TRUE;
	}

	if (check_dispel(level,victim,sn_lookup("stone skin")))
	{
		act("$n's skin regains its normal texture.",victim,NULL,NULL,TO_ROOM);
		found = TRUE;
	}

	if (check_dispel(level,victim,sn_lookup("weaken")))
	{
		act("$n looks stronger.",victim,NULL,NULL,TO_ROOM);
	    found = TRUE;
	}
 
	if (check_dispel(level,victim,sn_lookup("shielding"))) {
		found = TRUE;
		act("$n look intune with the Source.",victim,NULL,NULL,TO_ROOM);
	}

	if (check_dispel(level,victim,sn_lookup("fear"))) {
		found = TRUE;
		act("Fear vanishes from $n's eyes.",victim,NULL,NULL,TO_ROOM);
	}

	if (check_dispel(level,victim,sn_lookup("protection heat"))) {
		found = TRUE;
		act("$n looks a bit warmer.",victim,NULL,NULL,TO_ROOM);
	}

	if (check_dispel(level,victim,sn_lookup("protection cold"))) {
		found = TRUE;
		act("$n gets a slight chill.",victim,NULL,NULL,TO_ROOM);
	}

	if (check_dispel(level,victim,sn_lookup("magic resistance"))) {
		found = TRUE;
		act("$n looks more succeptible to magic.",victim,NULL,NULL,TO_ROOM);
	}

	if (check_dispel(level,victim,sn_lookup("hallucination"))) {
		found = TRUE;
		act("$n looks less delusional.",victim,NULL,NULL,TO_ROOM);
	}

	if (check_dispel(level,victim,sn_lookup("terangreal")))
		found = TRUE;

	if (check_dispel(level,victim,sn_lookup("power word stun"))) {
		found = TRUE;
		act("$n is no longer stunned.",victim,NULL,NULL,TO_ROOM);
	}

	if (check_dispel(level,victim,sn_lookup("corruption")))
	{
		act("$n looks healthier.",victim,NULL,NULL,TO_ROOM);
		found = TRUE;
	}

	if (check_dispel(level+10,victim,sn_lookup("web")))
	{
		act("The webs around $n dissolves.",victim,NULL,NULL,TO_ROOM);
		found = TRUE;
	}

	if (check_dispel(level,victim,sn_lookup("vampiric touch")))
	{
		act("$n is no longer troubled with nightmares.",victim,NULL,NULL,TO_ROOM);
		found = TRUE;
	}

	if (check_dispel(level,victim,sn_lookup("damnation")))
	{
		act("$n's skin takes on a healthier hue.",victim,NULL,NULL,TO_ROOM);
		found = TRUE;
	}
	return found;
}
