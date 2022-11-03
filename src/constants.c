/**
* @file constants.c
* Numeric and string contants used by the MUD.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
* @todo Come up with a standard for descriptive arrays. Either all end with
* newlines or all of them don not.
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"	/* alias_data */
#include "constants.h"

/** Current tbaMUD version.
 * @todo cpp_extern isn't needed here (or anywhere) as the extern reserved word
 * works correctly with C compilers (at least in my Experience)
 * Jeremy Osborne 1/28/2008 */
cpp_extern const char *tbamud_version = "tbaMUD 2021";

/* strings corresponding to ordinals/bitvectors in structs.h */
/* (Note: strings for class definitions in class.c instead of here) */

/** Description of cardinal directions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *dirs[] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "northwest", /* Diagonals only used if CONFIG_DIAGONAL_DIRS is set */
  "northeast",
  "southeast",
  "southwest",
  "\n"
};

const char *autoexits[] =
{
  "n",
  "e",
  "s",
  "w",
  "u",
  "d",
  "nw",
  "ne",
  "se",
  "sw",
  "\n"
};

/** Room flag descriptions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *room_bits[] = {
  "DARK",
  "DEATH",
  "NO_MOB",
  "INDOORS",
  "PEACEFUL",
  "SOUNDPROOF",
  "NO_TRACK",
  "NO_MAGIC",
  "TUNNEL",
  "PRIVATE",
  "GODROOM",
  "HOUSE",
  "HCRSH",
  "ATRIUM",
  "OLC",
  "*",				/* The BFS Mark. */
  "WORLDMAP",
  "\n"
};

/** Room flag descriptions. (ZONE_x)
 * @pre Must be in the same order as the defines in structs.h.
 * Must end array with a single newline. */
const char *zone_bits[] = {
  "CLOSED",
  "NO_IMMORT",
  "QUEST",
  "GRID",
  "NOBUILD",
  "!ASTRAL",
  "WORLDMAP",
  "\n"
};

/** Exit bits for doors.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *exit_bits[] = {
  "DOOR",
  "CLOSED",
  "LOCKED",
  "PICKPROOF",
  "\n"
};

/** Description of the room sector type.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *sector_types[] = {
  "Inside",
  "City",
  "Field",
  "Forest",
  "Hills",
  "Mountains",
  "Water (Swim)",
  "Water (No Swim)",
  "In Flight",
  "Underwater",
  "\n"
};

/** PC and NPC sex.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *genders[] =
{
  "neutral",
  "male",
  "female",
  "\n"
};

/** Character positions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *position_types[] = {
  "Dead",
  "Mortally wounded",
  "Incapacitated",
  "Stunned",
  "Sleeping",
  "Resting",
  "Sitting",
  "Fighting",
  "Standing",
  "\n"
};

/** Player flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *player_bits[] = {
  "KILLER",
  "THIEF",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",
  "CSH",
  "SITEOK",
  "NOSHOUT",
  "NOTITLE",
  "DELETED",
  "LOADRM",
  "NO_WIZL",
  "NO_DEL",
  "INVST",
  "CRYO",
  "DEAD",    /* You should never see this flag on a character in game. */
  "IBT_BUG",
  "IBT_IDEA",
  "IBT_TYPO",
  "UNUSED1",
  "UNUSED2",
  "UNUSED3",
  "UNUSED4",
  "UNUSED5",
  "\n"
};

/** Mob action flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *action_bits[] = {
  "SPEC",
  "SENTINEL",
  "SCAVENGER",
  "ISNPC",
  "AWARE",
  "AGGR",
  "STAY-ZONE",
  "WIMPY",
  "AGGR_EVIL",
  "AGGR_GOOD",
  "AGGR_NEUTRAL",
  "MEMORY",
  "HELPER",
  "NO_CHARM",
  "NO_SUMMN",
  "NO_SLEEP",
  "NO_BASH",
  "NO_BLIND",
  "NO_KILL",
  "DEAD",    /* You should never see this. */
  "\n"
};

