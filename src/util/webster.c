/* ************************************************************************
*  File: webster.c                                         Part of tbaMUD *
*  Usage: Use an online dictionary via tell m-w <word>.                   *
*                                                                         *
*  Based on the Circle 3.0 syntax checker and wld2html programs.          *
************************************************************************ */

#define log(msg) fprintf(stderr, "%s\n", msg)

#include "conf.h"
#include "sysdep.h"


#define MEM_USE 10000
char buf[MEM_USE];

int get_line(FILE * fl, char *buf);
void skip_spaces(char **string);
void parse_webster_html(char *arg);
int main(int argc, char **argv)
{
  int pid = 0;
  if (argc != 3) {
    return 0; /* no word/pid given */
  }
  pid = atoi(argv[2]);

  snprintf(buf, sizeof(buf), 
    "lynx -accept_all_cookies -source http://www.thefreedictionary.com/%s"
    " >webster.html", argv[1]);
  system(buf);

  parse_webster_html(argv[1]);

  if (pid)
    kill(pid, SIGUSR2);

  return (0);
}

void parse_webster_html(char *arg) {
  FILE *infile, *outfile;
  char scanbuf[MEM_USE], outline[MEM_USE], *p, *q;
  
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
    
    if (strncmp(buf, "<script>write_ads(AdsNum, 0)</script>", 37) != 0)
    	continue; // read until we hit the line with results in it.
    
    p = buf+37;

    if (strncmp(p, "<br>", 4) == 0)
    	{
    		fprintf(outfile, "That word could not be found.\n");
    		goto end;
    	}
		else if (strncmp(p, "<div ", 5) == 0) // definition is here, all in one line.
			{
        while (strncmp(p, "ds-list", 7)) //seek to the definition
          p++;
        
        strncpy(scanbuf, p, sizeof(scanbuf)); // strtok on a copy.
        
        p = strtok(scanbuf, ">"); // chop the line at the end of tags: <br><b>word</b> becomes "<br" "<b" "word</b"
        p = strtok(NULL, ">"); // skip the rest of this tag.

        fprintf(outfile, "Info on: %s\n\n", arg);

        while (1)
        {
     			q = outline;
     			
     			while (*p != '<')
     			{
     				assert(p < scanbuf+sizeof(scanbuf));
     			  *q++ = *p++;
     			 } 
       		if (!strncmp(p, "<br", 3) || !strncmp(p, "<p", 2) || !strncmp(p, "<div class=\"ds-list\"", 23) || !strncmp(p, "<div class=\"sds-list\"", 24))
       			*q++ = '\n';
        	  // if it's not a <br> tag or a <div class="sds-list"> or <div class="ds-list"> tag, ignore it.

					*q++='\0';
					fprintf(outfile, "%s", outline);
					
					if (!strncmp(p, "</table", 7))
						goto end;
						
				  p = strtok(NULL, ">");        	  
       	}	
			}
		else if (strncmp(p, "<div>", 5) == 0) // not found, but suggestions are ample:
			{
        strncpy(scanbuf, p, sizeof(scanbuf)); // strtok on a copy.
        
        p = strtok(scanbuf, ">"); // chop the line at the end of tags: <br><b>word</b> becomes "<br>" "<b>" "word</b>"
        p = strtok(NULL, ">"); // skip the rest of this tag.
        
        while (1)
        {
     			q = outline;
     			
     			while (*p != '<')
     			  *q++ = *p++;
     			  
       		if (!strncmp(p, "<td ", 4))
       			*q++ = '\n';
        	  // if it's not a <td> tag, ignore it.

					*q++='\0';
					fprintf(outfile, "%s", outline);
					
					if (!strncmp(p, "</table", 7))
						goto end;
						
				  p = strtok(NULL, ">");        	  
       	}	
			}
		else
			{
				// weird.. one of the above should be correct.
				fprintf(outfile, "It would appear that the free online dictionary has changed their format.\n"
				                 "Sorry, but you might need a webrowser instead.\n\n"
				                 "See http://www.thefreedictionary.com/%s", arg);
				goto end;
		  }
    }
    
end:
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
