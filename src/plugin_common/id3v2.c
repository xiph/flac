/* plugin_common - Routines common to several plugins
 * Copyright (C) 2002,2003,2004  Daisuke Shimamura
 *
 * Almost from id3_tag.c - 2001/02/16
 *  EasyTAG - Tag editor for MP3 and OGG files
 *  Copyright (C) 2001-2002  Jerome Couderc <j.couderc@ifrance.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "FLAC/assert.h"

#include <stdlib.h> /* for free() */
#include <string.h> /* for memset() */

#ifdef FLAC__HAS_ID3LIB
#include <id3.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "id3v1.h" /* for genre stuff */
#include "locale_hack.h"

#define ID3V2_MAX_STRING_LEN 4096
#define NUMBER_TRACK_FORMATED 1
#endif

/*
 * This should come after #include<id3.h> because id3.h doesn't #undef
 * true and false before redefining them, causing warnings.
 */
#include "id3v2.h"

#ifdef FLAC__HAS_ID3LIB
/* local__strip() based on glib's g_strchomp() and g_strchug():
 * GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 * (LGPL 2 follows)
 */
static void local__strip(char *string)
{
	char *s;

	if(0 == string)
		return;

	for(s = string; *s && isspace(*s); s++)
		;

	memmove(string, s, strlen((char*)s) + 1);

	if(!*string)
		return;

	for(s = string + strlen (string) - 1; s >= string && isspace(*s); s--)
		*s = '\0';
}


/*
 * As the ID3Tag_Link function of id3lib-3.8.0pre2 returns the ID3v1 tags
 * when a file has both ID3v1 and ID3v2 tags, we first try to explicitely
 * get the ID3v2 tags with ID3Tag_LinkWithFlags and, if we cannot get them,
 * fall back to the ID3v1 tags.
 * (Written by Holger Schemel).
 */
static size_t local__ID3Tag_Link_wrapper(ID3Tag *id3tag, const char *filename)
{
	size_t offset;

#   if ( (ID3LIB_MAJOR >= 3) && (ID3LIB_MINOR >= 8)  )
		/* First, try to get the ID3v2 tags */
		offset = ID3Tag_LinkWithFlags(id3tag, filename, ID3TT_ID3V2);
		if (offset == 0) {
			/* No ID3v2 tags available => try to get the ID3v1 tags */
			offset = ID3Tag_LinkWithFlags(id3tag, filename, ID3TT_ID3V1);
		}
#   else
		/* Function 'ID3Tag_LinkWithFlags' is not defined up to id3lib-.3.7.13 */
		offset = ID3Tag_Link(id3tag, filename);
#   endif
	return offset;
}


/*
 * As the ID3Field_GetASCII function differs with the version of id3lib, we must redefine it.
 */
/* [JEC] old id3lib versions may have used index_t for itemNum but size_t is what it wants now and seems safe enough. */
static size_t local__ID3Field_GetASCII_wrapper(const ID3Field *field, char *buffer, size_t maxChars, size_t itemNum)
{

	/* Defined by id3lib:   ID3LIB_MAJOR_VERSION, ID3LIB_MINOR_VERSION, ID3LIB_PATCH_VERSION
	 * Defined by autoconf: ID3LIB_MAJOR,         ID3LIB_MINOR,         ID3LIB_PATCH
	 *
	 * <= 3.7.12 : first item num is 1 for ID3Field_GetASCII
	 *  = 3.7.13 : first item num is 0 for ID3Field_GetASCII
	 * >= 3.8.0  : doesn't need item num for ID3Field_GetASCII
	 */
#	if (ID3LIB_MAJOR >= 3)
		 /* (>= 3.x.x) */
#		if (ID3LIB_MINOR <= 7)
			 /* (3.0.0 to 3.7.x) */
#			if (ID3LIB_PATCH >= 13)
				 /* (>= 3.7.13) */
				 return ID3Field_GetASCII(field, buffer, maxChars, itemNum);
#			else
				 return ID3Field_GetASCII(field, buffer, maxChars, itemNum+1);
#			endif
#		else
			 /* (>= to 3.8.0) */
			 /*return ID3Field_GetASCII(field, buffer, maxChars); */
			 return ID3Field_GetASCIIItem(field, buffer, maxChars, itemNum);
#		endif
#	else
		 /* Not tested (< 3.x.x) */
		 return ID3Field_GetASCII(field, buffer, maxChars, itemNum+1);
#	endif
}


/*
 * Returns the name of a genre code if found
 * Three states for genre code :
 *    - defined (0 to GENRE_MAX)
 *    - undefined/unknown (GENRE_MAX+1 to ID3_INVALID_GENRE-1)
 *    - invalid (>ID3_INVALID_GENRE)
 */
static const char *local__genre_to_string(unsigned genre_code)
{
	if(genre_code >= FLAC_PLUGIN__ID3V1_TAG_INVALID_GENRE)
		return "";
	else {
		const char *s = FLAC_plugin__id3v1_tag_get_genre_as_string((unsigned)genre_code);
		if(s[0] == 0)
			return "Unknown";
		else
			return s;
	}
}


