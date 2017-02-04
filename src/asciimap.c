/**************************************************************************
*  File: asciimap.c                                        Part of tbaMUD *
*  Usage: Generates an ASCII map of the player's surroundings.            *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "asciimap.h"

/******************************************************************************
 * Begin Local (File Scope) Defines and Global Variables
 *****************************************************************************/
/* Do not blindly change these values, as many values cause the map to stop working - backup first */
#define CANVAS_HEIGHT 19
#define CANVAS_WIDTH  51
#define LEGEND_WIDTH  15

#define DEFAULT_MAP_SIZE CONFIG_MAP_SIZE

#define MAX_MAP_SIZE (CANVAS_WIDTH - 1)/4
#define MAX_MAP      CANVAS_WIDTH

#define MAX_MAP_DIR 10
#define MAX_MAP_FOLLOW 10

#define SECT_EMPTY 30 /* anything greater than num sect types */
#define SECT_STRANGE (SECT_EMPTY + 1)
#define SECT_HERE  (SECT_STRANGE + 1)

#define DOOR_NS   -1
#define DOOR_EW   -2
#define DOOR_UP   -3
#define DOOR_DOWN -4
#define DOOR_DIAGNE      -5
#define DOOR_DIAGNW      -6
#define VDOOR_NS         -7
#define VDOOR_EW         -8
#define VDOOR_DIAGNE     -9
#define VDOOR_DIAGNW     -10
#define DOOR_UP_AND_NE   -11
#define DOOR_DOWN_AND_SE -12
#define DOOR_NONE        -13
#define NUM_DOOR_TYPES 13

#define MAP_CIRCLE    0
#define MAP_RECTANGLE 1

#define MAP_NORMAL  0
#define MAP_COMPACT 1

static bool show_worldmap(struct char_data *ch);

struct map_info_type
{
  int  sector_type;
  char disp[20];
};

static struct map_info_type door_info[] =
{
  { DOOR_NONE, "   " },
  { DOOR_DOWN_AND_SE, "\tr-\tn\\ " },
  { DOOR_UP_AND_NE,   "\tr+\tn/ " },
  { VDOOR_DIAGNW,     " \tm+\tn " },
  { VDOOR_DIAGNE,     " \tm+\tn "},
  { VDOOR_EW,  " \tm+\tn " },
  { VDOOR_NS,  " \tm+\tn "},
  { DOOR_DIAGNW,      " \\ " },
  { DOOR_DIAGNE,      " / " },
  { DOOR_DOWN, "\tr-\tn  " },
  { DOOR_UP,   "\tr+\tn  " },
  { DOOR_EW,   " - " },
  { DOOR_NS,   " | " }
};

static struct map_info_type compact_door_info[] =
{
  { DOOR_NONE, " " },
  { DOOR_DOWN_AND_SE, "\tR\\\tn" },
  { DOOR_UP_AND_NE,   "\tR/\tn" },
  { VDOOR_DIAGNW,   "\tm+\tn" },
  { VDOOR_DIAGNE,   "\tm+\tn"},
  { VDOOR_EW,  " \tm+\tn " },
  { VDOOR_NS,  " \tm+\tn "},
  { DOOR_DIAGNW,"\\" },
  { DOOR_DIAGNE,"/" },
  { DOOR_DOWN, "\tr-\tn" },
  { DOOR_UP,   "\tr+\tn" },
  { DOOR_EW,   "-" },
  { DOOR_NS,   " | " }
};

