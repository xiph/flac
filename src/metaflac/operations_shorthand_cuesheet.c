/* metaflac - Command-line FLAC metadata editor
 * Copyright (C) 2001,2002,2003,2004  Josh Coalson
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

#include "options.h"
#include "utils.h"
#include "FLAC/assert.h"
#include "share/grabbag.h"
#include <string.h>

static FLAC__bool import_cs_from(const char *filename, FLAC__StreamMetadata **cuesheet, const char *cs_filename, FLAC__bool *needs_write, FLAC__uint64 lead_out_offset, Argument_AddSeekpoint *add_seekpoint_link);
static FLAC__bool export_cs_to(const char *filename, const FLAC__StreamMetadata *cuesheet, const char *cs_filename);

FLAC__bool do_shorthand_operation__cuesheet(const char *filename, FLAC__Metadata_Chain *chain, const Operation *operation, FLAC__bool *needs_write)
{
	FLAC__bool ok = true;
	FLAC__StreamMetadata *cuesheet = 0;
	FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
	FLAC__uint64 lead_out_offset = 0;

	if(0 == iterator)
		die("out of memory allocating iterator");

	FLAC__metadata_iterator_init(iterator, chain);

	do {
		FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iterator);
		if(block->type == FLAC__METADATA_TYPE_STREAMINFO) {
			lead_out_offset = block->data.stream_info.total_samples;
			if(lead_out_offset == 0) {
				fprintf(stderr, "%s: ERROR: FLAC file must have total_samples set in STREAMINFO in order to import/export cuesheet\n", filename);
				FLAC__metadata_iterator_delete(iterator);
				return false;
			}
			if(block->data.stream_info.sample_rate != 44100) {
				fprintf(stderr, "%s: ERROR: FLAC stream must currently be 44.1kHz in order to import/export cuesheet\n", filename);
				FLAC__metadata_iterator_delete(iterator);
				return false;
			}
		}
		else if(block->type == FLAC__METADATA_TYPE_CUESHEET)
			cuesheet = block;
	} while(FLAC__metadata_iterator_next(iterator));

	if(lead_out_offset == 0) {
		fprintf(stderr, "%s: ERROR: FLAC stream has no STREAMINFO block\n", filename);
		FLAC__metadata_iterator_delete(iterator);
		return false;
	}

	switch(operation->type) {
		case OP__IMPORT_CUESHEET_FROM:
			if(0 != cuesheet) {
				fprintf(stderr, "%s: ERROR: FLAC file already has CUESHEET block\n", filename);
				ok = false;
			}
			else {
				ok = import_cs_from(filename, &cuesheet, operation->argument.import_cuesheet_from.filename, needs_write, lead_out_offset, operation->argument.import_cuesheet_from.add_seekpoint_link);
				if(ok) {
					/* append CUESHEET block */
					while(FLAC__metadata_iterator_next(iterator))
						;
					if(!FLAC__metadata_iterator_insert_block_after(iterator, cuesheet)) {
						fprintf(stderr, "%s: ERROR: adding new CUESHEET block to metadata, status =\"%s\"\n", filename, FLAC__Metadata_ChainStatusString[FLAC__metadata_chain_status(chain)]);
						FLAC__metadata_object_delete(cuesheet);
						ok = false;
					}
				}
			}
			break;
		case OP__EXPORT_CUESHEET_TO:
			if(0 == cuesheet) {
				fprintf(stderr, "%s: ERROR: FLAC file has no CUESHEET block\n", filename);
				ok = false;
			}
			else
				ok = export_cs_to(filename, cuesheet, operation->argument.filename.value);
			break;
		default:
			ok = false;
			FLAC__ASSERT(0);
			break;
	};

	FLAC__metadata_iterator_delete(iterator);
	return ok;
}

/*
 * local routines
 */

FLAC__bool import_cs_from(const char *filename, FLAC__StreamMetadata **cuesheet, const char *cs_filename, FLAC__bool *needs_write, FLAC__uint64 lead_out_offset, Argument_AddSeekpoint *add_seekpoint_link)
{
	FILE *f;
	const char *error_message;
	char **seekpoint_specification = add_seekpoint_link? &(add_seekpoint_link->specification) : 0;
	unsigned last_line_read;

	if(0 == cs_filename || strlen(cs_filename) == 0) {
		fprintf(stderr, "%s: ERROR: empty import file name\n", filename);
		return false;
	}
	if(0 == strcmp(cs_filename, "-"))
		f = stdin;
	else
		f = fopen(cs_filename, "r");

	if(0 == f) {
		fprintf(stderr, "%s: ERROR: can't open import file %s\n", filename, cs_filename);
		return false;
	}

	*cuesheet = grabbag__cuesheet_parse(f, &error_message, &last_line_read, /*@@@is_cdda=*/true, lead_out_offset);

	if(f != stdin)
		fclose(f);

	if(0 == *cuesheet) {
		fprintf(stderr, "%s: ERROR: while parsing cuesheet \"%s\" on line %u: %s\n", filename, cs_filename, last_line_read, error_message);
		return false;
	}

	/* add seekpoints for each index point if required */
	if(0 != seekpoint_specification) {
		char spec[128];
		unsigned track, index;
		const FLAC__StreamMetadata_CueSheet *cs = &(*cuesheet)->data.cue_sheet;
		if(0 == *seekpoint_specification)
			*seekpoint_specification = local_strdup("");
		for(track = 0; track < cs->num_tracks; track++) {
			const FLAC__StreamMetadata_CueSheet_Track *tr = cs->tracks+track;
			for(index = 0; index < tr->num_indices; index++) {
#ifdef _MSC_VER
				sprintf(spec, "%I64u;", tr->offset + tr->indices[index].offset);
#else
				sprintf(spec, "%llu;", tr->offset + tr->indices[index].offset);
#endif
				local_strcat(seekpoint_specification, spec);
			}
		}
	}

	*needs_write = true;
	return true;
}

FLAC__bool export_cs_to(const char *filename, const FLAC__StreamMetadata *cuesheet, const char *cs_filename)
{
	FILE *f;

	if(0 == cs_filename || strlen(cs_filename) == 0) {
		fprintf(stderr, "%s: ERROR: empty export file name\n", filename);
		return false;
	}
	if(0 == strcmp(cs_filename, "-"))
		f = stdout;
	else
		f = fopen(cs_filename, "w");

	if(0 == f) {
		fprintf(stderr, "%s: ERROR: can't open export file %s\n", filename, cs_filename);
		return false;
	}

	grabbag__cuesheet_emit(f, cuesheet, "\"dummy.wav\" WAVE");

	if(f != stdout)
		fclose(f);

	return true;
}
