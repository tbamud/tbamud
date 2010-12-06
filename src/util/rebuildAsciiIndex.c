/* ************************************************************************
*  file:  rebuildAsciiIndex.c                              Part of tbaMUD *
*  Copyright (C) 1990, 2010 - see 'license.doc' for complete information. *
*  All Rights Reserved                                                    *
************************************************************************* */

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#define READ_SIZE 256

int atoi(const char *str);
long atol(const char *str);


void walkdir(FILE* index_file, char *dir);
int get_line(FILE *fl, char *buf);

int main(int argc, char** argv)
{
 	FILE *index_file;
  if ( argc == 1 )	{
	  printf("Usage: %s indexfile\n",argv[0]);
	  return 0;
 	}
 	if (!(index_file = fopen(argv[1], "w"))) {
    perror("error opening index file");
    return 1;
  }
	walkdir(index_file, ".");

  fprintf(index_file, "~\n");
  fclose(index_file);
 	return 0;
}

char *parsename(char *filename) {
	static char copy[1024];
	strcpy(copy, filename);
	char *extension = strchr(copy, '.');
	if (extension == NULL) {
		return NULL;
	}
	if (strcmp(".plr", extension)) {
		return NULL;
	}
	*extension = '\0';
	return copy;
}

char *findLine(FILE *plr_file, char *tag) {
	static char line[5000];
	rewind(plr_file);

	while (get_line(plr_file, line)) {
		if(!strncmp(tag, line, strlen(tag))) {
			return line+strlen(tag);
		}
	}
	return NULL;
}

long parseid(FILE *plr_file) {
	return atol(findLine(plr_file, "Id  :"));
}

int parselevel(FILE *plr_file) {
	return atoi(findLine(plr_file, "Levl:"));
}

int parseadminlevel(FILE *plr_file, int level) {
	char *fromFile = findLine(plr_file, "Admn:");
	if (fromFile != NULL)
		return atoi(fromFile);

	if (level >= 30)
		return level-30;
	else
		return 0;
}

long parselast(FILE *plr_file) {
	return atol(findLine(plr_file, "Last:"));
}


void walkdir(FILE *index_file, char *dir) {
 	char filename_qfd[1000] ;
	struct dirent *dp;
 	DIR *dfd;

 	if ((dfd = opendir(dir)) == NULL)
 	{
	  fprintf(stderr, "Can't open %s\n", dir);
	  return;
 	}
 	while ((dp = readdir(dfd)) != NULL)
 	{
	  struct stat stbuf ;
	  sprintf( filename_qfd , "%s/%s",dir,dp->d_name) ;
	  if( stat(filename_qfd,&stbuf ) == -1 ) {
   		fprintf(stdout, "Unable to stat file: %s\n",filename_qfd) ;
   		continue ;
  	}

  	if ( ( stbuf.st_mode & S_IFMT ) == S_IFDIR ) {
			if (!strcmp(".", dp->d_name) || !strcmp("..", dp->d_name))
   			continue;

   		walkdir(index_file, filename_qfd);
  	} else {
			char *name = parsename(dp->d_name);

			if (name != NULL) {
  			FILE *plr_file = fopen(filename_qfd, "r");
 				long id = parseid(plr_file);

  			int level = parselevel(plr_file);
 				int adminlevel = parseadminlevel(plr_file, level);
 				if (level > 30)
 					level = 30;
 				long last = parselast(plr_file);

 				fprintf(index_file, "%ld %s %d %d 0 %ld\n", id, name, level, adminlevel, last);

        fclose(plr_file);
  		}
  	}
 	}
}

int get_line(FILE *fl, char *buf)
{
  char temp[READ_SIZE];
  int lines = 0;
  int sl;

  do {
    if (!fgets(temp, READ_SIZE, fl))
      return (0);
    lines++;
  } while (*temp == '*' || *temp == '\n' || *temp == '\r');

  /* Last line of file doesn't always have a \n, but it should. */
  sl = strlen(temp);
  while (sl > 0 && (temp[sl - 1] == '\n' || temp[sl - 1] == '\r'))
    temp[--sl] = '\0';

  strcpy(buf, temp); /* strcpy: OK, if buf >= READ_SIZE (256) */
  return (lines);
}
