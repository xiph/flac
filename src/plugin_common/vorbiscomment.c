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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vorbiscomment.h"
#include "FLAC/metadata.h"

static int local__vcentry_matches(const char *field_name, const FLAC__StreamMetadata_VorbisComment_Entry *entry)
{
#if defined _MSC_VER || defined __MINGW32__
#define FLAC__STRNCASECMP strnicmp
#else
#define FLAC__STRNCASECMP strncasecmp
#endif
	const FLAC__byte *eq = memchr(entry->entry, '=', entry->length);
	const unsigned field_name_length = strlen(field_name);
	return (0 != eq && (unsigned)(eq-entry->entry) == field_name_length && 0 == FLAC__STRNCASECMP(field_name, (const char *)entry->entry, field_name_length));
}

static void local__vcentry_parse_value(const FLAC__StreamMetadata_VorbisComment_Entry *entry, char **dest)
{
	const FLAC__byte *eq = memchr(entry->entry, '=', entry->length);

	if(0 == eq)
		return;
	else {
		const unsigned value_length = entry->length - (unsigned)((++eq) - entry->entry);

		*dest = malloc(value_length + 1);
		if(0 != *dest) {
			memcpy(*dest, eq, value_length);
			(*dest)[value_length] = '\0';
		}
	}
}

static void local__vc_change_field(FLAC__StreamMetadata *block, const char *name, const char *value)
{
	int i, l;
	char *s;

	/* find last */
	for (l = -1; (i = FLAC__metadata_object_vorbiscomment_find_entry_from(block, l + 1, name)) != -1; l = i)
		;
			
	if(!value || !strlen(value)) {
		if (l != -1)
			FLAC__metadata_object_vorbiscomment_delete_comment(block, l);
		return;
	}

	s = malloc(strlen(name) + strlen(value) + 2);
	if(s) {
		FLAC__StreamMetadata_VorbisComment_Entry entry;

		sprintf(s, "%s=%s", name, value);

		entry.length = strlen(s);
		entry.entry = (FLAC__byte *)s;
		
		if(l == -1)
			FLAC__metadata_object_vorbiscomment_insert_comment(block, block->data.vorbis_comment.num_comments, entry, /*copy=*/true);
		else
			FLAC__metadata_object_vorbiscomment_set_comment(block, l, entry, /*copy=*/true);
		free(s);
	}
}

void FLAC_plugin__vorbiscomment_get(const char *filename, FLAC_Plugin__CanonicalTag *tag)
{
	FLAC__Metadata_SimpleIterator *iterator = FLAC__metadata_simple_iterator_new();
	if(0 != iterator) {
		if(FLAC__metadata_simple_iterator_init(iterator, filename, /*read_only=*/true, /*preserve_file_stats=*/true)) {
			FLAC__bool got_vorbis_comments = false;
			do {
				if(FLAC__metadata_simple_iterator_get_block_type(iterator) == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
					FLAC__StreamMetadata *block = FLAC__metadata_simple_iterator_get_block(iterator);
					if(0 != block) {
						unsigned i;
						const FLAC__StreamMetadata_VorbisComment *vc = &block->data.vorbis_comment;
						for(i = 0; i < vc->num_comments; i++) {
							if(local__vcentry_matches("artist", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag->composer);
							else if(local__vcentry_matches("performer", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag->performer);
							else if(local__vcentry_matches("album", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag->album);
							else if(local__vcentry_matches("title", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag->title);
							else if(local__vcentry_matches("tracknumber", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag->track_number);
							else if(local__vcentry_matches("genre", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag->genre);
							else if(local__vcentry_matches("description", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag->comment);
							else if(local__vcentry_matches("date", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag->year_recorded);
						}
						FLAC__metadata_object_delete(block);
						got_vorbis_comments = true;
					}
				}
			} while (!got_vorbis_comments && FLAC__metadata_simple_iterator_next(iterator));
		}
		FLAC__metadata_simple_iterator_delete(iterator);
	}
}

FLAC__bool FLAC_plugin__vorbiscomment_set(const char *filename, FLAC_Plugin__CanonicalTag *tag)
{
	FLAC__bool got_vorbis_comments = false;
	FLAC__Metadata_SimpleIterator *iterator = FLAC__metadata_simple_iterator_new();
	FLAC__StreamMetadata *block;
		
	if(!iterator || !FLAC__metadata_simple_iterator_init(iterator, filename, /*read_only=*/false, /*preserve_file_stats=*/true))
		return false;

	do {
		if(FLAC__metadata_simple_iterator_get_block_type(iterator) == FLAC__METADATA_TYPE_VORBIS_COMMENT)
			got_vorbis_comments = true;
	} while (!got_vorbis_comments && FLAC__metadata_simple_iterator_next(iterator));

	if(!got_vorbis_comments) {
		/* create a new block */
		block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);

		if(!block) {
			FLAC__metadata_simple_iterator_delete(iterator);
			return false;
		}
	}
	else
		block = FLAC__metadata_simple_iterator_get_block(iterator);

	local__vc_change_field(block, "ARTIST", tag->composer);
	local__vc_change_field(block, "PERFORMER", tag->performer);
	local__vc_change_field(block, "ALBUM", tag->album);
	local__vc_change_field(block, "TITLE", tag->title);
	local__vc_change_field(block, "TRACKNUMBER", tag->track_number);
	local__vc_change_field(block, "GENRE", tag->genre);
	local__vc_change_field(block, "DESCRIPTION", tag->comment);
	local__vc_change_field(block, "DATE", tag->year_recorded);

	if(!got_vorbis_comments) {
		if(!FLAC__metadata_simple_iterator_insert_block_after(iterator, block, /*use_padding=*/true)) {
			FLAC__metadata_object_delete(block);
			FLAC__metadata_simple_iterator_delete(iterator);
			return false;
		}
	}
	else {
		if(!FLAC__metadata_simple_iterator_set_block(iterator, block, /*use_padding=*/true)) {
			FLAC__metadata_object_delete(block);
			FLAC__metadata_simple_iterator_delete(iterator);
			return false;
		}
	}

	FLAC__metadata_object_delete(block);
	FLAC__metadata_simple_iterator_delete(iterator);
	return true;
}
