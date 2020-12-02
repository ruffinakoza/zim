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
 * $Id: olc_obj.c 1017 2007-02-08 23:48:46Z zsuzsu $
 */

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "debug.h"
#include "olc.h"
#include "db/db.h"

#define EDIT_OBJ(ch, obj)	(obj = (OBJ_INDEX_DATA*) ch->desc->pEdit)

DECLARE_OLC_FUN(objed_create		);
DECLARE_OLC_FUN(objed_edit		);
DECLARE_OLC_FUN(objed_touch		);
DECLARE_OLC_FUN(objed_show		);
DECLARE_OLC_FUN(objed_list		);
DECLARE_OLC_FUN(objed_del		);

DECLARE_OLC_FUN(objed_name		);
DECLARE_OLC_FUN(objed_short		);
DECLARE_OLC_FUN(objed_long		);
DECLARE_OLC_FUN(objed_addaffect		);
DECLARE_OLC_FUN(objed_addapply		);
DECLARE_OLC_FUN(objed_delaffect		);
DECLARE_OLC_FUN(objed_value0		);
DECLARE_OLC_FUN(objed_value1		);
DECLARE_OLC_FUN(objed_value2		);
DECLARE_OLC_FUN(objed_value3		);
DECLARE_OLC_FUN(objed_value4		);
DECLARE_OLC_FUN(objed_value5		);
DECLARE_OLC_FUN(objed_value6		);
DECLARE_OLC_FUN(objed_weight		);
DECLARE_OLC_FUN(objed_limit		);
DECLARE_OLC_FUN(objed_cost		);
DECLARE_OLC_FUN(objed_exd		);

DECLARE_OLC_FUN(objed_extra		);
DECLARE_OLC_FUN(objed_wear		);
DECLARE_OLC_FUN(objed_type		);
DECLARE_OLC_FUN(objed_affect		);
DECLARE_OLC_FUN(objed_material		);
DECLARE_OLC_FUN(objed_matdesc		);
DECLARE_OLC_FUN(objed_level		);
DECLARE_OLC_FUN(objed_condition		);
DECLARE_OLC_FUN(objed_clan		);
DECLARE_OLC_FUN(objed_clone		);
DECLARE_OLC_FUN(objed_sex		);
DECLARE_OLC_FUN(objed_race          );
DECLARE_OLC_FUN(objed_class         );
DECLARE_OLC_FUN(objed_size         );
DECLARE_OLC_FUN(objed_autoweapon	);
DECLARE_OLC_FUN(objed_autoarmor		);

DECLARE_VALIDATE_FUN(validate_condition);

olc_cmd_t olc_cmds_obj[] =
{
/*	{ command	function		arg			}, */

	{ 2, "create",		objed_create				},
	{ 2, "edit",		objed_edit				},
	{ 2, "touch",		objed_touch				},
	{ 0, "show",		objed_show				},
	{ 0, "list",		objed_list				},
	{ 2, "delete_ob",	olced_spell_out				},
	{ 2, "delete_obj",	objed_del				},

	{ 2, "addaffect",	objed_addaffect				},
	{ 2, "addapply",	objed_addapply				},
	{ 2, "cost",		objed_cost				},
	{ 2, "delaffect",	objed_delaffect				},
	{ 2, "exd",		objed_exd				},
	{ 2, "long",		objed_long				},
	{ 2, "name",		objed_name				},
	{ 2, "short",		objed_short				},
	{ 2, "v0",		objed_value0				},
	{ 2, "v1",		objed_value1				},
	{ 2, "v2",		objed_value2				},
	{ 2, "v3",		objed_value3				},
	{ 2, "v4",		objed_value4				},
	{ 2, "v5",		objed_value5				},
	{ 2, "v6",		objed_value6				},
	{ 2, "weight",		objed_weight				},
	{ 2, "limit",		objed_limit				},

	{ 2, "extra",		objed_extra,		extra_flags	},
	{ 2, "wear",		objed_wear,		wear_flags	},
	{ 2, "type",		objed_type,		item_types	},
	{ 2, "material",	objed_material 				},
	{ 2, "matdesc",		objed_matdesc				},
	{ 2, "level",		objed_level				},
	{ 2, "condition",	objed_condition,	validate_condition},
	{ 2, "clan",		objed_clan				},
	{ 2, "clone",		objed_clone				},
	{ 2, "autoweapon",	objed_autoweapon			},
	{ 2, "autoarmor",	objed_autoarmor				},
	{ 2, "sex",		objed_sex,		rsex_flags	},
	{ 2, "race",		objed_race,             rrace_flags     },
	{ 2, "class", 		objed_class,            rclass_flags    },
	{ 2, "size", 		objed_size,             rsize_flags     },

	{ 0, "version",		show_version				},
	{ 0, "commands",	show_commands				},

	{ 0, NULL }
};

static void	show_obj_values	(BUFFER *output, OBJ_INDEX_DATA *pObj);
static int	set_obj_values	(BUFFER *output, OBJ_INDEX_DATA *pObj,
				 const char *argument, int value_num);
static void	show_spells	(BUFFER *output, int tar);

OLC_FUN(objed_create)
{
	OBJ_INDEX_DATA *pObj;
	AREA_DATA *pArea;
	int  value;
	int  iHash;
	char arg[MAX_STRING_LENGTH];

	one_argument(argument, arg, sizeof(arg));
	value = atoi(arg);
	if (!value) {
		do_help(ch, "'OLC CREATE'");
		return FALSE;
	}

	pArea = area_vnum_lookup(value);
	if (!pArea) {
		char_puts("ObjEd: That vnum is not assigned an area.\n", ch);
		return FALSE;
	}

	if (!IS_BUILDER(ch, pArea)) {
		char_puts("ObjEd: Insufficient security.\n", ch);
		return FALSE;
	}

	if (get_obj_index(value)) {
		char_puts("ObjEd: Object vnum already exists.\n", ch);
		return FALSE;
	}
		 
	pObj			= new_obj_index();
	pObj->vnum		= value;
		 
	if (value > top_vnum_obj)
		top_vnum_obj = value;

	iHash			= value % MAX_KEY_HASH;
	pObj->next		= obj_index_hash[iHash];
	obj_index_hash[iHash]	= pObj;

	ch->desc->pEdit		= (void *)pObj;
	OLCED(ch)		= olced_lookup(ED_OBJ);
	touch_area(pArea);
	char_puts("ObjEd: Object created.\n", ch);
	return FALSE;
}

OLC_FUN(objed_edit)
{
	char arg[MAX_INPUT_LENGTH];
	int value;
	OBJ_INDEX_DATA *pObj;
	AREA_DATA *pArea;

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		do_help(ch, "'OLC EDIT'");
		return FALSE;
	}

	value = atoi(arg);
	pObj = get_obj_index(value);
	if (!pObj) {
		char_puts("ObjEd: Vnum does not exist.\n", ch);
		return FALSE;
	}

	pArea = area_vnum_lookup(pObj->vnum);
	if (!IS_BUILDER(ch, pArea)) {
		char_puts("ObjEd: Insufficient security.\n", ch);
	       	return FALSE;
	}

	ch->desc->pEdit = (void*) pObj;
	OLCED(ch)	= olced_lookup(ED_OBJ);
	return FALSE;
}

OLC_FUN(objed_touch)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return touch_vnum(pObj->vnum);
}

OLC_FUN(objed_show)
{
	char arg[MAX_INPUT_LENGTH];
	OBJ_INDEX_DATA	*pObj;
	AREA_DATA	*pArea;
	AFFECT_DATA *paf;
	int cnt;
	BUFFER *output;
	clan_t *clan;
	int bp, bpmax;

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		if (IS_EDIT(ch, ED_OBJ))
			EDIT_OBJ(ch, pObj);
		else {
			do_help(ch, "'OLC EDIT'");
			return FALSE;
		}
	}
	else {
		int value = atoi(arg);
		pObj = get_obj_index(value);
		if (!pObj) {
			char_puts("ObjEd: Vnum does not exist.\n", ch);
			return FALSE;
		}
	}

	pArea = area_vnum_lookup(pObj->vnum);

	output = buf_new(-1);
	buf_printf(output, "Vnum:              {D[{C%5d{D]{x\n",
		pObj->vnum);

	buf_printf(output, "Name:              {D[{c%s{D]{x\n",
		pObj->name);

	buf_printf(output, "Area:              {D[{c%5d{D]{x {%c%s{x\n",
		pArea->vnum, 
		IS_SET(pArea->flags, AREA_CLOSED)      ? 'R' :
		IS_SET(pArea->flags, AREA_PLAYERHOUSE) ? 'G' :
		IS_SET(pArea->flags, AREA_HIDDEN)      ? 'D' : 'x',
		pArea->name);

	buf_printf(output, "Type:              {D[{c%s{D]{x\n",
		flag_string(item_types, pObj->item_type));

        buf_printf(output, "Sex Restriction:   {D[{c%s{D]{x\n",
		flag_string(rsex_flags, pObj->rsex));

        buf_printf(output, "Size Restriction:  {D[{c%s{D]{x\n",
 		flag_string(rsize_flags, pObj->rsize));

        buf_printf(output, "Race Restriction:  {D[{c%s{D]{x\n",
                flag_string(rrace_flags, pObj->rrace));
  
        buf_printf(output, "Class Restriction: {D[{c%s{D]{x\n",
 		flag_string(rclass_flags, pObj->rclass));

	if (pObj->clan && (clan = clan_lookup(pObj->clan))) 
		buf_printf(output, 
			   "Clan:              {D[{c%s{D]{x\n", clan->name);

	if (pObj->limit != -1)
		buf_printf(output, "Limit:             {D[{c%5d{D]{x\n", pObj->limit);
	else
		buf_add(output, "Limit:             {D[{cnone{D]{x\n");

	buf_printf(output, "Level:             {D[{c%5d{D]{x\n", pObj->level);

	buf_printf(output, "Wear flags:        {D[{c%s{D]{x\n",
		flag_string(wear_flags, pObj->wear_flags));

	buf_printf(output, "Extra flags:       {D[{c%s{D]{x\n",
		flag_string(extra_flags, pObj->extra_flags));

	buf_printf(output, "Material:          {D[{c%s{D]{x\n",
		(pObj->material) ? pObj->material->name : "null");
	buf_printf(output, "MatDesc:           {D[{x%s{D]{x\n", 
		pObj->material_descr);

	buf_printf(output, "Weight:            {D[{c%4d.%01d{D] lbs{x\n"
			   "Cost:              {D[{Y%3d{x%02d{D] ",
		pObj->weight / 10, 
		pObj->weight % 10,
		pObj->cost / 100,
		pObj->cost % 100);

	if (IS_SET(pObj->extra_flags, ITEM_QUEST))
		buf_printf(output, 
			"QP return % by level: 100% = level QPs{x\n");
	else
		buf_printf(output, 
			"sug: {Y%d{x gold {W%d{x silver\n",
			get_autocost(pObj) / 100,
			get_autocost(pObj) % 100);

	buf_printf(output, "Condition:         {D[{c%5d{D]{x\n",         /* ROM */
		pObj->condition);
	if (pObj->ed) {
		ED_DATA *ed;

		buf_add(output, "Ex desc kwd: ");

		for (ed = pObj->ed; ed; ed = ed->next)
			buf_printf(output, "{D[{c%s{D]{x", ed->keyword);

		buf_add(output, "\n");
	}

	buf_add(output, "\n");

	mlstr_dump(output, "Short desc:  ", pObj->short_descr);
	mlstr_dump(output, "Long desc:   ", pObj->description);
	buf_add(output, "\n");

	show_obj_values(output, pObj);

	buf_add(output, "\n");

	for (cnt = 0, paf = pObj->affected; paf; paf = paf->next) {
		where_t *w = where_lookup(paf->where);

		if (cnt == 0) {
			buf_add(output, "Number      Affects Modifier Affects Bitvector\n");
			buf_add(output, "{D------ ------------ -------- ------- -----------------------------------------{x\n");
		}
		buf_printf(output, "{D[{x%4d{D] {c%12.12s {C%8d {c%7.7s %s{x"
				   "\n",
			   cnt,
			   flag_string(apply_flags, paf->location),
			   paf->modifier,
			   flag_string(apply_types, paf->where),
			   w ? flag_string(w->table, paf->bitvector) : "none");
		cnt++;
	}
	/*Zz*/
	build_points(pObj, &bp, &bpmax);
	if (cnt != 0)
		buf_add(output, "\n");
	
	buf_printf(output, "Build Points:              {%c%d {Dmax: %d{x\n",
		build_point_percent_color(bp, bpmax),
		bp, 
		bpmax);

	page_to_char(buf_string(output), ch);
	buf_free(output);

	return FALSE;
}

