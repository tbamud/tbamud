/* ************************************************************************
*   File: wld2html.c                                                      *
*  Usage: Convert a DikuMUD .wld file into a series of .html files        *
*                                                                         *
*  This program is in the public domain.                                  *
*  Written (QUICKLY AND DIRTILY) by Jeremy Elson (jelson@circlemud.org)   *
*  Based on the Circle 3.0 syntax checker program (scheck.c)              *
************************************************************************ */

#define log(msg) fprintf(stderr, "%s\n", msg)

#include "conf.h"
#include "sysdep.h"


#define MEM_USE 10000
char buf[MEM_USE];

int get_line(FILE * fl, char *buf);
void skip_spaces(char **string);
void parse_webster_html(void);
int main(int argc, char **argv)
{
  int pid = 0;
  if (argc != 3) {
    return 0; /* no word/pid given */
  }
  pid = atoi(argv[2]);
  
  snprintf(buf, sizeof(buf), 
    "wget http://www.m-w.com/cgi-bin/dictionary?book=Dictionary\\&va=%s"
    " -Owebster.html -o/dev/null", argv[1]);
  system(buf);

  parse_webster_html();

  if (pid)
    kill(pid, SIGUSR2);

  return (0);
}

void parse_webster_html(void) {
  FILE *infile, *outfile;
  char scanbuf[MEM_USE], *p, *q;
  
  outfile = fopen("websterinfo", "w");
  if (!outfile) 
    exit(1);

  infile = fopen("webster.html", "r");
  if (!infile) {
    fprintf(outfile, "A bug has occured in webster. (no webster.html) Please notify Welcor.");
    fclose(outfile);
    return;
  }

  unlink("webster.html"); /* We can still read */
  
  for ( ; get_line(infile, buf)!=0; ) {
    p = buf;
    skip_spaces(&p);
    /* <PRE> tag means word wasn't found in dictionary */
    /* list on the form 

	 1. <a href="/cgi-bin/dictionary?va=XXX">XXX</a>
	 2. <a href="/cgi-bin/dictionary?va=YYY">YYY</a>
         ...
         </PRE>
       follows */
    if (!strncmp(p, "<PRE>", 5)) {
      fprintf(outfile, "Did you really mean any of these instead ?\n");
      for (; get_line(infile, buf) != 0;) {
        p = buf;
        skip_spaces(&p);
        if (!strncmp(p, "</PRE>", 6))
          break;
        p = strchr(p, '>');
        p++; /* p now points to first letter of word. */
        q = strchr(p, '<');
        *q = '\0';
        fprintf(outfile, "%s\n", p);
      }
      break;
    } else if (!strncmp(p, "Main Entry:", 10)) {
      int coloumn = 0;
      /* Date: means word was found in dictionary */
      /* M-W has changed their site layout, so we need to find the correct line :*/
      while (*p != '<') {
        get_line(infile, buf);
        p = buf;
        skip_spaces(&p);
      }  
      /* The next line contains ALL info on that word. 
       * Including html tags, this can be very much 
       */
      fprintf(outfile, "That means:\n");
      /* remove all tags from this line - ALL tags */
      for (q = scanbuf; *p && q - scanbuf < sizeof(scanbuf); p++) {
        if (*p == '&') {
          /* &gt; and &lt; translates into '"' */
          if ((*(p+1) == 'l' || *(p+1) == 'g') && *(p+2) == 't' && *(p+3) == ';') {
            *q++='"';
            coloumn++;
            p += 3;
            continue;
          }
        }
        if (*p == '<') {
          /* <br> tags translate into '\n' */
          if (*(p+1) == 'b' && *(p+2) == 'r') {
            *q++='\n';
            coloumn = 0;
          }
          for (; *p && *p != '>';p++) ;
          continue;
        }
        if (isspace(*p) && coloumn > 70) { /* wrap at first space after 70th coloumn */
          *q++='\n';
          coloumn = 0;
          continue;
        }
          
        *q++ = *p;
        coloumn++;
      }
      *q = '\0';

      fprintf(outfile, "%s\n", scanbuf);
      break;
    }
  }
  fclose(infile);
 
  fprintf(outfile, "~");
  fclose(outfile);
}

/* get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  
 */
int get_line(FILE * fl, char *buf)
{
  char temp[MEM_USE];

  do {
    fgets(temp, MEM_USE, fl);
    if (*temp)
      temp[strlen(temp) - 1] = '\0';
  } while (!feof(fl) && !*temp);

  if (feof(fl))
    return (0);
  else {
    strcpy(buf, temp);
    return (1);
  }
}

/*
 * Function to skip over the leading spaces of a string.
 */
void skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++);
}
