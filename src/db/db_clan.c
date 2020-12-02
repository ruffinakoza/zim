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
 * $Id: db_clan.c 984 2006-12-22 17:41:32Z zsuzsu $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "merc.h"
#include "db.h"

DECLARE_DBLOAD_FUN(load_clan);

DBFUN dbfun_clans[] =
{
	{ "CLAN",	load_clan	},
	{ NULL }
};

DBDATA db_clans = { dbfun_clans };

DECLARE_DBLOAD_FUN(load_plists);

DBFUN dbfun_plists[] =
{
	{ "PLISTS",	load_plists	},
	{ NULL }
};

DBDATA db_plists = { dbfun_plists };

DBLOAD_FUN(load_clan)
{
	clan_t *clan;

	clan = clan_new();
	clan->file_name = get_filename(filename);
	db_set_arg(&db_plists, "PLISTS", clan);

	for (;;) {
		char *word = feof(fp) ? "End" : fread_word(fp);
		bool fMatch = FALSE;

		switch (UPPER(word[0])) {
		case '#':
			fread_string(fp); /* eat the string */
			fMatch = TRUE;
			break;
		case 'A':
			KEY("Altar", clan->altar_vnum, fread_number(fp));
			break;
		case 'D':
			KEY("Donate", clan->donate_vnum, fread_number(fp));
			break;
		case 'E':
			if (!str_cmp(word, "End")) {
				if (IS_NULLSTR(clan->name)) {
					db_error("load_clan",
						 "clan name not defined");
					clan_free(clan);
					clans.nused--;
				}
				varr_qsort(&clan->skills, cmpint);
				if (dfexist(PLISTS_PATH, clan->file_name))
					db_load_file(&db_plists,
						     PLISTS_PATH,
						     clan->file_name);
				return;
			}
			break;
		case 'F':
			KEY("Flags", clan->flags,
			    fread_fstring(clan_flags, fp));
			break;
		case 'I':
			KEY("Item", clan->obj_vnum, fread_number(fp));
			break;
		case 'M':
			KEY("Mark", clan->mark_vnum, fread_number(fp));
			break;
		case 'N':
			SKEY("Name", clan->name);
			break;
		case 'R':
			KEY("Recall", clan->recall_vnum, fread_number(fp));
			break;
		case 'S':
			if (!str_cmp(word, "Skill")) {
				clskill_t *clsk = varr_enew(&clan->skills);
				clsk->sn = sn_lookup(fread_word(fp));	
				clsk->level = fread_number(fp);
				clsk->percent = fread_number(fp);
				fMatch = TRUE;
			}
		case 'T':
                        if (!str_cmp(word, "Title")) {
                                int rank;
                                int sex;

                                rank = fread_number(fp);
                                if (rank < 0 || rank > MAX_CLAN_RANK) {
                                        db_error("load_clan",
                                                 "invalid rank %d", rank);
                                        continue;
                                }
                                sex = fread_fword(sex_table, fp);
                                if (sex != SEX_MALE && sex != SEX_FEMALE) {
                                        db_error("load_clan", "invalid sex");
                                        continue;
                                }
                                clan->rank_table[rank].title[sex] = fread_string(fp);
                                fMatch = TRUE;
                        }
                        break;
		}

		if (!fMatch) 
			db_error("load_clan", "%s: Unknown keyword", word);
	}
}

DBLOAD_FUN(load_plists)
{
	clan_t *clan = arg;

	for (;;) {
		char *word = feof(fp) ? "End" : fread_word(fp);
		bool fMatch = FALSE;

		switch (UPPER(word[0])) {

/* 
 * Needs to be kept in sync with olc/olc_save.c 
 * Zz
 */

		case 'A':
			SKEY("Ambassadors", clan->member_list[CLAN_AMBASSADOR]);
			break;
		case 'C':
			SKEY("Captains", clan->member_list[CLAN_CAPTAIN]);
			break;
		case 'E':
			SKEY("Elders", clan->member_list[CLAN_ELDER]);
			if (!str_cmp(word, "End"))
				return;
			break;
		case 'L':
			SKEY("Leaders", clan->member_list[CLAN_LEADER]);
			SKEY("Lords", clan->member_list[CLAN_LORD]);
			SKEY("Lieutenants", clan->member_list[CLAN_LIEUTENANT]);
			break;
		case 'M':
			SKEY("Magistrates", clan->member_list[CLAN_MAGISTRATE]);
			break;
		case 'O':
			SKEY("Outcasts", clan->member_list[CLAN_OUTCAST]);
			break;
		case 'R':
			SKEY("Recruits", clan->member_list[CLAN_RECRUIT]);
			break;
		case 'S':
			SKEY("Soldiers", clan->member_list[CLAN_SOLDIER]);
			break;
		case 'P':
			SKEY("Patrons", clan->member_list[CLAN_PATRON]);
			SKEY("Probations", clan->member_list[CLAN_PROBATION]);
			break;
		case 'V':
			SKEY("Veterans", clan->member_list[CLAN_VETERAN]);
			SKEY("Vassals", clan->member_list[CLAN_VASSAL]);
			break;
		}

		if (!fMatch) 
			db_error("load_plists", "%s: Unknown keyword", word);
	}
}

