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
 * $Id: waffects.c 917 2006-10-13 22:06:33Z zsuzsu $
 */

#include <sys/types.h>
#       include <sys/socket.h>
#       include <netinet/in.h>
#       include <arpa/telnet.h>
#       include <arpa/inet.h>
#       include <unistd.h>
#       include <netdb.h>
#       include <sys/wait.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(SUNOS) || defined(SVR4) || defined(LINUX)
#       include <crypt.h>
#endif


#include "merc.h"
#include "debug.h"
#include "comm.h"
#include "comm_info.h"
#include "comm_colors.h"
#include "db/cmd.h"
#include "interp.h"
#include "quest.h"
#include "update.h"
#include "ban.h"
#include "charset.h"
#include "resolver.h"
#include "olc/olc.h"
#include "db/lang.h"
#include "pdata.h"

void close_descriptor(DESCRIPTOR_DATA *dclose);
void advance(CHAR_DATA *victim, int level);
bool    check_reconnect         (DESCRIPTOR_DATA *d, const char *name,
                                 bool fConn);
bool    check_playing           (DESCRIPTOR_DATA *d, const char *name);

/*static void cp_print(DESCRIPTOR_DATA* d);
int con_get_codepage (DESCRIPTOR_DATA * d, const char *argument);
*/

void comm_login (DESCRIPTOR_DATA * d, const char *argument);
int con_get_name (DESCRIPTOR_DATA * d, const char *argument);
int con_resolv (CHAR_DATA *ch, const char *argument);
int con_break_connect (CHAR_DATA *ch, const char *argument);
int con_confirm_new_name (CHAR_DATA *ch, const char *argument);
int con_get_new_password (CHAR_DATA *ch, const char *argument);
int con_confirm_new_password (CHAR_DATA *ch, const char *argument);
int con_ansi_detector (CHAR_DATA *ch, const char *argument);
int con_newbie_detector (CHAR_DATA *ch, const char *argument);
int con_newbie_protection (CHAR_DATA *ch, const char *argument);
int con_true_lifer (CHAR_DATA *ch, const char *argument);
int con_get_new_race (CHAR_DATA *ch, const char *argument);
int con_get_new_sex (CHAR_DATA *ch, const char *argument);
int con_get_new_class (CHAR_DATA *ch, const char *argument);
int con_roll_stats100 (CHAR_DATA *ch, const char *argument);
int con_roll_stats25 (CHAR_DATA *ch, const char *argument);
int con_get_alignment (CHAR_DATA *ch, const char *argument);
int con_get_ethos (CHAR_DATA *ch, const char *argument);
int con_pick_hometown (CHAR_DATA *ch, const char *argument);
int con_create_done (CHAR_DATA *ch, const char *argument);
int con_get_old_password (CHAR_DATA *ch, const char *argument);
int con_read_imotd (CHAR_DATA *ch, const char *argument);
int con_read_motd (CHAR_DATA *ch, const char *argument);
static void print_hometown(CHAR_DATA *ch);
bool class_ok(CHAR_DATA *ch, int class);
flag32_t align_restrict(CHAR_DATA *ch);
flag32_t ethos_restrict(CHAR_DATA *ch);

char    echo_off_str    [] = { IAC, WILL, TELOPT_ECHO, '\0' };
char    echo_on_str     [] = { IAC, WONT, TELOPT_ECHO, '\0' };

/**
 * if the descriptor isn't "PLAYING" process input from the 
 * descriptor in the following manner.
 */
void comm_login (DESCRIPTOR_DATA * d, const char *argument)
{
	CHAR_DATA *ch;
	int loop = FALSE;

	while (isspace(*argument))
		argument++;

	ch = d->character;

	do {
		switch (d->connected) {
		default:
			BUG("comm_login: bad d->connected %d.", d->connected);
			close_descriptor(d);
			return;

/*
		case CON_GET_CODEPAGE:
			loop = con_get_codepage(d, argument);
			break;
*/

		case CON_GET_NAME:
			loop = con_get_name(d, argument);
			break;

		case CON_RESOLV:
			loop = con_resolv(ch, argument);
			break;

		case CON_BREAK_CONNECT:
			loop = con_break_connect(ch, argument);
			break;

		case CON_CONFIRM_NEW_NAME:
			loop = con_confirm_new_name(ch, argument);
			break;

		case CON_GET_NEW_PASSWORD:
			loop = con_get_new_password(ch, argument);
			break;

		case CON_CONFIRM_NEW_PASSWORD:
			loop = con_confirm_new_password(ch, argument);
			break;

		case CON_ANSI_DETECTOR:
			loop = con_ansi_detector(ch, argument);
			break;

		case CON_NEWBIE_DETECTOR:
			loop = con_newbie_detector(ch, argument);
			break;

		case CON_NEWBIE_PROTECTION:
			loop = con_newbie_protection(ch, argument);
			break;

		case CON_TRUE_LIFER:
			loop = con_true_lifer(ch, argument);
			break;

		case CON_GET_NEW_RACE:
			loop = con_get_new_race(ch, argument);
			break;

		case CON_GET_NEW_SEX:
			loop = con_get_new_sex(ch, argument);
			break;

		case CON_GET_NEW_CLASS:
			loop = con_get_new_class(ch, argument);
			break;

		case CON_ROLL_STATS:
			loop = con_roll_stats100(ch, argument);
			break;

		case CON_GET_ALIGNMENT:
			loop = con_get_alignment(ch, argument);
			break;

		case CON_GET_ETHOS:
			loop = con_get_ethos(ch, argument);
			break;

		case CON_PICK_HOMETOWN:
			loop = con_pick_hometown(ch, argument);
			break;

		case CON_CREATE_DONE:
			loop = con_create_done(ch, argument);
			break;

		case CON_GET_OLD_PASSWORD:
			loop = con_get_old_password(ch, argument);
			break;

		case CON_READ_IMOTD:
			loop = con_read_imotd(ch, argument);
			break;

		case CON_READ_MOTD:
			loop = con_read_motd(ch, argument);
			break;
		}
		while (argument[0] != '\0' && argument++);
	} while (loop);
}

/*
int con_get_codepage (DESCRIPTOR_DATA *d, const char *argument)
{
	int num;

	if (argument[0] == '\0') {
		close_descriptor(d);
		return FALSE;
	}

	if (argument[1] != '\0'
	||  (num = argument[0] - '1') < 0
	||  num >= NCODEPAGES) {
		cp_print(d);
		return FALSE;
	}

	d->codepage = codepages+num;
	LOG("'%s' codepage selected", d->codepage->name);
	d->connected = CON_GET_NAME;

	return TRUE;
}

static void cp_print(DESCRIPTOR_DATA *d)
{
	char buf[MAX_STRING_LENGTH];
	int i;

	write_to_buffer(d, "\n\r", 0);
	for (i = 0; i < NCODEPAGES; i++) {
		snprintf(buf, sizeof(buf), "%s%d. %s",
			 i ? " " : "", i+1, codepages[i].name);
		write_to_buffer(d, buf, 0);
	}

	write_to_buffer(d, 
	"\n\rPlease enter the name you wish to be called by in this realm: ", 0);
} 
*/