OLC_FUN(objed_list)
{
	OBJ_INDEX_DATA	*pObjIndex;
	AREA_DATA	*pArea;
	BUFFER		*buffer;
	const char *	name;
	char		arg  [MAX_INPUT_LENGTH];
	char		buf  [MAX_INPUT_LENGTH];
	int ansi_len = 0;
	bool fAll, found;
	int vnum;
	int  col = 0;
	int bp, bpmax;
	int count = 0;
	int bp_tot = 0, 
	    bpmax_tot = 0;
	int limiteds = 0,
	    rares = 0;

	one_argument(argument, arg, sizeof(arg));
/*
	if (arg[0] == '\0') {
		arg = "all";
		do_help(ch, "'OLC ALIST'");
		return FALSE;
	}
*/

	if ((pArea = get_edited_area(ch)) == NULL)
		pArea = ch->in_room->area;

	buffer  = buf_new(-1);
	fAll    = arg[0] == '\0' || !str_cmp(arg, "all");
	found   = FALSE;

	for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++) {
		if ((pObjIndex = get_obj_index(vnum))) {
			if (fAll || is_name(arg, pObjIndex->name)
			|| flag_value(item_types, arg) == pObjIndex->item_type) {

				found = TRUE;
				build_points(pObjIndex, &bp, &bpmax);
				bp_tot += bp;
				bpmax_tot += bpmax;
				count++;
				if (IS_OBJ_LIMITED(pObjIndex))
					limiteds++;

				if (IS_SET(pObjIndex->extra_flags, ITEM_RARE))
					rares++;

				/*format: "{%c[{%c%5d{%c]{x %-17.16s{x"*/
				name = mlstr_mval(pObjIndex->short_descr);
				ansi_len = astrlen(name, 17);

				snprintf(buf, sizeof(buf),
					"{%%c[{%%c%%5d{%%c]{x %%-%d.%ds{x",
					17 + ansi_len,
					16 + ansi_len);

				buf_printf(buffer, buf,
					(IS_OBJ_LIMITED(pObjIndex)) ? 'B' :
					(IS_OBJ_RARE(pObjIndex))    ? 'b'
								    : 'D',	
					build_point_percent_color(bp, bpmax),
					pObjIndex->vnum,
					(IS_OBJ_LIMITED(pObjIndex)) ? 'B' :
					(IS_OBJ_RARE(pObjIndex))    ? 'b'
								    : 'D',	
					name);
				if (++col % 3 == 0)
					buf_add(buffer, "\n");
			}
		}
	}

	if (!found)
		char_puts("Object(s) not found in this area.\n", ch);
	else {
		if (col % 3 != 0)
			buf_add(buffer, "\n");

		buf_printf(buffer, "Average Build Points: {%c%d{x%%{x   "
				   "Rares: {c%d{x   Limiteds: {c%d{x",
				build_point_percent_color(bp_tot, bpmax_tot),
				(!bpmax_tot) ? 0 : (bp_tot * 100) / bpmax_tot,
				rares, limiteds);

		page_to_char(buf_string(buffer), ch);
	}

	buf_free(buffer);
	return FALSE;
}

OLC_FUN(objed_del)
{
	OBJ_INDEX_DATA *pObj;
	OBJ_DATA *obj, *obj_next;
	AREA_DATA *area;
	int i;
	bool error = FALSE;

	EDIT_OBJ(ch, pObj);

	if (olced_busy(ch, ED_OBJ, pObj, NULL))
		return FALSE;

/* check that pObj is not in resets */
	for (i = 0; i < MAX_KEY_HASH; i++) {
		ROOM_INDEX_DATA *room;

		for (room = room_index_hash[i]; room; room = room->next) {
			int j = 0;
			RESET_DATA *reset;

			for (reset = room->reset_first; reset;
							reset = reset->next) {
				bool found = FALSE;

				j++;
				switch (reset->command) {
				case 'P':
					if (reset->arg3 == pObj->vnum)
						found = TRUE;

					/* FALLTHRU */

				case 'O':
				case 'G':
				case 'E':
					if (reset->arg1 == pObj->vnum)
						found = TRUE;
					break;
				}

				if (!found)
					continue;

				if (!error) {
					error = TRUE;
					char_puts("ObjEd: can't delete obj "
						  "index: delete the "
						  "following resets:\n", ch);
				}

				char_printf(ch, "ObjEd: room %d, reset %d\n",
					    room->vnum, j);
			}
		}
	}

	if (error)
		return FALSE;

/* delete all the instances of obj index */
	for (obj = object_list; obj; obj = obj_next) {
		obj_next = obj->next;

		if (obj->pIndexData == pObj)
			extract_obj(obj, XO_F_NORECURSE);
	}

	if ((area = area_vnum_lookup(pObj->vnum)))
		touch_area(area);

/* delete obj index itself */
	i = pObj->vnum % MAX_KEY_HASH;
	if (pObj == obj_index_hash[i])
		obj_index_hash[i] = pObj->next;
	else {
		OBJ_INDEX_DATA *prev;

		for (prev = obj_index_hash[i]; prev; prev = prev->next)
			if (prev->next == pObj)
				break;

		if (prev)
			prev->next = pObj->next;
	}

	free_obj_index(pObj);
	char_puts("ObjEd: Obj index deleted.\n", ch);
	edit_done(ch->desc);
	return FALSE;
}

/*
 * Need to issue warning if flag isn't valid. -- does so now -- Hugin.
 */
OLC_FUN(objed_addaffect)
{
	int location;
	int modifier;
	flag32_t where;
	flag64_t bitvector;
	OBJ_INDEX_DATA *pObj;
	AFFECT_DATA *pAf;
	char loc[MAX_STRING_LENGTH];
	char mod[MAX_STRING_LENGTH];
	char wh[MAX_STRING_LENGTH];

	EDIT_OBJ(ch, pObj);

	argument = one_argument(argument, loc, sizeof(loc));
	argument = one_argument(argument, mod, sizeof(mod));
	argument = one_argument(argument, wh, sizeof(wh));

	if (loc[0] == '\0') {
		do_help(ch, "'OLC ADDAFFECT'");
		return FALSE;
	}

	if (!str_cmp(loc, "none")) {
		location = APPLY_NONE;
		modifier = 0;
	}
	else {
		if ((location = flag_value(apply_flags, loc)) < 0) {
			char_puts("Valid locations are:\n", ch);
			show_flags(ch, apply_flags);
			return FALSE;
		}

		if (!is_number(mod)) {
			do_help(ch, "'OLC ADDAFFECT'");
			return FALSE;
		}
		modifier = atoi(mod);
	}

	if (wh[0] == '\0') {
		where = -1;
		bitvector = 0;
	}
	else {
		where_t *w;

		if ((where = flag_value(apply_types, wh)) < 0) {
			char_puts("Valid bitaffect locations are:\n", ch);
			show_flags(ch, apply_types);
			return FALSE;
		}

		if ((w = where_lookup(where)) == NULL) {
			char_printf(ch, "%s: not in where_table.\n",
				    flag_string(apply_types, where));
			return FALSE;
		}

		if ((bitvector = flag_value(w->table, argument)) == 0) {
			char_printf(ch, "Valid '%s' bitaffect flags are:\n",
				    flag_string(apply_types, where));
			show_flags(ch, w->table);
			return FALSE;
		}
	}

	/* Zz */
	/* if not "unknown" affect, then add to value instead of
	 * adding another affect
	 */
	pAf = pObj->affected;
	while (pAf) {
		if (pAf->location != APPLY_NONE
		&& pAf->location == location) {
			pAf->modifier += modifier;
			char_puts("Previous affect adjusted.\n", ch);
			break;
		}
		pAf = pAf->next;
	}
	if (!pAf) {
		pAf             = aff_new();
		pAf->location   = location;
		pAf->modifier   = modifier;
		pAf->where	= where;
		pAf->type       = -1;
		pAf->duration   = -1;
		pAf->bitvector  = bitvector;
		pAf->level      = pObj->level;
		pAf->next       = pObj->affected;
		pObj->affected  = pAf;
		char_puts("New affect added.\n", ch);
	}

	return TRUE;
}

