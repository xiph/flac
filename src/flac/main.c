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
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if !defined _MSC_VER && !defined __MINGW32__
/* unlink is in stdio.h in VC++ */
#include <unistd.h> /* for unlink() */
#else
#define strcasecmp stricmp
#endif
#include "FLAC/all.h"
#include "share/grabbag.h"
#include "analyze.h"
#include "decode.h"
#include "encode.h"
#include "utils.h"
#include "vorbiscomment.h"

#if 0
/*[JEC] was:#if HAVE_GETOPT_LONG*/
/*[JEC] see flac/include/share/getopt.h as to why the change */
#  include <getopt.h>
#else
#  include "share/getopt.h"
#endif

typedef enum { RAW, WAV, AIF } FileFormat;

static int do_it();

static FLAC__bool init_options();
static int parse_options(int argc, char *argv[]);
static int parse_option(int short_option, const char *long_option, const char *option_argument);
static void free_options();

static int usage_error(const char *message, ...);
static void short_usage();
static void show_version();
static void show_help();
static void show_explain();
static void format_mistake(const char *infilename, const char *wrong, const char *right);

static int encode_file(const char *infilename, FLAC__bool is_first_file, FLAC__bool is_last_file);
static int decode_file(const char *infilename);

static const char *get_encoded_outfilename(const char *infilename);
static const char *get_decoded_outfilename(const char *infilename);

static void die(const char *message);
static char *local_strdup(const char *source);


/*
 * share__getopt format struct; note that for long options with no
 * short option equivalent we just set the 'val' field to 0.
 */
static struct share__option long_options_[] = {
	/*
	 * general options
	 */
	{ "help", 0, 0, 'h' },
	{ "explain", 0, 0, 'H' },
	{ "version", 0, 0, 'v' },
	{ "decode", 0, 0, 'd' },
	{ "analyze", 0, 0, 'a' },
	{ "test", 0, 0, 't' },
	{ "stdout", 0, 0, 'c' },
	{ "silent", 0, 0, 's' },
	{ "delete-input-file", 0, 0, 0 },
	{ "output-prefix", 1, 0, 0 },
	{ "output-name", 1, 0, 'o' },
	{ "skip", 1, 0, 0 },
	{ "until", 1, 0, 0 },

	/*
	 * decoding options
	 */
	{ "decode-through-errors", 0, 0, 'F' },

	/*
	 * encoding options
	 */
	{ "cuesheet", 1, 0, 0 },
	{ "no-cued-seekpoints", 0, 0, 0 },
	{ "tag", 1, 0, 'T' },
	{ "compression-level-0", 0, 0, '0' },
	{ "compression-level-1", 0, 0, '1' },
	{ "compression-level-2", 0, 0, '2' },
	{ "compression-level-3", 0, 0, '3' },
	{ "compression-level-4", 0, 0, '4' },
	{ "compression-level-5", 0, 0, '5' },
	{ "compression-level-6", 0, 0, '6' },
	{ "compression-level-7", 0, 0, '7' },
	{ "compression-level-8", 0, 0, '8' },
	{ "compression-level-9", 0, 0, '9' },
	{ "best", 0, 0, '8' },
	{ "fast", 0, 0, '0' },
	{ "super-secret-totally-impractical-compression-level", 0, 0, 0 },
	{ "verify", 0, 0, 'V' },
	{ "force-aiff-format", 0, 0, 0 },
	{ "force-raw-format", 0, 0, 0 },
	{ "lax", 0, 0, 0 },
	{ "replay-gain", 0, 0, 0 },
	{ "sector-align", 0, 0, 0 },
	{ "seekpoint", 1, 0, 'S' },
	{ "padding", 1, 0, 'P' },
#ifdef FLAC__HAS_OGG
	{ "ogg", 0, 0, 0 },
	{ "serial-number", 1, 0, 0 },
#endif
	{ "blocksize", 1, 0, 'b' },
	{ "exhaustive-model-search", 0, 0, 'e' },
#if 0
	/* @@@ deprecated: */
	{ "escape-coding", 0, 0, 'E' },
#endif
	{ "max-lpc-order", 1, 0, 'l' },
	{ "mid-side", 0, 0, 'm' },
	{ "adaptive-mid-side", 0, 0, 'M' },
	{ "qlp-coeff-precision-search", 0, 0, 'p' },
	{ "qlp-coeff-precision", 1, 0, 'q' },
	{ "rice-partition-order", 1, 0, 'r' },
#if 0
	/* @@@ deprecated: */
	{ "rice-parameter-search-distance", 1, 0, 'R' },
#endif
	{ "endian", 1, 0, 0 },
	{ "channels", 1, 0, 0 },
	{ "bps", 1, 0, 0 },
	{ "sample-rate", 1, 0, 0 },
	{ "sign", 1, 0, 0 },

	/*
	 * analysis options
	 */
	{ "residual-gnu-plot", 0, 0, 0 },
	{ "residual-text", 0, 0, 0 },

	/*
	 * negatives
	 */
	{ "no-decode-through-errors", 0, 0, 0 },
	{ "no-silent", 0, 0, 0 },
	{ "no-seektable", 0, 0, 0 },
	{ "no-delete-input-file", 0, 0, 0 },
	{ "no-replay-gain", 0, 0, 0 },
	{ "no-sector-align", 0, 0, 0 },
	{ "no-lax", 0, 0, 0 },
#ifdef FLAC__HAS_OGG
	{ "no-ogg", 0, 0, 0 },
#endif
	{ "no-exhaustive-model-search", 0, 0, 0 },
#if 0
	/* @@@ deprecated: */
	{ "no-escape-coding", 0, 0, 0 },
#endif
	{ "no-mid-side", 0, 0, 0 },
	{ "no-adaptive-mid-side", 0, 0, 0 },
	{ "no-qlp-coeff-prec-search", 0, 0, 0 },
	{ "no-padding", 0, 0, 0 },
	{ "no-verify", 0, 0, 0 },
	{ "no-residual-gnuplot", 0, 0, 0 },
	{ "no-residual-text", 0, 0, 0 },
	/*
	 * undocumented debugging options for the test suite
	 */
	{ "disable-constant-subframes", 0, 0, 0 },
	{ "disable-fixed-subframes", 0, 0, 0 },
	{ "disable-verbatim-subframes", 0, 0, 0 },

	{0, 0, 0, 0}
};


/*
 * global to hold command-line option values
 */

static struct {
	FLAC__bool show_help;
	FLAC__bool show_explain;
	FLAC__bool show_version;
	FLAC__bool mode_decode;
	FLAC__bool verify;
	FLAC__bool verbose;
	FLAC__bool continue_through_decode_errors;
	FLAC__bool lax;
	FLAC__bool test_only;
	FLAC__bool analyze;
	FLAC__bool use_ogg;
	FLAC__bool has_serial_number; /* true iff --serial-number was used */
	long serial_number; /* this is the Ogg serial number and is unused for native FLAC */
	FLAC__bool do_mid_side;
	FLAC__bool loose_mid_side;
	FLAC__bool do_exhaustive_model_search;
	FLAC__bool do_escape_coding;
	FLAC__bool do_qlp_coeff_prec_search;
	FLAC__bool force_to_stdout;
	FLAC__bool force_aiff_format;
	FLAC__bool force_raw_format;
	FLAC__bool delete_input;
	FLAC__bool replay_gain;
	FLAC__bool sector_align;
	const char *cmdline_forced_outfilename;
	const char *output_prefix;
	analysis_options aopts;
	int padding;
	unsigned max_lpc_order;
	unsigned qlp_coeff_precision;
	const char *skip_specification;
	const char *until_specification;
	int format_is_big_endian;
	int format_is_unsigned_samples;
	int format_channels;
	int format_bps;
	int format_sample_rate;
	int blocksize;
	int min_residual_partition_order;
	int max_residual_partition_order;
	int rice_parameter_search_dist;
	char requested_seek_points[50000]; /* @@@ bad MAGIC NUMBER but buffer overflow is checked */
	int num_requested_seek_points; /* -1 => no -S options were given, 0 => -S- was given */
	const char *cuesheet_filename;
	FLAC__bool cued_seekpoints;

	unsigned num_files;
	char **filenames;

	FLAC__StreamMetadata *vorbis_comment;

	struct {
		FLAC__bool disable_constant_subframes;
		FLAC__bool disable_fixed_subframes;
		FLAC__bool disable_verbatim_subframes;
	} debug;
} option_values;


/*
 * miscellaneous globals
 */

static FLAC__int32 align_reservoir_0[588], align_reservoir_1[588]; /* for carrying over samples from --sector-align */
static FLAC__int32 *align_reservoir[2] = { align_reservoir_0, align_reservoir_1 };
static unsigned align_reservoir_samples = 0; /* 0 .. 587 */


