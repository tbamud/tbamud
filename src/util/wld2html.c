/* ************************************************************************
*  File: wld2html.c                                        Part of tbaMUD *
*  Usage: Convert a DikuMUD .wld file into a series of .html files        *
*                                                                         *
*  This program is in the public domain.                                  *
*  Written (QUICKLY AND DIRTILY) by Jeremy Elson                          *
*  Based on the Circle 3.0 syntax checker program                         *
************************************************************************ */

#define log(msg) fprintf(stderr, "%s\n", msg)

#include "conf.h"
#include "sysdep.h"

#define NOWHERE    -1		/* nil reference for room-database         */

/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH          0
#define EAST           1
#define SOUTH          2
#define WEST           3
#define UP             4
#define DOWN           5
#define NORTHWEST      6
#define NORTHEAST      7
#define SOUTHEAST      8
#define SOUTHWEST      9

#define NUM_OF_DIRS    10

#define CREATE(result, type, number)  do {\
	if (!((result) = (type *) calloc ((number), sizeof(type))))\
		{ perror("malloc failure"); abort(); } } while(0)

/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR		(1 << 0)	/* Exit is a door          */
#define EX_CLOSED		(1 << 1)	/* The door is closed      */
#define EX_LOCKED		(1 << 2)	/* The door is locked      */
#define EX_PICKPROOF		(1 << 3)	/* Lock can't be picked    */

#define MAX_STRING_LENGTH	8192

typedef signed char sbyte;
typedef unsigned char ubyte;
typedef signed short int sh_int;
typedef unsigned short int ush_int;
typedef char bool;
typedef char byte;

typedef int room_num;
typedef sh_int obj_num;


char buf[MAX_STRING_LENGTH];
char buf1[MAX_STRING_LENGTH];
char buf2[MAX_STRING_LENGTH];
char arg[MAX_STRING_LENGTH];

int get_line(FILE * fl, char *buf);
int real_room(int virtual, int reference);

/* room-related structures *********************************************** */


struct room_direction_data {
  char *general_description;	/* When look DIR.                        */

  char *keyword;		/* for open/close                        */

  sh_int exit_info;		/* Exit info                             */
  obj_num key;			/* Key's number (-1 for no key)          */
  room_num to_room;		/* Where direction leads (NOWHERE)       */
};

struct extra_descr_data {
  char *keyword;		/* Keyword in look/examine          */
  char *description;		/* What to see                      */
  struct extra_descr_data *next;	/* Next in list                     */
};

struct reset_com {
  char command;			/* current command                      */

  bool if_flag;			/* if TRUE: exe only if preceding exe'd */
  int arg1;			/* */
  int arg2;			/* Arguments to the command             */
  int arg3;			/* */

  /*
   * Commands:              * 'M': Read a mobile     * 'O': Read an object    *
   * 'G': Give obj to mob   * 'P': Put obj in obj    * 'G': Obj to char       *
   * 'E': Obj to char equip * 'D': Set state of door *
   */
};



struct zone_data {
  char *name;			/* name of this zone                  */
  int lifespan;			/* how long between resets (minutes)  */
  int age;			/* current age of this zone (minutes) */
  int top;			/* upper limit for rooms in this zone */

  int reset_mode;		/* conditions for reset (see below)   */
  int number;			/* virtual number of this zone    */
  struct reset_com *cmd;	/* command table for reset                */

  /*
   * Reset mode:                              * 0: Don't reset, and don't
   * update age.    * 1: Reset if no PC's are located in zone. * 2: Just
   * reset.                           *
   */
};

/* ================== Memory Structure for room ======================= */
struct room_data {
  room_num number;		/* Rooms number (vnum)                */
  sh_int zone;			/* Room zone (for resetting)          */
  int sector_type;		/* sector type (move/hide)            */
  char *name;			/* Rooms name 'You are ...'           */
  char *description;		/* Shown when entered                 */
  struct extra_descr_data *ex_description;	/* for examine/look       */
  struct room_direction_data *dir_option[NUM_OF_DIRS];	/* Directions */
  int room_flags;		/* DEATH,DARK ... etc                 */

  byte light;			/* Number of lightsources in room     */
};

/* ====================================================================== */


/**************************************************************************
*  declarations of most of the 'global' variables                         *
************************************************************************ */

struct room_data *world = NULL;	/* array of rooms                */
int top_of_world = 0;		/* ref to top element of world   */
int rec_count = 0;



