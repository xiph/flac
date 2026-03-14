/* test_escapes - Simple tester for escapes routines in grabbag
 * Copyright (C) 2026  Xiph.Org Foundation
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
#include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FLAC/assert.h"
#include "share/grabbag.h"

static void print_hexdump(const char *str, size_t length)
{
	size_t i;
	for(i = 0; i < length; ++i) {
		unsigned char c = (unsigned char)str[i];
		if((c >= 0x20) && (c <= 0x7E)) {
			printf("%c", c);
		}
		else {
			printf("\\x%.2x", c);
		}
	}
}

static FLAC__bool test_escape()
{
	static const struct
	{
		const char *unescaped;
		const char *escaped;
	} test_data[] = {
		{ "\\", "\\\\" },
		{ "\r", "\\r" },
		{ "\n", "\\n" },

		/* one character at front */
		{ "a"
		  "\\",
		  "a"
		  "\\\\" },
		{ "a"
		  "\r",
		  "a"
		  "\\r" },
		{ "a"
		  "\n",
		  "a"
		  "\\n" },

		/* one character at end */
		{ "\\"
		  "a",
		  "\\\\"
		  "a" },
		{ "\r"
		  "a",
		  "\\r"
		  "a" },
		{ "\n"
		  "a",
		  "\\n"
		  "a" },

		/* escape in the middle */
		{ "a\\a", "a\\\\a" },
		{ "a\ra", "a\\ra" },
		{ "a\na", "a\\na" },

		/* characters in the middle */
		{ "\\a\\", "\\\\a\\\\" },
		{ "\ra\r", "\\ra\\r" },
		{ "\na\n", "\\na\\n" },

		/* double escaped */
		{ "\\\\\\", "\\\\\\\\\\\\" },
		{ "\\r", "\\\\r" },
		{ "\\n", "\\\\n" },
	};
	const size_t test_data_size = sizeof(test_data) / sizeof(test_data[0]);
	size_t i;
	FLAC__bool result = true;

	for(i = 0; i < test_data_size; ++i) {
		char *escaped = NULL;
		char *unescaped = NULL;
		FLAC__bool test_result = true;

		printf("Test escape %u:", (unsigned)i);

		if(!grabbag__escape_string_needed(test_data[i].unescaped, strlen(test_data[i].unescaped))) {
			test_result = false;
			printf(" ERROR: expected grabbag__escape_string_needed() to return true, but it didn't\n");
			printf("\n");
		}

		escaped = grabbag__create_escaped_string(test_data[i].unescaped, strlen(test_data[i].unescaped));
		if(escaped != NULL) {
			const size_t expected_escaped_size = strlen(test_data[i].escaped);
			const size_t escaped_size = strlen(escaped);
			if((expected_escaped_size != escaped_size) ||
			   (0 != memcmp(escaped, test_data[i].escaped, escaped_size))) {
				printf(" ERROR: grabbag__create_escaped_string() mismatch:\n");
				printf("  Expected: ");
				print_hexdump(escaped, expected_escaped_size);
				printf("\n");
				printf("  Got     : ");
				print_hexdump(test_data[i].escaped, escaped_size);
				printf("\n");
				test_result = false;
			}
			free(escaped);
			escaped = NULL;
		}
		else {
			printf(" ERROR: grabbag__create_escaped_string() returned NULL.");
			test_result = false;
		}

		if(!grabbag__unescape_string_needed(test_data[i].escaped, strlen(test_data[i].escaped))) {
			test_result = false;
			printf(" ERROR: expected grabbag__unescape_string_needed() to return true, but it didn't\n");
			printf("\n");
		}

		unescaped = grabbag__create_unescaped_string(test_data[i].escaped, strlen(test_data[i].escaped));
		if(unescaped != NULL) {
			const size_t expected_unescaped_size = strlen(test_data[i].unescaped);
			const size_t unescaped_size = strlen(unescaped);
			if((expected_unescaped_size != unescaped_size) ||
			   (0 != memcmp(unescaped, test_data[i].unescaped, unescaped_size))) {
				printf(" ERROR: grabbag__create_unescaped_string() mismatch:\n");
				printf("  Expected: ");
				print_hexdump(unescaped, expected_unescaped_size);
				printf("\n");
				printf("  Got     : ");
				print_hexdump(test_data[i].unescaped, unescaped_size);
				printf("\n");
				test_result = false;
			}
			free(unescaped);
			unescaped = NULL;
		}
		else {
			printf(" ERROR: grabbag__create_unescaped_string() returned NULL.\n");
			test_result = false;
		}

		if(test_result) {
			printf(" OK\n");
		}
		else {
			result = false;
		}
	}

	return result;
}