int main(int argc, char *argv[])
{
	int retval = 0;

	setlocale(LC_ALL, "");
	if(!init_options()) {
		fprintf(stderr, "ERROR: allocating memory\n");
		retval = 1;
	}
	else {
		if((retval = parse_options(argc, argv)) == 0)
			retval = do_it();
	}

	free_options();

	return retval;
}

int do_it()
{
	int retval = 0;

	if(option_values.show_version) {
		show_version();
		return 0;
	}
	else if(option_values.show_explain) {
		show_explain();
		return 0;
	}
	else if(option_values.show_help) {
		show_help();
		return 0;
	}
	else {
		if(option_values.num_files == 0) {
			short_usage();
			return 0;
		}

		/*
		 * tweak options; validate the values
		 */
		if(!option_values.mode_decode) {
			if(option_values.blocksize < 0) {
				if(option_values.max_lpc_order == 0)
					option_values.blocksize = 1152;
				else
					option_values.blocksize = 4608;
			}
			if(option_values.max_residual_partition_order < 0) {
				if(option_values.blocksize <= 1152)
					option_values.max_residual_partition_order = 2;
				else if(option_values.blocksize <= 2304)
					option_values.max_residual_partition_order = 3;
				else if(option_values.blocksize <= 4608)
					option_values.max_residual_partition_order = 3;
				else
					option_values.max_residual_partition_order = 4;
				option_values.min_residual_partition_order = option_values.max_residual_partition_order;
			}
			if(option_values.rice_parameter_search_dist < 0) {
				option_values.rice_parameter_search_dist = 0;
			}
		}
		else {
			if(option_values.test_only) {
				if(0 != option_values.skip_specification)
					return usage_error("ERROR: --skip is not allowed in test mode\n");
				if(0 != option_values.until_specification)
					return usage_error("ERROR: --until is not allowed in test mode\n");
			}
		}

		FLAC__ASSERT(option_values.blocksize >= 0 || option_values.mode_decode);

		if(option_values.format_channels >= 0) {
			if(option_values.format_channels == 0 || (unsigned)option_values.format_channels > FLAC__MAX_CHANNELS)
				return usage_error("ERROR: invalid number of channels '%u', must be > 0 and <= %u\n", option_values.format_channels, FLAC__MAX_CHANNELS);
		}
		if(option_values.format_bps >= 0) {
			if(option_values.format_bps != 8 && option_values.format_bps != 16 && option_values.format_bps != 24)
				return usage_error("ERROR: invalid bits per sample '%u' (must be 8/16/24)\n", option_values.format_bps);
		}
		if(option_values.format_sample_rate >= 0) {
			if(!FLAC__format_sample_rate_is_valid(option_values.format_sample_rate))
				return usage_error("ERROR: invalid sample rate '%u', must be > 0 and <= %u\n", option_values.format_sample_rate, FLAC__MAX_SAMPLE_RATE);
		}
		if(option_values.force_raw_format && option_values.force_aiff_format)
			return usage_error("ERROR: only one of --force-raw-format and --force-aiff-format allowed\n");
		if(option_values.mode_decode) {
			if(!option_values.force_raw_format) {
				if(option_values.format_is_big_endian >= 0)
					return usage_error("ERROR: --endian only allowed with --force-raw-format\n");
				if(option_values.format_is_unsigned_samples >= 0)
					return usage_error("ERROR: --sign only allowed with --force-raw-format\n");
			}
			if(option_values.format_channels >= 0)
				return usage_error("ERROR: --channels not allowed with --decode\n");
			if(option_values.format_bps >= 0)
				return usage_error("ERROR: --bps not allowed with --decode\n");
			if(option_values.format_sample_rate >= 0)
				return usage_error("ERROR: --sample-rate not allowed with --decode\n");
		}
		if(!option_values.mode_decode && ((unsigned)option_values.blocksize < FLAC__MIN_BLOCK_SIZE || (unsigned)option_values.blocksize > FLAC__MAX_BLOCK_SIZE)) {
			return usage_error("ERROR: invalid blocksize '%u', must be >= %u and <= %u\n", (unsigned)option_values.blocksize, FLAC__MIN_BLOCK_SIZE, FLAC__MAX_BLOCK_SIZE);
		}
		if(option_values.qlp_coeff_precision > 0 && option_values.qlp_coeff_precision < FLAC__MIN_QLP_COEFF_PRECISION) {
			return usage_error("ERROR: invalid value '%u' for qlp coeff precision, must be 0 or >= %u\n", option_values.qlp_coeff_precision, FLAC__MIN_QLP_COEFF_PRECISION);
		}

		if(option_values.sector_align) {
			if(option_values.mode_decode)
				return usage_error("ERROR: --sector-align only allowed for encoding\n");
			if(0 != option_values.skip_specification)
				return usage_error("ERROR: --sector-align not allowed with --skip\n");
			if(0 != option_values.until_specification)
				return usage_error("ERROR: --sector-align not allowed with --until\n");
			if(option_values.format_channels >= 0 && option_values.format_channels != 2)
				return usage_error("ERROR: --sector-align can only be done with stereo input\n");
			if(option_values.format_bps >= 0 && option_values.format_bps != 16)
				return usage_error("ERROR: --sector-align can only be done with 16-bit samples\n");
			if(option_values.format_sample_rate >= 0 && option_values.format_sample_rate != 44100)
				return usage_error("ERROR: --sector-align can only be done with a sample rate of 44100\n");
		}
		if(option_values.replay_gain) {
			if(option_values.force_to_stdout)
				return usage_error("ERROR: --replay-gain not allowed with -c/--stdout\n");
			if(option_values.mode_decode)
				return usage_error("ERROR: --replay-gain only allowed for encoding\n");
			if(option_values.format_channels > 2)
				return usage_error("ERROR: --replay-gain can only be done with mono/stereo input\n");
			if(option_values.format_sample_rate >= 0 && !grabbag__replaygain_is_valid_sample_frequency(option_values.format_sample_rate))
				return usage_error("ERROR: invalid sample rate used with --replay-gain\n");
			/*
			 * We want to reserve padding space for the ReplayGain
			 * tags that we will set later, to avoid rewriting the
			 * whole file.
			 */
			if(option_values.padding < 0) {
				fprintf(stderr, "NOTE: --replay-gain may leave a small PADDING block even with --no-padding\n");
				option_values.padding = GRABBAG__REPLAYGAIN_MAX_TAG_SPACE_REQUIRED;
			}
			else {
				option_values.padding += GRABBAG__REPLAYGAIN_MAX_TAG_SPACE_REQUIRED;
			}
		}
		if(option_values.num_files > 1 && option_values.cmdline_forced_outfilename) {
			return usage_error("ERROR: -o/--output-name cannot be used with multiple files\n");
		}
		if(option_values.cmdline_forced_outfilename && option_values.output_prefix) {
			return usage_error("ERROR: --output-prefix conflicts with -o/--output-name\n");
		}
		if(!option_values.mode_decode && 0 != option_values.cuesheet_filename && option_values.num_files > 1) {
			return usage_error("ERROR: --cuesheet cannot be used when encoding multiple files\n");
		}
	}
	if(option_values.verbose) {
		fprintf(stderr, "\n");
		fprintf(stderr, "flac %s, Copyright (C) 2000,2001,2002 Josh Coalson\n", FLAC__VERSION_STRING);
		fprintf(stderr, "flac comes with ABSOLUTELY NO WARRANTY.  This is free software, and you are\n");
		fprintf(stderr, "welcome to redistribute it under certain conditions.  Type `flac' for details.\n\n");

		if(!option_values.mode_decode) {
			char padopt[16];
			if(option_values.padding < 0)
				strcpy(padopt, "-");
			else
				sprintf(padopt, " %d", option_values.padding);
			fprintf(stderr,
				"options:%s%s%s%s -P%s -b %u%s -l %u%s%s%s -q %u -r %u,%u%s\n",
				option_values.delete_input?" --delete-input-file":"",
				option_values.sector_align?" --sector-align":"",
				option_values.use_ogg?" --ogg":"",
				option_values.lax?" --lax":"",
				padopt,
				(unsigned)option_values.blocksize,
				option_values.loose_mid_side?" -M":option_values.do_mid_side?" -m":"",
				option_values.max_lpc_order,
				option_values.do_exhaustive_model_search?" -e":"",
				option_values.do_escape_coding?" -E":"",
				option_values.do_qlp_coeff_prec_search?" -p":"",
				option_values.qlp_coeff_precision,
				(unsigned)option_values.min_residual_partition_order,
				(unsigned)option_values.max_residual_partition_order,
				option_values.verify? " -V":""
			);
		}
	}

	if(option_values.mode_decode) {
		FLAC__bool first = true;

		if(option_values.num_files == 0) {
			retval = decode_file("-");
		}
		else {
			unsigned i;
			if(option_values.num_files > 1)
				option_values.cmdline_forced_outfilename = 0;
			for(i = 0, retval = 0; i < option_values.num_files; i++) {
				if(0 == strcmp(option_values.filenames[i], "-") && !first)
					continue;
				retval |= decode_file(option_values.filenames[i]);
				first = false;
			}
		}
	}
	else { /* encode */
		FLAC__bool first = true;

		if(option_values.num_files == 0) {
			retval = encode_file("-", first, true);
		}
		else {
			unsigned i;
			if(option_values.num_files > 1)
				option_values.cmdline_forced_outfilename = 0;
			for(i = 0, retval = 0; i < option_values.num_files; i++) {
				if(0 == strcmp(option_values.filenames[i], "-") && !first)
					continue;
				retval |= encode_file(option_values.filenames[i], first, i == (option_values.num_files-1));
				first = false;
			}
			if(option_values.replay_gain && retval == 0) {
				float album_gain, album_peak;
				grabbag__replaygain_get_album(&album_gain, &album_peak);
				for(i = 0; i < option_values.num_files; i++) {
					const char *error, *outfilename = get_encoded_outfilename(option_values.filenames[i]);
					if(0 == strcmp(option_values.filenames[i], "-"))
						FLAC__ASSERT(0);
					if(0 != (error = grabbag__replaygain_store_to_file_album(outfilename, album_gain, album_peak, /*preserve_modtime=*/true))) {
						fprintf(stderr, "%s: ERROR writing ReplayGain album tags\n", outfilename);
						retval = 1;
					}
				}
			}
		}
	}

	return retval;
}

