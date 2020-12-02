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
 * $Id: util.c 849 2006-04-22 13:08:54Z zsuzsu $
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#if !defined (WIN32)
#include <unistd.h>
#endif

#include "merc.h"
#include "db/db.h"

#ifdef SUNOS
#	include "compat/compat.h"
#endif

#if defined(WIN32)
#define unlink	_unlink
#endif

void doprintf(DO_FUN *fn, CHAR_DATA* ch, const char* fmt, ...)
{
	char buf[MAX_STRING_LENGTH];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	fn(ch, buf);
	va_end(ap);
}

FILE *dfopen(const char *dir, const char *file, const char *mode)
{
	char name[PATH_MAX];
	FILE *f;
	snprintf(name, sizeof(name), "%s%c%s", dir, PATH_SEPARATOR, file);
	if ((f = fopen(name, mode)) == NULL)
		log_printf("%s: %s", name, strerror(errno));
	return f;
}

int dunlink(const char *dir, const char *file)
{
	char name[PATH_MAX];
	snprintf(name, sizeof(name), "%s%c%s", dir, PATH_SEPARATOR, file);
	return unlink(name);
}

int d2rename(const char *dir1, const char *file1,
	     const char *dir2, const char *file2)
{
	int res;
	char name1[PATH_MAX];
	char name2[PATH_MAX];
	snprintf(name1, sizeof(name1), "%s%c%s", dir1, PATH_SEPARATOR, file1);
	snprintf(name2, sizeof(name2), "%s%c%s", dir2, PATH_SEPARATOR, file2);
#if defined (WIN32)
	res = unlink(name2);
	if (res == -1)
		log_printf("d2rename: can't delete file %s", name2);
#endif
	res = rename(name1, name2);
	if (res < 0)
		log_printf("d2rename: error renaming %s -> %s", name1, name2);
	return res;
}

bool dfexist(const char *dir, const char *file)
{
	struct stat sb;
	char name[PATH_MAX];
	snprintf(name, sizeof(name), "%s%c%s", dir, PATH_SEPARATOR, file);
	return (stat(name, &sb) >= 0);
}

const char *get_filename(const char *name)
{
	const char *p = (p = strrchr(name, PATH_SEPARATOR)) ? ++p : name;
	return str_dup(p);
}

int cmpint(const void *p1, const void *p2)
{
	return *(int*) p1 - *(int*) p2;
}

/*
 * returns the number of characters that are 
 * not dedicated to ANSI coloring
 */
size_t cstrlen(const char *cstr)
{
	size_t res;

	if (cstr == NULL)
		return 0;

	res = strlen(cstr);
	while ((cstr = strchr(cstr, '{')) != NULL) {
		if (*(cstr+1) == '{')
			res--;
		else
			res -= 2;
		cstr += 2;
	}

	return res;
}

/*
 * returns the number of character dedicated
 * to ANSI coloring for the first "len"
 * number of printable characters.
 *
 * sending the len -1 would be the same as strlen(cstr)
 */
size_t astrlen(const char *cstr, size_t len)
{
	size_t res = 0;

	while (cstr != NULL && *cstr != '\0' && len != 0) {
		if (*cstr == '{') {
			if ((*(cstr+1) == '{')) {
				res++;	
			}
			else {
				res += 2;	
			}
			cstr++;
		}
		else {
			len--;
		}
		cstr++;
	}

	return res;
}

const char *cstrfirst(const char *cstr)
{
	if (cstr == NULL)
		return NULL;

	for (; *cstr == '{'; cstr++)
		if (*(cstr+1))
			cstr++;
	return cstr;
}

char *strtime(time_t time)
{
	char *p = ctime(&time);
	p[24] = '\0';
	return p;
}

/*
 * remove leading and trailing spaces.
 * by Zsuzsu
 */
char *chomp(char *str)
{
	char *ptr;
	int len;
	char buf[MAX_INPUT_LENGTH];

	if (!str) return NULL;

	len = strlen(str);

	ptr = &(str[len-1]);

	while (ptr != str && *ptr == ' ')
		*ptr-- = '\0';

	ptr = str;

	while (*ptr == ' ') ptr++;

	strncpy(buf, ptr, sizeof(buf));
	strncpy(str, buf, sizeof(buf));

	return str;
}

int area_avg_level_cmp (const void *a, const void *b)
{
	AREA_DATA *areaA = *((AREA_DATA **) a);
	AREA_DATA *areaB = *((AREA_DATA **) b);
	int avgA = (areaA->min_level + areaA->max_level) / 2;
	int avgB = (areaB->min_level + areaB->max_level) / 2;


	if (IS_SET(areaA->flags, AREA_CLANHALL)) avgA += 100;
	if (IS_SET(areaB->flags, AREA_CLANHALL)) avgB += 100;

	if (IS_SET(areaA->flags, AREA_PLAYERHOUSE)) avgA += 500;
	if (IS_SET(areaB->flags, AREA_PLAYERHOUSE)) avgB += 500;

	return avgA - avgB;
}

/*
 * returns an array of pointers to areas in avg level order
 * the last entry is NULL indicating the end of the list
 *
 * by Zsuzsu
 */
AREA_DATA **areas_avg_level_sorted()
{
	AREA_DATA **area_list;
	AREA_DATA *walker;
	int num_areas = 0;
	int i;

	walker = area_first;
	while (walker != NULL) {
		walker = walker->next;
		num_areas++;
	}

	if (num_areas <= 0) return NULL;

	area_list = (AREA_DATA **) malloc((num_areas+1) * sizeof(AREA_DATA *));

	if (area_list == NULL) {
		log_printf("do_areas couldn't allocate memory");
		return NULL;
	}

	walker = area_first;
	for (i=0; i < num_areas; i++) {
		area_list[i] = walker;
		walker = walker->next;
	}
	area_list[num_areas] = NULL;

	qsort(area_list, num_areas, sizeof(AREA_DATA *), 
		area_avg_level_cmp);

	return area_list;
}

char * compact_date_str (time_t attime)
{
	static char time_str[18];
	struct tm * timeinfo;
	
	timeinfo = localtime(&attime);

	snprintf(time_str, sizeof(time_str),
		"%04d%02d%02d-%02d:%02d:%02d",
		1900 + timeinfo->tm_year,
		timeinfo->tm_mon +1,
		timeinfo->tm_mday,
		timeinfo->tm_hour,
		timeinfo->tm_min,
		timeinfo->tm_sec);
	return time_str;
}
