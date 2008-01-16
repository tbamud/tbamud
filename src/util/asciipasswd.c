/* ************************************************************************
*  file:  asciipasswd.c (derived from mudpasswd.c)         Part of tbaMUD *
*  Usage: generating hashed passwords for an ascii playerfile.            *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*  All Rights Reserved                                                    *
************************************************************************* */


#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"


char *CAP(char *txt) {
  *txt = UPPER(*txt);
  return (txt);
}

int main(int argc, char **argv) {
  if (argc != 3)
    fprintf(stderr, "Usage: %s name password\n", argv[0]);
  else
    printf("Name: %s\nPass: %s\n", CAP(argv[1]), CRYPT(argv[2], CAP(argv[1])));
  return (0);
}

