#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef UINT
typedef unsigned int UINT;
#endif

#ifdef TAGZ_UNICODE
typedef unsigned short T_CHAR;
#else
#define T_CHAR char
#endif

typedef const T_CHAR* (*TAGFUNC)(const T_CHAR *tag,void *p);    /* return 0 if not found */
typedef void (*TAGFREEFUNC)(const T_CHAR *tag,void *p);


UINT tagz_format(const T_CHAR * spec,TAGFUNC f,TAGFREEFUNC ff,void *fp,T_CHAR * out,UINT max);
T_CHAR * tagz_format_r(const T_CHAR * spec,TAGFUNC f,TAGFREEFUNC ff,void * fp);

extern const char tagz_manual[];

#ifdef __cplusplus
}
#endif