/* get name ******************************************************/
int con_get_name (DESCRIPTOR_DATA *d, const char *argument)
{
	CHAR_DATA *ch;
	struct sockaddr_in sock;
	socklen_t size;

	if (argument == NULL) {
		write_to_buffer(d, 
			"By which name do you wish to be known? ", 
			0);
		return FALSE;
	}

	if (argument[0] == '\0') {
		close_descriptor(d);
		return FALSE;
	}

	if (!pc_name_ok(argument)) {
		write_to_buffer(d, "Illegal name, try another.\n\r"
				"Name: ", 0);
		return FALSE;
	}

	if (load_char_obj(d, argument))
		ch = d->character;
	else {
		ch = d->character = new_char_obj(argument);
		ch->desc = d;
		ch->acct_flags |= ACCT_NEW;
	}

	if (d->host == NULL) {
		size = sizeof(sock);
		if (getpeername(d->descriptor,
				(struct sockaddr *) &sock, &size) < 0)
			d->host = str_dup("(unknown)");
		else {
			fprintf(rfout, "%s@%s\n",
				ch->name, inet_ntoa(sock.sin_addr));
			d->connected = CON_RESOLV;
/* wait until sock.sin_addr gets resolved */
			return FALSE;
		}
	}

	return FALSE;
}

/* resolv ********************************************************/
int con_resolv (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;
	char buf[MAX_STRING_LENGTH];

/*
	while (isspace(*argument))
		argument++;
*/

	if (d->host == NULL)
		return TRUE;

	/*
	 * Swiftest: I added the following to ban sites.  I don't
	 * endorse banning of sites, but Copper has few descriptors now
	 * and some people from certain sites keep abusing access by
	 * using automated 'autodialers' and leaving connections hanging.
	 *
	 * Furey: added suffix check by request of Nickel of HiddenWorlds.
	 */
	if (check_ban(d->host, BAN_ALL)) {
		write_to_buffer(d,
				"Your site has been banned from this mud.\n\r",
				0);
		close_descriptor(d);
		return FALSE;
	}

	if (!IS_IMMORTAL(ch)) {
		if (check_ban(d->host, BAN_PLAYER)) {
			write_to_buffer(d,
					"Your site has been banned for players.\n\r",
					0);
			close_descriptor(d);
			return FALSE;
		}
#undef NO_PLAYING_TWICE
#ifdef NO_PLAYING_TWICE
		if (search_sockets(d)) {
			write_to_buffer(d,
					"Playing twice is restricted...\n\r",
					0);
			close_descriptor(d);
			return FALSE;
		}
#endif
		if (iNumPlayers > MAX_OLDIES
		    && !IS_SET(ch->acct_flags, ACCT_NEW)) {
			snprintf(buf, sizeof(buf),
				 "\nThere are currently %i players mudding out of a maximum of %i.\n"
				 "Please try again soon.\n",
				 iNumPlayers - 1, MAX_OLDIES);
			write_to_buffer(d, buf, 0);
			close_descriptor(d);
			return FALSE;
		}
		if (iNumPlayers > MAX_NEWBIES
		    && IS_SET(ch->acct_flags, ACCT_NEW)) {
			snprintf(buf, sizeof(buf),
				 "\nThere are currently %i players mudding.\n"
				 "New player creation is limited to when there are less than %i players.\n"
				 "Please try again soon.\n",
				 iNumPlayers - 1, MAX_NEWBIES);
			write_to_buffer(d, buf, 0);
			close_descriptor(d);
			return FALSE;
		}
	}

	if (IS_SET(ch->state_flags, STATE_DENY)) {
		log_printf("Denying access to %s@%s.", argument,
			   d->host);
		write_to_buffer(d, "You are denied access.\n\r", 0);
		close_descriptor(d);
		return FALSE;
	}

	if (check_reconnect(d, argument, FALSE))
		REMOVE_BIT(ch->acct_flags, ACCT_NEW);
	else if (wizlock && !IS_HERO(ch)) {
		write_to_buffer(d, "The game is wizlocked.\n\r", 0);
		close_descriptor(d);
		return FALSE;
	}

	if (!IS_SET(ch->acct_flags, ACCT_NEW)) {
		/* Old player */
		d->connected = CON_GET_OLD_PASSWORD;
		return TRUE;
	} else {
		/* New player */
		if (newlock) {
			write_to_buffer(d, "The game is newlocked.\n\r",
					0);
			close_descriptor(d);
			return FALSE;
		}

		if (check_ban(d->host, BAN_NEWBIES)) {
			write_to_buffer(d,
					"New players are not allowed from your site.\n\r",
					0);
			close_descriptor(d);
			return FALSE;
		}

		d->connected = CON_CONFIRM_NEW_NAME;
		return TRUE;
	}
}

/* break connection **********************************************/
int con_break_connect (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;
	DESCRIPTOR_DATA *d_old, *d_next;

	switch (*argument) {
	case 'y':
	case 'Y':
		for (d_old = descriptor_list; d_old; d_old = d_next) {
			CHAR_DATA *rch;

			d_next = d_old->next;
			if (d_old == d || d_old->character == NULL)
				continue;

			rch = d_old->original ? d_old->original :
			    d_old->character;
			if (str_cmp(ch->name, rch->name))
				continue;

			if (d_old->original)
				do_return(d_old->character, str_empty);
			close_descriptor(d_old);
		}

		if (check_reconnect(d, ch->name, TRUE))
			return FALSE;
		write_to_buffer(d, "Reconnect attempt failed.\n\r", 0);

		/* FALLTHRU */

	case 'n':
	case 'N':
		write_to_buffer(d, "Name: ", 0);
		if (d->character != NULL) {
			free_char(d->character);
			d->character = NULL;
		}
		d->connected = CON_GET_NAME;
		return TRUE;

	default:
		write_to_buffer(d, "Please type Y or N? ", 0);
	}
	return FALSE;
}

/* confirm new name **********************************************/
int con_confirm_new_name (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;

	switch (*argument) {
	case '\0':
		do_help(ch, "NAME");
		write_to_buffer(d,
				"Does your name fit the above criteria (Y/N)? ",
				0);
		return FALSE;
	case 'y':
	case 'Y':
		write_to_buffer(d, "New character.\n", 0);
		d->connected = CON_GET_NEW_PASSWORD;
		return TRUE;

	case 'n':
	case 'N':
		write_to_buffer(d, "Ok, what IS it, then? ", 0);
		free_char(d->character);
		d->character = NULL;
		d->connected = CON_GET_NAME;
		break;

	default:
		write_to_buffer(d, "Please type Yes or No? ", 0);
		break;
	}
	return FALSE;
}

/* get new passwd ************************************************/
int con_get_new_password (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;
	char buf[MAX_STRING_LENGTH];
	char *pwdnew;

#if defined(unix)
	write_to_buffer(d, "\n\r", 2);
#endif

	if (argument[0] == '\0') {
		snprintf(buf, sizeof(buf),
			 "Give me a password for %s: ", ch->name);
		write_to_buffer(d, buf, 0);
		write_to_descriptor(d->descriptor, echo_off_str, 0);
		return FALSE;
	}

	if (strlen(argument) < 5) {
		write_to_buffer(d,
				"Password must be at least five characters long.\n\rPassword: ",
				0);
		return FALSE;
	}

	pwdnew = crypt(argument, ch->name);
	free_string(ch->pcdata->pwd);
	ch->pcdata->pwd = str_dup(pwdnew);
	write_to_buffer(d, "Please retype password: ", 0);
	d->connected = CON_CONFIRM_NEW_PASSWORD;
	return FALSE;
}

/* confirm new passwd ********************************************/
int con_confirm_new_password (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;

	if (argument[0] == '\0') {
		write_to_buffer(d, "Please retype password: ", 0);
		return FALSE;
	}

#if defined(unix)
	write_to_buffer(d, "\n\r", 2);
#endif

	if (strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd)) {
		write_to_buffer(d, "Passwords don't match.\n\r"
				"Retype password: ", 0);
		d->connected = CON_GET_NEW_PASSWORD;
		return FALSE;
	}

	write_to_descriptor(d->descriptor, (char *) echo_on_str, 0);
	d->connected = CON_ANSI_DETECTOR;
	return TRUE;
}

