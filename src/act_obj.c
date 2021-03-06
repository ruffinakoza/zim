/*
 * $Id: act_obj.c 1015 2007-02-07 01:53:29Z zsuzsu $
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
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "debug.h"
#include "quest.h"
#include "update.h"
#include "mob_prog.h"
#include "obj_prog.h"
#include "fight.h"
#include "db/lang.h"
#include "waffects.h"
#include "clan.h"
#include "stats.h"

DECLARE_DO_FUN(do_split		);
DECLARE_DO_FUN(do_say		);
DECLARE_DO_FUN(do_whisper	);
DECLARE_DO_FUN(do_scan		);
DECLARE_DO_FUN(do_mount		);
DECLARE_DO_FUN(do_yell		);

/*
 * Local functions.
 */
CHAR_DATA *	find_keeper	(CHAR_DATA * ch);
uint		get_cost	(CHAR_DATA *ch, CHAR_DATA * keeper, 
				OBJ_DATA * obj, bool fBuy);
void		obj_to_keeper	(OBJ_DATA * obj, CHAR_DATA * ch);
OBJ_DATA *	get_obj_keeper	(CHAR_DATA * ch, CHAR_DATA * keeper,
				 const char *argument);
void		sac_obj		(CHAR_DATA * ch, OBJ_DATA *obj);
bool		can_write_on	(CHAR_DATA *ch, OBJ_DATA *obj);
bool		can_npc_wear_obj(CHAR_DATA *ch, OBJ_DATA *obj);

/* RT part of the corpse looting code */
bool can_loot(CHAR_DATA * ch, OBJ_DATA * obj)
{
	if (IS_IMMORTAL(ch))
		return TRUE;

	/*
	 * PC corpses in the ROOM_BATTLE_ARENA rooms can be looted
	 * only by owners
	 */
	if (obj->in_room
	&&  (IS_SET(obj->in_room->room_flags, ROOM_BATTLE_ARENA)
	     || is_affected_world(WAFF_ARENA))
	&&  !IS_OWNER(ch, obj))
		return FALSE;

	/*
	 * Newbies can't loot anyone but their own corpses
	 */
	if (IS_NEWBIE(ch)
	&& !IS_OWNER(ch, obj))
		return FALSE;

	/*
	 * characters under 20 can only loot in PK range
	 */
	if (ch->level < 20
	&& (obj->level < ch->level -3
	|| obj->level > ch->level +3))
		return FALSE;

	return TRUE;
}

bool get_obj(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * container)
{
	/* variables for AUTOSPLIT */
	CHAR_DATA      *gch;
	int             members;

	/* variables for getting money */
	OBJ_DATA       *obj_neo;
	int       	silvertaken = 0;
	int       	goldtaken = 0;
	int       	silveroverweight = 0;
	int       	goldoverweight = 0;
	int       	weightovermax = 0;
	int		diff = 0, mod = 100;


	if (!CAN_WEAR(obj, ITEM_TAKE)
	||  ((obj->pIndexData->item_type == ITEM_CORPSE_PC ||
	      obj->pIndexData->item_type == ITEM_CORPSE_NPC) &&
	     (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)) &&
	     !IS_IMMORTAL(ch))
	|| (obj->pIndexData->item_type == ITEM_PARCHMENT
	    && (IS_SET(obj->value[ITEM_PARCHMENT_FLAGS], WRITE_BULLETIN) && 
		!IS_IMMORTAL(ch)))) {
		char_puts("You can't take that.\n", ch);
		return FALSE;
	}

	if (obj->pIndexData->vnum == QUEST_VNUM_RUG
	&& IS_NEWBIE(ch)) {
		char_puts("Sorry, because you are a newbie you can't handle that item.", ch);
		return FALSE;
	}


	/* can't even get limited eq which does not match alignment */
	if (obj->pIndexData->limit != -1) {
		if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch))
		||  (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch))
		||  (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))) {
			act_puts("You are zapped by $p and drop it.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n is zapped by $p and drops it.",
			    ch, obj, NULL, TO_ROOM);
			return FALSE;
		}
		if ((IS_NPC(ch) && ch->master && IS_NEWBIE(ch->master))
		|| IS_NEWBIE(ch)) {
			act_puts("You are zapped by $p and drop it.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n is zapped by $p and drops it.",
			    ch, obj, NULL, TO_ROOM);
			return FALSE;
		}
	}

        if (!IS_IMMORTAL(ch)
	&& ch->carry_number + get_obj_number(obj) > can_carry_n(ch) &&
                obj->pIndexData->item_type != ITEM_MONEY) {
                act_puts("$d: you can't carry that many items.",
                         ch, NULL, obj->name, TO_CHAR, POS_DEAD);
                return FALSE;
        }

        if (!IS_IMMORTAL(ch)
	&& ch_weight_carried(ch) + get_obj_weight(obj) > ch_max_carry_weight(ch) &&
                obj->pIndexData->item_type != ITEM_MONEY) {
                act_puts("$d: you can't carry that much weight.",
                         ch, NULL, obj->name, TO_CHAR, POS_DEAD);
                return FALSE;
        }

	if (obj->in_room != NULL) {
		for (gch = obj->in_room->people; gch != NULL;
		     gch = gch->next_in_room)
			if (gch->on == obj) {
				act_puts("$N appears to be using $p.",
					 ch, obj, gch, TO_CHAR, POS_DEAD);
				return FALSE;
			}
	}

	/* get an object from a container*/
	if (container) {
		if (IS_SET(container->pIndexData->extra_flags, ITEM_PIT)
		&&  !IS_OBJ_STAT(obj, ITEM_HAD_TIMER))
			obj->timer = 0;

		act_puts("You get $p from $P.",
			 ch, obj, container, TO_CHAR, POS_DEAD);
		act("$n gets $p from $P.", ch, obj, container,
		    TO_ROOM | (IS_AFFECTED(ch, AFF_SNEAK) ? ACT_NOMORTAL : 0));

		REMOVE_BIT(obj->extra_flags, ITEM_HAD_TIMER);
		obj_from_obj(obj);

		/* clan item tranferance */
		if (IS_SET(obj->extra_flags, ITEM_CLAN)) {
			if (!clan_item_loot(ch, obj, container)) {
				act_puts("You put $p back in $P.",
					 ch, obj, container, TO_CHAR, POS_DEAD);
				act("$n puts $p back in $P.", ch, obj, container,
				    TO_ROOM | (IS_AFFECTED(ch, AFF_SNEAK) ? ACT_NOMORTAL : 0));
				obj_to_obj(obj, container);
				return FALSE;
			}
		}
	}
	else {
		act_puts("You get $p.", ch, obj, container, TO_CHAR, POS_DEAD);
		act("$n gets $p.", ch, obj, container,
		    TO_ROOM | (IS_AFFECTED(ch, AFF_SNEAK) ? ACT_NOMORTAL : 0));
		obj_from_room(obj);
	}

        if (obj->pIndexData->item_type == ITEM_MONEY) {

		/* diminish money for upper level characters preying on
		 * easy mobs
		 */
		if (DIMINISH_MONEY_LOOT_RATE
		&& container
		&& container->pIndexData->item_type == ITEM_CORPSE_NPC) {
			diff = ch->level - (container->level+3);
			if (diff > 0 && ch->level > 15)
				mod -= diff * DIMINISH_MONEY_LOOT_RATE;
			mod = UMAX(10, mod);
                	obj->value[ITEM_MONEY_SILVER] = obj->value[ITEM_MONEY_SILVER] * mod / 100;
                	obj->value[ITEM_MONEY_GOLD] =  obj->value[ITEM_MONEY_GOLD] * mod / 100;

		}
                ch->silver += obj->value[ITEM_MONEY_SILVER];
                ch->gold += obj->value[ITEM_MONEY_GOLD];
                silvertaken = obj->value[ITEM_MONEY_SILVER];
                goldtaken = obj->value[ITEM_MONEY_GOLD];

                if (ch_weight_carried(ch) > ch_max_carry_weight(ch)) {
                        act("$n couldn't carry all of their coins, and they drop some.", ch, obj, NULL,
                                TO_ROOM | (IS_AFFECTED(ch, AFF_SNEAK) ? ACT_NOMORTAL : 0));
                        act("You couldn't carry all of your coins, and you drop some.", ch, obj, NULL, TO_CHAR);

                        weightovermax = ch_weight_carried(ch) - ch_max_carry_weight(ch);

                        if (ch->silver / 10 > weightovermax) {
                                silveroverweight = weightovermax * 10;
                                weightovermax = 0;
                        } else {
                                silveroverweight = ch->silver;
                                weightovermax -= silveroverweight / 10;
                        }
                        if (ch->gold * 2 / 5 > weightovermax) {
                                goldoverweight = weightovermax * 5 / 2;
                                weightovermax = 0;
                        } else {
                                goldoverweight = ch->gold;
                                weightovermax = 0;
                        }
                        ch->silver -= silveroverweight;
                        ch->gold -= goldoverweight;

                        silvertaken -= silveroverweight;
                        goldtaken -= goldoverweight;

                        if(silvertaken < 0) {
                                silvertaken = 0;
                        }
                        if(goldtaken < 0) {
                                goldtaken = 0;
                        }

                        obj_neo = create_money(goldoverweight, silveroverweight);
                        obj_to_room(obj_neo, ch->in_room);
                }
		if (goldtaken > WIZNET_ECONOMY_DEPOSIT_GOLD_MIN
		|| silvertaken > WIZNET_ECONOMY_DEPOSIT_SILVER_MIN)
			wiznet2(WIZ_ECONOMY, IM, 0, ch,
				"$N {gpicked up{x {Y%d{x gold, %d silver.", 
				goldtaken, silvertaken);

                if (IS_SET(ch->conf_flags, PLR_CONF_AUTOSPLIT)) {
                        // AUTOSPLIT code //
                        members = 0;
                        for (gch = ch->in_room->people; gch != NULL;
                             gch = gch->next_in_room) {
                                if (!IS_AFFECTED(gch, AFF_CHARM)
                                    && is_same_group(gch, ch))
                                        members++;
                        }
                        if (members > 1 && (goldtaken > 1
                                            || silvertaken > 1)) {
                                doprintf(do_split, ch, "%d %d", silvertaken,
                                         goldtaken);
                	}
		}
                extract_obj(obj, 0);
        }
	else {
		obj_to_char(obj, ch);
		if (!IS_NPC(ch)) {
			int pa = -1;
			if (!IS_IMMORTAL(ch) && (pa = past_owner_alt(obj, ch)) != -1) {
				char str[MAX_STRING_LENGTH];
				snprintf(str, sizeof(str),
					"[%d] $N possible {Yaltdrop{x from %s[%d ago] of $p.",
					ch->in_room->vnum,
					obj->past_owner[pa],
					pa+1);

				wiznet(str,
					ch, obj, WIZ_ALTDROP, 0, ch->level);
			}
			add_past_owner(obj, ch);
		}
		oprog_call(OPROG_GET, obj, ch, NULL);
	}
	return TRUE;
}

void do_get(CHAR_DATA * ch, const char *argument)
{
	char            arg1[MAX_INPUT_LENGTH];
	char            arg2[MAX_INPUT_LENGTH];
	OBJ_DATA       *obj, *obj_next;
	OBJ_DATA       *container;
	bool            found;
	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (!str_cmp(arg2, "from"))
		argument = one_argument(argument, arg2, sizeof(arg2));

	/* Get type. */
	if (arg1[0] == '\0') {
		char_puts("Get what?\n", ch);
		return;
	}
	if (arg2[0] == '\0') {
		if (str_cmp(arg1, "all") && str_prefix("all.", arg1)) {
			/* 'get obj' */
			obj = get_obj_list(ch, arg1, ch->in_room->contents);
			if (obj == NULL) {
				act_puts("I see no $T here.",
					 ch, NULL, arg1, TO_CHAR, POS_DEAD);
				return;
			}

			get_obj(ch, obj, NULL);
		} else {
			/* 'get all' or 'get all.obj' */
			found = FALSE;
			for (obj = ch->in_room->contents; obj != NULL;
			     obj = obj_next) {
				obj_next = obj->next_content;
				if ((arg1[3] == '\0' || is_name(arg1+4, obj->name))
				&& can_see_obj(ch, obj)) {
					found = TRUE;
					get_obj(ch, obj, NULL);
				}
			}

			if (!found) {
				if (arg1[3] == '\0')
					char_puts("I see nothing here.\n", ch);
				else
					act_puts("I see no $T here.",
						 ch, NULL, arg1+4, TO_CHAR,
						 POS_DEAD);
			}
		}
		return;
	}
	/* 'get ... container' */
	if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2)) {
		char_puts("You can't do that.\n", ch);
		return;
	}
	if ((container = get_obj_here(ch, arg2)) == NULL) {
		act_puts("I see no $T here.",
			 ch, NULL, arg2, TO_CHAR, POS_DEAD);
		return;
	}

	switch (container->pIndexData->item_type) {
	default:
		char_puts("That is not a container.\n", ch);
		return;

	case ITEM_CONTAINER:
	case ITEM_CORPSE_NPC:
		break;

	case ITEM_CORPSE_PC:
		if (!can_loot(ch, container)) {
			char_puts("You can't do that.\n", ch);
			return;
		}
	}

	if (IS_SET(container->value[ITEM_CONTAINER_FLAGS], CONT_CLOSED)
	    && (!ch->clan 
	       || clan_lookup(ch->clan)->altar_ptr!=container)) {
		act_puts("The $d is closed.",
			 ch, NULL, container->name, TO_CHAR, POS_DEAD);
		return;
	}

	if (str_cmp(arg1, "all") && str_prefix("all.", arg1)) {
		/* 'get obj container' */
		obj = get_obj_list(ch, arg1, container->contains);


		if (obj == NULL) {
			act_puts("I see nothing like that in the $T.",
				 ch, NULL, arg2, TO_CHAR, POS_DEAD);
			return;
		}

		if (!IS_IMMORTAL(ch)
		&& (container->pIndexData->item_type == ITEM_CORPSE_PC
		|| container->pIndexData->item_type == ITEM_CONTAINER)
		&& !mlstr_null(container->owner)
		&& !IS_OWNER(ch, container))
			WAIT_STATE(ch, PULSE_VIOLENCE / 3);

		get_obj(ch, obj, container);
	} else {
		/* 'get all container' or 'get all.obj container' */
		found = FALSE;
		for (obj = container->contains; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			if ((arg1[3] == '\0' || is_name(&arg1[4], obj->name))
			    && can_see_obj(ch, obj)) {
				found = TRUE;
				if (((IS_SET(container->pIndexData->extra_flags, ITEM_PIT)
				&& container->in_room != get_altar(ch)->room)
				|| ((container->pIndexData->item_type == ITEM_CORPSE_PC ||
				   container->pIndexData->item_type == ITEM_CONTAINER)
					&& !mlstr_null(container->owner)
					&& !IS_OWNER(ch, container)))
				&&  !IS_IMMORTAL(ch)) {
					act_puts("Don't be so greedy!",
						 ch, NULL, NULL, TO_CHAR,
						 POS_DEAD);
					return;
				}
				if (!get_obj(ch, obj, container))
					break;
			}
		}

		if (!found) {
			if (arg1[3] == '\0')
				act_puts("I see nothing in the $T.",
					 ch, NULL, arg2, TO_CHAR, POS_DEAD);
			else
				act_puts("I see nothing like that in the $T.",
					 ch, NULL, arg2, TO_CHAR, POS_DEAD);
		}
	}
}

