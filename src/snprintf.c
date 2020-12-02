#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include "compat.h"

/*
 * $Id: snprintf.c 849 2006-04-22 13:08:54Z zsuzsu $
 */

int snprintf(char* buf, size_t size, const char* fmt, ...)
{
	int res;
	va_list ap;

	va_start(ap, fmt);
	res = vsnprintf(buf, size, fmt, ap);
	va_end(ap);

	return res;
}
