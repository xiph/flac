/* test_streams - Simple test pattern generator
 * Copyright (C) 2000,2001  Josh Coalson
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
#include <sys/time.h>
#include "FLAC/assert.h"
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

static void swap24(byte *x)
{
	if(is_big_endian_host) {
		x[0] = x[1];
		x[1] = x[2];
		x[2] = x[3];
	}
	else {
		byte b = x[0];
		x[0] = x[2];
		x[2] = b;
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

	FLAC__ASSERT(pattern != 0);

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

	FLAC__ASSERT(pattern != 0);

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

/* a stereo wasted-bits-per-sample 16bps stream */
static bool generate_wbps16(const char *fn, unsigned samples)
{
	FILE *f;
	unsigned sample;

	if(0 == (f = fopen(fn, mode)))
		return false;

	for(sample = 0; sample < samples; sample++) {
		int16 l = (sample % 2000) << 2;
		int16 r = (sample % 1000) << 3;
		swap16(&l);
		swap16(&r);
		if(fwrite(&l, sizeof(l), 1, f) < 1)
			goto foo;
		if(fwrite(&r, sizeof(r), 1, f) < 1)
			goto foo;
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a mono full-scale deflection 24bps stream */
static bool generate_fsd24(const char *fn, const int pattern[], unsigned reps)
{
	FILE *f;
	unsigned rep, p;

	FLAC__ASSERT(pattern != 0);

	if(0 == (f = fopen(fn, mode)))
		return false;

	for(rep = 0; rep < reps; rep++) {
		for(p = 0; pattern[p]; p++) {
			int32 x = pattern[p] > 0? 8388607 : -8388608;
			byte *b = (byte*)(&x);
			swap24(b);
			if(fwrite(b, 3, 1, f) < 1)
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
static bool generate_sine16_1(const char *fn, const double sample_rate, const unsigned samples, const double f1, const double a1, const double f2, const double a2)
{
	const int16 full_scale = 32767;
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

/* a stereo sine-wave 16bps stream */
static bool generate_sine16_2(const char *fn, const double sample_rate, const unsigned samples, const double f1, const double a1, const double f2, const double a2, double fmult)
{
	const int16 full_scale = 32767;
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
		val = -(a1*sin(theta1*fmult) + a2*sin(theta2*fmult))*(double)full_scale;
		v = (int16)(val + 0.5);
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

/* a mono sine-wave 24bps stream */
static bool generate_sine24_1(const char *fn, const double sample_rate, const unsigned samples, const double f1, const double a1, const double f2, const double a2)
{
	const int32 full_scale = 0x7fffff;
	const double delta1 = 2.0 * M_PI / ( sample_rate / f1);
	const double delta2 = 2.0 * M_PI / ( sample_rate / f2);
	FILE *f;
	double theta1, theta2;
	unsigned i;

	if(0 == (f = fopen(fn, mode)))
		return false;

	for(i = 0, theta1 = theta2 = 0.0; i < samples; i++, theta1 += delta1, theta2 += delta2) {
		double val = (a1*sin(theta1) + a2*sin(theta2))*(double)full_scale;
		int32 v = (int32)(val + 0.5);
		byte *b = (byte*)(&v);
		swap24(b);
		if(fwrite(b, 3, 1, f) < 1)
			goto foo;
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a stereo sine-wave 24bps stream */
static bool generate_sine24_2(const char *fn, const double sample_rate, const unsigned samples, const double f1, const double a1, const double f2, const double a2, double fmult)
{
	const int32 full_scale = 0x7fffff;
	const double delta1 = 2.0 * M_PI / ( sample_rate / f1);
	const double delta2 = 2.0 * M_PI / ( sample_rate / f2);
	FILE *f;
	double theta1, theta2;
	unsigned i;

	if(0 == (f = fopen(fn, mode)))
		return false;

	for(i = 0, theta1 = theta2 = 0.0; i < samples; i++, theta1 += delta1, theta2 += delta2) {
		double val = (a1*sin(theta1) + a2*sin(theta2))*(double)full_scale;
		int32 v = (int32)(val + 0.5);
		byte *b = (byte*)(&v);
		swap24(b);
		if(fwrite(b, 3, 1, f) < 1)
			goto foo;
		val = -(a1*sin(theta1*fmult) + a2*sin(theta2*fmult))*(double)full_scale;
		v = (int32)(val + 0.5);
		swap24(b);
		if(fwrite(b, 3, 1, f) < 1)
			goto foo;
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

static bool generate_noise(const char *fn, unsigned bytes)
{
	FILE *f;
	struct timeval tv;
	unsigned b;

	if(gettimeofday(&tv, 0) < 0) {
		fprintf(stderr, "WARNING: couldn't seed RNG with time\n");
		tv.tv_usec = 4321;
	}
	srandom(tv.tv_usec);

	if(0 == (f = fopen(fn, mode)))
		return false;

	for(b = 0; b < bytes; b++) {
		byte x = (byte)(((unsigned)random()) & 0xff);
		if(fwrite(&x, sizeof(x), 1, f) < 1)
			goto foo;
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

static bool generate_wackywavs()
{
	FILE *f;
	byte wav[] = {
		'R', 'I', 'F', 'F',  76,   0,   0,   0,
		'W', 'A', 'V', 'E', 'f', 'a', 'c', 't',
		  4,   0,   0,  0 , 'b', 'l', 'a', 'h',
		'p', 'a', 'd', ' ',   4,   0,   0,   0,
		'B', 'L', 'A', 'H', 'f', 'm', 't', ' ',
		 16,   0,   0,   0,   1,   0,   1,   0,
		0x44,0xAC,  0,   0,   0,   0,   0,   0,
		  2,   0,  16,   0, 'd', 'a', 't', 'a',
		 16,   0,   0,   0,   0,   0,   1,   0,
		  4,   0,   9,   0,  16,   0,  25,   0,
		 36,   0,  49,   0, 'p', 'a', 'd', ' ',
		  4,   0,   0,   0, 'b', 'l', 'a', 'h'
	};

	if(0 == (f = fopen("wacky1.wav", mode)))
		return false;
	if(fwrite(wav, 1, 84, f) < 84)
		goto foo;
	fclose(f);

	wav[4] += 12;
	if(0 == (f = fopen("wacky2.wav", mode)))
		return false;
	if(fwrite(wav, 1, 96, f) < 96)
		goto foo;
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

	if(!generate_fsd24("fsd24-01.raw", pattern01, 100)) return 1;
	if(!generate_fsd24("fsd24-02.raw", pattern02, 100)) return 1;
	if(!generate_fsd24("fsd24-03.raw", pattern03, 100)) return 1;
	if(!generate_fsd24("fsd24-04.raw", pattern04, 100)) return 1;
	if(!generate_fsd24("fsd24-05.raw", pattern05, 100)) return 1;
	if(!generate_fsd24("fsd24-06.raw", pattern06, 100)) return 1;
	if(!generate_fsd24("fsd24-07.raw", pattern07, 100)) return 1;

	if(!generate_wbps16("wbps16-01.raw", 1000)) return 1;

	if(!generate_sine16_1("sine16-00.raw", 44100.0, 80000, 441.0, 0.50, 441.0, 0.49)) return 1;
	if(!generate_sine16_1("sine16-01.raw", 44100.0, 80000, 441.0, 0.61, 661.5, 0.37)) return 1;
	if(!generate_sine16_1("sine16-02.raw", 44100.0, 80000, 441.0, 0.50, 882.0, 0.49)) return 1;
	if(!generate_sine16_1("sine16-03.raw", 44100.0, 80000, 441.0, 0.50, 4410.0, 0.49)) return 1;
	if(!generate_sine16_1("sine16-04.raw", 44100.0, 50000, 8820.0, 0.70, 4410.0, 0.29)) return 1;

	if(!generate_sine16_2("sine16-10.raw", 44100.0, 80000, 441.0, 0.50, 441.0, 0.49, 1.0)) return 1;
	if(!generate_sine16_2("sine16-11.raw", 44100.0, 80000, 441.0, 0.61, 661.5, 0.37, 1.0)) return 1;
	if(!generate_sine16_2("sine16-12.raw", 44100.0, 80000, 441.0, 0.50, 882.0, 0.49, 1.0)) return 1;
	if(!generate_sine16_2("sine16-13.raw", 44100.0, 80000, 441.0, 0.50, 4410.0, 0.49, 1.0)) return 1;
	if(!generate_sine16_2("sine16-14.raw", 44100.0, 50000, 8820.0, 0.70, 4410.0, 0.29, 1.0)) return 1;
	if(!generate_sine16_2("sine16-15.raw", 44100.0, 80000, 441.0, 0.50, 441.0, 0.49, 0.5)) return 1;
	if(!generate_sine16_2("sine16-16.raw", 44100.0, 80000, 441.0, 0.61, 661.5, 0.37, 2.0)) return 1;
	if(!generate_sine16_2("sine16-17.raw", 44100.0, 80000, 441.0, 0.50, 882.0, 0.49, 0.7)) return 1;
	if(!generate_sine16_2("sine16-18.raw", 44100.0, 80000, 441.0, 0.50, 4410.0, 0.49, 1.3)) return 1;
	if(!generate_sine16_2("sine16-19.raw", 44100.0, 50000, 8820.0, 0.70, 4410.0, 0.29, 0.1)) return 1;

	if(!generate_sine24_1("sine24-00.raw", 44100.0, 80000, 441.0, 0.50, 441.0, 0.49)) return 1;
	if(!generate_sine24_1("sine24-01.raw", 44100.0, 80000, 441.0, 0.61, 661.5, 0.37)) return 1;
	if(!generate_sine24_1("sine24-02.raw", 44100.0, 80000, 441.0, 0.50, 882.0, 0.49)) return 1;
	if(!generate_sine24_1("sine24-03.raw", 44100.0, 80000, 441.0, 0.50, 4410.0, 0.49)) return 1;
	if(!generate_sine24_1("sine24-04.raw", 44100.0, 50000, 8820.0, 0.70, 4410.0, 0.29)) return 1;

	if(!generate_sine24_2("sine24-10.raw", 44100.0, 80000, 441.0, 0.50, 441.0, 0.49, 1.0)) return 1;
	if(!generate_sine24_2("sine24-11.raw", 44100.0, 80000, 441.0, 0.61, 661.5, 0.37, 1.0)) return 1;
	if(!generate_sine24_2("sine24-12.raw", 44100.0, 80000, 441.0, 0.50, 882.0, 0.49, 1.0)) return 1;
	if(!generate_sine24_2("sine24-13.raw", 44100.0, 80000, 441.0, 0.50, 4410.0, 0.49, 1.0)) return 1;
	if(!generate_sine24_2("sine24-14.raw", 44100.0, 50000, 8820.0, 0.70, 4410.0, 0.29, 1.0)) return 1;
	if(!generate_sine24_2("sine24-15.raw", 44100.0, 80000, 441.0, 0.50, 441.0, 0.49, 0.5)) return 1;
	if(!generate_sine24_2("sine24-16.raw", 44100.0, 80000, 441.0, 0.61, 661.5, 0.37, 2.0)) return 1;
	if(!generate_sine24_2("sine24-17.raw", 44100.0, 80000, 441.0, 0.50, 882.0, 0.49, 0.7)) return 1;
	if(!generate_sine24_2("sine24-18.raw", 44100.0, 80000, 441.0, 0.50, 4410.0, 0.49, 1.3)) return 1;
	if(!generate_sine24_2("sine24-19.raw", 44100.0, 50000, 8820.0, 0.70, 4410.0, 0.29, 0.1)) return 1;

	if(!generate_noise("noise.raw", 65536 * 8 * 3)) return 1;
	if(!generate_wackywavs()) return 1;

	return 0;
}
