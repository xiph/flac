/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2001  Josh Coalson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#include <assert.h>
#include "private/bitmath.h"

unsigned FLAC__bitmath_ilog2(unsigned v)
{
	unsigned l = 0;
	assert(v > 0);
	while(v >>= 1)
		l++;
	return l;
}

unsigned FLAC__bitmath_silog2(int v)
{
	while(1) {
		if(v == 0) {
			return 0;
		}
		else if(v > 0) {
			unsigned l = 0;
			while(v) {
				l++;
				v >>= 1;
			}
			return l+1;
		}
		else if(v == -1) {
			return 2;
		}
		else {
			v = -(++v);
		}
	}
}
