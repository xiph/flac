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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h> /* for strlen() and memcpy() */

#include "canonical_tag.h"
#include "vorbiscomment.h"
#include "FLAC/assert.h"
#include "FLAC/metadata.h"

#include <wchar.h>

/*
 * Here lies hackage to get any missing wide character string functions we
 * need.  The fallback implementations here are from glibc.
 */

#if !defined(_MSC_VER) && !defined(HAVE_WCSDUP)
/* Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <wchar.h>
#include <string.h>
#include <stdlib.h>


/* Duplicate S, returning an identical malloc'd string.	 */
wchar_t *
wcsdup (s)
     const wchar_t *s;
{
  size_t len = (__wcslen (s) + 1) * sizeof (wchar_t);
  void *new = malloc (len);

  if (new == NULL)
    return NULL;

  return (wchar_t *) memcpy (new, (void *) s, len);
}
#endif

#if !defined(_MSC_VER) && !defined(HAVE_WCSCASECMP)
/* Copyright (C) 1991, 1992, 1995, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <wctype.h>
#include <wchar.h>

#ifndef weak_alias
# define __wcscasecmp wcscasecmp
# define TOLOWER(Ch) towlower (Ch)
#else
# ifdef USE_IN_EXTENDED_LOCALE_MODEL
#  define __wcscasecmp __wcscasecmp_l
#  define TOLOWER(Ch) __towlower_l ((Ch), loc)
# else
#  define TOLOWER(Ch) towlower (Ch)
# endif
#endif

#ifdef USE_IN_EXTENDED_LOCALE_MODEL
# define LOCALE_PARAM , loc
# define LOCALE_PARAM_DECL __locale_t loc;
#else
# define LOCALE_PARAM
# define LOCALE_PARAM_DECL
#endif

/* Compare S1 and S2, ignoring case, returning less than, equal to or
   greater than zero if S1 is lexicographically less than,
   equal to or greater than S2.  */
int
__wcscasecmp (s1, s2 LOCALE_PARAM)
     const wchar_t *s1;
     const wchar_t *s2;
     LOCALE_PARAM_DECL
{
  wint_t c1, c2;

  if (s1 == s2)
    return 0;

  do
    {
      c1 = TOLOWER (*s1++);
      c2 = TOLOWER (*s2++);
      if (c1 == L'\0')
	break;
    }
  while (c1 == c2);

  return c1 - c2;
}
#ifndef __wcscasecmp
weak_alias (__wcscasecmp, wcscasecmp)
#endif
#endif

/*
 *  helpers
 */

/* TODO: should be moved out somewhere? @@@ */

wchar_t *FLAC_plugin__convert_ansi_to_wide(const char *src)
{
	int len;
	wchar_t *dest;

	FLAC__ASSERT(0 != src);

	len = strlen(src) + 1;
	/* copy */
	dest = malloc(len*sizeof(wchar_t));
	if (dest) mbstowcs(dest, src, len);
	return dest;
}

/* TODO: more validation? @@@ */
static __inline int utf8len(const FLAC__byte *utf8)
{
	FLAC__ASSERT(0 != utf8);
	if ((*utf8 & 0x80) == 0)
		return 1;
	else if ((*utf8 & 0xE0) == 0xC0)
		return 2;
	else if ((*utf8 & 0xF0) == 0xE0)
		return 3;
	else return 0;
}

/* TODO: validation? @@@ */
static __inline int utf8_to_ucs2(const FLAC__byte *utf8, wchar_t *ucs2)
{
	int len;
	FLAC__ASSERT(utf8!=0 && *utf8!=0 && ucs2!=0);

	if (!(len = utf8len(utf8))) return 0;

	if (len == 1)
		*ucs2 = *utf8;
	else if (len == 2)
		*ucs2 = (*utf8 & 0x3F)<<6 | (*(utf8+1) & 0x3F);
	else if (len == 3)
		*ucs2 = (*utf8 & 0x1F)<<12 | (*(utf8+1) & 0x3F)<<6 | (*(utf8+2) & 0x3F);
	else {
		FLAC__ASSERT(len == 0);
	}

	return len;
}

