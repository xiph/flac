/* metaflac - Command-line FLAC metadata editor
 * Copyright (C) 2001  Josh Coalson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FLAC/all.h"

static int usage(const char *message, ...);

int main(int argc, char *argv[])
{
	bool verbose, list_mode;

	if(argc <= 1)
		return usage(0);

	/* get the options */
	for(i = 1; i < argc; i++) {
		if(argv[i][0] != '-' || argv[i][1] == 0)
			break;
		if(0 == strcmp(argv[i], "-l"))
			list_mode = true;
		else if(0 == strcmp(argv[i], "-v"))
			verbose = true;
		else if(0 == strcmp(argv[i], "-v-"))
			verbose = false;
		else {
			return usage("ERROR: invalid option '%s'\n", argv[i]);
		}
	}
	if(i + (list_mode? 1:2) != argc)
		return usage("ERROR: invalid arguments (more/less than %d filename%s?)\n", (list_mode? 1:2), (list_mode? "":"s"));

	return 0;
}

int usage(const char *message, ...)
{
	va_list args;

	if(message) {
		va_start(args, message);

		(void) vfprintf(stderr, message, args);

		va_end(args);

	}
	printf("==============================================================================\n");
	printf("metaflac - Command-line FLAC metadata editor version %s\n", FLAC__VERSION_STRING);
	printf("Copyright (C) 2001  Josh Coalson\n");
	printf("\n");
	printf("This program is free software; you can redistribute it and/or\n");
	printf("modify it under the terms of the GNU General Public License\n");
	printf("as published by the Free Software Foundation; either version 2\n");
	printf("of the License, or (at your option) any later version.\n");
	printf("\n");
	printf("This program is distributed in the hope that it will be useful,\n");
	printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
	printf("GNU General Public License for more details.\n");
	printf("\n");
	printf("You should have received a copy of the GNU General Public License\n");
	printf("along with this program; if not, write to the Free Software\n");
	printf("Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.\n");
	printf("==============================================================================\n");
	printf("Usage:\n");
	printf("  metaflac [options] infile [outfile]\n");
	printf("\n");
	printf("options:\n");
	printf("  -l : list metadata blocks\n");
	printf("  -v : verbose\n");
	printf("  -v- can all be used to turn off a particular option\n");

	return 1;
}
