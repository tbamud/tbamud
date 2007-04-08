/**************************************************************************
*  File: zmalloc.c                                         Part of tbaMUD *
*  Usage: A simple memory allocation monitor.                             *
*                                                                         *
*  Version 2. Copyright 1996, 1998, 1999, 2000 Eric Murray.               *
**************************************************************************/

/* local functions */
void zfree_special (int *, char *, int);

/* protect our calloc() and free() calls from recursive redefinition: */
#define ZMALLOC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* #define NO_MEMORY_PADDING */

#ifndef NO_MEMORY_PADDING
static unsigned char beginPad[4] = {
  0xde, 0xad, 0xde, 0xad };

static unsigned char endPad[4] = {
  0xde, 0xad, 0xde, 0xad };
#endif

extern int errno;

FILE * zfd;

typedef struct meminfo {
  struct meminfo *next;
  int size; /* number of bytes malloced */
  long addr;  /* address of memory returned */
  int frees; /* number of times that 'free' was called on this memory */
  char *file; /* file where malloc was called */
  int line; /* line in the code where malloc was called */
} meminfo;

static meminfo memlist;
/* -1 not checked yet, 0 = no zmalloc, 1 = zmalloc */
int zmallocstatus = -1;

/* functions: */
int getzmallocstatus(void);
int * zmalloc(int len,char * file,int line);
void zfree_special(int * what, char * file,int line);
void zfree(int * what, char * file,int line);
char *zstrdup(const char *src, char *file, int line);
void zmalloc_check( void );
void pad_check(meminfo *m);
void zmalloc_free_list(meminfo *m);

/* a chopped down version that works */
int getzmallocstatus()
{
  if (zmallocstatus == -1) {
    zfd = fopen("test.log","w+");
    zmallocstatus = 1;
  }
  return(zmallocstatus);
}

int * zmalloc(int len,char * file,int line)
{
  unsigned char * ret;
  meminfo *m;

  if (!zmallocstatus) {
    /* zmalloc turned off */
    ret = (unsigned char *)calloc(1,len);
    return((int *)ret);
  }

#ifndef NO_MEMORY_PADDING
  ret = (unsigned char *)calloc(1,len + sizeof(beginPad) + sizeof(endPad));
#else
  ret = (unsigned char *)calloc(1,len);
#endif

  if (!ret)
    fprintf(zfd,"zmalloc: malloc FAILED");

#ifndef NO_MEMORY_PADDING
  /* insert begin and end padding to detect buffer under/overruns: */
  memcpy(ret,beginPad,sizeof(beginPad));
  ret +=  sizeof(beginPad);  /* make ret skip begin pad */
  memcpy(ret + len,endPad,sizeof(endPad));
#endif

  if (getzmallocstatus() > 1)
    fprintf(zfd,"zmalloc: 0x%4.4x  %d bytes %s:%d\n",(int)ret,len,file,line);

  m = (meminfo *)calloc(1,sizeof(meminfo));
  if (!m) {
    fprintf(zfd,"zmalloc: FAILED mem alloc for zmalloc struct... bailing!\n");
    return((int*)0);
  }
  m->next = &memlist;
  m->addr = (long)ret;
  m->size = (int)len;
  m->file = (char *)strdup(file);
  m->frees = 0;
  if (!m->file) {
    fprintf(zfd,"zmalloc: FAILED mem alloc for zmalloc struct... bailing!\n");
    return((int*)0);
  }
  m->line = line;
  return((int *)ret);
}

