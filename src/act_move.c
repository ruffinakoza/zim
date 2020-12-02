/*
 * $Id: act_move.c 933 2006-11-19 22:37:00Z zsuzsu $
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
#include "merc.h"
#include "update.h"
#include "mob_prog.h"
#include "obj_prog.h"
#include "fight.h"
#include "stats.h"

char *	const	dir_name	[]		=
{
	"north", "east", "south", "west", "up", "down"
};

char *	const	from_name	[]		=
{
	"the north", "the east", "the south", "the west", "above", "below"
};


const	int	rev_dir		[]		=
{
	2, 3, 0, 1, 5, 4
};

const	int	movement_loss	[SECT_MAX]	=
{
	1,	/* inside */
	2,	/* city */
	2, 	/* field */
	3, 	/* forest */
	4, 	/* hills */
	6, 	/* mountains */
	4, 	/* water swim */
	1, 	/* water noswim */
	6, 	/* unused */
	10, 	/* air */
	6,	/* desert */
	8,	/* arctic */
	2,	/* road */
};

DECLARE_DO_FUN(do_look		);
DECLARE_DO_FUN(do_yell		);
DECLARE_DO_FUN(do_visible	);

/*
 * Local functions.
 */
bool	has_key		(CHAR_DATA *ch, int key);
int	mount_success	(CHAR_DATA *ch, CHAR_DATA *mount, int canattack);

void move_char(CHAR_DATA *ch, int door, bool follow)
{
	move_char_org(ch, door, follow, FALSE);
}