wchar_t *FLAC_plugin__convert_utf8_to_ucs2(const char *src, unsigned length)
{
	wchar_t *out, *p;
	const char *s;
	int len = 0;
	/* calculate length */
	for (s=src; length && *s; len++)
	{
		int l = utf8len(s);
		if (!l) break;
		s += l;
		length -= l;
	}
	/* allocate */
	len++;
	p = out = (wchar_t*)malloc(len * sizeof(wchar_t));
	if (!out) return NULL;
	/* convert */
	for (s=src; --len; p++)
	{
		int l = utf8_to_ucs2(s, p);
		/* l==0 is possible, because real conversion */
		/* might do more careful validation */
		if (!l) break;
		s += l;
	}
	*p = 0;

	return out;
}

static __inline int ucs2len(wchar_t ucs2)
{
	if (ucs2 < 0x0080)
		return 1;
	else if (ucs2 < 0x0800)
		return 2;
	else return 3;
}

static __inline int ucs2_to_utf8(wchar_t ucs2, FLAC__byte *utf8)
{
	if (ucs2 < 0x080)
	{
		utf8[0] = (FLAC__byte)ucs2;
		return 1;
	}
	else if (ucs2 < 0x800)
	{
		utf8[0] = 0xc0 | (ucs2 >> 6);
		utf8[1] = 0x80 | (ucs2 & 0x3f);
		return 2;
	}
	else
	{
		utf8[0] = 0xe0 | (ucs2 >> 12);
		utf8[1] = 0x80 | ((ucs2 >> 6) & 0x3f);
		utf8[2] = 0x80 | (ucs2 & 0x3f);
		return 3;
	}
}

char *FLAC_plugin__convert_ucs2_to_utf8(const wchar_t *src)
{
	const wchar_t *s;
	char *out, *p;
	int len = 0;
	FLAC__ASSERT(0 != src);
	/* calculate length */
	for (s=src; *s; s++)
		len += ucs2len(*s);
	/* allocate */
	len++;
	p = out = malloc(len);
	if (!out) return NULL;
	/* convert */
	for (s=src; *s; s++)
	{
		int l = ucs2_to_utf8(*s, p);
		p += l;
	}
	*p = 0;

	return out;
}

/*
 *  init/clear/delete
 */

FLAC_Plugin__CanonicalTag *FLAC_plugin__canonical_tag_new()
{
	FLAC_Plugin__CanonicalTag *object = (FLAC_Plugin__CanonicalTag*)malloc(sizeof(FLAC_Plugin__CanonicalTag));
	if (object != 0)
		FLAC_plugin__canonical_tag_init(object);
	return object;
}

void FLAC_plugin__canonical_tag_delete(FLAC_Plugin__CanonicalTag *object)
{
	FLAC_plugin__canonical_tag_clear(object);
	free(object);
}

void FLAC_plugin__canonical_tag_init(FLAC_Plugin__CanonicalTag *object)
{
	object->head = object->tail = 0;
	object->count = 0;
}

static void FLAC_plugin__canonical_tag_clear_entry(FLAC__tag_entry *entry)
{
	free(entry->name);
	free(entry->value);
	free(entry);
}

void FLAC_plugin__canonical_tag_clear(FLAC_Plugin__CanonicalTag *object)
{
	FLAC__tag_entry *entry = object->head;

	while (entry)
	{
		FLAC__tag_entry *next = entry->next;
		FLAC_plugin__canonical_tag_clear_entry(entry);
		entry = next;
	}

	FLAC_plugin__canonical_tag_init(object);
}

/*
 *  internal
 */

