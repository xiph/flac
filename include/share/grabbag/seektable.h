/* grabbag - Convenience lib for various routines common to several tools
 * Copyright (C) 2002,2003,2004,2005  Josh Coalson
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

/* Convenience routines for working with seek tables */

/* This .h cannot be included by itself; #include "share/grabbag.h" instead. */

#ifndef GRABAG__SEEKTABLE_H
#define GRABAG__SEEKTABLE_H

#include "FLAC/format.h"

#ifdef __cplusplus
extern "C" {
#endif

FLAC__bool grabbag__seektable_convert_specification_to_template(const char *spec, FLAC__bool only_explicit_placeholders, FLAC__uint64 total_samples_to_encode, unsigned sample_rate, FLAC__StreamMetadata *seektable_template, FLAC__bool *spec_has_real_points);

#ifdef __cplusplus
}
#endif

#endif
