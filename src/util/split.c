/* ************************************************************************
*  file:  split.c                                     Part of CircleMud   *
*  Usage: split one large file into multiple smaller ones, with index     *
*  Written by Jeremy Elson                                                *
*  All Rights Reserved                                                    *
*  Copyright (C) 1993 The Trustees of The Johns Hopkins University        *
************************************************************************* */

/*
 * This utility is meant to split a large file into multiple smaller ones,
 * mainly to help break huge world files (ala Diku) into zone-sized files
 * that are easier to manage.
 *
 * At each point in the original file where you want a break, insert a line
 * containng "=filename" at the break point.
 */

#define INDEX_NAME "index"
#define BSZ 256
#define MAGIC_CHAR '='

#include "conf.h"
#include "sysdep.h"

int main(void)
{
  char line[BSZ + 1];
  FILE *index = 0, *outfile = 0;

  if (!(index = fopen(INDEX_NAME, "w"))) {
    perror("error opening index for write");
    exit(1);
  }
  while (fgets(line, BSZ, stdin)) {
    if (*line == MAGIC_CHAR) {
      *(strchr(line, '\n')) = '\0';
      if (outfile) {
/*	fputs("$\n", outfile);*/
	fclose(outfile);
      }
      if (!(outfile = fopen((line + 1), "a"))) {
	perror("Error opening output file");
	exit(0);
      }
      fputs(line + 1, index);
      fputs("\n", index);
    } else if (outfile)
      fputs(line, outfile);
  }

  fputs("$\r\n", index);
  fclose(index);
  if (outfile)
    fclose(outfile);

  return (0);
}
