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
 * $Id: quest.h 849 2006-04-22 13:08:54Z zsuzsu $
 */

#ifndef _QUEST_H_
#define _QUEST_H_

/*
 * Quest obj vnums must take a continuous interval for proper quest generating.
 */
#define QUEST_OBJ_FIRST 84
#define QUEST_OBJ_LAST  87

#define QUEST_NEWBIE_DISCOUNT 1/3 /*multiplicative discount for newbies */
/*
 * quest items
 */
#define QUEST_VNUM_AMULET	94
#define QUEST_VNUM_RING		95
#define QUEST_VNUM_BRACER	49
#define QUEST_VNUM_RUG		50
#define QUEST_VNUM_SONG		40
/* weapons */
#define QUEST_VNUM_LANCE	111
#define QUEST_VNUM_BOW		110
#define QUEST_VNUM_POLEARM	105
#define QUEST_VNUM_BASTARDSWORD 101
#define QUEST_VNUM_BATTLEAXE	103
#define QUEST_VNUM_WARHAMMER	112
#define QUEST_VNUM_LONGSWORD	31
#define QUEST_VNUM_AXE		113
#define QUEST_VNUM_HAMMER	114
#define QUEST_VNUM_SHORTSWORD	104
#define QUEST_VNUM_STILETTO	115
#define QUEST_VNUM_DAGGER	32
#define QUEST_VNUM_FLAIL	107
#define QUEST_VNUM_MACE		33
#define QUEST_VNUM_SPEAR	106
#define QUEST_VNUM_STAFF	109
#define QUEST_VNUM_WHIP		108

//#define QUEST_VNUM_CANTEEN	69

#define TROUBLE_MAX -1 /*-1 is infinite*/ 

struct qtrouble_t {
	int vnum;
	int count;
	qtrouble_t *next;
};

#define IS_ON_QUEST(ch)	(ch->pcdata->questtime > 0)

void quest_handle_death(CHAR_DATA *ch, CHAR_DATA *victim);
void quest_cancel(CHAR_DATA *ch);
void quest_update(void);
bool is_quest_item(OBJ_INDEX_DATA *pObj);
bool is_quest_complete(CHAR_DATA *ch);

int qtrouble_get(CHAR_DATA *ch, int vnum);
void qtrouble_set(CHAR_DATA *ch, int vnumi, int count);

#endif

