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
    { "add-padding", 1, 0, 0 },
	/* major operations */
    { "help", 0, 0, 0 },
    { "list", 0, 0, 0 },
    { "append", 0, 0, 0 },
    { "remove", 0, 0, 0 },
    { "remove-all", 0, 0, 0 },
    { "merge-padding", 0, 0, 0 },
    { "sort-padding", 0, 0, 0 },
	/* major operation arguments */
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
	OP__ADD_PADDING,
	OP__LIST,
	OP__APPEND,
	OP__REMOVE,
	OP__REMOVE_ALL,
	OP__MERGE_PADDING,
	OP__SORT_PADDING
} OperationType;

typedef enum {
	ARG__BLOCK_NUMBER,
	ARG__BLOCK_TYPE,
	ARG__EXCEPT_BLOCK_TYPE,
	ARG__DATA_FORMAT,
	ARG__FROM_FILE
} ArgumentType;

typedef struct {
	char *field_name;
} Argument_VcFieldName;

typedef struct {
	char *field; /* the whole field as passed on the command line, i.e. "NAME=VALUE" */
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
	FLAC__MetaDataType type;
	char application_id[4]; /* only relevant if type == FLAC__STREAM_METADATA_TYPE_APPLICATION */
	FLAC__bool filter_application_by_id;
} Argument_BlockTypeEntry;

typedef struct {
	unsigned num_entries;
	Argument_BlockTypeEntry *entries;
} Argument_BlockType;

typedef struct {
	FLAC__bool is_binary;
} Argument_DataFormat;

typedef struct {
	char *file_name;
} Argument_FromFile;

typedef struct {
	unsigned length;
} Argument_AddPadding;

typedef struct {
	OperationType type;
	union {
		Argument_VcFieldName show_vc_field;
		Argument_VcFieldName remove_vc_field;
		Argument_VcFieldName remove_vc_firstfield;
		Argument_VcField set_vc_field;
		Argument_AddPadding add_padding;
	} argument;
} Operation;

typedef struct {
	ArgumentType type;
	union {
		Argument_BlockNumber block_number;
		Argument_BlockType block_type;
		Argument_DataFormat data_format;
		Argument_FromFile from_file;
	} value;
} Argument;

typedef struct {
	FLAC__bool preserve_modtime;
	FLAC__bool prefix_with_filename;
	FLAC__bool use_padding;
	FLAC__bool show_long_help;
	FLAC__bool application_data_format_is_hexdump;
	struct {
		Operation *operations;
		unsigned num_operations;
		unsigned capacity;
	} ops;
	struct {
		struct {
			unsigned num_shorthand_ops;
			unsigned num_major_ops;
			FLAC__bool has_block_type;
			FLAC__bool has_except_block_type;
		} checks;
		Argument *arguments;
		unsigned num_arguments;
		unsigned capacity;
	} args;
	unsigned num_files;
	char **filenames;
} CommandLineOptions;

static void die(const char *message);
static void init_options(CommandLineOptions *options);
static FLAC__bool parse_options(int argc, char *argv[], CommandLineOptions *options);
static FLAC__bool parse_option(int option_index, const char *option_argument, CommandLineOptions *options);
static void free_options(CommandLineOptions *options);
static void append_new_operation(CommandLineOptions *options, Operation operation);
static void append_new_argument(CommandLineOptions *options, Argument argument);
static Operation *append_major_operation(CommandLineOptions *options, OperationType type);
static Operation *append_shorthand_operation(CommandLineOptions *options, OperationType type);
static Argument *append_argument(CommandLineOptions *options, ArgumentType type);
static int short_usage(const char *message, ...);
static int long_usage(const char *message, ...);
static char *local_strdup(const char *source);
static FLAC__bool parse_vorbis_comment_field(const char *field_ref, char **field, char **name, char **value, unsigned *length);
static FLAC__bool parse_add_padding(const char *in, unsigned *out);
static FLAC__bool parse_block_number(const char *in, Argument_BlockNumber *out);
static FLAC__bool parse_block_type(const char *in, Argument_BlockType *out);
static FLAC__bool parse_data_format(const char *in, Argument_DataFormat *out);
static FLAC__bool parse_application_data_format(const char *in, FLAC__bool *out);
static FLAC__bool do_operations(const CommandLineOptions *options);
static FLAC__bool do_major_operation(const CommandLineOptions *options);
static FLAC__bool do_major_operation_on_file(const char *filename, const CommandLineOptions *options);
static FLAC__bool do_major_operation__list(const char *filename, FLAC__MetaData_Chain *chain, const CommandLineOptions *options);
static FLAC__bool do_major_operation__append(FLAC__MetaData_Chain *chain, const CommandLineOptions *options);
static FLAC__bool do_major_operation__remove(FLAC__MetaData_Chain *chain, const CommandLineOptions *options);
static FLAC__bool do_major_operation__remove_all(FLAC__MetaData_Chain *chain, const CommandLineOptions *options);
static FLAC__bool do_shorthand_operations(const CommandLineOptions *options);
static FLAC__bool do_shorthand_operations_on_file(const char *fielname, const CommandLineOptions *options);
static FLAC__bool do_shorthand_operation(const char *filename, FLAC__MetaData_Chain *chain, const Operation *operation, FLAC__bool *needs_write);
static FLAC__bool do_shorthand_operation__add_padding(FLAC__MetaData_Chain *chain, unsigned length, FLAC__bool *needs_write);
static FLAC__bool do_shorthand_operation__streaminfo(const char *filename, FLAC__MetaData_Chain *chain, OperationType op);
static FLAC__bool do_shorthand_operation__vorbis_comment(const char *filename, FLAC__MetaData_Chain *chain, const Operation *operation, FLAC__bool *needs_write);
static FLAC__bool passes_filter(const CommandLineOptions *options, const FLAC__StreamMetaData *block, unsigned block_number);
static void write_metadata(const char *filename, FLAC__StreamMetaData *block, unsigned block_number, FLAC__bool hexdump_application);
static void write_vc_field(const char *filename, const FLAC__StreamMetaData_VorbisComment_Entry *entry);
static void write_vc_fields(const char *filename, const char *field_name, const FLAC__StreamMetaData_VorbisComment_Entry entry[], unsigned num_entries);
static FLAC__bool remove_vc_all(FLAC__StreamMetaData *block, FLAC__bool *needs_write);
static FLAC__bool remove_vc_field(FLAC__StreamMetaData *block, const char *field_name, FLAC__bool *needs_write);
static FLAC__bool remove_vc_firstfield(FLAC__StreamMetaData *block, const char *field_name, FLAC__bool *needs_write);
static FLAC__bool set_vc_field(FLAC__StreamMetaData *block, const Argument_VcField *field, FLAC__bool *needs_write);
static FLAC__bool field_name_matches_entry(const char *field_name, unsigned field_name_length, const FLAC__StreamMetaData_VorbisComment_Entry *entry);
static void hexdump(const char *filename, const FLAC__byte *buf, unsigned bytes, const char *indent);