OLC_FUN(objed_addapply)
{
	int location, bv, where;
	OBJ_INDEX_DATA *pObj;
	AFFECT_DATA *pAf;
	where_t *wd;
	char loc[MAX_STRING_LENGTH];
	char mod[MAX_STRING_LENGTH];
	char type[MAX_STRING_LENGTH];
	char bvector[MAX_STRING_LENGTH];

	EDIT_OBJ(ch, pObj);

	argument = one_argument(argument, type, sizeof(type));
	argument = one_argument(argument, loc, sizeof(loc));
	argument = one_argument(argument, mod, sizeof(mod));
	one_argument(argument, bvector, sizeof(bvector));

	if ((where = flag_value(apply_types, type)) < 0) {
		char_puts("Invalid apply type. Valid apply types are:\n", ch);
		show_flags(ch, apply_types);
		return FALSE;
	}

	if ((location = flag_value(apply_flags, loc)) < 0) {
		char_puts("Valid applies are:\n", ch);
		show_flags(ch, apply_flags);
		return FALSE;
	}

	if ((wd = where_lookup(where)) == NULL) {
		char_puts("ObjEd: bit vector table undefined. "
			  "Report it to implementors.\n", ch);
		return FALSE;
	}

	if ((bv = flag_value(wd->table, bvector)) == 0) {
		char_puts("Valid bitvector types are:\n", ch);
		show_flags(ch, wd->table);
		return FALSE;
	}

	if (!is_number(mod)) {
		char_puts("Syntax: addapply type location "
			  "mod bitvector\n", ch);
		return FALSE;
	}

	pAf             = aff_new();
	pAf->location   = location;
	pAf->modifier   = atoi(mod);
	pAf->where	= where;
	pAf->type	= -1;
	pAf->duration   = -1;
	pAf->bitvector  = bv;
	pAf->level      = pObj->level;
	pAf->next       = pObj->affected;
	pObj->affected  = pAf;

	char_puts("Apply added.\n", ch);
	return TRUE;
}

/*
 * My thanks to Hans Hvidsten Birkeland and Noam Krendel(Walker)
 * for really teaching me how to manipulate pointers.
 */
OLC_FUN(objed_delaffect)
{
	OBJ_INDEX_DATA *pObj;
	AFFECT_DATA *pAf;
	AFFECT_DATA *pAf_next;
	char affect[MAX_STRING_LENGTH];
	int  value;
	int  cnt = 0;

	EDIT_OBJ(ch, pObj);

	one_argument(argument, affect, sizeof(affect));

	if (!is_number(affect) || affect[0] == '\0')
	{
		char_puts("Syntax:  delaffect [#xaffect]\n", ch);
		return FALSE;
	}

	value = atoi(affect);

	if (value < 0)
	{
		char_puts("Only non-negative affect-numbers allowed.\n", ch);
		return FALSE;
	}

	if (!(pAf = pObj->affected))
	{
		char_puts("ObjEd:  Non-existant affect.\n", ch);
		return FALSE;
	}

	if(value == 0)	/* First case: Remove first affect */
	{
		pAf = pObj->affected;
		pObj->affected = pAf->next;
		aff_free(pAf);
	}
	else		/* Affect to remove is not the first */
	{
		while ((pAf_next = pAf->next) && (++cnt < value))
			 pAf = pAf_next;

		if(pAf_next)		/* See if it's the next affect */
		{
			pAf->next = pAf_next->next;
			aff_free(pAf_next);
		}
		else                                 /* Doesn't exist */
		{
			 char_puts("No such affect.\n", ch);
			 return FALSE;
		}
	}

	char_puts("Affect removed.\n", ch);
	return TRUE;
}

OLC_FUN(objed_name)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_name(ch, argument, cmd, &pObj->name);
}

OLC_FUN(objed_short)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_mlstr(ch, argument, cmd, &pObj->short_descr);
}

OLC_FUN(objed_long)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_mlstr(ch, argument, cmd, &pObj->description);
}

/*****************************************************************************
 Name:		objed_values
 Purpose:	Finds the object and sets its value.
 Called by:	The four valueX functions below. (now five -- Hugin)
 ****************************************************************************/
bool objed_values(CHAR_DATA *ch, const char *argument, int value)
{
	BUFFER *output;
	int errcode = 1;
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);

	output = buf_new(-1);
	if (argument[0] == '\0'
	||  (errcode = set_obj_values(output, pObj, argument, value)) < 2)
		show_obj_values(output, pObj);
	page_to_char(buf_string(output), ch);
	buf_free(output);
	return !errcode;
}

OLC_FUN(objed_value0)
{
	return objed_values(ch, argument, 0);
}

OLC_FUN(objed_value1)
{
	return objed_values(ch, argument, 1);
}

OLC_FUN(objed_value2)
{
	return objed_values(ch, argument, 2);
}

OLC_FUN(objed_value3)
{
	return objed_values(ch, argument, 3);
}

OLC_FUN(objed_value4)
{
	return objed_values(ch, argument, 4);
}

OLC_FUN(objed_value5)
{
	return objed_values(ch, argument, 5);
}

OLC_FUN(objed_value6)
{
	return objed_values(ch, argument, 6);
}

OLC_FUN(objed_weight)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_number(ch, argument, cmd, &pObj->weight);
}

OLC_FUN(objed_limit)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_number(ch, argument, cmd, &pObj->limit);
}

OLC_FUN(objed_cost)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_number(ch, argument, cmd, &pObj->cost);
}

OLC_FUN(objed_exd)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_exd(ch, argument, cmd, &pObj->ed);
}

OLC_FUN(objed_extra)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_flag64(ch, argument, cmd, &pObj->extra_flags);
}

OLC_FUN(objed_wear)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_flag32(ch, argument, cmd, &pObj->wear_flags);
}

OLC_FUN(objed_type)
{
	bool changed;
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	changed = olced_flag32(ch, argument, cmd, &pObj->item_type);
	if (changed) {
		pObj->value[0] = 0;
		pObj->value[1] = 0;
		pObj->value[2] = 0;
		pObj->value[3] = 0;
		pObj->value[4] = 0;
		pObj->value[5] = 0;
		pObj->value[6] = 0;

		switch (pObj->item_type) {
			case ITEM_CONTAINER:
				pObj->value[ITEM_CONTAINER_WEIGHT_MULTI] = 100;
				break;
		}
	}
	return changed;
}

OLC_FUN(objed_matdesc)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_str(ch, argument, cmd, &pObj->material_descr);
}

OLC_FUN(objed_level)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_number(ch, argument, cmd, &pObj->level);
}

OLC_FUN(objed_condition)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_number(ch, argument, cmd, &pObj->condition);
}

OLC_FUN(objed_material)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_material(ch, argument, cmd, &pObj->material);
}

OLC_FUN(objed_clan)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_clan(ch, argument, cmd, &pObj->clan);
}

OLC_FUN(objed_clone)
{
	OBJ_INDEX_DATA *pObj;
	OBJ_INDEX_DATA *pFrom;
	char arg[MAX_INPUT_LENGTH];
	int i;
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_next;
	AFFECT_DATA **ppaf;

	one_argument(argument, arg, sizeof(arg));
	if (!is_number(arg)) {
		char_puts("Syntax: clone <vnum>\n", ch);
		return FALSE;
	}

	i = atoi(arg);
	if ((pFrom = get_obj_index(i)) == NULL) {
		char_printf(ch, "ObjEd: %d: Vnum does not exist.\n", i);
		return FALSE;
	}

	EDIT_OBJ(ch, pObj);
	if (pObj == pFrom)
		return FALSE;

	free_string(pObj->name);
	pObj->name		= str_qdup(pFrom->name);
	pObj->material		= pFrom->material;
	free_string(pObj->material_descr);
	pObj->material_descr	= str_qdup(pFrom->material_descr);
	mlstr_free(pObj->short_descr);
	pObj->short_descr	= mlstr_dup(pFrom->short_descr);
	mlstr_free(pObj->description);
	pObj->description	= mlstr_dup(pFrom->description);

	pObj->item_type		= pFrom->item_type;
	pObj->extra_flags	= pFrom->extra_flags;
	pObj->wear_flags	= pFrom->wear_flags;
	pObj->level		= pFrom->level;
	pObj->condition		= pFrom->condition;
	pObj->weight		= pFrom->weight;
	pObj->cost		= pFrom->cost;
	pObj->limit		= pFrom->limit;
	pObj->clan		= pFrom->clan;

	for (i = 0; i <ITEM_VALUE_MAX; i++)
		pObj->value[i]	= pFrom->value[i];

/* copy affects */
	for (paf = pObj->affected; paf; paf = paf_next) {
		paf_next = paf->next;
		aff_free(paf);
	}

	ppaf = &pObj->affected;
	for (paf = pFrom->affected; paf; paf = paf->next) {
		*ppaf = aff_dup(paf);
		ppaf = &(*ppaf)->next;
	}

/* copy extra descriptions */
	ed_free(pObj->ed);
	pObj->ed = ed_dup(pFrom->ed);

	return TRUE;
}

OLC_FUN(objed_sex)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_flag32(ch, argument, cmd, &pObj->rsex);
}

OLC_FUN(objed_race)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
 	return olced_flag32(ch, argument, cmd, &pObj->rrace);
}

