/* metaflac - Command-line FLAC metadata editor
 * Copyright (C) 2001,2002  Josh Coalson
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

/*@@@
more powerful operations yet to add:
	add a seektable, using same args as flac
*/

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "FLAC/metadata.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if HAVE_GETOPT_LONG
#  include <getopt.h>
#else
#  include "share/getopt.h"
#endif

/* getopt format struct; note we don't use short options so we just
   set the 'val' field to 1 everywhere to indicate a valid option. */
static struct option long_options_[] = {
	/* global options */
    { "preserve-modtime", 0, 0, 1 },
    { "with-filename", 0, 0, 1 },
    { "no-filename", 0, 0, 1 },
    { "dont-use-padding", 0, 0, 1 },
	/* shorthand operations */
    { "show-md5sum", 0, 0, 1 },
    { "show-min-blocksize", 0, 0, 1 },
    { "show-max-blocksize", 0, 0, 1 },
    { "show-min-framesize", 0, 0, 1 },
    { "show-max-framesize", 0, 0, 1 },
    { "show-sample-rate", 0, 0, 1 },
    { "show-channels", 0, 0, 1 },
    { "show-bps", 0, 0, 1 },
    { "show-total-samples", 0, 0, 1 },
    { "show-vc-vendor", 0, 0, 1 },
    { "show-vc-field", 1, 0, 1 },
    { "remove-vc-all", 0, 0, 1 },
    { "remove-vc-field", 1, 0, 1 },
    { "remove-vc-firstfield", 1, 0, 1 },
    { "set-vc-field", 1, 0, 1 },
	/* major operations */
    { "list", 0, 0, 1 },
    { "append", 0, 0, 1 },
    { "remove", 0, 0, 1 },
    { "remove-all", 0, 0, 1 },
    { "merge-padding", 0, 0, 1 },
    { "sort-padding", 0, 0, 1 },
    { "block-number", 1, 0, 1 },
    { "block-type", 1, 0, 1 },
    { "except-block-type", 1, 0, 1 },
    { "data-format", 1, 0, 1 },
    { "application-data-format", 1, 0, 1 },
    { "from-file", 1, 0, 1 },
    {0, 0, 0, 0}
};

typedef enum {
	OP__SHOW_MD5SUM,
	OP__SHOW_MIN_BLOCKSIZE,
	OP__SHOW_MAX_BLOCKSIZE,
	OP__SHOW_MIN_FRAMESIZE,
	OP__SHOW_MAX_FRAMESIZE,
	OP__SHOW_SAMPLE_RATE,
	OP__SHOW_CHANNELS,
	OP__SHOW_BPS,
	OP__SHOW_TOTAL_SAMPLES,
	OP__SHOW_VC_VENDOR,
	OP__SHOW_VC_FIELD,
	OP__REMOVE_VC_FIELD,
	OP__REMOVE_VC_FIRSTFIELD,
	OP__SET_VC_FIELD,
	OP__LIST,
	OP__APPEND,
	OP__REMOVE,
	OP__REMOVE_ALL,
	OP__MERGE_PADDING,
	OP__SORT_PADDING,
	OP__BLOCK_NUMBER,
	OP__BLOCK_TYPE,
	OP__EXCEPT_BLOCK_TYPE,
	OP__DATA_FORMAT,
	OP__APPLICATION_DATA_FORMAT,
	OP__FROM_FILE
} OperationType;

typedef struct {
	int dummy;
} Argument_VcFieldName;

typedef struct {
	int dummy;
} Argument_VcField;

typedef struct {
	int dummy;
} Argument_BlockNumber;

typedef struct {
	int dummy;
} Argument_BlockType;

typedef struct {
	int dummy;
} Argument_DataFormat;

typedef struct {
	int dummy;
} Argument_ApplicationDataFormat;

typedef struct {
	int dummy;
} Argument_FromFile;

