/* libxmms-flac - XMMS FLAC input plugin
 * Copyright (C) 2000,2001,2002,2003  Josh Coalson
 * Copyright (C) 2002,2003  Daisuke Shimamura
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

	FLAC_plugin__canonical_tag_init(&tag);

	FLAC_plugin__canonical_tag_get_combined(filename, &tag);

	if(flac_cfg.title.convert_char_set) {
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

	XMMS_NEW_TITLEINPUT(input);

	input->performer = local__getstr(tag.performer);
	if(!input->performer)
		input->performer = local__getstr(tag.composer);
	input->album_name = local__getstr(tag.album);
	input->track_name = local__getstr(tag.title);
	input->track_number = local__getnum(tag.track_number);
	input->year = local__getnum(tag.year_performed);
	input->genre = local__getstr(tag.genre);
	input->comment = local__getstr(tag.comment);

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
	return ret;
}