bool move_char_org(CHAR_DATA *ch, int door, bool follow, bool is_charge)
{
	CHAR_DATA *fch;
	CHAR_DATA *fch_next;
	CHAR_DATA *mount;
	ROOM_INDEX_DATA *in_room;
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	bool room_has_pc;
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	int act_flags1;
	char arrived_buf[80];
	int i, reverse_dir = -1;

	if (RIDDEN(ch) && !IS_NPC(ch->mount)) 
		return move_char_org(ch->mount,door,follow,is_charge);

	if (IS_AFFECTED(ch, AFF_DETECT_WEB) 
	|| (MOUNTED(ch) && IS_AFFECTED(ch->mount, AFF_DETECT_WEB))) {
		WAIT_STATE(ch, PULSE_VIOLENCE);
		if (number_percent() < get_curr_stat(ch, STAT_STR)) {
		 	affect_strip(ch, gsn_web);
		 	REMOVE_BIT(ch->affected_by, AFF_DETECT_WEB);
		 	act_puts("When you attempt to leave the room, you "
				 "break the webs holding you tight.",
				 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		 	act_puts("$n struggles against the webs which hold $m "
				 "in place, and breaks them.",
				 ch, NULL, NULL, TO_ROOM, POS_RESTING);
		}
		else {
			act_puts("You attempt to leave the room, but the webs "
				 "hold you tight.",
				 ch, NULL, NULL, TO_ROOM, POS_DEAD);
			act("$n struggles vainly against the webs which "
			    "hold $m in place.",
			    ch, NULL, NULL, TO_ROOM);
			return FALSE; 
		}
	}
	if (is_affected(ch, gsn_rnet_trap)
	|| (MOUNTED(ch) && is_affected(ch->mount, gsn_rnet_trap))) {
		act_puts("You are wrapped up in a net and cannot move!",ch,NULL,NULL,TO_CHAR,POS_DEAD);
		act("$n wriggles angrily inside the net that is constricting $m.",ch,NULL,NULL,TO_ROOM);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return FALSE;
	}
	for (fch = ch->in_room->people; fch; fch = fch->next_in_room) {
		if (fch->target == ch
		&&  IS_NPC(fch)
		&&  fch->pIndexData->vnum == MOB_VNUM_SHADOW) {
			char_puts("You attempt to leave your shadow alone,"
				" but fail.\n", ch);
			return FALSE;
		}
	}

	if (door < 0 || door >= MAX_DIR) {
		bug("move_char_org: bad door %d.", door);
		return FALSE;
	}

	if (IS_AFFECTED(ch, AFF_HIDE | AFF_FADE)
	&&  !IS_AFFECTED(ch, AFF_SNEAK)) {
		remove_hide_affect(ch, AFF_HIDE | AFF_FADE);
		act_puts("You step out of shadows.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		act_puts("$n steps out of shadows.",
			 ch, NULL, NULL, TO_ROOM, POS_RESTING);
	}

	if (IS_AFFECTED(ch, AFF_CAMOUFLAGE))  {
		int chance;

		if ((chance = get_skill(ch, gsn_creep)) == 0) {
			remove_hide_affect(ch, AFF_CAMOUFLAGE);
			act_puts("You step out from your cover.",
				 ch, NULL, NULL, TO_CHAR, POS_DEAD);
			act("$n steps out from $s cover.",
			    ch, NULL, NULL, TO_ROOM);
		}	    
		else if (number_percent() < chance)
			check_improve(ch, gsn_creep, TRUE, 5);
		else {
			remove_hide_affect(ch, AFF_CAMOUFLAGE);
			act_puts("You step out from your cover.",
				 ch, NULL, NULL, TO_CHAR, POS_DEAD);
			act("$n steps out from $s cover.",
			    ch, NULL, NULL, TO_ROOM);
			check_improve(ch, gsn_creep, FALSE, 5);
		}	    
	}

	/*
	 * Exit trigger, if activated, bail out. Only PCs are triggered.
	 */
	if (!IS_NPC(ch) && mp_exit_trigger(ch, door))
		return FALSE;

	in_room = ch->in_room;
	if ((pexit = in_room->exit[door]) == NULL
	||  (to_room = pexit->to_room.r) == NULL 
	||  !can_see_room(ch, pexit->to_room.r)) {
		char_puts("Alas, you cannot go that way.\n", ch);
		return FALSE;
	}

	if (IS_ROOM_AFFECTED(in_room, RAFF_RANDOMIZER) && !is_charge) {
		int d0;
		while (1) {
			d0 = number_range(0, MAX_DIR-1);
			if ((pexit = in_room->exit[d0]) == NULL
			||  (to_room = pexit->to_room.r) == NULL 
			||  !can_see_room(ch, pexit->to_room.r))
				continue;	  
			door = d0;
			break;
		}
		char_puts("Vertigo overcomes you!\n", ch);
	}

	if (IS_SET(pexit->exit_info, EX_CLOSED) 
	&&  (!IS_AFFECTED(ch, AFF_PASS_DOOR) ||
	     IS_SET(pexit->exit_info, EX_NOPASS))
	&&  !IS_TRUSTED(ch, ANGEL)) {
		if (IS_AFFECTED(ch, AFF_PASS_DOOR)
		&&  IS_SET(pexit->exit_info, EX_NOPASS)) {
  			act_puts("You failed to pass through the $d.",
				 ch, NULL, pexit->keyword, TO_CHAR, POS_DEAD);
			act("$n tries to pass through the $d, but $e fails.",
			    ch, NULL, pexit->keyword, TO_ROOM);
		}
		else {
			act_puts("The $d is closed.",
				 ch, NULL, pexit->keyword, TO_CHAR, POS_DEAD);
		}
		return FALSE;
	}

	if (IS_AFFECTED(ch, AFF_CHARM)
	&&  ch->master != NULL
	&&  in_room == ch->master->in_room) {
		char_puts("What? And leave your beloved master?\n", ch);
		return FALSE;
	}

/*    if (!is_room_owner(ch,to_room) && room_is_private(to_room))	*/
	if (room_is_private(to_room)) {
		char_puts("That room is private right now.\n", ch);
		return FALSE;
	}

	if (MOUNTED(ch)) {
		if (MOUNTED(ch)->position < POS_FIGHTING) {
			char_puts("Your mount must be standing.\n", ch);
			return FALSE; 
		}
		if (!mount_success(ch, MOUNTED(ch), FALSE)) {
			char_puts("Your mount subbornly refuses to go that way.\n", ch);
			return FALSE;
		}
	}

	if (!IS_NPC(ch)) {
		int move;

		if (!IS_IMMORTAL(ch)) {
			if (IS_SET(to_room->room_flags, ROOM_GUILD)
			&&  !guild_ok(ch, to_room)) {
				char_puts("You aren't allowed there.\n", ch);
				return FALSE;
			}

			if (IS_PUMPED(ch)
			&&  IS_SET(to_room->room_flags, ROOM_PEACE | ROOM_GUILD)) {
				act_puts("You feel too bloody to go in there now.",
					 ch, NULL, NULL, TO_CHAR, POS_DEAD);
				return FALSE;
			}
		}

		if (in_room->sector_type == SECT_AIR
		||  to_room->sector_type == SECT_AIR) {
			if (MOUNTED(ch)) {
		        	if(!IS_AFFECTED(MOUNTED(ch), AFF_FLYING)) {
		        		char_puts("Your mount can't fly.\n", ch);
						   return FALSE;
				}
			} 
			else if (!IS_AFFECTED(ch, AFF_FLYING)
			&& !IS_IMMORTAL(ch)) {
				act_puts("You can't fly.",
					 ch, NULL, NULL, TO_CHAR, POS_DEAD);
				return FALSE;
			} 
		}

		if ((in_room->sector_type == SECT_WATER_NOSWIM ||
		     to_room->sector_type == SECT_WATER_NOSWIM)
		&&  (MOUNTED(ch) && !IS_AFFECTED(MOUNTED(ch),AFF_FLYING))) {
			act_puts("You can't take your mount there.\n",
				 ch, NULL, NULL, TO_CHAR, POS_DEAD);
			return FALSE;
		}  

		if ((in_room->sector_type == SECT_WATER_NOSWIM ||
		     to_room->sector_type == SECT_WATER_NOSWIM)
		&&  (!MOUNTED(ch) && !IS_AFFECTED(ch, AFF_FLYING))) {
			OBJ_DATA *pObj;
			bool found;

		    /*
		     * Look for a boat.
		     */
		    found = FALSE;

		    if (IS_IMMORTAL(ch))
			found = TRUE;

		    for (pObj = ch->carrying; pObj != NULL; pObj = pObj->next_content)
		    {
			if (pObj->pIndexData->item_type == ITEM_BOAT)
			{
			    found = TRUE;
			    break;
			}
		    }
		    if (!found)
		    {
			char_puts("You need a boat to go there.\n", ch);
			return FALSE;
		    }
		}

		move = (movement_loss[UMIN(SECT_MAX-1, in_room->sector_type)]
		     + movement_loss[UMIN(SECT_MAX-1, to_room->sector_type)])/2;

		if (number_percent() < get_skill(ch, gsn_path_find)) {
			switch (in_room->sector_type) {
				case SECT_FOREST:
				case SECT_ARCTIC:
				case SECT_FIELD:
				case SECT_HILLS:
				case SECT_MOUNTAIN:
					move = move / 2;
					break;
			}
			check_improve(ch, gsn_path_find, TRUE, 10);
		}
		check_improve(ch, gsn_path_find, FALSE, 10);

		if (IS_AFFECTED(ch,AFF_FLYING)
		|| IS_AFFECTED(ch,AFF_HASTE))
			move = UMAX(1, move / 2);

		if (IS_AFFECTED(ch,AFF_SLOW))
			move *= 2;

		/* cost of moving while hidden */
		if (is_affected(ch, gsn_hide))
			move += move_cost(ch, gsn_hide, STAT_DEX, 1);

		if (is_affected(ch, gsn_fade))
			move += move_cost(ch, gsn_fade, 0, 0);

		if (is_affected(ch, gsn_camouflage))
			move += move_cost(ch, gsn_camouflage, STAT_WIS, 1);

		if (IS_AFFECTED(ch, AFF_IMP_INVIS)) {
			if (ch->mana < 100) {
				act_puts("Your mental powers faulterd.",
					 ch, NULL, NULL, TO_CHAR, POS_DEAD);
				do_visible(ch, str_empty);
			}
			else {
				if (get_curr_stat(ch, STAT_WIS) > 17)
					ch->mana -= 30 - (get_curr_stat(ch,STAT_WIS) - 17);
				else if (get_curr_stat(ch, STAT_WIS) < 14)
					ch->mana -= 30 - (get_curr_stat(ch,STAT_WIS) - 14);
				else
					ch->mana -= 30;
			}
		}

		if (IS_AFFECTED(ch, AFF_INVIS)) {
			if (ch->mana < 100) {
				act_puts("Your mental powers faulterd.",
					 ch, NULL, NULL, TO_CHAR, POS_DEAD);
				do_visible(ch, str_empty);
			}
			else
				ch->mana -= UMAX(2, 5 - (get_curr_stat(ch,STAT_WIS) - 17));
		}

		/* encumberance */
		move += UMAX(0, move * (ENCUMBERANCE(ch)-35) / 20);

		if (ENCUMBERANCE(ch) > 75)
			move += 10;

		if (ENCUMBERANCE(ch) > 100) {
			act_puts("You are unable to move under this burden.",
				 ch, NULL, NULL, TO_CHAR, POS_DEAD);
			ch->move -= move;
			return FALSE;
		}

		if (!MOUNTED(ch)) {
			if (ch->move < move) {
				act_puts("You are too exhausted.",
					 ch, NULL, NULL, TO_CHAR, POS_DEAD);
				return FALSE;
			}

			ch->move -= move;

			if (ch->in_room->sector_type == SECT_DESERT
			|| ch->in_room->sector_type == SECT_FOREST
			|| ch->in_room->sector_type == SECT_ARCTIC) {
				if (get_skill(ch, gsn_path_find) > 0)
					WAIT_STATE(ch, 1);
				else
					WAIT_STATE(ch, 2);
			}
			else
				WAIT_STATE(ch, 1);

			if (IS_WATER(ch->in_room))
				WAIT_STATE(ch, 2);
		}
	}

	/*Zz*/	
	/*find out what direction we came from*/
	for(i=0; i < 6; i++) {
		if (to_room->exit[i] != NULL
		   && to_room->exit[i]->to_room.r == in_room) {
			reverse_dir = i;
			break;
		}
	}

	if (!IS_AFFECTED(ch, AFF_SNEAK)
	&&  !IS_AFFECTED(ch, AFF_CAMOUFLAGE)
	&&  ch->invis_level < LEVEL_HERO)
		act_flags1 = TO_ROOM;
	else
		act_flags1 = TO_ROOM | ACT_NOMORTAL;

	/* check for quiet movement */
	if (act_flags1 == TO_ROOM
	&& !IS_NPC(ch)
	&& !MOUNTED(ch)
	&& ch->in_room != NULL
	&& ch->in_room->sector_type != SECT_CITY
	&& ch->in_room->sector_type != SECT_INSIDE
	&& get_skill(ch, gsn_quiet_movement) > 0) {
		if (number_percent() <= get_skill(ch, gsn_quiet_movement)) {
			act_flags1 = TO_ROOM | ACT_NOMORTAL;
			check_improve(ch,gsn_quiet_movement,TRUE,1);
		}
		else 
			check_improve(ch,gsn_quiet_movement,FALSE,1);
	}

	/*pure_sight checks*/
        for (fch = ch->in_room->people; fch; fch = fch_next) {
                fch_next = fch->next_in_room;
                if (is_affected(fch, gsn_pure_sight) 
		&& act_flags1 == (TO_ROOM | ACT_NOMORTAL)
                && IS_EVIL(ch) && !IS_NPC(ch)) {
                        if(!can_see(fch, ch)) {
				char_puts("An {revil {xpresence has left.\n",fch);
                        }
                        else
                                act("$N {rsneaks{x out of the room,"
					" unaware that $S {revil{x presence"
					" gave $M away.", 
					fch, NULL, ch, TO_CHAR);
                }
        }

	if (!IS_NPC(ch)
	&&  !is_charge
	&& number_percent() <= get_skill(ch, gsn_quiet_movement)) {
		act(MOUNTED(ch) ? "$n leaves, riding on $N." 
				: "$n leaves.",
		    ch, NULL, MOUNTED(ch), act_flags1);
	}
	else if (is_charge) {
		act("$n spurs $s $N, leaving $t.",
		    ch, dir_name[door], ch->mount,  TO_ROOM);
	}
	else {
		act(MOUNTED(ch) ? "$n leaves $t, riding on $N." :
				  "$n leaves $t.",
		    ch, dir_name[door], MOUNTED(ch), act_flags1 | ACT_TRANS);
	}

	if (IS_AFFECTED(ch, AFF_CAMOUFLAGE)
	&&  to_room->sector_type != SECT_FOREST
	&&  to_room->sector_type != SECT_MOUNTAIN
	&&  to_room->sector_type != SECT_HILLS) {
		remove_hide_affect(ch, AFF_CAMOUFLAGE);
		act_puts("You step out from your cover.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		act("$n steps out from $s cover.",
		    ch, NULL, NULL, TO_ROOM);
	}

	/* room record for tracking */
	if (!IS_NPC(ch))
		room_record(ch->name, in_room, door);

	/*
	 * now, after all the checks are done we should
	 * - take the char from the room
	 * - print the message to chars in to_room about ch arrival
	 * - put the char to to_room
	 * - CHECK THAT CHAR IS NOT DEAD after char_to_room
	 * - move all the followers and pull all the triggers
	 */
	mount = MOUNTED(ch);
	char_from_room(ch);

	if (!IS_AFFECTED(ch, AFF_SNEAK) && ch->invis_level < LEVEL_HERO) 
		act_flags1 = TO_ALL;
	else
		act_flags1 = TO_ALL | ACT_NOMORTAL;

	/* check for quiet movement */
	if (act_flags1 == TO_ALL
	&& !IS_NPC(ch)
	&& !MOUNTED(ch)
	&& ch->in_room != NULL
	&& ch->in_room->sector_type != SECT_CITY
	&& ch->in_room->sector_type != SECT_INSIDE
	&& get_skill(ch, gsn_quiet_movement) > 0) {
		if (number_percent() <= get_skill(ch, gsn_quiet_movement)) {
			act_flags1 = TO_ALL | ACT_NOMORTAL;
			check_improve(ch,gsn_quiet_movement,TRUE,1);
		}
		else 
			check_improve(ch,gsn_quiet_movement,FALSE,1);
	}
	/*pure_sight checks*/
	for (fch = to_room->people; fch; fch = fch_next) {
                fch_next = fch->next_in_room;
		if(is_affected(fch, gsn_pure_sight) && act_flags1 == (TO_ALL | ACT_NOMORTAL)
		&& IS_EVIL(ch) && !IS_NPC(ch))
		{
			if(!can_see(fch, ch))
			{
				char_puts("An {revil {xpresence has arrived.\n",fch);
			}
			else
				act("$N {rsneaks{x into the room, unaware that $S {revil{x presence gave $M away.", fch, NULL, ch, TO_CHAR);
		}
	}
	if(is_affected(ch, gsn_sling))
	{
		act("You are hurt by the dagger lodged in your leg.",ch,NULL,NULL,TO_CHAR);
		act("$n is hurt by the dagger lodged in $s leg.",ch,NULL,NULL,TO_ROOM);
		if((ch->level*2) < ch->hit)
			damage(ch, ch, dice(2,4), gsn_sling, DAM_PIERCE, DAMF_SHOW);
	}

	if (!is_charge) {
		if (reverse_dir != -1
		&& (number_percent() > get_skill(ch, gsn_quiet_movement))) 
			snprintf(arrived_buf, sizeof(arrived_buf),
				"$i arrives from %s%s",
				from_name[reverse_dir],
				mount ? ", riding $N." : ".");
		else
			snprintf(arrived_buf, sizeof(arrived_buf),
				"$i arrives%s",
				mount ? ", riding $N." : ".");
				
		act(arrived_buf, to_room->people, ch, mount, act_flags1);
	}

	char_to_room(ch, to_room);

	if (mount) {
		char_from_room(mount);
		char_to_room(mount, to_room);
  		ch->riding = TRUE;
  		mount->riding = TRUE;
	}

	if (!JUST_KILLED(ch))
		do_look(ch, "auto");

	if (in_room == to_room) /* no circular follows */
		return TRUE;

	/*
	 * move all the followers
	 */
	for (fch = in_room->people; fch; fch = fch_next) {
		fch_next = fch->next_in_room;

		if (fch->master != ch || fch->position != POS_STANDING
		||  !can_see_room(fch, to_room))
			continue;

		if (IS_SET(to_room->room_flags, ROOM_LAW)
		&&  IS_NPC(fch)
		&&  IS_AGGRO(fch, NULL)) {
			act_puts("You can't bring $N into the city.",
				 ch, NULL, fch, TO_CHAR, POS_DEAD);
			act("You aren't allowed in the city.",
			    fch, NULL, NULL, TO_CHAR);
			continue;
		}

		act_puts("You follow $N.", fch, NULL, ch, TO_CHAR, POS_DEAD);
		move_char(fch, door, TRUE);
	}

	if (JUST_KILLED(ch))
		return TRUE;

	room_has_pc = FALSE;
	for (fch = to_room->people; fch != NULL; fch = fch_next) {
		fch_next = fch->next_in_room;
		if (!IS_NPC(fch)) {
			room_has_pc = TRUE;
			break;
		}
	}

	if (!room_has_pc)
		return TRUE;

	/*
	 * pull GREET and ENTRY triggers
	 *
	 * if someone is following the char, these triggers get activated
	 * for the followers before the char, but it's safer this way...
	 */
	for (fch = to_room->people; fch; fch = fch_next) {
		fch_next = fch->next_in_room;

		/* greet progs for items carried by people in room */
		for (obj = fch->carrying; obj; obj = obj_next) {
			obj_next = obj->next_content;
			oprog_call(OPROG_GREET, obj, ch, NULL);
		}
	}

	for (obj = ch->in_room->contents; obj != NULL; obj = obj_next) {
		obj_next = obj->next_content;
		oprog_call(OPROG_GREET, obj, ch, NULL);
	}

	if (!IS_NPC(ch))
    		mp_greet_trigger(ch);

	for (obj = ch->carrying; obj; obj = obj_next) {
		obj_next = obj->next_content;
		oprog_call(OPROG_ENTRY, obj, NULL, NULL);
	}

	if (IS_NPC(ch) && HAS_TRIGGER(ch, TRIG_ENTRY))
		mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_ENTRY);

	return TRUE;
}