/** PC Preference flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *preference_bits[] = {
  "BRIEF",
  "COMPACT",
  "NO_SHOUT",
  "NO_TELL",
  "D_HP",
  "D_MANA",
  "D_MOVE",
  "AUTOEX",
  "NO_HASS",
  "QUEST",
  "SUMN",
  "NO_REP",
  "LIGHT",
  "C1",
  "C2",
  "NO_WIZ",
  "L1",
  "L2",
  "NO_AUC",
  "NO_GOS",
  "NO_GTZ",
  "RMFLG",
  "D_AUTO",
  "CLS",
  "BLDWLK",
  "AFK",
  "AUTOLOOT",
  "AUTOGOLD",
  "AUTOSPLIT",
  "AUTOSAC",
  "AUTOASSIST",
  "AUTOMAP",
  "AUTOKEY",
  "AUTODOOR",
  "ZONERESETS",
  "\n"
};

/** Affected bits.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *affected_bits[] =
{
  "\0", /* DO NOT REMOVE!! */
  "BLIND",
  "INVIS",
  "DET-ALIGN",
  "DET-INVIS",
  "DET-MAGIC",
  "SENSE-LIFE",
  "WATWALK",
  "SANCT",
  "GROUP",
  "CURSE",
  "INFRA",
  "POISON",
  "PROT-EVIL",
  "PROT-GOOD",
  "SLEEP",
  "NO_TRACK",
  "FLY",
  "SCUBA",
  "SNEAK",
  "HIDE",
  "UNUSED",
  "CHARM",
  "\n"
};

/** Connection type descriptions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *connected_types[] = {
  "Playing",
  "Disconnecting",
  "Get name",
  "Confirm name",
  "Get password",
  "Get new PW",
  "Confirm new PW",
  "Select sex",
  "Select class",
  "Reading MOTD",
  "Main Menu",
  "Get descript.",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",
  "Self-Delete 2",
  "Disconnecting",
  "Object edit",
  "Room edit",
  "Zone edit",
  "Mobile edit",
  "Shop edit",
  "Text edit",
  "Config edit",
  "Social edit",
  "Trigger edit",
  "Help edit",
  "Quest edit",
  "Preference edit",
  "IBT edit",
  "Message edit",
  "Protocol Detection",
  "\n"
};

/** Describes the position in the equipment listing.
 * @pre Must be in the same order as the defines.
 * Not used in sprinttype() so no \\n. */
const char *wear_where[] = {
  "<used as light>      ",
  "<worn on finger>     ",
  "<worn on finger>     ",
  "<worn around neck>   ",
  "<worn around neck>   ",
  "<worn on body>       ",
  "<worn on head>       ",
  "<worn on legs>       ",
  "<worn on feet>       ",
  "<worn on hands>      ",
  "<worn on arms>       ",
  "<worn as shield>     ",
  "<worn about body>    ",
  "<worn about waist>   ",
  "<worn around wrist>  ",
  "<worn around wrist>  ",
  "<wielded>            ",
  "<held>               "
};

