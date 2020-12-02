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
 * $Id: usage.h 851 2006-04-22 13:34:00Z zsuzsu $
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#define DEBUG_ALL_ON			0
#define DEBUG_ALL_OFF			1
#define DEBUG_BUG			2
#define DEBUG_STARTUP			3
#define DEBUG_STARTUP_PFILES		4
#define DEBUG_FILES			5
#define DEBUG_CHAR_CREATE		6
#define DEBUG_CHAR_ROLLS		7

#define DEBUG_CLAN			8
#define DEBUG_CLAN_ITEM			9
#define DEBUG_CLAN_RAID			10

#define DEBUG_BUILD_AUTO		11
#define DEBUG_MOB_AI			12

#define DEBUG_QUEST			13
#define DEBUG_EXP			14
#define DEBUG_GOLD			15
#define DEBUG_LIMITED			16
#define DEBUG_CHANNELS			17

#define DEBUG_PITS			18
#define DEBUG_RECLAIM			19
#define DEBUG_OBJ_STRIP			20

#define DEBUG_DAM			21
#define DEBUG_DAM_BREATH		22
#define DEBUG_DAM_ONEHIT		23
#define DEBUG_EVADE			24
#define DEBUG_SAVES			25
#define DEBUG_VAMPIRIC			26
#define DEBUG_DAM_AC_REDUCT		27
#define DEBUG_MELEE			28

#define DEBUG_ALIGN_STANDING		50

#define DEBUG_INVIS			60
#define DEBUG_HONOR			61
#define DEBUG_HEAL			62

#define DEBUG_NONE1			69

#define DEBUG_AUGMENT			70
#define DEBUG_AUGMENT_1			71
#define DEBUG_AUGMENT_2			72
#define DEBUG_AUGMENT_3			73
#define DEBUG_AUGMENT_4			74
#define DEBUG_AUGMENT_5			75

#define DEBUG_SKILL_STEAL		80
#define DEBUG_SKILL_DRINKING		81
#define DEBUG_SKILL_VTOUCH		82
#define DEBUG_SKILL_SHOOT		83
#define DEBUG_SKILL_STALKER		84
#define DEBUG_SKILL_LASH		85
#define DEBUG_SKILL_MORTAL_STRIKE	86
#define DEBUG_SKILL_BASH		87
#define DEBUG_SKILL_TRIP		88
#define DEBUG_SKILL_THROW		89
#define DEBUG_SKILL_SPELLBANE		90
#define DEBUG_SKILL_GUILE		91
#define DEBUG_SKILL_LOCKPICK		92

#define DEBUG_MAGIC_CAST_LEVEL		100
#define DEBUG_MAX			101

extern bool active_debugs[DEBUG_MAX];

#define DEBUG	 debug_printf
void debug_printf(int type, const char *format, ...);
void debug_init ();

#endif
