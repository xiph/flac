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

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FLAC/all.h"
#include "analyze.h"
#include "decode.h"
#include "encode.h"

static int usage(const char *message, ...);
static int encode_file(const char *infilename, const char *forced_outfilename);
static int decode_file(const char *infilename, const char *forced_outfilename);

bool verify = false, verbose = true, lax = false, test_only = false, analyze = false;
bool do_mid_side = true, loose_mid_side = false, do_exhaustive_model_search = false, do_qlp_coeff_prec_search = false;
bool force_to_stdout = false;
analysis_options aopts = { false, false };
unsigned padding = 0;
unsigned max_lpc_order = 8;
unsigned qlp_coeff_precision = 0;
uint64 skip = 0;
int format_is_wave = -1, format_is_big_endian = -1, format_is_unsigned_samples = false;
int format_channels = -1, format_bps = -1, format_sample_rate = -1;
int blocksize = -1, min_residual_partition_order = -1, max_residual_partition_order = -1, rice_parameter_search_dist = -1;
char requested_seek_points[50000]; /* @@@ bad MAGIC NUMBER */
int num_requested_seek_points = -1; /* -1 => no -S options were given, 0 => -S- was given */

int main(int argc, char *argv[])
{
	int i, retval = 0;
	bool mode_decode = false;

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
		else if(0 == strcmp(argv[i], "-c"))
			force_to_stdout = true;
		else if(0 == strcmp(argv[i], "-s"))
			verbose = false;
		else if(0 == strcmp(argv[i], "-s-"))
			verbose = true;
		else if(0 == strcmp(argv[i], "-S")) {
			if(num_requested_seek_points < 0)
				num_requested_seek_points = 0;
			num_requested_seek_points++;
			strcat(requested_seek_points, argv[++i]);
			strcat(requested_seek_points, "<");
		}
		else if(0 == strcmp(argv[i], "-S-")) {
			num_requested_seek_points = 0;
			requested_seek_points[0] = '\0';
		}
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
		else if(0 == strcmp(argv[i], "-m")) {
			do_mid_side = true;
			loose_mid_side = false;
		}
		else if(0 == strcmp(argv[i], "-m-"))
			do_mid_side = loose_mid_side = false;
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
		else if(0 == strcmp(argv[i], "-r")) {
			char *p = strchr(argv[++i], ',');
			if(0 == p) {
				min_residual_partition_order = 0;
				max_residual_partition_order = atoi(argv[i]);
			}
			else {
				min_residual_partition_order = atoi(argv[i]);
				max_residual_partition_order = atoi(++p);
			}
		}
		else if(0 == strcmp(argv[i], "-R"))
			rice_parameter_search_dist = atoi(argv[++i]);
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
		else if(0 == strcmp(argv[i], "--a-rgp"))
			aopts.do_residual_gnuplot = true;
		else if(0 == strcmp(argv[i], "--a-rgp-"))
			aopts.do_residual_gnuplot = false;
		else if(0 == strcmp(argv[i], "--a-rtext"))
			aopts.do_residual_text = true;
		else if(0 == strcmp(argv[i], "--a-rtext-"))
			aopts.do_residual_text = false;
		else if(0 == strcmp(argv[i], "-0")) {
			do_exhaustive_model_search = false;
			do_mid_side = false;
			loose_mid_side = false;
			qlp_coeff_precision = 0;
			min_residual_partition_order = max_residual_partition_order = 2;
			rice_parameter_search_dist = 0;
			max_lpc_order = 0;
		}
		else if(0 == strcmp(argv[i], "-1")) {
			do_exhaustive_model_search = false;
			do_mid_side = true;
			loose_mid_side = true;
			qlp_coeff_precision = 0;
			min_residual_partition_order = max_residual_partition_order = 2;
			rice_parameter_search_dist = 0;
			max_lpc_order = 0;
		}
		else if(0 == strcmp(argv[i], "-2")) {
			do_exhaustive_model_search = false;
			do_mid_side = true;
			loose_mid_side = false;
			qlp_coeff_precision = 0;
			min_residual_partition_order = 0;
			max_residual_partition_order = 3;
			rice_parameter_search_dist = 0;
			max_lpc_order = 0;
		}
		else if(0 == strcmp(argv[i], "-3")) {
			do_exhaustive_model_search = false;
			do_mid_side = false;
			loose_mid_side = false;
			qlp_coeff_precision = 0;
			min_residual_partition_order = max_residual_partition_order = 3;
			rice_parameter_search_dist = 0;
			max_lpc_order = 6;
		}
		else if(0 == strcmp(argv[i], "-4")) {
			do_exhaustive_model_search = false;
			do_mid_side = true;
			loose_mid_side = true;
			qlp_coeff_precision = 0;
			min_residual_partition_order = max_residual_partition_order = 3;
			rice_parameter_search_dist = 0;
			max_lpc_order = 8;
		}
		else if(0 == strcmp(argv[i], "-5")) {
			do_exhaustive_model_search = false;
			do_mid_side = true;
			loose_mid_side = false;
			qlp_coeff_precision = 0;
			min_residual_partition_order = max_residual_partition_order = 3;
			rice_parameter_search_dist = 0;
			max_lpc_order = 8;
		}
		else if(0 == strcmp(argv[i], "-6")) {
			do_exhaustive_model_search = false;
			do_mid_side = true;
			loose_mid_side = false;
			qlp_coeff_precision = 0;
			min_residual_partition_order = 0;
			max_residual_partition_order = 4;
			rice_parameter_search_dist = 0;
			max_lpc_order = 8;
		}
		else if(0 == strcmp(argv[i], "-7")) {
			do_exhaustive_model_search = true;
			do_mid_side = true;
			loose_mid_side = false;
			qlp_coeff_precision = 0;
			min_residual_partition_order = 0;
			max_residual_partition_order = 6;
			rice_parameter_search_dist = 0;
			max_lpc_order = 8;
		}
		else if(0 == strcmp(argv[i], "-8")) {
			do_exhaustive_model_search = true;
			do_mid_side = true;
			loose_mid_side = false;
			qlp_coeff_precision = 0;
			min_residual_partition_order = 0;
			max_residual_partition_order = 6;
			rice_parameter_search_dist = 0;
			max_lpc_order = 12;
		}
		else if(0 == strcmp(argv[i], "-9")) {
			do_exhaustive_model_search = true;
			do_mid_side = true;
			loose_mid_side = false;
			do_qlp_coeff_prec_search = true;
			min_residual_partition_order = 0;
			max_residual_partition_order = 16;
			rice_parameter_search_dist = 0;
			max_lpc_order = 32;
		}
		else if(isdigit((int)(argv[i][1]))) {
			return usage("ERROR: compression level '%s' is still reserved\n", argv[i]);
		}
		else {
			return usage("ERROR: invalid option '%s'\n", argv[i]);
		}
	}

	/* tweak options; validate the values */
	if(!mode_decode) {
		if(blocksize < 0) {
			if(max_lpc_order == 0)
				blocksize = 1152;
			else
				blocksize = 4608;
		}
		if(max_residual_partition_order < 0) {
			if(blocksize <= 1152)
				max_residual_partition_order = 2;
			else if(blocksize <= 2304)
				max_residual_partition_order = 3;
			else if(blocksize <= 4608)
				max_residual_partition_order = 3;
			else
				max_residual_partition_order = 4;
			min_residual_partition_order = max_residual_partition_order;
		}
		if(rice_parameter_search_dist < 0) {
			rice_parameter_search_dist = 0;
		}
	}
	else {
		if(test_only) {
			if(skip > 0)
				return usage("ERROR: --skip is not allowed in test mode\n");
		}
	}

	FLAC__ASSERT(blocksize >= 0 || mode_decode);

	if(format_channels >= 0) {
		if(format_channels == 0 || (unsigned)format_channels > FLAC__MAX_CHANNELS)
			return usage("ERROR: invalid number of channels '%u', must be > 0 and <= %u\n", format_channels, FLAC__MAX_CHANNELS);
	}
	if(format_bps >= 0) {
		if(format_bps != 8 && format_bps != 16 && format_bps != 24)
			return usage("ERROR: invalid bits per sample '%u' (must be 8/16/24)\n", format_bps);
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

	if(verbose) {
		fprintf(stderr, "\n");
		fprintf(stderr, "flac %s, Copyright (C) 2000,2001 Josh Coalson\n", FLAC__VERSION_STRING);
		fprintf(stderr, "flac comes with ABSOLUTELY NO WARRANTY.  This is free software, and you are\n");
		fprintf(stderr, "welcome to redistribute it under certain conditions.  Type `flac' for details.\n\n");

		if(!mode_decode) {
			fprintf(stderr, "options:%s -P %u -b %u%s -l %u%s%s -q %u -r %u,%u -R %u%s\n",
				lax?" --lax":"", padding, (unsigned)blocksize, loose_mid_side?" -M":do_mid_side?" -m":"", max_lpc_order,
				do_exhaustive_model_search?" -e":"", do_qlp_coeff_prec_search?" -p":"",
				qlp_coeff_precision,
				(unsigned)min_residual_partition_order, (unsigned)max_residual_partition_order, (unsigned)rice_parameter_search_dist,
				verify? " -V":""
			);
		}
	}

	if(mode_decode) {
		bool first = true;
		int save_format;

		if(i == argc)
			retval = decode_file("-", 0);
		else if(i + 2 == argc && 0 == strcmp(argv[i], "-"))
			retval = decode_file("-", argv[i+1]);
		else {
			for(retval = 0; i < argc && retval == 0; i++) {
				if(0 == strcmp(argv[i], "-") && !first)
					continue;
				save_format = format_is_wave;
				retval = decode_file(argv[i], 0);
				format_is_wave = save_format;
				first = false;
			}
		}
	}
	else { /* encode */
		bool first = true;
		int save_format;

		if(i == argc)
			retval = encode_file("-", 0);
		else if(i + 2 == argc && 0 == strcmp(argv[i], "-"))
			retval = encode_file("-", argv[i+1]);
		else {
			for(retval = 0; i < argc && retval == 0; i++) {
				if(0 == strcmp(argv[i], "-") && !first)
					continue;
				save_format = format_is_wave;
				retval = encode_file(argv[i], 0);
				format_is_wave = save_format;
				first = false;
			}
		}
	}

	return retval;
}

