/**
* @file zmalloc.h
* A simple memory allocation monitor, header.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* This code was not part of the original CircleMUD distribution. It has been
* included in tbaMUD and is attributed to:
* Version 1.1 Copyright 1996, 1998, 1999, 2000 Eric Murray ericm@lne.com               
*/

#ifndef _ZMALLOC_H_
#define _ZMALLOC_H_

unsigned char *zmalloc(int, char *, int);
unsigned char *zrealloc(unsigned char *, int, char *, int);
void zfree(unsigned char *, char *, int);
void zmalloc_init(void);
void zmalloc_check(void);
char *zstrdup(const char *, char *, int);

#define malloc(x)	zmalloc((x),__FILE__,__LINE__)
#define calloc(n,x)	zmalloc((n*x),__FILE__,__LINE__)
#define realloc(r,x)	zrealloc((unsigned char *)(r),(x),__FILE__,__LINE__)
#define free(x)		zfree((unsigned char *)(x),__FILE__,__LINE__)
#undef  strdup
#define strdup(x)	zstrdup((x), __FILE__, __LINE__)

#endif /* _ZMALLOC_H_ */
