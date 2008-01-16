/* ************************************************************************
*  file: sign.c                                            Part of tbaMUD *
*  Usage: A program to present text on a TCP port.                        *
*         sign <port> <filename | port>                                   *
*  Written by Jeremy Elson                                                *
************************************************************************* */

#define MAX_FILESIZE	8192
#define LINEBUF_SIZE	128

#include "conf.h"
#include "sysdep.h"


/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
int init_socket(int port)
{
  int s, opt;
  struct sockaddr_in sa;

  /*
   * Should the first argument to socket() be AF_INET or PF_INET?  I don't
   * know, take your pick.  PF_INET seems to be more widely adopted, and
   * Comer (_Internetworking with TCP/IP_) even makes a point to say that
   * people erroneously use AF_INET with socket() when they should be using
   * PF_INET.  However, the man pages of some systems indicate that AF_INET
   * is correct; some such as ConvexOS even say that you can use either one.
   * All implementations I've seen define AF_INET and PF_INET to be the same
   * number anyway, so ths point is (hopefully) moot.
   */

  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Create socket");
    exit(1);
  }
#if defined(SO_REUSEADDR)
  opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
    perror("setsockopt REUSEADDR");
    exit(1);
  }
#endif

#if defined(SO_LINGER)
  {
    struct linger ld;

    ld.l_onoff = 0;
    ld.l_linger = 0;
    if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld)) < 0) {
      perror("setsockopt LINGER");
      exit(1);
    }
  }
#endif

  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
    perror("bind");
    close(s);
    exit(1);
  }
  listen(s, 5);
  return (s);
}


char *get_text(char *fname)
{
  static char t[MAX_FILESIZE];
  char tmp[LINEBUF_SIZE + 2];
  FILE *fl = NULL;

  *t = '\0';

  if (!strcmp(fname, "-")) {
    fl = stdin;
    if (isatty(STDIN_FILENO))
      fprintf(stderr, "Enter sign text; terminate with Ctrl-D.\n");
  } else {
    if (!(fl = fopen(fname, "r"))) {
      perror(fname);
      exit(1);
    }
  }

  while (fgets(tmp, LINEBUF_SIZE, fl)) {
    if (strlen(tmp) + strlen(t) < MAX_FILESIZE - 1)
      strcat(t, strcat(tmp, "\r"));
    else {
      fprintf(stderr, "String too long.  Truncated.\n");
      break;
    }
  }

  return (t);
}


/* clean up our zombie kids to avoid defunct processes */
RETSIGTYPE reap(int sig)
{
  while (waitpid(-1, NULL, WNOHANG) > 0);

  signal(SIGCHLD, reap);
}


int main(int argc, char *argv[])
{
  char *txt;
  int desc, remaining, bytes_written, len, s, port, child;

  if (argc != 3 || (port = atoi(argv[1])) < 1024) {
    fprintf(stderr, "usage: %s <portnum> <\"-\" | filename>\n", argv[0]);
    exit(1);
  }
  s = init_socket(port);
  len = strlen(txt = get_text(argv[2]));

  if ((child = fork()) > 0) {
    fprintf(stderr, "Sign started on port %d (pid %d).\n", port, child);
    exit(0);
  }
  signal(SIGCHLD, reap);

  for (;;) {
    if ((desc = accept(s, (struct sockaddr *) NULL, 0)) < 0)
      continue;

    if (fork() == 0) {
      remaining = len;
      do {
	if ((bytes_written = write(desc, txt, remaining)) < 0)
	  exit(0);
	else {
	  txt += bytes_written;
	  remaining -= bytes_written;
	}
      } while (remaining > 0);
      exit(0);
    }
    close(desc);
  }
}
