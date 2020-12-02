
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if	defined (WIN32)
#	include <compat/compat.h>
#else
#	include <dirent.h>
#endif

#include "merc.h"
#include "skilltree.h"

DECLARE_DO_FUN(do_asave);

varr skilltrees = { sizeof(skilltree_t), 4 };

	skilltree_t *skilltree_new(void)
{
	skilltree_t *skilltree;

	skilltree = varr_enew(&skilltrees);
	skilltree->skills.nsize = sizeof(skilltree_t);
	skilltree->skills.nstep = 4;

	return skilltree;
}

void skilltree_free(skilltree_t *skilltree)
{
	varr_free(&skilltree->skills);
}

int skillt_lookup(const char *name)
{
	int stree;

	if (IS_NULLSTR(name))
		return -1;

	for (stree = 0; stree < skilltrees.nused; stree++)
		if (!str_cmp(name, SKILLTREE(stree)->name))
			return stree;

	return -1;
}

const char *skilltree_name(int st)
{
	skilltree_t *skilltree = skilltree_lookup(st);
	if (skilltree)
		return skilltree->name;
	return "None";
}



			
