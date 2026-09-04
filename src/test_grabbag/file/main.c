/* test_file - Tester for the TOCTOU fix in grabbag__file_change_stats()
 * and grabbag__file_copy_metadata() (GH-902)
 * Copyright (C) 2011-2025  Xiph.Org Foundation
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
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "share/grabbag.h"

static int fails = 0;

#define CHECK(cond, msg) do { \
	if(!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); fails++; } \
	else fprintf(stderr, "ok: %s\n", msg); \
} while(0)

static void test_basic_roundtrip(void)
{
	const char *f1 = "test_file_a.tmp";
	struct stat st;
	FILE *fp = fopen(f1, "w"); fputs("hi", fp); fclose(fp);
	chmod(f1, 0644);

	CHECK(grabbag__file_change_stats(f1, /*read_only=*/true), "set read-only succeeds");
	stat(f1, &st);
	CHECK(!(st.st_mode & 0222), "file has no write bits after read_only=true");

	CHECK(grabbag__file_change_stats(f1, /*read_only=*/false), "re-enable write succeeds");
	stat(f1, &st);
	CHECK((st.st_mode & S_IWUSR) != 0, "owner write bit restored");

	unlink(f1);
}

static void test_copy_metadata(void)
{
	const char *f1 = "test_file_b.tmp";
	const char *f2 = "test_file_c.tmp";
	struct stat st;
	FILE *fp;

	fp = fopen(f1, "w"); fputs("hi", fp); fclose(fp);
	chmod(f1, 0640);
	fp = fopen(f2, "w"); fputs("hi2", fp); fclose(fp);

	grabbag__file_copy_metadata(f1, f2);
	stat(f2, &st);
	CHECK((st.st_mode & 0777) == 0640, "copy_metadata applies source mode to dest");

	unlink(f1);
	unlink(f2);
}

/* GH-902: a symlink swapped into place between reading a file's mode and
 * applying a new one used to let chmod() run on whatever the symlink
 * pointed to instead of the intended file. Confirm the fixed function
 * refuses to touch anything through a symlink at all, and that a tight
 * race against a swapping symlink can no longer leak through. */
static void test_symlink_is_refused(void)
{
	const char *real = "test_file_real.tmp";
	const char *link = "test_file_link.tmp";
	struct stat st;
	int fd;

	fd = open(real, O_CREAT | O_WRONLY, 0644); close(fd);
	unlink(link);
	symlink(real, link);

	CHECK(!grabbag__file_change_stats(link, /*read_only=*/true), "refuses to operate through a symlink");
	stat(real, &st);
	CHECK((st.st_mode & 0777) == 0644, "the symlink target's mode is untouched");

	unlink(real);
	unlink(link);
}

static const char *race_target = "test_file_race_target.tmp";
static const char *race_secret = "test_file_race_secret.tmp";
static volatile int race_stop = 0;

static void *race_swap_loop(void *arg)
{
	int fd;
	(void)arg;
	while(!race_stop) {
		unlink(race_target);
		symlink(race_secret, race_target);
		usleep(1);
		unlink(race_target);
		fd = open(race_target, O_CREAT | O_WRONLY, 0644);
		if(fd >= 0) close(fd);
	}
	return NULL;
}

static void test_race_does_not_leak(void)
{
	pthread_t t;
	struct stat st;
	int fd, i;

	fd = open(race_secret, O_CREAT | O_WRONLY, 0600);
	close(fd);
	chmod(race_secret, 0600);

	pthread_create(&t, NULL, race_swap_loop, NULL);
	for(i = 0; i < 50000; i++)
		grabbag__file_change_stats(race_target, i % 2 == 0);
	race_stop = 1;
	pthread_join(t, NULL);

	stat(race_secret, &st);
	CHECK((st.st_mode & 0777) == 0600, "a swapping symlink race never touches an unrelated file's permissions");

	unlink(race_target);
	unlink(race_secret);
}

int main(void)
{
	test_basic_roundtrip();
	test_copy_metadata();
	test_symlink_is_refused();
	test_race_does_not_leak();

	if(fails) {
		fprintf(stderr, "%d check(s) FAILED\n", fails);
		return 1;
	}
	fprintf(stderr, "all checks passed\n");
	return 0;
}
