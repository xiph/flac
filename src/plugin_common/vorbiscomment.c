/* plugin_common - Routines common to several plugins
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

#include <string.h>
#include <stdlib.h>

#include "vorbiscomment.h"
#include "FLAC/metadata.h"

static int local__vcentry_matches(const char *field_name, const FLAC__StreamMetadata_VorbisComment_Entry *entry)
{
	const FLAC__byte *eq = memchr(entry->entry, '=', entry->length);
	const unsigned field_name_length = strlen(field_name);
	return (0 != eq && (unsigned)(eq-entry->entry) == field_name_length && 0 == strncasecmp(field_name, entry->entry, field_name_length));
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
