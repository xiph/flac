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

#include "share/grabbag.h"
#include "FLAC/assert.h"
#include <stdlib.h> /* for atoi() */
#include <string.h>

FLAC__bool grabbag__seektable_convert_specification_to_template(const char *spec, FLAC__bool only_explicit_placeholders, FLAC__uint64 total_samples_to_encode, unsigned sample_rate, FLAC__StreamMetadata *seektable_template, FLAC__bool *spec_has_real_points)
{
	unsigned i;
	const char *pt;

	FLAC__ASSERT(0 != spec);
	FLAC__ASSERT(0 != seektable_template);
	FLAC__ASSERT(seektable_template->type = FLAC__METADATA_TYPE_SEEKTABLE);

	if(0 != spec_has_real_points)
		*spec_has_real_points = false;

	for(pt = spec, i = 0; pt && *pt; i++) {
		const char *q = strchr(pt, ';');
		FLAC__ASSERT(0 != q);

		if(q > pt) {
			if(0 == strncmp(pt, "X;", 2)) { /* -S X */
				if(!FLAC__metadata_object_seektable_template_append_placeholders(seektable_template, 1))
					return false;
			}
			else if(q[-1] == 'x') { /* -S #x */
				if(total_samples_to_encode > 0) { /* we can only do these if we know the number of samples to encode up front */
					if(0 != spec_has_real_points)
						*spec_has_real_points = true;
					if(!only_explicit_placeholders) {
						if(!FLAC__metadata_object_seektable_template_append_spaced_points(seektable_template, atoi(pt), total_samples_to_encode))
							return false;
					}
				}
			}
			else if(q[-1] == 's') { /* -S #s */
				if(total_samples_to_encode > 0) { /* we can only do these if we know the number of samples to encode up front */
					FLAC__ASSERT(sample_rate > 0);
					if(0 != spec_has_real_points)
						*spec_has_real_points = true;
					if(!only_explicit_placeholders) {
						double sec = atof(pt);
						if(sec > 0.0) {
#if defined _MSC_VER || defined __MINGW32__
							/* with MSVC you have to spoon feed it the casting */
							unsigned n = (unsigned)((double)(FLAC__int64)total_samples_to_encode / (sec * (double)sample_rate));
#else
							unsigned n = (unsigned)((double)total_samples_to_encode / (sec * (double)sample_rate));
#endif
							if(!FLAC__metadata_object_seektable_template_append_spaced_points(seektable_template, n, total_samples_to_encode))
								return false;
						}
					}
				}
			}
			else { /* -S # */
				if(0 != spec_has_real_points)
					*spec_has_real_points = true;
				if(!only_explicit_placeholders) {
					FLAC__uint64 n = (unsigned)atoi(pt);
					if(!FLAC__metadata_object_seektable_template_append_point(seektable_template, n))
						return false;
				}
			}
		}

		pt = ++q;
	}

	if(!FLAC__metadata_object_seektable_template_sort(seektable_template, /*compact=*/true))
		return false;

	return true;
}
