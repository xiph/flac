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

#ifndef FLAC__PLUGIN_COMMON__CANONICAL_TAG_H
#define FLAC__PLUGIN_COMMON__CANONICAL_TAG_H

#include "id3v1.h"

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
} FLAC_Plugin__CanonicalTag;

FLAC_Plugin__CanonicalTag *FLAC_plugin__canonical_tag_new();
void FLAC_plugin__canonical_tag_delete(FLAC_Plugin__CanonicalTag *);
void FLAC_plugin__canonical_tag_init(FLAC_Plugin__CanonicalTag *);
void FLAC_plugin__canonical_tag_clear(FLAC_Plugin__CanonicalTag *);

/* For each null field in dest, move the corresponding field from src
 * WATCHOUT: note that src is not-const, because fields are 'moved' from
 * src to dest and the src field is set to null.
 */
void FLAC_plugin__canonical_tag_merge(FLAC_Plugin__CanonicalTag *dest, FLAC_Plugin__CanonicalTag *src);

void FLAC_plugin__canonical_tag_convert_from_id3v1(FLAC_Plugin__CanonicalTag *, const FLAC_Plugin__Id3v1_Tag *);

#endif
