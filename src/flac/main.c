/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2000,2001,2002  Josh Coalson
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
#if !defined _MSC_VER && !defined __MINGW32__
/* unlink is in stdio.h in VC++ */
#include <unistd.h> /* for unlink() */
#else
#define strcasecmp stricmp
#endif
#include "FLAC/all.h"
#include "analyze.h"
#include "decode.h"
#include "encode.h"
#include "file.h"

static int short_usage(const char *message, ...);
static int long_usage(const char *message, ...);
static int encode_file(const char *infilename, const char *forced_outfilename, FLAC__bool is_last_file);
static int decode_file(const char *infilename, const char *forced_outfilename);

FLAC__bool verify = false, verbose = true, lax = false, test_only = false, analyze = false, use_ogg = false;
FLAC__bool do_mid_side = true, loose_mid_side = false, do_exhaustive_model_search = false, do_escape_coding = false, do_qlp_coeff_prec_search = false;
FLAC__bool force_to_stdout = false, force_raw_format = false, delete_input = false, sector_align = false;
const char *cmdline_forced_outfilename = 0, *output_prefix = 0;
analysis_options aopts = { false, false };
unsigned padding = 0;
unsigned max_lpc_order = 8;
unsigned qlp_coeff_precision = 0;
FLAC__uint64 skip = 0;
int format_is_big_endian = -1, format_is_unsigned_samples = false;
int format_channels = -1, format_bps = -1, format_sample_rate = -1;
int blocksize = -1, min_residual_partition_order = -1, max_residual_partition_order = -1, rice_parameter_search_dist = -1;
char requested_seek_points[50000]; /* @@@ bad MAGIC NUMBER */
int num_requested_seek_points = -1; /* -1 => no -S options were given, 0 => -S- was given */
FLAC__int32 align_reservoir_0[588], align_reservoir_1[588]; /* for carrying over samples from --sector-align */
FLAC__int32 *align_reservoir[2] = { align_reservoir_0, align_reservoir_1 };
unsigned align_reservoir_samples = 0; /* 0 .. 587 */

static const char *flac_suffix = ".flac", *ogg_suffix = ".ogg";