void do_north(CHAR_DATA *ch, const char *argument)
{
	move_char(ch, DIR_NORTH, FALSE);
}

void do_east(CHAR_DATA *ch, const char *argument)
{
	move_char(ch, DIR_EAST, FALSE);
}

void do_south(CHAR_DATA *ch, const char *argument)
{
	move_char(ch, DIR_SOUTH, FALSE);
}

void do_west(CHAR_DATA *ch, const char *argument)
{
	move_char(ch, DIR_WEST, FALSE);
}

void do_up(CHAR_DATA *ch, const char *argument)
{
	move_char(ch, DIR_UP, FALSE);
}

void do_down(CHAR_DATA *ch, const char *argument)
{
	move_char(ch, DIR_DOWN, FALSE);
}

int find_exit(CHAR_DATA *ch, char *arg)
{
	int door;

	     if (!str_cmp(arg, "n") || !str_cmp(arg, "north")) door = 0;
	else if (!str_cmp(arg, "e") || !str_cmp(arg, "east" )) door = 1;
	else if (!str_cmp(arg, "s") || !str_cmp(arg, "south")) door = 2;
	else if (!str_cmp(arg, "w") || !str_cmp(arg, "west" )) door = 3;
	else if (!str_cmp(arg, "u") || !str_cmp(arg, "up"   )) door = 4;
	else if (!str_cmp(arg, "d") || !str_cmp(arg, "down" )) door = 5;
	else {
		act_puts("I see no exit $T here.",
			 ch, NULL, arg, TO_CHAR, POS_DEAD);
		return -1;
	}

	return door;
}

int find_door(CHAR_DATA *ch, char *arg)
{
	EXIT_DATA *pexit;
	int door;

	     if (!str_cmp(arg, "n") || !str_cmp(arg, "north")) door = 0;
	else if (!str_cmp(arg, "e") || !str_cmp(arg, "east" )) door = 1;
	else if (!str_cmp(arg, "s") || !str_cmp(arg, "south")) door = 2;
	else if (!str_cmp(arg, "w") || !str_cmp(arg, "west" )) door = 3;
	else if (!str_cmp(arg, "u") || !str_cmp(arg, "up"   )) door = 4;
	else if (!str_cmp(arg, "d") || !str_cmp(arg, "down" )) door = 5;
	else {
		for (door = 0; door <= 5; door++) {
		    if ((pexit = ch->in_room->exit[door]) != NULL
		    &&   IS_SET(pexit->exit_info, EX_ISDOOR)
		    &&   pexit->keyword != NULL
		    &&   is_name(arg, pexit->keyword))
			return door;
		}
		act_puts("I see no $T here.", ch, NULL, arg, TO_CHAR, POS_DEAD);
		return -1;
	}

	if ((pexit = ch->in_room->exit[door]) == NULL) {
		act_puts("I see no door $T here.",
			 ch, NULL, arg, TO_CHAR, POS_DEAD);
		return -1;
	}

	if (!IS_SET(pexit->exit_info, EX_ISDOOR)) {
		char_puts("You can't do that.\n", ch);
		return -1;
	}

	return door;
}