/* Add new sector types below for both map_info and world_map_info     */
/* The last 3 MUST remain the same, although the symbol can be changed */
/* New sectors also need to be added to the perform_map function below */
static struct map_info_type map_info[] =
{
  { SECT_INSIDE,       "\tc[\tn.\tc]\tn" }, /* 0 */
  { SECT_CITY,         "\tc[\twC\tc]\tn" },
  { SECT_FIELD,        "\tc[\tg,\tc]\tn" },
  { SECT_FOREST,       "\tc[\tgY\tc]\tn" },
  { SECT_HILLS,        "\tc[\tMm\tc]\tn" },
  { SECT_MOUNTAIN,     "\tc[\trM\tc]\tn" }, /* 5 */
  { SECT_WATER_SWIM,   "\tc[\tc~\tc]\tn" },
  { SECT_WATER_NOSWIM, "\tc[\tb=\tc]\tn" },
  { SECT_FLYING,       "\tc[\tC^\tc]\tn" },
  { SECT_UNDERWATER,   "\tc[\tbU\tc]\tn" },
  { -1,                ""        }, /* 10 */
  { -1,                ""        },
  { -1,                ""        },
  { -1,                ""        },
  { -1,                ""        },
  { -1,                ""        }, /* 15 */
  { -1,                ""        },
  { -1,                ""        },
  { -1,                ""        },
  { -1,                ""        },
  { -1,                ""        }, /* 20 */
  { -1,                ""        },
  { -1,                ""        },
  { -1,                ""        },
  { -1,                ""        },
  { -1,                ""        }, /* 25 */
  { -1,                ""        },
  { -1,                ""        },
  { -1,                ""        },
  { -1,                ""        },
  { SECT_EMPTY,        "   "     }, /* 30 */
  { SECT_STRANGE,      "\tc[\tR?\tc]\tn" },
  { SECT_HERE,         "\tc[\tB!\tc]\tn"     },
};

static struct map_info_type world_map_info[] =
{
  { SECT_INSIDE,       "\tn."  }, /* 0 */
  { SECT_CITY,         "\twC"  },
  { SECT_FIELD,        "\tg,"  },
  { SECT_FOREST,       "\tgY"  },
  { SECT_HILLS,        "\tMm"  },
  { SECT_MOUNTAIN,     "\trM"  }, /* 5 */
  { SECT_WATER_SWIM,   "\tc~"  },
  { SECT_WATER_NOSWIM, "\tb="  },
  { SECT_FLYING,       "\tC^"  },
  { SECT_UNDERWATER,   "\tbU"  },
  { -1,                ""     }, /* 10 */
  { -1,                ""     },
  { -1,                ""     },
  { -1,                ""     },
  { -1,                ""     },
  { -1,                ""     }, /* 15 */
  { -1,                ""     },
  { -1,                ""     },
  { -1,                ""     },
  { -1,                ""     },
  { -1,                ""     }, /* 20 */
  { -1,                ""     },
  { -1,                ""     },
  { -1,                ""     },
  { -1,                ""     },
  { -1,                ""     }, /* 25 */
  { -1,                ""     },
  { -1,                ""     },
  { -1,                ""     },
  { -1,                ""     },
  { SECT_EMPTY,        " "    }, /* 30 */
  { SECT_STRANGE,      "\tR?"  },
  { SECT_HERE,         "\tB!"  },
};


static int map[MAX_MAP][MAX_MAP];
/*
static int offsets[4][2] ={ {-2, 0},{ 0, 2},{ 2, 0},{ 0, -2} };
static int offsets_worldmap[4][2] ={ {-1, 0},{ 0, 1},{ 1, 0},{ 0, -1} };
static int door_offsets[6][2] ={ {-1, 0},{ 0, 1},{ 1, 0},{ 0, -1},{ -1, 1},{ 1, 1} };
static int door_marks[6] = { DOOR_NS, DOOR_EW, DOOR_NS, DOOR_EW, DOOR_UP, DOOR_DOWN };
*/
static int offsets[10][2] ={ {-2, 0},{ 0, 2},{ 2, 0},{ 0, -2},{0, 0},{ 0, 0},{ -2, -2},{ -2, 2},{2, 2},{ 2, -2} };
static int offsets_worldmap[10][2] ={ {-1, 0},{ 0, 1},{ 1, 0},{ 0, -1},{0, 0},{ 0, 0},{ -1, -1},{ -1, 1},{1, 1},{ 1, -1} };
static int door_offsets[10][2] ={ {-1, 0},{ 0, 1},{ 1, 0},{ 0, -1},{ -1, 1},{ 1, 1},{ -1, -1},{ -1, 1},{ 1, 1},{ 1, -1} };
static int door_marks[10] = { DOOR_NS, DOOR_EW, DOOR_NS, DOOR_EW, DOOR_UP, DOOR_DOWN, DOOR_DIAGNW, DOOR_DIAGNE, DOOR_DIAGNW, DOOR_DIAGNE};
static int vdoor_marks[4] = { VDOOR_NS, VDOOR_EW, VDOOR_NS, VDOOR_EW };
/******************************************************************************
 * End Local (File Scope) Defines and Global Variables
 *****************************************************************************/

