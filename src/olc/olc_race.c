/* $Id: olc_race.c 849 2006-04-22 13:08:54Z zsuzsu $ */

/************************************************************************************
 *    Copyright 2004 Astrum Metaphora consortium                                    *
 *                                                                                  *
 *    Licensed under the Apache License, Version 2.0 (the "License");               *
 *    you may not use this file except in compliance with the License.              *
 *    You may obtain a copy of the License at                                       *
 *                                                                                  *
 *    http://www.apache.org/licenses/LICENSE-2.0                                    *
 *                                                                                  *
 *    Unless required by applicable law or agreed to in writing, software           *
 *    distributed under the License is distributed on an "AS IS" BASIS,             *
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.      *
 *    See the License for the specific language governing permissions and           *
 *    limitations under the License.                                                *
 *                                                                                  *
 ************************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>

#include "merc.h"
#include "olc.h"

#define EDIT_RACE(ch, race) (race = (race_t*) ch->desc->pEdit)

DECLARE_OLC_FUN(raceed_create   );
DECLARE_OLC_FUN(raceed_edit     );
DECLARE_OLC_FUN(raceed_touch    );
DECLARE_OLC_FUN(raceed_show     );
DECLARE_OLC_FUN(raceed_list     );

DECLARE_OLC_FUN(raceed_name     );
DECLARE_OLC_FUN(raceed_filename );
DECLARE_OLC_FUN(raceed_flags    );
DECLARE_OLC_FUN(raceed_parts    );
DECLARE_OLC_FUN(raceed_act      );
DECLARE_OLC_FUN(raceed_off      );
DECLARE_OLC_FUN(raceed_aff      );
DECLARE_OLC_FUN(raceed_form     );
DECLARE_OLC_FUN(raceed_immune   );
DECLARE_OLC_FUN(raceed_resist   );
DECLARE_OLC_FUN(raceed_vulnerable   );
DECLARE_OLC_FUN(raceed_size     );
DECLARE_OLC_FUN(raceed_sex      );
DECLARE_OLC_FUN(raceed_ac       );
DECLARE_OLC_FUN(raceed_dodge    );
DECLARE_OLC_FUN(raceed_damtype  );
DECLARE_OLC_FUN(raceed_align	);


DECLARE_OLC_FUN(raceed_spec_part);
DECLARE_OLC_FUN(raceed_love_sect);
DECLARE_OLC_FUN(raceed_hate_sect);

DECLARE_OLC_FUN(pcraceed_create);
DECLARE_OLC_FUN(pcraceed_skill    );
DECLARE_OLC_FUN(pcraceed_age      );

DECLARE_OLC_FUN(pcraceed_skill_add);
DECLARE_OLC_FUN(pcraceed_skill_del);

static DECLARE_VALIDATE_FUN(validate_name  );
static DECLARE_VALIDATE_FUN(validate_haspcdata  );

static bool touch_race(race_t *r);

olc_cmd_t olc_cmds_race[] =
{
    { 5, "create",	raceed_create,				},
    { 5, "edit",	raceed_edit,				},
    { 5, "touch",	raceed_touch,				},
    { 0, "show",	raceed_show,				},
    { 0, "list",        raceed_list,				},

    { 5, "name",        raceed_name,		validate_name		},
    { 5, "filename",    raceed_filename,	validate_filename	},
    { 5, "align",	raceed_align,					},
    { 5, "sex",		raceed_sex,		sex_table		},
    { 5, "size",	raceed_size,		size_table		},
    /*
    { 5, "ac",		raceed_armor,		armor_table		},
    { 5, "armor",	raceed_armor,		armor_table		},
    */
    { 5, "dodge",       raceed_dodge,				},
    { 5, "damtype",	raceed_damtype,				},
    /*
    { 5, "spec_part",  raceed_spec_part,5,                   },
    { 5, "love_sect",  raceed_love_sect,5, sector_flag_types },
    { 5, "hate_sect",  raceed_hate_sect,5, sector_flag_types },
    */
    { 5, "form",        raceed_form,	form_flags		},
    { 5, "part",        raceed_parts,	part_flags		},
    { 5, "imm",         raceed_immune,	imm_flags		},
    { 5, "res",         raceed_resist,	imm_flags		},
    { 5, "vuln",        raceed_vulnerable,	imm_flags	},
    { 5, "off",         raceed_off,	off_flags		},
    { 5, "act",         raceed_act,	act_flags		},
    { 5, "affect",      raceed_aff,	affect_flags		},

   /* { 9, "pcrace",      pcraceed_create,				},
    { 8, "age",		pcraceed_age,				},
    { 5, "skill",       pcraceed_skill,	validate_haspcdata	},
    */

    { 0, "commands",    show_commands,				},
    { 0, NULL}
};

