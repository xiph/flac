/* metaflac - Command-line FLAC metadata editor
 * Copyright (C) 2001  Josh Coalson
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

/*
 * WATCHOUT - this is meant to be very lightweight an not even dependent
 * on libFLAC, so there are a couple places where FLAC__* variables are
 * duplicated here.  Look for 'DUPLICATE:' in comments.
 */

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FLAC/all.h"

static const char *sync_string_ = "fLaC"; /* DUPLICATE:FLAC__STREAM_SYNC_STRING */
static const char *metadata_type_string_[] = { /* DUPLICATE:FLAC__MetaDataTypeString */
	"STREAMINFO",
	"PADDING",
	"APPLICATION",
	"SEEKTABLE"
};
static const unsigned SEEKPOINT_LEN_ = 18; /* DUPLICATE:FLAC__STREAM_METADATA_SEEKPOINT_LEN */

static int usage(const char *message, ...);
static bool list(FILE *f, bool verbose);
static uint32 unpack_uint32(byte *b, unsigned bytes);
static uint64 unpack_uint64(byte *b, unsigned bytes);
static void hexdump(const byte *buf, unsigned bytes);

int main(int argc, char *argv[])
{
	int i;
	bool verbose = false, list_mode = true;

	if(argc <= 1)
		return usage(0);

	/* get the options */
	for(i = 1; i < argc; i++) {
		if(argv[i][0] != '-' || argv[i][1] == 0)
			break;
		if(0 == strcmp(argv[i], "-l"))
			list_mode = true;
		else if(0 == strcmp(argv[i], "-v"))
			verbose = true;
		else if(0 == strcmp(argv[i], "-v-"))
			verbose = false;
		else {
			return usage("ERROR: invalid option '%s'\n", argv[i]);
		}
	}
	if(i + (list_mode? 1:2) != argc)
		return usage("ERROR: invalid arguments (more/less than %d filename%s?)\n", (list_mode? 1:2), (list_mode? "":"s"));

	if(list_mode) {
		FILE *f = fopen(argv[i], "r");

		if(0 == f) {
			fprintf(stderr, "ERROR opening %s\n", argv[i]);
			return 1;
		}

		if(!list(f, verbose))
			return 1;

		fclose(f);
	}

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
	printf("metaflac - Command-line FLAC metadata editor version %s\n", FLAC__VERSION_STRING);
	printf("Copyright (C) 2001  Josh Coalson\n");
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
	printf("  metaflac [options] infile [outfile]\n");
	printf("\n");
	printf("options:\n");
	printf("  -l : list metadata blocks\n");
	printf("  -v : verbose\n");

	return 1;
}

