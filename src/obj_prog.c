/*
 * $Id: obj_prog.c 932 2006-11-11 00:07:22Z zsuzsu $
 */

/***************************************************************************
 *     ANATOLIA 2.1 is copyright 1996-1997 Serdar BULUT		           *	
 *     ANATOLIA has been brought to you by ANATOLIA consortium		   *
 *	 Serdar BULUT {Chronos}		bulut@rorqual.cc.metu.edu.tr       *
 *	 Ibrahim Canpunar  {Mandrake}	canpunar@rorqual.cc.metu.edu.tr    *	
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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "db/cmd.h"
#include "interp.h"
#include "fight.h"
#include "healer.h"

DECLARE_DO_FUN(do_yell);
DECLARE_DO_FUN(do_say);

#define DECLARE_OPROG(f) OPROG_FUN f

DECLARE_OPROG(wear_prog_excalibur);
DECLARE_OPROG(remove_prog_excalibur);
DECLARE_OPROG(death_prog_excalibur);
DECLARE_OPROG(speech_prog_excalibur);
DECLARE_OPROG(sac_prog_excalibur);

DECLARE_OPROG(fight_prog_sub_weapon);
DECLARE_OPROG(speech_prog_kassandra);

DECLARE_OPROG(fight_prog_chaos_blade);
DECLARE_OPROG(death_prog_chaos_blade);

DECLARE_OPROG(fight_prog_tattoo_atum_ra);
DECLARE_OPROG(fight_prog_tattoo_zeus);
DECLARE_OPROG(fight_prog_tattoo_siebele);
DECLARE_OPROG(fight_prog_tattoo_ahuramazda);
DECLARE_OPROG(fight_prog_tattoo_shamash);
DECLARE_OPROG(fight_prog_tattoo_ehrumen);
DECLARE_OPROG(fight_prog_tattoo_venus);
DECLARE_OPROG(fight_prog_tattoo_deimos);
DECLARE_OPROG(fight_prog_tattoo_odin);
DECLARE_OPROG(fight_prog_tattoo_phobos);
DECLARE_OPROG(fight_prog_tattoo_teshub);
DECLARE_OPROG(fight_prog_tattoo_ares);
DECLARE_OPROG(fight_prog_tattoo_goktengri);
DECLARE_OPROG(fight_prog_tattoo_hera);
DECLARE_OPROG(fight_prog_tattoo_seth);
DECLARE_OPROG(fight_prog_tattoo_enki);
DECLARE_OPROG(fight_prog_tattoo_eros);
DECLARE_OPROG(fight_prog_tattoo_sirala);

DECLARE_OPROG(fight_prog_golden_weapon);
DECLARE_OPROG(death_prog_golden_weapon);

DECLARE_OPROG(get_prog_heart);

DECLARE_OPROG(wear_prog_bracer);
DECLARE_OPROG(remove_prog_bracer);

DECLARE_OPROG(wear_prog_ranger_staff);
DECLARE_OPROG(fight_prog_ranger_staff);
DECLARE_OPROG(death_prog_ranger_staff);

DECLARE_OPROG(wear_prog_coconut);
DECLARE_OPROG(entry_prog_coconut);
DECLARE_OPROG(greet_prog_coconut);
DECLARE_OPROG(get_prog_coconut);
DECLARE_OPROG(remove_prog_coconut);

DECLARE_OPROG(fight_prog_firegauntlets);
DECLARE_OPROG(wear_prog_firegauntlets);
DECLARE_OPROG(remove_prog_firegauntlets);
/* ibrahim armbands */
DECLARE_OPROG(fight_prog_armbands);
DECLARE_OPROG(wear_prog_armbands);
DECLARE_OPROG(remove_prog_armbands);

DECLARE_OPROG(fight_prog_demonfireshield);
DECLARE_OPROG(wear_prog_demonfireshield);
DECLARE_OPROG(remove_prog_demonfireshield);

DECLARE_OPROG(fight_prog_vorpalblade);
DECLARE_OPROG(get_prog_spec_weapon);
DECLARE_OPROG(get_prog_quest_obj);
DECLARE_OPROG(fight_prog_shockwave);
DECLARE_OPROG(fight_prog_snake);

DECLARE_OPROG(drop_prog_dice);

/* new ones by chronos */
DECLARE_OPROG(wear_prog_wind_boots);
DECLARE_OPROG(remove_prog_wind_boots);

DECLARE_OPROG(wear_prog_arm_hercules);
DECLARE_OPROG(remove_prog_arm_hercules);

DECLARE_OPROG(wear_prog_girdle_giant);
DECLARE_OPROG(remove_prog_girdle_giant);

DECLARE_OPROG(wear_prog_breastplate_strength);
DECLARE_OPROG(remove_prog_breastplate_strength);

DECLARE_OPROG(wear_prog_boots_flying);
DECLARE_OPROG(remove_prog_boots_flying);

DECLARE_OPROG(fight_prog_rose_shield);
DECLARE_OPROG(fight_prog_lion_claw);

DECLARE_OPROG(speech_prog_ring_ra);
DECLARE_OPROG(wear_prog_eyed_sword);
DECLARE_OPROG(wear_prog_ruler_shield);
DECLARE_OPROG(wear_prog_katana_sword);

DECLARE_OPROG(wear_prog_snake);
DECLARE_OPROG(remove_prog_snake);
DECLARE_OPROG(get_prog_snake);

DECLARE_OPROG(wear_prog_fire_shield);
DECLARE_OPROG(remove_prog_fire_shield);
DECLARE_OPROG(wear_prog_quest_weapon);
DECLARE_OPROG(get_prog_quest_reward);

DECLARE_OPROG(fight_prog_swordbreaker);
DECLARE_OPROG(speech_prog_deathlure);

DECLARE_OPROG(speech_prog_azurethword);

char *optype_table[] = {
	"wear_prog",
	"remove_prog",
	"drop_prog",
	"sac_prog",
	"give_prog",
	"greet_prog",
	"fight_prog",
	"death_prog",
	"speech_prog",
	"entry_prog",
	"get_prog",
	"area_prog",
	NULL
};

struct oprog_data {
	char *name;
	OPROG_FUN *fn;
};
typedef struct oprog_data OPROG_DATA;

OPROG_DATA oprog_table[] = {
	{"wear_prog_excalibur", wear_prog_excalibur},
	{"remove_prog_excalibur", remove_prog_excalibur},
	{"death_prog_excalibur", death_prog_excalibur},
	{"speech_prog_excalibur", speech_prog_excalibur},
	{"sac_prog_excalibur", sac_prog_excalibur},
	{"fight_prog_sub_weapon", fight_prog_sub_weapon},
	{"speech_prog_kassandra", speech_prog_kassandra},
	{"fight_prog_chaos_blade", fight_prog_chaos_blade},
	{"death_prog_chaos_blade", death_prog_chaos_blade},
	{"fight_prog_tattoo_atum_ra", fight_prog_tattoo_atum_ra},
	{"fight_prog_tattoo_zeus", fight_prog_tattoo_zeus},
	{"fight_prog_tattoo_siebele", fight_prog_tattoo_siebele},
	{"fight_prog_tattoo_ahuramazda", fight_prog_tattoo_ahuramazda},
	{"fight_prog_tattoo_shamash", fight_prog_tattoo_shamash},
	{"fight_prog_tattoo_ehrumen", fight_prog_tattoo_ehrumen},
	{"fight_prog_tattoo_venus", fight_prog_tattoo_venus},
	{"fight_prog_tattoo_deimos", fight_prog_tattoo_deimos},
	{"fight_prog_tattoo_odin", fight_prog_tattoo_odin},
	{"fight_prog_tattoo_phobos", fight_prog_tattoo_phobos},
	{"fight_prog_tattoo_teshub", fight_prog_tattoo_teshub},
	{"fight_prog_tattoo_ares", fight_prog_tattoo_ares},
	{"fight_prog_tattoo_goktengri", fight_prog_tattoo_goktengri},
	{"fight_prog_tattoo_hera", fight_prog_tattoo_hera},
	{"fight_prog_tattoo_seth", fight_prog_tattoo_seth},
	{"fight_prog_tattoo_enki", fight_prog_tattoo_enki},
	{"fight_prog_tattoo_eros", fight_prog_tattoo_eros},
	{"fight_prog_tattoo_sirala", fight_prog_tattoo_sirala},
	{"fight_prog_golden_weapon", fight_prog_golden_weapon},
	{"death_prog_golden_weapon", death_prog_golden_weapon},
	{"fight_prog_breaker", fight_prog_swordbreaker},
	{"get_prog_heart", get_prog_heart},
	{"wear_prog_bracer", wear_prog_bracer},
	{"remove_prog_bracer", remove_prog_bracer},
	{"wear_prog_ranger_staff", wear_prog_ranger_staff},
	{"fight_prog_ranger_staff", fight_prog_ranger_staff},
	{"death_prog_ranger_staff", death_prog_ranger_staff},
	{"wear_prog_coconut", wear_prog_coconut},
	{"entry_prog_coconut", entry_prog_coconut},
	{"greet_prog_coconut", greet_prog_coconut},
	{"get_prog_coconut", get_prog_coconut},
	{"remove_prog_coconut", remove_prog_coconut},
	{"fight_prog_firegauntlets", fight_prog_firegauntlets},
	{"wear_prog_firegauntlets", wear_prog_firegauntlets},
	{"remove_prog_firegauntlets", remove_prog_firegauntlets},
	{"fight_prog_armbands", fight_prog_armbands},
	{"wear_prog_armbands", wear_prog_armbands},
	{"remove_prog_armbands", remove_prog_armbands},
	{"fight_prog_demonfireshield", fight_prog_demonfireshield},
	{"wear_prog_demonfireshield", wear_prog_demonfireshield},
	{"remove_prog_demonfireshield", remove_prog_demonfireshield},
	{"fight_prog_vorpalblade", fight_prog_vorpalblade},
	{"get_prog_spec_weapon", get_prog_spec_weapon},
	{"get_prog_quest_obj", get_prog_quest_obj},
	{"fight_prog_shockwave", fight_prog_shockwave},
	{"fight_prog_snake", fight_prog_snake},
	{"drop_prog_dice", drop_prog_dice},
	{"wear_prog_wind_boots", wear_prog_wind_boots},
	{"remove_prog_wind_boots", remove_prog_wind_boots},
	{"wear_prog_arm_hercules", wear_prog_arm_hercules},
	{"remove_prog_arm_hercules", remove_prog_arm_hercules},
	{"wear_prog_girdle_giant", wear_prog_girdle_giant},
	{"remove_prog_girdle_giant", remove_prog_girdle_giant},
	{"wear_prog_breastplate_strength", wear_prog_breastplate_strength},
	{"remove_prog_breastplate_strength", remove_prog_breastplate_strength},
	{"wear_prog_boots_flying", wear_prog_boots_flying},
	{"remove_prog_boots_flying", remove_prog_boots_flying},
	{"fight_prog_rose_shield", fight_prog_rose_shield},
	{"fight_prog_lion_claw", fight_prog_lion_claw},
	{"speech_prog_ring_ra", speech_prog_ring_ra},
	{"wear_prog_eyed_sword", wear_prog_eyed_sword},
	{"wear_prog_ruler_shield", wear_prog_ruler_shield},
	{"wear_prog_katana_sword", wear_prog_katana_sword},
	{"wear_prog_snake", wear_prog_snake},
	{"remove_prog_snake", remove_prog_snake},
	{"get_prog_snake", get_prog_snake},
	{"wear_prog_fire_shield", wear_prog_fire_shield},
	{"remove_prog_fire_shield", remove_prog_fire_shield},
	{"wear_prog_quest_weapon", wear_prog_quest_weapon},
	{"get_prog_quest_reward", get_prog_quest_reward},
	{"speech_prog_deathlure", speech_prog_deathlure},
	{"speech_prog_azurethword", speech_prog_azurethword},
	{NULL}
};

