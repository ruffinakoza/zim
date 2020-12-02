/*
 * $Id: act_info.c 991 2006-12-30 20:30:13Z zsuzsu $
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
#include <ctype.h>

#if defined(SUNOS) || defined(SVR4) || defined(LINUX)
#	include <crypt.h>
#endif

#include "merc.h"
#include "update.h"
#include "quest.h"
#include "obj_prog.h"
#include "fight.h"
#include "waffects.h"
#include "stats.h"
#include "debug.h"


/* command procedures needed */
DECLARE_DO_FUN(do_exits		);
DECLARE_DO_FUN(do_look		);
DECLARE_DO_FUN(do_help		);
DECLARE_DO_FUN(do_affects	);
DECLARE_DO_FUN(do_murder	);
DECLARE_DO_FUN(do_say		);
DECLARE_DO_FUN(do_alist		);

void do_help_border(CHAR_DATA *ch, const char *argument, bool border);

static int show_order[] = {
	WEAR_LIGHT,
	WEAR_HEAD,
	WEAR_NECK_1,
	WEAR_NECK_2,
	WEAR_ABOUT,
	WEAR_BODY,
	WEAR_ARMS,
	WEAR_WRIST_L,
	WEAR_WRIST_R,
	WEAR_HANDS,
	WEAR_FINGER_L,
	WEAR_FINGER_R,
	WEAR_WIELD,
	WEAR_SECOND_WIELD,
	WEAR_SHIELD,
	WEAR_HOLD,
	WEAR_WAIST,
	WEAR_LEGS,
	WEAR_FEET,
	WEAR_FLOAT,
	WEAR_TATTOO,
	WEAR_CLANMARK,
	-1
};

/* for do_count */
int max_on = 0;

/*
 * Local functions.
 */
char *	format_obj_to_char	(OBJ_DATA *obj, CHAR_DATA *ch, bool fShort);
void	show_char_to_char_0	(CHAR_DATA *victim, CHAR_DATA *ch);
void	show_char_to_char_1	(CHAR_DATA *victim, CHAR_DATA *ch);
void	show_char_to_char	(CHAR_DATA *list, CHAR_DATA *ch);
char * 	format_modifier		(char *buf, size_t len, int base, int mod);

DO_FUN(do_maxon) {
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0' || !is_number(arg)) {
		char_printf(ch, "Current maxon %d.", max_on);
		return;
	}

	max_on = atoi(arg);
	char_printf(ch, "Maxon now %d.", max_on);
}


char *format_obj_to_char(OBJ_DATA *obj, CHAR_DATA *ch, bool fShort)
{
	static char buf[MAX_STRING_LENGTH];

	buf[0] = '\0';
	if ((fShort && mlstr_null(obj->short_descr))
	||  mlstr_null(obj->description))
		return str_empty;

	if (IS_SET(ch->comm, COMM_LONG)) {
		if (IS_OBJ_STAT(obj, ITEM_INVIS))
			strnzcat(buf, sizeof(buf),
				 GETMSG("({yInvis{x) ", ch->lang));
		if (IS_OBJ_STAT(obj, ITEM_DARK))
			strnzcat(buf, sizeof(buf),
				 GETMSG("({DDark{x) ", ch->lang));
		if (IS_AFFECTED(ch, AFF_DETECT_EVIL)
		&&  IS_OBJ_STAT(obj, ITEM_EVIL))
			strnzcat(buf, sizeof(buf),
				 GETMSG("({RRed Aura{x) ", ch->lang));
		if (IS_AFFECTED(ch, AFF_DETECT_GOOD)
		&&  IS_OBJ_STAT(obj, ITEM_BLESS))
			strnzcat(buf, sizeof(buf),
				 GETMSG("({BBlue Aura{x) ", ch->lang));
		if (IS_AFFECTED(ch, AFF_DETECT_MAGIC)
		&&  IS_OBJ_STAT(obj, ITEM_MAGIC))
			strnzcat(buf, sizeof(buf),
				 GETMSG("({MMagical{x) ", ch->lang));
		if (IS_OBJ_STAT(obj, ITEM_GLOW))
			strnzcat(buf, sizeof(buf),
				 GETMSG("({WGlowing{x) ", ch->lang));
		if (IS_OBJ_STAT(obj, ITEM_HUM))
			strnzcat(buf, sizeof(buf),
				 GETMSG("({YHumming{x) ", ch->lang));
	}
	else {
		static char flag_tS[] = "{x[{y.{D.{R.{B.{M.{W.{Y.{x] ";
		strnzcpy(buf, sizeof(buf), flag_tS);
		if (IS_OBJ_STAT(obj, ITEM_INVIS)	)   buf[5] = 'I';
		if (IS_OBJ_STAT(obj, ITEM_DARK)		)   buf[8] = 'D';
		if (IS_AFFECTED(ch, AFF_DETECT_EVIL)
		&& IS_OBJ_STAT(obj, ITEM_EVIL)		)   buf[11] = 'E';
		if (IS_AFFECTED(ch, AFF_DETECT_GOOD)
		&&  IS_OBJ_STAT(obj,ITEM_BLESS)		)   buf[14] = 'B';
		if (IS_AFFECTED(ch, AFF_DETECT_MAGIC)
		&& IS_OBJ_STAT(obj, ITEM_MAGIC)		)   buf[17] = 'M';
		if (IS_OBJ_STAT(obj, ITEM_GLOW)		)   buf[20] = 'G';
		if (IS_OBJ_STAT(obj, ITEM_HUM)		)   buf[23] = 'H';
		if (strcmp(buf, flag_tS) == 0)
			buf[0] = '\0';
	}

	if (fShort) {
		strnzcat(buf, sizeof(buf),
			 format_short(obj->short_descr, obj->name, ch));
		if (obj->pIndexData->vnum > 5 /* not money, gold, etc */
		&&  (obj->condition < COND_EXCELLENT)) {
			char buf2[MAX_STRING_LENGTH];
			snprintf(buf2, sizeof(buf2), " [{g%s{x]",
				 GETMSG(get_cond_alias(obj), ch->lang));
			strnzcat(buf, sizeof(buf), buf2);
		}
		return buf;
	}

	if (obj->in_room && IS_WATER(obj->in_room)) {
		char* p;

		p = strchr(buf, '\0');
		strnzcat(buf, sizeof(buf),
			 format_short(obj->short_descr, obj->name, ch));
		p[0] = UPPER(p[0]);
		switch(number_range(1, 3)) {
		case 1:
			strnzcat(buf, sizeof(buf),
				 " is floating gently on the water.");
			break;
		case 2:
			strnzcat(buf, sizeof(buf),
				 " is making it's way on the water.");
			break;
		case 3:
			strnzcat(buf, sizeof(buf),
				 " is getting wet by the water.");
			break;
		}
	}
	else
		strnzcat(buf, sizeof(buf), format_descr(obj->description, ch));
	return buf;
}

/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char(OBJ_DATA *list, CHAR_DATA *ch,
		       bool fShort, bool fShowNothing)
{
	BUFFER *output;
	const char **prgpstrShow;
	int *prgnShow;
	char *pstrShow;
	OBJ_DATA *obj;
	int nShow;
	int iShow;
	int count;
	bool fCombine;

	if (ch->desc == NULL)
		return;

	/*
	 * Alloc space for output lines.
	 */
	output = buf_new(-1);

	count = 0;
	for (obj = list; obj != NULL; obj = obj->next_content)
		count++;
	prgpstrShow = malloc(count * sizeof(char *));
	prgnShow    = malloc(count * sizeof(int)  );
	nShow	= 0;

	/*
	 * Format the list of objects.
	 */
	for (obj = list; obj != NULL; obj = obj->next_content) {
		if ((obj->wear_loc == WEAR_NONE 
		|| obj->wear_loc == WEAR_MEDAL
		|| obj->wear_loc == WEAR_AWARD)
		&& can_see_obj(ch, obj)) {
			pstrShow = format_obj_to_char(obj, ch, fShort);

			fCombine = FALSE;

			if (IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE)) {
				/*
				 * Look for duplicates, case sensitive.
				 * Matches tend to be near end so run loop
				 * backwords.
				 */
				for (iShow = nShow - 1; iShow >= 0; iShow--) {
					if (!strcmp(prgpstrShow[iShow],
						    pstrShow)) {
						prgnShow[iShow]++;
						fCombine = TRUE;
						break;
					}
				}
			}

			/*
			 * Couldn't combine, or didn't want to.
			 */
			if (!fCombine) {
				prgpstrShow [nShow] = str_dup(pstrShow);
				prgnShow    [nShow] = 1;
				nShow++;
			}
		}
	}

	/*
	 * Output the formatted list.
	 */
	for (iShow = 0; iShow < nShow; iShow++) {
		if (prgpstrShow[iShow][0] == '\0')
			continue;

		if (IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE)) {
			if (prgnShow[iShow] != 1) 
				buf_printf(output, "(%2d) ", prgnShow[iShow]);
			else
				buf_add(output,"     ");
		}

		buf_add(output, prgpstrShow[iShow]);
		buf_add(output,"\n");
		free_string(prgpstrShow[iShow]);
	}

	if (fShowNothing && nShow == 0) {
		if (IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE))
			char_puts("     ", ch);
		char_puts("Nothing.\n", ch);
	}

	page_to_char(buf_string(output),ch);

	/*
	 * Clean up.
	 */
	buf_free(output);
	free(prgpstrShow);
	free(prgnShow);
}

#define flag_t_SET(pos, c, exp) (flag_tS[pos] = (exp) ? (anyflags = TRUE, c) : '.')

void show_char_to_char_0(CHAR_DATA *victim, CHAR_DATA *ch)
{
/*	const char *action = mlstr_mval(victim->action_desc);*/
	char buf[MAX_INPUT_LENGTH];
	const char *msg = str_empty;
	const void *arg = NULL;
	const void *arg3 = NULL;
	flag32_t flags = 0;

	if (is_affected(victim, gsn_doppelganger)
	&&  (IS_NPC(ch) || !IS_SET(ch->conf_flags, PLR_CONF_HOLYLIGHT)))
		victim = victim->doppel;

	if (IS_NPC(victim)) {
		if (!IS_NPC(ch) && ch->pcdata->questmob > 0
		&&  victim->hunter == ch)
			char_puts("{r[{RTARGET{r]{x ", ch);
	}
	else {
		if (IS_SET(victim->state_flags, STATE_WANTED))
			char_puts("({RWanted{x) ", ch);

		if (IS_SET(victim->comm, COMM_AFK))
			char_puts("{c[AFK]{x ", ch);

		if (!IS_NPC(victim)
		&& victim->pcdata->enslaver != NULL)
			char_puts("{r[Enslaved]{x ", ch);
	}

	if (IS_SET(ch->comm, COMM_LONG)) {
		if (IS_AFFECTED(victim, AFF_INVIS))
			char_puts("({yInvis{x) ", ch);
		if (IS_AFFECTED(victim, AFF_HIDE)) 
			char_puts("({DHidden{x) ", ch);
		if (IS_AFFECTED(victim, AFF_CHARM)) 
			char_puts("({mCharmed{x) ", ch);
		if (IS_AFFECTED(victim, AFF_PASS_DOOR)) 
			char_puts("({cTranslucent{x) ", ch);
		if (IS_AFFECTED(victim, AFF_FAERIE_FIRE)) 
			char_puts("({MPink Aura{x) ", ch);
		if (IS_NPC(victim)
		&&  IS_SET(victim->pIndexData->act, ACT_UNDEAD)
		&&  IS_AFFECTED(ch, AFF_DETECT_UNDEAD))
			char_puts("({DUndead{x) ", ch);
		if (RIDDEN(victim))
			char_puts("({GRidden{x) ", ch);
		if (IS_AFFECTED(victim,AFF_IMP_INVIS))
			char_puts("({bImproved{x) ", ch);
		if (IS_EVIL(victim) && IS_AFFECTED(ch, AFF_DETECT_EVIL))
			char_puts("({RRed Aura{x) ", ch);
		if (IS_GOOD(victim) && IS_AFFECTED(ch, AFF_DETECT_GOOD))
			char_puts("({YGolden Aura{x) ", ch);
		if (IS_AFFECTED(victim, AFF_SANCTUARY)
		|| IS_AFFECTED(victim, AFF_MINOR_SANCTUARY))
			char_puts("({WWhite Aura{x) ", ch);
		if (IS_AFFECTED(victim, AFF_BLACK_SHROUD)
		|| IS_AFFECTED(victim, AFF_MINOR_BLACK_SHROUD))
			char_puts("({DBlack Aura{x) ", ch);
		if (IS_AFFECTED(victim, AFF_FADE)) 
			char_puts("({yFade{x) ", ch);
		if (IS_AFFECTED(victim, AFF_CAMOUFLAGE)) 
			char_puts("({gCamf{x) ", ch);
	}
	else {
		static char flag_tS[] = "{x[{y.{D.{m.{c.{M.{D.{G.{b.{R.{Y.{W.{y.{g.{x] ";
		bool anyflags = FALSE;

		flag_t_SET( 5, 'I', IS_AFFECTED(victim, AFF_INVIS));
		flag_t_SET( 8, 'H', IS_AFFECTED(victim, AFF_HIDE));
		flag_t_SET(11, 'C', IS_AFFECTED(victim, AFF_CHARM));
		flag_t_SET(14, 'T', IS_AFFECTED(victim, AFF_PASS_DOOR));
		flag_t_SET(17, 'P', IS_AFFECTED(victim, AFF_FAERIE_FIRE));
		flag_t_SET(20, 'U', IS_NPC(victim) &&
				  IS_SET(victim->pIndexData->act, ACT_UNDEAD) &&
				  IS_AFFECTED(ch, AFF_DETECT_UNDEAD));
		flag_t_SET(23, 'R', RIDDEN(victim));
		flag_t_SET(26, 'I', IS_AFFECTED(victim, AFF_IMP_INVIS));
		flag_t_SET(29, 'E', IS_EVIL(victim) &&
				  IS_AFFECTED(ch, AFF_DETECT_EVIL));
		flag_t_SET(32, 'G', IS_GOOD(victim) &&
				  IS_AFFECTED(ch, AFF_DETECT_GOOD));

		if (IS_AFFECTED(victim, AFF_SANCTUARY)
				|| IS_AFFECTED(victim, AFF_MINOR_SANCTUARY))
			flag_t_SET(34, 'W', TRUE);
		flag_t_SET(35, 'S', IS_AFFECTED(victim, AFF_SANCTUARY));

		if (IS_AFFECTED(victim, AFF_MINOR_SANCTUARY)) {
			flag_t_SET(35, 's', TRUE);
		}


		if (IS_AFFECTED(victim, AFF_BLACK_SHROUD)
		|| IS_AFFECTED(victim, AFF_MINOR_BLACK_SHROUD)) {
			flag_t_SET(34, 'D', TRUE);
		}
		if (IS_AFFECTED(victim, AFF_BLACK_SHROUD))
			flag_t_SET(35, 'B', TRUE);
		if (IS_AFFECTED(victim, AFF_MINOR_BLACK_SHROUD))
			flag_t_SET(35, 'b', TRUE);


		flag_t_SET(38, 'C', IS_AFFECTED(victim, AFF_CAMOUFLAGE));
		flag_t_SET(41, 'F', IS_AFFECTED(victim, AFF_FADE));

		if (anyflags)
			char_puts(flag_tS, ch);
	}

	if (victim->invis_level >= LEVEL_HERO)
		char_puts("[{WWizi{x] ", ch);
	if (victim->incog_level >= LEVEL_HERO)
		char_puts("[{DIncog{x] ", ch);

	if (IS_NPC(victim) && victim->position == victim->start_pos) {
		char_puts(format_descr(victim->long_descr, ch), ch);
		return;
	}

	if (IS_IMMORTAL(victim))
		char_puts("{W", ch);
	else
		char_puts("{x", ch);

	switch (victim->position) {
	case POS_DEAD:
		msg = "$N {xis DEAD!!";
		break;
	
	case POS_MORTAL:
		msg = "$N {xis mortally wounded.";
		break;
	
	case POS_INCAP:
		msg = "$N {xis incapacitated.";
		break;
	
	case POS_STUNNED:
		msg = "$N {xis lying here stunned.";
		break;
	
	case POS_SLEEPING:
		if (victim->on == NULL) {
			msg = "$N {xis sleeping here.";
			break;
		}
	
		arg = victim->on;
		if (IS_SET(victim->on->value[ITEM_FURNITURE_FLAGS], SLEEP_AT))
			msg = "$N {xis sleeping at $p.";
		else if (IS_SET(victim->on->value[ITEM_FURNITURE_FLAGS], SLEEP_ON))
			msg = "$N {xis sleeping on $p.";
		else
			msg = "$N {xis sleeping in $p.";
		break;
	
	case POS_RESTING:
		if (victim->on == NULL) {
			msg = "$N {xis resting here.";
			break;
		}

		arg = victim->on;
		if (IS_SET(victim->on->value[ITEM_FURNITURE_FLAGS], REST_AT))
			msg = "$N {xis resting at $p.";
		else if (IS_SET(victim->on->value[ITEM_FURNITURE_FLAGS], REST_ON))
			msg = "$N {xis resting on $p.";
		else
			msg = "$N {xis resting in $p.";
		break;
	
	case POS_SITTING:
		if (victim->on == NULL) {
			if(is_affected(victim, gsn_rnet_trap))
				msg = "$N {xis situated uncomfortably in a hanging net.";
			else
			msg = "$N {xis sitting here.";
			break;
		}
	
		arg = victim->on;
		if (IS_SET(victim->on->value[ITEM_FURNITURE_FLAGS], SIT_AT))
			msg = "$N {xis sitting at $p.";
		else if (IS_SET(victim->on->value[ITEM_FURNITURE_FLAGS], SIT_ON))
			msg = "$N {xis sitting on $p.";
		else
			msg = "$N {xis sitting in $p.";
		break;
	
	case POS_STANDING:
		if (victim->on == NULL) {
			if (!IS_NPC(victim)
			&&  !IS_SET(ch->comm, COMM_BRIEF))
			{
				if ((ch->class != victim->class)
				&& (victim->level < LEVEL_HERO)
				&& (!IS_IMMORTAL(ch)))
					msg = "";
				else
					arg = victim->pcdata->title;
			}

			if (MOUNTED(victim)) {
				arg3 = MOUNTED(victim);
				msg = "$N{x$t {xis here, riding $I.";
			}
			else
			{
				msg = "$N{x$t {xis here.";
			}
			break;
		}
	
		arg = victim->on;
		if (IS_SET(victim->on->value[ITEM_FURNITURE_FLAGS],STAND_AT))
			msg = "$N {xis standing at $p.";
		else if (IS_SET(victim->on->value[ITEM_FURNITURE_FLAGS],STAND_ON))
			msg = "$N {xis standing on $p.";
		else
			msg = "$N {xis standing in $p.";
		break;
	
	case POS_FIGHTING:
		if (victim->fighting == NULL) {
			arg = "thin air??";
			flags = ACT_TRANS;
			msg = "$N {xis here, fighting with $t.";
		}
		else if (victim->fighting == ch) {
			arg = "YOU!";
			flags = ACT_TRANS;
			msg = "$N {xis here, fighting with $t.";
		}
		else if (victim->in_room == victim->fighting->in_room) {
			arg = victim->fighting;
			msg = "$N {xis here, fighting with $i.";
		}
		else {
			arg = "someone who left??";
			flags = ACT_TRANS;
			msg = "$N {xis here, fighting with $t.";
		}
		break;
	}

	if (victim->position != POS_DEAD
	&& IS_SET(victim->state_flags, STATE_GHOST))
		snprintf(buf, sizeof(buf), "The {cg{Chost{x of %s", msg);
	else
		strncpy(buf, msg, sizeof(buf));

	act_puts3(buf, ch, arg, victim, arg3,
		  TO_CHAR | ACT_FORMSH | flags, POS_DEAD);
}