bool put_obj(CHAR_DATA *ch, OBJ_DATA *container, OBJ_DATA *obj, int* count)
{
	OBJ_DATA *	objc;
	bool okay = FALSE;

	if (IS_SET(container->value[ITEM_CONTAINER_FLAGS], CONT_QUIVER)
	&&  (obj->pIndexData->item_type != ITEM_WEAPON ||
	     obj->value[ITEM_WEAPON_TYPE] != WEAPON_ARROW)) {
		act_puts("You can only put arrows in $p.",
			 ch, container, NULL, TO_CHAR, POS_DEAD);
		return FALSE;
	}

	if (IS_SET(container->pIndexData->extra_flags, ITEM_PIT)
	&&  !CAN_WEAR(obj, ITEM_TAKE)) {
		if (obj->timer)
			SET_BIT(obj->extra_flags, ITEM_HAD_TIMER);
		else
			obj->timer = number_range(100, 200);
	}

	if (obj->pIndexData->limit != -1
	||  IS_SET(obj->pIndexData->extra_flags, ITEM_QUEST)) {
		act_puts("This unworthy container won't hold $p.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		return TRUE;
	}

	/* clan item tranferance */
	if (IS_SET(obj->pIndexData->extra_flags, ITEM_CLAN)) {
		if (!clan_item_raid(ch, obj, container)) {
			act_puts("Only an altar will hold the power of $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			return TRUE;
		}
	}

	okay = FALSE;
	switch (obj->pIndexData->item_type) {
		case ITEM_POTION:
			okay = IS_CONTAINER_TYPE(container->pIndexData,
				CONT_POTION);
			break;
		case ITEM_SCROLL:
			okay = IS_CONTAINER_TYPE(container->pIndexData,
				CONT_SCROLL);
			break;
		case ITEM_PILL:
			okay = IS_CONTAINER_TYPE(container->pIndexData,
				CONT_PILL);
			break;
		case ITEM_WAND:
			okay = IS_CONTAINER_TYPE(container->pIndexData,
				CONT_WAND);
			break;
		case ITEM_STAFF:
			okay = IS_CONTAINER_TYPE(container->pIndexData,
				CONT_STAFF);
			break;
		default:
			okay = !IS_CONTAINER_TYPE(container->pIndexData,
				CONT_POTION)
				&& !IS_CONTAINER_TYPE(
				   container->pIndexData, CONT_SCROLL)
				&& !IS_CONTAINER_TYPE(
				   container->pIndexData, CONT_PILL)
				&& !IS_CONTAINER_TYPE(
				   container->pIndexData, CONT_WAND)
				&& !IS_CONTAINER_TYPE(
				   container->pIndexData, CONT_STAFF);
	}

	if (!okay) {
		act("Find a more suitable container for $p.",
			ch, obj, NULL, TO_CHAR);
		return FALSE;
	}

	if (obj->pIndexData->item_type == ITEM_POTION
	&&  IS_SET(container->wear_flags, ITEM_TAKE)) {
		int pcount = 0;
		for (objc = container->contains; objc; objc = objc->next_content)
			if (objc->pIndexData->item_type == ITEM_POTION)
				pcount++;
		if (pcount >= CONT_MAX_POTION) {
			act_puts("It's not safe to put more potions into $p.",
				 ch, container, NULL, TO_CHAR, POS_DEAD);
			return FALSE;
		}
	}

	if (obj->pIndexData->item_type == ITEM_PILL
	&&  IS_SET(container->wear_flags, ITEM_TAKE)) {
		int pcount = 0;
		for (objc = container->contains; objc; objc = objc->next_content)
			if (objc->pIndexData->item_type == ITEM_PILL)
				pcount++;
		if (pcount >= CONT_MAX_PILL) {
			act_puts("It's not safe to put more pills into $p.",
				 ch, container, NULL, TO_CHAR, POS_DEAD);
			return FALSE;
		}
	}

	if (obj->pIndexData->item_type == ITEM_SCROLL
	&&  IS_SET(container->wear_flags, ITEM_TAKE)) {
		int pcount = 0;
		for (objc = container->contains; objc; objc = objc->next_content)
			if (objc->pIndexData->item_type == ITEM_SCROLL)
				pcount++;
		if (pcount >= CONT_MAX_SCROLL) {
			act_puts("It's not safe to put more scrolls into $p.",
				 ch, container, NULL, TO_CHAR, POS_DEAD);
			return FALSE;
		}
	}

	if (obj->pIndexData->item_type == ITEM_WAND
	&&  IS_SET(container->wear_flags, ITEM_TAKE)) {
		int pcount = 0;
		for (objc = container->contains; objc; objc = objc->next_content)
			if (objc->pIndexData->item_type == ITEM_WAND)
				pcount++;
		if (pcount >= CONT_MAX_WAND) {
			act_puts("It's not safe to put more wands into $p.",
				 ch, container, NULL, TO_CHAR, POS_DEAD);
			return FALSE;
		}
	}

	if (obj->pIndexData->item_type == ITEM_STAFF
	&&  IS_SET(container->wear_flags, ITEM_TAKE)) {
		int pcount = 0;
		for (objc = container->contains; objc; objc = objc->next_content)
			if (objc->pIndexData->item_type == ITEM_STAFF)
				pcount++;
		if (pcount >= CONT_MAX_STAFF) {
			act_puts("It's not safe to put more staves into $p.",
				 ch, container, NULL, TO_CHAR, POS_DEAD);
			return FALSE;
		}
	}

	(*count)++;
	if (*count > container->value[ITEM_CONTAINER_WEIGHT]) {
		act_puts("It's not safe to put that much items into $p.",
			 ch, container, NULL, TO_CHAR, POS_DEAD);
		return FALSE;
	}

	obj_from_char(obj);
	obj_to_obj(obj, container);

	if (IS_SET(container->value[ITEM_CONTAINER_FLAGS], CONT_PUT_ON)) {
		act("$n puts $p on $P.", ch, obj, container, TO_ROOM);
		act_puts("You put $p on $P.",
			 ch, obj, container, TO_CHAR, POS_DEAD);
	}
	else {
		act("$n puts $p in $P.", ch, obj, container, TO_ROOM);
		act_puts("You put $p in $P.",
			 ch, obj, container, TO_CHAR, POS_DEAD);
	}

	return TRUE;
}

void do_put(CHAR_DATA * ch, const char *argument)
{
	char		arg1[MAX_INPUT_LENGTH];
	char		arg2[MAX_INPUT_LENGTH];
	OBJ_DATA *	container;
	OBJ_DATA *	obj;
	OBJ_DATA *	obj_next;
	OBJ_DATA *	objc;
	int		count;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (!str_cmp(arg2, "in") || !str_cmp(arg2, "on"))
		argument = one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0' || arg2[0] == '\0') {
		char_puts("Put what in what?\n", ch);
		return;
	}

	if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2)) {
		char_puts("You can't do that.\n", ch);
		return;
	}

	if ((container = get_obj_here(ch, arg2)) == NULL) {
		act_puts("I see no $T here.",
			 ch, NULL, arg2, TO_CHAR, POS_DEAD);
		return;
	}

	if (container->pIndexData->item_type != ITEM_CONTAINER) {
		char_puts("That is not a container.\n", ch);
		return;
	}

	if (IS_SET(container->value[ITEM_CONTAINER_FLAGS], CONT_CLOSED) 
	    && (!ch->clan 
	       || clan_lookup(ch->clan)->altar_ptr!=container)) {
		act_puts("The $d is closed.",
			 ch, NULL, container->name, TO_CHAR, POS_DEAD);
		return;
	}

	if (str_cmp(arg1, "all") && str_prefix("all.", arg1)) {
		/* 'put obj container' */
		if ((obj = get_obj_carry(ch, arg1)) == NULL) {
			char_puts("You do not have that item.\n", ch);
			return;
		}

		if (obj == container) {
			char_puts("You can't fold it into itself.\n", ch);
			return;
		}

		if (!can_drop_obj(ch, obj)) {
			char_puts("You can't let go of it.\n", ch);
			return;
		}

		if (WEIGHT_MULT(obj) != 100
		|| IS_SET(obj->extra_flags, ITEM_NOBAG)
		|| (obj->pIndexData->item_type == ITEM_CONTAINER
		   && (IS_CONTAINER_TYPE(obj->pIndexData, CONT_POTION)
		      || IS_CONTAINER_TYPE(obj->pIndexData, CONT_SCROLL)
		      || IS_CONTAINER_TYPE(obj->pIndexData, CONT_PILL)
		      || IS_CONTAINER_TYPE(obj->pIndexData, CONT_WAND)
		      || IS_CONTAINER_TYPE(obj->pIndexData, CONT_STAFF))))
		{
			char_puts("You have a feeling that would be a bad idea.\n", ch);
			return;
		}


		if (obj->pIndexData->limit != -1) {
			act_puts("This unworthy container won't hold $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			return;
		}

		if (get_obj_weight(obj) + get_true_weight(container) >
		    (container->value[ITEM_CONTAINER_WEIGHT] * 10)
		||  get_obj_weight(obj) > (container->value[ITEM_CONTAINER_PER_ITEM_WEIGHT] * 10)) {
			char_puts("It won't fit.\n", ch);
			return;
		}

		count = 0;
		for (objc = container->contains; objc; objc = objc->next_content)
			count++;

		put_obj(ch, container, obj, &count);
	}
	else {
		if (!IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)) {
			do_say(ch, "Nah, I won't do that.");
			return;
		}

		count = 0;
		for (objc = container->contains; objc; objc = objc->next_content)
			count++;

		/* 'put all container' or 'put all.obj container' */
		for (obj = ch->carrying; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;

			if ((arg1[3] != '\0' && !is_name(&arg1[4], obj->name))
			||  container == obj
			||  !can_see_obj(ch, obj)
			||  obj->wear_loc != WEAR_NONE
			||  !can_drop_obj(ch, obj)
			||  get_obj_weight(obj) + get_true_weight(container) > 
			    (container->value[ITEM_CONTAINER_WEIGHT] * 10)
			||  get_obj_weight(obj) >= (container->value[ITEM_CONTAINER_PER_ITEM_WEIGHT] * 10))
				continue;

			if (WEIGHT_MULT(obj) != 100
			|| IS_SET(obj->extra_flags, ITEM_NOBAG)
			|| (obj->pIndexData->item_type == ITEM_CONTAINER
			   && (IS_CONTAINER_TYPE(obj->pIndexData, CONT_POTION)
			      || IS_CONTAINER_TYPE(obj->pIndexData, CONT_SCROLL)
			      || IS_CONTAINER_TYPE(obj->pIndexData, CONT_PILL)
			      || IS_CONTAINER_TYPE(obj->pIndexData, CONT_WAND)
			      || IS_CONTAINER_TYPE(obj->pIndexData, CONT_STAFF))))
				continue;

			if (!put_obj(ch, container, obj, &count))
				continue;
		}
	}
}

void drop_obj(CHAR_DATA *ch, OBJ_DATA *obj)
{
	obj_from_char(obj);
	obj_to_room(obj, ch->in_room);

	act("$n drops $p.", ch, obj, NULL,
	    TO_ROOM | (IS_AFFECTED(ch, AFF_SNEAK) ? ACT_NOMORTAL : 0));
	act_puts("You drop $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);

	if (obj->pIndexData->vnum == OBJ_VNUM_POTION_VIAL
	&&  number_percent() < 51)
		switch (ch->in_room->sector_type) {
		case SECT_FOREST:
		case SECT_DESERT:
		case SECT_AIR:
		case SECT_WATER_NOSWIM:
		case SECT_WATER_SWIM:
		case SECT_FIELD:
			break;
		default:
			act("$p cracks and shaters into tiny pieces.",
			    ch, obj, NULL, TO_ROOM);
			act("$p cracks and shaters into tiny pieces.",
			    ch, obj, NULL, TO_CHAR);
			extract_obj(obj, 0);
			return;
		}

	oprog_call(OPROG_DROP, obj, ch, NULL);

	if (!may_float(obj) && cant_float(obj) && IS_WATER(ch->in_room)) {
		act("$p sinks down the water.", ch, obj, NULL,
		    TO_ROOM | (IS_AFFECTED(ch, AFF_SNEAK) ? ACT_NOMORTAL : 0));
		act("$p sinks down the water.", ch, obj, NULL, TO_CHAR);
		extract_obj(obj, 0);
	}
	else if (IS_OBJ_STAT(obj, ITEM_MELT_DROP)) {
		act("$p dissolves into smoke.", ch, obj, NULL,
		    TO_ROOM | (IS_AFFECTED(ch, AFF_SNEAK) ? ACT_NOMORTAL : 0));
		act("$p dissolves into smoke.", ch, obj, NULL, TO_CHAR);
		extract_obj(obj, 0);
	}
}

void do_drop(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];
	OBJ_DATA       *obj;
	OBJ_DATA       *obj_next;
	bool            found;

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Drop what?\n", ch);
		return;
	}
	if (is_number(arg)) {
		/* 'drop NNNN coins' */
		int             amount, gold = 0, silver = 0;
		amount = atoi(arg);
		argument = one_argument(argument, arg, sizeof(arg));
		if (amount <= 0
		    || (str_cmp(arg, "coins") && str_cmp(arg, "coin")
			&& str_cmp(arg, "gold") && str_cmp(arg, "silver"))) {
			char_puts("You can't do that.\n", ch);
			return;
		}
		if (!str_cmp(arg, "coins") || !str_cmp(arg, "coin")
		    || !str_cmp(arg, "silver")) {
			if (ch->silver < amount) {
				char_puts("You don't have that much silver.\n", ch);
				return;
			}
			ch->silver -= amount;
			silver = amount;
			if (amount > WIZNET_ECONOMY_DEPOSIT_SILVER_MIN)
				wiznet2(WIZ_ECONOMY, IM, 0, ch,
					"$N {gdropped{x %d{x silver.", amount);
		} else {
			if (ch->gold < amount) {
				char_puts("You don't have that much gold.\n", ch);
				return;
			}
			ch->gold -= amount;
			gold = amount;
			if (amount > WIZNET_ECONOMY_DEPOSIT_GOLD_MIN)
				wiznet2(WIZ_ECONOMY, IM, 0, ch,
					"$N {gdropped{x {Y%d{x gold.", amount);
		}

		for (obj = ch->in_room->contents; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;

			switch (obj->pIndexData->vnum) {
			case OBJ_VNUM_SILVER_ONE:
			case OBJ_VNUM_GOLD_ONE:
			case OBJ_VNUM_COINS:
			case OBJ_VNUM_GOLD_SOME:
			case OBJ_VNUM_SILVER_SOME:
				silver += obj->value[ITEM_MONEY_SILVER];
				gold += obj->value[ITEM_MONEY_GOLD];
				extract_obj(obj, 0);
				break;
			}
		}

		obj = create_money(gold, silver);
		obj_to_room(obj, ch->in_room);
		act("$n drops some coins.", ch, NULL, NULL,
		    TO_ROOM | (IS_AFFECTED(ch, AFF_SNEAK) ? ACT_NOMORTAL : 0));
		char_puts("Ok.\n", ch);
		if (IS_WATER(ch->in_room)) {
			extract_obj(obj, 0);
			act("The coins sink down, and disapear in the water.",
			    ch, NULL, NULL,
			    TO_ROOM | (IS_AFFECTED(ch, AFF_SNEAK) ? ACT_NOMORTAL : 0));
			act_puts("The coins sink down, and disapear in the water.",
				 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		}
		return;
	}
	if (str_cmp(arg, "all") && str_prefix("all.", arg)) {
		/* 'drop obj' */
		if ((obj = get_obj_carry(ch, arg)) == NULL) {
			char_puts("You do not have that item.\n", ch);
			return;
		}
		if (!can_drop_obj(ch, obj)) {
			char_puts("You can't let go of it.\n", ch);
			return;
		}
		drop_obj(ch, obj);
	}
	else {
/* 'drop all' or 'drop all.obj' */

		if (!IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)) {
			do_say(ch, "Nah, I won't do that.");
			return;
		}

		found = FALSE;
		for (obj = ch->carrying; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;

			if ((arg[3] == '\0' || is_name(&arg[4], obj->name))
			&&  can_see_obj(ch, obj)
			&&  obj->wear_loc == WEAR_NONE
			&&  can_drop_obj(ch, obj)) {
				found = TRUE;
				drop_obj(ch, obj);
			}
		}

		if (!found) {
			if (arg[3] == '\0')
				act_puts("You are not carrying anything.",
					 ch, NULL, arg, TO_CHAR, POS_DEAD);
			else
				act_puts("You are not carrying any $T.",
					 ch, NULL, &arg[4], TO_CHAR, POS_DEAD);
		}
	}
}

