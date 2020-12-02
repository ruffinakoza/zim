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
 * $Id: usage.c 1019 2007-02-15 00:52:41Z zsuzsu $
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "usage.h"
#include "pdata.h"
#include "clan.h"
#include "debug.h"
#include "db/db.h"

time_t server_last_usage_update = 0L;
int server_usage[USAGE_RECORDED_DAYS];
int usage_allowed_limiteds (CHAR_PDATA *pch);

/*
 * This file is for determining how much time a player spends
 * online with this character.  This gives a "usage" count.  Usage
 * is used to determine such things as how many artifacts (limited items)
 * the player can carry.
 *
 * This was developed to eliminate the problem of the best artifacts
 * constantly being unavailable because they were in pfiles of unactive
 * players who would log in once a month to check notes.
 *
 * by Zsuzsu
 */

void do_usage(CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_PDATA *pvictim;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0')
		pvictim = ch->pcdata->pdata;
	else 
		pvictim = ch_pdata_lookup(arg);
		
	if (pvictim == NULL) {
		if (!str_prefix(arg, "server")) 
			char_show_usage_generic(ch, server_usage, "Server");
		else 
			char_puts("That soul cannot be found in the Realm nor Astral Plane.\n", ch);
		return;
	}

	ch_update_usage(pvictim, TRUE);
	char_show_usage(ch, pvictim);
}

int * get_usage(const char *player)
{
	CHAR_PDATA *victim;

	if (player[0] == '\0')
		return NULL;

	victim = ch_pdata_lookup(player);

	if (victim == NULL)
		return NULL;
	else
		return victim->usage;
}

long usage_last_days (CHAR_PDATA *pch, int days)
{
	int *usage = NULL;
	long seconds = 0L;
	int j;

	usage = pch->usage;

	for (j = 0; j < USAGE_RECORDED_DAYS && j < days && usage[j] != -1; j++) {
		seconds += usage[j];
	}

	return seconds;
}

void char_show_usage(CHAR_DATA *ch, CHAR_PDATA *pvictim) 
{
	char_show_usage_generic(ch, pvictim->usage, pvictim->name);
	char_printf(ch, "\nArtifacts allowed by usage: {Y%2d{x\n",
			usage_allowed_limiteds(pvictim));
	char_printf(ch, "Artifacts allowed by clan activity: {Y%2d{x\n",
			clan_activity_limited_bonus(pvictim));
	char_printf(ch, "Artifacts allowed by clan raid: {Y%2d{x\n",
			clan_raid_limited_bonus(pvictim));
}