OLC_FUN(objed_class)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_flag32(ch, argument, cmd, &pObj->rclass);
}

OLC_FUN(objed_size)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);
	return olced_flag32(ch, argument, cmd, &pObj->rsize);
}

void show_obj_values(BUFFER *output, OBJ_INDEX_DATA *pObj)
{
	int avg_dnum, avg_dsize, autow_avg = 0, w_avg = 0;
	ROOM_INDEX_DATA *loc = NULL;
	OBJ_INDEX_DATA *key = NULL;

	switch(pObj->item_type) {
	default:	/* No values. */
		buf_add(output, "Currently edited obj has unknown item type.\n");
		/* FALLTHRU */

	case ITEM_TREASURE:
	case ITEM_CLOTHING:
	case ITEM_TRASH:
	case ITEM_KEY:
	case ITEM_BOAT:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
	case ITEM_PROTECT:
	case ITEM_MAP:
	case ITEM_WARP_STONE :
	case ITEM_ROOM_KEY:
	case ITEM_GEM:
	case ITEM_JEWELRY:
	case ITEM_JUKEBOX:
	case ITEM_TATTOO:
	case ITEM_MEDAL:
		break;

		     
	case ITEM_LIGHT:
		if (pObj->value[ITEM_LIGHT_DURATION] == -1)
			buf_printf(output, "{D[{xv2{D]{x Light:  {cInfinite[-1]{x\n");
		else
			buf_printf(output, "{D[{xv2{D]{x Light:  {D[{c%d{D]{x\n", 
				pObj->value[ITEM_LIGHT_DURATION]);
		break;

	case ITEM_WAND:
	case ITEM_STAFF:
		buf_printf(output,
			"{D[{xv0{D]{x Level:          {D[{c%d{D]{x\n"
			"{D[{xv1{D]{x Charges Total:  {D[{c%d{D]{x\n"
			"{D[{xv2{D]{x Charges Left:   {D[{c%d{D]{x\n"
			"{D[{xv3{D]{x Spell:          {c%s{x\n",
			pObj->value[ITEM_WAND_LEVEL],
			pObj->value[ITEM_WAND_CHARGES_TOTAL],
			pObj->value[ITEM_WAND_CHARGES_REMAINING],
			skill_name(pObj->value[ITEM_WAND_SPELL]));
		break;

	case ITEM_PORTAL:
		buf_printf(output,
			    "{D[{xv0{D]{x Charges:        {D[{c%d{D]\n"
			      "[{xv1{D]{x Exit Flags:     {c%s\n"
			    "{D[{xv2{D]{x Portal Flags:   {c%s\n"
			    "{D[{xv3{D]{x Goes to (vnum): {D[{c%d{D] {x%s\n"
			    "{D[{xv4{D]{x Key (vnum):     {D[{c%d{D] %s{x\n",
			    pObj->value[ITEM_PORTAL_CHARGES],
			    flag_string(exit_flags, pObj->value[ITEM_PORTAL_EXIT_FLAGS]),
			    flag_string(portal_flags , pObj->value[ITEM_PORTAL_FLAGS]),
			    pObj->value[ITEM_PORTAL_DEST],
			    ((loc = get_room_index(pObj->value[ITEM_PORTAL_DEST])) == NULL)
				? ""
				: mlstr_mval(loc->name),
			    pObj->value[ITEM_PORTAL_KEY],
			    ((key = get_obj_index(pObj->value[ITEM_PORTAL_KEY])) == NULL)
				? ""
				: OBJ_SHORTNAME(key));
		break;

	case ITEM_PARCHMENT:
		buf_printf(output,
			"{D[{xv1{D]{x Write Flags: {c%s{x\n",
		flag_string(write_flags, pObj->value[ITEM_PARCHMENT_FLAGS]));
		break;

			
	case ITEM_FURNITURE:          
		buf_printf(output,
			    "{D[{xv0{D]{x Max people:      {D[{c%d{D]\n"
			      "[{xv1{D]{x Max weight:      {D[{c%d{D]\n"
			      "[{xv2{D]{x Furniture Flags: {c%s\n"
			    "{D[{xv3{D]{x Heal rate:       {D[{c%d{D]\n"
			      "[{xv4{D]{x Mana rate:       {D[{c%d{D]{x\n",
			    pObj->value[ITEM_FURNITURE_QUANTITY],
			    pObj->value[ITEM_FURNITURE_WEIGHT],
			    flag_string(furniture_flags, pObj->value[ITEM_FURNITURE_FLAGS]),
			    pObj->value[ITEM_FURNITURE_HEAL_RATE],
			    pObj->value[ITEM_FURNITURE_MANA_RATE]);
		break;

	case ITEM_SCROLL:
	case ITEM_POTION:
	case ITEM_PILL:
		buf_printf(output,
			"{D[{xv0{D]{x Level:  {D[{c%d{D]\n"
			  "[{xv1{D]{x Spell:  {c%s\n"
			"{D[{xv2{D]{x Spell:  {c%s\n"
			"{D[{xv3{D]{x Spell:  {c%s\n"
			"{D[{xv4{D]{x Spell:  {c%s{x\n",
			pObj->value[ITEM_POTION_LEVEL],
			skill_name(pObj->value[ITEM_POTION_SPELL1]),
			skill_name(pObj->value[ITEM_POTION_SPELL2]),
			skill_name(pObj->value[ITEM_POTION_SPELL3]),
			skill_name(pObj->value[ITEM_POTION_SPELL4]));
		break;

/* ARMOR for ROM */

	case ITEM_ARMOR:
		buf_printf(output,
			"{D[{xv0{D]{x AC Pierce:      {D[{c%d{D]\n"
			  "[{xv1{D]{x AC Bash:        {D[{c%d{D]\n"
			  "[{xv2{D]{x AC Slash:       {D[{c%d{D]\n"
			  "[{xv3{D]{x AC Exotic:      {D[{c%d{D]\n"
			  "[{xv4{D]{x Armor Type:     {c%s\n"
			"{D[{xv5{D]{x Armor Flags:    {D[{c%s{D]{x\n",
			pObj->value[ITEM_ARMOR_AC_PIERCE],
			pObj->value[ITEM_ARMOR_AC_BASH],
			pObj->value[ITEM_ARMOR_AC_SLASH],
			pObj->value[ITEM_ARMOR_AC_EXOTIC],
			flag_string(armor_types, pObj->value[ITEM_ARMOR_TYPE]),
			flag_string(armor_flags, pObj->value[ITEM_ARMOR_FLAGS])
			);
		break;

/* WEAPON changed in ROM: */
/* I had to split the output here, I have no idea why, but it helped -- Hugin */
/* It somehow fixed a bug in showing scroll/pill/potions too ?! */
	case ITEM_WEAPON:
/*Zz*/
		autow_avg = get_autoweapon(pObj, &avg_dnum, &avg_dsize, 100);
		w_avg = (1 + pObj->value[ITEM_WEAPON_DICE_SIZE]) 
			* pObj->value[ITEM_WEAPON_DICE_NUM]/2;

		buf_printf(output, "Dam Average:         {%c%d {Dauto: %d (%d%%){x\n",
				w_avg > autow_avg * 1.20                ? 'R' :
				w_avg > autow_avg                       ? 'Y' : 
				w_avg > autow_avg * 0.90		? 'G' :
				w_avg > autow_avg * 0.80                ? 'B' : 'M',
				w_avg, autow_avg,
				(w_avg - autow_avg)*100 / autow_avg +100);

		buf_printf(output, 
			"{D[{xv0{D]{x Weapon class:   {c%s\n"
			"{D[{xv1{D]{x Number of dice: {D[{c%d{D]\n"
			  "[{xv2{D]{x Size of dice:   {D[{c%d{D]\n"
			  "[{xv3{D]{x Attack type:    {c%s\n"
			"{D[{xv4{D]{x Weapon Flags:   {c%s{x\n",
			flag_string(weapon_class, pObj->value[ITEM_WEAPON_TYPE]),
			pObj->value[ITEM_WEAPON_DICE_NUM],
			pObj->value[ITEM_WEAPON_DICE_SIZE],
			attack_table[pObj->value[ITEM_WEAPON_ATTACK_TYPE]].name,
			    flag_string(weapon_type2,  pObj->value[ITEM_WEAPON_FLAGS]));
		break;

	case ITEM_CONTAINER:
		buf_printf(output,
			"{D[{xv0{D]{x Total Weight:    {D[{c%d{x kg{D]\n"
			  "[{xv1{D]{x Flags:           {D[{c%s{D]\n"
			  "[{xv2{D]{x Key:             {D[{c%d{D] {c%s\n"
			"{D[{xv3{D]{x Per Item Weight: {D[{c%d{D]\n"
			  "[{xv4{D]{x Weight Mult:     {D[{c%d{D]\n"
			  "[{xv5{D]{x Max Quantity:    {D[{c%d{D] (0 unlimited){x\n",
			pObj->value[ITEM_CONTAINER_WEIGHT],
			flag_string(cont_flags, pObj->value[ITEM_CONTAINER_FLAGS]),
		        pObj->value[ITEM_CONTAINER_KEY],
			((key = get_obj_index(pObj->value[2])) == NULL)
				? ""
				: OBJ_SHORTNAME(key),
		        pObj->value[ITEM_CONTAINER_PER_ITEM_WEIGHT],
		        pObj->value[ITEM_CONTAINER_WEIGHT_MULTI],
		        pObj->value[ITEM_CONTAINER_QUANTITY]
			);

		if (IS_SET(pObj->extra_flags, ITEM_PIT))
			buf_printf(output, 
				"\n{Y*{x Cost of pit is trash compactor timer. {Y*{x\n");
		break;

	case ITEM_DRINK_CON:
	case ITEM_FOUNTAIN:
		buf_printf(output,
			    "{D[{xv0{D]{x Liquid Total: {D[{c%d{D]\n"
			      "[{xv1{D]{x Liquid Left:  {D[{c%d{D]\n"
			      "[{xv2{D]{x Liquid:       {c%s{x\n"
			    "{D[{xv3{D]{x Poisoned:     {c%s{x\n",
			    pObj->value[ITEM_DRINK_TOTAL],
			    pObj->value[ITEM_DRINK_REMAINING],
			    liq_table[pObj->value[ITEM_DRINK_TYPE]].liq_name,
			    pObj->value[ITEM_DRINK_POISON] ? "Yes" : "No");
		break;

	case ITEM_FOOD:
		buf_printf(output,
			"{D[{xv0{D]{x Food hours: {D[{c%d{D]\n"
			  "[{xv1{D]{x Full hours: {D[{c%d{D]\n"
			  "[{xv3{D]{x Poisoned:   {c%s{x\n",
			pObj->value[ITEM_FOOD_HOURS],
			pObj->value[ITEM_FOOD_FULL],
			pObj->value[ITEM_FOOD_POISON] ? "Yes" : "No");
		break;

	case ITEM_MONEY:
		buf_printf(output, "{D[{xv0{D]{x Silver: {D[{W%d{D]\n"
				     "[{xv1{D]{x Gold:   {D[{G%d{D]{x\n",
			   pObj->value[ITEM_MONEY_SILVER], 
			   pObj->value[ITEM_MONEY_GOLD]);
		break;

	case ITEM_AWARD:
		buf_printf(output, "{D[{xv0{D]{x Type: {D[{c%s{D]\n"
				     "[{xv1{D]{x Value:{D[{c%d{D]{x\n",
			flag_string(award_types, pObj->value[ITEM_AWARD_TYPE]),
			pObj->value[ITEM_AWARD_VALUE]);
		break;
	}
}

