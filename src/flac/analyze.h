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

#ifndef flac__analyze_h
#define flac__analyze_h

typedef struct {
	bool do_residual_text;
	bool do_residual_gnuplot;
} analysis_options;

void analyze_init(analysis_options aopts);
void analyze_frame(const FLAC__Frame *frame, unsigned frame_number, analysis_options aopts, FILE *fout);
void analyze_finish(analysis_options aopts);

#endif