/******************************************************************************
 * Begin Local (File Scope) Function Prototypes
 *****************************************************************************/
static void MapArea(room_rnum room, struct char_data *ch, int x, int y, int min, int max, sh_int xpos, sh_int ypos, bool worldmap);
static char *StringMap(int centre, int size);
static char *WorldMap(int centre, int size, int mapshape, int maptype );
static char *CompactStringMap(int centre, int size);
static void perform_map( struct char_data *ch, char *argument, bool worldmap );
/******************************************************************************
 * End Local (File Scope) Function Prototypes
 *****************************************************************************/


bool can_see_map(struct char_data *ch) {
  /* Is the map funcionality disabled? */
  if (CONFIG_MAP == MAP_OFF)
    return FALSE;
  else if ((CONFIG_MAP == MAP_IMM_ONLY) && (GET_LEVEL(ch) < LVL_IMMORT))
    return FALSE;

  return TRUE;
}

/* MapArea function - create the actual map */
static void MapArea(room_rnum room, struct char_data *ch, int x, int y, int min, int max, sh_int xpos, sh_int ypos, bool worldmap)
{
  room_rnum prospect_room;
  struct room_direction_data *pexit;
  int door, ew_size=0, ns_size=0, x_exit_pos=0, y_exit_pos=0;
  sh_int prospect_xpos, prospect_ypos;

  if (map[x][y] < 0)
    return; /* this is a door */

  /* marks the room as visited */
  if(room == IN_ROOM(ch))
    map[x][y] = SECT_HERE;
  else
    map[x][y] = SECT(room);

  if ( (x < min) || ( y < min) || ( x > max ) || ( y > max) ) return;

  /* Check for exits */
  for ( door = 0; door < MAX_MAP_DIR; door++ ) {

    if( door < MAX_MAP_FOLLOW &&
        xpos+door_offsets[door][0] >= 0 &&
        xpos+door_offsets[door][0] <= ns_size &&
        ypos+door_offsets[door][1] >= 0 &&
        ypos+door_offsets[door][1] <= ew_size)
    { /* Virtual exit */

      map[x+door_offsets[door][0]][y+door_offsets[door][1]] = vdoor_marks[door] ;
      if (map[x+offsets[door][0]][y+offsets[door][1]] == SECT_EMPTY )
        MapArea(room,ch,x + offsets[door][0], y + offsets[door][1], min, max, xpos+door_offsets[door][0], ypos+door_offsets[door][1], worldmap);
      continue;
    }

    if ( (pexit = world[room].dir_option[door]) != NULL  &&
         (pexit->to_room > 0 ) && (pexit->to_room != NOWHERE) &&
         (!IS_SET(pexit->exit_info, EX_CLOSED)) &&
         (!IS_SET(pexit->exit_info, EX_HIDDEN) || PRF_FLAGGED(ch, PRF_HOLYLIGHT)) )
    { /* A real exit */

      /* But is the door here... */
      switch (door) {
      case NORTH:
        if(xpos > 0 || ypos!=y_exit_pos) continue;
        break;
      case SOUTH:
        if(xpos < ns_size || ypos!=y_exit_pos) continue;
        break;
      case EAST:
        if(ypos < ew_size || xpos!=x_exit_pos) continue;
        break;
      case WEST:
        if(ypos > 0 || xpos!=x_exit_pos) continue;
        break;
      case NORTHWEST:
        if(xpos > 0 || ypos!=y_exit_pos || ypos > 0 || xpos!=x_exit_pos) continue;
        break;
      case NORTHEAST:
        if(xpos > 0 || ypos!=y_exit_pos || ypos < ew_size || xpos!=x_exit_pos) continue;
        break;
      case SOUTHEAST:
        if(xpos < ns_size || ypos!=y_exit_pos || ypos < ew_size || xpos!=x_exit_pos) continue;
        break;
      case SOUTHWEST:
        if(xpos < ns_size || ypos!=y_exit_pos || ypos > 0 || xpos!=x_exit_pos) continue;
        break;
      }


 /*     if ( (x < min) || ( y < min) || ( x > max ) || ( y > max) ) return;*/
      prospect_room = pexit->to_room;

        /* one way into area OR maze */
        if ( world[prospect_room].dir_option[rev_dir[door]] &&
             world[prospect_room].dir_option[rev_dir[door]]->to_room != room) {
          map[x][y] = SECT_STRANGE;
        return;
        }

      if(!worldmap) {
        if ((map[x+door_offsets[door][0]][y+door_offsets[door][1]] == DOOR_NONE) ||
            (map[x+door_offsets[door][0]][y+door_offsets[door][1]] == SECT_EMPTY)  ) {
          map[x+door_offsets[door][0]][y+door_offsets[door][1]] = door_marks[door];
        } else {
          if ( ((door == NORTHEAST) && (map[x+door_offsets[door][0]][y+door_offsets[door][1]] == DOOR_UP)) ||
               ((door == UP) && (map[x+door_offsets[door][0]][y+door_offsets[door][1]] == DOOR_DIAGNE))  ) {
            map[x+door_offsets[door][0]][y+door_offsets[door][1]] = DOOR_UP_AND_NE;
          }
          else if ( ((door == SOUTHEAST) && (map[x+door_offsets[door][0]][y+door_offsets[door][1]] == DOOR_DOWN)) ||
                    ((door == DOWN) && (map[x+door_offsets[door][0]][y+door_offsets[door][1]] == DOOR_DIAGNW))  ) {
            map[x+door_offsets[door][0]][y+door_offsets[door][1]] = DOOR_DOWN_AND_SE;
          }
        }
      }

      prospect_xpos = prospect_ypos = 0;
      switch (door) {
      case NORTH:
        prospect_xpos = ns_size;
      case SOUTH:
        prospect_ypos = world[prospect_room].dir_option[rev_dir[door]] ? y_exit_pos : ew_size/2;
      break;
      case WEST:
        prospect_ypos = ew_size;
      case EAST:
        prospect_xpos = world[prospect_room].dir_option[rev_dir[door]] ? x_exit_pos : ns_size/2;
        break;
      case NORTHEAST:
      case NORTHWEST:
      case SOUTHEAST:
      case SOUTHWEST:
        prospect_xpos = world[prospect_room].dir_option[rev_dir[door]] ? x_exit_pos : ns_size/2;
        prospect_ypos = world[prospect_room].dir_option[rev_dir[door]] ? y_exit_pos : ew_size/2;
        break;
      }

      if(worldmap) {
 if ( door < MAX_MAP_FOLLOW && map[x+offsets_worldmap[door][0]][y+offsets_worldmap[door][1]] == SECT_EMPTY )
   MapArea(pexit->to_room,ch,x + offsets_worldmap[door][0], y + offsets_worldmap[door][1], min, max, prospect_xpos, prospect_ypos, worldmap);
      } else {
 if ( door < MAX_MAP_FOLLOW && map[x+offsets[door][0]][y+offsets[door][1]] == SECT_EMPTY )
   MapArea(pexit->to_room,ch,x + offsets[door][0], y + offsets[door][1], min, max, prospect_xpos, prospect_ypos, worldmap);
      }
    } /* end if exit there */
  }
  return;
}