char* wear_loc_names[] =
{
	"<used as light>     $t",
	"<worn on finger>    $t",
	"<worn on finger>    $t",
	"<worn around neck>  $t",
	"<worn around neck>  $t",
	"<worn on torso>     $t",
	"<worn on head>      $t",
	"<worn on legs>      $t",
	"<worn on feet>      $t",
	"<worn on hands>     $t",
	"<worn on arms>      $t",
	"<worn as shield>    $t",
	"<worn about body>   $t",
	"<worn about waist>  $t",
	"<worn about wrist>  $t",
	"<worn about wrist>  $t",
	"<wielded>           $t",
	"<held>              $t",
	"<floating nearby>   $t",
	"<scratched tattoo>  $t",
	"<dual wielded>      $t",
	"<clan mark>         $t",
	"<stuck in>          $t",
};

void show_obj_to_char(CHAR_DATA *ch, OBJ_DATA *obj, flag32_t wear_loc)
{
	bool is_seen = can_see_obj(ch, obj);
	act(wear_loc_names[wear_loc], ch,
	    is_seen ? format_obj_to_char(obj, ch, TRUE) : "something",
	    NULL, TO_CHAR | (is_seen ? 0 : ACT_TRANS));
}

void show_char_to_char_1(CHAR_DATA *victim, CHAR_DATA *ch)
{
	OBJ_DATA *obj;
	int i;
	int percent;
	bool found;
	char *msg;
	const char *desc;
	CHAR_DATA *doppel = victim;
	CHAR_DATA *mirror = victim;
	char buf[MAX_STRING_LENGTH];

	if (is_affected(victim, gsn_doppelganger)) {
		if (IS_NPC(ch) || !IS_SET(ch->conf_flags, PLR_CONF_HOLYLIGHT)) {
			doppel = victim->doppel;
			if (is_affected(victim, gsn_mirror))
				mirror = victim->doppel;
		}
	}

	if (can_see(victim, ch)) {
		if (ch == victim)
			act("$n looks at $mself.",
			    ch, NULL, NULL, TO_ROOM);
		else {
			act_puts("$n looks at you.",
				 ch, NULL, victim, TO_VICT, POS_RESTING);
			act("$n looks at $N.",
			    ch, NULL, victim, TO_NOTVICT);
		}
	}

	if (IS_NPC(doppel))
		desc = mlstr_cval(doppel->description, ch);
	else
		desc = mlstr_mval(doppel->description);

	if (!IS_NULLSTR(desc))
		char_puts(desc, ch);
	else
		act_puts("You see nothing special about $m.",
			 victim, NULL, ch, TO_VICT, POS_DEAD);

	if (MOUNTED(victim))
		act_puts("$N is riding $i.",
			 ch, MOUNTED(victim), victim, TO_CHAR, POS_DEAD);
	if (RIDDEN(victim))
		act_puts("$N is being ridden by $i.",
			 ch, RIDDEN(victim), victim, TO_CHAR, POS_DEAD);

	if (victim->max_hit > 0)
		percent = (100 * victim->hit) / victim->max_hit;
	else
		percent = -1;

	if (percent >= 100)
		msg = "{Cis in perfect health{x.";
	else if (percent >= 90)
		msg = "{bhas a few scratches{x.";
	else if (percent >= 75)
		msg = "{Bhas some small but disgusting cuts{x.";
	else if (percent >= 50)
		msg = "{Gis covered with bleeding wounds{x.";
	else if (percent >= 30)
		msg = "{Yis gushing blood{x.";
	else if (percent >= 15)
		msg = "{Mis writhing in agony{x.";
	else if (percent >= 0)
		msg = "{Ris convulsing on the ground{x.";
	else
		msg = "{Ris nearly dead{x.";

	/* vampire ... */
	if (percent < 90 && HAS_SKILL(ch, gsn_vampire))
		gain_condition(ch, COND_BLOODLUST, -1);

	if (!IS_IMMORTAL(doppel)) {
		char_printf(ch, "(%s) ", capitalize (race_name(doppel->race)));
		char_printf(ch, "(%s) ", capitalize (flag_string(sex_table, doppel->sex)));
	}

	strnzcpy(buf, sizeof(buf), fix_short(PERS(victim, ch)));
	buf[0] = UPPER(buf[0]);
	char_printf(ch, "%s%s%s %s\n",
		    IS_IMMORTAL(victim) ? "{W" : str_empty,
		    buf,
		    IS_IMMORTAL(victim) ? "{x" : str_empty,
		    GETMSG(msg, ch->lang));

	found = FALSE;
	for (i = 0; show_order[i] != -1; i++)
		if ((obj = get_eq_char(mirror, show_order[i]))
		&&  can_see_obj(ch, obj)) {
			if (!found) {
				char_puts("\n", ch);
				act("$N is using:", ch, NULL, victim, TO_CHAR);
				found = TRUE;
			}

			show_obj_to_char(ch, obj, show_order[i]);
		}

	for (obj = mirror->carrying; obj; obj = obj->next_content)
		if (obj->wear_loc == WEAR_STUCK_IN
		&&  can_see_obj(ch, obj)) {
			if (!found) {
				char_puts("\n", ch);
				act("$N is using:", ch, NULL, victim, TO_CHAR);
				found = TRUE;
			}

			show_obj_to_char(ch, obj, WEAR_STUCK_IN);
		}

	if (!IS_NPC(victim)) {
		if (victim->pcdata->medals != NULL)
			act("   $N owns {Ymedals{x.", ch, NULL, victim, TO_CHAR);
		if (victim->pcdata->awards != NULL)
			act("   $N earned {Ckudos{x.", ch, NULL, victim, TO_CHAR);
	}

	if (victim != ch
	&& (!IS_IMMORTAL(victim) 
		|| (IS_IMMORTAL(ch) && ch->level >= victim->level))
	&&  (!IS_IMMORTAL(victim) || IS_IMMORTAL(ch))
	&&  !IS_NPC(ch)
	&&  number_percent() < get_skill(ch, gsn_peek)) {
		char_puts("\nYou peek at the inventory:\n", ch);
		check_improve(ch, gsn_peek, TRUE, 4);
		show_list_to_char(mirror->carrying, ch, TRUE, TRUE);
	}
}

void show_char_to_char(CHAR_DATA *list, CHAR_DATA *ch)
{
	CHAR_DATA *rch;
	int life_count = 0;
	int evil_count = 0;

	for (rch = list; rch; rch = rch->next_in_room) {
		if (rch == ch
		||  (!IS_TRUSTED(ch, rch->incog_level) &&
		     ch->in_room != rch->in_room))
			continue;
			
		if (!IS_TRUSTED(ch, rch->invis_level)) {
			AREA_DATA *pArea;

			if (!IS_NPC(rch))
				continue;

			pArea = area_vnum_lookup(rch->pIndexData->vnum);
			if (pArea == NULL
			||  !IS_BUILDER(ch, pArea))
				continue;
		}

		if (can_see(ch, rch))
			show_char_to_char_0(rch, ch);
		else {
			if (room_is_dark(ch) && IS_AFFECTED(rch, AFF_INFRARED))
				char_puts("You see {rglowing red eyes{x watching YOU!\n", ch);
			life_count++;
	                        if(IS_EVIL(rch))
                                evil_count++;
		}
	}

	if (list && list->in_room == ch->in_room
	&&  life_count
	&&  IS_AFFECTED(ch, AFF_DETECT_LIFE))
		act_puts("You feel $j more life $qj{forms} in the room.",
			 ch, (const void*) life_count, NULL,
			 TO_CHAR, POS_DEAD);
        if(list && list->in_room == ch->in_room && evil_count
                && is_affected(ch, gsn_pure_sight))
                        act_puts("You sense $j unseen {Revil{x $qj{forms} in the room.",
                                ch, (const void*) evil_count, NULL,
                                TO_CHAR, POS_DEAD);
}

void do_clear(CHAR_DATA *ch, const char *argument)
{
	if (!IS_NPC(ch))
		char_puts("\033[0;0H\033[2J", ch);
}

/* changes your scroll */
DO_FUN(do_scroll)
{
	char arg[MAX_INPUT_LENGTH];
	int lines;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_printf(ch, "You currently display %d lines per "
				"page.\n", ch->lines + 2);
		return;
	}

	if (!is_number(arg)) {
		char_puts("You must provide a number.\n",ch);
		return;
	}

	lines = atoi(arg);
	if (lines < SCROLL_MIN || lines > SCROLL_MAX) {
		char_printf(ch, "Valid scroll range is %d..%d.\n",
			    SCROLL_MIN, SCROLL_MAX);
		return;
	}

	char_printf(ch, "Scroll set to %d lines.\n", lines);
	ch->lines = lines - 2;
}

/* RT does socials */
void do_socials(CHAR_DATA *ch, const char *argument)
{
	do_alist(ch, "social");
}

/* RT Commands to replace news, motd, imotd, etc from ROM */
void do_motd(CHAR_DATA *ch, const char *argument)
{
	do_help_border(ch, "motd", FALSE);
}

void do_imotd(CHAR_DATA *ch, const char *argument)
{
	do_help_border(ch, "imotd", FALSE);
}

void do_rules(CHAR_DATA *ch, const char *argument)
{
	do_help(ch, "rules");
}

void do_story(CHAR_DATA *ch, const char *argument)
{
	do_help(ch, "story");
}

void do_wizlist(CHAR_DATA *ch, const char *argument)
{
	do_help_border(ch, "wizlist", FALSE);
}

/* RT this following section holds all the auto commands from ROM, as well as
   replacements for config */
#define do_print_sw(ch, swname, sw) \
		char_printf(ch, "%-16s %s\n", swname, sw ? "ON" : "OFF");