/* ANSI **********************************************************/
int con_ansi_detector (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;

	switch (argument[0]) {
	default:
		write_to_buffer(d,
				"Does your client understand ANSI color(Y/N)? ",
				0);
		return FALSE;
	case 'y':
	case 'Y':
		SET_BIT(ch->comm, COMM_COLOR);
		if (CHAR_CREATE_ALLOW_NEWBIES)
			d->connected = CON_NEWBIE_DETECTOR;
		else if (CHAR_CREATE_ALLOW_TRUE_LIFERS)
			d->connected = CON_TRUE_LIFER;
		else
			d->connected = CON_GET_NEW_RACE;
		char_puts("{GG{YR{BE{MA{RT{W!{x\n", ch);
		break;
	case 'n':
	case 'N':
		if (CHAR_CREATE_ALLOW_NEWBIES)
			d->connected = CON_NEWBIE_DETECTOR;
		else if (CHAR_CREATE_ALLOW_TRUE_LIFERS)
			d->connected = CON_TRUE_LIFER;
		else
			d->connected = CON_GET_NEW_RACE;
		char_puts("Too bad.\n", ch);
		break;
	}
	return TRUE;
}

/* newbie ********************************************************/
int con_newbie_detector (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;

	switch (argument[0]) {
	case 'Y':
	case 'y':
		char_printf(ch, "\n{D-{w={W[{x Welcome to %s! {W]{w={D-{x\n"
		     "{Y*{x remember to ask questions on the '{Mnewbie{x' channel.\n",
		     GAME_NAME);
		d->connected = CON_NEWBIE_PROTECTION;
		SET_BIT(ch->acct_flags, ACCT_NEW);
		REMOVE_BIT(ch->channels, CHAN_OOC);

		return TRUE;
	case 'N':
	case 'n':
		REMOVE_BIT(ch->acct_flags, ACCT_NEW);
		d->connected = CON_TRUE_LIFER;
		return TRUE;

	default:
		char_printf(ch, "\n-=[ Is this your first time visiting %s{x? ]=-\n",
			GAME_NAME);
	}
	char_puts("\nAre you completely {Mnew{x to the realm? (Y/N): ", ch);
	return FALSE;
}

/* newbie protection *********************************************/
int con_newbie_protection (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;

	switch (argument[0]) {
	case 'H':
	case 'h':
	case '?':
		do_help(ch, "1.newbie protection");
		break;
	case 'Y':
	case 'y':
		char_puts("\n{YWise Choice{x!"
			"  You are now invulnerable to {rPlayerKilling{x.",
			ch);
		SET_BIT(ch->acct_flags, ACCT_NEWBIE);
		REMOVE_BIT(ch->channels, CHAN_OOC);
		d->connected = CON_GET_NEW_RACE;
		return TRUE;
	case 'N':
	case 'n':
		d->connected = CON_GET_NEW_RACE;
		return TRUE;

	default:
		char_puts("-=[ {MExplore {CMode{x? ]=-\n", ch);
		do_help(ch, "newbie protection intro");
	}
	char_puts("\nWould you like to be protected (Y/N/?): ",ch);
	return FALSE;
}

/* true lifer ****************************************************/
int con_true_lifer (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;

	switch (argument[0]) {
	case 'H':
	case 'h':
	case '?':
		do_help(ch, "true lifer");
		break;
	case 'Y':
	case 'y':
		char_puts("\n{YGood {gluck{x . . . you'll need it!", ch);
		SET_BIT(ch->acct_flags, ACCT_TRUE_LIFER);
		DEBUG(DEBUG_CHAR_CREATE,	
		    "new character: %s has chosen the path of a True Lifer",
		    ch->name);
		d->connected = CON_GET_NEW_RACE;
		return TRUE;
	case 'N':
	case 'n':
		if (CHAR_CREATE_ONLY_TRUE_LIFERS) {
			write_to_buffer(d,
				"\n\rSorry, mate, only True Lifers are allowed.", 0);
			char_puts("\n{YGood {gluck{x . . . you'll need it!", ch);
			SET_BIT(ch->acct_flags, ACCT_TRUE_LIFER);
			DEBUG(DEBUG_CHAR_CREATE,	
			    "new character: %s didn't choose, but is a True Lifer",
			    ch->name);

		}
		d->connected = CON_GET_NEW_RACE;
		return TRUE;

	default:
		char_puts("\n-=[ {YT{yrue {YL{yifer{x? ]=-\n", ch);
		do_help(ch, "true life intro");
	}
	write_to_buffer(d,
		"\n\rOne life, one death, that's the deal.  You in (Y/N/?): ",
			0);
	return FALSE;
}

/* race **********************************************************/
int con_get_new_race (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;
	char arg[MAX_INPUT_LENGTH];
	int race, i;
	race_t *r;

	one_argument(argument, arg, sizeof(arg));

	if (*arg == '\0') {
		write_to_buffer(d,
				"Legends & Lore is home for the following races:\n\r",
				0);
		do_help(ch, "RACETABLE");
		char_puts("('{chelp <race>{x' for more information)\n"
				"What is your race? ", ch);
		return FALSE;
	} else if (!str_cmp(arg, "help")) {
		argument = one_argument(argument, arg, sizeof(arg));
		if (argument[0] == '\0') {
			do_help(ch, "RACETABLE");
		} else {
			do_help(ch, argument);
		}
		char_puts("('{chelp <race>{x' for more information)\n"
			"What is your race? ", ch);
		return FALSE;
	}

	race = rn_lookup(argument);
	r = RACE(race);

	if (race < 1 || !r->pcdata) {
		write_to_buffer(d, "That is not a valid race.\n\r", 0);
		write_to_buffer(d,
				"The following races are available:\n\r  ",
				0);
		for (race = 1; race < races.nused; race++) {
			r = RACE(race);
			if (!r->pcdata)
				break;
			if (race == 8 || race == 14)
				write_to_buffer(d, "\n\r  ", 0);
			write_to_buffer(d, "(", 0);
			write_to_buffer(d, r->name, 0);
			write_to_buffer(d, ") ", 0);
		}
		write_to_buffer(d, "\n\r", 0);
		char_puts("('{chelp <race>{x' for more information)\n"
			"What is your race? ", ch);
		return FALSE;
	}

	SET_ORG_RACE(ch, race);
	ch->race = race;
	for (i = 0; i < MAX_STATS; i++)
		ch->mod_stat[i] = 0;

	/* Add race stat modifiers 
	   for (i = 0; i < MAX_STATS; i++)
	   ch->mod_stat[i] += r->pcdata->stats[i];      */

	/* Add race modifiers */
	ch->max_hit += r->pcdata->hp_bonus;
	ch->hit = ch->max_hit;
	ch->max_mana += r->pcdata->mana_bonus;
	ch->mana = ch->max_mana;
	ch->practice = r->pcdata->prac_bonus;

	ch->affected_by = ch->affected_by | r->aff;
	ch->imm_flags = ch->imm_flags | r->imm;
	ch->res_flags = ch->res_flags | r->res;
	ch->vuln_flags = ch->vuln_flags | r->vuln;
	ch->form = r->form;
	ch->parts = r->parts;

	/* add cost */
	ch->pcdata->points = r->pcdata->points;
	ch->size = r->pcdata->size;

	d->connected = CON_GET_NEW_SEX;
	return TRUE;
}