/* Returns a string representation of the map */
static char *StringMap(int centre, int size)
{
  static char strmap[MAX_MAP*MAX_MAP*11 + MAX_MAP*2 + 1];
  char *mp = strmap;
  char *tmp;
  int x, y;

  /* every row */
  for (x = centre - CANVAS_HEIGHT/2; x <= centre + CANVAS_HEIGHT/2; x++) {
    /* every column */
    for (y = centre - CANVAS_WIDTH/6; y <= centre + CANVAS_WIDTH/6; y++) {
      if (abs(centre - x)<=size && abs(centre-y)<=size)
        tmp = (map[x][y]<0) ? \
       door_info[NUM_DOOR_TYPES + map[x][y]].disp : \
       map_info[map[x][y]].disp ;
      else
 tmp = map_info[SECT_EMPTY].disp;
      strcpy(mp, tmp);
      mp += strlen(tmp);
    }
    strcpy(mp, "\r\n");
    mp+=2;
  }
  *mp='\0';
  return strmap;
}

static char *WorldMap(int centre, int size, int mapshape, int maptype )
{
  static char strmap[MAX_MAP*MAX_MAP*4 + MAX_MAP*2 + 1];
  char *mp = strmap;
  int x, y;
  int xmin, xmax, ymin, ymax;

  switch(maptype) {
    case MAP_COMPACT:
      xmin = centre - size;
      xmax = centre + size;
      ymin = centre - 2*size;
      ymax = centre + 2*size;
      break;
    default:
      xmin = centre - CANVAS_HEIGHT/2;
      xmax = centre + CANVAS_HEIGHT/2;
      ymin = centre - CANVAS_WIDTH/2;
      ymax = centre + CANVAS_WIDTH/2;
  }


  /* every row */
  /* for (x = centre - size; x <= centre + size; x++) { */
  for (x = xmin; x <= xmax; x++) {
    /* every column */
    /* for (y = centre - (2*size) ; y <= centre + (2*size) ; y++) {  */
    for (y = ymin ; y <= ymax ; y++) {

      if((mapshape == MAP_RECTANGLE && abs(centre - y) <= size*2  && abs(centre - x) <= size ) ||
   ((mapshape == MAP_CIRCLE) && (centre-x)*(centre-x) + (centre-y)*(centre-y)/4 <= (size * size + 1))) {
        strcpy(mp, world_map_info[map[x][y]].disp);
        mp += strlen(world_map_info[map[x][y]].disp);
      } else {
 strcpy(mp++, " ");
      }
    }
    strcpy(mp, "\tn\r\n");
    mp+=4;
  }
  *mp='\0';
  return strmap;
}