int main(int argc, char *argv[])
{
	CommandLineOptions options;
	int ret = 0;

	init_options(&options);

	if(parse_options(argc, argv, &options))
		ret = !do_operations(&options);

	free_options(&options);

	return ret;
}

void die(const char *message)
{
	FLAC__ASSERT(0 != message);
	fprintf(stderr, "ERROR: %s\n", message);
	exit(1);
}

void init_options(CommandLineOptions *options)
{
	options->preserve_modtime = false;

	/* '2' is hack to mean "use default if not forced on command line" */
	FLAC__ASSERT(true != 2);
	options->prefix_with_filename = 2;

	options->use_padding = true;
	options->show_long_help = false;
	options->application_data_format_is_hexdump = false;

	options->ops.operations = 0;
	options->ops.num_operations = 0;
	options->ops.capacity = 0;

	options->args.arguments = 0;
	options->args.num_arguments = 0;
	options->args.capacity = 0;

	options->args.checks.num_shorthand_ops = 0;
	options->args.checks.num_major_ops = 0;
	options->args.checks.has_block_type = false;
	options->args.checks.has_except_block_type = false;

	options->num_files = 0;
	options->filenames = 0;
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
                had_error = true;
                break;
            default:
				FLAC__ASSERT(0);
				break;
        }
    }

	if(options->prefix_with_filename == 2)
		options->prefix_with_filename = (argc - optind > 1);

	if(optind >= argc && !options->show_long_help) {
		fprintf(stderr,"ERROR: you must specify at least one FLAC file;\n");
		fprintf(stderr,"       metaflac cannot be used as a pipe\n");
		had_error = true;
	}

	options->num_files = argc - optind;

	if(options->num_files > 0) {
		unsigned i = 0;
		if(0 == (options->filenames = malloc(sizeof(char *) * options->num_files)))
			die("out of memory allocating space for file names list");
		while(optind < argc)
			options->filenames[i++] = local_strdup(argv[optind++]);
	}

	if(options->args.checks.num_major_ops > 0) {
		if(options->args.checks.num_major_ops > 1) {
			fprintf(stderr, "ERROR: you may only specify one major operation at a time\n");
			had_error = true;
		}
		else if(options->args.checks.num_shorthand_ops > 0) {
			fprintf(stderr, "ERROR: you may not mix shorthand and major operations\n");
			had_error = true;
		}
	}

	if(options->args.checks.has_block_type && options->args.checks.has_except_block_type) {
		fprintf(stderr, "ERROR: you may not specify both '--block-type' and '--except-block-type'\n");
		had_error = true;
	}

	if(had_error)
		short_usage(0);

	return !had_error;
}

FLAC__bool parse_option(int option_index, const char *option_argument, CommandLineOptions *options)
{
	const char *opt = long_options_[option_index].name;
	Operation *op;
	Argument *arg;
	FLAC__bool ok = true;

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
		if(!parse_vorbis_comment_field(option_argument, &(op->argument.set_vc_field.field), &(op->argument.set_vc_field.field_name), &(op->argument.set_vc_field.field_value), &(op->argument.set_vc_field.field_value_length))) {
			fprintf(stderr, "ERROR: malformed vorbis comment field \"%s\"\n", option_argument);
			ok = false;
		}
	}
    else if(0 == strcmp(opt, "add-padding")) {
		op = append_shorthand_operation(options, OP__ADD_PADDING);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_add_padding(option_argument, &(op->argument.add_padding.length))) {
			fprintf(stderr, "ERROR: illegal length \"%s\", length must be >= 0 and < 2^%u\n", option_argument, FLAC__STREAM_METADATA_LENGTH_LEN);
			ok = false;
		}
	}
    else if(0 == strcmp(opt, "help")) {
		options->show_long_help = true;
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
		arg = append_argument(options, ARG__BLOCK_NUMBER);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_block_number(option_argument, &(arg->value.block_number))) {
			fprintf(stderr, "ERROR: malformed block number specification \"%s\"\n", option_argument);
			ok = false;
		}
	}
    else if(0 == strcmp(opt, "block-type")) {
		arg = append_argument(options, ARG__BLOCK_TYPE);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_block_type(option_argument, &(arg->value.block_type))) {
			fprintf(stderr, "ERROR: malformed block type specification \"%s\"\n", option_argument);
			ok = false;
		}
		options->args.checks.has_block_type = true;
	}
    else if(0 == strcmp(opt, "except-block-type")) {
		arg = append_argument(options, ARG__EXCEPT_BLOCK_TYPE);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_block_type(option_argument, &(arg->value.block_type))) {
			fprintf(stderr, "ERROR: malformed block type specification \"%s\"\n", option_argument);
			ok = false;
		}
		options->args.checks.has_except_block_type = true;
	}
    else if(0 == strcmp(opt, "data-format")) {
		arg = append_argument(options, ARG__DATA_FORMAT);
		FLAC__ASSERT(0 != option_argument);
		if(!parse_data_format(option_argument, &(arg->value.data_format))) {
			fprintf(stderr, "ERROR: illegal data format \"%s\"\n", option_argument);
			ok = false;
		}
	}
    else if(0 == strcmp(opt, "application-data-format")) {
		FLAC__ASSERT(0 != option_argument);
		if(!parse_application_data_format(option_argument, &(options->application_data_format_is_hexdump))) {
			fprintf(stderr, "ERROR: illegal application data format \"%s\"\n", option_argument);
			ok = false;
		}
	}
    else if(0 == strcmp(opt, "from-file")) {
		arg = append_argument(options, ARG__FROM_FILE);
		FLAC__ASSERT(0 != option_argument);
		arg->value.from_file.file_name = local_strdup(option_argument);
	}
	else {
		FLAC__ASSERT(0);
	}

	return ok;
}

