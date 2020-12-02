/*
 * $Id: channels.c 931 2006-11-08 02:24:28Z zsuzsu $
 */
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "merc.h"
#include "db/lang.h"

flag_t channel_flags[] =
{
	{ "",			TABLE_BITVAL			},

	{ "dmtalk",		CHAN_DM_WIZ,		TRUE	},
	{ "immtalk",		CHAN_WIZ,		TRUE	},
	{ "immhelp",		CHAN_WIZHELP,		TRUE	},
	{ "auction",		CHAN_AUCTION,		TRUE	},
	{ "advice",		CHAN_ADVICE,		TRUE	},
	{ "newbie",		CHAN_NEWBIE,		TRUE	},
	{ "gossip",		CHAN_GOSSIP,		TRUE	},
	{ "shout",		CHAN_SHOUT,		TRUE	},
	{ "pray",		CHAN_PRAY,		TRUE	},
	{ "clan",		CHAN_CLAN,		TRUE	},
	{ "tell",		CHAN_GLOBAL_TELL,	TRUE	},
	{ "yell",		CHAN_YELL,		TRUE	},
	{ "emote",		CHAN_GLOBAL_EMOTE,	TRUE	},
	{ "ooc",		CHAN_OOC,		TRUE	},
	{ "music",		CHAN_MUSIC,		TRUE	},
	{ "quest",		CHAN_QUEST,		TRUE	},
	{ "trade",		CHAN_TRADE,		TRUE	},
	{ "death",		CHAN_DEATH,		TRUE	},

	{ NULL }
};

/*
 * Unified channels
 * by Zsuzsu
 */
