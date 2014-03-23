/* test_libFLAC - Unit tester for libFLAC
 * Copyright (C) 2014  Xiph.Org Foundation
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>

#include "FLAC/assert.h"
#include "share/compat.h"
#include "private/md5.h"
#include "md5.h"


FLAC__bool test_md5(void)
{
	FLAC__MD5Context ctx;
	FLAC__byte digest[16];
	unsigned k ;
	char * cptr;

	printf("\n+++ libFLAC unit test: md5\n\n");

	printf("testing FLAC__MD5Init ... ");
	FLAC__MD5Init (&ctx);
	if (ctx.buf[0] != 0x67452301) {
		printf("FAILED!\n");
		return false;
	}
	printf("OK\n");

	printf("testing that FLAC__MD5Final clears the MD5Context ... ");
	FLAC__MD5Final(digest, &ctx);
	cptr = (char*) &ctx ;
	for (k = 0 ; k < sizeof (ctx) ; k++) {
		if (cptr [k]) {
			printf("FAILED, MD5 ctx has not been cleared after FLAC__MD5Final\n");
			return false;
		}
	}
	printf("OK\n");

	printf("\nPASSED!\n");
	return true;
}
