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

#include "FLAC/assert.h"
#include "FLAC/metadata.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_GETOPT_LONG
#  include <getopt.h>
#else
#  include "share/getopt.h"
#endif

/*
   getopt format struct; note we don't use short options so we just
   set the 'val' field to 0 everywhere to indicate a valid option.
*/
static struct option long_options_[] = {
	/* global options */
    { "preserve-modtime", 0, 0, 0 }, 
    { "with-filename", 0, 0, 0 }, 
    { "no-filename", 0, 0, 0 }, 
    { "dont-use-padding", 0, 0, 0 }, 
	/* shorthand operations */
    { "show-md5sum", 0, 0, 0 }, 
    { "show-min-blocksize", 0, 0, 0 }, 
    { "show-max-blocksize", 0, 0, 0 }, 
    { "show-min-framesize", 0, 0, 0 }, 
    { "show-max-framesize", 0, 0, 0 }, 
    { "show-sample-rate", 0, 0, 0 }, 
    { "show-channels", 0, 0, 0 }, 
    { "show-bps", 0, 0, 0 }, 
    { "show-total-samples", 0, 0, 0 }, 
    { "show-vc-vendor", 0, 0, 0 }, 
    { "show-vc-field", 1, 0, 0 }, 
    { "remove-vc-all", 0, 0, 0 },
    { "remove-vc-field", 1, 0, 0 },
    { "remove-vc-firstfield", 1, 0, 0 },
    { "set-vc-field", 1, 0, 0 },
	/* major operations */
    { "help", 0, 0, 0 },
    { "list", 0, 0, 0 },
    { "append", 0, 0, 0 },
    { "remove", 0, 0, 0 },
    { "remove-all", 0, 0, 0 },
    { "merge-padding", 0, 0, 0 },
    { "sort-padding", 0, 0, 0 },
    { "block-number", 1, 0, 0 },
    { "block-type", 1, 0, 0 },
    { "except-block-type", 1, 0, 0 },
    { "data-format", 1, 0, 0 },
    { "application-data-format", 1, 0, 0 },
    { "from-file", 1, 0, 0 },
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
	OP__REMOVE_VC_ALL,
	OP__REMOVE_VC_FIELD,
	OP__REMOVE_VC_FIRSTFIELD,
	OP__SET_VC_FIELD,
	OP__HELP,
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
	char *field_name;
} Argument_VcFieldName;

typedef struct {
	char *field_name;
	/* according to the vorbis spec, field values can contain \0 so simple C strings are not enough here */
	unsigned field_value_length;
	char *field_value;
} Argument_VcField;

typedef struct {
	unsigned num_entries;
	unsigned *entries;
} Argument_BlockNumber;

typedef struct {
	int dummy;
} Argument_BlockType;

typedef struct {
	FLAC__bool is_binary;
} Argument_DataFormat;

typedef struct {
	FLAC__bool is_hexdump;
} Argument_ApplicationDataFormat;

typedef struct {
	char *file_name;
} Argument_FromFile;

typedef struct {
	OperationType type;
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
	struct {
		unsigned num_shorthand_ops;
		unsigned num_major_ops;
		FLAC__bool has_block_type;
		FLAC__bool has_except_block_type;
	} checks;
	struct {
		Operation *operations;
		unsigned num_operations;
		unsigned capacity;
	} ops;
} CommandLineOptions;

static void init_options(CommandLineOptions *options);
static FLAC__bool parse_options(int argc, char *argv[], CommandLineOptions *options);
static FLAC__bool parse_option(int option_index, const char *option_argument, CommandLineOptions *options);
static void free_options(CommandLineOptions *options);
static void append_operation(CommandLineOptions *options, Operation operation);
static Operation *append_major_operation(CommandLineOptions *options, OperationType type);
static Operation *append_shorthand_operation(CommandLineOptions *options, OperationType type);
static int short_usage(const char *message, ...);
static int long_usage(const char *message, ...);
static void hexdump(const FLAC__byte *buf, unsigned bytes, const char *indent);
static char *local_strdup(const char *source);
static FLAC__bool parse_vorbis_comment_field(const char *field, char **name, char **value, unsigned *length);
static FLAC__bool parse_block_number(const char *in, Argument_BlockNumber *out);
static FLAC__bool parse_block_type(const char *in, Argument_BlockType *out);
static FLAC__bool parse_data_format(const char *in, Argument_DataFormat *out);
static FLAC__bool parse_application_data_format(const char *in, Argument_ApplicationDataFormat *out);

