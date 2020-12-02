/*-
 * Copyright (c) 2006 Zsuzsu <little_zsuzsu@hotmail.com>
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
 * $Id: $
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "const.h"
#include "db/db.h"

flag_t global_configs[] =
{
	{ "",	TABLE_INTVAL,		},
	{ "melee_base_damage",			
		CONFIG_MELEE_BASE_DAMAGE,		100	},

	{ "who_show_hidden",
		CONFIG_WHO_SHOW_HIDDEN,			0	},
	{ "who_hide_vamp_ugly",	
		CONFIG_WHO_HIDE_VAMP_UGLY,		1	},
	{ "who_show_vamp_ugly",
		CONFIG_WHO_SHOW_VAMP_UGLY,		0	},

	{ "char_create_ethos",
		CONFIG_CHAR_CREATE_ETHOS,		0	},
	{ "char_create_static_alignment",
		CONFIG_CHAR_CREATE_STATIC_ALIGNMENT,	1	},
	{ "char_create_allow_newbies",
		CONFIG_CHAR_CREATE_ALLOW_NEWBIES,	1	},
	{ "char_create_allow_true_lifers",
		CONFIG_CHAR_CREATE_ALLOW_TRUE_LIFERS,	1	},
	{ "char_create_only_true_lifers",
		CONFIG_CHAR_CREATE_ONLY_TRUE_LIFERS,	0	},
	{ "char_create_initial_mp",
		CONFIG_CHAR_CREATE_INITIAL_HP,		20	},
	{ "char_create_initial_mana",
		CONFIG_CHAR_CREATE_INITIAL_MANA,	50	},
	{ "char_create_initial_move",
		CONFIG_CHAR_CREATE_INITIAL_MOVE,	100	},
	{ "char_create_class_stat_bonus",
		CONFIG_CHAR_CREATE_CLASS_STAT_BONUS,	0	},

	{ "stat_train_rate",
		CONFIG_STAT_TRAIN_RATE,			0	},
	{ "stat_random_variance",
		CONFIG_STAT_RANDOM_VARIANCE,		15	},
	{ "stat_base",
		CONFIG_STAT_BASE,			100	},

	{ "guild_raid_min_level",
		CONFIG_GUILD_RAID_MIN_LEVEL,		20	},

	{ "pick_religion_to_commune",
		CONFIG_PICK_RELIGION_TO_COMMUNE,	0	},

	{ "wiznet_economy_deposit_silver_min",
		CONFIG_WIZNET_ECONOMY_DEPOSIT_SILVER_MIN, 500	},
	{ "wiznet_economy_deposit_gold_min",
		CONFIG_WIZNET_ECONOMY_DEPOSIT_GOLD_MIN,	1	},

	{ "clans_can_buy_pets",
		CONFIG_CLANS_CAN_BUY_PETS,		1	},

	{ "shop_money_sink_mod",
		CONFIG_SHOP_MONEY_SINK_MOD,		25	},
	{ "diminish_money_loot_rate",
		CONFIG_DIMINISH_MONEY_LOOT_RATE,	5	},

	{ NULL }
};

int global_melee_base_damage_modifier()
{
	return global_configs[CONFIG_MELEE_BASE_DAMAGE].settable;
}

void do_gconfig (CHAR_DATA *ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	BUFFER *output = NULL;
	int flag = 0;
	int value = 0, was_value = 0;
	flag_t *pflag = NULL;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));

	if (arg1[0] == '\0') {
		output = buf_new(-1);
		buf_add(output, "{yGlobal Configuration Settings "
		"{D-------------------------------------------\n");
		for (flag = 0; global_configs[flag].name; flag++) {
			if (global_configs[flag].name[0] != '\0') 
				buf_printf(output, "\t{C%5d\t{c%s\n", 
					global_configs[flag].settable,
					global_configs[flag].name);
		}
		buf_add(output, "{x\n");
		page_to_char(buf_string(output), ch);
		buf_free(output);
		return;
	}

	pflag = global_configs;
	while (pflag->name != NULL) {
		if (!str_prefix(arg1, pflag->name))
			break;
		pflag++;
	}

	if (!pflag) {
		char_printf(ch, "couldn't find a matching flag\n");
		return;
	}

	if (arg2[0] == '\0') {
		char_printf(ch, "config: '{c%s{x' is {c%d{x\n",
			pflag->name,
			pflag->settable);
		return;
	}

	was_value = pflag->settable;

	if (!str_prefix(arg2, "true"))
		value = 1;
	else if (!str_prefix(arg2, "false"))
		value = 0;
	else
		value = atoi(arg2);
	pflag->settable = value;

	char_printf(ch, "config: '{c%s{x' was {c%d{x now {C%d{x\n",
		pflag->name,
		was_value,
		pflag->settable);
}

void do_load_world_config (CHAR_DATA *ch, const char *argument)
{
	if (!load_world_config())
		char_printf(ch, "Couldn't load new config.\n");
	else
		char_printf(ch, "New config loaded.\n");
}

void do_save_world_config (CHAR_DATA *ch, const char *argument)
{
	if (!save_world_config())
		char_printf(ch, "You failed to save the world, for some reason.\n");
	else
		char_printf(ch, "Save the cheerleader, save the world.\n");
}

/*
 * Saves the state of the world, including waffs and global configurations.
 *
 * rvalue - FALSE something bad happened and it didn't save.
 */
#define END_TAG	"#END\n"
bool save_world_config ()
{
	FILE	*fp = NULL;
	int	flag = 0;
	int	size = 0;

	if ((fp = dfopen(ETC_PATH, TMP_FILE, "w")) == NULL)
		return FALSE;

	fprintf(fp, "#CONFIG\n");
	for (flag = 0; global_configs[flag].name; flag++) {
		if (global_configs[flag].name[0] != '\0') 
			fprintf(fp, "%s %d\n", 
				global_configs[flag].name,
				global_configs[flag].settable);
	}
	size = fprintf(fp, END_TAG);
	fclose(fp);

	if (size != strlen(END_TAG))
		return FALSE;

	d2rename(ETC_PATH, TMP_FILE, ETC_PATH, WORLD_CONF);
	return TRUE;
}

/*
 * load the configuration file
 */
bool load_world_config ()
{
	FILE	*fp = NULL;
	char	*word = "End";
	bool	fMatch = FALSE;
	flag_t  *pflag = NULL;
	bool	end	= FALSE;

	if (!dfexist(ETC_PATH, WORLD_CONF)
	||  (fp = dfopen(ETC_PATH, WORLD_CONF, "r")) == NULL)
		return FALSE;

	while (!end) {
		if ((end = feof(fp)))
			continue;

		word = fread_word(fp);
		fMatch = FALSE;

		switch (UPPER(word[0])) {
		case '#':
		case '*':
			fMatch = TRUE;
			fread_to_eol(fp);
			break;

		default:
			pflag = global_configs;
			while (pflag->name != NULL) {
				if (!str_cmp(word, pflag->name))
					break;
				pflag++;
			}
			if (pflag) {
				fMatch = TRUE;
				LOG("word: %s", word);
				pflag->settable = fread_number(fp);
				LOG("config: '%s' = %d",
					pflag->name,
					pflag->settable);
			}
			break;
		}

		if (!fMatch) {
			log_printf("load_world_config: %s: no match (%dth byte?)",
				word, ftell(fp));
			fread_to_eol(fp);
		}
	}

	fclose(fp);
	return TRUE;
}