channel_t channel_table[] = 
{
	/*
	{ name,		chan_flag,	prefix,
		default_emote,	default_emote_only,	quotes,
		read,	write,	show name,
		K.O.,	garb,	deaf,	clear,	anon,	lag,
		color,
		act_flags,
		locale,
		room_flag,
	}
	*/
	{ "advice",	CHAN_ADVICE,	"{M[{Cadvice{M]{m: ",
		CHAN_EMOTE_NONE,	TRUE,	FALSE,
		0,	IM,	TRUE,
		TRUE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE,
		'C',
		ACT_TOBUF,
		CHAN_LOCALE_GLOBAL,
		NONE,
	},
	{ "yell",	CHAN_YELL,	"",
		CHAN_EMOTE_YELL,	TRUE,	TRUE,
		0,	0, 	FALSE,
		FALSE,	TRUE,	FALSE,	FALSE,	TRUE,	FALSE,
		'M',
		ACT_STRANS | ACT_NODEAF,
		CHAN_LOCALE_AREA,
		NONE,
	},
	{ "gossip",	CHAN_GOSSIP,	"",
		CHAN_EMOTE_GOSSIP,	FALSE,	TRUE,
		0,	0, 	FALSE,
		FALSE,	TRUE,	FALSE,	FALSE,	TRUE,	TRUE,
		'R',
		ACT_NOTWIT | ACT_STRANS | ACT_NODEAF,
		CHAN_LOCALE_GLOBAL,
		NONE,
	},
	{ "ooc",	CHAN_OOC,	"{b[{BOoC{b] ",
		CHAN_EMOTE_COLON,	TRUE,	FALSE,
		0,	0, 	FALSE,
		FALSE,	FALSE,	FALSE,	FALSE,	TRUE,	TRUE,
		'B',
		ACT_NOTWIT | ACT_NODEAF | ACT_REALNAME,
		CHAN_LOCALE_GLOBAL,
		NONE,
	},
	{ "newbie",	CHAN_NEWBIE,	"{m[{Mnewbie{m] ",
		CHAN_EMOTE_COLON,	TRUE,	FALSE,
		0,	0, 	FALSE,
		FALSE,	FALSE,	FALSE,	FALSE,	TRUE,	TRUE,
		'M',
		ACT_NOTWIT | ACT_NODEAF | ACT_REALNAME,
		CHAN_LOCALE_GLOBAL,
		NONE,
	},
	{ "clan",	CHAN_CLAN,	"{C[{c%s{C] ",
		CHAN_EMOTE_SAY,		FALSE,	TRUE,
		0,	0, 	FALSE,
		FALSE,	TRUE,	FALSE,	FALSE,	FALSE,	FALSE,
		'C',
		ACT_TOBUF | ACT_STRANS | ACT_NOTWIT | ACT_NODEAF | ACT_REALNAME,
		CHAN_LOCALE_CLAN,
		NONE,
	},
	{ "dmtalk",	CHAN_DM_WIZ,	"{R[{rdm{R]{x ",
		CHAN_EMOTE_COLON,	TRUE,	FALSE,
		L1,	L1, 	FALSE,
		TRUE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE,
		'r',
		ACT_TOBUF,
		CHAN_LOCALE_GLOBAL,
		NONE,
	},
	{ "immtalk",	CHAN_WIZ,	"{y[{Yimm{y]{x ",
		CHAN_EMOTE_COLON,	TRUE,	FALSE,
		IM,	IM, 	FALSE,
		TRUE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE,
		'Y',
		ACT_TOBUF,
		CHAN_LOCALE_GLOBAL,
		NONE,
	},
	{ "immhelp",	CHAN_WIZHELP,	"{g[{Gimmhelp{g] ",
		CHAN_EMOTE_COLON,	TRUE,	FALSE,
		IM,	0, 	FALSE,
		TRUE,	TRUE,	TRUE,	TRUE,	TRUE,	TRUE,
		'G',
		ACT_TOBUF | ACT_REALNAME,
		CHAN_LOCALE_GLOBAL,
		NONE,
	},
	{ "pray",	CHAN_PRAY,	"{M[{m%s{M] ",
		CHAN_EMOTE_PRAY,	FALSE,	TRUE,
		0,	0, 	FALSE,
		FALSE,	TRUE,	FALSE,	FALSE,	FALSE,	FALSE,
		'm',
		ACT_TOBUF | ACT_STRANS | ACT_NOTWIT | ACT_NODEAF,
		CHAN_LOCALE_RELIGION,
		NONE,
	},
	{ "quest",	CHAN_QUEST,	"{C[{MQ{muest{C]{M:{x ",
		CHAN_EMOTE_COLON,	TRUE,	FALSE,
		0,	0,	TRUE,
		FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE,
		'c',
		ACT_TOBUF,
		CHAN_LOCALE_GLOBAL,
		NONE,
	},
	{ "shout",	CHAN_SHOUT,	"",
		CHAN_EMOTE_SHOUT,	TRUE,	TRUE,
		0,	0, 	FALSE,
		FALSE,	TRUE,	FALSE,	FALSE,	TRUE,	TRUE,
		'Y',
		ACT_NOTWIT | ACT_STRANS | ACT_NODEAF,
		CHAN_LOCALE_GLOBAL,
		NONE,
	},
	{ "music",	CHAN_MUSIC,	"{w[{Wmusic{w] ",
		CHAN_EMOTE_SING,	TRUE,	TRUE,
		0,	IM, 	FALSE,
		FALSE,	TRUE,	FALSE,	FALSE,	TRUE,	TRUE,
		'W',
		ACT_NOTWIT | ACT_STRANS | ACT_NODEAF,
		CHAN_LOCALE_GLOBAL,
		NONE,
	},
	{ "trade",	CHAN_TRADE,	"{Y[{ytrade{Y] ",
		CHAN_EMOTE_TRADE,	FALSE,	TRUE,
		0,	0, 	FALSE,
		FALSE,	TRUE,	FALSE,	FALSE,	TRUE,	TRUE,
		'y',
		ACT_NOTWIT | ACT_STRANS | ACT_NODEAF,
		CHAN_LOCALE_GLOBAL,
		NONE,
	},

	{ "death",	CHAN_DEATH,	"{x[{DDeath{x]:{x ",
		CHAN_EMOTE_COLON,	TRUE,	FALSE,
		0,	0,	TRUE,
		FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE,
		'r',
		ACT_TOBUF,
		CHAN_LOCALE_GLOBAL,
		NONE,
	},

	{ NULL }
};

channel_t * get_channel_type (flag64_t flag) 
{
	channel_t *ct;

	for (ct = channel_table; ct->name; ct++)
		if (ct->flag == flag)
			return ct;
	return NULL;
}

channel_t *channel_lookup (const char *name) 
{
	channel_t *ct;

	for (ct = channel_table; ct->name; ct++)
		if (!str_prefix(name, ct->name))
			return ct;

	return NULL;
}

