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


int walkdir(FILE* index_file, char *dir);
int get_line(FILE *fl, char *buf);

int main(int argc, char** argv)
{
 	FILE *index_file;
	int errors;

  if ( argc == 1 )	{
	  printf("Usage: %s indexfile\n",argv[0]);
	  return 0;
 	}
 	if (!(index_file = fopen(argv[1], "w"))) {
    perror("error opening index file");
    return 1;
  }

  errors = walkdir(index_file, ".");

  fprintf(index_file, "~\n");
  fclose(index_file);
	return errors ? 1 : 0;
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
	char *fromFile = findLine(plr_file, "Id  :");

	if (fromFile == NULL)
		return -1;

	return atol(fromFile);
}

int parselevel(FILE *plr_file) {
	char *fromFile = findLine(plr_file, "Levl:");

	/* The game omits the Levl tag when the level is the default (0), which
	 * is the case for characters saved during creation before do_start()
	 * runs. Match load_char() and treat a missing tag as level 0. */
	if (fromFile == NULL)
		return 0;

	return atoi(fromFile);
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
	char *fromFile = findLine(plr_file, "Last:");

	if (fromFile == NULL)
		return -1;

	return atol(fromFile);
}


int walkdir(FILE *index_file, char *dir) {
 	char filename_qfd[1000] ;
	struct dirent *dp;
 	DIR *dfd;
	int errors = 0;

 	if ((dfd = opendir(dir)) == NULL)
 	{
	  perror(dir);
	  return 1;
 	}

 	while ((dp = readdir(dfd)) != NULL)
 	{
	  struct stat stbuf ;
	  {
	    int n = snprintf(filename_qfd, sizeof(filename_qfd), "%s/%s", dir, dp->d_name);
	    if (n < 0 || n >= (int)sizeof(filename_qfd)) {
	      fprintf(stderr, "Path too long: %s/%s\n", dir, dp->d_name);
	      errors++;
	      continue;
	    }
	  }

	  if (stat(filename_qfd, &stbuf) == -1) {
	    perror(filename_qfd);
	    errors++;
	    continue;
	  }

  	if ( ( stbuf.st_mode & S_IFMT ) == S_IFDIR ) {
			if (!strcmp(".", dp->d_name) || !strcmp("..", dp->d_name))
   			continue;

		errors += walkdir(index_file, filename_qfd);
  	} else {
			char *name = parsename(dp->d_name);

			if (name != NULL) {
  			FILE *plr_file = fopen(filename_qfd, "r");

			if (plr_file == NULL) {
				perror(filename_qfd);
				errors++;
				continue;
			}

			long id = parseid(plr_file);
			if (id < 0) {
				fprintf(stderr,
				        "Skipping %s: missing Id field\n",
				        filename_qfd);
				fclose(plr_file);
				errors++;
				continue;
			}

			int level = parselevel(plr_file);

			long last = parselast(plr_file);
			if (last < 0) {
				fprintf(stderr,
				        "Skipping %s: missing Last field\n",
				        filename_qfd);
				fclose(plr_file);
				errors++;
				continue;
			}

			int adminlevel = parseadminlevel(plr_file, level);

			if (level > 30)
				level = 30;

			fprintf(index_file,
			        "%ld %s %d %d 0 %ld\n",
			        id, name, level, adminlevel, last);

        fclose(plr_file);
  		}
  	}
 	}

	closedir(dfd);
	return errors;
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
