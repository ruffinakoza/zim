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
 * $Id: quest.c 978 2006-12-08 07:38:16Z zsuzsu $
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "debug.h"
#include "quest.h"
#include "update.h"
#include "waffects.h"

#ifdef SUNOS
#	include <stdarg.h>
#	include "compat/compat.h"
#endif

enum qitem_type {
	TYPE_ITEM,
	TYPE_OTHER
};

typedef struct qitem_data QITEM_DATA;
struct qitem_data {
	char		*name;
	int		price;
	const char	*restrict_class;
	int		vnum;
	bool		(*do_buy)(CHAR_DATA *ch, CHAR_DATA *questor);
	int		*gsn;
};

static void 	quest_tell(CHAR_DATA *ch, CHAR_DATA *questor, const char *fmt, ...);
static CHAR_DATA *questor_lookup(CHAR_DATA *ch);
qtrouble_t 	*qtrouble_lookup(CHAR_DATA *ch, int vnum);
bool 		qtrouble_delete(CHAR_DATA *ch, int vnum);
int		quest_item_price (CHAR_DATA *ch, QITEM_DATA *qitem);

static void quest_points(CHAR_DATA *ch, char *arg);
static void quest_info(CHAR_DATA *ch, char *arg);
static void quest_time(CHAR_DATA *ch, char *arg);
static void quest_list(CHAR_DATA *ch, char *arg);
static void quest_buy(CHAR_DATA *ch, char *arg);
static void quest_return(CHAR_DATA *ch, char *arg);
static void quest_request(CHAR_DATA *ch, char *arg);
static void quest_complete(CHAR_DATA *ch, char *arg);
static void quest_trouble(CHAR_DATA *ch, char *arg);
static void quest_quit(CHAR_DATA *ch, char *arg);

static OBJ_DATA *quest_give_item(CHAR_DATA *ch, CHAR_DATA *questor,
			    int item_vnum);

/*static bool buy_gold(CHAR_DATA *ch, CHAR_DATA *questor);*/
static bool buy_tattoo(CHAR_DATA *ch, CHAR_DATA *questor);
static bool buy_item(CHAR_DATA *ch, CHAR_DATA *questor, int vnum);
static bool buy_con(CHAR_DATA *ch, CHAR_DATA *questor);
static bool buy_death(CHAR_DATA *ch, CHAR_DATA *questor);
static bool buy_katana(CHAR_DATA *ch, CHAR_DATA *questor);
static bool buy_vampire(CHAR_DATA *ch, CHAR_DATA *questor);

bool quest_item_usable (CHAR_DATA *ch, QITEM_DATA *qitem);
bool quest_has_similar (CHAR_DATA *ch, int vnum);
int  quest_item_extract(CHAR_DATA *ch, int vnum);
int  quest_extract_similar (CHAR_DATA *ch, int vnum);
int  quest_item_in_world (CHAR_DATA *ch, int vnum);
int  quest_item_group (int vnum);
QITEM_DATA * quest_item_by_vnum (int vnum);

struct qitem_data qitem_table[] = {
	{ "a flying rug",		25, 	NULL,
	   QUEST_VNUM_RUG,		NULL,	NULL		},

/*	{ "10,000 gold pieces",		250, 	NULL,
	   0,			buy_gold,	NULL		},
*/
	{ "tattoo of your religion",	200, 	NULL,
	   0, 			buy_tattoo,	NULL		},

/*
	{ "Heroic Constitution",	2000, 	NULL,
	   0, buy_con						},
*/

	{ "Decrease number of deaths",	10,	"samurai",
	   0,			buy_death,	NULL		},

	{ "Katana quest",		150,	"samurai",
	   0,			buy_katana,	NULL		},

	{ "Vampire skill",		20,	"vampire",
	   0,			buy_vampire,	NULL		},

/*	{ "Bottomless canteen with cranberry juice", 350, NULL,
	   QUEST_VNUM_CANTEEN, NULL				},
*/
	{ "Quest song",			1000,	NULL,
	   QUEST_VNUM_SONG, 		NULL,	NULL		},

	{ "Quest amulet",		2000,	NULL,
	   QUEST_VNUM_AMULET, 		NULL,	NULL		},

	{ "Quest bracer",		2500,	NULL,
	   QUEST_VNUM_BRACER, 		NULL,	NULL		},

	{ "Quest ring",			3000,	NULL,
	   QUEST_VNUM_RING, 		NULL,	NULL		},

	{ "Quest halberd",		3500,	NULL,
	   QUEST_VNUM_POLEARM,		NULL,	&gsn_polearm	},

	{ "Quest claymore",		3500,	NULL,
	   QUEST_VNUM_BASTARDSWORD,	NULL,	&gsn_bastardsword	},

	{ "Quest battleaxe",		3500,	NULL,
	   QUEST_VNUM_BATTLEAXE,	NULL,	&gsn_axe	},

	{ "Quest warhammer",		3500,	NULL,
	   QUEST_VNUM_WARHAMMER,	NULL,	&gsn_hammer	},

	{ "Quest lance",		3500,	NULL,
	   QUEST_VNUM_LANCE,		NULL,	&gsn_lance	},

	{ "Quest spear",		3500,	NULL,
	   QUEST_VNUM_SPEAR,		NULL,	&gsn_spear	},

	{ "Quest longsword",		3500,	NULL,
	   QUEST_VNUM_LONGSWORD,	NULL,	&gsn_longsword	},

	{ "Quest axe",			3500,	NULL,
	   QUEST_VNUM_AXE,		NULL,	&gsn_axe	},

	{ "Quest hammer",		3500,	NULL,
	   QUEST_VNUM_HAMMER,		NULL,	&gsn_hammer	},

	{ "Quest shortsword",		3500,	NULL,
	   QUEST_VNUM_SHORTSWORD,	NULL,	&gsn_shortsword	},

	{ "Quest stiletto",		3500,	NULL,
	   QUEST_VNUM_STILETTO,		NULL,	&gsn_dagger	},

	{ "Quest dagger",		3500,	NULL,
	   QUEST_VNUM_DAGGER,		NULL,	&gsn_dagger	},

	{ "Quest mace",			3500,	NULL,
	   QUEST_VNUM_MACE,		NULL,	&gsn_mace	},

	{ "Quest flail",		3500,	NULL,
	   QUEST_VNUM_FLAIL,		NULL,	&gsn_flail	},

	{ "Quest whip",			3500,	NULL,
	   QUEST_VNUM_WHIP,		NULL,	&gsn_whip	},

	{ "Quest bow",			3500,	NULL,
	   QUEST_VNUM_BOW,		NULL,	&gsn_bow	},

	{ NULL }
};