OLC_FUN(raceed_create)
{
    int rn;
    race_t *race;

    if (ch->pcdata->security < SECURITY_RACES)
    {
        char_puts("RaceEd: Insufficient security for editing races\n", ch);
        return FALSE;
    }

    if (argument[0] == '\0')
    {
        do_help(ch, "'OLC CREATE'");
        return FALSE;
    }

    if ((rn = rn_lookup(argument)) >= 0)
    {
        char_printf(ch, "RaceEd: %s: already exists.\n", RACE(rn)->name);
        return FALSE;
    }

    race            = race_new();
    race->name      = str_dup(argument);
    race->file_name = str_printf("race%02d.race", races.nused-1);

    ch->desc->pEdit     = (void *)race;
    OLCED(ch)   = olced_lookup(ED_RACE);
    touch_race(race);
    char_puts("Race created.\n",ch);
    return FALSE;
}

OLC_FUN(raceed_edit)
{
    int rn;
    char arg[MAX_STRING_LENGTH];

        if (ch->pcdata->security < SECURITY_RACES)
    {
        char_puts("RaceEd: Insufficient security.\n", ch);
        return FALSE;
    }
    one_argument(argument, arg, sizeof(arg));
    if (arg[0] == '\0')
    {
        do_help(ch, "'OLC EDIT'");
        return FALSE;
    }
    if ((rn = rn_lookup(arg)) < 0)
    {
        char_printf(ch, "RaceEd: %s: No such race.\n", argument);
        return FALSE;
    }
    ch->desc->pEdit     = RACE(rn);
    OLCED(ch)   = olced_lookup(ED_RACE);
    return FALSE;
}

OLC_FUN(raceed_touch)
{
    race_t *race;
    EDIT_RACE(ch, race);
    return touch_race(race);
}


OLC_FUN(raceed_dodge)
{
    race_t *race;
    EDIT_RACE(ch, race);
    
    return olced_number(ch, argument, cmd, &race->dodge);
}

OLC_FUN(raceed_parts    )
{
    race_t *race;
    EDIT_RACE(ch, race);

    return olced_flag32(ch, argument, cmd, &race->parts);
}

OLC_FUN(raceed_act      )
{
    race_t *race;
    EDIT_RACE(ch, race);

    return olced_flag64(ch, argument, cmd, &race->act);
}

OLC_FUN(raceed_off      )
{
    race_t *race;
    EDIT_RACE(ch, race);

    return olced_flag32(ch, argument, cmd, &race->off);
}

OLC_FUN(raceed_aff      )
{
    race_t *race;
    EDIT_RACE(ch, race);

    return olced_flag64(ch, argument, cmd, &race->aff);
}

OLC_FUN(raceed_form     )
{
    race_t *race;
    EDIT_RACE(ch, race);

    return olced_flag32(ch, argument, cmd, &race->form);
}

OLC_FUN(raceed_immune   )
{
    race_t *race;
    EDIT_RACE(ch, race);

    return olced_flag32(ch, argument, cmd, &race->imm);
}

OLC_FUN(raceed_resist   )
{
    race_t *race;
    EDIT_RACE(ch, race);

    return olced_flag32(ch, argument, cmd, &race->res);
}

OLC_FUN(raceed_vulnerable   )
{
    race_t *race;
    EDIT_RACE(ch, race);

    return olced_flag32(ch, argument, cmd, &race->vuln);
}

