/* test_streams - Simple test pattern generator
 * Copyright (C) 2000  Josh Coalson
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "FLAC/ordinals.h"

#ifdef _WIN32
	static const char *mode = "wb";
#else
	static const char *mode = "w";
#endif

static bool is_big_endian_host;

static void swap16(int16 *i)
{
	unsigned char *x = (unsigned char *)i, b;
	if(!is_big_endian_host) {
		b = x[0];
		x[0] = x[1];
		x[1] = b;
	}
}

/* a mono one-sample 16bps stream */
static bool generate_01()
{
	FILE *f;
	int16 x = -32768;

	if(0 == (f = fopen("test01.raw", mode)))
		return false;

	swap16(&x);
	if(fwrite(&x, sizeof(x), 1, f) < 1)
		goto foo;

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a stereo one-sample 16bps stream */
static bool generate_02()
{
	FILE *f;
	int16 xl = -32768, xr = 32767;

	if(0 == (f = fopen("test02.raw", mode)))
		return false;

	swap16(&xl);
	swap16(&xr);

	if(fwrite(&xl, sizeof(xl), 1, f) < 1)
		goto foo;
	if(fwrite(&xr, sizeof(xr), 1, f) < 1)
		goto foo;

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a mono five-sample 16bps stream */
static bool generate_03()
{
	FILE *f;
	int16 x[] = { -25, 0, 25, 50, 100 };
	unsigned i;

	if(0 == (f = fopen("test03.raw", mode)))
		return false;

	for(i = 0; i < 5; i++)
		swap16(x+i);

	if(fwrite(&x, sizeof(int16), 5, f) < 5)
		goto foo;

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a stereo five-sample 16bps stream */
static bool generate_04()
{
	FILE *f;
	int16 x[] = { -25, 500, 0, 400, 25, 300, 50, 200, 100, 100 };
	unsigned i;

	if(0 == (f = fopen("test04.raw", mode)))
		return false;

	for(i = 0; i < 10; i++)
		swap16(x+i);

	if(fwrite(&x, sizeof(int16), 10, f) < 10)
		goto foo;

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a mono full-scale deflection 8bps stream */
static bool generate_fsd8(const char *fn, const int pattern[], unsigned reps)
{
	FILE *f;
	unsigned rep, p;

	assert(pattern != 0);

	if(0 == (f = fopen(fn, mode)))
		return false;

	for(rep = 0; rep < reps; rep++) {
		for(p = 0; pattern[p]; p++) {
			signed char x = pattern[p] > 0? 127 : -128;
			if(fwrite(&x, sizeof(x), 1, f) < 1)
				goto foo;
		}
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a mono full-scale deflection 16bps stream */
static bool generate_fsd16(const char *fn, const int pattern[], unsigned reps)
{
	FILE *f;
	unsigned rep, p;

	assert(pattern != 0);

	if(0 == (f = fopen(fn, mode)))
		return false;

	for(rep = 0; rep < reps; rep++) {
		for(p = 0; pattern[p]; p++) {
			int16 x = pattern[p] > 0? 32767 : -32768;
			swap16(&x);
			if(fwrite(&x, sizeof(x), 1, f) < 1)
				goto foo;
		}
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a mono sine-wave 16bps stream */
static bool generate_sine16(const char *fn, const double sample_rate, const unsigned samples, const double f1, const double a1, const double f2, const double a2)
{
	const signed short full_scale = 32767;
	const double delta1 = 2.0 * M_PI / ( sample_rate / f1);
	const double delta2 = 2.0 * M_PI / ( sample_rate / f2);
	FILE *f;
	double theta1, theta2;
	unsigned i;

	if(0 == (f = fopen(fn, mode)))
		return false;

	for(i = 0, theta1 = theta2 = 0.0; i < samples; i++, theta1 += delta1, theta2 += delta2) {
		double val = (a1*sin(theta1) + a2*sin(theta2))*(double)full_scale;
		int16 v = (int16)(val + 0.5);
		swap16(&v);
		if(fwrite(&v, sizeof(v), 1, f) < 1)
			goto foo;
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

int main(int argc, char *argv[])
{
	uint32 test = 1;

	int pattern01[] = { 1, -1, 0 };
	int pattern02[] = { 1, 1, -1, 0 };
	int pattern03[] = { 1, -1, -1, 0 };
	int pattern04[] = { 1, -1, 1, -1, 0 };
	int pattern05[] = { 1, -1, -1, 1, 0 };
	int pattern06[] = { 1, -1, 1, 1, -1, 0 };
	int pattern07[] = { 1, -1, -1, 1, -1, 0 };

	(void)argc;
	(void)argv;
	is_big_endian_host = (*((byte*)(&test)))? false : true;

	if(!generate_01()) return 1;
	if(!generate_02()) return 1;
	if(!generate_03()) return 1;
	if(!generate_04()) return 1;

	if(!generate_fsd8("fsd8-01.raw", pattern01, 100)) return 1;
	if(!generate_fsd8("fsd8-02.raw", pattern02, 100)) return 1;
	if(!generate_fsd8("fsd8-03.raw", pattern03, 100)) return 1;
	if(!generate_fsd8("fsd8-04.raw", pattern04, 100)) return 1;
	if(!generate_fsd8("fsd8-05.raw", pattern05, 100)) return 1;
	if(!generate_fsd8("fsd8-06.raw", pattern06, 100)) return 1;
	if(!generate_fsd8("fsd8-07.raw", pattern07, 100)) return 1;

	if(!generate_fsd16("fsd16-01.raw", pattern01, 100)) return 1;
	if(!generate_fsd16("fsd16-02.raw", pattern02, 100)) return 1;
	if(!generate_fsd16("fsd16-03.raw", pattern03, 100)) return 1;
	if(!generate_fsd16("fsd16-04.raw", pattern04, 100)) return 1;
	if(!generate_fsd16("fsd16-05.raw", pattern05, 100)) return 1;
	if(!generate_fsd16("fsd16-06.raw", pattern06, 100)) return 1;
	if(!generate_fsd16("fsd16-07.raw", pattern07, 100)) return 1;

	if(!generate_sine16("sine-01.raw", 44100.0, 80000, 441.0, 0.50, 441.0, 0.49)) return 1;
	if(!generate_sine16("sine-02.raw", 44100.0, 80000, 441.0, 0.61, 661.5, 0.37)) return 1;
	if(!generate_sine16("sine-03.raw", 44100.0, 80000, 441.0, 0.50, 882.0, 0.49)) return 1;
	if(!generate_sine16("sine-04.raw", 44100.0, 80000, 441.0, 0.50, 4410.0, 0.49)) return 1;
	if(!generate_sine16("sine-05.raw", 44100.0, 50000, 8820.0, 0.70, 4410.0, 0.29)) return 1;

	return 0;
}
