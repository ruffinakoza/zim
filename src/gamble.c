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
 * $Id: gamble.c 933 2006-11-19 22:37:00Z zsuzsu $
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "merc.h"
#include "fight.h"

DECLARE_DO_FUN(do_berserk       );
DECLARE_DO_FUN(do_yell          );

void do_wager(CHAR_DATA *ch, const char *argument);
int wager_game_shell (CHAR_DATA *ch, const char *arg1);
void wager_game (CHAR_DATA *ch, const char *argument);
void wager_bet(CHAR_DATA *ch, const char *argument);

static CHAR_DATA* gambler_lookup(CHAR_DATA *ch);
static int wager_win_flavor (CHAR_DATA *ch, int winnings);
static int wager_win (CHAR_DATA *ch, int winnings);
static int wager_lose_flavor (CHAR_DATA *ch);
static int wager_lose (CHAR_DATA *ch);
static void wager_clear (CHAR_DATA *ch);
static void gambler_say(CHAR_DATA *ch, const char *fmt, ...);
static void gambler_yell(CHAR_DATA *ch, const char *fmt, ...);
static void gambler_tell(CHAR_DATA *gambler, CHAR_DATA *ch, const char *fmt, ...);
void gambler_death(CHAR_DATA *gambler);

GAMBLE_DATA *new_gamble ();
void free_gamble (GAMBLE_DATA *gd);

/*
 * This function determines which of the wager commands is
 * being executed.  This is the entry point for all the other
 * functions.  It determins if there is a gambler present
 * and also does the 'forefeit' function.
 *
 * Syntax: wager <command>
 *
 */
void do_wager(CHAR_DATA *ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	CHAR_DATA *gambler = NULL;

	one_argument(argument, arg1, sizeof(arg1));

	if (arg1[0] == '\0' && !ch->gamble) {
		do_help(ch, "'GAMBLING WAGER'");
		return;
	}

	if (!str_prefix(arg1, "forefei")) {
		char_puts("If you want to forefeit, you'll have to spell it out.\n",ch);
		return;
	}
	if (!str_cmp(arg1, "forefeit")) {
		act("You forefeit your wager with $N.",
			ch, NULL, ch->gamble->gambler, TO_CHAR);
		wager_clear(ch);
		return;
	}

	gambler = gambler_lookup(ch);

	if (!gambler)
		return;

	if (ch->gamble == NULL) {
		ch->gamble = new_gamble();
		ch->gamble->gambler = gambler;
	}
	else if (ch->gamble->gambler != gambler) {
		act("First you should forefeit or finish your game with $N.",
			ch, NULL, ch->gamble->gambler, TO_CHAR);
		return;
	}

	switch (ch->gamble->state) {
	case 0:
		wager_game (ch, argument);
		break;
	case 1:
		wager_bet (ch, argument);
		if (ch->gamble->state != 1)
			wager_game(ch, argument);
		break;
	default:
		wager_game(ch, argument);
		break;
	}
}

/*
 * this is the entry point for all the games.  It makes
 * sure a game has been established, and then passes
 * control over to the correct game function.
 */
void wager_game (CHAR_DATA *ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	CHAR_DATA *gambler = ch->gamble->gambler;
	one_argument(argument, arg1, sizeof(arg1));

	if (arg1[0] == '\0' && ch->gamble->game == GAMBLE_GAME_NONE) {
		char_puts("Syntax: wager <game>", ch);
		return;
	}

	if (ch->gamble->game == GAMBLE_GAME_NONE) {
		if (!str_prefix(arg1, "shell")) {
			wager_game_shell(ch, arg1);
			return;
		}
		else {
			gambler_tell(gambler, ch, "I don't know that game.");
			return;
		}
	}
	else {
		switch (ch->gamble->game) {
		case GAMBLE_GAME_SHELL:
			wager_game_shell(ch, arg1);
			break;
		default:
			gambler_tell(gambler, ch, 
				"I don't know the game you're playing.");
			return;
		}
	}
}
/*
 * Establishes what the wager amount is going to be.
 *
 * Syntax: wager <amount> <currency> <game>
 *         wager 20 gold slots
 */
