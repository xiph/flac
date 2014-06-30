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
#include <stdint.h>
#include <string.h>

#include "FLAC/assert.h"
#include "share/endswap.h"
#include "private/md5.h"
#include "endswap.h"


#define SWAP_TEST(str, bool) \
		{	printf("%s ... ", str); \
			if (bool) { \
			    puts("FAILED"); \
			    return false; \
			} \
			puts("OK"); \
		}


FLAC__bool test_endswap(void)
{
	int16_t i16 = 0x1234;
	uint16_t u16 = 0xabcd;
	int32_t i32 = 0x12345678;
	uint32_t u32 = 0xabcdef01;

	printf("\n+++ libFLAC unit test: endswap\n\n");

	SWAP_TEST("testing ENDSWAP_16 on int16_t",
		((int16_t) ENDSWAP_16 (i16)) == i16 || ((int16_t) ENDSWAP_16 (ENDSWAP_16 (i16))) != i16);

	SWAP_TEST("testing ENDSWAP_16 on uint16_t",
		((uint16_t) ENDSWAP_16 (u16)) == u16 || ((uint16_t) ENDSWAP_16 (ENDSWAP_16 (u16))) != u16);

	SWAP_TEST("testing ENDSWAP_32 on int32_t",
		((int32_t) ENDSWAP_32 (i32)) == i32 || ((int32_t) ENDSWAP_32 (ENDSWAP_32 (i32))) != i32);

	SWAP_TEST("testing ENDSWAP_32 on uint32_t",
		((uint32_t) ENDSWAP_32 (u32)) == u32 || ((uint32_t) ENDSWAP_32 (ENDSWAP_32 (u32))) != u32);

	printf("\nPASSED!\n");
	return true;
}
