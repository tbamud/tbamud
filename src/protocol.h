/******************************************************************************
 Protocol snippet by KaVir.  Released into the Public Domain in February 2011.
 ******************************************************************************/

#ifndef PROTOCOL_H
#define PROTOCOL_H

/******************************************************************************
 Set your MUD_NAME, and change descriptor_t if necessary.
 ******************************************************************************/

#define MUD_NAME "tbaMUD"

typedef struct descriptor_data descriptor_t;

/******************************************************************************
 If your mud supports MCCP (compression), uncomment the next line.
 ******************************************************************************/

/*
#define USING_MCCP
*/

/******************************************************************************
 If your offer a Mudlet GUI for autoinstallation, put the path/filename here.
 ******************************************************************************/

/*
#define MUDLET_PACKAGE "1\nhttp://blah.org/download/MY_GUI.mpackage"
*/

/******************************************************************************
 Symbolic constants.
 ******************************************************************************/

#define SNIPPET_VERSION                6 /* Helpful for debugging */

#define MAX_PROTOCOL_BUFFER            MAX_RAW_INPUT_LENGTH
#define MAX_VARIABLE_LENGTH            4096
#define MAX_OUTPUT_BUFFER              LARGE_BUFSIZE
#define MAX_MSSP_BUFFER                4096

#define SEND                           1
#define ACCEPTED                       2
#define REJECTED                       3

#define TELOPT_CHARSET                 42
#define TELOPT_MSDP                    69
#define TELOPT_MSSP                    70
#define TELOPT_MCCP                    86 /* This is MCCP version 2 */
#define TELOPT_MSP                     90
#define TELOPT_MXP                     91
#define TELOPT_ATCP                    200

#define MSDP_VAR                       1
#define MSDP_VAL                       2
#define MSDP_TABLE_OPEN                3
#define MSDP_TABLE_CLOSE               4
#define MSDP_ARRAY_OPEN                5
#define MSDP_ARRAY_CLOSE               6
#define MAX_MSDP_SIZE                  100

#define MSSP_VAR                       1
#define MSSP_VAL                       2

#define UNICODE_MALE                   9794
#define UNICODE_FEMALE                 9792
#define UNICODE_NEUTER                 9791

/******************************************************************************
 Types.
 ******************************************************************************/

typedef enum
{
   false, 
   true
} bool_t;

typedef enum
{
   eUNKNOWN, 
   eNO, 
   eSOMETIMES, 
   eYES
} support_t;

typedef enum
{
   eMSDP_NONE = -1,            /* This must always be first. */

   /* General */
   eMSDP_CHARACTER_NAME, 
   eMSDP_SERVER_ID, 
   eMSDP_SERVER_TIME, 
   eMSDP_SNIPPET_VERSION, 

   /* Character */
   eMSDP_AFFECTS, 
   eMSDP_ALIGNMENT, 
   eMSDP_EXPERIENCE, 
   eMSDP_EXPERIENCE_MAX, 
   eMSDP_EXPERIENCE_TNL, 
   eMSDP_HEALTH, 
   eMSDP_HEALTH_MAX, 
   eMSDP_LEVEL, 
   eMSDP_RACE, 
   eMSDP_CLASS, 
   eMSDP_MANA, 
   eMSDP_MANA_MAX, 
   eMSDP_WIMPY, 
   eMSDP_PRACTICE, 
   eMSDP_MONEY, 
   eMSDP_MOVEMENT, 
   eMSDP_MOVEMENT_MAX, 
   eMSDP_HITROLL, 
   eMSDP_DAMROLL, 
   eMSDP_AC, 
   eMSDP_STR, 
   eMSDP_INT, 
   eMSDP_WIS, 
   eMSDP_DEX, 
   eMSDP_CON, 
   eMSDP_STR_PERM, 
   eMSDP_INT_PERM, 
   eMSDP_WIS_PERM, 
   eMSDP_DEX_PERM, 
   eMSDP_CON_PERM, 

   /* Combat */
   eMSDP_OPPONENT_HEALTH, 
   eMSDP_OPPONENT_HEALTH_MAX, 
   eMSDP_OPPONENT_LEVEL, 
   eMSDP_OPPONENT_NAME, 

   /* World */
   eMSDP_AREA_NAME, 
   eMSDP_ROOM_EXITS, 
   eMSDP_ROOM_NAME, 
   eMSDP_ROOM_VNUM, 
   eMSDP_WORLD_TIME, 

   /* Configuration */
   eMSDP_CLIENT_ID, 
   eMSDP_CLIENT_VERSION, 
   eMSDP_PLUGIN_ID, 
   eMSDP_ANSI_COLORS, 
   eMSDP_XTERM_256_COLORS, 
   eMSDP_UTF_8, 
   eMSDP_SOUND, 
   eMSDP_MXP, 

   /* GUI variables */
   eMSDP_BUTTON_1, 
   eMSDP_BUTTON_2, 
   eMSDP_BUTTON_3, 
   eMSDP_BUTTON_4, 
   eMSDP_BUTTON_5, 
   eMSDP_GAUGE_1, 
   eMSDP_GAUGE_2, 
   eMSDP_GAUGE_3, 
   eMSDP_GAUGE_4, 
   eMSDP_GAUGE_5, 

   eMSDP_MAX                   /* This must always be last */
} variable_t;