void do_open(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int door;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Open what?\n", ch);
		return;
	}

	if ((obj = get_obj_here(ch, arg)) != NULL) {
 	/* open portal */
		if (obj->pIndexData->item_type == ITEM_PORTAL)
		{
		    if (!IS_SET(obj->value[ITEM_PORTAL_EXIT_FLAGS], EX_ISDOOR))
		    {
			char_puts("You can't do that.\n", ch);
			return;
		    }

		    if (!IS_SET(obj->value[ITEM_PORTAL_EXIT_FLAGS], EX_CLOSED)) {
			char_puts("It's already open.\n", ch);
			return;
		    }

		    if (IS_SET(obj->value[ITEM_PORTAL_EXIT_FLAGS], EX_LOCKED)) {
			char_puts("It's locked.\n", ch);
			return;
		    }

		    REMOVE_BIT(obj->value[ITEM_PORTAL_EXIT_FLAGS], EX_CLOSED);
		    act_puts("You open $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
		    act("$n opens $p.", ch, obj, NULL, TO_ROOM);
		    return;
 	}

		/* 'open object' */
		if (obj->pIndexData->item_type != ITEM_CONTAINER)
		    { char_puts("That's not a container.\n", ch); return; }
		if (!IS_SET(obj->value[ITEM_CONTAINER_FLAGS], CONT_CLOSED))
		    { char_puts("It's already open.\n", ch); return; }
		if (!IS_SET(obj->value[ITEM_CONTAINER_FLAGS], CONT_CLOSEABLE))
		    { char_puts("You can't do that.\n", ch); return; }
		if (IS_SET(obj->value[ITEM_CONTAINER_FLAGS], CONT_LOCKED))
		    { char_puts("It's locked.\n", ch); return; }

		REMOVE_BIT(obj->value[ITEM_CONTAINER_FLAGS], CONT_CLOSED);
		act_puts("You open $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
		act("$n opens $p.", ch, obj, NULL, TO_ROOM);
		return;
	}

	if ((door = find_door(ch, arg)) >= 0) {
		/* 'open door' */
		ROOM_INDEX_DATA *to_room;
		EXIT_DATA *pexit;
		EXIT_DATA *pexit_rev;

		pexit = ch->in_room->exit[door];
		if (!IS_SET(pexit->exit_info, EX_CLOSED))
		    { char_puts("It's already open.\n", ch); return; }
		if ( IS_SET(pexit->exit_info, EX_LOCKED))
		    { char_puts("It's locked.\n", ch); return; }

		REMOVE_BIT(pexit->exit_info, EX_CLOSED);
		act("$n opens the $d.", ch, NULL, pexit->keyword, TO_ROOM);
		char_puts("Ok.\n", ch);

		/* open the other side */
		if ((to_room   = pexit->to_room.r           ) != NULL
		&&   (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
		&&   pexit_rev->to_room.r == ch->in_room) {
			ROOM_INDEX_DATA *in_room;

			REMOVE_BIT(pexit_rev->exit_info, EX_CLOSED);

			in_room = ch->in_room;
			ch->in_room = to_room;
			act("The $d opens.", ch, NULL, pexit_rev->keyword,
			    TO_ROOM);
			ch->in_room = in_room;
		}
		return;
	}
}

void do_close(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int door;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Close what?\n", ch);
		return;
	}

	if ((obj = get_obj_here(ch, arg)) != NULL) {
		/* portal stuff */
		if (obj->pIndexData->item_type == ITEM_PORTAL)
		{

		    if (!IS_SET(obj->value[ITEM_PORTAL_FLAGS],EX_ISDOOR)
		    ||   IS_SET(obj->value[ITEM_PORTAL_FLAGS],EX_NOCLOSE))
		    {
			char_puts("You can't do that.\n", ch);
			return;
		    }

		    if (IS_SET(obj->value[ITEM_PORTAL_FLAGS],EX_CLOSED)) {
			char_puts("It's already closed.\n", ch);
			return;
		    }

		    SET_BIT(obj->value[ITEM_PORTAL_FLAGS],EX_CLOSED);
		    act_puts("You close $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
		    act("$n closes $p.", ch, obj, NULL, TO_ROOM);
		    return;
		}

		/* 'close object' */
		if (obj->pIndexData->item_type != ITEM_CONTAINER)
		    { char_puts("That's not a container.\n", ch); return; }
		if (IS_SET(obj->value[ITEM_CONTAINER_FLAGS], CONT_CLOSED))
		    { char_puts("It's already closed.\n", ch); return; }
		if (!IS_SET(obj->value[ITEM_CONTAINER_FLAGS], CONT_CLOSEABLE))
		    { char_puts("You can't do that.\n", ch); return; }

		SET_BIT(obj->value[ITEM_CONTAINER_FLAGS], CONT_CLOSED);
		act_puts("You close $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
		act("$n closes $p.", ch, obj, NULL, TO_ROOM);
		return;
	}

	if ((door = find_door(ch, arg)) >= 0) {
		/* 'close door' */
		ROOM_INDEX_DATA *to_room;
		EXIT_DATA *pexit;
		EXIT_DATA *pexit_rev;

		pexit	= ch->in_room->exit[door];
		if (IS_SET(pexit->exit_info, EX_CLOSED))
		    { char_puts("It's already closed.\n", ch); return; }

		SET_BIT(pexit->exit_info, EX_CLOSED);
		act("$n closes $d.", ch, NULL, pexit->keyword, TO_ROOM);
		char_puts("Ok.\n", ch);

		/* close the other side */
		if ((to_room   = pexit->to_room.r           ) != NULL
		&&   (pexit_rev = to_room->exit[rev_dir[door]]) != 0
		&&   pexit_rev->to_room.r == ch->in_room) {
			ROOM_INDEX_DATA *in_room;

			SET_BIT(pexit_rev->exit_info, EX_CLOSED);
			in_room = ch->in_room;
			ch->in_room = to_room;
			act("The $d closes.", ch, NULL, pexit_rev->keyword,
			    TO_ROOM);
			ch->in_room = in_room;
		}
		return;
	}
}

/* 
 * Added can_see check. Kio.
 */
bool has_key(CHAR_DATA *ch, int key)
{
	OBJ_DATA *obj;

	for (obj = ch->carrying; obj; obj = obj->next_content)
		if (obj->pIndexData->vnum == key
		&&  can_see_obj(ch, obj))
		    return TRUE;

	return FALSE;
}

bool has_key_ground(CHAR_DATA *ch, int key)
{
	OBJ_DATA *obj;

	for (obj = ch->in_room->contents; obj; obj = obj->next_content)
		if (obj->pIndexData->vnum == key
		&&  can_see_obj(ch, obj))
		    return TRUE;

	return FALSE;
}

void do_lock(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int door;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Lock what?\n", ch);
		return;
	}

	if ((obj = get_obj_here(ch, arg)) != NULL) {
		/* portal stuff */
		if (obj->pIndexData->item_type == ITEM_PORTAL) {
		    if (!IS_SET(obj->value[ITEM_PORTAL_FLAGS], EX_ISDOOR)
		    ||  IS_SET(obj->value[ITEM_PORTAL_FLAGS], EX_NOCLOSE)) {
			char_puts("You can't do that.\n", ch);
			return;
		    }
		    if (!IS_SET(obj->value[ITEM_PORTAL_FLAGS], EX_CLOSED)) {
			char_puts("It's not closed.\n", ch);
		 	return;
		    }

		    if (obj->value[ITEM_PORTAL_KEY] < 0 
		    || IS_SET(obj->value[ITEM_PORTAL_FLAGS], EX_NOLOCK)) {
			char_puts("It can't be locked.\n", ch);
			return;
		    }

		    if (!has_key(ch, obj->value[ITEM_PORTAL_KEY])) {
			char_puts("You lack the key.\n", ch);
			return;
		    }

		    if (IS_SET(obj->value[ITEM_PORTAL_EXIT_FLAGS], EX_LOCKED)) {
			char_puts("It's already locked.\n", ch);
			return;
		    }

		    SET_BIT(obj->value[ITEM_PORTAL_EXIT_FLAGS], EX_LOCKED);
		    act_puts("You lock $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
		    act("$n locks $p.", ch, obj, NULL, TO_ROOM);
		    return;
		}

		/* 'lock object' */
		if (obj->pIndexData->item_type != ITEM_CONTAINER)
		    { char_puts("That's not a container.\n", ch); return; }
		if (!IS_SET(obj->value[ITEM_CONTAINER_FLAGS], CONT_CLOSED))
		    { char_puts("It's not closed.\n", ch); return; }
		if (obj->value[ITEM_CONTAINER_KEY] < 0)
		    { char_puts("It can't be locked.\n", ch); return; }
		if (!has_key(ch, obj->value[ITEM_CONTAINER_KEY]))
		    { char_puts("You lack the key.\n", ch); return; }
		if (IS_SET(obj->value[ITEM_CONTAINER_FLAGS], CONT_LOCKED))
		    { char_puts("It's already locked.\n", ch); return; }

		SET_BIT(obj->value[ITEM_CONTAINER_FLAGS], CONT_LOCKED);
		act_puts("You lock $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
		act("$n locks $p.", ch, obj, NULL, TO_ROOM);
		return;
	}

	if ((door = find_door(ch, arg)) >= 0)
	{
		/* 'lock door' */
		ROOM_INDEX_DATA *to_room;
		EXIT_DATA *pexit;
		EXIT_DATA *pexit_rev;

		pexit	= ch->in_room->exit[door];
		if (!IS_SET(pexit->exit_info, EX_CLOSED))
		    { char_puts("It's not closed.\n", ch); return; }
		if (pexit->key < 0)
		    { char_puts("It can't be locked.\n", ch); return; }
		if (!has_key(ch, pexit->key) && 
		       !has_key_ground(ch, pexit->key))
		    { char_puts("You lack the key.\n", ch); return; }
		if (IS_SET(pexit->exit_info, EX_LOCKED))
		    { char_puts("It's already locked.\n", ch); return; }

		SET_BIT(pexit->exit_info, EX_LOCKED);
		char_puts("*Click*\n", ch);
		act("$n locks the $d.", ch, NULL, pexit->keyword, TO_ROOM);

		/* lock the other side */
		if ((to_room   = pexit->to_room.r           ) != NULL
		&&   (pexit_rev = to_room->exit[rev_dir[door]]) != 0
		&&   pexit_rev->to_room.r == ch->in_room) {
			ROOM_INDEX_DATA *in_room;

			SET_BIT(pexit_rev->exit_info, EX_LOCKED);

			in_room = ch->in_room;
			ch->in_room = to_room;
			act("The $d clicks.",
			    ch, NULL, pexit_rev->keyword, TO_ROOM);
			ch->in_room  = in_room;
		}
		return;
	}
}

void do_unlock(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int door;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Unlock what?\n", ch);
		return;
	}

	if ((obj = get_obj_here(ch, arg)) != NULL) {
 	/* portal stuff */
		if (obj->pIndexData->item_type == ITEM_PORTAL) {
		    if (IS_SET(obj->value[ITEM_PORTAL_EXIT_FLAGS],EX_ISDOOR)) {
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

		    if (!has_key(ch,obj->value[ITEM_PORTAL_KEY])) {
			char_puts("You lack the key.\n", ch);
			return;
		    }

		    if (!IS_SET(obj->value[ITEM_PORTAL_EXIT_FLAGS],EX_LOCKED)) {
			char_puts("It's already unlocked.\n", ch);
			return;
		    }

		    REMOVE_BIT(obj->value[ITEM_PORTAL_EXIT_FLAGS],EX_LOCKED);
		    act_puts("You unlock $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
		    act("$n unlocks $p.", ch, obj, NULL, TO_ROOM);
		    return;
		}

		/* 'unlock object' */
		if (obj->pIndexData->item_type != ITEM_CONTAINER)
		    { char_puts("That's not a container.\n", ch); return; }
		if (!IS_SET(obj->value[ITEM_CONTAINER_FLAGS], CONT_CLOSED))
		    { char_puts("It's not closed.\n", ch); return; }
		if (obj->value[ITEM_CONTAINER_KEY] < 0)
		    { char_puts("It can't be unlocked.\n", ch); return; }
		if (!has_key(ch, obj->value[ITEM_CONTAINER_KEY]))
		    { char_puts("You lack the key.\n", ch); return; }
		if (!IS_SET(obj->value[ITEM_CONTAINER_FLAGS], CONT_LOCKED))
		    { char_puts("It's already unlocked.\n", ch); return; }

		REMOVE_BIT(obj->value[ITEM_CONTAINER_FLAGS], CONT_LOCKED);
		act_puts("You unlock $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
		act("$n unlocks $p.", ch, obj, NULL, TO_ROOM);
		return;
	}

	if ((door = find_door(ch, arg)) >= 0)
	{
		/* 'unlock door' */
		ROOM_INDEX_DATA *to_room;
		EXIT_DATA *pexit;
		EXIT_DATA *pexit_rev;

		pexit = ch->in_room->exit[door];

		if (!IS_SET(pexit->exit_info, EX_CLOSED))
		    { char_puts("It's not closed.\n", ch); return; }
		if (pexit->key < 0)
		    { char_puts("It can't be unlocked.\n", ch); return; }
		if (!has_key(ch, pexit->key) && 
		       !has_key_ground(ch, pexit->key))
		    { char_puts("You lack the key.\n", ch); return; }
		if (!IS_SET(pexit->exit_info, EX_LOCKED))
		    { char_puts("It's already unlocked.\n", ch); return; }

		REMOVE_BIT(pexit->exit_info, EX_LOCKED);
		char_puts("*Click*\n", ch);
		act("$n unlocks the $d.", ch, NULL, pexit->keyword, TO_ROOM);

		/* unlock the other side */
		if ((to_room   = pexit->to_room.r           ) != NULL
		&&   (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
		&&   pexit_rev->to_room.r == ch->in_room) {
			ROOM_INDEX_DATA *in_room;

			REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);

			in_room = ch->in_room;
			ch->in_room = to_room;
			act("The $d clicks.",
			    ch, NULL, pexit_rev->keyword, TO_ROOM);
			ch->in_room = in_room;
		}
		return;
	}
}

void do_stand(CHAR_DATA *ch, const char *argument)
{
	OBJ_DATA *obj = NULL;

	if (is_affected(ch, gsn_rnet_trap)) {
		char_puts("There's not enough room to stand!\n",ch);
		return;
	}
	if (argument[0] != '\0') {
		if (ch->position == POS_FIGHTING) {
		    char_puts("Maybe you should finish fighting first?\n", ch);
		    return;
		}
		obj = get_obj_list(ch,argument,ch->in_room->contents);
		if (obj == NULL)
		{
		    char_puts("You don't see that here.\n", ch);
		    return;
		}
		if (obj->pIndexData->item_type != ITEM_FURNITURE
		||  (!IS_SET(obj->value[ITEM_FURNITURE_FLAGS],STAND_AT)
		&&   !IS_SET(obj->value[ITEM_FURNITURE_FLAGS],STAND_ON)
		&&   !IS_SET(obj->value[ITEM_FURNITURE_FLAGS],STAND_IN)))
		{
		    char_puts("You can't seem to find a place to stand.\n", ch);
		    return;
		}
		if (ch->on != obj 
		&& count_users(obj) >= obj->value[ITEM_FURNITURE_QUANTITY])
		{
		    act_puts("There's no room to stand on $p.", ch, obj, NULL, TO_ROOM, POS_DEAD);
		    return;
		}
	}
	switch (ch->position) {
	case POS_SLEEPING:
		if (IS_AFFECTED(ch, AFF_SLEEP))
		    { char_puts("You can't wake up!\n", ch); return; }
		
		if (obj == NULL) {
		    char_puts("You wake and stand up.\n", ch);
		    act("$n wakes and stands up.", ch, NULL, NULL, TO_ROOM);
				
		    ch->on = NULL;
		}
		else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS],STAND_AT))
		{
		   act_puts("You wake and stand at $p.",
			    ch, obj, NULL, TO_CHAR, POS_DEAD);
		   act("$n wakes and stands at $p.", ch, obj, NULL, TO_ROOM);
				
		}
		else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS],STAND_ON))
		{
		   act_puts("You wake and stand on $p.",
			    ch, obj, NULL, TO_CHAR, POS_DEAD);
		   act("$n wakes and stands on $p.", ch, obj, NULL, TO_ROOM);
		}
		else 
		{
		   act_puts("You wake and stand in $p.",
			    ch, obj, NULL, TO_CHAR, POS_DEAD);
		   act("$n wakes and stands in $p.", ch, obj, NULL, TO_ROOM);
		}

		if (IS_HARA_KIRI(ch)) {
			char_puts("You feel your blood heats your body.\n", ch);
			REMOVE_BIT(ch->state_flags, STATE_HARA_KIRI);
		}

		ch->position = POS_STANDING;
		do_look(ch,"auto");
		break;

	case POS_RESTING: case POS_SITTING:
		if (obj == NULL)
		{
		    char_puts("You stand up.\n", ch);
		    act("$n stands up.", ch, NULL, NULL, TO_ROOM);
		    ch->on = NULL;
		}
		else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS],STAND_AT))
		{
		    act_puts("You stand at $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
		    act("$n stands at $p.", ch, obj, NULL, TO_ROOM);
		}
		else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS],STAND_ON))
		{
		    act_puts("You stand on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
		    act("$n stands on $p.", ch, obj, NULL, TO_ROOM);
		}
		else
		{
		    act_puts("You stand in $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
		    act("$n stands in $p.", ch, obj, NULL, TO_ROOM);
		}
		ch->position = POS_STANDING;
		break;

	case POS_STANDING:
		char_puts("You are already standing.\n", ch);
		break;

	case POS_FIGHTING:
		char_puts("You are already fighting!\n", ch);
		break;
	}
}

