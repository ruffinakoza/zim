 /* $Id: olc_material.c,v 1.666 2004/09/20 10:50:30 shrike Exp $ */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "olc.h"
#include "material.h"

#define EDIT_MATERIAL(ch, material)	(material = (material_t*) ch->desc->pEdit)

DECLARE_OLC_FUN(materialed_create		);
DECLARE_OLC_FUN(materialed_edit			);
DECLARE_OLC_FUN(materialed_touch		);
DECLARE_OLC_FUN(materialed_show			);
DECLARE_OLC_FUN(materialed_list			);

DECLARE_OLC_FUN(materialed_name			);
DECLARE_OLC_FUN(materialed_type			);
DECLARE_OLC_FUN(materialed_flag			);
DECLARE_OLC_FUN(materialed_rigidity		);
DECLARE_OLC_FUN(materialed_fragility		);
DECLARE_OLC_FUN(materialed_density		);
DECLARE_OLC_FUN(materialed_noisy		);
DECLARE_OLC_FUN(materialed_antimagic		);
DECLARE_OLC_FUN(materialed_cost			);
DECLARE_OLC_FUN(materialed_bpcost		);

olc_cmd_t olc_cmds_material[] =
{
    { 5, "create",	materialed_create			},
    { 5, "edit",	materialed_edit				},
    { 5, "touch",	olced_dummy				},
    { 0, "show",	materialed_show				},
    { 5, "list",	materialed_list				},

    { 5, "name",	materialed_name				},
    { 5, "type",	materialed_type,	material_types	},
    { 5, "density",	materialed_density			},
    { 5, "rigidity",	materialed_rigidity			},
    { 5, "fragility",	materialed_fragility			},
    { 5, "noisy",	materialed_noisy			},
    { 5, "antimagic",	materialed_antimagic			},
    { 5, "flags",	materialed_flag,	material_flags	},
    { 5, "cost",	materialed_cost				},
    { 5, "bpcost",	materialed_bpcost			},

    { 0, "commands",	show_commands				},

    { 0, NULL}
};

OLC_FUN(materialed_create)
{
	char arg[MAX_STRING_LENGTH];
	material_t *material;
	int matnum = 0;

/*
	if (!char_security(ch,"SECURITY_OLC_MATERIAL")) {
		char_puts("MaterialEd: Insufficient security for editing materials.\n", ch);
		return FALSE;
	}
*/

	first_arg(argument, arg, sizeof(arg), FALSE);

	if (arg[0] == '\0') {
		do_help(ch, "'OLC CREATE'");
		return FALSE;
	}

	if (is_number(arg)) {
		char_puts("MaterialEd: Argument must be name.\n", ch);
		return FALSE;
	}

	if ((matnum = material_lookup_name(arg) > 0)) {
		char_printf(ch, "MaterialEd: '%s': already exists.\n", 
			MATERIAL(matnum)->name);
		return FALSE;
	}

	material = material_new();
	material->name = str_dup(arg);

	ch->desc->pEdit = (void *)material;
	OLCED(ch) = olced_lookup(ED_MATERIAL);

	char_printf(ch, "Material '%s' created.\n", material->name);
	return FALSE;
}

OLC_FUN(materialed_edit)
{
	char arg[MAX_STRING_LENGTH];
	int matnum = -1;

/*
	if (!char_security(ch,"SECURITY_OLC_MATERIAL")) {
		char_puts("MaterialEd: Insufficient security.\n", ch);
		return FALSE;
	}
*/

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		do_help(ch, "'OLC EDIT'");
		return FALSE;
	}

	if (is_number(arg)) {
		if (atoi(arg) < 0 || atoi(arg) > materials.nused) {
			char_printf(ch, "MaterialEd: '%s': No such material.\n", arg);
			return FALSE;
		}
		else
			ch->desc->pEdit     = MATERIAL(atoi(arg));
	}
	else {
		if ((matnum = material_lookup_name(arg)) < 0) {
			char_printf(ch, "MaterialEd: '%s': No such material.\n", 
				argument);
			return FALSE;
		}
		else
			ch->desc->pEdit     = MATERIAL(matnum);
	}

	OLCED(ch) = olced_lookup(ED_MATERIAL);
	return FALSE;
}