channel_emote_t channel_emote_table[] =
{
	{ CHAN_EMOTE_NONE,	NULL,
		"",		"",		},
	{ CHAN_EMOTE_DEFAULT,	NULL,
		"say",		"says", 	},
	{ CHAN_EMOTE_COLON,	NULL,
		":",		":",		},
	{ CHAN_EMOTE_SAY,	"say",
		"say",		"says",	 	},
	{ CHAN_EMOTE_GOSSIP,	"gossip",
		"gossip",	"gossips",	},
	{ CHAN_EMOTE_YELL,	"yell",
		"yell",		"yells",	},
	{ CHAN_EMOTE_SHOUT,	"shout",
		"shout",	"shouts",	},
	{ CHAN_EMOTE_SING,	"sing",
		"sing",		"sings",	},
	{ CHAN_EMOTE_PRAY,	"pray",
		"pray",		"prays",	},
	{ CHAN_EMOTE_TRADE,	"offer",
		"offer",	"offers",	},
	{ -1 }
};

int channel_emote_lookup (const char *emote) 
{
	int i;

	for (i=0; channel_emote_table[i].type != -1; i++) {
		if (channel_emote_table[i].name == NULL)
			continue;
		if (!str_prefix(emote, channel_emote_table[i].name))
			return channel_emote_table[i].type;
	}

	return CHAN_EMOTE_DEFAULT;
}

channel_emote_t *channel_emote (int idx)
{
	int i = 0;
	for (i=0; channel_emote_table[i].type != -1; i++)
		if (idx == channel_emote_table[i].type)
			return &channel_emote_table[i];

	return NULL;
}

void display_channel_raw (CHAR_DATA *ch, int chan_flag)
{
	channel_t *channel = CHANNEL(chan_flag);

	if (!channel) return;

	if (channel->read_level > ch->level
	&& channel->write_level > ch->level)
		return;

	if (IS_IMMORTAL(ch)) 
		char_printf(ch, "{%c%12s %-5s     {x%2d{D/{x%2d %-10s{x\n",
			!IS_SET(global_channels, chan_flag) ? 'D' : 'x',
			flag_string(channel_flags, chan_flag),
			IS_SET(ch->channels, chan_flag) ? "{GON" : "{ROFF",
			channel->read_level,
			channel->write_level,
			!IS_SET(global_channels, chan_flag) ? "{BDISABLED" 
			: IS_SET(ch->restricted_channels, chan_flag) ?  "{rFORBIDDEN" 
			: "");
	else
		char_printf(ch, "{%c%12s %-5s    %-10s{x\n",
			!IS_SET(global_channels, chan_flag) ? 'D' : 'x',
			flag_string(channel_flags, chan_flag),
			IS_SET(ch->channels, chan_flag) ? "{GON" : "{ROFF",
			!IS_SET(global_channels, chan_flag) ? "{BDISABLED" 
			: IS_SET(ch->restricted_channels, chan_flag) ?  "{rFORBIDDEN" 
			: "");
}

/*
 * re-written by Zsuzsu
 */
void do_channels(CHAR_DATA *ch, const char *argument)
{
	int flag = 0;

	/* lists all channels and their status */
	if (IS_IMMORTAL(ch)) {
		char_puts("     channel status   r{D/{xw\n",ch);
		char_puts("    {D-------- ------- -----{x\n",ch);
	}
	else {
		char_puts("     channel status\n",ch);
		char_puts("    {D-------- -------{x\n",ch);
	}

	for (flag =1; channel_flags[flag].name; flag++) {
		display_channel_raw(ch, channel_flags[flag].bit);
	}

	if (IS_SET(ch->comm, COMM_QUIET))
		 char_puts("Quiet mode has been set.\n", ch);

	if (IS_SET(ch->comm, COMM_SNOOP_PROOF))
		char_puts("You are immune to snooping.\n", ch);
	
	char_printf(ch, "You display %d lines of scroll.\n", ch->lines+2);

}

/*
 * Unified public channels
 * by Zsuzsu
 */