void do_rest(CHAR_DATA *ch, const char *argument)
{
	OBJ_DATA *obj = NULL;

	if (ch->position == POS_FIGHTING) {
		char_puts("You are already fighting!\n", ch);
		return;
	}

        if (is_affected(ch, gsn_rnet_trap)) {
                char_puts("How can you rest while ensnared like this?!\n",ch);
                return;
        }

	if (MOUNTED(ch)) {
		  char_puts("You can't rest while mounted.\n", ch);
		  return;
	}

	if (RIDDEN(ch)) {
		  char_puts("You can't rest while being ridden.\n", ch);
		  return;
	}

	if (IS_AFFECTED(ch, AFF_SLEEP))
	{ char_puts("You are already sleeping.\n", ch); return; }

	/* okay, now that we know we can rest, find an object to rest on */
	if (argument[0] != '\0') {
		obj = get_obj_list(ch,argument,ch->in_room->contents);
		if (obj == NULL) {
		    char_puts("You don't see that here.\n", ch);
		    return;
		}
	}
	else obj = ch->on;

	if (obj != NULL) {
		if (!IS_SET(obj->pIndexData->item_type,ITEM_FURNITURE) 
		||  (!IS_SET(obj->value[ITEM_FURNITURE_FLAGS],REST_ON) &&
		     !IS_SET(obj->value[ITEM_FURNITURE_FLAGS],REST_IN) &&
		     !IS_SET(obj->value[ITEM_FURNITURE_FLAGS],REST_AT))) {
		    char_puts("You can't rest on that.\n", ch);
		    return;
		}

		if (obj != NULL && ch->on != obj
		&&  count_users(obj) >= obj->value[ITEM_FURNITURE_QUANTITY]) {
			act_puts("There's no more room on $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			return;
		}
		
		ch->on = obj;
	}

	switch (ch->position) {
	case POS_SLEEPING:
		if (obj == NULL) {
		    char_puts("You wake up and start resting.\n", ch);
		    act("$n wakes up and starts resting.",
			ch, NULL, NULL, TO_ROOM);
				
		}
		else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS],REST_AT)) {
			act_puts("You wake up and rest at $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n wakes up and rests at $p.",
			    ch, obj, NULL, TO_ROOM);
		}
		else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS],REST_ON)) {
			act_puts("You wake up and rest on $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n wakes up and rests on $p.",
			    ch, obj, NULL, TO_ROOM);
		}
		else {
			act_puts("You wake up and rest in $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n wakes up and rests in $p.",
			    ch, obj, NULL, TO_ROOM);
		}
		ch->position = POS_RESTING;
		break;

	case POS_RESTING:
		char_puts("You are already resting.\n", ch);
		break;

	case POS_STANDING:
		if (obj == NULL) {
			char_puts("You rest.\n", ch);
			act("$n sits down and rests.",
			    ch, NULL, NULL, TO_ROOM);
		}
		else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS], REST_AT)) {
			act_puts("You sit down at $p and rest.",
			    ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n sits down at $p and rests.",
			    ch, obj, NULL, TO_ROOM);
		}
		else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS], REST_ON)) {
			act_puts("You sit down on $p and rest.",
			    ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n sits down on $p and rests.",
			    ch, obj, NULL, TO_ROOM);
		}
		else {
			act_puts("You sit down in $p and rest.",
			    ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n sits down in $p and rests.",
			    ch, obj, NULL, TO_ROOM);
		}
		ch->position = POS_RESTING;
		break;

	case POS_SITTING:
		if (obj == NULL) {
			char_puts("You rest.\n", ch);
			act("$n rests.", ch, NULL, NULL, TO_ROOM);
		}
		else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS],REST_AT)) {
			act_puts("You rest at $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n rests at $p.", ch, obj, NULL, TO_ROOM);
		} 
		else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS],REST_ON)) {
			act_puts("You rest on $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n rests on $p.", ch, obj, NULL, TO_ROOM);
		}
		else {
			act_puts("You rest in $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n rests in $p.", ch, obj, NULL, TO_ROOM);
		}
		ch->position = POS_RESTING;

		if (IS_HARA_KIRI(ch)) {
			char_puts("You feel your blood heats your body.\n",
				  ch);
			REMOVE_BIT(ch->state_flags, STATE_HARA_KIRI);
		}
		break;
	}
}

