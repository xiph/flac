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

#ifndef FLAC__PLUGIN_COMMON__ID3V2_H
#define FLAC__PLUGIN_COMMON__ID3V2_H

#include "FLAC/ordinals.h"

/*
 * This is a simple structure that holds pointers to field values (in ASCII)
 * for fields we care about.
 */
typedef struct {
	char *title;
	char *composer;
	char *performer;
	char *album;
	char *year_recorded;
	char *year_performed;
	char *track_number;
	char *tracks_in_album;
	char *genre;
	char *comment;
} FLAC_Plugin__Id3v2_Tag;

/* Fills up an existing FLAC_Plugin__Id3v2_Tag.  All pointers must be NULL on
 * entry or the function will return false.  For any field for which there is
 * no corresponding ID3 frame, it's pointer will be NULL.
 *
 * If loading fails, all pointers will be cleared and the function will return
 * false.
 *
 * If the function returns true, be sure to call FLAC_plugin__id3v2_tag_clear()
 * when you are done with 'tag'.
 */
FLAC__bool FLAC_plugin__id3v2_tag_get(const char *filename, FLAC_Plugin__Id3v2_Tag *tag);

/* free()s any non-NULL pointers in 'tag'.  Does NOT free(tag).
 */
void FLAC_plugin__id3v2_tag_clear(FLAC_Plugin__Id3v2_Tag *tag);

#endif