FLAC__bool init_options()
{
	option_values.show_help = false;
	option_values.show_explain = false;
	option_values.mode_decode = false;
	option_values.verify = false;
	option_values.verbose = true;
	option_values.continue_through_decode_errors = false;
	option_values.lax = false;
	option_values.test_only = false;
	option_values.analyze = false;
	option_values.use_ogg = false;
	option_values.has_serial_number = false;
	option_values.serial_number = 0;
	option_values.do_mid_side = true;
	option_values.loose_mid_side = false;
	option_values.do_exhaustive_model_search = false;
	option_values.do_escape_coding = false;
	option_values.do_qlp_coeff_prec_search = false;
	option_values.force_to_stdout = false;
	option_values.force_aiff_format = false;
	option_values.force_raw_format = false;
	option_values.delete_input = false;
	option_values.replay_gain = false;
	option_values.sector_align = false;
	option_values.cmdline_forced_outfilename = 0;
	option_values.output_prefix = 0;
	option_values.aopts.do_residual_text = false;
	option_values.aopts.do_residual_gnuplot = false;
	option_values.padding = 4096;
	option_values.max_lpc_order = 8;
	option_values.qlp_coeff_precision = 0;
	option_values.skip_specification = 0;
	option_values.until_specification = 0;
	option_values.format_is_big_endian = -1;
	option_values.format_is_unsigned_samples = -1;
	option_values.format_channels = -1;
	option_values.format_bps = -1;
	option_values.format_sample_rate = -1;
	option_values.blocksize = -1;
	option_values.min_residual_partition_order = -1;
	option_values.max_residual_partition_order = -1;
	option_values.rice_parameter_search_dist = -1;
	option_values.requested_seek_points[0] = '\0';
	option_values.num_requested_seek_points = -1;
	option_values.cuesheet_filename = 0;
	option_values.cued_seekpoints = true;

	option_values.num_files = 0;
	option_values.filenames = 0;

	if(0 == (option_values.vorbis_comment = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)))
		return false;

	option_values.debug.disable_constant_subframes = false;
	option_values.debug.disable_fixed_subframes = false;
	option_values.debug.disable_verbatim_subframes = false;

	return true;
}

int parse_options(int argc, char *argv[])
{
	int short_option;
	int option_index = 1;
	FLAC__bool had_error = false;
	/*@@@ E and R: are deprecated */
	const char *short_opts = "0123456789ab:cdeFhHl:mMo:pP:q:r:sS:tT:vV";

	while ((short_option = share__getopt_long(argc, argv, short_opts, long_options_, &option_index)) != -1) {
		switch (short_option) {
			case 0: /* long option with no equivalent short option */
				had_error |= (parse_option(short_option, long_options_[option_index].name, share__optarg) != 0);
				break;
			case '?':
			case ':':
				had_error = true;
				break;
			default: /* short option */
				had_error |= (parse_option(short_option, 0, share__optarg) != 0);
				break;
		}
	}

	if(had_error) {
		return 1;
	}

	FLAC__ASSERT(share__optind <= argc);

	option_values.num_files = argc - share__optind;

	if(option_values.num_files > 0) {
		unsigned i = 0;
		if(0 == (option_values.filenames = malloc(sizeof(char *) * option_values.num_files)))
			die("out of memory allocating space for file names list");
		while(share__optind < argc)
			option_values.filenames[i++] = local_strdup(argv[share__optind++]);
	}

	return 0;
}