void do_sit(CHAR_DATA *ch, const char *argument)
{
	OBJ_DATA *obj = NULL;

	if (ch->position == POS_FIGHTING) {
		char_puts("Maybe you should finish fighting first?\n", ch);
		return;
	}

	if (MOUNTED(ch)) {
		  char_puts("You can't sit while mounted.\n", ch);
		  return;
	}

	if (RIDDEN(ch)) {
		  char_puts("You can't sit while being ridden.\n", ch);
		  return;
	}

	if (IS_AFFECTED(ch, AFF_SLEEP))
	{ char_puts("You are already sleeping.\n", ch); return; }

	/* okay, now that we know we can sit, find an object to sit on */
	if (argument[0] != '\0') {
		obj = get_obj_list(ch,argument,ch->in_room->contents);
		if (obj == NULL) {
			char_puts("You don't see that here.\n", ch);
			return;
		}
	}
	else
		obj = ch->on;

	if (obj != NULL) {
		if (!IS_SET(obj->pIndexData->item_type,ITEM_FURNITURE)
		||  (!IS_SET(obj->value[ITEM_FURNITURE_FLAGS],SIT_ON)
		&&   !IS_SET(obj->value[ITEM_FURNITURE_FLAGS],SIT_IN)
		&&   !IS_SET(obj->value[ITEM_FURNITURE_FLAGS],SIT_AT))) {
			char_puts("You can't sit on that.\n", ch);
			return;
		}

		if (obj != NULL
		&&  ch->on != obj
		&&  count_users(obj) >= obj->value[ITEM_FURNITURE_QUANTITY]) {
			act_puts("There's no more room on $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			return;
		}

		ch->on = obj;
	}

	switch (ch->position) {
	case POS_SLEEPING:
		if (obj == NULL) {
			char_puts("You wake and sit up.\n", ch);
			act("$n wakes and sits up.", ch, NULL, NULL, TO_ROOM);
		}
		else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS],SIT_AT)) {
			act_puts("You wake and sit at $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM);
		}
		else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS],SIT_ON)) {
			act_puts("You wake and sit on $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n wakes and sits on $p.", ch, obj, NULL, TO_ROOM);
		}
		else {
			act_puts("You wake and sit in $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n wakes and sits in $p.", ch, obj, NULL, TO_ROOM);
		}

		ch->position = POS_SITTING;
		break;

	case POS_RESTING:
		if (obj == NULL)
			char_puts("You stop resting.\n", ch);
		else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS],SIT_AT)) {
			act_puts("You sit at $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n sits at $p.", ch, obj, NULL, TO_ROOM);
		}
		else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS],SIT_ON)) {
			act_puts("You sit on $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n sits on $p.", ch, obj, NULL, TO_ROOM);
		}
		else {
			act_puts("You sit in $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n sits in $p.", ch, obj, NULL, TO_ROOM);
		}
		ch->position = POS_SITTING;
		break;

	case POS_SITTING:
		char_puts("You are already sitting down.\n", ch);
		break;

	case POS_STANDING:
		if (obj == NULL) {
			char_puts("You sit down.\n", ch);
			act("$n sits down on the ground.",
			    ch, NULL, NULL, TO_ROOM);
		}
		else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS], SIT_AT)) {
			act_puts("You sit down at $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n sits down at $p.", ch, obj, NULL, TO_ROOM);
		}
		else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS],SIT_ON)) {
			act_puts("You sit down on $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n sits down on $p.", ch, obj, NULL, TO_ROOM);
		}
		else {
			act_puts("You sit down in $p.",
				 ch, obj, NULL, TO_CHAR, POS_DEAD);
			act("$n sits down in $p.", ch, obj, NULL, TO_ROOM);
		}
		ch->position = POS_SITTING;
		break;
	}

	if (IS_HARA_KIRI(ch)) {
		 char_puts("You feel your blood heats your body.\n", ch);
		 REMOVE_BIT(ch->state_flags, STATE_HARA_KIRI);
	}
}

void do_sleep(CHAR_DATA *ch, const char *argument)
{
	OBJ_DATA *obj = NULL;

	if (MOUNTED(ch)) {
		char_puts("You can't sleep while mounted.\n", ch);
		return;
	}

	if (is_affected(ch, gsn_rnet_trap)) {
		char_puts("There's not enough room to sleep!\n",ch);
		return;
	}

	if (RIDDEN(ch)) {
		char_puts("You can't sleep while being ridden.\n", ch);
		return;
	}

	switch (ch->position) {
	case POS_SLEEPING:
		char_puts("You are already sleeping.\n", ch);
		break;

	case POS_RESTING:
	case POS_SITTING:
	case POS_STANDING: 
		if (argument[0] == '\0' && ch->on == NULL) {
			char_puts("You go to sleep.\n", ch);
			act("$n goes to sleep.", ch, NULL, NULL, TO_ROOM);
		}
		else { /* find an object and sleep on it */
			if (argument[0] == '\0')
				obj = ch->on;
			else
				obj = get_obj_list(ch, argument,
						   ch->in_room->contents);

			if (obj == NULL) {
				char_puts("You don't see that here.\n", ch);
				return;
			}

			if (obj->pIndexData->item_type != ITEM_FURNITURE
			||  (!IS_SET(obj->value[ITEM_FURNITURE_FLAGS], SLEEP_ON) &&
			     !IS_SET(obj->value[ITEM_FURNITURE_FLAGS],SLEEP_IN) &&
			     !IS_SET(obj->value[ITEM_FURNITURE_FLAGS],SLEEP_AT))) {
				char_puts("You can't sleep on that.\n", ch);
				return;
			}

			if (ch->on != obj
			&&  count_users(obj) >= obj->value[ITEM_FURNITURE_QUANTITY]) {
				act_puts("There's no room on $p for you.",
					 ch, obj, NULL, TO_CHAR, POS_DEAD);
				return;
			}

			ch->on = obj;
			if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS], SLEEP_AT)) {
				act_puts("You go to sleep at $p.",
					 ch, obj, NULL, TO_CHAR, POS_DEAD);
				act("$n goes to sleep at $p.",
				    ch, obj, NULL, TO_ROOM);
			}
			else if (IS_SET(obj->value[ITEM_FURNITURE_FLAGS], SLEEP_ON)) {
				act_puts("You go to sleep on $p.",
					 ch, obj, NULL, TO_CHAR, POS_DEAD);
				act("$n goes to sleep on $p.",
				    ch, obj, NULL, TO_ROOM);
			}
			else {
				act_puts("You go to sleep in $p.",
					 ch, obj, NULL, TO_CHAR, POS_DEAD);
				act("$n goes to sleep in $p.",
				    ch, obj, NULL, TO_ROOM);
			}
		}
		ch->position = POS_SLEEPING;
		break;

	case POS_FIGHTING:
		char_puts("You are already fighting.\n", ch);
		break;
	}
}

void do_wake(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0')
		{ do_stand(ch, argument); return; }

	if (!IS_AWAKE(ch))
		{ char_puts("You are asleep yourself!\n", ch); return; }

	if ((victim = get_char_room(ch, arg)) == NULL)
		{ char_puts("They aren't here.\n", ch); return; }

	if (IS_AWAKE(victim)) { 
		act_puts("$N is already awake.",
			 ch, NULL, victim, TO_CHAR, POS_DEAD);
		return;
	}

	if (IS_AFFECTED(victim, AFF_SLEEP)) { 
		act_puts("You can't wake $M!",
			 ch, NULL, victim, TO_CHAR, POS_DEAD); 
		return; 
	}

	act_puts("$n wakes you", ch, NULL, victim, TO_VICT, POS_SLEEPING);
	do_sit(victim, str_empty);
	return;
}

/*
 * Contributed by Alander
 */
