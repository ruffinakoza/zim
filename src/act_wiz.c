/*
 * $Id: act_wiz.c 998 2007-01-21 21:23:49Z zsuzsu $
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
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>

#ifdef LINUX
#include <time.h>
#endif

#include "merc.h"
#include "debug.h"
#include "db/cmd.h"
#include "interp.h"
#include "update.h"
#include "quest.h"
#include "obj_prog.h"
#include "fight.h"
#include "quest.h"
#include "db/db.h"
#include "olc/olc.h"
#include "waffects.h"
#include "augment.h"
#include "stats.h"
#include "fixes.h"

/* command procedures needed */
DECLARE_DO_FUN(do_rstat	);
DECLARE_DO_FUN(do_mstat	);
DECLARE_DO_FUN(do_dstat	);
DECLARE_DO_FUN(do_ostat	);
DECLARE_DO_FUN(do_rset	);
DECLARE_DO_FUN(do_mset	);
DECLARE_DO_FUN(do_oset	);
DECLARE_DO_FUN(do_sset	);
DECLARE_DO_FUN(do_mfind_old	);
DECLARE_DO_FUN(do_ofind	);
DECLARE_DO_FUN(do_ofind_old	);
DECLARE_DO_FUN(do_mload	);
DECLARE_DO_FUN(do_oload	);
DECLARE_DO_FUN(do_save	);
DECLARE_DO_FUN(do_look	);
DECLARE_DO_FUN(do_stand	);
DECLARE_DO_FUN(do_help	);
DECLARE_DO_FUN(do_awardqp );
DECLARE_DO_FUN(do_awardxp );
DECLARE_DO_FUN(do_awardprac );
DECLARE_DO_FUN(do_warena );

qtrouble_t *qtrouble_lookup(CHAR_DATA *ch, int vnum);

void ofind_vnum (CHAR_DATA *ch, int vnum);
int repossess_limiteds (CHAR_DATA *ch, CHAR_PDATA *pvict);

extern int rebooter;

/*
 * display immortal commands
 */
void do_wizcmd(CHAR_DATA *ch, const char *argument)
{
	cmd_t *cmd;
	int col;
	int clevel;
	int control;
 
	if (IS_NPC(ch)) {
		char_puts("Huh?\n", ch);
		return;
	}

	for( clevel = LEVEL_HERO + 1; clevel < MAX_LEVEL + 1; clevel++ ) {
		control = 0;
	
		col = 0;
		for (cmd = cmd_table; cmd->name; cmd++) {
			if (cmd->level < LEVEL_IMMORTAL
			|| IS_SET(cmd->flags, CMD_HIDDEN))
				continue;
			if(cmd->level == clevel) {
				control++;
				if(control==1)
				char_printf(ch, 
"\n{C		==============={R Level {R%-2d {C ==============={x\n",clevel);

				if (ch->level < IMPLEMENTOR 
				&&  !is_name(cmd->name, ch->pcdata->granted)) {
					char_printf(ch, "{D%-12s{x",cmd->name);
					if (++col % 6 == 0)
						char_puts("\n", ch);
				}
				else {
					char_printf(ch, "{W%-12s{x",cmd->name);
					if (++col % 6 == 0)
						char_puts("\n", ch);
				}
			}
		}
	}
}

/*
 * stop all fighting in the realm
 */
void do_wpeace(CHAR_DATA *ch, const char *argument )
{
     CHAR_DATA *rch;
     char buf[MAX_STRING_LENGTH];

     rch = ch;

     for ( rch = char_list; rch; rch = rch->next )
      {
	if ( ch->desc == NULL || ch->desc->connected != CON_PLAYING )
	   continue;

	if ( rch->fighting )
	  {
	   sprintf( buf, "%s has declared World Peace.\n\r", ch->name );
	   send_to_char( buf, rch );
	   stop_fighting( rch, TRUE );
          }
      }

    send_to_char( "You have declared World Peace.\n\r", ch );
    return;
}

void do_objlist(CHAR_DATA *ch, const char *argument)
{
	FILE *fp;
	OBJ_DATA *obj;
	BUFFER *buf;
	int bp, bpmax;

	if ((fp = dfopen(TMP_PATH, "objlist.txt", "w+")) == NULL) {
	 	char_puts("File error.\n", ch);
	 	return;
	}

	buf = buf_new(-1);
	for(obj = object_list; obj != NULL; obj = obj->next) {
		if (obj->pIndexData->affected == NULL)
			continue;

		buf_clear(buf);
		buf_printf(buf, "\n#Obj: %s (Vnum : %d) \n",
			   mlstr_mval(obj->short_descr),
			   obj->pIndexData->vnum);
		format_obj(buf, obj);
		if (!IS_SET(obj->extra_flags, ITEM_ENCHANTED))
			format_obj_affects(buf, obj->pIndexData->affected,
					   FOA_F_NODURATION);
		format_obj_affects(buf, obj->affected, 0);
		build_points(obj->pIndexData, &bp, &bpmax);
		buf_printf(buf, "Build Points: %d/%d\n",
			bp, bpmax);
		fprintf(fp, "%s", fix_string(buf_string(buf)));
	}
	buf_free(buf);
	fclose(fp);
}

void do_limited(CHAR_DATA *ch, const char *argument)
{
	extern int top_obj_index;
	OBJ_DATA *obj;
	OBJ_INDEX_DATA *obj_index;
	int	lCount = 0;
	int	ingameCount;
	int 	nMatch;
	int 	vnum;
	int	bp = 0,
		bpmax = 0;

	if (argument[0] != '\0')  {
		obj_index = get_obj_index(atoi(argument));
		if (obj_index == NULL)  {
			char_puts("Not found.\n", ch);
			return;
		}
		if (obj_index->limit == -1)  {
			char_puts("Thats not a limited item.\n", ch);
			return;
		}
		nMatch = 0;

		build_points(obj_index, &bp, &bpmax);
		char_printf(ch, "{D[{%c%7d{D] {%c%3d{w of {C%3d{D max"
				" {c%3d{xlvl {W:{x %s\n", 
			build_point_percent_color(bp, bpmax),
			obj_index->vnum,
			(obj_index->count > obj_index->limit) ? 'R' : 'c',
			obj_index->count,
		        obj_index->limit, 
			obj_index->level,
			mlstr_mval(obj_index->short_descr));

		ingameCount = 0;
		for (obj=object_list; obj != NULL; obj=obj->next)
		  	if (obj->pIndexData->vnum == obj_index->vnum)  {
		      		ingameCount++;
		  	if (obj->carried_by != NULL) 
				char_printf(ch, 
					"   Carried by %s\n", obj->carried_by->name);
		  	else if (obj->in_room != NULL) 
				char_printf(ch, "   At {D[{W%7d{D]{x %s\n",
					obj->in_room->vnum,
					mlstr_cval(obj->in_room->name, ch));
		  	else if (obj->in_obj != NULL) 
				char_printf(ch, "   In {D[{W%7d{D]{x %s\n",
					obj->in_obj->pIndexData->vnum,
					mlstr_mval(obj->in_obj->short_descr));
		 	}
		char_printf(ch, "   {c%d {wfound in game. {c%d {wshould be in pFiles.{x\n", 
				ingameCount, obj_index->count-ingameCount);
		return;
	}

	nMatch = 0;
	for (vnum = 0; nMatch < top_obj_index; vnum++)
		if ((obj_index = get_obj_index(vnum)) != NULL) {
			nMatch++;
			if (obj_index->limit != -1)  {
				lCount++;

				build_points(obj_index, &bp, &bpmax);
				char_printf(ch, "{D[{%c%7d{D] {%c%3d{w of {C%3d{D max"
						" {c%3d{Dlvl {W:{x %s\n", 
					build_point_percent_color(bp, bpmax),
					obj_index->vnum,
					(obj_index->count > obj_index->limit) ? 'R' : 'c',
					obj_index->count,
					obj_index->limit, 
					obj_index->level, 
					mlstr_mval(obj_index->short_descr));
		}
	}
	char_printf(ch, "\n{c%d{w of {c%d {wobjects are limited.{x\n", lCount, nMatch);
}

void do_rare(CHAR_DATA *ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	extern int top_obj_index;
	OBJ_DATA *obj;
	OBJ_INDEX_DATA *obj_index;
	int	lCount = 0;
	int	ingameCount;
	int 	nMatch;
	int 	vnum;
	int	min_level, 
		max_level, 
		tmp;
	int	bp = 0,
		bpmax = 0;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0')  {
		char_puts("Syntax: rare [min-level] [max-level]\n"
			  "    or: rare [vnum]\n", ch);
		return;
	}

	if (arg2[0] == '\0')  {
		obj_index = get_obj_index(atoi(arg1));
		if (obj_index == NULL)  {
			char_puts("Not found.\n", ch);
			return;
		}
		if (IS_SET(obj_index->extra_flags, ITEM_RARE))  {
			char_puts("Thats not a rare item.\n", ch);
			return;
		}
		nMatch = 0;

		build_points(obj_index, &bp, &bpmax);
		char_printf(ch, "{D[{%c%7d{D] {%c%3d{w of {C%3d{D max"
				" {c%3d{xlvl {W:{x %s\n", 
			build_point_percent_color(bp, bpmax),
			obj_index->vnum,
			(obj_index->count > obj_index->limit) ? 'R' : 'c',
			obj_index->count,
		        obj_index->limit, 
			obj_index->level,
			mlstr_mval(obj_index->short_descr));

		ingameCount = 0;
		for (obj=object_list; obj != NULL; obj=obj->next)
		  	if (obj->pIndexData->vnum == obj_index->vnum)  {
		      		ingameCount++;
		  	if (obj->carried_by != NULL) 
				char_printf(ch, 
					"   Carried by %s\n", obj->carried_by->name);
		  	else if (obj->in_room != NULL) 
				char_printf(ch, "   At {D[{W%7d{D]{x %s\n",
					obj->in_room->vnum,
					mlstr_cval(obj->in_room->name, ch));
		  	else if (obj->in_obj != NULL) 
				char_printf(ch, "   In {D[{W%7d{D]{x %s\n",
					obj->in_obj->pIndexData->vnum,
					mlstr_mval(obj->in_obj->short_descr));
		 	}
		char_printf(ch, "   {c%d {wfound in game. {c%d {wshould be in pFiles.{x\n", 
				ingameCount, obj_index->count-ingameCount);
		return;
	}

	nMatch = 0;

	min_level = atoi(arg1);
	max_level = atoi(arg2);

	if (min_level > max_level) {
		tmp = min_level;
		min_level = max_level;
		max_level = tmp;
	}

	for (vnum = 0; nMatch < top_obj_index; vnum++)
		if ((obj_index = get_obj_index(vnum)) != NULL) {
			nMatch++;
			if (obj_index->level >= min_level
			&& obj_index->level <= max_level
			&& IS_SET(obj_index->extra_flags, ITEM_RARE))  {
				lCount++;

				build_points(obj_index, &bp, &bpmax);
				char_printf(ch, "{D[{%c%7d{D] {%c%3d{w of {C%3d{D max"
						" {c%3d{Dlvl {W:{x %s\n", 
					build_point_percent_color(bp, bpmax),
					obj_index->vnum,
					(obj_index->count > obj_index->limit) ? 'R' : 'c',
					obj_index->count,
					obj_index->limit, 
					obj_index->level, 
					mlstr_mval(obj_index->short_descr));
		}
	}
	char_printf(ch, "\n{c%d{w of {c%d {wobjects are rare.{x\n", lCount, nMatch);
}

/*
 * Syntax: ofind [vnum]
 *     or: ofind type [pill|wand|armor|all] level [min] [max] flags [rare][noremove]
 *     or: ofind weapon [spear|dagger] level [min] [max] flags [flaming]
 *
 * by Zsuzsu
 */
void do_ofind (CHAR_DATA *ch, const char *argument)
{
	extern int top_obj_index;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	int  affect[APPLY_MAX][3]; /* [argument]->[loc][min][max] */
	AFFECT_DATA *pAf;
	OBJ_INDEX_DATA *obj_index;
	BUFFER *output;
	AREA_DATA *pArea;
	int	lCount       = 0,
		nMatch       = 0,
		vnum         = 0,
		min_level    = 0, 
		max_level    = 999, 
		bp           = 0,
		bpmax        = 0,
		item_type    = 0,
		extra_flags  = 0,
		wear_flag    = 0,
		weapon       = -1,
		tmp          = 0,
		min_vnum     = 0,
		bp_percent   = -1,
		min_weight   = -1,
		max_weight   = -1,
		max_vnum     = top_obj_index;
	int 	i;
	bool	is_limited   = FALSE,
		closed_area  = FALSE;
	const flag_t *f      = NULL;

	for (i = 0; i<APPLY_MAX; i++) affect[i][0] = APPLY_NONE;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0')  {
		char_puts(
			 "Syntax: ofind [vnum]\n"
			 "    or: ofind [options]\n"
			 "		level  [min] [max]\n"
			 "		type   [pill|boat|light]\n"
			 "		wear   [neck|ring]\n"
			 "		weapon [spear|dagger]\n"
/*			 "		flags  [rare] [noremove]\n"
			 "		aff    [flaming]\n"
*/			 "		weight [min] [max]\n"
			 "		bp     [percent] (>=)\n"
			 "		affect [loc] [min] [max]\n"
			 "		limited\n"
			 "		closed (show closed areas)\n"
			, ch);
		return;
	}

	if (arg2[0] == '\0' && str_prefix(arg1, "limited")) {
		ofind_vnum(ch, atoi(arg1));
		return;
	}

	while (arg1[0] != '\0') {
		if (arg2[0] == '\0')
			argument = one_argument(argument, arg2, sizeof(arg2));

		/*limited items*/
		if (!str_prefix(arg1, "limited")) {
			is_limited = TRUE;
			strnzcpy(arg1, sizeof(arg1), arg2);
			arg2[0] = '\0';
			continue;
		}

		/*level range*/
		else if (!str_prefix(arg1, "levels")
		|| !str_prefix(arg1, "lvls")) {
			min_level = atoi(arg2);
			argument = one_argument(argument, arg2, sizeof(arg2));
			if (!isdigit(arg2[0])) {
				max_level = min_level;
				strnzcpy(arg1, sizeof(arg1), arg2);
				arg2[0] = '\0';
				continue;
			}
			max_level = atoi(arg2);

			if (min_level > max_level) {
				tmp = min_level;
				min_level = max_level;
				max_level = tmp;
			}
			arg2[0] = '\0';
		}

		/*weight range*/
		else if (!str_prefix(arg1, "weight")) {
			min_weight = atoi(arg2);
			argument = one_argument(argument, arg2, sizeof(arg2));
			if (!isdigit(arg2[0])) {
				max_weight = min_weight;
				strnzcpy(arg1, sizeof(arg1), arg2);
				arg2[0] = '\0';
				continue;
			}
			max_weight = atoi(arg2);

			if (min_weight > max_weight) {
				tmp = min_weight;
				min_weight = max_weight;
				max_weight = tmp;
			}
			arg2[0] = '\0';
		}

		/*item type*/
		else if (!str_prefix(arg1, "type")) {
			if ((f = flag_lookup(item_types, arg2)) == NULL) {
				char_puts("What type?\n", ch);
				show_flags(ch, item_types);
				return;
			}
			SET_BIT(item_type, f->bit);
			arg2[0] = '\0';
		}

		/*wear location*/
		else if (!str_prefix(arg1, "wear")
			|| !str_prefix(arg1, "location")) {
			if ((f = flag_lookup(wear_flags, arg2)) == NULL) {
				char_puts("Wear where?\n", ch);
				show_flags(ch, wear_flags);
				return;
			}
			SET_BIT(wear_flag, f->bit);
			arg2[0] = '\0';
		}

		/*weapon class*/
		else if (!str_prefix(arg1, "weapon")) {
			SET_BIT(item_type, ITEM_WEAPON);
			if ((weapon = flag_value(weapon_class, arg2)) < 0) {
				char_puts("What kind of weapon?\n", ch);
				show_flags(ch, weapon_class);
				return;
			}
			arg2[0] = '\0';
		}

		/* only for a certain area */
		else if (!str_prefix(arg1, "area")) {
			pArea = area_lookup(atoi(arg2));
			if (!pArea) {
				char_printf(ch, 
					"Can't find area #%s.\n", arg2);
				return;
			}
			min_vnum = pArea->min_vnum;
			max_vnum = pArea->max_vnum;
			arg2[0] = '\0';
		}

		/* build point percentage */
		else if (!str_cmp(arg1, "bp")) {
			bp_percent = atoi(arg2);
			arg2[0] = '\0';
		}

		/* show closed area items */
		else if (!str_prefix(arg1, "closed")) {
			closed_area = TRUE;
			strnzcpy(arg1, sizeof(arg1), arg2);
			arg2[0] = '\0';
		}

		/*affects*/
		else if (!str_prefix(arg1, "affect")) {
			int location;

			if ((location = flag_value(apply_flags, arg2)) < 0) {
				char_printf(ch, "Affect '%s' invalid\n"
					"Valid Affects are:\n", arg2);
				show_flags(ch, apply_flags);
				return;
			}
			for (i=0; 
			i < APPLY_MAX && affect[i][0] != APPLY_NONE; i++);

			if (i >= APPLY_MAX) {
				char_puts("Too many affects, sorry.\n", ch);
				return;
			}
			affect[i][0] = location;

			argument = one_argument(argument, arg2, sizeof(arg2));
			if (!isdigit(arg2[0])) {
				affect[i][1] = affect[i][2] = 0;
				strnzcpy(arg1, sizeof(arg1), arg2);
				arg2[0] = '\0';
				continue;
			}
			affect[i][1] = atoi(arg2);

			argument = one_argument(argument, arg2, sizeof(arg2));
			if (!isdigit(arg2[0])) {
				affect[i][2] = affect[i][1];
				strnzcpy(arg1, sizeof(arg1), arg2);
				arg2[0] = '\0';
				continue;
			}
			affect[i][2] = atoi(arg2);

			if (affect[i][1] > affect[i][2]) {
				tmp = affect[i][1];
				affect[i][1] = affect[i][2];
				affect[i][2] = tmp;
			}
			arg2[0] = '\0';
		}

		argument = one_argument(argument, arg1, sizeof(arg1));
	}

	output = buf_new(-1);
	/* now do the search */
	for (vnum = min_vnum; nMatch < max_vnum; vnum++)
		if ((obj_index = get_obj_index(vnum)) != NULL) {
			nMatch++;

			if (is_limited && !IS_OBJ_LIMITED(obj_index))
				continue;

			if (obj_index->level < min_level
			|| obj_index->level > max_level)
				continue;

			if (item_type
			&& obj_index->item_type !=  item_type)
				continue;

			if (min_weight > -1
			&& (obj_index->weight < min_weight
			|| obj_index->weight > max_weight))
				continue;

			if (wear_flag 
			&& !IS_SET(obj_index->wear_flags, wear_flag))
				continue;

			if (extra_flags 
			&& !IS_SET(obj_index->extra_flags, extra_flags)) 
				continue;

			if (weapon >= 0
			&& obj_index->value[ITEM_WEAPON_TYPE] != weapon)
				continue;

			if (bp_percent != -1) {
				build_points(obj_index, &bp, &bpmax);
				if (bp_percent == 101) {
					if (bp <= bpmax)
						continue;
				}
				else if (bp == 0 && bp_percent != 0)
					continue;
				else if (bp*100 < bpmax * bp_percent)
					continue;
			}

			if (affect[0][0] != APPLY_NONE) {
				bool has_affect = FALSE;
				for (i = 0; 
				i < APPLY_MAX && affect[i][0] != APPLY_NONE;
				i++) {
					pAf = obj_index->affected;
					while(pAf && !has_affect) {
						has_affect = 
						(affect[i][0] == pAf->location
						&& pAf->modifier >= affect[i][1]
						&& pAf->modifier <= affect[i][2]);
						pAf = pAf->next;
					}
					if (!has_affect) break;
				}
				if (!has_affect)
					continue;
			}

			pArea = area_vnum_lookup(obj_index->vnum);

			if (!closed_area && IS_SET(pArea->flags, AREA_CLOSED))
				continue;

			lCount++;

			if (bp_percent == -1)
				build_points(obj_index, &bp, &bpmax);

			buf_printf(output, "{D[{%c%7d{D] {%c%3d{w of {%c%3d{D max"
					" {%c%3d{Dlvl {W:{x %s\n", 
				build_point_percent_color(bp, bpmax),
				obj_index->vnum,
				(IS_SET(pArea->flags, AREA_CLOSED) 
				 && obj_index->count > 0) ? 'Y':
				(obj_index->limit == -1) ? 'c' :
				(obj_index->count > obj_index->limit) ? 'R' : 'C',
				obj_index->count,
				IS_SET(pArea->flags, AREA_CLOSED) 
				|| obj_index->limit == -1 ? 'D' : 'C',
				obj_index->limit, 
				IS_SET(pArea->flags, AREA_CLOSED) ? 'D' : 'c',
				obj_index->level, 
				mlstr_mval(obj_index->short_descr));
	}
	buf_printf(output, "\n   {c%d{w of {c%d {wobjects fit that criteria.{x\n", 
		lCount, nMatch);
	page_to_char(buf_string(output), ch);
	buf_free(output);
}

void ofind_vnum (CHAR_DATA *ch, int vnum)
{
	OBJ_DATA *obj;
	OBJ_INDEX_DATA *obj_index;
	int	ingameCount;
	int 	nMatch;
	int	bp = 0,
		bpmax = 0;
		
	obj_index = get_obj_index(vnum);

	if (obj_index == NULL)  {
		char_puts("Not found.\n", ch);
		return;
	}
	nMatch = 0;

	build_points(obj_index, &bp, &bpmax);
	char_printf(ch, "{D[{%c%7d{D] {%c%3d{w of {C%3d{D max"
			" {c%3d{xlvl {W:{x %s\n", 
		build_point_percent_color(bp, bpmax),
		obj_index->vnum,
		(obj_index->count > obj_index->limit) ? 'R' : 'c',
		obj_index->count,
		obj_index->limit, 
		obj_index->level,
		mlstr_mval(obj_index->short_descr));

	ingameCount = 0;
	for (obj=object_list; obj != NULL; obj=obj->next)
		if (obj->pIndexData->vnum == obj_index->vnum)  {
			ingameCount++;
		if (obj->carried_by != NULL) 
			char_printf(ch, 
				"   Carried by %s\n", obj->carried_by->name);
		else if (obj->in_room != NULL) 
			char_printf(ch, "   At {D[{W%7d{D]{x %s\n",
				obj->in_room->vnum,
				mlstr_cval(obj->in_room->name, ch));
		else if (obj->in_obj != NULL) 
			char_printf(ch, "   In {D[{W%7d{D]{x %s\n",
				obj->in_obj->pIndexData->vnum,
				mlstr_mval(obj->in_obj->short_descr));
		}
		char_printf(ch, "   {c%d {wfound in game. {c%d {wshould be in pFiles.{x\n", 
				ingameCount, obj_index->count-ingameCount);
}

void do_wiznet(CHAR_DATA *ch, const char *argument)
{
	int flag;

	if (argument[0] == '\0') {
		/* show wiznet status */
		BUFFER *output;

		output = buf_new(-1);
		buf_printf(output, "Wiznet status: %s\n",
			   IS_SET(ch->pcdata->wiznet, WIZ_ON) ? "ON" : "OFF");

		buf_add(output, "\nlvl | channel          | status");
		buf_add(output, "\n----|------------------|-------\n");
		for (flag = 0; wiznet_table[flag].name != NULL; flag++) {
			if (wiznet_table[flag].level == ML)
				continue;
			if (wiznet_table[flag].level > ch->level)
				continue;
			buf_printf(output, "%3d | {%c%-17s{x|  %s{x\n",
				wiznet_table[flag].level,
				wiznet_table[flag].level > ch->level ? 'D' : 'W', 
				wiznet_table[flag].name,
				wiznet_table[flag].level > ch->level ?
				"{DN/A" :
				IS_SET(ch->pcdata->wiznet, wiznet_table[flag].flag) ?
				"{GON" : "{rOFF");
		}
		page_to_char(buf_string(output), ch);
		buf_free(output);
		return;
	}

	if (!str_prefix(argument,"on")) {
		char_puts("Welcome to Wiznet!\n", ch);
		SET_BIT(ch->pcdata->wiznet, WIZ_ON);
		return;
	}

	if (!str_prefix(argument,"off")) {
		char_puts("Signing off of Wiznet.\n", ch);
		REMOVE_BIT(ch->pcdata->wiznet, WIZ_ON);
		return;
	}

	flag = wiznet_lookup(argument);

	if (flag == -1 
	|| ch->level < wiznet_table[flag].level
	|| (wiznet_table[flag].level == ML && !IS_TRUSTED_IMP(ch))) {
		char_puts("No such option.\n", ch);
		return;
	}
	 
	TOGGLE_BIT(ch->pcdata->wiznet, wiznet_table[flag].flag);
	if (!IS_SET(ch->pcdata->wiznet, wiznet_table[flag].flag))
		char_printf(ch,"You will no longer see %s on wiznet.\n",
		        wiznet_table[flag].name);
	else
		char_printf(ch, "You will now see %s on wiznet.\n",
			    wiznet_table[flag].name);
}
void wiznet2(flag64_t flag, int min_level, flag64_t flag_skip, 
	CHAR_DATA *ch, const char *format, ...)
{
	DESCRIPTOR_DATA *d;
	
	char msg[MAX_STRING_LENGTH];
	va_list ap;

	va_start(ap, format);
	vsnprintf(msg, sizeof(msg), format, ap);
	va_end(ap);

	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *vch = d->original ? d->original : d->character;

		if (d->connected != CON_PLAYING
		||  !vch
		||  vch->level < LEVEL_IMMORTAL
		||  !IS_SET(vch->pcdata->wiznet, WIZ_ON)
		||  (flag && !IS_SET(vch->pcdata->wiznet, flag))
		||  (flag_skip && IS_SET(vch->pcdata->wiznet, flag_skip))
		||  vch->level < min_level
		||  vch == ch)
			continue;

		if (IS_SET(vch->pcdata->wiznet, WIZ_PREFIX))
			act_puts("--> ", vch, NULL, NULL, TO_CHAR | ACT_NOLF,
				 POS_DEAD);

		act_puts(msg, vch, NULL, ch, TO_CHAR, POS_DEAD);
	}
}

void wiznet(const char *msg, CHAR_DATA *ch, const void *arg,
	    flag64_t flag, flag64_t flag_skip, int min_level)
{
	DESCRIPTOR_DATA *d;

	for (d = descriptor_list; d != NULL; d = d->next) {
		CHAR_DATA *vch = d->original ? d->original : d->character;

		if (d->connected != CON_PLAYING
		||  !vch
		||  vch->level < LEVEL_IMMORTAL
		||  !IS_SET(vch->pcdata->wiznet, WIZ_ON)
		||  (flag && !IS_SET(vch->pcdata->wiznet, flag))
		||  (flag_skip && IS_SET(vch->pcdata->wiznet, flag_skip))
		||  vch->level < min_level
		||  vch == ch)
			continue;

		if (IS_SET(vch->pcdata->wiznet, WIZ_PREFIX))
			act_puts("--> ", vch, NULL, NULL, TO_CHAR | ACT_NOLF,
				 POS_DEAD);
		act_puts(msg, vch, arg, ch, TO_CHAR, POS_DEAD);
	}
}