typedef struct
{
   variable_t   Variable;      /* The enum type of this variable */
   const char  *pName;         /* The string name of this variable */
   bool_t       bString;       /* Is this variable a string or a number? */
   bool_t       bConfigurable; /* Can it be configured by the client? */
   bool_t       bWriteOnce;    /* Can only set this variable once */
   bool_t       bGUI;          /* It's a special GUI configuration variable */
   int          Min;           /* The minimum valid value or string length */
   int          Max;           /* The maximum valid value or string length */
   int          Default;       /* The default value for a number */
   const char  *pDefault;      /* The default value for a string */
} variable_name_t;

typedef struct
{
   bool_t       bReport;       /* Is this variable being reported? */
   bool_t       bDirty;        /* Does this variable need to be sent again? */
   int          ValueInt;      /* The numeric value of the variable */
   char        *pValueString;  /* The string value of the variable */
} MSDP_t;

typedef struct
{
   const char  *pName;         /* The name of the MSSP variable */
   const char  *pValue;        /* The value of the MSSP variable */
   const char  *(*pFunction)();/* Optional function to return the value */
} MSSP_t;

typedef struct
{
   int       WriteOOB;         /* Used internally to indicate OOB data */
   bool_t    bIACMode;         /* Current mode - deals with broken packets */
   bool_t    bNegotiated;      /* Indicates client successfully negotiated */
   bool_t    bBlockMXP;        /* Used internally based on MXP version */
   bool_t    bTTYPE;           /* The client supports TTYPE */
   bool_t    bNAWS;            /* The client supports NAWS */
   bool_t    bCHARSET;         /* The client supports CHARSET */
   bool_t    bMSDP;            /* The client supports MSDP */
   bool_t    bATCP;            /* The client supports ATCP */
   bool_t    bMSP;             /* The client supports MSP */
   bool_t    bMXP;             /* The client supports MXP */
   bool_t    bMCCP;            /* The client supports MCCP */
   support_t b256Support;      /* The client supports XTerm 256 colors */
   int       ScreenWidth;      /* The client's screen width */
   int       ScreenHeight;     /* The client's screen height */
   char     *pMXPVersion;      /* The version of MXP supported */
   char     *pLastTTYPE;       /* Used for the cyclic TTYPE check */
   MSDP_t  **pVariables;       /* The MSDP variables */
} protocol_t;

/******************************************************************************
 Protocol functions.
 ******************************************************************************/

/* Function: ProtocolCreate
 *
 * Creates, initialises and returns a structure containing protocol data for a 
 * single user.  This should be called when the descriptor is initialised.
 */
protocol_t *ProtocolCreate( void );

/* Function: ProtocolDestroy
 *
 * Frees the memory allocated by the specified structure.  This should be 
 * called just before a descriptor is freed.
 */
void ProtocolDestroy( protocol_t *apProtocol );

/* Function: ProtocolNegotiate
 *
 * Negatiates with the client to see which protocols the user supports, and 
 * stores the results in the user's protocol structure.  Call this when you 
 * wish to perform negotiation (but only call it once).  It is usually called 
 * either immediately after the user has connected, or just after they have 
 * entered the game.
 */
void ProtocolNegotiate( descriptor_t *apDescriptor );

/* Function: ProtocolInput
 *
 * Extracts any negotiation sequences from the input buffer, and passes back 
 * whatever is left for the mud to parse normally.  Call this after data has 
 * been read into the input buffer, before it is used for anything else.
 */
 
/* MUD Primary Colours */
extern const char * RGBone;
extern const char * RGBtwo;
extern const char * RGBthree; 
 
ssize_t ProtocolInput( descriptor_t *apDescriptor, char *apData, int aSize, char *apOut );