OLC_FUN(raceed_show)
{
    char arg[MAX_STRING_LENGTH];
    int i;
    BUFFER *output;
    race_t *race;
    pcrace_t *pcr;

    one_argument(argument, arg, sizeof(arg));
    if (arg[0] == '\0')
    {
        if (IS_EDIT(ch, ED_RACE))
            EDIT_RACE(ch, race);
        else
        {
            do_help(ch, "'OLC ASHOW'");
            return FALSE;
        }
    }
    else
    {
        if ((i = rn_lookup(arg)) < 0)
        {
            char_printf(ch, "RaceEd: %s: No such race.\n",  argument);
            return FALSE;
        }
        race = RACE(i);
    }
    output = buf_new(ch->lang);

        buf_printf(output, "Name:          [%s]\n",    race->name);
        buf_printf(output, "Filename:      [%s]\n",    race->file_name);
	buf_printf(output, "Align:         [%4d]\n",race->alignment);
	buf_printf(output, "Sex:           [%s]\n", flag_string(sex_table, race->sex));
	buf_printf(output, "Size:          [%s]\n", flag_string(size_table, race->size));
        buf_printf(output, "Dodge Skill:   [%d] (NPC only)\n",    race->dodge);

	buf_printf(output, "DamType:       [%s]\n",	attack_table[race->dam_type].name);

        buf_printf(output, "Act:           [%s]\n",    flag_string(act_flags, race->act));
        buf_printf(output, "Aff:           [%s]\n",    flag_string(affect_flags, race->aff));
        buf_printf(output, "Off:           [%s]\n",    flag_string(off_flags, race->off));

        buf_printf(output, "Immune:        [%s]\n",    flag_string(imm_flags, race->imm));
        buf_printf(output, "Resists:       [%s]\n",    flag_string(imm_flags, race->res));
        buf_printf(output, "Vuln:          [%s]\n",    flag_string(imm_flags, race->vuln));
        buf_printf(output, "Body parts:    [%s]\n",    flag_string(part_flags, race->parts));
        buf_printf(output, "Flags:         [%s]\n",    flag_string(race_flags, race->flags));
    
    /*
    buf_printf (output, "Loves sectors: [%s]\n", flag_string(sector_flag_types, race->loved_sectors));
    buf_printf (output, "Hates sectors: [%s]\n", flag_string(sector_flag_types, race->hated_sectors));
    */

    
    /*
    for (i = 0; i < MAX_DAM; i++)
        if (race->resists[i])
            buf_printf(output, "Res:           [%-16.16s:%4d%%]\n", flag_string(dam_flags, i), race->resists[i]);
    
    for (i = 0; i < MAX_SPEC_PARTS; ++i)
        if (race->spec_parts[i] != 0)
            break;

    if (i < MAX_SPEC_PARTS)
    {
        OBJ_INDEX_DATA * obj;

        buf_add (output, "Special parts:\n");
        buf_add (output, "{x    Min. level Probability Object        :\n");
        for (i = 0; i < MAX_SPEC_PARTS; ++i)
        {
            obj = get_obj_index (race->spec_parts[i]);
            buf_printf (output, "{G%2d{x.   [%3d]       [%4d]   [%7d] %s\n", 
                i + 1,
                race->spec_levels[i],
                race->spec_prob[i],
                race->spec_parts[i], 
                obj == NULL ? "non-existent object" : mlstr_cval (obj->short_descr, ch));
        }
    }
    */
    if ((pcr = race->pcdata))
    {
        buf_printf(output, "\n{WPC data:{x\n\n");
        buf_printf(output, "ShortName:    [%s]\n", pcr->who_name);
        buf_printf(output, "Points:       [%d]\n", pcr->points);
        for (i = 0; i < pcr->classes.nused; i++) 
        {
             rclass_t *race_class = VARR_GET(&pcr->classes, i);
             int cn;

             if ((cn = cn_lookup(race_class->name)) < 0)
                        continue;
             buf_printf(output, "Class:        '%s' (mult %d)\n", CLASS(cn)->name, race_class->mult);
        }
        
	buf_printf(output, "BonusSkills   [%s]\n", pcr->bonus_skills);
        
        for (i = 0; i < pcr->skills.nused; i++) 
        {
             rskill_t *race_skill = VARR_GET(&pcr->skills, i);
             skill_t *skill;

             if (race_skill->sn <= 0  || (skill = skill_lookup(race_skill->sn)) == NULL)
             continue;
             buf_printf(output, "Skill:       '%s' (level %d)\n",
             
             skill->name, race_skill->level);
        }
        
        buf_printf(output, "Stats        ");
        for (i = 0; i < MAX_STATS; i++)         buf_printf(output, " %d", pcr->stats[i]);
        buf_printf(output, "\n");
        buf_printf(output, "MaxStats     ");
        for (i = 0; i < MAX_STATS; i++)         buf_printf(output, " %d", pcr->max_stats[i]);
        buf_printf(output, "\n");
	buf_printf(output, "Size:         [%s]\n", flag_string(size_table, pcr->size));
        buf_printf(output, "HPBonus:      [%d]\n", pcr->hp_bonus);
        buf_printf(output, "ManaBonus:    [%d]\n", pcr->mana_bonus);
        buf_printf(output, "PracBonus:    [%d]\n", pcr->prac_bonus);
        buf_printf(output, "RestrictAlign [%s]\n", flag_string(ralign_names, pcr->restrict_align));
        buf_printf(output, "RestrictEthos [%s]\n", flag_string(ethos_table, pcr->restrict_ethos));
	/*
        if (pcr->restrict_sex)     buf_printf(output, "RestrictSex   [%s]\n", flag_string(restrict_sex_table, pcr->restrict_sex));
	*/
        buf_printf(output, "Slang         [%s]\n", flag_string(slang_table, pcr->slang));
	buf_printf(output, "Flags:        [%s]",  flag_string(race_flags, pcr->flags));
    }

    buf_printf(output, "\n\r");
    page_to_char(buf_string(output), ch);
    buf_free(output);

    return FALSE;
}

