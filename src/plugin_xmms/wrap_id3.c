/* libxmms-flac - XMMS FLAC input plugin
 * Copyright (C) 2000,2001,2002  Josh Coalson
 * Copyright (C) 2002  Daisuke Shimamura
 *
 * Based on FLAC plugin.c and mpg123 plugin
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
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <xmms/plugin.h>
#include <xmms/util.h>
#include <xmms/configfile.h>
#include <xmms/titlestring.h>

#include "FLAC/metadata.h"
#include "plugin_common/id3v1.h"
#include "plugin_common/id3v2.h"
#include "charset.h"
#include "configure.h"

static FLAC__bool local__get_id3v1_tag_as_canonical(const char *filename, FLAC_Plugin__CanonicalTag *tag);

static char *local__extname(const char *filename);
static char* local__getstr(char* str);
static int local__getnum(char* str);

static int local__vcentry_matches(const char *field_name, const FLAC__StreamMetadata_VorbisComment_Entry *entry);
static void local__vcentry_parse_value(const FLAC__StreamMetadata_VorbisComment_Entry *entry, char **dest);

/*
 * Function flac_format_song_title (tag, filename)
 *
 *    Create song title according to `tag' and/or `filename' and
 *    return it.  The title must be subsequently freed using g_free().
 *
 */
char *flac_format_song_title(char *filename)
{
	char *ret = NULL;
	TitleInput *input = NULL;
	FLAC_Plugin__CanonicalTag tag, id3v1_tag, id3v2_tag;

	FLAC_plugin__canonical_tag_init(&tag);
	FLAC_plugin__canonical_tag_init(&id3v1_tag);
	FLAC_plugin__canonical_tag_init(&id3v2_tag);

	/* first, parse vorbis comments if they exist */
	{
		FLAC__Metadata_SimpleIterator *iterator = FLAC__metadata_simple_iterator_new();
		if(0 != iterator) {
			FLAC__bool got_vorbis_comments = false;
			do {
				if(FLAC__metadata_simple_iterator_get_block_type(iterator) == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
					FLAC__StreamMetadata *block = FLAC__metadata_simple_iterator_get_block(iterator);
					if(0 != block) {
						unsigned i;
						const FLAC__StreamMetadata_VorbisComment *vc = &block->data.vorbis_comment;
						for(i = 0; i < vc->num_comments; i++) {
							if(local__vcentry_matches("artist", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag.composer);
							else if(local__vcentry_matches("performer", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag.performer);
							else if(local__vcentry_matches("album", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag.album);
							else if(local__vcentry_matches("title", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag.title);
							else if(local__vcentry_matches("tracknumber", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag.track_number);
							else if(local__vcentry_matches("genre", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag.genre);
							else if(local__vcentry_matches("description", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag.comment);
						}
						FLAC__metadata_object_delete(block);
						got_vorbis_comments = true;
					}
				}
			} while (!got_vorbis_comments && FLAC__metadata_simple_iterator_next(iterator));
			FLAC__metadata_simple_iterator_delete(iterator);
		}
	}

	(void)FLAC_plugin__id3v2_tag_get(filename, &id3v2_tag);
	(void)local__get_id3v1_tag_as_canonical(filename, &id3v1_tag);

	XMMS_NEW_TITLEINPUT(input);

	/* merge tags, preferring, in order: vorbis comments, id3v2, id3v1 */
	FLAC_plugin__canonical_tag_merge(&tag, &id3v2_tag);
	FLAC_plugin__canonical_tag_merge(&tag, &id3v1_tag);

	if(flac_cfg.convert_char_set) {
		convert_from_file_to_user_in_place(&tag.title);
		convert_from_file_to_user_in_place(&tag.composer);
		convert_from_file_to_user_in_place(&tag.performer);
		convert_from_file_to_user_in_place(&tag.album);
		convert_from_file_to_user_in_place(&tag.year_recorded);
		convert_from_file_to_user_in_place(&tag.year_performed);
		convert_from_file_to_user_in_place(&tag.track_number);
		convert_from_file_to_user_in_place(&tag.tracks_in_album);
		convert_from_file_to_user_in_place(&tag.genre);
		convert_from_file_to_user_in_place(&tag.comment);
	}

	input->performer = local__getstr(tag.performer);
	input->album_name = local__getstr(tag.album);
	input->track_name = local__getstr(tag.title);
	input->track_number = local__getnum(tag.track_number);
	input->year = local__getnum(tag.year_performed);
	input->genre = local__getstr(tag.genre);
	input->comment = local__getstr(tag.comment);

	input->file_name = g_basename(filename);
	input->file_path = filename;
	input->file_ext = local__extname(filename);
	ret = xmms_get_titlestring(flac_cfg.tag_override ? flac_cfg.tag_format : xmms_get_gentitle_format(), input);
	g_free(input);

	if (!ret) {
		/*
		 * Format according to filename.
		 */
		ret = g_strdup(g_basename(filename));
		if (local__extname(ret) != NULL)
			*(local__extname(ret) - 1) = '\0';	/* removes period */
	}

	FLAC_plugin__canonical_tag_clear(&tag);
	FLAC_plugin__canonical_tag_clear(&id3v1_tag);
	FLAC_plugin__canonical_tag_clear(&id3v2_tag);
	return ret;
}

FLAC__bool local__get_id3v1_tag_as_canonical(const char *filename, FLAC_Plugin__CanonicalTag *tag)
{
	FLAC_Plugin__Id3v1_Tag id3v1_tag;

	if(FLAC_plugin__id3v1_tag_get(filename, &id3v1_tag)) {
		FLAC_plugin__canonical_tag_convert_from_id3v1(tag, &id3v1_tag);
		return true;
	}
	return false;
}

/*
 * Function local__extname (filename)
 *
 *    Return pointer within filename to its extenstion, or NULL if
 *    filename has no extension.
 *
 */
char *local__extname(const char *filename)
{
	char *ext = strrchr(filename, '.');

	if (ext != NULL)
		++ext;

	return ext;
}

char* local__getstr(char* str)
{
	if (str && strlen(str) > 0)
		return str;
	return NULL;
}

int local__getnum(char* str)
{
	if (str && strlen(str) > 0)
		return atoi(str);
	return 0;
}

int local__vcentry_matches(const char *field_name, const FLAC__StreamMetadata_VorbisComment_Entry *entry)
{
	const FLAC__byte *eq = memchr(entry->entry, '=', entry->length);
	const unsigned field_name_length = strlen(field_name);
	return (0 != eq && (unsigned)(eq-entry->entry) == field_name_length && 0 == strncasecmp(field_name, entry->entry, field_name_length));
}

void local__vcentry_parse_value(const FLAC__StreamMetadata_VorbisComment_Entry *entry, char **dest)
{
	const FLAC__byte *eq = memchr(entry->entry, '=', entry->length);

	if(0 == eq)
		return;
	else {
		const unsigned value_length = entry->length - (unsigned)((++eq) - entry->entry);

		*dest = g_malloc(value_length + 1);
		if(0 != *dest) {
			memcpy(*dest, eq, value_length);
			(*dest)[value_length] = '\0';
		}
	}
}
