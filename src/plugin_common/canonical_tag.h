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

#ifndef FLAC__PLUGIN_COMMON__CANONICAL_TAG_H
#define FLAC__PLUGIN_COMMON__CANONICAL_TAG_H

#include "FLAC/ordinals.h"

/* TODO: splay tree? */
typedef struct tagFLAC__tag_entry FLAC__tag_entry;
struct tagFLAC__tag_entry
{
	FLAC__tag_entry *next, *prev;
	/* TODO: name in ascii? */
	wchar_t *name;
	wchar_t *value;
};

typedef struct {
	FLAC__tag_entry *head, *tail;
	unsigned count;
} FLAC_Plugin__CanonicalTag;


typedef FLAC__tag_entry *FLAC__tag_iterator;

FLAC_Plugin__CanonicalTag *FLAC_plugin__canonical_tag_new();
void FLAC_plugin__canonical_tag_init(FLAC_Plugin__CanonicalTag *);
void FLAC_plugin__canonical_tag_clear(FLAC_Plugin__CanonicalTag *);
void FLAC_plugin__canonical_tag_delete(FLAC_Plugin__CanonicalTag *);

/* note that multiple fields with the same name are allowed.
 * set - adds field if it's not present yet, or replaces
 * existing field if it's present.
 */
void FLAC_plugin__canonical_set(FLAC_Plugin__CanonicalTag *tag, const wchar_t *name, const wchar_t *value);
void FLAC_plugin__canonical_set_ansi(FLAC_Plugin__CanonicalTag *tag, const wchar_t *name, const char *value);
/* set_new - only adds field if it's not present yet. */
void FLAC_plugin__canonical_set_new(FLAC_Plugin__CanonicalTag *tag, const wchar_t *name, const wchar_t *value);
/* add - adds field if it's not present yet, or merges value with existing
 * field, if it's present. (sep - separator string to use when merging;
 * if sep==NULL no merging occurs - always adds new field)
 */
void FLAC_plugin__canonical_add(FLAC_Plugin__CanonicalTag *tag, const wchar_t *name, const wchar_t *value, const wchar_t *sep);
void FLAC_plugin__canonical_add_utf8(FLAC_Plugin__CanonicalTag *tag, const char *name, const char *value, unsigned namelen, unsigned vallen, const char *sep); /* 'namelen'/'vallen' may be (unsigned)(-1) if 'name'/'value' is NUL-terminated */

/* gets value of the first field with the given name (NULL if field not found) */
const wchar_t *FLAC_plugin__canonical_get(const FLAC_Plugin__CanonicalTag *tag, const wchar_t *name);
/* removes first field with the given name.
 * (returns 'true' if deleted, 'false' if not found)
 */
FLAC__bool FLAC_plugin__canonical_remove(FLAC_Plugin__CanonicalTag *tag, const wchar_t *name);
/* removes all fields with the given name. */
void FLAC_plugin__canonical_remove_all(FLAC_Plugin__CanonicalTag *tag, const wchar_t *name);

/* enumeration */
unsigned FLAC_plugin__canonical_get_count(FLAC_Plugin__CanonicalTag *tag);
FLAC__tag_iterator FLAC_plugin__canonical_first(FLAC_Plugin__CanonicalTag *tag);
FLAC__tag_iterator FLAC_plugin__canonical_next(FLAC__tag_iterator it);
wchar_t *FLAC_plugin__canonical_get_name(FLAC__tag_iterator it);
wchar_t *FLAC_plugin__canonical_get_value(FLAC__tag_iterator it);

/* returns a new string containing the current entry in UTF-8 in "NAME=VALUE" form */
char *FLAC_plugin__canonical_get_formatted(FLAC__tag_iterator it);

void FLAC_plugin__canonical_tag_merge(FLAC_Plugin__CanonicalTag *dest, const FLAC_Plugin__CanonicalTag *src);

/* helpers */
wchar_t *FLAC_plugin__convert_ansi_to_wide(const char *src);
wchar_t *FLAC_plugin__convert_utf8_to_ucs2(const char *src, unsigned length); /* 'length' may be (unsigned)(-1) if 'src' is NUL-terminated */
char    *FLAC_plugin__convert_ucs2_to_utf8(const wchar_t *src);

#endif