OLC_FUN(raceed_list)
{
    int i;
    for (i = 0; i < races.nused; i++)
        char_printf(ch, "[%d] %s\n", i, RACE(i)->name);
    return FALSE;
}

OLC_FUN(raceed_name)
{
    race_t *race;
    EDIT_RACE(ch, race);
    return olced_str(ch, argument, cmd, &race->name);
}

OLC_FUN(raceed_filename)
{
    race_t *race;
    EDIT_RACE(ch, race);
    return olced_str(ch, argument, cmd, &race->file_name);
}

OLC_FUN(raceed_flags)
{
    race_t *race;
    EDIT_RACE(ch, race);
    return olced_flag32(ch, argument, cmd, &race->flags);
}


OLC_FUN(raceed_damtype)
{
	char arg[MAX_INPUT_LENGTH];
	int dt;
	race_t *race;
	EDIT_RACE(ch, race);

	one_argument(argument, arg, sizeof(arg));
	if (arg[0] == '\0') {
		char_puts("Syntax: damtype [damage message]\n", ch);
		char_puts("Syntax: damtype ?\n", ch);
		return FALSE;
	}

	if (!str_cmp(arg, "?")) {
		BUFFER *output = buf_new(-1);
		show_attack_types(output);
		page_to_char(buf_string(output), ch);
		buf_free(output);
		return FALSE;
	}

	if ((dt = attack_lookup(arg)) < 0) {
		char_printf(ch, "RaceEd: %s: unknown damtype.\n", arg);
		return FALSE;
	}

	race->dam_type = dt;
	char_puts("Damage type set.\n", ch);
	return TRUE;
}

OLC_FUN(raceed_align)
{
	race_t *race;
	EDIT_RACE(ch, race);
	return olced_number(ch, argument, cmd, &race->alignment);
}

OLC_FUN(raceed_sex)
{
	race_t *race;
	EDIT_RACE(ch, race);
	return olced_flag32(ch, argument, cmd, &race->sex);
}

OLC_FUN(raceed_size)
{
	race_t *race;
	EDIT_RACE(ch, race);
	return olced_flag32(ch, argument, cmd, &race->size);
}


/*
OLC_FUN(raceed_love_sect)
{
    race_t *race;
    EDIT_RACE(ch, race);
    return olced_flag64(ch, argument, cmd, &race->loved_sectors);
}

OLC_FUN(raceed_hate_sect)
{
    race_t *race;
    EDIT_RACE(ch, race);
    return olced_flag64(ch, argument, cmd, &race->hated_sectors);
}

OLC_FUN(raceed_spec_part)
{
    race_t *race;
    char arg[MAX_STRING_LENGTH];
    int num, vnum, prob, level;

    EDIT_RACE(ch, race);

    // number
    argument = one_argument (argument, arg, sizeof(arg));
    num = atoi (arg);
    if (num < 1 || num > MAX_SPEC_PARTS)
    {
        char_act ("Syntax: spec_part <num> <vnum> <probability> <min_level>", ch);
        char_printf (ch, "Num must be between 1 and %d.\n", MAX_SPEC_PARTS);
        return FALSE;
    }

    // vnum 
    argument = one_argument (argument, arg, sizeof(arg));
    vnum = atoi (arg);
    vnum = UMAX (0, vnum);
    // validate vnum
    if (vnum != 0 && get_obj_index (vnum) == NULL)
    {
        char_printf (ch, "Obj vnum %d does not exist.\n", vnum);
        return FALSE;
    }

    // probability
    argument = one_argument (argument, arg, sizeof(arg));
    prob = atoi (arg);
    prob = UMAX (0, prob);
    if (prob > MAX_SPEC_PROB)
    {
        char_act ("Syntax: spec_part <num> <vnum> <probability> <min_level>", ch);
        char_printf (ch, "Probability must be between 0 and %d.\n", MAX_SPEC_PROB);
        return FALSE;
    }

    // level
    argument = one_argument (argument, arg, sizeof(arg));
    level = atoi (arg);
    level = UMAX (0, level);

    // fill the arrays
    race->spec_prob[num - 1] = prob;
    race->spec_parts[num - 1] = vnum;
    race->spec_levels[num - 1] = level;
    char_act ("Ok.", ch);
    return TRUE;
}
*/