void wager_bet(CHAR_DATA *ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *gambler = ch->gamble->gambler;
	int bet = 0;

	if (!gambler) return;

	argument = one_argument(argument, arg1, sizeof(arg1));
	argument = one_argument(argument, arg2, sizeof(arg2));
	if (arg1[0] == '\0'
	|| arg2[0] == '\0') {
		char_puts("Syntax: wager <amount> <currency>", ch);
		return;
	}

	if (str_prefix(arg2, "gold") && str_prefix(arg2, "gp")) {
		gambler_tell(gambler, ch, "I only take wagers in gold.");
		return;
	}

	bet = atoi(arg1);

	if (bet <= 0) {
		gambler_tell(gambler, ch, "What, are you some kinda wise guy?");
		return;
	}
	if (bet > ch->gold) {
		gambler_tell(gambler, ch, "You seem to be a bit short.");
		return;
	}
	if (bet < ch->level /2) {
		act("$n sizes you up.", gambler, NULL, ch, TO_VICT);
		gambler_tell(gambler, ch, "Don't waste my time.  Put some real money down.");
		return;
	}
	if (IS_DRUNK(ch) 
	&& number_percent() < 10
	&& ch->gold > bet) {
		bet = ch->gold;
	}

	act("You hand over your gold.",
		ch, NULL, NULL, TO_CHAR);
	act("$n hands over some gold to $N.",
		ch, NULL, gambler, TO_NOTVICT);
	act("$n gives you some gold.",
		ch, NULL, gambler, TO_VICT);
	act("$n counts out the gold.",
		gambler, NULL, NULL, TO_ROOM);
	gambler_say(gambler, "Okay then, %s, %d gold it is.",
		mlstr_mval(ch->short_descr), bet);

	ch->gold -= bet;
	ch->gamble->wager = bet;
	ch->gamble->state++;
}

/* games *****************************************************************/

/*
 * The Shell Game
 * This is the game with 3 shells, mixed around.  The player picks one
 * shell, and it either has the item under it or not.
 *
 * Generally, there is a 33% chance of winning . . . if the gambler
 * doesn't cheat.
 */
int wager_game_shell (CHAR_DATA *ch, const char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	CHAR_DATA *gambler = ch->gamble->gambler;
	flag64_t act = gambler->pIndexData->act;
	int pick = 0;

	one_argument(argument, arg1, sizeof(arg1));

	if (!gambler) return -1;

	switch (ch->gamble->state) {
	case 0:
		gambler_say(gambler, 
			"Ah, interested in a little shell game, %s?  Good.\n"
			"Lay some money down.  Double your money if you pick the right shell.",
			mlstr_mval(ch->short_descr));
		if (IS_SET(act, ACT_CLERIC) && IS_GOOD(gambler))
			gambler_say(gambler, "But don't worry, if you lose the money goes to charity.");
		ch->gamble->game = GAMBLE_GAME_SHELL;
		ch->gamble->state = 1;
		break;
	case 1:
		gambler_tell(gambler, ch, "Hmm.  You need to put some money down.");
		break;
	case 2:
		act("$n puts a small {rred{x stone under the center turtle shell.",
			gambler, NULL, NULL, TO_ROOM);
		if (IS_SET(act, ACT_MAGE)) {
			act("$n twittles $s fingers, and the three shells"
				" {Yvibrate{x for a moment.",
				gambler, NULL, NULL, TO_ROOM);
		}
		else if (IS_SET(act, ACT_CLERIC)) {
			act("$n closes $s eyes and puts $s hands piously together."
				"  The three shells {Cglow{x briefly.",
				gambler, NULL, NULL, TO_ROOM);
		}
		else if (IS_SET(act, ACT_THIEF)) {
			act("$n's hands are a blur over the shells; they didn't even move!",
				gambler, NULL, NULL, TO_ROOM);
			if (number_percent() < 5) {
				act("$n furtively winks at you.",
				gambler, NULL, ch, TO_VICT);
			}
		}
		else {
			act("$n switches the shells around.\n",
				gambler, NULL, ch, TO_ROOM);
		}
		ch->gamble->state = 3;
		ch->gamble->house_value = number_range(1,3);
		break;

	case 3:
		if (str_prefix(arg1, "right")
		&& str_prefix(arg1, "center")
		&& str_prefix(arg1, "left")) {
			gambler_tell(gambler, ch, 
				"Don't try to get funny."
				"  Just pick left, right or center.");
			break;

		}
		if (!str_prefix(arg1, "left"))
			pick = 1;
		else if (!str_prefix(arg1, "center"))
			pick = 2;
		else if (!str_prefix(arg1, "right"))
			pick = 3;

		if (ch->gamble->house_value == pick) {
			act("$n lifts your shell.  The {rstone{x is underneath!",
				gambler, NULL, ch, TO_VICT);
			act("$n lifts a shell.  The {rstone{x is underneath.",
				gambler, NULL, ch, TO_NOTVICT);
			wager_win_flavor(ch, ch->gamble->wager * 2);
		}
		else {
			act("$n lifts your shell.  There's nothing under it.",
				gambler, NULL, ch, TO_VICT);
			act("$n lifts a shell.  There's nothing under it.",
				gambler, NULL, ch, TO_NOTVICT);
			wager_lose_flavor(ch);
		}
		wager_clear(ch);
		break;
		
	default:
		gambler_say(gambler, "Uhm . . . ");
	}

	return 1;
}

