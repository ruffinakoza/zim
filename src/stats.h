/*-
 * Copyright (c) 2006 Zsuzsu <little_zsuzsu@hotmail.com>
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
 * $Id: waffects.h 917 2006-10-13 22:06:33Z zsuzsu $
 */

#ifndef _STATS_H_
#define _STATS_H_

int ch_damroll (CHAR_DATA *ch);
int ch_hitroll (CHAR_DATA *ch);
int ch_mana_cost_mod (CHAR_DATA *ch);
int ch_learn_rate (CHAR_DATA *ch);
int ch_practice_per_level (CHAR_DATA *ch);
int ch_max_carry_weight(CHAR_DATA *ch);
int get_stat_str_tohit_mod (CHAR_DATA *ch, OBJ_DATA *wield);
int get_stat_str_todam_mod (CHAR_DATA *ch);
int get_stat_str_max_carry_weight (CHAR_DATA *ch);
int get_stat_str_max_wield_weight (CHAR_DATA *ch, int hand);
bool get_stat_str_can_wield (CHAR_DATA *ch, OBJ_DATA *wield, int hand);
bool get_stat_str_can_wear (CHAR_DATA *ch, OBJ_DATA *obj);
int get_stat_int_learn_rate (CHAR_DATA *ch);
int get_stat_wis_practice_per_level (CHAR_DATA *ch);
int get_stat_wis_mana_cost_mod (CHAR_DATA *ch);
int get_stat_dex_defense (CHAR_DATA *ch);
int ch_stat_avg(CHAR_DATA *ch, int stat);
int get_stat_cha_cost_mod (CHAR_DATA *ch);
int get_stat_cha_sell_mod (CHAR_DATA *ch);

#endif
