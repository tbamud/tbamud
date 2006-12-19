
/*
** Zmalloc, a simple memory-allocation monitor.
**
** Copyright 1996 Eric Murray, ericm@lne.com
**
** You may make free use of this code but please give me credit.
**
**
** Documentation: http://www.lne.com/ericm/zmalloc
**
** $Id: zmalloc.h,v 1.1 1998/05/25 16:31:35 ericm Exp $
** $Log: zmalloc.h,v $
** Revision 1.1  1998/05/25 16:31:35  ericm
** Initial revision
**
**
*/

#ifndef ZMALLOC_H
#define ZMALLOC_H

int *zmalloc(int, char *, int);
void zfree(int *, char *, int);
void zfree_special (int *, char *, int);
int zmalloc_check();
char *zstrdup(const char*, char*, int);

#define malloc(x)  zmalloc((x),__FILE__,__LINE__)
#define calloc(n,x) zmalloc((n*x),__FILE__,__LINE__)
#define free(x) zfree((int *)(x),__FILE__,__LINE__)
#undef strdup
#define strdup(x) zstrdup((x), __FILE__, __LINE__)

#endif /* ZMALLOC_H */