#define QUEST_GROUP_WEAPON	0
int quest_item_groups[][18] = { 
				{
				QUEST_VNUM_LANCE,
				QUEST_VNUM_BOW,
				QUEST_VNUM_POLEARM,
				QUEST_VNUM_BASTARDSWORD,
				QUEST_VNUM_BATTLEAXE,
				QUEST_VNUM_WARHAMMER,
				QUEST_VNUM_LONGSWORD,
				QUEST_VNUM_AXE,
				QUEST_VNUM_HAMMER,
				QUEST_VNUM_SHORTSWORD,
				QUEST_VNUM_STILETTO,
				QUEST_VNUM_DAGGER,
				QUEST_VNUM_FLAIL,
				QUEST_VNUM_MACE,
				QUEST_VNUM_SPEAR,
				QUEST_VNUM_STAFF,
				QUEST_VNUM_WHIP,
				0},

				{ 0 }
		};

struct qcmd_data {
	char *name;
	void (*do_fn)(CHAR_DATA *ch, char* arg);
	int min_position;
	int extra;
};
typedef struct qcmd_data Qcmd_t;

Qcmd_t qcmd_table[] = {
	{ "points",	quest_points,	POS_DEAD,	CMD_KEEP_HIDE},
	{ "info",	quest_info,	POS_DEAD,	CMD_KEEP_HIDE},
	{ "time",	quest_time,	POS_DEAD,	CMD_KEEP_HIDE},
	{ "list",	quest_list,	POS_RESTING,	0},
	{ "buy",	quest_buy,	POS_RESTING,	0},
	{ "request",	quest_request,	POS_RESTING,	0},
	{ "complete",	quest_complete,	POS_RESTING,	0},
	{ "return",	quest_return,	POS_RESTING,	0},
	{ "trouble",	quest_trouble,	POS_RESTING,	0},
	{ "quit",	quest_quit,	POS_DEAD,	CMD_KEEP_HIDE},
	{ NULL}
};

/*
 * The main quest function
 */
void do_quest(CHAR_DATA *ch, const char *argument)
{
	char cmd[MAX_INPUT_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	Qcmd_t *qcmd;

	argument = one_argument(argument, cmd, sizeof(cmd));
	argument = one_argument(argument, arg, sizeof(arg));

	if (IS_NPC(ch)) 
		return;

	for (qcmd = qcmd_table; qcmd->name != NULL; qcmd++)
		if (str_prefix(cmd, qcmd->name) == 0) {
			if (ch->position < qcmd->min_position) {
				char_puts("In your dreams, or what?\n", ch);
				return;
			}
			if (!IS_SET(qcmd->extra, CMD_KEEP_HIDE)
			&&  IS_SET(ch->affected_by, AFF_HIDE | AFF_FADE)) { 
				REMOVE_BIT(ch->affected_by,
					   AFF_HIDE | AFF_FADE);
				act_puts("You step out of shadows.",
					 ch, NULL, NULL, TO_CHAR, POS_DEAD);
				act("$n steps out of shadows.",
				    ch, NULL, NULL, TO_ROOM);
			}
			qcmd->do_fn(ch, arg);
			return;
		}
		
	char_puts("QUEST COMMANDS: points info time request complete list buy trouble quit.\n", ch);
	char_puts("For more information, type: help quests.\n", ch);
}

void quest_handle_death(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (IS_NPC(ch)
	&&  IS_SET(ch->pIndexData->act, ACT_SUMMONED)
	&&  ch->master != NULL)
		ch = ch->master;

	if (victim->hunter) {
		if (victim->hunter == ch || victim->hunter->master == ch) {
			act_puts("You have almost completed your QUEST!\n"
				 "Return to questmaster before your time "
				 "runs out!",
				 ch, NULL, NULL, TO_CHAR, POS_DEAD);
			ch->pcdata->questmob = -1;
		}
		else {
			act_puts("You have completed someone's quest.",
				 ch, NULL, NULL, TO_CHAR, POS_DEAD);

			ch = victim->hunter;
			act_puts("Someone has completed you quest.",
				 ch, NULL, NULL, TO_CHAR, POS_DEAD);
			quest_cancel(ch);
			ch->pcdata->questtime = -number_range(5, 10);
		}
	}
}

void quest_cancel(CHAR_DATA *ch)
{
	CHAR_DATA *fch;

	if (IS_NPC(ch)) {
		bug("quest_cancel: called for NPC", 0);
		return;
	}

	/*
	 * remove mob->hunter
	 */
	for (fch = npc_list; fch; fch = fch->next)
		if (fch->hunter == ch) {
			fch->hunter = NULL;
			break;
		}

	ch->pcdata->questtime = 0;
	ch->pcdata->questgiver = 0;
	ch->pcdata->questmob = 0;
	ch->pcdata->questobj = 0;
	ch->pcdata->questroom = NULL;
}

/*
 * Called from update_handler() by pulse_area
 */
void quest_update(void)
{
	CHAR_DATA *ch, *ch_next;

	for (ch = char_list; ch && !IS_NPC(ch); ch = ch_next) {
		ch_next = ch->next;

		if (ch->pcdata->questtime < 0) {
			if (++ch->pcdata->questtime == 0) {
				char_puts("{*You may now quest again.\n", ch);
				return;
			}
		} else if (IS_ON_QUEST(ch)) {
			if (--ch->pcdata->questtime == 0) {
				char_puts("You have run out of time for your quest!\n", ch);
				quest_cancel(ch);
				ch->pcdata->questtime = -number_range(4, 8);
			} else if (ch->pcdata->questtime < 6) {
				char_puts("Better hurry, you're almost out of time for your quest!\n", ch);
				return;
			}
		}
	}
}

void qtrouble_set(CHAR_DATA *ch, int vnum, int count)
{
	qtrouble_t *qt;

	if ((qt = qtrouble_lookup(ch, vnum)) != NULL)
		qt->count = count;
	else {
		qt = malloc(sizeof(*qt));
		qt->vnum = vnum;
		qt->count = count;
		qt->next = ch->pcdata->qtrouble;
		ch->pcdata->qtrouble = qt;
	}
}

/*
 * local functions
 */

static void quest_tell(CHAR_DATA *ch, CHAR_DATA *questor, const char *fmt, ...)
{
	va_list ap;
	char buf[MAX_STRING_LENGTH];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), GETMSG(fmt, ch->lang), ap);
	va_end(ap);

	do_tell_raw(questor, ch, buf);
}

static CHAR_DATA* questor_lookup(CHAR_DATA *ch)
{
	CHAR_DATA *vch;
	CHAR_DATA *questor = NULL;

	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
		if (!IS_NPC(vch)) 
			continue;
		if (IS_SET(vch->pIndexData->act, ACT_QUESTOR)) {
			questor = vch;
			break;
		}
	}

	if (questor == NULL) {
		char_puts("You can't do that here.\n", ch);
		return NULL;
	}

	if (questor->fighting != NULL) {
		char_puts("Wait until the fighting stops.\n", ch);
		return NULL;
	}

	return questor;
}

/* 
 * returns whether one was deleted or not
 */