static char *CompactStringMap(int centre, int size)
{
  static char strmap[MAX_MAP*MAX_MAP*12 + MAX_MAP*2 + 1];
  char *mp = strmap;
  int x, y;

  /* every row */
  for (x = centre - size; x <= centre + size; x++) {
    /* every column */
    for (y = centre - size; y <= centre + size; y++) {
      strcpy(mp, (map[x][y]<0) ? \
       compact_door_info[NUM_DOOR_TYPES + map[x][y]].disp : \
       map_info[map[x][y]].disp);
      mp += strlen((map[x][y]<0) ? \
       compact_door_info[NUM_DOOR_TYPES + map[x][y]].disp : \
       map_info[map[x][y]].disp);
    }
    strcpy(mp, "\r\n");
    mp+=2;
  }
  *mp='\0';
  return strmap;
}

/* Display a nicely formatted map with a legend */
static void perform_map( struct char_data *ch, char *argument, bool worldmap )
{
  int size = DEFAULT_MAP_SIZE;
  int centre, x, y, min, max;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  int count = 0;
  int ew_size=0, ns_size=0;
  int mapshape = MAP_CIRCLE;

  two_arguments( argument, arg1 , arg2 );
  if(*arg1)
  {
    size = atoi(arg1);
  }
  if (*arg2)
  {
    if (is_abbrev(arg2, "normal")) worldmap=FALSE;
    else if (is_abbrev(arg2, "world")) worldmap=TRUE;
    else {
      send_to_char(ch, "Usage: \tymap <distance> [ normal | world ]\tn");
      return;
    }
  }

  if(size<0) {
    size = -size;
    mapshape = MAP_RECTANGLE;
  }
  size = URANGE(1,size,MAX_MAP_SIZE);

  centre = MAX_MAP/2;

  if(worldmap) {
    min = centre - 2*size;
    max = centre + 2*size;
  } else {
    min = centre - size;
    max = centre + size;
  }

  /* Blank the map */
  for (x = 0; x < MAX_MAP; ++x)
      for (y = 0; y < MAX_MAP; ++y)
           map[x][y]= (!(y%2) && !worldmap) ? DOOR_NONE : SECT_EMPTY;

  /* starts the mapping with the centre room */
  MapArea(IN_ROOM(ch), ch, centre, centre, min, max, ns_size/2, ew_size/2, worldmap);

  /* marks the center, where ch is */
  map[centre][centre] = SECT_HERE;

  /* Feel free to put your own MUD name or header in here */
  send_to_char(ch, " \tY-\tytbaMUD Map System\tY-\tn\r\n"
                   "\tD  .-.__--.,--.__.-.\tn\r\n" );

  count += sprintf(buf + count, "\tn\tn\tn%s Up\\\\", door_info[NUM_DOOR_TYPES + DOOR_UP].disp);
  count += sprintf(buf + count, "\tn\tn\tn%s Down\\\\", door_info[NUM_DOOR_TYPES + DOOR_DOWN].disp);
  count += sprintf(buf + count, "\tn%s You\\\\", map_info[SECT_HERE].disp);
  count += sprintf(buf + count, "\tn%s Inside\\\\", map_info[SECT_INSIDE].disp);
  count += sprintf(buf + count, "\tn%s City\\\\", map_info[SECT_CITY].disp);
  count += sprintf(buf + count, "\tn%s Field\\\\", map_info[SECT_FIELD].disp);
  count += sprintf(buf + count, "\tn%s Forest\\\\", map_info[SECT_FOREST].disp);
  count += sprintf(buf + count, "\tn%s Hills\\\\", map_info[SECT_HILLS].disp);
  count += sprintf(buf + count, "\tn%s Mountain\\\\", map_info[SECT_MOUNTAIN].disp);
  count += sprintf(buf + count, "\tn%s Swim\\\\", map_info[SECT_WATER_SWIM].disp);
  count += sprintf(buf + count, "\tn%s Boat\\\\", map_info[SECT_WATER_NOSWIM].disp);
  count += sprintf(buf + count, "\tn%s Flying\\\\", map_info[SECT_FLYING].disp);
  count += sprintf(buf + count, "\tn%s Underwater\\\\", map_info[SECT_UNDERWATER].disp);

  strcpy(buf, strfrmt(buf, LEGEND_WIDTH, CANVAS_HEIGHT + 2, FALSE, TRUE, TRUE));

  /* Start with an empty column */
  strcpy(buf1, strfrmt("",0, CANVAS_HEIGHT + 2, FALSE, FALSE, TRUE));

  /* Paste the legend */
  strcpy(buf2, strpaste(buf1, buf, "\tD | \tn"));

  /* Set up the map */
  memset(buf, ' ', CANVAS_WIDTH);
  count = (CANVAS_WIDTH);
  if(worldmap)
    count += sprintf(buf + count , "\r\n%s", WorldMap(centre, size, mapshape, MAP_NORMAL));
  else
    count += sprintf(buf + count , "\r\n%s", StringMap(centre, size));
  memset(buf + count, ' ', CANVAS_WIDTH);
  strcpy(buf + count + CANVAS_WIDTH, "\r\n");
  /* Paste it on */
  strcpy(buf2, strpaste(buf2, buf, "\tD | \tn"));
  /* Paste on the right border */
  strcpy(buf2, strpaste(buf2, buf1, "  "));
  /* Print it all out */
  send_to_char(ch, "%s", buf2);

  send_to_char(ch, "\tD `.-.__--.,-.__.-.-'\tn\r\n");
  return;
}

