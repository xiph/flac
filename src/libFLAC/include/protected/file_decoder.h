/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001  Josh Coalson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#ifndef FLAC__PROTECTED__FILE_DECODER_H
#define FLAC__PROTECTED__FILE_DECODER_H

#include "FLAC/file_decoder.h"

typedef struct FLAC__FileDecoderProtected {
	bool check_md5; /* if true, generate MD5 signature of decoded data and compare against signature in the Encoding metadata block */
	FLAC__FileDecoderState state;
} FLAC__FileDecoderProtected;

#endif
