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
 * $Id: channels.h 901 2006-09-29 23:30:26Z zsuzsu $
 */

/*
 * channels - a unified communication protocol to allow for easy
 * 	to add channels w/o a lot of code repetition.
 *
 * by Zsuzsu
 */

#ifndef _CHANNELS_H_
#define _CHANNELS_H_

/* global channels */
#define CHAN_DM_WIZ		(B)
#define CHAN_WIZ		(C)
#define CHAN_AUCTION		(D)
#define CHAN_GOSSIP		(E)
#define CHAN_NEWBIE		(F)
#define CHAN_MUSIC 		(G)
#define CHAN_CLAN 		(H)
#define CHAN_OOC 		(I)
#define CHAN_GLOBAL_TELL	(J)
#define CHAN_GLOBAL_EMOTE	(K)
#define CHAN_SHOUT		(L)
#define CHAN_QUEST		(M)
#define CHAN_PRAY		(N)
#define CHAN_YELL		(O)
#define CHAN_WIZHELP		(P)
#define CHAN_ADVICE		(Q)
#define CHAN_TRADE		(R)
#define CHAN_DEATH		(S)

#define CHAN_LOCALE_GLOBAL	(A)
#define CHAN_LOCALE_AREA	(B)
#define CHAN_LOCALE_ROOM	(C)
#define CHAN_LOCALE_ROOM_FLAG	(D)
#define CHAN_LOCALE_CLAN	(E)
#define CHAN_LOCALE_RELIGION	(F)

struct channel_t
{
	const char *	name;		/* name of the channel */
	flag64_t	flag;		/* CHAN_ bit flag*/
	const char *	prefix;		/* prefix before the act() */
	int		default_emote;	/* default verb for the channel */
	bool		default_emote_only;
	bool		quote;		/* show quotes around message */
	int		read_level;	/* required level to listen*/
	int		write_level;	/* required level to talk */
	bool		anonymous;	/* speakers name is not used */
	bool		knockedout_ok;	/* can use this channel if knocked out*/
	bool		garbled_ok;	/* can use the channel if garbled*/
	bool		deaf_ok;	/* okay to hear/speak even if deaf*/
	bool		clear_speech;	/* don't garble, or drunk mesg*/
	bool		imm_anon_color;	/* use anon colors for this channel*/
	bool		lag;		/* does this incure lag when talking?*/
	char		color;		/* color of the mesg*/
	bool		act_flags;	/* flags sent to act() for this */
	flag32_t	locale;		/* global, area, roomtype, room */
	flag32_t	room_flags;	/* for room_flag locales */
};

#define CHANNEL(chan)	(get_channel_type(chan))

#define CHAN_EMOTE_NONE		0
#define CHAN_EMOTE_CUSTOM	1	/* set by the used at the time */
#define CHAN_EMOTE_DEFAULT	2	/* default emote for channels*/
#define CHAN_EMOTE_COLON	3
#define CHAN_EMOTE_SAY		4
#define CHAN_EMOTE_GOSSIP	5
#define	CHAN_EMOTE_YELL		6
#define CHAN_EMOTE_SHOUT	7
#define CHAN_EMOTE_SING		8
#define CHAN_EMOTE_PRAY		9
#define CHAN_EMOTE_TRADE	10

struct channel_emote_t
{
	int		type;		/* index number */
	const char *	name;		/* lookup name */
	const char *	self;		/* what you see */
	const char *	other;		/* what others see */
};

extern flag_t		channel_flags[];
extern channel_t	channel_table[];
extern channel_emote_t	channel_emote_table[];

channel_t *	get_channel_type (flag64_t flag);
channel_t *	channel_lookup (const char *name);
int		channel_emote_lookup (const char *emote);
channel_emote_t *channel_emote (int idx);
void		public_channel(CHAR_DATA *ch, const char *argument, int chan_flag);
const char *	garble(CHAR_DATA *ch, const char *i);

#endif

