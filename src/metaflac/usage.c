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

#include "usage.h"
#include "FLAC/format.h"
#include <stdarg.h>
#include <stdio.h>

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
	fprintf(out, "--no-utf8-convert     Do not convert Vorbis comments from UTF-8 to local charset,\n");
	fprintf(out, "                      or vice versa.  This is useful for scripts.\n");
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
	fprintf(out, "--remove-vc-field=name  Remove all Vorbis comment fields whose field name\n");
	fprintf(out, "                      is 'name'.\n");
	fprintf(out, "--remove-vc-firstfield=name  Remove first Vorbis comment field whose field\n");
	fprintf(out, "                      name is 'name'.\n");
	fprintf(out, "--remove-vc-all       Remove all Vorbis comment fields, leaving only the\n");
	fprintf(out, "                      vendor string in the VORBIS_COMMENT block.\n");
	fprintf(out, "--set-vc-field=field  Add a Vorbis comment field.  The field must comply with\n");
	fprintf(out, "                      the Vorbis comment spec, of the form \"NAME=VALUE\".  If\n");
	fprintf(out, "                      there is currently no VORBIS_COMMENT block, one will be\n");
	fprintf(out, "                      created.\n");
	fprintf(out, "--import-vc-from=file Import Vorbis comments from a file.  Use '-' for stdin.\n");
	fprintf(out, "                      Each line should be of the form NAME=VALUE.  Multi-\n");
	fprintf(out, "                      line comments are currently not supported.  Specify\n");
	fprintf(out, "                      --remove-vc-all and/or --no-utf8-convert before\n");
	fprintf(out, "                      --import-vc-from if necessary.\n");
	fprintf(out, "--export-vc-to=file   Export Vorbis comments to a file.  Use '-' for stdin.\n");
	fprintf(out, "                      Each line will be of the form NAME=VALUE.  Specify\n");
	fprintf(out, "                      --no-utf8-convert if necessary.\n");
	fprintf(out, "--import-cuesheet-from=file  Import a cuesheet from a file.  Only one FLAC\n");
	fprintf(out, "                      file may be specified.  Seekpoints for all indices in\n");
	fprintf(out, "                      the cuesheet will be added unless --no-cued-seekpoints\n");
	fprintf(out, "                      is specified.\n");
	fprintf(out, "--export-cuesheet-to=file  Export CUESHEET block to a cuesheet file, suitable\n");
	fprintf(out, "                      for use by CD authoring software.  Use '-' for stdin.\n");
	fprintf(out, "                      Only one FLAC file may be specified on the command line.\n");
	fprintf(out, "--add-replay-gain     Calculates the title and album gains/peaks of the given\n");
	fprintf(out, "                      FLAC files as if all the files were part of one album,\n");
	fprintf(out, "                      then stores them in the VORBIS_COMMENT block.  The tags\n");
	fprintf(out, "                      are the same as those used by vorbisgain.  Existing\n");
	fprintf(out, "                      ReplayGain tags will be replaced.  If only one FLAC file\n");
	fprintf(out, "                      is given, the album and title gains will be the same.\n");
	fprintf(out, "                      Since this operation requires two passes, it is always\n");
	fprintf(out, "                      executed last, after all other operations have been\n");
	fprintf(out, "                      completed and written to disk.  All FLAC files specified\n");
	fprintf(out, "                      must have the same resolution, sample rate, and number\n");
	fprintf(out, "                      of channels.  The sample rate must be one of 8, 11.025,\n");
	fprintf(out, "                      12, 16, 22.05, 24, 32, 44.1, or 48 kHz.\n");
	fprintf(out, "--add-seekpoint={#|X|#x|#s}  Add seek points to a SEEKTABLE block\n");
	fprintf(out, "       #  : a specific sample number for a seek point\n");
	fprintf(out, "       X  : a placeholder point (always goes at the end of the SEEKTABLE)\n");
	fprintf(out, "       #x : # evenly spaced seekpoints, the first being at sample 0\n");
	fprintf(out, "       #s : a seekpoint every # seconds; # does not have to be a whole number\n");
	fprintf(out, "                      If no SEEKTABLE block exists, one will be created.  If\n");
	fprintf(out, "                      one already exists, points will be added to the existing\n");
	fprintf(out, "                      table, and duplicates will be turned into placeholder\n");
	fprintf(out, "                      points.  You may use many --add-seekpoint options; the\n");
	fprintf(out, "                      resulting SEEKTABLE will be the unique-ified union of\n");
	fprintf(out, "                      all such values.  Example: --add-seekpoint=100x\n");
	fprintf(out, "                      --add-seekpoint=3.5s will add 100 evenly spaced\n");
	fprintf(out, "                      seekpoints and a seekpoint every 3.5 seconds.\n");
	fprintf(out, "--add-padding=length  Add a padding block of the given length (in bytes).\n");
	fprintf(out, "                      The overall length of the new block will be 4 + length;\n");
	fprintf(out, "                      the extra 4 bytes is for the metadata block header.\n");
	fprintf(out, "\n");
	fprintf(out, "Major operations:\n");
	fprintf(out, "--version\n");
	fprintf(out, "    Show the metaflac version number.\n");
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