int main(int argc, char *argv[])
{
	CommandLineOptions options;
	int ret;

	init_options(&options);

	ret = !parse_options(argc, argv, &options);

	free_options(&options);

	return ret;
}

void init_options(CommandLineOptions *options)
{
	options->preserve_modtime = false;

	/* hack to mean "use default if not forced on command line" */
	FLAC__ASSERT(true != 2);
	options->prefix_with_filename = 2;

	options->use_padding = true;
	options->checks.num_shorthand_ops = 0;
	options->checks.num_major_ops = 0;
	options->checks.has_block_type = false;
	options->checks.has_except_block_type = false;

	options->ops.operations = 0;
	options->ops.num_operations = 0;
	options->ops.capacity = 0;
}

FLAC__bool parse_options(int argc, char *argv[], CommandLineOptions *options)
{
    int ret;
    int option_index = 1;
	FLAC__bool had_error = false;

    while ((ret = getopt_long(argc, argv, "", long_options_, &option_index)) != -1) {
        switch (ret) {
            case 0:
				had_error |= !parse_option(option_index, optarg, options);
                break;
			case '?':
			case ':':
                short_usage(0);
                return false;
				had_error = true;
            default:
				FLAC__ASSERT(0);
        }
    }

	if(options->prefix_with_filename == 2)
		options->prefix_with_filename = (argc - optind > 1);

	if(optind < argc) {
		while(optind < argc)
			printf("%s ", argv[optind++]);
		printf("\n");
	}

	return had_error;
}