void free_options(CommandLineOptions *options)
{
	unsigned i;
	Operation *op;
	Argument *arg;

	FLAC__ASSERT(0 == options->ops.operations || options->ops.num_operations > 0);
	FLAC__ASSERT(0 == options->args.arguments || options->args.num_arguments > 0);

	for(i = 0, op = options->ops.operations; i < options->ops.num_operations; i++, op++) {
		switch(op->type) {
			case OP__SHOW_VC_FIELD:
			case OP__REMOVE_VC_FIELD:
			case OP__REMOVE_VC_FIRSTFIELD:
				if(0 != op->argument.show_vc_field.field_name)
					free(op->argument.show_vc_field.field_name);
				break;
			case OP__SET_VC_FIELD:
				if(0 != op->argument.set_vc_field.field)
					free(op->argument.set_vc_field.field);
				if(0 != op->argument.set_vc_field.field_name)
					free(op->argument.set_vc_field.field_name);
				if(0 != op->argument.set_vc_field.field_value)
					free(op->argument.set_vc_field.field_value);
				break;
			default:
				break;
		}
	}

	for(i = 0, arg = options->args.arguments; i < options->args.num_arguments; i++, arg++) {
		switch(arg->type) {
			case ARG__BLOCK_NUMBER:
				if(0 != arg->value.block_number.entries)
					free(arg->value.block_number.entries);
				break;
			case ARG__BLOCK_TYPE:
			case ARG__EXCEPT_BLOCK_TYPE:
				if(0 != arg->value.block_type.entries)
					free(arg->value.block_type.entries);
				break;
			case ARG__FROM_FILE:
				if(0 != arg->value.from_file.file_name)
					free(arg->value.from_file.file_name);
				break;
			default:
				break;
		}
	}

	if(0 != options->ops.operations)
		free(options->ops.operations);

	if(0 != options->args.arguments)
		free(options->args.arguments);

	if(0 != options->filenames)
		free(options->filenames);
}

void append_new_operation(CommandLineOptions *options, Operation operation)
{
	if(options->ops.capacity == 0) {
		options->ops.capacity = 50;
		if(0 == (options->ops.operations = malloc(sizeof(Operation) * options->ops.capacity)))
			die("out of memory allocating space for option list");
		memset(options->ops.operations, 0, sizeof(Operation) * options->ops.capacity);
	}
	if(options->ops.capacity <= options->ops.num_operations) {
		unsigned original_capacity = options->ops.capacity;
		options->ops.capacity *= 4;
		if(0 == (options->ops.operations = realloc(options->ops.operations, sizeof(Operation) * options->ops.capacity)))
			die("out of memory allocating space for option list");
		memset(options->ops.operations + original_capacity, 0, sizeof(Operation) * (options->ops.capacity - original_capacity));
	}

	options->ops.operations[options->ops.num_operations++] = operation;
}

void append_new_argument(CommandLineOptions *options, Argument argument)
{
	if(options->args.capacity == 0) {
		options->args.capacity = 50;
		if(0 == (options->args.arguments = malloc(sizeof(Argument) * options->args.capacity)))
			die("out of memory allocating space for option list");
		memset(options->args.arguments, 0, sizeof(Argument) * options->args.capacity);
	}
	if(options->args.capacity <= options->args.num_arguments) {
		unsigned original_capacity = options->args.capacity;
		options->args.capacity *= 4;
		if(0 == (options->args.arguments = realloc(options->args.arguments, sizeof(Argument) * options->args.capacity)))
			die("out of memory allocating space for option list");
		memset(options->args.arguments + original_capacity, 0, sizeof(Argument) * (options->args.capacity - original_capacity));
	}

	options->args.arguments[options->args.num_arguments++] = argument;
}

Operation *append_major_operation(CommandLineOptions *options, OperationType type)
{
	Operation op;
	memset(&op, 0, sizeof(op));
	op.type = type;
	append_new_operation(options, op);
	options->args.checks.num_major_ops++;
	return options->ops.operations + (options->ops.num_operations - 1);
}

Operation *append_shorthand_operation(CommandLineOptions *options, OperationType type)
{
	Operation op;
	memset(&op, 0, sizeof(op));
	op.type = type;
	append_new_operation(options, op);
	options->args.checks.num_shorthand_ops++;
	return options->ops.operations + (options->ops.num_operations - 1);
}