/* Function: ProtocolOutput
 *
 * This function takes a string, applies colour codes to it, and returns the 
 * result.  It should be called just before writing to the output buffer.
 * 
 * The special character used to indicate the start of a colour sequence is 
 * '\t' (i.e., a tab, or ASCII character 9).  This makes it easy to include 
 * in help files (as you can literally press the tab key) as well as strings 
 * (where you can use \t instead).  However players can't send tabs (on most 
 * muds at least), so this stops them from sending colour codes to each other.
 * 
 * The predefined colours are:
 * 
 *   n: no colour (switches colour off)
 *   r: dark red                        R: bright red
 *   g: dark green                      G: bright green
 *   b: dark blue                       B: bright blue
 *   y: dark yellow                     Y: bright yellow
 *   m: dark magenta                    M: bright magenta
 *   c: dark cyan                       C: bright cyan
 *   w: dark white                      W: bright white
 *   o: dark orange                     O: bright orange
 * 
 * So for example "This is \tOorange\tn." will colour the word "orange".  You 
 * can add more colours yourself just by updating the switch statement.
 * 
 * It's also possible to explicitly specify an RGB value, by including the four 
 * character colour sequence (as used by ColourRGB) within square brackets, eg:
 * 
 *    This is a \t[F010]very dark green foreground\tn.
 *    
 * The square brackets can also be used to send unicode characters, like this:
 * 
 *    Boat: \t[U9973/B]
 *    Rook: \t[U9814/C]
 * 
 * For example you might use 'B' to represent a boat on your ASCII map, or a 'C' 
 * to represent a castle - but players with UTF-8 support would actually see the 
 * appropriate unicode characters for a boat or a rook (the chess playing piece).
 * 
 * The exact syntax is '\t' (tab), '[', 'U' (indicating unicode), then the decimal 
 * number of the unicode character (see http://www.unicode.org/charts), then '/' 
 * followed by the ASCII character/s that should be used if the client doesn't 
 * support UTF-8.  The ASCII sequence can be up to 7 characters in length, but in 
 * most cases you'll only want it to be one or two characters (so that it has the 
 * same alignment as the unicode character).
 * 
 * Finally, this function also allows you to embed MXP tags.  The easiest and 
 * safest way to do this is via the ( and ) bracket options:
 *    
 *    From here, you can walk \t(north\t).
 * 
 * However it's also possible to include more explicit MSP tags, like this:
 * 
 *    The baker offers to sell you a \t<send href="buy pie">pie\t</send>.
 * 
 * Note that the MXP tags will automatically be removed if the user doesn't 
 * support MXP, but it's very important you remember to close the tags.
 */
const char *ProtocolOutput( descriptor_t *apDescriptor, const char *apData, int *apLength );

/******************************************************************************
 Copyover save/load functions.
 ******************************************************************************/

/* Function: CopyoverGet
 *
 * Returns the protocol values stored as a short string.  If your mud uses 
 * copyover, you should call this for each player and insert it after their 
 * name in the temporary text file.
 */
const char *CopyoverGet( descriptor_t *apDescriptor );

/* Function: CopyoverSet
 *
 * Call this function for each player after a copyover, passing in the string 
 * you added to the temporary text file.  This will restore their protocol 
 * settings, and automatically renegotiate MSDP/ATCP.
 * 
 * Note that the client doesn't recognise a copyover, and therefore refuses to 
 * renegotiate certain telnet options (to avoid loops), so they really need to 
 * be saved.  However MSDP/ATCP is handled through scripts, and we don't want 
 * to have to save all of the REPORT variables, so it's easier to renegotiate.
 * 
 * Client name and version are not saved.  It is recommended you save these in 
 * the player file, as then you can grep to collect client usage stats.
 */
void CopyoverSet( descriptor_t *apDescriptor, const char *apData );

/******************************************************************************
 MSDP functions.
 ******************************************************************************/

/* Function: MSDPUpdate
 *
 * Call this regularly (I'd suggest at least once per second) to flush every 
 * dirty MSDP variable that has been requested by the client via REPORT.  This 
 * will automatically use ATCP instead if MSDP is not supported by the client.
 */
void MSDPUpdate( descriptor_t *apDescriptor );

/* Function: MSDPFlush
 *
 * Works like MSDPUpdate(), except only flushes a specific variable.  The 
 * variable will only actually be sent if it's both reported and dirty.
 * 
 * Call this function after setting a variable if you want it to be reported 
 * immediately, instead of on the next update.
 */
void MSDPFlush( descriptor_t *apDescriptor, variable_t aMSDP );

/* Function: MSDPSend
 *
 * Send the specified MSDP variable to the player.  You shouldn't ever really 
 * need to do this manually, except perhaps when debugging something.  This 
 * will automatically use ATCP instead if MSDP is not supported by the client.
 */
void MSDPSend( descriptor_t *apDescriptor, variable_t aMSDP );

/* Function: MSDPSendPair
 *
 * Send the specified strings to the user as an MSDP variable/value pair.  This 
 * will automatically use ATCP instead if MSDP is not supported by the client.
 */
void MSDPSendPair( descriptor_t *apDescriptor, const char *apVariable, const char *apValue );

/* Function: MSDPSendList
 *
 * Works like MSDPSendPair, but the value is sent as an MSDP array.
 *
 * apValue should be a list of values separated by spaces.
 */
