/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2002,2003  Josh Coalson
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

#ifndef FLAC__PRIVATE__METADATA_H
#define FLAC__PRIVATE__METADATA_H

#include "FLAC/metadata.h"

void FLAC__metadata_object_delete_data(FLAC__StreamMetadata *object);
void FLAC__metadata_object_cuesheet_track_delete_data(FLAC__StreamMetadata_CueSheet_Track *object);

#endif