FLAC__bool parse_option(int option_index, const char *option_argument, CommandLineOptions *options)
{
	const char *opt = long_options_[option_index].name;
	Operation *op;
	FLAC__bool ret = true;

    if(0 == strcmp(opt, "preserve-modtime")) {
		options->preserve_modtime = true;
	}
    else if(0 == strcmp(opt, "with-filename")) {
		options->prefix_with_filename = true;
	}
    else if(0 == strcmp(opt, "no-filename")) {
		options->prefix_with_filename = false;
	}
    else if(0 == strcmp(opt, "dont-use-padding")) {
		options->use_padding = false;
	}
    else if(0 == strcmp(opt, "show-md5sum")) {
		(void) append_shorthand_operation(options, OP__SHOW_MD5SUM);
	}
    else if(0 == strcmp(opt, "show-min-blocksize")) {
		(void) append_shorthand_operation(options, OP__SHOW_MIN_BLOCKSIZE);
	}
    else if(0 == strcmp(opt, "show-max-blocksize")) {
		(void) append_shorthand_operation(options, OP__SHOW_MAX_BLOCKSIZE);
	}
    else if(0 == strcmp(opt, "show-min-framesize")) {
		(void) append_shorthand_operation(options, OP__SHOW_MIN_FRAMESIZE);
	}
    else if(0 == strcmp(opt, "show-max-framesize")) {
		(void) append_shorthand_operation(options, OP__SHOW_MAX_FRAMESIZE);
	}
    else if(0 == strcmp(opt, "show-sample-rate")) {
		(void) append_shorthand_operation(options, OP__SHOW_SAMPLE_RATE);
	}
    else if(0 == strcmp(opt, "show-channels")) {
		(void) append_shorthand_operation(options, OP__SHOW_CHANNELS);
	}
    else if(0 == strcmp(opt, "show-bps")) {
		(void) append_shorthand_operation(options, OP__SHOW_BPS);
	}
    else if(0 == strcmp(opt, "show-total-samples")) {
		(void) append_shorthand_operation(options, OP__SHOW_TOTAL_SAMPLES);
	}
    else if(0 == strcmp(opt, "show-vc-vendor")) {
		(void) append_shorthand_operation(options, OP__SHOW_VC_VENDOR);
	}
    else if(0 == strcmp(opt, "show-vc-field")) {
		op = append_shorthand_operation(options, OP__SHOW_VC_FIELD);
		FLAC__ASSERT(0 != option_argument);
		op->argument.show_vc_field.field_name = local_strdup(option_argument);
	}
    else if(0 == strcmp(opt, "remove-vc-all")) {
		(void) append_shorthand_operation(options, OP__REMOVE_VC_ALL);
	}
    else if(0 == strcmp(opt, "remove-vc-field")) {
		op = append_shorthand_operation(options, OP__REMOVE_VC_FIELD);
		FLAC__ASSERT(0 != option_argument);
		op->argument.remove_vc_field.field_name = local_strdup(option_argument);
	}
    else if(0 == strcmp(opt, "remove-vc-firstfield")) {
		op = append_shorthand_operation(options, OP__REMOVE_VC_FIRSTFIELD);
		FLAC__ASSERT(0 != option_argument);
		op->argument.remove_vc_firstfield.field_name = local_strdup(option_argument);
	}
    else if(0 == strcmp(opt, "set-vc-field")) {
		op = append_shorthand_operation(options, OP__SET_VC_FIELD);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_vorbis_comment_field(option_argument, &(op->argument.set_vc_field.field_name), &(op->argument.set_vc_field.field_value), &(op->argument.set_vc_field.field_value_length))) {
			fprintf(stderr, "ERROR: malformed vorbis comment field \"%s\"\n", option_argument);
			ret = false;
		}
	}
    else if(0 == strcmp(opt, "help")) {
		(void) append_major_operation(options, OP__HELP);
	}
    else if(0 == strcmp(opt, "list")) {
		(void) append_major_operation(options, OP__LIST);
	}
    else if(0 == strcmp(opt, "append")) {
		(void) append_major_operation(options, OP__APPEND);
	}
    else if(0 == strcmp(opt, "remove")) {
		(void) append_major_operation(options, OP__REMOVE);
	}
    else if(0 == strcmp(opt, "remove-all")) {
		(void) append_major_operation(options, OP__REMOVE_ALL);
	}
    else if(0 == strcmp(opt, "merge-padding")) {
		(void) append_major_operation(options, OP__MERGE_PADDING);
	}
    else if(0 == strcmp(opt, "sort-padding")) {
		(void) append_major_operation(options, OP__SORT_PADDING);
	}
    else if(0 == strcmp(opt, "block-number")) {
		op = append_major_operation(options, OP__BLOCK_NUMBER);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_block_number(option_argument, &(op->argument.block_number))) {
			fprintf(stderr, "ERROR: malformed block number specification \"%s\"\n", option_argument);
			ret = false;
		}
	}
    else if(0 == strcmp(opt, "block-type")) {
		op = append_major_operation(options, OP__BLOCK_TYPE);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_block_type(option_argument, &(op->argument.block_type))) {
			fprintf(stderr, "ERROR: malformed block type specification \"%s\"\n", option_argument);
			ret = false;
		}
	}
    else if(0 == strcmp(opt, "except-block-type")) {
		op = append_major_operation(options, OP__EXCEPT_BLOCK_TYPE);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_block_type(option_argument, &(op->argument.except_block_type))) {
			fprintf(stderr, "ERROR: malformed block type specification \"%s\"\n", option_argument);
			ret = false;
		}
	}
    else if(0 == strcmp(opt, "data-format")) {
		op = append_major_operation(options, OP__DATA_FORMAT);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_data_format(option_argument, &(op->argument.data_format))) {
			fprintf(stderr, "ERROR: illegal data format \"%s\"\n", option_argument);
			ret = false;
		}
	}
    else if(0 == strcmp(opt, "application-data-format")) {
		op = append_major_operation(options, OP__APPLICATION_DATA_FORMAT);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_application_data_format(option_argument, &(op->argument.application_data_format))) {
			fprintf(stderr, "ERROR: illegal application data format \"%s\"\n", option_argument);
			ret = false;
		}
	}
    else if(0 == strcmp(opt, "from-file")) {
		op = append_major_operation(options, OP__FROM_FILE);
		FLAC__ASSERT(0 != option_argument);
		op->argument.from_file.file_name = local_strdup(option_argument);
	}
	else {
		FLAC__ASSERT(0);
	}

	return ret;
}

void free_options(CommandLineOptions *options)
{
	unsigned i;
	Operation *op;

	FLAC__ASSERT(0 == options->ops.operations || options->ops.num_operations > 0);

	for(i = 0; i < options->ops.num_operations; i++) {
		op = options->ops.operations + i;
		switch(op->type) {
			case OP__SHOW_VC_FIELD:
			case OP__REMOVE_VC_FIELD:
			case OP__REMOVE_VC_FIRSTFIELD:
				FLAC__ASSERT(0 != op->argument.show_vc_field.field_name);
				free(op->argument.show_vc_field.field_name);
				break;
			case OP__SET_VC_FIELD:
				FLAC__ASSERT(0 != op->argument.set_vc_field.field_name);
				free(op->argument.set_vc_field.field_name);
				if(0 != op->argument.set_vc_field.field_value)
					free(op->argument.set_vc_field.field_value);
				break;
			case OP__BLOCK_NUMBER:
				/*@@@*/
				break;
			case OP__BLOCK_TYPE:
			case OP__EXCEPT_BLOCK_TYPE:
				/*@@@*/
				break;
			case OP__FROM_FILE:
				FLAC__ASSERT(0 != op->argument.from_file.file_name);
				free(op->argument.from_file.file_name);
				break;
			default:
				break;
		}
	}

	if(0 != options->ops.operations)
		free(options->ops.operations);
}