void zfree_special(int * what, char * file,int line)
{
  meminfo *m, *old;
  unsigned char *addr;
  int gotit = 0;

  if (what == 0) {
    fprintf(zfd,"zmalloc: Null pointer free'd: %s:%d\n", file, line);
    return;
  }

  if (!getzmallocstatus()) {
    free(what);
  } else {
    /* look up allocated mem in list: */
    old = m = &memlist;
    if (m == (meminfo *)0) {
      /* no memlist */
      free(what);
    } else {
      for(; m  ; m = m->next) {
        if (m->addr == (long)what) {
          /* got it.  Print it if verbose: */
          addr = (unsigned char *)m->addr;

          fprintf(zfd,"zmalloc: Freed 0x%4.4x %d bytes mallocd@%s:%d\n",
                        (int)addr,m->size,m->file,m->line);
          fprintf(zfd," free'd from %s:%d\n",
                        file, line);


          /* check the padding: */
          pad_check(m);

          /* note that we freed the memory */
          m->frees++;

          /* check to see if it was freed > once */
          if (m->frees > 1) {
            fprintf(zfd," ERR: multiple frees! 0x%4.4x %d bytes: %s:%d\n",
                        (int)addr, m->size, m->file, m->line);
            fprintf(zfd," free'd from %s:%d\n",
                        file, line);
          }
          gotit++;
          if (!m)
            break;
        }
      }
      if (!gotit && m) {
         fprintf(zfd,"zmalloc: ERR: Freed unallocated memory");
         fprintf(zfd," @0x%4.4x %d bytes %s:%d\n",
          (int)what,m->size,m->file,m->line);
      }
      else if (gotit > 1) {
        /* this shouldn't happen, eh? */
        fprintf(zfd," ERR: Multiply-allocd memory!\n");
      }
    }
  }
}
void zfree(int * what, char * file,int line)
{
  meminfo *m, *old;
  unsigned char *addr;
  int gotit = 0;

  if (what == 0) {
    fprintf(zfd,"zmalloc: Null pointer free'd: %s:%d\n", file, line);
    return;
  }

  if (!getzmallocstatus()) {
    free(what);
  } else {
    /* look up allocated mem in list: */
    old = m = &memlist;
    if (m == (meminfo *)0) {
      /* no memlist */
      free(what);
    } else {
      for(; m  ; m = m->next) {
        if (m->addr == (long)what) {
          /* got it.  Print it if verbose: */
          addr = (unsigned char *)m->addr;
          if (getzmallocstatus() > 1)
            fprintf(zfd,"zmalloc: Freed 0x%4.4x %d bytes mallocd@%s:%d\n",
                        (int)addr,m->size,m->file,m->line);

          /* check the padding: */
          pad_check(m);

          /* note that we freed the memory */
          m->frees++;

          /* check to see if it was freed > once */
          if (m->frees > 1) {
            fprintf(zfd," ERR: multiple frees! 0x%4.4x %d bytes: %s:%d\n",
                        (int)addr, m->size, m->file, m->line);
            fprintf(zfd," free'd from %s:%d\n",
                        file, line);
          }
          gotit++;
          if (!m)
            break;
        }
      }
      if (!gotit && m) {
         fprintf(zfd,"zmalloc: ERR: Freed unallocated memory");
         fprintf(zfd," @0x%4.4x %d bytes %s:%d\n",
          (int)what,m->size,m->file,m->line);
      }
      else if (gotit > 1) {
        /* this shouldn't happen, eh? */
        fprintf(zfd," ERR: Multiply-allocd memory!\n");
      }
    }
  }
}

/* zstrdup */
char *zstrdup(const char *src, char *file, int line)
{
    char *result;
#ifndef NO_MEMORY_STRDUP
  if (!getzmallocstatus()) {
    result = (char*)malloc(strlen(src) + 1);
    if (result == (char*)0)
	return (char*)0;
    strcpy(result, src);  /* strcpy ok, size checked above */
    return result;
  } else {
    result = (char*)zmalloc(strlen(src) + 1, file, line);
    if (result == (char*)0)
	return (char*)0;
    strcpy(result, src);
    return result;
  }
#else
    result = (char*)malloc(strlen(src) + 1);
    if (result == (char*)0)
	return (char*)0;
    strcpy(result, src);  /* strcpy ok, size checked above */
    return result;
#endif
}

