/* grabbag - Convenience lib for various routines common to several tools
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

#include "share/grabbag.h"
#include "FLAC/assert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GRABBAG_API unsigned grabbag__cuesheet_msf_to_frame(unsigned minutes, unsigned seconds, unsigned frames)
{
	return ((minutes * 60) + seconds) * 75 + frames;
}

GRABBAG_API void grabbag__cuesheet_frame_to_msf(unsigned frame, unsigned *minutes, unsigned *seconds, unsigned *frames)
{
	*frames = frame % 75;
	frame /= 75;
	*seconds = frame % 60;
	frame /= 60;
	*minutes = frame;
}