void do_visible(CHAR_DATA *ch, const char *argument)
{
	remove_hide_affect(ch, AFF_HIDE | AFF_FADE | AFF_CAMOUFLAGE);

	if (IS_AFFECTED(ch, AFF_HIDE | AFF_FADE)) {
		act_puts("You step out of shadows.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		act_puts("$n steps out of shadows.",
			 ch, NULL, NULL, TO_ROOM, POS_RESTING);
        	}

	if (IS_AFFECTED(ch, AFF_CAMOUFLAGE)) {
		act_puts("You step out from your cover.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		act("$n steps out from $s cover.",
		    ch, NULL, NULL, TO_ROOM);
	}

	if (IS_AFFECTED(ch, AFF_INVIS | AFF_IMP_INVIS)) {
		REMOVE_BIT(ch->affected_by, AFF_INVIS | AFF_IMP_INVIS);
		affect_bit_strip(ch, TO_AFFECTS, AFF_INVIS | AFF_IMP_INVIS);
		act_puts("You fade into existence.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		act("$n fades into existence.", ch, NULL, NULL, TO_ROOM);
	}
}

void do_recall(CHAR_DATA *ch, const char *argument)
{
	ROOM_INDEX_DATA *location;
	CHAR_DATA *gch;
 
	if (IS_NPC(ch)) {
		char_puts("Only players can recall.\n", ch);
		return;
	}

	if (ch->level >= 20 && !IS_IMMORTAL(ch)) {
		char_puts("Recall is for only levels below 20.\n", ch);
		return;
	}

	if (ch->desc) {
	/*	if (IS_PUMPED(ch)) {
			act_puts("You are too pumped to pray now.",
				 ch, NULL, NULL, TO_CHAR, POS_DEAD);
			return;
		}*/
		location = get_recall(ch);
	} 
	else 
		location = get_random_recall();

	act("$n prays for transportation!", ch, NULL, NULL, TO_ROOM);
	
	if (ch->in_room == location)
		return;

	if (IS_SET(ch->in_room->room_flags, ROOM_NORECALL)
	||  IS_AFFECTED(ch, AFF_CURSE) 
	||  IS_RAFFECTED(ch->in_room, RAFF_CURSE)) {
		char_puts("The gods have forsaken you.\n", ch);
		return;
	}

	ch->move /= 2;
	recall(ch, location);
        for (gch = npc_list; gch; gch = gch->next) {
                if (IS_AFFECTED(gch, AFF_CHARM)
                &&  gch->master == ch
		&& !IS_AFFECTED(gch, AFF_CURSE) 
		&& !IS_AFFECTED(gch, AFF_SLEEP)
		&& gch->position >= POS_STANDING) {
			recall(gch, location);
                }
        }
}

void do_train(CHAR_DATA *ch, const char *argument)
{
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *mob;
	int stat = - 1;
	char *pOutput = NULL;

	if (IS_NPC(ch))
		return;

	/*
	 * Check for trainer.
	 */
	for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
		if (IS_NPC(mob)
		&&  IS_SET(mob->pIndexData->act,
			   ACT_PRACTICE | ACT_TRAIN | ACT_GAIN))
			break;

	if (mob == NULL) {
		char_puts("You can't do that here.\n", ch);
		return;
	}

	if (argument[0] == '\0') {
		char_printf(ch, "You have %d training sessions.\n",
			    ch->train);
		argument = "foo";
	}

	if (!str_cmp(argument, "str")) {
		stat        = STAT_STR;
		pOutput     = "strength";
	}
	else if (!str_cmp(argument, "int")) {
		stat	    = STAT_INT;
		pOutput     = "intelligence";
	}
	else if (!str_cmp(argument, "wis")) {
		stat	    = STAT_WIS;
		pOutput     = "wisdom";
	}
	else if (!str_cmp(argument, "dex")) {
		stat  	    = STAT_DEX;
		pOutput     = "dexterity";
	}
	else if (!str_cmp(argument, "con")) {
		stat	    = STAT_CON;
		pOutput     = "constitution";
	}
	else if (!str_cmp(argument, "cha")) {
		stat	    = STAT_CHA;
		pOutput     = "charisma";
	}
	else {
		snprintf(buf, sizeof(buf),
			 GETMSG("You can train: %s%s%s%s%s%s", ch->lang),
			 ch->perm_stat[STAT_STR] < get_max_train(ch, STAT_STR) ?
			 	" str" : str_empty,
			 ch->perm_stat[STAT_INT] < get_max_train(ch, STAT_INT) ?
			 	" int" : str_empty,
			 ch->perm_stat[STAT_WIS] < get_max_train(ch, STAT_WIS) ?
			 	" wis" : str_empty,
			 ch->perm_stat[STAT_DEX] < get_max_train(ch, STAT_DEX) ?
			 	" dex" : str_empty,
			 ch->perm_stat[STAT_CON] < get_max_train(ch, STAT_CON) ?
			 	" con" : str_empty,
			 ch->perm_stat[STAT_CHA] < get_max_train(ch, STAT_CHA) ?
			 	" cha" : str_empty);

		if (buf[strlen(buf)-1] != ':')
			char_printf(ch, "%s.\n", buf);
		else {
			/*
			 * This message dedicated to Jordan ... you big stud!
			 */
			act("You have nothing left to train, you $T!",
			    ch, NULL,
			    ch->sex == SEX_MALE   ? "big stud" :
			    ch->sex == SEX_FEMALE ? "hot babe" :
						    "wild thing",
			    TO_CHAR | ACT_TRANS);
		}
		return;
	}

	if (ch->perm_stat[stat] >= get_max_train(ch,stat)) {
		act_puts("Your $T is already at maximum.",
			 ch, NULL, pOutput, TO_CHAR | ACT_TRANS, POS_DEAD);
		return;
	}

	if (ch->train < 1) {
		char_puts("You don't have enough training sessions.\n", ch);
		return;
	}

	ch->train--;
	ch->perm_stat[stat] += 1;
	act_puts("Your $T increases!",
		 ch, NULL, pOutput, TO_CHAR | ACT_TRANS, POS_DEAD);
	act("$n's $T increases!", ch, NULL, pOutput, TO_ROOM | ACT_TRANS);
}

void do_fly(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];

	if (IS_NPC(ch))
		return;

	argument = one_argument(argument, arg, sizeof(arg));

	if (!str_cmp(arg,"up")) {
		race_t *r;

		if (IS_AFFECTED(ch, AFF_FLYING)) {		       
			char_puts("You are already flying.\n", ch); 
			return;
		}

		if (is_bit_affected(ch, TO_AFFECTS, AFF_FLYING)
		||  ((r = race_lookup(ch->race)) && (r->aff & AFF_FLYING))
		||  has_obj_affect(ch, AFF_FLYING)) {
			SET_BIT(ch->affected_by, AFF_FLYING);
			char_puts("You start to fly.\n", ch);
		}
		else {
			char_puts("To fly find potion or wings.\n", ch); 
			return;
		}
	}
	else if (!str_cmp(arg,"down")) {
		if (IS_AFFECTED(ch,AFF_FLYING)) {
			REMOVE_BIT(ch->affected_by, AFF_FLYING);
			char_puts("You slowly touch the ground.\n", ch);
		}
		else {		       
			char_puts("You are already on the ground.\n", ch); 
			return;
		}
	}
 	else {
		char_puts("Type fly with 'up' or 'down'.\n", ch);
		return;
	}

	WAIT_STATE(ch, SKILL(gsn_fly)->beats);   
}

int mount_success(CHAR_DATA *ch, CHAR_DATA *mount, int canattack)
{
	int	percent;
	int	success;
	int	chance;

	if ((chance = get_skill(ch, gsn_riding)) == 0)
		return FALSE;

	percent = number_percent() + (ch->level < mount->level ? 
		  (mount->level - ch->level) * 3 : 
		  (mount->level - ch->level) * 2);

	if (!ch->fighting)
		percent -= 25;

	if (!IS_NPC(ch) && IS_DRUNK(ch)) {
		percent += chance / 2;
		char_puts("Due to your being under the influence, riding seems "
			  "a bit harder...\n", ch);
	}

	success = percent - chance;

	if (success <= 0) { /* Success */
		check_improve(ch, gsn_riding, TRUE, 1);
		return TRUE;
	}

	check_improve(ch, gsn_riding, FALSE, 1);
	if (success >= 10 && MOUNTED(ch) == mount) {
		act_puts("You lose control and fall off of $N.",
			 ch, NULL, mount, TO_CHAR, POS_DEAD);
		act("$n loses control and falls off of $N.",
		    ch, NULL, mount, TO_NOTVICT);
		act_puts("$n loses control and falls off of you.",
			 ch, NULL, mount, TO_VICT, POS_SLEEPING);

		ch->riding = FALSE;
		mount->riding = FALSE;
		if (ch->position > POS_STUNNED) 
			ch->position=POS_SITTING;
	
		ch->hit -= 5;
		update_pos(ch);
	}
	if (success >= 40 && canattack) {
		act_puts("$N doesn't like the way you've been treating $M.",
			 ch, NULL, mount, TO_CHAR, POS_DEAD);
		act("$N doesn't like the way $n has been treating $M.",
		    ch, NULL, mount, TO_NOTVICT);
		act_puts("You don't like the way $n has been treating you.",
			 ch, NULL, mount, TO_VICT, POS_SLEEPING);

		act_puts("$N snarls and attacks you!",
			 mount, NULL, ch, TO_VICT, POS_DEAD);
		act("$N snarls and attacks $n!",
		    mount, NULL, ch, TO_NOTVICT);
		act_puts("You snarl and attack $n!",
			 mount, NULL, ch, TO_CHAR, POS_SLEEPING);

		damage(mount, ch, number_range(1, mount->level),
			gsn_kick, DAM_BASH, DAMF_SHOW);
	}
	return FALSE;
}

/*
 * It is not finished yet to implement all.
 */
