#ifdef FLAC__STRINGS_IN_UTF8

#include <stdio.h>
#include <sys/stat.h>
#include <sys/utime.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h> /* for WideCharToMultiByte and MultiByteToWideChar */

/* convert WCHAR stored Unicode string to UTF-8. Caller is responsible for freeing memory */
char *utf8_from_wchar(const wchar_t *wstr)
{
	char *utf8str;
	int len;

	if (!wstr) return NULL;
	if ((len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL)) == 0) return NULL;
	if ((utf8str = (char *)malloc(++len)) == NULL) return NULL;
	if (WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8str, len, NULL, NULL) == 0) {
		free(utf8str);
		utf8str = NULL;
	}

	return utf8str;
}

/* convert UTF-8 back to WCHAR. Caller is responsible for freeing memory */
wchar_t *wchar_from_utf8(const char *str)
{
	wchar_t *widestr;
	int len;

	if (!str) return NULL;
	len=(int)strlen(str)+1;
	if ((widestr = (wchar_t *)malloc(len*sizeof(wchar_t))) != NULL) {
		if (MultiByteToWideChar(CP_UTF8, 0, str, len, widestr, len) == 0) {
			if (MultiByteToWideChar(CP_ACP, 0, str, len, widestr, len) == 0) { /* try conversion from Ansi in case the initial UTF-8 conversion had failed */
				free(widestr);
				widestr = NULL;
			}
		}
	}

	return widestr;
}

/* retrieve WCHAR commandline, expand wildcards and convert everything to UTF-8 */
int get_utf8_argv(int *argc, char ***argv)
{
	typedef int (__cdecl *__wgetmainargs_)(int*, wchar_t***, wchar_t***, int, int*);
	__wgetmainargs_ __wgetmainargs;
	HMODULE handle;
	int wargc;
	wchar_t **wargv;
	wchar_t **wenv;
	char **utf8argv;
	int ret, i;

	if ((handle = LoadLibrary("msvcrt.dll")) == NULL) return 1;
	if ((__wgetmainargs = (__wgetmainargs_)GetProcAddress(handle, "__wgetmainargs")) == NULL) return 1;
	i = 0;
	if (__wgetmainargs(&wargc, &wargv, &wenv, 1, &i) != 0) return 1;
	if ((utf8argv = (char **)malloc(wargc*sizeof(char*))) == NULL) return 1;
	ret = 0;

	for (i=0; i<wargc; i++) {
		if ((utf8argv[i] = utf8_from_wchar(wargv[i])) == NULL) {
			ret = 1;
			break;
		}
		if (ret != 0) break;
	}

	if (ret == 0) {
		*argc = wargc;
		*argv = utf8argv;
	} else {
		free(utf8argv);
	}

	return ret;
}

/* print functions */

int printf_utf8(const char *format, ...)
{
	char *utmp = NULL;
	wchar_t *wout = NULL;
	int ret = -1;

	while (1) {
		va_list argptr;
		if (!(utmp = (char *)malloc(32768*sizeof(char)))) break;
		va_start(argptr, format);
		ret = vsprintf(utmp, format, argptr);
		va_end(argptr);
		if (ret < 0) break;
		if (!(wout = wchar_from_utf8(utmp))) {
			ret = -1;
			break;
		}
		ret = wprintf(L"%s", wout);
		break;
	}
	if (utmp) free(utmp);
	if (wout) free(wout);

	return ret;
}

int fprintf_utf8(FILE *stream, const char *format, ...)
{
	char *utmp = NULL;
	wchar_t *wout = NULL;
	int ret = -1;

	while (1) {
		va_list argptr;
		if (!(utmp = (char *)malloc(32768*sizeof(char)))) break;
		va_start(argptr, format);
		ret = vsprintf(utmp, format, argptr);
		va_end(argptr);
		if (ret < 0) break;
		if (!(wout = wchar_from_utf8(utmp))) {
			ret = -1;
			break;
		}
		ret = fwprintf(stream, L"%s", wout);
		break;
	}
	if (utmp) free(utmp);
	if (wout) free(wout);

	return ret;
}

int vfprintf_utf8(FILE *stream, const char *format, va_list argptr)
{
	char *utmp = NULL;
	wchar_t *wout = NULL;
	int ret = -1;

	while (1) {
		if (!(utmp = (char *)malloc(32768*sizeof(char)))) break;
		if ((ret = vsprintf(utmp, format, argptr)) < 0) break;
		if (!(wout = wchar_from_utf8(utmp))) {
			ret = -1;
			break;
		}
		ret = fwprintf(stream, L"%s", wout);
		break;
	}
	if (utmp) free(utmp);
	if (wout) free(wout);

	return ret;
}

/* file functions */

FILE *fopen_utf8(const char *filename, const char *mode)
{
	wchar_t *wname = NULL;
	wchar_t *wmode = NULL;
	FILE *f = NULL;

	while (1) {
		if (!(wname = wchar_from_utf8(filename))) break;
		if (!(wmode = wchar_from_utf8(mode))) break;
		f = _wfopen(wname, wmode);
		break;
	}
	if (wname) free(wname);
	if (wmode) free(wmode);

	return f;
}

int _stat64_utf8(const char *path, struct _stat64 *buffer)
{
	wchar_t *wpath;
	int ret;

	if (!(wpath = wchar_from_utf8(path))) return -1;
	ret = _wstat64(wpath, buffer);
	free(wpath);

	return ret;
}

int chmod_utf8(const char *filename, int pmode)
{
	wchar_t *wname;
	int ret;

	if (!(wname = wchar_from_utf8(filename))) return -1;
	ret = _wchmod(wname, pmode);
	free(wname);

	return ret;
}

int utime_utf8(const char *filename, struct utimbuf *times)
{
	wchar_t *wname;
	struct _utimbuf ut;
	int ret;

	if (!(wname = wchar_from_utf8(filename))) return -1;
	ret = _wutime(wname, &ut);
	free(wname);

	if (ret != -1) {
		if (sizeof(*times) == sizeof(ut)) {
			memcpy(times, &ut, sizeof(ut));
		} else {
			times->actime = ut.actime;
			times->modtime = ut.modtime;
		}
	}

	return ret;
}

int unlink_utf8(const char *filename)
{
	wchar_t *wname;
	int ret;

	if (!(wname = wchar_from_utf8(filename))) return -1;
	ret = _wunlink(wname);
	free(wname);

	return ret;
}

int rename_utf8(const char *oldname, const char *newname)
{
	wchar_t *wold = NULL;
	wchar_t *wnew = NULL;
	int ret = -1;

	while (1) {
		if (!(wold = wchar_from_utf8(oldname))) break;
		if (!(wnew = wchar_from_utf8(newname))) break;
		ret = _wrename(wold, wnew);
		break;
	}
	if (wold) free(wold);
	if (wnew) free(wnew);

	return ret;
}

#endif