int parse_option(int short_option, const char *long_option, const char *option_argument)
{
	char *p;

	if(short_option == 0) {
		FLAC__ASSERT(0 != long_option);
		if(0 == strcmp(long_option, "delete-input-file")) {
			option_values.delete_input = true;
		}
		else if(0 == strcmp(long_option, "output-prefix")) {
			FLAC__ASSERT(0 != option_argument);
			option_values.output_prefix = option_argument;
		}
		else if(0 == strcmp(long_option, "skip")) {
			FLAC__ASSERT(0 != option_argument);
			option_values.skip_specification = option_argument;
		}
		else if(0 == strcmp(long_option, "until")) {
			FLAC__ASSERT(0 != option_argument);
			option_values.until_specification = option_argument;
		}
		else if(0 == strcmp(long_option, "cuesheet")) {
			FLAC__ASSERT(0 != option_argument);
			option_values.cuesheet_filename = option_argument;
		}
		else if(0 == strcmp(long_option, "no-cued-seekpoints")) {
			option_values.cued_seekpoints = false;
		}
		else if(0 == strcmp(long_option, "super-secret-totally-impractical-compression-level")) {
			option_values.lax = true;
			option_values.do_exhaustive_model_search = true;
			option_values.do_escape_coding = true;
			option_values.do_mid_side = true;
			option_values.loose_mid_side = false;
			option_values.do_qlp_coeff_prec_search = true;
			option_values.min_residual_partition_order = 0;
			option_values.max_residual_partition_order = 16;
			option_values.rice_parameter_search_dist = 0;
			option_values.max_lpc_order = 32;
		}
		else if(0 == strcmp(long_option, "force-aiff-format")) {
			option_values.force_aiff_format = true;
		}
		else if(0 == strcmp(long_option, "force-raw-format")) {
			option_values.force_raw_format = true;
		}
		else if(0 == strcmp(long_option, "lax")) {
			option_values.lax = true;
		}
		else if(0 == strcmp(long_option, "replay-gain")) {
			option_values.replay_gain = true;
		}
		else if(0 == strcmp(long_option, "sector-align")) {
			option_values.sector_align = true;
		}
#ifdef FLAC__HAS_OGG
		else if(0 == strcmp(long_option, "ogg")) {
			option_values.use_ogg = true;
		}
		else if(0 == strcmp(long_option, "serial-number")) {
			option_values.has_serial_number = true;
			option_values.serial_number = atol(option_argument);
		}
#endif
		else if(0 == strcmp(long_option, "endian")) {
			FLAC__ASSERT(0 != option_argument);
			if(0 == strncmp(option_argument, "big", strlen(option_argument)))
				option_values.format_is_big_endian = true;
			else if(0 == strncmp(option_argument, "little", strlen(option_argument)))
				option_values.format_is_big_endian = false;
			else
				return usage_error("ERROR: argument to --endian must be \"big\" or \"little\"\n");
		}
		else if(0 == strcmp(long_option, "channels")) {
			FLAC__ASSERT(0 != option_argument);
			option_values.format_channels = atoi(option_argument);
		}
		else if(0 == strcmp(long_option, "bps")) {
			FLAC__ASSERT(0 != option_argument);
			option_values.format_bps = atoi(option_argument);
		}
		else if(0 == strcmp(long_option, "sample-rate")) {
			FLAC__ASSERT(0 != option_argument);
			option_values.format_sample_rate = atoi(option_argument);
		}
		else if(0 == strcmp(long_option, "sign")) {
			FLAC__ASSERT(0 != option_argument);
			if(0 == strncmp(option_argument, "signed", strlen(option_argument)))
				option_values.format_is_unsigned_samples = false;
			else if(0 == strncmp(option_argument, "unsigned", strlen(option_argument)))
				option_values.format_is_unsigned_samples = true;
			else
				return usage_error("ERROR: argument to --sign must be \"signed\" or \"unsigned\"\n");
		}
		else if(0 == strcmp(long_option, "residual-gnu-plot")) {
			option_values.aopts.do_residual_gnuplot = true;
		}
		else if(0 == strcmp(long_option, "residual-text")) {
			option_values.aopts.do_residual_text = true;
		}
		/*
		 * negatives
		 */
		else if(0 == strcmp(long_option, "no-decode-through-errors")) {
			option_values.continue_through_decode_errors = false;
		}
		else if(0 == strcmp(long_option, "no-silent")) {
			option_values.verbose = true;
		}
		else if(0 == strcmp(long_option, "no-seektable")) {
			option_values.num_requested_seek_points = 0;
			option_values.requested_seek_points[0] = '\0';
		}
		else if(0 == strcmp(long_option, "no-delete-input-file")) {
			option_values.delete_input = false;
		}
		else if(0 == strcmp(long_option, "no-replay-gain")) {
			option_values.replay_gain = false;
		}
		else if(0 == strcmp(long_option, "no-sector-align")) {
			option_values.sector_align = false;
		}
		else if(0 == strcmp(long_option, "no-lax")) {
			option_values.lax = false;
		}
#ifdef FLAC__HAS_OGG
		else if(0 == strcmp(long_option, "no-ogg")) {
			option_values.use_ogg = false;
		}
#endif
		else if(0 == strcmp(long_option, "no-exhaustive-model-search")) {
			option_values.do_exhaustive_model_search = false;
		}
#if 0
		/* @@@ deprecated: */
		else if(0 == strcmp(long_option, "no-escape-coding")) {
			option_values.do_escape_coding = false;
		}
#endif
		else if(0 == strcmp(long_option, "no-mid-side")) {
			option_values.do_mid_side = option_values.loose_mid_side = false;
		}
		else if(0 == strcmp(long_option, "no-adaptive-mid-side")) {
			option_values.loose_mid_side = option_values.do_mid_side = false;
		}
		else if(0 == strcmp(long_option, "no-qlp-coeff-prec-search")) {
			option_values.do_qlp_coeff_prec_search = false;
		}
		else if(0 == strcmp(long_option, "no-padding")) {
			option_values.padding = -1;
		}
		else if(0 == strcmp(long_option, "no-verify")) {
			option_values.verify = false;
		}
		else if(0 == strcmp(long_option, "no-residual-gnuplot")) {
			option_values.aopts.do_residual_gnuplot = false;
		}
		else if(0 == strcmp(long_option, "no-residual-text")) {
			option_values.aopts.do_residual_text = false;
		}
		else if(0 == strcmp(long_option, "disable-constant-subframes")) {
			option_values.debug.disable_constant_subframes = true;
		}
		else if(0 == strcmp(long_option, "disable-fixed-subframes")) {
			option_values.debug.disable_fixed_subframes = true;
		}
		else if(0 == strcmp(long_option, "disable-verbatim-subframes")) {
			option_values.debug.disable_verbatim_subframes = true;
		}
	}
	else {
		const char *violation;
		switch(short_option) {
			case 'h':
				option_values.show_help = true;
				break;
			case 'H':
				option_values.show_explain = true;
				break;
			case 'v':
				option_values.show_version = true;
				break;
			case 'd':
				option_values.mode_decode = true;
				break;
			case 'a':
				option_values.mode_decode = true;
				option_values.analyze = true;
				break;
			case 't':
				option_values.mode_decode = true;
				option_values.test_only = true;
				break;
			case 'c':
				option_values.force_to_stdout = true;
				break;
			case 's':
				option_values.verbose = false;
				break;
			case 'o':
				FLAC__ASSERT(0 != option_argument);
				option_values.cmdline_forced_outfilename = option_argument;
				break;
			case 'F':
				option_values.continue_through_decode_errors = true;
				break;
			case 'T':
				FLAC__ASSERT(0 != option_argument);
				if(!flac__vorbiscomment_add(option_values.vorbis_comment, option_argument, &violation))
					return usage_error("ERROR: (-T/--tag) %s\n", violation);
				break;
			case '0':
				option_values.do_exhaustive_model_search = false;
				option_values.do_escape_coding = false;
				option_values.do_mid_side = false;
				option_values.loose_mid_side = false;
				option_values.qlp_coeff_precision = 0;
				option_values.min_residual_partition_order = option_values.max_residual_partition_order = 2;
				option_values.rice_parameter_search_dist = 0;
				option_values.max_lpc_order = 0;
				break;
			case '1':
				option_values.do_exhaustive_model_search = false;
				option_values.do_escape_coding = false;
				option_values.do_mid_side = true;
				option_values.loose_mid_side = true;
				option_values.qlp_coeff_precision = 0;
				option_values.min_residual_partition_order = option_values.max_residual_partition_order = 2;
				option_values.rice_parameter_search_dist = 0;
				option_values.max_lpc_order = 0;
				break;
			case '2':
				option_values.do_exhaustive_model_search = false;
				option_values.do_escape_coding = false;
				option_values.do_mid_side = true;
				option_values.loose_mid_side = false;
				option_values.qlp_coeff_precision = 0;
				option_values.min_residual_partition_order = 0;
				option_values.max_residual_partition_order = 3;
				option_values.rice_parameter_search_dist = 0;
				option_values.max_lpc_order = 0;
				break;
			case '3':
				option_values.do_exhaustive_model_search = false;
				option_values.do_escape_coding = false;
				option_values.do_mid_side = false;
				option_values.loose_mid_side = false;
				option_values.qlp_coeff_precision = 0;
				option_values.min_residual_partition_order = option_values.max_residual_partition_order = 3;
				option_values.rice_parameter_search_dist = 0;
				option_values.max_lpc_order = 6;
				break;
			case '4':
				option_values.do_exhaustive_model_search = false;
				option_values.do_escape_coding = false;
				option_values.do_mid_side = true;
				option_values.loose_mid_side = true;
				option_values.qlp_coeff_precision = 0;
				option_values.min_residual_partition_order = option_values.max_residual_partition_order = 3;
				option_values.rice_parameter_search_dist = 0;
				option_values.max_lpc_order = 8;
				break;
			case '5':
				option_values.do_exhaustive_model_search = false;
				option_values.do_escape_coding = false;
				option_values.do_mid_side = true;
				option_values.loose_mid_side = false;
				option_values.qlp_coeff_precision = 0;
				option_values.min_residual_partition_order = option_values.max_residual_partition_order = 3;
				option_values.rice_parameter_search_dist = 0;
				option_values.max_lpc_order = 8;
				break;
			case '6':
				option_values.do_exhaustive_model_search = false;
				option_values.do_escape_coding = false;
				option_values.do_mid_side = true;
				option_values.loose_mid_side = false;
				option_values.qlp_coeff_precision = 0;
				option_values.min_residual_partition_order = 0;
				option_values.max_residual_partition_order = 4;
				option_values.rice_parameter_search_dist = 0;
				option_values.max_lpc_order = 8;
				break;
			case '7':
				option_values.do_exhaustive_model_search = true;
				option_values.do_escape_coding = false;
				option_values.do_mid_side = true;
				option_values.loose_mid_side = false;
				option_values.qlp_coeff_precision = 0;
				option_values.min_residual_partition_order = 0;
				option_values.max_residual_partition_order = 6;
				option_values.rice_parameter_search_dist = 0;
				option_values.max_lpc_order = 8;
				break;
			case '8':
				option_values.do_exhaustive_model_search = true;
				option_values.do_escape_coding = false;
				option_values.do_mid_side = true;
				option_values.loose_mid_side = false;
				option_values.qlp_coeff_precision = 0;
				option_values.min_residual_partition_order = 0;
				option_values.max_residual_partition_order = 6;
				option_values.rice_parameter_search_dist = 0;
				option_values.max_lpc_order = 12;
				break;
			case '9':
				return usage_error("ERROR: compression level '9' is reserved\n");
			case 'V':
				option_values.verify = true;
				break;
			case 'S':
				FLAC__ASSERT(0 != option_argument);
				if(option_values.num_requested_seek_points < 0)
					option_values.num_requested_seek_points = 0;
				option_values.num_requested_seek_points++;
				if(strlen(option_values.requested_seek_points)+strlen(option_argument)+2 >= sizeof(option_values.requested_seek_points)) {
					return usage_error("ERROR: too many seekpoints requested\n");
				}
				else {
					strcat(option_values.requested_seek_points, option_argument);
					strcat(option_values.requested_seek_points, ";");
				}
				break;
			case 'P':
				FLAC__ASSERT(0 != option_argument);
				option_values.padding = atoi(option_argument);
				if(option_values.padding < 0)
					return usage_error("ERROR: argument to -P must be >= 0\n");
				break;
			case 'b':
				FLAC__ASSERT(0 != option_argument);
				option_values.blocksize = atoi(option_argument);
				break;
			case 'e':
				option_values.do_exhaustive_model_search = true;
				break;
			case 'E':
				option_values.do_escape_coding = true;
				break;
			case 'l':
				FLAC__ASSERT(0 != option_argument);
				option_values.max_lpc_order = atoi(option_argument);
				break;
			case 'm':
				option_values.do_mid_side = true;
				option_values.loose_mid_side = false;
				break;
			case 'M':
				option_values.loose_mid_side = option_values.do_mid_side = true;
				break;
			case 'p':
				option_values.do_qlp_coeff_prec_search = true;
				break;
			case 'q':
				FLAC__ASSERT(0 != option_argument);
				option_values.qlp_coeff_precision = atoi(option_argument);
				break;
			case 'r':
				FLAC__ASSERT(0 != option_argument);
				p = strchr(option_argument, ',');
				if(0 == p) {
					option_values.min_residual_partition_order = 0;
					option_values.max_residual_partition_order = atoi(option_argument);
				}
				else {
					option_values.min_residual_partition_order = atoi(option_argument);
					option_values.max_residual_partition_order = atoi(++p);
				}
				break;
			case 'R':
				FLAC__ASSERT(0 != option_argument);
				option_values.rice_parameter_search_dist = atoi(option_argument);
				break;
			default:
				FLAC__ASSERT(0);
		}
	}

	return 0;
}