OLC_FUN(pcraceed_create)
{
        race_t *race;
        EDIT_RACE(ch, race);
        if (race->pcdata != NULL)
        {
                char_printf(ch, "RaceEd: %s: pcrace already exists.\n", race->name);
                return FALSE;
        }
        race->pcdata = pcrace_new();
        return TRUE;
}

OLC_FUN(pcraceed_age)
{
    race_t *race;
    EDIT_RACE(ch, race);
        if (race->pcdata != NULL)
        {
                char_printf(ch, "RaceEd: %s: isn't a PC race.\n", race->name);
                return FALSE;
        }
    
    return olced_number(ch, argument, cmd, &race->pcdata->age_modifier);
}

OLC_FUN(pcraceed_skill)
{
    char arg[MAX_STRING_LENGTH];
    argument = one_argument(argument, arg, sizeof(arg));
    if (!str_prefix(arg, "add"))
        return pcraceed_skill_add(ch, argument, cmd);
    else if (!str_prefix(arg, "delete"))
        return pcraceed_skill_del(ch, argument, cmd);
    do_help(ch, "'OLC RACE SKILL'");
    return FALSE;
}

OLC_FUN(pcraceed_skill_add)
{
    int sn;
    rskill_t *race_skill;
    char    arg1[MAX_STRING_LENGTH];
    char    arg2[MAX_STRING_LENGTH];
    race_t *race;
    EDIT_RACE(ch, race);
    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));
    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
        do_help(ch, "'OLC RACE SKILL'");
        return FALSE;
    }
    if ((sn = sn_lookup(arg1)) <= 0)
    {
        char_printf(ch, "RaceEd: %s: unknown skill.\n", arg1);
        return FALSE;
    }

    if ((race_skill = rskill_lookup(race, sn)))
    {
        char_printf(ch, "RaceEd: %s: already there.\n",
                    SKILL(sn)->name);
        return FALSE;
    }

    race_skill = varr_enew(&race->pcdata->skills);
    race_skill->sn = sn;
    race_skill->level = atoi(arg2);
    varr_qsort(&race->pcdata->skills, cmpint);

    return TRUE;
}

OLC_FUN(pcraceed_skill_del)
{
    char    arg[MAX_STRING_LENGTH];
    rskill_t *race_skill;
    race_t *race;
    EDIT_RACE(ch, race);
    one_argument(argument, arg, sizeof(arg));
    if ((race_skill = skill_vlookup(&race->pcdata->skills, arg)) == NULL)
    {
        char_printf(ch, "RaceEd: %s: not found in pcrace skill list.\n", arg);
        return FALSE;
    }
    race_skill->sn = 0;
    varr_qsort(&race->pcdata->skills, cmpint);
    return TRUE;
}

static VALIDATE_FUN(validate_name)
{
    int i;
    race_t *race;
    EDIT_RACE(ch, race);

    for (i = 0; i < races.nused; i++)
        if (RACE(i) != race  &&  !str_cmp(RACE(i)->name, arg))
        {
            char_printf(ch, "RaceEd: %s: duplicate race name.\n", arg);
            return FALSE;
        }
    return TRUE;
}

static VALIDATE_FUN(validate_haspcdata)
{
        race_t *race;
        EDIT_RACE(ch, race);
        if (race->pcdata == NULL)
        {
                char_printf(ch, "RaceEd: %s: no race pcdata.\n", race->name);
                return FALSE;
        }
        return TRUE;
}

static bool touch_race(race_t *race)
{
    SET_BIT(race->flags, RACE_CHANGED);
    return FALSE;
}