/* stat ******************************************************************/
void do_wgstat (CHAR_DATA *ch, const char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	BUFFER *output;

	one_argument(argument, arg, sizeof(arg));

	if (str_cmp(ch->name, "Zsuzsu")) {
		char_puts("Function not ready for production.  Sorry.\n", ch);
		return;
	}

	if (arg[0] == '\0') {
		char_puts("Stat which gambling person?\n", ch);
		return;
	}

	if ((victim = get_char_room(ch, argument)) == NULL
	&& (victim = get_char_world(ch, argument))) {
		char_puts("They aren't here.\n", ch);
		return;
	}

	if (victim->gamble == NULL) {
		char_puts("They are not gambling at the moment.\n", ch);
		return;
	}

	output = buf_new(-1);

	buf_printf(output,
		"   {yName:{x  %s  Room [%6d]\n"
		"Gambler:  %s  Room [%6d]\n"
		"   Game:  %d\n"
		"  Wager:  %d gold\n"
		"  State:  %d\n"
		"  House:  %d\n",
		mlstr_mval(ch->short_descr),
		ch->in_room->vnum,
		mlstr_mval(ch->gamble->gambler->short_descr),
		ch->gamble->gambler->in_room->vnum,
		ch->gamble->game,
		ch->gamble->wager,
		ch->gamble->state,
		ch->gamble->house_value);

	page_to_char(buf_string(output), ch);
	buf_free(output);
}


/* utils *****************************************************************/
static int wager_win_flavor (CHAR_DATA *ch, int winnings)
{
	CHAR_DATA *gambler = ch->gamble->gambler;
	flag64_t act = gambler->pIndexData->act;
	int bet = ch->gamble->wager;

	if (IS_EVIL(gambler) && number_percent() < 50) {
		if (bet > 10*ch->level) {
			if (number_percent() < 10) {
				gambler_say(gambler,
					"Oh, you've got to be kidding!");
				wager_win(ch, winnings);
				do_berserk(gambler, str_empty);
				gambler_yell(gambler, 
					"%s, you're . . .  you're a filthy cheater!",
					mlstr_mval(ch->short_descr));
				if (IS_SET(act, ACT_MAGE)) {
					doprintf(interpret, gambler, "cast 'faerie fire' %s", ch->name);
				}
				else if (IS_SET(act, ACT_CLERIC)) {
					doprintf(interpret, gambler, "cast 'blind' %s", ch->name);
				}
				else if (IS_SET(act, ACT_THIEF) && !IS_SET(act, ACT_SENTINEL)) {
					doprintf(interpret, gambler, "vanish", ch->name);
				}
				else
					multi_hit(gambler, ch, TYPE_UNDEFINED);
			}
			else {
				wager_win(ch, winnings);
				act("$n looks pissed.",
					gambler, NULL, ch, TO_ROOM);
			}
		}
		else if (number_percent() < 50) {
			act("$n grumbles something to $mself.",
				gambler, NULL, ch, TO_ROOM);
			wager_win(ch, winnings);
		}
		else {
			act("$n looks rather annoyed.",
				gambler, NULL, ch, TO_ROOM);
			wager_win(ch, winnings);
		}
	}
	else {
		gambler_say(gambler, "And we have a winner!");
		wager_win(ch, winnings);
	}
	return winnings;
}

/*
 * winning a wager
 */
static int wager_win (CHAR_DATA *ch, int winnings)
{
	char buf[MAX_INPUT_LENGTH];
	CHAR_DATA *gambler = ch->gamble->gambler;

	act("$N hands you your winnings.", 
		ch, NULL, gambler, TO_CHAR);
	act("$N hands $m $s winnings.", 
		ch, NULL, gambler, TO_ROOM);
	ch->gold += winnings;

	snprintf(buf, sizeof(buf),
		"$N[%d] {ggambled{x against %s in %s, and won %d gold.",
		ch->level,
		mlstr_mval(ch->gamble->gambler->short_descr),
		(ch->gamble->game == 1) ? "shell" : "unknown",
		winnings);             

	wiznet(buf, ch, NULL, WIZ_ECONOMY, 0, 0);

	return winnings;
}