/* Describes where an item can be worn.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *equipment_types[] = {
  "Used as light",
  "Worn on right finger",
  "Worn on left finger",
  "First worn around Neck",
  "Second worn around Neck",
  "Worn on body",
  "Worn on head",
  "Worn on legs",
  "Worn on feet",
  "Worn on hands",
  "Worn on arms",
  "Worn as shield",
  "Worn about body",
  "Worn around waist",
  "Worn around right wrist",
  "Worn around left wrist",
  "Wielded",
  "Held",
  "\n"
};

/** Describes the type of object.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *item_types[] = {
  "UNDEFINED",
  "LIGHT",
  "SCROLL",
  "WAND",
  "STAFF",
  "WEAPON",
  "FURNITURE",
  "FREE",
  "TREASURE",
  "ARMOR",
  "POTION",
  "WORN",
  "OTHER",
  "TRASH",
  "FREE2",
  "CONTAINER",
  "NOTE",
  "LIQ CONTAINER",
  "KEY",
  "FOOD",
  "MONEY",
  "PEN",
  "BOAT",
  "FOUNTAIN",
  "\n"
};

/** Describes the wear flags set on an item.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *wear_bits[] = {
  "TAKE",
  "FINGER",
  "NECK",
  "BODY",
  "HEAD",
  "LEGS",
  "FEET",
  "HANDS",
  "ARMS",
  "SHIELD",
  "ABOUT",
  "WAIST",
  "WRIST",
  "WIELD",
  "HOLD",
  "\n"
};

/** Describes the extra flags applied to an item.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *extra_bits[] = {
  "GLOW",
  "HUM",
  "NO_RENT",
  "NO_DONATE",
  "NO_INVIS",
  "INVISIBLE",
  "MAGIC",
  "NO_DROP",
  "BLESS",
  "ANTI_GOOD",
  "ANTI_EVIL",
  "ANTI_NEUTRAL",
  "ANTI_MAGE",
  "ANTI_CLERIC",
  "ANTI_THIEF",
  "ANTI_WARRIOR",
  "NO_SELL",
  "QUEST_ITEM",
  "\n"
};

/** Describes the apply types.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *apply_types[] = {
  "NONE",
  "STR",
  "DEX",
  "INT",
  "WIS",
  "CON",
  "CHA",
  "CLASS",
  "LEVEL",
  "AGE",
  "CHAR_WEIGHT",
  "CHAR_HEIGHT",
  "MAXMANA",
  "MAXHIT",
  "MAXMOVE",
  "GOLD",
  "EXP",
  "ARMOR",
  "HITROLL",
  "DAMROLL",
  "SAVING_PARA",
  "SAVING_ROD",
  "SAVING_PETRI",
  "SAVING_BREATH",
  "SAVING_SPELL",
  "\n"
};

/** Describes the closure mechanism for a container.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *container_bits[] = {
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};

/** Describes the liquid description.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *drinks[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local speciality",
  "slime mold juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt water",
  "clear water",
  "\n"
};

/** Describes a one word alias for each type of liquid.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *drinknames[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local",
  "juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt",
  "water",
  "\n"
};

/** Define the effect of liquids on hunger, thirst, and drunkenness, in that
 * order. See values.doc for more information.
 * @pre Must be in the same order as the defines. */
int drink_aff[][3] = {
  {0, 1, 10},
  {3, 2, 5},
  {5, 2, 5},
  {2, 2, 5},
  {1, 2, 5},
  {6, 1, 4},
  {0, 1, 8},
  {10, 0, 0},
  {3, 3, 3},
  {0, 4, -8},
  {0, 3, 6},
  {0, 1, 6},
  {0, 1, 6},
  {0, 2, -1},
  {0, 1, -2},
  {0, 0, 13}
};

/** Describes the color of the various drinks.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *color_liquid[] =
{
  "clear",
  "brown",
  "clear",
  "brown",
  "dark",
  "golden",
  "red",
  "green",
  "clear",
  "light green",
  "white",
  "brown",
  "black",
  "red",
  "clear",
  "crystal clear",
  "\n"
};

/** Used to describe the level of fullness of a drink container. Not used in
 * sprinttype() so no \\n. */
const char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};

/* Strength attribute affects.
 * The fields are hit mod, damage mod, weight carried mod, and weight wielded
 * mod. */