OLC_FUN(materialed_list)
{
	int i;

	for (i = 0; i < materials.nused; i++) {
		char_printf(ch, "[%3d] %-15s  ", i, MATERIAL(i)->name);
		if ((i+1) % 3 == 0)
			char_printf(ch, "\n");
	}
	return FALSE;
}

OLC_FUN(materialed_name)
{
	material_t *material;

	EDIT_MATERIAL(ch, material);
	return olced_str(ch, argument, cmd, &material->name);
}

OLC_FUN(materialed_type)
{
	material_t *material;

	EDIT_MATERIAL(ch, material);
	return olced_flag32(ch, argument, cmd, &material->type);
}


OLC_FUN(materialed_flag)
{
	material_t *material;

	EDIT_MATERIAL(ch, material);
	return olced_flag64(ch, argument, cmd, &material->flags);
}

OLC_FUN(materialed_cost)
{
	material_t *material;

	EDIT_MATERIAL(ch, material);
	return olced_number(ch, argument, cmd, &material->cost);
}

OLC_FUN(materialed_bpcost)
{
	material_t *material;

	EDIT_MATERIAL(ch, material);
	return olced_number(ch, argument, cmd, &material->bp_cost);
}

OLC_FUN(materialed_rigidity)
{
	material_t *material;

	EDIT_MATERIAL(ch, material);
	return olced_number(ch, argument, cmd, &material->rigidity);
}

OLC_FUN(materialed_fragility)
{
	material_t *material;

	EDIT_MATERIAL(ch, material);
	return olced_number(ch, argument, cmd, &material->fragility);
}

OLC_FUN(materialed_density)
{
	material_t *material;

	EDIT_MATERIAL(ch, material);
	return olced_number(ch, argument, cmd, &material->density);
}

OLC_FUN(materialed_noisy)
{
	material_t *material;

	EDIT_MATERIAL(ch, material);
	return olced_number(ch, argument, cmd, &material->noisy);
}

OLC_FUN(materialed_antimagic)
{
	material_t *material;

	EDIT_MATERIAL(ch, material);
	return olced_number(ch, argument, cmd, &material->antimagic);
}

OLC_FUN(materialed_show)
{
	char              arg[MAX_STRING_LENGTH];
	BUFFER           *output;
	material_t       *material;

	one_argument(argument, arg, sizeof(arg));

	if (arg[0] == '\0') {
		if (IS_EDIT(ch, ED_MATERIAL))
			EDIT_MATERIAL(ch, material);
		else {
			do_help(ch, "'OLC ASHOW'");
			return FALSE;
		}
	}
	else {
		if (is_number(arg)) {
			if (atoi(arg) < 0 || atoi(arg) > materials.nused) {
				char_printf(ch, "MaterialEd: '%s':"
					" No such material.\n", 
					arg);
				return FALSE;
			}
			else
				material = MATERIAL(atoi(arg));
		}
		else {
			if (material_lookup_name(arg) < 0) {
			       char_printf(ch, "MaterialEd '%s':"
					" No such material.\n",
					arg);
			       return FALSE;
			}
			else
				material = MATERIAL(material_lookup_name(arg));
		}
	}

	/* display */
	output = buf_new(-1);

	buf_printf(output, "Name:       [{C%s{x]\n", material->name);
	buf_printf(output, "Type:       [{c%s{x]\n", flag_string(material_types, material->type));
	buf_printf(output, "Density:    [{c%d{x]\n", material->density);
	buf_printf(output, "Rigidity:   [{c%d{x]\n", material->rigidity);
	buf_printf(output, "Fragility:  [{c%d{x]\n", material->fragility);
	buf_printf(output, "Noisy:      [{c%d{x]\n", material->noisy);
	buf_printf(output, "AntiMagic:  [{c%d{x]\n", material->antimagic);
	buf_printf(output, "Flags:      [{c%s{x]\n", flag_string(material_flags, material->flags));
	buf_printf(output, "Cost:       [{c%d{x]\n", material->cost);
	buf_printf(output, "BPCost:     [{c%d{x]\n", material->bp_cost);

	buf_printf(output, "\n");

	page_to_char(buf_string(output), ch);
	buf_free(output);

	return FALSE;
}