/*
 * Death reports
 * by Zsuzsu
 */
void wiznet_death(CHAR_DATA *killer, CHAR_DATA *victim, int death_type) {

	if (killer == victim) {
		wiznet("{W$N{x has {Rdied{x of natural causes.",
			victim, NULL, WIZ_DEATHS, 0, 0);
		return;
	}

	switch (death_type) {
		case DEATH_DUEL:
			wiznet("{W$N{x has lost a {Rduel{x to {W$t{x.", 
				victim, mlstr_mval(killer->short_descr), 
				WIZ_DEATHS, 0, 0);
			break;
		case DEATH_ENSLAVED:
			wiznet("{W$N{x has been {renslaved{x by {W$t{x.", 
				victim, mlstr_mval(killer->short_descr), 
				WIZ_DEATHS, 0, 0);
			break;
		case DEATH_SLAY:
			wiznet("{W$N{x was {rslain{x by $t.", 
				victim, mlstr_mval(killer->short_descr),
				WIZ_DEATHS, 0, 0);
			break;
		case DEATH_KILL:
		default:
			if (IS_NPC(killer))
				wiznet("{W$N{x was {Rkilled{x by $t.",
					victim, mlstr_mval(killer->short_descr),
					WIZ_DEATHS, 0, 0);
			else {
				wiznet("{W$N{x was {rmurdered{x by {W$t{x.",
					victim, mlstr_mval(killer->short_descr),
					WIZ_DEATHS, 0, 0);

				log_printf("death: %s[%d] murdered by %s[%d](%d/%d)", 
					victim->name, victim->level,
					killer->name, killer->level,
					killer->hit, killer->max_hit);
			}
	}
}

void wiznet_theft(CHAR_DATA *ch, CHAR_DATA *victim,
		OBJ_DATA *container, OBJ_DATA *obj,
		int theft_type, int chance, bool success, int worth) {
	BUFFER *output;
	output = buf_new(-1);

	switch (theft_type) {

	case THEFT_STEAL:
		buf_printf(output,
			"{W$N{x {g%s{x ",
			success ? "stole" : "failed to steal");
		if (obj == NULL)
			buf_printf(output,
				"%d coins from %s",
				worth,
				mlstr_mval(victim->short_descr));
		else
			buf_printf(output,
				"%s{D(%d){x from %s",
				mlstr_mval(obj->short_descr),
				worth/100,
				mlstr_mval(victim->short_descr));

		if (container != NULL)
			buf_printf(output,
				"'s %s",
				mlstr_mval(container->short_descr));
		if (chance > 0)
			buf_printf(output,
				" {D(chance %d%%){x", chance);

		break;

	case THEFT_PLANT:
		buf_printf(output,
			"{W$N{x {g%s{x ",
			success ? "planted" : "failed to plant");
		if (obj == NULL)
			buf_printf(output,
				"%d %s on %s",
				(worth > 0) ? worth : worth*-1,
				(worth > 0) ? "silver" : "gold",
				mlstr_mval(victim->short_descr));
		else
			buf_printf(output,
				"%s %s %s",
				mlstr_mval(obj->short_descr),
				container ? "in" : "on",
				mlstr_mval(victim->short_descr));

		if (container != NULL)
			buf_printf(output,
				"'s %s",
				mlstr_mval(container->short_descr));
		if (chance > 0)
			buf_printf(output,
				" {D(chance %d%%){x", chance);

		break;

	case THEFT_PAWN:
		buf_printf(output,
			"{W$N{x {g%s{x %s to %s for %d coins {D(worth %d){x.",
			success ? "pawned" : "tried to pawn",
			mlstr_mval(obj->short_descr),
			mlstr_mval(victim->short_descr),
			chance,
			worth);
		break;

	case THEFT_SWITCH:
		buf_printf(output,
			"{W$N{x {g%s a switcheroo{x on %s of %s for %s {D(%d%%){x",
			success ? "pulled" : "failed",
			mlstr_mval(victim->short_descr),
			mlstr_mval(obj->short_descr),
			mlstr_mval(container->short_descr),
			chance);
		break;
	}

	wiznet(buf_string(output), ch, NULL, WIZ_ECONOMY, 0, 0);
	buf_free(output);
}

void do_tick(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	
	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0')  {
		char_puts("tick area : area update\n",ch);
		char_puts("tick char : char update\n",ch);
		char_puts("tick room : room update\n",ch);
		char_puts("tick track: track update\n",ch);
		char_puts("tick obj  : obj update\n",ch);
		return;
	}
	if (is_name(arg, "area"))  {
		area_update();
		char_puts("Area updated.\n", ch);
		return;
	}
	if (is_name(arg, "char player"))  {
		char_update();
		char_puts("Players updated.\n", ch);
		return;
	}
	if (is_name(arg, "room"))  {
		room_update();
		char_puts("Room updated.\n", ch);
		return;
	}
	if (is_name(arg, "track"))  {
		track_update();
		char_puts("Tracks updated.\n", ch);
		return;
	}
	if (is_name(arg, "obj"))  {
		obj_update();
		char_puts("Objects updated.\n", ch);
		return;
	}
	do_tick(ch,str_empty);
	return;
}

/* equips a character */
void do_outfit(CHAR_DATA *ch, const char *argument)
{
	OBJ_DATA *obj;
	class_t *cl = class_lookup(ch->class);
	int sn,vnum;

	if ((ch->level > 5 && !IS_IMMORTAL(ch))
	||  IS_NPC(ch) || cl == NULL) {
		char_puts("Find it yourself!\n",ch);
		return;
	}

	if ((obj = get_eq_char(ch, WEAR_LIGHT)) == NULL)
	{
	    obj = create_obj(get_obj_index(OBJ_VNUM_SCHOOL_BANNER), 0);
		obj->cost = 0;
		obj->condition = 100;
	    obj_to_char(obj, ch);
	    equip_char(ch, obj, WEAR_LIGHT);
	}
	
	if ((obj = get_eq_char(ch, WEAR_BODY)) == NULL)
	{
		obj = create_obj(get_obj_index(OBJ_VNUM_SCHOOL_VEST), 0);
		obj->cost = 0;
		obj->condition = 100;
	    obj_to_char(obj, ch);
	    equip_char(ch, obj, WEAR_BODY);
	}

	/* do the weapon thing */
	if ((obj = get_eq_char(ch,WEAR_WIELD)) == NULL) {
		sn = 0; 
		vnum = cl->weapon;
		obj = create_obj(get_obj_index(vnum),0);
		obj->condition = 100;
	 	obj_to_char(obj,ch);
		equip_char(ch,obj,WEAR_WIELD);
	}

	if (((obj = get_eq_char(ch,WEAR_WIELD)) == NULL 
	||   !IS_WEAPON_STAT(obj,WEAPON_TWO_HANDS)) 
	&&  (obj = get_eq_char(ch, WEAR_SHIELD)) == NULL)
	{
	    obj = create_obj(get_obj_index(OBJ_VNUM_SCHOOL_SHIELD), 0);
		obj->cost = 0;
		obj->condition = 100;
	    obj_to_char(obj, ch);
	    equip_char(ch, obj, WEAR_SHIELD);
	}

	char_puts("You have been equipped by gods.\n",ch);
}

void do_smote(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *vch;
	const char *letter, *name;
	char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
	int matches = 0;

	if (!IS_NPC(ch) && IS_SET(ch->restricted_channels, CHAN_GLOBAL_EMOTE)) {
	    char_puts("You can't show your emotions.\n", ch);
	    return;
	}

	if (argument[0] == '\0') {
	    char_puts("Emote what?\n", ch);
	    return;
	}

	if (strstr(argument,ch->name) == NULL) {
		char_puts("You must include your name in an smote.\n",ch);
		return;
	}

	char_printf(ch, "%s\n", argument);

	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
		if (vch->desc == NULL || vch == ch)
			continue;

		if ((letter = strstr(argument,vch->name)) == NULL) {
			char_printf(ch, "%s\n", argument);
			continue;
		}

		strnzcpy(temp, sizeof(temp), argument);
		temp[strlen(argument) - strlen(letter)] = '\0';
		last[0] = '\0';
		name = vch->name;

		for (; *letter != '\0'; letter++) {
			if (*letter == '\'' && matches == strlen(vch->name)) {
				strnzcat(temp, sizeof(temp), "r");
				continue;
			}

			if (*letter == 's' && matches == strlen(vch->name)) {
				matches = 0;
				continue;
			}

			if (matches == strlen(vch->name))
				matches = 0;

			if (*letter == *name) {
				matches++;
				name++;
				if (matches == strlen(vch->name)) {
					strnzcat(temp, sizeof(temp), "you");
					last[0] = '\0';
					name = vch->name;
					continue;
				}
				strnzncat(last, sizeof(last), letter, 1);
				continue;
			}

			matches = 0;
			strnzcat(temp, sizeof(temp), last);
			strnzncat(temp, sizeof(temp), letter, 1);
			last[0] = '\0';
			name = vch->name;
		}
		char_printf(ch, "%s\n", temp);
	}
}	

void do_bamfin(CHAR_DATA *ch, const char *argument)
{
	if (!IS_NPC(ch)) {
		if (argument[0] == '\0') {
			char_printf(ch, "Your poofin is '%s'\n",
				    ch->pcdata->bamfin);
			return;
		}

		if (strstr(argument, ch->name) == NULL) {
			char_puts("You must include your name.\n",ch);
			return;
		}
		     
		free_string(ch->pcdata->bamfin);
		ch->pcdata->bamfin = str_dup(argument);

		char_printf(ch, "Your poofin is now '%s'\n",
			    ch->pcdata->bamfin);
	}
}

void do_bamfout(CHAR_DATA *ch, const char *argument)
{
	if (!IS_NPC(ch)) {
		if (argument[0] == '\0') {
			char_printf(ch, "Your poofout is '%s'\n",
				    ch->pcdata->bamfout);
			return;
		}
	
		if (strstr(argument,ch->name) == NULL) {
			char_puts("You must include your name.\n", ch);
			return;
		}
	
		free_string(ch->pcdata->bamfout);
		ch->pcdata->bamfout = str_dup(argument);
	
		char_printf(ch, "Your poofout is now '%s'\n",
			    ch->pcdata->bamfout);
	}
}

void do_deny(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Deny whom?\n", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (IS_NPC(victim)) {
		char_puts("Not on NPC's.\n", ch);
		return;
	}

	if (victim->level >= ch->level) {
		char_puts("You failed.\n", ch);
		return;
	}

	SET_BIT(victim->state_flags, STATE_DENY);
	char_puts("You are denied access!\n", victim);
	wiznet("$N denies access to $i",
		ch, victim, WIZ_PENALTIES, 0, 0);
	char_puts("Ok.\n", ch);
	save_char_obj(victim, FALSE);
	stop_fighting(victim, TRUE);
	quit_char(victim, 0);
}

void do_disconnect(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Disconnect whom?\n", ch);
		return;
	}

	if (is_number(arg)) {
		int desc;

		desc = atoi(arg);
		for (d = descriptor_list; d != NULL; d = d->next) {
		        if (d->descriptor == desc) {
		        	close_descriptor(d);
	        		char_puts("Ok.\n", ch);
	        		return;
	        	}
		}
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim->desc == NULL) {
		act("$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR);
		return;
	}

	for (d = descriptor_list; d != NULL; d = d->next)
		if (d == victim->desc) {
			close_descriptor(d);
			char_puts("Ok.\n", ch);
			return;
		}

	bug("Do_disconnect: desc not found.", 0);
	char_puts("Descriptor not found!\n", ch);
	return;
}

void do_echo(CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d;
	
	if (argument[0] == '\0') {
		char_puts("Global echo what?\n", ch);
		return;
	}
	
	for (d = descriptor_list; d; d = d->next)
		if (d->connected == CON_PLAYING) {
			if (d->character->level >= ch->level)
				char_puts("global> ", d->character);
			char_printf(d->character, "%s\n", argument);
		}
}

void do_recho(CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d;
	
	if (argument[0] == '\0') {
		char_puts("Local echo what?\n", ch);
		return;
	}

	for (d = descriptor_list; d; d = d->next)
		if (d->connected == CON_PLAYING
		&&   d->character->in_room == ch->in_room) {
			if (d->character->level >= ch->level)
				char_puts("local> ",d->character);
			char_printf(d->character, "%s\n", argument);
		}
}

void do_zecho(CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d;

	if (argument[0] == '\0') {
		char_puts("Zone echo what?\n",ch);
		return;
	}

	for (d = descriptor_list; d; d = d->next)
		if (d->connected == CON_PLAYING
		&&  d->character->in_room != NULL && ch->in_room != NULL
		&&  d->character->in_room->area == ch->in_room->area) {
			if (d->character->level >= ch->level)
				char_puts("zone> ", d->character);
			char_printf(d->character, "%s\n", argument);
		}
}

void do_pecho(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg, sizeof(arg));
	
	if (argument[0] == '\0' || arg[0] == '\0') {
		char_puts("Personal echo what?\n", ch); 
		return;
	}
	 
	if  ((victim = get_char_world(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim->level >= ch->level && ch->level != MAX_LEVEL)
		char_puts("personal> ", victim);

	char_printf(victim, "%s\n", argument);
	char_printf(ch, "personal> %s\n", argument);
}

ROOM_INDEX_DATA *find_location(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;
	OBJ_DATA *obj;

	if (is_number(argument))
		return get_room_index(atoi(argument));

	if ((victim = get_char_world(ch, argument)) != NULL)
		return victim->in_room;

	if ((obj = get_obj_world(ch, argument)) != NULL)
		return obj->in_room;

	return NULL;
}

void do_transfer(CHAR_DATA *ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *location, *from_room;
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0') {
		char_puts("Transfer whom (and where)?\n", ch);
		return;
	}

	if (!str_cmp(arg1, "all")) {
		for (d = descriptor_list; d != NULL; d = d->next)
		    if (d->connected == CON_PLAYING
		    &&   d->character != ch
		    &&   d->character->in_room != NULL
		    &&   can_see(ch, d->character))
			doprintf(do_transfer, ch,
				"%s %s", d->character->name, arg2);
		return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim->in_room == NULL) {
		char_puts("They are in limbo.\n", ch);
		return;
	}

	/*
	 * Thanks to Grodyn for the optional location parameter.
	 */
	if (arg2[0]) {
		if ((location = find_location(ch, arg2)) == NULL) {
		    char_puts("No such location.\n", ch);
		    return;
		}

		if (room_is_private(location) && ch->level < MAX_LEVEL) {
		    char_puts("That room is private right now.\n", ch);
		    return;
		}
	} else {
		if (ch->in_room == victim->in_room 
		&& victim->last_transfer != NULL)
			location = victim->last_transfer;
		else
			location = ch->in_room;
	}

	if (victim->fighting != NULL)
		stop_fighting(victim, TRUE);
	act("$n disappears in a mushroom cloud.", victim, NULL, NULL, TO_ROOM);
	from_room = victim->in_room;
	char_from_room(victim);

/*Zz*/
	if (ch->in_room == location) {
		char_printf(ch, "{D[{W%d{D]{w %s arrives from a puff of smoke.", 
			from_room->vnum,
			mlstr_mval(victim->short_descr));

		act("$N arrives from a puff of smoke.",
		    ch, NULL, victim, TO_NOTVICT);
	}
	else {

		act("$N arrives from a puff of smoke.",
		    location->people, NULL, victim, TO_NOTVICT);
		act("$N transfered.", ch, NULL, victim, TO_CHAR);
	}

	victim->last_transfer = from_room;

	if (ch != victim)
		act("$n has transferred you.", ch, NULL, victim, TO_VICT);

	char_to_room(victim, location);
	if (JUST_KILLED(victim))
		return;

	do_look(victim, "auto");
}



void do_at(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *location;
	ROOM_INDEX_DATA *original;
	OBJ_DATA *on;
	
	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0' || argument[0] == '\0') {
		char_puts("At where what?\n", ch);
		return;
	}

	if ((location = find_location(ch, arg)) == NULL) {
		char_puts("No such location.\n", ch);
		return;
	}

	if (room_is_private(location) 	
	&&  ch->level < MAX_LEVEL) {
		char_puts("That room is private right now.\n", ch);
		return;
	}

	original = ch->in_room;
	on = ch->on;
	char_from_room(ch);

	char_to_room(ch, location);
	if (JUST_KILLED(ch))
		return;

	interpret(ch, argument);

	/* handle 'at xxx quit' */
	if (JUST_KILLED(ch) || ch->extracted)
		return;

	char_from_room(ch);
	char_to_room(ch, original);
	if (JUST_KILLED(ch))
		return;
	ch->on = on;
}

void do_goto(CHAR_DATA *ch, const char *argument)
{
	ROOM_INDEX_DATA *location;
	CHAR_DATA *rch;
	CHAR_DATA *pet = NULL;

	if (argument[0] == '\0') {
		char_puts("Goto where?\n", ch);
		return;
	}

	if ((location = find_location(ch, argument)) == NULL) {
		char_puts("No such location.\n", ch);
		return;
	}

	if (ch->level < LEVEL_IMMORTAL) {
		if (ch->fighting) {
			char_puts("No way! You are fighting.\n", ch);
			return;
		}

		if (IS_PUMPED(ch)) {
			act_puts("You are too pumped to pray now.",
				 ch, NULL, NULL, TO_CHAR, POS_DEAD);
			return;
		}
#if 0
		if (!IS_SET(ch->in_room->room_flags, ROOM_PEACE)) {
			char_puts("You must be in a safe place in order "
				  "to make a transportation.\n", ch);
			return;
		}
#endif
		if (!IS_BUILDER(ch, location->area)
		||  !IS_BUILDER(ch, ch->in_room->area)) {
			char_puts("You cannot transfer yourself there.\n", ch);
			return;
		}
	}

	if (ch->fighting != NULL)
		stop_fighting(ch, TRUE);

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
		if (IS_TRUSTED(rch, ch->invis_level)) {
			if (ch->pcdata != NULL
			&&  ch->pcdata->bamfout[0] != '\0')
				act("$t", ch, ch->pcdata->bamfout,
				    rch, TO_VICT);
			else
				act("$n leaves in a swirling mist.", ch, NULL,
				    rch, TO_VICT);
		}

	if (ch->pet && ch->in_room == ch->pet->in_room)
		pet = ch->pet;

	char_from_room(ch);

	for (rch = location->people; rch; rch = rch->next_in_room)
		if (IS_TRUSTED(rch, ch->invis_level)) {
			if (ch->pcdata
			&&  ch->pcdata->bamfin[0])
				act("$t",
				    rch, ch->pcdata->bamfin, NULL, TO_CHAR);
			else
				act("$N appears in a swirling mist.",
				    rch, NULL, ch, TO_CHAR);
		}

	char_to_room(ch, location);
	if (JUST_KILLED(ch))
		return;
	do_look(ch, "auto");

	if (pet && !IS_AFFECTED(pet, AFF_SLEEP)) {
		if (ch->pet->position != POS_STANDING)
			do_stand(pet, str_empty);
		char_from_room(pet);
		char_to_room(pet, location);
	}
}

void do_violate(CHAR_DATA *ch, const char *argument)
{
	ROOM_INDEX_DATA *location;
	CHAR_DATA *rch;
	
	if (argument[0] == '\0') {
	    char_puts("Goto where?\n", ch);
	    return;
	}
	
	if ((location = find_location(ch, argument)) == NULL) {
	    char_puts("No such location.\n", ch);
	    return;
	}

	if (!room_is_private(location)) {
	    char_puts("That room isn't private, use goto.\n", ch);
	    return;
	}
	
	if (ch->fighting != NULL)
		stop_fighting(ch, TRUE);
	
	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
		if (IS_TRUSTED(rch, ch->invis_level)) {
			if (ch->pcdata != NULL
			&&  ch->pcdata->bamfout[0] != '\0')
				act("$t", ch, ch->pcdata->bamfout,
				    rch, TO_VICT);
			else
				act("$n leaves in a swirling mist.", ch, NULL,
				    rch, TO_VICT);
		}
	
	char_from_room(ch);
	
	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
		if (IS_TRUSTED(rch, ch->invis_level)) {
			if (ch->pcdata && ch->pcdata->bamfin[0] != '\0')
				act("$t",
				    rch, ch->pcdata->bamfin, NULL, TO_CHAR);
			else
				act("$N appears in a swirling mist.",
				    rch, NULL, ch, TO_CHAR);
		}
	
	char_to_room(ch, location);
	if (JUST_KILLED(ch))
		return;
	do_look(ch, "auto");
}

/* RT to replace the 3 stat commands */

void do_stat(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	ROOM_INDEX_DATA *location;
	CHAR_DATA *victim;
	const char *string;

	string = one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Syntax:\n", ch);
		char_puts("  stat <name>\n", ch);
		char_puts("  stat obj <name>\n", ch);
		char_puts("  stat mob <name>\n", ch);
		char_puts("  stat room <number>\n", ch);
		char_puts("  stat desc <number>\n", ch);
		return;
	}

	if (!str_prefix(arg, "room")) {
		do_rstat(ch, string);
		return;
	}
	
	if (!str_prefix(arg, "obj")) {
		do_ostat(ch, string);
		return;
	}

	if (!str_prefix(arg, "char")
	|| !str_cmp(arg, "mob")) {
		do_mstat(ch, string);
		return;
	}

	if (!str_cmp(arg, "desc")) {
		do_dstat(ch, string);
		return;
	}

	/* do it the old way */

	obj = get_obj_world(ch, argument);
	if (obj != NULL) {
		do_ostat(ch, argument);
	 	return;
	}

	victim = get_char_world(ch, argument);
	if (victim != NULL) {
		do_mstat(ch, argument);
		return;
	}


	location = find_location(ch, argument);
	if (location != NULL) {
		do_rstat(ch, argument);
		return;
	}

	char_puts("Nothing by that name found anywhere.\n",ch);
}

void do_rstat(CHAR_DATA *ch, const char *argument)
{
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *location;
	ROOM_HISTORY_DATA *rh;
	OBJ_DATA *obj;
	CHAR_DATA *rch;
	BUFFER *output;
	int door;

	one_argument(argument, arg, sizeof(arg));
	location = (arg[0] == '\0') ? ch->in_room : find_location(ch, arg);
	if (location == NULL) {
		char_puts("No such location.\n", ch);
		return;
	}

	if (ch->in_room != location 
	&&  room_is_private(location) && !IS_TRUSTED(ch,IMPLEMENTOR)) {
		char_puts("That room is private right now.\n", ch);
		return;
	}

	output = buf_new(-1);

	if (ch->in_room->affected_by)
		buf_printf(output, "Affected by %s\n", 
			   flag_string(raffect_flags, ch->in_room->affected_by));

	if (ch->in_room->room_flags)
		buf_printf(output, "Room Flags %s\n", 
			   flag_string(room_flags, ch->in_room->room_flags));

	mlstr_dump(output, "Name: ", location->name);
	buf_printf(output, "Area: '%s'\nOwner: '%s'\n",
		   location->area->name,
		   location->owner);

	buf_printf(output,
		   "Vnum: %d  Sector: %d  Light: %d  Healing: %d  Mana: %d\n",
		   location->vnum,
		   location->sector_type,
		   location->light,
		   location->heal_rate,
		   location->mana_rate);

	buf_printf(output, "Room flags: [%s].\n",
		   flag_string(room_flags, location->room_flags));
	buf_add(output, "Description:\n");
	mlstr_dump(output, str_empty, location->description);

	if (location->ed != NULL) {
		ED_DATA *ed;

		buf_add(output, "Extra description keywords: '");
		for (ed = location->ed; ed; ed = ed->next) {
			buf_add(output, ed->keyword);
			if (ed->next != NULL)
				buf_add(output, " ");
		}
		buf_add(output, "'.\n");
	}

	buf_add(output, "Characters:");
	for (rch = location->people; rch; rch = rch->next_in_room) {
		if (can_see(ch,rch)) {
		    buf_add(output, " ");
		    one_argument(rch->name, buf, sizeof(buf));
		    buf_add(output, buf);
		}
	}

	buf_add(output, ".\nObjects:   ");
	for (obj = location->contents; obj; obj = obj->next_content) {
		buf_add(output, " ");
		one_argument(obj->name, buf, sizeof(buf));
		buf_add(output, buf);
	}
	buf_add(output, ".\n");

	for (door = 0; door <= 5; door++) {
		EXIT_DATA *pexit;

		if ((pexit = location->exit[door]) != NULL) {
			buf_printf(output, "Door: %d.  To: %d.  Key: %d.  Exit flags: %d.\nKeyword: '%s'.\n",
				door,
				pexit->to_room.r == NULL ?
				-1 : pexit->to_room.r->vnum,
		    		pexit->key,
		    		pexit->exit_info,
		    		pexit->keyword);
			mlstr_dump(output, "Description: ",
				     pexit->description);
		}
	}
	buf_add(output, "Tracks:\n");
	for (rh = location->history;rh != NULL;rh = rh->next)
		buf_printf(output,"%s took door %i.\n", rh->name, rh->went);

	send_to_char(buf_string(output), ch);
	buf_free(output);
}

