/* flac - Command-line FLAC encoder/decoder
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

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FLAC/all.h"
#include "decode.h"
#include "encode.h"

static int usage(const char *message, ...);

int main(int argc, char *argv[])
{
	int i;
	bool verify = false, verbose = true, lax = false, mode_decode = false, test_only = false, analyze = false;
	bool do_mid_side = true, loose_mid_side = false, do_exhaustive_model_search = false, do_qlp_coeff_prec_search = false;
	unsigned padding = 0;
	unsigned max_lpc_order = 8;
	unsigned qlp_coeff_precision = 0;
	uint64 skip = 0;
	int format_is_wave = -1, format_is_big_endian = -1, format_is_unsigned_samples = false;
	int format_channels = -1, format_bps = -1, format_sample_rate = -1;
	int blocksize = -1, rice_optimization_level = -1;

	if(argc <= 1)
		return usage(0);

	/* get the options */
	for(i = 1; i < argc; i++) {
		if(argv[i][0] != '-' || argv[i][1] == 0)
			break;
		if(0 == strcmp(argv[i], "-d"))
			mode_decode = true;
		else if(0 == strcmp(argv[i], "-a")) {
			mode_decode = true;
			analyze = true;
		}
		else if(0 == strcmp(argv[i], "-t")) {
			mode_decode = true;
			test_only = true;
		}
		else if(0 == strcmp(argv[i], "-s"))
			verbose = false;
		else if(0 == strcmp(argv[i], "-s-"))
			verbose = true;
		else if(0 == strcmp(argv[i], "--skip"))
			skip = (uint64)atoi(argv[++i]); /* @@@ takes a pretty damn big file to overflow atoi() here, but it could happen */
		else if(0 == strcmp(argv[i], "--lax"))
			lax = true;
		else if(0 == strcmp(argv[i], "--lax-"))
			lax = false;
		else if(0 == strcmp(argv[i], "-b"))
			blocksize = atoi(argv[++i]);
		else if(0 == strcmp(argv[i], "-e"))
			do_exhaustive_model_search = true;
		else if(0 == strcmp(argv[i], "-e-"))
			do_exhaustive_model_search = false;
		else if(0 == strcmp(argv[i], "-l"))
			max_lpc_order = atoi(argv[++i]);
		else if(0 == strcmp(argv[i], "-m"))
			do_mid_side = true;
		else if(0 == strcmp(argv[i], "-m-"))
			do_mid_side = false;
		else if(0 == strcmp(argv[i], "-M"))
			loose_mid_side = do_mid_side = true;
		else if(0 == strcmp(argv[i], "-M-"))
			loose_mid_side = do_mid_side = false;
		else if(0 == strcmp(argv[i], "-p"))
			do_qlp_coeff_prec_search = true;
		else if(0 == strcmp(argv[i], "-p-"))
			do_qlp_coeff_prec_search = false;
		else if(0 == strcmp(argv[i], "-P"))
			padding = atoi(argv[++i]);
		else if(0 == strcmp(argv[i], "-q"))
			qlp_coeff_precision = atoi(argv[++i]);
		else if(0 == strcmp(argv[i], "-r"))
			rice_optimization_level = atoi(argv[++i]);
		else if(0 == strcmp(argv[i], "-V"))
			verify = true;
		else if(0 == strcmp(argv[i], "-V-"))
			verify = false;
		else if(0 == strcmp(argv[i], "-fb"))
			format_is_big_endian = true;
		else if(0 == strcmp(argv[i], "-fl"))
			format_is_big_endian = false;
		else if(0 == strcmp(argv[i], "-fc"))
			format_channels = atoi(argv[++i]);
		else if(0 == strcmp(argv[i], "-fp"))
			format_bps = atoi(argv[++i]);
		else if(0 == strcmp(argv[i], "-fs"))
			format_sample_rate = atoi(argv[++i]);
		else if(0 == strcmp(argv[i], "-fu"))
			format_is_unsigned_samples = true;
		else if(0 == strcmp(argv[i], "-fr"))
			format_is_wave = false;
		else if(0 == strcmp(argv[i], "-fw"))
			format_is_wave = true;
		else if(0 == strcmp(argv[i], "-0")) {
			do_exhaustive_model_search = false;
			do_mid_side = false;
			loose_mid_side = false;
			qlp_coeff_precision = 0;
			rice_optimization_level = 0;
			max_lpc_order = 0;
		}
		else if(0 == strcmp(argv[i], "-1")) {
			do_exhaustive_model_search = false;
			do_mid_side = true;
			loose_mid_side = true;
			qlp_coeff_precision = 0;
			rice_optimization_level = 0;
			max_lpc_order = 0;
		}
		else if(0 == strcmp(argv[i], "-2")) {
			do_exhaustive_model_search = false;
			do_mid_side = true;
			loose_mid_side = false;
			qlp_coeff_precision = 0;
			max_lpc_order = 0;
		}
		else if(0 == strcmp(argv[i], "-4")) {
			do_exhaustive_model_search = false;
			do_mid_side = false;
			loose_mid_side = false;
			qlp_coeff_precision = 0;
			rice_optimization_level = 0;
			max_lpc_order = 8;
		}
		else if(0 == strcmp(argv[i], "-5")) {
			do_exhaustive_model_search = false;
			do_mid_side = true;
			loose_mid_side = true;
			qlp_coeff_precision = 0;
			rice_optimization_level = 0;
			max_lpc_order = 8;
		}
		else if(0 == strcmp(argv[i], "-6")) {
			do_exhaustive_model_search = false;
			do_mid_side = true;
			loose_mid_side = false;
			qlp_coeff_precision = 0;
			max_lpc_order = 8;
		}
		else if(0 == strcmp(argv[i], "-8")) {
			do_exhaustive_model_search = false;
			do_mid_side = true;
			loose_mid_side = false;
			qlp_coeff_precision = 0;
			max_lpc_order = 32;
		}
		else if(0 == strcmp(argv[i], "-9")) {
			do_exhaustive_model_search = true;
			do_mid_side = true;
			loose_mid_side = false;
			do_qlp_coeff_prec_search = true;
			rice_optimization_level = 99;
			max_lpc_order = 32;
		}
		else if(isdigit((int)(argv[i][1]))) {
			return usage("ERROR: compression level '%s' is still reserved\n", argv[i]);
		}
		else {
			return usage("ERROR: invalid option '%s'\n", argv[i]);
		}
	}
	if(i + (test_only? 1:2) != argc)
		return usage("ERROR: invalid arguments (more/less than %d filename%s?)\n", (test_only? 1:2), (test_only? "":"s"));

	/* tweak options based on the filenames; validate the values */
	if(!mode_decode) {
		if(format_is_wave < 0) {
			if(strstr(argv[i], ".wav") == argv[i] + (strlen(argv[i]) - strlen(".wav")))
				format_is_wave = true;
			else
				format_is_wave = false;
		}
		if(!format_is_wave) {
			if(format_is_big_endian < 0 || format_channels < 0 || format_bps < 0 || format_sample_rate < 0)
				return usage("ERROR: for encoding a raw file you must specify { -fb or -fl }, -fc, -fp, and -fs\n");
		}
		if(blocksize < 0) {
			if(max_lpc_order == 0)
				blocksize = 1152;
			else
				blocksize = 4608;
		}
		if(rice_optimization_level < 0) {
			if(blocksize <= 1152)
				rice_optimization_level = 4;
			else if(blocksize <= 2304)
				rice_optimization_level = 4;
			else if(blocksize <= 4608)
				rice_optimization_level = 4;
			else
				rice_optimization_level = 5;
		}
	}
	else {
		if(test_only) {
			if(skip > 0)
				return usage("ERROR: --skip is not allowed in test mode\n");
		}
		else if(!analyze) {
			if(format_is_wave < 0) {
				if(strstr(argv[i+1], ".wav") == argv[i+1] + (strlen(argv[i+1]) - strlen(".wav")))
					format_is_wave = true;
				else
					format_is_wave = false;
			}
			if(!format_is_wave) {
				if(format_is_big_endian < 0)
					return usage("ERROR: for decoding to a raw file you must specify -fb or -fl\n");
			}
		}
	}

	assert(blocksize >= 0 || mode_decode);

	if(format_channels >= 0) {
		if(format_channels == 0 || (unsigned)format_channels > FLAC__MAX_CHANNELS)
			return usage("ERROR: invalid number of channels '%u', must be > 0 and <= %u\n", format_channels, FLAC__MAX_CHANNELS);
	}
	if(format_bps >= 0) {
		if(format_bps != 8 && format_bps != 16)
			return usage("ERROR: invalid bits per sample '%u' (must be 8 or 16)\n", format_bps);
	}
	if(format_sample_rate >= 0) {
		if(format_sample_rate == 0 || (unsigned)format_sample_rate > FLAC__MAX_SAMPLE_RATE)
			return usage("ERROR: invalid sample rate '%u', must be > 0 and <= %u\n", format_sample_rate, FLAC__MAX_SAMPLE_RATE);
	}
	if(!mode_decode && ((unsigned)blocksize < FLAC__MIN_BLOCK_SIZE || (unsigned)blocksize > FLAC__MAX_BLOCK_SIZE)) {
		return usage("ERROR: invalid blocksize '%u', must be >= %u and <= %u\n", (unsigned)blocksize, FLAC__MIN_BLOCK_SIZE, FLAC__MAX_BLOCK_SIZE);
	}
	if(qlp_coeff_precision > 0 && qlp_coeff_precision < FLAC__MIN_QLP_COEFF_PRECISION) {
		return usage("ERROR: invalid value for -q '%u', must be 0 or >= %u\n", qlp_coeff_precision, FLAC__MIN_QLP_COEFF_PRECISION);
	}

	/* turn off verbosity if the output stream is going to stdout */
	if(!test_only && 0 == strcmp(argv[i+1], "-"))
		verbose = false;

	if(verbose) {
		printf("\n");
		printf("flac %s, Copyright (C) 2000,2001 Josh Coalson\n", FLAC__VERSION_STRING);
		printf("flac comes with ABSOLUTELY NO WARRANTY.  This is free software, and you are\n");
		printf("welcome to redistribute it under certain conditions.  Type `flac' for details.\n\n");

		if(!mode_decode) {
			printf("options:%s -P %u -b %u%s -l %u%s%s -q %u -r %u%s\n",
				lax?" --lax":"", padding, (unsigned)blocksize, loose_mid_side?" -M":do_mid_side?" -m":"", max_lpc_order,
				do_exhaustive_model_search?" -e":"", do_qlp_coeff_prec_search?" -p":"",
				qlp_coeff_precision, (unsigned)rice_optimization_level,
				verify? " -V":""
			);
		}
	}

	if(mode_decode)
		if(format_is_wave)
			return decode_wav(argv[i], test_only? 0 : argv[i+1], analyze, verbose, skip);
		else
			return decode_raw(argv[i], test_only? 0 : argv[i+1], analyze, verbose, skip, format_is_big_endian, format_is_unsigned_samples);
	else
		if(format_is_wave)
			return encode_wav(argv[i], argv[i+1], verbose, skip, verify, lax, do_mid_side, loose_mid_side, do_exhaustive_model_search, do_qlp_coeff_prec_search, rice_optimization_level, max_lpc_order, (unsigned)blocksize, qlp_coeff_precision, padding);
		else
			return encode_raw(argv[i], argv[i+1], verbose, skip, verify, lax, do_mid_side, loose_mid_side, do_exhaustive_model_search, do_qlp_coeff_prec_search, rice_optimization_level, max_lpc_order, (unsigned)blocksize, qlp_coeff_precision, padding, format_is_big_endian, format_is_unsigned_samples, format_channels, format_bps, format_sample_rate);

	return 0;
}

