/*-
 * Copyright (c) 2005 Zsuzsu <little_zsuzsu@hotmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
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
 * $Id: waffects.h 917 2006-10-13 22:06:33Z zsuzsu $
 */

#include <stdarg.h>
#include <stdio.h>
#include "typedef.h"
#include "const.h"
#include "merc.h"
#include "fixes.h"

void advance(CHAR_DATA *victim, int level);
/*
 * This is for bug fixes.
 */

flag_t fixed_flags[] =
{
        { "",                   TABLE_BITVAL                    },

        { "v2HP/DR",            FIXED_V2_HP_DR,         FALSE   },

        { NULL }
};



void fix_1_v2_hp_dr (CHAR_DATA *ch)
{
	int level = ch->level;
	OBJ_DATA *obj = NULL;
	OBJ_DATA *obj_next = NULL;
	OBJ_DATA *clanmark =  NULL;
	char title[MAX_INPUT_LENGTH] = "";

	if (IS_SET(ch->fixed_flags, FIXED_V2_HP_DR))
		return;

	if (level < 2 || level >= IM) {
		TOGGLE_BIT(ch->fixed_flags, FIXED_V2_HP_DR);
		return;
	}

	if (ch->level == LEVEL_HERO)
		strnzcpy(title, sizeof(title), ch->pcdata->title);

	clanmark = get_eq_char(ch, WEAR_CLANMARK);
	if (clanmark != NULL)
		obj_from_char(clanmark);


	for (obj = ch->carrying; obj != NULL; obj = obj_next) {
		obj_next = obj->next_content;
		if (obj->wear_loc != WEAR_NONE)
			unequip_char(ch, obj);
	}
	while (ch->affected)
		affect_remove(ch, ch->affected);
	ch->affected_by     = 0;

	advance(ch, 1);
	advance(ch, level);

	if (clanmark != NULL) {
		obj_to_char(clanmark, ch);
		equip_char(ch, clanmark, WEAR_CLANMARK);
	}
	if (ch->level == LEVEL_HERO) {
		ch->pcdata->title = str_dup(title);
	}

	ch->hit        = ch->max_hit;
	ch->mana       = ch->max_mana;
	ch->move       = ch->max_move;

	TOGGLE_BIT(ch->fixed_flags, FIXED_V2_HP_DR);
	char_puts("\n{WYou feel naked and reborn!{x\n", ch);
	wiznet("fixed: $N v2 hp/dr", ch, NULL, WIZ_ANNOUNCE, 0, 0);
}