void do_ostat(CHAR_DATA *ch, const char *argument)
{
	int i;
	BUFFER *output;
	char arg[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int autow_avg = 0,
	    w_avg = 0,
	    avg_dnum = 0,
	    avg_dsize = 0;
	bool from_world = FALSE;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Stat what?\n", ch);
		return;
	}

	if ((obj = get_obj_room(ch, argument)) == NULL) {
		if ((obj = get_obj_world(ch, argument)) == NULL) {
			char_puts("Nothing like that in hell, heaven or earth.\n",ch);
			return;
		}
		else
			from_world = TRUE;
	}

	output = buf_new(-1);
	buf_printf(output, "Name:   %s\n", obj->name);
	buf_printf(output, "Vnum:   %d  Type: %s  Resets: %d\n",
		obj->pIndexData->vnum,
		flag_string(item_types, obj->pIndexData->item_type),
		obj->pIndexData->reset_num);

	buf_printf(output, "Wear:   %s\n",
		   flag_string(wear_flags, obj->wear_flags));

	buf_printf(output, "Extra:  %s\n",
		   flag_string(extra_flags, obj->extra_flags));
	buf_printf(output, "Number: %d/%d  Weight: %d/%d/%d (10th pounds)\n",
		1,           get_obj_number(obj),
		obj->weight, get_obj_weight(obj),get_true_weight(obj));

	buf_printf(output,
		  "Level:  %d  Cost: %d  Condition: %d  Timer: %d Count: %d\n",
		  obj->level, obj->cost, obj->condition,
		  obj->timer, obj->pIndexData->count);

	buf_printf(output,
		  "Material: %s  MatDesc: %s\n",
		  obj->material->name,
		  obj->material_descr);

	buf_printf(output,
		"room:   %d  In object: %s  Carried by: %s  Wear_loc: %d\n",
		obj->in_room    == NULL    ?        0 : obj->in_room->vnum,
		obj->in_obj     == NULL    ? "(none)" : mlstr_mval(obj->in_obj->short_descr),
		obj->carried_by == NULL    ? "(none)" : 
		    can_see(ch,obj->carried_by) ? obj->carried_by->name
					 	: "someone",
		obj->wear_loc);

	if (!mlstr_null(obj->owner))
		buf_printf(output,
			"Owner:  %s\n", mlstr_mval(obj->owner));

	if (!mlstr_null(obj->killer))
		buf_printf(output,
			"Killer: %s\n", mlstr_mval(obj->killer));

	mlstr_dump(output, "Short description: ", obj->short_descr);
	mlstr_dump(output, "Long description: ", obj->description);

	buf_printf(output, "Values: %d %d %d %d %d\n",
		obj->value[0], obj->value[1], obj->value[2], obj->value[3],
		obj->value[4]);
	
	/* now give out vital statistics as per identify */
	
	switch (obj->pIndexData->item_type) {
		int i;
	case ITEM_SCROLL: 
	case ITEM_POTION:
	case ITEM_PILL:
		buf_printf(output, "Level %d spells of:", obj->value[0]);

		for (i = 1; i < 5; i++)
			if (obj->value[i] >= 0) 
				buf_printf(output, " '%s'",
					   skill_name(obj->value[i]));
		buf_add(output, ".\n");
		break;

	case ITEM_WAND: 
	case ITEM_STAFF: 
		buf_printf(output, "Has %d(%d) charges of level %d",
			   obj->value[ITEM_WAND_CHARGES_TOTAL], 
			   obj->value[ITEM_WAND_CHARGES_REMAINING], 
			   obj->value[ITEM_WAND_LEVEL]);
	  
		if (obj->value[ITEM_WAND_SPELL] >= 0) 
			buf_printf(output, " '%s'",
				   skill_name(obj->value[ITEM_WAND_SPELL]));
		buf_add(output, ".\n");
		break;

	case ITEM_DRINK_CON:
		buf_printf(output, "It holds %s-colored %s.\n",
			   liq_table[obj->value[ITEM_DRINK_TYPE]].liq_color,
			   liq_table[obj->value[ITEM_DRINK_TYPE]].liq_name);
		break;
	  
	case ITEM_WEAPON:
		buf_printf(output, "%s\n",
			   flag_string(weapon_class, obj->value[ITEM_WEAPON_TYPE]));

		autow_avg = get_autoweapon(obj->pIndexData, &avg_dnum, &avg_dsize, 100);
		w_avg = (1 + obj->value[ITEM_WEAPON_DICE_SIZE]) 
			* obj->value[ITEM_WEAPON_DICE_NUM]/2;

		buf_printf(output, "Dam Average:         {%c%d {Dauto: %d (%d%%){x\n",
				w_avg > autow_avg * 1.20               ? 'R' :
				w_avg > autow_avg                      ? 'Y' : 
				w_avg > autow_avg * 0.90               ? 'G' :
				w_avg > autow_avg * 0.80               ? 'B' : 'M',
				w_avg, autow_avg,
				(w_avg - autow_avg)*100 / autow_avg +100);

		buf_printf(output, "Damage noun is %s.\n",
			   attack_table[obj->value[ITEM_WEAPON_ATTACK_TYPE]].noun);
		    
		if (obj->value[ITEM_WEAPON_FLAGS])
		        buf_printf(output,"Weapons flags: %s\n",
				   flag_string(weapon_type2, 
				   	obj->value[ITEM_WEAPON_FLAGS]));
		break;

	case ITEM_ARMOR:
		buf_printf(output, 
		    "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\n",
		        obj->value[ITEM_ARMOR_AC_PIERCE], 
			obj->value[ITEM_ARMOR_AC_BASH], 
			obj->value[ITEM_ARMOR_AC_SLASH], 
			obj->value[ITEM_ARMOR_AC_EXOTIC]);
		break;

	case ITEM_CONTAINER:
	        buf_printf(output,"Capacity: %d#  Maximum weight: %d#  flags: %s\n",
	        	   obj->value[ITEM_CONTAINER_WEIGHT], 
			   obj->value[ITEM_CONTAINER_PER_ITEM_WEIGHT],
			   flag_string(cont_flags, obj->value[ITEM_CONTAINER_FLAGS]));
	        if (obj->value[ITEM_CONTAINER_WEIGHT_MULTI] != 100)
	        	buf_printf(output,"Weight multiplier: %d%%\n",
				   obj->value[ITEM_CONTAINER_WEIGHT_MULTI]);
		break;
	}

	if (obj->ed) {
		ED_DATA *ed;

		buf_add(output, "Extra description keywords: '");

		for (ed = obj->ed; ed; ed = ed->next) {
			buf_add(output, ed->keyword);
			if (ed->next)
				buf_add(output, " ");
		}

		buf_add(output, "'\n");
	}

	if (obj->pIndexData->ed) {
		ED_DATA *ed;

		buf_add(output, "pIndexData extra description keywords: '");

		for (ed = obj->pIndexData->ed; ed; ed = ed->next) {
			buf_add(output, ed->keyword);
			if (ed->next)
				buf_add(output, " ");
		}

		buf_add(output, "'\n");
	}

	if (!mlstr_null(obj->writtenDesc))
		mlstr_dump(output, "WrittenDesc: ", obj->writtenDesc);

	if (!IS_SET(obj->extra_flags, ITEM_ENCHANTED))
		format_obj_affects(output, obj->pIndexData->affected,
				   FOA_F_NODURATION);
	format_obj_affects(output, obj->affected, 0);

	show_augment(ch, obj, output);

	if (obj->pIndexData->oprogs) {
		buf_add(output, "Object progs:\n");
		for (i = 0; i < OPROG_MAX; i++)
			if (obj->pIndexData->oprogs[i] != NULL)
				buf_printf(output, "%s: {y%s{x\n",
					 optype_table[i],
					 oprog_name_lookup(obj->pIndexData->oprogs[i]));
	}
	buf_printf(output,"Damage condition : %d (%s)\n", obj->condition,
				get_cond_alias(obj));

	if (obj->stolen_from > 0)
		buf_printf(output,"Stolen from [%d]\n",
			obj->stolen_from);

	for (i = 0; i < PAST_OWNER_MAX; i++) {
		if (!IS_NULLSTR(obj->past_owner[i])) {
			buf_printf(output,
				"PastOwner[%d]: %s %s\n",
				i,
				obj->past_owner[i],
				obj->past_owner_ip[i]);
		}
	}

	if (from_world)
		buf_printf(output, "Not in this room.\n");

 	buf_printf(output,
		"Next:   %s   Next_content: %s\n",
		obj->next	== NULL	   ? "(none)" : mlstr_mval(obj->next->short_descr),
		obj->next_content == NULL  ? "(none)" : mlstr_mval(obj->next_content->short_descr));


	send_to_char(buf_string(output), ch);
	buf_free(output);
}

void do_mstat(CHAR_DATA *ch, const char *argument)
{
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	AFFECT_DATA *paf;
	CHAR_DATA *victim;
	BUFFER *output;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Stat whom?\n", ch);
		return;
	}

	if ((victim = get_char_world(ch, argument))
	&& IS_NPC(victim)) {
		if ((victim = get_char_room(ch, argument)) == NULL) {
			char_puts("They aren't here.\n", ch);
			return;
		}
	}

	if (!victim) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	output = buf_new(-1);

	buf_printf(output, 
		"{yName:{x [{C%s{x] Vnum: {D[{W%6d{D]{x  "
		"Reset Zone: {D%s{x Room: [{c%6d{x]\n", 
		victim->name,
		IS_NPC(victim) ? victim->pIndexData->vnum : 0,
		(IS_NPC(victim) &&victim->zone) ? victim->zone->name : "?",
		victim->in_room == NULL    ?        0 : victim->in_room->vnum);

	buf_printf(output, 
		"Level: {c%d{D + {c%d{x  "
		"Class: {c%s{x  "
		"Race: {c%s {D({c%s{D){x  "
		"Sex: {c%s{x\n",
		victim->level,
		victim->drain_level,
		class_name(victim),
		race_name(victim->race), race_name(ORG_RACE(victim)),
		flag_string(gender_table, victim->sex));

	if (IS_NPC(victim))
		snprintf(buf, sizeof(buf), "{c%d{x", victim->alignment);
	else  {
		snprintf(buf, sizeof(buf), "{c%s{D-{c%s{x", 
			 flag_string(ethos_table, victim->ethos),
			 flag_string(align_names, NALIGN(victim)));
	}

	if (IS_NPC(victim))
		buf_printf(output, 
				"Damage: {c%d{Dd{c%d{x  "
				"Message:  {c%s{x\n",
			   victim->damage[DICE_NUMBER],
			   victim->damage[DICE_TYPE],
			   attack_table[victim->dam_type].noun);

	buf_printf(output,
		"Size: {c%s{x  "
		"Align: %s  "
		"Religion: {c%s{x.\n",
		flag_string(size_table, victim->size),
		buf,
		IS_NPC(victim) || victim->religion > MAX_RELIGION-1
			? "Zsuzsu" 
			: religion_table[victim->religion].leader);

	buf_printf(output, 
			"Str: {C%2d{x({c%2d{x)     "
			"pierce: {c%5d{x     "
			"Wimpy: {c%4d{x        "
			"HP:   {C%4d{D/{c%-4d{x\n",
		get_curr_stat(victim,STAT_STR),
		victim->perm_stat[STAT_STR],
		GET_AC(victim,AC_PIERCE), 
		victim->wimpy,
		victim->hit, victim->max_hit);

	buf_printf(output, 
			"Int: {C%2d{x({c%2d{x)     "
			"bash:   {c%5d{x     "
			"Trains:  {c%2d{x        "
			"Mana: {C%4d{D/{c%-4d{x\n",
		get_curr_stat(victim,STAT_INT),
		victim->perm_stat[STAT_INT],
		GET_AC(victim,AC_BASH),
		victim->train,
		victim->mana, victim->max_mana);

	buf_printf(output, 
			"Wis: {C%2d{x({c%2d{x)     "
			"slash:  {c%5d{x     "
			"Practs: {c%3d{x        "
			"Move: {C%4d{D/{c%-4d{x\n",
		get_curr_stat(victim,STAT_WIS),
		victim->perm_stat[STAT_WIS],
		GET_AC(victim,AC_SLASH), 
		victim->practice,
		victim->move, victim->max_move);

	buf_printf(output, 
			"Dex: {C%2d{x({c%2d{x)     "
			"magic:  {c%5d{x     "
			"Deaths: {c%3d{x        ",
		get_curr_stat(victim,STAT_DEX),
		victim->perm_stat[STAT_DEX],
		GET_AC(victim,AC_EXOTIC),
		IS_NPC(victim) ? 0 : victim->pcdata->death);

	if (!IS_NPC(victim) && ch->level == ML) {
		buf_printf(output,
				"{rPKs:   %3d{D/{r%-3d{x",
				victim->pcdata->pk_kills,
				victim->pcdata->pk_deaths);
	}

	buf_add(output, "\n");

	buf_printf(output, 
			"Con: {C%2d{x({c%2d{x)     "
			"saves:  {c%5d{x     "
			"Hit:   {C%4d{x        "
			"Exp: {c%6d{x\n",
		get_curr_stat(victim,STAT_CON),
		victim->perm_stat[STAT_CON],
		victim->saving_throw,
		GET_HITROLL(victim),
		victim->exp);

	buf_printf(output, 
			"Cha: {C%2d{x({c%2d{x)     "
			"                  "
			"Dam:   {C%4d{x        "
			"tnl: {c%6d{x\n",
		get_curr_stat(victim,STAT_CHA),
		victim->perm_stat[STAT_CHA],
		GET_DAMROLL(victim), 
		IS_NPC(victim) ? 0 : exp_to_level(victim));

	if (IS_TRUSTED_IMP(ch) && !IS_NPC(victim)) {
		buf_printf(output, 
			"Lck: {C%2d{x({c%2d{x)     "
			"\n",
			get_curr_stat(victim,STAT_LCK),
			victim->perm_stat[STAT_LCK]);
		buf_printf(output,
			"Trends: Str: {C%2d{D/%2d{x"
			"  Int: {C%2d{D/%2d{x"
			"  Wis: {C%2d{D/%2d{x"
			"  Dex: {C%2d{D/%2d{x"
			"  Con: {C%2d{D/%2d{x"
			"  Cha: {C%2d{D/%2d{x\n",
			victim->pcdata->trend_stat[STAT_STR]/victim->level,
			victim->pcdata->trend_stat[STAT_STR],
			victim->pcdata->trend_stat[STAT_INT]/victim->level,
			victim->pcdata->trend_stat[STAT_INT],
			victim->pcdata->trend_stat[STAT_WIS]/victim->level,
			victim->pcdata->trend_stat[STAT_WIS],
			victim->pcdata->trend_stat[STAT_DEX]/victim->level,
			victim->pcdata->trend_stat[STAT_DEX],
			victim->pcdata->trend_stat[STAT_CON]/victim->level,
			victim->pcdata->trend_stat[STAT_CON],
			victim->pcdata->trend_stat[STAT_CHA]/victim->level,
			victim->pcdata->trend_stat[STAT_CHA]
			);

		buf_printf(output,
			"LvlAvg: Str: {C%2d{D/%2d{x"
			"  Int: {C%2d{D/%2d{x"
			"  Wis: {C%2d{D/%2d{x"
			"  Dex: {C%2d{D/%2d{x"
			"  Con: {C%2d{D/%2d{x"
			"  Cha: {C%2d{D/%2d{x\n",
			(victim->pcdata->stat_avg[STAT_STR][STAT_AVG_NUM]) 
			?  victim->pcdata->stat_avg[STAT_STR][STAT_AVG_TOTAL] /
			   victim->pcdata->stat_avg[STAT_STR][STAT_AVG_NUM] 
			:  0,
			victim->pcdata->stat_avg[STAT_STR][STAT_AVG_NUM],

			(victim->pcdata->stat_avg[STAT_INT][STAT_AVG_NUM]) 
			?  victim->pcdata->stat_avg[STAT_INT][STAT_AVG_TOTAL] /
			   victim->pcdata->stat_avg[STAT_INT][STAT_AVG_NUM] 
			:  0,
			victim->pcdata->stat_avg[STAT_INT][STAT_AVG_NUM],

			(victim->pcdata->stat_avg[STAT_WIS][STAT_AVG_NUM]) 
			?  victim->pcdata->stat_avg[STAT_WIS][STAT_AVG_TOTAL] /
			   victim->pcdata->stat_avg[STAT_WIS][STAT_AVG_NUM] 
			:  0,
			victim->pcdata->stat_avg[STAT_WIS][STAT_AVG_NUM],

			(victim->pcdata->stat_avg[STAT_DEX][STAT_AVG_NUM]) 
			?  victim->pcdata->stat_avg[STAT_DEX][STAT_AVG_TOTAL] /
			   victim->pcdata->stat_avg[STAT_DEX][STAT_AVG_NUM] 
			:  0,
			victim->pcdata->stat_avg[STAT_DEX][STAT_AVG_NUM],

			(victim->pcdata->stat_avg[STAT_CON][STAT_AVG_NUM]) 
			?  victim->pcdata->stat_avg[STAT_CON][STAT_AVG_TOTAL] /
			   victim->pcdata->stat_avg[STAT_CON][STAT_AVG_NUM] 
			:  0,
			victim->pcdata->stat_avg[STAT_CON][STAT_AVG_NUM],

			(victim->pcdata->stat_avg[STAT_CHA][STAT_AVG_NUM]) 
			?  victim->pcdata->stat_avg[STAT_CHA][STAT_AVG_TOTAL] /
			   victim->pcdata->stat_avg[STAT_CHA][STAT_AVG_NUM] 
			:  0,
			victim->pcdata->stat_avg[STAT_CHA][STAT_AVG_NUM]
			);
	}

	if (!IS_NPC(victim)) {
		buf_printf(output,
			   "Thirst: {c%d{x  Hunger: {c%d{x  Full: {c%d{x  "
			   "Drunk: {C%d{D/{c%d{x Bloodlust: {c%d{x Desire: {c%d{x\n",
			   victim->pcdata->condition[COND_THIRST],
			   victim->pcdata->condition[COND_HUNGER],
			   victim->pcdata->condition[COND_FULL],
			   victim->pcdata->condition[COND_DRUNK],
			   drunk_tolerance(victim),
			   victim->pcdata->condition[COND_BLOODLUST],
			   victim->pcdata->condition[COND_DESIRE]);
	}

	buf_printf(output,
		"Gold: {Y%ld{x  "
		"Silver: {W%ld{x  ",
		victim->gold, victim->silver);

	if (!IS_NPC(victim)) {
		buf_printf(output,
		"Bank Gold: {Y%ld{x  "
		"Silver: {W%ld{x\n",
		victim->pcdata->bank_g,
		victim->pcdata->bank_s);
	}

	buf_printf(output, 
		"Carry number: {C%d{D/{c%d{x  "
		"Carry weight: {C%ld{D/{c%ld{D (%d%%){x\n",
		victim->carry_number, 
		can_carry_n(victim), 
		ch_weight_carried(victim),
		ch_max_carry_weight(victim),
		ENCUMBERANCE(victim));


	if (!IS_NPC(victim)) {
		buf_printf(output, 
			   "Age: {c%3d{x "
			   "Played: {c%d{x  "
			   "Last Level: {c%d{x  "
			   "Timer: {c%d{x\n",
			   get_age(victim), 
			   (int) (victim->played+current_time-victim->logon) / 3600, 
			   victim->pcdata->last_level, 
			   victim->timer);
	}

	if (!IS_NPC(victim)) {
		buf_printf(output, "AlignKills: {c%d{x %s"
				"  {c%d{x %s"
				"  {c%d{x %s"
				"  {c%d{x %s\n",
			victim->pcdata->align_killed[ALIGN_INDEX_NONE], "non",
			victim->pcdata->align_killed[ALIGN_INDEX_GOOD], "good",
			victim->pcdata->align_killed[ALIGN_INDEX_NEUTRAL], "neutral",
			victim->pcdata->align_killed[ALIGN_INDEX_EVIL], "evil");

		buf_printf(output, "AlignStanding: {c%s{x",
				flag_string(standing_types, align_standing(victim)));

		if (ch->level >= ML) {
			buf_printf(output, "  {c%d{x %s"
				"  {c%d{x %s"
				"  {c%d{x %s",
				victim->pcdata->align_standing[ALIGN_INDEX_GOOD], "good",
				victim->pcdata->align_standing[ALIGN_INDEX_NEUTRAL], "neutral",
				victim->pcdata->align_standing[ALIGN_INDEX_EVIL], "evil");
		}
		buf_printf(output, "\n");
	}

	if (!IS_NPC(victim)) {
		buf_printf(output, "Acct:    [{c%s{x]\n",
			   flag_string(acct_flags, victim->acct_flags));

		buf_printf(output, "Config:  [{c%s{x]\n",
			   flag_string(plr_conf_flags, victim->conf_flags));

		buf_printf(output, "State:   [{c%s{x]\n",
			   flag_string(state_flags, victim->state_flags));


	}
	buf_printf(output, "Fixes:    [{c%s{x]\n",
		   flag_string(fixed_flags, victim->fixed_flags));
	
	if (victim->comm)
		buf_printf(output, "Comm:    [{c%s{x]\n",
			   flag_string(comm_flags, victim->comm));

	if (victim->channels) {
		channel_t *ct;
		char color = 'x';

		buf_printf(output, "Channels:[");
		for (ct = channel_table; ct->name; ct++) {
			if (victim->level >= ct->write_level
			|| victim->level >= ct->read_level) {
				if (IS_SET(victim->channels, ct->flag))
					color = 'c';
				else
					color = 'D';
				if (IS_SET(victim->restricted_channels, ct->flag))
					color = 'r';
				buf_printf(output, "{%c%s ", color, ct->name);
			}
		}
		buf_printf(output, "{x]\n");
	}
	else
		buf_printf(output, "Channels:[{Dnone{x]\n");

	if (!IS_NPC(victim)
	&& victim->pcdata->wiznet 
	&& (ch->level > victim->level
	|| IS_TRUSTED_IMP(ch))) {
		int flag;
		char color = 'x';

		buf_printf(output, "Wiznet:  [");
		for (flag = 0; wiznet_table[flag].name; flag++) {
			if (wiznet_table[flag].level <= victim->level) {
				if (IS_SET(victim->pcdata->wiznet, 
					   wiznet_table[flag].flag))
					color = 'c';
				else
					color = 'D';
				buf_printf(output, "{%c%s ", 
					color, wiznet_table[flag].name);
			}
		}
		buf_printf(output, "{x]\n");
	}


	if (IS_NPC(victim))
		buf_printf(output, "Act:     [{c%s{x]\n",
			   flag_string(act_flags,
				       victim->pIndexData->act));

	if (IS_NPC(victim))
		buf_printf(output, "Attr:    [{c%s{x]\n",
			   flag_string(mob_attr_flags,
				       victim->pIndexData->attr_flags));

	if (IS_NPC(victim) && victim->pIndexData->off_flags)
		buf_printf(output, "Offense: [{c%s{x]\n",
			   flag_string(off_flags,
				       victim->pIndexData->off_flags));

	if (victim->imm_flags)
		buf_printf(output, "Immune:  [{c%s{x]\n",
			   flag_string(imm_flags, victim->imm_flags));
	
	if (victim->res_flags)
		buf_printf(output, "Resist:  [{c%s{x]\n",
			   flag_string(res_flags, victim->res_flags));

	if (victim->vuln_flags)
		buf_printf(output, "Vulner:  [{c%s{x]\n",
			   flag_string(vuln_flags, victim->vuln_flags));

	if (victim->affected_by)
		buf_printf(output, "Affected [{C%s{x]\n", 
			   flag_string(affect_flags, victim->affected_by));

	for (paf = victim->affected; paf != NULL; paf = paf->next)
		buf_printf(output,
		    "Spell: '{c%s{x' modifies {c%s{x by {c%d{x for {c%d{x hours with bits {c%s{x, level {c%d{x.\n",
			skill_name(paf->type),
		    flag_string(apply_flags, paf->location),
		    paf->modifier,
		    paf->duration,
		    flag_string(affect_flags, paf->bitvector),
		    paf->level
		   );

	if (IS_NPC(victim)) {
		buf_printf(output,         "Form:    [{c%s{x]\n",
			   flag_string(form_flags, victim->form));
		buf_printf(output,         "Parts:   [{c%s{x]\n",
			   flag_string(part_flags, victim->parts));
	}

	mlstr_dump(output, "Short description: ", victim->short_descr);
	if (IS_NPC(victim))
		mlstr_dump(output, "Long description: ", victim->long_descr);

	if (IS_NPC(victim) && victim->spec_fun != 0)
		buf_printf(output, "Mobile has special procedure {y%s{x.\n",
			   spec_name(victim->spec_fun));

	buf_printf(output, 
		"Group: {c%d{x  "
		"Master: {c%s{x  Leader: {c%s{x  Pet: {c%s{x\n",
		IS_NPC(victim) ? victim->group : 0, 
		victim->master      ? victim->master->name   : "(none)",
		victim->leader      ? victim->leader->name   : "(none)",
		victim->pet 	    ? victim->pet->name	     : "(none)");


	if (!IS_NPC(victim)) {
		qtrouble_t *qt;

		if (IS_ON_QUEST(victim)) {
			buf_printf(output,
				   "QuestCnt:  [{C%d{x]  "
				   "QuestPnts: [{C%d{x]  "
				   "Questnext: [{c%d{x]\n"
				   "Questgiver: [{c%d{x]  ",
				   victim->pcdata->questcount,
				   victim->pcdata->questpoints,
				   victim->pcdata->questtime < 0 ?
					-victim->pcdata->questtime : 0,
				   victim->pcdata->questgiver);

		 	buf_printf(output,
				   "QuestCntDown: [{c%d{x]  "
				   "QuestObj:  [{c%d{x]  "
				   "Questmob: [{c%d{x]\n",
				   victim->pcdata->questtime > 0 ?
					victim->pcdata->questtime : 0,
				   victim->pcdata->questobj,
				   victim->pcdata->questmob);
		}
		else {
			buf_printf(output,
				   "QuestCnt:  [{C%d{x]  "
				   "QuestPnts: [{C%d{x]  "
				   "Questnext: [{c%d{x]  "
				   "NOT QUESTING\n",
				   victim->pcdata->questcount,
				   victim->pcdata->questpoints,
				   victim->pcdata->questtime < 0 ?
					-victim->pcdata->questtime : 0);
		}

		buf_add(output, "Quest Troubles: ");
		for (qt = victim->pcdata->qtrouble; qt; qt = qt->next)
			buf_printf(output, "[{W%d{x]{D-{x[{c%d{x] ", 
			qt->vnum, qt->count-1);
		buf_add(output, "{x\n");

		if (!IS_NULLSTR(victim->pcdata->twitlist))
			buf_printf(output, "Twitlist: [{D%s{x]\n",
				   victim->pcdata->twitlist);

		buf_printf(output, "Security: {c%d{x\n",
			   victim->pcdata->security);

		if (!IS_NULLSTR(victim->pcdata->granted))
			buf_printf(output, "Granted: [{y%s{x]\n",
				   victim->pcdata->granted);
	}

	if (IS_NPC(victim))
		buf_printf(output,"Count: {c%d{x  Killed: {c%d{x\n",
			victim->pIndexData->count, victim->pIndexData->killed);

	buf_printf(output, 
		"Fighting: {C%s{x "
		"In_mind: [{c%s{x], Target: [{c%s{x]\n", 
		victim->fighting ? victim->fighting->name : "(none)" ,
		victim->in_mind ? victim->in_mind : "none",
		victim->target ? victim->target->name : "none");

	buf_printf(output, 
		"Position: {c%s{x  ",
		flag_string(position_table, victim->position));

	if (IS_PUMPED(victim))
		buf_add(output, "Adrenaline is gushing.");

	buf_add(output, "\n");

	buf_printf(output,
		"Last fought: [{c%s{x], Last fight time: [{c%s{x]\n",
		victim->last_fought ? victim->last_fought->name : "none", 
		(victim->last_fight_time < 2) ? "-" : strtime(victim->last_fight_time));

	buf_printf(output, 
		"CursedByWitch: {c%s{x  ",
		(victim->cursed_by_witch) 
			? victim->cursed_by_witch->name : "(none)");

	if (!IS_NPC(victim)) {
		buf_printf(output, 
			"SavedStalkers: {c%2d{x  Enslaver: {c%s{x",
			victim->pcdata->saved_stalkers,
			(victim->pcdata->enslaver) ? victim->pcdata->enslaver : "(none)");
		if (!IS_NULLSTR(victim->pcdata->pk_ok_list))
			buf_printf(output, "\nPKOKlist: [{r%s{x]",
				   victim->pcdata->pk_ok_list);

		if (!IS_NULLSTR(victim->pcdata->fake_ip)
		&& ch->level > victim->level
		&& IS_TRUSTED_IMP(ch)) {
			buf_printf(output, "\nFakeIP: [{r%s{x]",
				   victim->pcdata->fake_ip);
		}
	}


	buf_add(output, "\n");

	page_to_char(buf_string(output), ch);
	buf_free(output);
}

