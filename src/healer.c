/*
 * $Id: healer.c 871 2006-05-31 04:08:21Z zsuzsu $
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
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,	   *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *									   *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael	   *
 *  Chastain, Michael Quan, and Mitchell Tse.				   *
 *									   *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc	   *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.						   *
 *									   *
 *  Much time and thought has gone into this software and you are	   *
 *  benefitting.  We hope that you share your changes too.  What goes	   *
 *  around, comes around.						   *
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
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "debug.h"
#include "fight.h"
#include "healer.h"

DECLARE_DO_FUN(do_say);

void do_heal(CHAR_DATA * ch, const char *argument)
{
	CHAR_DATA *mob;
	char arg[MAX_INPUT_LENGTH];
	int sn;
	int cost;
	SPELL_FUN *spell;
	char *words;
	float cost_multi = 1.0;

	/* check for healer */
	for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
		if (IS_NPC(mob) && IS_SET(mob->pIndexData->act, ACT_HEALER)
		    && (!mob->clan || mob->clan == ch->clan))
			break;

	if (mob == NULL) {
		char_puts("You can't do that here.\n", ch);
		return;
	}

	one_argument(argument, arg, sizeof(arg));

	if (ch->level < 10)
		cost_multi = 0.50;
	else
		cost_multi = 1.0 + ch->level / 30;

	if (arg[0] == '\0') {
		/* display price list */
		act("Healer offers the following spells.", ch, NULL, mob,
		    TO_CHAR);
		char_printf(ch,
			    "  {Clight    {w: {ccure light wounds    {Y%3.0f{x gold\n",
			    10 * cost_multi);
		char_printf(ch,
			    "  {Cserious  {w: {ccure serious wounds  {Y%3.0f{x gold\n",
			    15 * cost_multi);
		char_printf(ch,
			    "  {Ccritic   {w: {ccure critical wounds {Y%3.0f{x gold\n",
			    25 * cost_multi);
		char_printf(ch,
			    "  {Cheal     {w: {chealing spell        {Y%3.0f{x gold\n",
			    50 * cost_multi);
		char_printf(ch,
			    "  {Cblind    {w: {ccure blindness       {Y%3.0f{x gold\n",
			    20 * cost_multi);
		char_printf(ch,
			    "  {Cdisease  {w: {ccure disease         {Y%3.0f{x gold\n",
			    15 * cost_multi);
		char_printf(ch,
			    "  {Cpoison   {w: {ccure poison          {Y%3.0f{x gold\n",
			    25 * cost_multi);
		char_printf(ch,
			    "  {Cuncurse  {w: {cremove curse         {Y%3.0f{x gold\n",
			    50 * cost_multi);
		char_printf(ch,
			    "  {Crefresh  {w: {crestore movement     {Y%3.0f{x gold\n",
			    5 * cost_multi);
		char_printf(ch,
			    "  {Cmana     {w: {crestore mana         {Y%3.0f{x gold\n",
			    10 * cost_multi);
		char_printf(ch,
			    "  {Cmaster   {w: {cmaster heal spell    {Y%3.0f{x gold\n",
			    200 * cost_multi);
		char_printf(ch,
			    "  {Cenergize {w: {crestore 300 mana     {Y%3.0f{x gold\n",
			    200 * cost_multi);
		char_puts(" Type heal <type> to be healed.\n", ch);
		return;
	}

	if (!str_prefix(arg, "light")) {
		spell = spell_cure_light;
		sn = sn_lookup("cure light");
		words = "judicandus dies";
		cost = 1000 * cost_multi;
	}

	else if (!str_prefix(arg, "serious")) {
		spell = spell_cure_serious;
		sn = sn_lookup("cure serious");
		words = "judicandus gzfuajg";
		cost = 1500 * cost_multi;
	}

	else if (!str_prefix(arg, "critical")) {
		spell = spell_cure_critical;
		sn = sn_lookup("cure critical");
		words = "judicandus qfuhuqar";
		cost = 2500 * cost_multi;
	}

	else if (!str_prefix(arg, "heal")) {
		spell = spell_heal;
		sn = sn_lookup("heal");
		words = "pzar";
		cost = 5000 * cost_multi;
	}

	else if (!str_prefix(arg, "blindness")) {
		spell = spell_cure_blindness;
		sn = sn_lookup("cure blindness");
		words = "judicandus noselacri";
		cost = 2000 * cost_multi;
	}

	else if (!str_prefix(arg, "disease")) {
		spell = spell_cure_disease;
		sn = sn_lookup("cure disease");
		words = "judicandus eugzagz";
		cost = 1500 * cost_multi;
	}

	else if (!str_prefix(arg, "poison")) {
		spell = spell_cure_poison;
		sn = sn_lookup("cure poison");
		words = "judicandus sausabru";
		cost = 2500 * cost_multi;
	}

	else if (!str_prefix(arg, "uncurse") || !str_prefix(arg, "curse")) {
		spell = spell_remove_curse;
		sn = sn_lookup("remove curse");
		words = "candussido judifgz";
		cost = 5000 * cost_multi;
	}

	else if (!str_prefix(arg, "mana")) {
		spell = NULL;
		sn = -3;
		words = "candamira";
		cost = 1000 * cost_multi;
	}


	else if (!str_prefix(arg, "refresh") || !str_prefix(arg, "moves")) {
		spell = spell_refresh;
		sn = sn_lookup("refresh");
		words = "candusima";
		cost = 500 * cost_multi;
	}

	else if (!str_prefix(arg, "master")) {
		spell = spell_master_healing;
		sn = sn_lookup("master healing");
		words = "candastra nikazubra";
		cost = 20000 * cost_multi;
	}

	else if (!str_prefix(arg, "energize")) {
		spell = NULL;
		sn = -2;
		words = "energizer";
		cost = 20000 * cost_multi;
	}

	else {
		act("Healer does not offer that spell.  Type 'heal' for a list.", ch, NULL, mob, TO_CHAR);
		return;
	}

	if (cost > (ch->gold * 100 + ch->silver)) {
		act("You do not have that much gold.", ch, NULL, mob, TO_CHAR);
		return;
	}

	WAIT_STATE(ch, PULSE_VIOLENCE);

	deduct_cost(ch, cost);

	act("$n puts $s hands together and murmurs briefly.", mob, NULL, NULL,
	    TO_ROOM);
	if (sn == -2) {
		ch->mana += 300;
		ch->mana = UMIN(ch->mana, ch->max_mana);
		char_puts("A warm glow passes through you.\n", ch);
	}
	if (sn == -3) {
		ch->mana += dice(2, 8) + mob->level / 3;
		ch->mana = UMIN(ch->mana, ch->max_mana);
		char_puts("A warm glow passes through you.\n", ch);
	}

	if (sn < 0)
		return;

	spell(sn, mob->level, mob, ch, TARGET_CHAR);
}