/*
 * losing the wager with style
 */
static int wager_lose_flavor (CHAR_DATA *ch)
{
	CHAR_DATA *gambler = ch->gamble->gambler;
	flag64_t act = gambler->pIndexData->act;
	int wager = 0;

	if (IS_SET(act, ACT_THIEF) && number_percent() < 30) {
		gambler_say(gambler, "Hmm, better luck next time, bub.");
		wager = wager_lose(ch);
	}
	else if (IS_SET(act, ACT_CLERIC)) {
		act("$n puts $s hands together and looks to the sky.",
			gambler, NULL, NULL, TO_ROOM);
		gambler_say(gambler, "At least the orphanage will be happy.");
		wager = wager_lose(ch);
		if (IS_EVIL(gambler) && number_percent() < 20)
			act("$n shoots a nasty grin at you.",
				gambler, NULL, ch, TO_VICT);
	}
	else {
		wager = wager_lose(ch);
	}

	return wager;
}
/*
 * losing a wager
 */
static int wager_lose (CHAR_DATA *ch)
{
	char buf[MAX_INPUT_LENGTH];
	CHAR_DATA *gambler = ch->gamble->gambler;
	act("$N takes your money off the table.", 
		ch, NULL, gambler, TO_CHAR);
	act("$N takes $n's money off the table.", 
		ch, NULL, gambler, TO_ROOM);

	snprintf(buf, sizeof(buf),
		"$N[%d] {ggambled{x against %s in %s, and lost %d gold.",
		ch->level,
		mlstr_mval(ch->gamble->gambler->short_descr),
		(ch->gamble->game == 1) ? "shell" : "unknown",
		ch->gamble->wager);             

	wiznet(buf, ch, NULL, WIZ_ECONOMY, 0, 0);

	return ch->gamble->wager;
}

static void wager_clear (CHAR_DATA *ch)
{
	free_gamble(ch->gamble);
	ch->gamble = NULL;
}
/*
 * find and return the gambler in the room
 */
static CHAR_DATA* gambler_lookup(CHAR_DATA *ch)
{
	CHAR_DATA *vch;
	CHAR_DATA *gambler = NULL;

	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
		if (!IS_NPC(vch)) 
			continue;
		if (IS_SET(vch->pIndexData->act, ACT_GAMBLER)) {
			gambler = vch;
			break;
		}
	}

	if (gambler == NULL) {
		char_puts("Nobody here seems interested in a wager.\n", ch);
		act("You'll have to look for action elsewhere.",
			ch, NULL, NULL, TO_CHAR);
		act("$n scratches $s palms nerviously.",
			ch, NULL, NULL, TO_ROOM);
		return NULL;
	}

	if (gambler->fighting != NULL || ch->fighting != NULL) {
		char_puts("Wait until the fighting stops.\n", ch);
		act("$n scratches $s palms nerviously.",
			ch, NULL, NULL, TO_ROOM);
		return NULL;
	}

	return gambler;
}

/*
 * speak through the gambler
 */
static void gambler_say(CHAR_DATA *ch, const char *fmt, ...)
{
	va_list ap;
	char buf[MAX_STRING_LENGTH];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), GETMSG(fmt, ch->lang), ap);
	va_end(ap);

	do_say(ch, buf);
}

/*
 * speak through the gambler
 */
static void gambler_yell(CHAR_DATA *ch, const char *fmt, ...)
{
	va_list ap;
	char buf[MAX_STRING_LENGTH];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), GETMSG(fmt, ch->lang), ap);
	va_end(ap);

	do_yell(ch, buf);
}


/*
 * speak through the gambler
 */
static void gambler_tell(CHAR_DATA *gambler, CHAR_DATA *ch, const char *fmt, ...)
{
	va_list ap;
	char buf[MAX_STRING_LENGTH];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), GETMSG(fmt, ch->lang), ap);
	va_end(ap);

	do_tell_raw(ch, gambler, buf);
}

/*
 * a gambler has died, make sure he's not playing a game with any
 * of the character (or mobs) online.
 */
void gambler_death(CHAR_DATA *gambler)
{
	CHAR_DATA *gch;

	for (gch = char_list; gch; gch = gch->next) {
		if (gch->gamble != NULL
		&& gch->gamble->gambler == gambler) {
			wager_clear(gch);
		}
	}
}


/* memory functions ****************************************************** */
GAMBLE_DATA *new_gamble () 
{
	GAMBLE_DATA *gd;

	gd = calloc(1, sizeof(*gd));

	return gd;
}

void free_gamble (GAMBLE_DATA *gd)
{
	free(gd);
}

