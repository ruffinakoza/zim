/*-
 * Copyright (c) 2003 joe <joe.lawrence@business-aid.net>
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
 * $Id: olc_class.c,v 1.00 2003/02/15 
 */

#include <stdio.h>
#include <stdlib.h>

#include "merc.h"
#include "olc.h"

#define EDIT_CLASS(ch, class)	(class = (class_t*) ch->desc->pEdit)

DECLARE_OLC_FUN(classed_create		);
DECLARE_OLC_FUN(classed_edit		);
DECLARE_OLC_FUN(classed_touch		);
DECLARE_OLC_FUN(classed_show		);
DECLARE_OLC_FUN(classed_list		);

DECLARE_OLC_FUN(classed_filename	);
DECLARE_OLC_FUN(classed_name		);
DECLARE_OLC_FUN(classed_ShortName	);
DECLARE_OLC_FUN(classed_PrimStat	);
DECLARE_OLC_FUN(classed_SchoolWeapon	);
DECLARE_OLC_FUN(classed_GuildRoom	);
DECLARE_OLC_FUN(classed_SkillAdept	);
DECLARE_OLC_FUN(classed_Thac0_00	);
DECLARE_OLC_FUN(classed_Thac0_32	);
DECLARE_OLC_FUN(classed_HPRate	);
DECLARE_OLC_FUN(classed_ManaRate	);
DECLARE_OLC_FUN(classed_DamRate	);
DECLARE_OLC_FUN(classed_NimRate	);
DECLARE_OLC_FUN(classed_AcRate	);
DECLARE_OLC_FUN(classed_AddExp	);
DECLARE_OLC_FUN(classed_StatMod	);
DECLARE_OLC_FUN(classed_skill		);
DECLARE_OLC_FUN(classed_Title	);
DECLARE_OLC_FUN(classed_skill_add	);
DECLARE_OLC_FUN(classed_skill_del	);

static DECLARE_VALIDATE_FUN(validate_name);

olc_cmd_t olc_cmds_class[] =
{
	{ 9, "create",		classed_create					},
	{ 5, "edit",		classed_edit					},
	{ 5, "touch",		classed_touch					},
	{ 0, "show",		classed_show					},
	{ 0, "list",		classed_list					},
	
	{ 5, "filename",	classed_filename,	validate_filename	},
	{ 5, "name",		classed_name,		validate_name	 	},
	{ 5, "ShortName",	classed_ShortName				},
	{ 5, "PrimStat",	classed_PrimStat				},
	{ 5, "SchoolWeapon",	classed_SchoolWeapon				},
	{ 5, "GuildRoom",	classed_GuildRoom				},
	{ 5, "SkillAdept",	classed_SkillAdept				},
	{ 5, "Thac0_00",	classed_Thac0_00				},
	{ 5, "Thac0_32",	classed_Thac0_32				},
	{ 5, "HPRate",		classed_HPRate					},
	{ 5, "ManaRate",	classed_ManaRate				},
	{ 5, "DamRate",		classed_DamRate
},
	{ 5, "NimRate",		classed_NimRate
},
	{ 5, "AcRate",		classed_AcRate
},
	{ 5, "AddExp",		classed_AddExp					},
	{ 5, "AddExp",		classed_AddExp					},
	{ 5, "StatMod",		classed_StatMod					},
	{ 5, "skill",		classed_skill					},
	{ 5, "Title",		classed_Title					},
	{ 5, "skill_add",	classed_skill_add				},
	{ 5, "skill_del",	classed_skill_del				},

	{ 0, "commands",	show_commands					},
	{ 0, NULL }
};

OLC_FUN(classed_create)
{
	int cls;
	class_t *class;
	char arg[MAX_STRING_LENGTH];

	if (ch->pcdata->security < SECURITY_CLASS) {
		char_puts("ClassEd: Insufficient security for creating classes\n",
			  ch);
		return FALSE;
	}

	first_arg(argument, arg, sizeof(arg), FALSE);
	if (arg[0] == '\0') {
		do_help(ch, "'OLC CREATE'");
		return FALSE;
	}

	if ((cls = cls_lookup(arg)) >= 0) {
		char_printf(ch, "classEd: %s: already exists.\n",
			    class(cls)->name);
		return FALSE;
	}

	class		= class_new();
	class->name	= str_dup(arg);
	class->file_name	= str_printf("class%02d.class", classs.nused-1);

	ch->desc->pEdit	= (void *)class;
	OLCED(ch)	= olced_lookup(ED_class);
	touch_class(class);
	char_puts("class created.\n",ch);
	return FALSE;
}