bool list(FILE *f, bool verbose)
{
	byte buf[65536];
	byte *b = buf;
	FLAC__StreamMetaData metadata;
	unsigned blocknum = 0, byte_offset = 0, i;

	/* read the stream sync code */
	if(fread(buf, 1, 4, f) < 4 || memcmp(buf, sync_string_, 4)) {
		fprintf(stderr, "ERROR: not a FLAC file (no '%s' header)\n", sync_string_);
		return false;
	}
	byte_offset += 4;

	/* read the metadata blocks */
	do {
		/* read the metadata block header */
		if(fread(buf, 1, 4, f) < 4) {
			fprintf(stderr, "ERROR: short count reading metadata block header\n");
			return false;
		}
		metadata.is_last = (buf[0] & 0x80)? true:false;
		metadata.type = (FLAC__MetaDataType)(buf[0] & 0x7f);
		metadata.length = unpack_uint32(buf+1, 3);

		/* print header */
		printf("METADATA block #%u\n", blocknum);
		printf("byte offset: %u\n", byte_offset);
		printf("type: %u (%s)\n", (unsigned)metadata.type, metadata.type<=FLAC__METADATA_TYPE_SEEKTABLE? metadata_type_string_[metadata.type] : "UNKNOWN");
		printf("is last: %s\n", metadata.is_last? "true":"false");
		printf("length: %u\n", metadata.length);

		if(metadata.length > sizeof(buf)) {
			printf("SKIPPING large block\n\n");
			continue;
		}

		/* read the metadata block data */
		if(fread(buf, 1, metadata.length, f) < metadata.length) {
			fprintf(stderr, "ERROR: short count reading metadata block data\n\n");
			return false;
		}
		switch(metadata.type) {
			case FLAC__METADATA_TYPE_STREAMINFO:
				b = buf;
				metadata.data.stream_info.min_blocksize = unpack_uint32(b, 2); b += 2;
				metadata.data.stream_info.max_blocksize = unpack_uint32(b, 2); b += 2;
				metadata.data.stream_info.min_framesize = unpack_uint32(b, 3); b += 3;
				metadata.data.stream_info.max_framesize = unpack_uint32(b, 3); b += 3;
				metadata.data.stream_info.sample_rate = (unpack_uint32(b, 2) << 4) | ((unsigned)(b[2] & 0xf0) >> 4);
				metadata.data.stream_info.channels = (unsigned)((b[2] & 0x0e) >> 1) + 1;
				metadata.data.stream_info.bits_per_sample = ((((unsigned)(b[2] & 0x01)) << 1) | (((unsigned)(b[3] & 0xf0)) >> 4)) + 1;
				metadata.data.stream_info.total_samples = (((uint64)(b[3] & 0x0f)) << 32) | unpack_uint64(b+4, 4);
				memcpy(metadata.data.stream_info.md5sum, b+8, 16);
				break;
			case FLAC__METADATA_TYPE_PADDING:
				if(verbose) {
					/* dump contents */
				}
				break;
			case FLAC__METADATA_TYPE_APPLICATION:
				memcpy(buf, metadata.data.application.id, 4);
				metadata.data.application.data = buf+4;
				break;
			case FLAC__METADATA_TYPE_SEEKTABLE:
				metadata.data.seek_table.num_points = metadata.length / SEEKPOINT_LEN_;
				b = buf+4; /* we leave the points in buf for printing later */
				break;
			default:
				printf("SKIPPING block of unknown type\n\n");
				continue;
		}

		/* print data */
		switch(metadata.type) {
			case FLAC__METADATA_TYPE_STREAMINFO:
				printf("minumum blocksize: %u samples\n", metadata.data.stream_info.min_blocksize);
				printf("maximum blocksize: %u samples\n", metadata.data.stream_info.max_blocksize);
				printf("minimum framesize: %u bytes\n", metadata.data.stream_info.min_framesize);
				printf("maximum framesize: %u bytes\n", metadata.data.stream_info.max_framesize);
				printf("sample_rate: %u Hz\n", metadata.data.stream_info.sample_rate);
				printf("channels: %u\n", metadata.data.stream_info.channels);
				printf("bits-per-sample: %u\n", metadata.data.stream_info.bits_per_sample);
				printf("total samples: %llu\n", metadata.data.stream_info.total_samples);
				printf("MD5 signature: ");
				for(i = 0; i < 16; i++)
					printf("%02x", metadata.data.stream_info.md5sum[i]);
				printf("\n");
				break;
			case FLAC__METADATA_TYPE_PADDING:
				if(verbose) {
					printf("pad contents:\n");
					hexdump(buf, metadata.length);
				}
				break;
			case FLAC__METADATA_TYPE_APPLICATION:
				printf("Application ID: ");
				for(i = 0; i < 4; i++)
					printf("%02x", metadata.data.application.id[i]);
				printf("\n");
				if(verbose) {
					printf("data contents:\n");
					hexdump(metadata.data.application.data, metadata.length);
				}
				break;
			case FLAC__METADATA_TYPE_SEEKTABLE:
				printf("seek points: %u\n", metadata.data.seek_table.num_points);
				for(i = 0; i < metadata.data.seek_table.num_points; i++, b += SEEKPOINT_LEN_) {
					printf("\tpoint %d: sample_number=%llu, stream_offset=%llu, frame_samples=%u\n", i, unpack_uint64(b, 8), unpack_uint64(b+8, 8), unpack_uint32(b+16, 2));
				}
				break;
			default:
				assert(0);
		}

		if(!metadata.is_last)
			printf("\n");

		blocknum++;
		byte_offset += (4 + metadata.length);
	} while (!metadata.is_last);

	return true;
}

uint32 unpack_uint32(byte *b, unsigned bytes)
{
	uint32 ret = 0;
	unsigned i;

	for(i = 0; i < bytes; i++)
		ret = (ret << 8) | (uint32)(*b++);

	return ret;
}

uint64 unpack_uint64(byte *b, unsigned bytes)
{
	uint64 ret = 0;
	unsigned i;

	for(i = 0; i < bytes; i++)
		ret = (ret << 8) | (uint64)(*b++);

	return ret;
}

void hexdump(const byte *buf, unsigned bytes)
{
	unsigned i, left = bytes;
	const byte *b = buf;

	for(i = 0; i < bytes; i += 16) {
		printf("%08X: "
			"%02X %02X %02X %02X %02X %02X %02X %02X "
			"%02X %02X %02X %02X %02X %02X %02X %02X "
			"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
			i,
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