void do_give(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];
	CHAR_DATA      *victim;
	OBJ_DATA       *obj;

	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Give what to whom?\n", ch);
		return;
	}

	if (is_number(arg)) {
		/* 'give NNNN coins victim' */
		int             amount;
		bool            silver;

		amount = atoi(arg);

		argument = one_argument(argument, arg, sizeof(arg));
		if (arg[0] == '\0') {
			do_give(ch, str_empty);
			return;
		}

		if (amount <= 0
		||  (str_cmp(arg, "coins") && str_cmp(arg, "coin") &&
		     str_cmp(arg, "gold") && str_cmp(arg, "silver"))) {
			char_puts("Sorry, you can't do that.\n", ch);
			return;
		}

		silver = str_cmp(arg, "gold");

		argument = one_argument(argument, arg, sizeof(arg));
		if (!str_cmp(arg, "to"))
			argument = one_argument(argument, arg, sizeof(arg));
		if (arg[0] == '\0') {
			do_give(ch, str_empty);
			return;
		}

		if ((victim = get_char_room(ch, arg)) == NULL) {
			char_puts("They aren't here.\n", ch);
			return;
		}

		if ((!silver && ch->gold < amount)
		||  (silver && ch->silver < amount)) {
			char_puts("You haven't got that much.\n", ch);
			return;
		}

		if (silver) {
			ch->silver -= amount;
			victim->silver += amount;
		}
		else {
			ch->gold -= amount;
			victim->gold += amount;
		}

		if (!silver && amount > WIZNET_ECONOMY_DEPOSIT_GOLD_MIN)
			wiznet2(WIZ_ECONOMY, IM, 0, ch,
				"$N {ggave{x %s {Y%d{x gold.", 
				CHAR_NAME(victim), amount);

		if (silver && amount > WIZNET_ECONOMY_DEPOSIT_SILVER_MIN)
			wiznet2(WIZ_ECONOMY, IM, 0, ch,
				"$N {ggave{x %s %d silver.", 
				CHAR_NAME(victim), amount);

		act_puts3("$n gives you $J $t.",
			  ch, silver ? "silver" : "gold", victim,
			  (const void*) amount,
			  TO_VICT | ACT_TRANS, POS_RESTING);
		act("$n gives $N some coins.", ch, NULL, victim, TO_NOTVICT);
		act_puts3("You give $N $J $t.",
			  ch, silver ? "silver" : "gold", victim,
			  (const void*) amount,
			  TO_CHAR | ACT_TRANS, POS_DEAD);

		if (IS_NPC(victim)) {
			/* bribe trigger */
			if (HAS_TRIGGER(victim, TRIG_BRIBE))
				mp_bribe_trigger(victim, ch,
					 silver ? amount : amount * 100);

			/* money changer */
			else if (IS_SET(victim->pIndexData->act, ACT_CHANGER)) {
				int             change;

				change = (silver ? 95 * amount / 100 / 100
					  : 95 * amount);

				if (!silver && change > victim->silver)
					victim->silver += change;

				if (silver && change > victim->gold)
					victim->gold += change;

				if (change < 1 && can_see(victim, ch)) {
					act("$n tells you '{GI'm sorry, you did not give me enough to change.{x'",
					    victim, NULL, ch, TO_VICT);
					ch->reply = victim;
					doprintf(do_give, victim, "%d %s %s",
					amount, silver ? "silver" : "gold", ch->name);
				}
				else if (can_see(victim, ch)) {
					doprintf(do_give, victim, "%d %s %s",
					change, silver ? "gold" : "silver", ch->name);
					if (silver)
						doprintf(do_give, victim, "%d silver %s",
							 (95 * amount / 100 - change * 100), ch->name);
					act("$n tells you '{GThank you, come again.{x'",
					    victim, NULL, ch, TO_VICT);
					ch->reply = victim;
				}
				/*
				 * sanity restraints for the changers
				 * so you can't change, and kill him to rack up dough
				 */
				if (victim->silver > 10000) {
					victim->silver = number_range(5000, 10000);
					}
				if (victim->gold > 100) {
					victim->gold = number_range(50, 100);
				}
			}
			else if (!IS_IMMORTAL(ch) 
			&& !(IS_AFFECTED(victim, AFF_CHARM))
			&& victim->gold > victim->pIndexData->wealth / 100) {
				if (IS_GOOD(victim)) {
					doprintf(do_say, victim,
						"Thank you, the children of the halfling orphanage appreciate your generosity.");
				}
				else if (IS_EVIL(victim)) {
					act("$n uses your money to place a hit on someone.", victim, NULL, ch, TO_VICT);
					act("$n nods at you knowingly.",
						victim, NULL, ch, TO_VICT);
					act("$n nods at $N knowingly.",
						victim, NULL, ch, TO_NOTVICT);
				}
				else {
					if (number_percent() < 50)
						act("$n puts in all on {rred{x.", 
							victim, NULL, ch, TO_VICT);
					else
						act("$n puts in all on {Bblack{x.", 
							victim, NULL, ch, TO_VICT);
				}
				victim->gold = victim->pIndexData->wealth / 1000;
				victim->silver = 0;
			}
		}
		return;
	}

	if ((obj = get_obj_carry(ch, arg)) == NULL) {
		char_puts("You do not have that item.\n", ch);
		return;
	}

	argument = one_argument(argument, arg, sizeof(arg));
	if (!str_cmp(arg, "to"))
		argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		do_give(ch, str_empty);
		return;
	}

	if (obj->wear_loc != WEAR_NONE) {
		char_puts("You must remove it first.\n", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (IS_NPC(victim) && victim->pIndexData->pShop != NULL
	&&  !HAS_TRIGGER(victim, TRIG_GIVE)) {
		do_tell_raw(victim, ch, "Sorry, you'll have to sell that.");
		return;
	}

	if (!can_drop_obj(ch, obj)) {
		char_puts("You can't let go of it.\n", ch);
		return;
	}

	if (!IS_IMMORTAL(victim)
	&& victim->carry_number + get_obj_number(obj) > can_carry_n(victim)) {
		act("$N has $S hands full.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (!IS_IMMORTAL(victim)
	&& ch_weight_carried(victim) + get_obj_weight(obj) > ch_max_carry_weight(victim)) {
		act("$N can't carry that much weight.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (IS_SET(obj->pIndexData->extra_flags, ITEM_QUEST)
	&&  !IS_IMMORTAL(ch) && !IS_IMMORTAL(victim)) {
		act_puts("Even you are not that silly to give $p to $N.",
			 ch, obj, victim, TO_CHAR, POS_DEAD);
		return;
	}

	if (IS_SET(obj->pIndexData->extra_flags, ITEM_CLAN)
	&&  !IS_IMMORTAL(ch) && IS_NPC(victim)) {
		act_puts("$N wants no part of your political struggles.",
			 ch, obj, victim, TO_CHAR, POS_DEAD);
		return;
	}

	if (!can_see_obj(victim, obj)) {
		act("$N can't see it.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (obj->pIndexData->vnum == QUEST_VNUM_RUG
	&& IS_NEWBIE(victim)) {
		char_puts("That person's soul can't handle the power of an artifact.", ch);
		return;
	}

	if (obj->pIndexData->limit != -1) {
		if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(victim))
		||  (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(victim))
		||  (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(victim))) {
			char_puts("Your victim's alignment doesn't match the objects align.", ch);
			return;
		}
		if ((IS_NPC(victim) && victim->master && IS_NEWBIE(victim->master))
		|| IS_NEWBIE(victim)) {
			char_puts("That person's soul can't handle the power of an artifact.", ch);
			return;
		}
	}

	obj_from_char(obj);
	obj_to_char(obj, victim);
	act("$n gives $p to $N.", ch, obj, victim, TO_NOTVICT | ACT_NOTRIG);
	act("$n gives you $p.", ch, obj, victim, TO_VICT | ACT_NOTRIG);
	act("You give $p to $N.", ch, obj, victim, TO_CHAR | ACT_NOTRIG);

	/*
	 * Give trigger
	*/
	if (IS_NPC(victim) && HAS_TRIGGER(victim, TRIG_GIVE))
		mp_give_trigger(victim, ch, obj);

	oprog_call(OPROG_GIVE, obj, ch, victim);
}

/* for poisoning weapons and food/drink */
void do_envenom(CHAR_DATA * ch, const char *argument)
{
	OBJ_DATA       *obj;
	AFFECT_DATA     af;
	int		percent, chance;
	int		sn;

	/* find out what */
	if (argument == '\0') {
		char_puts("Envenom what item?\n", ch);
		return;
	}
	obj = get_obj_list(ch, argument, ch->carrying);

	if (obj == NULL) {
		char_puts("You don't have that item.\n", ch);
		return;
	}

	if ((sn = sn_lookup("envenom")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("Are you crazy? You'd poison yourself!\n", ch);
		return;
	}

	if (obj->pIndexData->item_type == ITEM_FOOD 
	|| obj->pIndexData->item_type == ITEM_DRINK_CON) {
		if (IS_OBJ_STAT(obj, ITEM_BLESS)
		||  IS_OBJ_STAT(obj, ITEM_BURN_PROOF)) {
			act("You fail to poison $p.", ch, obj, NULL, TO_CHAR);
			return;
		}

		WAIT_STATE(ch, SKILL(sn)->beats);
		if (number_percent() < chance) {	/* success! */
			act("$n treats $p with deadly poison.",
			    ch, obj, NULL, TO_ROOM);
			act("You treat $p with deadly poison.",
			    ch, obj, NULL, TO_CHAR);
			if (!obj->value[ITEM_FOOD_POISON]) {
				obj->value[ITEM_FOOD_POISON] = 1;
				check_improve(ch, sn, TRUE, 4);
			}
			return;
		}
		act("You fail to poison $p.", ch, obj, NULL, TO_CHAR);
		if (!obj->value[ITEM_FOOD_POISON])
			check_improve(ch, sn, FALSE, 4);
		return;
	}
	else if (obj->pIndexData->item_type == ITEM_WEAPON) {
		if (IS_WEAPON_STAT(obj, WEAPON_FLAMING)
		||  IS_WEAPON_STAT(obj, WEAPON_FROST)
		||  IS_WEAPON_STAT(obj, WEAPON_VAMPIRIC)
		||  IS_WEAPON_STAT(obj, WEAPON_SHARP)
		||  IS_WEAPON_STAT(obj, WEAPON_VORPAL)
		||  IS_WEAPON_STAT(obj, WEAPON_SHOCKING)) chance/=2;

		if (IS_WEAPON_STAT(obj, WEAPON_HOLY)
		||  IS_OBJ_STAT(obj, ITEM_BLESS)
		||  IS_OBJ_STAT(obj, ITEM_BURN_PROOF)) {
			act("You can't seem to envenom $p.",
			    ch, obj, NULL, TO_CHAR);
			return;
		}
		if (obj->value[ITEM_WEAPON_ATTACK_TYPE] < 0
		||  attack_table[obj->value[ITEM_WEAPON_ATTACK_TYPE]].damage == DAM_BASH) {
			char_puts("You can only envenom edged weapons.\n",
				  ch);
			return;
		}
		if (IS_WEAPON_STAT(obj, WEAPON_POISON)) {
			act("$p is already envenomed.", ch, obj, NULL, TO_CHAR);
			return;
		}

		WAIT_STATE(ch, SKILL(sn)->beats);
		percent = number_percent();
		if (percent < chance) {
			af.where = TO_WEAPON;
			af.type = gsn_poison;
			af.level = ch->level * percent / 100;
			af.duration = ch->level * percent / 100;
			af.location = 0;
			af.modifier = 0;
			af.bitvector = WEAPON_POISON;
			affect_to_obj(obj, &af);

			act("$n coats $p with deadly venom.", ch, obj, NULL,
			    TO_ROOM | (IS_AFFECTED(ch, AFF_SNEAK) ? ACT_NOMORTAL : 0));
			act("You coat $p with venom.", ch, obj, NULL, TO_CHAR);
			check_improve(ch, sn, TRUE, 3);
			return;
		}
		else {
			act("You fail to envenom $p.", ch, obj, NULL, TO_CHAR);
			check_improve(ch, sn, FALSE, 3);
			return;
		}
	}
	act("You can't poison $p.", ch, obj, NULL, TO_CHAR);
}

void do_fill(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];
	OBJ_DATA       *obj;
	OBJ_DATA       *fountain;
	bool            found;
	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Fill what?\n", ch);
		return;
	}
	if ((obj = get_obj_carry(ch, arg)) == NULL) {
		char_puts("You do not have that item.\n", ch);
		return;
	}
	found = FALSE;
	for (fountain = ch->in_room->contents; fountain != NULL;
	     fountain = fountain->next_content) {
		if (fountain->pIndexData->item_type == ITEM_FOUNTAIN) {
			found = TRUE;
			break;
		}
	}

	if (!found) {
		char_puts("There is no fountain here!\n", ch);
		return;
	}
	if (obj->pIndexData->item_type != ITEM_DRINK_CON) {
		char_puts("You can't fill that.\n", ch);
		return;
	}
	if (obj->value[ITEM_DRINK_REMAINING] != 0 
	&& obj->value[ITEM_DRINK_TYPE] != fountain->value[ITEM_DRINK_TYPE]) {
		char_puts("There is already another liquid in it.\n", ch);
		return;
	}
	if (obj->value[ITEM_DRINK_REMAINING] >= obj->value[ITEM_DRINK_TOTAL] 
	&& obj->value[ITEM_DRINK_TOTAL] >= 0) {
		char_puts("Your container is full.\n", ch);
		return;
	}

	act_puts3("You fill $p with $U from $P.",
		   ch, obj, fountain, 
		   liq_table[fountain->value[ITEM_DRINK_TYPE]].liq_name,
		   TO_CHAR | ACT_TRANS, POS_DEAD);

	act_puts3("$n fills $p with $U from $P.",
		   ch, obj, fountain, 
		   liq_table[fountain->value[ITEM_DRINK_TYPE]].liq_name,
		   TO_ROOM | ACT_TRANS, POS_RESTING);

	obj->value[ITEM_DRINK_TYPE] = fountain->value[ITEM_DRINK_TYPE];
	obj->value[ITEM_DRINK_REMAINING] = obj->value[ITEM_DRINK_TOTAL];

}

void do_pour(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_STRING_LENGTH];
	OBJ_DATA       *out, *in;
	CHAR_DATA      *vch = NULL;
	int             amount;
	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0' || argument[0] == '\0') {
		char_puts("Pour what into what?\n", ch);
		return;
	}
	if ((out = get_obj_carry(ch, arg)) == NULL) {
		char_puts("You don't have that item.\n", ch);
		return;
	}
	if (out->pIndexData->item_type != ITEM_DRINK_CON) {
		char_puts("That's not a drink container.\n", ch);
		return;
	}
	if (!str_cmp(argument, "out")) {
		if (out->value[ITEM_DRINK_REMAINING] == 0) {
			char_puts("It is empty.\n", ch);
			return;
		}
		out->value[ITEM_DRINK_REMAINING] = 0;
		out->value[ITEM_DRINK_POISON] = 0;
		act_puts3("You invert $p, spilling $T $U.",
			  ch, out, liq_table[out->value[ITEM_DRINK_TYPE]].liq_name,
			  IS_WATER(ch->in_room) ? "in to the water" :
						  "all over the ground",
			  TO_CHAR | ACT_TRANS, POS_DEAD);
		act_puts3("$n inverts $p, spilling $T $U.",
			  ch, out, liq_table[out->value[ITEM_DRINK_TYPE]].liq_name,
			  IS_WATER(ch->in_room) ? "in to the water" :
						  "all over the ground",
			  TO_ROOM | ACT_TRANS, POS_RESTING);
		return;
	}
	if ((in = get_obj_here(ch, argument)) == NULL) {
		vch = get_char_room(ch, argument);

		if (vch == NULL) {
			char_puts("Pour into what?\n", ch);
			return;
		}
		in = get_eq_char(vch, WEAR_HOLD);

		if (in == NULL) {
			char_puts("They aren't holding anything.", ch);
			return;
		}
	}
	if (in->pIndexData->item_type != ITEM_DRINK_CON) {
		char_puts("You can only pour into other drink containers.\n", ch);
		return;
	}
	if (in == out) {
		char_puts("You cannot change the laws of physics!\n", ch);
		return;
	}
	if (in->value[ITEM_DRINK_REMAINING] != 0 
	&& in->value[ITEM_DRINK_TYPE] != out->value[ITEM_DRINK_TYPE]) {
		char_puts("They don't hold the same liquid.\n", ch);
		return;
	}
	if (out->value[ITEM_DRINK_REMAINING] == 0) {
		act("There's nothing in $p to pour.", ch, out, NULL, TO_CHAR);
		return;
	}
	if (in->value[ITEM_DRINK_TOTAL] < 0) {
		act("You cannot fill $p.", ch, in, NULL, TO_CHAR);
		return;
	}
	if (in->value[ITEM_DRINK_REMAINING] >= in->value[ITEM_DRINK_TOTAL]) {
		act("$p is already filled to the top.", ch, in, NULL, TO_CHAR);
		return;
	}
	amount = UMIN(out->value[ITEM_DRINK_REMAINING], 
		in->value[ITEM_DRINK_TOTAL] - in->value[ITEM_DRINK_REMAINING]);

	in->value[ITEM_DRINK_REMAINING] += amount;

	if (out->value[ITEM_DRINK_TOTAL] > 0 ) 
		out->value[ITEM_DRINK_REMAINING] -= amount;

	in->value[ITEM_DRINK_TYPE] = out->value[ITEM_DRINK_TYPE];

	if (vch == NULL) {
		act_puts3("You pour $U from $p into $P.",
			  ch, out, in, liq_table[out->value[ITEM_DRINK_TYPE]].liq_name,
			  TO_CHAR | ACT_TRANS, POS_DEAD);
		act_puts3("$n pours $U from $p into $P.",
			  ch, out, in, liq_table[out->value[ITEM_DRINK_TYPE]].liq_name,
			  TO_ROOM | ACT_TRANS, POS_RESTING);
	}
	else {
		act_puts3("You pour some $U for $N.",
			  ch, NULL, vch, liq_table[out->value[ITEM_DRINK_TYPE]].liq_name,
			  TO_CHAR | ACT_TRANS, POS_DEAD);
		act_puts3("$n pours you some $U.",
			  ch, NULL, vch, liq_table[out->value[ITEM_DRINK_TYPE]].liq_name,
			  TO_VICT | ACT_TRANS, POS_RESTING);
		act_puts3("$n pours some $U for $N.",
			  ch, NULL, vch, liq_table[out->value[ITEM_DRINK_TYPE]].liq_name,
			  TO_NOTVICT | ACT_TRANS, POS_RESTING);
	}

}

void do_drink(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];
	OBJ_DATA       *obj;
	int             amount;
	int             liquid;
	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		for (obj = ch->in_room->contents; obj; obj= obj->next_content) {
			if (obj->pIndexData->item_type == ITEM_FOUNTAIN)
				break;
		}

		if (obj == NULL) {
			char_puts("Drink what?\n", ch);
			return;
		}
	} else {
		if ((obj = get_obj_here(ch, arg)) == NULL) {
			char_puts("You can't find it.\n", ch);
			return;
		}
	}

	if (!IS_NPC(ch) 
	&& ch->pcdata->condition[COND_DRUNK] > (drunk_tolerance(ch) * 1.30)) {
		char_puts("You fail to reach your mouth.  *Hic*\n", ch);
		return;
	}
	switch (obj->pIndexData->item_type) {
	default:
		char_puts("You can't drink from that.\n", ch);
		return;

	case ITEM_FOUNTAIN:
		if ((liquid = obj->value[ITEM_DRINK_TYPE]) < 0) {
			bug("Do_drink: bad liquid number %d.", liquid);
			liquid = obj->value[ITEM_DRINK_TYPE] = 0;
		}
		amount = liq_table[liquid].liq_affect[LIQ_AFFECT_SSIZE] * 3;
		break;

	case ITEM_DRINK_CON:
		if (obj->value[ITEM_DRINK_REMAINING] == 0) {
			char_puts("It is empty.\n", ch);
			return;
		}
		if ((liquid = obj->value[ITEM_DRINK_TYPE]) < 0) {
			bug("Do_drink: bad liquid number %d.", liquid);
			liquid = obj->value[ITEM_DRINK_TYPE] = 0;
		}
		amount = liq_table[liquid].liq_affect[LIQ_AFFECT_SSIZE];
		if (obj->value[ITEM_DRINK_TOTAL] >= 0) 
			amount = UMIN(amount, obj->value[ITEM_DRINK_REMAINING]);
		break;
	}
	if (!IS_NPC(ch) && !IS_IMMORTAL(ch)
	    && ch->pcdata->condition[COND_FULL] > 80) {
		char_puts("You're too full to drink more.\n", ch);
		return;
	}
	act("$n drinks $T from $p.",
	    ch, obj, liq_table[liquid].liq_name, TO_ROOM | ACT_TRANS);
	act_puts("You drink $T from $p.",
		 ch, obj, liq_table[liquid].liq_name,
		 TO_CHAR | ACT_TRANS, POS_DEAD);

	if (ch->fighting)
		WAIT_STATE(ch, 3 * PULSE_VIOLENCE);

	gain_condition(ch, COND_DRUNK,
		     amount * liq_table[liquid].liq_affect[LIQ_AFFECT_PROOF] / 36);
	gain_condition(ch, COND_FULL,
		       amount * liq_table[liquid].liq_affect[LIQ_AFFECT_FULL] / 2);
	gain_condition(ch, COND_THIRST,
		     amount * liq_table[liquid].liq_affect[LIQ_AFFECT_THIRST] / 5);
	gain_condition(ch, COND_HUNGER,
		     amount * liq_table[liquid].liq_affect[LIQ_AFFECT_FOOD] / 1);

	if (!IS_NPC(ch) && IS_DRUNK(ch))
		char_puts("You feel drunk.\n", ch);
	if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 60)
		char_puts("You are full.\n", ch);
	if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] > 60)
		char_puts("Your thirst is quenched.\n", ch);

	if (obj->value[ITEM_DRINK_POISON] != 0) {
		/* The drink was poisoned ! */
		AFFECT_DATA     af;

		act("$n chokes and gags.", ch, NULL, NULL, TO_ROOM);
		char_puts("You choke and gag.\n", ch);
		af.where = TO_AFFECTS;
		af.type = gsn_poison;
		af.level = number_fuzzy(amount);
		af.duration = 3 * amount;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = AFF_POISON;
		affect_join(ch, &af);
	}
	if (obj->value[ITEM_DRINK_TOTAL] > 0)
		obj->value[ITEM_DRINK_REMAINING] = UMAX(obj->value[ITEM_DRINK_REMAINING]-amount,0);
	return;
}

void do_eat(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];
	OBJ_DATA       *obj;

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Eat what?\n", ch);
		return;
	}
	if ((obj = get_obj_carry(ch, arg)) == NULL) {
		char_puts("You do not have that item.\n", ch);
		return;
	}
	if (!IS_IMMORTAL(ch)) {
		if ((obj->pIndexData->item_type != ITEM_FOOD ||
		     IS_SET(obj->extra_flags, ITEM_NOT_EDIBLE))
		&& obj->pIndexData->item_type != ITEM_PILL) {
			char_puts("That's not edible.\n", ch);
			return;
		}
		if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 80) {
			char_puts("You are too full to eat more.\n", ch);
			return;
		}
	}
	act("$n eats $p.", ch, obj, NULL, TO_ROOM);
	act_puts("You eat $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
	if (ch->fighting != NULL)
		WAIT_STATE(ch, 3 * PULSE_VIOLENCE);

	switch (obj->pIndexData->item_type) {

	case ITEM_FOOD:
		if (!IS_NPC(ch)) {
			int             condition;
			condition = ch->pcdata->condition[COND_HUNGER];
			gain_condition(ch, COND_FULL, obj->value[ITEM_FOOD_FULL] * 2);
			gain_condition(ch, COND_HUNGER, obj->value[ITEM_FOOD_HOURS] * 2);
			if (!condition
			&& ch->pcdata->condition[COND_HUNGER] > 0)
				char_puts("You are no longer hungry.\n", ch);
			else if (ch->pcdata->condition[COND_FULL] > 60)
				char_puts("You are full.\n", ch);
			if (obj->pIndexData->vnum == OBJ_VNUM_TORN_HEART) {
				ch->hit = UMIN(ch->max_hit, ch->hit+ch->level/10+number_range(1,5));
				char_puts("You feel empowered from the blood of your foe.\n",ch);
			};
		}
		if (obj->value[ITEM_FOOD_POISON] != 0) {
			/* The food was poisoned! */
			AFFECT_DATA     af;

			act("$n chokes and gags.", ch, NULL, NULL, TO_ROOM);
			char_puts("You choke and gag.\n", ch);

			af.where = TO_AFFECTS;
			af.type = gsn_poison;
			af.level = number_fuzzy(obj->value[ITEM_FOOD_HOURS]);
			af.duration = 2 * obj->value[ITEM_FOOD_FULL];
			af.location = APPLY_NONE;
			af.modifier = 0;
			af.bitvector = AFF_POISON;
			affect_join(ch, &af);
		}
		break;

	case ITEM_PILL:
/*Stuff Daphais put in*/ 
		if (ch->level < obj->level)
		{
		send_to_char("This magic is too powerful for you to ingest.\n\r",ch);
		extract_obj(obj, 0);
		return;
    		}

		obj_cast_spell(obj->value[ITEM_POTION_SPELL1], obj->value[ITEM_POTION_LEVEL], ch, ch, NULL);
		obj_cast_spell(obj->value[ITEM_POTION_SPELL2], obj->value[ITEM_POTION_LEVEL], ch, ch, NULL);
		obj_cast_spell(obj->value[ITEM_POTION_SPELL3], obj->value[ITEM_POTION_LEVEL], ch, ch, NULL);
		obj_cast_spell(obj->value[ITEM_POTION_SPELL4], obj->value[ITEM_POTION_LEVEL], ch, ch, NULL);
		break;
	}

	extract_obj(obj, 0);
	return;
}


/*
 * Remove an object.
 */