int usage(const char *message, ...)
{
	va_list args;

	if(message) {
		va_start(args, message);

		(void) vfprintf(stderr, message, args);

		va_end(args);

	}
	printf("==============================================================================\n");
	printf("flac - Command-line FLAC encoder/decoder version %s\n", FLAC__VERSION_STRING);
	printf("Copyright (C) 2000,2001  Josh Coalson\n");
	printf("\n");
	printf("This program is free software; you can redistribute it and/or\n");
	printf("modify it under the terms of the GNU General Public License\n");
	printf("as published by the Free Software Foundation; either version 2\n");
	printf("of the License, or (at your option) any later version.\n");
	printf("\n");
	printf("This program is distributed in the hope that it will be useful,\n");
	printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
	printf("GNU General Public License for more details.\n");
	printf("\n");
	printf("You should have received a copy of the GNU General Public License\n");
	printf("along with this program; if not, write to the Free Software\n");
	printf("Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.\n");
	printf("==============================================================================\n");
	printf("Usage:\n");
	printf("  flac [options] infile outfile\n");
	printf("\n");
	printf("For encoding:\n");
	printf("  infile may be a PCM RIFF WAVE file or raw samples\n");
	printf("  outfile will be in FLAC format\n");
	printf("For decoding, the reverse is be true\n");
	printf("\n");
	printf("infile may be - for stdin, outfile may be - for stdout\n");
	printf("\n");
	printf("If the unencoded filename ends with '.wav' or -fw is used, it's assumed to be\n");
	printf("RIFF WAVE.  Otherwise, it's assumed to be raw samples and you have to specify\n");
	printf("all the format options.  You can force a .wav file to be treated as a raw file\n");
	printf("using -fr.\n");
	printf("\n");
	printf("generic options:\n");
	printf("  -d : decode (default behavior is encode)\n");
	printf("  -t : test (same as -d except no decoded file is written)\n");
	printf("  -a : analyze (same as -d except an analysis file is written)\n");
	printf("  -s : silent (do not write runtime encode/decode statistics to stdout)\n");
	printf("  --skip samples : can be used both for encoding and decoding\n");
	printf("encoding options:\n");
	printf("  --lax : allow encoder to generate non-Subset files\n");
	printf("  -P bytes : write a PADDING block of the given length (0 => no PADDING block, default is -P 0)\n");
	printf("  -b blocksize : default is 1152 for -l 0, else 4608; should be 192/576/1152/2304/4608 (unless --lax is used)\n");
	printf("  -m : try mid-side coding for each frame (stereo input only)\n");
	printf("  -M : loose mid-side coding for all frames (stereo input only)\n");
	printf("  -0 .. -9 : fastest compression .. highest compression, default is -6\n");
	printf("             these are synonyms for other options:\n");
	printf("  -0 : synonymous with -l 0\n");
	printf("  -1 : synonymous with -l 0 -M\n");
	printf("  -2 : synonymous with -l 0 -m -r # (# is automatically determined by the block size)\n");
	printf("  -3 : reserved\n");
	printf("  -4 : synonymous with -l 8\n");
	printf("  -5 : synonymous with -l 8 -M\n");
	printf("  -6 : synonymous with -l 8 -m -r # (# is automatically determined by the block size)\n");
	printf("  -7 : reserved\n");
	printf("  -8 : synonymous with -l 32 -m -r # (# is automatically determined by the block size)\n");
	printf("  -9 : synonymous with -l 32 -m -e -r 99 -p (very slow!)\n");
	printf("  -e : do exhaustive model search (expensive!)\n");
	printf("  -l max_lpc_order : 0 => use only fixed predictors\n");
	printf("  -p : do exhaustive search of LP coefficient quantization (expensive!); overrides -q, does nothing if using -l 0\n");
	printf("  -q bits : precision of the quantized linear-predictor coefficients, 0 => let encoder decide (min is %u, default is -q 0)\n", FLAC__MIN_QLP_COEFF_PRECISION);
	printf("  -r level : rice parameter optimization level (level is 0..99, 0 => none, default is -r 0, above 4 doesn't usually help much)\n");
	printf("  -V : verify a correct encoding by decoding the output in parallel and comparing to the original\n");
	printf("  -m-, -M-, -e-, -p-, -V-, --lax- can all be used to turn off a particular option\n");
	printf("format options:\n");
	printf("  -fb | -fl : big-endian | little-endian byte order\n");
	printf("  -fc channels\n");
	printf("  -fp bits_per_sample\n");
	printf("  -fs sample_rate : in Hz\n");
	printf("  -fu : unsigned samples (default is signed)\n");
	printf("  -fr : force to raw format (even if filename ends in .wav)\n");
	printf("  -fw : force to RIFF WAVE\n");
	return 1;
}