DO_FUN(do_dstat)
{
	BUFFER *output;
	DESCRIPTOR_DATA *d;
	char arg[MAX_INPUT_LENGTH];
	int desc;

	one_argument(argument, arg, sizeof(arg));
	if (!is_number(arg)) {
		do_help(ch, "'WIZ STAT'");
		return;
	}

	desc = atoi(arg);
	for (d = descriptor_list; d; d = d->next)
		if (d->descriptor == desc)
			break;
	if (!d) {
		char_puts("dstat: descriptor not found\n", ch);
		return;
	}

	output = buf_new(-1);

	buf_printf(output, "Desc: [%d]  Conn: [%d]  "
			   "Outsize: [%d]  Outtop:  [%d]\n",
		   d->descriptor, d->connected, d->outsize, d->outtop);
	buf_printf(output, "Inbuf: [%s]\n", d->inbuf);
	buf_printf(output, "Incomm: [%s]\n", d->incomm);
	buf_printf(output, "Repeat: [%d]  Inlast: [%s]\n",
		   d->repeat, d->inlast);
	if (d->character)
		buf_printf(output, "Ch: [%s]\n", d->character->name);
	if (d->original)
		buf_printf(output, "Original: [%s]\n", d->original->name);
	if (d->olced)
		buf_printf(output, "OlcEd: [%s]\n", d->olced->name);
	if (d->pString)
		buf_printf(output, "pString: [%s]\n", *d->pString);
	if (d->showstr_head)
		buf_printf(output, "showstr_head: [%s]\n", d->showstr_head);

	page_to_char(buf_string(output), ch);
	buf_free(output);
}

void do_vnum(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	const char *string;

	string = one_argument(argument, arg, sizeof(arg));
	
	if (arg[0] == '\0') {
		char_puts("Syntax:\n",ch);
		char_puts("  vnum obj <name>\n",ch);
		char_puts("  vnum mob <name>\n",ch);
		return;
	}

	if (!str_cmp(arg, "obj")) {
		do_ofind_old(ch, string);
		return;
	}

	if (!str_cmp(arg, "mob") || !str_cmp(arg, "char")) { 
		do_mfind_old(ch, string);
		return;
	}

	/* do both */
	do_mfind_old(ch, argument);
	do_ofind_old(ch, argument);
}

void do_mfind_old(CHAR_DATA *ch, const char *argument)
{
	extern int top_mob_index;
	char arg[MAX_INPUT_LENGTH];
	MOB_INDEX_DATA *pMobIndex;
	int vnum;
	int nMatch;
	bool found;

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Find whom?\n", ch);
		return;
	}

	found	= FALSE;
	nMatch	= 0;

	/*
	 * Yeah, so iterating over all vnum's takes 10,000 loops.
	 * Get_mob_index is fast, and I don't feel like threading another link.
	 * Do you?
	 * -- Furey
	 */
	for (vnum = 0; nMatch < top_mob_index; vnum++)
		if ((pMobIndex = get_mob_index(vnum)) != NULL) {
		    nMatch++;
		    if (is_name(argument, pMobIndex->name)) {
			found = TRUE;
			char_printf(ch, "[%5d] %s\n", pMobIndex->vnum,
				    mlstr_mval(pMobIndex->short_descr));
		    }
		}

	if (!found)
		char_puts("No mobiles by that name.\n", ch);
}

void do_ofind_old(CHAR_DATA *ch, const char *argument)
{
	extern int top_obj_index;
	char arg[MAX_INPUT_LENGTH];
	OBJ_INDEX_DATA *pObjIndex;
	int vnum;
	int nMatch;
	bool found;

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Find what?\n", ch);
		return;
	}

	found	= FALSE;
	nMatch	= 0;

	/*
	 * Yeah, so iterating over all vnum's takes 10,000 loops.
	 * Get_obj_index is fast, and I don't feel like threading another link.
	 * Do you?
	 * -- Furey
	 */
	for (vnum = 0; nMatch < top_obj_index; vnum++)
		if ((pObjIndex = get_obj_index(vnum)) != NULL) {
		    nMatch++;
		    if (is_name(argument, pObjIndex->name)) {
			found = TRUE;
			char_printf(ch, "[%5d] %s\n", pObjIndex->vnum,
				    mlstr_mval(pObjIndex->short_descr));
		    }
		}

	if (!found)
		char_puts("No objects by that name.\n", ch);
}

void do_owhere(CHAR_DATA *ch, const char *argument)
{
	BUFFER *buffer = NULL;
	OBJ_DATA *obj;
	OBJ_DATA *in_obj;
	int number = 0, max_found = 200;
	int vnum = 0;
	bool limited = FALSE,
	     limited_pc = FALSE;
	bool corpse_pc;

	if (argument[0] == '\0') {
		char_puts("Find what?\n",ch);
		return;
	}

	vnum = atoi(argument);

	if (ch->level >= L3) {
		limited = !str_prefix("limited", argument);
		limited_pc = !str_cmp(argument, "limited_pc");
	}

	corpse_pc = !str_cmp(argument, "corpse_pc");

	for (obj = object_list; obj != NULL; obj = obj->next) {
		if (!can_see_obj(ch, obj) 
		|| ((vnum && obj->pIndexData->vnum != vnum) ||
		   (limited && !IS_OBJ_LIMITED(obj->pIndexData)) ||
		   (!vnum && !limited && !is_name(argument, obj->name)))
		||  (!IS_IMMORTAL(ch) && ch->level < obj->level))
	        	continue;

		if (buffer == NULL)
			buffer = buf_new(-1);

	
		for (in_obj = obj; in_obj->in_obj != NULL;
		     in_obj = in_obj->in_obj)
	        	;

		if (limited_pc 
		&& (in_obj->carried_by == NULL ||
		   IS_NPC(in_obj->carried_by)))
			continue;

		if (corpse_pc 
		&& in_obj->pIndexData->item_type != ITEM_CORPSE_PC)
			continue;

		if (in_obj->carried_by != NULL
		&& (in_obj->carried_by->invis_level > ch->level
		|| in_obj->carried_by->incog_level > ch->level))
			continue;

		number++;

		if (in_obj->carried_by != NULL
		&&  can_see(ch,in_obj->carried_by)
		&&  in_obj->carried_by->in_room != NULL)
			buf_printf(buffer,
			   "%3d) {D[{W%6d{D]{x %s is carried by %s {D[{wRoom {W%d{D]{x\n",
				number,
			   	obj->pIndexData->vnum,
				mlstr_mval(obj->short_descr),
				fix_short(PERS(in_obj->carried_by, ch)),
				in_obj->carried_by->in_room->vnum);
		else if (in_obj->in_room != NULL
		     &&  can_see_room(ch, in_obj->in_room))
	        	buf_printf(buffer, "%3d) {D[{W%6d{D]{x %s is in %s {D[{wRoom {W%d{D]{x\n",
	        		number,
			   	obj->pIndexData->vnum,
				mlstr_mval(obj->short_descr),
				mlstr_cval(in_obj->in_room->name, ch), 
				in_obj->in_room->vnum);
		else
			buf_printf(buffer, "%3d) {D[{W%6d{D]{x %s{x is somewhere\n",
				number,
			   	obj->pIndexData->vnum,
				mlstr_mval(obj->short_descr));
	
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

void do_mwhere(CHAR_DATA *ch, const char *argument)
{
	BUFFER *buffer;
	CHAR_DATA *victim;
	int count = 0;

	if (argument[0] == '\0') {
		DESCRIPTOR_DATA *d;

		/* show characters logged */

		buffer = buf_new(-1);
		for (d = descriptor_list; d != NULL; d = d->next)
		{
		    if (d->character != NULL && d->connected == CON_PLAYING
		    &&  d->character->in_room != NULL 
		    && d->character->in_room->area != NULL
	  	    && can_see(ch,d->character)
		    &&  can_see_room(ch,d->character->in_room))
		    {
			victim = d->character;
			count++;
			if (d->original != NULL)
			    buf_printf(buffer,"%3d){D%-15.15s{w %-10.10s {D({r%s{D) {D[{W%6d{D]{w %s\n",
				count,
				victim->in_room->area->name,
				victim->name,
				mlstr_mval(victim->short_descr),
				victim->in_room->vnum,
				mlstr_mval(victim->in_room->name)
				);
			else 
			    buf_printf(buffer,"%3d){D%-15.15s{w %-10.10s {D[{W%6d{D]{w %s\n",
				count,
				victim->in_room->area->name,
				victim->name,
				victim->in_room->vnum,
				mlstr_mval(victim->in_room->name)
				);
		    }
		}

	    page_to_char(buf_string(buffer),ch);
		buf_free(buffer);
		return;
	}

	buffer = NULL;
	for (victim = char_list; victim; victim = victim->next)
		if (victim->in_room
		&&  is_name(argument, victim->name)
		&&  can_see(ch,victim)) {
			if (buffer == NULL)
				buffer = buf_new(-1);

			count++;
			buf_printf(buffer, "%3d) {D[{W%6d{D]{w %-28s {D[{W%6d{D]{w %s\n",
			  count, IS_NPC(victim) ? victim->pIndexData->vnum : 0,
			  IS_NPC(victim) ?
				 mlstr_mval(victim->short_descr) : victim->name,
			  victim->in_room->vnum,
			  mlstr_mval(victim->in_room->name));
		}

	if (buffer) {
		page_to_char(buf_string(buffer),ch);
		buf_free(buffer);
	}
	else
		act("You didn't find any $T.", ch, NULL, argument, TO_CHAR);
}

void do_reboo(CHAR_DATA *ch, const char *argument)
{
	char_puts("If you want to REBOOT, spell it out.\n", ch);
}

void do_shutdow(CHAR_DATA *ch, const char *argument)
{
	char_puts("If you want to SHUTDOWN, spell it out.\n", ch);
}

void do_shutdown(CHAR_DATA *ch, const char *argument)
{
	bool active;
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		do_help(ch, "SHUTDOWN");
		return;
	}

	active = dfexist(TMP_PATH, SHUTDOWN_FILE);
		
	if (!str_prefix(arg, "status")) {
		char_printf(ch, "Shutdown status: %s\n",
			    active ? "active" : "inactive");
		return;
	}

	if (!str_prefix(arg, "activate")) {
		if (!active) {
			FILE *fp = dfopen(TMP_PATH, SHUTDOWN_FILE, "w");
			if (!fp) {
				char_printf(ch, "Error: %s.\n",
					    strerror(errno));
				return;
			}
			fclose(fp);
			wiznet("$N has activated shutdown", ch, NULL, 0, 0, 0);
			char_puts("Shutdown activated.\n", ch);
		}
		else
			char_puts("Shutdown already activated.\n", ch);
		return;
	}

	if (!str_prefix(arg, "deactivate") || !str_prefix(arg, "cancel")) {
		if (!active)
			char_puts("Shutdown already inactive.\n", ch);
		else {
			if (dunlink(TMP_PATH, SHUTDOWN_FILE) < 0) {
				char_printf(ch, "Error: %s.\n",
					    strerror(errno));
				return;
			}
			wiznet("$N has deactivated shutdown",
				ch, NULL, 0, 0, 0);
			char_puts("Shutdown deactivated.\n", ch);
		}
		return;
	}

	do_shutdown(ch, str_empty);
}

void do_protect(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;

	if (argument[0] == '\0') {
		char_puts("Protect whom from snooping?\n",ch);
		return;
	}

	if ((victim = get_char_world(ch,argument)) == NULL) {
		char_puts("You can't find them.\n",ch);
		return;
	}

	if (IS_SET(victim->comm,COMM_SNOOP_PROOF)) {
		act_puts("$N is no longer snoop-proof.", ch, NULL, victim,
			 TO_CHAR, POS_DEAD);
		char_puts("Your snoop-proofing was just removed.\n", victim);
		REMOVE_BIT(victim->comm, COMM_SNOOP_PROOF);
	}
	else {
		act_puts("$N is now snoop-proof.", ch, NULL, victim, TO_CHAR,
			 POS_DEAD);
		char_puts("You are now immune to snooping.\n", victim);
		SET_BIT(victim->comm, COMM_SNOOP_PROOF);
	}
}
	
void do_snoop(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Snoop whom?\n", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim->desc == NULL) {
		char_puts("No descriptor to snoop.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("Cancelling all snoops.\n", ch);
		wiznet("$N stops being such a snoop.", ch, NULL, WIZ_SNOOPS,
		       0, ch->level);
		for (d = descriptor_list; d != NULL; d = d->next)
			if (d->snoop_by == ch->desc)
				d->snoop_by = NULL;
		return;
	}

	if (victim->desc->snoop_by != NULL) {
		char_puts("Busy already.\n", ch);
		return;
	}

	if (!is_room_owner(ch,victim->in_room)
	&&  ch->in_room != victim->in_room 
	&&  room_is_private(victim->in_room)
	&&  !IS_TRUSTED(ch, IMPLEMENTOR)) {
		char_puts("That character is in a private room.\n",ch);
		return;
	}

	if (victim->level >= ch->level 
	||  IS_SET(victim->comm, COMM_SNOOP_PROOF)) {
		char_puts("You failed.\n", ch);
		return;
	}

	if (ch->desc != NULL)
		for (d = ch->desc->snoop_by; d != NULL; d = d->snoop_by)
		    if (d->character == victim || d->original == victim) {
			char_puts("No snoop loops.\n", ch);
			return;
		    }

	victim->desc->snoop_by = ch->desc;
	wiznet("$N starts snooping on $i.",
		ch, victim, WIZ_SNOOPS, 0, ch->level);
	char_puts("Ok.\n", ch);
}

void do_switch(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg, sizeof(arg));
	
	if (arg[0] == '\0') {
		char_puts("Switch into whom?\n", ch);
		return;
	}

	if (ch->desc == NULL)
		return;
	
	if (ch->desc->original != NULL) {
		char_puts("You are already switched.\n", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim == ch) {
		char_puts("Ok.\n", ch);
		return;
	}

	if (!IS_NPC(victim)) {
		char_puts("You can only switch into mobiles.\n", ch);
		return;
	}

	if (!is_room_owner(ch,victim->in_room)
	&&  ch->in_room != victim->in_room 
	&&  room_is_private(victim->in_room)
	&&  !IS_TRUSTED(ch,IMPLEMENTOR)) {
		char_puts("That character is in a private room.\n", ch);
		return;
	}

	if (victim->desc != NULL) {
		char_puts("Character in use.\n", ch);
		return;
	}

	wiznet("$N switches into $i.",
		ch, victim, WIZ_SWITCHES, 0, ch->level);

	ch->desc->character = victim;
	ch->desc->original  = ch;
	victim->desc        = ch->desc;
	ch->desc            = NULL;
	/* change communications to match */
	if (ch->prompt != NULL)
		victim->prompt = str_qdup(ch->prompt);
	victim->comm = ch->comm;
	victim->lines = ch->lines;

	if (IS_SET(ch->state_flags, STATE_LOG))
		SET_BIT(victim->state_flags, STATE_LOG);

	char_puts("You assume your new identity.\n", victim);
}

void do_return(CHAR_DATA *ch, const char *argument)
{
	if (ch->desc == NULL)
		return;

	if (ch->desc->original == NULL)
		return;

	char_puts("You return to your original body.\n", ch);

	if (ch->prompt != NULL) {
		free_string(ch->prompt);
		ch->prompt = NULL;
	}

	wiznet("$N returns from $i.",
		ch->desc->original, ch,
		WIZ_SWITCHES, 0, ch->desc->original->level);

	REMOVE_BIT(ch->desc->character->state_flags, STATE_LOG);
	ch->desc->character       = ch->desc->original;
	ch->desc->original        = NULL;
	ch->desc->character->desc = ch->desc; 
	ch->desc                  = NULL;

	do_replay(ch, str_empty);
}

/* trust levels for load and clone */
bool obj_check(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (IS_TRUSTED(ch, GOD)
	|| (IS_TRUSTED(ch, IMMORTAL) && obj->level <= 20 && obj->cost <= 1000)
	|| (IS_TRUSTED(ch, DEMI)     && obj->level <= 10 && obj->cost <= 500)
	|| (IS_TRUSTED(ch, ANGEL)    && obj->level <=  5 && obj->cost <= 250)
	|| (IS_TRUSTED(ch, AVATAR)   && obj->level ==  0 && obj->cost <= 100))
		return TRUE;
	else
		return FALSE;
}

bool mob_check(CHAR_DATA *ch, CHAR_DATA *mob)
{
	if ((mob->level > 20 && !IS_TRUSTED(ch, GOD))
	||  (mob->level > 10 && !IS_TRUSTED(ch, IMMORTAL))
	||  (mob->level >  5 && !IS_TRUSTED(ch, DEMI))
	||  (mob->level >  0 && !IS_TRUSTED(ch, ANGEL))
	||  !IS_TRUSTED(ch, AVATAR))
		return FALSE;
	else
		return TRUE;
}

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *clone)
{
	OBJ_DATA *c_obj, *t_obj;
	for (c_obj = obj->contains; c_obj != NULL; c_obj = c_obj->next_content)
		if (obj_check(ch, c_obj)) {
			t_obj = create_obj(c_obj->pIndexData, 0);
			clone_obj(c_obj, t_obj);
			obj_to_obj(t_obj, clone);
			recursive_clone(ch, c_obj, t_obj);
		}
}

/* command that is similar to load */
void do_clone(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	const char *rest;
	CHAR_DATA *mob;
	OBJ_DATA  *obj;

	rest = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Clone what?\n",ch);
		return;
	}

	if (!str_prefix(arg,"object")) {
		mob = NULL;
		obj = get_obj_here(ch,rest);
		if (obj == NULL) {
		    char_puts("You don't see that here.\n",ch);
		    return;
		}
	} else if (!str_prefix(arg,"mobile") || !str_prefix(arg,"character")) {
		obj = NULL;
		mob = get_char_room(ch,rest);
		if (mob == NULL) {
		    char_puts("You don't see that here.\n",ch);
		    return;
		}
	} else { /* find both */
		mob = get_char_room(ch,argument);
		obj = get_obj_here(ch,argument);
		if (mob == NULL && obj == NULL) {
			char_puts("You don't see that here.\n",ch);
			return;
		}
	}

	/* clone an object */
	if (obj) {
		OBJ_DATA *clone;

		if (!obj_check(ch,obj)) {
			char_puts("You haven't enough power.\n", ch);
			return;
		}

		clone = create_obj(obj->pIndexData, 0); 
		clone_obj(obj, clone);
		if (obj->carried_by != NULL)
		    obj_to_char(clone, ch);
		else
		    obj_to_room(clone, ch->in_room);
		recursive_clone(ch, obj, clone);

		act("$n has created $p.", ch, clone, NULL, TO_ROOM);
		act("You clone $p.", ch, clone, NULL, TO_CHAR);
		wiznet("$N clones $p.",
			ch, clone, WIZ_LOAD, 0, ch->level);
		return;
	} else if (mob != NULL) {
		CHAR_DATA *clone;
		OBJ_DATA *new_obj;

		if (!IS_NPC(mob)) {
		    char_puts("You can only clone mobiles.\n",ch);
		    return;
		}

		if (!mob_check(ch, mob)) {
			char_puts("You haven't enough power.\n", ch);
			return;
		}

		clone = create_mob(mob->pIndexData);
		clone_mob(mob,clone); 
		
		for (obj = mob->carrying; obj != NULL; obj = obj->next_content)
			if (obj_check(ch,obj)) {
				new_obj = create_obj(obj->pIndexData, 0);
				clone_obj(obj, new_obj);
				recursive_clone(ch, obj, new_obj);
				obj_to_char(new_obj, clone);
				new_obj->wear_loc = obj->wear_loc;
			}
		act("$n has created $N.", ch, NULL, clone, TO_ROOM);
		act("You clone $N.", ch, NULL, clone, TO_CHAR);
		wiznet("$N clones $i.",
			ch, clone, WIZ_LOAD, WIZ_SECURE, ch->level);
		char_to_room(clone, ch->in_room);
	}
}

/* RT to replace the two load commands */

void do_load(CHAR_DATA *ch, const char *argument)
{
	 char arg[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg, sizeof(arg));

	if (ch->level <= L7 &&
           !is_name(ch->name, ch->in_room->area->builders)) {
			char_puts("You can't load here!\n", ch);
			return;
	}

	if (arg[0] == '\0') {
		char_puts("Syntax:\n", ch);
		char_puts("  load mob <vnum>\n", ch);
		char_puts("  load obj <vnum> <level>\n", ch);
		return;
	}

	if (!str_cmp(arg,"mob") || !str_cmp(arg,"char")) {
		do_mload(ch, argument);
		return;
	}

	if (!str_cmp(arg,"obj")) {
		do_oload(ch, argument);
		return;
	}
	/* echo syntax */
	do_load(ch,str_empty);
}

void do_mload(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	MOB_INDEX_DATA *pMobIndex;
	CHAR_DATA *victim;
	int vnum;
	
	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0' || !is_number(arg)) {
		char_puts("Syntax: load mob <vnum>.\n", ch);
		return;
	}

	if ((pMobIndex = get_mob_index(vnum = atoi(arg))) == NULL) {
		char_printf(ch, "%d: No mob has that vnum.\n", vnum);
		return;
	}

	victim = create_mob(pMobIndex);
	act("$n has created $N!", ch, NULL, victim, TO_ROOM);
	wiznet("$N loads $i.", ch, victim, WIZ_LOAD, 0, ch->level);
	act("You have created $N!", ch, NULL, victim, TO_CHAR);
	char_to_room(victim, ch->in_room);
}

void do_oload(CHAR_DATA *ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH] ,arg2[MAX_INPUT_LENGTH];
	OBJ_INDEX_DATA *pObjIndex;
	OBJ_DATA *obj;
	int vnum;
	
	argument = one_argument(argument, arg1, sizeof(arg1));
	one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0' || !is_number(arg1)) {
		char_puts("Syntax: load obj <vnum>.\n", ch);
		return;
	}
	
	vnum = atoi(arg1);
	if ((pObjIndex = get_obj_index(vnum)) == NULL) {
		char_printf(ch, "%d: No objects with this vnum.\n", vnum);
		return;
	}

	obj = create_obj(pObjIndex, 0);
	if (CAN_WEAR(obj, ITEM_TAKE))
		obj_to_char(obj, ch);
	else
		obj_to_room(obj, ch->in_room);
	act("$n has created $p!", ch, obj, NULL, TO_ROOM);
	wiznet("$N loads $p.", ch, obj, WIZ_LOAD, 0, ch->level);
	act("You have created $p!", ch, obj, NULL, TO_CHAR);
}

