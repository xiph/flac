/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2000,2001  Josh Coalson
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FLAC/all.h"
#include "analyze.h"

typedef struct {
	int32 residual;
	unsigned count;
} pair_t;

static pair_t super_buckets[FLAC__MAX_BLOCK_SIZE];
static unsigned nsuper_buckets;

static void update_buckets(int32 residual, pair_t *buckets, unsigned *nbuckets, unsigned incr);
static bool print_buckets(const pair_t buckets[], const unsigned nbuckets, const char *filename);

void analyze_init()
{
	nsuper_buckets = 0;
}

void analyze_frame(const FLAC__Frame *frame, unsigned frame_number, analysis_options aopts, FILE *fout)
{
	const unsigned channels = frame->header.channels;
	char outfilename[1024];
	pair_t buckets[FLAC__MAX_BLOCK_SIZE];
	unsigned i, channel, nbuckets;

	/* do the human-readable part first */
	fprintf(fout, "frame=%u\tblocksize=%u\tsample_rate=%u\tchannels=%u\tchannel_assignment=%s\n", frame_number, frame->header.blocksize, frame->header.sample_rate, channels, FLAC__ChannelAssignmentString[frame->header.channel_assignment]);
	for(channel = 0; channel < channels; channel++) {
		const FLAC__Subframe *subframe = frame->subframes+channel;
		fprintf(fout, "\tsubframe=%u\ttype=%s", channel, FLAC__SubframeTypeString[subframe->type]);
		switch(subframe->type) {
			case FLAC__SUBFRAME_TYPE_CONSTANT:
				fprintf(fout, "\tvalue=%d\n", subframe->data.constant.value);
				break;
			case FLAC__SUBFRAME_TYPE_FIXED:
				fprintf(fout, "\torder=%u\tpartition_order=%u\n", subframe->data.fixed.order, subframe->data.fixed.entropy_coding_method.data.partitioned_rice.order); /*@@@ assumes method is partitioned-rice */
				for(i = 0; i < subframe->data.fixed.order; i++)
					fprintf(fout, "\t\twarmup[%u]=%d\n", i, subframe->data.fixed.warmup[i]);
				if(aopts.do_residual_text) {
					for(i = 0; i < frame->header.blocksize-subframe->data.fixed.order; i++)
						fprintf(fout, "\t\tresidual[%u]=%d\n", i, subframe->data.fixed.residual[i]);
				}
				break;
			case FLAC__SUBFRAME_TYPE_LPC:
				fprintf(fout, "\torder=%u\tpartition_order=%u\tqlp_coeff_precision=%u\tquantization_level=%d\n", subframe->data.lpc.order, subframe->data.lpc.entropy_coding_method.data.partitioned_rice.order, subframe->data.lpc.qlp_coeff_precision, subframe->data.lpc.quantization_level); /*@@@ assumes method is partitioned-rice */
				for(i = 0; i < subframe->data.lpc.order; i++)
					fprintf(fout, "\t\twarmup[%u]=%d\n", i, subframe->data.lpc.warmup[i]);
				if(aopts.do_residual_text) {
					for(i = 0; i < frame->header.blocksize-subframe->data.lpc.order; i++)
						fprintf(fout, "\t\tresidual[%u]=%d\n", i, subframe->data.lpc.residual[i]);
				}
				break;
			case FLAC__SUBFRAME_TYPE_VERBATIM:
				fprintf(fout, "\n");
				break;
		}
	}

	/* now do the residual distributions if requested */
	if(aopts.do_residual_gnuplot) {
		for(channel = 0; channel < channels; channel++) {
			const FLAC__Subframe *subframe = frame->subframes+channel;

			nbuckets = 0;

			switch(subframe->type) {
				case FLAC__SUBFRAME_TYPE_FIXED:
					for(i = 0; i < frame->header.blocksize-subframe->data.fixed.order; i++)
						update_buckets(subframe->data.fixed.residual[i], buckets, &nbuckets, 1);
					break;
				case FLAC__SUBFRAME_TYPE_LPC:
					for(i = 0; i < frame->header.blocksize-subframe->data.lpc.order; i++)
						update_buckets(subframe->data.lpc.residual[i], buckets, &nbuckets, 1);
					break;
				default:
					break;
			}

			/* update super_buckets */
			for(i = 0; i < nbuckets; i++) {
				update_buckets(buckets[i].residual, super_buckets, &nsuper_buckets, buckets[i].count);
			}

			/* write the subframe */
			sprintf(outfilename, "f%06u.s%u.gp", frame_number, channel);
			(void)print_buckets(buckets, nbuckets, outfilename);
		}
	}
}

void analyze_finish()
{
	(void)print_buckets(super_buckets, nsuper_buckets, "all");
}

void update_buckets(int32 residual, pair_t *buckets, unsigned *nbuckets, unsigned incr)
{
	unsigned i;

	for(i = 0; i < *nbuckets; i++) {
		if(buckets[i].residual == residual) {
			buckets[i].count += incr;
			return;
		}
	}
	buckets[*nbuckets].residual = residual;
	buckets[*nbuckets].count = incr;
	(*nbuckets)++;
}

bool print_buckets(const pair_t buckets[], const unsigned nbuckets, const char *filename)
{
	FILE *outfile;
	unsigned i;

	outfile = fopen(filename, "w");

	if(0 == outfile) {
		fprintf(stderr, "ERROR opening %s\n", filename);
		return false;
	}

	for(i = 0; i < nbuckets; i++) {
		fprintf(outfile, "%d %u\n", buckets[i].residual, buckets[i].count);
	}

	fclose(outfile);
	return true;
}
