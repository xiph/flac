#ifdef _WIN32

#ifndef flac__win_utf8_io_h
#define flac__win_utf8_io_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <sys/stat.h>
#include <stdarg.h>


int get_utf8_argv(int *argc, char ***argv);

int printf_utf8(const char *format, ...);
int fprintf_utf8(FILE *stream, const char *format, ...);
int vfprintf_utf8(FILE *stream, const char *format, va_list argptr);

FILE *fopen_utf8(const char *filename, const char *mode);
int stat_utf8(const char *path, struct stat *buffer);
int _stat64_utf8(const char *path, struct __stat64 *buffer);
int chmod_utf8(const char *filename, int pmode);
int utime_utf8(const char *filename, struct utimbuf *times);
int unlink_utf8(const char *filename);
int rename_utf8(const char *oldname, const char *newname);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
#endif