/* sex ***********************************************************/
int con_get_new_sex (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;

	switch (argument[0]) {
	case '\0':
		write_to_buffer(d, "\n", 0);
		write_to_buffer(d, "What is your sex (M/F)? ", 0);
		return FALSE;
	case 'm':
	case 'M':
		ch->sex = SEX_MALE;
		ch->pcdata->true_sex = SEX_MALE;
		ch->desc->connected = CON_GET_NEW_CLASS;
		break;
	case 'f':
	case 'F':
		ch->sex = SEX_FEMALE;
		ch->pcdata->true_sex = SEX_FEMALE;
		ch->desc->connected = CON_GET_NEW_CLASS;
		break;
	default:
		write_to_buffer(d,
				"That's not a sex.\n\rWhat IS your sex? ",
				0);
		return FALSE;
	}
	return TRUE;
}

/* class *********************************************************/
int con_get_new_class (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;
	char arg[MAX_INPUT_LENGTH];
	int iClass;
	int count = 0;

	one_argument(argument, arg, sizeof(arg));

	if (*arg == '\0') {
		count = 0;
		char_printf(ch, "\n{C%-12s %5s %3s %s\n",
			"CLASS",
			"PRIME",
			"PTS",
			"DESCRIPTION");
		char_printf(ch, "{c%-12s %5s %3s %s\n",
			"-----",
			"-----",
			"---",
			"-----------");
		for (iClass = 0; iClass < classes.nused; iClass++) {
			if (class_ok(ch, iClass)) {
				char_printf(ch, "{%c%-12s  %3s  %3d %s\n",
					class_ok(ch, iClass) ? 'x' : 'D',
					CLASS(iClass)->name,
					flag_string(stat_names, 
						CLASS(iClass)->attr_prime),
					CLASS(iClass)->points,
					CLASS(iClass)->description);
			}
		}
		for (iClass = 0; iClass < classes.nused; iClass++) {
			if (!class_ok(ch, iClass)) {
				char_printf(ch, "{%c%-12s  %3s  %3d %s\n",
					class_ok(ch, iClass) ? 'x' : 'D',
					CLASS(iClass)->name,
					flag_string(stat_names, 
						CLASS(iClass)->attr_prime),
					CLASS(iClass)->points,
					CLASS(iClass)->description);
			}
		}
		char_puts("\n{C* {Ddark{x classes not available to your race/sex.\n",
			ch);
		char_puts("\n('{chelp <class>{x' for more information)\n"
			"What is your class? ", ch);
		return FALSE;
	}

	iClass = cn_lookup(argument);

	if (!str_cmp(arg, "help")) {
		if (argument[0] == '\0')
			do_help(ch, "class help");
		else
			do_help(ch, argument);
		char_puts("('{chelp <class>{x' for more information)\n"
			"What is your class? ", ch);
		return FALSE;
	}

	if (iClass == -1) {
		write_to_buffer(d,
				"That's not a class.\n\rWhat IS your class? ",
				0);
		return FALSE;
	}

	if (iClass == CLASS_THIEF) {
		write_to_buffer(d,
				"That class is currently broken and not much fun to play.\n\rChoose again: ",
				0);
		return FALSE;
	}
	if (iClass == CLASS_ROGUE) {
		write_to_buffer(d,
				"That class is curently under development.\n\rChoose again: ",
				0);
		return FALSE;
	}

	if (!class_ok(ch, iClass)) {
		if (IS_NEWBIE(ch)) {
			write_to_buffer(d,
					"That class is not available for your race or sex"
					" or because it is advanced.\n\rChoose again: ",
					0);
		} else {
			write_to_buffer(d,
					"That class is not available for your race or sex.\n\r"
					"Choose again: ", 0);
		}
		return FALSE;
	}

	ch->class = iClass;
	ch->pcdata->points += CLASS(iClass)->points;
	act("You are now $t.", ch, CLASS(iClass)->name, NULL, TO_CHAR);
	ch->desc->connected = CON_ROLL_STATS;
	return TRUE;
}

/**
 * rolls stats randomly.
 * based on 100 max stat
 */
int con_roll_stats100 (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;
	const char *statfmt = 
			"Str:{c%s{x Int:{c%s{x Wis:{c%s{x"
			" Dex:{c%s{x  Con:{c%s{x  Cha:{c%s{x\n"
			"Accept (Y/N/?)? ";
	int i, j, k;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	one_argument(argument, arg1, sizeof(arg1));

	switch (arg1[0]) {
	case '\0':
		for (i = 0; i < MAX_STATS; i++) {
			ch->perm_stat[i] =
			    race_lookup(ch->pcdata->race)->pcdata->stats[i];
			if (CHAR_CREATE_CLASS_STAT_BONUS)
				ch->perm_stat[i] += CLASS(ch->class)->stats[i];
		}

		do_help(ch, "stats");
		char_puts("\nNow rolling your stats.\n", ch);
		char_puts("\nEveryone starts with the same stat total, you're choosing the allocation.\n", ch);

		if (STAT_TRAIN_RATE > 0)
			char_puts("You don't get many trains, so choose well.\n",ch);
		else
			char_puts("{Y*{x After you are born, stats can only be augmented by {mmagic{x.\n",ch);

		char_printf(ch, statfmt,
			get_stat_alias(ch, STAT_STR),
			get_stat_alias(ch, STAT_INT),
			get_stat_alias(ch, STAT_WIS),
			get_stat_alias(ch, STAT_DEX),
			get_stat_alias(ch, STAT_CON),
			get_stat_alias(ch, STAT_CHA));

		DEBUG(DEBUG_CHAR_ROLLS,
		    "ROLL: %s (%s/%s): Str:%d  Int:%d  Wis:%d  Dex:%d  Con:%d  Cha:%d Lck:%d",
		     ch->name, 
		     race_name(ch->race),
		     class_name(ch),
		     ch->perm_stat[STAT_STR],
		     ch->perm_stat[STAT_INT], 
		     ch->perm_stat[STAT_WIS],
		     ch->perm_stat[STAT_DEX], 
		     ch->perm_stat[STAT_CON],
		     ch->perm_stat[STAT_CHA],
		     ch->perm_stat[STAT_LCK]);
		break;

	case 'H':
	case 'h':
	case '?':
		one_argument(argument, arg2, sizeof(arg2));
		if (arg2[0] != '\0') {
			do_help(ch, arg2);
		}
		else
			do_help(ch, "stats");
		break;

	case 'y':
	case 'Y':
		for (i = 0; i < MAX_STATS; i++)
			ch->mod_stat[i] = 0;
		DEBUG(DEBUG_CHAR_CREATE,
		    "newchar %s (%s/%s): Str:%d  Int:%d  Wis:%d  Dex:%d  Con:%d  Cha:%d Lck:%d",
		     ch->name, 
		     race_name(ch->race),
		     class_name(ch),
		     ch->perm_stat[STAT_STR],
		     ch->perm_stat[STAT_INT], 
		     ch->perm_stat[STAT_WIS],
		     ch->perm_stat[STAT_DEX], 
		     ch->perm_stat[STAT_CON],
		     ch->perm_stat[STAT_CHA],
		     ch->perm_stat[STAT_LCK]
		     );
		char_puts("\n", ch);
		d->connected = CON_GET_ALIGNMENT;
		return TRUE;

	case 'n':
	case 'N':
		for (i = 0; i < MAX_STATS; i++) {
			ch->perm_stat[i] =
			    race_lookup(ch->pcdata->race)->pcdata->
			    stats[i];
			if (CHAR_CREATE_CLASS_STAT_BONUS)
				ch->perm_stat[i] += CLASS(ch->class)->stats[i];
		}

		for (i = 0; i < 100; i++) {
			j = number_range(0, MAX_STATS - 1);
			k = number_range(0, MAX_STATS - 1);
			if ((ch->perm_stat[j] < (RACE(ch->race)->pcdata->stats[j] +STAT_RANDOM_VARIANCE))
			&& (ch->perm_stat[k] > (RACE(ch->race)->pcdata->stats[k] -STAT_RANDOM_VARIANCE))) {
				ch->perm_stat[j]++;
				ch->perm_stat[k]--;
			}
		}
		char_printf(ch, statfmt,
			 get_stat_alias(ch, STAT_STR),
			 get_stat_alias(ch, STAT_INT),
			 get_stat_alias(ch, STAT_WIS),
			 get_stat_alias(ch, STAT_DEX),
			 get_stat_alias(ch, STAT_CON),
			 get_stat_alias(ch, STAT_CHA));

		break;

	default:
		char_puts("Please answer (Y/N/?): ",ch);
	}

	DEBUG(DEBUG_CHAR_ROLLS,
	    "ROLL: %s (%s/%s): Str:%d  Int:%d  Wis:%d  Dex:%d  Con:%d  Cha:%d Lck:%d",
	     ch->name, 
	     race_name(ch->race),
	     class_name(ch),
	     ch->perm_stat[STAT_STR],
	     ch->perm_stat[STAT_INT], 
	     ch->perm_stat[STAT_WIS],
	     ch->perm_stat[STAT_DEX], 
	     ch->perm_stat[STAT_CON],
	     ch->perm_stat[STAT_CHA],
	     ch->perm_stat[STAT_LCK]);

	return FALSE;
}