static FLAC__tag_entry *FLAC_plugin__canonical_find(const FLAC_Plugin__CanonicalTag *tag, const wchar_t *name)
{
	FLAC__tag_entry *entry = tag->head;

	while (entry)
	{
#if defined _MSC_VER || defined __MINGW32__
#define FLAC__WCSCASECMP wcsicmp
#else
#define FLAC__WCSCASECMP wcscasecmp
#endif
		if (!FLAC__WCSCASECMP(name, entry->name))
#undef FLAC__WCSCASECMP
			break;
		entry = entry->next;
	}

	return entry;
}

/* NOTE: does NOT copy strings. takes ownership over passed strings. */
static void FLAC_plugin__canonical_add_tail(FLAC_Plugin__CanonicalTag *tag, wchar_t *name, wchar_t *value)
{
	FLAC__tag_entry *entry = (FLAC__tag_entry*)malloc(sizeof(FLAC__tag_entry));
	if (!entry)
	{
		free(name);
		free(value);
		return;
	}
	/* init */
	entry->name = name;
	entry->value = value;
	/* add */
	entry->prev = tag->tail;
	if (tag->tail)
		tag->tail->next = entry;
	tag->tail = entry;
	if (!tag->head)
		tag->head = entry;
	entry->next = 0;
	tag->count++;
}

static void FLAC_plugin__canonical_add_new(FLAC_Plugin__CanonicalTag *tag, const wchar_t *name, const wchar_t *value)
{
	FLAC_plugin__canonical_add_tail(tag, wcsdup(name), wcsdup(value));
}

/* NOTE: does NOT copy value, but copies name */
static void FLAC_plugin__canonical_set_nc(FLAC_Plugin__CanonicalTag *tag, const wchar_t *name, wchar_t *value)
{
	FLAC__tag_entry *entry = FLAC_plugin__canonical_find(tag, name);

	if (entry)
	{
		free(entry->value);
		entry->value = value;
	}
	else FLAC_plugin__canonical_add_tail(tag, wcsdup(name), value);
}

/* NOTE: does NOT copy strings. takes ownership over passed strings. (except sep!) */
static void FLAC_plugin__canonical_add_nc(FLAC_Plugin__CanonicalTag *tag, wchar_t *name, wchar_t *value, const wchar_t *sep)
{
	FLAC__tag_entry *entry;

	if (sep && (entry = FLAC_plugin__canonical_find(tag, name)))
	{
		unsigned newlen = wcslen(entry->value) + wcslen(value) + wcslen(sep) + 1;
		wchar_t *newvalue = realloc(entry->value, newlen*sizeof(wchar_t));

		if (newvalue)
		{
			entry->value = newvalue;
			wcscat(entry->value, sep);
			wcscat(entry->value, value);
		}

		free(name);
		free(value);
	}
	else FLAC_plugin__canonical_add_tail(tag, name, value);
}

/*
 *  manipulation
 */

void FLAC_plugin__canonical_set(FLAC_Plugin__CanonicalTag *tag, const wchar_t *name, const wchar_t *value)
{
	FLAC_plugin__canonical_set_nc(tag, name, wcsdup(value));
}

void FLAC_plugin__canonical_set_new(FLAC_Plugin__CanonicalTag *tag, const wchar_t *name, const wchar_t *value)
{
	FLAC__tag_entry *entry = FLAC_plugin__canonical_find(tag, name);
	if (!entry) FLAC_plugin__canonical_add_new(tag, name, value);
}

void FLAC_plugin__canonical_set_ansi(FLAC_Plugin__CanonicalTag *tag, const wchar_t *name, const char *value)
{
	wchar_t *val = FLAC_plugin__convert_ansi_to_wide(value);
	if (val) FLAC_plugin__canonical_set_nc(tag, name, val);
}

void FLAC_plugin__canonical_add(FLAC_Plugin__CanonicalTag *tag, const wchar_t *name, const wchar_t *value, const wchar_t *sep)
{
	FLAC__tag_entry *entry;

	if (sep && (entry = FLAC_plugin__canonical_find(tag, name)))
	{
		unsigned newlen = wcslen(entry->value) + wcslen(value) + wcslen(sep) + 1;
		wchar_t *newvalue = realloc(entry->value, newlen*sizeof(wchar_t));

		if (newvalue)
		{
			entry->value = newvalue;
			wcscat(entry->value, sep);
			wcscat(entry->value, value);
		}
	}
	else FLAC_plugin__canonical_add_new(tag, name, value);
}