// d20 - str 10 = standard average human male strength.
// d20 - hit mod = standardized for str 10 average.
// d20 - dam mod = standardized for str 10 average.
// d20 - weight carried mod = standardized for str 10 average.
// d20 - weight carried mod formula = level * 15.
// d20 - weight wielded = standardized for str 10 average.
cpp_extern const struct str_app_type str_app[] = {
  {-10, -10, 0, 0},	/* str = 0 */
  {-9, -9, 15, 1},	/* str = 1 */
  {-8, -8, 30, 2},
  {-7, -7, 45, 3},
  {-6, -6, 60, 4},
  {-5, -5, 75, 5},	/* str = 5 */
  {-4, -4, 90, 6},
  {-3, -3, 105, 7},
  {-2, -2, 120, 8},
  {-1, -1, 135, 9},
  {0, 0, 150, 10},	/* str = 10 */
  {1, 1, 165, 11},
  {2, 2, 180, 12},
  {3, 3, 195, 13},
  {4, 4, 210, 14},
  {5, 5, 225, 15},	/* str = 15 */
  {6, 6, 240, 16},
  {7, 7, 255, 17},
  {8, 8, 270, 18},	/* str = 18 */
  {9, 9, 285, 19},
  {10, 10, 300, 20},	/* str = 20 */
  {11, 11, 315, 21},
  {12, 12, 330, 22},
  {13, 13, 345, 23},
  {14, 14, 360, 24},
  {15, 15, 375, 25},	/* str = 25 */
  {16, 16, 390, 26},
  {17, 17, 405, 27},
  {18, 18, 420, 28},
  {19, 19, 435, 29},
  {20, 20, 450, 30},	/* str = 30 */
  {21, 21, 465, 31},
  {22, 22, 480, 32},
  {23, 23, 495, 33},
  {24, 24, 510, 34} 	/* str = 34 */
};

/* Dexterity skill modifiers for thieves.
 * The fields are for pick pockets, pick locks, find traps, sneak, and hide. */
//d20 - dex 10 = standard average human male dexterity.
//d20 - pick pockets = standardized for dex 10 average.
//d20 - pick locks = standardized for dex 10 average.
//d20 - find traps = standardized for dex 10 average.
//d20 - sneak = standardized for dex 10 average.
//d20 - hide = standardized for dex 10 average.
cpp_extern const struct dex_skill_type dex_app_skill[] = {
  {-55, -55, -55, -55, -55},	/* dex = 0 */
  {-50, -50, -50, -50, -50},	/* dex = 1 */
  {-45, -45, -45, -45, -45},
  {-40, -40, -40, -40, -40},
  {-35, -35, -35, -35, -35},
  {-30, -30, -30, -30, -30},	/* dex = 5 */
  {-25, -25, -25, -25, -25},
  {-20, -20, -20, -20, -20},
  {-15, -15, -15, -15, -15},
  {-10, -10, -10, -10, -10},
  {0, 0, 0, 0, 0}, 	/* dex = 10 */
  {1, 1, 1, 1, 1},
  {2, 2, 2, 2, 2},
  {3, 3, 3, 3, 3},
  {4, 4, 4, 4, 4},
  {5, 5, 5, 5, 5}, 	/* dex = 15 */
  {6, 6, 6, 6, 6},
  {7, 7, 7, 7, 7},
  {8, 8, 8, 8, 8},		/* dex = 18 */
  {9, 9, 9, 9, 9},
  {10, 10, 10, 10, 10},		/* dex = 20 */
  {11, 11, 11, 11, 11},
  {12, 12, 12, 12, 12},
  {13, 13, 13, 13, 13},
  {14, 14, 14, 14, 14},
  {15, 15, 15, 15, 15},		/* dex = 25 */
  {16, 16, 16, 16, 16},
  {17, 17, 17, 17, 17},
  {18, 18, 18, 18, 18},
  {19, 19, 19, 19, 19},
  {20, 20, 20, 20, 20},		/* dex = 30 */
  {21, 21, 21, 21, 21},
  {22, 22, 22, 22, 22},
  {23, 23, 23, 23, 23},
  {24, 24, 24, 24, 24},		/* dex = 34 */
};

/* Dexterity attribute affects.
 * The fields are reaction, missile attacks, and defensive (armor class). */