void do_purge(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	DESCRIPTOR_DATA *d;


	one_argument(argument, arg, sizeof(arg));


	if (ch->level <= (L7) &&
           !is_name(ch->name, ch->in_room->area->builders)) {
			char_puts("You can't purge in this area!\n", ch);
			return;
	}

	if (arg[0] == '\0') {

		/* 'purge' */
		CHAR_DATA *vnext;
		OBJ_DATA  *obj_next;

		for (victim = ch->in_room->people; victim; victim = vnext) {
			vnext = victim->next_in_room;
			if (IS_NPC(victim)
			&&  !IS_SET(victim->pIndexData->act, ACT_NOPURGE)
			&&  victim != ch /* safety precaution */)
				extract_char(victim, 0);
		}

		for (obj = ch->in_room->contents; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			if (!IS_OBJ_STAT(obj, ITEM_NOPURGE))
				extract_obj(obj, 0);
		}

		act("$n purges the room!", ch, NULL, NULL, TO_ROOM);
		char_puts("Ok.\n", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (!IS_NPC(victim)) {
		if (ch == victim) {
			char_puts("Ho ho ho.\n", ch);
			return;
		}

		if (ch->level <= victim->level) {
			char_puts("Maybe that wasn't a good idea...\n",ch);
			char_printf(ch,"%s tried to purge you!\n",ch->name);
			return;
		}

		act("$n disintegrates $N.", ch, 0, victim, TO_NOTVICT);

		if (victim->level > 1)
			save_char_obj(victim, FALSE);
		d = victim->desc;
		extract_char(victim, 0);
		if (d)
			close_descriptor(d);
		return;
	}

	act("$n purges $N.", ch, NULL, victim, TO_NOTVICT);
	extract_char(victim, 0);
}

void do_restore(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *vch;
	DESCRIPTOR_DATA *d;

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0' || !str_cmp(arg,"room")) {
	/* cure room */
		
	    for (vch = ch->in_room->people; vch; vch = vch->next_in_room) {
	        affect_strip(vch,gsn_plague);
	        affect_strip(vch,gsn_poison);
	        affect_strip(vch,gsn_blindness);
	        affect_strip(vch,gsn_sleep);
	        affect_strip(vch,gsn_curse);
	        
	        vch->hit 	= vch->max_hit;
	        vch->mana	= vch->max_mana;
	        vch->move	= vch->max_move;
	        update_pos(vch);
	        act("$n has restored you.",ch,NULL,vch,TO_VICT);
	    }

		wiznet("$N restored room $j.",
			ch, (const void*) ch->in_room->vnum,
			WIZ_RESTORE, 0, ch->level);
	    
	    char_puts("Room restored.\n",ch);
	    return;

	}
	
	if (ch->level >=  MAX_LEVEL - 1 && !str_cmp(arg,"all")) {
	/* cure all */
	    for (d = descriptor_list; d != NULL; d = d->next) {
		    victim = d->character;

		    if (victim == NULL || IS_NPC(victim))
			continue;
	            
	        affect_strip(victim,gsn_plague);
	        affect_strip(victim,gsn_poison);
	        affect_strip(victim,gsn_blindness);
	        affect_strip(victim,gsn_sleep);
	        affect_strip(victim,gsn_curse);
	        
	        victim->hit 	= victim->max_hit;
	        victim->mana	= victim->max_mana;
	        victim->move	= victim->max_move;
	        update_pos(victim);
		    if (victim->in_room != NULL)
	            act("$n has restored you.",ch,NULL,victim,TO_VICT);
	    }
		char_puts("All active players restored.\n",ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	affect_strip(victim, gsn_plague);
	affect_strip(victim, gsn_poison);
	affect_strip(victim, gsn_blindness);
	affect_strip(victim, gsn_sleep);
	affect_strip(victim, gsn_curse);
	victim->hit  = victim->max_hit;
	victim->mana = victim->max_mana;
	victim->move = victim->max_move;
	update_pos(victim);
	act("$n has restored you.", ch, NULL, victim, TO_VICT);
	wiznet("$N restored $i",
		ch, victim, WIZ_RESTORE, 0, ch->level);
	char_puts("Ok.\n", ch);
}
		
void do_freeze(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Freeze whom?\n", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (IS_NPC(victim)) {
		char_puts("Not on NPC's.\n", ch);
		return;
	}

	if (victim->level >= ch->level) {
		char_puts("You failed.\n", ch);
		return;
	}

	TOGGLE_BIT(victim->state_flags, STATE_FREEZE);
	if (!IS_SET(victim->state_flags, STATE_FREEZE)) {
		char_puts("You can play again.\n", victim);
		char_puts("FREEZE removed.\n", ch);
		wiznet("$N thaws $i.",
			ch, victim, WIZ_PENALTIES, 0, 0);
	}
	else {
		char_puts("You can't do ANYthing!\n", victim);
		char_puts("FREEZE set.\n", ch);
		wiznet("$N puts $i in the deep freeze.",
			ch, victim, WIZ_PENALTIES, 0, 0);
	}
	save_char_obj(victim, FALSE);
}

void do_log(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Log whom?\n", ch);
		return;
	}

	if (!str_cmp(arg, "all")) {
		if (fLogAll) {
			fLogAll = FALSE;
			char_puts("Log ALL off.\n", ch);
		} else {
			fLogAll = TRUE;
			char_puts("Log ALL on.\n", ch);
		}
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (IS_NPC(victim)) {
		char_puts("Not on NPC's.\n", ch);
		return;
	}

	/*
	 * No level check, gods can log anyone.
	 */
	TOGGLE_BIT(victim->state_flags, STATE_LOG);
	if (!IS_SET(victim->state_flags, STATE_LOG))
		char_puts("LOG removed.\n", ch);
	else 
		char_puts("LOG set.\n", ch);
}

/* Coded by Thornan for Iceydepths */

void do_addlag(CHAR_DATA *ch, const char *argument)
{

	CHAR_DATA *victim;
	char arg[MAX_STRING_LENGTH];
	int x;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0')
	{
		send_to_char("addlag to who?\n", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL)
	{
		send_to_char("They're not here.\n", ch);
		return;
	}

	if ((x = atoi(argument)) <= 0)
	{
		send_to_char("That makes a LOT of sense.\n", ch);
		return;
	}

	if (x > 100)
	{
		send_to_char("There's a limit to cruel and unusual punishment\n", ch);
		return;
	}

	send_to_char("Somebody REALLY didn't like you\n", victim);
	WAIT_STATE(victim, x);
	send_to_char("Adding lag now...\n", ch);
	return;
}

/* void do_check(CHAR_DATA *ch, const char *argument)
{
        char buf[MAX_STRING_LENGTH];
        char arg[MAX_INPUT_LENGTH];
        BUFFER *buffer;
        CHAR_DATA *victim;
        int count = 1;

        argument = one_argument(argument, arg, sizeof(arg));

        if (arg[0] == '\0'|| !str_prefix(arg,"stats"))
        {
                buffer = buf_new(ch->lang);
                for (victim = char_list; victim != NULL; victim = victim->next)
                {
                        if (IS_NPC(victim) || !can_see(ch,victim))
                        continue;

                        if (victim->desc == NULL)
                        {
                                sprintf(buf,"%3d) %s is linkdead.\n\r", count,
                                                victim->name);
                                buf_add(buffer, buf);
                                count++;
                                continue;
                        }

                        if (victim->desc->connected >= CON_GET_NEW_RACE
                         && victim->desc->connected <= CON_PICK_WEAPON)
                        {
                                sprintf(buf,"%3d) %s is being created.\n\r",
                                        count, victim->name);
                                buf_add(buffer, buf);
                                count++;
                                continue;
                        }

                       if ( (victim->desc->connected == CON_GET_OLD_PASSWORD
                        || victim->desc->connected >= CON_READ_IMOTD))
                        {
                                sprintf(buf,"%3d) %s is connecting.\n\r",
                                                count, victim->name);
                                buf_add(buffer, buf);
                                count++;
                                continue;
                        }

                        if (victim->desc->connected == CON_PLAYING)
                        {
                                sprintf(buf,
        "%3d) %s, Level %d connected since {M%d{x hours ({G%d total hours{x)\n\r",
                                 count, victim->name,
                                 victim->level, ((int)(current_time
                                 - victim->pcdata->logon)) /3600,
                                 (victim->pcdata->played + (int)(current_time
                                 - victim->pcdata->logon)) /3600 );
                                buf_add(buffer, buf);
                                if (arg[0]!='\0' && !str_prefix(arg,
                                        "stats")) {
                                        sprintf(buf,
"   {C%5d HP %5d M ({Y%2dstr %2dint %2dwis %2ddex %2dcon{x) {m%3d Tr %3d Pr {B%4d golds %d Qpts{x\n\r",
                                         victim->max_hit,
                                         victim->max_mana,
                                         victim->perm_stat[STAT_STR],
                                         victim->perm_stat[STAT_INT],
                                         victim->perm_stat[STAT_WIS],
                                         victim->perm_stat[STAT_DEX],
                                         victim->perm_stat[STAT_CON],
                                         victim->pcdata->train,
                                         victim->pcdata->practice,
                                         victim->gold
                                         + victim->silver/100,
                                         victim->pcdata->questpoints);
                                        buf_add(buffer, buf);
                                }
                        count++;
                        continue;
                        }

                        sprintf(buf,
                        "%3d) BUGGY (Oops!)...please report to Implemetators: "
                                        "%s %d\n\r",
                                count, victim->name, victim->desc->connected);
                        buf_add(buffer, buf);
                        count++;
                }

                page_to_char(buf_string(buffer),ch);
                buf_free(buffer);
                return;
        }

        if (!str_prefix(arg,"eq")) {
                buffer = buf_new(ch->lang);
                for (victim = char_list; victim != NULL;
                                victim = victim->next) {
                        if (IS_NPC(victim)
                        || !victim->desc
                        || victim->desc->connected != CON_PLAYING
                        || !can_see(ch,victim))
                                continue;

                        sprintf(buf,
"%3d) %-10s, %2d items(weight %4d) {YHit:%2d, Dam:%2d, {WAC:%3d %3d %3d %3d, {ySav:%d{x\n\r",
                                count, victim->name, victim->carry_number,
                                victim->carry_weight, victim->hitroll,
                                victim->damroll,
                                victim->armor[AC_PIERCE],
                                victim->armor[AC_BASH],
                                victim->armor[AC_SLASH],
                                victim->armor[AC_EXOTIC],
                        buf_add(buffer, buf));
                        count++;
                }
                page_to_char(buf_string(buffer),ch);
                buf_free(buffer);
                return;
        }

        if (!str_prefix(arg,"snoop")) {
                char bufsnoop [100];

                if(ch->level < MAX_LEVEL ) {
                        char_puts("You can't use this check option.\n\r",ch);
                        return;
                }
                buffer = buf_new(ch->lang);

                for (victim = char_list; victim != NULL;
                                victim = victim->next) {
                        if (IS_NPC(victim)
                        || !victim->desc
                        || victim->desc->connected != CON_PLAYING
                        || !can_see(ch,victim))
                                continue;

                        if(victim->desc->snoop_by != NULL)
                                sprintf(bufsnoop," %15s .",
                                 victim->desc->snoop_by->character->name);
                        else
                                sprintf(bufsnoop,"     (none)      ." );

                        sprintf(buf,"%3d %15s : %s \n\r",count,victim->name,
                                        bufsnoop);
                        buf_add(buffer, buf);
                        count++;
                }
                page_to_char(buf_string(buffer),ch);
                buf_free(buffer);
                return;
        }

        char_puts("{CSyntax: {W'check'       {Ydisplay info about players\n\r",ch);
        char_puts("        {W'check stats' {Ydisplay info and resume stats\n\r",ch);
        char_puts("        {W'check eq'    {Yresume eq of all players\n\r",ch);
        char_puts("        {W'check snoop' {Ydisplay info about snoops\n\r",ch);
        char_puts("{RUse the stat command in case of doubt about someone...{x\n\r",
                        ch);
        return;
} */

void do_noexp(CHAR_DATA *ch, const char *argument)
{
        char arg[MAX_INPUT_LENGTH];
        CHAR_DATA *victim;
        //int gain;  
//        int xp; 


        one_argument(argument, arg, sizeof(arg));

        if (arg[0] == '\0') {
                char_puts("Freeze whom?\n", ch);
                return;
        }

        if ((victim = get_char_world(ch, arg)) == NULL) {
                char_puts("They aren't here.\n", ch);
                return;
        }

        if (IS_NPC(victim)) {
                char_puts("Not on NPC's.\n", ch);
                return;
        }


        if (!IS_NPC(victim) && victim->level >= ch->level) {
                char_puts("You failed to remove $i's exp gain.\n", ch);
                return;
        }
        TOGGLE_BIT(victim->state_flags, STATE_NOEXP);
        if (!IS_SET(victim->state_flags, STATE_NOEXP)) {
                char_puts("You can gain experience again.\n", victim);
                char_puts("NOEXP removed.\n", ch);
                wiznet("$N 'gives' $i experience ability back.",
                        ch, victim, WIZ_PENALTIES, 0, 0);
        }
        else {
                char_puts("You can't gain any experience!\n", victim);
                char_puts("NOEXP set.\n", ch);
                wiznet("$N removes $i 's experience ability.",
                        ch, victim, WIZ_PENALTIES, 0, 0);
        }
        save_char_obj(victim, FALSE);
}

void do_peace(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *rch;

	for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
		if (!rch->fighting)
			continue;
		stop_fighting(rch, TRUE);
		if (IS_NPC(rch)) {
			/*
			 * avoid aggressive mobs and hunting mobs attacks
			 */
			AFFECT_DATA af;

			af.where = TO_AFFECTS;
			af.type = gsn_reserved;
			af.level = MAX_LEVEL;
			af.duration = 15;
			af.location = APPLY_NONE;
			af.modifier = 0;
			af.bitvector = AFF_CALM | AFF_SCREAM;
			affect_to_char(rch, &af);
		}
	}

	char_puts("Ok.\n", ch);
}

void do_wizlock(CHAR_DATA *ch, const char *argument)
{
	extern bool wizlock;
	wizlock = !wizlock;

	if (wizlock) {
		wiznet("$N has wizlocked the game.", ch, NULL, 0, 0, 0);
		char_puts("Game wizlocked.\n", ch);
	}
	else {
		wiznet("$N removes wizlock.", ch, NULL, 0, 0, 0);
		char_puts("Game un-wizlocked.\n", ch);
	}
}

/* RT anti-newbie code */
void do_newlock(CHAR_DATA *ch, const char *argument)
{
	extern bool newlock;
	newlock = !newlock;
	
	if (newlock) {
		wiznet("$N locks out new characters.", ch, NULL, 0, 0, 0);
		char_puts("New characters have been locked out.\n", ch);
	}
	else {
		wiznet("$N allows new characters back in.", ch, NULL, 0, 0, 0);
		char_puts("Newlock removed.\n", ch);
	}
}

/* award gives a char specified amount of qp or xp */
#define AWARD_XP	1
#define AWARD_QP	2
#define AWARD_GP	3
#define AWARD_RP	4
#define AWARD_PRAC	5
void do_award(CHAR_DATA *ch, const char *argument)
{
        char arg1[MAX_INPUT_LENGTH];
        char arg2[MAX_INPUT_LENGTH];
        char arg3[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int award_type = 0;
	int value = 0;

        argument = one_argument(argument, arg1, sizeof(arg1));
        argument = one_argument(argument, arg2, sizeof(arg2));
        argument = one_argument(argument, arg3, sizeof(arg3));

        if (arg1[0] == '\0' 
	|| arg2[0] == '\0') {
		do_help(ch, "WIZ AWARD");
                return;
        }

        if ((victim = get_char_world(ch, arg1)) == NULL) {
                char_puts("They aren't here.\n", ch);
                return;
        }

        if (IS_NPC(victim)) {
                char_puts("Not on NPC's.\n", ch);
                return;
        }

	if (!str_prefix(arg3, "experience")
	|| !str_prefix(arg3, "xp")
	|| !str_prefix(arg3, "exp"))
		award_type = AWARD_XP;

	else if (!str_prefix(arg3, "questpoints")
	|| !str_prefix(arg3, "qps"))
		award_type = AWARD_QP;

	else if (!str_prefix(arg3, "gold")
	|| !str_prefix(arg3, "gps"))
		award_type = AWARD_GP;

	else if (!str_prefix(arg3, "roleplay")
	|| !str_prefix(arg3, "rps"))
		award_type = AWARD_RP;

	else if (!str_prefix(arg3, "practices"))
		award_type = AWARD_PRAC;

	if (arg2[0] == '\0' 
	&& award_type == AWARD_RP)
		value = 1;
	else if (arg2[0] == '\0'
        || !is_number(arg2)) {
                char_puts("Value must be numeric.\n", ch);
                return;
        }
	else
		value = atoi(arg2);

	switch (award_type) {
		case AWARD_XP:
			if (value < -2000 || value > 10000) {
				char_puts("Value range is -2000 to 10000.\n", ch);
				return;
			}
			act_puts("$N has awarded you {W$t{x experience points.\n",
				victim, arg2, ch, TO_CHAR, POS_DEAD);
			act_puts("You award {W$t{x experiene points to $N.\n",
				ch, arg2, victim, TO_CHAR, POS_DEAD);
			gain_exp(victim, value);
			break;

		case AWARD_QP:
			if (value < -5000 || value > 2000) {
				char_puts("Value range is -5000 to 2000.\n", ch);
				return;
			}
			if (value < 0 && victim->pcdata->questpoints - value < 0) {
				char_printf(ch, 
					"%s only has %d quest points!\n", 
					victim->name,
					victim->pcdata->questpoints);
				return;
			}
			act_puts("$N has awarded you {W$t{x quest points.\n",
				victim, arg2, ch, TO_CHAR, POS_DEAD);
			act_puts("You awarded {W$t{x quest points to $N.\n",
				ch, arg2, victim, TO_CHAR, POS_DEAD);
			victim->pcdata->questpoints += value;
			break;

		case AWARD_GP:
			if (value < -1000000 || value > 100000) {
				char_puts("Value range is -1000000 to 100000.\n", ch);
				return;
			}
			if (value < 0 && victim->pcdata->bank_g - value < 0) {
				char_printf(ch, 
					"%s only has %d gold in the bank!\n", 
					victim->name,
					victim->pcdata->bank_g);
				return;
			}
			act_puts("$N has transfered {W$t{x gold pieces into your bank account.\n",
				victim, arg2, ch, TO_CHAR, POS_DEAD);
			act_puts("You transfer {W$t{x gold pieces into $N's bank account.\n",
				ch, arg2, victim, TO_CHAR, POS_DEAD);
			victim->pcdata->bank_g += value;
			break;

		case AWARD_RP:
			if (value < -100 || value > 10) {
				char_puts("Value range is -100 to 10.\n", ch);
				return;
			}
			char_puts("Not yet implemented.\n", ch);
			return;
			break;

		case AWARD_PRAC:
			if (value < -100 || value > 25) {
				char_puts("Value range is -100 to 25.\n", ch);
				return;
			}
			if (value < 0 && victim->practice < 0) {
				char_printf(ch, 
					"%s only has %d practices!\n", 
					victim->name,
					victim->practice);
				return;
			}
			act_puts("$N has awarded you {W$t{x practice points.\n",
				victim, arg2, ch, TO_CHAR, POS_DEAD);
			act_puts("You awarded {W$t{x practice points to $N.\n",
				ch, arg2, victim, TO_CHAR, POS_DEAD);
			victim->practice += value;
			break;
		default:
			char_puts("Awarding that is not an option.\n", ch);
			return;
	}
}

/* RT set replaces sset, mset, oset, and rset */
void do_set(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("Syntax:\n",ch);
		char_puts("  set mob   <name> <field> <value>\n",ch);
		char_puts("  set obj   <name> <field> <value>\n",ch);
		char_puts("  set room  <room> <field> <value>\n",ch);
		char_puts("  set skill <name> <spell or skill> <value>\n",ch);
		return;
	}

	if (!str_prefix(arg,"mobile") || !str_prefix(arg,"character")) {
		do_mset(ch,argument);
		return;
	}

	if (!str_prefix(arg,"skill") || !str_prefix(arg,"spell")) {
		do_sset(ch,argument);
		return;
	}

	if (!str_prefix(arg,"object")) {
		do_oset(ch,argument);
		return;

	}

	if (!str_prefix(arg,"room")) {
		do_rset(ch,argument);
		return;
	}
	/* echo syntax */
	do_set(ch,str_empty);
}

void do_sset(CHAR_DATA *ch, const char *argument)
{
	char arg1 [MAX_INPUT_LENGTH];
	char arg2 [MAX_INPUT_LENGTH];
	char arg3 [MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int value;
	int sn;
	bool fAll;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
	argument = one_argument(argument, arg3, sizeof(arg3));

	if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
		char_puts("Syntax:\n",ch);
		char_puts("  set skill <name> <spell or skill> <value>\n", ch);
		char_puts("  set skill <name> all <value>\n",ch);  
		char_puts("(use the name of the skill, not the number)\n",ch);
		return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (IS_NPC(victim)) {
		char_puts("Not on NPC's.\n", ch);
		return;
	}

	fAll = !str_cmp(arg2, "all");
	sn   = 0;
	if (!fAll && (sn = sn_lookup(arg2)) < 0) {
		char_puts("No such skill or spell.\n", ch);
		return;
	}

	/*
	 * Snarf the value.
	 */
	if (!is_number(arg3)) {
		char_puts("Value must be numeric.\n", ch);
		return;
	}

	value = atoi(arg3);
	if (value < 0 || value > 100) {
		char_puts("Value range is 0 to 100.\n", ch);
		return;
	}

	if (fAll)
		for (sn = 0; sn < skills.nused; sn++)
			set_skill(victim, sn, 100);
	else
		set_skill(victim, sn, value);
	char_puts("Ok.\n", ch);
}

void do_string(CHAR_DATA *ch, const char *argument)
{
	char type[MAX_INPUT_LENGTH];
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *obj;

	argument = one_argument(argument, type, sizeof(type));
	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
	strnzcpy(arg3, sizeof(arg3), argument);

	if (type[0] == '\0' || arg1[0] == '\0'
	||  arg2[0] == '\0' || arg3[0] == '\0') {
		char_puts("Syntax:\n",ch);
		char_puts("  string char <name> <field> <string>\n",ch);
		char_puts("    fields: name short long desc title spec\n",ch);
		char_puts("  string obj  <name> <field> <string>\n",ch);
		char_puts("    fields: name short long extended\n",ch);
		return;
	}
	
	if (!str_prefix(type, "character") || !str_prefix(type, "mobile")) {
		if ((victim = get_char_room(ch, arg1)) == NULL) {
			char_puts("They aren't here.\n", ch);
			return;
		}

		/* clear zone for mobs */
		victim->zone = NULL;

		/* string something */

	 	if (!str_prefix(arg2, "name")) {
			if (!IS_NPC(victim)) {
				char_puts("Not on PC's.\n", ch);
				return;
			}
			free_string(victim->name);
			victim->name = str_dup(arg3);
			return;
		}
		
		if (!str_prefix(arg2, "short")) {
			if (!IS_NPC(victim)) {
				char_puts(" Not on PC's.\n", ch);
				return;
			}
			mlstr_edit(&victim->short_descr, arg3);
			return;
		}

		if (!str_prefix(arg2, "desc")) {
			mlstr_append(ch, &victim->description, arg3);
			return;
		}

		if (!str_prefix(arg2, "long")) {
			if (!IS_NPC(victim)) {
				char_puts("Not on PC's.\n", ch);
				return;
			}
			mlstr_editnl(&victim->long_descr, arg3);
			return;
		}

		if (!str_prefix(arg2, "title")) {
			if (IS_NPC(victim)) {
				char_puts("Not on NPC's.\n", ch);
				return;
			}

			if (victim->level != LEVEL_HERO && ch->level < L1) {
				char_puts("You don't have the power to set that title.\n", ch);
				return;
			}

			if (set_title(victim, arg3)) {
				char_printf(ch, "Title set: %s%s",
					victim->name,
					victim->pcdata->title);
			}
			else {
				char_printf(ch, 
					"The title was over the maximum of %d visible characters.",
					MAX_TITLE_LENGTH);
			}
			return;
		}
	}
	
	if (!str_prefix(type,"object")) {
		/* string an obj */
		
	 	if ((obj = get_obj_room(ch, arg1)) == NULL) {
			char_puts("Nothing like that in heaven or earth.\n",
				  ch);
			return;
		}
		
		if (obj->pIndexData->limit >= 0) {
			char_puts("You cannot string limited objs.\n", ch);
			return;
		}

		if (!str_prefix(arg2, "name")) {
			free_string(obj->name);
			obj->name = str_dup(arg3);
			return;
		}

		if (!str_prefix(arg2, "short")) {
			mlstr_edit(&obj->short_descr, arg3);
			return;
		}

		if (!str_prefix(arg2, "long")) {
			mlstr_edit(&obj->description, arg3);
			return;
		}

		if (!str_prefix(arg2, "ed")
		||  !str_prefix(arg2, "extended")
		||  !str_prefix(arg2, "exd")) {
			ED_DATA *ed;

			if (obj->carried_by != ch) {
				char_puts("Obj must be in your inventory.\n", ch);
				return;
			}

			argument = one_argument(argument, arg3, sizeof(arg3));
			if (argument == NULL) {
				char_puts("Syntax: oset <object> ed <keyword> "
					  "lang\n", ch);
				return;
			}

			ed = ed_new();
			ed->keyword	= str_dup(arg3);
			ed->next	= obj->ed;
			mlstr_append(ch, &ed->description, argument);
			obj->ed	= ed;
			return;
		}
	}
	
	/* echo bad use message */
	do_string(ch,str_empty);
}

void do_oset(CHAR_DATA *ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_STRING_LENGTH];
	OBJ_DATA *obj;
	int value;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
		   one_argument(argument, arg3, sizeof(arg3));

	if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
		char_puts("Syntax:\n",ch);
		char_puts("  set obj <object> <field> <value>\n",ch);
		char_puts("Field being one of:\n", ch);
		char_puts("value0 value1 value2 value3 value4 (v1-v4)\n", ch);
		char_puts("owner level cost timer\n",	ch);
		char_puts("material matdesc\n",	ch);
		return;
	}

	if ((obj = get_obj_world(ch, arg1)) == NULL) {
		char_puts("Nothing like that in heaven or earth.\n", ch);
		return;
	}

	value = atoi(arg3);

	/*
	 * Set something.
	 */
	if (!str_cmp(arg2, "value0") || !str_cmp(arg2, "v0")) {
		obj->value[0] = UMIN(50,value);
		return;
	}

	if (!str_cmp(arg2, "value1") || !str_cmp(arg2, "v1")) {
		obj->value[1] = value;
		return;
	}

	if (!str_cmp(arg2, "value2") || !str_cmp(arg2, "v2")) {
		obj->value[2] = value;
		return;
	}

	if (!str_cmp(arg2, "value3") || !str_cmp(arg2, "v3")) {
		obj->value[3] = value;
		return;
	}

	if (!str_cmp(arg2, "value4") || !str_cmp(arg2, "v4")) {
		obj->value[4] = value;
		return;
	}

	if (!str_prefix(arg2, "level")) {
		obj->level = value;
		return;
	}
		
	if (!str_prefix(arg2, "cost")) {
		obj->cost = value;
		return;
	}

	if (!str_prefix(arg2, "timer")) {
		obj->timer = value;
		return;
	}
		
	if (!str_prefix(arg2, "owner")) {
		mlstr_free(obj->owner);
		obj->owner = mlstr_new(arg3);
		return;
	}

	if (!str_prefix(arg2, "material")) {
		if ((value = material_lookup_name(arg3)) > 0) {
			obj->material = MATERIAL(value);
			char_printf(ch, "Material set to '%s'.\n",
				arg3);
		}
		else {
			char_printf(ch, "Couldn't find material '%s'.\n",
				arg3);
		}
		return;
	}


	/*
	 * Generate usage message.
	 */
	do_oset(ch, str_empty);
	return;
}

void do_rset(CHAR_DATA *ch, const char *argument)
{
	char arg1 [MAX_INPUT_LENGTH];
	char arg2 [MAX_INPUT_LENGTH];
	char arg3 [MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *location;
	int value;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
		   one_argument(argument, arg3, sizeof(arg3));

	if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
		char_puts("Syntax:\n",ch);
		char_puts("  set room <location> <field> <value>\n",ch);
		char_puts("  Field being one of:\n",			ch);
		char_puts("    flags sector\n",				ch);
		return;
	}

	if ((location = find_location(ch, arg1)) == NULL) {
		char_puts("No such location.\n", ch);
		return;
	}

	if (ch->in_room != location 
	&&  room_is_private(location) && !IS_TRUSTED(ch,IMPLEMENTOR)) {
	    char_puts("That room is private right now.\n",ch);
	    return;
	}

	/*
	 * Snarf the value.
	 */
	if (!is_number(arg3)) {
		char_puts("Value must be numeric.\n", ch);
		return;
	}
	value = atoi(arg3);

	/*
	 * Set something.
	 */
	if (!str_prefix(arg2, "flags")) {
		location->room_flags	= value;
		return;
	}

	if (!str_prefix(arg2, "sector")) {
		location->sector_type	= value;
		return;
	}

	/*
	 * Generate usage message.
	 */
	do_rset(ch, str_empty);
	return;
}

/*
 * 7 STAT Login Idl Name    Host
 *
 */
void do_sockets( CHAR_DATA *ch, const char *argument )
{
	BUFFER *output;
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	DESCRIPTOR_DATA *d;
	char *st = NULL;
	int count;

	count = 0;
	output = buf_new(-1);

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0')
		buf_printf(output, 
		"\n{D[{gNum {GState {gLogin {GIdl{D] {GPlayer     {gHost{x\n"
		"{D-------------------------------------------------{x\n");

	for (d = descriptor_list; d; d = d->next) {
		CHAR_DATA *vch = d->original ? d->original : d->character;

		if (vch) {
			if (!can_see(ch, vch)
			||  (arg[0] && str_prefix(arg, vch->name)))
				continue;
		}
		else if (arg[0])
			continue;

		count++;
		buf_printf(output, "{D[{y%3d ", d->descriptor);

		switch( d->connected ) {
		case CON_RESOLV:               st = "{Dresol";  break;
		case CON_BREAK_CONNECT:	       st = "{DLINKD"; 	break;
		case CON_PLAYING:              st = "{cPLAY ";  break;
		case CON_DUMMY:                st = "{Ddummy";  break;
		case CON_GET_NAME:             st = "{cName?";  break;
		case CON_CONFIRM_NEW_NAME:     st = "{cConNm";  break;
		case CON_GET_OLD_PASSWORD:     st = "{cOldPw";  break;
		case CON_GET_NEW_PASSWORD:     st = "{cNewPw";  break;
		case CON_CONFIRM_NEW_PASSWORD: st = "{cConPw";  break;
		case CON_ANSI_DETECTOR:        st = "{cAnsi?";  break;
		case CON_NEWBIE_DETECTOR:      st = "{cNewb?";  break;
		case CON_NEWBIE_PROTECTION:    st = "{cnProt";  break;
		case CON_TRUE_LIFER:	       st = "{cTLife";	break;
		case CON_GET_NEW_RACE:         st = "{cRace?";  break;
		case CON_GET_NEW_SEX:          st = "{cSex? ";  break;
		case CON_GET_NEW_CLASS:        st = "{cClass";  break;
		case CON_GET_ETHOS:            st = "{cEthos";  break;
		case CON_PICK_HOMETOWN:        st = "{cHome?";  break;
		case CON_ACCEPT_STATS:
		case CON_ROLL_STATS:	       st = "{cRoll ";  break;
		case CON_GET_ALIGNMENT:        st = "{cAlign";	break;
		case CON_DEFAULT_CHOICE:       st = "{cCust?";	break;
		case CON_GEN_GROUPS:	       st = "{cGCust";	break;
		case CON_PICK_WEAPON:	       st = "{cWepon";	break;
		case CON_CREATE_DONE:	       st = "{ccDone";  break;
		case CON_READ_IMOTD:	       st = "{cIMOTD"; 	break;
		case CON_READ_MOTD:            st = "{cMOTD ";  break;
		default:                       st = "{c ??? ";  break;
		}
		buf_printf(output, "%s ", st);

		if (vch) {
			strftime(buf, sizeof(buf), 
				"{C%H:%M", localtime(&vch->logon));
			buf_printf(output, "%s", buf);
		}
		else
			buf_printf(output, "--:--");

		if (vch && vch->timer > 0)
			buf_printf(output, " {y%3d{D]", vch->timer );
		else
			buf_printf(output, "    {D]");

		buf_printf(output,
			" {w%-10s", vch ? vch->name : "(none)");

		buf_printf(output,
			" {%c%s{x",
			(vch && vch->pcdata->fake_ip
			 && IS_TRUSTED_IMP(ch)) ? 'y' : 'c',
			(vch && vch->pcdata->fake_ip) ? vch->pcdata->fake_ip
				: d->host);

		buf_add(output, "\n");
	}

	if (count == 0) {
		char_puts("No one by that name is connected.\n",ch);
		buf_free(output);
		return;
	}

	buf_printf(output, "%d user%s\n",
		   count, count == 1 ? str_empty : "s");
	page_to_char(buf_string(output), ch);
	buf_free(output);
}

void do_sockets_old( CHAR_DATA *ch, const char *argument )
{
    CHAR_DATA       *vch;
    DESCRIPTOR_DATA *d;
    char            buf  [ MAX_STRING_LENGTH ];
    char            buf2 [ MAX_STRING_LENGTH ];
    int             count;
    char *          st;
    char            s[100];
    char            idle[10];


    count       = 0;
    buf[0]      = '\0';
    buf2[0]     = '\0';

    strcat( buf2, "\n\r[{BNum {YConnected_State {CLogin@ {yIdl{x] {RPlayer Name{MHost{x\n\r" );
    strcat( buf2,
"{C------------------------------------------------------------------------------{x\n");  
    for ( d = descriptor_list; d; d = d->next )
    {
        if ( d->character && can_see( ch, d->character ) )
        {
           /* NB: You may need to edit the CON_ values */
           switch( d->connected )
           {
              case CON_PLAYING:              st = "   {YPLAYING     ";  break;
              case CON_DUMMY:                st = "{YDUMMY - pload  ";  break;
              case CON_GET_NAME:             st = "{YGet Name       ";  break;
              case CON_GET_OLD_PASSWORD:     st = "{YGet Old Passwd ";  break;
              case CON_CONFIRM_NEW_NAME:     st = "{YConfirm Name   ";  break;
              case CON_GET_NEW_PASSWORD:     st = "{YGet New Passwd ";  break;
              case CON_CONFIRM_NEW_PASSWORD: st = "{YConfirm Passwd ";  break;
              case CON_ANSI_DETECTOR:        st = "{YANSI?          ";  break;
              case CON_NEWBIE_DETECTOR:      st = "{YNewbie?        ";  break;
	      case CON_NEWBIE_PROTECTION:    st = "{YNewbie Protect?";  break;
	      case CON_TRUE_LIFER:	     st = "{YTrue Lifer     "; 	break;
              case CON_GET_NEW_RACE:         st = "{YGet New Race   ";  break;
              case CON_GET_NEW_SEX:          st = "{YGet New Sex    ";  break;
              case CON_GET_NEW_CLASS:        st = "{YGet New Class  ";  break;
	      case CON_ACCEPT_STATS:
	      case CON_ROLL_STATS:	     st = "{YRolling Stats  "; 	break;
              case CON_GET_ALIGNMENT:        st = "{YGet New Align  ";	break;
              case CON_DEFAULT_CHOICE:	     st = "{YChoosing Cust  ";	break;
              case CON_GEN_GROUPS:	     st = "{YCustomization  ";	break;
              case CON_PICK_WEAPON:	     st = "{YPicking Weapon ";	break;
	      case CON_READ_IMOTD:	     st = "{YReading IMOTD  "; 	break;
	      case CON_BREAK_CONNECT:	     st = "{DLINKDEAD       "; 	break;
              case CON_READ_MOTD:            st = "{YReading MOTD   ";  break;
              default:                       st = "{B!UNKNOWN!      ";  break;
           }
           count++;
           
           /* Format "login" value... */
           vch = d->original ? d->original : d->character;
           strftime( s, 100, "{C%I:%M%p{x", localtime( &vch->logon ) );
           
           if ( vch->timer > 0 )
              sprintf( idle, "{y%-2d{x", vch->timer );
           else
              sprintf( idle, "{y  {x" );
           
           sprintf( buf, "[{y%3d %15s %7s %2s{x] {R%-10s {M%-70.70s{x\n\r",
              d->descriptor,
              st,
              s,
              idle,
              ( d->original ) ? d->original->name
                              : ( d->character )  ? d->character->name
                                                  : "{R(None!){x",
              d->host );
              
           strcat( buf2, buf );

        }
    }

    sprintf( buf, "{M\n\r%d user%s\n\r{x", count, count == 1 ? "" : "s" );
    strcat( buf2, buf );
    send_to_char( buf2, ch );
    return;
}

void do_osockets(CHAR_DATA *ch, const char *argument)
{
	BUFFER *output;
	char arg[MAX_INPUT_LENGTH];
	DESCRIPTOR_DATA *d;
	int count;

	count = 0;
	output = buf_new(-1);

	one_argument(argument, arg, sizeof(arg));
	for (d = descriptor_list; d; d = d->next) {
		CHAR_DATA *vch = d->original ? d->original : d->character;

		if (vch) {
			if (!can_see(ch, vch)
			||  (arg[0] && !is_name(arg, vch->name)))
				continue;
		}
		else if (arg[0])
			continue;

		count++;
		buf_printf(output, "[%3d %2d] %s@%s",
			   d->descriptor,
			   d->connected,
			   vch ? vch->name : "(none)",
			   (vch && vch->pcdata->fake_ip)
			   	? vch->pcdata->fake_ip
				: d->host);
		if (vch && vch->timer)
			buf_printf(output, " (idle %d)", vch->timer);
		buf_add(output, "\n");
	}

	if (count == 0) {
		char_puts("No one by that name is connected.\n",ch);
		buf_free(output);
		return;
	}

	buf_printf(output, "%d user%s\n",
		   count, count == 1 ? str_empty : "s");
	page_to_char(buf_string(output), ch);
	buf_free(output);
}

/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0' || argument[0] == '\0') {
		char_puts("Force whom to do what?\n", ch);
		return;
	}

	one_argument(argument, arg2, sizeof(arg2));
	
	if (!str_cmp(arg2,"delete") || !str_prefix(arg2,"mob")) {
		char_puts("That will NOT be done.\n",ch);
		return;
	}


	if (!str_cmp(arg, "all")) {
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;

		if (ch->level < MAX_LEVEL - 3) {
		    char_puts("Not at your level!\n",ch);
		    return;
		}

		for (vch = char_list; vch && !IS_NPC(vch); vch = vch_next) {
		    vch_next = vch->next;

		    if (vch->level < ch->level) {
			act_puts("$n forces you to '$t'.",
				 ch, argument, vch, TO_VICT, POS_DEAD);
			interpret_raw(vch, argument, TRUE, ch->level);
		    }
		}
	} else if (!str_cmp(arg,"players")) {
	    CHAR_DATA *vch;
	    CHAR_DATA *vch_next;
	
	    if (ch->level < MAX_LEVEL - 2) {
	        char_puts("Not at your level!\n",ch);
	        return;
	    }
	
	    for (vch = char_list; vch && !IS_NPC(vch); vch = vch_next) {
	        vch_next = vch->next;
	
	        if (vch->level < ch->level && !IS_IMMORTAL(vch)) {
			act_puts("$n forces you to '$t'.",
				 ch, argument, vch, TO_VICT, POS_DEAD);
	            interpret_raw(vch, argument, TRUE, ch->level);
	        }
	    }
	} else if (!str_cmp(arg,"gods")) {
	    CHAR_DATA *vch;
	    CHAR_DATA *vch_next;
	
	    if (ch->level < MAX_LEVEL - 2) {
	        char_puts("Not at your level!\n",ch);
	        return;
	    }
	
	    for (vch = char_list; vch && !IS_NPC(vch); vch = vch_next) {
	        vch_next = vch->next;
	
	        if (vch->level < ch->level && IS_IMMORTAL(vch)) {
			act_puts("$n forces you to '$t'.",
				 ch, argument, vch, TO_VICT, POS_DEAD);
	            interpret_raw(vch, argument, TRUE, ch->level);
	        }
	    }
	} else {
		CHAR_DATA *victim;

		if ((victim = get_char_world(ch, arg)) == NULL)
		{
		    char_puts("They aren't here.\n", ch);
		    return;
		}

		if (victim == ch)
		{
		    char_puts("Aye aye, right away!\n", ch);
		    return;
		}

		if (!is_room_owner(ch,victim->in_room) 
		&&  ch->in_room != victim->in_room 
	    &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
		{
	        char_puts("That character is in a private room.\n",ch);
	        return;
	    }

		if (!IS_NPC(victim)) {
			if (victim->level >= ch->level) {
				char_puts("Do it yourself!\n", ch);
				return;
			}

			if (ch->level < MAX_LEVEL -3) {
				char_puts("Not at your level!\n",ch);
				return;
			}
		}

		act_puts("$n forces you to '$t'.",
			 ch, argument, victim, TO_VICT, POS_DEAD);
		interpret(victim, argument);
	}

	char_puts("Ok.\n", ch);
}

