#include <stdio.h>

#include "merc.h"
#include "db.h"
#include "material.h"

DECLARE_DBLOAD_FUN(load_material);

DBFUN db_load_materials[] =
{
	{ "MATERIAL",   load_material},
	{ NULL}
};
DBDATA db_materials = { db_load_materials };

DBLOAD_FUN(load_material)
{
	material_t *material;

	material = varr_enew(&materials);

	while(1) {
		char *word = feof(fp) ? "End" : fread_word(fp);
		bool fMatch = FALSE;

		switch (UPPER(word[0])) {
		case 'A':
			KEY("AntiMagic", material->antimagic,fread_number(fp));
			break;

		case 'B':
			KEY("BPCost",   material->bp_cost,      fread_number(fp));
			break;
		case 'C':
			KEY("Cost",   material->cost,      fread_number(fp));
			break;

		case 'D':
			KEY("Density", material->density, fread_number(fp));
			break;

		case 'E':
			if (!str_cmp(word, "End")) {
				if (IS_NULLSTR(material->name)) {
					db_error("load_material",
						"material name not defined");
					material_free(material);
					materials.nused--;
				}
				return;
			}
			break;

		case 'F':
			KEY("Flags", material->flags, fread_fstring(material_flags, fp));
			KEY("Fragility", material->fragility,    fread_number(fp));
			break;

		case 'N':
			SKEY("Name",	material->name);
			KEY("Noisy",	material->noisy, fread_number(fp));
			break;

		case 'R':
			KEY("Rigidity",  material->rigidity, fread_number(fp));
			break;

		case 'T':
			if (!str_cmp(word, "Type")) {
				material->type = fread_fword(material_types, fp);
				fMatch = TRUE;
			}
			break;

		}

		if (!fMatch)
			db_error("load_material", "%s: Unknown keyword", word);
	}
}
