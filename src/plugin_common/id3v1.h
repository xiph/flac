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

#ifndef FLAC__PLUGIN_COMMON__ID3V1_H
#define FLAC__PLUGIN_COMMON__ID3V1_H

#include <string.h>

#include "FLAC/ordinals.h"

typedef struct {
	char tag[3];
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
			char zero;
			unsigned char track;
		} v1_1;
	} comment;
	unsigned char genre;
} FLAC_Plugin__Id3v1_Tag;

FLAC__bool FLAC_plugin__id3v1_tag_get(const char *filename, FLAC_Plugin__Id3v1_Tag *tag);


#define ID3_INVALID_GENRE 255

const char *FLAC_plugin__id3v1_tag_get_genre_as_string(unsigned char genre_code);
unsigned FLAC_plugin__id3v1_tag_genre_table_max();

#endif