void do_autolist(CHAR_DATA *ch, const char *argument)
{
	/* lists most player flags */
	if (IS_NPC(ch))
		return;

	char_puts("action         status\n",ch);
	char_puts("---------------------\n",ch);
	do_print_sw(ch, "autoassist", IS_SET(ch->conf_flags, PLR_CONF_AUTOASSIST));
	do_print_sw(ch, "autoexit", IS_SET(ch->conf_flags, PLR_CONF_AUTOEXIT));
	do_print_sw(ch, "autogold", IS_SET(ch->conf_flags, PLR_CONF_AUTOGOLD));
	do_print_sw(ch, "autolook", IS_SET(ch->conf_flags, PLR_CONF_AUTOLOOK));
	do_print_sw(ch, "autoloot", IS_SET(ch->conf_flags, PLR_CONF_AUTOLOOT));
	do_print_sw(ch, "autosac", IS_SET(ch->conf_flags, PLR_CONF_AUTOSAC));
	do_print_sw(ch, "autosplit", IS_SET(ch->conf_flags, PLR_CONF_AUTOSPLIT));

	if (get_skill(ch, gsn_enslave) > 0)
		do_print_sw(ch, "autoenslave", 
			IS_SET(ch->conf_flags, PLR_CONF_AUTO_CLAN_SKILL));

	if (IS_SET(ch->conf_flags, PLR_CONF_NOSUMMON))
		char_puts("You can only be summoned players within "
			     "your PK range.\n",ch);
	else
		char_puts("You can be summoned by anyone.\n",ch);

	if (IS_SET(ch->conf_flags, PLR_CONF_NOFOLLOW))
		char_puts("You do not welcome followers.\n",ch);
	else
		char_puts("You accept followers.\n",ch);
}

void do_autoenslave(CHAR_DATA *ch, const char *argument)
{
	if (IS_NPC(ch)) {
		char_puts("Huh?\n", ch);
		return;
	}

	if (get_skill(ch, gsn_enslave) <= 0) {
		char_puts("Huh?\n", ch);
		return;
	}

	TOGGLE_BIT(ch->conf_flags, PLR_CONF_AUTO_CLAN_SKILL);
	if (IS_SET(ch->conf_flags, PLR_CONF_AUTO_CLAN_SKILL))
		char_puts("You will now automatically enslave your victims.\n",ch);
	else
		char_puts("You will no longer automatically enslave your victims.\n",ch);
}


void do_autoassist(CHAR_DATA *ch, const char *argument)
{
	if (IS_NPC(ch)) {
		char_puts("Huh?\n", ch);
		return;
	}

	TOGGLE_BIT(ch->conf_flags, PLR_CONF_AUTOASSIST);
	if (IS_SET(ch->conf_flags, PLR_CONF_AUTOASSIST))
		char_puts("You will now assist when needed.\n",ch);
	else
		char_puts("Autoassist removed.\n",ch);
}

void do_autoexit(CHAR_DATA *ch, const char *argument)
{
	if (IS_NPC(ch)) {
		char_puts("Huh?\n", ch);
		return;
	}

	TOGGLE_BIT(ch->conf_flags, PLR_CONF_AUTOEXIT);
	if (IS_SET(ch->conf_flags, PLR_CONF_AUTOEXIT))
		char_puts("Exits will now be displayed.\n",ch);
	else 
		char_puts("Exits will no longer be displayed.\n",ch);
}

void do_autogold(CHAR_DATA *ch, const char *argument)
{
	if (IS_NPC(ch)) {
		char_puts("Huh?\n", ch);
		return;
	}

	TOGGLE_BIT(ch->conf_flags, PLR_CONF_AUTOGOLD);
	if (IS_SET(ch->conf_flags, PLR_CONF_AUTOGOLD))
		char_puts("Automatic gold looting set.\n",ch);
	else 
		char_puts("Autogold removed.\n",ch);
}

DO_FUN(do_autolook)
{
	if (IS_NPC(ch)) {
		char_puts("Huh?\n", ch);
		return;
	}

	TOGGLE_BIT(ch->conf_flags, PLR_CONF_AUTOLOOK);
	if (IS_SET(ch->conf_flags, PLR_CONF_AUTOLOOK))
		char_puts("Automatic corpse examination set.\n", ch);
	else
		char_puts("Autolooking removed.\n", ch);
}

void do_autoloot(CHAR_DATA *ch, const char *argument)
{
	if (IS_NPC(ch)) {
		char_puts("Huh?\n", ch);
		return;
	}

	TOGGLE_BIT(ch->conf_flags, PLR_CONF_AUTOLOOT);
	if (IS_SET(ch->conf_flags, PLR_CONF_AUTOLOOT))
		char_puts("Automatic corpse looting set.\n", ch);
	else
		char_puts("Autolooting removed.\n", ch);
}

void do_autosac(CHAR_DATA *ch, const char *argument)
{
	if (IS_NPC(ch)) {
		char_puts("Huh?\n", ch);
		return;
	}

	TOGGLE_BIT(ch->conf_flags, PLR_CONF_AUTOSAC);
	if (IS_SET(ch->conf_flags, PLR_CONF_AUTOSAC))
		char_puts("Automatic corpse sacrificing set.\n",ch);
	else
		char_puts("Autosacrificing removed.\n",ch);
}

void do_autosplit(CHAR_DATA *ch, const char *argument)
{
	if (IS_NPC(ch)) {
		char_puts("Huh?\n", ch);
		return;
	}

	TOGGLE_BIT(ch->conf_flags, PLR_CONF_AUTOSPLIT);
	if (IS_SET(ch->conf_flags, PLR_CONF_AUTOSPLIT))
		char_puts("Automatic gold splitting set.\n",ch);
	else
		char_puts("Autosplitting removed.\n",ch);
}

void do_prompt(CHAR_DATA *ch, const char *argument)
{
	const char *prompt;

	if (argument[0] == '\0'
	||  !str_prefix(argument, "show")) {
		char_printf(ch, "Current prompt is '%s'.\n", ch->prompt);
		return;
	}

	if (!str_cmp(argument, "all") || !str_cmp(argument, "default"))
		prompt = str_dup(DEFAULT_PROMPT);
	else
		prompt = str_printf("%s ", argument);

	free_string(ch->prompt);
	ch->prompt = prompt;
	char_printf(ch, "Prompt set to '%s'.\n", ch->prompt);
}

void do_nofollow(CHAR_DATA *ch, const char *argument)
{
	if (IS_NPC(ch)) {
		char_puts("Huh?\n", ch);
		return;
	}

	TOGGLE_BIT(ch->conf_flags, PLR_CONF_NOFOLLOW);
	if (IS_SET(ch->conf_flags,PLR_CONF_NOFOLLOW)) {
		char_puts("You no longer accept followers.\n", ch);
		die_follower(ch);
	}
	else
		char_puts("You now accept followers.\n", ch);
}

void do_nosummon(CHAR_DATA *ch, const char *argument)
{
	if (IS_NPC(ch)) {
		TOGGLE_BIT(ch->imm_flags, IMM_SUMMON);
		if (IS_SET(ch->imm_flags, IMM_SUMMON))
			char_puts("You are now immune to summoning.\n", ch);
		else
			char_puts("You are no longer immune "
				  "to summoning.\n", ch);
	}
	else {
		TOGGLE_BIT(ch->conf_flags, PLR_CONF_NOSUMMON);
		if (IS_SET(ch->conf_flags,PLR_CONF_NOSUMMON))
			char_puts("You may only be summoned by players "
				  "within your PK range.\n", ch);
		else 
			char_puts("You may now be summoned by anyone.\n", ch);
	}
}

void do_look_in(CHAR_DATA* ch, const char *argument)
{
	OBJ_DATA *obj;

	if ((obj = get_obj_here(ch, argument)) == NULL) {
		char_puts("You don't see that here.\n", ch);
		return;
	}

	switch (obj->pIndexData->item_type) {
	default:
		char_puts("That is not a container.\n", ch);
		break;

	case ITEM_DRINK_CON:
		if (obj->value[ITEM_DRINK_REMAINING] <= 0) {
			char_puts("It is empty.\n", ch);
			break;
		}

		act_puts("It's $tfilled with a $T liquid.",
			 ch,
			 obj->value[ITEM_DRINK_REMAINING] < obj->value[ITEM_DRINK_TOTAL] / 4 ?
				"less than half-" :
			 obj->value[ITEM_DRINK_REMAINING] < 3 * obj->value[ITEM_DRINK_TOTAL] / 4 ?
			 	"about half-" :
			 	"more than half-",
			 liq_table[obj->value[ITEM_DRINK_TYPE]].liq_color,
			 TO_CHAR | ACT_TRANS, POS_DEAD);
		break;

	case ITEM_CONTAINER:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
		if (IS_SET(obj->value[ITEM_CONTAINER_FLAGS], CONT_CLOSED) 
		&& (!ch->clan ||
		clan_lookup(ch->clan)->altar_ptr != obj)) {
			char_puts("It is closed.\n", ch);
			break;
		}

		act_puts("$p holds:", ch, obj, NULL, TO_CHAR, POS_DEAD);
		show_list_to_char(obj->contains, ch, TRUE, TRUE);
		break;
	}
}

