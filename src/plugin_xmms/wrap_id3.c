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

#include "mylocale.h"
#include "configure.h"

#ifdef FLAC__HAS_ID3LIB
#include <id3.h>
#include "id3_tag.h"

#else
#include "charset.h"
#include "genres.h"

typedef struct id3v1tag_t {
	char tag[3]; /* always "TAG": defines ID3v1 tag 128 bytes before EOF */
	char title[30];
	char artist[30];
	char album[30];
	char year[4];
	union {
		struct {
			char comment[30];
		} v1_0;
		struct {
			char comment[28];
			char __zero;
			unsigned char track;
		} v1_1;
	} u;
	unsigned char genre;
} id3v1_struct;

typedef struct id3tag_t {
	char title[64];
	char artist[64];
	char album[64];
	char comment[256];
	char genre[256];
	char year[16];
	char track[16];
} id3v2_struct;

static gboolean local__get_id3v1_tag_as_v2_(const char *filename, id3v2_struct *tag);
static void local__id3v1_to_id3v2(id3v1_struct *v1, id3v2_struct *v2);
static const char *local__get_id3_genre(unsigned char genre_code);
#endif /* FLAC__HAS_ID3LIB */

static gchar *local__extname(const char *filename);
static char* local__getstr(char* str);
static int local__getnum(char* str);

static int local__vcentry_matches(const char *field_name, const FLAC__StreamMetadata_VorbisComment_Entry *entry);
static void local__vcentry_parse_value(const FLAC__StreamMetadata_VorbisComment_Entry *entry, gchar **dest);

/*
 * Function flac_format_song_title (tag, filename)
 *
 *    Create song title according to `tag' and/or `filename' and
 *    return it.  The title must be subsequently freed using g_free().
 *
 */
