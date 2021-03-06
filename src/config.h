/*-
 * Copyright (c) 2007 Zsuzsu <little_zsuzsu@hotmail.com>
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
 * $Id: usage.h 851 2006-04-22 13:34:00Z zsuzsu $
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#define CONFIG_NONE				0
#define CONFIG_MELEE_BASE_DAMAGE		1
#define CONFIG_WHO_SHOW_HIDDEN			2
#define CONFIG_WHO_HIDE_VAMP_UGLY		3
#define CONFIG_WHO_SHOW_VAMP_UGLY		4
#define CONFIG_CHAR_CREATE_ETHOS		5
#define CONFIG_CHAR_CREATE_STATIC_ALIGNMENT	6
#define CONFIG_CHAR_CREATE_ALLOW_NEWBIES	7
#define CONFIG_CHAR_CREATE_ALLOW_TRUE_LIFERS	8
#define CONFIG_CHAR_CREATE_ONLY_TRUE_LIFERS	9
#define CONFIG_CHAR_CREATE_INITIAL_HP		10
#define CONFIG_CHAR_CREATE_INITIAL_MANA		11
#define CONFIG_CHAR_CREATE_INITIAL_MOVE		12
#define CONFIG_CHAR_CREATE_CLASS_STAT_BONUS	13
#define CONFIG_STAT_TRAIN_RATE			14
#define CONFIG_STAT_RANDOM_VARIANCE		15
#define CONFIG_STAT_BASE			16
#define CONFIG_GUILD_RAID_MIN_LEVEL		17
#define CONFIG_PICK_RELIGION_TO_COMMUNE		18
#define CONFIG_WIZNET_ECONOMY_DEPOSIT_SILVER_MIN	19
#define CONFIG_WIZNET_ECONOMY_DEPOSIT_GOLD_MIN	20
#define CONFIG_CLANS_CAN_BUY_PETS		21
#define CONFIG_SHOP_MONEY_SINK_MOD		22
#define CONFIG_DIMINISH_MONEY_LOOT_RATE		23

extern flag_t global_configs[];

/* options in the code */
#define MELEE_BASE_DAMAGE_MODIFIER	(global_configs[CONFIG_MELEE_BASE_DAMAGE].settable)

#define WHO_SHOW_HIDDEN			(global_configs[CONFIG_WHO_SHOW_HIDDEN].settable)
#define WHO_HIDE_VAMP_UGLY		(global_configs[CONFIG_WHO_HIDE_VAMP_UGLY].settable)
#define WHO_SHOW_VAMP_UGLY		(global_configs[CONFIG_WHO_SHOW_VAMP_UGLY].settable)


#define CHAR_CREATE_ETHOS		(global_configs[CONFIG_CHAR_CREATE_ETHOS].settable)
#define CHAR_CREATE_STATIC_ALIGNMENT	(global_configs[CONFIG_CHAR_CREATE_STATIC_ALIGNMENT].settable)
#define CHAR_CREATE_ALLOW_NEWBIES	(global_configs[CONFIG_CHAR_CREATE_ALLOW_NEWBIES].settable)
#define CHAR_CREATE_ALLOW_TRUE_LIFERS	(global_configs[CONFIG_CHAR_CREATE_ALLOW_TRUE_LIFERS].settable)
#define CHAR_CREATE_ONLY_TRUE_LIFERS	(global_configs[CONFIG_CHAR_CREATE_ONLY_TRUE_LIFERS].settable)
#define CHAR_CREATE_INITIAL_HP		(global_configs[CONFIG_CHAR_CREATE_INITIAL_HP].settable)
#define CHAR_CREATE_INITIAL_MANA	(global_configs[CONFIG_CHAR_CREATE_INITIAL_MANA].settable)
#define CHAR_CREATE_INITIAL_MOVE	(global_configs[CONFIG_CHAR_CREATE_INITIAL_MOVE].settable)
#define CHAR_CREATE_CLASS_STAT_BONUS	(global_configs[CONFIG_CHAR_CREATE_CLASS_STAT_BONUS].settable)

#define STAT_TRAIN_RATE			(global_configs[CONFIG_STAT_TRAIN_RATE].settable)
#define STAT_RANDOM_VARIANCE		(global_configs[CONFIG_STAT_RANDOM_VARIANCE].settable)
#define STAT_BASE			(global_configs[CONFIG_STAT_BASE].settable)

#define GUILD_RAID_MIN_LEVEL		(global_configs[CONFIG_GUILD_RAID_MIN_LEVEL].settable)
#define PICK_RELIGION_TO_COMMUNE	(global_configs[CONFIG_PICK_RELIGION_TO_COMMUNE].settable)

#define WIZNET_ECONOMY_DEPOSIT_SILVER_MIN (global_configs[CONFIG_WIZNET_ECONOMY_DEPOSIT_SILVER_MIN].settable)
#define WIZNET_ECONOMY_DEPOSIT_GOLD_MIN	(global_configs[CONFIG_WIZNET_ECONOMY_DEPOSIT_GOLD_MIN].settable)
#define CLANS_CAN_BUY_PETS		(global_configs[CONFIG_CLANS_CAN_BUY_PETS].settable)

#define SHOP_MONEY_SINK_MOD		(global_configs[CONFIG_SHOP_MONEY_SINK_MOD].settable)
#define DIMINISH_MONEY_LOOT_RATE	(global_configs[CONFIG_DIMINISH_MONEY_LOOT_RATE].settable)

int  global_melee_base_damage_modifier();
void do_gconfig	(CHAR_DATA *ch, const char *argument);
void do_save_world_config(CHAR_DATA *ch, const char *argument);
bool save_world_config();
bool load_world_config();

#endif