Argument *append_argument(CommandLineOptions *options, ArgumentType type)
{
	Argument arg;
	memset(&arg, 0, sizeof(arg));
	arg.type = type;
	append_new_argument(options, arg);
	return options->args.arguments + (options->args.num_arguments - 1);
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
    fprintf(out, "                      changes.  Use this option to tell metaflac to not take\n");
    fprintf(out, "                      advantage of padding this way.\n");
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
    fprintf(out, "--add-padding=length  Add a padding block of the given length (in bytes).\n");
    fprintf(out, "                      The overall length of the new block will be 4 + length;\n");
    fprintf(out, "                      the extra 4 bytes is for the metadata block header.\n");
    fprintf(out, "\n");
    fprintf(out, "Major operations:\n");
    fprintf(out, "--list\n");
    fprintf(out, "    List the contents of one or more metadata blocks to stdout.  By default,\n");
    fprintf(out, "    all metadata blocks are listed in text format.  Use the following options\n");
    fprintf(out, "    to change this behavior:\n");
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
    fprintf(out, "\n");
#endif
    fprintf(out, "    --application-data-format=hexdump|text\n");
    fprintf(out, "    If the application block you are displaying contains binary data but your\n");
    fprintf(out, "    --data-format=text, you can display a hex dump of the application data\n");
    fprintf(out, "    contents instead using --application-data-format=hexdump\n");
    fprintf(out, "\n");
#if 0
	/*@@@ not implemented yet */
    fprintf(out, "--append\n");
    fprintf(out, "    Insert a metadata block from a file.  The input file must be in the same\n");
    fprintf(out, "    format as generated with --list.\n");
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
    fprintf(out, "--remove\n");
    fprintf(out, "    Remove one or more metadata blocks from the metadata.  Unless\n");
    fprintf(out, "    --dont-use-padding is specified, the blocks will be replaced with padding.\n");
    fprintf(out, "    You may not remove the STREAMINFO block.\n");
    fprintf(out, "\n");
    fprintf(out, "    --block-number=#[,#[...]]\n");
    fprintf(out, "    --block-type=type[,type[...]]\n");
    fprintf(out, "    --except-block-type=type[,type[...]]\n");
    fprintf(out, "    See --list above for usage.\n");
    fprintf(out, "\n");
    fprintf(out, "    NOTE: if both --block-number and --[except-]block-type are specified,\n");
    fprintf(out, "          the result is the logical AND of both arguments.\n");
    fprintf(out, "\n");
    fprintf(out, "--remove-all\n");
    fprintf(out, "    Remove all metadata blocks (except the STREAMINFO block) from the\n");
    fprintf(out, "    metadata.  Unless --dont-use-padding is specified, the blocks will be\n");
    fprintf(out, "    replaced with padding.\n");
    fprintf(out, "\n");
    fprintf(out, "--merge-padding\n");
    fprintf(out, "    Merge adjacent PADDING blocks into single blocks.\n");
    fprintf(out, "\n");
    fprintf(out, "--sort-padding\n");
    fprintf(out, "    Move all PADDING blocks to the end of the metadata and merge them into a\n");
    fprintf(out, "    single block.\n");

	return message? 1 : 0;
}

char *local_strdup(const char *source)
{
	char *ret;
	FLAC__ASSERT(0 != source);
	if(0 == (ret = strdup(source)))
		die("out of memory during strdup()");
	return ret;
}

