/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2001,2002,2003  Josh Coalson
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

#include "private/bitmath.h"
#include "FLAC/assert.h"

/* An example of what FLAC__bitmath_ilog2() computes:
 *
 * ilog2( 0) = assertion failure
 * ilog2( 1) = 0
 * ilog2( 2) = 1
 * ilog2( 3) = 1
 * ilog2( 4) = 2
 * ilog2( 5) = 2
 * ilog2( 6) = 2
 * ilog2( 7) = 2
 * ilog2( 8) = 3
 * ilog2( 9) = 3
 * ilog2(10) = 3
 * ilog2(11) = 3
 * ilog2(12) = 3
 * ilog2(13) = 3
 * ilog2(14) = 3
 * ilog2(15) = 3
 * ilog2(16) = 4
 * ilog2(17) = 4
 * ilog2(18) = 4
 */
unsigned FLAC__bitmath_ilog2(unsigned v)
{
	unsigned l = 0;
	FLAC__ASSERT(v > 0);
	while(v >>= 1)
		l++;
	return l;
}

/* An example of what FLAC__bitmath_silog2() computes:
 *
 * silog2(-10) = 5
 * silog2(- 9) = 5
 * silog2(- 8) = 4
 * silog2(- 7) = 4
 * silog2(- 6) = 4
 * silog2(- 5) = 4
 * silog2(- 4) = 3
 * silog2(- 3) = 3
 * silog2(- 2) = 2
 * silog2(- 1) = 2
 * silog2(  0) = 0
 * silog2(  1) = 2
 * silog2(  2) = 3
 * silog2(  3) = 3
 * silog2(  4) = 4
 * silog2(  5) = 4
 * silog2(  6) = 4
 * silog2(  7) = 4
 * silog2(  8) = 5
 * silog2(  9) = 5
 * silog2( 10) = 5
 */
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
			v++;
			v = -v;
		}
	}
}

unsigned FLAC__bitmath_silog2_wide(FLAC__int64 v)
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
			v++;
			v = -v;
		}
	}
}