/*
 * force helpfiles on people
 */
void do_inform(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	char cmd[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0' || argument[0] == '\0') {
		char_puts("Inform whom about what?\n", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
	    char_puts("They aren't here.\n", ch);
	    return;
	}

	if (victim == ch) {
	    char_puts("Uhm, yeah.\n", ch);
	    return;
	}

	if (!IS_NPC(victim)) {
		if (victim->level >= ch->level) {
			char_puts("Do it yourself!\n", ch);
			return;
		}
		snprintf(cmd, sizeof(cmd),
			"help %s", argument);
		act_puts("$n forces you to '{c$t{x'.",
			 ch, cmd, victim, TO_VICT, POS_DEAD);
		interpret(victim, cmd);
	}
	char_puts("Ok.\n", ch);
}

/*
 * New routines by Dionysos.
 */
void do_invis(CHAR_DATA *ch, const char *argument)
{
	int level;
	char arg[MAX_STRING_LENGTH];

	/* RT code for taking a level argument */
	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0')
	/* take the default path */

	  if (ch->invis_level)
	  {
		  ch->invis_level = 0;
		  act("$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM);
		  char_puts("You slowly fade back into existence.\n", ch);
	  }
	  else
	  {
		  ch->invis_level = LEVEL_IMMORTAL;
		  act("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM);
		  char_puts("You slowly vanish into thin air.\n", ch);
	  }
	else
	/* do the level thing */
	{
	  level = atoi(arg);
	  if (ch->pcdata->security < 9) {
//		char_puts("LOL. NO SECURITY.\n",ch);
		if ((level < 2) || (level > ch->level))
		  {
		  	char_puts("Invis level must be between 2 and your level.\n",ch);
	    		return;
	  	  } else {
			  ch->reply = NULL;
		      ch->invis_level = level;
		      act("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM);
		      char_puts("You slowly vanish into thin air.\n", ch);
	  	  }

	  } else {
//		char_puts("WoooW. You've got security.\n",ch);
		if ((level < 2) || (level > MAX_LEVEL))
		  {
		  	char_printf(ch, "Invis level must be between 2 and %d for your security.\n", MAX_LEVEL);
	    		return;
	  	  } else {
			  ch->reply = NULL;
		      ch->invis_level = level;
		      act("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM);
		      char_puts("You slowly vanish into thin air.\n", ch);
		      char_printf(ch, "You are now Wizi %d\n", level);
	  	  }

	  }
	 }

}


void do_incognito(CHAR_DATA *ch, const char *argument)
{
	int level;
	char arg[MAX_STRING_LENGTH];
	
	/* RT code for taking a level argument */
	one_argument(argument, arg, sizeof(arg));
	
	if (arg[0] == '\0')
	/* take the default path */
	
	  if (ch->incog_level)
	  {
	      ch->incog_level = 0;
	      act("$n is no longer cloaked.", ch, NULL, NULL, TO_ROOM);
	      char_puts("You are no longer cloaked.\n", ch);
	  }
	  else
	  {
	      ch->incog_level = LEVEL_IMMORTAL;
	      act("$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM);
	      char_puts("You cloak your presence.\n", ch);
	  }
	else
	/* do the level thing */
	{
	  level = atoi(arg);
	  if (level < 2 || level > ch->level)
	  {
	    char_puts("Incog level must be between 2 and your level.\n",ch);
	    return;
	  }
	  else
	  {
	      ch->reply = NULL;
	      ch->incog_level = level;
	      act("$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM);
	      char_puts("You cloak your presence.\n", ch);
	  }
	}
}

void do_holylight(CHAR_DATA *ch, const char *argument)
{
	if (IS_NPC(ch))
		return;

	TOGGLE_BIT(ch->conf_flags, PLR_CONF_HOLYLIGHT);
	char_printf(ch, "Holy light mode %s.\n",
		    IS_SET(ch->conf_flags, PLR_CONF_HOLYLIGHT) ? "on" : "off");
}

/*
 * anoncolor - anonymous colors for imms so morts can
 * 	descern if several imms are taking part in 
 * 	the same conversation.
 *
 * by Zsuzsu
 */
void do_anoncolor(CHAR_DATA *ch, const char *argument)
{
	char buf[] = "rmgybmcRGYBMC";
	char *p = NULL;
	CHAR_DATA *wch;
	int i=0, j=0;
	char old_color = ch->pcdata->anon_color;
	char arg[MAX_STRING_LENGTH];
	
	one_argument(argument, arg, sizeof(arg));

	/* change color */
	if (old_color == 0 
	|| (!str_prefix(arg, "set") && ch->level >= L1)) {
		/* NOTE: allowing IMMs to set to a specific color,
		 * 	rather than a random one, will defeat the 
		 * 	purpose of this feature.  People will always 
		 * 	tend to favor certain colors and will 
		 * 	consistantly choose them, thus compromising
		 * 	the anonimity.
		 */
		for (wch = char_list; wch && !IS_NPC(wch); wch = wch->next) {
			if (IS_IMMORTAL(wch))
				if (wch->pcdata->anon_color) {
					p = strchr(buf, wch->pcdata->anon_color);
					*p= '-';
				}
		}

		for (i=0; i < sizeof(buf); i++) {
			if (buf[i] != '-')
				buf[j++] = buf[i];
		}
		buf[j] = '\0';

		ch->pcdata->anon_color = buf[number_range(0, strlen(buf)-1)];

		if (old_color) {
			log_printf("%s's anon color was '%s' is now '%s'", 
				ch->name,
				color_name(old_color),
				color_name(ch->pcdata->anon_color));
			char_printf(ch, 
				"Your anonymous color was '{%c%s{x' but is now '{%c%s{x.'\n", 
				old_color,
				color_name(old_color),
				ch->pcdata->anon_color,
				color_name(ch->pcdata->anon_color));
		}
		else {
			log_printf("%s's anon color is now '%s'", 
				ch->name,
				color_name(ch->pcdata->anon_color));
			char_printf(ch, "Your anonymous color is set to {%c%s{x.\n",
				ch->pcdata->anon_color,
				color_name(ch->pcdata->anon_color));
		}
	}
	/* list who has what color */
	else if (arg[0] == '\0' || !str_prefix(arg, "list")) {
		char_puts("Current anonymous immortal colors:\n", ch);
		for (wch = char_list; wch && !IS_NPC(wch); wch = wch->next) {
			if (IS_IMMORTAL(wch) && can_see(ch, wch))
				char_printf(ch, "%20s : {%c%s{x\n",
					wch->name,
					wch->pcdata->anon_color
						? wch->pcdata->anon_color
						: 'x',
					wch->pcdata->anon_color
						? color_name(wch->pcdata->anon_color)
						: "NONE");
		}
	}

	else {
		char_puts("Syntax: anoncolor [list|set]", ch);
	}
}

/* prefix command: it will put the string typed on each line typed */

DO_FUN(do_prefi)
{
	char_puts("You cannot abbreviate the prefix command.\n", ch);
}

DO_FUN(do_prefix)
{
	if (argument[0] == '\0') {
		if (ch->prefix[0] == '\0') {
			char_puts("You have no prefix to clear.\n",ch);
			return;
		}

		char_puts("Prefix removed.\n",ch);
		free_string(ch->prefix);
		ch->prefix = str_empty;
		return;
	}

	free_string(ch->prefix);
	ch->prefix = str_dup(argument);
	char_printf(ch, "Prefix set to '%s'.\n", argument);
}

void advance(CHAR_DATA *victim, int level)
{
	int iLevel;
	int tra;
	int pra;
	race_t *r = race_lookup(victim->race);

	tra = victim->train;
	pra = victim->practice;
	victim->pcdata->plevels = 0;

	/*
	 * Lower level:
	 *   Reset to level 1.
	 *   Then raise again.
	 *   Currently, an imp can lower another imp.
	 *   -- Swiftest
	 */
	if (level <= victim->level) {
		int temp_prac;

		char_puts("**** OOOOHHHHHHHHHH  NNNNOOOO ****\n", victim);
		temp_prac = victim->practice;
		victim->level		= 1;
		victim->exp		= base_exp(victim);
		victim->max_hit		= CHAR_CREATE_INITIAL_HP
				+ r->pcdata->hp_bonus;

		victim->max_mana	= CHAR_CREATE_INITIAL_MANA
				+ r->pcdata->mana_bonus
				+ (victim->perm_stat[STAT_INT] * 2/ 3 
				+ victim->perm_stat[STAT_WIS] /2);

		victim->max_move	= CHAR_CREATE_INITIAL_MOVE
				+ ((victim->perm_stat[STAT_CON]/5
				+ victim->perm_stat[STAT_DEX]/5) * 5);

		victim->practice	= 0;
		victim->hit		= victim->max_hit;
		victim->mana		= victim->max_mana;
		victim->move		= victim->max_move;
		victim->pcdata->perm_hit	= victim->max_hit;
		victim->pcdata->perm_mana	= victim->max_mana;
		victim->pcdata->perm_move	= victim->max_move;
		victim->pcdata->perm_dam	= 0;
		victim->pcdata->perm_nim	= 0;
		advance_level(victim);
		victim->practice	= temp_prac;
	}
	else 
		char_puts("**** OOOOHHHHHHHHHH  YYYYEEEESSS ****\n", victim);

	for (iLevel = victim->level; iLevel < level; iLevel++) {
		char_puts("{CYou raise a level!!{x ", victim);
		victim->exp += exp_to_level(victim);
		victim->level++;
		advance_level(victim);
	}
	victim->exp_tl		= 0;
	victim->train		= tra;
	victim->practice	= pra;
	save_char_obj(victim, FALSE);
}

void do_advance(CHAR_DATA *ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int level;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number(arg2)) {
		char_puts("Syntax: advance <char> <level>.\n", ch);
		return;
	}

	if ((victim = get_char_room(ch, arg1)) == NULL) {
		char_puts("That player is not here.\n", ch);
		return;
	}

	if (IS_NPC(victim)) {
		char_puts("Not on NPC's.\n", ch);
		return;
	}

	if ((level = atoi(arg2)) < 1 || level > MAX_LEVEL) {
		char_printf(ch, "Level must be 1 to %d.\n", MAX_LEVEL);
		return;
	}

	if (level > ch->level) {
		char_puts("Limited to your level.\n", ch);
		return;
	}

	advance(victim, level);
}

void do_mset(CHAR_DATA *ch, const char *argument)
{
	char arg1 [MAX_INPUT_LENGTH];
	char arg2 [MAX_INPUT_LENGTH];
	char arg3 [MAX_INPUT_LENGTH];
	char arg4 [MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int value, val2;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
	argument = one_argument(argument, arg3, sizeof(arg3));
		   one_argument(argument, arg4, sizeof(arg4));

	if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
		char_puts("Syntax:\n",ch);
		char_puts("  set char <name> <field> <value>\n",ch); 
		char_puts("  Field being one of:\n",			ch);
		char_puts("    str int wis dex con cha sex class level\n",ch);
		char_puts("    race align ethos practice train gold\n", ch);
		char_puts("    hp mana move thirst drunk full bloodlust desire\n", ch);
		char_puts("    hometown clan relig trouble questp questt\n", ch);
		char_puts("    noghost newbie multikiller security pkok\n", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	/*
	 * Snarf the value (which need not be numeric).
	 */
	value = is_number(arg3) ? atoi(arg3) : -1;
	val2  = is_number(arg4) ? atoi(arg4) : -1;

	/*
	 * Set something.
	 */
	if (!str_cmp(arg2, "str")) {
		if (value < 3 || value > get_max_train(victim,STAT_STR))
		{
		    char_printf(ch,
			"Strength range is 3 to %d\n.",
			get_max_train(victim,STAT_STR));
		    return;
		}

		victim->perm_stat[STAT_STR] = value;
		return;
	}

	if (!str_cmp(arg2, "trouble")) {
		if (IS_NPC(victim)) {
			char_puts("Not on NPC's.\n", ch);
			return;
		}
		
		if (value == -1 || val2 == -1) {
			char_puts("Usage: set char <name> trouble "
				  "<vnum> <value>.\n", ch);
			return;
		}

		qtrouble_set(victim, value, val2+1);
		char_puts("Ok.\n", ch);
		return;
	}

	if (!str_cmp(arg2, "security"))	{ /* OLC */
		if (IS_NPC(ch)) {
			char_puts("Si, claro.\n", ch);
			return;
		}

		if (IS_NPC(victim)) {
			char_puts("Not on NPC's.\n", ch);
			return;
		}

		if (value > ch->pcdata->security || value < 0) {
			if (ch->pcdata->security != 0)
				char_printf(ch, "Valid security is 0-%d.\n",
					    ch->pcdata->security);
			else
				char_puts("Valid security is 0 only.\n", ch);
			return;
		}
		victim->pcdata->security = value;
		return;
	}

	if (!str_cmp(arg2, "int"))
	{
	    if (value < 3 || value > get_max_train(victim,STAT_INT))
	    {
	        char_printf(ch, "Intelligence range is 3 to %d.\n",
			get_max_train(victim,STAT_INT));
	        return;
	    }
	
	    victim->perm_stat[STAT_INT] = value;
	    return;
	}

	if (!str_cmp(arg2, "wis"))
	{
		if (value < 3 || value > get_max_train(victim,STAT_WIS))
		{
		    char_printf(ch,
			"Wisdom range is 3 to %d.\n",get_max_train(victim,STAT_WIS));
		    return;
		}

		victim->perm_stat[STAT_WIS] = value;
		return;
	}
	if (!str_cmp(arg2, "questp"))
	{
		 if (value == -1) value = 0;
		 if (!IS_NPC(victim)) victim->pcdata->questpoints = value;
		return;
	}
	if (!str_cmp(arg2, "questt"))
	{
		 if (value == -1) value = 30;
		 if (!IS_NPC(victim)) victim->pcdata->questtime = value;
		return;
	}
	if (!str_cmp(arg2, "relig"))
	{
		 if (value == -1) value = 0;
		 victim->religion = value;
		return;
	}


	if (!str_cmp(arg2, "dex"))
	{
		if (value < 3 || value > get_max_train(victim,STAT_DEX))
		{
		    char_printf(ch,
			"Dexterity ranges is 3 to %d.\n",
			get_max_train(victim,STAT_DEX));
		    return;
		}

		victim->perm_stat[STAT_DEX] = value;
		return;
	}

	if (!str_cmp(arg2, "con"))
	{
		if (value < 3 || value > get_max_train(victim,STAT_CON))
		{
		    char_printf(ch,
			"Constitution range is 3 to %d.\n",
			get_max_train(victim,STAT_CON));
		    return;
		}

		victim->perm_stat[STAT_CON] = value;
		return;
	}
	if (!str_cmp(arg2, "cha"))
	{
		if (value < 3 || value > get_max_train(victim,STAT_CHA))
		{
		    char_printf(ch,
			"Constitution range is 3 to %d.\n",
			get_max_train(victim,STAT_CHA));
		    return;
		}

		victim->perm_stat[STAT_CHA] = value;
		return;
	}

	if (!str_prefix(arg2, "sex"))
	{
		if (value < 0 || value > 2)
		{
		    char_puts("Sex range is 0 to 2.\n", ch);
		    return;
		}

		if ((victim->class == CLASS_WARLOCK) 
		|| (victim->class == CLASS_WITCH)
		|| (victim->class == CLASS_NECROMANCER))
		{
		    char_puts("Because of class restrictions,"
			      " you can't change their sex.\n", ch);
		    return;
		}
		victim->sex = value;
		if (!IS_NPC(victim))
		    victim->pcdata->true_sex = value;
		return;
	}

	if (!str_prefix(arg2, "class")) {
		int cl;

		if (IS_NPC(victim)) {
			char_puts("Mobiles have no class.\n",ch);
			return;
		}

		cl = cn_lookup(arg3);
		if (cl < 0) {
			BUFFER *output;

			output = buf_new(-1);

			buf_add(output, "Possible classes are: ");
	    		for (cl = 0; cl < classes.nused; cl++) {
	        		if (cl > 0)
	                		buf_add(output, " ");
	        		buf_add(output, CLASS(cl)->name);
	    		}
	        	buf_add(output, ".\n");

			send_to_char(buf_string(output), ch);
			buf_free(output);
			return;
		}

		victim->class = cl;
		victim->exp = exp_for_level(victim, victim->level);
		victim->exp_tl = 0;
		update_skills(victim);
		return;
	}

	if (!str_prefix(arg2, "level"))
	{
		if (!IS_NPC(victim))
		{
		    char_puts("You must 'advance' players.\n", ch);
		    return;
		}

		if (value < 0 || value > MAX_LEVEL)
		{
		    char_printf(ch, "Level range is 0 to %d.\n", MAX_LEVEL);
		    return;
		}
		victim->level = value;
		return;
	}

	if (!str_prefix(arg2, "gold"))
	{
		victim->gold = value;
		return;
	}

	if (!str_prefix(arg2, "hp"))
	{
		if (value < -10 || value > 30000)
		{
		    char_puts("Hp range is -10 to 30,000 hit points.\n", ch);
		    return;
		}
		victim->max_hit = value;
	    if (!IS_NPC(victim))
	        victim->pcdata->perm_hit = value;
		return;
	}

	if (!str_prefix(arg2, "mana"))
	{
		if (value < 0 || value > 60000)
		{
		    char_puts("Mana range is 0 to 60,000 mana points.\n", ch);
		    return;
		}
		victim->max_mana = value;
	    if (!IS_NPC(victim))
	        victim->pcdata->perm_mana = value;
		return;
	}

	if (!str_prefix(arg2, "move")) {
		if (value < 0 || value > 60000) {
			char_puts("Move range is 0 to 60,000 move points.\n", ch);
			return;
		}
		victim->max_move = value;

		if (!IS_NPC(victim))
		        victim->pcdata->perm_move = value;
		return;
	}

	if (!str_prefix(arg2, "practice")) {
		if (value < 0 || value > 250) {
		    char_puts("Practice range is 0 to 250 sessions.\n", ch);
		    return;
		}
		victim->practice = value;
		return;
	}

	if (!str_prefix(arg2, "train")) {
		if (value < 0 || value > 50) {
			char_puts("Training session range is 0 to 50 sessions.\n", ch);
			return;
		}
		victim->train = value;
		return;
	}

	if (!str_prefix(arg2, "align")) {
		if (value < -1000 || value > 1000) {
			char_puts("Alignment range is -1000 to 1000.\n", ch);
			return;
		}
		victim->alignment = value;
		char_puts("Remember to check their hometown.\n", ch);
		return;
	}

	if (!str_prefix(arg2, "ethos")) {
		int ethos;

		if (IS_NPC(victim)) {
			char_puts("Mobiles don't have an ethos.\n", ch);
			return;
		}

		ethos = flag_value(ethos_table, arg3);
		if (ethos < 0) {
			char_puts("%s: Unknown ethos.\n", ch);
			char_puts("Valid ethos types are:\n", ch);
			show_flags(ch, ethos_table);
			return;
		}

		victim->ethos = ethos;
		return;
	}

	if (!str_prefix(arg2, "hometown"))
	{
	    if (IS_NPC(victim))
		{
		    char_puts("Mobiles don't have hometowns.\n", ch);
		    return;
		}
	    if (value < 0 || value > 8)
	    { 
	        char_puts("Please choose one of the following :.\n", ch);
	        char_puts("Town        Alignment       Value\n", ch);
	        char_puts("----        ---------       -----\n", ch);
	        char_puts("Midgaard     Any              0\n", ch);
	        char_puts("New Thalos   Any              1\n", ch);
	        char_puts("Titan        Any              2\n", ch);
	        char_puts("Old Mid      Evil             3\n", ch);
	        char_puts("Magic        Any              4\n", ch);
	        char_puts("Emerald      Any              5\n", ch);
		char_puts("Fayten       Good             6\n", ch);
		char_puts("Virgin       Any              7\n", ch);
		char_puts("Caerlon      Neutral          8\n", ch);
			return;
	    }

	    if ((value == 6 && !IS_GOOD(victim)) || (value == 8 &&
		!IS_NEUTRAL(victim)) || (value == 3 && !IS_EVIL(victim)))
	    { 
	        char_puts("The hometown doesn't match this character's alignment.\n", ch);
	        return;
	    }    
	    
	    victim->hometown = value;
	    return;
	}

	if (!str_prefix(arg2, "thirst"))
	{
		if (IS_NPC(victim))
		{
		    char_puts("Not on NPC's.\n", ch);
		    return;
		}

		if (value < -1 || value > 100)
		{
		    char_puts("Thirst range is -1 to 100.\n", ch);
		    return;
		}

		victim->pcdata->condition[COND_THIRST] = value;
		return;
	}

	if (!str_prefix(arg2, "drunk"))
	{
		if (IS_NPC(victim))
		{
		    char_puts("Not on NPC's.\n", ch);
		    return;
		}

		if (value < -1 || value > 100)
		{
		    char_puts("Drunk range is -1 to 100.\n", ch);
		    return;
		}

		victim->pcdata->condition[COND_DRUNK] = value;
		return;
	}

	if (!str_prefix(arg2, "full"))
	{
		if (IS_NPC(victim))
		{
		    char_puts("Not on NPC's.\n", ch);
		    return;
		}

		if (value < -1 || value > 100)
		{
		    char_puts("Full range is -1 to 100.\n", ch);
		    return;
		}

		victim->pcdata->condition[COND_FULL] = value;
		return;
	}

	if (!str_prefix(arg2, "hunger"))
	{
		if (IS_NPC(victim))
		{
		    char_puts("Not on NPC's.\n", ch);
		    return;
		}

		if (value < -1 || value > 100)
		{
		    char_puts("Hunger range is -1 to 100.\n", ch);
		    return;
		}

		victim->pcdata->condition[COND_HUNGER] = value;
		return;
	}

	if (!str_prefix(arg2, "bloodlust"))
	{
		if (IS_NPC(victim))
		{
		    char_puts("Not on NPC's.\n", ch);
		    return;
		}

		if (value < -1 || value > 100)
		{
		    char_puts("Full range is -1 to 100.\n", ch);
		    return;
		}

		victim->pcdata->condition[COND_BLOODLUST] = value;
		return;
	}

	if (!str_prefix(arg2, "desire"))
	{
		if (IS_NPC(victim))
		{
		    char_puts("Not on NPC's.\n", ch);
		    return;
		}

		if (value < -1 || value > 100)
		{
		    char_puts("Full range is -1 to 100.\n", ch);
		    return;
		}

		victim->pcdata->condition[COND_DESIRE] = value;
		return;
	}

	if (!str_prefix(arg2, "race")) {
		int race;

		race = rn_lookup(arg3);

		if (race == -1) {
			char_puts("That is not a valid race.\n",ch);
			return;
		}

		if (!IS_NPC(victim) && !RACE(race)->pcdata) {
			char_puts("That is not a valid player race.\n",ch);
			return;
		}

		victim->race = race;
		SET_ORG_RACE(victim, race);
		update_skills(victim);
		victim->exp = exp_for_level(victim, victim->level);
		victim->exp_tl = 0;
		return;
	}

	if (!str_prefix(arg2, "noghost")) {
		if (IS_NPC(victim)) {
			char_puts("Not on NPCs.\n", ch);
			return;
		}
		REMOVE_BIT(victim->state_flags, STATE_GHOST);
		REMOVE_BIT(victim->affected_by, AFF_PASS_DOOR);
		REMOVE_BIT(victim->affected_by, AFF_FLYING);
		char_puts("The player is no longer a ghost.\n", ch);
		return;
	}

	if (!str_prefix(arg2, "newbie")) {
		if (IS_NPC(victim)) {
			char_puts("Not on NPCs.\n", ch);
			return;
		}
		if (value == 0) {
			REMOVE_BIT(victim->acct_flags, ACCT_NEWBIE);
			char_puts("The player is not a newbie.\n", ch);
		}
		else if (value == 1) {
			SET_BIT(victim->acct_flags, ACCT_NEWBIE);
			char_puts("The player is now under newbie protection.\n", ch);
		}
		else {
			char_puts("Accepted values are 0 (false) or 1 (true).\n", ch);
		}

		return;
	}

	if (!str_prefix(arg2, "multikiller")) {
		if (IS_NPC(victim)) {
			char_puts("Not on NPCs.\n", ch);
			return;
		}
		if (value == 0) {
			REMOVE_BIT(victim->acct_flags, ACCT_MULTIKILLER);
			char_puts("The player is no longer a multikiller.\n", ch);
		}
		else if (value == 1) {
			SET_BIT(victim->acct_flags, ACCT_MULTIKILLER);
			char_puts("The player is now a multikiller.\n", ch);

		}
		else {
			char_puts("Accepted values are 0 (false) or 1 (true).\n", ch);
		}

		return;
	}

	if (!str_prefix(arg2, "truelifer")) {
		if (IS_NPC(victim)) {
			char_puts("Not on NPCs.\n", ch);
			return;
		}
		if (value == 0) {
			REMOVE_BIT(victim->acct_flags, ACCT_TRUE_LIFER);
			char_puts("The player is no longer a true lifer.\n", ch);
		}
		else if (value == 1) {
			if (ch->level >= ML-1) {
				SET_BIT(victim->acct_flags, ACCT_TRUE_LIFER);
				char_puts("The player is now a true lifer.\n", ch);
			}

		}
		else {
			char_puts("Accepted values are 0 (false) or 1 (true).\n", ch);
		}

		return;
	}

	if (!str_prefix(arg2, "pkok")) {
		name_toggle(&victim->pcdata->pk_ok_list, arg3, ch, "PKOK");
		return;
	}

	/* set a fake ip address for imms to see in sockets so upper
	 * level admins can have anonymous characters */
	if (!str_cmp(arg2, "fakeip") && IS_TRUSTED_IMP(ch)) {
		if (IS_NPC(victim)) {
			char_puts("Not on NPCs.\n", ch);
			return;
		}
		free_string(victim->pcdata->fake_ip);
		victim->pcdata->fake_ip = str_dup(arg3);
		return;
	}

	if (!str_prefix(arg2, "clan")) {
		int cn;

		if (IS_NPC(victim)) {
			char_puts("Not on an NPC.\n", ch);
			return;
		}

		if ((cn = cln_lookup(arg3)) < 0) {
			char_puts("Incorrect clan name.\n", ch);
			return;
		}

		if (cn != victim->clan) {
			clan_t *clan;

			if (victim->clan
			&&  (clan = clan_lookup(victim->clan))) {
				clan_update_lists(clan, victim);
				clan_save(clan);
			}

			victim->clan = cn;

			if (cn) {
				clan = CLAN(cn);
				if (IS_IMMORTAL(victim)) {
					victim->pcdata->clan_status = CLAN_PATRON;
					name_add(&clan->member_list[CLAN_PATRON], victim->name,
						 NULL, NULL);
				}
				else {
					victim->pcdata->clan_status = CLAN_RECRUIT;
					name_add(&clan->member_list[CLAN_RECRUIT], victim->name,
						 NULL, NULL);
				}

				clan_save(clan);
			}
			else {
				victim->pcdata->clan_status = CLAN_FREEMAN;
			}

			update_skills(victim);
		}

		char_puts("Ok.\n", ch);
		return;
	}

	/*
	 * Generate usage message.
	 */
	do_mset(ch, str_empty);
}


void do_desocket(CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d;
	int socket;
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg, sizeof(arg));

	if (!is_number(arg))

	{
	  char_puts("The argument must be a number.\n", ch);
	  return;
	}

	if (arg[0] == '\0')
	{
	  char_puts("Disconnect which socket?\n", ch);
	  return;
	}

	else
	{
	  socket = atoi(arg);
	  for (d = descriptor_list; d != NULL; d = d->next)      
		{
		  if (d->descriptor == socket)
		    {
		      if (d->character == ch)
			{
			  char_puts("It would be foolish to disconnect yourself.\n", ch);
			  return;
			}
		      if (d->connected == CON_PLAYING)
			{
			  char_puts("Why don't you just use disconnect?\n", ch);
			  return;
			}
		      write_to_descriptor(d->descriptor,
					  "You are being disconnected by an immortal.",
					  0);
		      close_descriptor(d);
		      char_puts("Done.\n", ch);
		      return;
		    }
		}
	  char_puts("No such socket is connected.\n", ch);
	  return;
	}
}

void do_smite(CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *victim;

	if (argument[0] == '\0') {
	  char_puts("You are so frustrated you smite yourself!  OWW!\n", 
			ch);
	  return;
	}

	if ((victim = get_char_world(ch, argument)) == NULL) {
	  char_puts("You'll have to smite them some other day.\n", ch);
	  return;
	}

	if (IS_NPC(victim) && ch->level < L5) {
	  char_puts("That poor mob never did anything to you.\n", ch);
	  return;
	}

	if (victim->level > ch->level) {
	  char_puts("How dare you!\n", ch);
	  return;
	}

	if (victim->position < POS_SLEEPING) {
	  char_puts("Take pity on the poor thing.\n", ch);
	  return;
	}

	act("A bolt comes down out of the heavens and smites you!", victim, NULL,
		ch, TO_CHAR);
	act("You reach down and smite $n!", victim, NULL, ch, TO_VICT);
	act("A bolt from the heavens smites $n!", victim, NULL, ch, TO_NOTVICT);
	victim->hit = victim->hit / 2;
	return;
}

void do_popularity(CHAR_DATA *ch, const char *argument)
{
	BUFFER *output;
	AREA_DATA *area;
	extern AREA_DATA *area_first;
	int i;

	output = buf_new(-1);
	buf_add(output, "Area popularity statistics (in char * ticks)\n");

	for (area = area_first,i=0; area != NULL; area = area->next,i++) {
		if (i % 2 == 0) 
			buf_add(output, "\n");
		buf_printf(output, "%-20s %-8lu       ",
			   area->name, area->count);
	}
	buf_add(output, "\n\n");
	page_to_char(buf_string(output), ch);
	buf_free(output);
}

void do_herotitle(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0')  {
		char_puts("Change whose title to what?\n", ch);
		return;
	}

	victim = get_char_world(ch, arg);
	if (victim == NULL)  {
		char_puts("Nobody is playing with that name.\n", ch);
		return;
	}

	if (IS_NPC(victim)) {
		char_puts("Not on NPC's.\n", ch);
		return;
	}

	if (argument[0] == '\0') {
		char_puts("Change the title to what?\n", ch);
		return;
	}

	/*
	if (ch->level < L1
	&& victim->level != LEVEL_HERO) {
		char_puts("You can only change the titles of heros.\n", ch);
		return;
	}
	*/

	if (set_title(victim, argument)) {
		char_printf(ch, "Title set to: %s%s\n", 
			victim->name,
			victim->pcdata->title);
		char_printf(victim, "Your title is set to: %s%s\n", 
			victim->name,
			victim->pcdata->title);
	}
	else {
		char_printf(ch, 
			"The title was over the maximum of %d visible characters.",
			MAX_TITLE_LENGTH);
	}
}

/*
 * .gz files are checked for too, just in case.
 */

void do_rename(CHAR_DATA* ch, const char *argument)
{
	char old_name[MAX_INPUT_LENGTH], 
	     new_name[MAX_INPUT_LENGTH];
	char strsave[PATH_MAX];
	char *file_name;

	CHAR_DATA *victim;
	clan_t *clan;
		
	argument = first_arg(argument, old_name, sizeof(old_name), FALSE); 
		   first_arg(argument, new_name, sizeof(new_name), FALSE);
		
	if (!old_name[0]) {
		char_puts("Rename who?\n",ch);
		return;
	}
		
	victim = get_char_world(ch, old_name);
		
	if (!victim) {
		char_puts("There is no such a person online.\n",ch);
		return;
	}
		
	if (IS_NPC(victim)) {   
		char_puts("You cannot use Rename on NPCs.\n",ch);
		return;
	}

	if (victim != ch && victim->level >= ch->level) {
		char_puts("You failed.\n",ch);
		return;
	}
		
	if (!victim->desc || (victim->desc->connected != CON_PLAYING)) {
		char_puts("This player has lost his link or is inside a pager or the like.\n",ch);
		return;
	}

	if (!new_name[0]) {
		char_puts("Rename to what new name?\n",ch);
		return;
	}
		
	if (!pc_name_ok(new_name)) {
		char_puts("The new name is illegal.\n",ch);
		return;
	}

	if (victim->clan && (clan = clan_lookup(victim->clan))) {
		bool touched = FALSE;

		if (name_delete(&clan->member_list[victim->pcdata->clan_status], 
		   old_name, NULL, NULL)) {
			touched = TRUE;
			name_add(&clan->member_list[victim->pcdata->clan_status], 
				new_name, NULL, NULL);
		}
		if (touched)
			clan_save(clan);
	}

/* delete old pfile */
	if (str_cmp(new_name, old_name)) {
		DESCRIPTOR_DATA *d;
		OBJ_DATA *obj;

		for (d = descriptor_list; d; d = d->next)
			if (d->character
			&&  !str_cmp(d->character->name, new_name)) {
				char_puts ("A player with the name you specified already exists!\n",ch);
				return;
			}

		/* check pfile */
		file_name = capitalize(new_name);
		if (dfexist(PLAYER_PATH, file_name)) {
			char_puts("A player with that name already exists!\n",
				  ch);
			return;		
		}

		/* check .gz pfile */
		snprintf(strsave, sizeof(strsave), "%s.gz", file_name);
		if (dfexist(PLAYER_PATH, strsave)) {
			char_puts ("A player with that name already exists in a compressed file!\n",ch);
			return;		
		}

		/* change object owners */
		for (obj = object_list; obj; obj = obj->next)
			if (obj->owner
			&&  !str_cmp(mlstr_mval(obj->owner), old_name)) {
				mlstr_free(obj->owner);
				obj->owner = mlstr_new(new_name);
			}

		dunlink(PLAYER_PATH, capitalize(old_name)); 
	}
/*
 * NOTE: Players who are level 1 do NOT get saved under a new name 
 */
	free_string(victim->name);
	victim->name = str_dup(new_name);
	mlstr_free(victim->short_descr);
	victim->short_descr = mlstr_new(new_name);
	save_char_obj(victim, FALSE);
		
	char_puts("Character renamed.\n", ch);
	act_puts("$n has renamed you to $N!",
		 ch, NULL, victim, TO_VICT, POS_DEAD);
} 

void do_notitle(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	if (!IS_IMMORTAL(ch))
	    return;

	argument = one_argument(argument, arg, sizeof(arg));

	if ((victim = get_char_world(ch ,arg)) == NULL) {
		char_puts("He is not currently playing.\n", ch);
		return;
	}
	 
	TOGGLE_BIT(victim->state_flags, STATE_NOTITLE);
	if (!IS_SET(victim->state_flags, STATE_NOTITLE))
	 	char_puts("You can change your title again.\n", victim);
	else 
		char_puts("You won't be able to change your title anymore.\n",
			  victim);
	char_puts("Ok.\n", ch);
}
	   
void do_noaffect(CHAR_DATA *ch, const char *argument)
{
	AFFECT_DATA *paf,*paf_next;
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	if (!IS_IMMORTAL(ch))
		return;

	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Noaff whom?\n", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		char_puts("He is not currently playing.\n", ch);
		return;
	}
	 
	for (paf = victim->affected; paf != NULL; paf = paf_next) {
		paf_next = paf->next;
		if (paf->duration >= 0) {
			skill_t *sk;

			if ((sk = skill_lookup(paf->type))
			&&  !IS_NULLSTR(sk->msg_off))
				act_puts(sk->msg_off, victim, NULL, NULL, 
					 TO_CHAR, POS_RESTING);
		  
			affect_remove(victim, paf);
		}
	}
}

void do_affrooms(CHAR_DATA *ch, const char *argument)
{
	ROOM_INDEX_DATA *room;
	ROOM_INDEX_DATA *room_next;
	int count = 0;

	if (!top_affected_room) 
		char_puts("No affected room.\n",ch);

	for (room = top_affected_room; room ; room = room_next) {
		room_next = room->aff_next;
		count++;
		char_printf(ch, "%d) [Vnum : %5d] %s\n",
			count, room->vnum , mlstr_cval(room->name, ch));
	}
}

void do_find(CHAR_DATA *ch, const char *argument)
{
	char* path;
	ROOM_INDEX_DATA *location;

	if (argument[0] == '\0') {
		char_puts("Ok. But what I should find?\n", ch);
		return;
	}

	if ((location = find_location(ch, argument)) == NULL) {
		char_puts("No such location.\n", ch);
		return;
	}

	path = find_way(ch, ch->in_room, location);
	char_printf(ch, "%s.\n", path);
	log_printf("From %d to %d: %s.\n",
		   ch->in_room->vnum, location->vnum, path);
	return;
}

void do_reboot(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg, sizeof(arg));    

	if (arg[0] == '\0') {
		char_puts("Usage: reboot now\n"
			  "Usage: reboot <ticks to reboot>\n"
			  "Usage: reboot cancel\n"
			  "Usage: reboot status\n", ch);
		return;
	}

	if (is_name(arg,"cancel")) {
		reboot_counter = -1;
		char_puts("Reboot canceled.\n", ch);
		return;
	}

	if (is_name(arg, "now")) {
		reboot_mud();
		return;
	}

	if (is_name(arg, "status")) {
		if (reboot_counter == -1) 
			char_printf(ch, "Automatic rebooting is inactive.\n");
		else
			char_printf(ch, "Reboot in %i minutes.\n",
				    reboot_counter);
		return;
	}

	if (is_number(arg)) {
		reboot_counter = atoi(arg);
		rebooter = 1;
		char_printf(ch, "Legends & Lore will reboot in %i ticks.\n",
			    reboot_counter);
		return;
	}

	do_reboot(ch, "");   
}

void reboot_mud(void)
{
	extern bool merc_down;
	DESCRIPTOR_DATA *d,*d_next;

	log("Rebooting Legends & Lore");
	for (d = descriptor_list; d != NULL; d = d_next) {
		d_next = d->next;
		write_to_buffer(d,"Legends & Lore is going down for rebooting NOW!\n\r",0);
		if (d->character)
			save_char_obj(d->character, TRUE);
		close_descriptor(d);
	}
	merc_down = TRUE;    
}

DO_FUN(do_msgstat)
{
	varr *v;
	mlstring **mlp;
	int i;
	BUFFER *output;

	if (argument[0] == '\0') {
		for (i = 0; i < MAX_MSG_HASH; i++) {
			varr *v = msg_hash_table+i;
			char_printf(ch, "%3d: %d msgs\n", i, v->nused);
		}
		return;
	}

	if (!is_number(argument)) {
		do_help(ch, "MSGSTAT");
		return;
	}

	i = atoi(argument);
	if (i < 0 || i >= MAX_MSG_HASH) {
		char_printf(ch, "Valid hash key range is 0..%d\n",
			    MAX_KEY_HASH);
		return;
	}

	v = msg_hash_table+i;
	output = buf_new(-1);
	buf_printf(output, "Dumping msgs with hash #%d\n", i);
	for (i = 0; i < v->nused; i++) {
		mlp = VARR_GET(v, i);
		mlstr_dump(output, str_empty, *mlp);
		buf_add(output, "\n");
	}
	page_to_char(buf_string(output), ch);
	buf_free(output);
}

extern int str_count;
extern int str_real_count; /* XXX */

DO_FUN(do_strstat)
{
	char_printf(ch, "Strings: %d\n"
			"Allocated: %d\n",
		    str_count, str_real_count);
}

DO_FUN(do_grant)
{
	cmd_t *cmd;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
	if (arg1[0] == '\0') {
		do_help(ch, "'WIZ GRANT'");
		return;
	}

	if ((victim = get_char_room(ch, arg1)) == NULL) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (IS_NPC(victim)) {
		char_puts("Not on NPC.\n", ch);
		return;
	}

	if (arg2[0] == '\0') {
		char_printf(ch, "Granted commands for %s: [%s]\n",
			    victim->name, victim->pcdata->granted);
		return;
	}

	if (is_number(arg2)) {
		int lev = atoi(arg2);

		if (lev < LEVEL_IMMORTAL) {
			char_printf(ch, "grant: granted level must be at least %d\n", LEVEL_HERO);
			return;
		}

		if (lev > ch->level) {
			char_puts("grant: granted level cannot be higher"
				  " than yours.\n", ch);
			return;
		}

		for (cmd = cmd_table; cmd->name; cmd++) {
			if (cmd->level < LEVEL_HERO
			||  cmd->level > lev)
				continue;

			name_add(&victim->pcdata->granted, cmd->name,
				 ch, "grant");
		}

		return;
	}

	for (; arg2[0]; argument = one_argument(argument, arg2, sizeof(arg2))) {
		if ((cmd = cmd_lookup(arg2)) == NULL
		&&  str_cmp(arg2, "none")
		&&  str_cmp(arg2, "all")) {
			char_printf(ch, "%s: command not found.\n", arg2);
			continue;
		}

		if (cmd && cmd->level < LEVEL_IMMORTAL) {
			char_printf(ch, "%s: not a wizard command.\n", arg2);
			continue;
		}
		if (cmd && cmd->level > ch->level) {
			char_printf(ch, "%s: is above your level.\n", arg2);
			continue;
		}
		name_toggle(&victim->pcdata->granted, arg2, ch, "grant");
	}
}

DO_FUN(do_disable)
{
	cmd_t *cmd;
	char arg[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		do_help(ch, "'WIZ ENABLE DISABLE'");
		return;
	}

	if (!str_cmp(arg, "?")) {
		char_puts("Disabled commands:\n", ch);
		for (cmd = cmd_table; cmd->name; cmd++)
			if (IS_SET(cmd->flags, CMD_DISABLED))
				char_printf(ch, "%s\n", cmd->name);
		return;
	}

	for (; arg[0]; argument = one_argument(argument, arg, sizeof(arg))) {
		if ((cmd = cmd_lookup(arg)) == NULL) {
			char_printf(ch, "%s: command not found.\n", arg);
			continue;
		}

		if (!str_cmp(cmd->name, "enable")) {
			char_puts("'enable' command cannot be disabled.\n",
				  ch);
			return;
		}

		SET_BIT(cmd->flags, CMD_DISABLED);
		char_printf(ch, "%s: command disabled.\n", cmd->name);
	}
}

DO_FUN(do_enable)
{
	cmd_t *cmd;
	char arg[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		do_help(ch, "'WIZ ENABLE DISABLE'");
		return;
	}

	for (; arg[0]; argument = one_argument(argument, arg, sizeof(arg))) {
		if ((cmd = cmd_lookup(arg)) == NULL) {
			char_printf(ch, "%s: command not found.\n", arg);
			continue;
		}

		REMOVE_BIT(cmd->flags, CMD_DISABLED);
		char_printf(ch, "%s: command enabled.\n", cmd->name);
	}
}

DO_FUN(do_warena)
{
	if (is_affected_world(WAFF_ARENA)) { 
		WORLD_AFFECT_DATA *paff;

		while ((paff = affect_find_world(WAFF_ARENA)) != NULL) {
			if (paff->level <= ch->level) {
				affect_remove_world(paff);
				wiznet("$N has removed {Yarena{x mode from the world.", 
					ch, NULL, WIZ_ANNOUNCE, 0, 0);
			}
			else
				char_puts("Sorry, you're not high enough level to do that.", ch);
		}
	}
	else {
		WORLD_AFFECT_DATA waff;
		waff.player		= ch;
		waff.player_name	= ch->name;
		waff.level		= LEVEL_IMMORTAL;
		waff.type		= WAFF_ARENA;
		waff.duration		= -1;
		waff.modifier		= 0;
		waff.chance		= 100;
		waff.min_level		= 0;
		waff.max_level		= LEVEL_HERO;
		waff.visible_level	= MIN_PK_LEVEL;
		waff.notify		= WAFF_NOTIFY_ON | WAFF_NOTIFY_OFF;
		waff.area		= NULL;
		affect_to_world(&waff);

		wiznet("$N has put the world in {Yarena{x mode.", 
			ch, NULL, WIZ_ANNOUNCE, 0, 0);
	}
}

/*
 * scatter all NPC mobs or objects in the room
 * 	throughout the world or area to be found for quests.
 *
 * by Zsuzsu
 */
DO_FUN(do_scatter)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	AREA_DATA *pArea = NULL;
	ROOM_INDEX_DATA *pRoom = NULL;
	CHAR_DATA *vch = NULL, *vch_next;
	OBJ_DATA *obj, *r_next_cont;
	int count = 0;


	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0') {
		do_help(ch, "'WIZ SCATTER'");
		return;
	}

	if (!str_prefix(arg1, "area")) {
		if (arg2[0] == '\0')
			pArea = ch->in_room->area;
		else if (!is_number(arg2) || (pArea = area_lookup(atoi(arg2))) == NULL) {
			char_puts("Can't find that area.\n", ch);
			return;
		}
	}
	else if (!str_prefix(arg1, "world")) {
		pArea = NULL;
	}
	else {
		do_help(ch, "'WIZ SCATTER'");
		return;
	}

	act("You toss everything in the room to the winds.",
		ch, NULL, NULL, TO_CHAR);

	act("$n tosses everything in the room to the winds.",
		ch, NULL, NULL, TO_ROOM);

        for (vch = ch->in_room->people; vch; vch = vch_next) {
		vch_next = vch->next_in_room;

		if (vch == ch
		||  !vch->in_room
		||  !IS_NPC(vch)
		||  IS_IMMORTAL(vch)
		||  !(pRoom = get_random_room(ch, pArea))
		||  !pRoom->area
		||  !pRoom->area->name
		||  !pRoom->name)
		continue;

		/*pRoom = get_random_room(ch, pArea);*/

		char_printf(ch, "{D%3d){x %s {Dsent to{x %-15.15s {D[{W%6d{D]{x %s\n",
			count++,
			mlstr_mval(vch->short_descr),
			pRoom->area->name, 
			pRoom->vnum,
			mlstr_mval(pRoom->name));

		transfer_char(vch, NULL, pRoom, NULL, NULL, NULL);
	}

	for (obj = ch->in_room->contents; obj; obj = r_next_cont) {
		r_next_cont = obj->next_content;

		pRoom = get_random_room(ch, pArea);

		char_printf(ch, "{D%3d){x %s {Dsent to{x %-15.15s {D[{W%6d{D]{x %s\n",
			count++,
			mlstr_mval(obj->short_descr),
			pRoom->area->name, 
			pRoom->vnum,
			mlstr_mval(pRoom->name));

		obj_from_room(obj);
		obj_to_room(obj, pRoom);
	}
}

/*
 * aschar - As Character
 * get some info that normally you'd have
 * to snoop and force to see.
 */
DO_FUN(do_aschar)
{
	char arg1[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg1, sizeof(arg1));

	if (arg1[0] == '\0' || argument[0] == '\0') {
		char_puts("As whom?  Do what?\n",ch);
		return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		char_puts("Couldn't find that person in the realm.\n",ch);
		return;
	}

	if (victim->level > ch->level) {
		char_puts("You don't have the power for that insight.\n",ch);
		return;
	}

	switch (argument[0]) {
		case 'p': /*practice*/
			show_skills(ch, victim);
			break;
		default:
			char_puts("Sorry, don't know how to do that.\n", ch);
	}
}

/*
 * reclaim - deletes objects from pfiles.
 * 	All files are loaded, the vnum of
 * 	the item is searched for in all the
 * 	player's containers, and is removed.
 *
 * 	This is mostly useful for reseting the
 * 	number of limiteds in a realm.
 *
 * by Zsuzsu
 */
DO_FUN(do_reclaim)
{
	char arg[MAX_INPUT_LENGTH];
	struct dirent *dp;
	DIR *dirp;
	DESCRIPTOR_DATA * d;
	CHAR_DATA *victim;
	OBJ_INDEX_DATA *obj_index;
	int count = 0, pcount = 0;


	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0')  {
		do_help(ch, "'WIZ RECLAIM'");
		return;
	}

	obj_index = get_obj_index(atoi(arg));

	if (!obj_index) {
		char_puts("Bad vnum?\n", ch);
		return;
	}

	if ((dirp = opendir(PLAYER_PATH)) == NULL) {
		char_printf(ch, "BUG: unable to open player directory %s\n",
			PLAYER_PATH);
		return;
	}

	char_printf(ch, "Attempting to reclaim: %s\n", 
			mlstr_mval(obj_index->short_descr));

	d = malloc(sizeof(*d));
	d->connected = CON_DUMMY;

	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {

#if defined (LINUX) || defined (WIN32)
		if (strlen(dp->d_name) < 3)
			continue;
#else
		if (dp->d_namlen < 3 || dp->d_type != DT_REG)
			continue;
#endif
		victim = get_char_world(ch, dp->d_name);

		if (!victim) {
			if (!load_char_obj(d, dp->d_name)) {
				DEBUG(DEBUG_RECLAIM,
					"reclaim couldn't load: %s", dp->d_name);
				continue;
			}
			victim = d->character;
		}

		pcount = strip_obj_from_char(ch, victim, obj_index);

		if (pcount > 0) {
			char_printf(ch,
				"{W%12s{x had {Y%3d{x.\n",
				victim->name,
				pcount);
			save_char_obj(victim, FALSE);
		}

		count += pcount;

		if (victim->desc != NULL
		&& victim->desc->connected == CON_DUMMY)
			free_char(victim);

		victim = NULL;

	}
	closedir(dirp);
	free(d);
	char_printf(ch,
		"Total reclaimed: {Y%3d{x.\n",
		count);
}

/*
 * Silence a player or the whole mud for a certain channel.
 * This replaces do_nogossip do_nochan etc.
 *
 * by Zsuzsu
 */
DO_FUN(do_silence)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	char buf[MAX_INPUT_LENGTH];
	char *chan_name = NULL;
	char *on_off = NULL;
	bool set_on = FALSE;
	bool toggle = FALSE;
	CHAR_DATA *victim;
	channel_t *channel;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
	argument = one_argument(argument, arg3, sizeof(arg2));

	if (arg1[0] == '\0')  {
		do_help(ch, "'WIZ SILENCE'");
		return;
	}

	victim = get_char_world(ch, arg1);
	if (victim && !IS_NPC(victim))  {
		chan_name = arg2;
		on_off = arg3;
	}
	else {
		victim = NULL;
		chan_name = arg1;
		on_off = arg2;
	}

	if (victim && victim->level >= ch->level) {
		char_puts("Nice try.\n", ch);
		return;
	}

	/* nochan - all except PRAY channel*/
	if (victim 
	&& (chan_name[0] == '\0' 
	|| !str_prefix(chan_name, "nochan"))) {
		TOGGLE_BIT(victim->restricted_channels,
				ALL ^ CHAN_WIZHELP);
		char_printf(ch, "%s's privileges to {WALL CHANNELS{x have been %s{x!",
			victim->name,
			IS_SET(victim->restricted_channels, ALL ^ CHAN_WIZHELP)
				? "{Rrevoked" : "{Grestored");
		char_printf(victim, "Your privileges to {WALL CHANNELS{x have been %s{x!",
			IS_SET(victim->restricted_channels, ALL ^ CHAN_WIZHELP)
				? "{Rrevoked" : "{Grestored");

		snprintf(buf, sizeof(buf),
			"$N %s{x $i's channel privilege to {WALL CHANNELS{x.",
			IS_SET(victim->restricted_channels, ALL ^ CHAN_WIZHELP)
				? "{Rrevoked" : "{Grestored");

		wiznet(buf, ch, victim, WIZ_PENALTIES, 0, 0);
		return;
	}

	channel = channel_lookup(chan_name);
	if (!channel) {
		char_puts("Illegal channel or player name.\n", ch);
		return;
	}

	if (!str_cmp(on_off, "on"))
		set_on = TRUE;
	else if (!str_cmp(on_off, "off"))
		set_on = FALSE;
	else {
		toggle = TRUE;;
	}

	if (victim) {
		if (toggle)
			TOGGLE_BIT(victim->restricted_channels,
				channel->flag);
		else if (set_on)
			SET_BIT(victim->restricted_channels,
				channel->flag);
		else
			REMOVE_BIT(victim->restricted_channels,
				channel->flag);

		char_printf(ch, "%s's privilege to {W%s{x has been %s{x.",
			victim->name,
			channel->name,
			IS_SET(victim->restricted_channels, channel->flag)
				? "{Rrevoked" : "{Grestored");
		char_printf(victim, "Your privilege to {W%s{x has been %s{x.",
			channel->name,
			IS_SET(victim->restricted_channels, channel->flag)
				? "{Rrevoked" : "{Grestored");

		snprintf(buf, sizeof(buf),
			"$N %s{x $i's channel privilege to {W%s{x.",
			IS_SET(victim->restricted_channels, channel->flag)
				? "{Rrevoked" : "{Grestored",
			channel->name);

		wiznet(buf, ch, victim, WIZ_PENALTIES, 0, 0);
	}
	else {
		if (toggle)
			TOGGLE_BIT(global_channels,
				channel->flag);
		else if (set_on)
			REMOVE_BIT(global_channels,
				channel->flag);
		else
			SET_BIT(global_channels,
				channel->flag);

		char_printf(ch, "Global privileges to {W%s{x have been %s{x.",
			channel->name,
			IS_SET(global_channels, channel->flag)
				? "{Grestored" : "{Rrevoked");

		snprintf(buf, sizeof(buf),
			"$N has %s{x {Bglobal{x channel privilege to {W%s{x.",
			!IS_SET(global_channels, channel->flag)
				? "{Rrevoked" : "{Grestored",
			channel->name);

		wiznet(buf, ch, NULL, WIZ_ANNOUNCE, 0, 0);
	}
}