/*
 * Read id3v1.x / id3v2 tag and load data into the File_Tag structure using id3lib functions.
 * Returns true on success, else false.
 * If a tag entry exists (ex: title), we allocate memory, else value stays to NULL
 */
static FLAC__bool local__get_tag(const char *filename, FLAC_Plugin__Id3v2_Tag *tag)
{
	FILE *file;
	ID3Tag *id3_tag = 0; /* Tag defined by id3lib */
	char *string, *string1;

	FLAC__ASSERT(0 != filename);
	FLAC__ASSERT(0 != tag);

	if(0 == (file = fopen(filename, "r"))) {
#ifdef DEBUG
		fprintf(stderr, _("ERROR while opening file: '%s' (%s).\n\a"), filename, strerror(errno));
#endif
		return false;
	}
	fclose(file); /* We close it cause id3lib opens/closes file itself */


	/* Get data from tag */
	if(0 != (id3_tag = ID3Tag_New())) {
		ID3Frame *id3_frame;
		ID3Field *id3_field;
		luint frm_size;
		luint num_chars;
		size_t field_num = 0; /* First field */

		/* Link the file to the tag */
		frm_size = local__ID3Tag_Link_wrapper(id3_tag, filename);

		string = malloc(ID3V2_MAX_STRING_LEN+1);

		/*********
		 * Title *
		 *********/
		if(0 != (id3_frame = ID3Tag_FindFrameWithID(id3_tag, ID3FID_TITLE))) {
			if(0 != (id3_field = ID3Frame_GetField(id3_frame, ID3FN_TEXT))) {
				/* Note: if 'num_chars' is equal to 0, then the field is empty or corrupted! */
				if((num_chars = local__ID3Field_GetASCII_wrapper(id3_field, string, ID3V2_MAX_STRING_LEN, field_num)) > 0 && string != NULL) {
					local__strip(string);
					tag->title = strdup(string);
				}
			}
		}


		/************
		 * Composer *
		 ************/
		if(0 != (id3_frame = ID3Tag_FindFrameWithID(id3_tag, ID3FID_COMPOSER))) {
			if(0 != (id3_field = ID3Frame_GetField(id3_frame, ID3FN_TEXT))) {
				if((num_chars = local__ID3Field_GetASCII_wrapper(id3_field, string, ID3V2_MAX_STRING_LEN, field_num)) > 0 && string != NULL) {
					local__strip(string);
					tag->composer = strdup(string);
				}
			}
		}


		/**********
		 * Artist *
		 **********/
		if(0 != (id3_frame = ID3Tag_FindFrameWithID(id3_tag, ID3FID_LEADARTIST))) {
			if(0 != (id3_field = ID3Frame_GetField(id3_frame, ID3FN_TEXT))) {
				if((num_chars = local__ID3Field_GetASCII_wrapper(id3_field, string, ID3V2_MAX_STRING_LEN, field_num)) > 0 && string != NULL) {
					local__strip(string);
					tag->performer = strdup(string);
				}
			}
		}


		/*********
		 * Album *
		 *********/
		if(0 != (id3_frame = ID3Tag_FindFrameWithID(id3_tag, ID3FID_ALBUM))) {
			if(0 != (id3_field = ID3Frame_GetField(id3_frame, ID3FN_TEXT))) {
				if((num_chars = local__ID3Field_GetASCII_wrapper(id3_field, string, ID3V2_MAX_STRING_LEN, field_num)) > 0 && string != NULL) {
					local__strip(string);
					tag->album = strdup(string);
				}
			}
		}


		/********
		 * Year *
		 ********/
		if(0 != (id3_frame = ID3Tag_FindFrameWithID(id3_tag, ID3FID_YEAR))) {
			if(0 != (id3_field = ID3Frame_GetField(id3_frame, ID3FN_TEXT))) {
				if((num_chars = local__ID3Field_GetASCII_wrapper(id3_field, string, ID3V2_MAX_STRING_LEN, field_num)) > 0 && string != NULL) {
					char *tmp_str;

					local__strip(string);

					/* Fix for id3lib 3.7.x: if the id3v1.x tag was filled with spaces
					 * instead of zeroes, then the year field contains garbages! */
					tmp_str = string;
					while (isdigit(*tmp_str)) tmp_str++;
					*tmp_str = 0;
					/* End of fix for id3lib 3.7.x */

					local__strip(string);
					tag->year_recorded = strdup(string);
					tag->year_performed = strdup(string);
				}
			}
		}


		/*************************
		 * Track and Total Track *
		 *************************/
		if(0 != (id3_frame = ID3Tag_FindFrameWithID(id3_tag, ID3FID_TRACKNUM))) {
			if(0 != (id3_field = ID3Frame_GetField(id3_frame, ID3FN_TEXT))) {
				if((num_chars = local__ID3Field_GetASCII_wrapper(id3_field, string, ID3V2_MAX_STRING_LEN, field_num)) > 0 && string != NULL) {
					local__strip(string);

					string1 = strchr(string, '/');
					if (NUMBER_TRACK_FORMATED) {
						if (string1) {
							/* Just to have numbers like this : '01', '05', '12', ... */
							tag->tracks_in_album = malloc(64);
							sprintf(tag->tracks_in_album, "%.2d", atoi(string1+1));
							*string1 = '\0';
						}
						/* Just to have numbers like this : '01', '05', '12', ... */
						tag->track_number = malloc(64);
						sprintf(tag->track_number, "%.2d", atoi(string));
					}
					else {
						if (string1) {
							tag->tracks_in_album = strdup(string1+1);
							*string1 = '\0';
						}
						tag->track_number = strdup(string);
					}
				}
			}
		}


		/*********
		 * Genre *
		 *********/
		if(0 != (id3_frame = ID3Tag_FindFrameWithID(id3_tag, ID3FID_CONTENTTYPE))) {
			if(0 != (id3_field = ID3Frame_GetField(id3_frame, ID3FN_TEXT))) {
				/*
				 * We manipulate only the name of the genre
				 */
				if((num_chars = local__ID3Field_GetASCII_wrapper(id3_field, string, ID3V2_MAX_STRING_LEN, field_num)) > 0 && string != NULL) {
					char *tmp;

					local__strip(string);

					if((string[0]=='(') && (tmp=strchr(string, ')')) && (strlen((tmp+1))>0)) {
						/* Convert a genre written as '(3)Dance' into 'Dance' */
						tag->genre = strdup(tmp+1);

					}
					else if((string[0]=='(') && (tmp=strchr(string, ')'))) {
						/* Convert a genre written as '(3)' into 'Dance' */
						*tmp = 0;
						tag->genre = strdup(local__genre_to_string((unsigned)atoi(string+1)));

					}
					else {
						/* Genre is already written as 'Dance' */
						tag->genre = strdup(string);
					}
				}
			}
		}


		/***********
		 * Comment *
		 ***********/
		if(0 != (id3_frame = ID3Tag_FindFrameWithID(id3_tag, ID3FID_COMMENT))) {
			if(0 != (id3_field = ID3Frame_GetField(id3_frame, ID3FN_TEXT))) {
				if((num_chars = local__ID3Field_GetASCII_wrapper(id3_field, string, ID3V2_MAX_STRING_LEN, field_num)) > 0 && string != NULL) {
					local__strip(string);
					tag->comment = strdup(string);
				}
			}
#if 0
			if(0 != (id3_field = ID3Frame_GetField(id3_frame, ID3FN_DESCRIPTION))) {
				char *comment1 = calloc(MAX_STRING_LEN+1);
				num_chars = ID3Field_GetASCII(id3_field, comment1, MAX_STRING_LEN, Item_Num);
				free(comment1);
			}
			if(0 != (id3_field = ID3Frame_GetField(id3_frame, ID3FN_LANGUAGE))) {
				char *comment2 = calloc(MAX_STRING_LEN+1);
				num_chars = ID3Field_GetASCII(id3_field, comment2, MAX_STRING_LEN, Item_Num);
				free(comment2);
			}
#endif
		}
		free(string);

		/* Free allocated data */
		ID3Tag_Delete(id3_tag);
	}

	return true;
}
#endif /* ifdef FLAC__HAS_ID3LIB */

