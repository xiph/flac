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

/* 
 * WATCHOUT - this is meant to be very lightweight an not even dependent
 * on libFLAC, so there are a couple places where FLAC__* variables are
 * duplicated here.  Look for 'DUPLICATE:' in comments.
 */

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FLAC/all.h"

static const char *sync_string_ = "fLaC"; /* DUPLICATE:FLAC__STREAM_SYNC_STRING */

static int usage(const char *message, ...);
static bool list(FILE *f, bool verbose);

int main(int argc, char *argv[])
{
	int i;
	bool verbose = false, list_mode = true;

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

	if(list_mode) {
		FILE *f = fopen(argv[i], "r");

		if(0 == f) {
			fprintf(stderr, "ERROR opening %s\n", argv[i]);
			return 1;
		}

		if(!list(f, verbose))
			return 1;

		fclose(f);
	}

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

bool list(FILE *f, bool verbose)
{
	static byte buf[65536];
	FLAC__StreamMetaData metadata;

	/* read the stream sync code */
	if(fread(buf, 1, 4, f) < 4 || memcmp(buf, sync_string_, 4)) {
		fprintf(stderr, "ERROR: not a FLAC file (no '%s' header)\n", sync_string_);
		return false;
	}

	/* read the metadata blocks */
	do {
		/* read the metadata block header */
		if(fread(buf, 1, 4, f) < 4) {
			fprintf(stderr, "ERROR: short count reading metadata block header\n");
			return false;
		}
		metadata.is_last = (buf[0] & 0x80)? true:false;
		metadata.type = (FLAC__MetaDataType)(buf[0] & 0x7f);
		metadata.length = (((unsigned)buf[1]) << 16) | (((unsigned)buf[2]) << 8) | ((unsigned)buf[3]);

		/* read the metadata block data */

		/* print it */
		printf("METADATA block:\n");	
		printf("\ttype: %u\n", (unsigned)metadata.type);
		printf("\tis last: %s\n", metadata.is_last? "true":"false");
		printf("\tlength: %u\n", metadata.length);
	} while (!metadata.is_last);

	return true;
}