void zmalloc_check()
{
  meminfo *m;
  char *admonishemnt;
  int total_leak = 0;
  int num_leaks = 0;

  if (getzmallocstatus() > 0) {
    /* look up allocated mem in list: */
    for(m = &memlist; m ; m = m->next) {
      if (m->addr != 0 && m->frees <= 0) {
        fprintf(zfd,"zmalloc: UNfreed memory 0x%4.4x %d bytes mallocd at %s:%d\n",
          (int)m->addr,m->size,m->file,m->line);

        /* check padding on un-freed memory too: */
        pad_check(m);

        total_leak += m->size;
        num_leaks ++;
      }
    }
    if (total_leak) {
      if (total_leak > 10000)
        admonishemnt = "you must work for Microsoft.";
      else if (total_leak > 5000)
        admonishemnt = "you should be ashamed!";
      else if (total_leak > 2000)
        admonishemnt = "you call yourself a programmer?";
      else if (total_leak > 1000)
        admonishemnt = "the X consortium has a job for you...";
      else
        admonishemnt = "close, but not there yet.";
      fprintf(zfd,"zmalloc: %d leaks totalling %d bytes... %s\n",num_leaks,total_leak,
        admonishemnt);
    }
    else {
      fprintf(zfd,"zmalloc: Congratulations: leak-free code!\n");
    }
  }
  /* free up our own internal list */
  zmalloc_free_list(&memlist);
}

void pad_check(meminfo *m)
{
#ifndef NO_MEMORY_PADDING
  unsigned char *addr = (unsigned char *)m->addr;

  /* check the padding: */
  if (memcmp((int *)(addr - sizeof(beginPad)),
      beginPad, sizeof(beginPad)) != 0)
    fprintf(zfd," ERR: beginPad was modified!\n");
  if (memcmp((int *)(addr + m->size),endPad,
      sizeof(endPad)) != 0)
    fprintf(zfd," ERR: endPad was modified!\n");
#endif
}

void zmalloc_free_list(meminfo *m)
{
  meminfo *old;
  for( m = m->next ; m  ; ) {
    old = m;
    m = m->next;
    free(old->file);
    free(old);
  }
}

#ifdef ZTEST
#undef ZMALLOC_H

#include "zmalloc.h"

main()
{
  unsigned char * tmp;


  printf("Testing Zmalloc.\n");
  printf("Malloc test..");
  printf("You should see no error here.\n");
  tmp = (unsigned char*)malloc(200);
  free(tmp);

  printf("Free free mem test...\n");
  printf("You should see an ERR: multiple frees here\n");
  tmp = (unsigned char*)malloc(200);
  free(tmp);
  free(tmp);

/*
  fprintf(zfd,"\nFree unallocated mem test \n");
  tmp += 4;
  free(tmp);
*/

  printf("Unfreed mem test...\n");
  printf("You should see an \"UNfreed mem at line %d\" (at end) because of this\n",__LINE__+1);
  tmp = (unsigned char*)malloc(200);

  printf("Buffer overrun test 1...\n");
  printf("You should see an ERR:endPad here\n");
  tmp = (unsigned char*)malloc(200);
  tmp[200] = 0xfa;
  free(tmp);

  printf("Buffer overrun test 2...\n");
  printf("You should see an ERR:endPad here\n");
  tmp = (unsigned char*)malloc(200);
  tmp[215] = 0xbb;
  free(tmp);

  printf("Buffer underrun test 1...\n");
  printf("You should see an ERR:beginPad here\n");
  tmp = (unsigned char*)malloc(200);
  tmp[-10] = 0x0f;
  free(tmp);

  printf("Buffer underrun test 2...\n");
  printf("You should see an ERR:beginPad here\n");
  tmp = (unsigned char*)malloc(200);
  tmp[-1] = 0x00;
  free(tmp);


  zmalloc_check();
  exit(0);
}
#endif /* ZTEST */
