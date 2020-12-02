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
 * $Id: clan.h 899 2006-09-29 19:23:00Z zsuzsu $
 */

#ifndef _CLAN_H_
#define _CLAN_H_

/*----------------------------------------------------------------------
 * clan stuff (clan.c)
 */

#define CLAN_ITEM_RAID_TIMER 30

#define CLAN_NONE 	 0

/* Clan status */

#define CLAN_FREEMAN     0

enum {
        CLAN_OUTCAST,
        CLAN_PROBATION,
	CLAN_RECRUIT,
	CLAN_SOLDIER,
	CLAN_VETERAN,
	CLAN_LIEUTENANT,
	CLAN_CAPTAIN,
	CLAN_VASSAL,
	CLAN_AMBASSADOR,
	CLAN_MAGISTRATE,
	CLAN_LORD,
	CLAN_ELDER,
	CLAN_LEADER,
	CLAN_PATRON,
	MAX_CLAN_RANK
};

enum {
	CLAN_ACTIVITY_DELETED,
	CLAN_ACTIVITY_INACTIVE,
	CLAN_ACTIVITY_MIA,
	CLAN_ACTIVITY_AWOL,
	CLAN_ACTIVITY_POOR,
	CLAN_ACTIVITY_SEMI,
	CLAN_ACTIVITY_ACTIVE,
	CLAN_ACTIVITY_HYPER
};


struct clan_titles
{
    const char *title[3];
};


/*
 * Clan structure
 */
struct clan_t
{
	const char *	name;		/* clan name */
	const char *	file_name;	/* file name */

	int	 	recall_vnum;	/* recall room vnum */

	varr		skills;		/* clan skills */

	flag32_t	flags;		/* clan flags */

	int 		altar_vnum;	/* vnum of room with clan item */
	int 		donate_vnum;    /*vnum of room to donate to*/
	int	 	obj_vnum;	/* vnum of clan item */
	int		mark_vnum;	/* vnum of clan mark */
	OBJ_DATA *	obj_ptr;	/* pointer to clan item */
	OBJ_DATA *	altar_ptr;	/* pointer to altar (obj with clan item)*/
	const char *    member_list[MAX_CLAN_RANK];		/*list of members at each rank*/
	struct clan_titles rank_table[MAX_CLAN_RANK];	/*list of rank titles*/
};

extern const struct clan_titles default_clan_rank_table[];

/* clan flags */
#define CLAN_HIDDEN	(A)		/* clan will not appear in who */
#define CLAN_CHANGED	(Z)

clan_t *	clan_new	(void);		/* allocate new clan data */
void		clan_free	(clan_t*);	/* free clan data */
int		cln_lookup	(const char* name); /* clan number lookup */
const char*	clan_name	(int cln);	/* clan name lookup */
bool		clan_item_ok	(int cln);	/* check clan item */

extern varr	clans;

#define CLAN(cln)		((clan_t*) VARR_GET(&clans, cln))
#define clan_lookup(cln)	((clan_t*) varr_get(&clans, cln))
#define CLAN_RANK_TITLE(ch)	(clan_lookup(ch->clan)->rank_table[ch->pcdata->clan_status].title[ch->sex])

struct clskill_t {
	int	sn;		/* skill number. leave this field first	 */
				/* in order sn_vlookup to work properly  */
	int	level;		/* level at which skill become available */
	int	percent;	/* initial percent			 */
};

#define clskill_lookup(clan, sn) \
	((clskill_t*) varr_bsearch(&clan->skills, &sn, cmpint))

/*
 * clan lists utils
 */
void	clan_update_lists	(clan_t *clan, CHAR_DATA *victim);
void	clan_update_list	(clan_t *clan, const char *name);
void	clan_save		(clan_t *clan);
bool	can_clan_wear		(CHAR_DATA *ch, int cln, OBJ_DATA *obj, int wear_loc);
bool	can_clan_wear_obj	(CHAR_DATA *ch, int cln, OBJ_DATA *obj);
bool	can_clan_wear_loc	(CHAR_DATA *ch, int cln, int wear_loc);
bool	clan_remove_outcast	(CHAR_DATA *ch);
bool	clan_item_loot		(CHAR_DATA *ch, OBJ_DATA *item, OBJ_DATA *container);
int	clan_raid_count		(clan_t *clan);
bool	clan_item_raid		(CHAR_DATA *ch, OBJ_DATA *item, OBJ_DATA *container);
bool	clan_item_expire	(OBJ_DATA *item);
int 	clan_act 		(clan_t *clan, const char *msg, 
				void *arg2, void *arg3, 
				int flags, int min_pos);
int	clan_free_slaves	(CHAR_DATA *ch);
int 	clan_minons_remove	(CHAR_DATA *ch, clan_t *clan);
int	clan_activity		(CHAR_DATA *ch);
int	clan_activity_for_name	(const char *name);
clan_t *clan_for_char		(const char *name);
int	clan_activity_limited_bonus	(CHAR_PDATA *pch);
int	clan_raid_limited_bonus	(CHAR_PDATA *pch);
int	clan_items_remove	(CHAR_DATA *ch, clan_t *clan);


#endif