void FLAC_plugin__canonical_add_utf8(FLAC_Plugin__CanonicalTag *tag, const char *name, const char *value, unsigned namelen, unsigned vallen, const char *sep)
{
	wchar_t *n = FLAC_plugin__convert_utf8_to_ucs2(name, namelen);
	wchar_t *v = FLAC_plugin__convert_utf8_to_ucs2(value, vallen);
	wchar_t *s = sep ? FLAC_plugin__convert_utf8_to_ucs2(sep, -1) : 0;

	if (n && v)
	{
		FLAC_plugin__canonical_add_nc(tag, n, v, s);
	}
	else
	{
		if (n) free(n);
		if (v) free(v);
	}
	if (s) free(s);
}

const wchar_t *FLAC_plugin__canonical_get(const FLAC_Plugin__CanonicalTag *tag, const wchar_t *name)
{
	FLAC__tag_entry *entry = FLAC_plugin__canonical_find(tag, name);
	return entry ? entry->value : 0;
}

FLAC__bool FLAC_plugin__canonical_remove(FLAC_Plugin__CanonicalTag *tag, const wchar_t *name)
{
	FLAC__tag_entry *entry = FLAC_plugin__canonical_find(tag, name);

	if (entry)
	{
		if (entry->prev)
			entry->prev->next = entry->next;
		else tag->head = entry->next;

		if (entry->next)
			entry->next->prev = entry->prev;
		else tag->tail = entry->prev;

		FLAC_plugin__canonical_tag_clear_entry(entry);
		tag->count--;
		return true;
	}

	return false;
}

void FLAC_plugin__canonical_remove_all(FLAC_Plugin__CanonicalTag *tag, const wchar_t *name)
{
	while (FLAC_plugin__canonical_remove(tag, name));
}

char *FLAC_plugin__canonical_get_formatted(FLAC__tag_iterator it)
{
	int len1 = wcslen(it->name);
	int len2 = wcslen(it->value);
	int len  = len1 + len2 + 1;
	wchar_t *val = malloc((len+1) * sizeof(wchar_t));

	if (val)
	{
		char *res;

		memcpy(val, it->name, len1 * sizeof(wchar_t));
		val[len1] = '=';
		memcpy(val+len1+1, it->value, len2 * sizeof(wchar_t));
		val[len] = 0;

		res = FLAC_plugin__convert_ucs2_to_utf8(val);
		free(val);
		return res;
	}

	return NULL;
}

/*
 * enumeration
 */
unsigned FLAC_plugin__canonical_get_count(FLAC_Plugin__CanonicalTag *tag)
{
	return tag->count;
}
FLAC__tag_iterator FLAC_plugin__canonical_first(FLAC_Plugin__CanonicalTag *tag)
{
	return tag->head;
}
FLAC__tag_iterator FLAC_plugin__canonical_next(FLAC__tag_iterator it)
{
	return it->next;
}
wchar_t *FLAC_plugin__canonical_get_name(FLAC__tag_iterator it)
{
	return it->name;
}
wchar_t *FLAC_plugin__canonical_get_value(FLAC__tag_iterator it)
{
	return it->value;
}

/*
 *  merging
 */

void FLAC_plugin__canonical_tag_merge(FLAC_Plugin__CanonicalTag *dest, const FLAC_Plugin__CanonicalTag *src)
{
	FLAC__tag_entry *entry = src->head;

	while (entry)
	{
		FLAC_plugin__canonical_set_new(dest, entry->name, entry->value);
		entry = entry->next;
	}
}

void FLAC_plugin__canonical_tag_get_combined(const char *filename, FLAC_Plugin__CanonicalTag *tag, const char *sep)
{
	FLAC_plugin__vorbiscomment_get(filename, tag, sep);
}
