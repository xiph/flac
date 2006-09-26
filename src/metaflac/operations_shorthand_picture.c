/* metaflac - Command-line FLAC metadata editor
 * Copyright (C) 2001,2002,2003,2004,2005,2006  Josh Coalson
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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <stdio.h> /* for snprintf() */
#include <string.h>
#include "options.h"
#include "utils.h"
#include "FLAC/assert.h"
#include "share/grabbag.h" /* for grabbag__picture_parse_specification */

static FLAC__bool import_pic_from(const char *filename, FLAC__StreamMetadata **picture, const char *specification, FLAC__bool *needs_write);

FLAC__bool do_shorthand_operation__picture(const char *filename, FLAC__Metadata_Chain *chain, const Operation *operation, FLAC__bool *needs_write)
{
	FLAC__bool ok = true, has_type1 = false, has_type2 = false;
	FLAC__StreamMetadata *picture = 0;
	FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();

	if(0 == iterator)
		die("out of memory allocating iterator");

	FLAC__metadata_iterator_init(iterator, chain);

	switch(operation->type) {
		case OP__IMPORT_PICTURE:
			ok = import_pic_from(filename, &picture, operation->argument.specification.value, needs_write);
			if(ok) {
				/* append PICTURE block */
				while(FLAC__metadata_iterator_next(iterator))
					;
				if(!FLAC__metadata_iterator_insert_block_after(iterator, picture)) {
					print_error_with_chain_status(chain, "%s: ERROR: adding new PICTURE block to metadata", filename);
					FLAC__metadata_object_delete(picture);
					ok = false;
				}
			}
			break;
		default:
			ok = false;
			FLAC__ASSERT(0);
			break;
	};

	/* check global PICTURE constraints (max 1 block each of type=1 and type=2) */
	while(FLAC__metadata_iterator_prev(iterator))
		;
	do {
		FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iterator);
		if(block->type == FLAC__METADATA_TYPE_PICTURE) {
			if(block->data.picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON_STANDARD) {
				if(has_type1) {
					print_error_with_chain_status(chain, "%s: ERROR: FLAC stream can only have one 32x32 standard icon (type=1) PICTURE block", filename);
					ok = false;
				}
				has_type1 = true;
			}
			else if(block->data.picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON) {
				if(has_type2) {
					print_error_with_chain_status(chain, "%s: ERROR: FLAC stream can only have one icon (type=2) PICTURE block", filename);
					ok = false;
				}
				has_type2 = true;
			}
		}
	} while(FLAC__metadata_iterator_next(iterator));


	FLAC__metadata_iterator_delete(iterator);
	return ok;
}

/*
 * local routines
 */

FLAC__bool import_pic_from(const char *filename, FLAC__StreamMetadata **picture, const char *specification, FLAC__bool *needs_write)
{
	const char *error_message;

	if(0 == specification || strlen(specification) == 0) {
		fprintf(stderr, "%s: ERROR: empty picture specification\n", filename);
		return false;
	}

	*picture = grabbag__picture_parse_specification(specification, &error_message);

	if(0 == *picture) {
		fprintf(stderr, "%s: ERROR: while parsing picture specification \"%s\": %s\n", filename, specification, error_message);
		return false;
	}

	if(!FLAC__format_picture_is_legal(&(*picture)->data.picture, &error_message)) {
		fprintf(stderr, "%s: ERROR: new PICTURE block for \"%s\" is illegal: %s\n", filename, specification, error_message);
		return false;
	}

	*needs_write = true;
	return true;
}