void do_look(CHAR_DATA *ch, const char *argument)
{
	char arg1 [MAX_INPUT_LENGTH];
	char arg2 [MAX_INPUT_LENGTH];
	char arg3 [MAX_INPUT_LENGTH];
	EXIT_DATA *pexit;
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	ED_DATA *ed;
	int door;
	int number,count;

	if (ch->desc == NULL)
		return;

	if (ch->position < POS_SLEEPING) {
		char_puts("You can't see anything but stars!\n", ch);
		return;
	}

	if (ch->position == POS_SLEEPING) {
		char_puts("You can't see anything, you're sleeping!\n", ch);
		return;
	}

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
	number = number_argument(arg1, arg3, sizeof(arg3));
	count = 0;

	if (arg1[0] == '\0' || !str_cmp(arg1, "auto")) {

		/* 'look' or 'look auto' */

		if (!room_is_dark(ch) && check_blind_raw(ch)) {
			const char *name;
			const char *engname;



			name = mlstr_cval(ch->in_room->name, ch);
			engname = mlstr_mval(ch->in_room->name);
			char_printf(ch, "{W%s", name);
			if (ch->lang && name != engname)
				char_printf(ch, " (%s){x", engname);
			else
				char_puts("{x", ch);

			if (IS_IMMORTAL(ch)
			||  IS_BUILDER(ch, ch->in_room->area))
				char_printf(ch, " {D[{W%d{D]{x",ch->in_room->vnum);

			if (IS_BUILDER(ch, ch->in_room->area)) {
				char_printf(ch, " [");
				switch (ch->in_room->sector_type) {
				case SECT_INSIDE:
					char_printf(ch, "{DI{x");
					break;
				case SECT_CITY:
					char_printf(ch, "{YC{x");
					break;
				case SECT_FIELD:
					char_printf(ch, "{GG{x");
					break;
				case SECT_FOREST:
					char_printf(ch, "{gF{x");
					break;
				case SECT_HILLS:
					char_printf(ch, "{yH{x");
					break;
				case SECT_MOUNTAIN:
					char_printf(ch, "{yM{x");
					break;
				case SECT_WATER_SWIM:
					char_printf(ch, "{bW{x");
					break;
				case SECT_WATER_NOSWIM:
					char_printf(ch, "{bw{x");
					break;
				case SECT_UNUSED:
					char_printf(ch, "-");
					break;
				case SECT_AIR:
					char_printf(ch, "{CA{x");
					break;
				case SECT_DESERT:
					char_printf(ch, "{YD{x");
					break;
				case SECT_ARCTIC:
					char_printf(ch, "{WS{x");
					break;
				case SECT_ROAD:
					char_printf(ch, "{DR{x");
					break;

				default:
					char_printf(ch, "?");
					break;
				}
				char_printf(ch, "/");

				if (IS_SET(ch->in_room->room_flags, ROOM_FFA))
					char_printf(ch, "{rF");
				if (IS_SET(ch->in_room->room_flags, ROOM_BATTLE_ARENA))
					char_printf(ch, "{YA");
				if (IS_SET(ch->in_room->room_flags, ROOM_PEACE))
					char_printf(ch, "{BP");
				if (IS_SET(ch->in_room->room_flags, ROOM_SAFE))
					char_printf(ch, "{MS");

				if (IS_SET(ch->in_room->room_flags, ROOM_DARK))
					char_printf(ch, "{DD");
				if (IS_SET(ch->in_room->room_flags, ROOM_INDOORS))
					char_printf(ch, "{wI");

				if (IS_SET(ch->in_room->room_flags, ROOM_NORECALL))
					char_printf(ch, "{rr");
				if (IS_SET(ch->in_room->room_flags, ROOM_NOSUMMON))
					char_printf(ch, "{rs");
				if (IS_SET(ch->in_room->room_flags, ROOM_NOMAGIC))
					char_printf(ch, "{rm");
				if (IS_SET(ch->in_room->room_flags, ROOM_NORANDOM))
					char_printf(ch, "{Mr");

				if (IS_SET(ch->in_room->room_flags, ROOM_LAW))
					char_printf(ch, "{YL");
				if (IS_SET(ch->in_room->room_flags, ROOM_NOWHERE))
					char_printf(ch, "{DW");

				if (IS_SET(ch->in_room->room_flags, ROOM_BANK))
					char_printf(ch, "{gB");
				if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP))
					char_printf(ch, "{gP");
				if (IS_SET(ch->in_room->room_flags, ROOM_GUILD))
					char_printf(ch, "{gG");
				if (IS_SET(ch->in_room->room_flags, ROOM_REGISTRY))
					char_printf(ch, "{gR");

				if (IS_SET(ch->in_room->room_flags, ROOM_IMP_ONLY))
					char_printf(ch, "{bi");
				if (IS_SET(ch->in_room->room_flags, ROOM_GODS_ONLY))
					char_printf(ch, "{bg");
				if (IS_SET(ch->in_room->room_flags, ROOM_HEROES_ONLY))
					char_printf(ch, "{bh");
				if (IS_SET(ch->in_room->room_flags, ROOM_NOMOB))
					char_printf(ch, "{bm");
				if (IS_SET(ch->in_room->room_flags, ROOM_NEWBIES_ONLY))
					char_printf(ch, "{bn");
				if (IS_SET(ch->in_room->room_flags, ROOM_SOLITARY))
					char_printf(ch, "{bs");
				if (IS_SET(ch->in_room->room_flags, ROOM_PRIVATE))
					char_printf(ch, "{bp");

				char_printf(ch, "{x]");

			}
			else if (IS_SET(ch->in_room->room_flags, ROOM_BATTLE_ARENA)
			|| IS_SET(ch->in_room->room_flags, ROOM_SAFE)
			|| IS_SET(ch->in_room->room_flags, ROOM_PEACE)
			|| IS_SET(ch->in_room->room_flags, ROOM_FFA)) {
				char_printf(ch, " [");
				if (IS_SET(ch->in_room->room_flags, ROOM_FFA))
					char_printf(ch, "{rF");
				if (IS_SET(ch->in_room->room_flags, ROOM_BATTLE_ARENA))
					char_printf(ch, "{YA");
				if (IS_SET(ch->in_room->room_flags, ROOM_SAFE))
					char_printf(ch, "{MS");
				if (IS_SET(ch->in_room->room_flags, ROOM_PEACE))
					char_printf(ch, "{BP");

				char_printf(ch, "{x]");
			}
			char_printf(ch, "\n");


			if (arg1[0] == '\0'
			||  (!IS_NPC(ch) && !IS_SET(ch->comm, COMM_BRIEF)))
				char_printf(ch, "  %s",
					    mlstr_cval(ch->in_room->description, ch));

			if (!IS_NPC(ch) && IS_SET(ch->conf_flags, PLR_CONF_AUTOEXIT)) {
				char_puts("\n", ch);
				do_exits(ch, "auto");
			}
		}
		else 
			char_puts("It is pitch black...\n", ch);

		show_list_to_char(ch->in_room->contents, ch, FALSE, FALSE);
		show_char_to_char(ch->in_room->people, ch);
		return;
	}

	if (!check_blind(ch))
		return;

	if (!str_cmp(arg1, "i")
	||  !str_cmp(arg1, "in")
	||  !str_cmp(arg1,"on")) {
		/* 'look in' */
		if (arg2[0] == '\0') {
			char_puts("Look in what?\n", ch);
			return;
		}

		do_look_in(ch, arg2);
		return;
	}

	if ((victim = get_char_room(ch, arg1)) != NULL) {
		show_char_to_char_1(victim, ch);

		/* Love potion */
		if (is_affected(ch, gsn_love_potion) && (victim != ch)) {
			AFFECT_DATA af;

			affect_strip(ch, gsn_love_potion);

			if (ch->master)
				stop_follower(ch);
			add_follower(ch, victim);
			ch->leader = victim;

			af.where = TO_AFFECTS;
			af.type = gsn_charm_person;
			af.level = ch->level;
			af.duration =  number_fuzzy(victim->level / 4);
			af.bitvector = AFF_CHARM;
			af.modifier = 0;
			af.location = 0;
			affect_to_char(ch, &af);

			act("Isn't $n just so nice?",
			    victim, NULL, ch, TO_VICT);
			act("$N looks at you with adoring eyes.",
			    victim, NULL, ch, TO_CHAR);
			act("$N looks at $n with adoring eyes.",
			    victim, NULL, ch, TO_NOTVICT);
		}

		return;
	}

	/*object carried on the character*/
	for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {
		if (can_see_obj(ch, obj)) {
			/* player can see object */
			ed = ed_lookup(arg3, obj->ed);
			if (ed != NULL) {
				if (++count == number) {
					char_mlputs(ed->description, ch);
					return;
				}
				else
					continue;
			}

			ed = ed_lookup(arg3, obj->pIndexData->ed);

			if (ed != NULL) {
				if (++count == number) {
					char_mlputs(ed->description, ch);
					if (obj->writtenDesc != NULL)
						char_puts("There is something written on it.\n",ch);
					return;
				}
				else
					continue;
			}

			if (is_name(arg3, obj->name))
				if (++count == number) {

					if (obj->writtenDesc != NULL)
						char_puts("There is something written on it.\n",ch);
					else
						act_puts("You see nothing special about $p.",
							 ch, obj, NULL, TO_CHAR, POS_DEAD);
					return;
				}
		}
	}

	/*objects in the room*/
	for (obj = ch->in_room->contents;
	     obj != NULL; obj = obj->next_content) {
		if (can_see_obj(ch, obj)) {
			ed = ed_lookup(arg3, obj->ed);
			if (ed != NULL)
				if (++count == number) {
					char_mlputs(ed->description, ch);
					return;
				}

			ed = ed_lookup(arg3, obj->pIndexData->ed);
			if (ed != NULL)
				if (++count == number) {
					char_mlputs(ed->description, ch);
					if (obj->writtenDesc != NULL)
						char_puts("There is something written on it.\n",ch);
					return;
				}
		}

		if (is_name(arg3, obj->name))
			if (++count == number) {
				char_puts(format_descr(obj->description, ch),
					  ch);
				char_puts("\n", ch);
				if (obj->writtenDesc != NULL) {
					act_puts("$p reads as follows:", ch, obj, NULL,
						TO_CHAR, POS_DEAD);
					char_mlputs(obj->writtenDesc, ch);
				}
				return;
			}
	}

	ed = ed_lookup(arg3, ch->in_room->ed);
	if (ed != NULL) {
		if (++count == number) {
			char_mlputs(ed->description, ch);
			return;
		}
	}

	if (count > 0 && count != number) {
		if (count == 1)
			act_puts("You only see one $t here.",
				 ch, arg3, NULL, TO_CHAR, POS_DEAD);
		else
			act_puts("You only see $j of those here.",
				 ch, (const void*) count, NULL,
				 TO_CHAR, POS_DEAD);
		return;
	}

		   if (!str_cmp(arg1, "n") || !str_cmp(arg1, "north")) door = 0;
	else if (!str_cmp(arg1, "e") || !str_cmp(arg1, "east")) door = 1;
	else if (!str_cmp(arg1, "s") || !str_cmp(arg1, "south")) door = 2;
	else if (!str_cmp(arg1, "w") || !str_cmp(arg1, "west")) door = 3;
	else if (!str_cmp(arg1, "u") || !str_cmp(arg1, "up" )) door = 4;
	else if (!str_cmp(arg1, "d") || !str_cmp(arg1, "down")) door = 5;
	else {
		char_puts("You don't see that here.\n", ch);
		return;
	}

	/* 'look direction' */
	if ((pexit = ch->in_room->exit[door]) == NULL) {
		char_puts("Nothing special there.\n", ch);
		return;
	}

	if (!IS_NULLSTR(mlstr_mval(pexit->description)))
		char_mlputs(pexit->description, ch);
	else
		char_puts("Nothing special there.\n", ch);

	if (pexit->keyword    != NULL
	&&  pexit->keyword[0] != '\0'
	&&  pexit->keyword[0] != ' ') {
		if (IS_SET(pexit->exit_info, EX_CLOSED)) {
			act_puts("The $d is closed.",
				 ch, NULL, pexit->keyword, TO_CHAR, POS_DEAD);
		}
		else if (IS_SET(pexit->exit_info, EX_ISDOOR))
			act_puts("The $d is open.",
				 ch, NULL, pexit->keyword, TO_CHAR, POS_DEAD);
	}
}

void do_examine(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;

	one_argument(argument, arg, sizeof(arg));

	if (ch->desc == NULL)
		return;

	if (ch->position < POS_SLEEPING) {
		char_puts("You can't see anything but stars!\n", ch);
		return;
	}

	if (ch->position == POS_SLEEPING) {
		char_puts("You can't see anything, you're sleeping!\n", ch);
		return;
	}

	if (!check_blind(ch))
		return;

	if (arg[0] == '\0') {
		char_puts("Examine what?\n", ch);
		return;
	}

	if ((obj = get_obj_here(ch, arg)) == NULL) {
		do_look(ch, arg);
		return;
	}

	switch (obj->pIndexData->item_type) {
	case ITEM_MONEY: {
		const char *msg;

		if (obj->value[ITEM_MONEY_SILVER] == 0) {
			if (obj->value[ITEM_MONEY_GOLD] == 0)
				msg = "Odd...there are no coins in the pile.";
			else if (obj->value[ITEM_MONEY_GOLD] == 1)
				msg = "Wow. One gold coin.";
			else
				msg = "There are $J $qJ{gold coins} in this pile.";
		}
		else if (obj->value[ITEM_MONEY_GOLD] == 0) {
			if (obj->value[ITEM_MONEY_SILVER] == 1)
				msg = "Wow. One silver coin.";
			else
				msg = "There are $j $qj{silver coins} in the pile.";
		}
		else {
			msg = "There are $J gold and $j $qj{silver coins} in the pile."; 
		}
		act_puts3(msg, ch,
			  (const void*) obj->value[ITEM_MONEY_SILVER], NULL,
			  (const void*) obj->value[ITEM_MONEY_GOLD],
			  TO_CHAR, POS_DEAD);
		break;
	}
        case ITEM_PARCHMENT:
	case ITEM_AWARD:
	case ITEM_MEDAL:
		if (obj->writtenDesc != NULL) {
			act_puts("$p reads as follows:", 
				ch, obj, NULL, TO_CHAR, POS_DEAD);
			char_mlputs(obj->writtenDesc, ch);
		}
		else
			act_puts("There is nothing written on $p.", 
				ch, obj, NULL, TO_CHAR, POS_DEAD);
                break;

	case ITEM_DRINK_CON:
	case ITEM_CONTAINER:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
		do_look_in(ch, argument);
		break;

	default:
		do_look(ch, arg);
	}
}

/*
 * Thanks to Zrin for auto-exit part.
 */
void do_exits(CHAR_DATA *ch, const char *argument)
{
	EXIT_DATA *pexit;
	bool found;
	bool fAuto;
	int door;

	fAuto  = !str_cmp(argument, "auto");

	if (fAuto)
		char_puts("{C[Exits:", ch);
	else if (IS_IMMORTAL(ch) || IS_BUILDER(ch, ch->in_room->area))
		char_printf(ch, "Obvious exits from room %d:\n",
			    ch->in_room->vnum);
	else
		char_puts("Obvious exits:\n", ch);

	found = FALSE;
	for (door = 0; door < MAX_DIR; door++) {
		if ((pexit = ch->in_room->exit[door]) != NULL
		&&  pexit->to_room.r != NULL
		&&  can_see_room(ch, pexit->to_room.r)
		&&  check_blind_raw(ch)) { 
			bool show_closed = FALSE;

		if (IS_SET(pexit->exit_info, EX_CLOSED)) {
			int chance;

			if (IS_IMMORTAL(ch))
				show_closed = TRUE;
			else if((chance = get_skill(ch, gsn_perception))){
				 if (number_percent() < chance) {
					check_improve(ch, gsn_perception,
							      TRUE, 5);
					show_closed = TRUE;
				}
			}
			if (!show_closed) continue;
		}

			found = TRUE;
			if (fAuto)
				char_printf(ch, " %s%s", dir_name[door],
					    show_closed ? "*" : str_empty);
			else {
				char_printf(ch, "{C%-5s%s{x - %s",
					    capitalize(dir_name[door]),
					    show_closed ? "*" : str_empty,
					    room_dark(pexit->to_room.r) ?
					    GETMSG("Too dark to tell", ch->lang) :
					    mlstr_cval(pexit->to_room.r->name,
							ch));
				if (IS_IMMORTAL(ch)
				||  IS_BUILDER(ch, pexit->to_room.r->area))
					char_printf(ch, " (room %d)",
						    pexit->to_room.r->vnum);
				char_puts("\n", ch);
			}
		}
	}

	if (!found)
		char_puts(fAuto ? " none" : "None.\n", ch);

	if (fAuto)
		char_puts("]{x\n", ch);
}

void do_worth(CHAR_DATA *ch, const char *argument)
{
	char_printf(ch, "You have {Y%d{x gold, {W%d{x silver", ch->gold, ch->silver);
	if (!IS_NPC(ch) && ch->level < LEVEL_HERO)
		char_printf(ch, ", and {W%d{x experience (%d exp to level)",
			    ch->exp, exp_to_level(ch));
	char_puts(".\n", ch);

	if (!IS_NPC(ch)) {
		char_printf(ch, 
			"You have taken {Y%d{x angelic, {W%d{x balanced, and {r%d{x demonic souls.\n",
			ch->pcdata->align_killed[ALIGN_INDEX_GOOD],
			ch->pcdata->align_killed[ALIGN_INDEX_NEUTRAL],
			ch->pcdata->align_killed[ALIGN_INDEX_EVIL]);

		char_printf(ch, "Moral Standing: {%c%s{x\n",
				(align_standing_leaning(ch) > 0) ? 'C':
				(align_standing_leaning(ch) < 0) ? 'r': 'y',
				flag_string(standing_types, align_standing(ch)));
	}
}

char *	const	day_name	[] =
{
	"the Moon", "the Bull", "Deception", "Thunder", "Freedom",
	"the Great Gods", "the Sun"
};

char *	const	month_name	[] =
{
	"Winter", "the Winter Wolf", "the Frost Giant", "the Old Forces",
	"the Grand Struggle", "the Spring", "Nature", "Futility", "the Dragon",
	"the Sun", "the Heat", "the Battle", "the Dark Shades", "the Shadows",
	"the Long Shadows", "the Ancient Darkness", "the Great Evil"
};

void do_time(CHAR_DATA *ch, const char *argument)
{
	extern char str_boot_time[];
	char *suf;
	int day;

	day	= time_info.day + 1;

	     if (day > 4 && day <  20) suf = "th";
	else if (day % 10 ==  1      ) suf = "st";
	else if (day % 10 ==  2      ) suf = "nd";
	else if (day % 10 ==  3      ) suf = "rd";
	else			       suf = "th";

	char_printf(ch,
		    "It is %d o'clock %s, Day of %s, %d%s the Month of %s.\n",
		    (time_info.hour % 12 == 0) ? 12 : time_info.hour %12,
		    time_info.hour >= 12 ? "pm" : "am",
		    day_name[day % 7],
		    day, suf, month_name[time_info.month]);

	if (!IS_SET(ch->in_room->room_flags, ROOM_INDOORS) || IS_IMMORTAL(ch))
		act_puts("It's $T.", ch, NULL,
			(time_info.hour>=5 && time_info.hour<9) ?   "dawn"    :
			(time_info.hour>=9 && time_info.hour<12) ?  "morning" :
			(time_info.hour>=12 && time_info.hour<18) ? "mid-day" :
			(time_info.hour>=18 && time_info.hour<21) ? "evening" :
								    "night",
			TO_CHAR | ACT_TRANS, POS_DEAD);

	if (!IS_IMMORTAL(ch))
		return;

	char_printf(ch, "\nLegends and Lore started up at %s\n"
			"The system time is %s.\n",
			str_boot_time, strtime(time(NULL)));
}

DO_FUN(do_date)
{
	char_printf(ch, "%s\n", strtime(time(NULL)));
}

void do_weather(CHAR_DATA *ch, const char *argument)
{
	static char * const sky_look[4] = {
		"cloudless",
		"cloudy",
		"rainy",
		"lit by flashes of lightning"
	};

	if (!IS_OUTSIDE(ch)) {
		char_puts("You can't see the weather indoors.\n", ch);
		return;
	}

	char_printf(ch, "The sky is %s and %s.\n",
		    sky_look[weather_info.sky],
		    weather_info.change >= 0 ?
		    "a warm southerly breeze blows" :
		    "a cold northern gust blows");
}


