/* metaflac - Command-line FLAC metadata editor
 * Copyright (C) 2001-2009  Josh Coalson
 * Copyright (C) 2011-2025  Xiph.Org Foundation
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "FLAC/format.h"
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#if ENABLE_EXPERIMENTAL_FLOAT_SAMPLE_CODING

#include "options.h"
#include "utils.h"
#include "FLAC/assert.h"
#include "FLAC/metadata.h"
#include "share/compat.h"
#include "operations_shorthand.h"

FLAC__bool check_extension(FLAC__Metadata_Chain *chain)
{
	FLAC__StreamMetadata *block;
	FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();

	if(0 == iterator)
		die("out of memory allocating iterator");

	FLAC__metadata_iterator_init(iterator, chain);

	block = FLAC__metadata_iterator_get_block(iterator);
	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_STREAMINFO);

	FLAC__metadata_iterator_delete(iterator);

	return block->data.stream_info.bits_per_sample == 1;
}

FLAC__bool do_shorthand_operation__streaminfo_extension(const char *filename, FLAC__bool prefix_with_filename, FLAC__Metadata_Chain *chain, const Operation *operation, FLAC__bool *needs_write)
{
	FLAC__bool ok = true;
	FLAC__StreamMetadata *block;
	FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();

	if(0 == iterator)
		die("out of memory allocating iterator");

	FLAC__metadata_iterator_init(iterator, chain);

	block = FLAC__metadata_iterator_get_block(iterator);
	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_STREAMINFO);

	if(block->data.stream_info.bits_per_sample == 1) {
		FLAC__metadata_iterator_next(iterator);

		block = FLAC__metadata_iterator_get_block(iterator);
		FLAC__ASSERT(0 != block);
		FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_STREAMINFO_EXTENSION);
	}
	else {
		if(operation->type == OP__SET_SAMPLE_RATE_EXTENSION || operation->type == OP__SET_CHANNELS || operation->type == OP__SET_CHANNEL_MASK || operation->type == OP__SET_BPS || operation->type == OP__SET_SAMPLE_TYPE) {
			// create a new block
			block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_STREAMINFO_EXTENSION);
			if(0 == block)
				die("out of memory allocating STREAMINFO_EXTENSION block");
			if(!FLAC__metadata_iterator_insert_block_after(iterator, block)) {
				print_error_with_chain_status(chain, "%s: ERROR: adding new STREAMINFO_EXTENSION block to metadata", filename);
				return false;
			}
			/* iterator is left pointing to new block */
			FLAC__ASSERT(FLAC__metadata_iterator_get_block(iterator) == block);
		}
		else {
			// Nothing to show
			FLAC__metadata_iterator_delete(iterator);
			return ok;
		}
	}

	if(prefix_with_filename)
		flac_printf("%s:", filename);

	switch(operation->type) {
		case OP__SHOW_SAMPLE_RATE:
			flac_printf("%f\n", block->data.stream_info_extension.sample_rate);
			break;
		case OP__SHOW_CHANNELS:
			flac_printf("%u\n", block->data.stream_info_extension.channels);
			break;
		case OP__SHOW_CHANNEL_MASK:
			flac_printf("%u\n", block->data.stream_info_extension.channel_mask);
			break;
		case OP__SHOW_BPS:
			flac_printf("%u\n", block->data.stream_info_extension.bits_per_sample);
			break;
		case OP__SHOW_SAMPLE_TYPE:
			if(block->data.stream_info_extension.sample_type == FLAC__SAMPLE_TYPE_FLOAT)
				flac_printf("LPCM floating point (IEEE 754 binary32)\n");
			else
				flac_printf("LPCM integer\n");
			break;
		case OP__SET_SAMPLE_RATE_EXTENSION:
			block->data.stream_info_extension.sample_rate = operation->argument.streaminfo_extention_float64.value;
			*needs_write = true;
			break;
		case OP__SET_CHANNELS:
			block->data.stream_info_extension.channels = operation->argument.streaminfo_uint32.value;
			*needs_write = true;
			break;
		case OP__SET_CHANNEL_MASK:
			block->data.stream_info_extension.channel_mask = operation->argument.streaminfo_uint32.value;
			*needs_write = true;
			break;
		case OP__SET_BPS:
			block->data.stream_info_extension.bits_per_sample = operation->argument.streaminfo_uint32.value;
			*needs_write = true;
			break;
		case OP__SET_SAMPLE_TYPE:
			block->data.stream_info_extension.sample_type = operation->argument.streaminfo_uint32.value;
			if(block->data.stream_info_extension.sample_type == FLAC__SAMPLE_TYPE_FLOAT) {
				block->data.stream_info_extension.bits_per_sample = 32;
			}
			*needs_write = true;
			break;
		default:
			ok = false;
			FLAC__ASSERT(0);
			break;
	};

	FLAC__metadata_iterator_delete(iterator);

	return ok;
}

#endif /* ENABLE_EXPERIMENTAL_FLOAT_SAMPLE_CODING */
