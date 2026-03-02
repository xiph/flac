/* grabbag - Convenience lib for various routines common to several tools
 * Copyright (C) 2026  Xiph.Org Foundation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <stddef.h>
#include "share/grabbag.h"

FLAC__bool grabbag__escape_string_needed(const char *src, size_t src_size)
{
	for(size_t s = 0; s < src_size; ++s) {
		if((src[s] == '\\') ||
		   (src[s] == '\r') ||
		   (src[s] == '\n')) {
			return true;
		}
	}
	return false;
}

static size_t grabbag__escape_string_size(const char *src, size_t src_size)
{
	size_t dst_size = src_size;
	for(size_t s = 0; s < src_size; ++s) {
		if((src[s] == '\\') ||
		   (src[s] == '\r') ||
		   (src[s] == '\n')) {
			++dst_size;
		}
	}
	return dst_size;
}

static size_t grabbag__escape_string(char *dst, size_t dst_size, const char *src, size_t src_size, FLAC__bool *error)
{
	size_t d = 0;
	size_t s = 0;
	*error = false;

	while((d < dst_size) && (s < src_size)) {
		switch(src[s]) {
			case '\\':
				dst[d++] = '\\';
				dst[d++] = '\\';
				break;
			case '\r':
				dst[d++] = '\\';
				dst[d++] = 'r';
				break;
			case '\n':
				dst[d++] = '\\';
				dst[d++] = 'n';
				break;
			default:
				dst[d++] = src[s];
				break;
		}
		++s;
	}

	if((d > dst_size) || (s != src_size)) {
		/* dst ran out of space, or src was partially read */
		*error = true;
	}

	return d;
}

char *grabbag__create_escaped_string(const char *src, size_t src_size)
{
	const size_t dst_size = grabbag__escape_string_size(src, src_size);
	char *dst = malloc(dst_size + 1);
	if(dst != NULL) {
		FLAC__bool error = false;
		const size_t size = grabbag__escape_string(dst, dst_size, src, src_size, &error);
		if(error || (size > dst_size)) {
			free(dst);
			dst = NULL;
		}
		else {
			/* Add null terminator */
			dst[size] = '\0';
		}
	}
	return dst;
}

FLAC__bool grabbag__unescape_string_needed(const char *src, size_t src_size)
{
	for(size_t s = 0; s < src_size; ++s) {
		if(src[s] == '\\') {
			return true;
		}
	}
	return false;
}

static size_t grabbag__unescape_string_size(const char *src, size_t src_size)
{
	size_t dst_size = 0;
	size_t s = 0;
	while(s < src_size) {
		if(src[s] == '\\') {
			s += 2;
		}
		else {
			++s;
		}
		++dst_size;
	}
	return dst_size;
}

static size_t grabbag__unescape_string(char *dst, size_t dst_size, const char *src, size_t src_size, FLAC__bool *error)
{
	size_t d = 0;
	size_t s = 0;
	*error = false;

	while((d < dst_size) && (s < src_size)) {
		if(src[s] == '\\') {
			++s;
			if(s < src_size) {
				switch(src[s]) {
					case '\\':
						dst[d++] = '\\';
						break;
					case 'r':
						dst[d++] = '\r';
						break;
					case 'n':
						dst[d++] = '\n';
						break;
					default:
						/* Unsupported escape character */
						*error = true;
						break;
				}
				++s;
			}
			else {
				/* Truncated escape character */
				*error = true;
			}
		}
		else {
			dst[d++] = src[s++];
		}
	}

	if((d > dst_size) || (s != src_size)) {
		/* dst ran out of space, or src was partially read */
		*error = true;
	}

	return d;
}

char *grabbag__create_unescaped_string(const char *src, size_t src_size)
{
	const size_t dst_size = grabbag__unescape_string_size(src, src_size);
	char *dst = malloc(dst_size + 1);
	if(dst != NULL) {
		FLAC__bool error = false;
		const size_t size = grabbag__unescape_string(dst, dst_size, src, src_size, &error);
		if(error || (size > dst_size)) {
			free(dst);
			dst = NULL;
		}
		else {
			/* Add null terminator */
			dst[size] = '\0';
		}
	}
	return dst;
}