void free_options()
{
	unsigned i;
	if(0 != option_values.filenames) {
		for(i = 0; i < option_values.num_files; i++) {
			if(0 != option_values.filenames[i])
				free(option_values.filenames[i]);
		}
		free(option_values.filenames);
	}
	if(0 != option_values.vorbis_comment)
		FLAC__metadata_object_delete(option_values.vorbis_comment);
}

int usage_error(const char *message, ...)
{
	va_list args;

	FLAC__ASSERT(0 != message);

	va_start(args, message);

	(void) vfprintf(stderr, message, args);

	va_end(args);

	printf("Type \"flac\" for a usage summary or \"flac --help\" for all options\n");

	return 1;
}

void show_version()
{
	printf("flac %s\n", FLAC__VERSION_STRING);
}

static void usage_header()
{
	printf("===============================================================================\n");
	printf("flac - Command-line FLAC encoder/decoder version %s\n", FLAC__VERSION_STRING);
	printf("Copyright (C) 2000,2001,2002  Josh Coalson\n");
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
	printf("===============================================================================\n");
}

static void usage_summary()
{
	printf("Usage:\n");
	printf("\n");
	printf(" Encoding: flac [<general-options>] [<encoding/format-options>] [INPUTFILE [...]]\n");
	printf(" Decoding: flac -d [<general-options>] [<format-options>] [FLACFILE [...]]\n");
	printf("  Testing: flac -t [<general-options>] [FLACFILE [...]]\n");
	printf("Analyzing: flac -a [<general-options>] [<analysis-options>] [FLACFILE [...]]\n");
	printf("\n");
}

void short_usage()
{
	usage_header();
	printf("\n");
	printf("This is the short help; for all options use 'flac --help'; for even more\n");
	printf("instructions use 'flac --explain'\n");
	printf("\n");
	printf("To encode:\n");
	printf("  flac [-#] [INPUTFILE [...]]\n");
	printf("\n");
	printf("  -# is -0 (fastest compression) to -8 (highest compression); -5 is the default\n");
	printf("\n");
	printf("To decode:\n");
	printf("  flac -d [INPUTFILE [...]]\n");
	printf("\n");
	printf("To test:\n");
	printf("  flac -t [INPUTFILE [...]]\n");
}

void show_help()
{
	usage_header();
	usage_summary();
	printf("general options:\n");
	printf("  -v, --version                Show the flac version number\n");
	printf("  -h, --help                   Show this screen\n");
	printf("  -H, --explain                Show detailed explanation of usage and options\n");
	printf("  -d, --decode                 Decode (the default behavior is to encode)\n");
	printf("  -t, --test                   Same as -d except no decoded file is written\n");
	printf("  -a, --analyze                Same as -d except an analysis file is written\n");
	printf("  -c, --stdout                 Write output to stdout\n");
	printf("  -s, --silent                 Do not write runtime encode/decode statistics\n");
	printf("  -o, --output-name=FILENAME   Force the output file name\n");
	printf("      --output-prefix=STRING   Prepend STRING to output names\n");
	printf("      --delete-input-file      Deletes after a successful encode/decode\n");
	printf("      --skip={#|mm:ss.ss}      Skip the given initial samples for each input\n");
	printf("      --until={#|[+|-]mm:ss.ss}  Stop at the given sample for each input file\n");
#ifdef FLAC__HAS_OGG
	printf("      --ogg                    Use Ogg as transport layer\n");
	printf("      --serial-number          Serial number to use for the FLAC stream\n");
#endif
	printf("analysis options:\n");
	printf("      --residual-text          Include residual signal in text output\n");
	printf("      --residual-gnuplot       Generate gnuplot files of residual distribution\n");
	printf("decoding options:\n");
	printf("  -F, --decode-through-errors  Continue decoding through stream errors\n");
	printf("encoding options:\n");
	printf("  -V, --verify                 Verify a correct encoding\n");
	printf("      --lax                    Allow encoder to generate non-Subset files\n");
	printf("      --sector-align           Align multiple files on sector boundaries\n");
	printf("      --replay-gain            Calculate ReplayGain & store in Vorbis comments\n");
	printf("      --cuesheet=FILENAME      Import cuesheet and store in CUESHEET block\n");
	printf("  -T, --tag=FIELD=VALUE        Add a Vorbis comment; may appear multiple times\n");
	printf("  -S, --seekpoint={#|X|#x|#s}  Add seek point(s)\n");
	printf("  -P, --padding=#              Write a PADDING block of length #\n");
	printf("  -0, --compression-level-0, --fast  Synonymous with -l 0 -b 1152 -r 2,2\n");
	printf("  -1, --compression-level-1          Synonymous with -l 0 -b 1152 -M -r 2,2\n");
	printf("  -2, --compression-level-2          Synonymous with -l 0 -b 1152 -m -r 3\n");
	printf("  -3, --compression-level-3          Synonymous with -l 6 -b 4608 -r 3,3\n");
	printf("  -4, --compression-level-4          Synonymous with -l 8 -b 4608 -M -r 3,3\n");
	printf("  -5, --compression-level-5          Synonymous with -l 8 -b 4608 -m -r 3,3\n");
	printf("  -6, --compression-level-6          Synonymous with -l 8 -b 4608 -m -r 4\n");
	printf("  -7, --compression-level-7          Synonymous with -l 8 -b 4608 -m -e -r 6\n");
	printf("  -8, --compression-level-8, --best  Synonymous with -l 12 -b 4608 -m -e -r 6\n");
	printf("  -b, --blocksize=#                  Specify blocksize in samples\n");
	printf("  -m, --mid-side                     Try mid-side coding for each frame\n");
	printf("  -M, --adaptive-mid-side            Adaptive mid-side coding for all frames\n");
	printf("  -e, --exhaustive-model-search      Do exhaustive model search (expensive!)\n");
#if 0
	/*@@@ deprecated: */
	printf("  -E, --escape-coding                Do escape coding in the entropy coder\n");
#endif
	printf("  -l, --max-lpc-order=#              Max LPC order; 0 => only fixed predictors\n");
	printf("  -p, --qlp-coeff-precision-search   Exhaustively search LP coeff quantization\n");
	printf("  -q, --qlp-coeff-precision=#        Specify precision in bits\n");
	printf("  -r, --rice-partition-order=[#,]#   Set [min,]max residual partition order\n");
#if 0
	/*@@@ deprecated: */
	printf("  -R, -rice-parameter-search-distance=#   Rice parameter search distance\n");
#endif
	printf("format options:\n");
	printf("      --endian={big|little}    Set byte order for samples\n");
	printf("      --channels=#             Number of channels\n");
	printf("      --bps=#                  Number of bits per sample\n");
	printf("      --sample-rate=#          Sample rate in Hz\n");
	printf("      --sign={signed|unsigned} Sign of samples\n");
	printf("      --force-aiff-format      Force decoding to AIFF format\n");
	printf("      --force-raw-format       Treat input or output as raw samples\n");
	printf("negative options:\n");
	printf("      --no-adaptive-mid-side\n");
	printf("      --no-decode-through-errors\n");
	printf("      --no-delete-input-file\n");
#if 0
/* @@@ deprecated: */
	printf("      --no-escape-coding\n");
#endif
	printf("      --no-exhaustive-model-search\n");
	printf("      --no-lax\n");
	printf("      --no-mid-side\n");
#ifdef FLAC__HAS_OGG
	printf("      --no-ogg\n");
#endif
	printf("      --no-padding\n");
	printf("      --no-qlp-coeff-prec-search\n");
	printf("      --no-replay-gain\n");
	printf("      --no-residual-gnuplot\n");
	printf("      --no-residual-text\n");
	printf("      --no-sector-align\n");
	printf("      --no-seektable\n");
	printf("      --no-silent\n");
	printf("      --no-verify\n");
}