bool remove_obj(CHAR_DATA * ch, int iWear, bool fReplace)
{
	OBJ_DATA       *obj;
	if ((obj = get_eq_char(ch, iWear)) == NULL)
		return TRUE;

	if (!fReplace)
		return FALSE;

	if (!IS_IMMORTAL(ch) && IS_SET(obj->extra_flags, ITEM_NOREMOVE)) {
		act_puts("You can't remove $p.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		return FALSE;
	}
	if ((obj->pIndexData->item_type == ITEM_TATTOO) && (!IS_IMMORTAL(ch))) {
		act_puts("You must scratch it off to remove $p.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		return FALSE;
	}
	if (iWear == WEAR_STUCK_IN) {
		unequip_char(ch, obj);

		if (get_eq_char(ch, WEAR_STUCK_IN) == NULL) {
			if (is_affected(ch, gsn_arrow))
				affect_strip(ch, gsn_arrow);
			if (is_affected(ch, gsn_spear))
				affect_strip(ch, gsn_spear);
			if (is_affected(ch, gsn_sling))
				affect_strip(ch, gsn_sling);
		}
		act_puts("You remove $p, in pain.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		act("$n removes $p, in pain.", ch, obj, NULL, TO_ROOM);
		WAIT_STATE(ch, 4);
		return TRUE;
	}
	unequip_char(ch, obj);
	act("$n stops using $p.", ch, obj, NULL, TO_ROOM);
	act_puts("You stop using $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);

	if (iWear == WEAR_WIELD
	    && (obj = get_eq_char(ch, WEAR_SECOND_WIELD)) != NULL) {
		unequip_char(ch, obj);
		equip_char(ch, obj, WEAR_WIELD);
	}
	return TRUE;
}

/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 */
void wear_obj(CHAR_DATA * ch, OBJ_DATA * obj, bool fReplace)
{
	int wear_level = get_wear_level(ch, obj);

	if (wear_level < obj->level) {
		char_printf(ch, "You must be level %d to use this object.\n",
			    obj->level - wear_level + ch->level);
		act("$n tries to use $p, but is too inexperienced.",
		    ch, obj, NULL, TO_ROOM);
		return; 
	}

	if (!can_clan_wear_obj(ch, ch->clan, obj)) {
		act_puts("Your vows to your clan do not permit you to wear $p!\n",
			ch, obj, NULL, TO_CHAR, POS_DEAD);
		return;
	}

	if (IS_NPC(ch) && !can_npc_wear_obj(ch, obj)) {
		act("$n seems incapable of handling $p!\n",
			ch, obj, NULL, TO_ALL);
		return;
	}

	if (obj->pIndexData->item_type == ITEM_LIGHT) {
		if (!remove_obj(ch, WEAR_LIGHT, fReplace))
			return;
		if (obj->value[ITEM_LIGHT_DURATION] == 0) {
			act("$n lights $p and holds it, but is sputters and goes out.", 
				ch, obj, NULL, TO_ROOM);
			act_puts("You light $p and hold it, but it sputters and goes out.",
				ch, obj, NULL, TO_CHAR, POS_DEAD);
		}
		else {
			act("$n lights $p and holds it.", ch, obj, NULL, TO_ROOM);
			act_puts("You light $p and hold it.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
		}
		equip_char(ch, obj, WEAR_LIGHT);
		return;
	}
	if (CAN_WEAR(obj, ITEM_WEAR_FINGER)) {
		if (get_eq_char(ch, WEAR_FINGER_L) != NULL
		&&  get_eq_char(ch, WEAR_FINGER_R) != NULL
		&&  !remove_obj(ch, WEAR_FINGER_L, fReplace)
		&&  !remove_obj(ch, WEAR_FINGER_R, fReplace))
			return;

		if (get_eq_char(ch, WEAR_FINGER_L) == NULL) {
			act("$n wears $p on $s left finger.",
			    ch, obj, NULL, TO_ROOM);
			act_puts("You wear $p on your left finger.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			equip_char(ch, obj, WEAR_FINGER_L);
			return;
		}
		if (get_eq_char(ch, WEAR_FINGER_R) == NULL) {
			act("$n wears $p on $s right finger.",
			    ch, obj, NULL, TO_ROOM);
			act_puts("You wear $p on your right finger.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			equip_char(ch, obj, WEAR_FINGER_R);
			return;
		}
		bug("Wear_obj: no free finger.", 0);
		char_puts("You already wear two rings.\n", ch);
		return;
	}
	if (CAN_WEAR(obj, ITEM_WEAR_NECK)) {
		if (get_eq_char(ch, WEAR_NECK_1) != NULL
		    && get_eq_char(ch, WEAR_NECK_2) != NULL
		    && !remove_obj(ch, WEAR_NECK_1, fReplace)
		    && !remove_obj(ch, WEAR_NECK_2, fReplace))
			return;

		if (get_eq_char(ch, WEAR_NECK_1) == NULL) {
			act("$n wears $p around $s neck.",
			    ch, obj, NULL, TO_ROOM);
			act_puts("You wear $p around your neck.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			equip_char(ch, obj, WEAR_NECK_1);
			return;
		}
		if (get_eq_char(ch, WEAR_NECK_2) == NULL) {
			act("$n wears $p around $s neck.",
			    ch, obj, NULL, TO_ROOM);
			act_puts("You wear $p around your neck.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			equip_char(ch, obj, WEAR_NECK_2);
			return;
		}
		bug("Wear_obj: no free neck.", 0);
		char_puts("You already wear two neck items.\n", ch);
		return;
	}
	if (CAN_WEAR(obj, ITEM_WEAR_BODY)) {
		if (!remove_obj(ch, WEAR_BODY, fReplace))
			return;
		act("$n wears $p on $s torso.", ch, obj, NULL, TO_ROOM);
		act_puts("You wear $p on your torso.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		equip_char(ch, obj, WEAR_BODY);
		return;
	}
	if (CAN_WEAR(obj, ITEM_WEAR_HEAD)) {
		if (!remove_obj(ch, WEAR_HEAD, fReplace))
			return;
		act("$n wears $p on $s head.", ch, obj, NULL, TO_ROOM);
		act_puts("You wear $p on your head.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		equip_char(ch, obj, WEAR_HEAD);
		return;
	}
	if (CAN_WEAR(obj, ITEM_WEAR_LEGS)) {
		if (!remove_obj(ch, WEAR_LEGS, fReplace))
			return;
		act("$n wears $p on $s legs.", ch, obj, NULL, TO_ROOM);
		act_puts("You wear $p on your legs.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		equip_char(ch, obj, WEAR_LEGS);
		return;
	}
	if (CAN_WEAR(obj, ITEM_WEAR_FEET)) {
		if (!remove_obj(ch, WEAR_FEET, fReplace))
			return;
		act("$n wears $p on $s feet.", ch, obj, NULL, TO_ROOM);
		act_puts("You wear $p on your feet.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		equip_char(ch, obj, WEAR_FEET);
		return;
	}
	if (CAN_WEAR(obj, ITEM_WEAR_HANDS)) {
		if (!remove_obj(ch, WEAR_HANDS, fReplace))
			return;
		act("$n wears $p on $s hands.", ch, obj, NULL, TO_ROOM);
		act_puts("You wear $p on your hands.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		equip_char(ch, obj, WEAR_HANDS);
		return;
	}
	if (CAN_WEAR(obj, ITEM_WEAR_ARMS)) {
		if (!remove_obj(ch, WEAR_ARMS, fReplace))
			return;
		act("$n wears $p on $s arms.", ch, obj, NULL, TO_ROOM);
		act_puts("You wear $p on your arms.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		equip_char(ch, obj, WEAR_ARMS);
		return;
	}
	if (CAN_WEAR(obj, ITEM_WEAR_ABOUT)) {
		if (!remove_obj(ch, WEAR_ABOUT, fReplace))
			return;
		act("$n wears $p on $s torso.", ch, obj, NULL, TO_ROOM);
		act_puts("You wear $p on your torso.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		equip_char(ch, obj, WEAR_ABOUT);
		return;
	}
	if (CAN_WEAR(obj, ITEM_WEAR_WAIST)) {
		if (!remove_obj(ch, WEAR_WAIST, fReplace))
			return;
		act("$n wears $p about $s waist.", ch, obj, NULL, TO_ROOM);
		act_puts("You wear $p about your waist.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		equip_char(ch, obj, WEAR_WAIST);
		return;
	}
	if (CAN_WEAR(obj, ITEM_WEAR_WRIST)) {
		if (get_eq_char(ch, WEAR_WRIST_L) != NULL
		    && get_eq_char(ch, WEAR_WRIST_R) != NULL
		    && !remove_obj(ch, WEAR_WRIST_L, fReplace)
		    && !remove_obj(ch, WEAR_WRIST_R, fReplace))
			return;

		if (get_eq_char(ch, WEAR_WRIST_L) == NULL) {
			act("$n wears $p around $s left wrist.",
			    ch, obj, NULL, TO_ROOM);
			act_puts("You wear $p around your left wrist.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			equip_char(ch, obj, WEAR_WRIST_L);
			return;
		}
		if (get_eq_char(ch, WEAR_WRIST_R) == NULL) {
			act("$n wears $p around $s right wrist.",
			    ch, obj, NULL, TO_ROOM);
			act_puts("You wear $p around your right wrist.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			equip_char(ch, obj, WEAR_WRIST_R);
			return;
		}
		bug("Wear_obj: no free wrist.", 0);
		char_puts("You already wear two wrist items.\n", ch);
		return;
	}
	if (CAN_WEAR(obj, ITEM_WEAR_SHIELD)) {
		OBJ_DATA       *weapon;
		if (get_eq_char(ch, WEAR_SECOND_WIELD) != NULL) {
			char_puts("You can't use a shield while using a second weapon.\n", ch);
			return;
		}
		if (!remove_obj(ch, WEAR_SHIELD, fReplace))
			return;

		weapon = get_eq_char(ch, WEAR_WIELD);
		if (weapon != NULL && ch->size < SIZE_LARGE
		&&  IS_WEAPON_STAT(weapon, WEAPON_TWO_HANDS)) {
			char_puts("Your hands are tied up with your weapon!\n", ch);
			return;
		}
		act("$n wears $p as a shield.", ch, obj, NULL, TO_ROOM);
		act_puts("You wear $p as a shield.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		equip_char(ch, obj, WEAR_SHIELD);
		return;
	}
	if (CAN_WEAR(obj, ITEM_WIELD)) {
		int             skill;
		OBJ_DATA       *dual;

		if ((dual = get_eq_char(ch, WEAR_SECOND_WIELD)) != NULL) {
			unequip_char(ch, dual);
		}

		if (dual && !can_dual_weapons(obj, dual)) {
		   	char_puts("You find that weapon combination unmanageable.\n", ch);
			equip_char(ch, dual, WEAR_SECOND_WIELD);
			return;
		}

		if (!remove_obj(ch, WEAR_WIELD, fReplace))
			return;

		if (!IS_NPC(ch)
		&& !get_stat_str_can_wield(ch, obj, HAND_PRIMARY)) {
			char_puts("It is too heavy for you to wield.\n", ch);
			if (dual)
				equip_char(ch, dual, WEAR_SECOND_WIELD);
			return;
		}
		if (IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS)
		&&  ((!IS_NPC(ch) && ch->size < SIZE_LARGE &&
		      get_eq_char(ch, WEAR_SHIELD) != NULL) ||
		      dual)) {
			char_puts("You need two hands free for that weapon.\n", ch);
			if (dual)
				equip_char(ch, dual, WEAR_SECOND_WIELD);
			return;
		}

		act("$n wields $p.", ch, obj, NULL, TO_ROOM);
		act_puts("You wield $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
		obj = equip_char(ch, obj, WEAR_WIELD);
		if (dual)
			equip_char(ch, dual, WEAR_SECOND_WIELD);
		else if (!get_eq_char(ch, WEAR_SHIELD)
			&& !get_eq_char(ch, WEAR_HOLD)
			&& get_skill(ch, gsn_double_grip) > 0) {
			act_puts("You secure your grip with your second hand.", 
				ch, NULL, NULL, TO_CHAR, POS_DEAD);
		}

		if (obj == NULL)
			return;

		skill = get_weapon_skill(ch, get_weapon_sn(obj));

		if (skill >= 100)
			act_puts("$p feels like a part of you!",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
		else if (skill > 85)
			act_puts("You feel quite confident with $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
		else if (skill > 70)
			act_puts("You are skilled with $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
		else if (skill > 50)
			act_puts("Your skill with $p is adequate.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
		else if (skill > 25)
			act_puts("$p feels a little clumsy in your hands.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
		else if (skill > 1)
			act_puts("You fumble and almost drop $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
		else
			act_puts("You don't even know which end is up on $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
		return;
	}
	if (CAN_WEAR(obj, ITEM_HOLD)) {
		if (get_eq_char(ch, WEAR_SECOND_WIELD) != NULL) {
			act_puts("You can't hold an item while using 2 weapons.",
				 ch, NULL, NULL, TO_CHAR, POS_DEAD);
			return;
		}
		if (!remove_obj(ch, WEAR_HOLD, fReplace))
			return;
		act("$n holds $p in $s hand.", ch, obj, NULL, TO_ROOM);
		act_puts("You hold $p in your hand.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		equip_char(ch, obj, WEAR_HOLD);
		return;
	}
	if (CAN_WEAR(obj, ITEM_WEAR_FLOAT)) {
		if (!remove_obj(ch, WEAR_FLOAT, fReplace))
			return;
		act("$n releases $p to float next to $m.",
		    ch, obj, NULL, TO_ROOM);
		act_puts("You release $p and it floats next to you.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		equip_char(ch, obj, WEAR_FLOAT);
		return;
	}
	if (CAN_WEAR(obj, ITEM_WEAR_TATTOO) && IS_IMMORTAL(ch)) {
		if (!remove_obj(ch, WEAR_TATTOO, fReplace))
			return;
		act("$n now uses $p as tattoo of $s religion.",
		    ch, obj, NULL, TO_ROOM);
		act_puts("You now use $p as the tattoo of your religion.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		equip_char(ch, obj, WEAR_TATTOO);
		return;
	}
	if (CAN_WEAR(obj, ITEM_WEAR_CLANMARK)) {
		if (!remove_obj(ch, WEAR_CLANMARK, fReplace))
			return;
		act("$n now uses $p as $s clan mark.",
		    ch, obj, NULL, TO_ROOM);
		act_puts("You now use $p as  your clan mark.",
		    ch, obj, NULL, TO_CHAR, POS_DEAD);
		equip_char(ch, obj, WEAR_CLANMARK);
		return;
	}
	if (fReplace)
		char_puts("You can't wear, wield, or hold that.\n", ch);
}

void do_wear(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];
	OBJ_DATA       *obj;
	OBJ_DATA       *obj_next;
	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Wear, wield, or hold what?\n", ch);
		return;
	}
	if (!str_cmp(arg, "all"))
		for (obj = ch->carrying; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj))
				wear_obj(ch, obj, FALSE);
		}
	else if ((obj = get_obj_carry(ch, arg)) == NULL) {
		char_puts("You do not have that item.\n", ch);
		return;
	} else
	    wear_obj(ch, obj, TRUE);
}

void do_remove(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];
	OBJ_DATA       *obj;
	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Remove what?\n", ch);
		return;
	}
	if (!str_cmp(arg, "all")) {
		OBJ_DATA       *obj_next;

		if (!IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)) {
			do_say(ch, "Nah, I won't do that.");
			return;
		}

		for (obj = ch->carrying; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			if (obj->wear_loc != WEAR_NONE && can_see_obj(ch, obj))
				remove_obj(ch, obj->wear_loc, TRUE);
		}
		return;
	}
	if ((obj = get_obj_wear(ch, arg)) == NULL) {
		char_puts("You do not have that item.\n", ch);
		return;
	}
	remove_obj(ch, obj->wear_loc, TRUE);
	return;
}

void do_sacrifice(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];
	OBJ_DATA       *obj;
	OBJ_DATA       *r_next_cont;

	if (!strcmp(argument, "all")) {
		for (obj = ch->in_room->contents; obj; obj = r_next_cont) {
			r_next_cont = obj->next_content;
			sac_obj(ch, obj);
		}
		return;
	}

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0' || !str_cmp(arg, ch->name)) {
		act("$n offers $mself to gods, who graciously declines.",
		    ch, NULL, NULL, TO_ROOM);
		char_puts("Gods appreciates your offer "
			  "and may accept it later.\n", ch);
		return;
	}

	obj = get_obj_list(ch, arg, ch->in_room->contents);
	if (obj == NULL) {
		char_puts("You can't find it.\n", ch);
		return;
	}

	sac_obj(ch, obj);
}

void sac_obj(CHAR_DATA * ch, OBJ_DATA *obj)
{
	int             silver;
	CHAR_DATA      *gch;
	int             members;

	if ((obj->pIndexData->item_type == ITEM_CORPSE_PC &&
	     ch->level < MAX_LEVEL)
	||  (QUEST_OBJ_FIRST <= obj->pIndexData->vnum &&
	     obj->pIndexData->vnum <= QUEST_OBJ_LAST)) {
		char_puts("Gods wouldn't like that.\n", ch);
		return;
	}

	if (!CAN_WEAR(obj, ITEM_TAKE) 
	|| CAN_WEAR(obj, ITEM_NO_SAC)
	|| IS_SET(obj->extra_flags, ITEM_NOSAC)) {
		if (!IS_NULLSTR(obj->description))
			act_puts("$p is not an acceptable sacrifice.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
		return;
	}

	act("$n sacrifices $p to gods.", ch, obj, NULL, TO_ROOM);

	if (oprog_call(OPROG_SAC, obj, ch, NULL))
		return;

	silver = UMAX(1, number_fuzzy(obj->level));

	if (obj->pIndexData->item_type != ITEM_CORPSE_NPC
	&&  obj->pIndexData->item_type != ITEM_CORPSE_PC)
		silver = UMIN(silver, obj->cost);

	act_puts("Gods give you $j silver $qj{coins} for your sacrifice.",
		 ch, (const void*) silver, NULL, TO_CHAR, POS_DEAD);

	ch->silver += silver;

	if (IS_SET(ch->conf_flags, PLR_CONF_AUTOSPLIT)) {
		/* AUTOSPLIT code */
		members = 0;
		for (gch = ch->in_room->people; gch != NULL;
		     gch = gch->next_in_room)
			if (is_same_group(gch, ch))
				members++;

		if (members > 1 && silver > 1)
			doprintf(do_split, ch, "%d", silver);
	}

	if (obj->pIndexData->item_type == ITEM_CORPSE_NPC
	||  obj->pIndexData->item_type == ITEM_CORPSE_PC) {
		OBJ_DATA       *obj_content;
		OBJ_DATA       *obj_next;
		OBJ_DATA       *two_objs[2];

		int	iScatter = 0;

		const char *qty;
		const char *where;

		for (obj_content = obj->contains; obj_content;
		     obj_content = obj_next) {
			obj_next = obj_content->next_content;
			two_objs[iScatter < 1 ? 0 : 1] = obj_content;
			obj_from_obj(obj_content);
			obj_to_room(obj_content, ch->in_room);
			iScatter++;
		}

		switch (iScatter) {
		case 0:
			break;

		case 1:
			act_puts("Your sacrifice reveals $p.",
				 ch, two_objs[0], NULL, TO_CHAR, POS_DEAD);
			act("$p is revealed by $n's sacrifice.",
			    ch, two_objs[0], NULL, TO_ROOM);
			break;

		case 2:
			act_puts("Your sacrifice reveals $p and $P.",
				 ch, two_objs[0], two_objs[1],
				 TO_CHAR, POS_DEAD);
			act("$p and $P are revealed by $n's sacrifice.",
			    ch, two_objs[0], two_objs[1], TO_ROOM);
			break;

		default: 
			if (iScatter < 5)
				qty = "few things";
			else if (iScatter < 9)
				qty = "a bunch of objects";
			else if (iScatter < 15)
				qty = "many things";
			else
				qty = "a lot of objects";

			switch (ch->in_room->sector_type) {
			case SECT_FIELD:
			case SECT_FOREST:
				where = "on the dirt";
				break;

			case SECT_WATER_SWIM:
			case SECT_WATER_NOSWIM:
				where = "over the water";
				break;

			default:
				where = "around";
				break;
			}
			act_puts("As you sacrifice the corpse, $t on it "
				 "scatter $T.",
				 ch, qty, where, TO_CHAR | ACT_TRANS, POS_DEAD);
			act("As $n sacrifices the corpse, $t on it scatter $T.",
			    ch, qty, where, TO_ROOM | ACT_TRANS);
			break;
		}
	}
	extract_obj(obj, 0);
}

void quaff_obj(CHAR_DATA *ch, OBJ_DATA *obj)
{
	act("$n quaffs $p.", ch, obj, NULL, TO_ROOM);
	act_puts("You quaff $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);

	obj_cast_spell(obj->value[ITEM_POTION_SPELL1], obj->value[ITEM_POTION_LEVEL], ch, ch, NULL);
	obj_cast_spell(obj->value[ITEM_POTION_SPELL2], obj->value[ITEM_POTION_LEVEL], ch, ch, NULL);
	obj_cast_spell(obj->value[ITEM_POTION_SPELL3], obj->value[ITEM_POTION_LEVEL], ch, ch, NULL);
	obj_cast_spell(obj->value[ITEM_POTION_SPELL4], obj->value[ITEM_POTION_LEVEL], ch, ch, NULL);

	if (IS_PUMPED(ch) || ch->fighting != NULL)
		WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

	extract_obj(obj, 0);
	obj_to_char(create_obj(get_obj_index(OBJ_VNUM_POTION_VIAL), 0), ch);
}

void do_quaff(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];
	OBJ_DATA       *obj;
	one_argument(argument, arg, sizeof(arg));
	
	if (HAS_SKILL(ch, gsn_spellbane)) {
		char_puts("You are a Barbarian, not filthy magician!\n",ch);
		return;
	}

	if (arg[0] == '\0') {
		char_puts("Quaff what?\n", ch);
		return;
	}

	if ((obj = get_obj_carry(ch, arg)) == NULL) {
		char_puts("You do not have that potion.\n", ch);
		return;
	}

	if (obj->pIndexData->item_type != ITEM_POTION) {
		char_puts("You can quaff only potions.\n", ch);
		return;
	}

	if (ch->level < obj->level) {
		char_puts("This liquid is too powerful for you to drink.\n", ch);
		return;
	}
	quaff_obj(ch, obj);
}

void do_recite(CHAR_DATA * ch, const char *argument)
{
	char            arg1[MAX_INPUT_LENGTH];
	char            arg2[MAX_INPUT_LENGTH];
	CHAR_DATA      *victim;
	OBJ_DATA       *scroll;
	OBJ_DATA       *obj;
	int		sn;

	if (HAS_SKILL(ch, gsn_spellbane)) {
		char_puts ("RECITE? You are Barbarian!\n", ch);
		return;
	}

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if ((scroll = get_obj_carry(ch, arg1)) == NULL) {
		char_puts("You do not have that scroll.\n", ch);
		return;
	}

	if (scroll->pIndexData->item_type != ITEM_SCROLL) {
		char_puts("You can recite only scrolls.\n", ch);
		return;
	}

	if (ch->level < scroll->level
	||  (sn = sn_lookup("scrolls")) < 0) {
		char_puts("This scroll is too complex for you to comprehend.\n", ch);
		return;
	}

	obj = NULL;
	if (arg2[0] == '\0')
		victim = ch;
	else if ((victim = get_char_room(ch, arg2)) == NULL
	&&       (obj = get_obj_here(ch, arg2)) == NULL) {
		WAIT_STATE(ch, MISSING_TARGET_DELAY);
		char_puts("You can't find it.\n", ch);
		return;
	}

	act("$n recites $p.", ch, scroll, NULL, TO_ROOM);
	act_puts("You recite $p.", ch, scroll, NULL, TO_CHAR, POS_DEAD);

	if (number_percent() >= get_skill(ch, sn) * 4 / 5
	|| (IS_DRUNK(ch) && number_percent() < 15)) {
		act_puts("You mispronounce a syllable.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		if (!IS_DRUNK(ch))
			check_improve(ch, sn, FALSE, 2);
	} else {
		obj_cast_spell(scroll->value[ITEM_POTION_SPELL1], scroll->value[ITEM_POTION_LEVEL],
			       ch, victim, obj);
		obj_cast_spell(scroll->value[ITEM_POTION_SPELL2], scroll->value[ITEM_POTION_LEVEL],
			       ch, victim, obj);
		obj_cast_spell(scroll->value[ITEM_POTION_SPELL3], scroll->value[ITEM_POTION_LEVEL],
			       ch, victim, obj);
		obj_cast_spell(scroll->value[ITEM_POTION_SPELL4], scroll->value[ITEM_POTION_LEVEL],
			       ch, victim, obj);
		check_improve(ch, sn, TRUE, 2);

		if (IS_PUMPED(ch) || ch->fighting != NULL)
			WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
	}

	extract_obj(scroll, 0);
}

void do_brandish(CHAR_DATA * ch, const char *argument)
{
	CHAR_DATA      *vch;
	CHAR_DATA      *vch_next;
	OBJ_DATA       *staff;
	int             sn;
	skill_t *	sk;

	if (HAS_SKILL(ch, gsn_spellbane)) {
		char_puts("BRANDISH? You are not a filthy magician!\n",ch);
		return;
	}

	if ((staff = get_eq_char(ch, WEAR_HOLD)) == NULL) {
		char_puts("You hold nothing in your hand.\n", ch);
		return;
	}

	if (staff->pIndexData->item_type != ITEM_STAFF) {
		char_puts("You can brandish only with a staff.\n", ch);
		return;
	}

	if ((sk = skill_lookup(staff->value[ITEM_WAND_SPELL])) == NULL
	||  sk->spell_fun == NULL
	||  (sn = sn_lookup("staves")) < 0)
		return;

	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

	if (staff->value[ITEM_WAND_CHARGES_REMAINING] > 0) {
		act("$n brandishes $p.", ch, staff, NULL, TO_ROOM);
		act("You brandish $p.", ch, staff, NULL, TO_CHAR);
		if (ch->level + 3 < staff->level
		|| number_percent() >= 10 + get_skill(ch, sn) * 4 / 5) {
			act("You fail to invoke $p.", ch, staff, NULL, TO_CHAR);
			act("...and nothing happens.", ch, NULL, NULL, TO_ROOM);
			check_improve(ch, sn, FALSE, 2);
		}
		else {
			skill_t *spell = skill_lookup(staff->value[ITEM_WAND_SPELL]);

			if (!spell)
				return;

			for (vch = ch->in_room->people; vch; vch = vch_next) {
				vch_next = vch->next_in_room;

				switch (sk->target) {
				default:
					return;

				case TAR_IGNORE:
					if (vch != ch)
						continue;
					break;

				case TAR_CHAR_OFFENSIVE:
					if (IS_NPC(ch) ? IS_NPC(vch) : !IS_NPC(vch))
						continue;
					break;

				case TAR_CHAR_DEFENSIVE:
					if (IS_NPC(ch) ? !IS_NPC(vch) : IS_NPC(vch))
						continue;
					break;

				case TAR_CHAR_SELF:
					if (vch != ch)
						continue;
					break;
				}

				obj_cast_spell(staff->value[ITEM_WAND_SPELL],
					       staff->value[ITEM_WAND_LEVEL], 
					       ch, vch, NULL);
				if (IS_SET(spell->flags, SKILL_AREA_ATTACK))
					break;
			}
			check_improve(ch, sn, TRUE, 2);
		}
	}

	if (--staff->value[ITEM_WAND_CHARGES_REMAINING] <= 0) {
		act("$n's $p blazes bright and is gone.",
		    ch, staff, NULL, TO_ROOM);
		act("Your $p blazes bright and is gone.",
		    ch, staff, NULL, TO_CHAR);
		extract_obj(staff, 0);
	}
}

void do_zap(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];
	CHAR_DATA      *victim;
	OBJ_DATA       *wand;
	OBJ_DATA       *obj;
	int		sn;
	
	if (HAS_SKILL(ch, gsn_spellbane)) {
		char_puts("You'd destroy magic, not use it!\n",ch);
		return;
	}

	one_argument(argument, arg, sizeof(arg));
	if ((wand = get_eq_char(ch, WEAR_HOLD)) == NULL) {
		char_puts("You hold nothing in your hand.\n", ch);
		return;
	}
	if (wand->pIndexData->item_type != ITEM_WAND) {
		char_puts("You can zap only with a wand.\n", ch);
		return;
	}

	if ((sn = sn_lookup("wands")) < 0)
		return;

	obj = NULL;
	if (arg[0] == '\0') {
		if (ch->fighting && ch->fighting->in_room == ch->in_room)
			victim = ch->fighting;
		else {
			char_puts("Zap whom or what?\n", ch);
			return;
		}
	}
	else {
		if ((victim = get_char_room(ch, arg)) == NULL
		    && (obj = get_obj_here(ch, arg)) == NULL) {
			WAIT_STATE(ch, MISSING_TARGET_DELAY);
			char_puts("You can't find it.\n", ch);
			return;
		}
	}

	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

	if (wand->value[ITEM_WAND_CHARGES_REMAINING] > 0) {
		if (victim != NULL) {
			act("$n zaps $N with $p.", ch, wand, victim, TO_ROOM);
			act("You zap $N with $p.", ch, wand, victim, TO_CHAR);
		} else {
			act("$n zaps $P with $p.", ch, wand, obj, TO_ROOM);
			act("You zap $P with $p.", ch, wand, obj, TO_CHAR);
		}

		if (ch->level + 5 < wand->level
		|| number_percent() >= 20 + get_skill(ch, sn) * 4 / 5) {
			act("Your efforts with $p produce only smoke and sparks.",
			    ch, wand, NULL, TO_CHAR);
			act("$n's efforts with $p produce only smoke and sparks.",
			    ch, wand, NULL, TO_ROOM);
			check_improve(ch, sn, FALSE, 2);
		} else {
			obj_cast_spell(wand->value[ITEM_WAND_SPELL], 
					wand->value[ITEM_WAND_LEVEL],
				       ch, victim, obj);
			check_improve(ch, sn, TRUE, 2);
		}
	}
	if (--wand->value[ITEM_WAND_CHARGES_REMAINING] <= 0) {
		act("$n's $p explodes into fragments.",
		    ch, wand, NULL, TO_ROOM);
		act("Your $p explodes into fragments.",
		    ch, wand, NULL, TO_CHAR);
		extract_obj(wand, 0);
	}
}

/*
 * Shopping commands.
 */
CHAR_DATA * find_keeper(CHAR_DATA * ch)
{
	CHAR_DATA      *keeper;
	SHOP_DATA      *pShop = NULL;

	for (keeper = ch->in_room->people; keeper; keeper = keeper->next_in_room) {
		if (IS_NPC(keeper)
		&&  (pShop = keeper->pIndexData->pShop) != NULL)
			break;
	}

	if (pShop == NULL) {
		char_puts("You can't do that here.\n", ch);
		return NULL;
	}

	if (!IS_NPC(ch)
	&&  IS_SET(ch->state_flags, STATE_WANTED)
	&&  !IS_SET(keeper->pIndexData->attr_flags, MOB_ATTR_BLACKMARKET)) {
        /*&&  IS_SET(ch->in_room->room_flags, ROOM_LAW)
	&&  (!keeper->clan || (keeper->clan != ch->clan))) {*/
		do_say(keeper, "What, you think this is the blackmarket?!");
		doprintf(do_yell, keeper, "%s the CRIMINAL is over here!\n",
			 ch->name);
		return NULL;
	}

	/*
	 * Shop hours.
	 */
	if (time_info.hour < pShop->open_hour) {
		do_say(keeper, "Sorry, I am closed. Come back later.");
		return NULL;
	}
	if (time_info.hour > pShop->close_hour) {
		do_say(keeper, "Sorry, I am closed. Come back tomorrow.");
		return NULL;
	}
	/*
	 * Invisible or hidden people.
	 */
	if (!can_see(keeper, ch) && !IS_IMMORTAL(ch)) {
		do_say(keeper, "I don't trade with folks I can't see.");
		do_scan(keeper, str_empty);
		return NULL;
	}
	return keeper;
}

/* insert an object at the right spot for the keeper */
void obj_to_keeper(OBJ_DATA * obj, CHAR_DATA * ch)
{
	OBJ_DATA       *t_obj, *t_obj_next;
	/* see if any duplicates are found */
	for (t_obj = ch->carrying; t_obj != NULL; t_obj = t_obj_next) {
		t_obj_next = t_obj->next_content;

		if (obj->pIndexData == t_obj->pIndexData
		&& !mlstr_cmp(obj->short_descr, t_obj->short_descr)) {
			if (IS_OBJ_STAT(t_obj, ITEM_INVENTORY)) {
				extract_obj(obj, 0);
				return;
			}
			obj->cost = t_obj->cost;	/* keep it standard */
			break;
		}
	}

	if (t_obj == NULL) {
		obj->next_content = ch->carrying;
		ch->carrying = obj;
	} else {
		obj->next_content = t_obj->next_content;
		t_obj->next_content = obj;
	}

	obj->carried_by = ch;
	obj->in_room = NULL;
	obj->in_obj = NULL;
	ch->carry_number += get_obj_number(obj);
	ch->carry_weight += get_obj_weight(obj);
}

/* get an object from a shopkeeper's list */
OBJ_DATA * get_obj_keeper(CHAR_DATA * ch, CHAR_DATA * keeper, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];
	OBJ_DATA       *obj;
	int             number;
	int             count;
	number = number_argument(argument, arg, sizeof(arg));
	count = 0;
	for (obj = keeper->carrying; obj != NULL; obj = obj->next_content) {
		if (obj->wear_loc == WEAR_NONE
		    && can_see_obj(keeper, obj)
		    && can_see_obj(ch, obj)
		    && is_name(arg, obj->name)) {
			if (++count == number)
				return obj;

			/* skip other objects of the same name */
			while (obj->next_content != NULL
			  && obj->pIndexData == obj->next_content->pIndexData
			  && !mlstr_cmp(obj->short_descr, obj->next_content->short_descr))
				obj = obj->next_content;
		}
	}

	return NULL;
}

uint get_cost(CHAR_DATA *ch, CHAR_DATA * keeper, OBJ_DATA * obj, bool fBuy)
{
	SHOP_DATA *	pShop;
	uint		cost;
	int 		inflation = 100;
	WORLD_AFFECT_DATA *waff;

	if (obj == NULL || (pShop = keeper->pIndexData->pShop) == NULL)
		return 0;

	if (IS_OBJ_STAT(obj, ITEM_NOSELL))
		return 0;

	if ((waff = ch_waffected(ch, WAFF_INFLATION)) != NULL) {
		if (waff->modifier <= 0) {
			BUG("World Affect inflation would cause negative prices."
			    "  Setting to 100% (no inflation).");
			waff->modifier = 100;
		}
		inflation = waff->modifier;
	}

	if (fBuy) {
		cost = obj->cost * pShop->profit_buy / 100;
		cost = cost * get_stat_cha_cost_mod(ch) / 100;
	}
	else {
		OBJ_DATA       *obj2;
		int             itype;
		cost = 0;

		if (obj->pIndexData->item_type == ITEM_GEM)
			cost = obj->cost * pShop->profit_sell / 100;
		else
			for (itype = 0; itype < MAX_TRADE; itype++) {
				if (obj->pIndexData->item_type == pShop->buy_type[itype]) {
					cost = obj->cost * pShop->profit_sell / 100;
					break;
				}
			}

		if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT))
			for (obj2 = keeper->carrying; obj2; obj2 = obj2->next_content) {
				if (obj->pIndexData == obj2->pIndexData
				&&  !mlstr_cmp(obj->short_descr,
					       obj2->short_descr))
					return 0;
/*
	 	    if (IS_OBJ_STAT(obj2,ITEM_INVENTORY))
				cost /= 2;
		    else
		              	cost = cost * 3 / 4;
*/
			}
		cost = cost * get_stat_cha_sell_mod(ch) / 100;
	}


	if (obj->pIndexData->item_type == ITEM_STAFF
	||  obj->pIndexData->item_type == ITEM_WAND) {
		if (obj->value[ITEM_WAND_CHARGES_TOTAL] == 0)
			cost /= 4;
		else
			cost = cost * obj->value[ITEM_WAND_CHARGES_REMAINING] 
				/ obj->value[ITEM_WAND_CHARGES_TOTAL];
	}

	cost = cost * inflation / 100;
	return UMAX(1, cost);
}

void do_buy_pet(CHAR_DATA * ch, const char *argument)
{
	int		cost, roll;
	char            arg[MAX_INPUT_LENGTH];
	CHAR_DATA	*pet;
	flag64_t		act;
	ROOM_INDEX_DATA *pRoomIndexNext;
	ROOM_INDEX_DATA *in_room;

	if (IS_NPC(ch))
		return;

	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Buy what?\n", ch);
		return;
	}

	pRoomIndexNext = get_room_index(ch->in_room->vnum + 1);
	if (pRoomIndexNext == NULL) {
		bug("Do_buy: bad pet shop at vnum %d.", ch->in_room->vnum);
		char_puts("Sorry, you can't buy that here.\n", ch);
		return;
	}
	in_room = ch->in_room;
	ch->in_room = pRoomIndexNext;
	pet = get_char_room(ch, arg);
	ch->in_room = in_room;

	if (!pet
	||  !IS_NPC(pet)
	||  !IS_SET(act = pet->pIndexData->act, ACT_PET)) {
		char_puts("Sorry, you can't buy that here.\n", ch);
		return;
	}

	if (!CLANS_CAN_BUY_PETS
	&& !IS_NPC(ch)
	&& !IS_IMMORTAL(ch)
	&& ch->clan != 0
	&& in_room->clan == 0) {
		char_puts("The merchant guild prohibits sales to clan members.\n", ch);
		return;
	}

	
	if (IS_SET(act, ACT_RIDEABLE)
	&&  get_skill(ch, gsn_riding) && !MOUNTED(ch)) {
		cost = 10 * pet->level * pet->level;

		if ((ch->silver + 100 * ch->gold) < cost) {
			char_puts("You can't afford it.\n", ch);
			return;
		}
		if (ch->level < pet->level + 5) {
			char_puts("You're not powerful enough "
				     "to master this pet.\n", ch);
			return;
		}
		deduct_cost(ch, cost);
		pet = create_mob(pet->pIndexData);
		pet->restricted_channels = ALL ^ CHAN_YELL;

		char_to_room(pet, ch->in_room);
		if (JUST_KILLED(pet))
			return;

		do_mount(ch, pet->name);
		char_puts("Enjoy your mount.\n", ch);
		act("$n bought $N as a mount.", ch, NULL, pet, TO_ROOM);
		return;
	}
	if (ch->pet != NULL) {
		char_puts("You already own a pet.\n", ch);
		return;
	}
	cost = 10 * pet->level * pet->level;

	if ((ch->silver + 100 * ch->gold) < cost) {
		char_puts("You can't afford it.\n", ch);
		return;
	}
	if (ch->level < pet->level) {
		char_puts("You're not powerful enough "
			     "to master this pet.\n", ch);
		return;
	}
	/* haggle */
	roll = number_percent();
	if (roll < get_skill(ch, gsn_haggle)) {
		cost -= cost / 2 * roll / 100;
		char_printf(ch, "You haggle the price down to %d coins.\n",
			    cost);
		check_improve(ch, gsn_haggle, TRUE, 4);
	}
	deduct_cost(ch, cost);

	pet = create_mob(pet->pIndexData);
	SET_BIT(pet->affected_by, AFF_CHARM);

	pet->restricted_channels = ALL ^ CHAN_YELL;

	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] != '\0')
		pet->name = str_printf(pet->pIndexData->name, arg);
	pet->description = mlstr_printf(pet->pIndexData->description, ch->name);

	char_to_room(pet, ch->in_room);
	if (JUST_KILLED(pet))
		return;

	add_follower(pet, ch);
	pet->leader = ch;
	ch->pet = pet;
	char_puts("Enjoy your pet.\n", ch);
	act("$n bought $N as a pet.", ch, NULL, pet, TO_ROOM);

	wiznet2(WIZ_ECONOMY, IM, 0, ch, "$N {gbought{x %s for {Y%d{x%02d.",
		CHAR_NAME(pet),
		cost / 100,
		cost % 100);
}

void do_buy(CHAR_DATA * ch, const char *argument)
{
	int		cost, roll;
	CHAR_DATA      *keeper;
	OBJ_DATA       *obj, *t_obj;
	char            arg[MAX_INPUT_LENGTH];
	uint		number, count = 1;
	int 		inflation = 100;
	WORLD_AFFECT_DATA *waff;

	if ((keeper = find_keeper(ch)) == NULL)
		return;

	if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP)) {
		do_buy_pet(ch, argument);
		return;
	}

	number = mult_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0' || number == 0) {
		char_puts("Buy what?\n", ch);
		return;
	}

	if (number < 0) {
		char_puts("What? Try sell instead.\n", ch);
		return;
	}

	if ((waff = ch_waffected(ch, WAFF_INFLATION)) != NULL) {
		if (waff->modifier <= 0) {
			BUG("World Affect inflation would cause negative prices."
			    "  Setting to 100% (no inflation).");
			waff->modifier = 100;
		}
		inflation = waff->modifier;
	}

	obj = get_obj_keeper(ch, keeper, arg);
	cost = get_cost(ch, keeper, obj, TRUE);
	cost = cost * inflation / 100;

	if (cost <= 0 || !can_see_obj(ch, obj)) {
		do_tell_raw(keeper, ch, "I don't sell that -- try 'list'");
		return;
	}
	if (!IS_OBJ_STAT(obj, ITEM_INVENTORY)) {
		for (t_obj = obj->next_content; count < number && t_obj != NULL;
		     t_obj = t_obj->next_content) {
			if (t_obj->pIndexData == obj->pIndexData
			&& !mlstr_cmp(t_obj->short_descr, obj->short_descr))
				count++;
			else
				break;
		}

		if (count < number) {
			act("$n tells you '{GI don't have that many in stock.{x'",
			    keeper, NULL, ch, TO_VICT);
			ch->reply = keeper;
			return;
		}
	}
	if ((ch->silver + ch->gold * 100) < cost * number) {
		if (number > 1)
			act("$n tells you '{GYou can't afford to buy that many.{x'",
			    keeper, obj, ch, TO_VICT);
		else
			act("$n tells you '{GYou can't afford to buy $p.{x'",
			    keeper, obj, ch, TO_VICT);
		ch->reply = keeper;
		return;
	}
	if (obj->level > get_wear_level(ch, obj)) {
		act("$n tells you '{GYou can't use $p yet.{x'",
		    keeper, obj, ch, TO_VICT);
		ch->reply = keeper;
		return;
	}
	if (!IS_IMMORTAL(ch)
	&& ch->carry_number + number * get_obj_number(obj) > can_carry_n(ch)) {
		char_puts("You can't carry that many items.\n", ch);
		return;
	}
	if (!IS_IMMORTAL(ch)
	&& ch->carry_weight + number * get_obj_weight(obj) > ch_max_carry_weight(ch)) {
		char_puts("You can't carry that much weight.\n", ch);
		return;
	}
	/* haggle */
	roll = number_percent();
	if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT)
	    && roll < get_skill(ch, gsn_haggle)) {
		cost -= obj->cost / 2 * roll / 100;
		act("You haggle with $N.", ch, NULL, keeper, TO_CHAR);
		check_improve(ch, gsn_haggle, TRUE, 4);
	}
	if (number > 1) {
		act_puts("$n buys $P[$j].",
			 ch, (const void*) number, obj, TO_ROOM, POS_RESTING);
		act_puts3("You buy $P[$j] for $J silver.",
			  ch, (const void*) number,
			  obj, (const void*) (cost*number),
			  TO_CHAR, POS_DEAD);
	} else {
		act("$n buys $P.", ch, NULL, obj, TO_ROOM);
		act_puts("You buy $P for $j silver.",
			 ch, (const void*) cost, obj, TO_CHAR, POS_DEAD);
	}
	wiznet2(WIZ_ECONOMY, IM, 0, ch, "$N {gbought{x %s for {Y%d{x%02d.",
		OBJ_SHORTNAME(obj),
		cost / 100,
		cost % 100);

	deduct_cost(ch, cost * number);
	keeper->gold += (cost * number / 100) * SHOP_MONEY_SINK_MOD / 100;
	keeper->silver += (cost * number - (cost * number / 100) * 100) 
		* SHOP_MONEY_SINK_MOD / 100;

	for (count = 0; count < number; count++) {
		if (IS_SET(obj->extra_flags, ITEM_INVENTORY))
			t_obj = create_obj(obj->pIndexData, 0);
		else {
			t_obj = obj;
			obj = obj->next_content;
			obj_from_char(t_obj);
		}

		if (t_obj->timer > 0 && !IS_OBJ_STAT(t_obj, ITEM_HAD_TIMER))
			t_obj->timer = 0;
		REMOVE_BIT(t_obj->extra_flags, ITEM_HAD_TIMER);
		obj_to_char(t_obj, ch);
		if (cost < t_obj->cost)
			t_obj->cost = cost;
	}
}

void do_list(CHAR_DATA * ch, const char *argument)
{
	int inflation = 100;
	WORLD_AFFECT_DATA *waff;

	if ((waff = ch_waffected(ch, WAFF_INFLATION)) != NULL) {
		if (waff->modifier <= 0) {
			BUG("World Affect inflation would cause negative prices."
			    "  Setting to 100% (no inflation).");
			waff->modifier = 100;
		}
		inflation = waff->modifier;
	}

	if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP)) {
		ROOM_INDEX_DATA *pRoomIndexNext;
		CHAR_DATA      *pet;
		bool            found;

		pRoomIndexNext = get_room_index(ch->in_room->vnum + 1);
		if (pRoomIndexNext == NULL) {
			bug("Do_list: bad pet shop at vnum %d.", ch->in_room->vnum);
			char_puts("You can't do that here.\n", ch);
			return;
		}
		found = FALSE;
		for (pet = pRoomIndexNext->people; pet; pet = pet->next_in_room) {
			if (!IS_NPC(pet))
				continue;	/* :) */
			if (IS_SET(pet->pIndexData->act, ACT_PET)) {
				if (!found) {
					found = TRUE;
					char_puts("Pets for sale:\n", ch);
				}
				char_printf(ch, "[%2d] %8d - %s\n",
					    pet->level,
					    10 * pet->level * pet->level * inflation/100,
					    format_short(pet->short_descr,
							 pet->name, ch));
			}
		}
		if (!found)
			char_puts("Sorry, we're out of pets right now.\n", ch);
		return;
	} else {
		CHAR_DATA      *keeper;
		OBJ_DATA       *obj;
		int             cost, count;
		bool            found;
		char            arg[MAX_INPUT_LENGTH];
		if ((keeper = find_keeper(ch)) == NULL)
			return;
		one_argument(argument, arg, sizeof(arg));

		found = FALSE;
		for (obj = keeper->carrying; obj; obj = obj->next_content) {
			if (obj->wear_loc == WEAR_NONE
			    && can_see_obj(ch, obj)
			    && (cost = get_cost(ch, keeper, obj, TRUE)) > 0
			    && (arg[0] == '\0'
				|| is_name(arg, obj->name))) {
				cost = cost * inflation / 100;
				if (!found) {
					found = TRUE;
					char_puts("[Lv Price Qty] Item\n", ch);
				}
				if (IS_OBJ_STAT(obj, ITEM_INVENTORY))
					char_printf(ch, "[%2d %5d -- ] %s\n",
						obj->level, cost,
						format_short(obj->short_descr,
							     obj->name, ch));
				else {
					count = 1;

					while (obj->next_content != NULL
					       && obj->pIndexData == obj->next_content->pIndexData
					       && !mlstr_cmp(obj->short_descr,
					   obj->next_content->short_descr)) {
						obj = obj->next_content;
						count++;
					}
					char_printf(ch, "[%2d %5d %2d ] %s\n",
						obj->level, cost, count,
						format_short(obj->short_descr,
							     obj->name, ch));
				}
			}
		}

		if (!found)
			char_puts("You can't buy anything here.\n", ch);
		return;
	}
}

void do_sell(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];
	CHAR_DATA      *keeper;
	CHAR_DATA      *guard;
	OBJ_DATA       *obj;
	int		cost, roll;
	uint		gold, silver;
	int		chance = 0;
	int		chance_cha = 0;
	int		chance_skill = 0;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Sell what?\n", ch);
		return;
	}
	if ((keeper = find_keeper(ch)) == NULL)
		return;

	if ((obj = get_obj_carry(ch, arg)) == NULL) {
		act("$n tells you '{GYou don't have that item.{x'",
		    keeper, NULL, ch, TO_VICT);
		ch->reply = keeper;
		return;
	}
	if (!can_drop_obj(ch, obj)) {
		char_puts("You can't let go of it.\n", ch);
		return;
	}
	if (!can_see_obj(keeper, obj)) {
		act("$n doesn't see what you are offering.", keeper, NULL, ch, TO_VICT);
		return;
	}

	/* can't sell stolen goods to same mob, in same area, or a law room */
	if (obj->stolen_from == keeper->pIndexData->vnum
	|| (obj->stolen_from > 0 
	&& keeper->in_room != NULL
	&& IS_SET(keeper->in_room->room_flags, ROOM_LAW))
	|| area_vnum_lookup(obj->stolen_from) 
		== area_vnum_lookup(keeper->pIndexData->vnum)) {
		if (obj->stolen_from == keeper->pIndexData->vnum) {
			act("$N eyes the $p carefully and $S eyes go wide.",
				ch, obj, keeper, TO_NOTVICT);
			doprintf(do_say, keeper,
				"Hey, this was stolen from my shop!");
		}
		else if (IS_SET(keeper->pIndexData->attr_flags, MOB_ATTR_BLACKMARKET)) {
			act("$N turns $p around in $S hands.  Looks over $S shoulder.",
				ch, obj, keeper, TO_NOTVICT);
			act("$N nervously hands $p back to you.",
				ch, obj, keeper, TO_NOTVICT);
			doprintf(do_whisper, keeper,
				"Not here.");
			return;
		}
		else {
			act("$N turns $p around in $S hands.  Slowly $S eyes narrow.",
				ch, obj, keeper, TO_NOTVICT);
			doprintf(do_say, keeper,
				"Hey, this was reported stolen nearby!");
		}

		chance = 10;
		chance = chance_cha = ((get_curr_stat(ch,STAT_CHA) - 50)/4) * 10;
		chance = chance_skill
			= chance * (get_skill(ch, gsn_guile) /100);

		DEBUG(DEBUG_SKILL_GUILE,
			"%s guile %s: cha:%+d skill:%d%% = %d",
			mlstr_mval(ch->short_descr),
			mlstr_mval(keeper->short_descr),
			chance_cha, chance_skill, chance);

		if (number_percent() < chance) {
			act("You pretend to be just as shocked as $N.",
				ch, NULL, keeper, TO_CHAR);
			act("$n seems just as shocked as $N.",
				ch, NULL, keeper, TO_ROOM);
			check_improve(ch, gsn_guile, TRUE, 1);
			return;
		}
		else {
			check_improve(ch, gsn_guile, FALSE, 3);
			if (obj->stolen_from == keeper->pIndexData->vnum) {
				act("$n snatches $p from your hands.",
					keeper, obj, ch, TO_VICT);
				act("$n snatches $p from $N's hands.",
					keeper, obj, ch, TO_NOTVICT);
				obj_from_char(obj);
				obj_to_keeper(obj, keeper);
				obj->stolen_from = 0;
				if (IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT))
					extract_obj(obj, 0);
			}

			doprintf(do_yell, keeper, 
				"Guards!  Apprehend this fool, %s!",
				PERS(ch, keeper));

			guard = summon_shop_guard(keeper, ch);

			if (number_percent() < 50)
				guard = summon_shop_guard(keeper, ch);

			multi_hit(guard, ch, TYPE_UNDEFINED);
			return;
		}
	}


	if ((cost = get_cost(ch, keeper, obj, FALSE)) <= 0) {
		act("$n looks uninterested in $p.", keeper, obj, ch, TO_VICT);
		return;
	}
	if ((obj->pIndexData->item_type == ITEM_GEM
	&& cost > (keeper->silver + 100 * keeper->gold)/100)
	|| cost > (keeper->silver + 100 * keeper->gold)) {
		act("$n tells you '{GI'm afraid I don't have enough wealth to buy $p.{x'",
		    keeper, obj, ch, TO_VICT);
		return;
	}
	act("$n sells $p.", ch, obj, NULL, TO_ROOM);

	/* haggle */
	roll = number_percent();
	if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT) 
	&& roll < get_skill(ch, gsn_haggle)) {
		roll = get_skill(ch, gsn_haggle) + number_range(1, 20) - 10;
		char_puts("You haggle with the shopkeeper.\n", ch);
		cost += obj->cost / 2 * roll / 100;
		cost = UMIN(cost, 95 * get_cost(ch, keeper, obj, TRUE) / 100);
		cost = UMIN(cost, (keeper->silver + 100 * keeper->gold));
		check_improve(ch, gsn_haggle, TRUE, 4);
	}
	silver = cost - (cost / 100) * 100;
	gold = cost / 100;

	if (obj->stolen_from != 0) {
		if (!IS_SET(keeper->pIndexData->attr_flags, MOB_ATTR_BLACKMARKET)) {
			doprintf(do_say, keeper, 
				"Well, I don't usually deal in items like this."
				"  Here, take this and be happy"
				" I don't turn you in.");

			gold = gold / 2;
			silver = silver / 2;
		}
		wiznet_theft(ch, keeper, NULL, obj, THEFT_PAWN, 
				gold *100 + silver, TRUE, obj->cost);
	}

	if (gold && silver) {
		act_puts3("You sell $P for $j gold and $J silver $qJ{pieces}.",
			  ch, (const void*) gold, obj, (const void*) silver,
			  TO_CHAR, POS_DEAD);
	}
	else if (gold) {
		act_puts("You sell $P for $j gold $qj{pieces}.",
			 ch, (const void*) gold, obj, TO_CHAR, POS_DEAD);
	}
	else if (silver) {
		act_puts("You sell $P for $j silver $qj{pieces}.",
			 ch, (const void*) silver, obj, TO_CHAR, POS_DEAD);
	}
	ch->gold += gold;
	ch->silver += silver;
	deduct_cost(keeper, cost/100);

	wiznet2(WIZ_ECONOMY, IM, 0, ch, "$N {gsold{x %s for {Y%d{x%02d.",
		OBJ_SHORTNAME(obj),
		gold,
		silver);

	if (obj->pIndexData->item_type == ITEM_TRASH
	||  IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT))
		extract_obj(obj, 0);
	else {
		obj_from_char(obj);
		if (obj->timer)
			SET_BIT(obj->extra_flags, ITEM_HAD_TIMER);
		else
			obj->timer = number_range(50, 100);
		obj_to_keeper(obj, keeper);
	}
}

void do_value(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];
	CHAR_DATA      *keeper;
	OBJ_DATA       *obj;
	int             cost;
	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Value what?\n", ch);
		return;
	}
	if ((keeper = find_keeper(ch)) == NULL)
		return;

	if ((obj = get_obj_carry(ch, arg)) == NULL) {
		act("$n tells you '{GYou don't have that item.{x'",
		    keeper, NULL, ch, TO_VICT);
		ch->reply = keeper;
		return;
	}
	if (!can_see_obj(keeper, obj)) {
		act("$n doesn't see what you are offering.", keeper, NULL, ch, TO_VICT);
		return;
	}
	if (!can_drop_obj(ch, obj)) {
		char_puts("You can't let go of it.\n", ch);
		return;
	}
	if ((cost = get_cost(ch, keeper, obj, FALSE)) <= 0) {
		act("$n looks uninterested in $p.", keeper, obj, ch, TO_VICT);
		return;
	}

	act_puts("$N tells you '{G", ch, NULL, keeper,
		 TO_CHAR | ACT_NOLF, POS_DEAD);
	act_puts3("I'll give you $j silver and $J gold $qJ{coins} for $P.",
		  ch, (const void*) (cost%100), obj, (const void*) (cost/100),
		  TO_CHAR | ACT_NOLF, POS_DEAD);
	act_puts("{x'", ch, NULL, NULL, TO_CHAR, POS_DEAD);
}

void do_herbs(CHAR_DATA * ch, const char *argument)
{
	CHAR_DATA      *victim;
	char            arg[MAX_INPUT_LENGTH];
	int		sn;

	if (IS_NPC(ch)
	||  (sn = sn_lookup("herbs")) < 0)
		return;

	one_argument(argument, arg, sizeof(arg));

	if (is_affected(ch, sn)) {
		char_puts("You can't find any more herbs.\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(sn)->beats);

	if (arg[0] == '\0')
		victim = ch;
	else if ((victim = get_char_room(ch, arg)) == NULL) {
		char_puts("They're not here.\n", ch);
		return;
	}

	if (ch->in_room->sector_type != SECT_INSIDE
	&&  ch->in_room->sector_type != SECT_CITY
	&&  number_percent() < get_skill(ch, sn)) {
		AFFECT_DATA     af;
		af.where = TO_AFFECTS;
		af.type = sn;
		af.level = ch->level;
		af.duration = 5;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.bitvector = 0;

		affect_to_char(ch, &af);

		char_puts("You gather some beneficial herbs.\n", ch);
		act("$n gathers some herbs.", ch, NULL, NULL, TO_ROOM);

		if (ch != victim) {
			act("$n gives you some herbs to eat.", ch, NULL, victim, TO_VICT);
			act("You give the herbs to $N.", ch, NULL, victim, TO_CHAR);
			act("$n gives the herbs to $N.", ch, NULL, victim, TO_NOTVICT);
		}
		if (victim->hit < victim->max_hit) {
			char_puts("You feel better.\n", victim);
			act("$n looks better.", victim, NULL, NULL, TO_ROOM);
		}
		victim->hit = UMIN(victim->max_hit, victim->hit + 5 * ch->level);
		check_improve(ch, sn, TRUE, 1);
		if (is_affected(victim, gsn_plague)) {
			if (check_dispel(ch->level, victim, gsn_plague)) {
				char_puts("Your sores vanish.\n", victim);
				act("$n looks relieved as $s sores vanish.",
				    victim, NULL, NULL, TO_ROOM);
			}
		}
	} else {
		char_puts("You search for herbs but find none here.\n", ch);
		act("$n looks around for herbs.", ch, NULL, NULL, TO_ROOM);
		check_improve(ch, sn, FALSE, 1);
	}
}

void do_lore_raw(CHAR_DATA *ch, OBJ_DATA *obj, BUFFER *output)
{
	int		chance;
	int		percent;
	int		value0, value1, value2, value3, value4;
	int		mana;
	int		max_skill;
	int		sn;

	if ((sn = sn_lookup("lore")) < 0
	||  (percent = get_skill(ch, sn)) < 10) {
		buf_add(output, "The meaning of this object escapes you for the moment.\n");
		return;
	}

	mana = SKILL(sn)->min_mana;
	if (ch->mana < mana) {
		buf_add(output, "You don't have enough mana.\n");
		return;
	}
	ch->mana -= mana;
	WAIT_STATE(ch, SKILL(sn)->beats);

	/* a random lore */
	chance = number_percent();

	if (percent < 20) {
		buf_printf(output, "Object '%s'.\n", obj->name);
		check_improve(ch, sn, TRUE, 8);
		return;
	}
	else if (percent < 40) {
		buf_printf(output,
			    "Object '%s'.  Weight is %d, value is %d.\n",
			    obj->name,
		chance < 60 ? obj->weight : number_range(1, 2 * obj->weight),
		     chance < 60 ? number_range(1, 2 * obj->cost) : obj->cost
			);
		buf_printf(output, "Material is %s.\n", 
			(obj->material_descr) ? obj->material_descr 
						: obj->material->name);
		check_improve(ch, sn, TRUE, 7);
		return;
	}
	else if (percent < 60) {
		buf_printf(output,
			    "Object '%s' has weight %d.\nValue is %d, level is %d.\nMaterial is %s.\n",
			    obj->name,
			    obj->weight,
		    chance < 60 ? number_range(1, 2 * obj->cost) : obj->cost,
		  chance < 60 ? obj->level : number_range(1, 2 * obj->level),
			(obj->material_descr) ? obj->material_descr 
						: obj->material->name
			);
		check_improve(ch, sn, TRUE, 6);
		return;
	}
	else if (percent < 80) {
		buf_printf(output,
			    "Object '%s' is type %s, extra flags %s.\nWeight is %d, value is %d, level is %d.\nMaterial is %s.\n",
			    obj->name,
			    flag_string(item_types, obj->pIndexData->item_type),
			    flag_string(extra_flags, obj->extra_flags),
			    obj->weight,
		    chance < 60 ? number_range(1, 2 * obj->cost) : obj->cost,
		  chance < 60 ? obj->level : number_range(1, 2 * obj->level),
			(obj->material_descr) ? obj->material_descr 
						: obj->material->name
			);
		check_improve(ch, sn, TRUE, 5);
		return;
	}
	else if (percent < 85) 
		buf_printf(output,
			    "Object '%s' is type %s, extra flags %s.\nWeight is %d, value is %d, level is %d.\nMaterial is %s.\n",
			    obj->name,
			    flag_string(item_types, obj->pIndexData->item_type),
			    flag_string(extra_flags, obj->extra_flags),
			    obj->weight,
			    obj->cost,
			    obj->level,
			(obj->material_descr) ? obj->material_descr 
						: obj->material->name
			);
	else
		buf_printf(output,
			    "Object '%s' is type %s, extra flags %s.\nWeight is %d, value is %d, level is %d.\nMaterial is %s.\n",
			    obj->name,
			    flag_string(item_types, obj->pIndexData->item_type),
			    flag_string(extra_flags, obj->extra_flags),
			    obj->weight,
			    obj->cost,
			    obj->level,
			(obj->material_descr) ? obj->material_descr 
						: obj->material->name
			);

	value0 = obj->value[0];
	value1 = obj->value[1];
	value2 = obj->value[2];
	value3 = obj->value[3];
	value4 = obj->value[4];

	max_skill = skills.nused;

	switch (obj->pIndexData->item_type) {
	case ITEM_SCROLL:
	case ITEM_POTION:
	case ITEM_PILL:
		if (percent < 85) {
			value0 = number_range(1, 60);
			if (chance > 40) {
				value1 = number_range(1, (max_skill - 1));
				if (chance > 60) {
					value2 = number_range(1, (max_skill - 1));
					if (chance > 80)
						value3 = number_range(1, (max_skill - 1));
				}
			}
		}
		else {
			if (chance > 60) {
				value1 = number_range(1, (max_skill - 1));
				if (chance > 80) {
					value2 = number_range(1, (max_skill - 1));
					if (chance > 95)
						value3 = number_range(1, (max_skill - 1));
				}
			}
		}

		buf_printf(output, "Level %d spells of:", 
			obj->value[ITEM_POTION_LEVEL]);
		if (value1 >= 0)
			buf_printf(output, " '%s'", skill_name(value1));
		if (value2 >= 0)
			buf_printf(output, " '%s'", skill_name(value2));
		if (value3 >= 0)
			buf_printf(output, " '%s'", skill_name(value3));
		if (value4 >= 0)
			buf_printf(output, " '%s'", skill_name(value4));
		buf_add(output, ".\n");
		break;

	case ITEM_WAND:
	case ITEM_STAFF:
		if (percent < 85) {
			value0 = number_range(1, 60);
			if (chance > 40) {
				value3 = number_range(1, (max_skill - 1));
				if (chance > 60) {
					value2 = number_range(0, 
						2 * obj->value[ITEM_WAND_CHARGES_REMAINING]);
					if (chance > 80)
						value1 = number_range(0, value2);
				}
			}
		}
		else {
			if (chance > 60) {
				value3 = number_range(1, (max_skill - 1));
				if (chance > 80) {
					value2 = number_range(0, 2 * obj->value[ITEM_WAND_CHARGES_REMAINING]);
					if (chance > 95)
						value1 = number_range(0, value2);
				}
			}
		}

		buf_printf(output, "Has %d(%d) charges of level %d '%s'.\n",
			    value1, value2, value0, skill_name(value3));
		break;

	case ITEM_WEAPON:
		buf_add(output, "Weapon type is ");
		if (percent < 85) {
			value0 = number_range(0, 8);
			if (chance > 33) {
				value1 = number_range(1, 2 * obj->value[ITEM_WEAPON_DICE_NUM]);
				if (chance > 66)
					value2 = number_range(1, 2 * obj->value[ITEM_WEAPON_DICE_SIZE]);
			}
		}
		else {
			if (chance > 50) {
				value1 = number_range(1, 2 * obj->value[ITEM_WEAPON_DICE_NUM]);
				if (chance > 75)
					value2 = number_range(1, 2 * obj->value[ITEM_WEAPON_DICE_SIZE]);
			}
		}

		buf_printf(output, "%s.\n", flag_string(weapon_class, value0));

		buf_printf(output, "Damage is %dd%d (average %d).\n",
			    value1, value2,
			    (1 + value2) * value1 / 2);
		break;

	case ITEM_ARMOR:
		if (percent < 85) {
			if (chance > 25) {
				value2 = number_range(0, 2 * obj->value[ITEM_ARMOR_AC_SLASH]);
				if (chance > 45) {
					value0 = number_range(0, 2 * obj->value[ITEM_ARMOR_AC_EXOTIC]);
					if (chance > 65) {
						value3 = number_range(0, 2 * obj->value[ITEM_ARMOR_AC_PIERCE]);
						if (chance > 85)
							value1 = number_range(0, 2 * obj->value[ITEM_ARMOR_AC_BASH]);
					}
				}
			}
		}
		else {
			if (chance > 45) {
				value2 = number_range(0, 2 * obj->value[ITEM_ARMOR_AC_SLASH]);
				if (chance > 65) {
					value0 = number_range(0, 2 * obj->value[ITEM_ARMOR_AC_PIERCE]);
					if (chance > 85) {
						value3 = number_range(0, 2 * obj->value[ITEM_ARMOR_AC_EXOTIC]);
						if (chance > 95)
							value1 = number_range(0, 2 * obj->value[ITEM_ARMOR_AC_BASH]);
					}
				}
			}
		}

		buf_printf(output,
			    "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic.\n",
			    value0, value1, value2, value3);
		break;
	}

	if (percent < 87) {
		check_improve(ch, sn, TRUE, 5);
		return;
	}

	if (!IS_SET(obj->extra_flags, ITEM_ENCHANTED))
		format_obj_affects(output, obj->pIndexData->affected,
				   FOA_F_NODURATION);
	format_obj_affects(output, obj->affected, 0);
	check_improve(ch, sn, TRUE, 5);
}

void do_lore(CHAR_DATA *ch, const char *argument)
{
	char		arg[MAX_INPUT_LENGTH];
	BUFFER *	output;
	OBJ_DATA *	obj;

	argument = one_argument(argument, arg, sizeof(arg));
	if ((obj = get_obj_carry(ch, arg)) == NULL) {
		char_puts("You do not have that object.\n", ch);
		return;
	}

	output = buf_new(-1);
	do_lore_raw(ch, obj, output);
	page_to_char(buf_string(output), ch);
	buf_free(output);
}

void do_butcher(CHAR_DATA * ch, const char *argument)
{
	OBJ_DATA       *obj;
	char            arg[MAX_STRING_LENGTH];
	OBJ_DATA       *tmp_obj;
	OBJ_DATA       *tmp_next;
	int		sn;
	int		chance;

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Butcher what?\n", ch);
		return;
	}
	if ((obj = get_obj_here(ch, arg)) == NULL) {
		char_puts("You do not see that here.\n", ch);
		return;
	}
	if (obj->pIndexData->item_type != ITEM_CORPSE_PC
	&&  obj->pIndexData->item_type != ITEM_CORPSE_NPC) {
		char_puts("You can't butcher that.\n", ch);
		return;
	}

	if (obj->pIndexData->item_type == ITEM_CORPSE_PC
	&& !IS_OWNER(ch, obj)
	&& obj->contains != NULL) {
		char_puts("You need to strip the body first.\n", ch);
		return;
	}

	if (obj->carried_by != NULL) {
		char_puts("Put it down first.\n", ch);
		return;
	}

	if ((sn = sn_lookup("butcher")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("You don't have the precision instruments "
			     "for that.", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(sn)->beats);

	obj_from_room(obj);

	for (tmp_obj = obj->contains; tmp_obj != NULL; tmp_obj = tmp_next) {
		tmp_next = tmp_obj->next_content;
		obj_from_obj(tmp_obj);
		obj_to_room(tmp_obj, ch->in_room);
	}

	if (number_percent() < chance) {
		int             numsteaks;
		int             i;
		OBJ_DATA       *steak;
		numsteaks = number_bits(2) + 1;

		act("$n butchers $P and creates $j $qj{steaks}.",
		    ch, (const void*) numsteaks, obj, TO_ROOM);
		act_puts("You butcher $P and create $j $qj{steaks}.",
			 ch, (const void*) numsteaks, obj,
			 TO_CHAR, POS_DEAD);
		check_improve(ch, sn, TRUE, 1);

		for (i = 0; i < numsteaks; i++) {
			steak = create_obj_of(get_obj_index(OBJ_VNUM_STEAK),
					      obj->short_descr);
			obj_to_room(steak, ch->in_room);
		}
	} else {
		act("You fail and destroy $p.", ch, obj, NULL, TO_CHAR);
		act("$n fails to butcher $p and destroys it.",
		    ch, obj, NULL, TO_ROOM);

		check_improve(ch, sn, FALSE, 1);
	}
	extract_obj(obj, 0);
}

void do_scalp(CHAR_DATA *ch, const char *argument)
{
	char            arg[MAX_STRING_LENGTH];
	OBJ_DATA	*obj;
	OBJ_DATA	*scalp;
	char		mesg[MAX_STRING_LENGTH];
	int		sn, chance;

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Scalp what?\n", ch);
		return;
	}
	if ((obj = get_obj_here(ch, arg)) == NULL) {
		char_puts("You do not see that here.\n", ch);
		return;
	}

	if ((sn = sn_lookup("scalp")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("What a ghastly thought.", ch);
		return;
	}

	if (obj->pIndexData->item_type != ITEM_CORPSE_PC
	&&  obj->pIndexData->item_type != ITEM_CORPSE_NPC) {
		char_puts("You can't scalp that.\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(sn)->beats);

	if (obj->pIndexData->item_type == ITEM_CORPSE_NPC
	|| obj->killer == NULL) {
		char_puts("That corpse isn't worthy of scalping.\n", ch);
		return;
	}

	if (mlstr_cmp(ch->short_descr, obj->killer)) {
		char_puts("Even you're not low enough to scalp someone else's kill.\n",
			ch);
		return;
	}

	scalp = create_obj_of(get_obj_index(OBJ_VNUM_PC_SCALP),
			      obj->owner);

	name_add(&scalp->name, mlstr_mval(obj->owner), NULL, NULL);

	snprintf(mesg, sizeof(mesg),
		"This was cut off the fallen corpse of {D%s{x of rank {W%d{x "
		"on %s.",
		mlstr_mval(obj->owner),
		obj->level,
		strtime(current_time));

	scalp->writtenDesc = mlstr_new(mesg);

	obj_to_char(scalp, ch);

	scalp->killer = obj->killer;
	obj->killer = NULL;

	act("With a flick of the wrist, $n slices off $p.",
		ch, scalp, NULL, TO_ROOM);
	act("You take a small pairing knife and slice off $p.",
		ch, scalp, NULL, TO_CHAR);

	check_improve(ch, sn, TRUE, 1);
}

void do_crucify(CHAR_DATA *ch, const char *argument)
{	
	OBJ_DATA *obj;
	char arg[MAX_STRING_LENGTH];
	OBJ_DATA *obj2, *next;
	OBJ_DATA *cross;
	int sn;
	int chance;
	AFFECT_DATA af;

	sn = sn_lookup("crucify");

	if ((chance = get_skill(ch, sn)) == 0) {
		char_puts("Oh no, you can't do that.\n", ch);
		return;
	}

	if (is_affected(ch, sn)) {
		char_puts("You are not yet ready to make a sacrifice.\n", ch);
		return;
	}

	/* if (IS_PUMPED(ch) 
	|| IS_AFFECTED(ch, AFF_BERSERK) 
	|| is_affected(ch, gsn_frenzy)) {
		char_puts("Calm down first.\n", ch);
		return;
	} */

	one_argument(argument, arg, sizeof(arg));

	if (arg == '\0') {
		char_puts("Crucify what?\n", ch);
		return;
	}

	if ((obj = get_obj_here(ch, arg)) == NULL) {
		char_puts("You do not see that here.", ch);
		return;
	}

	if (obj->pIndexData->item_type != ITEM_CORPSE_PC
	 && obj->pIndexData->item_type != ITEM_CORPSE_NPC) {
		char_puts("You cannot crucify that.\n", ch);
		return;
	 }

	if (obj->pIndexData->item_type == ITEM_CORPSE_PC
	&& !IS_OWNER(ch, obj)
	&& obj->contains != NULL) {
		char_puts("You need to strip the body first.\n", ch);
		return;
	}

	if (obj->carried_by != NULL) {
		char_puts("Put it down first.\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(sn)->beats);

	obj_from_room(obj);
	for(obj2 = obj->contains; obj2; obj2 = next) {
		next = obj2->next_content;
		obj_from_obj(obj2);
		obj_to_room(obj2, ch->in_room);
	}
	
	if (number_percent() > chance) {
		act("You attempt a ritual crucification of $p, "
		   "but fail and ruin it.", ch, obj, NULL, TO_CHAR);
		act("$n attempts to crucify $p, but fails and ruins it.",
		    ch, obj, NULL, TO_ROOM);
		extract_obj(obj, 0);
		check_improve(ch, sn, FALSE, 1);
	}
	else {
		cross = create_obj_of(get_obj_index(OBJ_VNUM_CROSS),
				      obj->owner);
		cross->timer = 20 * ch->level;
		obj_to_room(cross, ch->in_room);
		act("With a crunch of bone and splash of blood you nail "
		    "$p to a sacrificial cross.", ch, obj, NULL, TO_CHAR);
		act("With a crunch of bone and splash of blood $n nails "
		    "$p to a sacrificial cross.", ch, obj, NULL, TO_ROOM);
		char_puts("You are filled with a dark energy.\n", ch);
		ch->hit += obj->level*3/2;
		af.where 	= TO_AFFECTS;
		af.type  	= sn;
		af.level	= ch->level;
		af.duration	= 15;
		af.modifier	= UMAX(1, obj->level/8);
		af.bitvector	= AFF_BERSERK;

		af.location	= APPLY_HITROLL;
		affect_to_char(ch, &af);

		af.location	= APPLY_DAMROLL;
		affect_to_char(ch, &af);

		af.modifier 	= UMAX(10, obj->level);
		af.location	= APPLY_AC;

		affect_to_char(ch, &af);
		extract_obj(obj, 0);
		check_improve(ch, sn, TRUE, 1);
	}
}

void do_balance(CHAR_DATA * ch, const char *argument)
{
	char buf[160];
	int bank_g;
	int bank_s;

	if (IS_NPC(ch)) {
		char_puts("You don't have a bank account.\n", ch);
		return;
	}
	if (!IS_SET(ch->in_room->room_flags, ROOM_BANK)) {
		char_puts("You are not in a bank.\n", ch);
		return;
	}

	if (ch->pcdata->bank_s + ch->pcdata->bank_g == 0) {
		char_puts("You don't have any money in the bank.\n", ch);
		return;
	}

	bank_g = ch->pcdata->bank_g;
	bank_s = ch->pcdata->bank_s;
	snprintf(buf, sizeof(buf), "You have %s%s%s coin%s in the bank.\n",
		bank_g ? "%ld gold" : str_empty,
		(bank_g) && (bank_s) ? " and " : str_empty,
		bank_s ? "%ld silver" : str_empty,
		bank_s + bank_g > 1 ? "s" : str_empty);
	if (bank_g == 0)
		char_printf(ch, buf, bank_s);
	else
		char_printf(ch, buf, bank_g, bank_s);
}

void do_withdraw(CHAR_DATA * ch, const char *argument)
{
	int	amount;
	int	fee;
	bool	silver = FALSE;
	char	arg[MAX_INPUT_LENGTH];

	if (IS_NPC(ch)) {
		char_puts("You don't have a bank account.\n", ch);
		return;
	}

	if (!IS_SET(ch->in_room->room_flags, ROOM_BANK)) {
		char_puts("The mosquito by your feet "
			  "will not give you any money.\n", ch);
		return;
	}

	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Withdraw how much?\n", ch);
		return;
	}

	amount = atoi(arg);
	if (amount <= 0) {
		char_puts("What?\n", ch);
		return;
	}

	if (!str_cmp(argument, "silver"))
		silver = TRUE;
	else if (str_cmp(argument, "gold") && argument[0] != '\0') {
		char_puts("You can withdraw gold and silver coins only.", ch);
		return;
	}

	fee = UMAX(1, amount * (silver ? 10 : 2) / 100);
	
	if ((silver && amount+fee > ch->pcdata->bank_s)
	||  (!silver && amount+fee > ch->pcdata->bank_g)) {
		char_puts("Sorry, we don't give loans.\n", ch);
		return;
	}

	if (ch_weight_carried(ch) + (amount - fee) * (silver ? 4 : 1) / 10 >
		ch_max_carry_weight(ch)) {
		char_puts("You can't carry that weight.\n", ch);
		return;
	}

	if (silver) {
		ch->silver += amount;
		ch->pcdata->bank_s -= amount + fee;
		if (amount > WIZNET_ECONOMY_DEPOSIT_SILVER_MIN)
			wiznet2(WIZ_ECONOMY, IM, 0, ch,
				"$N {gwithdrew{x %d{x silver.", amount);
	}
	else {
		ch->gold += amount;
		ch->pcdata->bank_g -= amount + fee;
		if (amount > WIZNET_ECONOMY_DEPOSIT_GOLD_MIN)
			wiznet2(WIZ_ECONOMY, IM, 0, ch,
				"$N {gwithdrew{x {Y%d{x gold.", amount);
	}

	char_printf(ch,
		    "Here are your {%c%d{x %s coin(s).\n"
		    "A {%c%d{x coin(s) withdrawal fee has been charged to your account.\n",
		    silver ? 'W' : 'Y',
		    amount, 
		    silver ? "silver" : "gold", 
		    silver ? 'W' : 'Y',
		    fee);
	act("$n steps up to the teller window.", ch, NULL, NULL, TO_ROOM);
}

void do_deposit(CHAR_DATA * ch, const char *argument)
{
	int	amount;
	bool	silver = FALSE;
	char	arg[MAX_INPUT_LENGTH];

	if (IS_NPC(ch)) {
		char_puts("You don't have a bank account.\n", ch);
		return;
	}

	if (!IS_SET(ch->in_room->room_flags, ROOM_BANK)) {
		char_puts("The ant by your feet can't carry your gold.\n", ch);
		return;
	}

	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Deposit how much?\n", ch);
		return;
	}

	amount = atoi(arg);
	if (amount <= 0) {
		char_puts("What?\n", ch);
		return;
	}

	if (!str_cmp(argument, "silver"))
		silver = TRUE;
	else if (str_cmp(argument, "gold") && argument[0] != '\0') {
		char_puts("You can deposit gold and silver coins only.", ch);
		return;
	}

	if ((silver && amount > ch->silver)
	||  (!silver && amount > ch->gold)) {
		char_puts("That's more than you've got.\n", ch);
		return;
	}

	if (silver) {
		ch->pcdata->bank_s += amount;
		ch->silver -= amount;
		if (amount > WIZNET_ECONOMY_DEPOSIT_SILVER_MIN)
			wiznet2(WIZ_ECONOMY, IM, 0, ch,
				"$N {gdeposited{x %d silver.", amount);
	}
	else {
		ch->pcdata->bank_g += amount;
		ch->gold -= amount;
		if (amount > WIZNET_ECONOMY_DEPOSIT_GOLD_MIN)
			wiznet2(WIZ_ECONOMY, IM, 0, ch,
				"$N {gdeposited{x {Y%d{x gold.", amount);
	}

	if (amount == 1)
		char_printf(ch, "Oh boy! One %s{x coin!\n",
			    silver ? "{Wsilver" : "{Ygold");
	else
		char_printf(ch, "{%c%d{x %s coins deposited. Come again soon!\n",
			    silver ? 'W' : 'Y',
			    amount, 
			    silver ? "silver" : "gold");
	act("$n steps up to the teller window.", ch, NULL, NULL, TO_ROOM);
}

/* wear object as a secondary weapon */
void do_second_wield(CHAR_DATA * ch, const char *argument)
{
	OBJ_DATA *	obj;
	int		skill;
	int		wear_lev;

	if (get_skill(ch, gsn_second_weapon) == 0) {
		char_puts("You don't know how to wield a second weapon.\n", ch);
		return;
	}
	if (argument[0] == '\0') {
		char_puts("Wear which weapon in your off-hand?\n", ch);
		return;
	}
	obj = get_obj_carry(ch, argument);
	if (obj == NULL) {
		char_puts("You don't have that item.\n", ch);
		return;
	}
	if (!CAN_WEAR(obj, ITEM_WIELD)) {
		char_puts("You can't wield that as your second weapon.\n", ch);
		return;
	}
	if ((get_eq_char(ch, WEAR_SHIELD) != NULL) ||
	    (get_eq_char(ch, WEAR_HOLD) != NULL)) {
		char_puts("You cannot use a secondary weapon while using a shield or holding an item.\n", ch);
		return;
	}
	wear_lev = get_wear_level(ch, obj);
	if (wear_lev < obj->level) {
		char_printf(ch, "You must be level %d to use this object.\n",
			    wear_lev);
		act("$n tries to use $p, but is too inexperienced.",
		    ch, obj, NULL, TO_ROOM);
		return; 
	}
	if (IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS) && ch->size < SIZE_LARGE) {
		char_puts("You can't dual wield a two-handed weapon!\n", ch);
		return;
	}
	if (get_eq_char(ch, WEAR_WIELD) == NULL) {
		char_puts("You need to wield a primary weapon, before using a secondary one!\n", ch);
		return;
	}
	if (!get_stat_str_can_wield(ch, obj, HAND_SECONDARY)) {
		char_puts("This weapon is too heavy to be used as a secondary weapon by you.\n", ch);
		return;
	}

	if (IS_WEAPON_STAT(get_eq_char(ch, WEAR_WIELD), WEAPON_TWO_HANDS) 
	&& ch->size < SIZE_LARGE) {
		char_puts("You're already wielding a two-handed weapon.\n", ch);
		return;
	}

	if (!can_dual_weapons(get_eq_char(ch, WEAR_WIELD), obj)) {
		char_puts("You find that weapon combination unmanageable.\n", ch);
		return;
	}

	if (!remove_obj(ch, WEAR_SECOND_WIELD, TRUE))
		return;

	act("$n wields $p in $s off-hand.", ch, obj, NULL, TO_ROOM);
	act("You wield $p in your off-hand.", ch, obj, NULL, TO_CHAR);
	obj = equip_char(ch, obj, WEAR_SECOND_WIELD);
	if (obj == NULL)
		return;

	skill = get_weapon_skill(ch, get_weapon_sn(obj));

	if (skill >= 100)
		act("$p feels like a part of you!", ch, obj, NULL, TO_CHAR);
	else if (skill > 85)
		act("You feel quite confident with $p.", ch, obj, NULL, TO_CHAR);
	else if (skill > 70)
		act("You are skilled with $p.", ch, obj, NULL, TO_CHAR);
	else if (skill > 50)
		act("Your skill with $p is adequate.", ch, obj, NULL, TO_CHAR);
	else if (skill > 25)
		act("$p feels a little clumsy in your hands.", ch, obj, NULL, TO_CHAR);
	else if (skill > 1)
		act("You fumble and almost drop $p.", ch, obj, NULL, TO_CHAR);
	else
		act("You don't even know which end is up on $p.",
		    ch, obj, NULL, TO_CHAR);
}
/*
 * This idea here is that only certain weapons combos can be dual wielded
 */
bool can_dual_weapons(OBJ_DATA *firstObj, OBJ_DATA *secondObj) {
	int first, second; 

	if (firstObj == NULL || secondObj == NULL) 
		return FALSE;

	first = firstObj->value[ITEM_WEAPON_TYPE];
	second = secondObj->value[ITEM_WEAPON_TYPE];

	if (first == WEAPON_KATANA
	&& second != WEAPON_SHORTSWORD)
		return FALSE;

	switch (first) {
		case WEAPON_BASTARDSWORD:
			switch(second) {
				case WEAPON_LONGSWORD:
				case WEAPON_SHORTSWORD:
				case WEAPON_DAGGER:
				case WEAPON_MACE:
					return TRUE;
			}
			break;
		case WEAPON_LONGSWORD:
		case WEAPON_KATANA:
			switch(second) {
				case WEAPON_SHORTSWORD:
				case WEAPON_DAGGER:
					return TRUE;
			}
			break;
		case WEAPON_SHORTSWORD:
			if (second == WEAPON_DAGGER)
				return TRUE;
			break;
		case WEAPON_SPEAR:
			switch(second) {
				case WEAPON_SHORTSWORD:
				case WEAPON_DAGGER:
				case WEAPON_FLAIL:
				case WEAPON_MACE:
					return TRUE;
			}
			break;
		case WEAPON_STAFF:
		case WEAPON_AXE:
		case WEAPON_MACE:
		case WEAPON_WHIP:
			switch(second) {
				case WEAPON_SHORTSWORD:
				case WEAPON_DAGGER:
				case WEAPON_MACE:
				case WEAPON_AXE:
					return TRUE;
			}
		case WEAPON_FLAIL:
			switch(second) {
				case WEAPON_SHORTSWORD:
				case WEAPON_DAGGER:
				case WEAPON_MACE:
				case WEAPON_AXE:
					return TRUE;
			}
			break;
		case WEAPON_DAGGER:
			if (second == WEAPON_DAGGER)
				return TRUE;
			break;

		case WEAPON_POLEARM:
		case WEAPON_LANCE:
		case WEAPON_SHURIKEN:
		case WEAPON_EXOTIC:
			break;
	}
	return FALSE;
}

void do_enchant(CHAR_DATA * ch, const char *argument)
{
	OBJ_DATA *	obj;
	int		chance;
	int		sn;
	int		wear_level;

	if ((sn = sn_lookup("enchant sword")) < 0
	||  (chance = get_skill(ch, sn)) == 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	if (argument[0] == '\0') {	/* empty */
		char_puts("Which weapon do you want to enchant?\n", ch);
		return;
	}
	obj = get_obj_carry(ch, argument);

	if (obj == NULL) {
		char_puts("You don't have that item.\n", ch);
		return;
	}

	wear_level = get_wear_level(ch, obj);
	if (wear_level < obj->level) {
		char_printf(ch, "You must be level %d to be able to enchant this object.\n",
			    obj->level - wear_level + ch->level);
		act("$n tries to enchant $p, but is too inexperienced.",
		    ch, obj, NULL, TO_ROOM);
		return; 
	}
	if (ch->mana < 100) {
		char_puts("You don't have enough mana.\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(sn)->beats);
	if (number_percent() > chance) {
		char_puts("You lost your concentration.\n", ch);
		act("$n tries to enchant $p, but he forgets how for a moment.",
		    ch, obj, NULL, TO_ROOM);
		check_improve(ch, sn, FALSE, 6);
		ch->mana -= 50;
		return;
	}
	ch->mana -= 100;
	spell_enchant_weapon(24, ch->level, ch, obj, TARGET_OBJ);
	check_improve(ch, sn, TRUE, 2);
}

void do_label(CHAR_DATA* ch, const char *argument)
{
	OBJ_DATA *obj;
	const char *p;
	char obj_name[MAX_INPUT_LENGTH];
	char label[MAX_INPUT_LENGTH];
	
	if (IS_NPC(ch))
		return;

	argument = one_argument(argument, obj_name, sizeof(obj_name));
	first_arg(argument, label, sizeof(label), FALSE);
	
	if (!obj_name[0]) {
		char_puts("Label what?\n",ch);
		return;
	}

	if (!label[0]) {
		char_puts("How do you want to label it?\n",ch);
		return;
	}

	if ((obj = get_obj_carry(ch, obj_name)) == NULL) {
		char_puts("You don't have that object.\n", ch);
		return;
	}
	
	if (ch->pcdata->questpoints < 10) {
		char_puts("You do not have enough questpoints for labeling.\n",
			  ch);
		return;
	}

	if ((strlen(obj->name) + strlen(label) + 2) >= MAX_STRING_LENGTH) {
		char_puts("You can't label this object with this label.\n", ch);
		return;
	}

	p = obj->name;
	obj->name = str_printf("%s %s", obj->name, label);
	free_string(p);
	
	ch->pcdata->questpoints -= 10;
	char_puts("Ok.\n",ch);
}


bool can_write_on(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if(obj->pIndexData->item_type == ITEM_AWARD
	|| (obj->pIndexData->item_type == ITEM_PARCHMENT
	&& !(IS_SET(obj->value[ITEM_PARCHMENT_FLAGS], WRITE_PERM)
	&& (obj->writtenDesc != NULL
	|| !mlstr_null(obj->writtenDesc)))))
		return TRUE;
	return FALSE;
}

void do_write(CHAR_DATA *ch, const void *argument)
{
	OBJ_DATA *obj;
	char arg[MAX_INPUT_LENGTH];
	bool found;
        
        argument = one_argument(argument, arg, sizeof(arg));

	if(ch->position <= POS_SLEEPING){
		switch(ch->position){
		case POS_SLEEPING:
			char_puts("In your dreams or what?\n",ch);
			return;
		case POS_DEAD:
			char_puts("The dead don't write!\n",ch);
			return;
		case POS_STUNNED:
			char_puts("You're too stunned to do that.\n",ch);
			return;
		}
	}
	if(arg[0] == '\0')
	{
		char_puts("What do you wish to write on?\n",ch);
		return;
	}
	found = FALSE;
	for(obj = ch->carrying; obj != NULL; obj = obj->next_content){
		if(is_name(arg, obj->name)
		   && can_see_obj(ch, obj)
		   && obj->wear_loc == WEAR_NONE){
			found = TRUE;
			break;
		}
	}
	if(!found){
		char_puts("You don't have that.\n",ch);
		return;
	}
	if(!can_write_on(ch, obj)){
		act_puts("Alas, your scribblings on $p are to no avail; it cannot be written on.", 
			ch, obj, NULL, TO_CHAR, POS_DEAD);
		return;
	}
	
	act_puts("You begin writing on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
	act("$n begins writing on $p.", ch, obj, NULL, TO_ROOM);
	string_append(ch, mlstr_convert(&obj->writtenDesc, -1));
	return;
}

bool donate_obj (CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *pit) 
{
	if (obj == NULL ) {
		char_puts("Donate what?\n",ch);
		return FALSE;
	}

	if (ch->position == POS_FIGHTING) {
		char_puts("You're {Yfighting!{x\n",ch);
		return FALSE;
	}

	if (IS_PUMPED(ch)) {
		char_puts("You're too excited to donate something.\n", ch);
		return FALSE;
	}

	if (!can_drop_obj(ch, obj) && ch->level < 91) {
		char_puts("Its stuck to you.\n",ch);
		return FALSE;
	}

	if ((obj->pIndexData->item_type == ITEM_CORPSE_NPC) 
	|| (obj->pIndexData->item_type == ITEM_CORPSE_PC)) {
		char_puts("You cannot donate that!\n",ch);
		return FALSE;
	}

	if (obj->timer > 0
	|| IS_SET(obj->pIndexData->extra_flags, ITEM_CLAN)
	|| IS_SET(obj->pIndexData->extra_flags, ITEM_QUEST)) {
		act("You cannot donate $p.\n",
			ch, obj, NULL, TO_CHAR);
		return FALSE;
	}

	if (ch->in_room != get_room_index(ROOM_VNUM_PIT))
		act("$n donates {Y$p{x.", 
			ch, obj, NULL, TO_ROOM);

	act("You donate {Y$p{x.",ch,obj,NULL,TO_CHAR);

	/* pit lookup */
	if (obj->timer)
		SET_BIT(obj->extra_flags, ITEM_HAD_TIMER);
	else
		obj->timer = number_range(100, 200);

	obj_from_char(obj);

	if (pit)
		obj_to_obj(obj, pit);
	else {
		
		BUG("%s tried to donate an object to a NULL pit.",
			ch->name);
		extract_obj(obj, 0);
	}

	return TRUE;
}

void do_donate( CHAR_DATA *ch, const char *argument)
{
	OBJ_DATA *pit;
	OBJ_DATA *obj;
	altar_t *altar = NULL;
	char arg[MAX_INPUT_LENGTH];
	//int amount;

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0' ) {
		send_to_char("Donate what?\n",ch);
		return;
	}

	if ((obj = get_obj_carry (ch, arg)) == NULL) {
		send_to_char("You do not have that!\n",ch); 
		return;
	}

	pit = NULL;
	altar = get_altar(ch);
	for (pit = altar->room->contents; pit; pit = pit->next_content) { 
		if (pit->pIndexData == altar->pit)
		break;
	}

	if (pit == NULL) {
		BUG("%s doesn't have donation pit.",
			ch->name);
		char_puts("Hmm, can't find your pit right now.", ch);
		return;
	}

	if (donate_obj(ch, obj, pit)) {
		act_puts("Your donation is greatly appreciated.\n",
			ch, NULL, NULL, TO_CHAR, POS_DEAD);
	}
}

void do_cdonate( CHAR_DATA *ch, const char *argument)
{
	OBJ_DATA *pit;
	OBJ_DATA *obj;
	ROOM_INDEX_DATA *original;
	clan_t  *clan;
	char arg[MAX_INPUT_LENGTH];

	if (ch->clan == 0 
	|| (clan = clan_lookup(ch->clan)) == NULL) {
		char_puts("You are not in a clan.\n", ch);
                return;
        }

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0' ) {
		send_to_char("Donate what?\n\r",ch);
		return;
	}

	original = ch->in_room;

	if ((obj = get_obj_carry (ch, arg)) == NULL) {
		send_to_char("You do not have that!\n\r",ch);
		return;
	}

	char_from_room(ch);
	char_to_room(ch,get_room_index(clan->donate_vnum));
	pit = get_obj_list(ch, "pit", ch->in_room->contents);
	char_from_room(ch);
	char_to_room(ch,original);

	if (pit == NULL) {
		DEBUG(DEBUG_CLAN,
			"%s and clan %s, doesn't have a 'pit' in their donate room %d.",
			ch->name, clan->name, clan->donate_vnum);
		char_puts("Hmm, can't find your pit right now.", ch);
		return;
	}

	if (donate_obj(ch, obj, pit)) {
		act_puts("$t greatly appreciates your donation.\n",
			ch, clan->name, NULL, TO_CHAR, POS_DEAD);
	}
}

/*
 * pin medals on players for events.
 * these medals do not go in the inventory, but on their own list.
 * by Zsuzsu
 */
void do_pin_medal(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];
	CHAR_DATA      *victim;
	OBJ_DATA       *obj;
	struct tm	*time_tm;

	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Pin what on whom?\n", ch);
		return;
	}

	if ((obj = get_obj_carry(ch, arg)) == NULL) {
		char_puts("You do not have that item.\n", ch);
		return;
	}

	argument = one_argument(argument, arg, sizeof(arg));
	if (!str_cmp(arg, "on"))
		argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		do_pin_medal(ch, str_empty);
		return;
	}

	if (obj->wear_loc != WEAR_NONE) {
		char_puts("You must remove it first.\n", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (obj->pIndexData->item_type != ITEM_MEDAL
	&& obj->pIndexData->item_type != ITEM_AWARD) {
		char_puts("You can't pin that on someone.\n", ch);
		return;
	}

	if (IS_NPC(victim)) {
		char_puts("Souless creatures have no use for trinkets.\n", ch);
		return;
	}

	if (!can_drop_obj(ch, obj)) {
		char_puts("You can't let go of it.\n", ch);
		return;
	}

	if (!can_see_obj(victim, obj)) {
		act("$N can't see it.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (!IS_IMMORTAL(ch) && !check_trust(ch, victim)) {
		act("$N doesn't trust you to do that.", ch, NULL, victim, TO_CHAR);
		act("You don't trust $n to pin a medal on you.",
			victim, NULL, ch, TO_CHAR);
		return;
	}

	obj_from_char(obj);
	obj->owner = mlstr_dup(ch->short_descr);
	time_tm = localtime(&current_time);
	obj->value[ITEM_AWARD_YEAR] = time_tm->tm_year + 1900;
	obj->value[ITEM_AWARD_MONTH] = time_tm->tm_mon + 1;
	obj->value[ITEM_AWARD_DAY] = time_tm->tm_mday;
	if (obj->pIndexData->item_type == ITEM_MEDAL)
		medal_to_char(obj, victim);
	else if (obj->pIndexData->item_type == ITEM_AWARD)
		award_to_char(obj, victim);

	if (ch == victim) {
		act("You pin $p on yourself.", ch, obj, victim, TO_CHAR | ACT_NOTRIG);
		act("$n pins $p on $mself.", ch, obj, victim, TO_NOTVICT | ACT_NOTRIG);
	}
	else {
		act("$n pins $p on $N.", ch, obj, victim, TO_NOTVICT | ACT_NOTRIG);
		act("$n pins $p on you.", ch, obj, victim, TO_VICT | ACT_NOTRIG);
		act("You pin $p on $N.", ch, obj, victim, TO_CHAR | ACT_NOTRIG);
	}

	oprog_call(OPROG_GIVE, obj, ch, victim);
}

void do_bestow_award(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];
	CHAR_DATA      *victim;
	OBJ_DATA       *obj;
	struct tm	*time_tm;

	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Bestow what on whom?\n", ch);
		return;
	}

	if ((obj = get_obj_carry(ch, arg)) == NULL) {
		char_puts("You do not have that item.\n", ch);
		return;
	}

	argument = one_argument(argument, arg, sizeof(arg));
	if (!str_cmp(arg, "on"))
		argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		do_bestow_award(ch, str_empty);
		return;
	}

	if (obj->wear_loc != WEAR_NONE) {
		char_puts("You must remove it first.\n", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (obj->pIndexData->item_type != ITEM_AWARD) {
		char_puts("You can't bestow that on someone.\n", ch);
		return;
	}

	if (IS_NPC(victim)) {
		char_puts("Souless creatures have no use for trinkets.\n", ch);
		return;
	}

	if (!can_drop_obj(ch, obj)) {
		char_puts("You can't let go of it.\n", ch);
		return;
	}

	if (!can_see_obj(victim, obj)) {
		act("$N can't see it.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (!IS_IMMORTAL(ch) && !check_trust(ch, victim)) {
		act("$N doesn't trust you to do that.", ch, NULL, victim, TO_CHAR);
		act("You don't trust $n to bestow an award on you.",
			victim, NULL, ch, TO_CHAR);
		return;
	}

	obj_from_char(obj);
	obj->owner = mlstr_dup(ch->short_descr);

	time_tm = localtime(&current_time);
	obj->value[ITEM_AWARD_YEAR] = time_tm->tm_year + 1900;
	obj->value[ITEM_AWARD_MONTH] = time_tm->tm_mon + 1;
	obj->value[ITEM_AWARD_DAY] = time_tm->tm_mday;

	award_to_char(obj, victim);

	if (ch == victim) {
		act("You bestow $p on yourself.", ch, obj, victim, TO_CHAR | ACT_NOTRIG);
		act("$n bestows $p on $mself.", ch, obj, victim, TO_NOTVICT | ACT_NOTRIG);
	}
	else {
		act("$n bestows $p on $N.", ch, obj, victim, TO_NOTVICT | ACT_NOTRIG);
		act("$n bestows $p on you.", ch, obj, victim, TO_VICT | ACT_NOTRIG);
		act("You bestow $p on $N.", ch, obj, victim, TO_CHAR | ACT_NOTRIG);
	}

	oprog_call(OPROG_GIVE, obj, ch, victim);
}

void do_unpin_medal(CHAR_DATA * ch, const char *argument)
{
	char            arg1[MAX_INPUT_LENGTH];
	char            arg2[MAX_INPUT_LENGTH];
	CHAR_DATA      *victim;
	OBJ_DATA       *obj;

	argument = one_argument(argument, arg1, sizeof(arg1));
	if (arg1[0] == '\0') {
		char_puts("Unpin what from whom?\n", ch);
		return;
	}

	argument = one_argument(argument, arg2, sizeof(arg2));

	if (!str_cmp(arg2, "from"))
		argument = one_argument(argument, arg2, sizeof(arg2));

	if (arg2[0] == '\0') {
		do_unpin_medal(ch, str_empty);
		return;
	}

	if ((victim = get_char_room(ch, arg2)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (IS_NPC(victim)) {
		char_puts("Souless creatures do not bother with trinkets.\n", ch);
		return;
	}

	if ((obj = get_obj_medal(victim, arg1)) == NULL) {
		char_puts("They have not been awarded that medal.\n", ch);
		return;
	}

	if (!can_see_obj(ch, obj)) {
		act("You can't see it.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (obj->pIndexData->item_type != ITEM_MEDAL) {
		char_puts("That doesn't seem to be a medal.\n", ch);
		return;
	}

	if (victim != ch && !IS_OWNER(ch, obj) && !IS_IMMORTAL(ch)) {
		char_puts("You can't unpin something you did not bestow!\n", ch);
		return;
	}

	obj_from_char(obj);
	mlstr_free(obj->owner);
	obj->owner = NULL;
	obj_to_char(obj, ch);

	if (ch == victim) {
		act("You unpin $p from yourself.", ch, obj, victim, TO_CHAR | ACT_NOTRIG);
		act("$n unpins $p from $mself.", ch, obj, victim, TO_NOTVICT | ACT_NOTRIG);
	}
	else {
		act("$n unpins $p from $N.", ch, obj, victim, TO_NOTVICT | ACT_NOTRIG);
		act("$n unpins $p from you.", ch, obj, victim, TO_VICT | ACT_NOTRIG);
		act("You unpin $p from $N.", ch, obj, victim, TO_CHAR | ACT_NOTRIG);
	}

	oprog_call(OPROG_GIVE, obj, victim, ch);
}

void do_unbestow_award(CHAR_DATA * ch, const char *argument)
{
	char            arg1[MAX_INPUT_LENGTH];
	char            arg2[MAX_INPUT_LENGTH];
	CHAR_DATA      *victim;
	OBJ_DATA       *obj;

	argument = one_argument(argument, arg1, sizeof(arg1));
	if (arg1[0] == '\0') {
		char_puts("Unbestow what from whom?\n", ch);
		return;
	}

	argument = one_argument(argument, arg2, sizeof(arg2));

	if (!str_cmp(arg2, "from"))
		argument = one_argument(argument, arg2, sizeof(arg2));

	if (arg2[0] == '\0') {
		do_unpin_medal(ch, str_empty);
		return;
	}

	if ((victim = get_char_room(ch, arg2)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (IS_NPC(victim)) {
		char_puts("Souless creatures do not bother with trinkets.\n", ch);
		return;
	}

	if ((obj = get_obj_award(victim, arg1)) == NULL) {
		char_puts("That award has not been bestowed on them.\n", ch);
		return;
	}

	if (!can_see_obj(ch, obj)) {
		act("You can't see it.", ch, NULL, victim, TO_CHAR);
		return;
	}

	if (obj->pIndexData->item_type != ITEM_AWARD) {
		char_puts("That doesn't seem to be an award.\n", ch);
		return;
	}

	if (!IS_OWNER(ch, obj) && !IS_IMMORTAL(ch)) {
		char_puts("You can't take back something you did not bestow!\n", ch);
		return;
	}

	obj_from_char(obj);
	mlstr_free(obj->owner);
	obj->owner = NULL;
	obj_to_char(obj, ch);

	if (ch == victim) {
		act("You take back $p from yourself.", ch, obj, victim, TO_CHAR | ACT_NOTRIG);
		act("$n takes back $p from $mself.", ch, obj, victim, TO_NOTVICT | ACT_NOTRIG);
	}
	else {
		act("$n takes back $p from $N.", ch, obj, victim, TO_NOTVICT | ACT_NOTRIG);
		act("$n takes back $p from you.", ch, obj, victim, TO_VICT | ACT_NOTRIG);
		act("You take back $p from $N.", ch, obj, victim, TO_CHAR | ACT_NOTRIG);
	}

	oprog_call(OPROG_GIVE, obj, victim, ch);
}

bool can_npc_wear_obj(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (!IS_SET(ch->pIndexData->act, ACT_PET) 
	&& !IS_SET(ch->pIndexData->act, ACT_SUMMONED))
		return TRUE;

	if (IS_SET(obj->pIndexData->wear_flags, ITEM_WIELD)
	&& IS_SET(ch->pIndexData->attr_flags, MOB_ATTR_PET_WEAPONED))
		return TRUE;

	else if (!IS_SET(obj->pIndexData->wear_flags, ITEM_WIELD)
	&& IS_SET(ch->pIndexData->attr_flags, MOB_ATTR_PET_ARMORED))
		return TRUE;

	return FALSE;

}
