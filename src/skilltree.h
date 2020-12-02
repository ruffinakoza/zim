
#ifndef _SKILLTREE_H_
#define _SKILLTREE_H_

/*----------------------------------------------------------------------
 * skilltree info (skilltree.c)
 */

#define SKILLTREE_NONE 	 0

/*
 * skill tree structure
 */
struct skilltree_t
{
	const char *	name;		/* skill tree name */
	const char *	file_name;	/* file name */
	varr		skills;		/*  skills */
};


skilltree_t *	skilltree_new	(void);		/* allocate new mastry data */
void		skilltree_free	(skilltree_t*);	/* free mastry data */
int		skillt_lookup	(const char* name); /* mastry number lookup */
const char*	skilltree_name	(int st);	/* mastry name lookup */

extern varr	skilltrees;

#define SKILLTREE(st)		((skilltree_t*) VARR_GET(&skilltrees, st))
#define skilltree_lookup(st)	((skilltree_t*) varr_get(&skilltrees, st))

struct treeskill_t {
	int	sn;		/* skill number. leave this field first	 */
				/* in order sn_vlookup to work properly  */
	int	level;		/* level at which skill become available */
	int	percent;	/* initial percent			 */
};

#define treeskill_lookup(skilltree, sn) \
	((treeskill_t*) varr_bsearch(&skilltree->skills, &sn, cmpint))

/*
 *  lists utils
 */
void	skilltree_update_lists	(skilltree_t *skilltree, CHAR_DATA *victim, bool memb);
void	skilltree_save		(skilltree_t *skilltree);

#endif