OLC_FUN(objed_autoweapon)
{
	OBJ_INDEX_DATA *pObj;
	EDIT_OBJ(ch, pObj);

	if (pObj->item_type != ITEM_WEAPON)
	{
		send_to_char( "{rAutoweapon only works on weapons...{x\n\r", ch);
		return FALSE;
	}

	if (pObj->level < 1)
	{
		send_to_char( "{cAutoweapon requires a level to be set on the weapon first.{x\n\r", ch);
		return FALSE;
	}

	get_autoweapon(pObj, &pObj->value[1], &pObj->value[2], 100);
	pObj->cost = 25 * (pObj->value[2] * (pObj->value[1] + 1)) + 20 * pObj->level;
	pObj->weight = pObj->level + 1;

	send_to_char("{cExperimental values set on weapon...{x\n\r", ch);
	return TRUE;
}

OLC_FUN(objed_autoarmor)
{
	OBJ_INDEX_DATA *pObj;
	int size;
	EDIT_OBJ(ch, pObj);

	if (pObj->item_type != ITEM_ARMOR)
	{
		send_to_char( "{rAutoArmor only works on Armor ...{x\n", ch);
		return FALSE;
	}

	if (pObj->level < 1)
	{
		send_to_char( "{cAutoArmor requires a level to be set on the armor first.{x\n", ch);
		return FALSE;
	}

	size = UMAX(1, pObj->level/2.8 + 1);

	pObj->weight = pObj->level + 1;
	pObj->cost = pObj->level^2 * 2;
	pObj->value[0] = size;
	pObj->value[1] = size;
	pObj->value[2] = size;
	pObj->value[3] = (size - 1);

	send_to_char( "{cAutoArmor has set experimental values for AC.{x\n", ch);
	return TRUE;
}

/*
 * Return values:
 *	0 - pObj was changed successfully
 *	1 - pObj was not changed
 *	2 - pObj was not changed, do not show obj values
 */