void show_explain()
{
	usage_header();
	usage_summary();
	printf("For encoding:\n");
	printf("  The input file(s) may be a PCM RIFF WAVE file, AIFF file, or raw samples.\n");
	printf("  The output file(s) will be in native FLAC or Ogg FLAC format\n");
	printf("For decoding, the reverse is true.\n");
	printf("\n");
	printf("A single INPUTFILE may be - for stdin.  No INPUTFILE implies stdin.  Use of\n");
	printf("stdin implies -c (write to stdout).  Normally you should use:\n");
	printf("   flac [options] -o outfilename  or  flac -d [options] -o outfilename\n");
	printf("instead of:\n");
	printf("   flac [options] > outfilename   or  flac -d [options] > outfilename\n");
	printf("since the former allows flac to seek backwards to write the STREAMINFO or\n");
	printf("WAVE/AIFF header contents when necessary.\n");
	printf("\n");
	printf("flac checks for the presence of a AIFF/WAVE header to decide whether or not to\n");
	printf("treat an input file as AIFF/WAVE format or raw samples.  If any input file is\n");
	printf("raw you must specify the format options {-fb|fl} -fc -fp and -fs, which will\n");
	printf("apply to all raw files.  You can force AIFF/WAVE files to be treated as raw\n");
	printf("files using -fr.\n");
	printf("\n");
	printf("general options:\n");
	printf("  -v, --version                Show the flac version number\n");
	printf("  -h, --help                   Show basic usage a list of all options\n");
	printf("  -H, --explain                Show this screen\n");
	printf("  -d, --decode                 Decode (the default behavior is to encode)\n");
	printf("  -t, --test                   Same as -d except no decoded file is written\n");
	printf("  -a, --analyze                Same as -d except an analysis file is written\n");
	printf("  -c, --stdout                 Write output to stdout\n");
	printf("  -s, --silent                 Do not write runtime encode/decode statistics\n");
	printf("  -o, --output-name=FILENAME   Force the output file name; usually flac just\n");
	printf("                               changes the extension.  May only be used when\n");
	printf("                               encoding a single file.  May not be used in\n");
	printf("                               conjunction with --output-prefix.\n");
	printf("      --output-prefix=STRING   Prefix each output file name with the given\n");
	printf("                               STRING.  This can be useful for encoding or\n");
	printf("                               decoding files to a different directory.  Make\n");
	printf("                               sure if your STRING is a path name that it ends\n");
	printf("                               with a '/' slash.\n");
	printf("      --delete-input-file      Automatically delete the input file after a\n");
	printf("                               successful encode or decode.  If there was an\n");
	printf("                               error (including a verify error) the input file\n");
	printf("                               is left intact.\n");
	printf("      --skip={#|mm:ss.ss}      Skip the first # samples of each input file; can\n");
	printf("                               be used both for encoding and decoding.  The\n");
	printf("                               alternative form mm:ss.ss can be used to specify\n");
	printf("                               minutes, seconds, and fractions of a second.\n");
	printf("      --until={#|[+|-]mm:ss.ss}  Stop at the given sample number for each input\n");
	printf("                               file.  The given sample number is not included\n");
	printf("                               in the decoded output.  The alternative form\n");
	printf("                               mm:ss.ss can be used to specify minutes,\n");
	printf("                               seconds, and fractions of a second.  If a `+'\n");
	printf("                               sign is at the beginning, the --until point is\n");
	printf("                               relative to the --skip point.  If a `-' sign is\n");
	printf("                               at the beginning, the --until point is relative\n");
	printf("                               to end of the audio.\n");
#ifdef FLAC__HAS_OGG
	printf("      --ogg                    When encoding, generate Ogg-FLAC output instead\n");
	printf("                               of native-FLAC.  Ogg-FLAC streams are FLAC\n");
	printf("                               streams wrapped in an Ogg transport layer.  The\n");
	printf("                               resulting file should have an '.ogg' extension\n");
	printf("                               and will still be decodable by flac.  When\n");
	printf("                               decoding, force the input to be treated as\n");
	printf("                               Ogg-FLAC.  This is useful when piping input\n");
	printf("                               from stdin or when the filename does not end in\n");
	printf("                               '.ogg'.\n");
	printf("      --serial-number          Serial number to use for the FLAC stream.  When\n");
	printf("                               encoding and no serial number is given, flac\n");
	printf("                               uses '0'.  When decoding and no number is\n");
	printf("                               given, flac uses the serial number of the first\n");
	printf("                               page.\n");
#endif
	printf("analysis options:\n");
	printf("      --residual-text          Include residual signal in text output.  This\n");
	printf("                               will make the file very big, much larger than\n");
	printf("                               even the decoded file.\n");
	printf("      --residual-gnuplot       Generate gnuplot files of residual distribution\n");
	printf("                               of each subframe\n");
	printf("decoding options:\n");
	printf("  -F, --decode-through-errors  By default flac stops decoding with an error\n");
	printf("                               and removes the partially decoded file if it\n");
	printf("                               encounters a bitstream error.  With -F, errors\n");
	printf("                               are still printed but flac will continue\n");
	printf("                               decoding to completion.  Note that errors may\n");
	printf("                               cause the decoded audio to be missing some\n");
	printf("                               samples or have silent sections.\n");
	printf("encoding options:\n");
	printf("  -V, --verify                 Verify a correct encoding by decoding the\n");
	printf("                               output in parallel and comparing to the\n");
	printf("                               original\n");
	printf("      --lax                    Allow encoder to generate non-Subset files\n");
	printf("      --sector-align           Align encoding of multiple CD format WAVE files\n");
	printf("                               on sector boundaries.\n");
	printf("      --replay-gain            Calculate ReplayGain values and store in Vorbis\n");
	printf("                               comments.  Title gains/peaks will be computed\n");
	printf("                               for each file, and an album gain/peak will be\n");
	printf("                               computed for all files.  All input files must\n");
	printf("                               have the same resolution, sample rate, and\n");
	printf("                               number of channels.  The sample rate must be\n");
	printf("                               one of 8, 11.025, 12, 16, 22.05, 24, 32, 44.1,\n");
	printf("                               or 48 kHz.  NOTE: this option may also leave a\n");
	printf("                               few extra bytes in the PADDING block.\n");
	printf("      --cuesheet=FILENAME      Import the given cuesheet file and store it in\n");
	printf("                               a CUESHEET metadata block.  This option may only\n");
	printf("                               be used when encoding a single file.  A\n");
	printf("                               seekpoint will be added for each index point in\n");
	printf("                               the cuesheet to the SEEKTABLE unless\n");
	printf("                               --no-cued-seekpoints is specified.\n");
	printf("  -T, --tag=FIELD=VALUE        Add a Vorbis comment.  Make sure to quote the\n");
	printf("                               comment if necessary.  This option may appear\n");
	printf("                               more than once to add several comments.  NOTE:\n");
	printf("                               all tags will be added to all encoded files.\n");
	printf("  -S, --seekpoint={#|X|#x|#s}  Include a point or points in a SEEKTABLE\n");
	printf("       #  : a specific sample number for a seek point\n");
	printf("       X  : a placeholder point (always goes at the end of the SEEKTABLE)\n");
	printf("       #x : # evenly spaced seekpoints, the first being at sample 0\n");
	printf("       #s : a seekpoint every # seconds; # does not have to be a whole number\n");
	printf("     You may use many -S options; the resulting SEEKTABLE will be the unique-\n");
	printf("           ified union of all such values.\n");
	printf("     With no -S options, flac defaults to '-S 100x'.  Use -S- for no SEEKTABLE.\n");
	printf("     Note: -S #x and -S #s will not work if the encoder can't determine the\n");
	printf("           input size before starting.\n");
	printf("     Note: if you use -S # and # is >= samples in the input, there will be\n");
	printf("           either no seek point entered (if the input size is determinable\n");
	printf("           before encoding starts) or a placeholder point (if input size is not\n");
	printf("           determinable)\n");
	printf("  -P, --padding=#              Tell the encoder to write a PADDING metadata\n");
	printf("                               block of the given length (in bytes) after the\n");
	printf("                               STREAMINFO block.  This is useful if you plan\n");
	printf("                               to tag the file later with an APPLICATION\n");
	printf("                               block; instead of having to rewrite the entire\n");
	printf("                               file later just to insert your block, you can\n");
	printf("                               write directly over the PADDING block.  Note\n");
	printf("                               that the total length of the PADDING block will\n");
	printf("                               be 4 bytes longer than the length given because\n");
	printf("                               of the 4 metadata block header bytes.  You can\n");
	printf("                               force no PADDING block at all to be written with\n");
	printf("                               --no-padding, which is the default.\n");
	printf("  -b, --blocksize=#            Specify the blocksize in samples; the default is\n");
	printf("                               1152 for -l 0, else 4608; must be one of 192,\n");
	printf("                               576, 1152, 2304, 4608, 256, 512, 1024, 2048,\n");
	printf("                               4096, 8192, 16384, or 32768 (unless --lax is\n");
	printf("                               used)\n");
	printf("  -0, --compression-level-0, --fast  Synonymous with -l 0 -b 1152 -r 2,2\n");
	printf("  -1, --compression-level-1          Synonymous with -l 0 -b 1152 -M -r 2,2\n");
	printf("  -2, --compression-level-2          Synonymous with -l 0 -b 1152 -m -r 3\n");
	printf("  -3, --compression-level-3          Synonymous with -l 6 -b 4608 -r 3,3\n");
	printf("  -4, --compression-level-4          Synonymous with -l 8 -b 4608 -M -r 3,3\n");
	printf("  -5, --compression-level-5          Synonymous with -l 8 -b 4608 -m -r 3,3\n");
	printf("                                     -5 is the default setting\n");
	printf("  -6, --compression-level-6          Synonymous with -l 8 -b 4608 -m -r 4\n");
	printf("  -7, --compression-level-7          Synonymous with -l 8 -b 4608 -m -e -r 6\n");
	printf("  -8, --compression-level-8, --best  Synonymous with -l 12 -b 4608 -m -e -r 6\n");
	printf("  -m, --mid-side                     Try mid-side coding for each frame\n");
	printf("                                     (stereo only)\n");
	printf("  -M, --adaptive-mid-side            Adaptive mid-side coding for all frames\n");
	printf("                                     (stereo only)\n");
	printf("  -e, --exhaustive-model-search      Do exhaustive model search (expensive!)\n");
#if 0
	/*@@@ deprecated: */
	printf("  -E, --escape-coding                Do escape coding in the entropy coder.\n");
	printf("                                     This causes the encoder to use an\n");
	printf("                                     unencoded representation of the residual\n");
	printf("                                     in a partition if it is smaller.  It\n");
	printf("                                     increases the runtime and usually results\n");
	printf("                                     in an improvement of less than 1%.\n");
#endif
	printf("  -l, --max-lpc-order=#              Max LPC order; 0 => only fixed predictors\n");
	printf("  -p, --qlp-coeff-precision-search   Do exhaustive search of LP coefficient\n");
	printf("                                     quantization (expensive!); overrides -q;\n");
	printf("                                     does nothing if using -l 0\n");
	printf("  -q, --qlp-coeff-precision=#        Specify precision in bits of quantized\n");
	printf("                                     linear-predictor coefficients; 0 => let\n");
	printf("                                     encoder decide (the minimun is %u, the\n", FLAC__MIN_QLP_COEFF_PRECISION);
	printf("                                     default is -q 0)\n");
	printf("  -r, --rice-partition-order=[#,]#   Set [min,]max residual partition order\n");
	printf("                                     (# is 0..16; min defaults to 0; the\n");
	printf("                                     default is -r 0; above 4 doesn't usually\n");
	printf("                                     help much)\n");
#if 0
	/*@@@ deprecated: */
	printf("  -R, -rice-parameter-search-distance=#   Rice parameter search distance\n");
#endif
	printf("format options:\n");
	printf("      --endian={big|little}    Set byte order for samples\n");
	printf("      --channels=#             Number of channels\n");
	printf("      --bps=#                  Number of bits per sample\n");
	printf("      --sample-rate=#          Sample rate in Hz\n");
	printf("      --sign={signed|unsigned} Sign of samples (the default is signed)\n");
	printf("      --force-aiff-format      Force the decoder to output AIFF format.  This\n");
	printf("                               option is not needed if the output filename (as\n");
	printf("                               set by -o) ends with .aiff; this option has no\n");
	printf("                               effect when encoding since input AIFF is auto-\n");
	printf("                               detected.\n");
	printf("      --force-raw-format       Force input (when encoding) or output (when\n");
	printf("                               decoding) to be treated as raw samples\n");
	printf("negative options:\n");
	printf("      --no-adaptive-mid-side\n");
	printf("      --no-decode-through-errors\n");
	printf("      --no-delete-input-file\n");
#if 0
/* @@@ deprecated: */
	printf("      --no-escape-coding\n");
#endif
	printf("      --no-exhaustive-model-search\n");
	printf("      --no-lax\n");
	printf("      --no-mid-side\n");
#ifdef FLAC__HAS_OGG
	printf("      --no-ogg\n");
#endif
	printf("      --no-padding\n");
	printf("      --no-qlp-coeff-prec-search\n");
	printf("      --no-residual-gnuplot\n");
	printf("      --no-residual-text\n");
	printf("      --no-sector-align\n");
	printf("      --no-seektable\n");
	printf("      --no-silent\n");
	printf("      --no-verify\n");
}