/* Display a string with the map beside it */
void str_and_map(char *str, struct char_data *ch, room_vnum target_room ) {
  int size, centre, x, y, min, max, char_size;
  int ew_size=0, ns_size=0;
  bool worldmap;

  /* Check MUDs map config options - if disabled, just show room decsription */
  if (!can_see_map(ch)) {
    send_to_char(ch, "%s", strfrmt(str, GET_SCREEN_WIDTH(ch), 1, FALSE, FALSE, FALSE));
    return;
  }

  worldmap = show_worldmap(ch);

  if(!PRF_FLAGGED(ch, PRF_AUTOMAP)) {
    send_to_char(ch, "%s", strfrmt(str, GET_SCREEN_WIDTH(ch), 1, FALSE, FALSE, FALSE));
    return;
  }

  size = CONFIG_MINIMAP_SIZE;
  centre = MAX_MAP/2;
  min = centre - 2*size;
  max = centre + 2*size;

  for (x = 0; x < MAX_MAP; ++x)
    for (y = 0; y < MAX_MAP; ++y)
      map[x][y]= (!(y%2) && !worldmap) ? DOOR_NONE : SECT_EMPTY;

  /* starts the mapping with the center room */
MapArea(target_room, ch, centre, centre, min, max, ns_size/2, ew_size/2, worldmap ); 
  map[centre][centre] = SECT_HERE;

  /* char_size = rooms + doors + padding */
  if(worldmap)
    char_size = size * 4 + 5;
  else
    char_size = 3*(size+1) + (size) + 4;

  if(worldmap)
    send_to_char(ch, "%s", strpaste(WorldMap(centre, size, MAP_CIRCLE, MAP_COMPACT), strfrmt(str, GET_SCREEN_WIDTH(ch) - char_size, size*2 + 1, FALSE, TRUE, TRUE), " \tn"));
  else
    send_to_char(ch, "%s", strpaste(CompactStringMap(centre, size), strfrmt(str, GET_SCREEN_WIDTH(ch) - char_size, size*2 + 1, FALSE, TRUE, TRUE), " \tn"));

}

static bool show_worldmap(struct char_data *ch) {
  room_rnum rm = IN_ROOM(ch);
  zone_rnum zn = GET_ROOM_ZONE(rm);

  if (ROOM_FLAGGED(rm, ROOM_WORLDMAP)) return TRUE;
  if (ZONE_FLAGGED(zn, ZONE_WORLDMAP)) return TRUE;

  return FALSE;
}

ACMD(do_map) {
  if (!can_see_map(ch)) {
    send_to_char(ch, "Sorry, the map is disabled!\r\n");
    return;
  }
  if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char(ch, "It is too dark to see the map.\r\n");
    return;
  } else if (AFF_FLAGGED(ch, AFF_BLIND) && GET_LEVEL(ch) < LVL_IMMORT) {
    send_to_char(ch, "You can't see the map while blind!\r\n");
    return;
  }
  perform_map(ch, argument, show_worldmap(ch));
}