FLAC__bool parse_vorbis_comment_field(const char *field_ref, char **field, char **name, char **value, unsigned *length)
{
	char *p, *s;

	if(0 != field)
		*field = local_strdup(field_ref);

	s = local_strdup(field_ref);

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

FLAC__bool parse_add_padding(const char *in, unsigned *out)
{
	*out = (unsigned)strtoul(in, 0, 10);
	return *out < (1u << FLAC__STREAM_METADATA_LENGTH_LEN);
}

FLAC__bool parse_block_number(const char *in, Argument_BlockNumber *out)
{
	char *p, *q, *s, *end;
	long i;
	unsigned entry;

	if(*in == '\0')
		return false;

	s = local_strdup(in);

	/* first count the entries */
	for(out->num_entries = 1, p = strchr(s, ','); p; out->num_entries++, p = strchr(++p, ','))
		;

	/* make space */
	FLAC__ASSERT(out->num_entries > 0);
	if(0 == (out->entries = malloc(sizeof(unsigned) * out->num_entries)))
		die("out of memory allocating space for option list");

	/* load 'em up */
	entry = 0;
	q = s;
	while(q) {
		FLAC__ASSERT(entry < out->num_entries);
		if(0 != (p = strchr(q, ',')))
			*p++ = '\0';
		if(!isdigit((int)(*q)) || (i = strtol(q, &end, 10)) < 0 || *end) {
			free(s);
			return false;
		}
		out->entries[entry++] = (unsigned)i;
		q = p;
	}
	FLAC__ASSERT(entry == out->num_entries);

	free(s);
	return true;
}

FLAC__bool parse_block_type(const char *in, Argument_BlockType *out)
{
	char *p, *q, *r, *s;
	unsigned entry;

	if(*in == '\0')
		return false;

	s = local_strdup(in);

	/* first count the entries */
	for(out->num_entries = 1, p = strchr(s, ','); p; out->num_entries++, p = strchr(++p, ','))
		;

	/* make space */
	FLAC__ASSERT(out->num_entries > 0);
	if(0 == (out->entries = malloc(sizeof(Argument_BlockTypeEntry) * out->num_entries)))
		die("out of memory allocating space for option list");

	/* load 'em up */
	entry = 0;
	q = s;
	while(q) {
		FLAC__ASSERT(entry < out->num_entries);
		if(0 != (p = strchr(q, ',')))
			*p++ = 0;
		r = strchr(q, ':');
		if(r)
			*r++ = '\0';
		if(0 != r && 0 != strcmp(q, "APPLICATION")) {
			free(s);
			return false;
		}
		if(0 == strcmp(q, "STREAMINFO")) {
			out->entries[entry++].type = FLAC__METADATA_TYPE_STREAMINFO;
		}
		else if(0 == strcmp(q, "PADDING")) {
			out->entries[entry++].type = FLAC__METADATA_TYPE_PADDING;
		}
		else if(0 == strcmp(q, "APPLICATION")) {
			out->entries[entry].type = FLAC__METADATA_TYPE_APPLICATION;
			out->entries[entry].filter_application_by_id = (0 != r);
			if(0 != r) {
				if(strlen(r) == 4) {
					strcpy(out->entries[entry].application_id, r);
				}
				else if(strlen(r) == 10 && strncmp(r, "0x", 2) == 0 && strspn(r+2, "0123456789ABCDEFabcdef") == 8) {
					FLAC__uint32 x = strtoul(r+2, 0, 16);
					out->entries[entry].application_id[3] = (FLAC__byte)(x & 0xff);
					out->entries[entry].application_id[2] = (FLAC__byte)((x>>=8) & 0xff);
					out->entries[entry].application_id[1] = (FLAC__byte)((x>>=8) & 0xff);
					out->entries[entry].application_id[0] = (FLAC__byte)((x>>=8) & 0xff);
				}
				else {
					free(s);
					return false;
				}
			}
			entry++;
		}
		else if(0 == strcmp(q, "SEEKTABLE")) {
			out->entries[entry++].type = FLAC__METADATA_TYPE_SEEKTABLE;
		}
		else if(0 == strcmp(q, "VORBIS_COMMENT")) {
			out->entries[entry++].type = FLAC__METADATA_TYPE_VORBIS_COMMENT;
		}
		else {
			free(s);
			return false;
		}
		q = p;
	}
	FLAC__ASSERT(entry == out->num_entries);

	free(s);
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

FLAC__bool parse_application_data_format(const char *in, FLAC__bool *out)
{
	if(0 == strcmp(in, "hexdump"))
		*out = true;
	else if(0 == strcmp(in, "text"))
		*out = false;
	else
		return false;
	return true;
}

FLAC__bool do_operations(const CommandLineOptions *options)
{
	FLAC__bool ok = true;

	if(options->show_long_help) {
		long_usage(0);
	}
	else if(options->args.checks.num_major_ops > 0) {
		FLAC__ASSERT(options->args.checks.num_shorthand_ops == 0);
		FLAC__ASSERT(options->args.checks.num_major_ops == 1);
		FLAC__ASSERT(options->args.checks.num_major_ops == options->ops.num_operations);
		ok = do_major_operation(options);
	}
	else if(options->args.checks.num_shorthand_ops > 0) {
		FLAC__ASSERT(options->args.checks.num_shorthand_ops == options->ops.num_operations);
		ok = do_shorthand_operations(options);
	}

	return ok;
}

FLAC__bool do_major_operation(const CommandLineOptions *options)
{
	unsigned i;
	FLAC__bool ok = true;

	/*@@@ to die after first error,  v---  add '&& ok' here */
	for(i = 0; i < options->num_files; i++)
		ok &= do_major_operation_on_file(options->filenames[i], options);

	return ok;
}

FLAC__bool do_major_operation_on_file(const char *filename, const CommandLineOptions *options)
{
	FLAC__bool ok = true, needs_write = false;
	FLAC__MetaData_Chain *chain = FLAC__metadata_chain_new();

	if(0 == chain)
		die("out of memory allocating chain");

	if(!FLAC__metadata_chain_read(chain, filename)) {
		fprintf(stderr, "ERROR: reading metadata, status = \"%s\"\n", FLAC__MetaData_ChainStatusString[FLAC__metadata_chain_status(chain)]);
		return false;
	}

	switch(options->ops.operations[0].type) {
		case OP__LIST:
			ok = do_major_operation__list(options->prefix_with_filename? filename : 0, chain, options);
			break;
		case OP__APPEND:
			ok = do_major_operation__append(chain, options);
			needs_write = true;
			break;
		case OP__REMOVE:
			ok = do_major_operation__remove(chain, options);
			needs_write = true;
			break;
		case OP__REMOVE_ALL:
			ok = do_major_operation__remove_all(chain, options);
			needs_write = true;
			break;
		case OP__MERGE_PADDING:
			FLAC__metadata_chain_merge_padding(chain);
			needs_write = true;
			break;
		case OP__SORT_PADDING:
			FLAC__metadata_chain_sort_padding(chain);
			needs_write = true;
			break;
		default:
			FLAC__ASSERT(0);
			return false;
	}

	if(ok && needs_write) {
		if(options->use_padding)
			FLAC__metadata_chain_sort_padding(chain);
		ok = FLAC__metadata_chain_write(chain, options->use_padding, options->preserve_modtime);
		if(!ok)
			fprintf(stderr, "ERROR: writing FLAC file %s, error = %s\n", filename, FLAC__MetaData_ChainStatusString[FLAC__metadata_chain_status(chain)]);
	}

	FLAC__metadata_chain_delete(chain);

	return ok;
}

FLAC__bool do_major_operation__list(const char *filename, FLAC__MetaData_Chain *chain, const CommandLineOptions *options)
{
	FLAC__MetaData_Iterator *iterator = FLAC__metadata_iterator_new();
	FLAC__StreamMetaData *block;
	FLAC__bool ok = true;
	unsigned block_number;

	if(0 == iterator)
		die("out of memory allocating iterator");

	FLAC__metadata_iterator_init(iterator, chain);

	block_number = 0;
	do {
		block = FLAC__metadata_iterator_get_block(iterator);
		ok &= (0 != block);
		if(!ok)
			fprintf(stderr, "ERROR: couldn't get block from chain\n");
		else if(passes_filter(options, FLAC__metadata_iterator_get_block(iterator), block_number))
			write_metadata(filename, block, block_number, options->application_data_format_is_hexdump);
		block_number++;
	} while(ok && FLAC__metadata_iterator_next(iterator));

	FLAC__metadata_iterator_delete(iterator);

	return ok;
}

FLAC__bool do_major_operation__append(FLAC__MetaData_Chain *chain, const CommandLineOptions *options)
{
	(void) chain, (void) options;
	fprintf(stderr, "ERROR: --append not implemented yet\n"); /*@@@*/
	return false;
}

FLAC__bool do_major_operation__remove(FLAC__MetaData_Chain *chain, const CommandLineOptions *options)
{
	FLAC__MetaData_Iterator *iterator = FLAC__metadata_iterator_new();
	FLAC__bool ok = true;
	unsigned block_number;

	if(0 == iterator)
		die("out of memory allocating iterator");

	FLAC__metadata_iterator_init(iterator, chain);

	block_number = 0;
	while(ok && FLAC__metadata_iterator_next(iterator)) {
		block_number++;
		if(passes_filter(options, FLAC__metadata_iterator_get_block(iterator), block_number))
			ok &= FLAC__metadata_iterator_delete_block(iterator, options->use_padding);
	}

	FLAC__metadata_iterator_delete(iterator);

	return ok;
}

FLAC__bool do_major_operation__remove_all(FLAC__MetaData_Chain *chain, const CommandLineOptions *options)
{
	FLAC__MetaData_Iterator *iterator = FLAC__metadata_iterator_new();
	FLAC__bool ok = true;

	if(0 == iterator)
		die("out of memory allocating iterator");

	FLAC__metadata_iterator_init(iterator, chain);

	while(ok && FLAC__metadata_iterator_next(iterator))
		ok &= FLAC__metadata_iterator_delete_block(iterator, options->use_padding);

	FLAC__metadata_iterator_delete(iterator);

	return ok;
}

FLAC__bool do_shorthand_operations(const CommandLineOptions *options)
{
	unsigned i;
	FLAC__bool ok = true;

	/*@@@ to die after first error,  v---  add '&& ok' here */
	for(i = 0; i < options->num_files; i++)
		ok &= do_shorthand_operations_on_file(options->filenames[i], options);

	return ok;
}

FLAC__bool do_shorthand_operations_on_file(const char *filename, const CommandLineOptions *options)
{
	unsigned i;
	FLAC__bool ok = true, needs_write = false;
	FLAC__MetaData_Chain *chain = FLAC__metadata_chain_new();

	if(0 == chain)
		die("out of memory allocating chain");

	if(!FLAC__metadata_chain_read(chain, filename)) {
		fprintf(stderr, "ERROR: reading metadata, status = \"%s\"\n", FLAC__MetaData_ChainStatusString[FLAC__metadata_chain_status(chain)]);
		return false;
	}

	for(i = 0; i < options->ops.num_operations && ok; i++)
		ok &= do_shorthand_operation(options->prefix_with_filename? filename : 0, chain, &options->ops.operations[i], &needs_write);

	if(ok) {
		if(options->use_padding)
			FLAC__metadata_chain_sort_padding(chain);
		ok = FLAC__metadata_chain_write(chain, options->use_padding, options->preserve_modtime);
		if(!ok)
			fprintf(stderr, "ERROR: writing FLAC file %s, error = %s\n", filename, FLAC__MetaData_ChainStatusString[FLAC__metadata_chain_status(chain)]);
	}

	FLAC__metadata_chain_delete(chain);

	return ok;
}

FLAC__bool do_shorthand_operation(const char *filename, FLAC__MetaData_Chain *chain, const Operation *operation, FLAC__bool *needs_write)
{
	FLAC__bool ok = true;

	switch(operation->type) {
		case OP__SHOW_MD5SUM:
		case OP__SHOW_MIN_BLOCKSIZE:
		case OP__SHOW_MAX_BLOCKSIZE:
		case OP__SHOW_MIN_FRAMESIZE:
		case OP__SHOW_MAX_FRAMESIZE:
		case OP__SHOW_SAMPLE_RATE:
		case OP__SHOW_CHANNELS:
		case OP__SHOW_BPS:
		case OP__SHOW_TOTAL_SAMPLES:
			ok = do_shorthand_operation__streaminfo(filename, chain, operation->type);
			break;
		case OP__SHOW_VC_VENDOR:
		case OP__SHOW_VC_FIELD:
		case OP__REMOVE_VC_ALL:
		case OP__REMOVE_VC_FIELD:
		case OP__REMOVE_VC_FIRSTFIELD:
		case OP__SET_VC_FIELD:
			ok = do_shorthand_operation__vorbis_comment(filename, chain, operation, needs_write);
			break;
		case OP__ADD_PADDING:
			ok = do_shorthand_operation__add_padding(chain, operation->argument.add_padding.length, needs_write);
			break;
		default:
			ok = false;
			FLAC__ASSERT(0);
			break;
	};

	return ok;
}

FLAC__bool do_shorthand_operation__add_padding(FLAC__MetaData_Chain *chain, unsigned length, FLAC__bool *needs_write)
{
	FLAC__StreamMetaData *padding = 0;
	FLAC__MetaData_Iterator *iterator = FLAC__metadata_iterator_new();

	if(0 == iterator)
		die("out of memory allocating iterator");

	FLAC__metadata_iterator_init(iterator, chain);

	while(FLAC__metadata_iterator_next(iterator))
		;

	padding = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);
	if(0 == padding)
		die("out of memory allocating PADDING block");

	padding->length = length;

	if(!FLAC__metadata_iterator_insert_block_after(iterator, padding)) {
		fprintf(stderr, "ERROR: adding new PADDING block to metadata, status =\"%s\"\n", FLAC__MetaData_ChainStatusString[FLAC__metadata_chain_status(chain)]);
		FLAC__metadata_object_delete(padding);
		return false;
	}

	*needs_write = true;
	return true;
}

FLAC__bool do_shorthand_operation__streaminfo(const char *filename, FLAC__MetaData_Chain *chain, OperationType op)
{
	unsigned i;
	FLAC__bool ok = true;
	FLAC__StreamMetaData *block;
	FLAC__MetaData_Iterator *iterator = FLAC__metadata_iterator_new();

	if(0 == iterator)
		die("out of memory allocating iterator");

	FLAC__metadata_iterator_init(iterator, chain);

	block = FLAC__metadata_iterator_get_block(iterator);

	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_STREAMINFO);

	if(0 != filename)
		printf("%s:", filename);

	switch(op) {
		case OP__SHOW_MD5SUM:
			for(i = 0; i < 16; i++)
				printf("%02x", block->data.stream_info.md5sum[i]);
			printf("\n");
			break;
		case OP__SHOW_MIN_BLOCKSIZE:
			printf("%u\n", block->data.stream_info.min_blocksize);
			break;
		case OP__SHOW_MAX_BLOCKSIZE:
			printf("%u\n", block->data.stream_info.max_blocksize);
			break;
		case OP__SHOW_MIN_FRAMESIZE:
			printf("%u\n", block->data.stream_info.min_framesize);
			break;
		case OP__SHOW_MAX_FRAMESIZE:
			printf("%u\n", block->data.stream_info.max_framesize);
			break;
		case OP__SHOW_SAMPLE_RATE:
			printf("%u\n", block->data.stream_info.sample_rate);
			break;
		case OP__SHOW_CHANNELS:
			printf("%u\n", block->data.stream_info.channels);
			break;
		case OP__SHOW_BPS:
			printf("%u\n", block->data.stream_info.bits_per_sample);
			break;
		case OP__SHOW_TOTAL_SAMPLES:
			printf("%llu\n", block->data.stream_info.total_samples);
			break;
		default:
			ok = false;
			FLAC__ASSERT(0);
			break;
	};

	FLAC__metadata_iterator_delete(iterator);

	return ok;
}