bool qtrouble_delete(CHAR_DATA *ch, int vnum)
{
	qtrouble_t *qt, *doomed;
	bool found = FALSE;

	qt = ch->pcdata->qtrouble;
	while (qt != NULL) {

		if (qt == ch->pcdata->qtrouble
		&& qt->vnum == vnum) {
			ch->pcdata->qtrouble = qt->next;
			free(qt);
			qt = ch->pcdata->qtrouble;
			found = TRUE;
		}
		else if (qt->next && qt->next->vnum == vnum) {
			doomed = qt->next;
			qt->next = qt->next->next;
			free(doomed);
			found = TRUE;
		}

		if (qt)
			qt = qt->next;

	}

	return found;
}

qtrouble_t *qtrouble_lookup(CHAR_DATA *ch, int vnum)
{
	qtrouble_t *qt;

	for (qt = ch->pcdata->qtrouble; qt != NULL; qt = qt->next)
		if (qt->vnum == vnum)
			return qt;

	return NULL;
}

/*
 * quest do functions
 */

static void quest_points(CHAR_DATA *ch, char* arg)
{
	char_printf(ch, "You have {W%d{x quest points.\n",
		    ch->pcdata->questpoints);
}

static void quest_info(CHAR_DATA *ch, char* arg)
{
	if (!IS_ON_QUEST(ch)) {
		char_puts("You aren't currently on a quest.\n", ch);
		return;
	}

	if (ch->pcdata->questmob == -1) {
		char_puts("Your quest is ALMOST complete!\nGet back to questor before your time runs out!\n", ch);
		return;
	}

	if (ch->pcdata->questobj > 0) {
		OBJ_INDEX_DATA *qinfoobj;

		qinfoobj = get_obj_index(ch->pcdata->questobj);
		if (qinfoobj != NULL) {
			char_printf(ch, "You are on a quest to recover the fabled {W%s{x!\n",
				    qinfoobj->name);
			if (ch->pcdata->questroom)
				char_printf(ch, "That location is in general area of {W%s{x for {W%s{x.\n",
					    ch->pcdata->questroom->area->name, 
					    mlstr_mval(ch->pcdata->questroom->name));
		}
		else 
			char_puts("You aren't currently on a quest.\n", ch);
		return;
	}

	if (ch->pcdata->questmob > 0) {
		MOB_INDEX_DATA *questinfo;

		questinfo = get_mob_index(ch->pcdata->questmob);
		if (questinfo != NULL) {
			char_printf(ch, "You are on a quest to slay the dreaded {W%s{x!\n",
				    mlstr_mval(questinfo->short_descr));
			if (ch->pcdata->questroom)
				char_printf(ch, "That location is in general area of {W%s{x for {W%s{x.\n",
					    ch->pcdata->questroom->area->name, 
					    mlstr_mval(ch->pcdata->questroom->name));
		} else 
			char_puts("You aren't currently on a quest.\n", ch);
		return;
	}
}

static void quest_time(CHAR_DATA *ch, char* arg)
{
	if (!IS_ON_QUEST(ch)) {
		char_puts("You aren't currently on a quest.\n", ch);
		if (ch->pcdata->questtime < -1)
			char_printf(ch, "There are {W%d{x minutes remaining until you can go on another quest.\n",
				    -ch->pcdata->questtime);
	    	else if (ch->pcdata->questtime == -1)
			char_puts("There is less than a minute remaining until you can go on another quest.\n", ch);
	}
	else
		char_printf(ch, "Time left for current quest: {W%d{x.\n",
			    ch->pcdata->questtime);
}

static void quest_list(CHAR_DATA *ch, char *arg)
{
	CHAR_DATA *questor;
	QITEM_DATA *qitem;
	class_t *cl;

	if ((questor = questor_lookup(ch)) == NULL
	||  (cl = class_lookup(ch->class)) == NULL)
		return;

	act("$n asks $N for list of quest items.", ch, NULL, questor, TO_ROOM);
	act_puts("You ask $N for list of quest items.",
		 ch, NULL, questor, TO_CHAR, POS_DEAD);

	char_puts("Current Quest Items available for Purchase:\n", ch);
	for (qitem = qitem_table; qitem->name; qitem++) {

		if (!quest_item_usable(ch, qitem))
			continue;

		if (arg[0] != '\0' && !is_name(arg, qitem->name))
			continue;

		char_printf(ch, "{M%5d{mqp{x...........{B%-17s{x",
			quest_item_price(ch, qitem), qitem->name); 

		if (qitem->vnum != 0) {
			if (get_obj_index(qitem->vnum) == NULL) {
				bug("missing quest item: vnum ", qitem->vnum);
				continue;
			}
			char_printf(ch, " {D(level %2d){x",
				    get_obj_index(qitem->vnum)->level);
		}
		char_printf(ch, "\n");

	}
	char_puts("To buy an item, type 'QUEST BUY <item>'.\n", ch);
}

static void quest_buy(CHAR_DATA *ch, char *arg)
{
	CHAR_DATA *questor;
	QITEM_DATA *qitem;
	int price = 0;
	class_t *cl;

	if ((questor = questor_lookup(ch)) == NULL
	||  (cl = class_lookup(ch->class)) == NULL)
		return;

	if (arg[0] == '\0') {
		char_puts("To buy an item, type 'QUEST BUY <item>'.\n", ch);
		return;
	}

	for (qitem = qitem_table; qitem->name; qitem++)
		if (is_name(arg, qitem->name)) {
			bool buy_ok = FALSE;

			if (!quest_item_usable(ch, qitem))
				continue;

			price = quest_item_price(ch, qitem);

			if (ch->pcdata->questpoints < price) {
				quest_tell(ch, questor,
					   "Sorry, {W%s{z, but you don't have "
					   "enough quest points for that.",
					   ch->name);
				return;
			}

			if (qitem->vnum == 0)
				buy_ok = qitem->do_buy(ch, questor);
			else
				buy_ok = buy_item(ch, questor,
						qitem->vnum);

			if (buy_ok) {
				ch->pcdata->questpoints -= price;

				DEBUG(DEBUG_QUEST,
					"%s[%d][%d] quest buy [%d] %s",
					ch->name, ch->level, 
					ch->pcdata->questpoints, 
					qitem->vnum, qitem->name);
				wiznet2(WIZ_QUESTS, IM, 0, ch,
					"$N {mquest buy{x %s.",
					qitem->name);
			}
			return;
		}

	quest_tell(ch, questor, "I do not have that item, %s.", ch->name);
}

static void quest_quit(CHAR_DATA *ch, char* arg)
{
	char buf[MAX_STRING_LENGTH];

        if (!IS_ON_QUEST(ch)) {
                char_puts("You aren't currently on a quest.\n", ch);
		return;
	}
	else
	char_puts("Your quest has been cancelled.\n",ch);
	quest_cancel(ch);
	ch->pcdata->questtime = -number_range(5, 8);

	snprintf(buf, sizeof(buf),
		"$N[%d] {mquest quit{x.",
		ch->level);
	wiznet(buf,
		ch, NULL, WIZ_QUESTS, 0, 0);

}



#define MAX_QMOB_COUNT 512

