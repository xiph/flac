/* plugin_common - Routines common to several plugins
 * Copyright (C) 2002,2003,2004  Josh Coalson
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

#include <stdlib.h>
#include <stdio.h>

#include "canonical_tag.h"
#include "id3v2.h"
#include "vorbiscomment.h"
#include "FLAC/assert.h"
#include "FLAC/metadata.h"

static void local__safe_free(void *object)
{
	if(0 != object)
		free(object);
}

static void local__copy_field(char **dest, const char *src, unsigned n)
{
	if(n > 0) {
		const char *p = src + n;
		while(p > src && *(--p) == ' ')
			;
		n = p - src + 1;
		if(0 != (*dest = malloc(n+1))) {
			memcpy(*dest, src, n);
			(*dest)[n] = '\0';
		}
	}
	else
		*dest = 0;
}

static FLAC__bool local__get_id3v1_tag_as_canonical(const char *filename, FLAC_Plugin__CanonicalTag *tag)
{
	FLAC_Plugin__Id3v1_Tag id3v1_tag;

	if(FLAC_plugin__id3v1_tag_get(filename, &id3v1_tag)) {
		FLAC_plugin__canonical_tag_convert_from_id3v1(tag, &id3v1_tag);
		return true;
	}
	return false;
}

FLAC_Plugin__CanonicalTag *FLAC_plugin__canonical_tag_new()
{
	FLAC_Plugin__CanonicalTag *object = (FLAC_Plugin__CanonicalTag*)malloc(sizeof(FLAC_Plugin__CanonicalTag));
	if(0 != object)
		FLAC_plugin__canonical_tag_init(object);
	return object;
}

void FLAC_plugin__canonical_tag_delete(FLAC_Plugin__CanonicalTag *object)
{
	FLAC__ASSERT(0 != object);
	FLAC_plugin__canonical_tag_clear(object);
	free(object);
}

void FLAC_plugin__canonical_tag_init(FLAC_Plugin__CanonicalTag *object)
{
	FLAC__ASSERT(0 != object);
	object->title = 0;
	object->composer = 0;
	object->performer = 0;
	object->album = 0;
	object->year_recorded = 0;
	object->year_performed = 0;
	object->track_number = 0;
	object->tracks_in_album = 0;
	object->genre = 0;
	object->comment = 0;
}

void FLAC_plugin__canonical_tag_clear(FLAC_Plugin__CanonicalTag *object)
{
	FLAC__ASSERT(0 != object);
	local__safe_free(object->title);
	local__safe_free(object->composer);
	local__safe_free(object->performer);
	local__safe_free(object->album);
	local__safe_free(object->year_recorded);
	local__safe_free(object->year_performed);
	local__safe_free(object->track_number);
	local__safe_free(object->tracks_in_album);
	local__safe_free(object->genre);
	local__safe_free(object->comment);
	FLAC_plugin__canonical_tag_init(object);
}

static void local__grab(char **dest, char **src)
{
	if(0 == *dest) {
		*dest = *src;
		*src = 0;
	}
}

void FLAC_plugin__canonical_tag_merge(FLAC_Plugin__CanonicalTag *dest, FLAC_Plugin__CanonicalTag *src)
{
	local__grab(&dest->title, &src->title);
	local__grab(&dest->composer, &src->composer);
	local__grab(&dest->performer, &src->performer);
	local__grab(&dest->album, &src->album);
	local__grab(&dest->year_recorded, &src->year_recorded);
	local__grab(&dest->year_performed, &src->year_performed);
	local__grab(&dest->track_number, &src->track_number);
	local__grab(&dest->tracks_in_album, &src->tracks_in_album);
	local__grab(&dest->genre, &src->genre);
	local__grab(&dest->comment, &src->comment);
}

void FLAC_plugin__canonical_tag_convert_from_id3v1(FLAC_Plugin__CanonicalTag *object, const FLAC_Plugin__Id3v1_Tag *id3v1_tag)
{
	local__copy_field(&object->title, id3v1_tag->title, 30);
	local__copy_field(&object->composer, id3v1_tag->artist, 30);
	local__copy_field(&object->performer, id3v1_tag->artist, 30);
	local__copy_field(&object->album, id3v1_tag->album, 30);
	local__copy_field(&object->year_performed, id3v1_tag->year, 4);

	/* Check for v1.1 tags. */
	if (id3v1_tag->comment.v1_1.zero == 0) {
		if(0 != (object->track_number = malloc(4)))
			sprintf(object->track_number, "%u", (unsigned)id3v1_tag->comment.v1_1.track);
		local__copy_field(&object->comment, id3v1_tag->comment.v1_1.comment, 28);
	}
	else {
		object->track_number = strdup("0");
		local__copy_field(&object->comment, id3v1_tag->comment.v1_0.comment, 30);
	}

	object->genre = strdup(FLAC_plugin__id3v1_tag_get_genre_as_string(id3v1_tag->genre));
}

void FLAC_plugin__canonical_tag_get_combined(const char *filename, FLAC_Plugin__CanonicalTag *tag)
{
	FLAC_Plugin__CanonicalTag id3v1_tag, id3v2_tag;

	FLAC_plugin__vorbiscomment_get(filename, tag);

	FLAC_plugin__canonical_tag_init(&id3v2_tag);
	(void)FLAC_plugin__id3v2_tag_get(filename, &id3v2_tag);

	FLAC_plugin__canonical_tag_init(&id3v1_tag);
	(void)local__get_id3v1_tag_as_canonical(filename, &id3v1_tag);

	/* merge tags, preferring, in order: vorbis comments, id3v2, id3v1 */
	FLAC_plugin__canonical_tag_merge(tag, &id3v2_tag);
	FLAC_plugin__canonical_tag_merge(tag, &id3v1_tag);

	FLAC_plugin__canonical_tag_clear(&id3v1_tag);
	FLAC_plugin__canonical_tag_clear(&id3v2_tag);
}
