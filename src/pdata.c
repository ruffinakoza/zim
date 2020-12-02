/*-
 * Copyright (c) 2005 Zsuzsu <little_zsuzsu@hotmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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
 * $Id: pdata.c 948 2006-12-04 00:55:16Z zsuzsu $
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "db/cmd.h"
#include "interp.h"
#include "pdata.h"

/*
 * pdata - persistant data for when mortals are not online
 *
 * by Zsuzsu
 */
CHAR_PDATA *	ch_pdata_list = NULL;

/*
 * lookup the character name in the pdata list
 * and return the pdata structure or NULL if not found.
 */
CHAR_PDATA * ch_pdata_lookup (const char *name)
{
	CHAR_PDATA *pch;

	if (IS_NULLSTR(name))
		return NULL;

	for (pch = ch_pdata_list; pch; pch = pch->next) {
		if (is_name(pch->name, name))
			return pch;
	}

	return NULL;
}

int ch_pdata_level (CHAR_PDATA *pch) 
{

	if (pch == NULL)
		return -1;
	if (pch->online == NULL)
		return pch->level;
	else
		return pch->online->level;
}

/* list / character integrity functions *****************************************/
CHAR_PDATA *sync_ch_pdata (CHAR_DATA *ch)
{
	CHAR_PDATA *pdata = NULL;

	if (IS_NPC(ch))
		return NULL;

	if (ch->pcdata->pdata == NULL)
		add_char_pdata(ch);

	pdata = ch->pcdata->pdata;
	pdata->online = ch;
	pdata->name = ch->name;
	pdata->level = ch->level;
	/*pdata->last_logoff = ch->logon; don't update*/
	pdata->limiteds = count_limiteds(ch);

	return pdata;
}

/*
 * either looks-up the ch's pdata and returns it
 * or creates a new instance of pdata and adds it to
 * the pdata list.
 */
CHAR_PDATA *add_char_pdata (CHAR_DATA *ch)
{
	CHAR_PDATA *pdata = NULL;

	pdata = ch_pdata_lookup(ch->name);

	if (ch->pcdata->pdata == NULL)
		ch->pcdata->pdata = pdata;
	else if (!pdata) {
		BUG("character has pdata but it's not in the pdata list.");
		/* not freeing it in case someone points to it.
		 * no good way to solve this one.*/
	}
	else if (pdata != ch->pcdata->pdata) {
		BUG("character has pdata which is not equal to pdata in list");
		free_char_pdata(ch->pcdata->pdata);

		return ch->pcdata->pdata = pdata;
	}

	if (!pdata) {
		pdata = new_char_pdata(ch->name);
		ch->pcdata->pdata = pdata;
		pdata->next = ch_pdata_list;
		ch_pdata_list = pdata;
	}

	return pdata;
}

/* Memory management functions *******************************************/

CHAR_PDATA *new_char_pdata (const char *name)
{
	CHAR_PDATA *pdata = NULL;
	int i;

	pdata = calloc(1, sizeof(*pdata));

	pdata->name = name;
	pdata->level = -1;
	pdata->online = NULL;
	pdata->deleted = FALSE;
	pdata->limiteds = 0;

	pdata->last_usage_update = current_time;
	for (i=0; i < USAGE_RECORDED_DAYS; i++)
		pdata->usage[i] = USAGE_UNDEFINED;

	return pdata;
}

void free_char_pdata (CHAR_PDATA *pdata)
{
	pdata->name = NULL;
	pdata->level = -1;
	pdata->online = NULL;
	pdata->next = NULL;

	free(pdata);
}

/* player commands *********************************************************/

DO_FUN(do_pdata)
{
	char arg1[MAX_INPUT_LENGTH];
	argument = one_argument(argument, arg1, sizeof(arg1));
	char helpname[] = "'WIZ PDATA'";
	CHAR_PDATA *pch;
	bool found = FALSE;
	struct tm *timeinfo;
	bool exact_name = FALSE;

	if (arg1[0] == '\0')  {
		do_help(ch, helpname);
		return;
	}

	if (arg1[strlen(arg1) -1] == '#') {
		exact_name = TRUE;
		arg1[strlen(arg1) -1] = '\0';
	}

	for (pch = ch_pdata_list; !found && pch; pch = pch->next) {
		if ((exact_name && !str_cmp(arg1, pch->name))
		|| (!exact_name && !str_prefix(arg1, pch->name))) {
			char_printf(ch, "PData for %s\n", pch->name);
			char_printf(ch, "Level:        {c%d{x\n", 
					pch->level);
			char_printf(ch, "Online:       {c%s{x\n", 
					(pch->online) ? "true" : "false");
			char_printf(ch, "Deleted:      {c%s{x\n", 
					(pch->deleted) ? "true" : "false");
			char_printf(ch, "PKs:          {r%d{x kills {D%d{x deaths {D(%d) old kills{x\n",
					pch->pk_kills, pch->pk_deaths, pch->pc_killed);
			char_printf(ch, "Hours Played: {c%d{x hours\n", 
					(int) (pch->played / 3600));
			char_printf(ch, "QPs:          {c%d{xpts {c%d{x quests\n",
					pch->questpoints, pch->questcount);
			char_printf(ch, "Bank Gold:    {c%d{x\n",
					pch->bank_g);
			char_printf(ch, "Artifacts:    {c%d{x possessed {c%d{x allowed\n", 
					pch->limiteds, usage_allowed_limiteds(pch));

			timeinfo = localtime(&pch->last_logoff);
			char_printf(ch, "Last logoff:  {c%s{x",
					asctime(timeinfo));
			char_printf(ch, "\n");
			found = TRUE;
		}
	}
	if (!found) {
		char_printf(ch, "Couldn't find %s", arg1);
	}
}