void char_show_usage_generic(CHAR_DATA *ch, int *usage, const char *name)
{
	int i, j, k, ct_wday; 
	struct tm *ct, *mt;
	int a_week = 60 * 60 * 24 * 7;
	int a_day = 60 * 60 * 24;
	time_t monday_time;
	int seconds = 0, days = 0;
	float per_week[USAGE_RECORDED_DAYS / 7 +2];
	float per_week_avg[USAGE_RECORDED_DAYS / 7 +2];
	float sum = 0.0;
	
	char *tab = "         ";
	        /*  " 00:00:00 " */

	if (usage == NULL) {
		BUG("couldn't find usage for %s", name);
		return;
	}

	ct = localtime(&current_time);

	ct_wday = (ct->tm_wday == 0) ? 7 : ct->tm_wday;

	monday_time = current_time - ((ct_wday-1) * a_day);

	char_printf(ch, "Last %d day usage for: {W%s{x\n\n", 
			USAGE_RECORDED_DAYS,
			name);

	char_printf(ch, 
	"date   {Yh/d{G    Mon   {g|{G   Tue   {g|{G   Wed   {g|{G   Thr   {g|{G   Fri   {g|{G   Sat   {g|{G   Sun{x\n");
	/* 2 1 0
	 * 9 8 7 6 5 4 3
	 *           B A
	 */
	for (j = 0; j < (USAGE_RECORDED_DAYS / 7 +2); j++) {
		mt = localtime(&monday_time);

		seconds = 0;
		days = 0;
		i = (j * 7) + ct_wday -1;

		for (k = 6; k >= 0 && i >= 0; k--) {
			if ((i < USAGE_RECORDED_DAYS && i >= 0)
			&& usage[i] != USAGE_UNDEFINED) {
				seconds += usage[i];
				days++;
			}
			i--;
		}
		per_week[j] = (days) ? (seconds / 3600) : -1.0;
		per_week_avg[j] = (days) ? ((seconds / 3600) / days) : -1.0;

		if (!days) break;

		char_printf(ch, "{g%02d{G/{g%02d ",
			mt->tm_mon +1, mt->tm_mday);

		char_printf(ch, "{Y%4.1f{x", per_week_avg[j]);

		i = (j * 7) + ct_wday -1;

		for (k = 6; k >= 0 && i >= 0; k--) {
			if (i > USAGE_RECORDED_DAYS -1)
				char_printf(ch, tab);
			else if (usage[i] == 0)
				char_printf(ch, "%s {D--:--:--{x", k==6 ? "" : " ");
			else if (usage[i] == USAGE_UNDEFINED) {
				char_printf(ch, tab);
			}
			else
				char_printf(ch, "%s %02d{D:{x%02d{D:{x%02d", 
					k==6 ? "" : " ",
					usage[i] / (60 * 60),
					(usage[i] % (60 * 60)) / 60,
					(usage[i] % 60));
			i--;
		}

		char_printf(ch, "\n");

		monday_time = monday_time - a_week;
	}

	char_printf(ch, "\n");

	/* hours per week / in 4 week periods
	 * percentage for the last 7 days
	 * how many limiteds does he qualify for?
	 */

	k = 0;
	sum = 0.0;
	for (j = 0; j < (USAGE_RECORDED_DAYS / 7 +2) && per_week[j] != -1.0; j++) {


		if (k == 0)
			char_printf(ch, "Hours per week: ");
		
		sum += (per_week[j] > 0) ? per_week[j] : 0;
		char_printf(ch, "%5.1f ", per_week[j]);


		if (k++ >= 3) {
			char_printf(ch, " = {G%5.1f{x total,"
					"  {G%5.1f{x/week"
					"  {G%4.1f{x/day\n",
				sum, sum / 4, sum / 28);
			sum = 0.0;
			k = 0;
		}
	}
	char_printf(ch, "\n");

}

int allowed_limiteds (CHAR_PDATA *pch)
{
	int allowed = 0;

	allowed += usage_allowed_limiteds(pch);
	allowed += clan_activity_limited_bonus(pch);
	allowed += clan_raid_limited_bonus(pch);
	return allowed;
}

/*
 * calculate how many limiteds the character is allowed to 
 * to save with.
 *
 * characters are allowed 3 limiteds if:
 *    4hrs/day last 3 days = 1
 *    1hr/day last week    = 1
 *    2hr/day last week    = 2
 *    1hr/day last month   = 3
 *    3hr/day last month   = 4
 *    less than a month recorded activity = 2
 */