FLAC__bool FLAC_plugin__id3v2_tag_get(const char *filename, FLAC_Plugin__Id3v2_Tag *tag)
{
	FLAC__ASSERT(0 != tag);
	if(
		0 != tag->title ||
		0 != tag->composer ||
		0 != tag->performer ||
		0 != tag->album ||
		0 != tag->year_recorded ||
		0 != tag->year_performed ||
		0 != tag->track_number ||
		0 != tag->tracks_in_album ||
		0 != tag->genre ||
		0 != tag->comment
	)
		return false;
#ifdef FLAC__HAS_ID3LIB
	return local__get_tag(filename, tag);
#else
	(void)filename, (void)tag;
	return false;
#endif
}

void FLAC_plugin__id3v2_tag_clear(FLAC_Plugin__Id3v2_Tag *tag)
{
	FLAC__ASSERT(0 != tag);
	if(0 != tag->title)
		free(tag->title);
	if(0 != tag->composer)
		free(tag->composer);
	if(0 != tag->performer)
		free(tag->performer);
	if(0 != tag->album)
		free(tag->album);
	if(0 != tag->year_recorded)
		free(tag->year_recorded);
	if(0 != tag->year_performed)
		free(tag->year_performed);
	if(0 != tag->track_number)
		free(tag->track_number);
	if(0 != tag->tracks_in_album)
		free(tag->tracks_in_album);
	if(0 != tag->genre)
		free(tag->genre);
	if(0 != tag->comment)
		free(tag->comment);
	memset(tag, 0, sizeof(*tag));
}