int set_obj_values(BUFFER *output, OBJ_INDEX_DATA *pObj,
		   const char *argument, int value_num)
{
	OBJ_INDEX_DATA *key = NULL;

	switch (pObj->item_type) {
		int	val;

	default:
		return 1;
		     
	case ITEM_LIGHT:
		switch (value_num) {
		default:
			return 1;
		case ITEM_LIGHT_DURATION:
			buf_add(output, "HOURS OF LIGHT SET.\n\n");
			pObj->value[ITEM_LIGHT_DURATION] = atoi(argument);
			break;
		}
		break;

	case ITEM_WAND:
	case ITEM_STAFF:
		switch (value_num) {
		default:
			return 1;
		case ITEM_WAND_LEVEL:
			buf_add(output, "SPELL LEVEL SET.\n\n");
			pObj->value[ITEM_WAND_LEVEL] = atoi(argument);
			break;
		case ITEM_WAND_CHARGES_TOTAL:
			buf_add(output, "TOTAL NUMBER OF CHARGES SET.\n\n");
			pObj->value[ITEM_WAND_CHARGES_TOTAL] = atoi(argument);
			break;
		case ITEM_WAND_CHARGES_REMAINING:
			buf_add(output, "CURRENT NUMBER OF CHARGES SET.\n\n");
			pObj->value[ITEM_WAND_CHARGES_REMAINING] = atoi(argument);
			break;
		case ITEM_WAND_SPELL:
			if (!str_cmp(argument, "?")
			||  (val = sn_lookup(argument)) < 0) {
				show_spells(output, -1);
				return 2;
			}
			buf_add(output, "SPELL TYPE SET.\n");
			pObj->value[ITEM_WAND_SPELL] = val;
			break;
		}
		break;

	case ITEM_SCROLL:
	case ITEM_POTION:
	case ITEM_PILL:
		switch (value_num) {
		case ITEM_POTION_LEVEL:
			buf_add(output, "SPELL LEVEL SET.\n\n");
			pObj->value[ITEM_POTION_LEVEL] = atoi(argument);
			break;
		case ITEM_POTION_SPELL1:
		case ITEM_POTION_SPELL2:
		case ITEM_POTION_SPELL3:
		case ITEM_POTION_SPELL4:
			if (!str_cmp(argument, "?")
			||  (val = sn_lookup(argument)) < 0) {
				show_spells(output, -1);
				return 2;
			}
			buf_printf(output, "SPELL TYPE %d SET.\n\n", value_num);
			pObj->value[value_num] = val;
			break;
 		}
		break;

/* ARMOR for ROM: */

	case ITEM_ARMOR:
		switch (value_num) {
		default:
			return 1;
		case ITEM_ARMOR_AC_PIERCE:
			buf_add(output, "AC PIERCE SET.\n\n");
			pObj->value[ITEM_ARMOR_AC_PIERCE] = atoi(argument);
			break;
		case ITEM_ARMOR_AC_BASH:
			buf_add(output, "AC BASH SET.\n\n");
			pObj->value[ITEM_ARMOR_AC_BASH] = atoi(argument);
			break;
		case ITEM_ARMOR_AC_SLASH:
			buf_add(output, "AC SLASH SET.\n\n");
			pObj->value[ITEM_ARMOR_AC_SLASH] = atoi(argument);
			break;
		case ITEM_ARMOR_AC_EXOTIC:
			buf_add(output, "AC EXOTIC SET.\n\n");
			pObj->value[ITEM_ARMOR_AC_EXOTIC] = atoi(argument);
			break;

		case ITEM_ARMOR_TYPE:
			if (!str_cmp(argument, "?")
			||  (val = flag_value(armor_types, argument)) < 0) {
				show_flags_buf(output, armor_types);
				return 2;
			}
			buf_add(output, "ARMOR TYPE SET.\n\n");
			pObj->value[ITEM_ARMOR_TYPE] = val;
			break;

		case ITEM_ARMOR_FLAGS:
			if (!str_cmp(argument, "?")
			||  (val = flag_value(armor_flags, argument)) < 0) {
				show_flags_buf(output, armor_flags);
				return 2;
			}
			buf_add(output, "ARMOR FLAG SET.\n\n");
			TOGGLE_BIT(pObj->value[ITEM_ARMOR_FLAGS], val);
			break;
		}
		break;

/* WEAPONS changed in ROM */

	case ITEM_WEAPON:
		switch (value_num) {
		default:
			return 1;
		case ITEM_WEAPON_TYPE:
			if (!str_cmp(argument, "?")
			||  (val = flag_value(weapon_class, argument)) < 0) {
				show_flags_buf(output, weapon_class);
				return 2;
			}
			buf_add(output, "WEAPON TYPE SET.\n\n");
			pObj->value[ITEM_WEAPON_TYPE] = val;
			break;
		case ITEM_WEAPON_DICE_NUM:
			buf_add(output, "NUMBER OF DICE SET.\n\n");
			pObj->value[ITEM_WEAPON_DICE_NUM] = atoi(argument);
			break;
		case ITEM_WEAPON_DICE_SIZE:
			buf_add(output, "TYPE OF DICE SET.\n\n");
			pObj->value[ITEM_WEAPON_DICE_SIZE] = atoi(argument);
			break;
		case ITEM_WEAPON_ATTACK_TYPE:
			if (!str_cmp(argument, "?")
			||  (val = attack_lookup(argument)) < 0) {
				show_attack_types(output);
				return 2;
			}
			buf_add(output, "WEAPON TYPE SET.\n\n");
			pObj->value[ITEM_WEAPON_ATTACK_TYPE] = val;
			break;
		case ITEM_WEAPON_FLAGS:
			if (!str_cmp(argument, "?")
			||  (val = flag_value(weapon_type2, argument)) < 0) {
				show_flags_buf(output, weapon_type2);
				return 2;
			}
			buf_add(output, "SPECIAL WEAPON TYPE TOGGLED.\n\n");
			TOGGLE_BIT(pObj->value[ITEM_WEAPON_FLAGS], val);
			break;
		}
		break;

	case ITEM_PORTAL:
		switch (value_num) {
		default:
			return 1;
		case ITEM_PORTAL_CHARGES:
			buf_add(output, "CHARGES SET.\n\n");
				     pObj->value[ITEM_PORTAL_CHARGES] = atoi(argument);
			break;
		case ITEM_PORTAL_EXIT_FLAGS:
			if (!str_cmp(argument, "?")
			||  (val = flag_value(exit_flags, argument)) < 0) {
				show_flags_buf(output, exit_flags);
				return 2;
			}
			buf_add(output, "EXIT FLAGS TOGGLED.\n\n");
			TOGGLE_BIT(pObj->value[ITEM_PORTAL_EXIT_FLAGS], val);
			break;
		case ITEM_PORTAL_FLAGS:
			if (!str_cmp(argument, "?")
			||  (val = flag_value(portal_flags, argument)) < 0) {
				show_flags_buf(output, portal_flags);
				return 2;
			}
			buf_add(output, "PORTAL FLAGS TOGGLED.\n\n");
			TOGGLE_BIT(pObj->value[ITEM_PORTAL_FLAGS], val);
			break;
		case ITEM_PORTAL_DEST:
			buf_add(output, "EXIT VNUM SET.\n\n");
			pObj->value[ITEM_PORTAL_DEST] = atoi(argument);
			break;
		case ITEM_PORTAL_KEY:
			if ((key = get_obj_index(atoi(argument))) == NULL) {
				buf_add(output, "ObjEd: key vnum doesn't exist.\n");
				return 2;
			}
			pObj->value[ITEM_PORTAL_KEY] = key->vnum;
			buf_printf(output, "PORTAL KEY SET [%d] %s",
				key->vnum,
				OBJ_SHORTNAME(key));
			break;
		}
		break;

	case ITEM_PARCHMENT:
		switch (value_num) {
		default:
			return 1;
		case ITEM_PARCHMENT_FLAGS:
			if(!str_cmp(argument, "?") 
			|| (val = flag_value(write_flags, argument)) < 0) {
				show_flags_buf(output, write_flags);
				return 1;
			}
			buf_add(output, "WRITE FLAG TOGGLED.\n\n");
			TOGGLE_BIT(pObj->value[ITEM_PARCHMENT_FLAGS], val);
				break;
		}
		break;

	case ITEM_FURNITURE:
		switch (value_num) {
		default:
			return 1;
		case ITEM_FURNITURE_QUANTITY:
			buf_add(output, "NUMBER OF PEOPLE SET.\n\n");
			pObj->value[ITEM_FURNITURE_QUANTITY] = atoi(argument);
			break;
		case ITEM_FURNITURE_WEIGHT:
			buf_add(output, "MAX WEIGHT SET.\n\n");
			pObj->value[ITEM_FURNITURE_WEIGHT] = atoi(argument);
			break;
		case ITEM_FURNITURE_FLAGS:
			if (!str_cmp(argument, "?")
			||  (val = flag_value(furniture_flags, argument)) < 0) {
				show_flags_buf(output, furniture_flags);
				return 2;
			}
		        buf_add(output, "FURNITURE FLAGS TOGGLED.\n\n");
			TOGGLE_BIT(pObj->value[ITEM_FURNITURE_FLAGS], val);
			break;
		case ITEM_FURNITURE_HEAL_RATE:
			buf_add(output, "HEAL RATE SET (100 is normal).\n\n");
			pObj->value[ITEM_FURNITURE_HEAL_RATE] = atoi(argument);
			break;
		case ITEM_FURNITURE_MANA_RATE:
			buf_add(output, "MANA RATE SET (100 is normal).\n\n");
			pObj->value[ITEM_FURNITURE_MANA_RATE] = atoi(argument);
			break;
		}
		break;
		   
	case ITEM_CONTAINER:
		switch (value_num) {
		default:
			return 1;
		case ITEM_CONTAINER_WEIGHT:
			buf_add(output, "TOTAL MAX WEIGHT CAPACITY SET.\n\n");
			pObj->value[ITEM_CONTAINER_WEIGHT] = atoi(argument);
			break;
		case ITEM_CONTAINER_FLAGS:
			if (!str_cmp(argument, "?")
			||  (val = flag_value(cont_flags, argument)) < 0) {
				show_flags_buf(output, cont_flags);
				return 2;
			}
			buf_add(output, "CONTAINER FLAGS SET.\n\n");
			TOGGLE_BIT(pObj->value[ITEM_CONTAINER_FLAGS], val);
			break;
		case ITEM_CONTAINER_KEY:
			if (atoi(argument) != 0) {
				if ((key = get_obj_index(atoi(argument))) == NULL) {
					buf_add(output, "THERE IS NO SUCH ITEM.\n\n");
					return 1;
				}

				if (key->item_type != ITEM_KEY) {
					buf_add(output, "THAT ITEM IS NOT A KEY.\n\n");
					return 1;
				}
			}
			buf_add(output, "CONTAINER KEY SET.\n\n");
			pObj->value[ITEM_CONTAINER_KEY] = key->vnum;
			buf_printf(output, "CONTAINER KEY SET [%d] %s",
				key->vnum,
				OBJ_SHORTNAME(key));
			break;
		case ITEM_CONTAINER_PER_ITEM_WEIGHT:
			buf_add(output, "CONTAINER MAX INDIVIDUAL ITEM WEIGHT SET.\n");
			pObj->value[ITEM_CONTAINER_PER_ITEM_WEIGHT] = atoi(argument);
			break;
		case ITEM_CONTAINER_WEIGHT_MULTI:
			buf_add(output, "WEIGHT MULTIPLIER SET.\n\n");
			pObj->value[ITEM_CONTAINER_WEIGHT_MULTI] = atoi(argument);
			break;
		case ITEM_CONTAINER_QUANTITY:
			buf_add(output, "CONTAINER MAX QUANTITY SET.\n\n");
			pObj->value[ITEM_CONTAINER_QUANTITY] = atoi(argument);
			break;
		}
		break;

	case ITEM_DRINK_CON:
	case ITEM_FOUNTAIN:
		switch (value_num) {
		default:
			return 1;
		case ITEM_DRINK_TOTAL:
			buf_add(output, "MAXIMUM AMOUT OF LIQUID SET.\n\n");
			pObj->value[ITEM_DRINK_TOTAL] = atoi(argument);
			break;
		case ITEM_DRINK_REMAINING:
			buf_add(output, "CURRENT AMOUNT OF LIQUID SET.\n\n");
			pObj->value[ITEM_DRINK_REMAINING] = atoi(argument);
			break;
		case ITEM_DRINK_TYPE:
			if (!str_cmp(argument, "?")
			||  (val = liq_lookup(argument)) < 0) {
				show_liq_types(output);
				return 2;
			}
			buf_add(output, "LIQUID TYPE SET.\n\n");
			pObj->value[ITEM_DRINK_TYPE] = val;
			break;
		case ITEM_DRINK_POISON:
			buf_add(output, "POISON VALUE TOGGLED.\n\n");
			pObj->value[ITEM_DRINK_POISON] = !pObj->value[ITEM_DRINK_POISON];
			break;
		}
		break;

	case ITEM_FOOD:
		switch (value_num) {
		default:
			return 1;
		case ITEM_FOOD_HOURS:
			buf_add(output, "HOURS OF FOOD SET.\n\n");
			pObj->value[ITEM_FOOD_HOURS] = atoi(argument);
			break;
		case ITEM_FOOD_FULL:
			buf_add(output, "HOURS OF FULL SET.\n\n");
			pObj->value[ITEM_FOOD_FULL] = atoi(argument);
			break;
		case ITEM_FOOD_POISON:
			buf_add(output, "POISON VALUE TOGGLED.\n\n");
			pObj->value[ITEM_FOOD_POISON] = !pObj->value[ITEM_FOOD_POISON];
			break;
		}
		break;

	case ITEM_MONEY:
		switch (value_num) {
		default:
			return 1;
		case ITEM_MONEY_SILVER:
			buf_add(output, "SILVER AMOUNT SET.\n\n");
			pObj->value[ITEM_MONEY_SILVER] = atoi(argument);
			break;
		case ITEM_MONEY_GOLD:
			buf_add(output, "GOLD AMOUNT SET.\n\n");
			pObj->value[ITEM_MONEY_GOLD] = atoi(argument);
			break;
		}
		break;

	case ITEM_AWARD:
		switch (value_num) {
		default:
			return 1;
		case ITEM_AWARD_TYPE:
			if (!str_cmp(argument, "?")
			||  (val = flag_value(award_types, argument)) < 0) {
				show_flags_buf(output, award_types);
				return 2;
			}
			buf_add(output, "AWARD TYPE SET.\n\n");
			pObj->value[ITEM_AWARD_TYPE] = val;
			break;
		case ITEM_AWARD_VALUE:
			buf_add(output, "AWARD AMOUNT SET.\n\n");
			pObj->value[ITEM_AWARD_VALUE] = atoi(argument);
			break;
		}
		break;
	}
	return 0;
}

/*****************************************************************************
 Name:		show_spells
 Purpose:	Displays all spells.
 ***************************************************************************/
static void show_spells(BUFFER *output, int tar)
{
	int  sn;
	int  col;
 
	col = 0;
	for (sn = 0; sn < skills.nused; sn++) {
		skill_t *sk = SKILL(sn);

		if (!str_cmp(sk->name, "reserved") || sk->spell_fun == NULL)
			continue;

		if (tar == -1 || sk->target == tar) {
			buf_printf(output, "%-19.18s", sk->name);
			if (++col % 4 == 0)
				buf_add(output, "\n");
		}
	}
 
	if (col % 4 != 0)
		buf_add(output, "\n");
}

void show_liqlist(CHAR_DATA *ch)
{
	int liq;
	BUFFER *buffer;
	
	buffer = buf_new(-1);
	
	for (liq = 0; liq_table[liq].liq_name != NULL; liq++) {
		if ((liq % 21) == 0)
			buf_add(buffer,"Name                 Color          Proof Full Thirst Food Ssize\n");

		buf_printf(buffer, "%-20s %-14s %5d %4d %6d %4d %5d\n",
			liq_table[liq].liq_name,liq_table[liq].liq_color,
			liq_table[liq].liq_affect[0],liq_table[liq].liq_affect[1],
			liq_table[liq].liq_affect[2],liq_table[liq].liq_affect[3],
			liq_table[liq].liq_affect[4]);
	}

	page_to_char(buf_string(buffer), ch);
	buf_free(buffer);
}