void do_mount(CHAR_DATA *ch, const char *argument)
{
	char 		arg[MAX_INPUT_LENGTH];
	CHAR_DATA *	mount;

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		if (ch->mount && ch->mount->in_room == ch->in_room)
			mount = ch->mount;
		else {
			char_puts("Mount what?\n", ch);
			return;
		}
	}
	else if ((mount = get_char_room(ch, arg)) == NULL) {
		char_puts("You don't see that here.\n", ch);
		return;
  	}
 
	if (!IS_NPC(mount) || !IS_SET(mount->pIndexData->act, ACT_RIDEABLE)
	||  IS_SET(mount->pIndexData->act, ACT_NOTRACK)) { 
		char_puts("You can't ride that.\n", ch); 
		return;
	}
  
	if (mount->level - 5 > ch->level) {
		char_puts("That beast is too powerful for you to ride.\n", ch);
		return;
	}

	if ((mount->mount) && (!mount->riding) && (mount->mount != ch)) {
		act_puts("$N belongs to $i, not you.",
			 ch, mount->mount, mount,
			 TO_CHAR, POS_DEAD);
		return;
	} 

	if (mount->position < POS_STANDING) {
		char_puts("Your mount must be standing.\n", ch);
		return;
	}

	if (RIDDEN(mount)) {
		char_puts("This beast is already ridden.\n", ch);
		return;
	} else if (MOUNTED(ch)) {
		char_puts("You are already riding.\n", ch);
		return;
	}

	if(!mount_success(ch, mount, TRUE)) {
		char_puts("You fail to mount the beast.\n", ch);  
		return; 
	}

	act_puts("You hop on $N's back.", ch, NULL, mount, TO_CHAR, POS_DEAD);
	act("$n hops on $N's back.", ch, NULL, mount, TO_NOTVICT);
	act_puts("$n hops on your back!", ch, NULL, mount, TO_VICT, POS_SLEEPING);
 
	ch->mount = mount;
	ch->riding = TRUE;
	mount->mount = ch;
	mount->riding = TRUE;
  
	remove_hide_affect(ch, AFF_HIDE | AFF_FADE | AFF_CAMOUFLAGE);
	affect_bit_strip(ch, TO_AFFECTS, AFF_INVIS | AFF_IMP_INVIS | AFF_SNEAK);
	REMOVE_BIT(ch->affected_by, AFF_HIDE | AFF_FADE | AFF_CAMOUFLAGE |
				    AFF_INVIS | AFF_IMP_INVIS | AFF_SNEAK);
}

void do_dismount(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *mount;

	if ((mount = MOUNTED(ch))) {
		act_puts("You dismount from $N.",
			 ch, NULL, mount, TO_CHAR, POS_DEAD); 
		act("$n dismounts from $N.", ch, NULL, mount, TO_NOTVICT);
		act_puts("$n dismounts from you.",
			 ch, NULL, mount, TO_VICT, POS_SLEEPING);

		ch->riding = FALSE;
		mount->riding = FALSE;
	}
	else {
		char_puts("You aren't mounted.\n", ch);
		return;
	}
} 

char *find_way(CHAR_DATA *ch,ROOM_INDEX_DATA *rstart, ROOM_INDEX_DATA *rend) 
{
	int direction;
	static char buf[1024];
	EXIT_DATA *pExit;
	char buf2[2];

	snprintf(buf, sizeof(buf), "Bul: ");
	while (1) {
		if ((rend == rstart))
			return buf;

		if ((direction = find_path(rstart->vnum, rend->vnum,
					   ch, -40000, 0)) == -1) {
			strnzcat(buf, sizeof(buf), " BUGGY");
			return buf;
		}

		if (direction < 0 || direction > 5) {
			strnzcat(buf, sizeof(buf), " VERY BUGGY");
			return buf;
		}

		buf2[0] = dir_name[direction][0];
		buf2[1] = '\0';
		strnzcat(buf, sizeof(buf), buf2);

		/* find target room */
		pExit = rstart->exit[ direction ];
		if (!pExit)  {
			strnzcat(buf, sizeof(buf), " VERY VERY BUGGY");
			return buf;
		}
		else
			rstart = pExit->to_room.r;
	}
}	

/* RT Enter portals */
void do_enter(CHAR_DATA *ch, const char *argument)
{    
	ROOM_INDEX_DATA *location; 
	ROOM_INDEX_DATA *old_room;
	OBJ_DATA *portal;
	CHAR_DATA *fch, *fch_next, *mount;

	if (ch->fighting != NULL) 
		return;

	/* nifty portal stuff */
	if (argument[0] == '\0') {
		char_puts("Nope, can't do it.\n",ch);
		return;
	}

	old_room = ch->in_room;
	portal = get_obj_list(ch, argument, ch->in_room->contents);
	
	if (portal == NULL) {
		char_puts("You don't see that here.\n",ch);
		return;
	}

	if (portal->pIndexData->item_type != ITEM_PORTAL 
	||  (IS_SET(portal->value[ITEM_PORTAL_EXIT_FLAGS], EX_CLOSED) 
	&& !IS_TRUSTED(ch, ANGEL))) {
		char_puts("You can't seem to find a way in.\n", ch);
		return;
	}

	if (IS_SET(portal->value[ITEM_PORTAL_FLAGS], GATE_NOCURSE)
	&&  !IS_TRUSTED(ch, ANGEL)
	&&  (IS_AFFECTED(ch, AFF_CURSE) ||
	     IS_SET(old_room->room_flags, ROOM_NORECALL) ||
	     IS_RAFFECTED(old_room, RAFF_CURSE))) {
		char_puts("Something prevents you from leaving...\n",ch);
		return;
	}

	if (IS_SET(portal->value[ITEM_PORTAL_FLAGS], GATE_RANDOM) 
	|| portal->value[ITEM_PORTAL_DEST] == -1) {
		location = get_random_room(ch, NULL);
		if (location != NULL)
			portal->value[ITEM_PORTAL_DEST] = location->vnum; /* keeps record */
	}
	else if (IS_SET(portal->value[ITEM_PORTAL_FLAGS], GATE_BUGGY) 
	&& (number_percent() < 5))
		location = get_random_room(ch, NULL);
	else
		location = get_room_index(portal->value[ITEM_PORTAL_DEST]);

	if (location == NULL
	||  location == old_room
	||  !can_see_room(ch, location) 
	||  (room_is_private(location) && !IS_TRUSTED(ch, IMPLEMENTOR))) {
		act("$p doesn't seem to go anywhere.", ch, portal,NULL,TO_CHAR);
		return;
	}

	if (IS_NPC(ch) && IS_AGGRO(ch, NULL)
	&&  IS_SET(location->room_flags, ROOM_LAW)) {
	        char_puts("Something prevents you from leaving...\n",ch);
	        return;
	}

	act(MOUNTED(ch) ? "$n steps into $p, riding on $N." :
			  "$n steps into $p.",
	    ch, portal, MOUNTED(ch), TO_ROOM);
	
	act(IS_SET(portal->value[ITEM_PORTAL_FLAGS], GATE_NORMAL_EXIT) ?
	    "You enter $p." :
	    "You walk through $p and find yourself somewhere else...",
	    ch, portal, NULL, TO_CHAR); 

	mount = MOUNTED(ch);
	char_from_room(ch);

	if (IS_SET(portal->value[ITEM_PORTAL_FLAGS], GATE_GOWITH)) {/* take the gate along */
		obj_from_room(portal);
		obj_to_room(portal, location);
	}

	if (IS_SET(portal->value[ITEM_PORTAL_FLAGS], GATE_NORMAL_EXIT))
		act_puts3(mount ? "$i arrives, riding $I" :
				  "$i arrives.",
			  location->people, ch, portal, mount,
			  TO_ROOM, POS_RESTING);
	else
		act_puts3(mount ? "$i arrives through $P, riding $I." :
	        		  "$i arrives through $P.",
			  location->people, ch, portal, mount,
			  TO_ROOM, POS_RESTING);

	char_to_room(ch, location);

	/* uncamo */
	if (location->sector_type != SECT_FOREST
	&& location->sector_type != SECT_ARCTIC
	&& IS_AFFECTED(ch, AFF_CAMOUFLAGE))
		REMOVE_BIT(ch->affected_by, AFF_CAMOUFLAGE);

	if (mount) {
		char_from_room(mount);
		char_to_room(mount, location);
  		ch->riding = TRUE;
  		mount->riding = TRUE;
	}

	if (!JUST_KILLED(ch))
		do_look(ch,"auto");

	/* charges */
	if (portal->value[ITEM_PORTAL_CHARGES] > 0) {
		portal->value[ITEM_PORTAL_CHARGES]--;
		if (portal->value[ITEM_PORTAL_CHARGES] == 0)
			portal->value[ITEM_PORTAL_CHARGES] = -1;
	}

	/* protect against circular follows */
	if (old_room == location)
		return;

	for (fch = old_room->people; fch != NULL; fch = fch_next) {
	        fch_next = fch->next_in_room;

		/* no following through dead portals */
	        if (portal == NULL 
		|| portal->value[ITEM_PORTAL_CHARGES] == -1) 
	        	continue;
 
	        if (fch->master != ch || fch->position != POS_STANDING)
			continue;

	        if (IS_SET(location->room_flags, ROOM_LAW)
	        &&  IS_NPC(fch)
		&&  IS_AGGRO(fch, NULL)) {
	        	act("You can't bring $N into the city.",
	                    ch, NULL, fch, TO_CHAR);
			act("You aren't allowed in the city.",
			    fch, NULL, NULL, TO_CHAR);
			continue;
	        }
 
	        act("You follow $N.", fch, NULL, ch, TO_CHAR);
		do_enter(fch,argument);
	}

 	if (portal != NULL && portal->value[ITEM_PORTAL_CHARGES] == -1) {
		act("$p fades out of existence.", ch, portal, NULL, TO_CHAR);
		if (ch->in_room == old_room)
			act("$p fades out of existence.",
			    ch, portal, NULL, TO_ROOM);
		else if (old_room->people != NULL) {
			act("$p fades out of existence.", 
			    old_room->people, portal, NULL, TO_CHAR);
			act("$p fades out of existence.",
			    old_room->people,portal,NULL,TO_ROOM);
		}
		extract_obj(portal, 0);
	}

	if (JUST_KILLED(ch))
		return;

	/* 
	 * If someone is following the char, these triggers get
	 * activated for the followers before the char,
	 * but it's safer this way...
	 */
	if (IS_NPC(ch) && HAS_TRIGGER(ch, TRIG_ENTRY))
		mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_ENTRY);
	if (!IS_NPC(ch))
		mp_greet_trigger(ch);
}

