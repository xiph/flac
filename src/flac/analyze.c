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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FLAC/all.h"
#include "analyze.h"

typedef struct {
	int32 residual;
	unsigned count;
} pair_t;

typedef struct {
	pair_t buckets[FLAC__MAX_BLOCK_SIZE];
	int peak_index;
	unsigned nbuckets;
	unsigned nsamples;
	double sum, sos;
	double variance;
	double mean;
	double stddev;
} subframe_stats_t;

static subframe_stats_t all_;

static void init_stats(subframe_stats_t *stats);
static void update_stats(subframe_stats_t *stats, int32 residual, unsigned incr);
static void compute_stats(subframe_stats_t *stats);
static bool dump_stats(const subframe_stats_t *stats, const char *filename);

void analyze_init()
{
	init_stats(&all_);
}

void analyze_frame(const FLAC__Frame *frame, unsigned frame_number, analysis_options aopts, FILE *fout)
{
	const unsigned channels = frame->header.channels;
	char outfilename[1024];
	subframe_stats_t stats;
	unsigned i, channel;

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
			unsigned residual_samples;

			init_stats(&stats);

			switch(subframe->type) {
				case FLAC__SUBFRAME_TYPE_FIXED:
					residual_samples = frame->header.blocksize - subframe->data.fixed.order;
					for(i = 0; i < residual_samples; i++)
						update_stats(&stats, subframe->data.fixed.residual[i], 1);
					break;
				case FLAC__SUBFRAME_TYPE_LPC:
					residual_samples = frame->header.blocksize - subframe->data.lpc.order;
					for(i = 0; i < residual_samples; i++)
						update_stats(&stats, subframe->data.lpc.residual[i], 1);
					break;
				default:
					break;
			}

			/* update all_ */
			for(i = 0; i < stats.nbuckets; i++) {
				update_stats(&all_, stats.buckets[i].residual, stats.buckets[i].count);
			}

			/* write the subframe */
			sprintf(outfilename, "f%06u.s%u.gp", frame_number, channel);
			compute_stats(&stats);
if(frame_number<50)//@@@
			(void)dump_stats(&stats, outfilename);
		}
	}
}

void analyze_finish()
{
	compute_stats(&all_);
	(void)dump_stats(&all_, "all");
}

void init_stats(subframe_stats_t *stats)
{
	stats->peak_index = -1;
	stats->nbuckets = 0;
	stats->nsamples = 0;
	stats->sum = 0.0;
	stats->sos = 0.0;
}

void update_stats(subframe_stats_t *stats, int32 residual, unsigned incr)
{
	unsigned i;
	const double r = (double)residual, a = r*incr;

	stats->nsamples += incr;
	stats->sum += a;
	stats->sos += (a*r);

	for(i = 0; i < stats->nbuckets; i++) {
		if(stats->buckets[i].residual == residual) {
			stats->buckets[i].count += incr;
			goto find_peak;
		}
	}
	/* not found, make a new bucket */
	i = stats->nbuckets;
	stats->buckets[i].residual = residual;
	stats->buckets[i].count = incr;
	stats->nbuckets++;
find_peak:
	if(stats->peak_index < 0 || stats->buckets[i].count > stats->buckets[stats->peak_index].count)
		stats->peak_index = i;
}

void compute_stats(subframe_stats_t *stats)
{
	stats->mean = stats->sum / (double)stats->nsamples;
	stats->variance = (stats->sos - (stats->sum * stats->sum / stats->nsamples)) / stats->nsamples;
	stats->stddev = sqrt(stats->variance);
}

bool dump_stats(const subframe_stats_t *stats, const char *filename)
{
	FILE *outfile;
	unsigned i;
	const double m = stats->mean;
	const double s1 = stats->stddev, s2 = s1*2, s3 = s1*3, s4 = s1*4, s5 = s1*5, s6 = s1*6;
	const double p = stats->buckets[stats->peak_index].count;

	outfile = fopen(filename, "w");

	if(0 == outfile) {
		fprintf(stderr, "ERROR opening %s\n", filename);
		return false;
	}

	fprintf(outfile, "plot '-' title 'PDF', '-' title 'mean' with impulses, '-' title '1-stddev' with histeps, '-' title '2-stddev' with histeps, '-' title '3-stddev' with histeps, '-' title '4-stddev' with histeps, '-' title '5-stddev' with histeps, '-' title '6-stddev' with histeps\n");

	for(i = 0; i < stats->nbuckets; i++) {
		fprintf(outfile, "%d %u\n", stats->buckets[i].residual, stats->buckets[i].count);
	}
	fprintf(outfile, "e\n");

	fprintf(outfile, "%f %f\ne\n", stats->mean, p);
	fprintf(outfile, "%f %f\n%f %f\ne\n", m-s1, p*0.8, m+s1, p*0.8);
	fprintf(outfile, "%f %f\n%f %f\ne\n", m-s2, p*0.7, m+s2, p*0.7);
	fprintf(outfile, "%f %f\n%f %f\ne\n", m-s3, p*0.6, m+s3, p*0.6);
	fprintf(outfile, "%f %f\n%f %f\ne\n", m-s4, p*0.5, m+s4, p*0.5);
	fprintf(outfile, "%f %f\n%f %f\ne\n", m-s5, p*0.4, m+s5, p*0.4);
	fprintf(outfile, "%f %f\n%f %f\ne\n", m-s6, p*0.3, m+s6, p*0.3);

	fprintf(outfile, "pause -1 'waiting...'\n");

	fclose(outfile);
	return true;
}