#if 0
	{ "type",	item_types,	"Types of objects."		},
	{ "extra",	extra_flags,	"Object attributes."		},
	{ "wear",	wear_flags,	"Where to wear object."		},
	{ "wear-loc",	wear_loc_flags,	"Where mobile wears object."	},
	{ "container",	cont_flags,"Container status."		},

/* ROM specific bits: */

	{ "armor",	ac_type,	"Ac for different attacks."	},
	{ "apply",	apply_flags,	"Apply flags"			},
	{ "wclass",     weapon_class,   "Weapon class."                }, 
	{ "wtype",      weapon_type2,   "Special weapon type."         },
	{ "portal",	portal_flags,	"Portal types."		},
	{ "furniture",	furniture_flags,"Furniture types."		},
	{ "liquid",	liq_table,	"Liquid types."		},
	{ "apptype",	apply_types,	"Apply types."			},
	{ "weapon",	attack_table,	"Weapon types."		},
	{ NULL,		NULL,		 NULL				}
};
#endif

VALIDATE_FUN(validate_condition)
{
	int val = *(int*) arg;

	if (val < 0 || val > 100) {
		char_puts("ObjEd: condition can range from 0 (ruined) "
			  "to 100 (perfect).\n", ch);
		return FALSE;
	}
	return TRUE;
}

/* get_autoweapon - calculates the appropriate damage values
 * 	for different types of weapons.
 * 	The Object is not modified, only the r_dice and r_size values.
 *
 * 	percent_damage - applied against normal damage for the item
 *
 * by Zsuzsu
 */
int get_autoweapon(OBJ_INDEX_DATA* pObj, int *r_dice, int *r_size, int percent_damage) {
	int dice, size; 
	double avg;
        bool twoh = IS_WEAPON_STAT(pObj, WEAPON_TWO_HANDS);

	/*two-handed, bigger dice size for greater variation*/
	if (twoh) 
		dice = (pObj->level/30 + 1);
	else 
		dice = (pObj->level/20 + 1);

	avg = pObj->level;

	avg = avg * percent_damage / 100;

	switch (pObj->value[0])
	{
		case WEAPON_POLEARM:       avg *= (twoh) ? 0.80 : 0.00; dice-=2; break;
		case WEAPON_LANCE:         avg *= (twoh) ? 0.00 : 0.70; break;
		case WEAPON_BASTARDSWORD:  avg *= (twoh) ? 0.75 : 0.00; break;
		case WEAPON_KATANA:	   avg *= (twoh) ? 0.00 : 0.75; break;
		case WEAPON_LONGSWORD:     avg *= (twoh) ? 0.00 : 0.65; break;
		case WEAPON_AXE:	   avg *= (twoh) ? 0.75 : 0.65; break;
		case WEAPON_HAMMER:	   avg *= (twoh) ? 0.70 : 0.60; break;
		case WEAPON_SPEAR:	   avg *= (twoh) ? 0.70 : 0.55; break;
		case WEAPON_MACE:	   avg *= (twoh) ? 0.67 : 0.55; break;
		case WEAPON_FLAIL:	   avg *= (twoh) ? 0.67 : 0.55; break;
		case WEAPON_STAFF:         avg *= (twoh) ? 0.67 : 0.55; break;
		case WEAPON_BOW:           avg *= (twoh) ? 0.65 : 0.00; break;
		case WEAPON_SHORTSWORD:	   avg *= (twoh) ? 0.00 : 0.50; dice++; break;
		case WEAPON_DAGGER:	   avg *= (twoh) ? 0.00 : 0.45; dice++; break;
		case WEAPON_WHIP: 	   avg *= (twoh) ? 0.00 : 0.40; break;
		case WEAPON_ARROW:	   avg *= (twoh) ? 0.00 : 0.60; dice+=2;break;
		case WEAPON_SHURIKEN:	   avg *= (twoh) ? 0.00 : 0.10; break;
		case WEAPON_EXOTIC:
		default:
					   avg *= (twoh) ? 0.70 : 0.55; dice--; break;
	}

	/* if avg is 0 then there must be something wrong */
	if (avg <= 0.0) {
		DEBUG(DEBUG_BUILD_AUTO,
			"Autoweapon: obj[%d] has 0 avg damage",
			pObj->vnum, avg);
		*r_dice = 1;
		*r_size = 1;
		return 1;
	}

	if (pObj->value[0] != WEAPON_KATANA) {
		dice = UMAX(1, dice);

		size = 2 * avg / dice -1;

		size = UMAX(1, size);
	}
	/* katana - needs to be closer to average because of 'master sword'*/
	else {
		size = UMAX(1, dice);
		dice = 2 * avg / (size +1);
		dice = UMAX(1, dice);
	}

	*r_dice = dice;
	*r_size = size;

	return (1 + *r_size) * *r_dice / 2;
}

int get_autocost (OBJ_INDEX_DATA *pObjIndex)
{
	int level = pObjIndex->level;
	int cost = 1;
	int bp = 0, bpmax = 0;
	int liquid = 0;

	build_points(pObjIndex, &bp, &bpmax);

	cost = bp * 100;

	if (IS_OBJ_LIMITED(pObjIndex))
		cost = cost * 2;
	else if (IS_OBJ_RARE(pObjIndex))
		cost = cost * 3/2;
		
	switch (pObjIndex->item_type) {
	case ITEM_LIGHT:
		cost = cost * 1/2; break;
		if (!cost)
			cost = 20 * level;
		break;

	case ITEM_FOOD:
		cost = cost * 10/100;
		if (!cost) {
			cost = (pObjIndex->value[ITEM_FOOD_HOURS] * 12)
				- (pObjIndex->value[ITEM_FOOD_FULL] * 2);
		}
		
		break;

	case ITEM_DRINK_CON:
		cost = cost * 20/100;
		liquid = pObjIndex->value[ITEM_DRINK_TYPE];
		if (!cost) {
			cost = (pObjIndex->value[ITEM_DRINK_TOTAL] * 60)
				+ (pObjIndex->value[ITEM_DRINK_REMAINING] * 2);
			cost = cost * (100 
				+ liq_table[liquid].liq_affect[LIQ_AFFECT_PROOF]) / 100;
		}
		break;

	case ITEM_BOAT:
		if (!cost) cost = 2000;
		cost = cost * UMAX(10, 1000 - pObjIndex->weight) / 10;
		break;

	case ITEM_WEAPON:
		cost = cost * UMAX(1, (pObjIndex->level / 10));

		switch (pObjIndex->value[ITEM_WEAPON_TYPE]) {
		case WEAPON_KATANA:       cost = cost * 500 / 100; break;
		case WEAPON_BASTARDSWORD: cost = cost * 180 / 100; break;
		case WEAPON_LONGSWORD:    cost = cost * 150 / 100; break;
		case WEAPON_SHORTSWORD:   cost = cost *  70 / 100; break;
		case WEAPON_DAGGER:	  cost = cost *  50 / 100; break;
		case WEAPON_AXE:          cost = cost * 140 / 100; break;
		case WEAPON_MACE:	  cost = cost *  80 / 100; break;
		case WEAPON_HAMMER:       cost = cost * 100 / 100; break;
		case WEAPON_FLAIL:	  cost = cost * 110 / 100; break;
		case WEAPON_WHIP:	  cost = cost *  30 / 100; break;
		case WEAPON_POLEARM:	  cost = cost * 200 / 100; break;
		case WEAPON_LANCE:	  cost = cost * 200 / 100; break;
		case WEAPON_BOW:	  cost = cost * 120 / 100; break;
		case WEAPON_ARROW:	  cost = cost *  20 / 100; break;
		case WEAPON_STAFF:	  cost = cost *  60 / 100; break;
		case WEAPON_SHURIKEN:	  cost = cost *  20 / 100; break;
		case WEAPON_EXOTIC:       cost = cost * 120 / 100; break;
		}

		if (IS_SET(pObjIndex->value[ITEM_WEAPON_FLAGS], WEAPON_TWO_HANDS)) {
			switch (pObjIndex->value[ITEM_WEAPON_TYPE]) {
			case WEAPON_POLEARM:
			case WEAPON_BOW:
			case WEAPON_LANCE:
			case WEAPON_BASTARDSWORD:
				break;
			default:
				cost = cost * 170/100;
			}
		}

		break;

	case ITEM_ARMOR:
		cost = cost * 150/100;
		/* fall through */
	case ITEM_CLOTHING:
		cost = cost * UMAX(1, (pObjIndex->level / 10));

		if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_FINGER))
			cost = cost *  70/100;
		else if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_NECK))
			cost = cost *  80/100;
		else if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_LEGS))
			cost = cost * 130/100;
		else if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_FEET))
			cost = cost * 100/100;
		else if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_HANDS))
			cost = cost * 110/100;
		else if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_ARMS))
			cost = cost * 120/100;
		else if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_BODY))
			cost = cost * 200/100;
		else if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_SHIELD))
			cost = cost * 130/100;
		else if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_ABOUT))
			cost = cost * 110/100;
		else if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_WAIST))
			cost = cost *  90/100;
		else if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_WRIST))
			cost = cost *  80/100;
		else if (IS_SET(pObjIndex->wear_flags, ITEM_WEAR_FLOAT))
			cost = cost * 180/100;
		
		break;

	case ITEM_CONTAINER:
		cost += pObjIndex->value[ITEM_CONTAINER_WEIGHT] * 10;
		if (IS_SET(pObjIndex->value[ITEM_CONTAINER_FLAGS], CONT_QUIVER))
			cost = cost * 200/100;
		if (IS_SET(pObjIndex->value[ITEM_CONTAINER_FLAGS], CONT_SCROLL))
			cost = cost * 120/100;
		if (IS_SET(pObjIndex->value[ITEM_CONTAINER_FLAGS], CONT_POTION))
			cost = cost * 120/100;
		if (IS_SET(pObjIndex->value[ITEM_CONTAINER_FLAGS], CONT_PILL))
			cost = cost * 130/100;
		if (IS_SET(pObjIndex->value[ITEM_CONTAINER_FLAGS], CONT_WAND))
			cost = cost * 130/100;
		if (IS_SET(pObjIndex->value[ITEM_CONTAINER_FLAGS], CONT_STAFF))
			cost = cost * 140/100;
		if (IS_SET(pObjIndex->value[ITEM_CONTAINER_FLAGS], CONT_PICKPROOF))
			cost = cost * 300/100;

		if (IS_SET(pObjIndex->extra_flags, ITEM_BURN_PROOF))
			cost = cost * 500/100;

		break;

	case ITEM_WARP_STONE:
	case ITEM_SCROLL:
	case ITEM_WAND:
	case ITEM_STAFF:
	case ITEM_POTION:
	case ITEM_TRASH:
	default:
		cost = 0; break;


	}

	if (!IS_SET(pObjIndex->wear_flags, ITEM_TAKE))
		cost = 0;

	return cost;
}

