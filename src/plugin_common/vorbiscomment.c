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


static void local__add_vcentry(FLAC_Plugin__CanonicalTag *tag, FLAC__StreamMetadata_VorbisComment_Entry *entry, const char *sep)
{
	FLAC__byte *value = memchr(entry->entry, '=', entry->length);
	int len;
	if (!value) return;
	len = value - entry->entry;
	value++;
	FLAC_plugin__canonical_add_utf8(tag, entry->entry, value, len, entry->length-len-1, sep);
}

void FLAC_plugin__vorbiscomment_get(const char *filename, FLAC_Plugin__CanonicalTag *tag, const char *sep)
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

						for(i = 0; i < vc->num_comments; i++)
							local__add_vcentry(tag, vc->comments+i, sep);

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
	FLAC__bool got_vorbis_comments = false, result;
	FLAC__Metadata_SimpleIterator *iterator = FLAC__metadata_simple_iterator_new();
	FLAC__StreamMetadata *block;
	FLAC__tag_iterator it;
	unsigned position = 0;
		
	if (!iterator || !FLAC__metadata_simple_iterator_init(iterator, filename, /*read_only=*/false, /*preserve_file_stats=*/true))
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

	FLAC__metadata_object_vorbiscomment_resize_comments(block, FLAC_plugin__canonical_get_count(tag));

	for (it=FLAC_plugin__canonical_first(tag); it; it=FLAC_plugin__canonical_next(it))
	{
		FLAC__StreamMetadata_VorbisComment_Entry entry;
		/* replace entry */
		entry.entry = FLAC_plugin__canonical_get_formatted(it);
		entry.length = strlen(entry.entry);
		FLAC__metadata_object_vorbiscomment_set_comment(block, position++, entry, /*copy=*/false);
	}

	if (!got_vorbis_comments)
		result = FLAC__metadata_simple_iterator_insert_block_after(iterator, block, /*use_padding=*/true);
	else
		result = FLAC__metadata_simple_iterator_set_block(iterator, block, /*use_padding=*/true);

	FLAC__metadata_object_delete(block);
	FLAC__metadata_simple_iterator_delete(iterator);
	return result;
}