/* roll random stats.  based on 25max.  14 base.
 */
int con_roll_stats25 (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;
	char buf[MAX_STRING_LENGTH];
	int i, j;

	switch (argument[0]) {
	case '\0':
		for (i = 0; i < MAX_STATS; i++) {
			ch->perm_stat[i] = 15;
			ch->perm_stat[i] +=
			    race_lookup(ch->pcdata->race)->pcdata->stats[i];
			ch->perm_stat[i] += CLASS(ch->class)->stats[i];
		}

		for (i = 0; i < 10; i++) {
			j = number_range(0, MAX_STATS - 1);
			ch->perm_stat[j] =
			    UMIN(ch->perm_stat[j] + 1, get_max_train(ch, j));

			j = number_range(0, MAX_STATS - 2);
			ch->perm_stat[j] = UMAX(3, ch->perm_stat[j] - 1);
		}

		snprintf(buf, sizeof(buf),
			 "Str:%s  Int:%s  Wis:%s  Dex:%s  Con:%s  Cha:%s\n\rAccept (Y/N)? ",
			 get_stat_alias(ch, STAT_STR),
			 get_stat_alias(ch, STAT_INT),
			 get_stat_alias(ch, STAT_WIS),
			 get_stat_alias(ch, STAT_DEX),
			 get_stat_alias(ch, STAT_CON),
			 get_stat_alias(ch, STAT_CHA));


		do_help(ch, "stats");
		write_to_buffer(d, "\n\rNow rolling for your stats.\n\r", 0);
		write_to_buffer(d,
				"\n\rEveryone starts with the same stat total, you're choosing the allocation.\n\r",
			0);
		write_to_buffer(d,
			"You don't get many trains, so choose well.\n\r",
			0);
		write_to_buffer(d, buf, 0);
		return FALSE;

	case 'H':
	case 'h':
	case '?':
		do_help(ch, "stats");
		break;
	case 'y':
	case 'Y':
		for (i = 0; i < MAX_STATS; i++)
			ch->mod_stat[i] = 0;
		DEBUG(DEBUG_CHAR_CREATE,
		    "newchar %s (%s/%s): Str:%d  Int:%d  Wis:%d  Dex:%d  Con:%d  Cha:%d",
		     ch->name, 
		     race_name(ch->race),
		     class_name(ch),
		     ch->perm_stat[STAT_STR],
		     ch->perm_stat[STAT_INT], ch->perm_stat[STAT_WIS],
		     ch->perm_stat[STAT_DEX], ch->perm_stat[STAT_CON],
		     ch->perm_stat[STAT_CHA]);
		write_to_buffer(d, "\n\r", 2);
		d->connected = CON_GET_ALIGNMENT;
		return TRUE;

	case 'n':
	case 'N':
		for (i = 0; i < MAX_STATS; i++) {
			ch->perm_stat[i] = 14;
			ch->perm_stat[i] +=
			    race_lookup(ch->pcdata->race)->pcdata->
			    stats[i];
			ch->perm_stat[i] += CLASS(ch->class)->stats[i];
		}

		for (i = 0; i < 10; i++) {
			j = number_range(0, MAX_STATS - 1);
			ch->perm_stat[j] =
			    UMIN(ch->perm_stat[j] + 1,
				 get_max_train(ch, j));
			j = number_range(0, MAX_STATS - 2);
			ch->perm_stat[j] =
			    UMAX(3, ch->perm_stat[j] - 1);
		}
		snprintf(buf, sizeof(buf),
			 "Str:%s  Int:%s  Wis:%s  Dex:%s  Con:%s  Cha:%s\n\rAccept (Y/N/?): ",
			 get_stat_alias(ch, STAT_STR),
			 get_stat_alias(ch, STAT_INT),
			 get_stat_alias(ch, STAT_WIS),
			 get_stat_alias(ch, STAT_DEX),
			 get_stat_alias(ch, STAT_CON),
			 get_stat_alias(ch, STAT_CHA));

		write_to_buffer(d, buf, 0);
		return FALSE;

	default:
		write_to_buffer(d, "Please answer (Y/N/?): ", 0);
		return FALSE;
	}
	return FALSE;
}

/* alignment *****************************************************/
int con_get_alignment (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;
	flag32_t allowed_align = align_restrict(ch);
	bool align_set = FALSE;

	if (allowed_align == RA_GOOD) {
		ch->alignment = 1000; 
		write_to_buffer(d, "You are compelled to follow the path of Good.\n\r",0);
		align_set = TRUE;
	}
	else if (allowed_align == RA_NEUTRAL) {
		ch->alignment = 0; 
		write_to_buffer(d, "You are compelled to follow the path of Neutrality.\n\r",0);
		align_set = TRUE;
	}
	else if (allowed_align == RA_EVIL) {
		ch->alignment = -1000; 
		write_to_buffer(d, "You are compelled to follow the path of Evil.\n\r",0);
		align_set = TRUE;
	}

	if (!align_set && argument != NULL) {
		switch(argument[0]) {
			case 'g' : case 'G' : 
				if (IS_SET(allowed_align, RA_GOOD)) {
					ch->alignment = 1000; 
					write_to_buffer(d, "Now your character is good.\n\r",0);
					align_set = TRUE;
				}
				else {
					write_to_buffer(d,"That's not a valid alignment.\n\r",0);
				}
				break;
			case 'n' : case 'N' : 
				if (IS_SET(allowed_align, RA_NEUTRAL)) {
					ch->alignment = 0; 
					write_to_buffer(d, "Now your character is neutral.\n\r",0);
					align_set = TRUE;
				}
				else {
					write_to_buffer(d,"That's not a valid alignment.\n\r",0);
				}
				break;
			case 'e' : case 'E' : 
				if (IS_SET(allowed_align, RA_EVIL)) {
					ch->alignment = -1000; 
					write_to_buffer(d, "Now your character is evil.\n\r",0);
					align_set = TRUE;
				}
				else {
					write_to_buffer(d,"That's not a valid alignment.\n\r",0);
				}
				break;
			case '?' : case 'h': case 'H':
				do_help(ch, "'alignment'");
				break;
		}
	}

	if (!align_set) {
		write_to_buffer(d, "\n\rChoose your alignment (", 0);
		if (IS_SET(allowed_align, RA_GOOD))
			write_to_buffer(d, "Good ", 0);
		if (IS_SET(allowed_align, RA_NEUTRAL))
			write_to_buffer(d, "Neutral ", 0);
		if (IS_SET(allowed_align, RA_EVIL))
			write_to_buffer(d, "Evil", 0);
		write_to_buffer(d, ")[?]: ", 0);
	}
	else {
		if (CHAR_CREATE_ETHOS)
			d->connected = CON_GET_ETHOS;
		else
			d->connected = CON_PICK_HOMETOWN;
		return TRUE;
	}
	return FALSE;
}