int optype_lookup(const char *name)
{
	int i;

	for (i = 0; optype_table[i] != NULL; i++)
		if (str_cmp(optype_table[i], name) == 0)
			return i;
	return -1;
}

int oprog_call(int optype, OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (obj->pIndexData->oprogs && obj->pIndexData->oprogs[optype] != NULL)
		return (obj->pIndexData->oprogs[optype]) (obj, ch, arg);
	return 0;
}

OPROG_DATA *oprog_lookup(const char *name)
{
	OPROG_DATA *p;

	for (p = oprog_table; p->name != NULL; p++)
		if (str_cmp(p->name, name) == 0)
			return p;
	return NULL;
}

char *oprog_name_lookup(OPROG_FUN * fn)
{
	OPROG_DATA *p;

	for (p = oprog_table; p->name != NULL; p++)
		if (p->fn == fn)
			break;

	return p->name;
}

void oprog_set(OBJ_INDEX_DATA * pObjIndex, const char *progtype,
	       const char *name)
{
	int opindex;
	OPROG_DATA *oprog;

	opindex = optype_lookup(progtype);
	if (opindex == -1) {
		log_printf("oprog_set: vnum %d: unknown obj prog type `%s'",
			   pObjIndex->vnum, progtype);
		exit(1);
	}

	oprog = oprog_lookup(name);
	if (oprog == NULL) {
		log_printf("oprog_set: vnum %d: unknown obj prog `%s'",
			   pObjIndex->vnum, name);
		exit(1);
	}

	if (pObjIndex->oprogs == NULL)
		pObjIndex->oprogs = calloc(1, sizeof(*pObjIndex->oprogs) *
					   OPROG_MAX);
	pObjIndex->oprogs[opindex] = oprog->fn;
}

int wear_prog_excalibur(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	act("$p begins to shine a bright {Wwhite{x.", ch, obj, NULL, TO_CHAR);
	act("$p begins to shine a bright {Wwhite{x.", ch, obj, NULL, TO_ROOM);

	obj->level = ch->level + 10;
	autoweapon(obj, 110);

	return 0;
}

int wear_prog_bracer(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	AFFECT_DATA af;

	if (!is_affected(ch, gsn_haste)) {
		char_puts
		    ("As you slide your arms into these bracers, they mold to your skin.\n",
		     ch);
		char_puts("Your hands and arms feel incredibly light.\n", ch);

		af.where = TO_AFFECTS;
		af.type = gsn_haste;
		af.duration = -2;
		af.level = ch->level;
		af.bitvector = AFF_HASTE;
		af.location = APPLY_DEX;
		af.modifier =
		    1 + (ch->level >= 18) + (ch->level >= 30) + (ch->level >=
								 45);
		affect_to_char(ch, &af);
	}
	return 0;
}

int remove_prog_bracer(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (is_affected(ch, gsn_haste)) {
		affect_strip(ch, gsn_haste);
		char_puts("Your hands and arms feel heavy again.\n", ch);
	}
	return 0;
}


int remove_prog_excalibur(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	act("$p stops glowing.", ch, obj, NULL, TO_CHAR);
	act("$p stops glowing.", ch, obj, NULL, TO_ROOM);
	return 0;
}

bool death_prog_excalibur(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (number_percent() < 10) {
		act_puts("$p starts to glow with a {Bblue{x aura.",
			 ch, obj, NULL, TO_CHAR, POS_DEAD);
		act("$p starts to glow with a {Bblue{x aura,",
		    ch, obj, NULL, TO_ROOM);
		heal(ch, NULL, ch, gsn_assist, ch->level, ch->max_hit / 5);
		return TRUE;
	} else
		return FALSE;
}

int speech_prog_excalibur(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	char *speech = (char *) arg;

	if (!str_cmp(speech, "one land, one king!")
	    && (ch->fighting)
	    && ((get_eq_char(ch, WEAR_WIELD) == obj))) {
		act("A {Wwhite{x light runs down $p's blade and coats $n.",
		    ch, obj, NULL, TO_ROOM);
		act("A {Wwhite{x light runs down $p's blade and coats you.",
		    ch, obj, NULL, TO_CHAR);
		spell_assist(gsn_assist, ch->level - 5, ch, ch, TARGET_CHAR);
		WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
	}
	return 0;
}

bool sac_prog_excalibur(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	act("The gods are infuriated!", ch, NULL, NULL, TO_CHAR);
	act("The gods are infuriated!", ch, NULL, NULL, TO_ROOM);
	damage(ch, ch,
	       (ch->hit - 1) > 1000 ? 1000 : (ch->hit - 1),
	       TYPE_HIT, DAM_HOLY, TRUE);
	ch->gold = 0;
	return TRUE;
}

int fight_prog_ranger_staff(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if ((get_eq_char(ch, WEAR_WIELD) == obj
	     || get_eq_char(ch, WEAR_SECOND_WIELD) == obj)
	    && number_percent() < 10) {
		act("Your $p glows {Cblue{x!", ch, obj, NULL, TO_CHAR);
		act("$n's $p glows {Cblue{x!", ch, obj, NULL, TO_ROOM);
		heal(ch, obj, ch, gsn_cure_critical, ch->level,
		     dice(3, 8) + ch->level / 2);
	}
	return 0;
}

int fight_prog_sub_weapon(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_WIELD) == obj && number_percent() < 30) {
		if (((float) ch->hit) / ((float) ch->max_hit) > 0.9)
			char_puts
			    ("Your weapon whispers, 'You're doing great!'\n",
			     ch);
		else if (((float) ch->hit) / ((float) ch->max_hit) > 0.6)
			char_puts
			    ("Your weapon whispers, 'Keep up the good work!'\n",
			     ch);
		else if (((float) ch->hit) / ((float) ch->max_hit) > 0.4)
			char_puts("Your weapon whispers, 'You can do it!'\n",
				  ch);
		else
			char_puts
			    ("Your weapon whispers, 'Run away! Run away!'\n",
			     ch);
	}
	return 0;
}

bool death_prog_ranger_staff(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	char_puts("Your ranger's staff disappears.\n", ch);
	act("$n's ranger's staff disappears.", ch, NULL, NULL, TO_ROOM);
	extract_obj(obj, 0);
	return 0;
}