/*
 * matchup two players (or mobs)
 * so you can compare their viability in battle
 * in such things as dodge odds against each other.
 *
 * by Zsuzsu
 */
DO_FUN(do_matchup)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *player1;
	CHAR_DATA *player2;
	int chance1 = 100;
	int chance2 = 100;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0' || arg2[0] == '\0')  {
		do_help(ch, "'MATCHUP'");
		return;
	}

	player1 = get_char_world(ch, arg1);
	if (player1 == NULL)  {
		char_puts("Can't find the attacker in the realm.\n", ch);
		return;
	}

	player2 = get_char_world(ch, arg2);
	if (player2 == NULL)  {
		char_puts("Can't find the defender in the realm.\n", ch);
		return;
	}

	char_printf(ch, "Battle Odds:\n"); 
	
	char_printf(ch,"     {r%20.20s{x v {b%s{x\n",
		player1->name, player2->name);

	char_printf(ch, "Level:                {r%3d{x /{b%3d{x\n",
		LEVEL(player1), LEVEL(player2));

	char_printf(ch, "Str:                   {r%2d{x / {b%2d{x\n",
		get_curr_stat(player1, STAT_STR), 
		get_curr_stat(player2, STAT_STR));

	char_printf(ch, "Dex:                   {r%2d{x / {b%2d{x\n",
		get_curr_stat(player1, STAT_DEX), 
		get_curr_stat(player2, STAT_DEX));

	char_printf(ch, "Encumberance:        {r%3d{x% / {b%d{x%\n",
		ENCUMBERANCE(player1),
		ENCUMBERANCE(player2));

	char_printf(ch, "Saves:                {r%3d{x / {b%3d{x\n",
		player1->saving_throw,
		player2->saving_throw);

	char_printf(ch, "\n");

	char_printf(ch, "HitRoll:              {r%3d{x / {b%3d{x\n",
		GET_HITROLL(player1),
		GET_HITROLL(player2));

	char_printf(ch, "toHit:               {r%4d{x / {b%d{x {D(needed of d20){x\n",
		tohit_chance(player1, player2, 0, WEAR_WIELD),
		tohit_chance(player2, player1, 0, WEAR_WIELD));

	char_printf(ch, "toHit(2nd weapon):   {r%4d{x / {b%d{x\n",
		tohit_chance(player1, player2, 0, WEAR_SECOND_WIELD),
		tohit_chance(player2, player1, 0, WEAR_SECOND_WIELD));

	char_printf(ch, "\n");

	char_printf(ch, "Parry:        {D(%3d%%) {r%3d{x%% /{b%3d{x%% {D(%3d%%){x\n", 
		get_skill(player1, gsn_parry),
		parry_chance(player2, player1),
		parry_chance(player1, player2),
		get_skill(player2, gsn_parry));

	chance1 = chance1 - (chance1 * parry_chance(player2, player1) / 100);
	chance2 = chance2 - (chance2 * parry_chance(player1, player2) / 100);

	char_printf(ch, "Dodge:        {D(%3d%%) {r%3d{x%% /{b%3d{x%% {D(%3d%%){x\n", 
		get_skill(player1, gsn_dodge),
		dodge_chance(player2, player1),
		dodge_chance(player1, player2),
		get_skill(player2, gsn_dodge));

	chance1 = chance1 - (chance1 * dodge_chance(player2, player1) / 100);
	chance2 = chance2 - (chance2 * dodge_chance(player1, player2) / 100);

	char_printf(ch, "Shield Block  {D(%3d%%) {r%3d{x%% /{b%3d{x%% {D(%3d%%){x\n", 
		get_skill(player1, gsn_shield_block),
		shield_block_chance(player2, player1),
		shield_block_chance(player1, player2),
		get_skill(player2, gsn_shield_block));

	chance1 = chance1 - (chance1 * shield_block_chance(player2, player1) / 100);
	chance2 = chance2 - (chance2 * shield_block_chance(player1, player2) / 100);

	char_printf(ch, "Hand Block:   {D(%3d%%) {r%3d{x%% /{b%3d{x%% {D(%3d%%){x\n", 
		get_skill(player1, gsn_hand_block),
		hand_block_chance(player2, player1),
		hand_block_chance(player1, player2),
		get_skill(player2, gsn_hand_block));

	chance1 = chance1 - (chance1 * hand_block_chance(player2, player1) / 100);
	chance2 = chance2 - (chance2 * hand_block_chance(player1, player2) / 100);

	char_printf(ch, "Haft Block:   {D(%3d%%) {r%3d{x%% /{b%3d{x%% {D(%3d%%){x\n", 
		get_skill(player1, gsn_haft_block),
		haft_block_chance(player2, player1),
		haft_block_chance(player1, player2),
		get_skill(player2, gsn_haft_block));

	chance1 = chance1 - (chance1 * haft_block_chance(player2, player1) / 100);
	chance2 = chance2 - (chance2 * haft_block_chance(player1, player2) / 100);

	char_printf(ch, "Tumble:       {D(%3d%%) {r%3d{x%% /{b%3d{x%% {D(%3d%%){x\n", 
		get_skill(player1, gsn_tumble),
		tumble_chance(player2, player1),
		tumble_chance(player1, player2),
		get_skill(player2, gsn_tumble));

	chance1 = chance1 - (chance1 * tumble_chance(player2, player1) / 100);
	chance2 = chance2 - (chance2 * tumble_chance(player1, player2) / 100);

	char_printf(ch, "Blink:        {D(%3d%%) {r%3d{x%% /{b%3d{x%% {D(%3d%%){x\n", 
		get_skill(player1, gsn_blink),
		blink_chance(player2, player1),
		blink_chance(player1, player2),
		get_skill(player2, gsn_blink));
	
	chance1 = chance1 - (chance1 * blink_chance(player2, player1) / 100);
	chance2 = chance2 - (chance2 * blink_chance(player1, player2) / 100);

	char_printf(ch, "Total Evasion:       {r%3d{x%% /{b%3d{x%%\n",
			100 - chance1, 100 - chance2);
	/*
	char_printf(ch, "\n");

	char_printf(ch, "Saves:                {r%3d{x / {b%3d{x\n",
		player1->saving_throw, 
		player2->saving_throw);
	*/

	char_printf(ch, "\n");
	char_printf(ch, "Invis Level:          {r%3d{x / {b%3d{x\n",
			get_invis_level(player1), get_invis_level(player2));
	char_printf(ch, "Detect Invis Level:   {r%3d{x / {b%3d{x\n",
			get_detect_invis_level(player1), get_detect_invis_level(player2));
}