int usage(const char *message, ...)
{
	va_list args;

	if(message) {
		va_start(args, message);

		(void) vfprintf(stderr, message, args);

		va_end(args);

	}
	fprintf(stderr, "===============================================================================\n");
	fprintf(stderr, "flac - Command-line FLAC encoder/decoder version %s\n", FLAC__VERSION_STRING);
	fprintf(stderr, "Copyright (C) 2000,2001  Josh Coalson\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "This program is free software; you can redistribute it and/or\n");
	fprintf(stderr, "modify it under the terms of the GNU General Public License\n");
	fprintf(stderr, "as published by the Free Software Foundation; either version 2\n");
	fprintf(stderr, "of the License, or (at your option) any later version.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "This program is distributed in the hope that it will be useful,\n");
	fprintf(stderr, "but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	fprintf(stderr, "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
	fprintf(stderr, "GNU General Public License for more details.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "You should have received a copy of the GNU General Public License\n");
	fprintf(stderr, "along with this program; if not, write to the Free Software\n");
	fprintf(stderr, "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.\n");
	fprintf(stderr, "===============================================================================\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  flac [options] [infile [...]]\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "For encoding:\n");
	fprintf(stderr, "  the input file(s) may be a PCM RIFF WAVE file or raw samples\n");
	fprintf(stderr, "  the output file(s) will be in FLAC format\n");
	fprintf(stderr, "For decoding, the reverse is true\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "A single 'infile' may be - for stdin.  No 'infile' implies stdin.  Use of\n");
	fprintf(stderr, "stdin implies -c (write to stdout).  However, flac allows the special\n");
	fprintf(stderr, "encoding/decoding forms:\n");
	fprintf(stderr, "   flac [options] - outfilename   or  flac -d [options] - outfilename\n");
	fprintf(stderr, "which is better than:\n");
	fprintf(stderr, "   flac [options] > outfilename   or  flac -d [options] > outfilename\n");
	fprintf(stderr, "since the former allows flac to seek backwards to write the STREAMINFO or\n");
	fprintf(stderr, "RIFF WAVE header contents when necessary.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "If the unencoded filename ends with '.wav' or -fw is used, it's assumed to be\n");
	fprintf(stderr, "RIFF WAVE.  Otherwise, flac will check for the presence of a RIFF header.  If\n");
	fprintf(stderr, "any infile is raw you must specify the format options {-fb|fl} -fc -fp and -fs,\n");
	fprintf(stderr, "which will apply to all raw files.  You can force a .wav file to be treated as\n");
	fprintf(stderr, "a raw file using -fr.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "generic options:\n");
	fprintf(stderr, "  -d : decode (default behavior is encode)\n");
	fprintf(stderr, "  -t : test (same as -d except no decoded file is written)\n");
	fprintf(stderr, "  -a : analyze (same as -d except an analysis file is written)\n");
	fprintf(stderr, "  -c : write output to stdout\n");
	fprintf(stderr, "  -s : silent (do not write runtime encode/decode statistics)\n");
	fprintf(stderr, "  --skip samples : can be used both for encoding and decoding\n");
	fprintf(stderr, "analyze options:\n");
	fprintf(stderr, "  --a-rtext : include residual signal in text output\n");
	fprintf(stderr, "  --a-rgp : generate gnuplot files of residual distribution of each subframe\n");
	fprintf(stderr, "encoding options:\n");
	fprintf(stderr, "  --lax : allow encoder to generate non-Subset files\n");
	fprintf(stderr, "  -S { # | X | #x } : include a point or points in a SEEKTABLE\n");
	fprintf(stderr, "       #  : a specific sample number for a seek point\n");
	fprintf(stderr, "       X  : a placeholder point (always goes at the end of the SEEKTABLE)\n");
	fprintf(stderr, "       #x : # evenly spaced seekpoints, the first being at sample 0\n");
	fprintf(stderr, "     You may use many -S options; the resulting SEEKTABLE will be the unique-\n");
	fprintf(stderr, "           ified union of all such values.\n");
	fprintf(stderr, "     With no -S options, flac defaults to '-S 100x'.  Use -S- for no SEEKTABLE.\n");
	fprintf(stderr, "     Note: -S #x will not work if the encoder can't determine the input size\n");
	fprintf(stderr, "           before starting.\n");
	fprintf(stderr, "     Note: if you use -S # and # is >= samples in the input, there will be\n");
	fprintf(stderr, "           either no seek point entered (if the input size is determinable\n");
	fprintf(stderr, "           before encoding starts) or a placeholder point (if input size is not\n");
	fprintf(stderr, "           determinable)\n");
	fprintf(stderr, "  -P # : write a PADDING block of # bytes (goes after SEEKTABLE)\n");
	fprintf(stderr, "         (0 => no PADDING block, default is -P 0)\n");
	fprintf(stderr, "  -b # : specify blocksize in samples; default is 1152 for -l 0, else 4608;\n");
	fprintf(stderr, "         must be 192/576/1152/2304/4608/256/512/1024/2048/4096/8192/16384/32768\n");
	fprintf(stderr, "         (unless --lax is used)\n");
	fprintf(stderr, "  -m   : try mid-side coding for each frame (stereo input only)\n");
	fprintf(stderr, "  -M   : loose mid-side coding for all frames (stereo input only)\n");
	fprintf(stderr, "  -0 .. -9 : fastest compression .. highest compression, default is -5\n");
	fprintf(stderr, "             these are synonyms for other options:\n");
	fprintf(stderr, "  -0   : synonymous with -l 0 -b 1152 -r 2,2\n");
	fprintf(stderr, "  -1   : synonymous with -l 0 -b 1152 -M -r 2,2\n");
	fprintf(stderr, "  -2   : synonymous with -l 0 -b 1152 -m -r 3\n");
	fprintf(stderr, "  -3   : synonymous with -l 6 -b 4608 -r 3,3\n");
	fprintf(stderr, "  -4   : synonymous with -l 8 -b 4608 -M -r 3,3\n");
	fprintf(stderr, "  -5   : synonymous with -l 8 -b 4608 -m -r 3,3\n");
	fprintf(stderr, "  -6   : synonymous with -l 8 -b 4608 -m -r 4\n");
	fprintf(stderr, "  -7   : synonymous with -l 8 -b 4608 -m -e -r 6\n");
	fprintf(stderr, "  -8   : synonymous with -l 12 -b 4608 -m -e -r 6\n");
	fprintf(stderr, "  -9   : synonymous with -l 32 -b 4608 -m -e -r 16 -p (very slow!)\n");
	fprintf(stderr, "  -e   : do exhaustive model search (expensive!)\n");
	fprintf(stderr, "  -l # : specify max LPC order; 0 => use only fixed predictors\n");
	fprintf(stderr, "  -p   : do exhaustive search of LP coefficient quantization (expensive!);\n");
	fprintf(stderr, "         overrides -q, does nothing if using -l 0\n");
	fprintf(stderr, "  -q # : specify precision in bits of quantized linear-predictor coefficients;\n");
	fprintf(stderr, "         0 => let encoder decide (min is %u, default is -q 0)\n", FLAC__MIN_QLP_COEFF_PRECISION);
	fprintf(stderr, "  -r [#,]# : [min,]max residual partition order (# is 0..16; min defaults to 0;\n");
	fprintf(stderr, "         default is -r 0; above 4 doesn't usually help much)\n");
	fprintf(stderr, "  -R # : Rice parameter search distance (# is 0..32; above 2 doesn't help much)\n");
	fprintf(stderr, "  -V   : verify a correct encoding by decoding the output in parallel and\n");
	fprintf(stderr, "         comparing to the original\n");
	fprintf(stderr, "  -S-, -m-, -M-, -e-, -p-, -V-, --lax- can all be used to turn off a particular\n");
	fprintf(stderr, "  option\n");
	fprintf(stderr, "format options:\n");
	fprintf(stderr, "  -fb | -fl : big-endian | little-endian byte order\n");
	fprintf(stderr, "  -fc channels\n");
	fprintf(stderr, "  -fp bits_per_sample\n");
	fprintf(stderr, "  -fs sample_rate : in Hz\n");
	fprintf(stderr, "  -fu : unsigned samples (default is signed)\n");
	fprintf(stderr, "  -fr : force to raw format (even if filename ends in .wav)\n");
	fprintf(stderr, "  -fw : force to RIFF WAVE\n");
	return 1;
}

int encode_file(const char *infilename, const char *forced_outfilename)
{
	FILE *encode_infile;
	char outfilename[4096]; /* @@@ bad MAGIC NUMBER */
	char *p;

	if(0 == strcmp(infilename, "-")) {
		encode_infile = stdin;
	}
	else {
		if(0 == (encode_infile = fopen(infilename, "rb"))) {
			fprintf(stderr, "ERROR: can't open input file %s\n", infilename);
			return 1;
		}
	}

	if(verbose)
		fprintf(stderr, "%s:\n", infilename);

	if(format_is_wave < 0) {
		/* lamely attempt to guess the file type based on the first 4 bytes (which is all ungetc will guarantee us) */
		char head[4];
		int h, n;
		/* first set format based on name */
		if(strstr(infilename, ".wav") == infilename + (strlen(infilename) - strlen(".wav")))
			format_is_wave = true;
		else
			format_is_wave = false;
		if((n = fread(head, 1, 4, encode_infile)) < 4) {
			if(format_is_wave)
				fprintf(stderr, "WARNING: %s is not a WAVE file, treating as a raw file\n", infilename);
			format_is_wave = false;
		}
		else {
			if(strncmp(head, "RIFF", 4)) {
				if(format_is_wave)
					fprintf(stderr, "WARNING: %s is not a WAVE file, treating as a raw file\n", infilename);
				format_is_wave = false;
			}
			else
				format_is_wave = true;
		}
		for(h = n-1; h >= 0; h--)
			ungetc(head[h], encode_infile);
	}

	if(!format_is_wave) {
		if(format_is_big_endian < 0 || format_channels < 0 || format_bps < 0 || format_sample_rate < 0)
			return usage("ERROR: for encoding a raw file you must specify { -fb or -fl }, -fc, -fp, and -fs\n");
	}

	if(encode_infile == stdin || force_to_stdout)
		strcpy(outfilename, "-");
	else {
		strcpy(outfilename, infilename);
		if(0 == (p = strrchr(outfilename, '.')))
			strcat(outfilename, ".flac");
		else {
			if(0 == strcmp(p, ".flac"))
				strcpy(p, "_new.flac");
			else
				strcpy(p, ".flac");
		}
	}
	if(0 == forced_outfilename)
		forced_outfilename = outfilename;

	if(format_is_wave)
		return encode_wav(encode_infile, infilename, forced_outfilename, verbose, skip, verify, lax, do_mid_side, loose_mid_side, do_exhaustive_model_search, do_qlp_coeff_prec_search, min_residual_partition_order, max_residual_partition_order, rice_parameter_search_dist, max_lpc_order, (unsigned)blocksize, qlp_coeff_precision, padding, requested_seek_points, num_requested_seek_points);
	else
		return encode_raw(encode_infile, infilename, forced_outfilename, verbose, skip, verify, lax, do_mid_side, loose_mid_side, do_exhaustive_model_search, do_qlp_coeff_prec_search, min_residual_partition_order, max_residual_partition_order, rice_parameter_search_dist, max_lpc_order, (unsigned)blocksize, qlp_coeff_precision, padding, requested_seek_points, num_requested_seek_points, format_is_big_endian, format_is_unsigned_samples, format_channels, format_bps, format_sample_rate);
}

int decode_file(const char *infilename, const char *forced_outfilename)
{
	static const char *suffixes[] = { ".wav", ".raw" };
	char outfilename[4096]; /* @@@ bad MAGIC NUMBER */
	char *p;

	if(!test_only && !analyze) {
		if(format_is_wave < 0) {
			format_is_wave = true;
		}
		if(!format_is_wave) {
			if(format_is_big_endian < 0)
				return usage("ERROR: for decoding to a raw file you must specify -fb or -fl\n");
		}
	}

	if(0 == strcmp(infilename, "-") || force_to_stdout)
		strcpy(outfilename, "-");
	else {
		const char *suffix = suffixes[format_is_wave? 0:1];
		strcpy(outfilename, infilename);
		if(0 == (p = strrchr(outfilename, '.')))
			strcat(outfilename, suffix);
		else {
			if(0 == strcmp(p, suffix)) {
				strcpy(p, "_new");
				strcat(p, suffix);
			}
			else
				strcpy(p, suffix);
		}
	}
	if(0 == forced_outfilename)
		forced_outfilename = outfilename;

	if(format_is_wave)
		return decode_wav(infilename, test_only? 0 : forced_outfilename, analyze, aopts, verbose, skip);
	else
		return decode_raw(infilename, test_only? 0 : forced_outfilename, analyze, aopts, verbose, skip, format_is_big_endian, format_is_unsigned_samples);
}
