/**************************************************************************
*  File: zmalloc.c                                         Part of tbaMUD *
*  Usage: A simple memory allocation monitor.                             *
*                                                                         *
*  Version 2. Copyright 1996, 1998, 1999, 2000 Eric Murray.               *
**************************************************************************/

/*
** Zmalloc, a simple memory-allocation monitor.
**
** Copyright 1996,1998,1999,2000 Eric Murray, ericm@lne.com
** You may make free use of this code but please give me credit.
** Documentation: http://www.lne.com/ericm/zmalloc
*
*  Usage: to enable call zmalloc_init() at the very start of your
*  program to open the logfile and call zmalloc_check() at the end
*  to display memory leaks and free all allocated mem.
*  See the main() test function at the bottom for an example.
*/

/* protect our calloc() and free() calls from recursive redefinition: */
#define ZMALLOC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//#define NO_MEMORY_PADDING

#ifndef NO_MEMORY_PADDING
static unsigned char beginPad[4] = {
  0xde, 0xad, 0xde, 0xad };

static unsigned char endPad[4] = {
  0xde, 0xad, 0xde, 0xad };
#endif

FILE *zfd = NULL;

typedef struct meminfo {
  struct meminfo *next;
  int size; /* number of bytes malloced */
  unsigned char *addr;  /* address of memory returned */
  int frees; /* number of times that 'free' was called on this memory */
  char *file; /* file where malloc was called */
  int line; /* line in the code where malloc was called */
} meminfo;

static meminfo *memlist = NULL;

/*
 * 0 = only end summary
 * 1 = show errors
 * 2 = errors with dumps
 * 3 = all of the above plus all mallocs/frees
 */
int zmalloclogging = 2;

/* functions: */
unsigned char *zmalloc(int len, char *file, int line);
unsigned char *zrealloc(unsigned char *what, int len, char *file, int line);
void zdump(meminfo *m);
void zfree(unsigned char *what, char *file, int line);
char *zstrdup(const char *src, char *file, int line);
void zmalloc_init(void);
void zmalloc_check(void);
void pad_check(meminfo *m);
void zmalloc_free_list(meminfo *m);


void zmalloc_init(void) {
  zfd = fopen("zmalloc.log","w+");
}


void zdump(meminfo *m)
{
#define MAX_ZDUMP_SIZE 32
  const unsigned char *hextab = (unsigned char *)"0123456789ABCDEF";
  unsigned char hexline[37], ascline[17], *hexp, *ascp, *inp;
  int len, c = 1;

  if (m->addr == NULL || m->size <= 0)
    return;

  hexp = hexline;
  ascp = ascline;
  inp = m->addr;
  len = (m->size > MAX_ZDUMP_SIZE ? MAX_ZDUMP_SIZE : m->size);

  for ( ; len > 0; len--, inp++, c++) {
    *(hexp++) = hextab[(int) (*inp & 0xF0) >> 4];	/* high 4 bit */
    *(hexp++) = hextab[(int) (*inp & 0x0F)];		/* low 4 bit */
    if (c % 4 == 0) *(hexp++) = ' ';
    *(ascp++) = isprint(*inp) ? *inp : '.';
    if (c % 16 == 0 || len <= 1) {
      *hexp = '\0';
      *ascp = '\0';
      fprintf(zfd, "     %-40.40s%s\n", hexline, ascline);
      hexp = hexline;
      ascp = ascline;
    }
  }
  fprintf(zfd, "\n");
}