/*
 * healing either from players or object
 * returns false if no healing performed
 *
 * level - level of the healing spell
 *
 * by Zsuzsu
 */
/* *INDENT-OFF* */
bool heal (CHAR_DATA *ch, OBJ_DATA *obj, CHAR_DATA *victim,
	int sn, int level, int heal)
{
	int  percent = 0;
	char *wounds, *power;
	char buf[MAX_STRING_LENGTH]; 
	AFFECT_DATA *condemnation = NULL;
	int condemnation_save = 0;

	if (IS_UNDEAD(victim)) {
		char_puts("Your flesh is not affected by healing powers.\n", ch);
		act_puts("$n is unphased by the healing powers.", 
			victim, NULL, ch, TO_ROOM, POS_DEAD);
		return FALSE;
	}

	if (victim->hit >= victim->max_hit) {
		char_puts("You overflow with good health.\n", victim);
		act_puts("$n overflows with good health.", 
			victim, NULL, ch, TO_ROOM, POS_DEAD);
		return FALSE;
	}

	/* 
	 * condemnation effect -
	 * 	healing is nullified or halved if saving throw is made
	 * 	so high level healers can overcome low level condemnation.
	 * 	NPC healing is drastically reduced to prevent the
	 * 	use of Aedar and large sums of cash to get around condemnation.
	 */
	if (!IS_IMMORTAL(ch) 
	&& (condemnation = affect_find(victim->affected, gsn_condemnation))) {
		condemnation_save = URANGE(5, 
			10 + (level - condemnation->level) * 5, 95);

		if (number_percent() < condemnation_save)
			heal /= IS_NPC(ch) ? 10 : 2;
		else {
			char_puts("Goose flesh forms on your ashen skin.\n", victim);
			act("Goose flesh forms on $n's skin for a moment!",
				victim, NULL, NULL, TO_ROOM);
			return FALSE;
		}

		/* those with delve notice the condemnation*/
		if (number_percent() < get_skill(ch, gsn_delve)
		|| (IS_NPC(ch) && IS_SET(ch->pIndexData->act, ACT_CLERIC))) {
			if (ch == victim)
				act("Your flesh seems to resist your healing powers!",
					ch, NULL, NULL, TO_CHAR);
			else 
				act("$N's flesh seems to resist your healing powers!",
					ch, NULL, victim, TO_CHAR);
		}
	}

	/* percent is the lesser of the damage or amount healed */
	if (victim->max_hit > 0) {
		percent = (victim->max_hit - victim->hit) * 100 
			/ victim->max_hit;
		percent = (percent > ((heal * 100) / victim->max_hit))
				? (heal * 100) / victim->max_hit
				: percent;
	}

	DEBUG(DEBUG_HEAL,
		"%s[%d] heals %d for %d%% of %s",
		ch->name, level, heal, percent, victim->name);

	if (percent >= 90)
		wounds = "{Ddeadly";
	else if (percent >= 80)
		wounds = "{rcritical";
	else if (percent >= 70)
		wounds = "{rsevere";
	else if (percent >= 60)
		wounds = "{rmajor";
	else if (percent >= 50)
		wounds = "{rheavy";
	else if (percent >= 40)
		wounds = "{Rmedium";
	else if (percent >= 30)
		wounds = "{Rlesser";
	else if (percent >= 20)
		wounds = "{Rminor";
	else if (percent >= 10)
		wounds = "{Rlight";
	else if (percent >= 5)
		wounds = "{Rsmall";
	else if (percent > 0)
		wounds = "{Rslight";
	else
		wounds = "{Rnascient";

	if (heal >= 1000) 
		power = "{Wb{Cl{cindi{Cn{Wg";
	else if (heal >= 500)
		power = "{cf{Ci{We{Cr{cy";
	else if (heal >= 300)
		power = "{Cintense";
	else if (heal >= 150)
		power = "{Cbr{Wi{Cght";
	else if (heal >= 100)
		power = "{Cgl{co{Cwing";
	else if (heal >= 75)
		power = "{Csh{ci{Cmm{ce{Cr{ci{Cng";
	else if (heal >= 50)
		power = "{cgli{Ctt{cering";
	else if (heal >= 25)
		power = "{csoft";
	else if (heal >= 15)
		power = "{bdim";
	else 
		power = "{bvague";

	if (sn == sn_lookup("aid")
	|| sn == sn_lookup("assist"))
		victim->hit += heal;
	else
		victim->hit = UMIN(victim->hit + heal, victim->max_hit);

	update_pos(victim);
	
	char_printf(victim, 
		"Your %s{x wounds radiate a %s{x aura and vanish.\n", 
		wounds, power);
	snprintf(buf, sizeof(buf),
		"$n's %s{x wounds radiate a %s{x aura and vanish.",
		wounds, power);
	act_puts(buf, victim, NULL, NULL, TO_ROOM, POS_RESTING);

	if (victim->hit > victim->max_hit) {
		char_puts("You brim with life!\n", victim);
		act_puts("$N brims with life.\n", 
			ch, NULL, victim, TO_ROOM, POS_DEAD);
	}
	else if (ch != victim 
		&& (HAS_SKILL(ch, gsn_delve)
		|| IS_IMMORTAL(ch)
		|| (IS_NPC(ch) && IS_SET(ch->pIndexData->act, ACT_CLERIC)))) {

		if (!HAS_SKILL(ch, gsn_delve)
		|| number_percent() <= get_skill(ch, gsn_delve)) {

			if (victim->max_hit > 0)
				percent = (100 * victim->hit) / victim->max_hit;
			else
				percent = -1;
		 
			if (percent >= 100)
				act_puts("{C$E is in perfect health{x.", 
					ch, NULL, victim, TO_CHAR, POS_DEAD);
			else if (percent >= 90)
				act_puts("{b$E has a few scratches{x.", 
					ch, NULL, victim, TO_CHAR, POS_DEAD);
			else if (percent >= 75)
				act_puts("{B$E has some small but disgusting cuts{x.", 
					ch, NULL, victim, TO_CHAR, POS_DEAD);
			else if (percent >= 50)
				act_puts("{G$E is covered with bleeding wounds{x.", 
					ch, NULL, victim, TO_CHAR, POS_DEAD);
			else if (percent >= 30)
				act_puts("{Y$E is gushing blood{x.", 
					ch, NULL, victim, TO_CHAR, POS_DEAD);
			else if (percent >= 15)
				act_puts("{M$E is writhing in agony{x.", 
					ch, NULL, victim, TO_CHAR, POS_DEAD);
			else if (percent >= 0)
				act_puts("{R$E is convulsing on the ground{x.", 
					ch, NULL, victim, TO_CHAR, POS_DEAD);
			else
				act_puts("{R$E is nearly dead{x.", 
					ch, NULL, victim, TO_CHAR, POS_DEAD);

			if (HAS_SKILL(ch, gsn_delve))
				check_improve(ch, gsn_delve, TRUE, 1);
		}
		else {
			if (HAS_SKILL(ch, gsn_delve))
				check_improve(ch, gsn_delve, FALSE, 2);
		}
	}
	else if (victim->hit == victim->max_hit) {
		char_puts("You feel whole again.\n", victim);
		act_puts("$N looks whole.\n", 
			ch, NULL, victim, TO_ROOM, POS_RESTING);
	}

	return TRUE;
}
/* *INDENT-ON* */