typedef struct {
	OperationType operation;
	union {
		Argument_VcFieldName show_vc_field;
		Argument_VcFieldName remove_vc_field;
		Argument_VcFieldName remove_vc_firstfield;
		Argument_VcField set_vc_field;
		Argument_BlockNumber block_number;
		Argument_BlockType block_type;
		Argument_BlockType except_block_type;
		Argument_DataFormat data_format;
		Argument_ApplicationDataFormat application_data_format;
		Argument_FromFile from_file;
	} argument;
} Operation;

typedef struct {
	FLAC__bool preserve_modtime;
	FLAC__bool prefix_with_filename;
	FLAC__bool use_padding;
	Operation *operations;
	struct {
		FLAC__bool has_shorthand_op;
		FLAC__bool has_major_op;
		FLAC__bool has_block_type;
		FLAC__bool has_except_block_type;
	} checks;
} CommandLineOptions;

static void parse_options(int argc, char *argv[], CommandLineOptions *options);
static int short_usage(const char *message, ...);
static int long_usage(const char *message, ...);
static void hexdump(const FLAC__byte *buf, unsigned bytes, const char *indent);

int main(int argc, char *argv[])
{
	CommandLineOptions o;
	parse_options(argc, argv, &o);
	return 3;
}

void parse_options(int argc, char *argv[], CommandLineOptions *options)
{
    int ret;
    int option_index = 1;

    while ((ret = getopt_long(argc, argv, "", long_options_, &option_index)) != -1) {
        switch (ret) {
            case 0:
                fprintf(stderr, "Internal error parsing command options\n");
                exit(1);
                break;
            case 1:
                break;
			case '?':
                break;
			case ':':
                break;
            default:
                /*@@@usage();*/
                exit(1);
        }
    }

#if 0
    /* remaining bits must be the filenames */
    if((param->mode == MODE_LIST && (argc-optind) != 1) ||
       ((param->mode == MODE_WRITE || param->mode == MODE_APPEND) &&
       ((argc-optind) < 1 || (argc-optind) > 2))) {
            usage();
            exit(1);
    }

    param->infilename = strdup(argv[optind]);
    if (param->mode == MODE_WRITE || param->mode == MODE_APPEND)
    {
        if(argc-optind == 1)
        {
            param->tempoutfile = 1;
            param->outfilename = malloc(strlen(param->infilename)+8);
            strcpy(param->outfilename, param->infilename);
            strcat(param->outfilename, ".vctemp");
        }
        else
            param->outfilename = strdup(argv[optind+1]);
    }
#endif
}