DO_FUN(do_pload) {
	DESCRIPTOR_DATA *d = NULL;
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		do_help(ch, "'WIZ PLOAD'");
		return;
	}

	victim = get_char_world_unrestricted(arg);

	if (victim != NULL) {
		char_puts("But they are already in the realm!\n",ch);
		return;
	}

	d = new_descriptor();

	d->connected	= CON_DUMMY;

	if (!load_char_obj(d, arg)) {
		char_puts("A soul by that name can't be found in the astral plane\n",ch);
		free_descriptor(d);
		return;
	}

	d->character->desc = d;
	d->character->next = char_list;
	char_list          = d->character;

	if (d->character->in_room != NULL) {
		char_to_room( d->character, ch->in_room);
	}

	act( "$n summons $N's soul from the astral plane.",
		ch, NULL, d->character, TO_ROOM );
	act( "You summon $N's soul from the astral plane.",
		ch, NULL, d->character, TO_CHAR );

	if (d->character->pet != NULL) {
		char_to_room(d->character->pet,d->character->in_room);
		act("$n arrived, tethered to $r master's soul.",
			d->character->pet,NULL,NULL,TO_ALL);
	}
}

DO_FUN(do_punload) {
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	victim = get_char_world_unrestricted(arg);

	if (!victim) {
		char_puts("They are already wandering the astral plane\n",ch);
		return;
	}

	if (victim->desc->connected != CON_DUMMY) {
		char_puts("Whoa, you got a live one there!\n",ch);
		return;
	}

	act("$n releases $N's soul back into the astral plane.\n",
		ch, NULL, victim, TO_ROOM);
	act("You release $N's soul back into the astral plane.\n",
		ch, NULL, victim, TO_CHAR);

	if (victim->was_in_room) {
		char_to_room(victim, victim->was_in_room);
		if (victim->pet) {
			act("$N follows $r master.\n",
				ch, NULL, victim->pet, TO_ALL);
			char_to_room(victim->pet, victim->was_in_room);
		}
	}

	save_char_obj(victim, FALSE);
	do_quit(victim, "");
}

int zz_object_costs (CHAR_DATA *ch);
int zz_finger_armor_to_jewelery (CHAR_DATA *ch);
int zz_wealth_reduction (CHAR_DATA *ch);
int zz_mobs_material_flesh (CHAR_DATA *ch);
int zz_wear_about_armor (CHAR_DATA *ch);
int zz_mob_shopkeeper_profit (CHAR_DATA *ch);
int zz_onehanded_bows (CHAR_DATA *ch);

/*
 * Zsuzsu's One-Shot
 * this is a function that should be run exactly once.
 * usually it's to update the entire mud of something.
 */
DO_FUN(do_zzoneshot)
{
	bool sec = FALSE;
	if (strcmp(ch->name, "Zsuzsu")) {
		char_puts("Curious little bugger, aren't you. {Y:){z\n", ch);
		return;
	}

	if ((sec = IS_SET(ch->pcdata->wiznet, WIZ_SECURE)))
		REMOVE_BIT(ch->pcdata->wiznet, WIZ_SECURE);

	zz_object_costs(ch);
	zz_mob_shopkeeper_profit(ch);
	char_puts("Remember to asave the changes, sweetie.\n", ch);

	if (sec)
		SET_BIT(ch->pcdata->wiznet, WIZ_SECURE);
}

/* set all obj's cost for certain types*/
int zz_object_costs (CHAR_DATA *ch)
{
	extern int top_obj_index;
	OBJ_INDEX_DATA *pObjIndex;
	int nMatch = 0;
	int vnum;
	int count = 0;
	int cost = 0;
	int rcost = 0;

	for (vnum = 0; nMatch < top_obj_index; vnum++)
		if ((pObjIndex = get_obj_index(vnum)) != NULL) {
			nMatch++;

			if (pObjIndex->vnum < 200)
				continue;

			if ((cost = get_autocost(pObjIndex))) {
				rcost = number_range(cost * 90/100, cost * 110/100);
				if (pObjIndex->vnum >= 17600
				&& pObjIndex->vnum < 17700)
					rcost = rcost / 10;

				rcost = UMAX(5, rcost);
				LOG("changing: %d from %d sug %d to %d", vnum,
					pObjIndex->cost, cost, rcost);
				pObjIndex->cost = rcost;
				count++;
			}
		}
	
	char_printf(ch, "Item costs changed: {c%d{x.\n", count);
	return count;
}

/* turn all wear_about items into clothing instead of armor */
int zz_wear_about_armor (CHAR_DATA *ch)
{
	extern int top_obj_index;
	OBJ_INDEX_DATA *pObjIndex;
	int nMatch = 0;
	int vnum;
	int count = 0;

	for (vnum = 0; nMatch < top_obj_index; vnum++)
		if ((pObjIndex = get_obj_index(vnum)) != NULL) {
			nMatch++;

			if (pObjIndex->vnum < 200)
				continue;

			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_ABOUT)
			&& pObjIndex->item_type == ITEM_ARMOR) {
				pObjIndex->item_type = ITEM_CLOTHING;
				LOG("changing: %d to clothing", vnum);
				count++;
			}
		}
	
	char_printf(ch, "wear_about Armors changed: {c%d{x.\n", count);
	return count;
}

/* change rings to jewelery type */
int zz_finger_armor_to_jewelery (CHAR_DATA *ch)
{
	extern int top_obj_index;
	OBJ_INDEX_DATA *pObjIndex;
	int nMatch = 0;
	int vnum;
	int count = 0;

	for (vnum = 0; nMatch < top_obj_index; vnum++)
		if ((pObjIndex = get_obj_index(vnum)) != NULL) {
			nMatch++;

			if (pObjIndex->vnum < 200)
				continue;
			
			if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_FINGER)
			&& pObjIndex->item_type == ITEM_ARMOR) {
				LOG("changing: %d", vnum);
				pObjIndex->item_type = ITEM_JEWELRY;
				count++;
			}
		}

	char_printf(ch, "Items changed: {c%d{x.\n", count);
	return count;
}

/* change rings to jewelery type */
int zz_onehanded_bows (CHAR_DATA *ch)
{
	extern int top_obj_index;
	OBJ_INDEX_DATA *pObjIndex;
	int nMatch = 0;
	int vnum;
	int count = 0;

	for (vnum = 0; nMatch < top_obj_index; vnum++)
		if ((pObjIndex = get_obj_index(vnum)) != NULL) {
			nMatch++;

			if (pObjIndex->vnum < 200)
				continue;
			
			if (pObjIndex->item_type == ITEM_WEAPON
			&& !IS_SET(pObjIndex->value[ITEM_WEAPON_FLAGS], WEAPON_TWO_HANDS)
			&& (pObjIndex->value[ITEM_WEAPON_TYPE] == WEAPON_BOW
			|| pObjIndex->value[ITEM_WEAPON_TYPE] == WEAPON_POLEARM
			|| pObjIndex->value[ITEM_WEAPON_TYPE] == WEAPON_BASTARDSWORD)) {
				SET_BIT(pObjIndex->value[ITEM_WEAPON_FLAGS], WEAPON_TWO_HANDS);
				LOG("adding twohanded: %d", vnum);
				count++;
			}
			if (pObjIndex->item_type == ITEM_WEAPON
			&& IS_SET(pObjIndex->value[ITEM_WEAPON_FLAGS], WEAPON_TWO_HANDS)
			&& (pObjIndex->value[ITEM_WEAPON_TYPE] == WEAPON_LONGSWORD
			|| pObjIndex->value[ITEM_WEAPON_TYPE] == WEAPON_WHIP)) {
				REMOVE_BIT(pObjIndex->value[ITEM_WEAPON_FLAGS], WEAPON_TWO_HANDS);
				LOG("removing twohanded: %d", vnum);
				count++;
			}
		}

	char_printf(ch, "Items changed: {c%d{x.\n", count);
	return count;
}

/* reduce all mob's wealth */
int zz_wealth_reduction (CHAR_DATA *ch) 
{
	int count = 0;
	int vnum;
	MOB_INDEX_DATA *pMobIndex;
	for (vnum = 200; vnum <= 120000; vnum++) {
		if ((pMobIndex = get_mob_index(vnum)) != NULL) {
			if ((pMobIndex->wealth/100) > ((pMobIndex->level)/2)) {
				if (pMobIndex->level > 91 && pMobIndex->pShop)
					continue;
				if (pMobIndex->mana[2] == 95)
					pMobIndex->wealth = UMIN(pMobIndex->wealth, number_range(pMobIndex->level * 30, pMobIndex->level * 60));
				else
					pMobIndex->wealth = UMIN(pMobIndex->wealth, number_range(0, pMobIndex->level * 50));
				count++;
			}
		}
	}
	char_printf(ch, "Mobs changed: {c%d{x.\n", count);
	return count;
}

/* take all mobs with an 'unknown' material and make them
 * 'flesh'.
 */
int zz_mobs_material_flesh (CHAR_DATA *ch) 
{
	int count = 0;
	int vnum;
	MOB_INDEX_DATA *pMobIndex;
	char_printf(ch, "Changing mob material to flesh if not set.\n");

	for (vnum = 0; vnum <= 120000; vnum++) {
		if ((pMobIndex = get_mob_index(vnum)) != NULL) {
			if (IS_NULLSTR(pMobIndex->material_descr)
			&& pMobIndex->material == MATERIAL(MAT_UNKNOWN)) {
				pMobIndex->material = MATERIAL(material_lookup_name("flesh"));
				count++;
			}
		}
	}
	char_printf(ch, "Mobs changed: {c%d{x.\n", count);
	return count;
}

/* shopkeepers profit */
int zz_mob_shopkeeper_profit (CHAR_DATA *ch) 
{
	int count = 0;
	int vnum = 0;
	MOB_INDEX_DATA *pMobIndex;

	for (vnum = 0; vnum <= 120000; vnum++) {
		if ((pMobIndex = get_mob_index(vnum)) != NULL) {
			if (pMobIndex->pShop) {
				if (pMobIndex->vnum >= 17600
				&& pMobIndex->vnum < 17700)

					pMobIndex->pShop->profit_sell = number_range(75, 90);
				else
					pMobIndex->pShop->profit_sell = number_range(10, 20);
				count++;
			}
		}
	}
	char_printf(ch, "Shopkeeper profit changed: {c%d{x.\n", count);
	return count;
}

/*
 * if the obj has a desc, and it matches an existing material, 
 * assign the material to that type, and remove the description.
 */
int zz_obj_material_descr (CHAR_DATA *ch) 
{
	extern int top_obj_index;
	OBJ_INDEX_DATA *pObjIndex;
	int nMatch = 0;
	int vnum;
	int count = 0;

	for (vnum = 0; nMatch < top_obj_index; vnum++)
		if ((pObjIndex = get_obj_index(vnum)) != NULL) {
			nMatch++;
		}

	char_printf(ch, "Items changed: {c%d{x.\n", count);
	return count;
}