int get_prog_spec_weapon(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (IS_OWNER(ch, obj)) {
		if (IS_AFFECTED(ch, AFF_POISON) && (dice(1, 5) == 1)) {
			char_puts("Your weapon glows blue.", ch);
			act("$n's weapon glows blue.", ch, NULL, NULL, TO_ROOM);
			spell_cure_poison(gsn_cure_poison, 30, ch, ch,
					  TARGET_CHAR);
			return 0;
		}

		if (IS_AFFECTED(ch, AFF_CURSE) && (dice(1, 5) == 1)) {
			char_puts("Your weapon glows blue.", ch);
			act("$n's weapon glows blue.", ch, NULL, NULL, TO_ROOM);
			spell_remove_curse(gsn_remove_curse, 30, ch, ch,
					   TARGET_CHAR);
			return 0;
		}
		char_puts("Your weapon's humming gets lauder.\n", ch);
		return 0;
	}

	act("You are zapped by $p and drop it.", ch, obj, NULL, TO_CHAR);

	obj_from_char(obj);
	obj_to_room(obj, ch->in_room);

	switch (dice(1, 10)) {
	case 1:
		spell_curse(gsn_curse, ch->level < 10 ? 1 : ch->level - 9,
			    ch, ch, TARGET_CHAR);
		break;
	case 2:
		spell_poison(gsn_poison, ch->level < 10 ? 1 : ch->level - 9,
			     ch, ch, TARGET_CHAR);
		break;
	}
	return 0;
}

int get_prog_quest_obj(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (IS_OWNER(ch, obj)) {
		if (IS_AFFECTED(ch, AFF_POISON) && (dice(1, 5) == 1)) {
			act("$p glows blue.", ch, obj, NULL, TO_ROOM);
			act_puts("$p glows blue.", ch, obj, NULL, TO_CHAR,
				 POS_DEAD);
			spell_cure_poison(gsn_cure_poison, 30,
					  ch, ch, TARGET_CHAR);
			return 0;
		}

		char_puts("Quest staff waits patiently to return.\n", ch);
		return 0;
	}

	if (!IS_IMMORTAL(ch)) {
		act("You are zapped by $p and drop it.", ch, obj, NULL,
		    TO_CHAR);

		obj_from_char(obj);
		obj_to_room(obj, ch->in_room);

		switch (dice(1, 10)) {
		case 1:
			spell_curse(gsn_curse,
				    ch->level < 10 ? 1 : ch->level - 9, ch, ch,
				    TARGET_CHAR);
			break;
		case 2:
			spell_poison(gsn_poison,
				     ch->level < 10 ? 1 : ch->level - 9, ch, ch,
				     TARGET_CHAR);
			break;
		}
	}
	return 0;
}

int speech_prog_kassandra(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	char *speech = (char *) arg;

	if (!str_cmp(speech, "kassandra") && (get_eq_char(ch, WEAR_HOLD) == obj)
	    && !IS_NPC(ch))
		obj_cast_spell(gsn_kassandra, ch->level, ch, ch, NULL);

	else if (!str_cmp(speech, "sebat")
		 && (get_eq_char(ch, WEAR_HOLD) == obj)
		 && !IS_NPC(ch))
		obj_cast_spell(gsn_sebat, ch->level, ch, ch, NULL);

	else if (!str_cmp(speech, "matandra")
		 && (get_eq_char(ch, WEAR_HOLD) == obj)
		 && (ch->fighting) && !IS_NPC(ch)) {
		act("A blast of energy bursts from your hand toward $N!",
		    ch, NULL, ch->fighting, TO_CHAR);
		act("A blast of energy bursts from $n's hand toward you!",
		    ch, NULL, ch->fighting, TO_VICT);
		act("A blast of energy bursts from $n's hand toward $N!",
		    ch, NULL, ch->fighting, TO_NOTVICT);
		obj_cast_spell(gsn_matandra, ch->level, ch, ch->fighting, NULL);
	}
	return 0;
}

int fight_prog_chaos_blade(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_WIELD) != obj
	    && get_eq_char(ch, WEAR_SECOND_WIELD) != obj)
		return 0;

	switch (number_bits(7)) {
	case 0:
		act("The chaotic blade screams loudly!",
		    ch, NULL, NULL, TO_ROOM);
		char_puts
		    ("The chaos blade screams loudly but your opponent isn't affected!\n",
		     ch);
		WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
		break;

	case 1:
		act("The chaotic blade shakes a bit.", ch, NULL, NULL, TO_ROOM);
		char_puts("Your chaotic blade shakes a bit.\n", ch);
		obj_cast_spell(gsn_garble, ch->level, ch, ch->fighting, obj);
		WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
		break;

	case 2:
		act("The chaotic blade shivers uncontrollably!",
		    ch, NULL, NULL, TO_ROOM);
		char_puts("Your chaotic blade shivers uncontrollably!\n", ch);
		obj_cast_spell(gsn_confuse, ch->level, ch, ch->fighting, obj);
		WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
		break;
	}
	return 0;
}

bool death_prog_chaos_blade(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	char_puts("Your chaotic blade disappears.\n", ch);
	act("$n's chaotic blade disappears.", ch, NULL, NULL, TO_ROOM);
	extract_obj(obj, 0);
	return 0;
}

int fight_prog_tattoo_atum_ra(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) != obj)
		return 0;

	switch (number_bits(6)) {
	case 0:
	case 1:
		char_puts("The tattoo on your shoulder glows {Cblue{x.\n", ch);
		spell_cure_serious(gsn_cure_serious, ch->level,
				   ch, ch, TARGET_CHAR);
		break;
	case 2:
		char_puts("The tattoo on your shoulder glows {Rred{x.\n", ch);
		do_yell(ch, "Ever dance with good....");
		spell_holy_word(sn_lookup("holy word"), ch->level,
				ch, NULL, TARGET_NONE);
		break;
	}
	return 0;
}


int fight_prog_tattoo_zeus(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) != obj)
		return 0;

	switch (number_bits(10)) {
	case 0:
	case 1:
	case 2:
		char_puts("The tattoo on your shoulder glows {Cblue{x.\n", ch);
		spell_cure_critical(gsn_cure_critical, ch->level,
				    ch, ch, TARGET_CHAR);
		break;
	case 3:
		char_puts("The tattoo on your shoulder glows {Cblue{x.\n", ch);
		if (IS_AFFECTED(ch, AFF_PLAGUE))
			spell_cure_disease(sn_lookup("cure disease"),
					   MAX_LEVEL, ch, ch, TARGET_CHAR);
		if (IS_AFFECTED(ch, AFF_POISON))
			spell_cure_poison(gsn_cure_poison,
					  MAX_LEVEL, ch, ch, TARGET_CHAR);
		break;
	}
	return 0;
}

int fight_prog_tattoo_siebele(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) != obj)
		return 0;

	switch (number_bits(6)) {
	case 0:
		char_puts("The tattoo on your shoulder glows {Cblue{x.\n", ch);
		spell_cure_serious(gsn_cure_serious, ch->level,
				   ch, ch, TARGET_CHAR);
		break;
	case 1:
		char_puts("The tattoo on your shoulder glows {Rred{x.\n", ch);
		spell_bluefire(gsn_bluefire, ch->level,
			       ch, ch->fighting, TARGET_CHAR);
		break;
	}
	return 0;
}

int fight_prog_tattoo_ahuramazda(OBJ_DATA * obj, CHAR_DATA * ch,
				 const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) != obj)
		return 0;

	switch (number_bits(6)) {
	case 0:
		char_puts("The tattoo on your shoulder glows {Cblue{x.\n", ch);
		spell_cure_serious(gsn_cure_serious, ch->level,
				   ch, ch, TARGET_CHAR);
		break;
	case 1:
		char_puts("The tattoo on your shoulder glows {Rred{x.\n", ch);
		spell_dispel_evil(sn_lookup("dispel evil"), ch->level,
				  ch, ch->fighting, TARGET_CHAR);
		break;
	}
	return 0;
}

int fight_prog_tattoo_shamash(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) != obj)
		return 0;

	switch (number_bits(6)) {
	case 0:
	case 1:
		char_puts("The tattoo on your shoulder glows {Cblue{x.\n", ch);
		spell_cure_serious(gsn_cure_serious, ch->level,
				   ch, ch, TARGET_CHAR);
		break;
	case 2:
		char_puts("The tattoo on your shoulder glows {Rred{x.\n", ch);
		do_yell(ch, "And justice for all!....");
		spell_scream(gsn_scream, ch->level,
			     ch, ch->fighting, TARGET_CHAR);
		break;
	}
	return 0;
}

int fight_prog_tattoo_ehrumen(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) != obj)
		return 0;

	switch (number_bits(5)) {
	case 0:
		char_puts("The tattoo on your shoulder glows {Cblue{x.\n", ch);
		spell_cure_critical(gsn_cure_light, ch->level,
				    ch, ch, TARGET_CHAR);
		break;
	case 1:
		char_puts("The tattoo on your shoulder glows {Rred{x.\n", ch);
		spell_demonfire(gsn_demonfire, ch->level - 5,
				ch, ch->fighting, TARGET_CHAR);
		break;
	}
	return 0;
}