static void usage_header(FILE *out)
{
	fprintf(out, "==============================================================================\n");
	fprintf(out, "metaflac - Command-line FLAC metadata editor version %s\n", FLAC__VERSION_STRING);
	fprintf(out, "Copyright (C) 2001,2002  Josh Coalson\n");
	fprintf(out, "\n");
	fprintf(out, "This program is free software; you can redistribute it and/or\n");
	fprintf(out, "modify it under the terms of the GNU General Public License\n");
	fprintf(out, "as published by the Free Software Foundation; either version 2\n");
	fprintf(out, "of the License, or (at your option) any later version.\n");
	fprintf(out, "\n");
	fprintf(out, "This program is distributed in the hope that it will be useful,\n");
	fprintf(out, "but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	fprintf(out, "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
	fprintf(out, "GNU General Public License for more details.\n");
	fprintf(out, "\n");
	fprintf(out, "You should have received a copy of the GNU General Public License\n");
	fprintf(out, "along with this program; if not, write to the Free Software\n");
	fprintf(out, "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.\n");
	fprintf(out, "==============================================================================\n");
}

static void usage_summary(FILE *out)
{
    fprintf(out, "Usage:\n");
    fprintf(out, "  metaflac [options] [operations] FLACfile [FLACfile ...]\n");
    fprintf(out, "\n");
    fprintf(out, "Use metaflac to list, add, remove, or edit metadata in one or more FLAC files.\n");
    fprintf(out, "You may perform one major operation, or many shorthand operations at a time.\n");
    fprintf(out, "\n");
    fprintf(out, "Options:\n");
    fprintf(out, "--preserve-modtime    Preserve the original modification time in spite of edits\n");
    fprintf(out, "--with-filename       Prefix each output line with the FLAC file name\n");
    fprintf(out, "                      (the default if more than one FLAC file is specified)\n");
    fprintf(out, "--no-filename         Do not prefix each output line with the FLAC file name\n");
    fprintf(out, "                      (the default if only one FLAC file is specified)\n");
    fprintf(out, "--dont-use-padding    By default metaflac tries to use padding where possible\n");
    fprintf(out, "                      to avoid rewriting the entire file if the metadata size\n");
    fprintf(out, "                      changes.  Use this option to tell metaflac to not use\n");
    fprintf(out, "                      padding at all.\n");
}

int short_usage(const char *message, ...)
{
	va_list args;

	if(message) {
		va_start(args, message);

		(void) vfprintf(stderr, message, args);

		va_end(args);

	}
	usage_header(stderr);
	fprintf(stderr, "\n");
	fprintf(stderr, "This is the short help; for full help use 'metaflac --help'\n");
	fprintf(stderr, "\n");
	usage_summary(stderr);

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
	usage_header(out);
    fprintf(out, "\n");
	usage_summary(out);
    fprintf(out, "\n");
    fprintf(out, "Shorthand operations:\n");
    fprintf(out, "--show-md5sum         Show the MD5 signature from the STREAMINFO block.\n");
    fprintf(out, "--show-min-blocksize  Show the minimum block size from the STREAMINFO block.\n");
    fprintf(out, "--show-max-blocksize  Show the maximum block size from the STREAMINFO block.\n");
    fprintf(out, "--show-min-framesize  Show the minimum frame size from the STREAMINFO block.\n");
    fprintf(out, "--show-max-framesize  Show the maximum frame size from the STREAMINFO block.\n");
    fprintf(out, "--show-sample-rate    Show the sample rate from the STREAMINFO block.\n");
    fprintf(out, "--show-channels       Show the number of channels from the STREAMINFO block.\n");
    fprintf(out, "--show-bps            Show the # of bits per sample from the STREAMINFO block.\n");
    fprintf(out, "--show-total-samples  Show the total # of samples from the STREAMINFO block.\n");
    fprintf(out, "\n");
    fprintf(out, "--show-vc-vendor      Show the vendor string from the VORBIS_COMMENT block.\n");
    fprintf(out, "--show-vc-field=name  Show all Vorbis comment fields where the the field name\n");
    fprintf(out, "                      matches 'name'.\n");
    fprintf(out, "--remove-vc-field=name\n");
    fprintf(out, "                      Remove all Vorbis comment fields whose field name is\n");
    fprintf(out, "                      'name'.\n");
    fprintf(out, "--remove-vc-firstfield=name\n");
    fprintf(out, "                      Remove first Vorbis comment field whose field name is\n");
    fprintf(out, "                      'name'.\n");
    fprintf(out, "--remove-vc-all       Remove all Vorbis comment fields, leaving only the\n");
    fprintf(out, "                      vendor string in the VORBIS_COMMENT block.\n");
    fprintf(out, "--set-vc-field=field  Add a Vorbis comment field.  The field must comply with\n");
    fprintf(out, "                      the Vorbis comment spec, of the form \"NAME=VALUE\".  If\n");
    fprintf(out, "                      there is currently no VORBIS_COMMENT block, one will be\n");
    fprintf(out, "                      created.\n");
    fprintf(out, "\n");
    fprintf(out, "Major operations:\n");
    fprintf(out, "--list : List the contents of one or more metadata blocks to stdout.  By\n");
    fprintf(out, "         default, all metadata blocks are listed in text format.  Use the\n");
    fprintf(out, "         following options to change this behavior:\n");
    fprintf(out, "\n");
    fprintf(out, "    --block-number=#[,#[...]]\n");
    fprintf(out, "    An optional comma-separated list of block numbers to display.  The first\n");
    fprintf(out, "    block, the STREAMINFO block, is block 0.\n");
    fprintf(out, "\n");
    fprintf(out, "    --block-type=type[,type[...]]\n");
    fprintf(out, "    --except-block-type=type[,type[...]]\n");
    fprintf(out, "    An optional comma-separated list of block types to included or ignored\n");
    fprintf(out, "    with this option.  Use only one of --block-type or --except-block-type.\n");
    fprintf(out, "    The valid block types are: STREAMINFO, PADDING, APPLICATION, SEEKTABLE,\n");
    fprintf(out, "    VORBIS_COMMENT.  You may narrow down the types of APPLICATION blocks\n");
    fprintf(out, "    displayed as follows:\n");
    fprintf(out, "        APPLICATION:abcd        The APPLICATION block(s) whose textual repre-\n");
    fprintf(out, "                                sentation of the 4-byte ID is \"abcd\"\n");
    fprintf(out, "        APPLICATION:0xXXXXXXXX  The APPLICATION block(s) whose hexadecimal big-\n");
    fprintf(out, "                                endian representation of the 4-byte ID is\n");
    fprintf(out, "                                \"0xXXXXXXXX\".  For the example \"abcd\" above the\n");
    fprintf(out, "                                hexadecimal equivalalent is 0x61626364\n");
    fprintf(out, "\n");
    fprintf(out, "    NOTE: if both --block-number and --[except-]block-type are specified,\n");
    fprintf(out, "          the result is the logical AND of both arguments.\n");
    fprintf(out, "\n");
#if 0
	/*@@@ not implemented yet */
    fprintf(out, "    --data-format=binary|text\n");
    fprintf(out, "    By default a human-readable text representation of the data is displayed.\n");
    fprintf(out, "    You may specify --data-format=binary to dump the raw binary form of each\n");
    fprintf(out, "    metadata block.  The output can be read in using a subsequent call to\n");
    fprintf(out, "    "metaflac --append --from-file=..."\n");
#endif
    fprintf(out, "\n");
    fprintf(out, "    --application-data-format=hexdump|text\n");
    fprintf(out, "    If the application block you are displaying contains binary data but your\n");
    fprintf(out, "    --data-format=text, you can display a hex dump of the application data\n");
    fprintf(out, "    contents instead using --application-data-format=hexdump\n");
    fprintf(out, "\n");
#if 0
	/*@@@ not implemented yet */
    fprintf(out, "--append : Insert a metadata block from a file.  The input file must be in the\n");
    fprintf(out, "           same format as generated with --list.\n");
    fprintf(out, "\n");
    fprintf(out, "    --block-number=#\n");
    fprintf(out, "    Specify the insertion point (defaults to last block).  The new block will\n");
    fprintf(out, "    be added after the given block number.  This prevents the illegal insertion\n");
    fprintf(out, "    of a block before the first STREAMINFO block.  You may not --append another\n");
    fprintf(out, "    STREAMINFO block.\n");
    fprintf(out, "\n");
    fprintf(out, "    --from-file=filename\n");
    fprintf(out, "    Mandatory 'option' to specify the input file containing the block contents.\n");
    fprintf(out, "\n");
    fprintf(out, "    --data-format=binary|text\n");
    fprintf(out, "    By default the block contents are assumed to be in binary format.  You can\n");
    fprintf(out, "    override this by specifying --data-format=text\n");
    fprintf(out, "\n");
#endif
    fprintf(out, "--remove : Remove one or more metadata blocks from the metadata.  Unless\n");
    fprintf(out, "           --dont-use-padding is specified, the blocks will be replaced with\n");
    fprintf(out, "           padding.  You may not remove the STREAMINFO block.\n");
    fprintf(out, "\n");
    fprintf(out, "    --block-number=#[,#[...]]\n");
    fprintf(out, "    --block-type=type[,type[...]]\n");
    fprintf(out, "    --except-block-type=type[,type[...]]\n");
    fprintf(out, "    See --list above for usage.\n");
    fprintf(out, "\n");
    fprintf(out, "    NOTE: if both --block-number and --[except-]block-type are specified,\n");
    fprintf(out, "          the result is the logical AND of both arguments.\n");
    fprintf(out, "\n");
    fprintf(out, "--remove-all : Remove all metadata blocks (except the STREAMINFO block) from\n");
    fprintf(out, "               the metadata.  Unless --dont-use-padding is specified, the\n");
    fprintf(out, "               blocks will be replaced with padding.\n");
    fprintf(out, "\n");
    fprintf(out, "--merge-padding : Merge adjacent PADDING blocks into single blocks.\n");
    fprintf(out, "\n");
    fprintf(out, "--sort-padding : Move all PADDING blocks to the end of the metadata and merge\n");
    fprintf(out, "                 them into a single block.\n");

	return message? 1 : 0;
}

void hexdump(const FLAC__byte *buf, unsigned bytes, const char *indent)
{
	unsigned i, left = bytes;
	const FLAC__byte *b = buf;

	for(i = 0; i < bytes; i += 16) {
		printf("%s%08X: "
			"%02X %02X %02X %02X %02X %02X %02X %02X "
			"%02X %02X %02X %02X %02X %02X %02X %02X "
			"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
			indent, i,
			left >  0? (unsigned char)b[ 0] : 0,
			left >  1? (unsigned char)b[ 1] : 0,
			left >  2? (unsigned char)b[ 2] : 0,
			left >  3? (unsigned char)b[ 3] : 0,
			left >  4? (unsigned char)b[ 4] : 0,
			left >  5? (unsigned char)b[ 5] : 0,
			left >  6? (unsigned char)b[ 6] : 0,
			left >  7? (unsigned char)b[ 7] : 0,
			left >  8? (unsigned char)b[ 8] : 0,
			left >  9? (unsigned char)b[ 9] : 0,
			left > 10? (unsigned char)b[10] : 0,
			left > 11? (unsigned char)b[11] : 0,
			left > 12? (unsigned char)b[12] : 0,
			left > 13? (unsigned char)b[13] : 0,
			left > 14? (unsigned char)b[14] : 0,
			left > 15? (unsigned char)b[15] : 0,
			(left >  0) ? (isprint(b[ 0]) ? b[ 0] : '.') : ' ',
			(left >  1) ? (isprint(b[ 1]) ? b[ 1] : '.') : ' ',
			(left >  2) ? (isprint(b[ 2]) ? b[ 2] : '.') : ' ',
			(left >  3) ? (isprint(b[ 3]) ? b[ 3] : '.') : ' ',
			(left >  4) ? (isprint(b[ 4]) ? b[ 4] : '.') : ' ',
			(left >  5) ? (isprint(b[ 5]) ? b[ 5] : '.') : ' ',
			(left >  6) ? (isprint(b[ 6]) ? b[ 6] : '.') : ' ',
			(left >  7) ? (isprint(b[ 7]) ? b[ 7] : '.') : ' ',
			(left >  8) ? (isprint(b[ 8]) ? b[ 8] : '.') : ' ',
			(left >  9) ? (isprint(b[ 9]) ? b[ 9] : '.') : ' ',
			(left > 10) ? (isprint(b[10]) ? b[10] : '.') : ' ',
			(left > 11) ? (isprint(b[11]) ? b[11] : '.') : ' ',
			(left > 12) ? (isprint(b[12]) ? b[12] : '.') : ' ',
			(left > 13) ? (isprint(b[13]) ? b[13] : '.') : ' ',
			(left > 14) ? (isprint(b[14]) ? b[14] : '.') : ' ',
			(left > 15) ? (isprint(b[15]) ? b[15] : '.') : ' '
		);
		left -= 16;
		b += 16;
   }
}
