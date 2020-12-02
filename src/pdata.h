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
 * $Id: pdata.h 849 2006-04-22 13:08:54Z zsuzsu $
 */

/*
 * pdata - persistant data for when mortals are not online
 *
 * by Zsuzsu
 */

#ifndef _PDATA_H_
#define _PDATA_H_

/* typedef struct char_pdata	CHAR_PDATA; put in typedef.h*/

struct char_pdata {
	CHAR_PDATA *	next;

	/* redundant data */
	const char *	name;
	CHAR_DATA *	online;		/* actual online character */
	int		level;

	int		bank_g;

	int		played;
        int		pc_killed;
	int		pk_kills;
	int		pk_deaths;
	int		questpoints;
	int		questcount;

	/* unique data */
	bool		deleted;
	time_t		last_logoff;

	int		usage[USAGE_RECORDED_DAYS];
	time_t		last_usage_update;
	time_t		last_limited_reclaim;
	int		limiteds;

};

/* globals */
extern CHAR_PDATA *	ch_pdata_list;

/* functions */
CHAR_DATA *	ch_lookup (const char *name);
CHAR_PDATA *	ch_pdata_lookup (const char *name);
int		ch_pdata_level (CHAR_PDATA *pch);
time_t		ch_pdata_last_logoff (CHAR_PDATA *pch);
int *		ch_pdata_usage (CHAR_PDATA *pch);
time_t		ch_pdata_last_usage_update (CHAR_PDATA *pch);
time_t		ch_pdata_last_limited_reclaim (CHAR_PDATA *pch);
CHAR_PDATA *	sync_ch_pdata (CHAR_DATA *ch);
CHAR_PDATA *	add_char_pdata (CHAR_DATA *ch);
CHAR_PDATA *	new_char_pdata (const char *name);
void		free_char_pdata (CHAR_PDATA *pdata);

#endif