FLAC__bool do_shorthand_operation__vorbis_comment(const char *filename, FLAC__MetaData_Chain *chain, const Operation *operation, FLAC__bool *needs_write)
{
	FLAC__bool ok = true, found_vc_block = false;
	FLAC__StreamMetaData *block = 0;
	FLAC__MetaData_Iterator *iterator = FLAC__metadata_iterator_new();

	if(0 == iterator)
		die("out of memory allocating iterator");

	FLAC__metadata_iterator_init(iterator, chain);

	do {
		block = FLAC__metadata_iterator_get_block(iterator);
		if(block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
			found_vc_block = true;
	} while(!found_vc_block && FLAC__metadata_iterator_next(iterator));

	/* create a new block if necessary */
	if(!found_vc_block && operation->type == OP__SET_VC_FIELD) {
		block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
		if(0 == block)
			die("out of memory allocating VORBIS_COMMENT block");
		while(FLAC__metadata_iterator_next(iterator))
			;
		if(!FLAC__metadata_iterator_insert_block_after(iterator, block)) {
			fprintf(stderr, "ERROR: adding new VORBIS_COMMENT block to metadata, status =\"%s\"\n", FLAC__MetaData_ChainStatusString[FLAC__metadata_chain_status(chain)]);
			return false;
		}
		/* iterator is left pointing to new block */
		FLAC__ASSERT(FLAC__metadata_iterator_get_block(iterator) == block);
	}

	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);

	switch(operation->type) {
		case OP__SHOW_VC_VENDOR:
			write_vc_field(filename, &block->data.vorbis_comment.vendor_string);
			break;
		case OP__SHOW_VC_FIELD:
			write_vc_fields(filename, operation->argument.show_vc_field.field_name, block->data.vorbis_comment.comments, block->data.vorbis_comment.num_comments);
			break;
		case OP__REMOVE_VC_ALL:
			ok = remove_vc_all(block, needs_write);
			break;
		case OP__REMOVE_VC_FIELD:
			ok = remove_vc_field(block, operation->argument.remove_vc_field.field_name, needs_write);
			break;
		case OP__REMOVE_VC_FIRSTFIELD:
			ok = remove_vc_firstfield(block, operation->argument.remove_vc_firstfield.field_name, needs_write);
			break;
		case OP__SET_VC_FIELD:
			ok = set_vc_field(block, &operation->argument.set_vc_field, needs_write);
			break;
		default:
			ok = false;
			FLAC__ASSERT(0);
			break;
	};

	return ok;
}