int fight_prog_tattoo_venus(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) != obj)
		return 0;

	switch (number_bits(7)) {
	case 0:
	case 1:
	case 2:
		char_puts("The tattoo on your shoulder glows {Cblue{x.\n", ch);
		act_puts("The tattoo on your shoulder glows blue.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);
		spell_cure_light(gsn_cure_light, ch->level,
				 ch, ch, TARGET_CHAR);
		break;
	case 3:
		char_puts("The tattoo on your shoulder glows {Rred{x.\n", ch);
		spell_plague(gsn_plague, ch->level,
			     ch, ch->fighting, TARGET_CHAR);
		break;
	case 4:
		char_puts("The tattoo on your shoulder glows {Cblue{x.\n", ch);
		spell_bless(gsn_bless, ch->level, ch, ch, TARGET_CHAR);
		break;
	}
	return 0;
}

int fight_prog_tattoo_seth(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) != obj)
		return 0;

	switch (number_bits(5)) {
	case 0:
		char_puts("The tattoo on your shoulder glows {Cblue{x.\n", ch);
		spell_dragon_strength(gsn_dragon_strength, ch->level,
				      ch, ch, TARGET_CHAR);
		break;
	case 1:
		char_puts("The tattoo on your shoulder glows {Rred{x.\n", ch);
		spell_dragon_breath(gsn_dragon_breath, ch->level,
				    ch, ch->fighting, TARGET_CHAR);
		break;
	}
	return 0;
}


int fight_prog_tattoo_odin(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) != obj)
		return 0;

	switch (number_bits(5)) {
	case 0:
		char_puts("The tattoo on your shoulder glows {Cblue{x.\n", ch);
		spell_cure_critical(gsn_cure_critical, ch->level,
				    ch, ch, TARGET_CHAR);
		break;
	case 1:
		char_puts("The tattoo on your shoulder glows {Rred{x.\n", ch);
		spell_faerie_fire(gsn_faerie_fire, ch->level,
				  ch, ch->fighting, TARGET_CHAR);
		break;
	}
	return 0;
}

int fight_prog_tattoo_phobos(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) != obj)
		return 0;

	switch (number_bits(6)) {
	case 0:
		char_puts("The tattoo on your shoulder glows {Cblue{x.\n", ch);
		spell_cure_serious(gsn_cure_serious, ch->level,
				   ch, ch, TARGET_CHAR);
		break;
	case 1:
		char_puts("The tattoo on your shoulder glows {Rred{x.\n", ch);
		spell_colour_spray(sn_lookup("colour spray"), ch->level,
				   ch, ch->fighting, TARGET_CHAR);
		break;
	}
	return 0;
}

int fight_prog_tattoo_teshub(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) != obj)
		return 0;

	switch (number_bits(7)) {
	case 0:
		char_puts("The tattoo on your shoulder glows {Rred{x.\n", ch);
		spell_blindness(gsn_blindness, ch->level,
				ch, ch->fighting, TARGET_CHAR);
		char_puts("You send out a cloud of confusion!\n", ch);
		break;
	case 1:
		char_puts("The tattoo on your shoulder glows {Rred{x.\n", ch);
		spell_poison(gsn_poison, ch->level,
			     ch, ch->fighting, TARGET_CHAR);
		char_puts("Some of your insanity rubs off on your opponent.\n",
			  ch);
		break;
	case 2:
		spell_haste(gsn_haste, ch->level, ch, ch, TARGET_CHAR);
		char_puts("You suddenly feel more hyperactive!\n", ch);
		break;
	case 3:
		spell_shield(gsn_shield, ch->level, ch, ch, TARGET_CHAR);
		char_puts("You feel even more paranoid!\n", ch);
		break;
	}
	return 0;
}

int fight_prog_tattoo_ares(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	AFFECT_DATA af;

	if (get_eq_char(ch, WEAR_TATTOO) != obj)
		return 0;

	if (number_percent() < 50) {
		switch (number_bits(4)) {
		case 0:
			if (IS_AFFECTED(ch, AFF_BERSERK)
			    || is_affected(ch, gsn_berserk)
			    || is_affected(ch, gsn_frenzy)) {
				char_puts("You get a little madder.\n", ch);
				return 0;
			}

			af.where = TO_AFFECTS;
			af.type = gsn_berserk;
			af.level = ch->level;
			af.duration = ch->level / 3;
			af.modifier = ch->level / 5;
			af.bitvector = AFF_BERSERK;

			af.location = APPLY_HITROLL;
			affect_to_char(ch, &af);

			af.location = APPLY_DAMROLL;
			affect_to_char(ch, &af);

			af.modifier = 10 * (ch->level / 10);
			af.location = APPLY_AC;
			affect_to_char(ch, &af);

			ch->hit += ch->level * 2;
			ch->hit = UMIN(ch->hit, ch->max_hit);

			char_puts("Your pulse races as you are consumned "
				  "by rage!\n", ch);
			act("$n gets a wild look in $s eyes.",
			    ch, NULL, NULL, TO_ROOM);
			break;
		}
	} else {
		switch (number_bits(4)) {
		case 0:
			do_yell(ch, "Cry Havoc and Let Loose the Dogs of War!");
			break;
		case 1:
			do_yell(ch, "No Mercy!");
			break;
		case 2:
			do_yell(ch, "Los Valdar Cuebiyari!");
			break;
		case 3:
			do_yell(ch, "Carai an Caldazar! Carai an Ellisande! "
				"Al Ellisande!");
			break;
		case 4:
			do_yell(ch, "Siempre Vive el Riesgo!");
			break;
		}
	}
	return 0;
}

int fight_prog_tattoo_hera(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) != obj)
		return 0;

	switch (number_bits(10)) {
	case 0:
		char_puts("The tattoo on your shoulder glows {Rred{x.\n", ch);
		spell_plague(gsn_plague, ch->level,
			     ch, ch->fighting, TARGET_CHAR);
		break;
	case 1:
		char_puts("The tattoo on your shoulder glows {Rred{x.\n", ch);
		spell_poison(gsn_poison, ch->level,
			     ch, ch->fighting, TARGET_CHAR);
		/* FALLTHRU */
	case 2:
		char_puts("The tattoo on your shoulder glows {Rred{x.\n", ch);
		spell_weaken(gsn_weaken, ch->level,
			     ch, ch->fighting, TARGET_CHAR);
		/* FALLTHRU */
	case 3:
		char_puts("The tattoo on your shoulder glows {Rred{x.\n", ch);
		spell_slow(gsn_slow, ch->level, ch, ch->fighting, TARGET_CHAR);
		break;
	}
	return 0;
}


int fight_prog_tattoo_deimos(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) != obj)
		return 0;

	switch (number_bits(6)) {
	case 0:
	case 1:
		char_puts("The tattoo on your shoulder glows {Cblue{x.\n", ch);
		spell_cure_serious(gsn_cure_serious, ch->level,
				   ch, ch, TARGET_CHAR);
		break;
	case 2:
		char_puts("The tattoo on your shoulder glows {Rred{x.\n", ch);
		spell_web(gsn_web, ch->level, ch, ch->fighting, TARGET_CHAR);
		break;
	}
	return 0;
}

int fight_prog_tattoo_enki(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) == obj)
		switch (number_bits(8)) {
		case 0:
			act_puts("The tattoo on your shoulder glows {Cblue{x.",
				 ch, NULL, NULL, TO_CHAR, POS_DEAD);
			spell_cure_critical(gsn_cure_critical, ch->level,
					    ch, ch, TARGET_CHAR);
			break;
		case 1:
			act_puts("The tattoo on your shoulder glows {Rred{x.",
				 ch, NULL, NULL, TO_CHAR, POS_DEAD);

			if (IS_EVIL(ch->fighting) && !IS_EVIL(ch))
				spell_dispel_evil(sn_lookup("dispel evil"),
						  ch->level * 12 / 10,
						  ch, ch->fighting,
						  TARGET_CHAR);
			else if (IS_GOOD(ch->fighting) && !IS_GOOD(ch))
				spell_dispel_good(sn_lookup("dispel good"),
						  ch->level * 12 / 10,
						  ch, ch->fighting,
						  TARGET_CHAR);
			else
				spell_lightning_bolt(sn_lookup
						     ("lightning bolt"),
						     ch->level * 12 / 10, ch,
						     ch->fighting, TARGET_CHAR);
			break;
		}
	return 0;
}

int fight_prog_tattoo_eros(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) != obj)
		return 0;

	switch (number_bits(5)) {
	case 0:
	case 1:
		char_puts("The tattoo on your shoulder glows {Cblue{x.\n", ch);
		spell_heal(sn_lookup("heal"), ch->level, ch, ch, TARGET_CHAR);
		break;
	case 2:
		char_puts("The tattoo on your shoulder glows {Cblue{x.\n", ch);
		spell_group_heal(sn_lookup("group heal"), ch->level,
				 ch, ch, TARGET_CHAR);
		break;
	}
	return 0;
}

