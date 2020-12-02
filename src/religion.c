/*
 * $Id: religion.c 941 2006-11-27 00:42:35Z zsuzsu $
 */

/***************************************************************************
 *     ANATOLIA 2.1 is copyright 1996-1997 Serdar BULUT, Ibrahim CANPUNAR  *	
 *     ANATOLIA has been brought to you by ANATOLIA consortium		   *
 *	 Serdar BULUT {Chronos}		bulut@rorqual.cc.metu.edu.tr       *
 *	 Ibrahim Canpunar  {Asena}	canpunar@rorqual.cc.metu.edu.tr    *	
 *	 Murat BICER  {KIO}		mbicer@rorqual.cc.metu.edu.tr	   *	
 *	 D.Baris ACAR {Powerman}	dbacar@rorqual.cc.metu.edu.tr	   *	
 *     By using this code, you have agreed to follow the terms of the      *
 *     ANATOLIA license, in the file Anatolia/anatolia.licence             *	
 ***************************************************************************/

/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*	ROM 2.4 is copyright 1993-1995 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@pacinfo.com)				   *
*	    Gabrielle Taylor (gtaylor@pacinfo.com)			   *
*	    Brian Moore (rom@rom.efn.org)				   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "fight.h"

/* vnums for tattoos */
#define OBJ_VNUM_TATTOO_ATUM_RA 	51
#define OBJ_VNUM_TATTOO_ZEUS		52
#define OBJ_VNUM_TATTOO_SIEBELE 	53
#define OBJ_VNUM_TATTOO_SHAMASH		54
#define OBJ_VNUM_TATTOO_AHURAMAZDA	55
#define OBJ_VNUM_TATTOO_EHRUMEN 	56
#define OBJ_VNUM_TATTOO_DEIMOS		57
#define OBJ_VNUM_TATTOO_PHOBOS		58
#define OBJ_VNUM_TATTOO_ODIN		59
#define OBJ_VNUM_TATTOO_TESHUB		60
#define OBJ_VNUM_TATTOO_ARES		61
#define OBJ_VNUM_TATTOO_GOKTENGRI	62
#define OBJ_VNUM_TATTOO_HERA		63
#define OBJ_VNUM_TATTOO_VENUS		64
#define OBJ_VNUM_TATTOO_SETH		65
#define OBJ_VNUM_TATTOO_ENKI		66
#define OBJ_VNUM_TATTOO_EROS		67
#define OBJ_VNUM_TATTOO_SIRALA		68

/* God's Name, name of religion, tattoo vnum  */
const struct religion_type religion_table [] =
{
  { str_empty,		"None",			0			},
  { "Atheist",		"Nothing",		0			}
};

char *religion_name(int religion)
{
	return religion <= RELIGION_NONE || religion > MAX_RELIGION ?
			"none" : religion_table[religion].leader;
}

/*Zz*/
/* religion_lookup - converts a religion string to the index number
 *    it is case insensive.
 */
int religion_lookup(const char *religion) {
	int index = RELIGION_NONE;
	if (strlen(religion) < 2) return index;
	switch (UPPER(religion[0])) {
		case 'A':
			switch (LOWER(religion[1])) {
				case 't': 
					switch (LOWER(religion[2])) {
						case 'u': index = RELIGION_ATUM_RA; break;
						case 'h': index = RELIGION_ATHEIST; break;
					}
					break;
				case 'h': index = RELIGION_AHURAMAZDA; break;
				case 'r': index = RELIGION_ARES; break;
			}
			break;
		case 'D': index = RELIGION_DEIMOS; break;
		case 'E':
			switch (LOWER(religion[1])) {
				case 'n': index = RELIGION_ENKI; break;
				case 'r': index = RELIGION_EROS; break;
				case 'h': index = RELIGION_EHRUMEN; break;
			}
			break;
		case 'G': index = RELIGION_GOKTENGRI; break;
		case 'H': index = RELIGION_HERA; break;
		case 'P': index = RELIGION_PHOBOS; break;
		case 'O': index = RELIGION_ODIN; break;

		case 'S':
			switch (LOWER(religion[1])) {
				case 'i': 
					if (strlen(religion) < 3) break;
					else
						switch (LOWER(religion[2])) {
							case 'r': index = RELIGION_SIRALA; 
								break;
							case 'e': index = RELIGION_SIEBELE; 
								break;
						}
					break;
				case 'e': index = RELIGION_SETH; break;
				case 'h': index = RELIGION_SHAMASH; break;
			}
			break;
		case 'T': index = RELIGION_TESHUB; break;
		case 'V': index = RELIGION_VENUS; break;
		case 'Z': index = RELIGION_ZEUS; break;
	}
	return index;
}

void do_repudiate(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		char_puts("To proclaim your atheism, name your current god.\n"
			  "Or if you have none, type 'gods'\n", ch);
		return;
	}

	if (ch->religion == 0) {
		if (str_cmp("gods", arg)) {
			char_puts("You must denounce 'gods'.\n", ch);
			return;
		}
	}
	else if (str_cmp(religion_name(ch->religion), arg)) {
		char_puts("How can you denounce a god you do not already worship?\n", ch);
		return;
	}

	if (get_eq_char(ch, WEAR_TATTOO) != NULL) {
		char_puts("You must first scratch off your tattoo.\n", ch);
		return;
	}

	act("A lightning bolt shoots down from the heavens and strikes $n!",
	    ch, NULL, NULL, TO_ROOM);
	act("A lightning bolt shoots down from the heavens and strikes you!",
	    ch, NULL, NULL, TO_CHAR);
	damage(ch, ch, ch->hit +1, TYPE_HUNGER, DAM_OTHER, TRUE);
	ch->hit  = -1;
	ch->mana = 0;
	ch->move = 0;
	ch->position = POS_MORTAL;
	ch->religion = RELIGION_ATHEIST;
}

/* 
 * removing a tattoo
 *
 * by Zsuzsu
 */
void do_scratch(CHAR_DATA * ch, const char *argument)
{
	char            arg[MAX_INPUT_LENGTH];
	OBJ_DATA       *obj;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0' || str_cmp(arg, "tattoo")) {
		char_puts("To remove your tattoo you must type: scratch tattoo\n", ch);
		return;
	}

	obj = get_eq_char(ch, WEAR_TATTOO);

	if (!obj) {
		char_puts("You have no tattoo to scratch out.\n", ch);
		return;
	}
	act("$n claws at $p with $s fingernails, gouging it out.",
	    ch, obj, NULL, TO_ROOM);
	act("$n screams in pain!", ch, NULL, NULL, TO_ROOM);
	act("You claw at $p with your fingernails, gouging it out.",
	    ch, obj, NULL, TO_CHAR);
	act("You scream in pain!", ch, NULL, NULL, TO_CHAR);

	damage(ch, ch, ch->hit * 2/ 3+1, TYPE_HIT, DAM_OTHER, TRUE);
	unequip_char(ch, obj);
	extract_obj(obj, 0);
}