static void quest_request(CHAR_DATA *ch, char *arg)
{
	int i;
	CHAR_DATA *mobs[MAX_QMOB_COUNT];
	size_t mob_count;
	CHAR_DATA *victim = NULL;
	CHAR_DATA *questor;
	char buf[MAX_STRING_LENGTH];

	if ((questor = questor_lookup(ch)) == NULL)
		return;

	act("$n asks $N for a quest.", ch, NULL, questor, TO_ROOM);
	act_puts("You ask $N for a quest.",
		 ch, NULL, questor, TO_CHAR, POS_DEAD);

	if (IS_ON_QUEST(ch)) {
    		quest_tell(ch, questor, "But you are already on a quest!");
    		return;
	} 

	if (ch->pcdata->questtime < 0) {
		quest_tell(ch, questor,
			   "You're very brave, {W%s{z, but let someone else "
			   "have a chance.", ch->name);
		quest_tell(ch, questor, "Come back later.");
		return;
	}

	if (is_affected(ch, gsn_dishonor)) {
		quest_tell(ch, questor,
			   "Perhaps, {W%s{z, you should be more concerned "
			   "regaining your fallen honor.", ch->name);
		return;
	}

	quest_tell(ch, questor, "Thank you, brave {W%s{z!", ch->name);

	/*
	 * find MAX_QMOB_COUNT quest mobs and store their vnums in mob_buf
	 */
	mob_count = 0;
	for (victim = npc_list; victim; victim = victim->next) {
		int diff = victim->level - ch->level;

		if (!IS_NPC(victim)
		||  (ch->level < 51 && (diff > 4 || diff < -1))
		||  (ch->level > 50 && (diff > (ch->level/12 -1) || diff < 0))
		||  victim->pIndexData->pShop
		||  IS_SET(victim->pIndexData->attr_flags, MOB_ATTR_NOQUEST)
		||  victim->race == ch->race
		||  victim->invis_level
		||  (IS_EVIL(victim) && IS_EVIL(ch))
		||  (IS_NEUTRAL(victim) && IS_NEUTRAL(ch))
		||  (IS_GOOD(victim) && IS_GOOD(ch))
		||  victim->pIndexData->vnum < 200
		||  IS_SET(victim->pIndexData->act,
			   ACT_TRAIN | ACT_PRACTICE | ACT_HEALER |
			   ACT_NOTRACK | ACT_PET)
/*		||  IS_SET(victim->pIndexData->imm_flags, IMM_SUMMON) */
		||  questor->pIndexData == victim->pIndexData
		||  victim->in_room == NULL
		||  (IS_SET(victim->pIndexData->act, ACT_SENTINEL) &&
		     IS_SET(victim->in_room->room_flags,
			    ROOM_PRIVATE | ROOM_SOLITARY))
		||  !str_cmp(victim->in_room->area->name,
			     hometown_name(ch->hometown))
		||  IS_SET(victim->in_room->area->flags,
			   AREA_CLOSED | AREA_NOQUEST))
			continue;

		mobs[mob_count++] = victim;
		if (mob_count >= MAX_QMOB_COUNT)
			break;
	}

	if (mob_count == 0) {
		quest_tell(ch, questor, "I'm sorry, but i don't have any quests for you at this time.");
		quest_tell(ch, questor, "Try again later.");
		ch->pcdata->questtime = -5;
		return;
	}

	victim = mobs[number_range(0, mob_count-1)];
	ch->pcdata->questroom = victim->in_room;

	if (chance(40 - (ch->level /4))) { /* Quest to find an obj */
		OBJ_DATA *eyed;
		int obj_vnum;

		if (IS_GOOD(ch))
			i = 0;
		else if (IS_EVIL(ch))
			i = 2;
		else
			i = 1;

		obj_vnum = number_range(QUEST_OBJ_FIRST, QUEST_OBJ_LAST);
		eyed = create_obj(get_obj_index(obj_vnum), 0);
		eyed->level = ch->level;
		eyed->owner = mlstr_dup(ch->short_descr);
		eyed->ed = ed_new2(eyed->pIndexData->ed, ch->name);
		eyed->cost = 0;
		eyed->timer = 30;

		obj_to_room(eyed, victim->in_room);
		ch->pcdata->questobj = eyed->pIndexData->vnum;

		quest_tell(ch, questor,
			   "Vile pilferers have stolen {W%s{z "
			   "from the royal treasury!",
			   mlstr_mval(eyed->short_descr));
		quest_tell(ch, questor,
			   "My court wizardess, with her magic mirror, "
			   "has pinpointed its location.");
		quest_tell(ch, questor,
			   "Look in the general area of {W%s{z for {W%s{z!",
			   victim->in_room->area->name,
			   mlstr_mval(victim->in_room->name));

		snprintf(buf, sizeof(buf),
			"$N[%d] {mon quest{x to {W%s{x in {W%s{x to find %s.",
			ch->level,
			victim->in_room->area->name,
			mlstr_mval(victim->in_room->name),
			mlstr_mval(eyed->short_descr));
		wiznet(buf,
			ch, NULL, WIZ_QUESTS, 0, 0);
	}
	else {	/* Quest to kill a mob */
		if (IS_GOOD(ch)) {
			quest_tell(ch, questor,
				   "Rune's most heinous criminal, {W%s{z, "
				   "has escaped from the dungeon.",
				   mlstr_mval(victim->short_descr));
			quest_tell(ch, questor,
				   "Since the escape, {W%s{z has murdered {W%d{z civilians!",
				   mlstr_mval(victim->short_descr),
				   number_range(2, 20));
			quest_tell(ch, questor, "The penalty for this crime is death, and you are to deliver the sentence!");
		}
		else {
			quest_tell(ch, questor,
				   "An enemy of mine, {W%s{z, "
				   "is making vile threats against the crown.",
				   mlstr_mval(victim->short_descr));
			quest_tell(ch, questor,
				   "This threat must be eliminated!");
		}

		quest_tell(ch, questor,
			   "Seek {W%s{z out in the vicinity of {W%s{z!",
			   mlstr_mval(victim->short_descr),
			   mlstr_mval(victim->in_room->name));
		quest_tell(ch, questor,
			   "That location is in general area of {W%s{z.",
			   victim->in_room->area->name);

		ch->pcdata->questmob = victim->pIndexData->vnum;
		victim->hunter = ch;

		snprintf(buf, sizeof(buf),
			"$N[%d] {mon quest{x to {W%s{x in {W%s{x to kill %s.",
			ch->level,
			victim->in_room->area->name,
			mlstr_mval(victim->in_room->name),
			mlstr_mval(victim->short_descr));
		wiznet(buf,
			ch, NULL, WIZ_QUESTS, 0, 0);
	}

	ch->pcdata->questgiver = questor->pIndexData->vnum;
	ch->pcdata->questtime = number_range(10, 20) + ch->level/10;
	quest_tell(ch, questor,
		   "You have {W%d{z minutes to complete this quest.", 
		   ch->pcdata->questtime);
	quest_tell(ch, questor, "May the gods go with you!");
}