int fight_prog_tattoo_sirala(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) == obj)
		switch (number_bits(6)) {
		case 0:
		case 1:
		case 2:

			if (IS_EVIL(ch->fighting) && !IS_EVIL(ch)) {
				act_puts
				    ("The tattoo on your shoulder shines like {Ymorning{x.",
				     ch, NULL, NULL, TO_CHAR, POS_DEAD);
				spell_dispel_evil(sn_lookup("dispel evil"),
						  ch->level * 12 / 10, ch,
						  ch->fighting, TARGET_CHAR);
			} else if (IS_GOOD(ch->fighting) && !IS_GOOD(ch)) {
				act_puts
				    ("The tattoo on your shoulder darkens like {Dnight{x.",
				     ch, NULL, NULL, TO_CHAR, POS_DEAD);
				spell_dispel_good(sn_lookup("dispel good"),
						  ch->level * 12 / 10, ch,
						  ch->fighting, TARGET_CHAR);
			} else {
				act_puts
				    ("The tattoo on your shoulder settles to {wgrey{x.",
				     ch, NULL, NULL, TO_CHAR, POS_DEAD);
				spell_cure_critical(gsn_cure_critical,
						    ch->level, ch, ch,
						    TARGET_CHAR);
			}
			break;
		}
	return 0;
}

bool death_prog_golden_weapon(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	char_puts("Your golden weapon disappears.\n", ch);
	act("$n's golden weapon disappears.", ch, NULL, NULL, TO_ROOM);
	extract_obj(obj, 0);
	return 0;
}

int fight_prog_golden_weapon(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if ((get_eq_char(ch, WEAR_WIELD) == obj) ||
	    (get_eq_char(ch, WEAR_SECOND_WIELD) == obj)) {
		if (number_percent() < 4) {
			act("Your $p glows bright blue!\n", ch, obj, NULL,
			    TO_CHAR);
			act("$n's $p glows bright blue!", ch, obj, NULL,
			    TO_ROOM);

			obj_cast_spell(gsn_cure_serious, ch->level, ch, ch,
				       obj);
			return 0;
		} else if (number_percent() > 92) {
			act("Your $p glows bright blue!\n", ch, obj, NULL,
			    TO_CHAR);
			act("$n's $p glows bright blue!", ch, obj, NULL,
			    TO_ROOM);

			obj_cast_spell(gsn_cure_light, ch->level, ch, ch, obj);
		}
	}
	return 0;
}

int get_prog_heart(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (obj->timer == 0)
		obj->timer = 24;
	return 0;
}

int fight_prog_snake(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if ((get_eq_char(ch, WEAR_WIELD) == obj) ||
	    (get_eq_char(ch, WEAR_SECOND_WIELD) == obj)) {
		switch (number_bits(7)) {
		case 0:
			act("One of the snake heads on your whip bites $N!", ch,
			    NULL, ch->fighting, TO_CHAR);
			act("A snake from $n's whip strikes out and bites you!",
			    ch, NULL, ch->fighting, TO_VICT);
			act("One of the snakes from $n's whip strikes at $N!",
			    ch, NULL, ch->fighting, TO_NOTVICT);
			obj_cast_spell(gsn_poison, ch->level, ch, ch->fighting,
				       obj);
			break;
		case 1:
			act("One of the snake heads on your whip bites $N!", ch,
			    NULL, ch->fighting, TO_CHAR);
			act("A snake from $n's whip strikes out and bites you!",
			    ch, NULL, ch->fighting, TO_VICT);
			act("One of the snakes from $n's whip strikes at $N!",
			    ch, NULL, ch->fighting, TO_NOTVICT);
			obj_cast_spell(gsn_weaken, ch->level, ch, ch->fighting,
				       obj);
			break;
		}
	}
	return 0;
}

int fight_prog_shockwave(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if ((get_eq_char(ch, WEAR_WIELD) == obj) ||
	    (get_eq_char(ch, WEAR_SECOND_WIELD) == obj))
		switch (number_bits(6)) {
		case 0:
			act("A bolt of lightning arcs out from your bolt, hitting $N!", ch, NULL, ch->fighting, TO_CHAR);
			act("A bolt of lightning crackles along $n's bolt and arcs towards you!", ch, NULL, ch->fighting, TO_VICT);
			act("A bolt of lightning shoots out from $n's bolt, arcing towards $N!", ch, NULL, ch->fighting, TO_NOTVICT);
			obj_cast_spell(gsn_lightning_bolt, ch->level, ch,
				       ch->fighting, NULL);
			break;
		}
	return 0;
}

int wear_prog_ranger_staff(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	class_t *cl;

	if (!IS_IMMORTAL(ch)
	    && ((cl = class_lookup(ch->class)) == NULL
		|| str_cmp(cl->name, "ranger"))) {
		char_puts("You don't know to use this thing.\n", ch);
		unequip_char(ch, obj);
		char_puts("Ranger staff slides off from your hand.\n", ch);
		obj_from_char(obj);
		obj_to_room(obj, ch->in_room);
	}
	return 0;
}

int wear_prog_coconut(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	act("You start to bang the coconut shells together.", ch, NULL, NULL,
	    TO_CHAR);
	act("You hear a sound like horses galloping and you mount your steed.",
	    ch, NULL, NULL, TO_CHAR);
	act("$n pretends to mount an invisible horse.", ch, NULL, NULL,
	    TO_ROOM);
	return 0;
}

int entry_prog_coconut(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (obj->carried_by != NULL)
		if (get_eq_char(obj->carried_by, WEAR_HOLD) == obj)
			act("$n gallops in on his invisible steed, banging two coconuts together.", obj->carried_by, NULL, NULL, TO_ROOM);
	return 0;
}

int greet_prog_coconut(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (obj->carried_by != NULL) {
		if (get_eq_char(obj->carried_by, WEAR_HOLD) == obj &&
		    obj->carried_by != ch)
			act("You hear the sound of galloping horses.", ch, NULL,
			    NULL, TO_CHAR);
	} else
		char_puts
		    ("$p beckons with the faint sound of galloping horses.\n",
		     ch);
	return 0;
}

int get_prog_coconut(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	char_puts("You hold the coconut up to your ear and suddenly you hear "
		  "the faint\nroar of galloping horses.\n", ch);
	act("$n holds a coconut up to $s ear.", ch, NULL, NULL, TO_ROOM);
	return 0;
}

int remove_prog_coconut(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	char_puts("The sounds of horses fade away.\n", ch);
	act("$n pretends to dismount a horse.", ch, NULL, NULL, TO_ROOM);
	return 0;
}

int fight_prog_firegauntlets(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	int dam;

	if (!(get_eq_char(ch, WEAR_WIELD) == NULL &&
	      get_eq_char(ch, WEAR_SECOND_WIELD) == NULL))
		return 0;

	if (get_eq_char(ch, WEAR_HANDS) != obj)
		return 0;
	if (IS_NPC(ch))
		return 0;

	if (number_percent() < 20) {
		dam = number_percent() / 2 + 30 + 2 * ch->level;
		act("Your gauntlets burns $N's face!", ch, NULL, ch->fighting,
		    TO_CHAR);
		act("$n's gauntlets burns $N's face!", ch, NULL, ch->fighting,
		    TO_NOTVICT);
		act("$N's gauntlets burns your face!", ch->fighting, NULL, ch,
		    TO_CHAR);
		damage(ch, ch->fighting, dam, gsn_burning_hands, DAM_FIRE,
		       TRUE);
		if (ch == NULL || ch->fighting == NULL)
			return 0;
		fire_effect(ch->fighting, obj->level / 2, dam / 2, TARGET_CHAR);
	}
	return 0;
}

int wear_prog_firegauntlets(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	char_puts("Your hands warm up by the gauntlets.\n", ch);
	return 0;
}

int remove_prog_firegauntlets(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	char_puts("Your hands cool down.\n", ch);
	return 0;
}

int fight_prog_armbands(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	int dam;
	if (get_eq_char(ch, WEAR_ARMS) != obj)
		return 0;

	if (IS_NPC(ch))
		return 0;

	if (number_percent() < 20) {
		dam = number_percent() / 2 + 30 + 5 * ch->level;
		act("Your armbands burns $N's face!", ch, NULL, ch->fighting,
		    TO_CHAR);
		act("$n's armbands burns $N's face!", ch, NULL, ch->fighting,
		    TO_NOTVICT);
		act("$N's armbands burns your face!", ch->fighting, NULL, ch,
		    TO_CHAR);
		damage(ch, ch->fighting, dam, gsn_burning_hands, DAM_FIRE,
		       TRUE);
		if (ch == NULL || ch->fighting == NULL)
			return 0;
		fire_effect(ch->fighting, obj->level / 2, dam, TARGET_CHAR);
	}
	return 0;
}

int wear_prog_armbands(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	char_puts("Your arms warm up by the armbands of the volcanoes.\n", ch);
	return 0;
}

int remove_prog_armbands(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	char_puts("Your arms cool down again.\n", ch);
	return 0;
}