unsigned char *zmalloc(int len, char *file, int line)
{
  unsigned char *ret;
  meminfo *m;

#ifndef NO_MEMORY_PADDING
  ret = (unsigned char *) calloc(1, len + sizeof(beginPad) + sizeof(endPad));
#else
  ret = (unsigned char *) calloc(1, len);
#endif

  if (!ret) {
    fprintf(zfd,"zmalloc: malloc FAILED");
    return NULL;
  }
#ifndef NO_MEMORY_PADDING
  /* insert begin and end padding to detect buffer under/overruns: */
  memcpy(ret, beginPad, sizeof(beginPad));
  ret +=  sizeof(beginPad);  /* make ret skip begin pad */
  memcpy(ret + len, endPad, sizeof(endPad));
#endif

  if (zmalloclogging > 2)
    fprintf(zfd,"zmalloc: 0x%4.4x  %d bytes %s:%d\n",(int)ret,len,file,line);

  m = (meminfo *) calloc(1, sizeof(meminfo));
  if (!m) {
    fprintf(zfd,"zmalloc: FAILED mem alloc for zmalloc struct... bailing!\n");
    return NULL;
  }
  m->addr = ret;
  m->size = len;
  m->frees = 0;
  m->file = strdup(file);
  if (!m->file) {
    fprintf(zfd,"zmalloc: FAILED mem alloc for zmalloc struct... bailing!\n");
    free(m);
    return NULL;
  }
  m->line = line;
  m->next = memlist;
  memlist = m;
  return (ret);
}

unsigned char *zrealloc(unsigned char *what, int len, char *file, int line)
{
  unsigned char *ret;
  meminfo *m;

  if (what) {
    for (m = memlist; m; m = m->next) {
      if (m->addr == what) {
#ifndef NO_MEMORY_PADDING
	ret = (unsigned char *) realloc(what - sizeof(beginPad), len + sizeof(beginPad) + sizeof(endPad));
#else
	ret = (unsigned char *) realloc(what, len);
#endif
	if (!ret) {
	  fprintf(zfd,"zrealloc: FAILED for 0x%4.4x %d bytes mallocd at %s:%d,\n"
		      "          %d bytes reallocd at %s:%d.\n",
		    (int)m->addr, m->size, m->file, m->line, len, file, line);
	  if (zmalloclogging > 1) zdump(m);
	  return NULL;
	}
#ifndef NO_MEMORY_PADDING
	/* insert begin and end padding to detect buffer under/overruns: */
	memcpy(ret, beginPad, sizeof(beginPad));
	ret +=  sizeof(beginPad);  /* make ret skip begin pad */
	memcpy(ret + len, endPad, sizeof(endPad));
#endif
	if (zmalloclogging > 2)
	  fprintf(zfd,"zrealloc: 0x%4.4x %d bytes mallocd at %s:%d, %d bytes reallocd at %s:%d.\n",
		    (int)m->addr, m->size, m->file, m->line, len, file, line);

	m->addr = ret;
	m->size = len;
	if (m->file) free(m->file);
	m->file = strdup(file);
	m->line = line;
	/* could continue the loop to check for multiply-allocd memory */
	/* but that's highly improbable so lets just return instead. */
	return (ret);
      }
    }
  }

  /* NULL or invalid pointer given */
  fprintf(zfd,"zrealloc: invalid pointer 0x%4.4x, %d bytes to realloc at %s:%d.\n",
	    (int)what, len, file, line);

  return (zmalloc(len, file, line));
}

/* doesn't actually free memory */
void zfree(unsigned char *what, char *file, int line)
{
  meminfo *m;
  int gotit = 0;

  if (!what) {
    fprintf(zfd,"zfree: ERR: Null pointer free'd: %s:%d.\n", file, line);
    return;
  }

  /* look up allocated mem in list: */
  for (m = memlist; m; m = m->next) {
    if (m->addr == what) {
      /* got it.  Print it if verbose: */
      if (zmalloclogging > 2) {
	fprintf(zfd,"zfree: Freed 0x%4.4x %d bytes mallocd at %s:%d, freed at %s:%d\n",
		    (int)m->addr, m->size, m->file, m->line, file, line);
      }
      /* check the padding: */
      pad_check(m);

      /* note that we freed the memory */
      m->frees++;  

      /* check to see if it was freed > once */
      if (m->frees > 1) {
        fprintf(zfd,"zfree: ERR: multiple frees! 0x%4.4x %d bytes\n"
		    "       mallocd at %s:%d, freed at %s:%d.\n",
                    (int)m->addr, m->size, m->file, m->line, file, line);
	if (zmalloclogging > 1) zdump(m);
      }
      gotit++;
    }
  } /* for.. */

  if (!gotit) {
    fprintf(zfd,"zfree: ERR: attempt to free unallocated memory 0x%4.4x at %s:%d.\n",
		(int)what, file, line);
  }
  if (gotit > 1) {
    /* this shouldn't happen, eh? */
    fprintf(zfd,"zfree: ERR: Multiply-allocd memory 0x%4.4x.\n", (int)what);
  }
}