static void quest_complete(CHAR_DATA *ch, char *arg)
{
	bool complete = FALSE;
	char buf[MAX_INPUT_LENGTH];
	CHAR_DATA *questor;
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	WORLD_AFFECT_DATA *waf;

	int gold_reward = 0;
	int qp_reward = 0;
	int exp_reward = 0;
	
	if ((questor = questor_lookup(ch)) == NULL)
		return;

	act("$n informs $N $e has completed $s quest.",
	    ch, NULL, questor, TO_ROOM);
	act_puts("You inform $N you have completed your quest.",
		 ch, NULL, questor, TO_CHAR, POS_DEAD);

	if (!IS_ON_QUEST(ch)) {
		quest_tell(ch, questor, "You have to REQUEST a quest first, {W%s{z.",
			   ch->name); 
		return;
	}

	if (ch->pcdata->questgiver != questor->pIndexData->vnum) {
		quest_tell(ch, questor,
			   "I never sent you on a quest! Perhaps you're "
			   "thinking of someone else.");
		return;
	}

	if (ch->pcdata->questobj > 0)
		for (obj = ch->carrying; obj; obj = obj_next) {
			obj_next = obj->next_content;

			if (obj->pIndexData->vnum == ch->pcdata->questobj
			&&  IS_OWNER(ch, obj)) {
				act_puts("You hand {W$p{x to $N.",
					 ch, obj, questor, TO_CHAR, POS_DEAD);
				act("$n hands {W$p{x to $N.",
				    ch, obj, questor, TO_ROOM);
				extract_obj(obj, 0);

				complete = TRUE;
				break;
			}
		}
	else if (ch->pcdata->questmob == -1) {
		complete = TRUE;
	}

	if (!complete) {
		quest_tell(ch, questor,
			   "You haven't completed the quest yet, but there is "
			   "still time!");
		return;
	}

	/* quest rewards */
	if (ch->pcdata->questobj > 0) {
		qp_reward = number_range((ch->level - ch->level / 10 +1),
					 (ch->level + ch->level / 50 +1));
		qp_reward = UMAX(1, qp_reward);
		gold_reward = UMAX(1, number_range(1, ch->level/10));
		exp_reward = number_range(75, 125);
	}
	/* mob quest */
	else {
		qp_reward = number_range((ch->level - ch->level / 10 +1),
					 (ch->level + ch->level / 10 +1));
		qp_reward = UMAX(1, qp_reward);
		gold_reward = UMAX(2, number_range(1, ch->level/8));
	}

	if ((waf = ch_waffected(ch, WAFF_QP)) != NULL) {
		if (number_percent() <= waf->chance)
			qp_reward = qp_reward * waf->modifier / 100;
	}

	/*ch->gold += gold_reward;*/
	ch->pcdata->questpoints += qp_reward;

	quest_tell(ch, questor, "Congratulations on completing your quest!");
	ch->pcdata->questcount++;
	if (exp_reward > 0 && ch->level < HERO) {
		quest_tell(ch, questor,
			   "As a reward,"
			   " I am giving you {Y%d{z quest points,"
			   " {Y%d{z experience points."
			   /*" and {Y%d{z gold to cover expenses."*/,
			   qp_reward, exp_reward/*, gold_reward*/);
		gain_exp(ch, exp_reward);
	}
	else
		quest_tell(ch, questor,
			   "As a reward,"
			   " I am giving you {Y%d{z quest points."
			   /*" and {Y%d{z gold to cover expenses."*/,
			   qp_reward/*, gold_reward*/);

	snprintf(buf, sizeof(buf),
		"$N[%d] {mrewarded{x {M%d{xqp by $t.",
		ch->level,
		qp_reward);
	wiznet(buf,
		ch, mlstr_mval(questor->short_descr), WIZ_QUESTS, 0, 0);

	quest_cancel(ch);
	ch->pcdata->questtime = -number_range(6, 9);
}

/*
 * give questus an item
 *    if this is a quest item he will give them
 *    a percentage of the original cost.
 *
 *    Also, you can return IMM quest items to Questus
 *    and he will give you the cost in quest points.
 *
 * by Zsuzsu
 */
static void quest_return(CHAR_DATA *ch, char *arg)
{
	CHAR_DATA *questor;
	QITEM_DATA *qitem;
	qtrouble_t *qt = NULL;
	OBJ_DATA *obj;
	int qp_reward = 0;
	char buf[MAX_STRING_LENGTH];

	if ((questor = questor_lookup(ch)) == NULL)
		return;

	if (arg[0] == '\0') {
		char_puts("To turn in an item, type 'QUEST RETURN <item>'.\n", ch);
		return;
	}	

        if ((obj = get_obj_carry(ch, arg)) == NULL) {
		char_puts("You do not have that item.\n", ch);
		return;
	}

	/* non-quest reward */

	if (!IS_SET(obj->pIndexData->extra_flags, ITEM_QUEST)) {
		quest_tell(ch, questor,
			   "It's nice of you to offer,"
			   " but no thank you.");
		return;
	}

	qt = qtrouble_lookup(ch, obj->pIndexData->vnum);
	qitem = quest_item_by_vnum(obj->pIndexData->vnum);

	/* non-quest item */
	if (!qitem) {
		qp_reward = UMIN(obj->cost, obj->pIndexData->cost);

		if (qp_reward > 300) {
			quest_tell(ch, questor,
				   "I'm sorry, but that item is so valuable"
				   " only an immortal can reward you for it.");
			return;
		}

		qp_reward = UMAX(1, qp_reward * ch->level / 100);

		DEBUG(DEBUG_QUEST,
			"quest return: %s[%d](%d) rewarded %d for [%d] %s",
			ch->name, ch->level, 
			ch->pcdata->questpoints, 
			qp_reward,
			obj->pIndexData->vnum, obj->name);

		act_puts("You hand {W$p{x to $N.",
			 ch, obj, questor, TO_CHAR, POS_DEAD);

		act_puts("$n hands {W$p{x to $N.",
		    ch, obj, questor, TO_ROOM, POS_RESTING);

		quest_tell(ch, questor,
			"Oh, {W%s{z, won't the immortals be pleased!",
			ch->name);

		quest_tell(ch, questor,
			   "As a reward,"
			   " I am giving you {Y%d{z quest points.",
			   qp_reward);

		ch->pcdata->questpoints += qp_reward;

		snprintf(buf, sizeof(buf),
			"$N[%d] {mrewarded{x {M%d{xqp for returning $p.",
			ch->level,
			qp_reward);
		wiznet(buf,
			ch, obj, WIZ_QUESTS, 0, 0);

		extract_obj(obj, 0);

		return;
	}


	/* item not in the world */
	if (quest_item_in_world(ch, qitem->vnum) <= 0) {
		quest_tell(ch, questor,
			   "If you had one, I might take it.");
			return;
	}

	/* now delete it from the record */
	qtrouble_delete(ch, obj->pIndexData->vnum);
	quest_item_extract(ch, obj->pIndexData->vnum);

	qp_reward = quest_item_price(ch, qitem) / 2;

	DEBUG(DEBUG_QUEST,
		"quest return: %s[%d](%d) reinbursed %d for [%d] %s",
		ch->name, ch->level, 
		ch->pcdata->questpoints, 
		qp_reward,
		obj->pIndexData->vnum, qitem->name);

	ch->pcdata->questpoints += qp_reward;

	quest_tell(ch, questor,
		"Thank you, {W%s{z."
		"  I'll reinburse you, {Y%d{z quest points.",
		ch->name, qp_reward);
}


