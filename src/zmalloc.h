/**************************************************************************
*  File: zmalloc.h                                         Part of tbaMUD *
*  Usage: A simple memory allocation monitor, header.                     *
*                                                                         *
*  Version 1.1 Copyright 1996, 1998, 1999, 2000 Eric Murray.              *
**************************************************************************/

#ifndef ZMALLOC_H
#define ZMALLOC_H

int *zmalloc(int, char *, int);
void zfree(int *, char *, int);
int zmalloc_check();
char *zstrdup(const char*, char*, int);

#define malloc(x)  zmalloc((x),__FILE__,__LINE__)
#define calloc(n,x) zmalloc((n*x),__FILE__,__LINE__)
#define free(x) zfree((int *)(x),__FILE__,__LINE__)
#undef strdup
#define strdup(x) zstrdup((x), __FILE__, __LINE__)

#endif /* ZMALLOC_H */