/* hometown ******************************************************/
int con_pick_hometown (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;
	int htn;

	if (IS_NEWBIE(ch)) {
		if (ch->class != CLASS_NECROMANCER
		    && ch->class != CLASS_VAMPIRE) {
			ch->hometown = htn_lookup("Midgaard");
			write_to_buffer(d,
					"Since you are new here, your hometown"
					" has been automagically set to Midgaard.\n\r",
					0);
		} else {
			ch->hometown =
			    htn_lookup("Old Midgaard");
			write_to_buffer(d,
					"Your hometown has been automagically set to "
					" Old Midgaard.\n\r",
					0);
		}
		d->connected = CON_CREATE_DONE;
		return TRUE;

	} else if (argument[0] == '\0'
		   || (htn = htn_lookup(argument)) < 0
		   || hometown_restrict(HOMETOWN(htn), ch)) {
		/*char_puts("That's not a valid hometown.\n", ch);*/
		print_hometown(ch);
		return FALSE;
	}

	ch->hometown = htn;
	char_printf(ch, "\nNow your hometown is %s.\n"
		    "[Hit Return to continue]\n",
		    hometown_name(htn));
	d->connected = CON_CREATE_DONE;
	return FALSE;
}

/* create done ***************************************************/
int con_create_done (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;

	if (IS_NEWBIE(ch)) {
		log_printf("%s@%s newbie player.", ch->name, d->host);
	} else
		log_printf("%s@%s new player.", ch->name, d->host);
	write_to_buffer(d, "\n\r", 2);
	do_help(ch, "motd");
	char_puts("[Press Enter to continue]", ch);
	ch->lines = PAGELEN;
	d->connected = CON_READ_MOTD;
	return FALSE;
}

/* old passwd ****************************************************/
int con_get_old_password (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;

	if (argument[0] == '\0') {
		write_to_descriptor(d->descriptor, echo_off_str, 0);
		write_to_buffer(d, "Password: ", 0);
		return FALSE;
	}

	write_to_buffer(d, "\n\r", 2);

	if (strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd)) {
		write_to_buffer(d, "Wrong password.\n\r", 0);
		LOG("Wrong password by %s@%s", ch->name, d->host);
		if (ch->endur == 2)
			close_descriptor(d);
		else {
			write_to_descriptor(d->descriptor,
					    (char *) echo_off_str, 0);
			write_to_buffer(d, "Password: ", 0);
			d->connected = CON_GET_OLD_PASSWORD;
			ch->endur++;
		}
		return FALSE;
	}

	if (ch->pcdata->pwd[0] == '\0') {
		write_to_buffer(d, "Warning! Null password!\n\r"
				"Type 'password null <new password>'"
				" to fix.\n\r", 0);
	}

	write_to_descriptor(d->descriptor, (char *) echo_on_str, 0);

	if (check_playing(d, ch->name)
	    || check_reconnect(d, ch->name, TRUE))
		return FALSE;

	log_printf("%s@%s has connected.", ch->name, d->host);
	d->connected = CON_READ_IMOTD;

	/* FALL THRU */
	return TRUE;
}

/* imotd *********************************************************/
int con_read_imotd (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;

	write_to_buffer(d, "\n\r", 2);

	if (IS_SET(ch->acct_flags, ACCT_NEWBIE))
		do_help(ch, "1.newbie");
	else
		do_help(ch, "motd");

	write_to_buffer(d, "\n\r", 2);

	if (IS_IMMORTAL(ch))
		do_help(ch, "imotd");

	d->connected = CON_READ_MOTD;
	return TRUE;
}