/*
 * recover lost items
 *
 * rewritten by Zsusu
 */
static void quest_trouble(CHAR_DATA *ch, char *arg)
{
	CHAR_DATA *questor;
	QITEM_DATA *qitem;
	OBJ_DATA *reward;
	qtrouble_t *qt = NULL;
	class_t *cl;
	OBJ_INDEX_DATA *pObjIndex;

	if ((questor = questor_lookup(ch)) == NULL
	||  (cl = class_lookup(ch->class)) == NULL)
		return;

	if (arg[0] == '\0') {
		char_puts("To correct a quest award's trouble,"
			  " type: 'quest trouble <award>'.\n", ch);
		return;
	}

	/* which item are we talking about */
	for (qitem = qitem_table; qitem->name; qitem++) {

		if (!qitem->vnum) 
			continue;

		/* if they already have one -- even though
		 * they no longer can */
		qt = qtrouble_lookup(ch, qitem->vnum);

		if (is_name(arg, qitem->name)) {
			if (!quest_item_usable(ch, qitem)) {
				if (qt)
					break;
				else
					continue;
			}
			break;
		}
	}
	if (!qitem || !qitem->name) {
		quest_tell(ch, questor,
			   "{W%s{z, I'm not really sure what you're trying to do.",
			   ch->name);
		return;
	}

	if (!qt)
		qt = qtrouble_lookup(ch, qitem->vnum);

	/* ch has never bought this item */
	if (!qt) {
		quest_tell(ch, questor,
			"Sorry, {W%s{z, but you haven't bought "
			"that quest award, yet.\n",
			ch->name);    
		return;
	}

	/* item still in the world, don't trouble to get
	 * it back */
	if (quest_item_in_world(ch, qitem->vnum) > 0) {
		quest_tell(ch, questor,
			   "Maybe you should look for it"
			   " before you try to trouble it.");
			return;
	}

	/* look up object */
	pObjIndex = get_obj_index(qitem->vnum);

	/* check if they've exceeded their trouble max
	 * or, if item doesn't have the "quest" flag
	 * refuse to trouble the item */
	if ((TROUBLE_MAX != -1 && qt->count > TROUBLE_MAX)
	||  !IS_SET(pObjIndex->extra_flags, ITEM_QUEST)) {

		quest_tell(ch, questor,
			   "This item is beyond the trouble option.");
		return;
	}

#if 0
	/* this can be used to give people items back that they
	 * have lost.  It should probable come with a cost */

	/* look for item in the world and extract */
	was_in_world = quest_item_extract(ch, qt->vnum);

	debug_printf("%s troubled %s in world: [%d] %s.",
		ch->name, 
		(was_in_world) ? "item" : "item not",
		qitem->vnum,
		qitem->name);
#endif

	reward = quest_give_item(ch, questor, qitem->vnum);

	if (!reward)
		return;

	DEBUG(DEBUG_QUEST,
		"%s[%d][%d] quest trouble [%d/%d] %s",
		ch->name, ch->level, 
		ch->pcdata->questpoints, 
		qitem->vnum, qt->count, qitem->name);

	quest_tell(ch, questor,
		   "This is the {Y%i{x time that I am giving "
		   "that award back.",
		   qt->count);

	qt->count++;


	if (TROUBLE_MAX != -1 && qt->count > TROUBLE_MAX) 
		quest_tell(ch, questor,
			   "And I won't give you that again, "
			   "with trouble option.\n");
}

/*
 * generate the quest object and give it to the char
 *
 * rewritten by Zsuzsu
 */
static OBJ_DATA *quest_give_item(CHAR_DATA *ch, CHAR_DATA *questor,
			    int item_vnum)
{
	OBJ_DATA *reward;
	OBJ_INDEX_DATA *pObjIndex = get_obj_index(item_vnum);
	AFFECT_DATA af;
	char buf[MAX_STRING_LENGTH];

	/* ok, give him requested item */
	reward = create_obj(pObjIndex, 0);
	
	if (get_wear_level(ch, reward) < reward->level) {
		quest_tell(ch, questor,
			   "This item is too powerful for you.\n");
		extract_obj(reward, 0);
		return NULL;
	}

	/* if it's a quest item brand it with the owner's name */
	if (IS_SET(pObjIndex->extra_flags, ITEM_QUEST)) {
		reward->owner = mlstr_dup(ch->short_descr);
		mlstr_free(reward->short_descr);
		reward->short_descr =
			mlstr_printf(reward->pIndexData->short_descr,
				     IS_GOOD(ch) ?	"{Choly{x" :
				     IS_NEUTRAL(ch) ?	"{bblue{w-{ggreen{x" : 
							"{revil{x", 
				     ch->name);

		if (IS_EVIL(ch)) {
			SET_BIT(reward->extra_flags, ITEM_EVIL);
		}

		if (IS_SET(pObjIndex->item_type, ITEM_WEAPON)) {
			af.where	= TO_WEAPON;
			af.type		= 0;
			af.level	= 91;
			af.duration	= -1;
			af.location	= 0;
			af.modifier	= 0;
			af.bitvector	= IS_GOOD(ch) ? WEAPON_HOLY :
					  IS_EVIL(ch) ? WEAPON_VAMPIRIC :
							WEAPON_FLAMING;

			/*
			if (IS_EVIL(ch)
			&& HAS_SKILL(ch, gsn_spellbane))
				af.bitvector = WEAPON_TAINTED;
			*/

			affect_to_obj(reward, &af);
		}
	}

	obj_to_char(reward, ch);

	act("$N gives {W$p{x to $n.", ch, reward, questor, TO_ROOM);
	act_puts("$N gives you {W$p{x.",
		 ch, reward, questor, TO_CHAR, POS_DEAD);

	snprintf(buf, sizeof(buf),
		"$N[%d] {mquest bought{x $p.",
		ch->level);
	wiznet(buf,
		ch, reward, WIZ_QUESTS, 0, 0);

	return reward;
}

/*
 * buy item
 *
 * rewritten by Zsuzsu
 */