/*
 * Calculate Build Points for the item
 * by Zsuzsu
 * XXX - one problem with this is if multiple
 * 	affects are added for the same location.
 * 	This will throw off the total.
 */
void build_points (OBJ_INDEX_DATA *pObj, int *bp, int *bpmax) {

	AFFECT_DATA *paf;
	int i;
	int elem_count = 0,
	    misc_count = 0,
	    level      = 0,
	    base_max   = 0,
	    stat_pos   = 0,
	    stat_neg   = 0;
	int count = 0;
	int item_max = 0;
	int autow_avg  = 0,
	    avg_dnum   = 0,
	    avg_dsize  = 0,
	    w_avg      = 0;


	*bp = 0;
	*bpmax = 0;

	/* don't count items you can't take */
	if (!IS_SET(pObj->wear_flags, ITEM_TAKE))
		return;

	level = (pObj->level > 10) ? pObj->level : 10;

	base_max = level * 3;

	*bpmax = level * (IS_OBJ_LIMITED(pObj) ? 5 :
		    IS_OBJ_RARE(pObj)          ? 4 : 
			    		         3);

	for (paf = pObj->affected; paf; paf = paf->next) {
		switch (paf->location) {
			case APPLY_STR:
			case APPLY_DEX:
			case APPLY_INT:
			case APPLY_WIS:
			case APPLY_CON:
			case APPLY_CHA:
				if (paf->modifier > 0)
					stat_pos += paf->modifier;
				else
					stat_neg += ABS(paf->modifier);
				break;

			case APPLY_AC:
				if (paf->modifier < 0) {
					i = ABS(paf->modifier) - level / 5; 
					i = (i > 0) ? i : 0;

					*bp += (ABS(paf->modifier) - i) * 1 ;
					*bp += i * 2;
				}
				else
					*bpmax += paf->modifier;
				break;

			case APPLY_MOVE:
				if (paf->modifier > 0) {
					i = paf->modifier - (level/10)*20; 
					i = (i > 0) ? i : 0;

					*bp += (paf->modifier - i) *1 ;
					*bp += i * 2;
				}
				else
					*bpmax += ABS(paf->modifier) / 2;
				break;

			case APPLY_MANA:
				if (paf->modifier > 0) {
					i = paf->modifier - (level/10)*20; 
					i = (i > 0) ? i : 0;

					*bp += (paf->modifier - i) *1 ;
					*bp += i * 2;
				}
				else
					*bpmax += ABS(paf->modifier) / 2;
				break;

			case APPLY_HIT:
				if (paf->modifier > 0) {
					i = paf->modifier - (level/10)*10; 
					i = (i > 0) ? i : 0;

					*bp += (paf->modifier - i) * 2;
					*bp += i * 3;
				}
				else
					*bpmax += ABS(paf->modifier) * 1;
				break;

			case APPLY_HITROLL:
			case APPLY_DAMROLL:
				if (paf->modifier > 0) {
					i = paf->modifier - (level / 10); 
					i = (i > 0) ? i : 0;

					*bp += (paf->modifier - i) * 10;
					*bp += i * 15;
				}
				else
					*bpmax += ABS(paf->modifier) * 8;
				break;

			case APPLY_SAVES:
			/* case APPLY_SAVING_PARA: */
			case APPLY_SAVING_ROD:
			case APPLY_SAVING_PETRI:
			case APPLY_SAVING_BREATH:
			case APPLY_SAVING_SPELL:
				if (paf->modifier < 0) {
					i = ABS(paf->modifier) - (level / 10); 
					i = (i > 0) ? i : 0;

					*bp += (ABS(paf->modifier) - i) * 20;
					*bp += i * 30;
				}
				else
					*bpmax += paf->modifier * 20;
				break;

			case APPLY_SPELL_AFFECT:
				break;

			case APPLY_LEVEL:
			case APPLY_RACE:
			case APPLY_SIZE:
				*bp = 1000;
				break;

			case APPLY_SEX:
			case APPLY_AGE:
			case APPLY_CLASS:
			case APPLY_HEIGHT:
			case APPLY_WEIGHT:
			case APPLY_GOLD:
			case APPLY_EXP:
			default:
				break;
		}
	}

	/* calc stat modifier costs */
	for (i = 1; i <= stat_pos; i++) {
		*bp += i * 10 + 10;
	}

	for (i = 1; i <= stat_neg; i++) {
		*bpmax += i * 7 + 7;
	}

	/* calc weapon flag costs */
	switch (pObj->item_type) {
	case ITEM_WEAPON:
		autow_avg = get_autoweapon(pObj, &avg_dnum, &avg_dsize, 100);
		w_avg = (1 + pObj->value[2]) * pObj->value[1]/2;

		if (w_avg > (autow_avg * 150/100))
			*bp += 1000;
		if (w_avg > (autow_avg * 140/100))
			*bp += IS_OBJ_LIMITED(pObj) ? base_max*5/3 : 1000;
		else if (w_avg > autow_avg * 130/100)
			*bp += base_max;
		else if (w_avg > autow_avg * 120/100)
			*bp += base_max * 2/3;
		else if (w_avg > autow_avg * 110/100)
			*bp += base_max * 1/2;
		else if (w_avg > autow_avg * 105/100)
			*bp += base_max * 1/3;
		else if (w_avg > autow_avg)
			*bp += base_max * 1/4;

		if (IS_WEAPON_STAT(pObj,WEAPON_FLAMING)) {
			*bp += base_max * 0.30;
			elem_count++;
		}
		if (IS_WEAPON_STAT(pObj,WEAPON_SHOCKING)) {
			*bp += base_max * 0.20;
			elem_count++;
		}
		if (IS_WEAPON_STAT(pObj,WEAPON_FROST)) {
			*bp += base_max * 0.20;
			elem_count++;
		}
		if (elem_count > 1)
			*bp += 1000;

		if (IS_WEAPON_STAT(pObj,WEAPON_VAMPIRIC)) {
			*bp += base_max * 0.20;
			misc_count++;
		}
		if (IS_WEAPON_STAT(pObj,WEAPON_SHARP)) {
			*bp += base_max * 0.20;
			misc_count++;
		}
		if (IS_WEAPON_STAT(pObj,WEAPON_VORPAL)) {
			*bp += base_max * 0.20;
			misc_count++;
		}
		if (IS_WEAPON_STAT(pObj,WEAPON_POISON))	{
			*bp += base_max * 0.15;
			misc_count++;
		}
		if (IS_WEAPON_STAT(pObj,WEAPON_HOLY)) {
			*bp += base_max * 0.20;
			misc_count++;
		}
		if (IS_WEAPON_STAT(pObj,WEAPON_KATANA))	{
			*bp += base_max * 0.20;
			misc_count++;
		}
		if (IS_WEAPON_STAT(pObj,WEAPON_CRUSH)) {
			*bp += base_max * 0.20;
			misc_count++;
		}

		*bp += (misc_count > 2) 
			? (misc_count -2) * base_max * 0.30
			: 0;

		if (!IS_OBJ_LIMITED(pObj) 
		&& elem_count + misc_count > 2)
			*bp += 1000;

		break;

	case ITEM_CONTAINER:
		item_max = 100;
		/* establishes maximum capacity for each type*/
		if (IS_CONTAINER_TYPE(pObj, CONT_QUIVER)) {
			count++;
			item_max = UMIN(item_max, 50);
		}
		if (IS_CONTAINER_TYPE(pObj, CONT_SCROLL)) {
			count++;
			item_max = UMIN(item_max, 25);
		}
		if (IS_CONTAINER_TYPE(pObj, CONT_PILL)) {
			count++;
			item_max = UMIN(item_max, 25);
		}
		if (IS_CONTAINER_TYPE(pObj, CONT_POTION)) {
			count++;
			item_max = UMIN(item_max, 15);
		}
		if (IS_CONTAINER_TYPE(pObj, CONT_WAND)) {
			count++;
			item_max = UMIN(item_max, 10);
		}
		if (IS_CONTAINER_TYPE(pObj, CONT_STAFF)) {
			count++;
			item_max = UMIN(item_max, 5);
		}

#if 0
		/* XXX - this really doesn't mean number of items, 
		 * need to do this later */
		/* capacity */
		if (pObj->value[3] > item_max)
			*bp += base_max * (pObj->value[3] - item_max) * 0.10;
#endif

		if (count > 1)
			*bp += base_max * ((count -1) * 0.33);

		/* BPs for weight multiplier */
		if (pObj->value[4] < 20) {
			*bp += 1000;
		}

		else if (pObj->value[4] > 100) {
			*bp += ABS(100 - pObj->value[4]) * 2;
		}
		else 
			*bp += (100 - pObj->value[4]) * 2;
		break;

	case ITEM_FURNITURE:
		if (pObj->value[3] > 300 || pObj->value[4] > 300)
			*bp += 1000;
		break;
	}
}

/* returns a color code indicating the percentage of build points
 * used compared to build point max for the item
 */
char build_point_percent_color (int bp, int bpmax) {
	return (bp > bpmax)		? 'r' :
		(bp > bpmax * 9/10) 	? 'R' :
		(bp > bpmax * 3/4) 	? 'Y' :
		(bp > bpmax * 1/2)	? 'G' : 
		(bp > bpmax * 1/4)	? 'b' : 
		(bp > 0)		? 'y' :
		(bp == 0)		? 'x' :
					  'D';
}
