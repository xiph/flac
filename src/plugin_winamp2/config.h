/* in_flac - Winamp2 FLAC input plugin
 * Copyright (C) 2002,2003,2004,2005,2006  Josh Coalson
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

#include "playback.h"

/*
 *  common stuff
 */

typedef struct {
	struct {
		char tag_format[256];
		char sep[16];
		WCHAR *tag_format_w;
	} title;
	struct {
		BOOL reserve_space;
	} tag;
	struct {
		FLAC__bool show_bps;
	} display;
	output_config_t output;
} flac_config_t;

extern flac_config_t flac_cfg;

/*
 *  prototypes
 */

void InitConfig();
void ReadConfig();
void WriteConfig();
int  DoConfig(HINSTANCE inst, HWND parent);