void MSDPSendList( descriptor_t *apDescriptor, const char *apVariable, const char *apValue );

/* Function: MSDPSetNumber
 *
 * Call this whenever an MSDP integer variable has changed.  The easiest 
 * approach is to send every MSDP variable within an update function (and 
 * this is what the snippet does by default), but if the variable is only 
 * set in one place you can just move its MDSPSend() call to there.
 * 
 * You can also this function for bools, chars, enums, short ints, etc.
 */
void MSDPSetNumber( descriptor_t *apDescriptor, variable_t aMSDP, int aValue );

/* Function: MSDPSetString
 *
 * Call this whenever an MSDP string variable has changed.  The easiest 
 * approach is to send every MSDP variable within an update function (and 
 * this is what the snippet does by default), but if the variable is only 
 * set in one place you can just move its MDSPSend() call to there.
 */
void MSDPSetString( descriptor_t *apDescriptor, variable_t aMSDP, const char *apValue );

/* Function: MSDPSetTable
 *
 * Works like MSDPSetString, but the value is sent as an MSDP table.
 *
 * You must add the MSDP_VAR and MSDP_VAL manually, for example:
 *
 * sprintf( Buffer, "%c%s%c%s", (char)MSDP_VAR, Name, (char)MSDP_VAL, Value );
 * MSDPSetTable( d, eMSDP_TEST, Buffer );
 */
void MSDPSetTable( descriptor_t *apDescriptor, variable_t aMSDP, const char *apValue );

/* Function: MSDPSetArray
 *
 * Works like MSDPSetString, but the value is sent as an MSDP array.
 *
 * You must add the MSDP_VAR before each element manually, for example:
 *
 * sprintf( Buffer, "%c%s%c%s", (char)MSDP_VAL, Val1, (char)MSDP_VAL, Val2 );
 * MSDPSetArray( d, eMSDP_TEST, Buffer );
 */
void MSDPSetArray( descriptor_t *apDescriptor, variable_t aMSDP, const char *apValue );

/******************************************************************************
 MSSP functions.
 ******************************************************************************/

/* Function: MSSPSetPlayers
 *
 * Stores the current number of players.  The first time it's called, it also 
 * stores the uptime.
 */
void MSSPSetPlayers( int aPlayers );

/******************************************************************************
 MXP functions.
 ******************************************************************************/

/* Function: MXPCreateTag
 *
 * Puts the specified tag into a secure line, if MXP is supported.  If the user 
 * doesn't support MXP they will see the string unchanged, meaning they will 
 * see the <send> tags or whatever.  You should therefore check for support and 
 * provide a different sequence for other users, or better yet just embed MXP 
 * tags for the ProtocolOutput() function.
 */
const char *MXPCreateTag( descriptor_t *apDescriptor, const char *apTag );

/* Function: MXPSendTag
 *
 * This works like MXPCreateTag, but instead of returning the string it sends 
 * it directly to the user.  This is mainly useful for the <VERSION> tag.
 */
void MXPSendTag( descriptor_t *apDescriptor, const char *apTag );

/******************************************************************************
 Sound functions.
 ******************************************************************************/

/* Function: SoundSend
 *
 * Sends the specified sound trigger to the player, using MSDP or ATCP if 
 * supported, MSP if not.  The trigger string itself is a relative path and 
 * filename, eg: SoundSend( pDesc, "monster/growl.wav" );
 */
void SoundSend( descriptor_t *apDescriptor, const char *apTrigger );

/******************************************************************************
 Colour functions.
 ******************************************************************************/

/* Function: ColourRGB
 *
 * Returns a colour as an escape code, based on the RGB value provided.  The 
 * string must be four characters, where the first is either 'f' for foreground 
 * or 'b' for background (case insensitive), and the other three characters are 
 * numeric digits in the range 0 to 5, representing red, green and blue.
 * 
 * For example "F500" returns a red foreground, "B530" an orange background, 
 * and so on.  An invalid colour will clear whatever you've set previously.
 * 
 * If the user doesn't support XTerm 256 colours, this function will return the 
 * best-fit ANSI colour instead.
 * 
 * If you wish to embed colours in strings, use ProtocolOutput().
 */
const char *ColourRGB( descriptor_t *apDescriptor, const char *apRGB );

/******************************************************************************
 Unicode (UTF-8 conversion) functions.
 ******************************************************************************/

/* Function: UnicodeGet
 *
 * Returns the UTF-8 sequence for the specified unicode value.
 */
char *UnicodeGet( int aValue );

/* Function: UnicodeAdd
 *
 * Adds the UTF-8 sequence for the specified unicode value onto the end of the 
 * string, without adding a NUL character at the end.
 */
void UnicodeAdd( char **apString, int aValue );

#endif /* PROTOCOL_H */