int fight_prog_demonfireshield(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	int dam;

	if (get_eq_char(ch, WEAR_SHIELD) != obj)
		return 0;
	if (IS_NPC(ch))
		return 0;

	if (number_percent() < 15) {
		dam = number_percent() / 2 + (ch->level / 20) * ch->level;
		act("A magical hole appears in your shield !", ch, NULL,
		    ch->fighting, TO_CHAR);
		act("Your shield burns $N's face!", ch, NULL, ch->fighting,
		    TO_CHAR);
		act("$n's shield burns $N's face!", ch, NULL, ch->fighting,
		    TO_NOTVICT);
		act("$N's shield burns your face!", ch->fighting, NULL, ch,
		    TO_CHAR);
		damage(ch, ch->fighting, dam, gsn_demonfire, DAM_FIRE, TRUE);
		if (ch == NULL || ch->fighting == NULL)
			return 0;
		fire_effect(ch->fighting, obj->level, dam, TARGET_CHAR);
	}
	return 0;
}

int wear_prog_demonfireshield(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	char_puts("Your hands warm up by the fire shield.\n", ch);
	return 0;
}

int remove_prog_demonfireshield(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	char_puts("Your hands cool down.\n", ch);
	return 0;
}

int fight_prog_vorpalblade(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	CHAR_DATA *victim;

	if (IS_NPC(ch))
		return 0;

	if ((get_eq_char(ch, WEAR_WIELD) != obj)
	    || (get_eq_char(ch, WEAR_SECOND_WIELD) != obj))
		return 0;

	victim = ch->fighting;
	if (number_percent() < 5 && !IS_IMMORTAL(victim)) {
		char_puts("Your weapon swings at your victim's neck "
			  "without your control!\n", ch);
		if (number_percent() < 40) {
			act("It makes an huge arc in the air, chopping $N's head OFF!", ch, NULL, victim, TO_CHAR);
			act("$N's weapon whistles in the air, chopping your head OFF!", ch, NULL, victim, TO_NOTVICT);
			act("$n's weapon whistles in the air, chopping $N's head OFF!", ch, NULL, victim, TO_ROOM);
			act("$n is DEAD!!", victim, NULL, NULL, TO_ROOM);
			act("$n is DEAD!!", victim, NULL, NULL, TO_CHAR);
			handle_death(ch, victim);
			char_puts("You have been KILLED!!\n", victim);
		}
	}
	return 0;
}

int wear_prog_wind_boots(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	AFFECT_DATA af;

	if (!is_affected(ch, gsn_fly)) {
		char_puts
		    ("As you wear wind boots on your feet, they hold you up.\n",
		     ch);
		char_puts("You start to fly.\n", ch);

		af.where = TO_AFFECTS;
		af.type = gsn_fly;
		af.duration = -2;
		af.level = ch->level;
		af.bitvector = AFF_FLYING;
		af.location = 0;
		af.modifier = 0;
		affect_to_char(ch, &af);
	}
	return 0;
}

int remove_prog_wind_boots(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (is_affected(ch, gsn_fly)) {
		affect_strip(ch, gsn_fly);
		char_puts("You fall down to the ground.\n", ch);
		char_puts("Ouch!.\n", ch);
	}
	return 0;
}

int wear_prog_boots_flying(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	AFFECT_DATA af;

	if (!is_affected(ch, gsn_fly)) {
		char_puts
		    ("As you wear boots of flying on your feet, they hold you up.\n",
		     ch);
		char_puts("You start to fly.\n", ch);

		af.where = TO_AFFECTS;
		af.type = gsn_fly;
		af.duration = -2;
		af.level = ch->level;
		af.bitvector = AFF_FLYING;
		af.location = 0;
		af.modifier = 0;
		affect_to_char(ch, &af);
	}
	return 0;
}

int remove_prog_boots_flying(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (is_affected(ch, gsn_fly)) {
		affect_strip(ch, gsn_fly);
		char_puts("You fall down to the ground.\n", ch);
		char_puts("You start to walk again instead of flying!.\n", ch);
	}
	return 0;
}

int wear_prog_arm_hercules(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	AFFECT_DATA af;

	if (!is_affected(ch, gsn_giant_strength)) {
		char_puts
		    ("As you wear your arms these plates, You feel your self getting stronger.\n",
		     ch);
		char_puts("Your muscles seems incredibly huge.\n", ch);

		af.where = TO_AFFECTS;
		af.type = gsn_giant_strength;
		af.duration = -2;
		af.level = ch->level;
		af.bitvector = 0;
		af.location = APPLY_STR;
		af.modifier =
		    1 + (ch->level >= 18) + (ch->level >= 30) + (ch->level >=
								 45);
		affect_to_char(ch, &af);
	}
	return 0;
}

int remove_prog_arm_hercules(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (is_affected(ch, gsn_giant_strength)) {
		affect_strip(ch, gsn_giant_strength);
		char_puts("Your muscles regain its original value.\n", ch);
	}
	return 0;
}

int wear_prog_girdle_giant(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	AFFECT_DATA af;

	if (!is_affected(ch, gsn_giant_strength)) {
		char_puts
		    ("As you wear this girdle, You feel your self getting stronger.\n",
		     ch);
		char_puts("Your muscles seems incredibly huge.\n", ch);

		af.where = TO_AFFECTS;
		af.type = gsn_giant_strength;
		af.duration = -2;
		af.level = ch->level;
		af.bitvector = 0;
		af.location = APPLY_STR;
		af.modifier =
		    1 + (ch->level >= 18) + (ch->level >= 30) + (ch->level >=
								 45);
		affect_to_char(ch, &af);
	}
	return 0;
}

int remove_prog_girdle_giant(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (is_affected(ch, gsn_giant_strength)) {
		affect_strip(ch, gsn_giant_strength);
		char_puts("Your muscles regain its original value.\n", ch);
	}
	return 0;
}

int wear_prog_breastplate_strength(OBJ_DATA * obj, CHAR_DATA * ch,
				   const void *arg)
{
	AFFECT_DATA af;

	if (!is_affected(ch, gsn_giant_strength)) {
		char_puts
		    ("As you wear breastplate of strength, You feel yourself getting stronger.\n",
		     ch);
		char_puts("Your muscles seems incredibly huge.\n", ch);

		af.where = TO_AFFECTS;
		af.type = gsn_giant_strength;
		af.duration = -2;
		af.level = ch->level;
		af.bitvector = 0;
		af.location = APPLY_STR;
		af.modifier =
		    1 + (ch->level >= 18) + (ch->level >= 30) + (ch->level >=
								 45);
		affect_to_char(ch, &af);
	}
	return 0;
}

int remove_prog_breastplate_strength(OBJ_DATA * obj, CHAR_DATA * ch,
				     const void *arg)
{
	if (is_affected(ch, gsn_giant_strength)) {
		affect_strip(ch, gsn_giant_strength);
		char_puts("Your muscles regain its original value.\n", ch);
	}
	return 0;
}

int fight_prog_rose_shield(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (!((ch->in_room->sector_type != SECT_FIELD) ||
	      (ch->in_room->sector_type != SECT_FOREST) ||
	      (ch->in_room->sector_type != SECT_MOUNTAIN) ||
	      (ch->in_room->sector_type != SECT_HILLS)))
		return 0;

	if (get_eq_char(ch, WEAR_SHIELD) != obj)
		return 0;

	if (number_percent() < 90)
		return 0;

	char_puts("The leaves of your shield grows suddenly.\n", ch);
	char_puts("The leaves of shield surrounds you!.\n", ch->fighting);
	act("$n's shield of rose grows suddenly.", ch, NULL, NULL, TO_ROOM);
	obj_cast_spell(gsn_slow, ch->level, ch, ch->fighting, NULL);
	return 0;
}

int fight_prog_lion_claw(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (number_percent() < 90)
		return 0;

	if ((obj == get_eq_char(ch, WEAR_WIELD)) ||
	    (obj == get_eq_char(ch, WEAR_SECOND_WIELD))) {
		char_puts("The nails of your claw appears from its fingers.\n",
			  ch);
		act_puts("The nails of $n's claw appears for an instant.", ch,
			 NULL, NULL, TO_ROOM, POS_DEAD);
		one_hit(ch, ch->fighting, TYPE_HIT, WEAR_WIELD);
		one_hit(ch, ch->fighting, TYPE_HIT, WEAR_WIELD);
		one_hit(ch, ch->fighting, TYPE_HIT, WEAR_WIELD);
		one_hit(ch, ch->fighting, TYPE_HIT, WEAR_WIELD);
		char_puts("The nails of your claw disappears.\n", ch);
		act_puts("the nails of $n's claw disappears suddenly.",
			 ch, NULL, NULL, TO_ROOM, POS_DEAD);
	}
	return 0;
}

int speech_prog_ring_ra(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	char *speech = (char *) arg;

	if (!str_cmp(speech, "punish")
	    && (ch->fighting) &&
	    ((get_eq_char(ch, WEAR_FINGER_L) == obj)
	     || (get_eq_char(ch, WEAR_FINGER_R)))) {
		char_puts("An electrical arc sprays from the ring.\n", ch);
		act("An electrical arc sprays from the ring.", ch, NULL, NULL,
		    TO_ROOM);
		obj_cast_spell(gsn_lightning_breath, ch->level, ch,
			       ch->fighting, NULL);
		WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
	}
	return 0;
}