static bool buy_item(CHAR_DATA *ch, CHAR_DATA *questor, int item_vnum)
{
	qtrouble_t *qt = NULL;
	OBJ_DATA *reward = NULL;
	OBJ_INDEX_DATA *pObjIndex = get_obj_index(item_vnum);

	qt = qtrouble_lookup(ch, item_vnum);

	/* check if they bought the item 
	 * and still have troubles left */
	if (qt && (TROUBLE_MAX == -1 || qt->count <= TROUBLE_MAX)) {
		quest_tell(ch, questor,
			   "You have already bought this item.");
		return FALSE;
	}

	/* don't let people buy two quest items of the same type (weapon)*/
	if (quest_has_similar(ch, item_vnum)) {
		quest_tell(ch, questor,
			   "You already have a similar item.");
			return FALSE;
	}

	/* if the item is in the world, don't sell them a new one */
	if (quest_item_in_world(ch, item_vnum)) {
		quest_tell(ch, questor,
			   "Maybe you should look for it"
			   " before you try to buy another.");
			return FALSE;
	}

	reward = quest_give_item(ch, questor, item_vnum);

	if (reward) {
		if (qt)
			qt->count = 1;

		else if (IS_SET(pObjIndex->extra_flags, ITEM_QUEST)) {
			qt = malloc(sizeof(*qt));
			qt->vnum = item_vnum;
			qt->count = 1;
			qt->next = ch->pcdata->qtrouble;
			ch->pcdata->qtrouble = qt;
		}
	}


	return reward != NULL;
}

/*
static bool buy_gold(CHAR_DATA *ch, CHAR_DATA *questor)
{
	ch->pcdata->bank_g += 10000;
	act("$N gives 10,000 gold pieces to $n.", 
		ch, NULL, questor, TO_ROOM);
	act("$N transfers {Y10,000{x gold pieces to your bank account.\n",
		ch, NULL, questor, TO_CHAR);
	return TRUE;
}
*/

static bool buy_tattoo(CHAR_DATA *ch, CHAR_DATA *questor)
{
	OBJ_DATA *tattoo;
	char buf[MAX_STRING_LENGTH];

	if (!ch->religion || ch->religion == RELIGION_ATHEIST) {
		char_puts("You must worship a god to have a tattoo.\n", ch);
		return FALSE;
	}

	tattoo = get_eq_char(ch, WEAR_TATTOO);
	if (tattoo != NULL) {
		char_puts("But you have already your tattoo!\n", ch);
		return FALSE;
	}

	tattoo = create_obj(get_obj_index(religion_table[ch->religion].vnum), 0);

	obj_to_char(tattoo, ch);
	equip_char(ch, tattoo, WEAR_TATTOO);
	act("$N tattoos $n with {W$p{x!", ch, tattoo, questor, TO_ROOM);
	act_puts("$N tattoos you with {W$p{x!",
		 ch, tattoo, questor, TO_CHAR, POS_DEAD);

	snprintf(buf, sizeof(buf),
		"$N[%d] {mquest bought{x $p.",
		ch->level);
	wiznet(buf,
		ch, tattoo, WIZ_QUESTS, 0, 0);

	return TRUE;
}

/* buy consitution points
 * 	only for those truely in desperate straits
 *
 * by Zsuzsu
 */
static bool buy_con(CHAR_DATA *ch, CHAR_DATA *questor)
{
	CHAR_DATA *vch;
	char buf[MAX_STRING_LENGTH];

	if (ch->level < LEVEL_HERO) {
		quest_tell(ch, questor,
		"Sorry, but, {W%s{z, but you'll have to earn"
		" Hero status before you can do that.", 
			ch->name);
		return FALSE;
	}
	if (ch->perm_stat[STAT_CON] >= get_max_train(ch,STAT_CON)) {
		quest_tell(ch, questor, 
			"You look healthy enough to me, {W%s{z.",
			ch->name);
		return FALSE;
	}

	if (ch->train > 0 || ch->practice > 9) {
		quest_tell(ch, questor, 
			"You don't look that bad off."
			" Perhaps you should visit the trainer before coming to me.",
			ch->name);
		return FALSE;
	}

	act("$n looks up to the sky and mumbles in conversation before"
		" turning $s gaze back to $N.  With a wink, a pulse of "
		" {mviolet{x light spreads over $N into the corners of the room.",
		questor, NULL, ch, TO_NOTVICT);
	act_puts("$n looks up to the sky and mumbles in conversation before"
		" turning $s gaze back to you.  With a wink, a pulse of"
		" {mviolet{x light spreads over you and into the corners of the room.",
		questor, NULL, ch, TO_VICT, POS_DEAD);

	ch->perm_stat[STAT_CON] += 1;
	act_puts("Your constitution returns!",
		 ch, NULL, NULL, TO_CHAR, POS_DEAD);

	/* just a little something for everyone in the room */
	for (vch = ch->in_room->people; vch; vch = vch->next_in_room) {
		affect_strip(vch,gsn_plague);
		affect_strip(vch,gsn_poison);
		affect_strip(vch,gsn_blindness);
		affect_strip(vch,gsn_curse);

		if (vch->hit < vch->max_hit)
			vch->hit        = vch->max_hit;

		act_puts("The {mviolet{x light restores you.",
			 vch, NULL, NULL, TO_CHAR, POS_DEAD);
	}

	snprintf(buf, sizeof(buf),
		"$N[%d] {mquest bought{x {Wconstitution{x[%d].",
		ch->level,
		ch->perm_stat[STAT_CON]);
	wiznet(buf,
		ch, NULL, WIZ_QUESTS, 0, 0);


	return TRUE;
}

static bool buy_death(CHAR_DATA *ch, CHAR_DATA *questor)
{
	char buf[MAX_STRING_LENGTH];

	if (ch->pcdata->death < 1) {
		quest_tell(ch, questor, 
			   "Sorry, {W%s{z, but you haven't got any deaths yet.",
			   ch->name);
		return FALSE;
	}

	ch->pcdata->death -= 1;

	snprintf(buf, sizeof(buf),
		"$N[%d] {mquest bought{x a {Ddeath{x[%d].",
		ch->level,
		ch->pcdata->death);
	wiznet(buf,
		ch, NULL, WIZ_QUESTS, 0, 0);

	return TRUE;
}

static bool buy_katana(CHAR_DATA *ch, CHAR_DATA *questor)
{
	AFFECT_DATA af;
	OBJ_DATA *katana;

	if ((katana = get_obj_list(ch, "katana", ch->carrying)) == NULL) {
		quest_tell(ch, questor, "Sorry, {W%s{z, but you don't have your katana with you.",
			   ch->name);
		return FALSE;
	}

	af.where	= TO_WEAPON;
	af.type 	= gsn_katana;
	af.level	= 100;
	af.duration	= -1;
	af.modifier	= 0;
	af.bitvector	= WEAPON_KATANA_QUEST;
	af.location	= APPLY_NONE;
	affect_to_obj(katana, &af);
	quest_tell(ch, questor, "As you wield it, you will feel that its power will increase continuosly.");
	return TRUE;
}

static bool buy_vampire(CHAR_DATA *ch, CHAR_DATA *questor)
{
	set_skill(ch, gsn_vampire, 100);
	act("$N tells a creepy secret to $n.", ch, NULL, questor, TO_ROOM);
	act_puts("$N lets you in on the secret of the undead.",
		 ch, NULL, questor, TO_CHAR, POS_DEAD);
	act("Lightning flashes in the sky.", ch, NULL, NULL, TO_ALL);
	return TRUE;
}