int usage_allowed_limiteds (CHAR_PDATA *pch) 
{
	int *usage = NULL;
	int sum = 0;
	int hour = 60*60;
	int j;
	int uptime;
	int min_up = 20;

	usage = pch->usage;

	if (usage == NULL)
		return 0;

	/* special case for those who really play a lot.
	 * the 6hrs a day cap is for people who want to 
	 * log on for 24hrs just to blow the average.
	 */
	sum = 0;
	uptime = 0;
	for (j = 0; j < 28; j++) {
		if (server_usage[j] > (min_up*hour)
		|| usage[j] > hour)
			uptime++;
		if (usage[j] != USAGE_UNDEFINED)
			sum += (usage[j] > 8*hour) ? 8*hour : usage[j];
	}
	sum = sum / hour;

	if (uptime < 1)
		return 3;
	if ((sum / uptime) >= 3)
		return 4;
	else if ((sum / 28) >= 1)
		return 3;

	sum = 0;
	uptime = 0;
	for (j = 0; j < 7; j++) {
		if (server_usage[j] > (min_up*hour)
		|| usage[j] > hour)
			uptime++;
		if (usage[j] != USAGE_UNDEFINED)
			sum += (usage[j] > 8*hour) ? 8*hour : usage[j];
	}
	sum = sum / hour;

	if (uptime < 1)
		return 2;
	else if ((sum / uptime) >= 2)
		return 2;
	else if (usage[28] == USAGE_UNDEFINED)
		return 2;
	else if ((sum / uptime) >= 1)
		return 1;

	sum = 0;
	uptime = 0;
	for (j = 0; j < 3; j++) {
		if (server_usage[j] > (min_up*hour)
		|| usage[j] > hour)
			uptime++;
		sum += (usage[j] > 8*hour) ? 8*hour : usage[j];
	}
	sum = sum / hour;

	if (uptime < 1)
		return 1;
	if ((sum / uptime) >= 4)
		return 1;

	return 0;
}

DO_FUN(do_artifacts)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int artifacts = 0;
	int allowed = 0;
	int clan_bonus = 0;
	int clan_raid_bonus = 0;

	one_argument(argument, arg, sizeof(arg));

	victim = ch;

	allowed = usage_allowed_limiteds(victim->pcdata->pdata);
	artifacts = count_limiteds(victim);
	clan_bonus = clan_activity_limited_bonus(victim->pcdata->pdata);
	clan_raid_bonus = clan_raid_limited_bonus(victim->pcdata->pdata);

	if (allowed == 0)
		char_printf(ch, 
"Your prolonged time in the astral plane has left you too weak\n"
"to carry artifacts from the material plane into your next astral journey.\n");
	else
		char_printf(ch, 
"You feel well grounded in the material plane,\n"
"and so may take {Y%d{x artifacts with you into the astral plane.\n",
			allowed);

	if (clan_bonus > 0)
		char_printf(ch, 
"The health of your clan and your influence over it allow you\n"
"an additional {Y%d{x artifacts.\n",
				clan_bonus);

	if (clan_raid_bonus > 0)
		char_printf(ch, 
"Your clan's domination over the realm allows you to control\n"
"an additional {Y%d{x artifacts.\n",
				clan_raid_bonus);

	if (artifacts == 0)
		char_printf(ch, "You possess no artifacts.\n");
	else
		char_printf(ch, "You have {Y%d{x artifacts in your possession.\n",
				artifacts);
	
}

/*
 * Zsuzsu's way to make sure limiteds aren't kept in dead pfiles.
 * This function looks through the pdata for everyone and finds
 * those people who have too many limiteds for what little time
 * they spend on the mud.  Then the pfile is loaded and the
 * limited items removed to later respawn in the mud.
 */
DO_FUN(do_repossess)
{
	char arg1[MAX_INPUT_LENGTH];
	argument = one_argument(argument, arg1, sizeof(arg1));
	char helpname[] = "'WIZ REPOSSESS'";
	CHAR_PDATA *pch;
	int allowed = 0;
	int repo = 0;
	int players = 0;

	if (arg1[0] == '\0')  {
		do_help(ch, helpname);
		return;
	}

	char_puts("You send an army of astral archeologists"
		  " to unearth dormant treasures.\n", ch);

	if (!str_cmp(arg1, "all")) {
		for (pch = ch_pdata_list; pch; pch = pch->next) {

			if (pch->online == NULL
			&& pch->level < LEVEL_IMMORTAL
			&& pch->limiteds > (allowed = allowed_limiteds(pch))) {
				char_printf(ch, "Repo: %s allowed: %d has: %d\n",
					pch->name, allowed, pch->limiteds);
				repo += repossess_limiteds (ch, pch);
				players++;
			}
		}
	}

	char_printf(ch, "Repossession complete.  %d artifacts from %d dead-beats.", 
			repo, players);
}