/* local functions */
char *fread_string(FILE * fl, char *error);
void setup_dir(FILE * fl, int room, int dir);
void index_boot(int cnt, char **name);
void discrete_load(FILE * fl);
void parse_room(FILE * fl, int virtual_nr);
void parse_mobile(FILE * mob_f, int nr);
char *parse_object(FILE * obj_f, int nr);
void assign_rooms(void);
void renum_world(void);
void write_output(void);


char *dir_names[] =
{"North", "East", "South", "West", "Up", "Down","North West","North East","South East","South West"};


/*************************************************************************
*  routines for booting the system                                       *
*********************************************************************** */

/* body of the booting system */
int main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <world-file-name(s)>\n", argv[0]);
    exit(1);
  }

  index_boot(argc,argv);

  log("Writing output.");
  write_output();
  log("Done.");

  return (0);
}

/* Since the world is loaded into memory by index
 * and not room number, we need to search through
 * all rooms and return the correct one. This is
 * used to generate door information (ie: the name)
 */
struct room_data* findRoom(int nr)
{
  int i;
  
  for (i=0;i<rec_count;i++)
    if (world[i].number==nr)
      return &world[i];

  return NULL;
}

void write_output(void)
{
  int i;
  FILE *fl;
  char buf[128];
  register int door, found;

  for (i=0;i<rec_count;i++) {
    //print the record number, but no linefeed.
    fprintf(stderr, "Record: %d ",i);
    if (world[i].name == NULL) {
      //linefeed the prior record since we're skipping this one.
      log("");
      //the name is blank, which means, most likely, the record is bad as well.
      continue;
    }
    sprintf(buf, "Writing %d.html", world[i].number);
    log(buf);
    //for some reason, if you use %d with sprintf it rolls over like its 16bit,
    //but only if you use the buffer with fopen.
    //using %ld and casting to long solves this in case someone really wants
    //to use negative room numbers.
    sprintf(buf, "%ld.html",(long)world[i].number);
    if (!(fl = fopen(buf, "w"))) {
      perror("opening output file");
      exit(1);
    }
    fprintf(fl, "<title> %s </title>\n", world[i].name);
    fprintf(fl, "<h1> %s </h1>\n", world[i].name);
    fprintf(fl, "<pre>\n");
    fputs(world[i].description, fl);
    fprintf(fl, "</pre>\n");
    fprintf(fl, "<P> Exits: <P> \n");

    found = 0;
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (world[i].dir_option[door] &&
          world[i].dir_option[door]->to_room != NOWHERE) {
        found = 1;
	//this call gets a pointer to the room referenced by the to_room for the door.
	//This fixes a lot of issues introduced with the whole 'renumbering rooms' call
	//and the binary search that didn't work well.
	struct room_data* to_room = findRoom(world[i].dir_option[door]->to_room);
        fprintf(fl, "<a href = \"%d.html\"> %s to %s</a> <p>\n",
                to_room->number,
                dir_names[door],
                to_room->name);
      }
    if (!found)
      fprintf(fl, "None!");
    fclose(fl);
  }
}  

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE * fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return (count);
}



void index_boot(int cnt, char **names)
{
  FILE *db_file;

  //throw first entry away as that is the executable.
  for (int i=1;i<cnt;i++) {
    if (!(db_file = fopen(names[i], "r"))) {
      perror("error opening world file");
      exit(1);
    }
    //have to loop through files twice.
    //once to get total record count
    //second time to load them
    rec_count += count_hash_records(db_file);
    fclose(db_file);
  }
  sprintf(buf,"Total records: %d\n",rec_count);
  log(buf);
  //now that we know how many records in total
  //we can create the memory structure
  CREATE(world, struct room_data, rec_count);
  //now loop through files and load them
  for (int i=1;i<cnt;i++) {
    if (!(db_file = fopen(names[i], "r"))) {
      perror("error opening world file");
      exit(1);
    }
    discrete_load(db_file);
  }
}


void discrete_load(FILE * fl)
{
  int nr = -1, last = 0;
  char line[256];

  for (;;) {
    if (!get_line(fl, line)) {
      fprintf(stderr, "Format error after room #%d\n", nr);
      exit(1);
    }
    if (*line == 'T') //Toss triggers. THey currently break this util.
      continue;
	  
    if (*line == '$')
      return;

    if (*line == '#') {
      last = nr;
      if (sscanf(line, "#%d", &nr) != 1) {
	fprintf(stderr, "Format error after room #%d\n", last);
	exit(1);
      }
      if (nr >= 99999)
	return;
      else
	parse_room(fl, nr);
    } else {
      fprintf(stderr, "Format error in world file near room #%d\n", nr);
      fprintf(stderr, "Offending line: '%s'\n", line);
      exit(1);
    }
  }
}