FLAC__bool passes_filter(const CommandLineOptions *options, const FLAC__StreamMetaData *block, unsigned block_number)
{
	unsigned i, j;
	FLAC__bool matches_number = false, matches_type = false;
	FLAC__bool has_block_number_arg = false;

	for(i = 0; i < options->args.num_arguments; i++) {
		if(options->args.arguments[i].type == ARG__BLOCK_TYPE || options->args.arguments[i].type == ARG__EXCEPT_BLOCK_TYPE) {
			for(j = 0; j < options->args.arguments[i].value.block_type.num_entries; j++) {
				if(options->args.arguments[i].value.block_type.entries[j].type == block->type) {
					if(block->type != FLAC__METADATA_TYPE_APPLICATION || !options->args.arguments[i].value.block_type.entries[j].filter_application_by_id || 0 == memcmp(options->args.arguments[i].value.block_type.entries[j].application_id, block->data.application.id, FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8))
						matches_type = true;
				}
			}
		}
		else if(options->args.arguments[i].type == ARG__BLOCK_NUMBER) {
			has_block_number_arg = true;
			for(j = 0; j < options->args.arguments[i].value.block_number.num_entries; j++) {
				if(options->args.arguments[i].value.block_number.entries[j] == block_number)
					matches_number = true;
			}
		}
	}

	if(!has_block_number_arg)
		matches_number = true;

	if(options->args.checks.has_block_type) {
		FLAC__ASSERT(!options->args.checks.has_except_block_type);
	}
	else if(options->args.checks.has_except_block_type)
		matches_type = !matches_type;
	else
		matches_type = true;

	return matches_number && matches_type;
}