void
format_mistake(const char *infilename, const char *wrong, const char *right)
{
	fprintf(stderr, "WARNING: %s is not a %s file; treating as a %s file\n", infilename, wrong, right);
}

int encode_file(const char *infilename, FLAC__bool is_first_file, FLAC__bool is_last_file)
{
	FILE *encode_infile;
	FLAC__byte lookahead[12];
	unsigned lookahead_length = 0;
	FileFormat fmt= RAW;
	int retval;
	long infilesize;
	encode_options_t common_options;
	const char *outfilename = get_encoded_outfilename(infilename);

	if(0 == strcmp(infilename, "-")) {
		infilesize = -1;
		encode_infile = grabbag__file_get_binary_stdin();
	}
	else {
		infilesize = grabbag__file_get_filesize(infilename);
		if(0 == (encode_infile = fopen(infilename, "rb"))) {
			fprintf(stderr, "ERROR: can't open input file %s\n", infilename);
			return 1;
		}
	}

	if(!option_values.force_raw_format) {
		/* first set format based on name */
		if(strlen(infilename) > 3 && 0 == strcasecmp(infilename+(strlen(infilename)-4), ".wav"))
			fmt= WAV;
		else if(strlen(infilename) > 3 && 0 == strcasecmp(infilename+(strlen(infilename)-4), ".aif"))
			fmt= AIF;
		else if(strlen(infilename) > 4 && 0 == strcasecmp(infilename+(strlen(infilename)-5), ".aiff"))
			fmt= AIF;

		/* attempt to guess the file type based on the first 12 bytes */
		if((lookahead_length = fread(lookahead, 1, 12, encode_infile)) < 12) {
			if(fmt != RAW)
				format_mistake(infilename, fmt == AIF ? "AIFF" : "WAVE", "raw");
			fmt= RAW;
		}
		else {
			if(!strncmp(lookahead, "RIFF", 4) && !strncmp(lookahead+8, "WAVE", 4))
				fmt= WAV;
			else if(!strncmp(lookahead, "FORM", 4) && !strncmp(lookahead+8, "AIFF", 4))
				fmt= AIF;
			else {
				if(fmt != RAW)
					format_mistake(infilename, fmt == AIF ? "AIFF" : "WAVE", "raw");
				fmt= RAW;
			}
		}
	}

	if(option_values.sector_align && fmt == RAW && infilesize < 0) {
		fprintf(stderr, "ERROR: can't --sector-align when the input size is unknown\n");
		return 1;
	}

	if(fmt == RAW) {
		if(option_values.format_is_big_endian < 0 || option_values.format_is_unsigned_samples < 0 || option_values.format_channels < 0 || option_values.format_bps < 0 || option_values.format_sample_rate < 0)
			return usage_error("ERROR: for encoding a raw file you must specify a value for --endian, --sign, --channels, --bps, and --sample-rate\n");
	}

	if(encode_infile == stdin || option_values.force_to_stdout) {
		if(option_values.replay_gain)
			return usage_error("ERROR: --replay-gain cannot be used when encoding to stdout\n");
	}

	if(!flac__utils_parse_skip_until_specification(option_values.skip_specification, &common_options.skip_specification) || common_options.skip_specification.is_relative)
		return usage_error("ERROR: invalid value for --skip\n");

	if(!flac__utils_parse_skip_until_specification(option_values.until_specification, &common_options.until_specification)) //@@@@ more checks: no + without --skip, no - unless known total_samples_to_{en,de}code
		return usage_error("ERROR: invalid value for --until\n");
	/* if there is no "--until" we want to default to "--until=-0" */
	if(0 == option_values.until_specification)
		common_options.until_specification.is_relative = true;

	common_options.verbose = option_values.verbose;
	common_options.verify = option_values.verify;
#ifdef FLAC__HAS_OGG
	common_options.use_ogg = option_values.use_ogg;
	common_options.has_serial_number = option_values.has_serial_number;
	common_options.serial_number = option_values.serial_number;
#endif
	common_options.lax = option_values.lax;
	common_options.do_mid_side = option_values.do_mid_side;
	common_options.loose_mid_side = option_values.loose_mid_side;
	common_options.do_exhaustive_model_search = option_values.do_exhaustive_model_search;
	common_options.do_escape_coding = option_values.do_escape_coding;
	common_options.do_qlp_coeff_prec_search = option_values.do_qlp_coeff_prec_search;
	common_options.min_residual_partition_order = option_values.min_residual_partition_order;
	common_options.max_residual_partition_order = option_values.max_residual_partition_order;
	common_options.rice_parameter_search_dist = option_values.rice_parameter_search_dist;
	common_options.max_lpc_order = option_values.max_lpc_order;
	common_options.blocksize = (unsigned)option_values.blocksize;
	common_options.qlp_coeff_precision = option_values.qlp_coeff_precision;
	common_options.padding = option_values.padding;
	common_options.requested_seek_points = option_values.requested_seek_points;
	common_options.num_requested_seek_points = option_values.num_requested_seek_points;
	common_options.cuesheet_filename = option_values.cuesheet_filename;
	common_options.cued_seekpoints = option_values.cued_seekpoints;
	common_options.is_first_file = is_first_file;
	common_options.is_last_file = is_last_file;
	common_options.align_reservoir = align_reservoir;
	common_options.align_reservoir_samples = &align_reservoir_samples;
	common_options.replay_gain = option_values.replay_gain;
	common_options.sector_align = option_values.sector_align;
	common_options.vorbis_comment = option_values.vorbis_comment;
	common_options.debug.disable_constant_subframes = option_values.debug.disable_constant_subframes;
	common_options.debug.disable_fixed_subframes = option_values.debug.disable_fixed_subframes;
	common_options.debug.disable_verbatim_subframes = option_values.debug.disable_verbatim_subframes;

	if(fmt == RAW) {
		raw_encode_options_t options;

		options.common = common_options;
		options.is_big_endian = option_values.format_is_big_endian;
		options.is_unsigned_samples = option_values.format_is_unsigned_samples;
		options.channels = option_values.format_channels;
		options.bps = option_values.format_bps;
		options.sample_rate = option_values.format_sample_rate;

		retval = flac__encode_raw(encode_infile, infilesize, infilename, outfilename, lookahead, lookahead_length, options);
	}
	else {
		wav_encode_options_t options;

		options.common = common_options;

		if(fmt == AIF)
			retval = flac__encode_aif(encode_infile, infilesize, infilename, outfilename, lookahead, lookahead_length, options);
		else
			retval = flac__encode_wav(encode_infile, infilesize, infilename, outfilename, lookahead, lookahead_length, options);
	}

	if(retval == 0 && strcmp(infilename, "-")) {
		if(strcmp(outfilename, "-")) {
			if(option_values.replay_gain) {
				float title_gain, title_peak;
				const char *error;
				grabbag__replaygain_get_title(&title_gain, &title_peak);
				if(0 != (error = grabbag__replaygain_store_to_file_title(outfilename, title_gain, title_peak, /*preserve_modtime=*/true))) {
					fprintf(stderr, "%s: ERROR writing ReplayGain title tags\n", outfilename);
				}
			}
			grabbag__file_copy_metadata(infilename, outfilename);
		}
		if(option_values.delete_input)
			unlink(infilename);
	}

	return retval;
}

