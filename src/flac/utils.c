/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2002,2003  Josh Coalson
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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "utils.h"
#include "FLAC/assert.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static FLAC__bool local__parse_uint64_(const char *s, FLAC__uint64 *value)
{
	FLAC__uint64 ret = 0;
	char c;

	if(*s == '\0')
		return false;

	while('\0' != (c = *s++))
		if(c >= '0' && c <= '9')
			ret = ret * 10 + (c - '0');
		else
			return false;

	*value = ret;
	return true;
}

static FLAC__bool local__parse_timecode_(const char *s, double *value)
{
	double ret;
	unsigned i;
	char c;

	/* parse [0-9][0-9]*: */
	c = *s++;
	if(c >= '0' && c <= '9')
		i = (c - '0');
	else
		return false;
	while(':' != (c = *s++)) {
		if(c >= '0' && c <= '9')
			i = i * 10 + (c - '0');
		else
			return false;
	}
	ret = (double)i * 60.;

	/* parse [0-9]*[.]?[0-9]* i.e. a sign-less rational number */
	if(strspn(s, "1234567890.") != strlen(s))
		return false;
	{
		const char *p = strchr(s, '.');
		if(p && 0 != strchr(++p, '.'))
			return false;
	}
	ret += atof(s);

	*value = ret;
	return true;
}

#ifdef FLAC__VALGRIND_TESTING
size_t flac__utils_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = fwrite(ptr, size, nmemb, stream);
	if(!ferror(stream))
		fflush(stream);
	return ret;
}
#endif

FLAC__bool flac__utils_parse_skip_until_specification(const char *s, utils__SkipUntilSpecification *spec)
{
	FLAC__uint64 val;
	FLAC__bool is_negative = false;

	FLAC__ASSERT(0 != spec);

	spec->is_relative = false;
	spec->value_is_samples = true;
	spec->value.samples = 0;

	if(0 != s) {
		if(s[0] == '-') {
			is_negative = true;
			spec->is_relative = true;
			s++;
		}
		else if(s[0] == '+') {
			spec->is_relative = true;
			s++;
		}

		if(local__parse_uint64_(s, &val)) {
			spec->value_is_samples = true;
			spec->value.samples = (FLAC__int64)val;
			if(is_negative)
				spec->value.samples = -(spec->value.samples);
		}
		else {
			double d;
			if(!local__parse_timecode_(s, &d))
				return false;
			spec->value_is_samples = false;
			spec->value.seconds = d;
			if(is_negative)
				spec->value.seconds = -(spec->value.seconds);
		}
	}

	return true;
}

void flac__utils_canonicalize_skip_until_specification(utils__SkipUntilSpecification *spec, unsigned sample_rate)
{
	FLAC__ASSERT(0 != spec);
	if(!spec->value_is_samples) {
		spec->value.samples = (FLAC__int64)(spec->value.seconds * (double)sample_rate);
		spec->value_is_samples = true;
	}
}