/* look to see if the character already has a similar item */
bool quest_has_similar (CHAR_DATA *ch, int vnum) {
	qtrouble_t *qt		= NULL;
	bool has_similar	= FALSE;
	int quest_group = -1;
	int j = 0;

	quest_group = quest_item_group(vnum);

	if (quest_group == -1)
		return FALSE;

	j = 0;
	while (!has_similar && quest_item_groups[quest_group][j]) {
		/* if not the same item
		 * and there exists a trouble record
		 * and troubles are infinite
		 * or the trouble count is lower than the MAX */
		if (quest_item_groups[quest_group][j] != vnum
		&& (qt = qtrouble_lookup(ch, quest_item_groups[quest_group][j])) != NULL
		&& (TROUBLE_MAX == -1 || qt->count <= TROUBLE_MAX))
			has_similar = TRUE;

		/* look to see if they have an item that's out of troubles
		 * but still in the world */
		else if (quest_item_groups[quest_group][j] != vnum
		&& quest_item_in_world(ch, quest_item_groups[quest_group][j]))
			has_similar = TRUE;
		j++;
	}

	return has_similar;
}

/* extract all similar items */
int quest_extract_similar (CHAR_DATA *ch, int vnum) {
	int quest_group = -1;
	int j = 0;
	int count = 0;

	quest_group = quest_item_group(vnum);

	if (quest_group == -1)
		return FALSE;

	j = 0;
	while (quest_item_groups[quest_group][j]) {
		count += quest_item_extract(ch, quest_item_groups[quest_group][j]);
		j++;
	}
	return count;
}

/*
 * find out which group the item is in to check for similarity
 */
int quest_item_group (int vnum) {
	int quest_group = -1;
	int i = 0;
	int j = 0;

	/* find out which group our vnum is in */
	while (quest_group == -1 && quest_item_groups[i][0]) {
		while (quest_item_groups[i][j]) {
			if (quest_item_groups[i][j] == vnum) {
				quest_group = i;
				break;
			}
			j++;
		}
		i++;
	}
	return quest_group;
}

/*
 * find your quest item in the world and extract it
 * so there are no duplicates
 *
 * returns the number of them extracted
 */
int quest_item_extract(CHAR_DATA *ch, int vnum) {
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	int count = 0;

	if (vnum <= 0) return 0;

	for (obj = object_list; obj != NULL; obj = obj_next) {
		obj_next = obj->next;
		if (obj->pIndexData->vnum == vnum 
		&&  IS_OWNER(ch, obj)) {
			extract_obj(obj, 0);
			count++;
			break;
		}
	}
	return count;
}

/*
 * return the number of this type of quest item in 
 * the world that belongs to this player
 */
int quest_item_in_world (CHAR_DATA *ch, int vnum) {
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;
	int count = 0;

	if (vnum <= 0) return 0;

	for (obj = object_list; obj != NULL; obj = obj_next) {
		obj_next = obj->next;
		if (obj->pIndexData->vnum == vnum 
		&&  IS_OWNER(ch, obj)) {
			count++;
			break;
		}
	}
	return count;
}

bool is_quest_item (OBJ_INDEX_DATA *pObj)
{
	switch (pObj->vnum) {
		case QUEST_VNUM_RING:
		case QUEST_VNUM_BRACER:
		case QUEST_VNUM_AMULET:
		case QUEST_VNUM_SONG:

		case QUEST_VNUM_LANCE:
		case QUEST_VNUM_BOW:
		case QUEST_VNUM_POLEARM:
		case QUEST_VNUM_BASTARDSWORD:
		case QUEST_VNUM_BATTLEAXE:
		case QUEST_VNUM_WARHAMMER:
		case QUEST_VNUM_LONGSWORD:
		case QUEST_VNUM_AXE:
		case QUEST_VNUM_HAMMER:
		case QUEST_VNUM_SHORTSWORD:
		case QUEST_VNUM_STILETTO:
		case QUEST_VNUM_DAGGER:
		case QUEST_VNUM_FLAIL:
		case QUEST_VNUM_MACE:
		case QUEST_VNUM_SPEAR:
		case QUEST_VNUM_STAFF:
		case QUEST_VNUM_WHIP:
			return TRUE;
	}
	return FALSE;
}

bool quest_item_usable (CHAR_DATA *ch, QITEM_DATA *qitem) {
	class_t *cl = class_lookup(ch->class);
	int group = -1;

	if (!qitem || !cl)
		return FALSE;

	/* if the item is restricted to a class */
	if (qitem->restrict_class != NULL) {
		if (is_name(cl->name, qitem->restrict_class))
			return TRUE;
		else
			return FALSE;
	}

	if (!qitem->vnum)
		return TRUE;

	if (IS_NEWBIE(ch) && qitem->vnum == QUEST_VNUM_RUG)
		return FALSE;

	group = quest_item_group(qitem->vnum);

	if (ch->class == CLASS_SAMURAI
	&& group == QUEST_GROUP_WEAPON)
		return FALSE;

	if (ch->class == CLASS_CLERIC
	&& qitem->vnum ==  QUEST_VNUM_WARHAMMER)
		return FALSE;

	if ((ch->race == RACE_DWARF
	|| ch->race == RACE_DUERGAR
	|| ch->size < SIZE_MEDIUM)
	&& (qitem->vnum == QUEST_VNUM_POLEARM
	|| qitem->vnum == QUEST_VNUM_BASTARDSWORD))
		return FALSE;

	if (!qitem->gsn)
		return TRUE;

	if (get_skill(ch, *(qitem->gsn)) <= 0)
		return FALSE;

	return TRUE;
}

QITEM_DATA * quest_item_by_vnum (int vnum)
{
	QITEM_DATA *qitem;
	
	for (qitem = qitem_table; qitem->name; qitem++)
		if (qitem->vnum == vnum)
			return qitem;

	return NULL;
}

/*
 * set dynamic prices for progressive quest rewards
 */
int quest_item_price (CHAR_DATA *ch, QITEM_DATA *qitem)
{
	int price = qitem->price;

		switch (qitem->vnum) {
			case QUEST_VNUM_RUG:
				price = qitem->price * URANGE(10, ch->level, LEVEL_HERO);
				break;
		}

		if (qitem->vnum != 0 && IS_NEWBIE(ch))
			price = price * QUEST_NEWBIE_DISCOUNT;

		/* samurai deaths */
		if (qitem->do_buy == buy_death)
			price = price * URANGE(1, ch->level, LEVEL_HERO);


	return price;
}

bool is_quest_complete (CHAR_DATA *ch)
{
	OBJ_DATA *obj, *obj_next;
	bool complete = FALSE;

	if (IS_NPC(ch))
		return FALSE;

	if (!IS_ON_QUEST(ch))
		return FALSE;

	if (ch->pcdata->questobj > 0)
		for (obj = ch->carrying; obj; obj = obj_next) {
			obj_next = obj->next_content;

			if (obj->pIndexData->vnum == ch->pcdata->questobj
			&&  IS_OWNER(ch, obj)) {
				complete = TRUE;
				break;
			}
		}
	else if (ch->pcdata->questmob == -1) {
		complete = TRUE;
	}

	return complete;
}

