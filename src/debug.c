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
 * $Id: debug.c 1013 2007-02-07 01:24:35Z zsuzsu $
 */

/*
 * These are a collection of various utilities used in the past for
 * debugging specific problems. (since gdb output is sometime a
 * bit unmanageable.
 *
 * by Zsuzsu
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "debug.h"
#include "flag.h"

void debug_descriptor_list (CHAR_DATA *ch, const char *argument);
void debug_list (CHAR_DATA *ch);
void debug_toggle (CHAR_DATA *ch, const char *argument);
void debug_linkdead_chars (CHAR_DATA *ch, const char *argument);

bool active_debugs[DEBUG_MAX];

flag_t debug_types[] =
{
	{ "",				TABLE_INTVAL,			},
	{ "all_on",			DEBUG_ALL_ON,			TRUE},
	{ "all_off",			DEBUG_ALL_OFF,			TRUE},
	{ "bug",			DEBUG_BUG,			TRUE},
	{ "startup",			DEBUG_STARTUP,			FALSE},
	{ "startup_pfiles",		DEBUG_STARTUP_PFILES,		FALSE},
	{ "files",			DEBUG_FILES,			TRUE},
	{ "creation",			DEBUG_CHAR_CREATE,		TRUE},
	{ "rolls",			DEBUG_CHAR_ROLLS,		FALSE},

	{ "clan",			DEBUG_CLAN,			TRUE},
	{ "clan_item",			DEBUG_CLAN_ITEM,		FALSE}, 
	{ "clan_raid",			DEBUG_CLAN_RAID,		FALSE}, 

	{ "builder",			DEBUG_BUILD_AUTO,		TRUE},
	{ "mob",			DEBUG_MOB_AI,			TRUE},

	{ "quest",			DEBUG_QUEST,			TRUE},
	{ "exp",			DEBUG_EXP,			TRUE},
	{ "gold",			DEBUG_GOLD,			TRUE},
	{ "limited",			DEBUG_LIMITED,			TRUE},
	{ "channels",			DEBUG_CHANNELS,			FALSE},
	{ "pits",			DEBUG_PITS,			FALSE},
	{ "reclaim",			DEBUG_RECLAIM,			FALSE},
	{ "obj_strip",			DEBUG_OBJ_STRIP,		FALSE},

	{ "damage",			DEBUG_DAM,			TRUE},
	{ "melee",			DEBUG_MELEE,			TRUE},
	{ "damage_ac",			DEBUG_DAM_AC_REDUCT,		FALSE},
	{ "damage_breath",		DEBUG_DAM_BREATH,		FALSE},
	{ "damage_onehit",		DEBUG_DAM_ONEHIT,		FALSE},
	{ "evade",			DEBUG_EVADE,			FALSE},
	{ "saves",			DEBUG_SAVES,			FALSE},
	{ "vampiric",			DEBUG_VAMPIRIC,			FALSE},

	{ "align_standing",		DEBUG_ALIGN_STANDING,		FALSE},

	{ "invis",			DEBUG_INVIS,			FALSE},
	{ "honor",			DEBUG_HONOR,			FALSE},
	{ "heal",			DEBUG_HEAL,			FALSE},

	{ "augment",			DEBUG_AUGMENT,			TRUE},
	{ "augment1",			DEBUG_AUGMENT_1,		FALSE},
	{ "augment2",			DEBUG_AUGMENT_2,		FALSE},
	{ "augment3",			DEBUG_AUGMENT_3,		FALSE},
	{ "augment4",			DEBUG_AUGMENT_4,		FALSE},
	{ "augment5",			DEBUG_AUGMENT_5,		FALSE},

	{ "steal",			DEBUG_SKILL_STEAL,		TRUE},
	{ "guile",			DEBUG_SKILL_GUILE,		TRUE},
	{ "drinking",			DEBUG_SKILL_DRINKING,		FALSE},
	{ "vtouch",			DEBUG_SKILL_VTOUCH,		FALSE},
	{ "shoot",			DEBUG_SKILL_SHOOT,		FALSE},
	{ "stalker",			DEBUG_SKILL_STALKER,		TRUE},
	{ "lash",			DEBUG_SKILL_LASH,		FALSE},
	{ "mortal_strike",		DEBUG_SKILL_MORTAL_STRIKE,	FALSE},
	{ "bash",			DEBUG_SKILL_BASH,		FALSE},
	{ "trip",			DEBUG_SKILL_TRIP,		FALSE},
	{ "throw",			DEBUG_SKILL_THROW,		FALSE},
	{ "spellbane",			DEBUG_SKILL_SPELLBANE,		FALSE},
	{ "lockpick",			DEBUG_SKILL_LOCKPICK,		FALSE},
	{ "magic_castlevel",		DEBUG_MAGIC_CAST_LEVEL,		TRUE},
	{ NULL }
};

void debug_init ()
{
	int i;

	for (i = 0; i < DEBUG_MAX; i++) {
		active_debugs[i] = flag_settable(debug_types, i);
	}
}

void do_debug (CHAR_DATA *ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg1, sizeof(arg1));

	if (arg1[0] == '\0')  {
		char_puts("debug commands:", ch);
		char_puts("   list toggle descriptor_list", ch);
	}

	if (!str_prefix(arg1, "list"))
		debug_list(ch);
	else if (!str_prefix(arg1, "toggle"))
		debug_toggle(ch, argument);
	else if (!str_prefix(arg1, "descriptor_list"))
		debug_descriptor_list(ch, argument);
	else if (!str_prefix(arg1, "linkdead"))
		debug_linkdead_chars(ch,argument);
}

void debug_list (CHAR_DATA *ch)
{
	BUFFER *output;
	int i;

	output = buf_new(-1);
	for (i=0; i<DEBUG_MAX; i++) {
		if (str_cmp("unknown", flag_string(debug_types, i))) {
			buf_printf(output, "   %30s %s\n",
				flag_string(debug_types, i),
				active_debugs[i] ? "{GON{x" : "{DOFF{x");
		}
	}

	page_to_char(buf_string(output), ch);
	buf_free(output);
}

void debug_toggle (CHAR_DATA *ch, const char *argument)
{
	const flag_t *type;

	type = flag_lookup(debug_types, argument);
	if (!type) {
		BUG("bad debug toggle: %s", argument);
		return;
	}
	active_debugs[type->bit] = !active_debugs[type->bit];
	char_printf(ch, "{DDEBUG:{x %s now %s\n",
		argument,
		active_debugs[type->bit] ? "ON" : "OFF");
}

void debug_descriptor_list (CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	DESCRIPTOR_DATA *d;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0')  {
	}

	for (d = descriptor_list; d != NULL; d = d->next) {
		DEBUG(0, "con: %d name: %s host: %s",
			d->connected,
			(d->character != NULL) ? d->character->name : "null",
			IS_TRUSTED_IMP(ch) ? d->host : "");
	}
}

/*
 * list all player characters w/o descriptors
 */