static FLAC__bool test_escape_not_needed()
{
	static const struct
	{
		const char *str;
	} test_data[] = {
		{ "" },
		{ "a" },
		{ "r" },
		{ "n" },
		{ "0" },
		{ "=" },
		{ "A Longer String" },
		{ "\t" },
		{ "\v" },
		{ "\a" },
		{ "\b" },
		{ "\xFF" },
	};
	const size_t test_data_size = sizeof(test_data) / sizeof(test_data[0]);
	size_t i;
	FLAC__bool result = true;

	for(i = 0; i < test_data_size; ++i) {
		const char *str = test_data[i].str;
		const size_t str_size = strlen(test_data[i].str);
		char *escaped = NULL;
		char *unescaped = NULL;
		FLAC__bool test_result = true;

		printf("Test escape not needed %u:", (unsigned)i);

		if(grabbag__escape_string_needed(str, str_size)) {
			test_result = false;
			printf(" ERROR: expected grabbag__escape_string_needed() to return false, but it didn't\n");
			printf("\n");
		}

		escaped = grabbag__create_escaped_string(str, str_size);
		if(escaped != NULL) {
			const size_t expected_escaped_size = str_size;
			const size_t escaped_size = strlen(escaped);
			if((expected_escaped_size != escaped_size) ||
			   (0 != memcmp(escaped, str, escaped_size))) {
				printf(" ERROR: grabbag__create_escaped_string() mismatch:\n");
				printf("  Expected: ");
				print_hexdump(escaped, expected_escaped_size);
				printf("\n");
				printf("  Got     : ");
				print_hexdump(str, str_size);
				printf("\n");
				test_result = false;
			}
			free(escaped);
			escaped = NULL;
		}
		else {
			printf(" ERROR: grabbag__create_escaped_string() returned NULL.");
			test_result = false;
		}

		if(grabbag__unescape_string_needed(str, str_size)) {
			test_result = false;
			printf(" ERROR: expected grabbag__unescape_string_needed() to return false, but it didn't\n");
			printf("\n");
		}

		unescaped = grabbag__create_unescaped_string(str, str_size);
		if(unescaped != NULL) {
			const size_t expected_unescaped_size = str_size;
			const size_t unescaped_size = strlen(unescaped);
			if((expected_unescaped_size != unescaped_size) ||
			   (0 != memcmp(unescaped, str, unescaped_size))) {
				printf(" ERROR: grabbag__create_unescaped_string() mismatch:\n");
				printf("  Expected: ");
				print_hexdump(unescaped, expected_unescaped_size);
				printf("\n");
				printf("  Got     : ");
				print_hexdump(str, str_size);
				printf("\n");
				test_result = false;
			}
			free(unescaped);
			unescaped = NULL;
		}
		else {
			printf(" ERROR: grabbag__create_unescaped_string() returned NULL.\n");
			test_result = false;
		}

		if(test_result) {
			printf(" OK\n");
		}
		else {
			result = false;
		}
	}

	return result;
}

static FLAC__bool test_invalid_escape()
{
	static const struct
	{
		const char *escaped;
	} test_data[] = {
		/* invalid escape */
		{ "\\x20" },
		{ "\\x{20}" },
		{ "\\X20" },
		{ "\\X{20}" },
		{ "\\0" },
		{ "\\01" },
		{ "\\012" },
		{ "\\1" },
		{ "\\12" },
		{ "\\123" },
		{ "\\2" },
		{ "\\3" },
		{ "\\4" },
		{ "\\5" },
		{ "\\6" },
		{ "\\7" },
		{ "\\8" },
		{ "\\9" },
		{ "\\u0020" },
		{ "\\u{20}" },
		{ "\\U00000020" },
		{ "\\U{20}" },
		{ "\\a" },
		{ "\\b" },
		{ "\\c" },
		{ "\\e" },
		{ "\\E" },
		{ "\\f" },
		{ "\\t" },
		{ "\\v" },
		{ "\\\"" },
		{ "\\'" },
		{ "\\\n" },
		{ "\\\r" },
		{ "\\?" },

		/* truncated escape */
		{ "\\" },
		{ "a\\" },
		{ "a\\n\\" },
	};
	const size_t test_data_size = sizeof(test_data) / sizeof(test_data[0]);
	size_t i;
	FLAC__bool result = true;

	for(i = 0; i < test_data_size; ++i) {
		FLAC__bool test_result = true;
		char *unescaped = NULL;

		printf("Test invalid escape %u:", (unsigned)i);

		if(!grabbag__unescape_string_needed(test_data[i].escaped, strlen(test_data[i].escaped))) {
			test_result = false;
			printf(" ERROR: expected grabbag__unescape_string_needed() to return true, but it didn't\n");
			printf("\n");
		}

		unescaped = grabbag__create_unescaped_string(test_data[i].escaped, strlen(test_data[i].escaped));
		if(unescaped != NULL) {
			test_result = false;
			printf(" ERROR: expected grabbag__create_unescaped_string() to fail, but it didn't\n");
			printf("Unescaped mismatch:\n");
			printf("  Expected: NULL\n");
			printf("  Got     : ");
			print_hexdump(unescaped, strlen(unescaped));
			printf("\n");
			free(unescaped);
			unescaped = NULL;
		}

		if(test_result) {
			printf(" OK\n");
		}
		else {
			result = false;
		}
	}

	return result;
}

int main(int argc, char *argv[])
{
	const char *usage = "usage: test_escapes\n";

	if(argc > 1 && 0 == strcmp(argv[1], "-h")) {
		puts(usage);
		return 0;
	}

	if(!test_escape()) {
		return 1;
	}

	if(!test_escape_not_needed()) {
		return 1;
	}

	if(!test_invalid_escape()) {
		return 1;
	}

	return 0;
}