int main(int argc, char *argv[])
{
	int i, retval = 0;
	FLAC__bool mode_decode = false;

	if(argc <= 1)
		return short_usage(0);

	/* get the options */
	for(i = 1; i < argc; i++) {
		if(argv[i][0] != '-' || argv[i][1] == 0)
			break;
		if(0 == strcmp(argv[i], "-H") || 0 == strcmp(argv[i], "--help"))
			return long_usage(0);
		else if(0 == strcmp(argv[i], "-d"))
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
			if(++i >= argc)
				return long_usage("ERROR: must specify a value with -S\n");
			if(num_requested_seek_points < 0)
				num_requested_seek_points = 0;
			num_requested_seek_points++;
			strcat(requested_seek_points, argv[i]);
			strcat(requested_seek_points, "<");
		}
		else if(0 == strcmp(argv[i], "-S-")) {
			num_requested_seek_points = 0;
			requested_seek_points[0] = '\0';
		}
		else if(0 == strcmp(argv[i], "--delete-input-file"))
			delete_input = true;
		else if(0 == strcmp(argv[i], "--delete-input-file-"))
			delete_input = false;
		else if(0 == strcmp(argv[i], "--output-prefix")) {
			if(++i >= argc)
				return long_usage("ERROR: must specify a value with --output-prefix\n");
			output_prefix = argv[i];
		}
		else if(0 == strcmp(argv[i], "--sector-align"))
			sector_align = true;
		else if(0 == strcmp(argv[i], "--sector-align-"))
			sector_align = false;
		else if(0 == strcmp(argv[i], "--skip")) {
			if(++i >= argc)
				return long_usage("ERROR: must specify a value with --skip\n");
			skip = (FLAC__uint64)atoi(argv[i]); /* @@@ takes a pretty damn big file to overflow atoi() here, but it could happen */
		}
		else if(0 == strcmp(argv[i], "--lax"))
			lax = true;
		else if(0 == strcmp(argv[i], "--lax-"))
			lax = false;
#ifdef FLAC__HAS_OGG
		else if(0 == strcmp(argv[i], "--ogg"))
			use_ogg = true;
		else if(0 == strcmp(argv[i], "--ogg-"))
			use_ogg = false;
#endif
		else if(0 == strcmp(argv[i], "-b")) {
			if(++i >= argc)
				return long_usage("ERROR: must specify a value with -b\n");
			blocksize = atoi(argv[i]);
		}
		else if(0 == strcmp(argv[i], "-e"))
			do_exhaustive_model_search = true;
		else if(0 == strcmp(argv[i], "-e-"))
			do_exhaustive_model_search = false;
		else if(0 == strcmp(argv[i], "-E"))
			do_escape_coding = true;
		else if(0 == strcmp(argv[i], "-E-"))
			do_escape_coding = false;
		else if(0 == strcmp(argv[i], "-l")) {
			if(++i >= argc)
				return long_usage("ERROR: must specify a value with -l\n");
			max_lpc_order = atoi(argv[i]);
		}
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
		else if(0 == strcmp(argv[i], "-o")) {
			if(++i >= argc)
				return long_usage("ERROR: must specify a value with -o\n");
			cmdline_forced_outfilename = argv[i];
		}
		else if(0 == strcmp(argv[i], "-p"))
			do_qlp_coeff_prec_search = true;
		else if(0 == strcmp(argv[i], "-p-"))
			do_qlp_coeff_prec_search = false;
		else if(0 == strcmp(argv[i], "-P")) {
			if(++i >= argc)
				return long_usage("ERROR: must specify a value with -P\n");
			padding = atoi(argv[i]);
		}
		else if(0 == strcmp(argv[i], "-q")) {
			if(++i >= argc)
				return long_usage("ERROR: must specify a value with -q\n");
			qlp_coeff_precision = atoi(argv[i]);
		}
		else if(0 == strcmp(argv[i], "-r")) {
			char *p;
			if(++i >= argc)
				return long_usage("ERROR: must specify a value with -r\n");
			p = strchr(argv[i], ',');
			if(0 == p) {
				min_residual_partition_order = 0;
				max_residual_partition_order = atoi(argv[i]);
			}
			else {
				min_residual_partition_order = atoi(argv[i]);
				max_residual_partition_order = atoi(++p);
			}
		}
		else if(0 == strcmp(argv[i], "-R")) {
			if(++i >= argc)
				return long_usage("ERROR: must specify a value with -R\n");
			rice_parameter_search_dist = atoi(argv[i]);
		}
		else if(0 == strcmp(argv[i], "-V"))
			verify = true;
		else if(0 == strcmp(argv[i], "-V-"))
			verify = false;
		else if(0 == strcmp(argv[i], "-fb"))
			format_is_big_endian = true;
		else if(0 == strcmp(argv[i], "-fl"))
			format_is_big_endian = false;
		else if(0 == strcmp(argv[i], "-fc")) {
			if(++i >= argc)
				return long_usage("ERROR: must specify a value with -fc\n");
			format_channels = atoi(argv[i]);
		}
		else if(0 == strcmp(argv[i], "-fp")) {
			if(++i >= argc)
				return long_usage("ERROR: must specify a value with -fp\n");
			format_bps = atoi(argv[i]);
		}
		else if(0 == strcmp(argv[i], "-fs")) {
			if(++i >= argc)
				return long_usage("ERROR: must specify a value with -fs\n");
			format_sample_rate = atoi(argv[i]);
		}
		else if(0 == strcmp(argv[i], "-fu"))
			format_is_unsigned_samples = true;
		else if(0 == strcmp(argv[i], "-fr"))
			force_raw_format = true;
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
			do_escape_coding = false;
			do_mid_side = false;
			loose_mid_side = false;
			qlp_coeff_precision = 0;
			min_residual_partition_order = max_residual_partition_order = 2;
			rice_parameter_search_dist = 0;
			max_lpc_order = 0;
		}
		else if(0 == strcmp(argv[i], "-1")) {
			do_exhaustive_model_search = false;
			do_escape_coding = false;
			do_mid_side = true;
			loose_mid_side = true;
			qlp_coeff_precision = 0;
			min_residual_partition_order = max_residual_partition_order = 2;
			rice_parameter_search_dist = 0;
			max_lpc_order = 0;
		}
		else if(0 == strcmp(argv[i], "-2")) {
			do_exhaustive_model_search = false;
			do_escape_coding = false;
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
			do_escape_coding = false;
			do_mid_side = false;
			loose_mid_side = false;
			qlp_coeff_precision = 0;
			min_residual_partition_order = max_residual_partition_order = 3;
			rice_parameter_search_dist = 0;
			max_lpc_order = 6;
		}
		else if(0 == strcmp(argv[i], "-4")) {
			do_exhaustive_model_search = false;
			do_escape_coding = false;
			do_mid_side = true;
			loose_mid_side = true;
			qlp_coeff_precision = 0;
			min_residual_partition_order = max_residual_partition_order = 3;
			rice_parameter_search_dist = 0;
			max_lpc_order = 8;
		}
		else if(0 == strcmp(argv[i], "-5")) {
			do_exhaustive_model_search = false;
			do_escape_coding = false;
			do_mid_side = true;
			loose_mid_side = false;
			qlp_coeff_precision = 0;
			min_residual_partition_order = max_residual_partition_order = 3;
			rice_parameter_search_dist = 0;
			max_lpc_order = 8;
		}
		else if(0 == strcmp(argv[i], "-6")) {
			do_exhaustive_model_search = false;
			do_escape_coding = false;
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
			do_escape_coding = false;
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
			do_escape_coding = false;
			do_mid_side = true;
			loose_mid_side = false;
			qlp_coeff_precision = 0;
			min_residual_partition_order = 0;
			max_residual_partition_order = 6;
			rice_parameter_search_dist = 0;
			max_lpc_order = 12;
		}
		else if(0 == strcmp(argv[i], "--super-secret-impractical-compression-level")) {
			do_exhaustive_model_search = true;
			do_escape_coding = true;
			do_mid_side = true;
			loose_mid_side = false;
			do_qlp_coeff_prec_search = true;
			min_residual_partition_order = 0;
			max_residual_partition_order = 16;
			rice_parameter_search_dist = 0;
			max_lpc_order = 32;
		}
		else if(isdigit((int)(argv[i][1]))) {
			return long_usage("ERROR: compression level '%s' is reserved\n", argv[i]);
		}
		else {
			return long_usage("ERROR: invalid option '%s'\n", argv[i]);
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
				return long_usage("ERROR: --skip is not allowed in test mode\n");
		}
	}

	FLAC__ASSERT(blocksize >= 0 || mode_decode);

	if(format_channels >= 0) {
		if(format_channels == 0 || (unsigned)format_channels > FLAC__MAX_CHANNELS)
			return long_usage("ERROR: invalid number of channels '%u', must be > 0 and <= %u\n", format_channels, FLAC__MAX_CHANNELS);
	}
	if(format_bps >= 0) {
		if(format_bps != 8 && format_bps != 16 && format_bps != 24)
			return long_usage("ERROR: invalid bits per sample '%u' (must be 8/16/24)\n", format_bps);
	}
	if(format_sample_rate >= 0) {
		if(format_sample_rate == 0 || (unsigned)format_sample_rate > FLAC__MAX_SAMPLE_RATE)
			return long_usage("ERROR: invalid sample rate '%u', must be > 0 and <= %u\n", format_sample_rate, FLAC__MAX_SAMPLE_RATE);
	}
	if(!mode_decode && ((unsigned)blocksize < FLAC__MIN_BLOCK_SIZE || (unsigned)blocksize > FLAC__MAX_BLOCK_SIZE)) {
		return long_usage("ERROR: invalid blocksize '%u', must be >= %u and <= %u\n", (unsigned)blocksize, FLAC__MIN_BLOCK_SIZE, FLAC__MAX_BLOCK_SIZE);
	}
	if(qlp_coeff_precision > 0 && qlp_coeff_precision < FLAC__MIN_QLP_COEFF_PRECISION) {
		return long_usage("ERROR: invalid value for -q '%u', must be 0 or >= %u\n", qlp_coeff_precision, FLAC__MIN_QLP_COEFF_PRECISION);
	}

	if(sector_align) {
		if(mode_decode)
			return long_usage("ERROR: --sector-align only allowed for encoding\n");
		else if(skip > 0)
			return long_usage("ERROR: --sector-align not allowed with --skip\n");
		else if(format_channels >= 0 && format_channels != 2)
			return long_usage("ERROR: --sector-align can only be done with stereo input\n");
		else if(format_sample_rate >= 0 && format_sample_rate != 2)
			return long_usage("ERROR: --sector-align can only be done with sample rate of 44100\n");
	}
	if(argc - i > 1 && cmdline_forced_outfilename) {
		return long_usage("ERROR: -o cannot be used with multiple files\n");
	}
	if(cmdline_forced_outfilename && output_prefix) {
		return long_usage("ERROR: --output-prefix conflicts with -o\n");
	}

	if(verbose) {
		fprintf(stderr, "\n");
		fprintf(stderr, "flac %s, Copyright (C) 2000,2001,2002 Josh Coalson\n", FLAC__VERSION_STRING);
		fprintf(stderr, "flac comes with ABSOLUTELY NO WARRANTY.  This is free software, and you are\n");
		fprintf(stderr, "welcome to redistribute it under certain conditions.  Type `flac' for details.\n\n");

		if(!mode_decode) {
			fprintf(stderr,
				"options:%s%s"
#ifdef FLAC__HAS_OGG
				"%s"
#endif
				"%s -P %u -b %u%s -l %u%s%s%s -q %u -r %u,%u -R %u%s\n",
				delete_input?" --delete-input-file":"", sector_align?" --sector-align":"",
#ifdef FLAC__HAS_OGG
				use_ogg?" --ogg":"",
#endif
				lax?" --lax":"",
				padding, (unsigned)blocksize, loose_mid_side?" -M":do_mid_side?" -m":"", max_lpc_order,
				do_exhaustive_model_search?" -e":"", do_escape_coding?" -E":"", do_qlp_coeff_prec_search?" -p":"",
				qlp_coeff_precision,
				(unsigned)min_residual_partition_order, (unsigned)max_residual_partition_order, (unsigned)rice_parameter_search_dist,
				verify? " -V":""
			);
		}
	}

	if(mode_decode) {
		FLAC__bool first = true;

		if(i == argc) {
			retval = decode_file("-", 0);
		}
		else {
			if(i + 1 != argc)
				cmdline_forced_outfilename = 0;
			for(retval = 0; i < argc && retval == 0; i++) {
				if(0 == strcmp(argv[i], "-") && !first)
					continue;
				retval = decode_file(argv[i], 0);
				first = false;
			}
		}
	}
	else { /* encode */
		FLAC__bool first = true;

		if(i == argc) {
			retval = encode_file("-", 0, true);
		}
		else {
			if(i + 1 != argc)
				cmdline_forced_outfilename = 0;
			for(retval = 0; i < argc && retval == 0; i++) {
				if(0 == strcmp(argv[i], "-") && !first)
					continue;
				retval = encode_file(argv[i], 0, i == (argc-1));
				first = false;
			}
		}
	}

	return retval;
}