// d20 - dex 10 = standard average human male dexterity.
// d20 - reaction = standardized for dex 10 average.
// d20 - missile attacks = standardized for dex 10 average.
// d20 - defensive (armor class) = standardized for dex 10 average.
cpp_extern const struct dex_app_type dex_app[] = {
  {-10, -10, 10},	/* dex = 0 */
  {-9, -9, 9},		/* dex = 1 */
  {-8, -8, 8},
  {-7, -7, 7},
  {-6, -6, 6},
  {-5, -5, 5},		/* dex = 5*/
  {-4 -4, 4},
  {3, -3, 3},
  {2, -2, 2},
  {1, -1, 1},
  {0, 0, 0},			/* dex = 10 */
  {1, 1, -1},
  {2, 2, -2},
  {3, 3, -3},
  {4, 4, -4},
  {5, 5, -5},		 /* dex = 15 */
  {6, 6, -6},
  {7, 7, -7},
  {8, 8, -8},		 /* dex = 18 */
  {9, 9, -9},
  {10, 10, -10},		/* dex = 20 */
  {11, 11, -11},
  {12, 12, -12},
  {13, 13, -13},
  {14, 14, -14},
  {15, 15, -15},		/* dex = 25 */
  {16, 16, -16},
  {17, 17, -17},
  {18, 18, -18},
  {19, 19, -29},
  {20, 20, -20},		/* dex = 30 */
  {21, 21, -21},
  {22, 22, -22},
  {23, 23, -23},
  {24, 24, -24}	  	/* dex = 34 */
};

/* Constitution attribute affects.
 * The field referenced is for hitpoint bonus. */
// d20 - con 10 = standard average human male constitution.
cpp_extern const struct con_app_type con_app[] = {
  {-10},	/* con = 0 */
  {-9},		/* con = 1 */
  {-8},
  {-7},
  {-6},
  {-5},		/* con = 5 */
  {-4},
  {-3},
  {-2},
  {-1},
  {0},		/* con = 10 */
  {1},
  {2},
  {3},
  {4},
  {5},		/* con = 15 */
  {6},
  {7},
  {8},		/* con = 18 */
  {9},
  {10},		/* con = 20 */
  {11},
  {12},
  {13},
  {14},
  {15},		/* con = 25 */
  {16},
  {17},
  {18},
  {19},
  {20},		/* con = 30 */
  {21},
  {22},
  {23},
  {24}		/* con = 34 */
};

/* Intelligence attribute affects.
 * The field shows how much practicing affects a skill/spell. */
// d20 - int 10 = standard average human male intelligence.
cpp_extern const struct int_app_type int_app[] = {
  {5},		/* int = 0 */
  {6},		/* int = 1 */
  {7},
  {8},
  {9},
  {10},		/* int = 5 */
  {12},
  {14},
  {16},
  {18},
  {20},		/* int = 10 */
  {22},
  {24},
  {26},
  {28},
  {30},		/* int = 15 */
  {35},
  {40},
  {45},		/* int = 18 */
  {50},
  {55},		/* int = 20 */
  {56},
  {57},
  {58},
  {59},
  {60},		/* int = 25 */
  {61},
  {62},
  {63},
  {64},
  {65},		/* int = 30 */
  {66},
  {67},
  {68},
  {69}		/* int = 34 */
};

/* Wisdom attribute affects.
 * The field represents how many extra practice points are gained per level. */
// d20 - wis 10 = standard average human male wisdom.
cpp_extern const struct wis_app_type wis_app[] = {
  {0},	/* wis = 0 */
  {0},  /* wis = 1 */
  {0},
  {0},
  {0},
  {0},  /* wis = 5 */
  {0},
  {0},
  {0},
  {0},
  {1},  /* wis = 10 */
  {1},
  {2},
  {2},
  {3},
  {3},  /* wis = 15 */
  {4},
  {4},
  {5},	/* wis = 18 */
  {5},
  {6},  /* wis = 20 */
  {6},
  {7},
  {7},
  {8},
  {8},  /* wis = 25 */
  {8},
  {9},
  {9},
  {10},
  {10},  /* wis = 30 */
  {11},
  {11},
  {12},
  {12}   /* wis = 34 */
};

/** Define a set of opposite directions from the cardinal directions. */
int rev_dir[] =
{
  SOUTH,
  WEST,
  NORTH,
  EAST,
  DOWN,
  UP,
  SOUTHEAST,
  SOUTHWEST,
  NORTHWEST,
  NORTHEAST
};

