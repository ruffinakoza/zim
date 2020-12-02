/*-
 * Copyright (c) 1998 fjoe <fjoe@iclub.nsu.ru>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * $Id: race.c 851 2006-04-22 13:34:00Z zsuzsu $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "merc.h"
#include "varr.h"
#include "material.h"

varr materials = { sizeof(material_t), 20 };

flag_t material_types[] =
{
	{ "",		TABLE_INTVAL				},

	{ "unknown",	MAT_TYPE_UNKNOWN,	TRUE		},
	{ "metal",	MAT_TYPE_METAL,		TRUE		},
	{ "wood",	MAT_TYPE_WOOD,		TRUE		},
	{ "animal",	MAT_TYPE_ANIMAL,	TRUE		},
	{ "plant",	MAT_TYPE_PLANT,		TRUE		},
	{ "stone",	MAT_TYPE_STONE,		TRUE		},
	{ "gem",	MAT_TYPE_GEM,		TRUE		},
	{ "liquid",	MAT_TYPE_LIQUID,	TRUE		},
	{ "gas",	MAT_TYPE_GAS,		TRUE		},
	{ "energy",	MAT_TYPE_ENERGY,	TRUE		},
	{ "magic",	MAT_TYPE_MAGIC,		TRUE		},

	{ NULL }
};

flag_t material_flags[] =
{
	{ "",		TABLE_BITVAL				},

	{ "none",	MAT_NONE,		TRUE		},
	{ "burns",	MAT_BURNS,		TRUE		},
	{ "melts",	MAT_MELTS,		TRUE		},
	{ "floats",	MAT_FLOATS,		TRUE		},
	{ "evaporates",	MAT_EVAPORATES,		TRUE		},
	{ "rusts",	MAT_RUSTS,		TRUE		},
	{ "fuses",	MAT_FUSES,		TRUE		},
	{ "rots",	MAT_ROTS,		TRUE		},
	{ "fragile",	MAT_FRAGILE,		TRUE		},
	{ "nocleave",	MAT_NOCLEAVE,		TRUE		},

	{ NULL }
};

material_t *material_new (void)
{
	material_t *mat;
	mat = varr_enew(&materials);

	mat->name = str_empty;

	return mat;
}

void material_free(material_t *mat)
{
	if (!mat) {
		BUG("trying to free null material");
		return;
	}
	free_string(mat->name);
}

/* lookups **********************************************************/
const char *material_name(int i)
{
	material_t *mat = material_lookup(i);

	return (mat) ? mat->name : "unknown";
}

int material_lookup_name(const char *name)
{
	int num;
 
	for (num = 0; num < materials.nused; num++) {
		material_t *m = MATERIAL(num);

		if (LOWER(name[0]) == LOWER(m->name[0])
		&&  !str_prefix(name, (m->name)))
			return num;
	}
 
	return MAT_UNKNOWN;
}


/*
 * Check the material
 */
bool check_material(OBJ_DATA * obj, char *material)
{
	return strstr(obj->material->name, material) ? TRUE : FALSE;
}

bool is_metal(OBJ_DATA * obj)
{

	return obj->material->type == MAT_TYPE_METAL;
}

bool may_float(OBJ_DATA * obj)
{

	if (IS_SET(obj->material->flags, MAT_FLOATS))
		return TRUE;

	if (obj->pIndexData->item_type == ITEM_CORPSE_PC)
		return TRUE;
	if (obj->pIndexData->item_type == ITEM_BOAT)
		return TRUE;
	if (obj->pIndexData->item_type == ITEM_QUESTFIND)
		return TRUE;
	if (IS_SET(obj->pIndexData->extra_flags, ITEM_QUEST))
		return TRUE;
	return FALSE;
}


bool cant_float(OBJ_DATA * obj)
{
	if (IS_SET(obj->pIndexData->extra_flags, ITEM_QUEST))
		return FALSE;
	if (!IS_SET(obj->material->flags, MAT_FLOATS))
		return TRUE;

	return FALSE;
}
