/* grabbag - Convenience lib for various routines common to several tools
 * Copyright (C) 2002  Josh Coalson
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

#include "share/grabbag.h"
#include "FLAC/assert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GRABBAG_API unsigned grabbag__cuesheet_msf_to_frame(unsigned minutes, unsigned seconds, unsigned frames)
{
	return ((minutes * 60) + seconds) * 75 + frames;
}

GRABBAG_API void grabbag__cuesheet_frame_to_msf(unsigned frame, unsigned *minutes, unsigned *seconds, unsigned *frames)
{
	*frames = frame % 75;
	frame /= 75;
	*seconds = frame % 60;
	frame /= 60;
	*minutes = frame;
}

/* since we only care about values >= 0 or error, returns < 0 for any illegal int, else value */
static int local__parse_int_(const char *s)
{
	int ret = 0;
	char c;

	if(*s == '\0')
		return -1;

	while('\0' != (c = *s++))
		if(c >= '0' && c <= '9')
			ret = ret * 10 + (c - '0');
		else
			return -1;

	return ret;
}

static char *local__get_field_(char **s)
{
	char *p;

	FLAC__ASSERT(0 != s);

	if(0 == *s)
		return 0;

	/* skip leading whitespace */
	while(**s && 0 != strchr(" \t\r\n", **s))
		(*s)++;

	if(**s == 0)
		*s = 0;

	p = *s;

	if(p) {
		while(**s && 0 == strchr(" \t\r\n", **s))
			(*s)++;
		if(**s)
			**s = '\0';
		else
			*s = 0;
	}

	return p;
}

static FLAC__bool local__cuesheet_parse_(FILE *file, const char **error_message, unsigned *last_line_read, FLAC__StreamMetadata *cuesheet, FLAC__bool check_cd_da_subset)
{
#if defined _MSC_VER || defined __MINGW32__
#define FLAC__STRCASECMP stricmp
#else
#define FLAC__STRCASECMP strcasecmp
#endif
	char buffer[4096], *line, *field;
	unsigned linelen;
	int in_track = -1, in_index = -1;

	while(0 != fgets(buffer, sizeof(buffer), file)) {
		*last_line_read++;
		line = buffer;

		linelen = strlen(line);
		if(line[linelen-1] != '\n') {
			*error_message = "line too long";
			return false;
		}

		if(0 != (field = local__get_field_(&line))) {
			if(0 == FLAC__STRCASECMP(field, "CATALOG")) {
				/*@@@@ error if already encountered CATALOG */
				/*@@@@ check_cd_da_subset: 13 digits ['0'..'9'] */
			}
			else if(0 == FLAC__STRCASECMP(field, "FLAGS")) {
				/*@@@@ error if already encountered FLAGS in this track */
				if(in_track < 0) {
					/*@@@@*/
				}
				if(in_index >= 0) {
					/*@@@@*/
				}
				/*@@@@ search for PRE flag only */
			}
			else if(0 == FLAC__STRCASECMP(field, "INDEX")) {
				if(in_track < 0) {
					*error_message = "found INDEX before any TRACK";
					return false;
				}
				/*@@@@ check_cd_da_subset: 0..99 */
				/*@@@@ check_cd_da_subset: first index of first track is 00:00:00 */
				/*@@@@ check first is 0 or 1 */
				/*@@@@ check sequential */
				/*@@@@ parse msf (or sample offset if !check_cd_da_subset)
			}
			else if(0 == FLAC__STRCASECMP(field, "ISRC")) {
				/*@@@@ error if already encountered ISRC in this track */
				if(in_track < 0) {
					/*@@@@*/
				}
				if(in_index >= 0) {
					/*@@@@*/
				}
			}
			else if(0 == FLAC__STRCASECMP(field, "TRACK")) {
				if(0 == (field = local__get_field_(&line))) {
					*error_message = "TRACK is missing track number";
					return false;
				}
				in_track = local__parse_int_(field);
				in_index = -1;
				if(in_track <= 0) {
					*error_message = "TRACK has invalid track number";
					return false;
				}
				/*@@@@ check_cd_da_subset: 1..99 */
				/*@@@@ check_cd_da_subset: sequential */
			}
		}
	}

	if(!feof(file)) {
		*error_message = "read error";
		return false;
	}
	return true;
#undef FLAC__STRCASECMP
}

GRABBAG_API FLAC__StreamMetadata *grabbag__cuesheet_parse(FILE *file, const char **error_message, unsigned *last_line_read, FLAC__bool check_cd_da_subset)
{
	FLAC__StreamMetadata *cuesheet;

	FLAC__ASSERT(0 != file);
	FLAC__ASSERT(0 != error_message);
	FLAC__ASSERT(0 != last_line_read);

	*last_line_read = 0;
	cuesheet = FLAC__metadata_object_new(FLAC__METADATA_TYPE_CUESHEET);

	if(0 == cuesheet) {
		*error_message = "memory allocation error";
		return 0;
	}

	if(!local__cuesheet_parse_(file, error_message, last_line_read, cuesheet, check_cd_da_subset)) {
		FLAC__metadata_object_delete(cuesheet);
		return 0;
	}

	return cuesheet;
}
