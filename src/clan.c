/*-
 * Copyright (c) 1998 fjoe <fjoe@iclub.nsu.ru>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * $Id: clan.c 933 2006-11-19 22:37:00Z zsuzsu $
 */

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if	defined (WIN32)
#	include <compat/compat.h>
#else
#	include <dirent.h>
#endif

#include "merc.h"
#include "debug.h"
#include "usage.h"

DECLARE_DO_FUN(do_asave);
void do_promote(CHAR_DATA *ch, const char *argument);
void promote_all(CHAR_DATA *ch, clan_t *clan, const char *name, 
			int oldrank, int rank);
void clan_promotion (CHAR_DATA *ch, clan_t *clan, const char *name, 
			int oldrank, int rank);
bool fits_clan_requirements (CHAR_DATA *victim, int cln);

varr clans = { sizeof(clan_t), 4 };
/*
 * Clan rewritten for structure and multiple ranks 
 * by Zsuzsu
 */
const struct clan_titles default_clan_rank_table[] = {
    {
         {"Outcast",	"Outcast",	"Outcast"	}
    },
    {
         {"Probation",	"Probation",	"Probation"	}
    },
    {
         {"Recruit",	"Recruit",	"Recruit"	}
    },
    {
         {"Foot Soldier","Foot Soldier","Foot Soldier"	}
    },
    {
         {"Veteran",	"Veteran",	"Veteran"	}
    },
    {
         {"Lieutenant",	"Lieutenant",	"Lieutenant"	}
    },
    {
         {"Captain",	"Captain",	"Captain"	}
    },
    {
         {"Vassal",	"Vassal",	"Vassal"	}
    },
    {
         {"Ambassador",	"Ambassador",	"Ambassadress"	}
    },
    {
         {"Magistrate",	"Magistrate",	"Magistrate"	}
    },
    {
         {"Lord",	"Lord",		"Lady"		}
    },
    {
         {"Elder",	"Elder",	"Elder"		}
    },
    {
         {"Leader",	"Leader",	"Leader"	}
    },
    {
         {"Patron",	"Patron",	"Patron"	}
    },
    {
         { NULL, NULL, NULL }
    }
};

clan_t *clan_new(void)
{
	clan_t *clan;
	int i,j;

	clan = varr_enew(&clans);
	clan->skills.nsize = sizeof(clskill_t);
	clan->skills.nstep = 4;

	/*copy default rank names*/
	for(i=0; i< MAX_CLAN_RANK; i++) {
		for (j=0; j < 3; j++)
			clan->rank_table[i].title[j] = default_clan_rank_table[i].title[j];
	}

	return clan;
}

void clan_free(clan_t *clan)
{
	varr_free(&clan->skills);
}

void clan_save(clan_t *clan)
{
	SET_BIT(clan->flags, CLAN_CHANGED);
	do_asave(NULL, "clans");
}

int cln_lookup(const char *name)
{
	int cln;

	if (IS_NULLSTR(name))
		return -1;

	for (cln = 0; cln < clans.nused; cln++)
		if (!str_cmp(name, CLAN(cln)->name))
			return cln;

	return -1;
}

const char *clan_name(int cln)
{
	clan_t *clan = clan_lookup(cln);
	if (clan)
		return clan->name;
	return "None";
}

void do_petitio(CHAR_DATA *ch, const char *argument)
{
	char_puts("You must enter full command to petition.\n",ch);
}

/*
 * clan_update_lists - remove 'victim' the clan list
 */
void clan_update_lists(clan_t *clan, CHAR_DATA *victim) {
	clan_update_list(clan, victim->name);
}

void clan_update_list(clan_t *clan, const char *name)
{
	const char **nl = NULL;
	int i =0;

	for (i = 0; i < MAX_CLAN_RANK; i++) {
		nl = &clan->member_list[i];
		if (nl)
			name_delete(nl, name, NULL, NULL);
	}
}

void do_mark(CHAR_DATA *ch, const char *argument)
{
	OBJ_DATA *mark;
	clan_t *clan = NULL;

	if ((ch->clan == 0) || ((clan=clan_lookup(ch->clan)) == NULL)) {
		char_puts("You are not in clan.\n", ch);
		return;
	}
	if (!clan->mark_vnum) {
		char_puts ("Your clan do not have any mark.\n", ch);
		return;
	}
	if ((mark=get_eq_char(ch, WEAR_CLANMARK))!=NULL) {
		obj_from_char(mark);
		extract_obj(mark, 0);
	}
	mark = create_obj(get_obj_index(clan->mark_vnum), 0);
	obj_to_char (mark, ch);
	equip_char (ch, mark, WEAR_CLANMARK);
	char_puts ("You are now inducted into the clan.\n", ch);
}