static void usage_header()
{
	fprintf(stderr, "===============================================================================\n");
	fprintf(stderr, "flac - Command-line FLAC encoder/decoder version %s\n", FLAC__VERSION_STRING);
	fprintf(stderr, "Copyright (C) 2000,2001,2002  Josh Coalson\n");
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
}

int short_usage(const char *message, ...)
{
	va_list args;

	if(message) {
		va_start(args, message);

		(void) vfprintf(stderr, message, args);

		va_end(args);

	}
	usage_header();
	fprintf(stderr, "\n");
	fprintf(stderr, "This is the short help; for full help use 'flac --help'\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "To encode:\n");
	fprintf(stderr, "  flac [-#] [infile [...]]\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  -# is -0 (fastest compression) to -8 (highest compression); -5 is the default\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "To decode:\n");
	fprintf(stderr, "  flac -d [infile [...]]\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "To test:\n");
	fprintf(stderr, "  flac -t [infile [...]]\n");

	return message? 1 : 0;
}

int long_usage(const char *message, ...)
{
	FILE *out = (message? stderr : stdout);
	va_list args;

	if(message) {
		va_start(args, message);

		(void) vfprintf(stderr, message, args);

		va_end(args);

	}
	usage_header();
	fprintf(out, "Usage:\n");
	fprintf(out, "  flac [options] [infile [...]]\n");
	fprintf(out, "\n");
	fprintf(out, "For encoding:\n");
	fprintf(out, "  the input file(s) may be a PCM RIFF WAVE file or raw samples\n");
	fprintf(out, "  the output file(s) will be in FLAC format\n");
	fprintf(out, "For decoding, the reverse is true\n");
	fprintf(out, "\n");
	fprintf(out, "A single 'infile' may be - for stdin.  No 'infile' implies stdin.  Use of\n");
	fprintf(out, "stdin implies -c (write to stdout).  Normally you should use:\n");
	fprintf(out, "   flac [options] -o outfilename  or  flac -d [options] -o outfilename\n");
	fprintf(out, "instead of:\n");
	fprintf(out, "   flac [options] > outfilename   or  flac -d [options] > outfilename\n");
	fprintf(out, "since the former allows flac to seek backwards to write the STREAMINFO or\n");
	fprintf(out, "RIFF WAVE header contents when necessary.\n");
	fprintf(out, "\n");
	fprintf(out, "flac checks for the presence of a RIFF WAVE header to decide whether or not\n");
	fprintf(out, "to treat an input file as WAVE format or raw samples.  If any infile is raw\n");
	fprintf(out, "you must specify the format options {-fb|fl} -fc -fp and -fs, which will\n");
	fprintf(out, "apply to all raw files.  You can force WAVE files to be treated as a raw files\n");
	fprintf(out, "using -fr.\n");
	fprintf(out, "\n");
	fprintf(out, "generic options:\n");
	fprintf(out, "  -d : decode (default behavior is encode)\n");
	fprintf(out, "  -t : test (same as -d except no decoded file is written)\n");
	fprintf(out, "  -a : analyze (same as -d except an analysis file is written)\n");
	fprintf(out, "  -c : write output to stdout\n");
	fprintf(out, "  -s : silent (do not write runtime encode/decode statistics)\n");
	fprintf(out, "  -o filename : force the output file name (usually flac just changes the\n");
	fprintf(out, "                extension)\n");
	fprintf(out, "  --delete-input-file : deletes the input file after a successful encode/decode\n");
	fprintf(out, "  --skip samples : can be used both for encoding and decoding\n");
	fprintf(out, "analyze options:\n");
	fprintf(out, "  --a-rtext : include residual signal in text output\n");
	fprintf(out, "  --a-rgp : generate gnuplot files of residual distribution of each subframe\n");
	fprintf(out, "encoding options:\n");
#ifdef FLAC__HAS_OGG
	fprintf(out, "  --ogg : output Ogg-FLAC stream instead of native FLAC\n");
#endif
	fprintf(out, "  --lax : allow encoder to generate non-Subset files\n");
	fprintf(out, "  --sector-align : align encoding of multiple files on sector boundaries\n");
	fprintf(out, "  -S { # | X | #x } : include a point or points in a SEEKTABLE\n");
	fprintf(out, "       #  : a specific sample number for a seek point\n");
	fprintf(out, "       X  : a placeholder point (always goes at the end of the SEEKTABLE)\n");
	fprintf(out, "       #x : # evenly spaced seekpoints, the first being at sample 0\n");
	fprintf(out, "     You may use many -S options; the resulting SEEKTABLE will be the unique-\n");
	fprintf(out, "           ified union of all such values.\n");
	fprintf(out, "     With no -S options, flac defaults to '-S 100x'.  Use -S- for no SEEKTABLE.\n");
	fprintf(out, "     Note: -S #x will not work if the encoder can't determine the input size\n");
	fprintf(out, "           before starting.\n");
	fprintf(out, "     Note: if you use -S # and # is >= samples in the input, there will be\n");
	fprintf(out, "           either no seek point entered (if the input size is determinable\n");
	fprintf(out, "           before encoding starts) or a placeholder point (if input size is not\n");
	fprintf(out, "           determinable)\n");
	fprintf(out, "  -P # : write a PADDING block of # bytes (goes after SEEKTABLE)\n");
	fprintf(out, "         (0 => no PADDING block, default is -P 0)\n");
	fprintf(out, "  -b # : specify blocksize in samples; default is 1152 for -l 0, else 4608;\n");
	fprintf(out, "         must be 192/576/1152/2304/4608/256/512/1024/2048/4096/8192/16384/32768\n");
	fprintf(out, "         (unless --lax is used)\n");
	fprintf(out, "  -m   : try mid-side coding for each frame (stereo input only)\n");
	fprintf(out, "  -M   : adaptive mid-side coding for all frames (stereo input only)\n");
	fprintf(out, "  -0 .. -8 : fastest compression .. highest compression, default is -5\n");
	fprintf(out, "             these are synonyms for other options:\n");
	fprintf(out, "  -0   : synonymous with -l 0 -b 1152 -r 2,2\n");
	fprintf(out, "  -1   : synonymous with -l 0 -b 1152 -M -r 2,2\n");
	fprintf(out, "  -2   : synonymous with -l 0 -b 1152 -m -r 3\n");
	fprintf(out, "  -3   : synonymous with -l 6 -b 4608 -r 3,3\n");
	fprintf(out, "  -4   : synonymous with -l 8 -b 4608 -M -r 3,3\n");
	fprintf(out, "  -5   : synonymous with -l 8 -b 4608 -m -r 3,3\n");
	fprintf(out, "  -6   : synonymous with -l 8 -b 4608 -m -r 4\n");
	fprintf(out, "  -7   : synonymous with -l 8 -b 4608 -m -e -r 6\n");
	fprintf(out, "  -8   : synonymous with -l 12 -b 4608 -m -e -r 6\n");
	fprintf(out, "  -e   : do exhaustive model search (expensive!)\n");
	fprintf(out, "  -E   : include escape coding in the entropy coder\n");
	fprintf(out, "  -l # : specify max LPC order; 0 => use only fixed predictors\n");
	fprintf(out, "  -p   : do exhaustive search of LP coefficient quantization (expensive!);\n");
	fprintf(out, "         overrides -q, does nothing if using -l 0\n");
	fprintf(out, "  -q # : specify precision in bits of quantized linear-predictor coefficients;\n");
	fprintf(out, "         0 => let encoder decide (min is %u, default is -q 0)\n", FLAC__MIN_QLP_COEFF_PRECISION);
	fprintf(out, "  -r [#,]# : [min,]max residual partition order (# is 0..16; min defaults to 0;\n");
	fprintf(out, "         default is -r 0; above 4 doesn't usually help much)\n");
	fprintf(out, "  -R # : Rice parameter search distance (# is 0..32; above 2 doesn't help much)\n");
	fprintf(out, "  -V   : verify a correct encoding by decoding the output in parallel and\n");
	fprintf(out, "         comparing to the original\n");
	fprintf(out, "  -S-, -m-, -M-, -e-, -E-, -p-, -V-, --delete-input-file-,%s --lax-, --sector-align-\n",
#ifdef FLAC__HAS_OGG
		" --ogg-,"
#else
		""
#endif
	);
	fprintf(out, "  can all be used to turn off a particular option\n");
	fprintf(out, "format options:\n");
	fprintf(out, "  -fb | -fl : big-endian | little-endian byte order\n");
	fprintf(out, "  -fc channels\n");
	fprintf(out, "  -fp bits_per_sample\n");
	fprintf(out, "  -fs sample_rate : in Hz\n");
	fprintf(out, "  -fu : unsigned samples (default is signed)\n");
	fprintf(out, "  -fr : force input to be treated as raw samples\n");

	return message? 1 : 0;
}

int encode_file(const char *infilename, const char *forced_outfilename, FLAC__bool is_last_file)
{
	FILE *encode_infile;
	char outfilename[4096]; /* @@@ bad MAGIC NUMBER */
	char *p;
	FLAC__byte lookahead[12];
	unsigned lookahead_length = 0;
	FLAC__bool treat_as_wave = false;
	int retval;
	long infilesize;
	encode_options_t common_options;

	if(0 == strcmp(infilename, "-")) {
		infilesize = -1;
		encode_infile = stdin;
	}
	else {
		infilesize = flac__file_get_filesize(infilename);
		if(0 == (encode_infile = fopen(infilename, "rb"))) {
			fprintf(stderr, "ERROR: can't open input file %s\n", infilename);
			return 1;
		}
	}

	if(!force_raw_format) {
		/* first set format based on name */
		if(0 == strcasecmp(infilename+(strlen(infilename)-4), ".wav"))
			treat_as_wave = true;
		else
			treat_as_wave = false;

		/* attempt to guess the file type based on the first 12 bytes */
		if((lookahead_length = fread(lookahead, 1, 12, encode_infile)) < 12) {
			if(treat_as_wave)
				fprintf(stderr, "WARNING: %s is not a WAVE file, treating as a raw file\n", infilename);
			treat_as_wave = false;
		}
		else {
			if(strncmp(lookahead, "RIFF", 4) || strncmp(lookahead+8, "WAVE", 4)) {
				if(treat_as_wave)
					fprintf(stderr, "WARNING: %s is not a WAVE file, treating as a raw file\n", infilename);
				treat_as_wave = false;
			}
			else
				treat_as_wave = true;
		}
	}

	if(sector_align && !treat_as_wave && infilesize < 0) {
		fprintf(stderr, "ERROR: can't --sector-align when the input size is unknown\n");
		return 1;
	}

	if(!treat_as_wave) {
		if(format_is_big_endian < 0 || format_channels < 0 || format_bps < 0 || format_sample_rate < 0)
			return long_usage("ERROR: for encoding a raw file you must specify { -fb or -fl }, -fc, -fp, and -fs\n");
	}

	if(encode_infile == stdin || force_to_stdout)
		strcpy(outfilename, "-");
	else {
		const char *suffix = (use_ogg? ogg_suffix : flac_suffix);
		strcpy(outfilename, output_prefix? output_prefix : "");
		strcat(outfilename, infilename);
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
	if(0 != cmdline_forced_outfilename)
		forced_outfilename = cmdline_forced_outfilename;

	common_options.verbose = verbose;
	common_options.skip = skip;
	common_options.verify = verify;
#ifdef FLAC__HAS_OGG
	common_options.use_ogg = use_ogg;
#endif
	common_options.lax = lax;
	common_options.do_mid_side = do_mid_side;
	common_options.loose_mid_side = loose_mid_side;
	common_options.do_exhaustive_model_search = do_exhaustive_model_search;
	common_options.do_escape_coding = do_escape_coding;
	common_options.do_qlp_coeff_prec_search = do_qlp_coeff_prec_search;
	common_options.min_residual_partition_order = min_residual_partition_order;
	common_options.max_residual_partition_order = max_residual_partition_order;
	common_options.rice_parameter_search_dist = rice_parameter_search_dist;
	common_options.max_lpc_order = max_lpc_order;
	common_options.blocksize = (unsigned)blocksize;
	common_options.qlp_coeff_precision = qlp_coeff_precision;
	common_options.padding = padding;
	common_options.requested_seek_points = requested_seek_points;
	common_options.num_requested_seek_points = num_requested_seek_points;

	if(treat_as_wave) {
		wav_encode_options_t options;

		options.common = common_options;
		options.is_last_file = is_last_file;
		options.align_reservoir = align_reservoir;
		options.align_reservoir_samples = &align_reservoir_samples;
		options.sector_align = sector_align;

		retval = flac__encode_wav(encode_infile, infilesize, infilename, forced_outfilename, lookahead, lookahead_length, options);
	}
	else {
		raw_encode_options_t options;

		options.common = common_options;
		options.is_big_endian = format_is_big_endian;
		options.is_unsigned_samples = format_is_unsigned_samples;
		options.channels = format_channels;
		options.bps = format_bps;
		options.sample_rate = format_sample_rate;

		retval = flac__encode_raw(encode_infile, infilesize, infilename, forced_outfilename, lookahead, lookahead_length, options);
	}

	if(retval == 0 && strcmp(infilename, "-")) {
		if(strcmp(forced_outfilename, "-"))
			flac__file_copy_metadata(infilename, forced_outfilename);
		if(delete_input)
			unlink(infilename);
	}

	return 0;
}

int decode_file(const char *infilename, const char *forced_outfilename)
{
	static const char *suffixes[] = { ".wav", ".raw", ".ana" };
	char outfilename[4096]; /* @@@ bad MAGIC NUMBER */
	char *p;
	int retval;
	FLAC__bool treat_as_ogg = false;
	decode_options_t common_options;

	if(!test_only && !analyze) {
		if(force_raw_format && format_is_big_endian < 0)
			return long_usage("ERROR: for decoding to a raw file you must specify -fb or -fl\n");
	}

	if(0 == strcmp(infilename, "-") || force_to_stdout)
		strcpy(outfilename, "-");
	else {
		const char *suffix = suffixes[analyze? 2 : force_raw_format? 1 : 0];
		strcpy(outfilename, output_prefix? output_prefix : "");
		strcat(outfilename, infilename);
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
	if(0 != cmdline_forced_outfilename)
		forced_outfilename = cmdline_forced_outfilename;

	if(use_ogg)
		treat_as_ogg = true;
	else if(0 == strcasecmp(infilename+(strlen(infilename)-4), ".ogg"))
		treat_as_ogg = true;
	else
		treat_as_ogg = false;

#ifndef FLAC__HAS_OGG
	if(treat_as_ogg) {
		fprintf(stderr, "%s: Ogg support has not been built into this copy of flac\n", infilename);
		return 1;
	}
#endif

	common_options.verbose = verbose;
#ifdef FLAC__HAS_OGG
	common_options.is_ogg = treat_as_ogg;
#endif
	common_options.skip = skip;

	if(!force_raw_format) {
		wav_decode_options_t options;

		options.common = common_options;

		retval = flac__decode_wav(infilename, test_only? 0 : forced_outfilename, analyze, aopts, options);
	}
	else {
		raw_decode_options_t options;

		options.common = common_options;
		options.is_big_endian = format_is_big_endian;
		options.is_unsigned_samples = format_is_unsigned_samples;

		retval = flac__decode_raw(infilename, test_only? 0 : forced_outfilename, analyze, aopts, options);
	}

	if(retval == 0 && strcmp(infilename, "-")) {
		if(strcmp(forced_outfilename, "-"))
			flac__file_copy_metadata(infilename, forced_outfilename);
		if(delete_input)
			unlink(infilename);
	}

	return 0;
}