int wear_prog_eyed_sword(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	act("$p's eye opens.", ch, obj, NULL, TO_CHAR);
	act("$p's eye opens.", ch, obj, NULL, TO_ROOM);

	obj->level = ch->level + 10;
	autoweapon(obj, 110);

	return 0;
}

int wear_prog_katana_sword(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	obj->value[ITEM_WEAPON_TYPE] = WEAPON_KATANA;
	if (obj->pIndexData->item_type == ITEM_WEAPON
	    && IS_WEAPON_STAT(obj, WEAPON_KATANA_QUEST)
	    && IS_OWNER(ch, obj)) {
		obj->level = ch->level + 15;
		autoweapon(obj, 110);
		char_puts("Your katana feels like an extension of yourself.\n",
			  ch);

	}
	return 0;
}

int fight_prog_tattoo_goktengri(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (get_eq_char(ch, WEAR_TATTOO) == obj)
		switch (number_bits(4)) {
		case 0:
		case 1:
			if (is_affected(ch, gsn_dishonor)) {
				act_puts
				    ("The tattoo on your shoulder smolders with {rpain{x!",
				     ch, NULL, NULL, TO_CHAR, POS_DEAD);
				damage(ch, ch, ch->level / 10, gsn_dishonor,
				       DAM_OTHER, TRUE);
			} else {
				act_puts
				    ("The tattoo on your shoulder glows white.",
				     ch, NULL, NULL, TO_CHAR, POS_DEAD);
				do_say(ch, "My honour is my life.");
				one_hit(ch, ch->fighting, TYPE_UNDEFINED,
					WEAR_WIELD);
			}
			break;
		}
	return 0;
}

int wear_prog_snake(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	act_puts("Snakes of whip starts to breath a poisonous air.",
		 ch, obj, NULL, TO_CHAR, POS_DEAD);
	act_puts("Snakes of whip starts to breath a poisonous air.",
		 ch, obj, NULL, TO_ROOM, POS_DEAD);
	if (ch->level <= 10)
		obj->value[ITEM_WEAPON_DICE_SIZE] = 3;
	else if (ch->level > 10 && ch->level <= 20)
		obj->value[ITEM_WEAPON_DICE_SIZE] = 4;
	else if (ch->level > 20 && ch->level <= 30)
		obj->value[ITEM_WEAPON_DICE_SIZE] = 5;
	else if (ch->level > 30 && ch->level <= 40)
		obj->value[ITEM_WEAPON_DICE_SIZE] = 6;
	else if (ch->level > 40 && ch->level <= 50)
		obj->value[ITEM_WEAPON_DICE_SIZE] = 7;
	else if (ch->level > 50 && ch->level <= 60)
		obj->value[ITEM_WEAPON_DICE_SIZE] = 8;
	else if (ch->level > 60 && ch->level <= 70)
		obj->value[ITEM_WEAPON_DICE_SIZE] = 9;
	else if (ch->level > 70 && ch->level <= 80)
		obj->value[ITEM_WEAPON_DICE_SIZE] = 10;
	else
		obj->value[ITEM_WEAPON_DICE_SIZE] = 11;
	return 0;
}


int remove_prog_snake(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	act_puts("Snakes of whip slowly melds to non-living skin.",
		 ch, obj, NULL, TO_CHAR, POS_DEAD);
	act_puts("Snakes of whip slowy melds to non-living skin.",
		 ch, obj, NULL, TO_ROOM, POS_DEAD);
	return 0;
}

int get_prog_snake(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	act("You feel as if snakes of whip moved.", ch, obj, NULL, TO_CHAR);
	return 0;
}

int wear_prog_fire_shield(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	AFFECT_DATA af;

	if (strstr(mlstr_mval(obj->ed->description), "cold") != NULL) {
		if (!is_affected(ch, gsn_fire_shield)) {
			char_puts
			    ("As you wear shield, you become resistive to cold.\n",
			     ch);

			af.where = TO_RESIST;
			af.type = gsn_fire_shield;
			af.duration = -2;
			af.level = ch->level;
			af.bitvector = RES_COLD;
			af.location = 0;
			af.modifier = 0;
			affect_to_char(ch, &af);
		}
	} else {
		if (!is_affected(ch, gsn_fire_shield)) {
			char_puts
			    ("As you wear shield, you become resistive to fire.\n",
			     ch);

			af.where = TO_RESIST;
			af.type = gsn_fire_shield;
			af.duration = -2;
			af.level = ch->level;
			af.bitvector = RES_FIRE;
			af.location = 0;
			af.modifier = 0;
			affect_to_char(ch, &af);
		}
	}
	return 0;
}

int remove_prog_fire_shield(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (is_affected(ch, gsn_fire_shield)) {
		affect_strip(ch, gsn_fire_shield);
		if (strstr(mlstr_mval(obj->ed->description), "cold") != NULL)
			char_puts("You have become normal to cold attacks.\n",
				  ch);
		else
			char_puts("You have become normal to fire attacks.\n",
				  ch);
	}
	return 0;
}

int wear_prog_quest_weapon(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (IS_OWNER(ch, obj)) {
		act_puts("Your weapon starts glowing.",
			 ch, NULL, NULL, TO_CHAR, POS_DEAD);

		obj->level = ch->level + 10;
		autoweapon(obj, 100);
		obj->level = ch->level;
		return 0;
	}

	if (!IS_IMMORTAL(ch)) {
		act("You are zapped by $p and drop it.", ch, obj, NULL,
		    TO_CHAR);

		obj_from_char(obj);
		obj_to_room(obj, ch->in_room);
	}
	return 0;
}

int get_prog_quest_reward(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	if (IS_OWNER(ch, obj) || IS_IMMORTAL(ch)) {
		act_puts("Your $p starts glowing.",
			 ch, obj, NULL, TO_CHAR, POS_SLEEPING);
		return 0;
	}

	act("You are zapped by $p and drop it.", ch, obj, NULL, TO_CHAR);
	act("$n is zapped by $p and drops it.", ch, obj, NULL, TO_ROOM);

	obj_from_char(obj);
	obj_to_room(obj, ch->in_room);
	return 0;
}

int wear_prog_ruler_shield(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	clan_t *clan = clan_lookup(ch->clan);

	if (!IS_IMMORTAL(ch) && (!clan || str_cmp(clan->name, "ruler"))) {
		act("You are zapped by $p and drop it.",
		    ch, obj, NULL, TO_CHAR);
		act("$n is zapped by $p and drops it.", ch, obj, NULL, TO_ROOM);
		obj_from_char(obj);
		obj_to_room(obj, ch->in_room);
	}

	return 0;
}

int fight_prog_swordbreaker(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	CHAR_DATA *victim;
	OBJ_DATA *wield;
	victim = ch->fighting;
	if ((wield = get_eq_char(victim, WEAR_WIELD)) == NULL)
		return 0;
	if ((wield->value[ITEM_WEAPON_TYPE] == WEAPON_LONGSWORD
	     || wield->value[ITEM_WEAPON_TYPE] == WEAPON_BASTARDSWORD
	     || wield->value[ITEM_WEAPON_TYPE] == WEAPON_KATANA
	     || wield->value[ITEM_WEAPON_TYPE] == WEAPON_SHORTSWORD)
	    && (get_eq_char(ch, WEAR_WIELD) == obj
		|| get_eq_char(ch, WEAR_SECOND_WIELD) == obj)
	    && number_percent() < 10) {
		act("You {Wcleaved{x $N's sword into two.",
		    ch, NULL, victim, TO_CHAR);
		act("$n {Wcleaved{x your sword into two.",
		    ch, NULL, victim, TO_VICT);
		act("$n {Wcleaved{x $N's sword into two.",
		    ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, gsn_weapon_cleave, TRUE, 1);
		extract_obj(get_eq_char(victim, WEAR_WIELD), 0);
	}
	return 0;
}





int speech_prog_deathlure(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	char *speech = (char *) arg;
	if (!str_cmp(speech, "consume") && (ch->fighting)
	    && (get_eq_char(ch, WEAR_HOLD) == obj) && !IS_NPC(ch)) {
		act("A sticky flaming substance shoots out of {rDea{Rth{r's {MLure{x toward $N!", ch, NULL, ch->fighting, TO_CHAR);
		act("A sticky flaming substance shoots out of {rDea{Rth{r's {MLure{x toward you!", ch, NULL, ch->fighting, TO_VICT);
		act("A sticky flaming substance shoots out of {rDea{Rth{r's {MLure{x toward $N!", ch, NULL, ch->fighting, TO_NOTVICT);
		obj_cast_spell(gsn_faerie_fire, ch->level, ch, ch->fighting,
			       NULL);
	}
	return 0;
}

