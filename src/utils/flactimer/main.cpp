/* flactimer - Runs a command and prints timing information
 * Copyright (C) 2007-2009  Josh Coalson
 * Copyright (C) 2011-2016  Xiph.Org Foundation
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
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "share/compat.h"
#include "share/safe_str.h"

static inline FLAC__uint64 time2nsec(const FILETIME &t)
{
	FLAC__uint64 n = t.dwHighDateTime;
	n <<= 32;
	n |= (FLAC__uint64)t.dwLowDateTime;
	return n * 100;
}

static void printtime(FILE *fout, FLAC__uint64 nsec, FLAC__uint64 total)
{
	FLAC__uint32 pct = (FLAC__uint32)(100.0 * ((double)nsec / (double)total));
	FLAC__uint64 msec = nsec / 1000000; nsec -= msec * 1000000;
	FLAC__uint64 sec = msec / 1000; msec -= sec * 1000;
	FLAC__uint64 min = sec / 60; sec -= min * 60;
	FLAC__uint64 hour = min / 60; min -= hour * 60;
	fprintf(fout, " %5u.%03u = %02u:%02u:%02u.%03u = %3u%%\n",
		(FLAC__uint32)((hour*60+min)*60+sec),
		(FLAC__uint32)msec,
		(FLAC__uint32)hour,
		(FLAC__uint32)min,
		(FLAC__uint32)sec,
		(FLAC__uint32)msec,
		pct
	);
}

int main(int argc, char *argv[])
{
	const char *usage = "usage: flactimer [-1 | -2 | -o outputfile] command\n";
	FILE *fout = stderr;

	if(argc == 1 || (argc > 1 && 0 == strcmp(argv[1], "-h"))) {
		fprintf(stderr, usage);
		return 0;
	}
	argv++;
	argc--;
	if(0 == strcmp(argv[0], "-1") || 0 == strcmp(argv[0], "/1")) {
		fout = stdout;
		argv++;
		argc--;
	}
	else if(0 == strcmp(argv[0], "-2") || 0 == strcmp(argv[0], "/2")) {
		fout = stdout;
		argv++;
		argc--;
	}
	else if(0 == strcmp(argv[0], "-o")) {
		if(argc < 2) {
			fprintf(stderr, usage);
			return 1;
		}
		fout = fopen(argv[1], "w");
		if(!fout) {
			fprintf(stderr, "ERROR opening file %s for writing\n", argv[1]);
			return 1;
		}
		argv += 2;
		argc -= 2;
	}
	if(argc <= 0) {
		fprintf(fout, "ERROR, no command!\n\n");
		fprintf(fout, usage);
		fclose(fout);
		return 1;
	}

	// improvement: double-quote all args
	int i, n = 0;
	for(i = 0; i < argc; i++) {
		if(i > 0)
			n++;
		n += strlen(argv[i]);
	}
	char *args = (char*)malloc(n+1);
	if(!args) {
		fprintf(fout, "ERROR, no memory\n");
		fclose(fout);
		return 1;
	}
	args[0] = '\0';
	for(i = 0; i < argc; i++) {
		if(i > 0)
			safe_strncat(args, " ", sizeof(args));
		safe_strncat(args, argv[i], sizeof(args));
	}

	//fprintf(stderr, "@@@ cmd=[%s] args=[%s]\n", argv[0], args);

	STARTUPINFOA si;
	GetStartupInfoA(&si);

	DWORD wallclock_msec = GetTickCount();

	PROCESS_INFORMATION pi;
	BOOL ok = CreateProcessA(
		argv[0], // lpApplicationName
		args, // lpCommandLine
		NULL, // lpProcessAttributes
		NULL, // lpThreadAttributes
		FALSE, // bInheritHandles
		0, // dwCreationFlags
		NULL, // lpEnvironment
		NULL, // lpCurrentDirectory
		&si, // lpStartupInfo (inherit from this proc?)
		&pi // lpProcessInformation
	);

	if(!ok) {
		fprintf(fout, "ERROR running command\n");
		free(args); //@@@ ok to free here or have to wait to wait till process is reaped?
		fclose(fout);
		return 1;
	}

	//fprintf(stderr, "@@@ waiting...\n");
	WaitForSingleObject(pi.hProcess, INFINITE);
	//fprintf(stderr, "@@@ done\n");

	wallclock_msec = GetTickCount() - wallclock_msec;

	FILETIME creation_time;
	FILETIME exit_time;
	FILETIME kernel_time;
	FILETIME user_time;
	if(!GetProcessTimes(pi.hProcess, &creation_time, &exit_time, &kernel_time, &user_time)) {
		fprintf(fout, "ERROR getting time info\n");
		free(args); //@@@ ok to free here or have to wait to wait till process is reaped?
		fclose(fout);
		return 1;
	}
	FLAC__uint64 kernel_nsec = time2nsec(kernel_time);
	FLAC__uint64 user_nsec = time2nsec(user_time);

	fprintf(fout, "Kernel Time  = "); printtime(fout, kernel_nsec, (FLAC__uint64)wallclock_msec * 1000000);
	fprintf(fout, "User Time    = "); printtime(fout, user_nsec, (FLAC__uint64)wallclock_msec * 1000000);
	fprintf(fout, "Process Time = "); printtime(fout, kernel_nsec+user_nsec, (FLAC__uint64)wallclock_msec * 1000000);
	fprintf(fout, "Global Time  = "); printtime(fout, (FLAC__uint64)wallclock_msec * 1000000, (FLAC__uint64)wallclock_msec * 1000000);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	free(args); //@@@ always causes crash, maybe CreateProcess takes ownership?
	fclose(fout);
	return 0;
}