void do_help(CHAR_DATA *ch, const char *argument)
{
	do_help_border(ch, argument, TRUE);
}

void do_help_border(CHAR_DATA *ch, const char *argument, bool border)
{
	BUFFER *output;
	output = buf_new(ch->lang);
	help_show(ch, output, argument, border);
	page_to_char(buf_string(output), ch);
	buf_free(output);
}



/* This file goes into act_info.c */

int do_who_raw(CHAR_DATA* ch, CHAR_DATA *wch, BUFFER* output)
{
	clan_t *clan;
	class_t *cl;
	race_t *r;
	char *strtmp = NULL;

	if ((cl = class_lookup(wch->class)) == NULL
	||  (r = race_lookup(wch->race)) == NULL
	||  !r->pcdata)
		return 0;

	/* don't show uglies */
	if (WHO_HIDE_VAMP_UGLY
	&& !IS_IMMORTAL(ch) 
	&& (ch != wch)
	&& is_affected(wch,gsn_vampire))
		return 0;

	if (IS_IMMORTAL(ch))
	{
		if (wch->level < LEVEL_HERO)
		  buf_printf (output, "{B| {C%3d{B ", wch->level);
		else
		{
	                buf_add(output, "{B| {G");
                        switch (wch->level)
			{ 
                        	case IMPLEMENTOR:       buf_add(output, " DM "); break;
              	         	case CREATOR:           buf_add(output, "CRE "); break;
               		        case SUPREME:           buf_add(output, "SUP "); break;
                        	case DEITY:             buf_add(output, "DEI "); break;
                        	case GOD:               buf_add(output, "GOD "); break;
                        	case IMMORTAL:          buf_add(output, "IMM "); break;
                       		case DEMI:              buf_add(output, "DEM "); break;
                        	case ANGEL:             buf_add(output, "ANG "); break;
                        	case AVATAR:            buf_add(output, "AVA "); break;
                        	case HERO:              buf_add(output, "HER "); break;
			}                               
		}
		buf_add(output, "{B|");
	}
	else
	{
		if (wch->level >= LEVEL_HERO)
		{
                        buf_add(output, "{B| {G");
                        switch (wch->level)
			{ 
                        	case IMPLEMENTOR:       buf_add(output, " DM "); break;
                        	case CREATOR:           buf_add(output, "CRE "); break;
                        	case SUPREME:           buf_add(output, "SUP "); break;
                        	case DEITY:             buf_add(output, "DEI "); break;
                        	case GOD:               buf_add(output, "GOD "); break;
                        	case IMMORTAL:          buf_add(output, "IMM "); break;
                        	case DEMI:              buf_add(output, "DEM "); break;
                        	case ANGEL:             buf_add(output, "ANG "); break;
                        	case AVATAR:            buf_add(output, "AVA "); break;
                        	case HERO:              buf_add(output, "HER "); break;
                       }       
                       buf_add(output, "{B|");
                }
		else
                  buf_printf(output, "{B| {w---{B |");
	}     

	if (IS_SET(wch->acct_flags, ACCT_NEWBIE)) {
		buf_add(output, " {W*{mN{ce{mw{cb{mi{ce{W*  ");
		buf_add(output, "{B|{x");
	}

	else if (IS_SET(wch->conf_flags, PLR_CONF_NEWBIE_HELPER)) {
		buf_add(output, "{MN{mewbie {CH{celp");
		buf_add(output, "{B|{x");
	}

	else if ((wch->clan || IS_IMMORTAL(ch) || (ch==wch)) 
	&&  (clan = clan_lookup(wch->clan))
        &&  (!IS_SET(clan->flags, CLAN_HIDDEN) 
	|| (ch && (wch->clan == ch->clan || IS_IMMORTAL(ch))))) {
		switch (wch->clan)
		{
			case 1:	buf_add(output, "{C  Knights  "); break;
			case 2:	buf_add(output, "{W  {WS{wh{Dad{wo{Ww   "); break;
			case 3: buf_add(output, "{m  E{Ml{wi{Wd{wo{MD{mi  "); break;
			case 4: buf_add(output, "{r   S{Rc{ri{Ro{rn   "); break;
			case 5:	buf_add(output, "{y Barbarian "); break;
			case 6:	buf_add(output, "{Y   R{Bu{Yl{Be{Yr   "); break;
			case 7:	buf_add(output, "{M   C{Rh{Ba{Mo{Rs   "); break;
			case 8:	buf_add(output, "{M  {yS{gy{Glv{ga{yn   "); break;
			case 9: buf_add(output, "{M Horde     "); break;
			case 10: buf_add(output, "{W Sentinel  "); break;  
                	default: buf_printf(output, "{w    ---    ");
		}
		buf_add(output, "{B|{x");
	}
        else
                buf_printf(output, "{w    ---   {B |{x");

	if (IS_IMMORTAL(ch)) {
/*Zz*/
		buf_printf(output, "%s{%c%s{B|{x", 
			IS_SET(wch->acct_flags, ACCT_MULTIKILLER)
			&& IS_SET(wch->acct_flags, ACCT_TRUE_LIFER) ? "{Y+" 
			: IS_SET(wch->acct_flags, ACCT_MULTIKILLER) ? "{D+"
			: IS_SET(wch->acct_flags, ACCT_TRUE_LIFER)  ? "{Y-" :" ",
			((wch->sex == 1) ? 'C' : 'R'), cl->who_name);	

		strtmp = &r->pcdata->who_name[1];

		buf_printf(output, "{%c%c{%c%-4s{B|{x ", 
			(wch->ethos == ETHOS_LAWFUL) ? 'Y' 
				: ((wch->ethos == ETHOS_NEUTRAL) ? 'y' : 'D'), 
			r->pcdata->who_name[0], 
			(IS_GOOD(wch)) ? 'C'
				: (IS_EVIL(wch) ? 'r' : 'w'),
			strtmp);	
	}
	else
	{
		if (in_PK(ch, wch) 
		&& !IS_IMMORTAL(wch)) {
			if (IS_SET(wch->acct_flags, ACCT_MULTIKILLER))
				buf_printf(output, " {DPK{B |{x ");
			else
				buf_printf(output, " {RPK{B |{x ");
		}
                else if (!IS_IMMORTAL(wch)
			&& (ch->level >= wch->level -8 && ch->level <= wch->level+8 ))
			buf_printf(output, " {GGR {B|{x ");
		else
			buf_printf(output, " {w-- {B|{x ");
	}

	if (IS_SET(wch->comm, COMM_AFK))
		buf_add(output, "{c[AFK]{x ");

	if (wch->invis_level > 0 || wch->incog_level > 0 ) {
		if (wch->invis_level > 0)
		{
			if (ch->level >= wch->invis_level)
			  buf_printf (output, "{M%d{x%c", 
				wch->invis_level,
				(wch->incog_level > 0) ? '/' : ' ');
		}
		if (wch->incog_level > 0)
		{
			if (ch->level >= wch->incog_level)
			  buf_printf (output, "{m%d{x ", wch->incog_level);
		}
	}

	if (IS_SET(wch->state_flags, STATE_WANTED))
		buf_add(output, "{R(WANTED){x ");

	if (wch->pcdata->enslaver != NULL)
		buf_add(output, "{r[Enslaved]{x ");

	if (IS_IMMORTAL(wch))
		buf_printf(output, "{W%s{x", wch->name);
	else
	{
		if (WHO_SHOW_VAMP_UGLY
		&& is_affected(wch,gsn_vampire)
		&& !IS_IMMORTAL(ch)
		&& (ch!=wch)
		&& !(ch->clan == 6 && IS_SET(wch->state_flags, STATE_WANTED)))
			buf_add(output, "An ugly creature");
		else
			buf_add(output, wch->name);
	}
	if ((ch->class != wch->class) 
	&& (!IS_IMMORTAL(ch)) && (wch->level < LEVEL_HERO) 
	&& ((ch->clan == 0) || (ch->clan != wch->clan)))
		buf_printf(output, " the %s", capitalize (race_name(wch->race)));
	else
	{	
		if (WHO_SHOW_VAMP_UGLY
		&& is_affected(wch,gsn_vampire)&&!IS_IMMORTAL(ch) &&(ch!=wch))
			buf_add(output, "");

		else if (wch->clan != 0 &&
		   (IS_IMMORTAL(ch) || ch->clan == wch->clan) 
		   && ch != wch 
		   && wch->level < LEVEL_HERO)
          		buf_printf(output, " the %s", CLAN_RANK_TITLE(wch));
		else
			buf_add(output, wch->pcdata->title);
	}

	buf_add(output, "{x\n");
	return 1;
}

#define WHO_F_IMM	(A)		/* imm only			*/
#define WHO_F_PK	(B)		/* PK only			*/
#define WHO_F_TATTOO	(C)		/* same tattoo only		*/
#define WHO_F_CLAN	(D)		/* clan only			*/
#define WHO_F_RCLAN	(E)		/* specified clans only		*/
#define WHO_F_RRACE	(F)		/* specified races only		*/
#define WHO_F_RCLASS	(G)		/* specified classes only	*/

DO_FUN(do_who)
{
	BUFFER *output;
	DESCRIPTOR_DATA *d;
	flag32_t flags = 0;
	flag32_t ralign = 0;
	flag32_t rethos = 0;

	int iLevelLower = 0;
	int iLevelUpper = MAX_LEVEL;

	int tattoo_vnum = 0;	/* who tattoo data */
	OBJ_DATA *obj;

	int nNumber;
	int nMatch = 0;
	int count = 0;

	const char *clan_names = str_empty;
	const char *race_names = str_empty;
	const char *class_names = str_empty;
	char *p;

	int avg_align = 0;
	char *title = NULL;
	int someone = 0;

	/*
	 * Parse arguments.
	 */
	nNumber = 0;
	for (;;) {
		int i;
		char arg[MAX_INPUT_LENGTH];

		argument = one_argument(argument, arg, sizeof(arg));
		if (arg[0] == '\0')
			break;

		if (!str_prefix(arg, "immortals")) {
			SET_BIT(flags, WHO_F_IMM);
			continue;
		}

		if (!str_cmp(arg, "pk")) {
			SET_BIT(flags, WHO_F_PK);
			continue;
		}

		if (!str_cmp(arg, "tattoo")) {
			if ((obj = get_eq_char(ch, WEAR_TATTOO)) == NULL) {
				char_puts("You haven't got a tattoo yet!\n", ch);
				goto bail_out;
			}
			SET_BIT(flags, WHO_F_TATTOO);
			tattoo_vnum = obj->pIndexData->vnum;
			continue;
		}

		if (!str_cmp(arg, "clan")) {
			SET_BIT(flags, WHO_F_CLAN);
			continue;
		}

		if ((i = cln_lookup(arg)) > 0) {
			name_add(&clan_names, CLAN(i)->name, NULL, NULL);
			SET_BIT(flags, WHO_F_RCLAN);
			continue;
		}

		if ((i = rn_lookup(arg)) > 0 && RACE(i)->pcdata) {
			name_add(&race_names, RACE(i)->name, NULL, NULL);
			SET_BIT(flags, WHO_F_RRACE);
			continue;
		}

		if (!IS_IMMORTAL(ch))
			continue;

		if ((i = cn_lookup(arg)) >= 0) {
			name_add(&class_names, CLASS(i)->name, NULL, NULL);
			SET_BIT(flags, WHO_F_RCLASS);
			continue;
		}

		if ((p = strchr(arg, '-'))) {
			*p++ = '\0';
			if (arg[0]) {
				if ((i = flag_value(ethos_table, arg)))
					SET_BIT(rethos, i);
				else
					char_printf(ch, "%s: unknown ethos.\n", arg);
			}
			if (*p) {
				if ((i = flag_value(ralign_names, p)))
					SET_BIT(ralign, i);
				else
					char_printf(ch, "%s: unknown align.\n", p);
			}
			continue;
		}

		if (is_number(arg)) {
			switch (++nNumber) {
			case 1:
				iLevelLower = atoi(arg);
				break;
			case 2:
				iLevelUpper = atoi(arg);
				break;
			default:
				char_printf(ch,
					    "%s: explicit argument (skipped)\n",
					    arg);
				break;
			}
			continue;
		}
	}
	/*
	 * Now show matching chars.
	 */
	output = buf_new(ch->lang);
	avg_align = world_alignment();

	if (avg_align > 333)
		title = "{C  Legends {w&{C Lore  ";
	else if (avg_align < -333)
		title = "{r  Legends {w&{r Lore  ";
	else
		title = "{b  Legends {w& {gLore  ";

	if (IS_IMMORTAL(ch)) {
		buf_add(output, "\n{B.----------------------------.{x\n");
		buf_printf(output,   "{B|     %s     {B|{x\n", title);
		buf_printf(output,   "{B|------------%s-----------|{x\n",
		is_affected_world(WAFF_ARENA) ? "{WArena{B" : "-----");
	}
	else {
		buf_add(output, "\n{B.----------------------.{x\n");
		buf_printf(output,   "{B|  %s  {B|{x\n", title);
		buf_add(output, "{B|----------------------|{x\n");
	}
	for (d = descriptor_list; d; d = d->next) {
		CHAR_DATA *wch;

		clan_t *clan;
		race_t *race;
		class_t *class;

		if (d->connected != CON_PLAYING)
			continue;
		count++;

		wch = d->original ? d->original : d->character;
		if (!wch || !can_see(ch, wch)) {
			if (IS_IMMORTAL(wch))
				continue;
			else {
				someone++;
				continue;
			}
		}

		/* if (is_affected(wch, gsn_vampire)
		&&  !IS_IMMORTAL(ch) && ch != wch)
			continue; */

		if (wch->level < iLevelLower || wch->level > iLevelUpper
		||  (IS_SET(flags, WHO_F_IMM) && wch->level < LEVEL_IMMORTAL)
		||  (IS_SET(flags, WHO_F_PK) && !in_PK(ch, wch))
		||  (IS_SET(flags, WHO_F_CLAN) && !wch->clan)
		||  (ralign && ((RALIGN(wch) & ralign) == 0))
		||  (rethos && ((wch->ethos & rethos) == 0)))
			continue;

		if (IS_SET(flags, WHO_F_TATTOO)) {
			if ((obj = get_eq_char(wch, WEAR_TATTOO)) == NULL
			||  tattoo_vnum != obj->pIndexData->vnum)
				continue;
		}

		if (IS_SET(flags, WHO_F_RCLAN)) {
			if (!wch->clan
			||  (clan = clan_lookup(wch->clan)) == NULL
			||  !is_name(clan->name, clan_names)
			|| (ch->clan != wch->clan 
				&& IS_SET(clan->flags, CLAN_HIDDEN)
				&& !IS_IMMORTAL(ch)))
				continue;
		}

		if (IS_SET(flags, WHO_F_RRACE)) {
			if ((race = race_lookup(wch->race)) == NULL
			||  !is_name(race->name, race_names))
				continue;
		}

		if (IS_SET(flags, WHO_F_RCLASS)) {
			if ((class = class_lookup(wch->class)) == NULL
			||  !is_name(class->name, class_names))
				continue;
		}

		nMatch += do_who_raw(ch, wch, output);
	}

	if (IS_IMMORTAL(ch))
		buf_add(output, "{B ~--------------------------~{x\n");
	else
		buf_add(output, "{B ~--------------------~{x\n");

	max_on = UMAX(count, max_on);
	buf_printf(output, "{x\nPlayers found: {C%d{x",
		   nMatch + ((WHO_SHOW_HIDDEN) ? someone : 0));
	if (WHO_SHOW_HIDDEN)
		buf_printf(output, "  ({C%d{x hidden).",
			   someone);

	buf_printf(output, "  Most so far today: {C%d{x.\n",
		   max_on);
	page_to_char(buf_string(output), ch);
	buf_free(output);

bail_out:
	free_string(clan_names);
	free_string(class_names);
	free_string(race_names);
}