/*
 * load the pfile remove the correct number of limited items
 * and save the character -- updating the pdata.
 *
 * returns the number of limiteds claimed
 */
int repossess_limiteds (CHAR_DATA *ch, CHAR_PDATA *pvict)
{
	int allowed = usage_allowed_limiteds(pvict);
	int limiteds = 0;
	CHAR_DATA *victim = NULL;
	DESCRIPTOR_DATA *d = NULL;
	OBJ_DATA *pObj = NULL, *obj_next = NULL;
	int repo = 0;
	OBJ_DATA *note = NULL;
	
	DEBUG(DEBUG_RECLAIM,
		"repossessing %s", pvict->name);

	d = malloc(sizeof(*d));
	d->connected = CON_DUMMY;

	if (!load_char_obj(d, pvict->name)) {
		DEBUG(DEBUG_RECLAIM, "repossess couldn't load: %s", 
			pvict->name);
		return -1;
	}
	victim = d->character;

	for (pObj = victim->carrying; pObj; pObj = obj_next) {
		obj_next = pObj->next_content;

		if (IS_OBJ_LIMITED(pObj->pIndexData)) {
			limiteds++;
			if (limiteds > allowed) {
				if (ch)
					char_printf(ch, "repo: %s loses %s\n",
						victim->name,
						mlstr_mval(pObj->pIndexData->short_descr));
				extract_obj(pObj, 0);
				repo++;
			}
			else {
				if (ch)
					char_printf(ch, "repo: %s keeps %s\n",
						victim->name,
						mlstr_mval(pObj->pIndexData->short_descr));
			}
		}

	}
	/* give the player a note about losing the item */
	if (repo > 0) {
		note = create_obj_of(get_obj_index(OBJ_VNUM_REPOSSESS_NOTE), 0);
		obj_to_char(note, victim);
	}

	pvict->limiteds = allowed;
	pvict->last_limited_reclaim = current_time;
	save_char_obj(victim, FALSE);
	free_char(victim);
	free(d);

	return repo;
}

/*
 * takes the current time, and compares it against the last time
 * the usage counters were updated.  If an update is needed
 * the number of seconds difference is added to the day's usage.
 * If midnight has rolled around, all the day's are shuffled
 * down to make room for the new day's data.
 *
 * by Zsuzsu
 */
void update_usage (time_t *last_update, int *usage, bool is_time_online)
{
	double delta_sec;
	struct tm ct_midnight,
		  lt_midnight;
	time_t ct_midnight_time,
	       lt_midnight_time;
	int shift_days = 0;
	int i;

	/* find the last midnight */
	localtime_r(&current_time, &ct_midnight);
	ct_midnight.tm_hour = 0;
	ct_midnight.tm_min = 0;
	ct_midnight.tm_sec = 0;
	ct_midnight_time = mktime(&ct_midnight);

	/* find the last midnight */
	localtime_r(last_update, &lt_midnight);
	lt_midnight.tm_hour = 0;
	lt_midnight.tm_min = 0;
	lt_midnight.tm_sec = 0;
	lt_midnight_time = mktime(&lt_midnight);

	/*
	 * Check if it's a different day to shift the stats
	 * down the array to make room for the new day's stats.
	 */ 
	if (ct_midnight_time != lt_midnight_time) {

		delta_sec = difftime(ct_midnight_time, lt_midnight_time);

		shift_days = delta_sec / (24 * 60 * 60);

		if (shift_days < 0) {
			bug("update_usage - shift days is negative?", shift_days);
			shift_days = USAGE_RECORDED_DAYS;
		}

		for (i = USAGE_RECORDED_DAYS-1; i >= 0; i--) {
			if (shift_days >= USAGE_RECORDED_DAYS ||  i < shift_days)
				usage[i] = 0;
			else
				usage[i] = usage[i-shift_days];
		}
	}

	/*
	 * if this is time online, then update the counter
	 */
	if (is_time_online) {
		delta_sec = difftime(current_time, *last_update);
		usage[0] += delta_sec;
	}

	/* *last_update = current_time;*/
	*last_update = current_time;
}