OLC_FUN(classed_edit)
{
	int cls;
	char arg[MAX_STRING_LENGTH];

	if (ch->pcdata->security < SECURITY_class) {
		char_puts("classEd: Insufficient security.\n", ch);
		return FALSE;
	}

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		do_help(ch, "'OLC EDIT'");
		return FALSE;
	}

	if ((cls = cls_lookup(arg)) < 0) {
		char_printf(ch, "classEd: %s: No such class.\n", arg);
		return FALSE;
	}

	ch->desc->pEdit	= class(cls);
	OLCED(ch)	= olced_lookup(ED_class);
	return FALSE;
}

OLC_FUN(classed_touch)
{
	class_t *class;
	EDIT_class(ch, class);
	return touch_class(class);
}

OLC_FUN(classed_show)
{
	char arg[MAX_STRING_LENGTH];
	int i;
	BUFFER *output;
	class_t *class;

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		if (IS_EDIT(ch, ED_class))
			EDIT_class(ch, class);
		else {
			do_help(ch, "'OLC ASHOW'");
			return FALSE;
		}
	}
	else {
		if ((i = cls_lookup(arg)) < 0) {
			char_printf(ch, "classEd: %s: No such class.\n", arg);
			return FALSE;
		}
		class = class(i);
	}

	output = buf_new(-1);
	buf_printf(output,
		   "Name:        [%s]\n"
		   "Filename:    [%s]\n",
		   class->name,
		   class->file_name);
	if (class->flags)
		buf_printf(output, "Flags:       [%s]\n",
			   flag_string(class_type_flags, class->flags));
	if (class->recall_vnum)
		buf_printf(output, "Recall:      [%d]\n",
			   class->recall_vnum);
	if (class->obj_vnum)
		buf_printf(output, "Item:        [%d]\n",
			   class->obj_vnum);
	if (class->mark_vnum) 
		buf_printf(output, "Mark:	 [%d]\n",
			   class->mark_vnum);
	if (class->altar_vnum)
		buf_printf(output, "Altar:       [%d]\n",
			   class->altar_vnum);

	for (i = 0; i < class->skills.nused; i++) {
		clskill_t *cs = VARR_GET(&class->skills, i);
		skill_t *sk;

		if (cs->sn <= 0
		||  (sk = skill_lookup(cs->sn)) == NULL)
			continue;
		buf_printf(output, "Skill:       '%s' (level %d, %d%%)\n",
			   sk->name, cs->level, cs->percent);
	}

	page_to_char(buf_string(output), ch);
	buf_free(output);

	return FALSE;
}
/* i added stuff here--joe*/
OLC_FUN(classed_ShortName)
{
	class_t *class;
	EDIT_class(ch, class);
	return olced_str(ch, argument, cmd, &class->ShortName);
	else {
		do_help(ch, "'OLC CREATE'");
		return FALSE;
	}
}

OLC_FUN(classed_list)
{
	int i;

	for (i = 0; i < classs.nused; i++)
		char_printf(ch, "[%d] %s\n", i, class(i)->name);
	return FALSE;
}

OLC_FUN(classed_name)
{
	class_t *class;
	EDIT_class(ch, class);
	return olced_str(ch, argument, cmd, &class->name);
}

OLC_FUN(classed_filename)
{
	class_t *class;
	EDIT_class(ch, class);
	return olced_str(ch, argument, cmd, &class->file_name);
}

OLC_FUN(classed_recall)
{
	class_t *class;
	EDIT_class(ch, class);
	return olced_number(ch, argument, cmd, &class->recall_vnum);
}

OLC_FUN(classed_item)
{
	class_t *class;
	EDIT_class(ch, class);
	return olced_number(ch, argument, cmd, &class->obj_vnum);
}

OLC_FUN(classed_mark)
{
	class_t *class;
	EDIT_class(ch, class);
	return olced_number(ch, argument, cmd, &class->mark_vnum);
}

OLC_FUN(classed_altar)
{
	class_t *class;
	EDIT_class(ch, class);
	return olced_number(ch, argument, cmd, &class->altar_vnum);
}