void append_operation(CommandLineOptions *options, Operation operation)
{
	if(options->ops.capacity == 0) {
		options->ops.capacity = 50;
		if(0 == (options->ops.operations = malloc(sizeof(Operation) * options->ops.capacity))) {
			fprintf(stderr, "ERROR: out of memory allocating space for option list\n");
			exit(1);
		}
		memset(options->ops.operations, 0, sizeof(Operation) * options->ops.capacity);
	}
	if(options->ops.capacity <= options->ops.num_operations) {
		unsigned original_capacity = options->ops.capacity;
		options->ops.capacity *= 4;
		if(0 == (options->ops.operations = realloc(options->ops.operations, sizeof(Operation) * options->ops.capacity))) {
			fprintf(stderr, "ERROR: out of memory allocating space for option list\n");
			exit(1);
		}
		memset(options->ops.operations + original_capacity, 0, sizeof(Operation) * (options->ops.capacity - original_capacity));
	}

	options->ops.operations[options->ops.num_operations++] = operation;
}

Operation *append_major_operation(CommandLineOptions *options, OperationType type)
{
	Operation op;
	op.type = type;
	append_operation(options, op);
	options->checks.num_major_ops++;
	return options->ops.operations + (options->ops.num_operations - 1);
}

Operation *append_shorthand_operation(CommandLineOptions *options, OperationType type)
{
	Operation op;
	op.type = type;
	append_operation(options, op);
	options->checks.num_shorthand_ops++;
	return options->ops.operations + (options->ops.num_operations - 1);
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

char *local_strdup(const char *source)
{
	char *ret;
	FLAC__ASSERT(0 != source);
	if(0 == (ret = strdup(source))) {
		fprintf(stderr, "ERROR: out of memory during strdup()\n");
		exit(1);
	}
	return ret;
}

FLAC__bool parse_vorbis_comment_field(const char *field, char **name, char **value, unsigned *length)
{
	char *p, *s = local_strdup(field);

	if(0 == (p = strchr(s, '='))) {
		free(s);
		return false;
	}
	*p++ = '\0';
	*name = local_strdup(s);
	*value = local_strdup(p);
	*length = strlen(p);

	free(s);
	return true;
}

FLAC__bool parse_block_number(const char *in, Argument_BlockNumber *out)
{
	char *p, *q, *s;
	int i;
	unsigned entry;

	if(*in == '\0')
		return false;

	s = local_strdup(in);

	/* first count the entries */
	for(out->num_entries = 1, p = strchr(s, ','); p; out->num_entries++, p = strchr(s, ','))
		;

	/* make space */
	FLAC__ASSERT(out->num_entries > 0);
	if(0 == (out->entries = malloc(sizeof(unsigned) * out->num_entries))) {
		fprintf(stderr, "ERROR: out of memory allocating space for option list\n");
		exit(1);
	}

	/* load 'em up */
	entry = 0;
	q = s;
	while(q) {
		if(0 != (p = strchr(q, ',')))
			*p++ = 0;
		if(!isdigit((int)(*q)) || (i = atoi(q)) < 0) {
			free(s);
			return false;
		}
		FLAC__ASSERT(entry < out->num_entries);
		out->entries[entry++] = (unsigned)i;
		q = p;
	}
	FLAC__ASSERT(entry == out->num_entries);

	free(s);
	return true;
}

FLAC__bool parse_block_type(const char *in, Argument_BlockType *out)
{
	return true;
}

FLAC__bool parse_data_format(const char *in, Argument_DataFormat *out)
{
	if(0 == strcmp(in, "binary"))
		out->is_binary = true;
	else if(0 == strcmp(in, "text"))
		out->is_binary = false;
	else
		return false;
	return true;
}

FLAC__bool parse_application_data_format(const char *in, Argument_ApplicationDataFormat *out)
{
	if(0 == strcmp(in, "hexdump"))
		out->is_hexdump = true;
	else if(0 == strcmp(in, "text"))
		out->is_hexdump = false;
	else
		return false;
	return true;
}