void ch_update_usage (CHAR_PDATA *pch, bool is_time_online)
{
	update_usage(&pch->last_usage_update, pch->usage, 
	is_time_online
	&& pch->online 
	&& pch->online->desc && pch->online->desc->connected != CON_DUMMY);
}

void server_update_usage ()
{
	update_usage(&server_last_usage_update, server_usage, TRUE);
}

/*
 * parses a usage string in the form of: <seconds> <seconds> <seconds>~
 * These are put into the character's usage[].
 */
void parse_usage (int *usage, const char *str)
{
	char delim[] = " ";
	char *p = NULL;
	int i = 0;

	if (!usage) {
		BUG("parse_usage: sent null usage array");
		exit(1);
	}

	p = strtok((char *) str, delim);

	while (p && *p != '~' && i < USAGE_RECORDED_DAYS) {
		usage[i++] = atoi(p);
		p = strtok(NULL, delim);
	}

	for(;i < USAGE_RECORDED_DAYS; i++) {
		usage[i] = USAGE_UNDEFINED;
	}
}

/*
 * update server's usage for the greatest
 * number of minutes anyone has been on for the day.
 * which might be the total uptime for some IMMs.
 */
void approximate_server_usage_from_pdata ()
{
	CHAR_PDATA *pch = ch_pdata_list;
	int i;

	while (pch) {
		for (i=0; i < USAGE_RECORDED_DAYS; i++) {
			if (server_usage[i] < pch->usage[i])
				server_usage[i] = pch->usage[i];
		}
		pch = pch->next;
	}
	server_last_usage_update = current_time;
}

#define END_TAG "#END\n"
/*
 * save server usage to a file, so we can recover it after reboot
 *  . . . or crash.
 */
bool save_server_usage ()
{
	FILE *fp = NULL;
	int i;
	int size = 0;

	if ((fp = dfopen(ETC_PATH, TMP_FILE, "w")) == NULL)
		return FALSE;

	fprintf(fp, "#ServerUsageTimestamp %s\n", strtime(current_time));
	fprintf(fp, "LastUpdate %ld\n\n", current_time);

	fprintf(fp, "Usage ");
	for (i = 0; i < USAGE_RECORDED_DAYS
	&& server_usage[i] != USAGE_UNDEFINED; i++) {
		fprintf(fp, " %d", server_usage[i]);
	}
	fprintf(fp, "~\n\n");

	size = fprintf(fp, END_TAG);
	fclose(fp);

        if (size != strlen(END_TAG))
	                return FALSE;

	d2rename(ETC_PATH, TMP_FILE, ETC_PATH, USAGE_STATE);
	return TRUE;
}

bool load_server_usage ()
{
	FILE    *fp    = NULL;
	char    *word  = "#END";
	bool    fMatch = FALSE;
	int	sanity = 0;

	Line_Number = 0;

        if (!dfexist(ETC_PATH, USAGE_STATE)
	||  (fp = dfopen(ETC_PATH, USAGE_STATE, "r")) == NULL)
		return FALSE;

	while (sanity++ < 1000) {
		word = feof(fp) ? "#END" : fread_word(fp);
		fMatch = FALSE;

		switch (UPPER(word[0])) {
		case '#':
			
			if (!str_cmp(word, "#END")) {
				fclose(fp);
				return TRUE;
			}
			else {
				fMatch = TRUE;
				fread_to_eol(fp);
			}
			break;
		case 'L':
			KEY("LastUpdate", server_last_usage_update, fread_number(fp));
			break;

		case 'U':
			if (!str_cmp(word, "Usage")) {
				const char *s = fread_string(fp);
				parse_usage(server_usage, s);
				free_string(s);
				fMatch = TRUE;
			}
			break;
		}
		if (!fMatch) {
			LOG("load_waffs: '%s' no match (%dth byte?) line %d",
				word, ftell(fp), Line_Number);
			fread_to_eol(fp);
		}
	}
	fclose(fp);
	return FALSE;
}
