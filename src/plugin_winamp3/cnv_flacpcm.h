/* FLAC input plugin for Winamp3
 * Copyright (C) 2000,2001,2002  Josh Coalson
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
 *
 * NOTE: this code is derived from the 'rawpcm' example by
 * Nullsoft; the original license for the 'rawpcm' example follows.
 */
/*

  Nullsoft WASABI Source File License

  Copyright 1999-2001 Nullsoft, Inc.

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.


  Brennan Underwood
  brennan@nullsoft.com

*/

#ifndef _CNV_FLACPCM_H
#define _CNV_FLACPCM_H

#include "studio/wac.h"
#include "common/rootcomp.h"
#include "attribs/cfgitemi.h"
#include "attribs/attrint.h"

#define WACNAME WACcnv_flacpcm

class WACNAME : public WAComponentClient, public CfgItemI {
public:
	WACNAME();
	virtual ~WACNAME();

	virtual const char *getName() { return "FLAC to PCM converter"; };
	virtual GUID getGUID();

	virtual void onRegisterServices();
	virtual void onDestroy();

	virtual int getDisplayComponent() { return FALSE; };

	virtual CfgItem *getCfgInterface(int n) { return this; }
};

#endif