/* whois command */
void do_whois(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	BUFFER *output = NULL;
	DESCRIPTOR_DATA *d;

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("You must provide a name.\n", ch);
		return;
	}

	output = buf_new(-1);

        buf_printf(output, "\n{B.----------------------------------.{x\n");
        buf_printf(output, "{B|  {g Players Matching Your Search{B   |{x\n");
        buf_printf(output, "{B|----------------------------------|{x\n");

	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *wch;

		if (d->connected != CON_PLAYING || !can_see(ch,d->character))
				continue;

		if (d->connected != CON_PLAYING
		||  (is_affected(d->character, gsn_vampire) &&
		     !IS_IMMORTAL(ch) && (ch != d->character)))
			continue;

		wch = (d->original != NULL) ? d->original : d->character;

		if (!can_see(ch,wch))
			continue;

		if (!str_prefix(arg,wch->name)) {
			if (output == NULL)
			  output = buf_new(-1);
			
			do_who_raw(ch, wch, output);
		}
	}

	if (output == NULL) {
		char_puts("No one of that name is playing.\n", ch);
		return;
	}

	buf_add(output, "{x");
	page_to_char(buf_string(output), ch);
	buf_free(output);
}

void do_count(CHAR_DATA *ch, const char *argument)
{
	int count;
	DESCRIPTOR_DATA *d;

	count = 0;

	for (d = descriptor_list; d != NULL; d = d->next)
		if (d->connected == CON_PLAYING && can_see(ch, d->character))
			count++;

	max_on = UMAX(count,max_on);

	char_printf(ch, "There are %d characters on, ", count);
	if (max_on == count)
		char_puts("the most so far this reboot", ch);
	else
		char_printf(ch, "the most on this reboot was %d", max_on);
	char_puts(".\n", ch);
}

void do_inventory(CHAR_DATA *ch, const char *argument)
{
	char_puts("You are carrying:\n", ch);
	show_list_to_char(ch->carrying, ch, TRUE, TRUE);
}

