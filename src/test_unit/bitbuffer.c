/* test_unit - Simple FLAC unit tester
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

#include "private/bitbuffer.h" /* from the libFLAC private include area */

static bool dummy_read_callback(byte buffer[], unsigned *bytes, void *client_data)
{
	(void)buffer, (void)bytes, (void)client_data;
	return true;
}

int test_bitbuffer()
{
	FLAC__BitBuffer bb, bb_zero, bb_one, bbcopy;
	bool ok;
	unsigned i, j;
	static byte test_pattern1[19] = { 0xaa, 0xf0, 0xaa, 0xbe, 0xaa, 0xaa, 0xaa, 0xa8, 0x30, 0x0a, 0xaa, 0xaa, 0xaa, 0xad, 0xea, 0xdb, 0xee, 0xfa, 0xce };

	printf("testing init... OK\n");
	FLAC__bitbuffer_init(&bb);
	FLAC__bitbuffer_init(&bb_zero);
	FLAC__bitbuffer_init(&bb_one);
	FLAC__bitbuffer_init(&bbcopy);

	printf("testing clear... ");
	ok = FLAC__bitbuffer_clear(&bb) && FLAC__bitbuffer_clear(&bb_zero) && FLAC__bitbuffer_clear(&bb_one) && FLAC__bitbuffer_clear(&bbcopy);
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok)
		return 1;

	printf("setting up bb_one... ");
	ok = FLAC__bitbuffer_write_raw_uint32(&bb_one, 1, 7) && FLAC__bitbuffer_read_raw_uint32(&bb_one, &i, 6, dummy_read_callback, 0);
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok)
		return 1;
	FLAC__bitbuffer_dump(&bb_one, stdout);

	printf("capacity = %u\n", bb.capacity);

	printf("testing zeroes, raw_uint32*... ");
	ok =
		FLAC__bitbuffer_write_raw_uint32(&bb, 0x1, 1) &&
		FLAC__bitbuffer_write_raw_uint32(&bb, 0x1, 2) &&
		FLAC__bitbuffer_write_raw_uint32(&bb, 0xa, 5) &&
		FLAC__bitbuffer_write_raw_uint32(&bb, 0xf0, 8) &&
		FLAC__bitbuffer_write_raw_uint32(&bb, 0x2aa, 10) &&
		FLAC__bitbuffer_write_raw_uint32(&bb, 0xf, 4) &&
		FLAC__bitbuffer_write_raw_uint32(&bb, 0xaaaaaaaa, 32) &&
		FLAC__bitbuffer_write_zeroes(&bb, 4) &&
		FLAC__bitbuffer_write_raw_uint32(&bb, 0x3, 2) &&
		FLAC__bitbuffer_write_zeroes(&bb, 8) &&
		FLAC__bitbuffer_write_raw_uint64(&bb, 0xaaaaaaaadeadbeef, 64) &&
		FLAC__bitbuffer_write_raw_uint32(&bb, 0xace, 12)
	;
	if(!ok) {
		printf("FAILED\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.bytes != sizeof(test_pattern1)) {
		printf("FAILED byte count %u != %u\n", bb.bytes, sizeof(test_pattern1));
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.bits != 0) {
		printf("FAILED bit count %u != 0\n", bb.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.total_bits != 8*bb.bytes+bb.bits) {
		printf("FAILED total_bits count %u != %u (%u:%u)\n", bb.total_bits, 8*bb.bytes+bb.bits, bb.bytes, bb.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(memcmp(bb.buffer, test_pattern1, sizeof(byte)*sizeof(test_pattern1)) != 0) {
		printf("FAILED pattern match\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	printf("OK\n");
	FLAC__bitbuffer_dump(&bb, stdout);

	printf("testing raw_uint32 some more... ");
	ok = FLAC__bitbuffer_write_raw_uint32(&bb, 0x3d, 6);
	if(!ok) {
		printf("FAILED\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.bytes != sizeof(test_pattern1)) {
		printf("FAILED byte count %u != %u\n", bb.bytes, sizeof(test_pattern1));
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.bits != 6) {
		printf("FAILED bit count %u != 6\n", bb.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.total_bits != 8*bb.bytes+bb.bits) {
		printf("FAILED total_bits count %u != %u (%u:%u)\n", bb.total_bits, 8*bb.bytes+bb.bits, bb.bytes, bb.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(memcmp(bb.buffer, test_pattern1, sizeof(byte)*sizeof(test_pattern1)) != 0 || bb.buffer[bb.bytes] != 0x3d) {
		printf("FAILED pattern match\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	printf("OK\n");
	FLAC__bitbuffer_dump(&bb, stdout);

	printf("testing concatenate_aligned (bb_zero)... ");
	ok = FLAC__bitbuffer_concatenate_aligned(&bb, &bb_zero);
	if(!ok) {
		printf("FAILED\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.bytes != sizeof(test_pattern1)) {
		printf("FAILED byte count %u != %u\n", bb.bytes, sizeof(test_pattern1));
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.bits != 6) {
		printf("FAILED bit count %u != 6\n", bb.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.total_bits != 8*bb.bytes+bb.bits) {
		printf("FAILED total_bits count %u != %u (%u:%u)\n", bb.total_bits, 8*bb.bytes+bb.bits, bb.bytes, bb.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(memcmp(bb.buffer, test_pattern1, sizeof(byte)*sizeof(test_pattern1)) != 0 || bb.buffer[bb.bytes] != 0x3d) {
		printf("FAILED pattern match\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	printf("OK\n");
	FLAC__bitbuffer_dump(&bb, stdout);

	printf("testing concatenate_aligned (bb_one)... ");
	ok = FLAC__bitbuffer_concatenate_aligned(&bb, &bb_one);
	if(!ok) {
		printf("FAILED\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.bytes != sizeof(test_pattern1)) {
		printf("FAILED byte count %u != %u\n", bb.bytes, sizeof(test_pattern1));
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.bits != 7) {
		printf("FAILED bit count %u != 7\n", bb.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.total_bits != 8*bb.bytes+bb.bits) {
		printf("FAILED total_bits count %u != %u (%u:%u)\n", bb.total_bits, 8*bb.bytes+bb.bits, bb.bytes, bb.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(memcmp(bb.buffer, test_pattern1, sizeof(byte)*sizeof(test_pattern1)) != 0 || bb.buffer[bb.bytes] != 0x7b) {
		printf("FAILED pattern match\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	printf("OK\n");
	FLAC__bitbuffer_dump(&bb, stdout);

	printf("testing concatenate_aligned (bb_one again)... ");
	(void)FLAC__bitbuffer_write_raw_uint32(&bb_one, 1, 1);
	(void)FLAC__bitbuffer_read_raw_uint32(&bb_one, &i, 1, dummy_read_callback, 0);
	ok = FLAC__bitbuffer_concatenate_aligned(&bb, &bb_one);
	if(!ok) {
		printf("FAILED\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.bytes != sizeof(test_pattern1)+1) {
		printf("FAILED byte count %u != %u\n", bb.bytes, sizeof(test_pattern1)+1);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.bits != 0) {
		printf("FAILED bit count %u != 0\n", bb.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.total_bits != 8*bb.bytes+bb.bits) {
		printf("FAILED total_bits count %u != %u (%u:%u)\n", bb.total_bits, 8*bb.bytes+bb.bits, bb.bytes, bb.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(memcmp(bb.buffer, test_pattern1, sizeof(byte)*sizeof(test_pattern1)) != 0 || bb.buffer[bb.bytes-1] != 0xf7) {
		printf("FAILED pattern match\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	printf("OK\n");
	FLAC__bitbuffer_dump(&bb, stdout);

	printf("testing concatenate_aligned (bb_four)... ");
	(void)FLAC__bitbuffer_clear(&bb_one);
	(void)FLAC__bitbuffer_write_raw_uint32(&bb_one, 8, 4);
	ok = FLAC__bitbuffer_concatenate_aligned(&bb, &bb_one);
	if(!ok) {
		printf("FAILED\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.bytes != sizeof(test_pattern1)+1) {
		printf("FAILED byte count %u != %u\n", bb.bytes, sizeof(test_pattern1)+1);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.bits != 4) {
		printf("FAILED bit count %u != 4\n", bb.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.total_bits != 8*bb.bytes+bb.bits) {
		printf("FAILED total_bits count %u != %u (%u:%u)\n", bb.total_bits, 8*bb.bytes+bb.bits, bb.bytes, bb.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(memcmp(bb.buffer, test_pattern1, sizeof(byte)*sizeof(test_pattern1)) != 0 || bb.buffer[bb.bytes] != 0x08) {
		printf("FAILED pattern match\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	printf("OK\n");
	FLAC__bitbuffer_dump(&bb, stdout);

	printf("testing concatenate_aligned (bb_eight)... ");
	(void)FLAC__bitbuffer_read_raw_uint32(&bb_one, &i, 4, dummy_read_callback, 0);
	(void)FLAC__bitbuffer_write_raw_uint32(&bb_one, 0xaa, 8);
	ok = FLAC__bitbuffer_concatenate_aligned(&bb, &bb_one);
	if(!ok) {
		printf("FAILED\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.bytes != sizeof(test_pattern1)+2) {
		printf("FAILED byte count %u != %u\n", bb.bytes, sizeof(test_pattern1)+2);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.bits != 4) {
		printf("FAILED bit count %u != 4\n", bb.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.total_bits != 8*bb.bytes+bb.bits) {
		printf("FAILED total_bits count %u != %u (%u:%u)\n", bb.total_bits, 8*bb.bytes+bb.bits, bb.bytes, bb.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(memcmp(bb.buffer, test_pattern1, sizeof(byte)*sizeof(test_pattern1)) != 0 || bb.buffer[bb.bytes-1] != 0x8a || bb.buffer[bb.bytes] != 0x0a) {
		printf("FAILED pattern match\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	printf("OK\n");
	FLAC__bitbuffer_dump(&bb, stdout);

	printf("testing concatenate_aligned (bb_seventeen)... ");
	(void)FLAC__bitbuffer_write_raw_uint32(&bb_one, 0x155, 9);
	ok = FLAC__bitbuffer_concatenate_aligned(&bb, &bb_one);
	if(!ok) {
		printf("FAILED\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.bytes != sizeof(test_pattern1)+4) {
		printf("FAILED byte count %u != %u\n", bb.bytes, sizeof(test_pattern1)+4);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.bits != 5) {
		printf("FAILED bit count %u != 5\n", bb.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(bb.total_bits != 8*bb.bytes+bb.bits) {
		printf("FAILED total_bits count %u != %u (%u:%u)\n", bb.total_bits, 8*bb.bytes+bb.bits, bb.bytes, bb.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	if(memcmp(bb.buffer, test_pattern1, sizeof(byte)*sizeof(test_pattern1)) != 0 || bb.buffer[bb.bytes-3] != 0x8a || bb.buffer[bb.bytes-2] != 0xaa || bb.buffer[bb.bytes-1] != 0xaa || bb.buffer[bb.bytes] != 0x15) {
		printf("FAILED pattern match\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	printf("OK\n");
	FLAC__bitbuffer_dump(&bb, stdout);

	printf("testing utf8_uint32(0x00000000)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint32(&bb, 0x00000000);
	ok = bb.total_bits == 8 && bb.buffer[0] == 0;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint32(0x0000007F)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint32(&bb, 0x0000007F);
	ok = bb.total_bits == 8 && bb.buffer[0] == 0x7F;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint32(0x00000080)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint32(&bb, 0x00000080);
	ok = bb.total_bits == 16 && bb.buffer[0] == 0xC2 && bb.buffer[1] == 0x80;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint32(0x000007FF)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint32(&bb, 0x000007FF);
	ok = bb.total_bits == 16 && bb.buffer[0] == 0xDF && bb.buffer[1] == 0xBF;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint32(0x00000800)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint32(&bb, 0x00000800);
	ok = bb.total_bits == 24 && bb.buffer[0] == 0xE0 && bb.buffer[1] == 0xA0 && bb.buffer[2] == 0x80;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint32(0x0000FFFF)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint32(&bb, 0x0000FFFF);
	ok = bb.total_bits == 24 && bb.buffer[0] == 0xEF && bb.buffer[1] == 0xBF && bb.buffer[2] == 0xBF;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint32(0x00010000)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint32(&bb, 0x00010000);
	ok = bb.total_bits == 32 && bb.buffer[0] == 0xF0 && bb.buffer[1] == 0x90 && bb.buffer[2] == 0x80 && bb.buffer[3] == 0x80;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint32(0x001FFFFF)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint32(&bb, 0x001FFFFF);
	ok = bb.total_bits == 32 && bb.buffer[0] == 0xF7 && bb.buffer[1] == 0xBF && bb.buffer[2] == 0xBF && bb.buffer[3] == 0xBF;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint32(0x00200000)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint32(&bb, 0x00200000);
	ok = bb.total_bits == 40 && bb.buffer[0] == 0xF8 && bb.buffer[1] == 0x88 && bb.buffer[2] == 0x80 && bb.buffer[3] == 0x80 && bb.buffer[4] == 0x80;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint32(0x03FFFFFF)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint32(&bb, 0x03FFFFFF);
	ok = bb.total_bits == 40 && bb.buffer[0] == 0xFB && bb.buffer[1] == 0xBF && bb.buffer[2] == 0xBF && bb.buffer[3] == 0xBF && bb.buffer[4] == 0xBF;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint32(0x04000000)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint32(&bb, 0x04000000);
	ok = bb.total_bits == 48 && bb.buffer[0] == 0xFC && bb.buffer[1] == 0x84 && bb.buffer[2] == 0x80 && bb.buffer[3] == 0x80 && bb.buffer[4] == 0x80 && bb.buffer[5] == 0x80;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint32(0x7FFFFFFF)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint32(&bb, 0x7FFFFFFF);
	ok = bb.total_bits == 48 && bb.buffer[0] == 0xFD && bb.buffer[1] == 0xBF && bb.buffer[2] == 0xBF && bb.buffer[3] == 0xBF && bb.buffer[4] == 0xBF && bb.buffer[5] == 0xBF;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint64(0x0000000000000000)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint64(&bb, 0x0000000000000000);
	ok = bb.total_bits == 8 && bb.buffer[0] == 0;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint64(0x000000000000007F)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint64(&bb, 0x000000000000007F);
	ok = bb.total_bits == 8 && bb.buffer[0] == 0x7F;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint64(0x0000000000000080)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint64(&bb, 0x0000000000000080);
	ok = bb.total_bits == 16 && bb.buffer[0] == 0xC2 && bb.buffer[1] == 0x80;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint64(0x00000000000007FF)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint64(&bb, 0x00000000000007FF);
	ok = bb.total_bits == 16 && bb.buffer[0] == 0xDF && bb.buffer[1] == 0xBF;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint64(0x0000000000000800)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint64(&bb, 0x0000000000000800);
	ok = bb.total_bits == 24 && bb.buffer[0] == 0xE0 && bb.buffer[1] == 0xA0 && bb.buffer[2] == 0x80;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint64(0x000000000000FFFF)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint64(&bb, 0x000000000000FFFF);
	ok = bb.total_bits == 24 && bb.buffer[0] == 0xEF && bb.buffer[1] == 0xBF && bb.buffer[2] == 0xBF;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint64(0x0000000000010000)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint64(&bb, 0x0000000000010000);
	ok = bb.total_bits == 32 && bb.buffer[0] == 0xF0 && bb.buffer[1] == 0x90 && bb.buffer[2] == 0x80 && bb.buffer[3] == 0x80;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint64(0x00000000001FFFFF)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint64(&bb, 0x00000000001FFFFF);
	ok = bb.total_bits == 32 && bb.buffer[0] == 0xF7 && bb.buffer[1] == 0xBF && bb.buffer[2] == 0xBF && bb.buffer[3] == 0xBF;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint64(0x0000000000200000)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint64(&bb, 0x0000000000200000);
	ok = bb.total_bits == 40 && bb.buffer[0] == 0xF8 && bb.buffer[1] == 0x88 && bb.buffer[2] == 0x80 && bb.buffer[3] == 0x80 && bb.buffer[4] == 0x80;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint64(0x0000000003FFFFFF)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint64(&bb, 0x0000000003FFFFFF);
	ok = bb.total_bits == 40 && bb.buffer[0] == 0xFB && bb.buffer[1] == 0xBF && bb.buffer[2] == 0xBF && bb.buffer[3] == 0xBF && bb.buffer[4] == 0xBF;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint64(0x0000000004000000)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint64(&bb, 0x0000000004000000);
	ok = bb.total_bits == 48 && bb.buffer[0] == 0xFC && bb.buffer[1] == 0x84 && bb.buffer[2] == 0x80 && bb.buffer[3] == 0x80 && bb.buffer[4] == 0x80 && bb.buffer[5] == 0x80;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint64(0x000000007FFFFFFF)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint64(&bb, 0x000000007FFFFFFF);
	ok = bb.total_bits == 48 && bb.buffer[0] == 0xFD && bb.buffer[1] == 0xBF && bb.buffer[2] == 0xBF && bb.buffer[3] == 0xBF && bb.buffer[4] == 0xBF && bb.buffer[5] == 0xBF;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint64(0x0000000080000000)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint64(&bb, 0x0000000080000000);
	ok = bb.total_bits == 56 && bb.buffer[0] == 0xFE && bb.buffer[1] == 0x82 && bb.buffer[2] == 0x80 && bb.buffer[3] == 0x80 && bb.buffer[4] == 0x80 && bb.buffer[5] == 0x80 && bb.buffer[6] == 0x80;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing utf8_uint64(0x0000000FFFFFFFFF)... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_utf8_uint64(&bb, 0x0000000FFFFFFFFF);
	ok = bb.total_bits == 56 && bb.buffer[0] == 0xFE && bb.buffer[1] == 0xBF && bb.buffer[2] == 0xBF && bb.buffer[3] == 0xBF && bb.buffer[4] == 0xBF && bb.buffer[5] == 0xBF && bb.buffer[6] == 0xBF;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}

	printf("testing grow... ");
	FLAC__bitbuffer_clear(&bb);
	FLAC__bitbuffer_write_raw_uint32(&bb, 0xa, 4);
	j = bb.capacity;
	for(i = 0; i < j; i++)
		FLAC__bitbuffer_write_raw_uint32(&bb, 0xaa, 8);
	ok = bb.total_bits = i*8+4 && bb.buffer[0] == 0xaa && bb.buffer[i] == 0xa;
	printf("%s\n", ok?"OK":"FAILED");
	if(!ok) {
		FLAC__bitbuffer_dump(&bb, stdout);
		return 1;
	}
	printf("capacity = %u\n", bb.capacity);

	printf("testing clone... ");
	ok = FLAC__bitbuffer_clone(&bbcopy, &bb);
	if(!ok) {
		printf("FAILED\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		FLAC__bitbuffer_dump(&bbcopy, stdout);
		return 1;
	}
	if(bb.bytes != bbcopy.bytes) {
		printf("FAILED byte count %u != %u\n", bb.bytes, bbcopy.bytes);
		FLAC__bitbuffer_dump(&bb, stdout);
		FLAC__bitbuffer_dump(&bbcopy, stdout);
		return 1;
	}
	if(bb.bits != bbcopy.bits) {
		printf("FAILED bit count %u != %u\n", bb.bits, bbcopy.bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		FLAC__bitbuffer_dump(&bbcopy, stdout);
		return 1;
	}
	if(bb.total_bits != bbcopy.total_bits) {
		printf("FAILED total_bits count %u != %u\n", bb.total_bits, bbcopy.total_bits);
		FLAC__bitbuffer_dump(&bb, stdout);
		FLAC__bitbuffer_dump(&bbcopy, stdout);
		return 1;
	}
	if(memcmp(bb.buffer, bbcopy.buffer, sizeof(byte)*bb.capacity) != 0) {
		printf("FAILED pattern match\n");
		FLAC__bitbuffer_dump(&bb, stdout);
		FLAC__bitbuffer_dump(&bbcopy, stdout);
		return 1;
	}
	printf("OK\n");

	printf("testing free... OK\n");
	FLAC__bitbuffer_free(&bb);
	FLAC__bitbuffer_free(&bbcopy);

	printf("\nPASSED!\n");
	return 0;
}
