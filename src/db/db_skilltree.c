
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "merc.h"
#include "db.h"

DECLARE_DBLOAD_FUN(load_skilltree);

DBFUN dbfun_skilltrees[] =
{
	{ "SKILLTREE",	load_skilltree	},
	{ NULL }
};

DBDATA db_skilltrees = { dbfun_skilltrees };


DBLOAD_FUN(load_skilltree)
{
	skilltree_t *skilltree;

	skilltree = skilltree_new();
	skilltree->file_name = get_filename(filename);
	
	for (;;) {
		char *word = feof(fp) ? "End" : fread_word(fp);
		bool fMatch = FALSE;

		switch (UPPER(word[0])) {
	case 'E':
			if (!str_cmp(word, "End")) {
				if (IS_NULLSTR(skilltree->name)) {
					db_error("load_skilltree",
						 "skill tree name not defined");
					skilltree_free(skilltree);
					skilltrees.nused--;
				}
				varr_qsort(&skilltree->skills, cmpint);
						return;
			}
			break;
		case 'N':
			SKEY("Name", skilltree->name);
			break;
		case 'S':
			if (!str_cmp(word, "Skill")) {
				treeskill_t *tssk = varr_enew(&skilltree->skills);
				tssk->sn = sn_lookup(fread_word(fp));	
				tssk->level = fread_number(fp);
				tssk->percent = fread_number(fp);
				fMatch = TRUE;
			}
		}

		if (!fMatch) 
			db_error("load_skilltree", "%s: Unknown keyword", word);
	}
}