int speech_prog_azurethword(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	char *speech = (char *) arg;
	if (!str_cmp(speech, "ignite") && (!ch->fighting)
	    && (get_eq_char(ch, WEAR_LIGHT) == obj) && !IS_NPC(ch)) {
		if (IS_SET(ch->in_room->room_flags, ROOM_NORECALL)
		    || IS_AFFECTED(ch, AFF_CURSE)
		    || IS_RAFFECTED(ch->in_room, RAFF_CURSE)) {
			char_puts("Spell failed.\n", ch);
			return 0;
		}
		act("You disappear in a cloud of smoke!",
		    ch, NULL, NULL, TO_CHAR);
		act("$n disappears in a cloud of smoke!",
		    ch, NULL, NULL, TO_ROOM);
		char_from_room(ch);
		char_to_room(ch, get_room_index(28040));
		act("$n appears in an explosion of smoke and flame!",
		    ch, NULL, NULL, TO_ROOM);
		if (!JUST_KILLED(ch))
			do_look(ch, "auto");
	}
	return 0;
}

int drop_prog_dice(OBJ_DATA * obj, CHAR_DATA * ch, const void *arg)
{
	int count = 35;
	if (number_percent() > (100 - 100 / (count--))) {
		act("You roll three grims, for a score of 12!", ch, NULL, NULL,
		    TO_CHAR);
		act("$n rolls three grims, for a score of 12!", ch, NULL, NULL,
		    TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll two grims and one {rd{Ra{rg{Rg{re{Rr{x, for a score of 11!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls two grims and one {rd{Ra{rg{Rg{re{Rr{x, for a score of 11!", ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll two {Chorses{x and one {Wcross{x, for a score of 1.", ch, NULL, NULL, TO_CHAR);
		act("$n rolls two {Chorses{x and one {Wcross{x, for a score of 1.", ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll three {Chorses{x, for an incredible score of ZERO!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls three {Chorses{x, for an incredible score of ZERO!", ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one {Dc{Wo{wi{Dn{x and two grims for a score of 10!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {Dc{Wo{wi{Dn{x and two grims for a score of 10!", ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll two {rd{Ra{rg{Rg{re{Rr{rs{x and one grim for a score of 10!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls two {rd{Ra{rg{Rg{re{Rr{rs{x and one grim for a score of 10!", ch, NULL, NULL, TO_ROOM);
	}

	else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one {Chorse{x and two {Wcrosses{x for a score of 2!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {Chorse{x and two {Wcrosses{x for a score of 2!", ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll two {Chorses{x and one {Dc{Wo{wi{Dn{x for a score of 2!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls two {Chorses{x and one {Dc{Wo{wi{Dn{x for a score of 2!", ch, NULL, NULL, TO_ROOM);
	}

	else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll three {Wcrosses{x, for a score of 3!", ch, NULL,
		    NULL, TO_CHAR);
		act("$n rolls three {Wcrosses{x, for a score of 3!", ch, NULL,
		    NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one {Chorse{x, one {Wcross{x and one {Dc{Wo{wi{Dn{x, for a score of 3!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {Chorse{x, one {Wcross{x and one {Dc{Wo{wi{Dn{x, for a score of 3!", ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one {rd{Ra{rg{Rg{re{Rr{x and two {Chorses{x, for a score of 3!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {rd{Ra{rg{Rg{re{Rr{x and two {Chorses{x, for a score of 3!", ch, NULL, NULL, TO_ROOM);
	}

	else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one {Wcross{x and two grims, for a score of 9!",
		    ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {Wcross{x and two grims, for a score of 9!",
		    ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll three {rd{Ra{rg{Rg{re{Rr{rs{x for a score of 9!",
		    ch, NULL, NULL, TO_CHAR);
		act("$n rolls three {rd{Ra{rg{Rg{re{Rr{rs{x for a score of 9!",
		    ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one {Dc{Wo{wi{Dn{x, one {rd{Ra{rg{Rg{re{Rr{x and one grim, for a score of 9!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {Dc{Wo{wi{Dn{x, one {rd{Ra{rg{Rg{re{Rr{x and one grim, for a score of 9!", ch, NULL, NULL, TO_ROOM);
	}

	else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one {Chorse{x and two grim, for a score of 8!",
		    ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {Chorse{x and two grim, for a score of 8!",
		    ch, NULL, NULL, TO_ROOM);
	}

	else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one {Dc{Wo{wi{Dn{x and two {rd{Ra{rg{Rg{re{Rr{rs{x for a score of 8!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {Dc{Wo{wi{Dn{x and two {rd{Ra{rg{Rg{re{Rr{rs{x for a score of 8!", ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll two {Dc{Wo{wi{Dns{x and one grim, for a score of 8!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls two {Dc{Wo{wi{Dns{x and one grim, for a score of 8!", ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one {Wcross{x, one {rd{Ra{rg{Rg{re{Rr{x and one grim for a score of 8!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {Wcross{x, one {rd{Ra{rg{Rg{re{Rr{x and one grim for a score of 8!", ch, NULL, NULL, TO_ROOM);
	}

	else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one {Wcross{x and two {rd{Ra{rg{Rg{re{Rr{rs{x, for a score of 7!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {Wcross{x and two {rd{Ra{rg{Rg{re{Rr{rs{x, for a score of 7!", ch, NULL, NULL, TO_ROOM);
	}

	else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one {Chorse{x, one {rd{Ra{rg{Rg{re{Rr{x and one grim, for a score of 7!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {Chorse{x, one {rd{Ra{rg{Rg{re{Rr{x and one grim, for a score of 7!", ch, NULL, NULL, TO_ROOM);
	}

	else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll two {Dc{Wo{wi{Dns{x and one {rd{Ra{rg{Rg{re{Rr{x, for a score of 7!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls two {Dc{Wo{wi{Dns{x and one {rd{Ra{rg{Rg{re{Rr{x, for a score of 7!", ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one {Wcross{x, one {Dc{Wo{wi{Dn{x and one grim, for a score of 7!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {Wcross{x, one {Dc{Wo{wi{Dn{x and one grim, for a score of 7!", ch, NULL, NULL, TO_ROOM);
	}

	else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one {Chorse{x, one {Wcross{x and one grim, for a score of 5!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {Chorse{x, one {Wcross{x and one grim, for a score of 5!", ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one {Chorse{x, one {Dc{Wo{wi{Dn{x and one {rd{Ra{rg{Rg{re{Rr{x, for a score of 5!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {Chorse{x, one {Dc{Wo{wi{Dn{x and one {rd{Ra{rg{Rg{re{Rr{x, for a score of 5!", ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll two {Dc{Wo{wi{Dns{x and one {Wcross{x, for a score of 5!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls two {Dc{Wo{wi{Dns{x and one {Wcross{x, for a score of 5!", ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll two {Wcrosses{x and one {rd{Ra{rg{Rg{re{Rr{x, for a score of 5!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls two {Wcrosses{x and one {rd{Ra{rg{Rg{re{Rr{x, for a score of 5!", ch, NULL, NULL, TO_ROOM);
	}

	else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll two {Dc{Wo{wi{Dns{x and one {Chorse{x, for a score of 4!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls two {Dc{Wo{wi{Dns{x and one {Chorse{x, for a score of 4!", ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one {Wcross{x, one {rd{Ra{rg{Rg{re{Rr{x and one {Chorse{x, for a score of 4!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {Wcross{x, one {rd{Ra{rg{Rg{re{Rr{x and one {Chorse{x, for a score of 4!", ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one grim and two {Chorses{x, for a score of 4!",
		    ch, NULL, NULL, TO_CHAR);
		act("$n rolls one grim and two {Chorses{x, for a score of 4!",
		    ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one {Dc{Wo{wi{Dn{x and two {Wcrosses{x, for a score of 4!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {Dc{Wo{wi{Dn{x and two {Wcrosses{x, for a score of 4!", ch, NULL, NULL, TO_ROOM);
	}

	else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one grim, one {Dc{Wo{wi{Dn{x and one {Chorse{x, for a score of 6!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls one grim, one {Dc{Wo{wi{Dn{x and one {Chorse{x, for a score of 6!", ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll one grim and two {Wcrosses{x, for a score of 6!",
		    ch, NULL, NULL, TO_CHAR);
		act("$n rolls one grim and two {Wcrosses{x, for a score of 6!",
		    ch, NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll three {Dc{Wo{wi{Dns{x, for a score of 6!", ch,
		    NULL, NULL, TO_CHAR);
		act("$n rolls three {Dc{Wo{wi{Dns{x, for a score of 6!", ch,
		    NULL, NULL, TO_ROOM);
	} else if (number_percent() > (100 - 100 / (count--))) {
		act("You roll two {rd{Ra{rg{Rg{re{Rr{rs{x and one {Chorse{x, for a score of 6!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls two {rd{Ra{rg{Rg{re{Rr{rs{x and one {Chorse{x, for a score of 6!", ch, NULL, NULL, TO_ROOM);
	} else {
		act("You roll one {Dc{Wo{wi{Dn{x, one {rd{Ra{rg{Rg{re{Rr{x and one {Wcross{x, for a score of 6!", ch, NULL, NULL, TO_CHAR);
		act("$n rolls one {Dc{Wo{wi{Dn{x, one {rd{Ra{rg{Rg{re{Rr{x and one {Wcross{x, for a score of 6!", ch, NULL, NULL, TO_ROOM);
	}
	return 1;
}