OLC_FUN(classed_flags)
{
	class_t *class;
	EDIT_class(ch, class);
	return olced_flag32(ch, argument, cmd, &class->flags);
}

OLC_FUN(classed_skill)
{
	char arg[MAX_STRING_LENGTH];

	argument = one_argument(argument, arg, sizeof(arg));
	if (!str_prefix(arg, "add")) 
		return classed_skill_add(ch, argument, cmd);
	else if (!str_prefix(arg, "delete"))
		return classed_skill_del(ch, argument, cmd);

	do_help(ch, "'OLC class SKILL'");
	return FALSE;
}

OLC_FUN(classed_plist)
{
	const char **nl;
	const char *name;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	class_t *class;
	EDIT_class(ch, class);
	
	if (ch->pcdata->security < SECURITY_class_PLIST) {
		char_puts("classEd: Insufficient security.\n", ch);
		return FALSE;
	}

	argument = one_argument(argument, arg1, sizeof(arg1));
		   one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0') {
		do_help(ch, "'OLC class PLIST'");
		return FALSE;
	}

	if (!str_prefix(arg1, "member")) {
		nl = &class->member_list;
		name = "members";
	}
	else if (!str_prefix(arg1, "leader")) {
		nl = &class->leader_list;
		name = "leaders";
	}
	else if (!str_prefix(arg1, "second")) {
		nl = &class->second_list;
		name = "secondaries";
	}
	else
		return classed_plist(ch, str_empty, cmd);

	if (arg2[0] == '\0') {
		char_printf(ch, "List of %s of %s: [%s]\n",
			    name, class->name, *nl);
		return FALSE;
	}
			    
	if (!pc_name_ok(arg2)) {
		char_printf(ch, "classEd: %s: Illegal name\n", arg2);
		return FALSE;
	}

	name_toggle(nl, arg2, ch, "classEd");
	return TRUE;
}

OLC_FUN(classed_skill_add)
{
	int sn;
	int percent;
	clskill_t *clsk;
	char	arg1[MAX_STRING_LENGTH];
	char	arg2[MAX_STRING_LENGTH];
	char	arg3[MAX_STRING_LENGTH];
	class_t *class;
	EDIT_class(ch, class);

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
		   one_argument(argument, arg3, sizeof(arg3));

	if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
		do_help(ch, "'OLC class SKILL'");
		return FALSE;
	}

	if ((sn = sn_lookup(arg1)) <= 0) {
		char_printf(ch, "classEd: %s: unknown skill.\n", arg1);
		return FALSE;
	}

	if (!IS_SET(SKILL(sn)->flags, SKILL_class)) {
		char_printf(ch, "classEd: %s: not a class skill.\n",
			    SKILL(sn)->name);
		return FALSE;
	}

	if ((clsk = clskill_lookup(class, sn))) {
		char_printf(ch, "classEd: %s: already there.\n",
			    SKILL(sn)->name);
		return FALSE;
	}

	percent = atoi(arg3);
	if (percent < 1 || percent > 100) {
		char_puts("classEd: percent value must be in range 1..100.\n",
			  ch);
		return FALSE;
	}

	clsk = varr_enew(&class->skills);
	clsk->sn = sn;
	clsk->level = atoi(arg2);
	clsk->percent = percent;
	varr_qsort(&class->skills, cmpint);

	return TRUE;
}

OLC_FUN(classed_skill_del)
{
	char	arg[MAX_STRING_LENGTH];
	clskill_t *clsk;
	class_t *class;
	EDIT_class(ch, class);

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		do_help(ch, "'OLC class SKILL'");
		return FALSE;
	}

	if ((clsk = skill_vlookup(&class->skills, arg)) == NULL) {
		char_printf(ch, "classEd: %s: not found in class skill list.\n",
			    arg);
		return FALSE;
	}
	clsk->sn = 0;
	varr_qsort(&class->skills, cmpint);
	return TRUE;
}







bool touch_class(class_t *class)
{
	SET_BIT(class->flags, class_CHANGED);
	return FALSE;
}

static VALIDATE_FUN(validate_name)
{
	int i;
	class_t *class;
	EDIT_class(ch, class);

	for (i = 0; i < classs.nused; i++)
		if (class(i) != class
		&&  !str_cmp(class(i)->name, arg)) {
			char_printf(ch, "classEd: %s: duplicate class name.\n",
				    arg);
			return FALSE;
		}

	return TRUE;
}