char *zstrdup(const char *src, char *file, int line)
{
  char *result;
#ifndef NO_MEMORY_STRDUP    
  result = (char*)zmalloc(strlen(src) + 1, file, line);
  if (!result)
    return NULL;
  strcpy(result, src);
  return result;
#else
  result = (char*)malloc(strlen(src) + 1);
  if (!result)
    return NULL;
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

  fprintf(zfd, "\n------------ Checking leaks ------------\n\n");
  /* look up allocated mem in list: */
  for(m = memlist; m; m = m->next) {
    if (m->addr != 0 && m->frees <= 0) {
      fprintf(zfd,"zmalloc: UNfreed memory 0x%4.4x %d bytes mallocd at %s:%d\n",
		(int)m->addr, m->size, m->file, m->line);
      if (zmalloclogging > 1) zdump(m);

      /* check padding on un-freed memory too: */
      pad_check(m);

      total_leak += m->size;
      num_leaks++;
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
    fprintf(zfd,"zmalloc: %d leaks totalling %d bytes... %s\n",
		num_leaks, total_leak, admonishemnt);
  }
  else {
    fprintf(zfd,"zmalloc: Congratulations: leak-free code!\n");
  }

  /* free up our own internal list */
  zmalloc_free_list(memlist);

  if (zfd) {
    fflush(zfd);
    fclose(zfd);
  }
}


void pad_check(meminfo *m)
{
#ifndef NO_MEMORY_PADDING
  if (memcmp(m->addr - sizeof(beginPad), beginPad, sizeof(beginPad)) != 0) {
    fprintf(zfd,"pad_check: ERR: beginPad was modified! (mallocd@ %s:%d)\n", m->file, m->line);
    if (zmalloclogging > 1) zdump(m);
  }
  if (memcmp(m->addr + m->size, endPad, sizeof(endPad)) != 0) {
    fprintf(zfd,"pad_check: ERR: endPad was modified! (mallocd@ %s:%d)\n", m->file, m->line);
    if (zmalloclogging > 1) zdump(m);
  }
#endif
}


void zmalloc_free_list(meminfo *m)
{
  meminfo *next_m;
  for (; m; m = next_m) {
    next_m = m->next;
#ifndef NO_MEMORY_PADDING
    if (m->addr) free(m->addr - sizeof(beginPad));
#else
    if (m->addr) free(m->addr);
#endif
    if (m->file) free(m->file);
    free(m);
  }
}
      

#ifdef ZTEST
#undef ZMALLOC_H

#include "zmalloc.h"

int main()
{
  unsigned char * shit;

  zmalloc_init();

/* You should see no error here. */
  shit = (unsigned char*)malloc(200);
  free(shit);

/* Multiple frees test */
  shit = (unsigned char*)malloc(200);
  strcpy(shit, "This should show up in the dump but truncated to MAX_ZDUMP_SIZE chars");
  free(shit);
  free(shit);

/* Free unallocated mem test */
  shit += 4;
  free(shit);

/* Unfreed mem test... You should see "UNfreed mem at line 370" (at end) because of this */
  shit = (unsigned char*)malloc(200);
  strcpy(shit, "This is unfreed memory!");

/* Buffer overrun test... You should see an ERR:endPad here */
  shit = (unsigned char*)malloc(200);
  shit[202] = 0xbb;
  free(shit);

/* Buffer underrun test... You should see an ERR:beginPad here */
  shit = (unsigned char*)malloc(200);
  shit[-3] = 0x0f;
  free(shit);

/* Free NULL pointer test... */
  shit = NULL;
  free(shit);

  printf("Test completed. See zmalloc.log for the messages.\n");

  zmalloc_check();
  exit(0);
}
#endif /* ZTEST */