/* motd **********************************************************/
int con_read_motd (CHAR_DATA *ch, const char *argument)
{
	DESCRIPTOR_DATA *d = ch->desc;
	char buf[MAX_STRING_LENGTH];
	int nextquest = 0;
	int i = 0;

	ch->desc->connected = CON_PLAYING;
	update_skills(ch);
	char_printf(ch, "\nWelcome to %s. Enjoy . . .\n", GAME_NAME);
	ch->next = char_list;
	char_list = ch;
	if (!char_list_lastpc)
		char_list_lastpc = ch;
	{
		int count;
		FILE *max_on_file;
		int tmp = 0;
		count = 0;
		for (d = descriptor_list; d != NULL; d = d->next)
			if (d->connected == CON_PLAYING)
				count++;
		max_on = UMAX(count, max_on);
		if ((max_on_file = dfopen(TMP_PATH, MAXON_FILE, "r"))) {
			fscanf(max_on_file, "%d", &tmp);
			fclose(max_on_file);
		}
		if (tmp < max_on
		    && (max_on_file =
			dfopen(TMP_PATH, MAXON_FILE, "w"))) {
			fprintf(max_on_file, "%d", max_on);
			log("Global max_on changed.");
			fclose(max_on_file);
		}
	}

	if (ch->level == 0) {
		ch->max_mana = (ch->perm_stat[STAT_INT]2/3
			+ ch->perm_stat[STAT_WIS]/2);
		ch->max_move += (ch->perm_stat[STAT_CON]/5
			+ ch->perm_stat[STAT_DEX]/5) * 5;
	}

	reset_char(ch);

	/* quest code */
	nextquest = -abs(ch->pcdata->questtime);
	quest_cancel(ch);
	ch->pcdata->questtime = nextquest;
	/* !quest code */

	snprintf(buf, sizeof(buf),
		 " login: {W$N{x - %s",
		 (ch->desc) 
		 ? ((ch->pcdata->fake_ip)
			? ch->pcdata->fake_ip : ch->desc->host)
		 : "<no descriptor>");
	wiznet(buf, ch, NULL, WIZ_LOGINS, 0, ch->level);

	for (i = 0; i < MAX_STATS; i++) {
		int max_stat = get_max_train(ch, i);

		if (ch->perm_stat[i] > max_stat) {
			ch->train += ch->perm_stat[i] - max_stat;
			ch->perm_stat[i] = max_stat;
		}
	}

	if (!IS_IMMORTAL(ch)
	    && ch->gold > 1000 && ch->gold - 1000 / 2 > 0) {
		char_printf(ch, "You are taxed %d gold to pay for"
			    " the Mayor's bar tab.\n\r",
			    (ch->gold - 1000) / 2);
		ch->gold -= (ch->gold - 1000) / 2;
	}

	if (!IS_IMMORTAL(ch)) {
		for (i = 2; exp_for_level(ch, i) < ch->exp; i++);

		if (i < ch->level) {
			int con;
			int wis;
			int inte;
			int dex;

			con = ch->perm_stat[STAT_CON];
			wis = ch->perm_stat[STAT_WIS];
			inte = ch->perm_stat[STAT_INT];
			dex = ch->perm_stat[STAT_DEX];
			ch->perm_stat[STAT_CON] =
			    get_max_train(ch, STAT_CON);
			ch->perm_stat[STAT_WIS] =
			    get_max_train(ch, STAT_WIS);
			ch->perm_stat[STAT_INT] =
			    get_max_train(ch, STAT_INT);
			ch->perm_stat[STAT_DEX] =
			    get_max_train(ch, STAT_DEX);
			do_remove(ch, "all");
			advance(ch, i - 1);
			ch->perm_stat[STAT_CON] = con;
			ch->perm_stat[STAT_WIS] = wis;
			ch->perm_stat[STAT_INT] = inte;
			ch->perm_stat[STAT_DEX] = dex;
		}
	}

	if (ch->level == 0) {
		OBJ_DATA *wield;
		OBJ_INDEX_DATA *map;

		ch->level = 1;
		ch->exp = base_exp(ch);
		ch->hit = ch->max_hit;
		ch->mana = ch->max_mana;
		ch->move = ch->max_move;
		if (STAT_TRAIN_RATE > 0)
			ch->train = 3;
		ch->practice += 5;
		ch->pcdata->death = 0;

		set_title(ch, title_lookup(ch));

		do_outfit(ch, str_empty);

		obj_to_char(create_obj(get_obj_index(OBJ_VNUM_MAP), 0),
			    ch);
		obj_to_char(create_obj
			    (get_obj_index(OBJ_VNUM_NMAP1), 0), ch);
		obj_to_char(create_obj
			    (get_obj_index(OBJ_VNUM_NMAP2), 0), ch);
		obj_to_char(create_obj
			    (get_obj_index(OBJ_VNUM_MUSHROOM), 0), ch);
		obj_to_char(create_obj
			    (get_obj_index(OBJ_VNUM_MUSHROOM), 0), ch);
		obj_to_char(create_obj
			    (get_obj_index(OBJ_VNUM_MUSHROOM), 0), ch);
		obj_to_char(create_obj
			    (get_obj_index(OBJ_VNUM_SCHOOL_FLASK), 0),
			    ch);
		obj_to_char(create_obj
			    (get_obj_index(OBJ_VNUM_NEWBIE_GUIDE), 0),
			    ch);

		if ((map = get_map(ch)) != NULL)
			obj_to_char(create_obj(map, 0), ch);

		if ((wield = get_eq_char(ch, WEAR_WIELD)))
			set_skill_raw(ch, get_weapon_sn(wield),
				      40, FALSE);

		char_puts("\n", ch);
		do_help(ch, "NEWBIE INFO");
		char_puts("\n", ch);
		char_to_room(ch, get_room_index(ROOM_VNUM_SCHOOL));

	} else {
		CHAR_DATA *pet;
		ROOM_INDEX_DATA *to_room;

		if (ch->in_room
		    && (room_is_private(ch->in_room) ||
			(ch->in_room->area->clan &&
			 ch->in_room->area->clan != ch->clan)))
			ch->in_room = NULL;

		if (ch->in_room)
			to_room = ch->in_room;
		else if (IS_IMMORTAL(ch))
			to_room = get_room_index(ROOM_VNUM_CHAT);
		else
			to_room = get_room_index(ROOM_VNUM_TEMPLE);

		pet = ch->pet;
		act("$N emerges from the astral plane.",
		    to_room->people, NULL, ch, TO_ALL);
		char_to_room(ch, to_room);

		if (pet) {
			act("$N emerges following $S master.",
			    to_room->people, NULL, pet, TO_ROOM);
			char_to_room(pet, to_room);

			/*expire if no clan item to support them*/
			if (!clan_item_ok(pet->pIndexData->clan)) {
				act("$n withers away as the source of"
				" $s existance wanes.",
				pet, NULL, NULL, TO_ROOM);
			}
		}
	}

	sync_ch_pdata(ch);
	update_usage(ch->pcdata->pdata, FALSE);

	if (!JUST_KILLED(ch)) {
		do_look(ch, "auto");
		do_unread(ch, "login");
	}

	if (IS_IMMORTAL(ch)) {
		char_puts("\n", ch);
		doprintf(do_anoncolor, ch, "set");
	}

	if (clan_remove_outcast(ch))
		save_char_obj(ch, FALSE);

	/* very inefficent, but it works */
	if (ch->clan != CLAN_NONE && !clan_item_ok(ch->clan))
		clan_items_remove(ch, clan_lookup(ch->clan));

	/* welcome newbies */
	if (ch->level < 2
	    && (IS_SET(ch->acct_flags, ACCT_NEW) || IS_NEWBIE(ch))) {
		DESCRIPTOR_DATA *dt;
		for (dt = descriptor_list; dt; dt = dt->next)
			if (dt->connected == CON_PLAYING) {
				char_printf(dt->character,
					    "{W[{mN{ce{mw{cb{mi{ce{W]{x:"
					    " {MEveryone give warm welcome to"
					    " our newest player, {Y%s{M!{x\n",
					    ch->name);
			}
		char_puts
		    ("{gNote{x: you must be level 2 to save your character.\n",
		     ch);
	}

	fix_1_v2_hp_dr(ch);
	return FALSE;
}

/* class ok ******************************************************
 * checks to see if the class is okay for the current defined character
 */
bool class_ok(CHAR_DATA *ch, int class)
{
	race_t *r;
	class_t *cl;

	if ((cl = class_lookup(class)) == NULL
	||  (r = race_lookup(ORG_RACE(ch))) == NULL
	||  !r->pcdata)
		return FALSE;

	if (IS_NEWBIE(ch) && class == CLASS_SAMURAI)
		return FALSE;

	if (rclass_lookup(r, cl->name) == NULL
	||  (cl->restrict_sex >= 0 && cl->restrict_sex != ch->sex))
		return FALSE;

	if (cl->restrict_align 
	&& r->pcdata->restrict_align
	&& !(r->pcdata->restrict_align & cl->restrict_align))
		return FALSE;

	if (cl->restrict_ethos
	&& r->pcdata->restrict_ethos
	&& !(r->pcdata->restrict_ethos & cl->restrict_ethos))
		return FALSE;

	return TRUE;
}

/* possible align ************************************************/
flag32_t align_restrict(CHAR_DATA *ch)
{
	race_t *r;
	flag32_t align_restrict = RA_NONE;

	if ((r = race_lookup(ORG_RACE(ch))) == NULL
	||  !r->pcdata)
		return RA_NONE;

	if (IS_SET(r->pcdata->restrict_align, RA_GOOD)
	&&  IS_SET(CLASS(ch->class)->restrict_align, RA_GOOD))
		align_restrict |= RA_GOOD;

	if (IS_SET(r->pcdata->restrict_align, RA_NEUTRAL)
	&&  IS_SET(CLASS(ch->class)->restrict_align, RA_NEUTRAL))
		align_restrict |= RA_NEUTRAL;

	if (IS_SET(r->pcdata->restrict_align, RA_EVIL)
	&&  IS_SET(CLASS(ch->class)->restrict_align, RA_EVIL))
		align_restrict |= RA_EVIL;

	return align_restrict;
}

