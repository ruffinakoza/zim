/*-
 * Copyright (c) 2006 Zsuzsu <little_zsuzsu@hotmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
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
 * $Id: waffects.h 851 2006-04-22 13:34:00Z zsuzsu $
 */

/*
 * materials - stuff things are made out of
 */

#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#define MAT_UNKNOWN	0

#define MAT_TYPE_UNKNOWN 0
#define MAT_TYPE_METAL	 1
#define MAT_TYPE_WOOD	 2
#define MAT_TYPE_ANIMAL	 3
#define MAT_TYPE_PLANT	 4
#define MAT_TYPE_STONE	 5
#define MAT_TYPE_GEM	 6
#define MAT_TYPE_LIQUID	 7
#define MAT_TYPE_GAS	 8
#define MAT_TYPE_ENERGY	 9
#define MAT_TYPE_MAGIC	 10

#define MAT_NONE	(0)
#define MAT_BURNS	(A)	/*heat*/
#define MAT_MELTS	(B)	/*heat*/
#define MAT_FLOATS	(C)	/*water*/
#define	MAT_EVAPORATES	(D)	/*heat*/
#define MAT_RUSTS	(E)	/*water*/
#define MAT_FUSES	(F)	/*electricity */
#define MAT_ROTS	(G)
#define MAT_FRAGILE	(H)
#define MAT_NOCLEAVE	(I)	/*can you cleave it*/

typedef struct material_t	material_t;

struct material_t
{
	material_t 		*next;
	const char 		*name;
	int			type;
	flag64_t		flags;
	int			density;
	int			rigidity;
	int			fragility;
	int			cost;
	int			bp_cost;
	int			noisy;
	int			antimagic;
	int			resist[DAM_MAX];
	int			vuln[DAM_MAX];
};

/* global variables */
extern varr materials;
extern flag_t   armor_class_types[];
extern flag_t	material_types[];
extern flag_t	material_flags[];

#define MATERIAL(i)		((material_t *) VARR_GET(&materials, i))
#define material_lookup(i)	((material_t *) varr_get(&materials, i))
#define MAT_DEFAULT_MOB		material_lookup_name("flesh")

/* free/delete */
material_t *	material_new	(void);
void		material_free	(material_t *pmat);
const char *	material_name	(int i);
int		material_lookup_name	(const char *name);

#endif
