/*
 * $Id: string_edit.h 849 2006-04-22 13:08:54Z zsuzsu $
 */

/***************************************************************************
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/

#ifndef _STRING_EDIT_H_
#define _STRING_EDIT_H_

void		string_append   (CHAR_DATA *ch, const char **pString);
const char *	string_replace	(const char * orig, char * old, char * new);
void		string_add      (CHAR_DATA *ch, const char *argument);
const char *	format_string   (const char *oldstring /*, bool fSpace */);

#endif