/* possible ethos ************************************************/
flag32_t ethos_restrict(CHAR_DATA *ch)
{
	race_t *r;
	flag32_t ethos_restrict = RA_NONE;

	if ((r = race_lookup(ORG_RACE(ch))) == NULL
	||  !r->pcdata)
		return RA_NONE;

	if (IS_SET(r->pcdata->restrict_ethos, RE_LAWFUL)
	&&  IS_SET(CLASS(ch->class)->restrict_ethos, RE_LAWFUL))
		ethos_restrict |= RE_LAWFUL;

	if (IS_SET(r->pcdata->restrict_ethos, RE_NEUTRAL)
	&&  IS_SET(CLASS(ch->class)->restrict_ethos, RE_NEUTRAL))
		ethos_restrict |= RE_NEUTRAL;

	if (IS_SET(r->pcdata->restrict_ethos, RE_CHAOTIC)
	&&  IS_SET(CLASS(ch->class)->restrict_ethos, RE_CHAOTIC))
		ethos_restrict |= RE_CHAOTIC;

	return ethos_restrict;
}

/* ethos *********************************************************/
int con_get_ethos (CHAR_DATA *ch, const char *argument) 
{
	DESCRIPTOR_DATA *d = ch->desc;
	flag32_t allowed_ethos = ethos_restrict(ch);
	bool ethos_set = FALSE;

	if (allowed_ethos == RE_LAWFUL) {
		ch->ethos = ETHOS_LAWFUL; 
		write_to_buffer(d, "You are compelled to follow the Law.\n\r",0);
		ethos_set = TRUE;
	}
	else if (allowed_ethos == RE_NEUTRAL) {
		ch->ethos = ETHOS_NEUTRAL; 
		write_to_buffer(d, "You are compelled to be be ambigious toward the Law.\n\r",0);
		ethos_set = TRUE;
	}
	else if (allowed_ethos == RE_CHAOTIC) {
		ch->ethos = ETHOS_CHAOTIC; 
		write_to_buffer(d, "You are compelled to have contempt for the Law.\n\r",0);
		ethos_set = TRUE;
	}

	if (!ethos_set && argument != NULL) {
		switch(argument[0]) {
			case 'l' : case 'L' : 
				if (IS_SET(allowed_ethos, RE_LAWFUL)) {
					ch->ethos = ETHOS_LAWFUL; 
					write_to_buffer(d, "You follow the Law.\n\r",0);
					ethos_set = TRUE;
				}
				else {
					write_to_buffer(d,"That's not a valid ethos.\n\r",0);
				}
				break;
			case 'n' : case 'N' : 
				if (IS_SET(allowed_ethos, RE_NEUTRAL)) {
					ch->ethos = ETHOS_NEUTRAL; 
					write_to_buffer(d, "You are ambigious toward the Law.\n\r",0);
					ethos_set = TRUE;
				}
				else {
					write_to_buffer(d,"That's not a valid ethos.\n\r",0);
				}
				break;
			case 'c' : case 'C' : 
				if (IS_SET(allowed_ethos, RE_CHAOTIC)) {
					ch->ethos = ETHOS_CHAOTIC; 
					write_to_buffer(d, "You have contempt for the Law.\n\r",0);
					ethos_set = TRUE;
				}
				else {
					write_to_buffer(d,"That's not a valid ethos.\n\r",0);
				}
				break;
			case '?' : case 'h': case 'H':
				do_help(ch, "'ethos'");
				break;
		}
	}

	if (!ethos_set) {
		write_to_buffer(d, "\n\rChoose your ethos (", 0);
		if (IS_SET(allowed_ethos, RE_LAWFUL))
			write_to_buffer(d, "Lawful ", 0);
		if (IS_SET(allowed_ethos, RE_NEUTRAL))
			write_to_buffer(d, "Neutral ", 0);
		if (IS_SET(allowed_ethos, RE_CHAOTIC))
			write_to_buffer(d, "Chaotic", 0);
		write_to_buffer(d, ")[?]: ", 0);
	}
	else {
		d->connected = CON_PICK_HOMETOWN;
		return TRUE;
	}
	return FALSE;
}

/* display hometown **********************************************/
static void print_hometown(CHAR_DATA *ch)
{
	race_t *r;
	class_t *cl;
	int htn;

	if ((r = race_lookup(ORG_RACE(ch))) == NULL
	||  !r->pcdata
	||  (cl = class_lookup(ch->class)) == NULL) {
		char_puts("You should create your character anew.\n", ch);
		close_descriptor(ch->desc);
		return;
	}

	if ((htn = hometown_permanent(ch)) >= 0) {
		ch->hometown = htn;
		char_printf(ch, "\nYour hometown is %s, permanently.\n"
				"[Hit Return to continue]\n",
			    hometown_name(htn));

/* XXX */
		ch->desc->connected = CON_CREATE_DONE;
		return;
	}

	char_puts("\n", ch);
	do_help(ch, "HOMETOWN");
	hometown_print_avail(ch);
	char_puts("? ", ch);
	ch->desc->connected = CON_PICK_HOMETOWN;
}

/*
 * look for link-dead player to reconnect.
 *
 * when fConn == FALSE then
 * simple copy password for newly [re]connected character
 * authentication
 *
 * otherwise reconnect attempt is made
 */
bool check_reconnect(DESCRIPTOR_DATA *d, const char *name, bool fConn)
{
	CHAR_DATA *ch;
	DESCRIPTOR_DATA *d2;

	if (!fConn) {
		for (d2 = descriptor_list; d2; d2 = d2->next) {
			if (d2 == d)
				continue;
			ch = d2->original ? d2->original : d2->character;
			if (ch && !str_cmp(d->character->name, ch->name)) {
				free_string(d->character->pcdata->pwd);
				d->character->pcdata->pwd = str_qdup(ch->pcdata->pwd);
				return TRUE;
			}
		}
	}

	for (ch = char_list; ch && !IS_NPC(ch); ch = ch->next) {
		if ((!fConn || ch->desc == NULL)
		&&  !str_cmp(d->character->name, ch->name)) {
			if (!fConn) {
				free_string(d->character->pcdata->pwd);
				d->character->pcdata->pwd = str_qdup(ch->pcdata->pwd);
			}
			else {
				free_char(d->character);
				d->character	= ch;
				ch->desc	= d;
				ch->timer	= 0;
				char_puts("Reconnecting. Type replay to see missed tells.\n", ch);
				act("$n emerges from the astral plane.",
				    ch, NULL, NULL, TO_ROOM);

				log_printf("%s@%s reconnected.",
					   ch->name, d->host);
				wiznet("$N grasps the fullness of $S link.",
				       ch, NULL, WIZ_LINKS, 0, 
				       (ch->invis_level > ch->incog_level) 
				       ? ch->invis_level
				       : ch->incog_level);
				d->connected = CON_PLAYING;
				sync_ch_pdata(ch);

				if (!IS_IMMORTAL(ch)
				&& ch->in_room->area->clan != CLAN_FREEMAN
				&& ch->clan != ch->in_room->area->clan)
					ch->in_room->area->interlopers++;

			}
			return TRUE;
		}
	}

	return FALSE;
}

/*
 * Check if already playing.
 */
bool check_playing(DESCRIPTOR_DATA *d, const char *name)
{
	DESCRIPTOR_DATA *dold;

	for (dold = descriptor_list; dold; dold = dold->next) {
		if (dold != d
		&&  dold->character != NULL
/*		&&  dold->connected != CON_GET_CODEPAGE */
		&&  dold->connected != CON_GET_NAME
		&&  dold->connected != CON_RESOLV
		&&  dold->connected != CON_GET_OLD_PASSWORD
		&&  !str_cmp(name, dold->original ?  dold->original->name :
						     dold->character->name)) {
			write_to_buffer(d, "That character is already playing.\n\r",0);
			write_to_buffer(d, "Do you wish to connect anyway (Y/N)?",0);
			d->connected = CON_BREAK_CONNECT;
			return TRUE;
		}
	}

	return FALSE;
}
