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
 * $Id: waffects.h 1019 2007-02-15 00:52:41Z zsuzsu $
 */

/*
 * This file handles the World State.
 * Effects that are world-wide, and other persistant effects and
 * items that are supposed to survive reboot.
 */

#ifndef _WAFFECTS_H_
#define _WAFFECTS_H_

/*
 * affects applied to the whole realm
 */
typedef struct world_affect_data	WORLD_AFFECT_DATA;

struct world_affect_data
{
	WORLD_AFFECT_DATA*	next;
	CHAR_DATA *		player;		/* so mob death can invalidate*/
	const char *		player_name;
	int			level;		/* use for who can repeal the affect */
	int			type;		/* kind of affect */
	int			modifier;
	int			duration;	/* how long in ticks */
	int			interval;	/* ticks between affect active*/
	int			timer;		/* ticks left/between*/
	int			chance;		/* chance it affects you */
	int			min_level;	/* min level affected by it */
	int			max_level;
	int			visible_level;	/* min level that can see the affect*/
	int			start_hour;	/* schedule a start time 24h */
	int			repeat;		/* number of times to repeat */
	bool			active;
	flag32_t		notify;		/* notify players when on/off*/
	AREA_DATA *		area;		/* the area affected, null is world */
};

/* global variables */
extern          WORLD_AFFECT_DATA *	world_affect_list;
extern		flag_t			waff_types[];
extern		flag_t			waff_mortal_on[];
extern		flag_t			waff_mortal_off[];

/*
 *  WAFF global affects
 *   
 */
#define WAFF_NONE		0
#define WAFF_ARENA              1	/*put the world in arena mode*/
#define WAFF_PEACE              2	/*one global peace room*/
#define WAFF_EXP		3	/*exp multiplier*/
#define WAFF_QP			4	/*base quest point multiplier*/
#define WAFF_GOLD		5	/*dropped money multipler*/
#define WAFF_PVP_DAMAGE		6	/*damage players do to each other*/
#define WAFF_PVM_DAMAGE		7	/*damage players do to mobs*/
#define WAFF_AUCTION		8	/*turn auction on (1) or off (-1)*/
#define WAFF_INFLATION		9	/*adjust prices by X amount */
#define WAFF_FFA		10	/*no PK range over level 10 */
#define WAFF_PK_RANGE		11	/* increase/decrease PK range */

#define WAFF_NOTIFY_NONE	0	/*don't notify morts*/
#define WAFF_NOTIFY_ON		(A)	/*notify when affect goes active*/
#define WAFF_NOTIFY_OFF		(B)	/*notify when deactivate affect*/

#define WAFF_REPEAT_PERM	-1
#define WAFF_REPEAT_NONE	0

/* functions */
bool			is_affected_area	(int sn, AREA_DATA *area);
bool			is_affected_world	(int sn);
bool			is_waffected		(CHAR_DATA *ch, int sn);
WORLD_AFFECT_DATA *	ch_waffected		(CHAR_DATA *ch, int sn);
WORLD_AFFECT_DATA *	affect_find_area	(int sn, AREA_DATA *area);
WORLD_AFFECT_DATA *	affect_find_world	(int sn);
void			affect_to_area 		(WORLD_AFFECT_DATA *paf);
void			affect_to_world		(WORLD_AFFECT_DATA *paf);
void			affect_remove_area 	(WORLD_AFFECT_DATA *paf);
void			affect_remove_world	(WORLD_AFFECT_DATA *paf);
void			affect_strip_world	(int sn);
void			affect_strip_area	(int sn, AREA_DATA *area);
void			affect_join_world	(WORLD_AFFECT_DATA *paf);

/* persistance */
void			do_save_waffs		(CHAR_DATA *ch, 
						const char *argument);
int			load_waffs		(void);
bool 			save_waffs		(void);

/* update.c */
void			world_affect_update	(void);
/* free/delete */
WORLD_AFFECT_DATA *	waff_new		(void);
WORLD_AFFECT_DATA *	waff_dup		(const WORLD_AFFECT_DATA *paf);
void			waff_free		(WORLD_AFFECT_DATA *paf);

#endif