void do_medals(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		victim = ch;
	}
	else if ((victim = get_char_room(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (IS_NPC(victim)) {
		char_puts("The souless have no use for medals.\n", ch);
		return;
	}

	if (ch != victim && can_see(victim, ch)) {
		act("You notice $N admiring your medals.",
			victim, NULL, ch, TO_CHAR);
	}

	if (victim == ch) { 
		char_puts("You have the following trophies:\n", ch);
	}
	else {
		act("$N has the following trophies:",
			ch, NULL, victim, TO_CHAR);
	}

	show_list_to_char(victim->pcdata->medals, ch, TRUE, TRUE);
}

void do_kudos(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		victim = ch;
	}
	else if ((victim = get_char_room(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (IS_NPC(victim)) {
		char_puts("The souless never break character.\n", ch);
		return;
	}

	if (ch != victim && can_see(victim, ch)) {
		act("You notice $N admiring your awards.",
			victim, NULL, ch, TO_CHAR);
	}

	if (victim == ch) { 
		char_puts("You've been awarded:\n", ch);
	}
	else {
		act("$N has been awarded:",
			ch, NULL, victim, TO_CHAR);
	}

	show_list_to_char(victim->pcdata->awards, ch, TRUE, TRUE);
}

void do_equipment(CHAR_DATA *ch, const char *argument)
{
	OBJ_DATA *obj;
	int i;
	bool found;

	char_puts("You are using:\n", ch);
	found = FALSE;
	for (i = 0; show_order[i] >= 0; i++) {
		if ((obj = get_eq_char(ch, show_order[i])) == NULL)
			continue;

		show_obj_to_char(ch, obj, show_order[i]);
		found = TRUE;
	}

	for(obj = ch->carrying; obj != NULL; obj = obj->next_content) {
		if (obj->wear_loc != WEAR_STUCK_IN)
			continue;

		show_obj_to_char(ch, obj, WEAR_STUCK_IN);
		found = TRUE;
	}

	if (!found)
		char_puts("Nothing.\n", ch);
}

void do_compare(CHAR_DATA *ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	OBJ_DATA *obj1;
	OBJ_DATA *obj2;
	int value1;
	int value2;
	char *cmsg;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
	if (arg1[0] == '\0') {
		char_puts("Compare what to what?\n", ch);
		return;
	}

	if ((obj1 = get_obj_carry(ch, arg1)) == NULL) {
		char_puts("You do not have that item.\n", ch);
		return;
	}

	if (arg2[0] == '\0') {
		for (obj2 = ch->carrying;
		     obj2 != NULL; obj2 = obj2->next_content)
			if (obj2->wear_loc != WEAR_NONE
			&&  can_see_obj(ch,obj2)
			&&  obj1->pIndexData->item_type == obj2->pIndexData->item_type
			&&  (obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE))
				break;

		if (obj2 == NULL) {
			char_puts("You aren't wearing anything comparable.\n", ch);
			return;
		}
	}
	else if ((obj2 = get_obj_carry(ch,arg2)) == NULL) {
		char_puts("You do not have that item.\n", ch);
		return;
	}

	cmsg		= NULL;
	value1	= 0;
	value2	= 0;

	if (obj1 == obj2)
		cmsg = "You compare $p to itself.  It looks about the same.";
	else if (obj1->pIndexData->item_type != obj2->pIndexData->item_type)
		cmsg = "You can't compare $p and $P.";
	else {
		switch (obj1->pIndexData->item_type) {
		default:
			cmsg = "You can't compare $p and $P.";
			break;

		case ITEM_ARMOR:
			value1 = obj1->value[ITEM_ARMOR_AC_PIERCE]
				+obj1->value[ITEM_ARMOR_AC_BASH]
				+obj1->value[ITEM_ARMOR_AC_SLASH];
			value2 = obj2->value[ITEM_ARMOR_AC_PIERCE]
				+obj2->value[ITEM_ARMOR_AC_BASH]
				+obj2->value[ITEM_ARMOR_AC_SLASH];
			break;

		case ITEM_WEAPON:
			value1 = (1 + obj1->value[ITEM_WEAPON_DICE_SIZE]) 
				* obj1->value[ITEM_WEAPON_DICE_NUM];
			value2 = (1 + obj2->value[ITEM_WEAPON_DICE_SIZE]) 
				* obj2->value[ITEM_WEAPON_DICE_NUM];
			break;
		}
	}

	if (cmsg == NULL) {
		if (value1 == value2)
			cmsg = "$p and $P look about the same.";
		else if (value1  > value2)
			cmsg = "$p looks better than $P.";
		else
			cmsg = "$p looks worse than $P.";
	}

	act(cmsg, ch, obj1, obj2, TO_CHAR);
}

void do_credits(CHAR_DATA *ch, const char *argument)
{
	do_help(ch, "'LNL'");
}

void do_where(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	DESCRIPTOR_DATA *d;
	bool found;
	bool fPKonly = FALSE;

	one_argument(argument, arg, sizeof(arg));

	if (!check_blind(ch))
		return;

	if (room_is_dark(ch)) {
		char_puts("It's too dark to see.\n", ch);
		return;
	}

	if (!str_cmp(arg,"pk"))
		fPKonly = TRUE;

	if (arg[0] == '\0' || fPKonly) {
		char_puts("Players near you:\n", ch);
		found = FALSE;
		for (d = descriptor_list; d; d = d->next) {
			if (d->connected == CON_PLAYING
			&&  (victim = d->character) != NULL
			&&  !IS_NPC(victim)
			&&  (!fPKonly || in_PK(ch, victim))
			&&  victim->in_room != NULL
			&&  victim->in_room->area == ch->in_room->area
			&&  !IS_SET(victim->in_room->room_flags, ROOM_NOWHERE)
			&&  can_see(ch, victim)) {
				CHAR_DATA *doppel;
				found = TRUE;

				if (is_affected(victim, gsn_doppelganger)
				&&  (IS_NPC(ch) || !IS_SET(ch->conf_flags, PLR_CONF_HOLYLIGHT)))
					doppel = victim->doppel;
				else
					doppel = victim;

				char_printf(ch, "%s%-28s %s\n",
					(in_PK(ch, doppel) &&
					!IS_IMMORTAL(ch)) ?
					(IS_SET(victim->acct_flags, ACCT_MULTIKILLER) ?
					"{r[{DPK{r]{x " : "{r[{RPK{r]{x ") : "     ",
					PERS(victim, ch),
					mlstr_mval(victim->in_room->name));
			}
		}
		if (!found)
			char_puts("None.\n", ch);
	}
	else {
		found = FALSE;
		for (victim = char_list; victim; victim = victim->next) {
			if (victim->in_room
			&&  victim->in_room->area == ch->in_room->area
			&&  can_see(ch, victim)
			&&  is_name(arg, victim->name)) {
				found = TRUE;
				char_printf(ch, "%-28s %s\n",
					fix_short(PERS(victim, ch)),
					mlstr_mval(victim->in_room->name));
				break;
			}
		}
		if (!found) {
			act_puts("You didn't find any $T.",
				 ch, NULL, arg, TO_CHAR, POS_DEAD);
		}
	}
}

void do_consider(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	char *cmsg;
	char *align;
	int diff;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Consider killing whom?\n", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("Suicide is against your way.\n", ch);
		return;
	}

	if (!in_PK(ch, victim)) {
		char_puts("Don't even think about it.\n", ch);
		return;
	}

	diff = victim->level - ch->level;

	     if (diff <= -10) cmsg = "You can kill $N naked and weaponless.";
	else if (diff <=  -5) cmsg = "$N is no match for you.";
	else if (diff <=  -2) cmsg = "$N looks like an easy kill.";
	else if (diff <=   1) cmsg = "The perfect match!";
	else if (diff <=   4) cmsg = "$N says '{GDo you feel lucky, punk?{x'.";
	else if (diff <=   9) cmsg = "$N laughs at you mercilessly.";
	else if (diff <=  15) cmsg = "You feel foolish even considering it.";
	else if (diff <=  25) cmsg = "You are but a gnat before $N.";
	else                  cmsg = "Death will thank you for your gift.";

	act(cmsg, ch, NULL, victim, TO_CHAR);
	if (!IS_NPC(victim))
		return;

	if (IS_EVIL(ch) && IS_EVIL(victim))
		align = "$N grins evilly with you.";
	else if (IS_GOOD(victim) && IS_GOOD(ch))
		align = "$N greets you warmly.";
	else if (IS_GOOD(victim) && IS_EVIL(ch))
		align = "$N smiles at you, hoping you will turn from your evil path.";
	else if (IS_EVIL(victim) && IS_GOOD(ch))
		align = "$N grins evilly at you.";
	else if (IS_NEUTRAL(ch) && IS_EVIL(victim))
		align = "$N grins evilly.";
	else if (IS_NEUTRAL(ch) && IS_GOOD(victim))
		align = "$N smiles happily.";
	else if (IS_NEUTRAL(ch) && IS_NEUTRAL(victim))
		align = "$N looks just as disinterested as you.";
	else
		align = "$N looks very disinterested.";

	act(align, ch, NULL, victim, TO_CHAR);
}

int set_title(CHAR_DATA *ch, const char *title)
{
	char buf[MAX_INPUT_LENGTH];
	static char nospace[] = "-.,!?':{";

	buf[0] = '\0';

	if (title) {
		if (cstrlen(title) > MAX_TITLE_LENGTH)
			return FALSE;

		if (strchr(nospace, *cstrfirst(title)) == NULL) {
			buf[0] = ' ';
			buf[1] = '\0';
		}

		strnzcat(buf, sizeof(buf), title);
	}

	free_string(ch->pcdata->title);
	ch->pcdata->title = str_dup(buf);
	return TRUE;
}

/*void do_title(CHAR_DATA *ch, const char *argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->state_flags, STATE_NOTITLE)) {
		char_puts("You can't change your title.\n", ch);
		return;
	}

	if (argument[0] == '\0') {
		char_puts("Change your title to what?\n", ch);
		return;
	}

	if (strstr(argument, "{/")) {
		char_puts("Illegal characters in title.\n", ch);
		return;
	}
		
	if (set_title(ch, argument)) {
		char_puts("Title set.\n", ch);
	else
		char_puts("Title wast too long.\n", ch);
}
*/

void do_description(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_STRING_LENGTH];

	if (IS_NPC(ch)) {
		char_puts("Huh?\n", ch);
		return;
	}

	one_argument(argument, arg, sizeof(arg));

	if (str_cmp(arg, "edit") == 0) {
		string_append(ch, mlstr_convert(&ch->description, -1));
		return;
	}

	char_printf(ch, "Your description is:\n"
			 "%s\n"
			 "Use 'desc edit' to edit your description.\n",
		    mlstr_mval(ch->description));
}

void do_report(CHAR_DATA *ch, const char *argument)
{
	doprintf(do_say, ch, "I have %d/%d hp %d/%d mana %d/%d mv.",
		 ch->hit, ch->max_hit,
		 ch->mana, ch->max_mana,
		 ch->move, ch->max_move);
}

/*
 * 'Wimpy' originally by Dionysos.
 */
void do_wimpy(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	int wimpy;
	class_t *cl;

	if ((cl = class_lookup(ch->class))
	&&  !CAN_FLEE(ch, cl)) {
		char_printf(ch, "You don't deal with wimpies, "
				"or such fearful things.\n");
		if (ch->wimpy)
			ch->wimpy = 0;
		return;
	}

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0')
		wimpy = ch->max_hit / 5;
	else
		wimpy = atoi(arg);

	if (wimpy < 0) {
		char_puts("Your courage exceeds your wisdom.\n", ch);
		return;
	}

	if (wimpy > ch->max_hit/2) {
		char_puts("Such cowardice ill becomes you.\n", ch);
		return;
	}

	ch->wimpy	= wimpy;

	char_printf(ch, "Wimpy set to %d hit points.\n", wimpy);
	return;
}

void do_password(CHAR_DATA *ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char *pwdnew;

	if (IS_NPC(ch))
		return;

	argument = first_arg(argument, arg1, sizeof(arg1), FALSE);
	argument = first_arg(argument, arg2, sizeof(arg2), FALSE);

	if (arg1[0] == '\0' || arg2[0] == '\0') {
		char_puts("Syntax: password <old> <new>.\n", ch);
		return;
	}

	if (strcmp(crypt(arg1, ch->pcdata->pwd), ch->pcdata->pwd)) {
		WAIT_STATE(ch, 10 * PULSE_PER_SECOND);
		char_puts("Wrong password.  Wait 10 seconds.\n", ch);
		return;
	}

	if (strlen(arg2) < 5) {
		char_puts("New password must be at least "
			     "five characters long.\n", ch);
		return;
	}

	/*
	 * No tilde allowed because of player file format.
	 */
	pwdnew = crypt(arg2, ch->name);
	if (strchr(pwdnew, '~') != NULL) {
		char_puts("New password not acceptable, "
			     "try again.\n", ch);
		return;
	}

	free_string(ch->pcdata->pwd);
	ch->pcdata->pwd = str_dup(pwdnew);
	save_char_obj(ch, FALSE);
	char_puts("Ok.\n", ch);
}

void scan_list(ROOM_INDEX_DATA *scan_room, CHAR_DATA *ch, 
		int depth, int door)
{
	CHAR_DATA *rch;

	if (scan_room == NULL) 
		return;

	for (rch = scan_room->people; rch; rch = rch->next_in_room) {
		if (rch == ch || !can_see(ch, rch))
			continue;
		char_printf(ch, "	%s.\n",
			    format_short(rch->short_descr, rch->name, ch));
	}
}

void do_scan2(CHAR_DATA *ch, const char *argument)
{
	EXIT_DATA *pExit;
	int door;

	act("$n looks all around.", ch, NULL, NULL, TO_ROOM);
	if (!check_blind(ch))
		return;

	char_puts("Looking around you see:\n", ch);

	char_puts("{Chere{x:\n", ch);
	scan_list(ch->in_room, ch, 0, -1);
	for (door = 0; door < 6; door++) {
		if ((pExit = ch->in_room->exit[door]) == NULL
		|| !pExit->to_room.r
		|| !can_see_room(ch,pExit->to_room.r))
			continue;
		char_printf(ch, "{C%s{x:\n", dir_name[door]);
		if (IS_SET(pExit->exit_info, EX_CLOSED)) {
			char_puts("	You see closed door.\n", ch);
			continue;
		}
		scan_list(pExit->to_room.r, ch, 1, door);
	}
}

void do_scan(CHAR_DATA *ch, const char *argument)
{
	char dir[MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *in_room;
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pExit;
	int door;
	int range;
	int i;
	CHAR_DATA *person;
	int numpeople;

	one_argument(argument, dir, sizeof(dir));

	if (dir[0] == '\0') {
		do_scan2(ch, str_empty);
		return;
	}

	switch (dir[0]) {
	case 'N':
	case 'n':
		door = 0;
		break;
	case 'E':
	case 'e':
		door = 1;
		break;
	case 'S':
	case 's':
		door = 2;
		break;
	case 'W':
	case 'w':
		door = 3;
		break;
	case 'U':
	case 'u':
		door = 4;
		break;
	case 'D':
	case 'd':
		door = 5;
		break;
	default:
		char_puts("Wrong direction.\n", ch);
		return;
	}

	act("$n scans $t.", ch, dir_name[door], NULL, TO_ROOM | ACT_TRANS);
	if (!check_blind(ch))
		return;

	act_puts("You scan $t.", ch, dir_name[door], NULL, TO_CHAR | ACT_TRANS,
		 POS_DEAD);

	range = 1 + ch->level/10;

	in_room = ch->in_room;
	for (i = 1; i <= range; i++) {
		pExit = in_room->exit[door];
		if (!pExit)
			return;
		to_room = pExit->to_room.r;
		if (!to_room)
			return;

		if (IS_SET(pExit->exit_info,EX_CLOSED)
		&&  can_see_room(ch,pExit->to_room.r)) {
			char_puts("	You see closed door.\n", ch);
			return;
		}
		for (numpeople = 0, person = to_room->people; person != NULL;
		     person = person->next_in_room)
			if (can_see(ch,person)) {
				numpeople++;
				break;
			}

		if (numpeople) {
			char_printf(ch, "***** Range %d *****\n", i);
			show_char_to_char(to_room->people, ch);
			char_puts("\n", ch);
		}
		in_room = to_room;
	}
}

void do_identify(CHAR_DATA *ch, const char *argument)
{
	OBJ_DATA *obj;
	CHAR_DATA *rch;
	int cost = 1;

	if ((obj = get_obj_carry(ch, argument)) == NULL) {
		 char_puts("You are not carrying that.\n", ch);
		 return;
	}

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
		if (IS_NPC(rch) && IS_SET(rch->pIndexData->act, ACT_SAGE))
			break;

	if (!rch) {
		 char_puts("No one here seems to know much "
			      "about that.\n", ch);
		 return;
	}

	cost = (ch->level < 20) ? 1 : ch->level + (ch->level / 10);

	if (IS_OBJ_LIMITED(obj->pIndexData))
		cost *= 5;
	else if (IS_OBJ_RARE(obj->pIndexData))
		cost *= 3;

	if (IS_IMMORTAL(ch))
		act("$n looks at you!", rch, obj, ch, TO_VICT);
	else if (ch->gold < cost) {
		act("$n refuses to identify $p.",
		       rch, obj, 0, TO_ROOM);
		char_printf(ch, "The sage insists on %d gold to identify this for you.\n",
			cost);
		return;
	}
	else {
		ch->gold -= cost;
		char_puts("Your purse feels lighter.\n", ch);
	}

	act("$n gives a wise look at $p.", rch, obj, 0, TO_ROOM);
	spell_identify(0, 0, ch, obj ,0);
}

static void format_stat(char *buf, size_t len, CHAR_DATA *ch, int stat)
{
	if (ch->level > 20 || IS_NPC(ch))
		snprintf(buf, len, "{%c%2d {D(%2d %s%2d)",
			((ch->perm_stat[stat] + ch->mod_stat[stat]) 
				>= UMIN(get_max_train(ch, stat), 25)
				? 'W' : 'w'),
			 get_curr_stat(ch, stat),
			 ch->perm_stat[stat],
			(ch->mod_stat[stat] < 0) ? "-" : 
				(ch->mod_stat[stat] > 0) ? "+" : "+",
			 ABS(ch->mod_stat[stat]));
	else {
		snprintf(buf, len, "%s{x{x", get_stat_alias(ch, stat));
	}
}

char * format_modifier(char *buf, size_t len, int base, int mod)
{
		snprintf(buf, len, "(%d %c %d)",
			base,
			(mod < 0) ? '-' : '+',
			ABS(mod));
	return buf;
}

void do_score(CHAR_DATA *ch, const char *argument)
{
	char buf2[MAX_INPUT_LENGTH],
	     buf3[MAX_INPUT_LENGTH];
	char title[MAX_STRING_LENGTH];
	const char *name;
	int ekle = 0;
	int delta;
	class_t *cl;
	BUFFER *output;
	bool can_flee;

	if ((cl = class_lookup(ch->class)) == NULL)
		return;

	output = buf_new(ch->lang);
	buf_add(output, "\n      {G/~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/~~\\{x\n");

	strnzcpy(title, sizeof(title),
		 IS_NPC(ch) ? " Believer of Thornan." : ch->pcdata->title);
	name = IS_NPC(ch) ? capitalize(mlstr_val(ch->short_descr, ch->lang)) :
			    ch->name;
	delta = strlen(title) - cstrlen(title) + MAX_CHAR_NAME - strlen(name);
	if (delta < -32) delta = -32;
	title[32+delta] = '\0';
	snprintf(buf2, sizeof(buf2), "     {G|{x   %%s%%-%ds {Y%%3d years old   {G|____|{x\n", 33+delta);
	buf_printf(output, buf2, name, title, get_age(ch));

	buf_add(output, "     {G|{C+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+{G|{x\n");

	format_stat(buf2, sizeof(buf2), ch, STAT_STR);
	buf_printf(output, 
"     {G| {RLevel: {x%-3d (%+3d)    {C| {RStr: {x%-15.15s {C| {RReligion  : {x%-10.10s {G|{x\n",
		   ch->level,
		   ch->drain_level,
		   buf2,
		   religion_name(ch->religion));

	format_stat(buf2, sizeof(buf2), ch, STAT_INT);
	buf_printf(output,
"     {G| {RRace : {x%-11.11s  {C| {RInt: {x%-15.15s {C| {RPractice  : {x%-3d        {G|{x\n",
		race_name(ch->race),
		buf2,
		ch->practice);

	format_stat(buf2, sizeof(buf2), ch, STAT_WIS);
	buf_printf(output,
"     {G| {RSex  : {x%-11.11s  {C| {RWis: {x%-15.15s {C| {RTrain     : {x%-3d        {G|{x\n",
		   ch->sex == 0 ?	"sexless" :
		   ch->sex == 1 ?	"male" :
					"female",
		   buf2,
		   ch->train);

	format_stat(buf2, sizeof(buf2), ch, STAT_DEX);
	buf_printf(output,
"     {G| {RClass: {x%-12.12s {C| {RDex: {x%-15.15s {C| {RQuest Pnts: {x%-5d      {G|{x\n",
		IS_NPC(ch) ? "mobile" : cl->name,
		buf2,
		IS_NPC(ch) ? 0 : ch->pcdata->questpoints);

	format_stat(buf2, sizeof(buf2), ch, STAT_CON);
	buf_printf(output,
"     {G| {RAlign: {x%-12.12s {C| {RCon: {x%-15.15s {C| {R%-10.10s: {x%-3d        {G|{x\n",
		flag_string(align_names, NALIGN(ch)),
		buf2,
		IS_NPC(ch) ? "Quest?" : (IS_ON_QUEST(ch) ? "Quest Time" : "Next Quest"),
		IS_NPC(ch) ? 0 : abs(ch->pcdata->questtime));
	can_flee = CAN_FLEE(ch, cl);
	format_stat(buf2, sizeof(buf2), ch, STAT_CHA);
	buf_printf(output,
"     {G| {REthos: {x%-12.12s {C| {RCha: {x%-15.15s {C| {R%s     : {x%-5d      {G|{x\n",
		IS_NPC(ch) ? "mobile" : flag_string(ethos_table, ch->ethos),
		buf2,
		can_flee ? "Wimpy" : "Death",
		can_flee ? ch->wimpy : ch->pcdata->death);

	snprintf(buf2, sizeof(buf2), "%s %s.",
		 GETMSG("You are", ch->lang),
		 GETMSG(flag_string(position_names, ch->position), ch->lang));
	buf_printf(output, "     {G| {RHome : {x%-31.31s {C|{x %-22.22s {G|{x\n",
		IS_NPC(ch) ? "Midgaard" : hometown_name(ch->hometown),
		buf2);

	buf_add(output, "     {G|{C+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+{G|{x{x\n");

	if (ch->guarding != NULL) {
		ekle = 1;
		buf_printf(output,
"     {G| {GYou are guarding: {x%-12.12s                                  {G|{x\n",
			    ch->guarding->name);
	}

	if (ch->guarded_by != NULL) {
		ekle = 1;
		buf_printf(output,
"     {G| {GYou are guarded by: {x%-12.12s                                {G|{x\n",
			    ch->guarded_by->name);
	}

	if (!IS_NPC(ch)) {
		if (ch->pcdata->condition[COND_DRUNK] > 10) {
			ekle = 1;
			buf_printf(output,
"     {G| {GYou are drunk.                                                  {G|{x\n");
		}

		if (ch->pcdata->condition[COND_THIRST] <= 0) {
			ekle = 1;
			buf_printf(output,
"     {G| {YYou are thirsty.                                                {G|{x\n");
		}
/*		if (ch->pcdata->condition[COND_FULL]   ==	0) */
		if (ch->pcdata->condition[COND_HUNGER] <= 0) {
			ekle = 1;
			buf_printf(output,
"     {G| {YYou are hungry.                                                 {G|{x\n");
		}

		if (IS_SET(ch->state_flags, STATE_GHOST)) {
			ekle = 1;
			buf_add(output,
"     {G| {cYou are a ghost.                                                {G|{x\n");
		}

		if (ch->pcdata->condition[COND_BLOODLUST] <= 0) {
			ekle = 1;
			buf_printf(output,
"     {G| {YYou are hungry for blood.                                       {G|{x\n");
		}

		if (ch->pcdata->condition[COND_DESIRE] <=  0) {
			ekle = 1;
			buf_printf(output,
"     {G| {YYou desire your homeland.                                       {G|{x\n");
		}
	}

	if (!IS_IMMORTAL(ch) && IS_PUMPED(ch)) {
		ekle = 1;
		buf_printf(output,
"     {G| {RYour adrenaline is gushing!                                     {G|{x\n");
	}

	if (ekle)
		buf_add(output,
"     {G|{C+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+{G|{x\n");

	buf_printf(output,
"     {G| {RItems Carried :  {x%3d/%-4d        {RArmor vs magic  : {x%5d        {G|{x\n",
		ch->carry_number, can_carry_n(ch),
		GET_AC(ch,AC_EXOTIC));

	buf_printf(output,
"     {G| {RPounds Carried: {x%4d/%-5d {D(%2d%%) {RArmor vs bash   : {x%5d        {G|{x\n",
	ch_weight_carried(ch)/10, 
	IS_IMMORTAL(ch) ? -1 :  ch_max_carry_weight(ch)/10, 
	ENCUMBERANCE(ch),
	GET_AC(ch,AC_BASH));

	buf_printf(output,
"     {G| {RGold          :  {Y%-10d      {RArmor vs pierce : {x%5d        {G|{x\n",
		 ch->gold,GET_AC(ch,AC_PIERCE));

	buf_printf(output,
"     {G| {RSilver        :  {W%-10d      {RArmor vs slash  : {x%5d        {G|{x\n",
		 ch->silver,GET_AC(ch,AC_SLASH));

	buf_printf(output,
"     {G| {RCurrent exp   :  {x%-6d          {RSaves vs Spell  : {x%5d        {G|{x\n",
		ch->exp,ch->saving_throw);

/*Zz*/
	buf_printf(output,
"     {G| {RExp to level  :  {x%-6d          {RHitP:{x%5d/%-5d {D%-14.14s{G|{x\n",
		IS_NPC(ch) ? 0 : exp_to_level(ch), 
		ch->hit, 
		ch->max_hit,
		IS_NPC(ch) ? " "
			: format_modifier(buf2, sizeof(buf2),
			ch->pcdata->perm_hit, 
			ch->max_hit - ch->pcdata->perm_hit)
		);

	buf_printf(output,
"     {G| {RHitroll       :  {x%-3d {D%-10.10s  {RMana:{x%5d/%-5d {D%-14.14s{G|{x\n",
		GET_HITROLL(ch),
		"",
		ch->mana, ch->max_mana,
		IS_NPC(ch) ? " "
			: format_modifier(buf3, sizeof(buf3),
				ch->pcdata->perm_mana,
				ch->max_mana - ch->pcdata->perm_mana)
		);
	buf_printf(output,
"     {G| {RDamroll       :  {x%-3d {D%-10.10s  {RMove:{x%5d/%-5d {D%-14.14s{G|{x\n",
		GET_DAMROLL(ch),
		"",
		ch->move, ch->max_move,
		IS_NPC(ch) ? " "
			: format_modifier(buf2, sizeof(buf2),
				ch->pcdata->perm_move,
				ch->max_move - ch->pcdata->perm_move)
		);
	buf_add(output, "  {G/~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/   |{x\n");
	buf_add(output, "  {G\\________________________________________________________________\\__/{x\n");

	if (IS_SET(ch->comm, COMM_SHOWAFF))
		show_affects(ch, output);
	send_to_char(buf_string(output), ch);
	buf_free(output);
}

DO_FUN(do_oscore)
{
	class_t *cl;
	char buf2[MAX_STRING_LENGTH];
	int i;
	BUFFER *output;

	if ((cl = class_lookup(ch->class)) == NULL)
		return;

	output = buf_new(ch->lang);

	buf_printf(output, "%s %s%s\n{x",
		GETMSG("You are", ch->lang),
		IS_NPC(ch) ? capitalize(mlstr_val(ch->short_descr, ch->lang)) :
			     ch->name,
		IS_NPC(ch) ? " The Believer of Chronos." : ch->pcdata->title);

	buf_printf(output, "Level {c%d(%+d){x, {c%d{x years old (%d hours).\n",
		ch->level, ch->drain_level, get_age(ch),
		(ch->played + (int) (current_time - ch->logon)) / 3600);

	buf_printf(output,
		"Race: {c%s{x  Sex: {c%s{x  Class: {c%s{x  "
		"Hometown: {c%s{x\n",
		race_name(ch->race),
		ch->sex == 0 ? "sexless" : ch->sex == 1 ? "male" : "female",
		IS_NPC(ch) ? "mobile" : cl->name,
		IS_NPC(ch) ? "Midgaard" : hometown_name(ch->hometown));

	buf_printf(output,
		"You have {c%d{x/{c%d{x hit, {c%d{x/{c%d{x mana, "
		"{c%d{x/{c%d{x movement.\n",
		ch->hit, ch->max_hit, ch->mana, ch->max_mana,
		ch->move, ch->max_move);

	buf_printf(output,
		"You have {c%d{x practices and "
		"{c%d{x training sessions.\n",
		ch->practice, ch->train);

	buf_printf(output, "You are carrying {c%d{x/{c%d{x items "
		"with weight {c%ld{x/{c%d{x pounds.\n",
		ch->carry_number, can_carry_n(ch),
		ch_weight_carried(ch), ch_max_carry_weight(ch));

	if (ch->level > 20 || IS_NPC(ch))
		buf_printf(output,
			"Str: {c%d{x({c%d{x)  Int: {c%d{x({c%d{x)  "
			"Wis: {c%d{x({c%d{x)  Dex: {c%d{x({c%d{x)  "
			"Con: {c%d{x({c%d{x)  Cha: {c%d{x({c%d{x)\n",
			ch->perm_stat[STAT_STR], get_curr_stat(ch, STAT_STR),
			ch->perm_stat[STAT_INT], get_curr_stat(ch, STAT_INT),
			ch->perm_stat[STAT_WIS], get_curr_stat(ch, STAT_WIS),
			ch->perm_stat[STAT_DEX], get_curr_stat(ch, STAT_DEX),
			ch->perm_stat[STAT_CON], get_curr_stat(ch, STAT_CON),
			ch->perm_stat[STAT_CHA], get_curr_stat(ch, STAT_CHA));
	else
		buf_printf(output,
			"Str: {c%-9s{x Wis: {c%-9s{x Con: {c%-9s{x\n"
			"Int: {c%-9s{x Dex: {c%-9s{x Cha: {c%-11s{x\n",
			get_stat_alias(ch, STAT_STR),
			get_stat_alias(ch, STAT_WIS),
			get_stat_alias(ch, STAT_CON),
			get_stat_alias(ch, STAT_INT),
			get_stat_alias(ch, STAT_DEX),
			get_stat_alias(ch, STAT_CHA));

	snprintf(buf2, sizeof(buf2),
		 "You have scored {c%d{x exp, and have %s%s%s.\n",
		 ch->exp,
		 ch->gold + ch->silver == 0 ? "no money" :
					      ch->gold ? "{Y%ld gold{x " : str_empty,
		 ch->silver ? "{W%ld silver{x " : str_empty,
		 ch->gold + ch->silver ? ch->gold + ch->silver == 1 ?
					"coin" : "coins" : str_empty);
	if (ch->gold)
		buf_printf(output, buf2, ch->gold, ch->silver);
	else
		buf_printf(output, buf2, ch->silver);

	/* KIO shows exp to level */
	if (!IS_NPC(ch) && ch->level < LEVEL_HERO)
		buf_printf(output, "You need {c%d{x exp to level.\n",
			exp_to_level(ch));

	if (!IS_NPC(ch))
		buf_printf(output,
			"Quest Points: {c%d{x.  "
			"%s: {c%d{x.\n",
			ch->pcdata->questpoints, 
			IS_NPC(ch) ? "Quest?" : (IS_ON_QUEST(ch) ? 
					"Quest Time" : "Next Quest"),
			IS_NPC(ch) ? 0 : abs(ch->pcdata->questtime));

	if (CAN_FLEE(ch, cl))
		buf_printf(output, "Wimpy set to {c%d{x hit points.",
			   ch->wimpy);
	else
		buf_printf(output, "Total {c%d{x deaths up to now.",
			   ch->pcdata->death);

	if (ch->guarding)
		buf_printf(output, "  You are guarding: {W%s{x",
			   ch->guarding->name);

	if (ch->guarded_by)
		buf_printf(output, "  You are guarded by: {W%s{x",
			   ch->guarded_by->name);
	buf_add(output, "\n");

	if (!IS_NPC(ch)) {
		if (ch->pcdata->condition[COND_DRUNK] > 10)
			buf_add(output, "You are {cdrunk{x.\n");

		if (ch->pcdata->condition[COND_THIRST] <= 0)
			buf_add(output, "You are {rthirsty{x.\n");

/*		if (ch->pcdata->condition[COND_FULL] == 0) */
		if (ch->pcdata->condition[COND_HUNGER] <= 0)
			buf_add(output, "You are {rhungry{x.\n");
		if (ch->pcdata->condition[COND_BLOODLUST] <= 0)
			buf_add(output, "You are {rhungry for {Rblood{x.\n");
		if (ch->pcdata->condition[COND_DESIRE] <= 0)
			buf_add(output, "You {rdesire your homeland{x.\n");
		if (IS_SET(ch->state_flags, STATE_GHOST))
			buf_add(output, "You are a {cghost{x.\n");
	}

	buf_printf(output, "You are %s.\n",
		   GETMSG(flag_string(position_names, ch->position), ch->lang));

	if ((ch->position == POS_SLEEPING || ch->position == POS_RESTING ||
	     ch->position == POS_FIGHTING || ch->position == POS_STANDING)
	&& !IS_IMMORTAL(ch) && IS_PUMPED(ch))
		buf_add(output, "Your {radrenaline is gushing{x!\n");

	/* print AC values */
	if (ch->level >= 25) {
		buf_printf(output,
			   "Armor: pierce: {c%d{x  bash: {c%d{x  "
			   "slash: {c%d{x  magic: {c%d{x\n",
			   GET_AC(ch, AC_PIERCE), GET_AC(ch, AC_BASH),
			   GET_AC(ch, AC_SLASH), GET_AC(ch, AC_EXOTIC));

		buf_printf(output,
			   "Saves vs. spell: {c%d{x\n",
			   ch->saving_throw);
	}
	else {
		for (i = 0; i < 4; i++) {
			static char* ac_name[4] = {
				"{cpiercing{x",
				"{cbashing{x",
				"{cslashing{x",
				"{cmagic{x"
			};

			buf_add(output, "You are ");
			if (GET_AC(ch,i) >= 101)
				buf_printf(output,
					   "{chopelessly vulnerable{x to %s.\n",
					   ac_name[i]);
			else if (GET_AC(ch,i) >= 80)
				buf_printf(output,
					   "{cdefenseless against{x %s.\n",
					   ac_name[i]);
			else if (GET_AC(ch,i) >= 60)
				buf_printf(output, "{cbarely protected{x from %s.\n",
					   ac_name[i]);
			else if (GET_AC(ch,i) >= 40)
				buf_printf(output, "{cslightly armored{x against %s.\n",
					   ac_name[i]);
			else if (GET_AC(ch,i) >= 20)
				buf_printf(output, "{csomewhat armored{x against %s.\n",
					   ac_name[i]);
			else if (GET_AC(ch,i) >= 0)
				buf_printf(output, "{carmored{x against %s.\n",
					   ac_name[i]);
			else if (GET_AC(ch,i) >= -20)
				buf_printf(output, "{cwell-armored{x against %s.\n",
					   ac_name[i]);
			else if (GET_AC(ch,i) >= -40)
				buf_printf(output, "{cvery well-armored{x against %s.\n",
					   ac_name[i]);
			else if (GET_AC(ch,i) >= -60)
				buf_printf(output, "{cheavily armored{x against %s.\n",
					   ac_name[i]);
			else if (GET_AC(ch,i) >= -80)
				buf_printf(output, "{csuperbly armored{x against %s.\n",
					   ac_name[i]);
			else if (GET_AC(ch,i) >= -100)
				buf_printf(output, "{calmost invulnerable{x to %s.\n",
					   ac_name[i]);
			else
				buf_printf(output, "{cdivinely armored{x against %s.\n",
					   ac_name[i]);
		}
	}

	/* RT wizinvis and holy light */
	if (IS_IMMORTAL(ch)) {
		buf_printf(output, "Holy Light: {c%s{x",
			   IS_SET(ch->conf_flags, PLR_CONF_HOLYLIGHT) ?
			   "on" : "off");

		if (ch->invis_level)
			buf_printf(output, "  Invisible: {clevel %d{x",
				ch->invis_level);

		if (ch->incog_level)
			buf_printf(output, "  Incognito: {clevel %d{x",
				ch->incog_level);
		buf_add(output, "\n");
	}

	if (ch->level >= 20)
		buf_printf(output, "Hitroll: {c%d{x  Damroll: {c%d{x.\n",
			GET_HITROLL(ch), GET_DAMROLL(ch));

	buf_add(output, "You are ");
	if (IS_GOOD(ch))
		buf_add(output, "good.");
	else if (IS_EVIL(ch))
		buf_add(output, "evil.");
	else
		buf_add(output, "neutral.");

	switch (ch->ethos) {
	case ETHOS_LAWFUL:
		buf_add(output, "  You have a lawful ethos.\n");
		break;
	case ETHOS_NEUTRAL:
		buf_add(output, "  You have a neutral ethos.\n");
		break;
	case ETHOS_CHAOTIC:
		buf_add(output, "  You have a chaotic ethos.\n");
		break;
	/*
	default:
		buf_add(output, "  You have no ethos");
		if (!IS_NPC(ch))
			buf_add(output, ", report it to the gods!\n");
		else
			buf_add(output, ".\n");
	*/
	}

	if (IS_NPC(ch))
		ch->religion = 0; /* XXX */

	if ((ch->religion <= RELIGION_NONE) || (ch->religion > MAX_RELIGION))
		buf_add(output, "You don't believe any religion.\n");
	else
		buf_printf(output,"Your religion is the way of %s.\n",
			religion_table[ch->religion].leader);

	if (IS_SET(ch->comm, COMM_SHOWAFF))
		show_affects(ch, output);
	send_to_char(buf_string(output), ch);
	buf_free(output);
}

void do_affects(CHAR_DATA *ch, const char *argument)
{
	BUFFER *output;

	output = buf_new(ch->lang);
	show_affects(ch, output);
	page_to_char(buf_string(output), ch);
	buf_free(output);
}

/* object condition aliases */
char *get_cond_alias(OBJ_DATA *obj)
{
	char *stat;
	int istat = obj->condition;

	     if	(istat >= COND_EXCELLENT)	stat = "excellent";
	else if (istat >= COND_FINE)		stat = "fine";
	else if (istat >= COND_GOOD)		stat = "good";
	else if (istat >= COND_AVERAGE)		stat = "average";
	else if (istat >= COND_POOR)		stat = "poor";
	else					stat = "fragile";

	return stat;
}

/*
 * Reports list of areas sorted by average level.
 * Some areas are hidden from morts by area flag.
 *
 * by Zsuzsu
 */
void do_areas(CHAR_DATA *ch, const char *argument)
{
	AREA_DATA **area_list;
	AREA_DATA *iArea;
	AREA_DATA **walker;
	int num_areas = 0;
	int total_areas = 0;
	BUFFER *output;

	if (argument[0] != '\0') {
		char_puts("No argument is used with this command.\n",ch);
		return;
	}

	area_list = areas_avg_level_sorted();

	output = buf_new(-1);
	buf_add(output, "Current areas of Legends & Lore: \n");

	walker = area_list;
	iArea = *walker;
	while (iArea != NULL) {
		total_areas++;

		if (IS_IMMORTAL(ch)
		|| (!IS_SET(iArea->flags, AREA_CLOSED)
		&& !IS_SET(iArea->flags, AREA_HIDDEN))) {

			buf_printf(output,"{D{{{x%2d %3d{D} {%c%-20.20s{x %8.8s{x",
				iArea->min_level,
				iArea->max_level,
				IS_SET(iArea->flags, AREA_CLOSED)	? 'R' : 
				IS_SET(iArea->flags, AREA_CLANHALL)	? 'b' : 
				IS_SET(iArea->flags, AREA_PLAYERHOUSE)	? 'G' : 
				IS_SET(iArea->flags, AREA_HIDDEN)	? 'D' : 
									  'B',
				iArea->name,
				iArea->credits);

			if (num_areas++ % 2 == 0)
				buf_add(output, " ");
			else
				buf_add(output, "\n");
		}
		walker++;
		iArea = *walker;
	}

	if (num_areas % 2 == 1) buf_add(output,"\n");	
	buf_printf(output,"Areas: %d out of a total of %d\n", 
		num_areas, total_areas);
	page_to_char(buf_string(output), ch);	
	buf_free(output);
	free(area_list);
}