void write_metadata(const char *filename, FLAC__StreamMetaData *block, unsigned block_number, FLAC__bool hexdump_application)
{
	unsigned i;

/*@@@ yuck, should do this with a varargs function or something: */
#define PPR if(filename)printf("%s:",filename);
	PPR; printf("METADATA block #%u\n", block_number);
	PPR; printf("  type: %u (%s)\n", (unsigned)block->type, block->type<=FLAC__METADATA_TYPE_VORBIS_COMMENT? FLAC__MetaDataTypeString[block->type] : "UNKNOWN");
	PPR; printf("  is last: %s\n", block->is_last? "true":"false");
	PPR; printf("  length: %u\n", block->length);

	switch(block->type) {
		case FLAC__METADATA_TYPE_STREAMINFO:
			PPR; printf("  minumum blocksize: %u samples\n", block->data.stream_info.min_blocksize);
			PPR; printf("  maximum blocksize: %u samples\n", block->data.stream_info.max_blocksize);
			PPR; printf("  minimum framesize: %u bytes\n", block->data.stream_info.min_framesize);
			PPR; printf("  maximum framesize: %u bytes\n", block->data.stream_info.max_framesize);
			PPR; printf("  sample_rate: %u Hz\n", block->data.stream_info.sample_rate);
			PPR; printf("  channels: %u\n", block->data.stream_info.channels);
			PPR; printf("  bits-per-sample: %u\n", block->data.stream_info.bits_per_sample);
			PPR; printf("  total samples: %llu\n", block->data.stream_info.total_samples);
			PPR; printf("  MD5 signature: ");
			for(i = 0; i < 16; i++) {
				PPR; printf("%02x", block->data.stream_info.md5sum[i]);
			}
			PPR; printf("\n");
			break;
		case FLAC__METADATA_TYPE_PADDING:
			/* nothing to print */
			break;
		case FLAC__METADATA_TYPE_APPLICATION:
			PPR; printf("  application ID: ");
			for(i = 0; i < 4; i++) {
				PPR; printf("%02x", block->data.application.id[i]);
			}
			PPR; printf("\n");
			PPR; printf("  data contents:\n");
			if(0 != block->data.application.data) {
				if(hexdump_application)
					hexdump(filename, block->data.application.data, block->length - FLAC__STREAM_METADATA_HEADER_LENGTH, "    ");
				else
					(void) fwrite(block->data.application.data, 1, block->length - FLAC__STREAM_METADATA_HEADER_LENGTH, stdout);
			}
			break;
		case FLAC__METADATA_TYPE_SEEKTABLE:
			PPR; printf("  seek points: %u\n", block->data.seek_table.num_points);
			for(i = 0; i < block->data.seek_table.num_points; i++) {
				PPR; printf("    point %d: sample_number=%llu, stream_offset=%llu, frame_samples=%u\n", i, block->data.seek_table.points[i].sample_number, block->data.seek_table.points[i].stream_offset, block->data.seek_table.points[i].frame_samples);
			}
			break;
		case FLAC__METADATA_TYPE_VORBIS_COMMENT:
			PPR; printf("  vendor string: ");
			fwrite(block->data.vorbis_comment.vendor_string.entry, 1, block->data.vorbis_comment.vendor_string.length, stdout);
			printf("\n");
			PPR; printf("  comments: %u\n", block->data.vorbis_comment.num_comments);
			for(i = 0; i < block->data.vorbis_comment.num_comments; i++) {
				PPR; printf("    comment[%u]: ", i);
				fwrite(block->data.vorbis_comment.comments[i].entry, 1, block->data.vorbis_comment.comments[i].length, stdout);
				printf("\n");
			}
			break;
		default:
			PPR; printf("SKIPPING block of unknown type\n");
			break;
	}
#undef PPR
}

void write_vc_field(const char *filename, const FLAC__StreamMetaData_VorbisComment_Entry *entry)
{
	if(filename)
		printf("%s:", filename);
	(void) fwrite(entry->entry, 1, entry->length, stdout);
	printf("\n");
}

void write_vc_fields(const char *filename, const char *field_name, const FLAC__StreamMetaData_VorbisComment_Entry entry[], unsigned num_entries)
{
	unsigned i;
	const unsigned field_name_length = strlen(field_name);

	for(i = 0; i < num_entries; i++) {
		if(field_name_matches_entry(field_name, field_name_length, entry + i))
			write_vc_field(filename, entry + i);
	}
}

FLAC__bool remove_vc_all(FLAC__StreamMetaData *block, FLAC__bool *needs_write)
{
	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__ASSERT(0 != needs_write);

	if(0 != block->data.vorbis_comment.comments) {
		FLAC__ASSERT(block->data.vorbis_comment.num_comments == 0);
		if(!FLAC__metadata_object_vorbiscomment_resize_comments(block, 0))
			return false;
		*needs_write = true;
	}
	else {
		FLAC__ASSERT(block->data.vorbis_comment.num_comments > 0);
	}

	return true;
}

FLAC__bool remove_vc_field(FLAC__StreamMetaData *block, const char *field_name, FLAC__bool *needs_write)
{
	FLAC__bool ok = true;
	const unsigned field_name_length = strlen(field_name);
	int i;

	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__ASSERT(0 != needs_write);

	/* must delete from end to start otherwise it will interfere with our iteration */
	for(i = (int)block->data.vorbis_comment.num_comments - 1; ok && i >= 0; i--) {
		if(field_name_matches_entry(field_name, field_name_length, block->data.vorbis_comment.comments + i)) {
			ok &= FLAC__metadata_object_vorbiscomment_delete_comment(block, (unsigned)i);
			if(ok)
				*needs_write = true;
		}
	}

	return ok;
}

FLAC__bool remove_vc_firstfield(FLAC__StreamMetaData *block, const char *field_name, FLAC__bool *needs_write)
{
	const unsigned field_name_length = strlen(field_name);
	unsigned i;

	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__ASSERT(0 != needs_write);

	for(i = 0; i < block->data.vorbis_comment.num_comments; i++) {
		if(field_name_matches_entry(field_name, field_name_length, block->data.vorbis_comment.comments + i)) {
			if(!FLAC__metadata_object_vorbiscomment_delete_comment(block, (unsigned)i))
				return false;
			else
				*needs_write = true;
			break;
		}
	}

	return true;
}

FLAC__bool set_vc_field(FLAC__StreamMetaData *block, const Argument_VcField *field, FLAC__bool *needs_write)
{
	FLAC__StreamMetaData_VorbisComment_Entry entry;
	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__ASSERT(0 != field);
	FLAC__ASSERT(0 != needs_write);

	entry.length = strlen(field->field);
	entry.entry = field->field;

	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, block->data.vorbis_comment.num_comments, entry, /*copy=*/true)) {
		return false;
	}
	else {
		*needs_write = true;
		return true;
	}
}

FLAC__bool field_name_matches_entry(const char *field_name, unsigned field_name_length, const FLAC__StreamMetaData_VorbisComment_Entry *entry)
{
	return (0 != memchr(entry->entry, '=', entry->length) && 0 == strncmp(field_name, entry->entry, field_name_length));
}

void hexdump(const char *filename, const FLAC__byte *buf, unsigned bytes, const char *indent)
{
	unsigned i, left = bytes;
	const FLAC__byte *b = buf;

	for(i = 0; i < bytes; i += 16) {
		printf("%s%s%s%08X: "
			"%02X %02X %02X %02X %02X %02X %02X %02X "
			"%02X %02X %02X %02X %02X %02X %02X %02X "
			"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
			filename? filename:"", filename? ":":"",
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
