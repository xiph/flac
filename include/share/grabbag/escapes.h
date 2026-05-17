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

/* This .h cannot be included by itself; #include "share/grabbag.h" instead. */

#ifndef GRABBAG__ESCAPES_H
#define GRABBAG__ESCAPES_H

#include <stdio.h>
#include <stddef.h>

#include "FLAC/ordinals.h"

#ifdef __cplusplus
extern "C" {
#endif

FLAC__bool grabbag__escape_string_needed(const char *src, size_t src_size);
char *grabbag__create_escaped_string(const char *src, size_t src_size);

FLAC__bool grabbag__unescape_string_needed(const char *src, size_t src_size);
char *grabbag__create_unescaped_string(const char *src, size_t src_size);

#ifdef __cplusplus
}
#endif

#endif
