/*
 * $Id: olc_mpcode.c 849 2006-04-22 13:08:54Z zsuzsu $
 */

/* The following code is based on ILAB OLC by Jason Dinkel */
/* Mobprogram code by Lordrom for Nevermore Mud */

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "olc.h"

#define EDIT_MPCODE(ch, mpcode)   (mpcode = (MPCODE*) ch->desc->pEdit)

/* Mobprog editor */
DECLARE_OLC_FUN(mped_create		);
DECLARE_OLC_FUN(mped_edit		);
DECLARE_OLC_FUN(mped_touch		);
DECLARE_OLC_FUN(mped_show		);
DECLARE_OLC_FUN(mped_list		);

DECLARE_OLC_FUN(mped_code		);

olc_cmd_t olc_cmds_mpcode[] =
{
/*	{ command	function	}, */

	{ 3, "create",	mped_create	},
	{ 3, "edit",	mped_edit	},
	{ 3, "touch",	mped_touch	},
	{ 0, "show",	mped_show	},
	{ 0, "list",	mped_list	},

	{ 3, "code",	mped_code	},

	{ 0, "commands",show_commands	},

	{ 0, NULL }
};

OLC_FUN(mped_create)
{
	MPCODE *mpcode;
	int value;
	AREA_DATA *pArea;
	char arg[MAX_STRING_LENGTH];

	one_argument(argument, arg, sizeof(arg));
	value = atoi(arg);
	if (!value) {
		do_help(ch, "'OLC CREATE'");
		return FALSE;
	}

	pArea = area_vnum_lookup(value);
	if (!pArea) {
		char_puts("MPEd: That vnum is not assigned an area.\n", ch);
		return FALSE;
	}

	if (!IS_BUILDER(ch, pArea)) {
		char_puts("MPEd: Insufficient security.\n", ch);
		return FALSE;
	}

	if (mpcode_lookup(value)) {
		char_puts("MPEd: vnum already exists.\n", ch);
		return FALSE;
	}

	mpcode		= mpcode_new();
	mpcode->vnum	= value;
	mpcode_add(mpcode);

	ch->desc->pEdit	= (void*) mpcode;
	OLCED(ch)	= olced_lookup(ED_MPCODE);
	touch_area(pArea);
	char_puts("MPEd: mpcode created.\n", ch);
	return FALSE;
}

OLC_FUN(mped_edit)
{
	MPCODE *mpcode;
	AREA_DATA *pArea;
	int value;
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		do_help(ch, "'OLC EDIT'");
		return FALSE;
	}

	value = atoi(arg);
	mpcode = mpcode_lookup(value);
	if (!mpcode) {
		char_puts("MPEd: Vnum does not exist.\n", ch);
		return FALSE;
	}

	pArea = area_vnum_lookup(mpcode->vnum);
	if (!IS_BUILDER(ch, pArea)) {
		char_puts("MPEd: Insufficient security.\n", ch);
	       	return FALSE;
	}

	ch->desc->pEdit = (void*) mpcode;
	OLCED(ch)	= olced_lookup(ED_MPCODE);
	return FALSE;
}

OLC_FUN(mped_touch)
{
	MPCODE *mpcode;
	EDIT_MPCODE(ch, mpcode);
	return touch_vnum(mpcode->vnum);
}

OLC_FUN(mped_show)
{
	MPCODE *mpcode;
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		if (IS_EDIT(ch, ED_MPCODE))
			EDIT_MPCODE(ch, mpcode);
		else {
			do_help(ch, "'OLC ASHOW'");
			return FALSE;
		}
	}
	else {
		int value = atoi(arg);
		mpcode = mpcode_lookup(value);
		if (!mpcode) {
			char_puts("MPEd: Vnum does not exist.\n", ch);
			return FALSE;
		}
	}

	char_printf(ch, "Vnum:       [%d]\n"
			"Code:\n%s\n",
		   mpcode->vnum, mpcode->code);

	return FALSE;
}

OLC_FUN(mped_list)
{
	int count = 1;
	MPCODE *mpcode;
	BUFFER *buffer;
	bool fAll = !str_cmp(argument, "all");
	char blah;
	AREA_DATA *ad;

	buffer = buf_new(-1);

	for (mpcode = mpcode_list; mpcode !=NULL; mpcode = mpcode->next)
		if (fAll || ENTRE(ch->in_room->area->min_vnum, mpcode->vnum, ch->in_room->area->max_vnum)) {
			ad = area_vnum_lookup(mpcode->vnum);

			if (ad == NULL)
				blah = '?';
			else
			if (IS_BUILDER(ch, ad))
				blah = '*';
			else
				blah = ' ';

			buf_printf(buffer, "{D[{W%3d{D]{x (%c) %5d\n", count, blah, mpcode->vnum);
			count++;
		}

	if (count == 1) {
		if (fAll)
			buf_add(buffer, "No mobprogs found.\n");
		else
			buf_add(buffer, "No mobprogs found in this area.\n");
	}

	page_to_char(buf_string(buffer), ch);
	buf_free(buffer);

	return FALSE;
}

OLC_FUN(mped_code)
{
	MPCODE *mpcode;
	EDIT_MPCODE(ch, mpcode);
	return olced_str_text(ch, argument, cmd, &mpcode->code);
}