/** How much movement is lost moving through a particular sector type. */
int movement_loss[] =
{
  1,	/* Inside     */
  1,	/* City       */
  2,	/* Field      */
  3,	/* Forest     */
  4,	/* Hills      */
  6,	/* Mountains  */
  4,	/* Swimming   */
  1,	/* Unswimable */
  1,	/* Flying     */
  5   /* Underwater */
};

/** The names of the days of the mud week. Not used in sprinttype(). */
const char *weekdays[] = {
  "the Day of the Moon",
  "the Day of the Bull",
  "the Day of the Deception",
  "the Day of Thunder",
  "the Day of Freedom",
  "the Day of the Great Gods",
  "the Day of the Sun"
};

/** The names of the mud months. Not used in sprinttype(). */
const char *month_name[] = {
  "Month of Winter",		/* 0 */
  "Month of the Winter Wolf",
  "Month of the Frost Giant",
  "Month of the Old Forces",
  "Month of the Grand Struggle",
  "Month of the Spring",
  "Month of Nature",
  "Month of Futility",
  "Month of the Dragon",
  "Month of the Sun",
  "Month of the Heat",
  "Month of the Battle",
  "Month of the Dark Shades",
  "Month of the Shadows",
  "Month of the Long Shadows",
  "Month of the Ancient Darkness",
  "Month of the Great Evil"
};

/** Names for mob trigger types. */
const char *trig_types[] = {
  "Global",
  "Random",
  "Command",
  "Speech",
  "Act",
  "Death",
  "Greet",
  "Greet-All",
  "Entry",
  "Receive",
  "Fight",
  "HitPrcnt",
  "Bribe",
  "Load",
  "Memory",
  "Cast",
  "Leave",
  "Door",
  "UNUSED",
  "Time",
  "\n"
};

/** Names for object trigger types. */
const char *otrig_types[] = {
  "Global",
  "Random",
  "Command",
  "UNUSED1",
  "UNUSED2",
  "Timer",
  "Get",
  "Drop",
  "Give",
  "Wear",
  "UNUSED3",
  "Remove",
  "UNUSED4",
  "Load",
  "UNUSED5",
  "Cast",
  "Leave",
  "UNUSED6",
  "Consume",
  "Time",
  "\n"
};

/** Names for world (room) trigger types. */
const char *wtrig_types[] = {
  "Global",
  "Random",
  "Command",
  "Speech",
  "UNUSED1",
  "Zone Reset",
  "Enter",
  "Drop",
  "UNUSED2",
  "UNUSED3",
  "UNUSED4",
  "UNUSED5",
  "UNUSED6",
  "UNUSED7",
  "UNUSED8",
  "Cast",
  "Leave",
  "Door",
  "Login",
  "Time",
  "\n"
};

/** The names of the different channels that history is stored for.
 * @todo Only referenced by do_history at the moment. Should be moved local
 * to that function. */
const char *history_types[] = {
  "all",
  "say",
  "gossip",
  "wiznet",
  "tell",
  "shout",
  "grats",
  "holler",
  "auction",
  "\n"
};

/** Flag names for Ideas, Bugs and Typos (defined in ibt.h) */
const char *ibt_bits[] = {
  "Resolved",
  "Important",
  "InProgress",
  "\n"
};
/* --- End of constants arrays. --- */

/* Various arrays we count so we can check the world files.  These
 * must be at the bottom of the file so they're pre-declared. */
  /** Number of defined room bit descriptions. */
  size_t	room_bits_count = sizeof(room_bits) / sizeof(room_bits[0]) - 1,
  /** Number of defined action bit descriptions. */
	action_bits_count = sizeof(action_bits) / sizeof(action_bits[0]) - 1,
	/** Number of defined affected bit descriptions. */
	affected_bits_count = sizeof(affected_bits) / sizeof(affected_bits[0]) - 1,
	/** Number of defined extra bit descriptions. */
	extra_bits_count = sizeof(extra_bits) / sizeof(extra_bits[0]) - 1,
	/** Number of defined wear bit descriptions. */
	wear_bits_count = sizeof(wear_bits) / sizeof(wear_bits[0]) - 1;