void debug_linkdead_chars (CHAR_DATA *ch, const char *argument)
{
	CHAR_DATA *vch = NULL;
	int count = 0;

	char_printf(ch, "Characters w/o descriptors:\n");
	for (vch = char_list; vch && !IS_NPC(vch); vch = vch->next) {
		if (vch->desc == NULL)
			char_printf(ch, "%2d) %s\n", 
				++count, vch->name);
	}
	char_printf(ch, "Total: %d\n", count);
}

/*
 * Sends a String to WizNet Debug, and the logfile
 */
void debug_printf(int type, const char *format, ...)
{
#if defined(WIN32)
	FILE *logfile;
#endif
	time_t current_time;
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	va_list ap;

	if (!active_debugs[type]) return;

	va_start(ap, format);
	vsnprintf(buf2, sizeof(buf2), format, ap);
	va_end(ap);

	time(&current_time);
	snprintf(buf, sizeof(buf), "DEBUG: %s", buf2);
	fprintf(stderr, "%s :: %s\n", strtime(current_time), buf);
	snprintf(buf, sizeof(buf), "{w[{DDEBUG{w]{x %s", buf2);

	wiznet(buf, NULL, NULL, WIZ_DEBUG, 0, 0);

#if defined (WIN32)
	/* Also add to logfile */
	logfile = fopen("sog.log", "a+b");
	if (logfile) {
		fprintf(logfile, "%s :: %s\n", strtime(current_time), buf);
		fclose(logfile);
	}
#endif
}