long asciiflag_conv(char *flag)
{
  long flags = 0;
  int is_number = 1;
  register char *p;

  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1 << (*p - 'a');
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A'));

    if (!isdigit(*p))
      is_number = 0;
  }

  if (is_number)
    flags = atol(flag);

  return (flags);
}

/* load the rooms */
void parse_room(FILE * fl, int virtual_nr)
{
  static int room_nr = 0, zone = 0;
  int t[10], i;
  char line[256], flags[128];
  struct extra_descr_data *new_descr;

  sprintf(buf2, "room #%d", virtual_nr);

  world[room_nr].zone = zone;
  world[room_nr].number = virtual_nr;
  world[room_nr].name = fread_string(fl, buf2);
  world[room_nr].description = fread_string(fl, buf2);

  if (!get_line(fl, line) || sscanf(line, " %d %s %d ", t, flags, t + 2) != 3) {
    fprintf(stderr, "Format error in room #%d\n", virtual_nr);
    exit(1);
  }
  /* t[0] is the zone number; ignored with the zone-file system */
  world[room_nr].room_flags = asciiflag_conv(flags);
  world[room_nr].sector_type = t[2];

  world[room_nr].light = 0;	/* Zero light sources */

  for (i = 0; i < NUM_OF_DIRS; i++)
    world[room_nr].dir_option[i] = NULL;

  world[room_nr].ex_description = NULL;

  sprintf(buf, "Format error in room #%d (expecting D/E/S)", virtual_nr);

  for (;;) {
    if (!get_line(fl, line)) {
      fprintf(stderr, "%s\n", buf);
      exit(1);
    }
    switch (*line) {
    case 'D':
      setup_dir(fl, room_nr, atoi(line + 1));
      break;
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(fl, buf2);
      new_descr->description = fread_string(fl, buf2);
      new_descr->next = world[room_nr].ex_description;
      world[room_nr].ex_description = new_descr;
      break;
    case 'S':			/* end of room */
      top_of_world = room_nr++;
      return;
      break;
    default:
      fprintf(stderr, "%s\n", buf);
      exit(1);
      break;
    }
  }
}



/* read direction data */
void setup_dir(FILE * fl, int room, int dir)
{
  int t[5];
  char line[256];

  sprintf(buf2, "room #%d, direction D%d", world[room].number, dir);

  CREATE(world[room].dir_option[dir], struct room_direction_data, 1);
  world[room].dir_option[dir]->general_description = fread_string(fl, buf2);
  world[room].dir_option[dir]->keyword = fread_string(fl, buf2);

  if (!get_line(fl, line)) {
    fprintf(stderr, "Format error, %s\n", buf2);
    exit(1);
  }
  if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
    fprintf(stderr, "Format error, %s\n", buf2);
    exit(1);
  }
  if (t[0] == 1)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR;
  else if (t[0] == 2)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
  else
    world[room].dir_option[dir]->exit_info = 0;

  world[room].dir_option[dir]->key = t[1];
  world[room].dir_option[dir]->to_room = t[2];
}


/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*********************************************************************** */

/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE * fl, char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[512], *rslt;
  register char *point;
  int done = 0, length = 0, templength = 0;

  *buf = '\0';

  do {
    if (!fgets(tmp, 512, fl)) {
      fprintf(stderr, "SYSERR: fread_string: format error at or near %s\n",
	      error);
      exit(1);
    }
    /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
    if ((point = strchr(tmp, '~')) != NULL) {
      *point = '\0';
      done = 1;
    } else {
      point = tmp + strlen(tmp) - 1;
      *(point++) = '\r';
      *(point++) = '\n';
      *point = '\0';
    }

    templength = strlen(tmp);

    if (length + templength >= MAX_STRING_LENGTH) {
      log("SYSERR: fread_string: string too large (db.c)");
      log(error);
      exit(1);
    } else {
      strcat(buf + length, tmp);
      length += templength;
    }
  } while (!done);

  /* allocate space for the new string and copy it */
  if (strlen(buf) > 0) {
    CREATE(rslt, char, length + 1);
    strcpy(rslt, buf);
  } else
    rslt = NULL;

  return (rslt);
}



/* get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 */
int get_line(FILE * fl, char *buf)
{
  char temp[256], *buf2;

  do {
    buf2 = fgets(temp, 256, fl);
    if (*temp)
      temp[strlen(temp) - 1] = '\0';
  } while (!feof(fl) && (*temp == '*' || !*temp));

  if (feof(fl))
    return (0);
  else {
    strcpy(buf, temp);
    return (1);
  }
}