void public_channel (CHAR_DATA *ch, const char *argument, int chan_flag)
{
	DESCRIPTOR_DATA *d;
	channel_t *channel = NULL;
	char buf[MAX_INPUT_LENGTH];
	char prefix[30];
	char mesg[MAX_INPUT_LENGTH];
	char *adverb = NULL;
	char *ptr = NULL;
	int  emote = CHAN_EMOTE_DEFAULT;
	char *emote_self = NULL;
	char *emote_other = NULL;
	char delim = '^';

	if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM))
		return;

	if (IS_NPC(ch) && IS_SET(ch->pIndexData->act, ACT_MUTE))
		return;

	if (argument[0] == '\0') {
		TOGGLE_BIT(ch->channels, chan_flag);
		char_printf(ch, "Your {W%s{x channel is now %s{x\n",
			flag_string(channel_flags, chan_flag),
			IS_SET(ch->channels, chan_flag) ? "{GON" : "{ROFF");
		return;
	}

	if (IS_SET(ch->restricted_channels, chan_flag)) {
		char_puts("{RThe gods have revoked your privilege to that channel.{x\n", 
			ch);
		return;
	}

	/* turn the channel on if they don't have it on already */
	if (!IS_SET(ch->channels, chan_flag))
		public_channel(ch, str_empty, chan_flag);

	channel = CHANNEL(chan_flag);

	if (channel == NULL) {
		bug("public_channel: illegal channel flag (%d)",
			chan_flag);
		char_puts("Sorry, not sure what channel that is.\n", ch);
		return;
	}

	if (!channel->knockedout_ok 
	&& IS_AFFECTED(ch, AFF_SLEEP)) {
		char_puts("In your dreams?\n", ch);
		return;
	}

	/* figure out if there is a custom emote with adverbs
	 * in the form of: command adverb "message
	 */
	if (!channel->default_emote_only
	&& !channel->anonymous
	&& ((ptr = strchr(argument, delim)) != NULL)) {
		*ptr = '\0';
		adverb = (char *) argument;
		strncpy(mesg, ++ptr, sizeof(mesg));
		chomp(adverb);
		chomp(mesg);

		if (*adverb == '\0')
			adverb = NULL;
	}
	else {
		strncpy(mesg, argument, sizeof(mesg));
	}

	emote = channel->default_emote;

	if (channel->lag)
		WAIT_STATE(ch, PULSE_VIOLENCE);

	if (!IS_SET(global_channels, chan_flag) && !IS_IMMORTAL(ch)) {
		if (chan_flag == CHAN_OOC)
			char_puts("Perhaps you could find a way to say"
				" that in character.\n", ch);
		else 
			char_puts("This channel is not currently available.\n", 
				ch);
		return;
	}

	if (ch->level < channel->write_level) {
		if (chan_flag == CHAN_OOC)
			char_puts("Perhaps you could find a way to say"
				" that in character.\n", ch);
		else 
			char_printf(ch, "Communication on that channel is currently"
				" reserved for people level %d and higher.\n", 
				channel->write_level);
		return;
	}

	if (is_affected(ch, gsn_garble)) {
		if (!channel->garbled_ok) {
			char_puts("You probably wouldn't be understood anyway.\n",ch);
			return;
		}
		if (!channel->clear_speech) {
			strncpy(mesg, garble(ch, mesg), sizeof(mesg));
			adverb = NULL;
		}
	}

	emote_self = (char *) channel_emote(emote)->self;
	emote_other = (char *) channel_emote(emote)->other;

	if (channel->anonymous)
		snprintf(buf, sizeof(buf), 
			"%s{%c$t",
			channel->prefix, 
			channel->color);

	/* if the prefix has a %s (assume 's')
	 * then do something special for certain cases
	 */
	if (strchr(channel->prefix, '%') != NULL) {
		switch (channel->flag) {
			case CHAN_PRAY:
				/*specifically for our atheist clan*/
				if (ch->religion == RELIGION_ATHEIST)
					snprintf(prefix, sizeof(prefix),
						channel->prefix,
						"ElidoDi");
				else
					snprintf(prefix, sizeof(prefix),
						channel->prefix,
						religion_table[ch->religion].leader);
				break;
			case CHAN_CLAN:
				snprintf(prefix, sizeof(prefix),
					channel->prefix,
					clan_name(ch->clan));
				break;
			default:
				BUG("public_channel: unknown"
					" prefix '%%' in chan %s",
					channel->name);
				strncpy(prefix, channel->prefix, sizeof(prefix));
		}
	}
	else {
		strncpy(prefix, channel->prefix, sizeof(prefix));
	}

	/* anonymous color 
	 * so morts can keep track of how many wizi immorts
	 * are talking
	 */
	if (channel->anonymous) {
		snprintf(buf, sizeof(buf), 
			"%s{%c$t{x",
			prefix, 
			channel->color
			);
	}
	else if (channel->imm_anon_color
	&& IS_IMMORTAL(ch)
	&& (ch->invis_level > 0 || ch->incog_level > 0))
		snprintf(buf, sizeof(buf), 
			"%s{%cYou{x%s%s%s%s%s{%c$t{x%s",
			prefix, 
			ch->pcdata->anon_color
				? ch->pcdata->anon_color
				: 'x',
			emote != CHAN_EMOTE_COLON ? " " : "",
			emote_self,
			adverb ? " " : "",
			adverb ? adverb : "",
			channel->quote ? ", '" : " ",
			channel->color,
			channel->quote ? "'" : ""
			);
	else
		snprintf(buf, sizeof(buf), "%s{%cYou{x%s%s%s%s%s{%c$t{x%s",
			prefix,
			(IS_IMMORTAL(ch) && chan_flag != CHAN_WIZ 
			 && chan_flag != CHAN_DM_WIZ)
			? 'W' : 'x',
			emote != CHAN_EMOTE_COLON ? " " : "",
			emote_self,
			adverb ? " " : "",
			adverb ? adverb : "",
			channel->quote ? ", '" : " ",
			channel->color,
			channel->quote ? "'" : ""
			);

	/*
	act_puts(buf, ch, mesg, NULL,
		 TO_CHAR | ACT_NODEAF, POS_DEAD);
	 */
	act_puts(buf, ch, mesg, NULL,
		 TO_CHAR | channel->act_flags, POS_DEAD);

	if (channel->anonymous) { }

	else if (channel->imm_anon_color
	&& IS_IMMORTAL(ch) 
	&& (ch->invis_level > 0 || ch->incog_level > 0))
		snprintf(buf, sizeof(buf), 
			"%s{%c$n{x%s%s%s%s%s{%c$t{x%s",
			prefix, 
			ch->pcdata->anon_color
				? ch->pcdata->anon_color
				: 'x',
			emote != CHAN_EMOTE_COLON ? " " : "",
			emote_other,
			adverb ? " " : "",
			adverb ? adverb : "",
			channel->quote ? ", '" : " ",
			channel->color,
			channel->quote ? "'" : ""
			);
	else
		snprintf(buf, sizeof(buf), 
			"%s{%c$n{x%s%s%s%s%s{%c$t{x%s",
			prefix, 
			(IS_IMMORTAL(ch) && chan_flag != CHAN_WIZ 
			 && chan_flag != CHAN_DM_WIZ)
			? 'W' : 'x',
			emote != CHAN_EMOTE_COLON ? " " : "",
			emote_other,
			adverb ? " " : "",
			adverb ? adverb : "",
			channel->quote ? ", '" : " ",
			channel->color,
			channel->quote ? "'" : ""
			);

	/* now, send it to everyone on the channel */
	for (d = descriptor_list; d; d = d->next) {
                if (d->connected == CON_PLAYING
                &&  d->character != ch
                &&  IS_SET(d->character->channels, chan_flag)
		&&  d->character->level >= channel->read_level) {

			/* character must be in the same room */
			if (IS_SET(channel->locale, CHAN_LOCALE_ROOM)
			&& ch->in_room != d->character->in_room)
				continue;

			/* character must be in the same area */
			if (IS_SET(channel->locale, CHAN_LOCALE_AREA)
			&& (!d->character->in_room
			|| ch->in_room->area != d->character->in_room->area))
				continue;

			/* character must be in rooms with a certain flag */
			if (IS_SET(channel->locale, CHAN_LOCALE_ROOM_FLAG)
			&& (!d->character->in_room
			|| !IS_SET(d->character->in_room->room_flags, 
			channel->room_flags)))
				continue;

			/* character must believe in the same god */
			if (IS_SET(channel->locale, CHAN_LOCALE_RELIGION)
			&& (ch->religion != d->character->religion
			&& !(IS_IMMORTAL(d->character)
			&& IS_SET(d->character->pcdata->wiznet, WIZ_PRAY_CHAN)
			&& d->character->level == ML
			&& d->character->race == RACE_GNOME)))
				continue;

			/* character must be in the same clan */
			if (IS_SET(channel->locale, CHAN_LOCALE_CLAN)
			&& (ch->clan != d->character->clan
			&& !(IS_IMMORTAL(d->character)
			&& IS_SET(d->character->pcdata->wiznet, WIZ_CLAN_CHAN)
			&& d->character->level == ML
			&& d->character->race == RACE_GNOME)))
				continue;

                        act_puts(buf,
                                 ch, mesg, d->character,
				 TO_VICT | channel->act_flags,
                                 POS_DEAD);
		}
	}
}  

/*
 * from Anatolia MUD
 */
const char *garble(CHAR_DATA *ch, const char *i)
{
	static char not_garbled[] = "?!()[]{},.:;'\" ";
	static char buf[MAX_STRING_LENGTH];
	char *o;

	if (!is_affected(ch, gsn_garble))
		return i;

	for (o = buf; *i && o-buf < sizeof(buf)-1; i++, o++) {
		if (strchr(not_garbled, *i))
			*o = *i;
		else
			*o = number_range(' ', 254);
	}
	*o = '\0';
	return buf;
}
