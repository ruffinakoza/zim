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
 * $Id: usage.h 1019 2007-02-15 00:52:41Z zsuzsu $
 */

#ifndef _USAGE_H_
#define _USAGE_H_

#define USAGE_RECORDED_DAYS 90
#define USAGE_UNDEFINED -1

extern int server_usage[USAGE_RECORDED_DAYS];
extern time_t server_last_usage_update;

void do_usage (CHAR_DATA *ch, const char *argument);
void ch_update_usage (CHAR_PDATA* pch, bool is_online_time);
void server_update_usage (void);
void parse_usage (int *usage, const char *str);
int *get_usage (const char *name);
long usage_last_days (CHAR_PDATA *pch, int days);
int usage_allowed_limiteds (CHAR_PDATA *pch);
int allowed_limiteds (CHAR_PDATA *pch);
void char_show_usage (CHAR_DATA *ch, CHAR_PDATA *pvictim);
void char_show_usage_generic(CHAR_DATA *ch, int *usage, const char *name);
int repossess_limiteds (CHAR_DATA *ch, CHAR_PDATA *pvict);

bool save_server_usage (void);
void approximate_server_usage_from_pdata (void);
bool load_server_usage (void);

#endif
