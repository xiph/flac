/* replaygain - Convenience lib for calculating/storing ReplayGain in FLAC
 * Copyright (C) 2002  Josh Coalson
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

#include "share/replaygain.h"
#include "share/gain_analysis.h"
#include "FLAC/assert.h"
#include "FLAC/format.h"

#ifdef local_min
#undef local_min
#endif
#define local_min(a,b) ((a)<(b)?(a):(b))

REPLAYGAIN_API FLAC__bool FLAC__replaygain_init(unsigned sample_frequency)
{
	return InitGainAnalysis((long)sample_frequency) == INIT_GAIN_ANALYSIS_OK;
}

REPLAYGAIN_API FLAC__bool FLAC__replaygain_analyze(const FLAC__int32 * const input[], FLAC__bool is_stereo, unsigned bps, unsigned samples)
{
	static Float_t lbuffer[4096], rbuffer[4096];
	static const unsigned nbuffer = sizeof(lbuffer) / sizeof(lbuffer[0]);
	unsigned i;

	FLAC__ASSERT(bps >= 4 && bps <= 32);
	FLAC__ASSERT(FLAC__MIN_BITS_PER_SAMPLE == 4);
	FLAC__ASSERT(FLAC__MAX_BITS_PER_SAMPLE == 32);

	if(bps == 16) {
		if(is_stereo) {
			while(samples > 0) {
				const unsigned n = local_min(samples, nbuffer);
				for(i = 0; i < n; i++) {
					lbuffer[i] = (Float_t)input[0][i];
					rbuffer[i] = (Float_t)input[1][i];
				}
				samples -= n;
				if(AnalyzeSamples(lbuffer, rbuffer, n, 2) != GAIN_ANALYSIS_OK)
					return false;
			}
		}
		else {
			while(samples > 0) {
				const unsigned n = local_min(samples, nbuffer);
				for(i = 0; i < n; i++)
					lbuffer[i] = (Float_t)input[0][i];
				samples -= n;
				if(AnalyzeSamples(lbuffer, 0, n, 1) != GAIN_ANALYSIS_OK)
					return false;
			}
		}
	}
	else {
		const double scale = (
			(bps > 16)?
				(double)1. / (double)(1u << (bps - 16)) :
				(double)(1u << (16 - bps))
		);

		if(is_stereo) {
			while(samples > 0) {
				const unsigned n = local_min(samples, nbuffer);
				for(i = 0; i < n; i++) {
					lbuffer[i] = (Float_t)(scale * (double)input[0][i]);
					rbuffer[i] = (Float_t)(scale * (double)input[1][i]);
				}
				samples -= n;
				if(AnalyzeSamples(lbuffer, rbuffer, n, 2) != GAIN_ANALYSIS_OK)
					return false;
			}
		}
		else {
			while(samples > 0) {
				const unsigned n = local_min(samples, nbuffer);
				for(i = 0; i < n; i++)
					lbuffer[i] = (Float_t)(scale * (double)input[0][i]);
				samples -= n;
				if(AnalyzeSamples(lbuffer, 0, n, 1) != GAIN_ANALYSIS_OK)
					return false;
			}
		}
	}

	return true;
}

REPLAYGAIN_API float FLAC__replaygain_get_album_gain()
{
	return GetAlbumGain();
}

REPLAYGAIN_API float FLAC__replaygain_get_title_gain()
{
	return GetTitleGain();
}
