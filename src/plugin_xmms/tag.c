/* libxmms-flac - XMMS FLAC input plugin
 * Copyright (C) 2000,2001,2002,2003,2004  Josh Coalson
 * Copyright (C) 2002,2003,2004  Daisuke Shimamura
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
#include "plugin_common/canonical_tag.h"
#include "plugin_common/vorbiscomment.h"
#include "charset.h"
#include "configure.h"

/*
 * Function local__extname (filename)
 *
 *    Return pointer within filename to its extenstion, or NULL if
 *    filename has no extension.
 *
 */
static char *local__extname(const char *filename)
{
	char *ext = strrchr(filename, '.');

	if (ext != NULL)
		++ext;

	return ext;
}

static char *local__getstr(char* str)
{
	if (str && strlen(str) > 0)
		return str;
	return NULL;
}

static int local__getnum(char* str)
{
	if (str && strlen(str) > 0)
		return atoi(str);
	return 0;
}

static char *local__getfield(FLAC_Plugin__CanonicalTag *tag, const wchar_t *name)
{
	const wchar_t *ucs2 = FLAC_plugin__canonical_get(tag, name);
	if (0 != ucs2) {
		char *utf8 = FLAC_plugin__convert_ucs2_to_utf8(FLAC_plugin__canonical_get(tag, name));
		if(flac_cfg.title.convert_char_set) {
			char *user = convert_from_utf8_to_user(utf8);
			free(utf8);
			return user;
		}
		else
			return utf8;
	}
	else
		return 0;
}

static void local__safe_free(char *s)
{
	if (0 != s)
		free(s);
}

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
	FLAC_Plugin__CanonicalTag tag;
	char *title, *artist, *performer, *album, *date, *tracknumber, *genre, *description;

	FLAC_plugin__canonical_tag_init(&tag);

	FLAC_plugin__vorbiscomment_get(filename, &tag, /*sep=*/0);

	title       = local__getfield(&tag, L"TITLE");
	artist      = local__getfield(&tag, L"ARTIST");
	performer   = local__getfield(&tag, L"PERFORMER");
	album       = local__getfield(&tag, L"ALBUM");
	date        = local__getfield(&tag, L"DATE");
	tracknumber = local__getfield(&tag, L"TRACKNUMBER");
	genre       = local__getfield(&tag, L"GENRE");
	description = local__getfield(&tag, L"DESCRIPTION");

	XMMS_NEW_TITLEINPUT(input);

	input->performer = local__getstr(performer);
	if(!input->performer)
		input->performer = local__getstr(artist);
	input->album_name = local__getstr(album);
	input->track_name = local__getstr(title);
	input->track_number = local__getnum(tracknumber);
	input->year = local__getnum(date);
	input->genre = local__getstr(genre);
	input->comment = local__getstr(description);

	input->file_name = g_basename(filename);
	input->file_path = filename;
	input->file_ext = local__extname(filename);
	ret = xmms_get_titlestring(flac_cfg.title.tag_override ? flac_cfg.title.tag_format : xmms_get_gentitle_format(), input);
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
	local__safe_free(title);
	local__safe_free(artist);
	local__safe_free(performer);
	local__safe_free(album);
	local__safe_free(date);
	local__safe_free(tracknumber);
	local__safe_free(genre);
	local__safe_free(description);
	return ret;
}