gchar *flac_format_song_title(gchar * filename)
{
	gchar *ret = NULL;
	TitleInput *input = NULL;
	gboolean rc;
#ifdef FLAC__HAS_ID3LIB
	File_Tag tag;
#else
	id3v2_struct tag;
#endif
	int got_vorbis_comments = 0;

#ifdef FLAC__HAS_ID3LIB
	Initialize_File_Tag_Item (&tag);
#else
	memset(&tag, 0, sizeof(tag));
#endif

	/* first, parse vorbis comments if they exist */
	{
		FLAC__Metadata_SimpleIterator *iterator = FLAC__metadata_simple_iterator_new();
		if(0 != iterator) {
			do {
				if(FLAC__metadata_simple_iterator_get_block_type(iterator) == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
					FLAC__StreamMetadata *block = FLAC__metadata_simple_iterator_get_block(iterator);
					if(0 != block) {
						unsigned i;
						const FLAC__StreamMetadata_VorbisComment *vc = &block->data.vorbis_comment;
						for(i = 0; i < vc->num_comments; i++) {
#if 0
@@@@@
							if(local__vcentry_matches("artist", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag.artist);
							else if(local__vcentry_matches("performer", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag.artist);
							else if(local__vcentry_matches("album", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag.album);
							else if(local__vcentry_matches("title", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag.title);
							else if(local__vcentry_matches("tracknumber", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag.track);
							else if(local__vcentry_matches("genre", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag.genre);
							else if(local__vcentry_matches("description", &vc->comments[i]))
								local__vcentry_parse_value(&vc->comments[i], &tag.comment);
#endif
						}
						FLAC__metadata_object_delete(block);
						got_vorbis_comments = 1;
					}
				}
			} while (!got_vorbis_comments && FLAC__metadata_simple_iterator_next(iterator));
			FLAC__metadata_simple_iterator_delete(iterator);
		}
	}

	if(!got_vorbis_comments) {
#ifdef FLAC__HAS_ID3LIB
		rc = Id3tag_Read_File_Tag(filename, &tag);
#else
		rc = local__get_id3v1_tag_as_v2_(filename, &tag);
#endif
	}

	XMMS_NEW_TITLEINPUT(input);

	if (got_vorbis_comments || rc)
	{
		input->performer = local__getstr(tag.artist);
		input->album_name = local__getstr(tag.album);
		input->track_name = local__getstr(tag.title);
		input->track_number = local__getnum(tag.track);
		input->year = local__getnum(tag.year);
		input->genre = local__getstr(tag.genre);
		input->comment = local__getstr(tag.comment);
	}
	input->file_name = g_basename(filename);
	input->file_path = filename;
	input->file_ext = local__extname(filename);
	ret = xmms_get_titlestring(flac_cfg.tag_override ? flac_cfg.tag_format : xmms_get_gentitle_format(), input);
	g_free(input);

	if (!ret)
	{
		/*
		 * Format according to filename.
		 */
		ret = g_strdup(g_basename(filename));
		if (local__extname(ret) != NULL)
			*(local__extname(ret) - 1) = '\0';	/* removes period */
	}

#ifdef FLAC__HAS_ID3LIB
	Free_File_Tag_Item  (&tag);
#endif
	return ret;
}

#ifndef FLAC__HAS_ID3LIB
/*
 * Function get_idv2_tag_(filename, ID3v2tag)
 *
 *    Get ID3v2 tag from file.
 *
 */
gboolean local__get_id3v1_tag_as_v2_(const char *filename, id3v2_struct *id3v2tag)
{
	FILE *file;
	id3v1_struct id3v1tag;

	memset(id3v2tag, 0, sizeof(id3v2_struct));

	if ((file = fopen(filename, "rb")) != 0)
	{
		if ((fseek(file, -1 * sizeof (id3v1tag), SEEK_END) == 0) &&
		    (fread(&id3v1tag, 1, sizeof (id3v1tag), file) == sizeof (id3v1tag)) &&
		    (strncmp(id3v1tag.tag, "TAG", 3) == 0))
		{
			local__id3v1_to_id3v2(&id3v1tag, id3v2tag);

			if (flac_cfg.convert_char_set)
			{
				gchar *string;

				string = convert_from_file_to_user(id3v2tag->title);
				strcpy(id3v2tag->title, string);
				g_free(string);

				string = convert_from_file_to_user(id3v2tag->artist);
				strcpy(id3v2tag->artist, string);
				g_free(string);

				string = convert_from_file_to_user(id3v2tag->album);
				strcpy(id3v2tag->album, string);
				g_free(string);

				string = convert_from_file_to_user(id3v2tag->comment);
				strcpy(id3v2tag->comment, string);
				g_free(string);

				string = convert_from_file_to_user(id3v2tag->genre);
				strcpy(id3v2tag->genre, string);
				g_free(string);

				string = convert_from_file_to_user(id3v2tag->year);
				strcpy(id3v2tag->year, string);
				g_free(string);

				string = convert_from_file_to_user(id3v2tag->track);
				strcpy(id3v2tag->track, string);
				g_free(string);
			}
		}

	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

/*
 * Function local__id3v1_to_id3v2 (v1, v2)
 *
 *    Convert ID3v1 tag `v1' to ID3v2 tag `v2'.
 *
 */
void local__id3v1_to_id3v2(id3v1_struct *v1, id3v2_struct *v2)
{
	memset(v2,0,sizeof(id3v2_struct));
	strncpy(v2->title, v1->title, 30);
	strncpy(v2->artist, v1->artist, 30);
	strncpy(v2->album, v1->album, 30);
	strncpy(v2->comment, v1->u.v1_0.comment, 30);
	strncpy(v2->genre, local__get_id3_genre(v1->genre), sizeof (v2->genre));
	strncpy(v2->year, v1->year, 4);

	/* Check for v1.1 tags. */
	if (v1->u.v1_1.__zero == 0)
		sprintf(v2->track, "%d", v1->u.v1_1.track);
	else
		strcpy(v2->track, "0");

	g_strstrip(v2->title);
	g_strstrip(v2->artist);
	g_strstrip(v2->album);
	g_strstrip(v2->comment);
	g_strstrip(v2->genre);
	g_strstrip(v2->year);
	g_strstrip(v2->track);
}

const char *local__get_id3_genre(unsigned char genre_code)
{
	if (genre_code < GENRE_MAX)
		return gettext(id3_genres[genre_code]);

	return "";
}
#endif /* ifndef FLAC__HAS_ID3LIB */

/*
 * Function local__extname (filename)
 *
 *    Return pointer within filename to its extenstion, or NULL if
 *    filename has no extension.
 *
 */
gchar *local__extname(const char *filename)
{
	gchar *ext = strrchr(filename, '.');

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

void local__vcentry_parse_value(const FLAC__StreamMetadata_VorbisComment_Entry *entry, gchar **dest)
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