void do_petition(CHAR_DATA *ch, const char *argument)
{
	bool accept;
	int cln = 0;
	clan_t *clan = NULL;
	char arg1[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *mark;

	if (IS_NPC(ch))
		return;	

	if (IS_NEWBIE(ch)) {
		char_puts("Newbies can't join clans.\n", ch);
		return;
	}

	argument = one_argument(argument, arg1, sizeof(arg1));

	if (IS_NULLSTR(arg1)) {
		if (IS_IMMORTAL(ch)
		||  (ch->clan && (ch->pcdata->clan_status >= CLAN_MAGISTRATE
				  && ch->pcdata->clan_status != CLAN_ELDER)))
			char_printf(ch,
				    "Usage: petition %s<accept | reject> "
				    "<char name>\n",
				    IS_IMMORTAL(ch) ? "<clan name> " : str_empty);
		if (IS_IMMORTAL(ch) || !ch->clan)
			char_puts("Usage: petition <clan name>\n", ch);
		return;
	}

	if (IS_IMMORTAL(ch)) {
		cln = cln_lookup(arg1);
		if (cln <= 0) {
			char_printf(ch, "%s: unknown clan\n", arg1);
			do_petition(ch, str_empty);
			return;
		}
		argument = one_argument(argument, arg1, sizeof(arg1));
		if (IS_NULLSTR(arg1)) {
			do_petition(ch, str_empty);
			return;
		}
		clan = CLAN(cln);
	}

	if ((accept = !str_prefix(arg1, "accept"))
	||  !str_prefix(arg1, "reject")) {
		CHAR_DATA *victim;
		char arg2[MAX_STRING_LENGTH];

		if (!IS_IMMORTAL(ch)) {
			if (ch->clan == 0
			||  (clan = clan_lookup(ch->clan)) == NULL) {
				do_petition(ch, str_empty);
				return;
			}
			cln = ch->clan;
		}

		argument = one_argument(argument, arg2, sizeof(arg2));
		if (IS_NULLSTR(arg2)) {
			do_petition(ch, str_empty);
			return;
		}

		if ((ch->pcdata->clan_status < CLAN_MAGISTRATE 
		   || ch->pcdata->clan_status == CLAN_ELDER)
		&&  !IS_IMMORTAL(ch)) {
			char_puts("You don't have the authority to "
				  "accept/reject petitions.\n", ch);
			return;
		}

                if (!(victim = get_char_world(ch, arg2))
                ||  IS_NPC(victim)) {
                        char_puts("Can't find them.\n", ch);
                        return;
                }

		if (IS_NEWBIE(victim)) {
                        char_puts("Newbies can't join clans.\n", ch);
                        return;
		}

		if (accept) {
			if (victim->pcdata->petition != cln) {
				char_puts("They didn't petition.\n", ch);
				return;
			}

			if (!fits_clan_requirements(victim, cln)) {
				act("Hmm, doesn't look like $N fits the clan requirements.",
					ch, NULL, victim, TO_CHAR);
				act("You do not seem to fit the clan's requiremnts.",
					victim, NULL, victim, TO_CHAR);
				return;
			}

			victim->clan = cln;
			victim->pcdata->clan_status = CLAN_RECRUIT;
			update_skills(victim);

			name_add(&clan->member_list[CLAN_RECRUIT], victim->name, NULL, NULL);
			clan_save(clan);

			snprintf(buf, MAX_STRING_LENGTH, 
				"$N has inducted {W%s{x into %s.",
				victim->name,
				clan_name(cln));

			wiznet(buf, ch, NULL, WIZ_POLITICS, 0, ch->level);

			char_puts("Greet new member!\n", ch);
			char_printf(victim, "Your petition to %s has been "
				    "accepted.\n",
				    clan->name);
			char_printf(victim, "You are now one of %s!\n",
				    clan->name);
			if ((mark = get_eq_char(victim, WEAR_CLANMARK)) != NULL) {
				obj_from_char(mark);
				extract_obj(mark, 0);
			}
			if (clan->mark_vnum) {
				mark = create_obj(get_obj_index(clan->mark_vnum), 0);
				obj_to_char(mark, victim);
				equip_char(victim, mark, WEAR_CLANMARK);
			};
			return;
		}

/* handle 'petition reject' */
		if (victim->clan == cln) {
			if (victim->pcdata->clan_status == CLAN_LEADER
			&&  !IS_IMMORTAL(ch)) {
				char_puts("You don't have enough power "
					  "to do that.\n", ch);
				return;
			}

			clan_update_lists(clan, victim);
			clan_save(clan);

			snprintf(buf, MAX_STRING_LENGTH, 
				"$N has removed {W%s{x from %s.",
				victim->name,
				clan_name(cln));

			wiznet(buf, ch, NULL, WIZ_POLITICS, 0, ch->level);


			victim->clan = CLAN_NONE;
			REMOVE_BIT(victim->pcdata->trust, TRUST_CLAN);
			update_skills(victim);

			char_printf(ch, "They are not a member of %s "
					"anymore.\n", clan->name);
			char_printf(victim, "You are not a member of %s "
					    "anymore.\n", clan->name);

			if ((mark = get_eq_char(victim, WEAR_CLANMARK))) {
				obj_from_char(mark);
				extract_obj(mark, 0);
			}

			return;
		}

		if (victim->pcdata->petition == cln) {
			victim->pcdata->petition = CLAN_NONE;
			char_puts("Petition was rejected.\n", ch);
			char_printf(victim, "Your petition to %s was "
				    "rejected.\n",
				    clan->name);
			return;
		}

		char_puts("They didn't petition.\n", ch);
		return;
	}

	if (IS_IMMORTAL(ch)
	||  (ch->clan && (ch->pcdata->clan_status >= CLAN_MAGISTRATE 
			  && ch->pcdata->clan_status != CLAN_ELDER))) {
		DESCRIPTOR_DATA *d;
		bool found = FALSE;

		if (IS_IMMORTAL(ch)) {
			if ((cln = cln_lookup(arg1)) <= 0) {
				char_puts("No such clan.\n", ch);
				return;
			}
		}
		else
			cln = ch->clan;
			
		for (d = descriptor_list; d; d = d->next) {
			CHAR_DATA *vch = d->original ? d->original :
						       d->character;

			if (!vch
			||  vch->clan
			||  vch->pcdata->petition != cln)
				continue;

			if (!found) {
				found = TRUE;
				char_puts("List of players petitioned to "
					  "your clan:\n", ch);
			}
			char_printf(ch, "%s\n", vch->name);
		}

		if (!found) 
			char_puts("Noone has petitioned to your clan.\n", ch);
		return;
	}

	if ((cln = cln_lookup(arg1)) <= 0) {
		char_puts("No such clan.\n", ch);
		return;
	}

	if (ch->clan) {
		char_puts("You cannot leave your clan this way.\n", ch);
		return;
	}

	ch->pcdata->petition = cln;
	char_puts("Petition sent.\n", ch);
}

int get_clan_rank_from_list(clan_t *clan, const char *name) {
	int found = -1;
	int i = 0;

	for (i = MAX_CLAN_RANK-1; i >= 0 && found == -1; i--) {
		if (name_find(clan->member_list[i], name))
			found = i;
	}
	return found;
}

void promote_all(CHAR_DATA *ch, clan_t *clan, const char *list, int oldrank, int rank)
{
        char name[MAX_STRING_LENGTH];
        char *str = (char *) malloc (sizeof(char) * MAX_STRING_LENGTH);

	strncpy(str, list, MAX_STRING_LENGTH);
        
	str = (char *) first_arg((const char *) str, name, sizeof(name), FALSE);

        while (name[0]) {
		clan_promotion(ch, clan, name, oldrank, rank);
		str = (char *) first_arg((const char *) str, name, sizeof(name), FALSE);
	}
	free(str);
}

void do_promote(CHAR_DATA *ch, const char *argument)
{
	char arg1[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	char arg3[MAX_STRING_LENGTH];
	char *vname;
	CHAR_DATA *victim;
	clan_t *clan;
	int newrank = 0;
	int rank = 0;

	if (IS_NPC(ch)
	||  (!IS_IMMORTAL(ch) && ch->pcdata->clan_status < CLAN_MAGISTRATE)) {
		char_puts("Huh?\n", ch);
		return;
	}

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
	argument = one_argument(argument, arg3, sizeof(arg3));

	if (arg1 == NULL || arg2 == NULL 
	    || arg1[0] == '\0' 
	    || arg2[0] == '\0' 
	    || !isdigit(arg2[0])) {
		if (IS_IMMORTAL(ch))
			char_printf(ch, "Usage: promote <char name> <0-%d> [clan]", 
				CLAN_LEADER);
		else 
			char_printf(ch, "Usage: promote <char name> <0-%d>", CLAN_LEADER-1);
		return;
	}
	else
	    newrank = atoi(arg2);

	if (newrank < 0 || newrank > MAX_CLAN_RANK-1) {
		char_puts("Invalid clan rank number.\n",ch);
		return;
	}

	victim = get_char_world_unrestricted(arg1);
	if (!victim || IS_NPC(victim)) {
		vname = arg1;
		vname[0] = toupper(vname[0]);
		if (IS_IMMORTAL(ch)) {
			if (arg3 == NULL || arg3[0] == '\0') {
				char_puts("They aren't here.\n", ch);
				return;
			}
			else {
				if ((clan = clan_lookup(cln_lookup(arg3))) == NULL) {
					char_puts("Can't find that clan.\n", ch);
					return;
				}
			}
		}
		else if (ch->pcdata->clan_status == CLAN_LEADER) {
			clan = clan_lookup(ch->clan);
		}
		else {
				char_puts("They aren't here.\n", ch);
				return;
		}

		if ((rank = get_clan_rank_from_list(clan, vname)) == -1) {
			char_puts("They aren't a member of that clan.\n", ch);
			return;
		}
	}
	/* if they are here */
	else {
		if (victim->clan == 0 || (clan = clan_lookup(victim->clan)) == NULL
		||  (victim->clan != ch->clan 
			&& !IS_IMMORTAL(ch))) {
			char_puts("They are not in your clan.\n", ch);
			return;
		}
		clan = clan_lookup(victim->clan);
		vname = (char *) victim->name;
		rank = victim->pcdata->clan_status;
	}

	if (!IS_NPC(ch) && IS_IMMORTAL(ch)) {
		if (clan_lookup(ch->clan) != clan
		&& ch->level < GOD) {
			char_puts("You don't have the power to affect another clan.\n",
				  ch);
			return;
		}
	}

	if (!IS_IMMORTAL(ch) && rank == CLAN_LEADER && ch != victim) {
		char_puts("You don't have the power to affect a leader's rank.\n",
			  ch);
		return;
	}

	
	if (!IS_NPC(ch) && !IS_IMMORTAL(ch)) {
		if  (ch->pcdata->clan_status < CLAN_MAGISTRATE 
		    || ch->pcdata->clan_status == CLAN_ELDER) {
			char_puts("You do not have the authority to promote anyone.\n", ch);
			return;
		}
		else if (newrank >= ch->pcdata->clan_status) {
			char_puts("You do not have the authority to promote someone that high.\n", ch);
			return;
		}
	}
	if (rank == newrank) {
		char_puts("They already hold that rank in the clan.\n", ch);
		return;
	}

	if (newrank == CLAN_PATRON) {
		if (!victim) 
			char_puts("They must be present to be promoted to that rank.\n", ch);
		else if (!IS_IMMORTAL(victim))
			char_puts("Mere mortals cannot hold that rank.\n", ch);
		return;
	}

	/*make the old leader(s) veterans */
	if (newrank == CLAN_LEADER && !is_name_empty(clan->member_list[CLAN_LEADER])) {
		promote_all(ch, clan, clan->member_list[CLAN_LEADER], 
			CLAN_LEADER, CLAN_VETERAN);

	}
	/* do the promotion */
	clan_promotion(ch, clan, vname, rank, newrank);
	clan_save(clan);
}

void clan_promotion (CHAR_DATA *ch, clan_t *clan, const char *name, int oldrank, int rank) {
	CHAR_DATA *victim;
	char buf[MAX_STRING_LENGTH];

	clan_update_list(clan, name);

	name_add(&clan->member_list[rank], name, NULL, NULL);

	victim = get_char_world_unrestricted(name);

	if (victim && !IS_NPC(victim)) {
		oldrank = victim->pcdata->clan_status;
		victim->pcdata->clan_status = rank;
		char_printf(victim, "You have been %smoted{x to clan %s.\n", 
			(oldrank > rank) ? "{yde" : "{cpro",
			clan->rank_table[rank].title[victim->sex]);

		snprintf(buf, MAX_STRING_LENGTH, "$N has %smoted {W%s{x from %s to %s of %s.",
			(oldrank > rank) ? "{yde" : "{cpro",
			victim->name,
			clan->rank_table[oldrank].title[victim->sex],
			clan->rank_table[rank].title[victim->sex],
			clan->name);

		wiznet(buf, ch, NULL, WIZ_POLITICS, 0, ch->level);
	}
	else {
		snprintf(buf, MAX_STRING_LENGTH, "$N has %smoted {W%s{x from %s to %s of %s.",
			(oldrank > rank) ? "{yde" : "{cpro",
			name,
			clan->rank_table[oldrank].title[SEX_MALE],
			clan->rank_table[rank].title[SEX_MALE],
			clan->name);

		wiznet(buf, ch, NULL, WIZ_POLITICS, 0, ch->level);
	}
	char_printf(ch, "%s is now a clan %s.\n",
		name, clan->rank_table[rank].title[SEX_MALE]);
}

void show_clanlist(CHAR_DATA *ch, clan_t *clan,
		   const char *list, const char *name_list)
{
	BUFFER *output;
	char name[MAX_STRING_LENGTH];
	bool found = FALSE;
	int activity = CLAN_ACTIVITY_DELETED;
	const char *active = NULL;

	output = buf_new(-1);
	buf_printf(output, "%s members of rank %s:\n", clan->name, name_list);

	list = first_arg(list, name, sizeof(name), FALSE);
	for (; name[0]; list = first_arg(list, name, sizeof(name), FALSE)) {
		found = TRUE;
		activity = clan_activity_for_name(name);
		switch (activity) {
		case CLAN_ACTIVITY_DELETED:
			active = "{r RIP  ";
			break;
		case CLAN_ACTIVITY_INACTIVE:
			active = "{D MIA  ";
			break;
		case CLAN_ACTIVITY_MIA:
			active = "{g MIA  ";
			break;
		case CLAN_ACTIVITY_AWOL:
			active = "{g AWOL ";
			break;
		case CLAN_ACTIVITY_POOR:
			active = "{g POOR ";
			break;
		case CLAN_ACTIVITY_SEMI:
			active = "{g SEMI ";
			break;
		case CLAN_ACTIVITY_ACTIVE:
			active = "{gACTIVE";
			break;
		case CLAN_ACTIVITY_HYPER:
			active = "{g HYPER";
			break;
		default:
			active = "{r ???? ";
		}

		buf_printf(output, "  {D[{g%s{D]{x {%c%s{x\n", 
			active,
		      (ch != NULL && !strcmp(ch->name, name)) ? 'G' : 'x',
		      name);
	}

	if (found)
		page_to_char(buf_string(output), ch);

	buf_free(output);
}

void do_clanlist(CHAR_DATA *ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	clan_t *clan = NULL;
	int i = 0;

	argument = one_argument(argument, arg1, sizeof(arg1));

	if (IS_IMMORTAL(ch) && arg1[0]) {
		int cln;

		if ((cln = cln_lookup(arg1)) < 0) {
			char_printf(ch, "%s: no such clan.\n", arg1);
			return;
		}
		clan = CLAN(cln);
	}

	if (!clan
	&&  (!ch->clan || (clan = clan_lookup(ch->clan)) == NULL)) {
		char_puts("You are not in a clan.\n", ch);
		return;
	}

	for (i=MAX_CLAN_RANK-1; i >= 0; i--) {
		show_clanlist(ch, clan, clan->member_list[i],
			clan->rank_table[i].title[1]); /*show male titles*/
	}
}

void do_clantitles(CHAR_DATA *ch, const char *argument) {
	char arg1[MAX_INPUT_LENGTH];
	clan_t *clan = NULL;
	int i = 0;

	argument = one_argument(argument, arg1, sizeof(arg1));

	if (IS_IMMORTAL(ch) && arg1[0]) {
		int cln;

		if ((cln = cln_lookup(arg1)) < 0) {
			char_printf(ch, "%s: no such clan.\n", arg1);
			return;
		}
		clan = CLAN(cln);
	}

	if (!clan
	&&  (!ch->clan || (clan = clan_lookup(ch->clan)) == NULL)) {
		char_puts("You are not in a clan.\n", ch);
		return;
	}

	for (i=MAX_CLAN_RANK-1; i >= 0; i--)
		char_printf(ch, " {%c%2d{D:{x {%c%25s{x {D/{x {%c%s{x\n",
		      	(i >= CLAN_MAGISTRATE && i != CLAN_ELDER) ? 'g' : 
			(i < CLAN_RECRUIT)    ? 'r' : 'w',
		      	i, 
		      	(!IS_IMMORTAL(ch) 
			 && ch->pcdata->clan_status == i
			 && ch->sex == SEX_MALE) ? 'G' : 'x',
			clan->rank_table[i].title[1],
		      	(!IS_IMMORTAL(ch) 
			 && ch->pcdata->clan_status == i
			 && ch->sex == SEX_FEMALE) ? 'G' : 'x',
			clan->rank_table[i].title[2]);

}

void do_clanitem(CHAR_DATA* ch, const char* argument)
{
	clan_t* clan = NULL;
	OBJ_DATA* in_obj;
	int cln;
	char arg[MAX_STRING_LENGTH];

	one_argument(argument, arg, sizeof(arg));
	if (IS_IMMORTAL(ch) && arg[0]) {
		if ((cln = cln_lookup(arg)) < 0) {
			char_printf(ch, "%s: no such clan.\n", arg);
			return;
		}
		clan = CLAN(cln);
	}

	if (!clan
	&&  (!ch->clan || (clan = clan_lookup(ch->clan)) == NULL)) {
		char_puts("You are not in clan, you should not worry about your clan item.\n", ch);
		return;
	}

	if (clan->obj_ptr == NULL) {
		char_puts("Your clan do not have an item of power.\n",ch);
		return;
	}

	for (in_obj = clan->obj_ptr; in_obj->in_obj; in_obj = in_obj->in_obj)
		;

	if (in_obj->carried_by) {
		act_puts3("$p is in $R, carried by $N.",
			  ch, clan->obj_ptr, in_obj->carried_by,
			  in_obj->carried_by->in_room,
			  TO_CHAR, POS_DEAD);
	}
	else if (in_obj->in_room) {
		act_puts3("$p is in $R.",
			  ch, clan->obj_ptr, NULL, in_obj->in_room,
			  TO_CHAR, POS_DEAD);
		for (cln = 0; cln < clans.nused; cln++) 
			if (in_obj->in_room->vnum == CLAN(cln)->altar_vnum) {
				act_puts("It is the altar of $t",
					 ch, CLAN(cln)->name, NULL,
					 TO_CHAR | ACT_TRANS, POS_DEAD);
			}
	}
	else 
		act_puts("$p is somewhere.",
			 ch, clan->obj_ptr, NULL, TO_CHAR, POS_DEAD);
}


/*
 * clans have certain requirements that prolly need to be
 * enforced by the code, instead of the inductee inspection,
 * such as religion, or wearing certain equipment.
 *
 * by Zsuzsu
 */
bool fits_clan_requirements (CHAR_DATA *ch, int cln)
{
	clan_t *clan;
	bool fits = TRUE;
	int i = 0;
	OBJ_DATA *obj;

	if (!(clan=clan_lookup(cln))) {
		BUG("%s was being inducted into bad clan %d",
			ch->name, cln);
		return FALSE;
	}

	if (!str_cmp(clan->name, "elidodi")) {
		fits = (ch->religion == RELIGION_ATHEIST);
	}

	/* check all worn items */
	for (i = 0; fits && i < MAX_WEAR; i++) {
		if ((obj = get_eq_char(ch, i)) != NULL) {
			fits = can_clan_wear_obj(ch, cln, obj);
		}
	}

	DEBUG(DEBUG_CLAN,
		"clan_req: %s[%d] into clan %s[%d]: %s",
		ch->name,
		ch->level,
		clan->name,
		cln,
		fits ? "fits" : "no-fit");

	return fits;
}

/*
 * can a clan wear a certain item?
 * This check is done to help make sure clans are balanced
 * against non-clanned members.  Some clan must give up
 * equipment slots if they do not have an RP disposition
 * that makes them vulnerable.
 *
 * by Zsuzsu
 */
bool can_clan_wear(CHAR_DATA *ch, int cln, OBJ_DATA *obj, flag32_t wear_loc)
{
	clan_t* clan;
	bool can_wear = TRUE;

	if (!(clan=clan_lookup(cln))) {
		return TRUE;
	}

	if (wear_loc == 0) {
		if (obj == NULL) {
			bug("can_clan_wear: wear_loc and obj both NULL", 0);
			return TRUE;
		}
		wear_loc = obj->wear_flags;
	}

	if (obj != NULL 
	&& !str_cmp(clan->name, "barbarian")
	&& IS_SET(obj->extra_flags, ITEM_MAGIC)) {
		can_wear = FALSE;
	}

	if (!str_cmp(clan->name, "nightfall")) {
		if (obj != NULL && obj->pIndexData->item_type == ITEM_LIGHT)
			can_wear = FALSE;
	}

	return can_wear;
}

bool can_clan_wear_loc(CHAR_DATA *ch, int cln, flag32_t wear_loc)
{
	return can_clan_wear(ch, cln, NULL, wear_loc);
}

bool can_clan_wear_obj(CHAR_DATA *ch, int cln, OBJ_DATA *obj)
{
	return can_clan_wear(ch, cln, obj, 0);
}

/*
 * check to see if the clan member is an outcast, and if
 * so, boot them from the clan.
 */
bool clan_remove_outcast(CHAR_DATA *ch)
{
	clan_t* clan;
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *mark;

	clan = clan_lookup(ch->clan);
	if (!clan || IS_NPC(ch) || ch->clan == CLAN_FREEMAN) {
		return FALSE;
	}

	if (ch->pcdata->clan_status == CLAN_OUTCAST) {
		clan_update_lists(clan, ch);
		clan_save(clan);
		ch->clan = CLAN_NONE;
		REMOVE_BIT(ch->pcdata->trust, TRUST_CLAN);
		update_skills(ch);

		if ((mark = get_eq_char(ch, WEAR_CLANMARK))) {
			obj_from_char(mark);
			extract_obj(mark, 0);
		}

		char_printf(ch, "\nYou've been branded an {routcast{x of %s!",
			clan->name);

		snprintf(buf, sizeof(buf),
			"{W$N{x has been auto-removed from %s.",
			clan->name);

		wiznet(buf, ch, NULL, WIZ_POLITICS, 0, 0);

		return TRUE;
	}

	return FALSE;
}

/*
 * does the clan have its item, and therefore powers?
 */
bool clan_item_ok(int cln)
{
	clan_t* clan;
	OBJ_DATA* obj;
	int room_in;
	/*int i;*/

	if (!(clan=clan_lookup(cln)) || !(clan->obj_ptr)) {
		DEBUG(DEBUG_CLAN_ITEM,
			"clan_item_ok: true - no clan/clan item");
		return TRUE;
	}

	for (obj = clan->obj_ptr; obj->in_obj != NULL; obj = obj->in_obj);

	if (obj->in_room) 
		room_in = obj->in_room->vnum;
	else {
		DEBUG(DEBUG_CLAN_ITEM, 
			"clan_item_ok: false - obj not in room");
		return FALSE;
	}

	if (room_in == clan->altar_vnum) {
		DEBUG(DEBUG_CLAN_ITEM, 
			"clan_item_ok: true - obj in altar room");
		return TRUE;
	}

	DEBUG(DEBUG_CLAN_ITEM,
		"clan_item_ok: false - obj not in altar room %d", room_in);
	return FALSE;

	/*
	for (i=0; i < clans.nused; i++)
		if (room_in == clan_lookup(i)->altar_vnum) return FALSE;

	return TRUE;
	*/
}

/*
 * looting of a clan item
 * If the item is taken out of its altar, then it's a clan loot
 */
bool clan_item_loot(CHAR_DATA *ch, OBJ_DATA *item, OBJ_DATA *container)
{
	clan_t* clan;  /*looter's clan*/
	clan_t* vclan; /*victim clan*/
	clan_t* cclan; /*container clan*/
	char buf[MAX_STRING_LENGTH];

	clan = clan_lookup(ch->clan);
	vclan = clan_lookup(item->pIndexData->clan);
	cclan = clan_lookup(container->pIndexData->clan);

	if (!vclan && IS_SET(item->pIndexData->extra_flags, ITEM_CLAN)) {
		BUG("clan_item_loot: item %d is a clanitem,"
			" but not labeled for a clan",
			item->pIndexData->vnum);
		return FALSE;
	}

	if (vclan->obj_ptr != item) {
		BUG("clan_item_loot: item %d is a clanitem,"
			" but set to a clan that doesn't think"
			" it's the clan item",
			item->pIndexData->vnum);
		return FALSE;
	}

	if (!cclan) {
		BUG("clan_item_look: container %d isn't clan affiliated.",
			container->pIndexData->vnum);
		return FALSE;
	}

	if (vclan == cclan
	&& clan == vclan 
	&& !IS_IMMORTAL(ch)) {
		char_puts("Trying to raid your own clan?!",
			ch);
		char_puts("On second though, you put it back and hope nobody noticed.",
			ch);
		return FALSE;
	}

	item->timer = CLAN_ITEM_RAID_TIMER;

	act("$p pulsates in your hands with emense power.",
		ch, item, NULL, TO_CHAR);
	act("$p pulsates in $n's hands with emense power.",
		ch, item, NULL, TO_ROOM);

	/* loot */
	if (vclan == cclan) {

		if (ch->invis_level < LEVEL_HERO) {
			clan_act(vclan, "You feel a tremor in your soul.", 
				NULL, NULL, TO_CHAR, POS_DEAD);
		}

		wiznet("$N is raiding $t.", 
			ch, vclan->name, WIZ_POLITICS, 0, ch->level);

		return TRUE;
	}
	else {
		if (ch->invis_level < LEVEL_HERO) {
			clan_act(vclan, 
				"You feel a brief jolt of power in your soul.", 
				NULL, NULL, TO_CHAR, POS_DEAD);
		}

		snprintf(buf, sizeof(buf),
			"$N took the $p from the altar of %s.",
			cclan->name);

		wiznet(buf, ch, item, WIZ_POLITICS, 0, ch->level);

		return TRUE;
	}


	return FALSE;
}

/*
 * how many clan items in a clan's altar, not including
 * their own.
 */
int clan_raid_count (clan_t *clan)
{
	int count = 0;
	OBJ_DATA *obj = NULL;

	if (!clan) {
		BUG("clan_raid_count: null clan");
		return 0;
	}

	if (!clan->altar_ptr) {
		BUG("clan_raid_count: null altar");
		return 0;
	}

	for (obj = clan->altar_ptr->contains; obj != NULL; obj = obj->next_content) {
		if (IS_SET(obj->pIndexData->extra_flags, ITEM_CLAN)
		&& obj != clan->obj_ptr)
			count++;
	}

	return count;
}

/*
 * when the clan has their item put into another clan's altar
 */
bool clan_item_raid(CHAR_DATA *ch, OBJ_DATA *item, OBJ_DATA *container)
{
	clan_t* aclan;  /*clan of the altar*/
	clan_t* vclan;  /*victim clan*/
	char buf[MAX_STRING_LENGTH];

	vclan = clan_lookup(item->pIndexData->clan);
	aclan = clan_lookup(container->pIndexData->clan);

	if (!vclan && IS_SET(item->pIndexData->extra_flags, ITEM_CLAN)) {
		BUG("clan_item_raid: item %d is a clanitem,"
			" but not labeled for a clan",
			item->pIndexData->vnum);
		return FALSE;
	}

	if (vclan->obj_ptr != item) {
		BUG("clan_item_raid: item %d is a clanitem,"
			" but set to a clan that doesn't think"
			" it's the clan item",
			item->pIndexData->vnum);
		return FALSE;
	}

	if (aclan->altar_ptr != container) {
		BUG("clan_item_raid: item %d is not the clanaltar of %s",
			container->pIndexData->vnum,
			aclan->name);
		return FALSE;
	}

	item->timer = -1;

	if (aclan == vclan) {
		/* return */
		if (ch->invis_level < LEVEL_HERO)
			clan_act(vclan, "You feel a sudden surge of power.", 
				NULL, NULL, TO_CHAR, POS_DEAD);

		snprintf(buf, sizeof(buf),
			"$N has returned $p to the altar of %s.",
			aclan->name);
	} else {
		/*raid*/
		if (ch->invis_level < LEVEL_HERO)
			clan_act(vclan, "You feel a sudden loss.", 
				NULL, NULL, TO_CHAR, POS_DEAD);

		clan_free_slaves(ch);
		clan_minons_remove(ch, vclan);
		clan_items_remove(ch, vclan);

		snprintf(buf, sizeof(buf),
			"$N has put $p in the altar of %s.",
			aclan->name);
	}

	wiznet(buf, ch, item, WIZ_POLITICS, 0, ch->level);

	return TRUE;
}
	
/*
 * the timer on the clan item has run out and
 * the item returns to the altar
 */
bool clan_item_expire (OBJ_DATA *item)
{
	clan_t* clan;  /*clan of the item*/
	char buf[MAX_STRING_LENGTH];

	clan = clan_lookup(item->pIndexData->clan);

	if (!clan) {
		BUG("clan_item_expire: item %d is a clanitem,"
			" but not labeled for a clan",
			item->pIndexData->vnum);
		return FALSE;
	}

	if (clan->obj_ptr != item) {
		BUG("clan_item_expire: item %d is a clanitem,"
			" but set to a clan that doesn't think"
			" it's the clan item",
			item->pIndexData->vnum);
		return FALSE;
	}

	if (clan->altar_ptr == NULL) {
		BUG("clan_item_expire: %s doesn't have an altar",
			clan->name);
		return FALSE;
	}

	/* raid */
	item->timer = -1;
	if (item->in_obj)
		obj_from_obj(item);
	else if (item->carried_by)
		obj_from_char(item);
	else if (item->in_room)
		obj_from_room(item);
	else
		BUG("clan item %d in unknown state",
			item->pIndexData->vnum);

	obj_to_obj(item, clan->altar_ptr);

	clan_act(clan, "You feel a sudden surge of power.", 
		NULL, NULL, TO_CHAR, POS_DEAD);

	snprintf(buf, sizeof(buf),
		"$p has returned to %s's altar.",
		clan->name);

	wiznet(buf, NULL, item, WIZ_POLITICS, 0, 0);

	return TRUE;
}

/*
 * act() for clans.
 * sends the message to all clan members.
 */
int clan_act (clan_t *clan, const char *msg, void *arg2, void *arg3, 
		int flags, int min_pos)
{
	CHAR_DATA *gch;
	int cln = cln_lookup(clan->name);
	int count = 0;

	for (gch = char_list; gch; gch = gch->next) {
		if (gch->clan == cln) {
			act_puts(msg, gch,
				arg2, arg3, flags, min_pos);
			count++;
		}
	}
	return count;
}

/*
 * free Nightfall's slaves if they get raided
 */
int clan_free_slaves(CHAR_DATA *ch)
{
	CHAR_DATA *gch;
	int count = 0;

	for (gch = char_list; gch && !IS_NPC(gch); gch = gch->next) {
		if (gch->pcdata->enslaver != NULL) {
			act_puts("$N has unleashed your soul from bondage!",
				gch, NULL, ch, TO_CHAR, POS_DEAD);
			gch->pcdata->enslaver = NULL;
			save_char_obj(gch, FALSE);
			count++;
		}
	}
	return count;
}

/*
 * when a clan is raided all mobs set to the clan are removed.
 */
int clan_minons_remove(CHAR_DATA *ch, clan_t *clan)
{
	int cln = cln_lookup(clan->name);
	CHAR_DATA *gch, *next;
	int count = 0;

	if (cln == 0) return -1;

	if (!str_cmp(clan->name, "Ruler")) {
		for (gch = char_list; gch && !IS_NPC(gch); gch = gch->next) {
			if (gch->pcdata->saved_stalkers > 0) {
				act_puts("$N has thrown the Law off your trail.",
					gch, NULL, ch, TO_CHAR, POS_DEAD);
				gch->pcdata->saved_stalkers = 0;
				save_char_obj(gch, FALSE);
				count++;
			}
		}
	}
	for (gch = npc_list; gch; gch = next) {
		next = gch->next;
		if (gch->clan == cln) {
			if (gch->in_room)
				act("$n withers away as the source of"
					" $s existance wanes.", 
					gch, NULL, NULL,
					TO_ROOM);
			extract_char(gch, 0);
			count++;
		}
	}
	return count;
}

/*
 * remove all items of a certain clan from the realm.
 *
 * returns the number of items removed.
 */
int clan_items_remove(CHAR_DATA *ch, clan_t *clan)
{
	int cln = 0;
	CHAR_DATA *gch = NULL;
	int count = 0;
	OBJ_DATA *obj, *obj_next;

	if (clan == NULL) return -1;

	cln = cln_lookup(clan->name);

	if (cln == 0) return -1;

	for (obj = object_list; obj != NULL; obj = obj_next) {
		obj_next = obj->next;

		if (obj->pIndexData->clan != cln
		|| IS_SET(obj->pIndexData->extra_flags, ITEM_NOPURGE)
		|| CAN_WEAR(obj, ITEM_WEAR_CLANMARK)
		|| IS_SET(obj->pIndexData->extra_flags, ITEM_CLAN))
			continue;

		DEBUG(DEBUG_CLAN_RAID,
			"[%s] item is going poof due to raid %s.",
			clan->name,
			mlstr_mval(obj->short_descr));

		if (obj->in_room != NULL)
			gch = obj->in_room->people;

		if (obj->carried_by != NULL
		&& obj->carried_by->in_room)
			gch = obj->carried_by->in_room->people;

		if (gch)
			act("$p flickers out of existance as the"
				" source of its power wanes.", 
				gch, obj, NULL, TO_ALL);

		extract_obj(obj, 0);
		count++;
	 }
	 return count;
}

int clan_activity (CHAR_DATA *ch)
{
	if (IS_NPC(ch))
		return CLAN_ACTIVITY_DELETED;
	return clan_activity_for_name(ch->name);
}

int clan_activity_for_name (const char *name)
{
	CHAR_PDATA *pch = ch_pdata_lookup(name);
	long seconds = -1;
	int active = CLAN_ACTIVITY_DELETED;

	if (pch == NULL)
		seconds = -1L;
	else {
		seconds = usage_last_days(pch, 10); 
	}

	if (seconds == -1L)
		active = CLAN_ACTIVITY_DELETED;
	else if (seconds == 0L)
		active = CLAN_ACTIVITY_INACTIVE;
	else if (seconds < (60 * 60))
		active = CLAN_ACTIVITY_MIA;
	else if (seconds < (3 * 60 * 60))
		active = CLAN_ACTIVITY_AWOL;
	else if (seconds < (5 * 60 * 60))
		active = CLAN_ACTIVITY_POOR;
	else if (seconds < (10 * 60 * 60))
		active = CLAN_ACTIVITY_SEMI;
	else if (seconds < (20 * 60 * 60))
		active = CLAN_ACTIVITY_ACTIVE;
	else 
		active = CLAN_ACTIVITY_HYPER;

	return active;
}

/*
 * look in the clan member lists to see if this character
 * is in one of the clans.
 * return which clan.
 */
clan_t *clan_for_char (const char *name)
{
	int cln = 0;
	const char *nl = NULL;
	int i = 0;
	bool found = FALSE;

	if (IS_NULLSTR(name))
		return NULL;

	for (cln = 0; cln < clans.nused && !found; cln++) {
		for (i = 0; i < MAX_CLAN_RANK && !found; i++) {
			nl = clan_lookup(cln)->member_list[i];
			if (nl)
				found = name_find(nl, name);
			if (found)
				break;
		}
		if (found) break;
	}
	if (found)
		return CLAN(cln);
	else
		return NULL;
}

/*
 * what rank does this name/character hold in said clan
 */
int clan_rank_for_name (clan_t *clan, const char *name)
{
	int cln = 0;
	const char *nl = NULL;
	int i = 0;

	if (IS_NULLSTR(name))
		return -1;

	for (cln = 0; cln < clans.nused; cln++) {
		for (i = 0; i < MAX_CLAN_RANK; i++) {
			nl = clan_lookup(cln)->member_list[i];
			if (nl && name_find(nl, name))
				return i;
		}
	}

	return -1;
}

/*
 * look up each member of a clan and return the number of 
 * members that rank equal to or above the activity_level,
 * or less than or equal to if 'greaterthan' is FALSE.
 *
 * elders - TRUE count elders
 *
 * note: this is pretty inefficent.  With a large playerbase
 * with large clans, this should be optimized to do fewer 
 * string comparisons, or called less frequently.
 */
int clan_member_count (int cln, int activity_level, 
		bool greaterthan, bool elders)
{
	char name[MAX_STRING_LENGTH];
	int count = 0;
	int activity = CLAN_ACTIVITY_DELETED;
	const char *list = NULL;
	int i = 0;
	clan_t *clan = NULL;

	clan = clan_lookup(cln);

	for (i=CLAN_RECRUIT; i <= CLAN_LEADER; i++) {
		if (i == CLAN_ELDER && !elders)
			continue;
		list = clan->member_list[i];
		list = first_arg(list, name, sizeof(name), FALSE);
		for (; name[0]; list = first_arg(list, name, sizeof(name), FALSE)) {
			activity = clan_activity_for_name(name);
			if ((greaterthan && activity >= activity_level)
			|| (!greaterthan && activity <= activity_level))
				count++;
		}
	}

	return count;
}

/*
 * clan members get a bonus to their limited allowance if their
 * clan is active.  Leaders especially benefit from this, as they
 * receive a large bonus for their activity and for the activity 
 * of their clan.
 */
int clan_activity_limited_bonus (CHAR_PDATA *pch)
{
	clan_t *clan = clan_for_char(pch->name);
	int rank = -1;
	int activity = CLAN_ACTIVITY_DELETED;
	int bonus = 0;
	int clan_active = 0;
	int clan_inactive = 0;

	if (!clan)
		return 0;

	rank = clan_rank_for_name(clan, pch->name);
	activity = clan_activity_for_name(pch->name);
	clan_active = clan_member_count(cln_lookup(clan->name), 
			CLAN_ACTIVITY_ACTIVE, TRUE, TRUE);
	clan_inactive = clan_member_count(cln_lookup(clan->name), 
			CLAN_ACTIVITY_POOR, FALSE, FALSE);

	if (activity < CLAN_ACTIVITY_ACTIVE)
		return 0;

	if (rank == CLAN_LEADER) {
		bonus += 1;

		if (activity == CLAN_ACTIVITY_HYPER)
			bonus += 2;

		bonus += clan_active / 2; /*pch helps round up*/
	}
	else if (rank >= CLAN_RECRUIT) {
		if (clan_inactive == 0)
			bonus++;
		else
			bonus += (clan_active > clan_inactive) 
				? UMAX(1, (clan_active - clan_inactive) /2) : 0;
	}
	else {
		bonus = 0;
	}

	return bonus;
}

/*
 * artifact bonus due to raiding other clans.
 */
int clan_raid_limited_bonus (CHAR_PDATA *pch)
{
	clan_t *clan = clan_for_char(pch->name);
	int rank = clan_rank_for_name(clan, pch->name);
	int activity = clan_activity_for_name(pch->name);
	int bonus = 0;

	if (!clan
	|| activity < CLAN_ACTIVITY_ACTIVE
	|| rank < CLAN_RECRUIT)
		return 0;

	bonus += clan_raid_count(clan);

	return bonus;
}
