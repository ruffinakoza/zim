/*
 * $Id: mem.c 933 2006-11-19 22:37:00Z zsuzsu $
 */

/***************************************************************************
 *  File: mem.c                                                            *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "db/db.h"

/*
 * Globals
 */
extern          int                     top_reset;
extern          int                     top_area;
extern          int                     top_exit;
extern          int                     top_room;
extern		int			top_mprog_index;

void	aff_free	(AFFECT_DATA *af);

RESET_DATA *new_reset_data(void)
{
	RESET_DATA *pReset;

	pReset = calloc(1, sizeof(*pReset));
	pReset->command = 'X';

	top_reset++;
	return pReset;
}

void free_reset_data(RESET_DATA *pReset)
{
	if (!pReset)
		return;
	top_reset--;
	free(pReset);
}

AREA_DATA *new_area(void)
{
	AREA_DATA *pArea;

	pArea = calloc(1, sizeof(*pArea));
	pArea->vnum		= top_area;
	pArea->file_name	= str_printf("area%d.are", pArea->vnum);
	pArea->builders		= str_empty;
	pArea->name		= str_dup("New area");
	pArea->empty		= TRUE;              /* ROM patch */
	pArea->security		= 1;

	top_area++;
	return pArea;
}

/*****************************************************************************
 Name:		area_lookup
 Purpose:	Returns pointer to area with given vnum.
 Called by:	do_aedit(olc.c).
 ****************************************************************************/
AREA_DATA *area_lookup(int vnum)
{
	AREA_DATA *pArea;

	for (pArea = area_first; pArea; pArea = pArea->next)
		if (pArea->vnum == vnum)
			return pArea;

	return 0;
}

AREA_DATA *area_vnum_lookup(int vnum)
{
	AREA_DATA *pArea;

	for (pArea = area_first; pArea; pArea = pArea->next) {
		 if (vnum >= pArea->min_vnum
		 &&  vnum <= pArea->max_vnum)
		     return pArea;
	}

	return 0;
}

void free_area(AREA_DATA *pArea)
{
	free_string(pArea->name);
	free_string(pArea->file_name);
	free_string(pArea->builders);
	free_string(pArea->credits);
	top_area--;
	free(pArea);
}

EXIT_DATA *new_exit(void)
{
	EXIT_DATA *pExit;

        pExit = calloc(1, sizeof(*pExit));
	pExit->keyword = str_empty;

        top_exit++;
	return pExit;
}

void free_exit(EXIT_DATA *pExit)
{
	if (!pExit)
		return;

	free_string(pExit->keyword);
	mlstr_free(pExit->description);

	top_exit--;
	free(pExit);
}

ROOM_INDEX_DATA *new_room_index(void)
{
	ROOM_INDEX_DATA *pRoom;

        pRoom = calloc(1, sizeof(*pRoom));
	pRoom->owner = str_empty;
	pRoom->heal_rate = 100;
	pRoom->mana_rate = 100;

        top_room++;
	return pRoom;
}

void free_room_index(ROOM_INDEX_DATA *pRoom)
{
	int door;
	RESET_DATA *pReset;

	mlstr_free(pRoom->name);
	mlstr_free(pRoom->description);
	free_string(pRoom->owner);

	for (door = 0; door < MAX_DIR; door++)
        	if (pRoom->exit[door])
			free_exit(pRoom->exit[door]);

	ed_free(pRoom->ed);

	for (pReset = pRoom->reset_first; pReset; pReset = pReset->next)
		free_reset_data(pReset);

	top_room--;
	free(pRoom);
}

SHOP_DATA *new_shop(void)
{
	SHOP_DATA *pShop;

        pShop = calloc(1, sizeof(*pShop));
	pShop->profit_buy   =   100;
	pShop->profit_sell  =   100;
	pShop->close_hour   =   23;

        top_shop++;
	return pShop;
}

void free_shop(SHOP_DATA *pShop)
{
	if (!pShop)
		return;
	top_shop--;
	free(pShop);
}

OBJ_INDEX_DATA *new_obj_index(void)
{
	OBJ_INDEX_DATA *pObj;

        pObj = calloc(1, sizeof(*pObj));

	pObj->name		= str_dup(str_empty);
	pObj->item_type		= ITEM_TRASH;
	pObj->rrace		= RRACE_NONE;
	pObj->rsex		= RSEX_NONE;
	pObj->rclass		= RCLASS_NONE;
	pObj->rsize		= RSIZE_NONE;
	pObj->material		= MATERIAL(MAT_UNKNOWN);
	pObj->material_descr	= str_dup(str_empty);
	pObj->condition		= 100;
	pObj->limit		= -1;

        top_obj_index++;
	return pObj;
}

void free_obj_index(OBJ_INDEX_DATA *pObj)
{
	AFFECT_DATA *paf, *paf_next;

	if (!pObj)
		return;

	free_string(pObj->name);
	free_string(pObj->material_descr);
	pObj->material = NULL;
	mlstr_free(pObj->short_descr);
	mlstr_free(pObj->description);

	for (paf = pObj->affected; paf; paf = paf_next) {
		paf_next = paf->next;
		aff_free(paf);
	}

	ed_free(pObj->ed);
    
	top_obj_index--;
	free(pObj);
}

MOB_INDEX_DATA *new_mob_index(void)
{
	MOB_INDEX_DATA *pMob;

        pMob = calloc(1, sizeof(*pMob));
	pMob->name		= str_dup(str_empty);
	pMob->act		= ACT_NPC;
	pMob->race		= rn_lookup("human");
	pMob->material		= MATERIAL(MAT_DEFAULT_MOB);
	pMob->material_descr	= str_dup(str_empty);
	pMob->size		= SIZE_MEDIUM;
	pMob->start_pos		= POS_STANDING;
	pMob->default_pos	= POS_STANDING;

	top_mob_index++;
	return pMob;
}

void free_mob_index(MOB_INDEX_DATA *pMob)
{
	if (!pMob)
		return;

	free_string(pMob->name);
	free_string(pMob->material_descr);
	mlstr_free(pMob->short_descr);
	mlstr_free(pMob->long_descr);
	mlstr_free(pMob->description);
	mptrig_free(pMob->mptrig_list);
	free_shop(pMob->pShop);

	top_mob_index--;
	free(pMob);
}

MPCODE *mpcode_list;

MPCODE *mpcode_new(void)
{
	MPCODE *mpcode;

        mpcode = calloc(1, sizeof(*mpcode));
	mpcode->code = str_empty;

	top_mprog_index++;
	return mpcode;
}

void mpcode_add(MPCODE *mpcode)
{
	if (mpcode_list == NULL)
		mpcode_list = mpcode;
	else {
		mpcode->next = mpcode_list;
		mpcode_list = mpcode;
	}
}

MPCODE *mpcode_lookup(int vnum)
{
	MPCODE *mpcode;
	for (mpcode = mpcode_list; mpcode; mpcode = mpcode->next)
	    	if (mpcode->vnum == vnum)
        		return mpcode;
	return NULL;
}    
 
void mpcode_free(MPCODE *mpcode)
{
	if (!mpcode)
		return;

	free_string(mpcode->code);

	top_mprog_index--;
	free(mpcode);
}