int decode_file(const char *infilename)
{
	int retval;
	FLAC__bool treat_as_ogg = false;
	decode_options_t common_options;
	const char *outfilename = get_decoded_outfilename(infilename);

	if(!option_values.test_only && !option_values.analyze) {
		if(option_values.force_raw_format && (option_values.format_is_big_endian < 0 || option_values.format_is_unsigned_samples < 0))
			return usage_error("ERROR: for decoding to a raw file you must specify a value for --endian and --sign\n");
	}

	if(option_values.use_ogg)
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

	if(!flac__utils_parse_skip_until_specification(option_values.skip_specification, &common_options.skip_specification) || common_options.skip_specification.is_relative)
		return usage_error("ERROR: invalid value for --skip\n");

	if(!flac__utils_parse_skip_until_specification(option_values.until_specification, &common_options.until_specification)) //@@@@ more checks: no + without --skip, no - unless known total_samples_to_{en,de}code
		return usage_error("ERROR: invalid value for --until\n");
	/* if there is no "--until" we want to default to "--until=-0" */
	if(0 == option_values.until_specification)
		common_options.until_specification.is_relative = true;

	common_options.verbose = option_values.verbose;
	common_options.continue_through_decode_errors = option_values.continue_through_decode_errors;
#ifdef FLAC__HAS_OGG
	common_options.is_ogg = treat_as_ogg;
	common_options.use_first_serial_number = !option_values.has_serial_number;
	common_options.serial_number = option_values.serial_number;
#endif

	if(!option_values.force_raw_format) {
		wav_decode_options_t options;

		options.common = common_options;

		if(
			option_values.force_aiff_format ||
			(strlen(outfilename) > 3 && 0 == strcasecmp(outfilename+(strlen(outfilename)-4), ".aif")) ||
			(strlen(outfilename) > 4 && 0 == strcasecmp(outfilename+(strlen(outfilename)-5), ".aiff"))
		)
			retval = flac__decode_aiff(infilename, option_values.test_only? 0 : outfilename, option_values.analyze, option_values.aopts, options);
		else
			retval = flac__decode_wav(infilename, option_values.test_only? 0 : outfilename, option_values.analyze, option_values.aopts, options);
	}
	else {
		raw_decode_options_t options;

		options.common = common_options;
		options.is_big_endian = option_values.format_is_big_endian;
		options.is_unsigned_samples = option_values.format_is_unsigned_samples;

		retval = flac__decode_raw(infilename, option_values.test_only? 0 : outfilename, option_values.analyze, option_values.aopts, options);
	}

	if(retval == 0 && strcmp(infilename, "-")) {
		if(strcmp(outfilename, "-"))
			grabbag__file_copy_metadata(infilename, outfilename);
		if(option_values.delete_input && !option_values.test_only && !option_values.analyze)
			unlink(infilename);
	}

	return retval;
}

const char *get_encoded_outfilename(const char *infilename)
{
	if(0 == option_values.cmdline_forced_outfilename) {
		static char buffer[4096]; /* @@@ bad MAGIC NUMBER */

		if(0 == strcmp(infilename, "-") || option_values.force_to_stdout) {
			strcpy(buffer, "-");
		}
		else {
			const char *suffix = (option_values.use_ogg? ".ogg" : ".flac");
			char *p;
			strcpy(buffer, option_values.output_prefix? option_values.output_prefix : "");
			strcat(buffer, infilename);
			if(0 == (p = strrchr(buffer, '.')))
				strcat(buffer, suffix);
			else {
				if(0 == strcmp(p, suffix)) {
					strcpy(p, "_new");
					strcat(p, suffix);
				}
				else
					strcpy(p, suffix);
			}
		}
		return buffer;
	}
	else
		return option_values.cmdline_forced_outfilename;
}

const char *get_decoded_outfilename(const char *infilename)
{
	if(0 == option_values.cmdline_forced_outfilename) {
		static char buffer[4096]; /* @@@ bad MAGIC NUMBER */

		if(0 == strcmp(infilename, "-") || option_values.force_to_stdout) {
			strcpy(buffer, "-");
		}
		else {
			static const char *suffixes[] = { ".wav", ".aiff", ".raw", ".ana" };
			const char *suffix = suffixes[
				option_values.analyze? 3 :
				option_values.force_raw_format? 2 :
				option_values.force_aiff_format? 1 :
				0
			];
			char *p;
			strcpy(buffer, option_values.output_prefix? option_values.output_prefix : "");
			strcat(buffer, infilename);
			if(0 == (p = strrchr(buffer, '.')))
				strcat(buffer, suffix);
			else {
				if(0 == strcmp(p, suffix)) {
					strcpy(p, "_new");
					strcat(p, suffix);
				}
				else
					strcpy(p, suffix);
			}
		}
		return buffer;
	}
	else
		return option_values.cmdline_forced_outfilename;
}

void die(const char *message)
{
	FLAC__ASSERT(0 != message);
	fprintf(stderr, "ERROR: %s\n", message);
	exit(1);
}

char *local_strdup(const char *source)
{
	char *ret;
	FLAC__ASSERT(0 != source);
	if(0 == (ret = strdup(source)))
		die("out of memory during strdup()");
	return ret;
}
