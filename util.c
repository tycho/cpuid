#include "prefix.h"

#include "util.h"

#include <ctype.h>

void squeeze(char *str)
{
	int r; /* next character to be read */
	int w; /* next character to be written */

	r=w=0;
	while (str[r])
	{
		if (isspace(str[r]) || iscntrl(str[r]))
		{
			if (w > 0 && !isspace(str[w-1]))
				str[w++] = ' ';
		}
		else
			str[w++] = str[r];
		r++;
	}
	str[w] = 0;
}
