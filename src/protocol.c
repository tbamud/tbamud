/******************************************************************************
 Protocol snippet by KaVir.  Released into the Public Domain in February 2011.
 ******************************************************************************/

/******************************************************************************
 Header files.
 ******************************************************************************/

#include "conf.h"
#ifndef CIRCLE_WINDOWS
#include <arpa/telnet.h>
#endif
#include <sys/types.h>
#include "protocol.h"

/******************************************************************************
 The following section is for Diku/Merc derivatives.  Replace as needed.
 ******************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "act.h"
#include "modify.h"

/* Globals */
const char * RGBone = "F022";
const char * RGBtwo = "F055";
const char * RGBthree = "F555";

static void Write( descriptor_t *apDescriptor, const char *apData )
{
   if ( apDescriptor != NULL)
   {
      if ( apDescriptor->pProtocol->WriteOOB > 0 || *(apDescriptor->output) == '\0' )
      {
         apDescriptor->pProtocol->WriteOOB = 2;
      }
   }
   write_to_output( apDescriptor, apData, 0 );
}

static void ReportBug( const char *apText )
{
   log( "%s", apText);
}

static void InfoMessage( descriptor_t *apDescriptor, const char *apData )
{
   Write( apDescriptor, "\t[F210][\toINFO\t[F210]]\tn " );
   Write( apDescriptor, apData );
}

static void CompressStart( descriptor_t *apDescriptor )
{
   /* If your mud uses MCCP (Mud Client Compression Protocol), you need to 
    * call whatever function normally starts compression from here - the 
    * ReportBug() call should then be deleted.
    * 
    * Otherwise you can just ignore this function.
    */
   ReportBug( "CompressStart() in protocol.c is being called, but it doesn't do anything!\n" );
}

static void CompressEnd( descriptor_t *apDescriptor )
{
   /* If your mud uses MCCP (Mud Client Compression Protocol), you need to 
    * call whatever function normally starts compression from here - the 
    * ReportBug() call should then be deleted.
    * 
    * Otherwise you can just ignore this function.
    */
   ReportBug( "CompressEnd() in protocol.c is being called, but it doesn't do anything!\n" );
}

/******************************************************************************
 MSDP file-scope variables.
 ******************************************************************************/

/* These are for the GUI_VARIABLES, my unofficial extension of MSDP.  They're 
 * intended for clients that wish to offer a generic GUI - not as nice as a 
 * custom GUI, admittedly, but still better than a plain terminal window.
 *
 * These are per-player so that they can be customised for different characters 
 * (eg changing 'mana' to 'blood' for vampires).  You could even allow players 
 * to customise the buttons and gauges themselves if you wish.
 */
static const char s_Button1[] = "\005\002Help\002help\006";
static const char s_Button2[] = "\005\002Look\002look\006";
static const char s_Button3[] = "\005\002Score\002help\006";
static const char s_Button4[] = "\005\002Equipment\002equipment\006";
static const char s_Button5[] = "\005\002Inventory\002inventory\006";

static const char s_Gauge1[]  = "\005\002Health\002red\002HEALTH\002HEALTH_MAX\006";
static const char s_Gauge2[]  = "\005\002Mana\002blue\002MANA\002MANA_MAX\006";
static const char s_Gauge3[]  = "\005\002Movement\002green\002MOVEMENT\002MOVEMENT_MAX\006";
static const char s_Gauge4[]  = "\005\002Exp TNL\002yellow\002EXPERIENCE\002EXPERIENCE_MAX\006";
static const char s_Gauge5[]  = "\005\002Opponent\002darkred\002OPPONENT_HEALTH\002OPPONENT_HEALTH_MAX\006";

/******************************************************************************
 MSDP variable table.
 ******************************************************************************/

/* Macros for readability, but you can remove them if you don't like them */
#define NUMBER_READ_ONLY           false, false, false, false, -1, -1,  0, NULL
#define NUMBER_READ_ONLY_SET_TO(x) false, false, false, false, -1, -1,  x, NULL
#define STRING_READ_ONLY           true,  false, false, false, -1, -1,  0, NULL
#define BOOLEAN_SET_TO(x)          false, true,  false, false,  0,  1,  x, NULL
#define STRING_WITH_LENGTH_OF(x,y) true,  true,  false, false,  x,  y,  0, NULL
#define STRING_WRITE_ONCE(x,y)     true,  true,  true,  false, -1, -1,  0, NULL
#define STRING_GUI(x)              true,  false, false, true,  -1, -1,  0, x

static variable_name_t VariableNameTable[eMSDP_MAX+1] = 
{
   /* General */
   { eMSDP_CHARACTER_NAME,   "CHARACTER_NAME",   STRING_READ_ONLY }, 
   { eMSDP_SERVER_ID,        "SERVER_ID",        STRING_READ_ONLY }, 
   { eMSDP_SERVER_TIME,      "SERVER_TIME",      NUMBER_READ_ONLY }, 
   { eMSDP_SNIPPET_VERSION,  "SNIPPET_VERSION",  NUMBER_READ_ONLY_SET_TO(SNIPPET_VERSION) }, 

   /* Character */
   { eMSDP_AFFECTS,          "AFFECTS",          STRING_READ_ONLY }, 
   { eMSDP_ALIGNMENT,        "ALIGNMENT",        NUMBER_READ_ONLY }, 
   { eMSDP_EXPERIENCE,       "EXPERIENCE",       NUMBER_READ_ONLY }, 
   { eMSDP_EXPERIENCE_MAX,   "EXPERIENCE_MAX",   NUMBER_READ_ONLY }, 
   { eMSDP_EXPERIENCE_TNL,   "EXPERIENCE_TNL",   NUMBER_READ_ONLY }, 
   { eMSDP_HEALTH,           "HEALTH",           NUMBER_READ_ONLY }, 
   { eMSDP_HEALTH_MAX,       "HEALTH_MAX",       NUMBER_READ_ONLY }, 
   { eMSDP_LEVEL,            "LEVEL",            NUMBER_READ_ONLY }, 
   { eMSDP_RACE,             "RACE",             STRING_READ_ONLY }, 
   { eMSDP_CLASS,            "CLASS",            STRING_READ_ONLY }, 
   { eMSDP_MANA,             "MANA",             NUMBER_READ_ONLY }, 
   { eMSDP_MANA_MAX,         "MANA_MAX",         NUMBER_READ_ONLY }, 
   { eMSDP_WIMPY,            "WIMPY",            NUMBER_READ_ONLY }, 
   { eMSDP_PRACTICE,         "PRACTICE",         NUMBER_READ_ONLY }, 
   { eMSDP_MONEY,            "MONEY",            NUMBER_READ_ONLY }, 
   { eMSDP_MOVEMENT,         "MOVEMENT",         NUMBER_READ_ONLY }, 
   { eMSDP_MOVEMENT_MAX,     "MOVEMENT_MAX",     NUMBER_READ_ONLY }, 
   { eMSDP_HITROLL,          "HITROLL",          NUMBER_READ_ONLY }, 
   { eMSDP_DAMROLL,          "DAMROLL",          NUMBER_READ_ONLY }, 
   { eMSDP_AC,               "AC",               NUMBER_READ_ONLY }, 
   { eMSDP_STR,              "STR",              NUMBER_READ_ONLY }, 
   { eMSDP_INT,              "INT",              NUMBER_READ_ONLY }, 
   { eMSDP_WIS,              "WIS",              NUMBER_READ_ONLY }, 
   { eMSDP_DEX,              "DEX",              NUMBER_READ_ONLY }, 
   { eMSDP_CON,              "CON",              NUMBER_READ_ONLY }, 
   { eMSDP_STR_PERM,         "STR_PERM",         NUMBER_READ_ONLY }, 
   { eMSDP_INT_PERM,         "INT_PERM",         NUMBER_READ_ONLY }, 
   { eMSDP_WIS_PERM,         "WIS_PERM",         NUMBER_READ_ONLY }, 
   { eMSDP_DEX_PERM,         "DEX_PERM",         NUMBER_READ_ONLY }, 
   { eMSDP_CON_PERM,         "CON_PERM",         NUMBER_READ_ONLY }, 

   /* Combat */
   { eMSDP_OPPONENT_HEALTH,  "OPPONENT_HEALTH",  NUMBER_READ_ONLY }, 
   { eMSDP_OPPONENT_HEALTH_MAX,"OPPONENT_HEALTH_MAX",NUMBER_READ_ONLY }, 
   { eMSDP_OPPONENT_LEVEL,   "OPPONENT_LEVEL",   NUMBER_READ_ONLY }, 
   { eMSDP_OPPONENT_NAME,    "OPPONENT_NAME",    STRING_READ_ONLY }, 

   /* World */
   { eMSDP_AREA_NAME,        "AREA_NAME",        STRING_READ_ONLY }, 
   { eMSDP_ROOM_EXITS,       "ROOM_EXITS",       STRING_READ_ONLY }, 
   { eMSDP_ROOM_NAME,        "ROOM_NAME",        STRING_READ_ONLY }, 
   { eMSDP_ROOM_VNUM,        "ROOM_VNUM",        NUMBER_READ_ONLY }, 
   { eMSDP_WORLD_TIME,       "WORLD_TIME",       NUMBER_READ_ONLY }, 

   /* Configurable variables */
   { eMSDP_CLIENT_ID,        "CLIENT_ID",        STRING_WRITE_ONCE(1,40) }, 
   { eMSDP_CLIENT_VERSION,   "CLIENT_VERSION",   STRING_WRITE_ONCE(1,40) }, 
   { eMSDP_PLUGIN_ID,        "PLUGIN_ID",        STRING_WITH_LENGTH_OF(1,40) }, 
   { eMSDP_ANSI_COLORS,      "ANSI_COLORS",      BOOLEAN_SET_TO(1) }, 
   { eMSDP_XTERM_256_COLORS, "XTERM_256_COLORS", BOOLEAN_SET_TO(0) }, 
   { eMSDP_UTF_8,            "UTF_8",            BOOLEAN_SET_TO(0) }, 
   { eMSDP_SOUND,            "SOUND",            BOOLEAN_SET_TO(0) }, 
   { eMSDP_MXP,              "MXP",              BOOLEAN_SET_TO(0) }, 

   /* GUI variables */
   { eMSDP_BUTTON_1,         "BUTTON_1",         STRING_GUI(s_Button1) }, 
   { eMSDP_BUTTON_2,         "BUTTON_2",         STRING_GUI(s_Button2) }, 
   { eMSDP_BUTTON_3,         "BUTTON_3",         STRING_GUI(s_Button3) }, 
   { eMSDP_BUTTON_4,         "BUTTON_4",         STRING_GUI(s_Button4) }, 
   { eMSDP_BUTTON_5,         "BUTTON_5",         STRING_GUI(s_Button5) }, 
   { eMSDP_GAUGE_1,          "GAUGE_1",          STRING_GUI(s_Gauge1) }, 
   { eMSDP_GAUGE_2,          "GAUGE_2",          STRING_GUI(s_Gauge2) }, 
   { eMSDP_GAUGE_3,          "GAUGE_3",          STRING_GUI(s_Gauge3) }, 
   { eMSDP_GAUGE_4,          "GAUGE_4",          STRING_GUI(s_Gauge4) }, 
   { eMSDP_GAUGE_5,          "GAUGE_5",          STRING_GUI(s_Gauge5) }, 

   { eMSDP_MAX,              "", false, false, false, false, 0, 0, 0, NULL } /* This must always be last. */
};

/******************************************************************************
 MSSP file-scope variables.
 ******************************************************************************/

static int    s_Players = 0;
static time_t s_Uptime  = 0;

/******************************************************************************
 Local function prototypes.
 ******************************************************************************/

static void Negotiate            ( descriptor_t *apDescriptor );
static void PerformHandshake     ( descriptor_t *apDescriptor, char aCmd, char aProtocol );
static void PerformSubnegotiation( descriptor_t *apDescriptor, char aCmd, char *apData, int aSize );

static void ParseMSDP            ( descriptor_t *apDescriptor, const char *apData );
static void ExecuteMSDPPair      ( descriptor_t *apDescriptor, const char *apVariable, const char *apValue );

static void ParseATCP            ( descriptor_t *apDescriptor, const char *apData );
#ifdef MUDLET_PACKAGE
static void SendATCP             ( descriptor_t *apDescriptor, const char *apVariable, const char *apValue );
#endif /* MUDLET_PACKAGE */

static void SendMSSP             ( descriptor_t *apDescriptor );

static char *GetMxpTag           ( const char *apTag, const char *apText );

static const char *GetAnsiColour ( bool_t abBackground, int aRed, int aGreen, int aBlue );
static const char *GetRGBColour  ( bool_t abBackground, int aRed, int aGreen, int aBlue );
static bool_t IsValidColour      ( const char *apArgument );

static bool_t MatchString        ( const char *apFirst, const char *apSecond );
static bool_t PrefixString       ( const char *apPart, const char *apWhole );
static bool_t IsNumber           ( const char *apString );
static char  *AllocString        ( const char *apString );

/******************************************************************************
 ANSI colour codes.
 ******************************************************************************/

static const char s_Clean       [] = "\033[0;00m"; /* Remove colour */

static const char s_DarkBlack   [] = "\033[0;30m"; /* Black foreground */
static const char s_DarkRed     [] = "\033[0;31m"; /* Red foreground */
static const char s_DarkGreen   [] = "\033[0;32m"; /* Green foreground */
static const char s_DarkYellow  [] = "\033[0;33m"; /* Yellow foreground */
static const char s_DarkBlue    [] = "\033[0;34m"; /* Blue foreground */
static const char s_DarkMagenta [] = "\033[0;35m"; /* Magenta foreground */
static const char s_DarkCyan    [] = "\033[0;36m"; /* Cyan foreground */
static const char s_DarkWhite   [] = "\033[0;37m"; /* White foreground */

static const char s_BoldBlack   [] = "\033[1;30m"; /* Grey foreground */
static const char s_BoldRed     [] = "\033[1;31m"; /* Bright red foreground */
static const char s_BoldGreen   [] = "\033[1;32m"; /* Bright green foreground */
static const char s_BoldYellow  [] = "\033[1;33m"; /* Bright yellow foreground */
static const char s_BoldBlue    [] = "\033[1;34m"; /* Bright blue foreground */
static const char s_BoldMagenta [] = "\033[1;35m"; /* Bright magenta foreground */
static const char s_BoldCyan    [] = "\033[1;36m"; /* Bright cyan foreground */
static const char s_BoldWhite   [] = "\033[1;37m"; /* Bright white foreground */

static const char s_BackBlack   [] = "\033[1;40m"; /* Black background */
static const char s_BackRed     [] = "\033[1;41m"; /* Red background */
static const char s_BackGreen   [] = "\033[1;42m"; /* Green background */
static const char s_BackYellow  [] = "\033[1;43m"; /* Yellow background */
static const char s_BackBlue    [] = "\033[1;44m"; /* Blue background */
static const char s_BackMagenta [] = "\033[1;45m"; /* Magenta background */
static const char s_BackCyan    [] = "\033[1;46m"; /* Cyan background */
static const char s_BackWhite   [] = "\033[1;47m"; /* White background */

/******************************************************************************
 Protocol global functions.
 ******************************************************************************/

protocol_t *ProtocolCreate( void )
{
   int i; /* Loop counter */
   protocol_t *pProtocol;

   /* Called the first time we enter - make sure the table is correct */
   static bool_t bInit = false;
   if ( !bInit )
   {
      bInit = true;
      for ( i = eMSDP_NONE+1; i < eMSDP_MAX; ++i )
      {
         if ( VariableNameTable[i].Variable != i )
         {
            ReportBug( "MSDP: Variable table does not match the enums in the header.\n" );
            break;
         }
      }
   }

   pProtocol = (protocol_t *) malloc(sizeof(protocol_t));
   pProtocol->WriteOOB = 0;
   pProtocol->bIACMode = false;
   pProtocol->bNegotiated = false;
   pProtocol->bBlockMXP = false;
   pProtocol->bTTYPE = false;
   pProtocol->bNAWS = false;
   pProtocol->bCHARSET = false;
   pProtocol->bMSDP = false;
   pProtocol->bATCP = false;
   pProtocol->bMSP = false;
   pProtocol->bMXP = false;
   pProtocol->bMCCP = false;
   pProtocol->b256Support = eUNKNOWN;
   pProtocol->ScreenWidth = 0;
   pProtocol->ScreenHeight = 0;
   pProtocol->pMXPVersion = AllocString("Unknown");
   pProtocol->pLastTTYPE = NULL;
   pProtocol->pVariables = (MSDP_t **) malloc(sizeof(MSDP_t*)*eMSDP_MAX);

   for ( i = eMSDP_NONE+1; i < eMSDP_MAX; ++i )
   {
      pProtocol->pVariables[i] = (MSDP_t *) malloc(sizeof(MSDP_t));
      pProtocol->pVariables[i]->bReport = false;
      pProtocol->pVariables[i]->bDirty = false;
      pProtocol->pVariables[i]->ValueInt = 0;
      pProtocol->pVariables[i]->pValueString = NULL;

      if ( VariableNameTable[i].bString )
      {
         if ( VariableNameTable[i].pDefault != NULL )
            pProtocol->pVariables[i]->pValueString = AllocString(VariableNameTable[i].pDefault);
         else if ( VariableNameTable[i].bConfigurable )
            pProtocol->pVariables[i]->pValueString = AllocString("Unknown");
         else /* Use an empty string */
            pProtocol->pVariables[i]->pValueString = AllocString("");
      }
      else if ( VariableNameTable[i].Default != 0 )
      {
         pProtocol->pVariables[i]->ValueInt = VariableNameTable[i].Default;
      }
   }

   return pProtocol;
}

void ProtocolDestroy( protocol_t *apProtocol )
{
   int i; /* Loop counter */

   for ( i = eMSDP_NONE+1; i < eMSDP_MAX; ++i )
   {
      if (apProtocol->pVariables[i]->pValueString)
        free(apProtocol->pVariables[i]->pValueString);
      free(apProtocol->pVariables[i]);
   }

   free(apProtocol->pVariables);
   if (apProtocol->pLastTTYPE) /* Isn't saved over copyover so may still be NULL */
     free(apProtocol->pLastTTYPE);
   free(apProtocol->pMXPVersion);
   free(apProtocol);
}

ssize_t ProtocolInput( descriptor_t *apDescriptor, char *apData, int aSize, char *apOut )
{
   static char CmdBuf[MAX_PROTOCOL_BUFFER+1];
   static char IacBuf[MAX_PROTOCOL_BUFFER+1];
   ssize_t CmdIndex = 0;
   ssize_t IacIndex = 0;
   ssize_t Index;

   protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

   for ( Index = 0; Index < aSize; ++Index )
   {
      /* If we'd overflow the buffer, we just ignore the input */
      if ( CmdIndex >= MAX_PROTOCOL_BUFFER || IacIndex >= MAX_PROTOCOL_BUFFER )
      {
         ReportBug("ProtocolInput: Too much incoming data to store in the buffer.\n");
         return (-1);
      }

      /* IAC IAC is treated as a single value of 255 */
      if ( apData[Index] == (char)IAC && apData[Index+1] == (char)IAC )
      {
         if ( pProtocol->bIACMode )
            IacBuf[IacIndex++] = (char)IAC;
         else /* In-band command */
            CmdBuf[CmdIndex++] = (char)IAC;
         Index++;
      }
      else if ( pProtocol->bIACMode )
      {
         /* End subnegotiation. */
         if ( apData[Index] == (char)IAC && apData[Index+1] == (char)SE )
         {
            Index++;
            pProtocol->bIACMode = false;
            IacBuf[IacIndex] = '\0';
            if ( IacIndex >= 2 )
               PerformSubnegotiation( apDescriptor, IacBuf[0], &IacBuf[1], IacIndex-1 );
            IacIndex = 0;
         }
         else
           IacBuf[IacIndex++] = apData[Index];
      }
      else if ( apData[Index] == (char)27 && apData[Index+1] == '[' && 
         isdigit(apData[Index+2]) && apData[Index+3] == 'z' )
      {
         char MXPBuffer [1024];
         char *pMXPTag = NULL;
         int i = 0; /* Loop counter */

         Index += 4; /* Skip to the start of the MXP sequence. */

         while ( Index < aSize && apData[Index] != '>' && i < 1000 )
         {
            MXPBuffer[i++] = apData[Index++];
         }
         MXPBuffer[i++] = '>';
         MXPBuffer[i] = '\0';

         if ( ( pMXPTag = GetMxpTag( "CLIENT=", MXPBuffer ) ) != NULL )
         {
            /* Overwrite the previous client name - this is harder to fake */
            free(pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString);
            pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString = AllocString(pMXPTag);
         }

         if ( ( pMXPTag = GetMxpTag( "VERSION=", MXPBuffer ) ) != NULL )
         {
            const char *pClientName = pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString;

            InfoMessage(apDescriptor, "Receiving MXP Version From Client.\r\n");

            free(pProtocol->pVariables[eMSDP_CLIENT_VERSION]->pValueString);
            pProtocol->pVariables[eMSDP_CLIENT_VERSION]->pValueString = AllocString(pMXPTag);

            if ( MatchString( "MUSHCLIENT", pClientName ) )
            {
               /* MUSHclient 4.02 and later supports 256 colours. */
               if ( strcmp(pMXPTag, "4.02") >= 0 )
               {
                  pProtocol->pVariables[eMSDP_XTERM_256_COLORS]->ValueInt = 1;
                  pProtocol->b256Support = eYES;
               }
               else /* We know for sure that 256 colours are not supported */
                  pProtocol->b256Support = eNO;
            }
            else if ( MatchString( "CMUD", pClientName ) )
            {
               /* CMUD 3.04 and later supports 256 colours. */
               if ( strcmp(pMXPTag, "3.04") >= 0 )
               {
                  pProtocol->pVariables[eMSDP_XTERM_256_COLORS]->ValueInt = 1;
                  pProtocol->b256Support = eYES;
               }
               else /* We know for sure that 256 colours are not supported */
                  pProtocol->b256Support = eNO;
            }
            else if ( MatchString( "ATLANTIS", pClientName ) )
            {
               /* Atlantis 0.9.9.0 supports XTerm 256 colours, but it doesn't 
                * yet have MXP.  However MXP is planned, so once it responds 
                * to a <VERSION> tag we'll know we can use 256 colours.
                */
               pProtocol->pVariables[eMSDP_XTERM_256_COLORS]->ValueInt = 1;
               pProtocol->b256Support = eYES;
            }
         }

         if ( ( pMXPTag = GetMxpTag( "MXP=", MXPBuffer ) ) != NULL )
         {
            free(pProtocol->pMXPVersion);
            pProtocol->pMXPVersion = AllocString(pMXPTag);
         }
 
         /* No longer necessary
          * 
         if ( strcmp(pProtocol->pMXPVersion, "Unknown") )
         {
            Write( apDescriptor, "\n" );
            sprintf( MXPBuffer, "MXP version %s detected and enabled.\r\n", 
               pProtocol->pMXPVersion );
            InfoMessage( apDescriptor, MXPBuffer );
         } */
      }
      else /* In-band command */
      {
         if ( apData[Index] == (char)IAC )
         {
            switch ( apData[Index+1] )
            {
               case (char)SB: /* Begin subnegotiation. */
                  Index++;
                  pProtocol->bIACMode = true;
                  break;

               case (char)DO: /* Handshake. */
               case (char)DONT:
               case (char)WILL:
               case (char)WONT: 
                  PerformHandshake( apDescriptor, apData[Index+1], apData[Index+2] );
                  Index += 2;
                  break;

               case (char)IAC: /* Two IACs count as one. */
                  CmdBuf[CmdIndex++] = (char)IAC;
                  Index++;
                  break;

               default: /* Skip it. */
                  Index++;
                  break;
            }
         }
         else
            CmdBuf[CmdIndex++] = apData[Index];
      }
   }

   /* Terminate the two buffers */
   IacBuf[IacIndex] = '\0';
   CmdBuf[CmdIndex] = '\0';

   /* Copy the input buffer back to the player. */
   strcat( apOut, CmdBuf );
   return (CmdIndex);
}

const char *ProtocolOutput( descriptor_t *apDescriptor, const char *apData, int *apLength )
{
   static char Result[MAX_OUTPUT_BUFFER+1];
   const char Tab[] = "\t";
   const char MSP[] = "!!";
   const char MXPStart[] = "\033[1z<";
   const char MXPStop[] = ">\033[7z";
   const char LinkStart[] = "\033[1z<send>\033[7z";
   const char LinkStop[] = "\033[1z</send>\033[7z";
   bool_t bTerminate = false, bUseMXP = false, bUseMSP = false;
   int i = 0, j = 0; /* Index values */

   protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
   if ( pProtocol == NULL || apData == NULL )
      return apData;

   /* Strip !!SOUND() triggers if they support MSP or are using sound */
   if ( pProtocol->bMSP || pProtocol->pVariables[eMSDP_SOUND]->ValueInt )
      bUseMSP = true;

   for ( ; i < MAX_OUTPUT_BUFFER && apData[j] != '\0' && !bTerminate && 
      (*apLength <= 0 || j < *apLength); ++j )
   {
      if ( apData[j] == '\t' )
      {
         const char *pCopyFrom = NULL;

         switch ( apData[++j] )
         {
            case '\t': /* Two tabs in a row will display an actual tab */
               pCopyFrom = Tab;
               break;
            case '_':
               pCopyFrom = "\x1B[4m"; /* Underline... if supported */
               break;
            case '+':
               pCopyFrom = "\x1B[1m"; /* Bold... if supported */
               break;
            case '-':
               pCopyFrom = "\x1B[5m"; /* Blinking... if supported */
               break;
            case '=':
               pCopyFrom = "\x1B[7m"; /* Reverse... if supported */
               break;
            case '*':
               pCopyFrom = "@"; /* The At Symbol... I don't really like this, but it seems like
                                   a simple way to allow for the @ symbol while maintain portability
                                   between pre-ProtocolOutput() muds and post ProtocolOutput() muds.*/
               break;
            /* 1,2,3 to be used a MUD's base colour palette. Just to maintain
             * some sort of common colouring scheme amongst coders/builders */
            case '1':
               pCopyFrom = ColourRGB(apDescriptor, RGBone);
               break;
            case '2':
               pCopyFrom = ColourRGB(apDescriptor, RGBtwo);
               break;
            case '3':
               pCopyFrom = ColourRGB(apDescriptor, RGBthree);
               break;
            case 'n':
               pCopyFrom = s_Clean;
               break;
            case 'd': /* dark grey / black */
               pCopyFrom = ColourRGB(apDescriptor, "F000");
               break;
            case 'D': /* light grey */
               pCopyFrom = ColourRGB(apDescriptor, "F111");
               break;
            case 'a': /* dark azure */
               pCopyFrom = ColourRGB(apDescriptor, "F021");
               break;
            case 'A': /* light Azure */
               pCopyFrom = ColourRGB(apDescriptor, "F053");
               break;
            case 'r': /* dark red */
               pCopyFrom = ColourRGB(apDescriptor, "F200");
               break;
            case 'R': /* light red */
               pCopyFrom = ColourRGB(apDescriptor, "F500");
               break;
            case 'g': /* dark green */
               pCopyFrom = ColourRGB(apDescriptor, "F020");
               break;
            case 'G': /* light green */
               pCopyFrom = ColourRGB(apDescriptor, "F050");
               break;
            case 'y': /* dark yellow */
               pCopyFrom = ColourRGB(apDescriptor, "F330");
               break;
            case 'Y': /* light yellow */
               pCopyFrom = ColourRGB(apDescriptor, "F550");
               break;
            case 'b': /* dark blue */
               pCopyFrom = ColourRGB(apDescriptor, "F012");
               break;
            case 'B': /* light blue */
               pCopyFrom = ColourRGB(apDescriptor, "F025");
               break;
            case 'm': /* dark magenta */
               pCopyFrom = ColourRGB(apDescriptor, "F202");
               break;
            case 'M': /* light magenta */
               pCopyFrom = ColourRGB(apDescriptor, "F505");
               break;
            case 'c': /* dark cyan */
               pCopyFrom = ColourRGB(apDescriptor, "F022");
               break;
            case 'C': /* light cyan */
               pCopyFrom = ColourRGB(apDescriptor, "F055");
               break;
            case 'w': /* dark white */
               pCopyFrom = ColourRGB(apDescriptor, "F333");
               break;
            case 'W': /* light white */
               pCopyFrom = ColourRGB(apDescriptor, "F555");
               break;
            case 'o': /* dark orange */
               pCopyFrom = ColourRGB(apDescriptor, "F520");
               break;
            case 'O': /* light orange */
               pCopyFrom = ColourRGB(apDescriptor, "F530");
               break;
            case 'p': /* dark pink */
               pCopyFrom = ColourRGB(apDescriptor, "F301");
               break;
            case 'P': /* light pink */
               pCopyFrom = ColourRGB(apDescriptor, "F501");
               break;
            case '(': /* MXP link */
               if ( !pProtocol->bBlockMXP && pProtocol->pVariables[eMSDP_MXP]->ValueInt )
                  pCopyFrom = LinkStart;
               break;
            case ')': /* MXP link */
               if ( !pProtocol->bBlockMXP && pProtocol->pVariables[eMSDP_MXP]->ValueInt )
                  pCopyFrom = LinkStop;
               pProtocol->bBlockMXP = false;
               break;
            case '<':
               if ( !pProtocol->bBlockMXP && pProtocol->pVariables[eMSDP_MXP]->ValueInt )
               {
                  pCopyFrom = MXPStart;
                  bUseMXP = true;
               }
               else /* No MXP support, so just strip it out */
               {
                  while ( apData[j] != '\0' && apData[j] != '>' )
                     ++j;
               }
               pProtocol->bBlockMXP = false;
               break;
            case '[':
               if ( tolower(apData[++j]) == 'u' )
               {
                  char Buffer[8] = {'\0'}, BugString[256];
                  int Index = 0;
                  int Number = 0;
                  bool_t bDone = false, bValid = true;

                  while ( isdigit(apData[++j]) )
                  {
                     Number *= 10;
                     Number += (apData[j])-'0';
                  }

                  if ( apData[j] == '/' )
                     ++j;

                  while ( apData[j] != '\0' && !bDone )
                  {
                     if ( apData[j] == ']' )
                        bDone = true;
                     else if ( Index < 7 )
                        Buffer[Index++] = apData[j++];
                     else /* It's too long, so ignore the rest and note the problem */
                     {
                        j++;
                        bValid = false;
                     }
                  }

                  if ( !bDone )
                  {
                     sprintf( BugString, "BUG: Unicode substitute '%s' wasn't terminated with ']'.\n", Buffer );
                     ReportBug( BugString );
                  }
                  else if ( !bValid )
                  {
                     sprintf( BugString, "BUG: Unicode substitute '%s' truncated.  Missing ']'?\n", Buffer );
                     ReportBug( BugString );
                  }
                  else if ( pProtocol->pVariables[eMSDP_UTF_8]->ValueInt )
                  {
                     pCopyFrom = UnicodeGet(Number);
                  }
                  else /* Display the substitute string */
                  {
                     pCopyFrom = Buffer;
                  }

                  /* Terminate if we've reached the end of the string */
                  bTerminate = !bDone;
               }
               else if ( tolower(apData[j]) == 'f' || tolower(apData[j]) == 'b' )
               {
                  char Buffer[8] = {'\0'}, BugString[256];
                  int Index = 0;
                  bool_t bDone = false, bValid = true;

                  /* Copy the 'f' (foreground) or 'b' (background) */
                  Buffer[Index++] = apData[j++];

                  while ( apData[j] != '\0' && !bDone && bValid )
                  {
                     if ( apData[j] == ']' )
                        bDone = true;
                     else if ( Index < 4 )
                        Buffer[Index++] = apData[j++];
                     else /* It's too long, so drop out - the colour code may still be valid */
                        bValid = false;
                  }

                  if ( !bDone || !bValid)
                  {
                     sprintf( BugString, "BUG: RGB %sground colour '%s' wasn't terminated with ']'.\n", 
                        (tolower(Buffer[0]) == 'f') ? "fore" : "back", &Buffer[1] );
                     ReportBug( BugString );
                  }
                  else if ( !IsValidColour(Buffer) )
                  {
                     sprintf( BugString, "BUG: RGB %sground colour '%s' invalid (each digit must be in the range 0-5).\n", 
                        (tolower(Buffer[0]) == 'f') ? "fore" : "back", &Buffer[1] );
                     ReportBug( BugString );
                  }
                  else /* Success */
                  {
                     pCopyFrom = ColourRGB(apDescriptor, Buffer);
                  }
               }
               else if ( tolower(apData[j]) == 'x' )
               {
                  char Buffer[8] = {'\0'}, BugString[256];
                  int Index = 0;
                  bool_t bDone = false, bValid = true;

                  ++j; /* Skip the 'x' */

                  while ( apData[j] != '\0' && !bDone )
                  {
                     if ( apData[j] == ']' )
                        bDone = true;
                     else if ( Index < 7 )
                        Buffer[Index++] = apData[j++];
                     else /* It's too long, so ignore the rest and note the problem */
                     {
                        j++;
                        bValid = false;
                     }
                  }

                  if ( !bDone )
                  {
                     sprintf( BugString, "BUG: Required MXP version '%s' wasn't terminated with ']'.\n", Buffer );
                     ReportBug( BugString );
                  }
                  else if ( !bValid )
                  {
                     sprintf( BugString, "BUG: Required MXP version '%s' too long.  Missing ']'?\n", Buffer );
                     ReportBug( BugString );
                  }
                  else if ( !strcmp(pProtocol->pMXPVersion, "Unknown") || 
                     strcmp(pProtocol->pMXPVersion, Buffer) < 0 )
                  {
                     /* Their version of MXP isn't high enough */
                     pProtocol->bBlockMXP = true;
                  }
                  else /* MXP is sufficient for this tag */
                  {
                     pProtocol->bBlockMXP = false;
                  }

                  /* Terminate if we've reached the end of the string */
                  bTerminate = !bDone;
               }
               break;
            case '!': /* Used for in-band MSP sound triggers */
               pCopyFrom = MSP;
               break;
            case '\0':
               bTerminate = true;
               break;
            default:
               break;
         }

         /* Copy the colour code, if any. */
         if ( pCopyFrom != NULL )
         {
            while ( *pCopyFrom != '\0' && i < MAX_OUTPUT_BUFFER )
               Result[i++] = *pCopyFrom++;
         }
      }
      else if ( bUseMXP && apData[j] == '>' )
      {
         const char *pCopyFrom = MXPStop;
         while ( *pCopyFrom != '\0' && i < MAX_OUTPUT_BUFFER)
            Result[i++] = *pCopyFrom++;
         bUseMXP = false;
      }
      else if ( bUseMSP && j > 0 && apData[j-1] == '!' && apData[j] == '!' && 
         PrefixString("SOUND(", &apData[j+1]) )
      {
         /* Avoid accidental triggering of old-style MSP triggers */
         Result[i++] = '?';
      }
      else /* Just copy the character normally */
      {
         Result[i++] = apData[j];
      }
   }

   /* If we'd overflow the buffer, we don't send any output */
   if ( i >= MAX_OUTPUT_BUFFER )
   {
      i = 0;
      ReportBug("ProtocolOutput: Too much outgoing data to store in the buffer.\n");
   }

   /* Terminate the string */
   Result[i] = '\0';

   /* Store the length */
   if ( apLength )
      *apLength = i;

   /* Return the string */
   return Result;
}

/* Some clients (such as GMud) don't properly handle negotiation, and simply 
 * display every printable character to the screen.  However TTYPE isn't a 
 * printable character, so we negotiate for it first, and only negotiate for 
 * other protocols if the client responds with IAC WILL TTYPE or IAC WONT 
 * TTYPE.  Thanks go to Donky on MudBytes for the suggestion.
 */
void ProtocolNegotiate( descriptor_t *apDescriptor )
{
   static const char DoTTYPE [] = { (char)IAC, (char)DO, TELOPT_TTYPE, '\0' };
   Write(apDescriptor, DoTTYPE);
}

/******************************************************************************
 Copyover save/load functions.
 ******************************************************************************/

const char *CopyoverGet( descriptor_t *apDescriptor )
{
   static char Buffer[64];
   char *pBuffer = Buffer;
   protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

   if ( pProtocol != NULL )
   {
      sprintf(Buffer, "%d/%d", pProtocol->ScreenWidth, pProtocol->ScreenHeight);

      /* Skip to the end */
      while ( *pBuffer != '\0' )
         ++pBuffer;

      if ( pProtocol->bTTYPE )
         *pBuffer++ = 'T';
      if ( pProtocol->bNAWS )
         *pBuffer++ = 'N';
      if ( pProtocol->bMSDP )
         *pBuffer++ = 'M';
      if ( pProtocol->bATCP )
         *pBuffer++ = 'A';
      if ( pProtocol->bMSP )
         *pBuffer++ = 'S';
      if ( pProtocol->pVariables[eMSDP_MXP]->ValueInt )
         *pBuffer++ = 'X';
      if ( pProtocol->bMCCP )
      {
         *pBuffer++ = 'c';
         CompressEnd(apDescriptor);
      }
      if ( pProtocol->pVariables[eMSDP_XTERM_256_COLORS]->ValueInt )
         *pBuffer++ = 'C';
      if ( pProtocol->bCHARSET )
         *pBuffer++ = 'H';
      if ( pProtocol->pVariables[eMSDP_UTF_8]->ValueInt )
         *pBuffer++ = 'U';
   }

   /* Terminate the string */
   *pBuffer = '\0';

   return Buffer;
}

void CopyoverSet( descriptor_t *apDescriptor, const char *apData )
{
   protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

   if ( pProtocol != NULL && apData != NULL )
   {
      int Width = 0, Height = 0;
      bool_t bDoneWidth = false;
      int i; /* Loop counter */

      for ( i = 0; apData[i] != '\0'; ++i )
      {
         switch ( apData[i] )
         {
            case 'T':
               pProtocol->bTTYPE = true;
               break;
            case 'N':
               pProtocol->bNAWS = true;
               break;
            case 'M':
               pProtocol->bMSDP = true;
               break;
            case 'A':
               pProtocol->bATCP = true;
               break;
            case 'S':
               pProtocol->bMSP = true;
               break;
            case 'X':
               pProtocol->bMXP = true;
               pProtocol->pVariables[eMSDP_MXP]->ValueInt = 1;
               break;
            case 'c':
               pProtocol->bMCCP = true;
               CompressStart(apDescriptor);
               break;
            case 'C':
               pProtocol->pVariables[eMSDP_XTERM_256_COLORS]->ValueInt = 1;
               break;
            case 'H':
               pProtocol->bCHARSET = true;
               break;
            case 'U':
               pProtocol->pVariables[eMSDP_UTF_8]->ValueInt = 1;
               break;
            default:
               if ( apData[i] == '/' )
                  bDoneWidth = true;
               else if ( isdigit(apData[i]) )
               {
                  if ( bDoneWidth )
                  {
                     Height *= 10;
                     Height += (apData[i] - '0');
                  }
                  else /* We're still calculating height */
                  {
                     Width *= 10;
                     Width += (apData[i] - '0');
                  }
               }
               break;
         }
      }

      /* Restore the width and height */
      pProtocol->ScreenWidth = Width;
      pProtocol->ScreenHeight = Height;

      /* If we're using MSDP or ATCP, we need to renegotiate it so that the 
       * client can resend the list of variables it wants us to REPORT.
       *
       * Note that we only use ATCP if MSDP is not supported.
       */
      if ( pProtocol->bMSDP )
      {
         char WillMSDP [] = { (char)IAC, (char)WILL, TELOPT_MSDP, '\0' };
         Write(apDescriptor, WillMSDP);
      }
      else if ( pProtocol->bATCP )
      {
         char DoATCP [] = { (char)IAC, (char)DO, (char)TELOPT_ATCP, '\0' };
         Write(apDescriptor, DoATCP);
      }

      /* Ask the client to send its MXP version again */
      if ( pProtocol->bMXP )
         MXPSendTag( apDescriptor, "<VERSION>" );
   }
}

/******************************************************************************
 MSDP global functions.
 ******************************************************************************/

void MSDPUpdate( descriptor_t *apDescriptor )
{
   int i; /* Loop counter */

   protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

   for ( i = eMSDP_NONE+1; i < eMSDP_MAX; ++i )
   {
      if ( pProtocol->pVariables[i]->bReport )
      {
         if ( pProtocol->pVariables[i]->bDirty )
         {
            MSDPSend( apDescriptor, (variable_t)i );
            pProtocol->pVariables[i]->bDirty = false;
         }
      }
   }
}

void MSDPFlush( descriptor_t *apDescriptor, variable_t aMSDP )
{
   if ( aMSDP > eMSDP_NONE && aMSDP < eMSDP_MAX )
   {
      protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

      if ( pProtocol->pVariables[aMSDP]->bReport )
      {
         if ( pProtocol->pVariables[aMSDP]->bDirty )
         {
            MSDPSend( apDescriptor, aMSDP );
            pProtocol->pVariables[aMSDP]->bDirty = false;
         }
      }
   }
}

void MSDPSend( descriptor_t *apDescriptor, variable_t aMSDP )
{
   char MSDPBuffer[MAX_VARIABLE_LENGTH+1] = { '\0' };

   if ( aMSDP > eMSDP_NONE && aMSDP < eMSDP_MAX )
   {
      protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

      if ( VariableNameTable[aMSDP].bString )
      {
         /* Should really be replaced with a dynamic buffer */
         int RequiredBuffer = strlen(VariableNameTable[aMSDP].pName) + 
            strlen(pProtocol->pVariables[aMSDP]->pValueString) + 12;

         if ( RequiredBuffer >= MAX_VARIABLE_LENGTH )
         {
            sprintf( MSDPBuffer, 
               "MSDPSend: %s %d bytes (exceeds MAX_VARIABLE_LENGTH of %d).\n", 
               VariableNameTable[aMSDP].pName, RequiredBuffer, 
               MAX_VARIABLE_LENGTH );
            ReportBug( MSDPBuffer );
            MSDPBuffer[0] = '\0';
         }
         else if ( pProtocol->bMSDP )
         {
            sprintf( MSDPBuffer, "%c%c%c%c%s%c%s%c%c", 
               IAC, SB, TELOPT_MSDP, MSDP_VAR, 
               VariableNameTable[aMSDP].pName, MSDP_VAL, 
               pProtocol->pVariables[aMSDP]->pValueString, IAC, SE );
         }
         else if ( pProtocol->bATCP )
         {
            sprintf( MSDPBuffer, "%c%c%cMSDP.%s %s%c%c", 
               IAC, SB, TELOPT_ATCP, 
               VariableNameTable[aMSDP].pName, 
               pProtocol->pVariables[aMSDP]->pValueString, IAC, SE );
         }
      }
      else /* It's an integer, not a string */
      {
         if ( pProtocol->bMSDP )
         {
            sprintf( MSDPBuffer, "%c%c%c%c%s%c%d%c%c", 
               IAC, SB, TELOPT_MSDP, MSDP_VAR, 
               VariableNameTable[aMSDP].pName, MSDP_VAL, 
               pProtocol->pVariables[aMSDP]->ValueInt, IAC, SE );
         }
         else if ( pProtocol->bATCP )
         {
            sprintf( MSDPBuffer, "%c%c%cMSDP.%s %d%c%c", 
               IAC, SB, TELOPT_ATCP, 
               VariableNameTable[aMSDP].pName, 
               pProtocol->pVariables[aMSDP]->ValueInt, IAC, SE );
         }
      }

      /* Just in case someone calls this function without checking MSDP/ATCP */
      if ( MSDPBuffer[0] != '\0' )
         Write( apDescriptor, MSDPBuffer );
   }
}

void MSDPSendPair( descriptor_t *apDescriptor, const char *apVariable, const char *apValue )
{
   char MSDPBuffer[MAX_VARIABLE_LENGTH+1] = { '\0' };

   if ( apVariable != NULL && apValue != NULL )
   {
      protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

      /* Should really be replaced with a dynamic buffer */
      int RequiredBuffer = strlen(apVariable) + strlen(apValue) + 12;

      if ( RequiredBuffer >= MAX_VARIABLE_LENGTH )
      {
         if ( RequiredBuffer - strlen(apValue) < MAX_VARIABLE_LENGTH )
         {
            sprintf( MSDPBuffer, 
               "MSDPSendPair: %s %d bytes (exceeds MAX_VARIABLE_LENGTH of %d).\n", 
               apVariable, RequiredBuffer, MAX_VARIABLE_LENGTH );
         }
         else /* The variable name itself is too long */
         {
            sprintf( MSDPBuffer, 
               "MSDPSendPair: Variable name has a length of %d bytes (exceeds MAX_VARIABLE_LENGTH of %d).\n", 
               RequiredBuffer, MAX_VARIABLE_LENGTH );
         }

         ReportBug( MSDPBuffer );
         MSDPBuffer[0] = '\0';
      }
      else if ( pProtocol->bMSDP )
      {
         sprintf( MSDPBuffer, "%c%c%c%c%s%c%s%c%c", 
            IAC, SB, TELOPT_MSDP, MSDP_VAR, apVariable, MSDP_VAL, 
            apValue, IAC, SE );
      }
      else if ( pProtocol->bATCP )
      {
         sprintf( MSDPBuffer, "%c%c%cMSDP.%s %s%c%c", 
            IAC, SB, TELOPT_ATCP, apVariable, apValue, IAC, SE );
      }

      /* Just in case someone calls this function without checking MSDP/ATCP */
      if ( MSDPBuffer[0] != '\0' )
         Write( apDescriptor, MSDPBuffer );
   }
}

void MSDPSendList( descriptor_t *apDescriptor, const char *apVariable, const char *apValue )
{
   char MSDPBuffer[MAX_VARIABLE_LENGTH+1] = { '\0' };

   if ( apVariable != NULL && apValue != NULL )
   {
      protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

      /* Should really be replaced with a dynamic buffer */
      int RequiredBuffer = strlen(apVariable) + strlen(apValue) + 12;

      if ( RequiredBuffer >= MAX_VARIABLE_LENGTH )
      {
         if ( RequiredBuffer - strlen(apValue) < MAX_VARIABLE_LENGTH )
         {
            sprintf( MSDPBuffer, 
               "MSDPSendList: %s %d bytes (exceeds MAX_VARIABLE_LENGTH of %d).\n", 
               apVariable, RequiredBuffer, MAX_VARIABLE_LENGTH );
         }
         else /* The variable name itself is too long */
         {
            sprintf( MSDPBuffer, 
               "MSDPSendList: Variable name has a length of %d bytes (exceeds MAX_VARIABLE_LENGTH of %d).\n", 
               RequiredBuffer, MAX_VARIABLE_LENGTH );
         }

         ReportBug( MSDPBuffer );
         MSDPBuffer[0] = '\0';
      }
      else if ( pProtocol->bMSDP )
      {
         int i; /* Loop counter */
         sprintf( MSDPBuffer, "%c%c%c%c%s%c%c%c%s%c%c%c", 
            IAC, SB, TELOPT_MSDP, MSDP_VAR, apVariable, MSDP_VAL, 
            MSDP_ARRAY_OPEN, MSDP_VAL, apValue, MSDP_ARRAY_CLOSE, IAC, SE );

         /* Convert the spaces to MSDP_VAL */
         for ( i = 0; MSDPBuffer[i] != '\0'; ++i )
         {
            if ( MSDPBuffer[i] == ' ' )
               MSDPBuffer[i] = MSDP_VAL;
         }
      }
      else if ( pProtocol->bATCP )
      {
         sprintf( MSDPBuffer, "%c%c%cMSDP.%s %s%c%c", 
            IAC, SB, TELOPT_ATCP, apVariable, apValue, IAC, SE );
      }

      /* Just in case someone calls this function without checking MSDP/ATCP */
      if ( MSDPBuffer[0] != '\0' )
         Write( apDescriptor, MSDPBuffer );
   }
}

void MSDPSetNumber( descriptor_t *apDescriptor, variable_t aMSDP, int aValue )
{
   protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

   if ( pProtocol != NULL && aMSDP > eMSDP_NONE && aMSDP < eMSDP_MAX )
   {
      if ( !VariableNameTable[aMSDP].bString )
      {
         if ( pProtocol->pVariables[aMSDP]->ValueInt != aValue )
         {
            pProtocol->pVariables[aMSDP]->ValueInt = aValue;
            pProtocol->pVariables[aMSDP]->bDirty = true;
         }
      }
   }
}

void MSDPSetString( descriptor_t *apDescriptor, variable_t aMSDP, const char *apValue )
{
   protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

   if ( pProtocol != NULL && apValue != NULL )
   {
      if ( VariableNameTable[aMSDP].bString )
      {
         if ( strcmp(pProtocol->pVariables[aMSDP]->pValueString, apValue) )
         {
            free(pProtocol->pVariables[aMSDP]->pValueString);
            pProtocol->pVariables[aMSDP]->pValueString = AllocString(apValue);
            pProtocol->pVariables[aMSDP]->bDirty = true;
         }
      }
   }
}

void MSDPSetTable( descriptor_t *apDescriptor, variable_t aMSDP, const char *apValue )
{
   protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

   if ( pProtocol != NULL && apValue != NULL )
   {
      if ( *apValue == '\0' )
      {
         /* It's easier to call MSDPSetString if the value is empty */
         MSDPSetString(apDescriptor, aMSDP, apValue);
      }
      else if ( VariableNameTable[aMSDP].bString )
      {
         const char MsdpTableStart[] = { (char)MSDP_TABLE_OPEN, '\0' };
         const char MsdpTableStop[]  = { (char)MSDP_TABLE_CLOSE, '\0' };

         char *pTable = (char *) malloc(strlen(apValue) + 3); /* 3: START, STOP, NUL */

         strcpy(pTable, MsdpTableStart);
         strcat(pTable, apValue);
         strcat(pTable, MsdpTableStop);

         if ( strcmp(pProtocol->pVariables[aMSDP]->pValueString, pTable) )
         {
            free(pProtocol->pVariables[aMSDP]->pValueString);
            pProtocol->pVariables[aMSDP]->pValueString = pTable;
            pProtocol->pVariables[aMSDP]->bDirty = true;
         }
         else /* Just discard the table, we've already got one */
         {
            free(pTable);
         }
      }
   }
}

void MSDPSetArray( descriptor_t *apDescriptor, variable_t aMSDP, const char *apValue )
{
   protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

   if ( pProtocol != NULL && apValue != NULL )
   {
      if ( *apValue == '\0' )
      {
         /* It's easier to call MSDPSetString if the value is empty */
         MSDPSetString(apDescriptor, aMSDP, apValue);
      }
      else if ( VariableNameTable[aMSDP].bString )
      {
         const char MsdpArrayStart[] = { (char)MSDP_ARRAY_OPEN, '\0' };
         const char MsdpArrayStop[]  = { (char)MSDP_ARRAY_CLOSE, '\0' };

         char *pArray = (char *) malloc(strlen(apValue) + 3); /* 3: START, STOP, NUL */

         strcpy(pArray, MsdpArrayStart);
         strcat(pArray, apValue);
         strcat(pArray, MsdpArrayStop);

         if ( strcmp(pProtocol->pVariables[aMSDP]->pValueString, pArray) )
         {
            free(pProtocol->pVariables[aMSDP]->pValueString);
            pProtocol->pVariables[aMSDP]->pValueString = pArray;
            pProtocol->pVariables[aMSDP]->bDirty = true;
         }
         else /* Just discard the array, we've already got one */
         {
            free(pArray);
         }
      }
   }
}

/******************************************************************************
 MSSP global functions.
 ******************************************************************************/

void MSSPSetPlayers( int aPlayers )
{
   s_Players = aPlayers;

   if ( s_Uptime == 0 )
      s_Uptime = time(0);
}

/******************************************************************************
 MXP global functions.
 ******************************************************************************/

const char *MXPCreateTag( descriptor_t *apDescriptor, const char *apTag )
{
   protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

   if ( pProtocol != NULL && pProtocol->pVariables[eMSDP_MXP]->ValueInt && 
      strlen(apTag) < 1000 )
   {
      static char MXPBuffer [1024];
      sprintf( MXPBuffer, "\033[1z%s\033[7z", apTag );
      return MXPBuffer;
   }
   else /* Leave the tag as-is, don't try to MXPify it */
   {
      return apTag;
   }
}

void MXPSendTag( descriptor_t *apDescriptor, const char *apTag )
{
   protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

   if ( pProtocol != NULL && pProtocol->pVariables[eMSDP_MXP]->ValueInt && 
      strlen(apTag) < 1000 )
   {
      char MXPBuffer [1024];
      sprintf(MXPBuffer, "\033[1z%s\033[7z\r\n", apTag );
      Write(apDescriptor, MXPBuffer);
   }
}

/******************************************************************************
 Sound global functions.
 ******************************************************************************/

void SoundSend( descriptor_t *apDescriptor, const char *apTrigger )
{
   const int MaxTriggerLength = 128; /* Used for the buffer size */

   if ( apDescriptor != NULL && apTrigger != NULL )
   {
      protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

      if ( pProtocol != NULL && pProtocol->pVariables[eMSDP_SOUND]->ValueInt )
      {
         if ( pProtocol->bMSDP || pProtocol->bATCP )
         {
            /* Send the sound trigger through MSDP or ATCP */
            MSDPSendPair( apDescriptor, "PLAY_SOUND", apTrigger );
         }
         else if ( strlen(apTrigger) <= MaxTriggerLength )
         {
            /* Use an old MSP-style trigger */
            char *pBuffer = alloca(MaxTriggerLength+10);
            sprintf( pBuffer, "\t!SOUND(%s)", apTrigger );
            Write(apDescriptor, pBuffer);
         }
      }
   }
}

/******************************************************************************
 Colour global functions.
 ******************************************************************************/

const char *ColourRGB( descriptor_t *apDescriptor, const char *apRGB )
{
   protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;
   bool charHasColor = TRUE;
   
   if (apDescriptor->character && !clr(apDescriptor->character, C_CMP))
     charHasColor = FALSE;

   if ( pProtocol && pProtocol->pVariables[eMSDP_ANSI_COLORS]->ValueInt && charHasColor )
   {
      if ( IsValidColour(apRGB) )
      {
         bool_t bBackground = (tolower(apRGB[0]) == 'b');
         int Red = apRGB[1] - '0';
         int Green = apRGB[2] - '0';
         int Blue = apRGB[3] - '0';

         if ( pProtocol->pVariables[eMSDP_XTERM_256_COLORS]->ValueInt )
            return GetRGBColour( bBackground, Red, Green, Blue );
         else /* Use regular ANSI colour */
            return GetAnsiColour( bBackground, Red, Green, Blue );
      }
      else /* Invalid colour - use this to clear any existing colour. */
      {
         return s_Clean;
      }
   }
   else /* Don't send any colour, not even clear */
   {
      return "";
   }
}

/******************************************************************************
 UTF-8 global functions.
 ******************************************************************************/

char *UnicodeGet( int aValue )
{
   static char Buffer[8];
   char *pString = Buffer;

   UnicodeAdd( &pString, aValue );
   *pString = '\0';

   return Buffer;
}

void UnicodeAdd( char **apString, int aValue )
{
   if ( aValue < 0x80 )
   {
      *(*apString)++ = (char)aValue;
   }
   else if ( aValue < 0x800 )
   {
      *(*apString)++ = (char)(0xC0 | (aValue>>6));
      *(*apString)++ = (char)(0x80 | (aValue & 0x3F));
   }
   else if ( aValue < 0x10000 )
   {
      *(*apString)++ = (char)(0xE0 | (aValue>>12));
      *(*apString)++ = (char)(0x80 | (aValue>>6 & 0x3F));
      *(*apString)++ = (char)(0x80 | (aValue & 0x3F));
   }
   else if ( aValue < 0x200000 )
   {
      *(*apString)++ = (char)(0xF0 | (aValue>>18));
      *(*apString)++ = (char)(0x80 | (aValue>>12 & 0x3F));
      *(*apString)++ = (char)(0x80 | (aValue>>6 & 0x3F));
      *(*apString)++ = (char)(0x80 | (aValue & 0x3F));
   }
}

/******************************************************************************
 Local negotiation functions.
 ******************************************************************************/

static void Negotiate( descriptor_t *apDescriptor )
{
   protocol_t *pProtocol = apDescriptor->pProtocol;

   if ( pProtocol->bNegotiated )
   {
      const char RequestTTYPE   [] = { (char)IAC, (char)SB,   TELOPT_TTYPE, SEND, (char)IAC, (char)SE, '\0' };
      const char DoNAWS         [] = { (char)IAC, (char)DO,   TELOPT_NAWS,      '\0' };
      const char DoCHARSET      [] = { (char)IAC, (char)DO,   TELOPT_CHARSET,   '\0' };
      const char WillMSDP       [] = { (char)IAC, (char)WILL, TELOPT_MSDP,      '\0' };
      const char WillMSSP       [] = { (char)IAC, (char)WILL, TELOPT_MSSP,      '\0' };
      const char DoATCP         [] = { (char)IAC, (char)DO,   (char)TELOPT_ATCP,'\0' };
      const char WillMSP        [] = { (char)IAC, (char)WILL, TELOPT_MSP,       '\0' };
      const char DoMXP          [] = { (char)IAC, (char)DO,   TELOPT_MXP,       '\0' };

#ifdef USING_MCCP
      const char WillMCCP       [] = { (char)IAC, (char)WILL, TELOPT_MCCP,      '\0' };
#endif // USING_MCCP

      /* Request the client type if TTYPE is supported. */
      if ( pProtocol->bTTYPE )
         Write(apDescriptor, RequestTTYPE);

      /* Check for other protocols. */
      Write(apDescriptor, DoNAWS);
      /* Gnome-mud seems to have an issue with negotiating DoCHARSET under
       * certain conditions cause it to crash. This is a GNOME-MUD issue
       * and not an issue on our end. Never-the-less, if you discover you
       * are having this issue you can either a) disable protocol negotiation
       * in cedit, or b) disable detection of CHARSET by deleting/commenting
       * the following line.
       * 
       * For more information on gnome-mud's bug see:
       * https://bugs.launchpad.net/ubuntu/+source/gnome-mud/+bug/398340 */
      Write(apDescriptor, DoCHARSET);
      Write(apDescriptor, WillMSDP);
      Write(apDescriptor, WillMSSP);
      Write(apDescriptor, DoATCP);
      Write(apDescriptor, WillMSP);
      Write(apDescriptor, DoMXP);

#ifdef USING_MCCP
      Write(apDescriptor, WillMCCP);
#endif // USING_MCCP
   }
}

static void PerformHandshake( descriptor_t *apDescriptor, char aCmd, char aProtocol )
{
   bool_t bResult = true;
   protocol_t *pProtocol = apDescriptor->pProtocol;

   switch ( aProtocol )
   {
      case (char)TELOPT_TTYPE:
         if ( aCmd == (char)WILL )
         {
            if ( !pProtocol->bNegotiated )
            {
               /* Negotiate for the remaining protocols. */
               pProtocol->bNegotiated = true;
               pProtocol->bTTYPE = true;
               Negotiate(apDescriptor);
            }
         }
         else if ( aCmd == (char)WONT )
         {
            if ( !pProtocol->bNegotiated )
            {
               /* Still negotiate, as this client obviously knows how to 
                * correctly respond to negotiation attempts - but we don't 
                * ask for TTYPE, as it doesn't support it.
                */
               pProtocol->bNegotiated = true;
               pProtocol->bTTYPE = false;
               Negotiate(apDescriptor);
            }
         }
         else /* Anything else is invalid. */
            bResult = false;
         break;

      case (char)TELOPT_NAWS:
         if ( aCmd == (char)WILL )
            pProtocol->bNAWS = true;
         else if ( aCmd == (char)WONT )
            pProtocol->bNAWS = false;
         else /* Anything else is invalid. */
            bResult = false;
         break;

      case (char)TELOPT_CHARSET:
         if ( aCmd == (char)WILL )
         {
            char charset_utf8 [] = { (char)IAC, (char)SB, TELOPT_CHARSET, 1, ' ', 'U', 'T', 'F', '-', '8', (char)IAC, (char)SE, '\0' };
            Write(apDescriptor, charset_utf8);
            pProtocol->bCHARSET = true;
         }
         else if ( aCmd == (char)WONT )
            pProtocol->bCHARSET = false;
         else /* Anything else is invalid. */
            bResult = false;
         break;

      case (char)TELOPT_MSDP:
         if ( aCmd == (char)DO )
         {
            pProtocol->bMSDP = true;

            /* Identify the mud to the client. */
            MSDPSendPair( apDescriptor, "SERVER_ID", MUD_NAME );
         }
         else if ( aCmd == (char)DONT )
            pProtocol->bMSDP = false;
         else /* Anything else is invalid. */
            bResult = false;
         break;

      case (char)TELOPT_MSSP:
         if ( aCmd == (char)DO )
            SendMSSP( apDescriptor );
         else if ( aCmd == (char)DONT )
            ; /* Do nothing. */
         else /* Anything else is invalid. */
            bResult = false;
         break;

      case (char)TELOPT_MCCP:
         if ( aCmd == (char)DO )
         {
            pProtocol->bMCCP = true;
            CompressStart( apDescriptor );
         }
         else if ( aCmd == (char)DONT )
         {
            pProtocol->bMCCP = false;
            CompressEnd( apDescriptor );
         }
         else // Anything else is invalid.
            bResult = false;
         break;

      case (char)TELOPT_MSP:
         if ( aCmd == (char)DO )
            pProtocol->bMSP = true;
         else if ( aCmd == (char)DONT )
            pProtocol->bMSP = false;
         else /* Anything else is invalid. */
            bResult = false;
         break;

      case (char)TELOPT_MXP:
         if ( aCmd == (char)WILL || aCmd == (char)DO )
         {
            /* Enable MXP. */
            const char EnableMXP[] = { (char)IAC, (char)SB, TELOPT_MXP, (char)IAC, (char)SE, '\0' };
            Write(apDescriptor, EnableMXP);

            /* Create a secure channel, and note that MXP is active. */
            Write(apDescriptor, "\033[7z");
            pProtocol->bMXP = true;
            pProtocol->pVariables[eMSDP_MXP]->ValueInt = 1;
         }
         else if ( aCmd == (char)WONT )
         {
            if ( !pProtocol->bMXP )
            {
               /* The MXP standard doesn't actually specify whether you should 
                * negotiate with IAC DO MXP or IAC WILL MXP.  As a result, some 
                * clients support one, some the other, and some support both.
                * 
                * Therefore we first try IAC DO MXP, and if the client replies 
                * with WONT, we try again (here) with IAC WILL MXP.
                */
               const char WillMXP[] = { (char)IAC, (char)WILL, TELOPT_MXP, '\0' };
               Write(apDescriptor, WillMXP);
            }
            else // The client is actually asking us to switch MXP off.
            {
               pProtocol->bMXP = false;
            }
         }
         else if ( aCmd == (char)DONT )
            pProtocol->bMXP = false;
         else /* Anything else is invalid. */
            bResult = false;
         break;

      case (char)TELOPT_ATCP:
         if ( aCmd == (char)WILL )
         {
            /* If we don't support MSDP, fake it with ATCP */
            if ( !pProtocol->bMSDP )
            {
               pProtocol->bATCP = true;

#ifdef MUDLET_PACKAGE
               /* Send the Mudlet GUI package to the user. */
               if ( MatchString( "Mudlet",
                  pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString ) )
               {
                  SendATCP( apDescriptor, "Client.GUI", MUDLET_PACKAGE );
               }
#endif /* MUDLET_PACKAGE */

               /* Identify the mud to the client. */
               MSDPSendPair( apDescriptor, "SERVER_ID", MUD_NAME );
            }
         }
         else if ( aCmd == (char)WONT )
            pProtocol->bATCP = false;
         else /* Anything else is invalid. */
            bResult = false;
         break;

      default:
         bResult = false;
   }
}

static void PerformSubnegotiation( descriptor_t *apDescriptor, char aCmd, char *apData, int aSize )
{
   protocol_t *pProtocol = apDescriptor->pProtocol;

   switch ( aCmd )
   {
      case (char)TELOPT_TTYPE:
         if ( pProtocol->bTTYPE )
         {
            /* Store the client name. */
            const int MaxClientLength = 64;
            char *pClientName = alloca(MaxClientLength+1);
            int i = 0, j = 1;
            bool_t bStopCyclicTTYPE = false;

            for ( ; apData[j] != '\0' && i < MaxClientLength; ++j )
            {
               if ( isprint(apData[j]) )
                  pClientName[i++] = apData[j];
            }
            pClientName[i] = '\0';

            /* Store the first TTYPE as the client name */
            if ( !strcmp(pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString, "Unknown") )
            {
               free(pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString);
               pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString = AllocString(pClientName);

               /* This is a bit nasty, but using cyclic TTYPE on windows telnet 
                * causes it to lock up.  None of the clients we need to cycle 
                * with send ANSI to start with anyway, so we shouldn't have any 
                * conflicts.
                * 
                * An alternative solution is to use escape sequences to check 
                * for windows telnet prior to negotiation, and this also avoids 
                * the player losing echo, but it has other issues.  Because the 
                * escape codes are technically in-band, even though they'll be 
                * stripped from the display, the newlines will still cause some 
                * scrolling.  Therefore you need to either pause the session 
                * for a few seconds before displaying the login screen, or wait 
                * until the player has entered their name before negotiating.
                */
               if ( !strcmp(pClientName,"ANSI") )
                  bStopCyclicTTYPE = true;
            }

            /* Cycle through the TTYPEs until we get the same result twice, or 
             * find ourselves back at the start.
             * 
             * If the client follows RFC1091 properly then it will indicate the 
             * end of the list by repeating the last response, and then return 
             * to the top of the list.  If you're the trusting type, then feel 
             * free to remove the second strcmp ;)
             */
            if ( pProtocol->pLastTTYPE == NULL || 
               (strcmp(pProtocol->pLastTTYPE, pClientName) && 
               strcmp(pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString, pClientName)) )
            {
               char RequestTTYPE [] = { (char)IAC, (char)SB, TELOPT_TTYPE, SEND, (char)IAC, (char)SE, '\0' };
               const char *pStartPos = strstr( pClientName, "-" );

               /* Store the TTYPE */
               if (pProtocol->pLastTTYPE)
                 free(pProtocol->pLastTTYPE);
               pProtocol->pLastTTYPE = AllocString(pClientName);

               /* Look for 256 colour support */
               if ( (pStartPos != NULL && MatchString(pStartPos, "-256color")) || MatchString(pClientName, "xterm") )
               {
                  /* This is currently the only way to detect support for 256 
                   * colours in TinTin++, WinTin++ and BlowTorch.
                   */
                  pProtocol->pVariables[eMSDP_XTERM_256_COLORS]->ValueInt = 1;
                  pProtocol->b256Support = eYES;
               }

               /* Request another TTYPE */
               if ( !bStopCyclicTTYPE )
                  Write(apDescriptor, RequestTTYPE);
            }

            if ( PrefixString("Mudlet", pClientName) )
            {
               /* Mudlet beta 15 and later supports 256 colours, but we can't 
                * identify it from the mud - everything prior to 1.1 claims 
                * to be version 1.0, so we just don't know.
                */ 
               pProtocol->b256Support = eSOMETIMES;

               if ( strlen(pClientName) > 7 )
               {
                  pClientName[6] = '\0';
                  free(pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString);
                  pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString = AllocString(pClientName);
                  free(pProtocol->pVariables[eMSDP_CLIENT_VERSION]->pValueString);
                  pProtocol->pVariables[eMSDP_CLIENT_VERSION]->pValueString = AllocString(pClientName+7);

                  /* Mudlet 1.1 and later supports 256 colours. */
                  if ( strcmp(pProtocol->pVariables[eMSDP_CLIENT_VERSION]->pValueString, "1.1") >= 0 )
                  {
                     pProtocol->pVariables[eMSDP_XTERM_256_COLORS]->ValueInt = 1;
                     pProtocol->b256Support = eYES;
                  }
               }
            }
            else if ( MatchString(pClientName, "EMACS-RINZAI") )
            {
               /* We know for certain that this client has support */
               pProtocol->pVariables[eMSDP_XTERM_256_COLORS]->ValueInt = 1;
               pProtocol->b256Support = eYES;
            }
            else if ( PrefixString("DecafMUD", pClientName) )
            {
               /* We know for certain that this client has support */
               pProtocol->pVariables[eMSDP_XTERM_256_COLORS]->ValueInt = 1;
               pProtocol->b256Support = eYES;

               if ( strlen(pClientName) > 9 )
               {
                  pClientName[8] = '\0';
                  free(pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString);
                  pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString = AllocString(pClientName);
                  free(pProtocol->pVariables[eMSDP_CLIENT_VERSION]->pValueString);
                  pProtocol->pVariables[eMSDP_CLIENT_VERSION]->pValueString = AllocString(pClientName+9);
               }
            }
            else if ( MatchString(pClientName, "MUSHCLIENT") || 
               MatchString(pClientName, "CMUD") || 
               MatchString(pClientName, "ATLANTIS") || 
               MatchString(pClientName, "KILDCLIENT") || 
               MatchString(pClientName, "TINTIN++") || 
               MatchString(pClientName, "TINYFUGUE") )
            {
               /* We know that some versions of this client have support */
               pProtocol->b256Support = eSOMETIMES;
            }
            else if ( MatchString(pClientName, "ZMUD") )
            {
               /* We know for certain that this client does not have support */
               pProtocol->b256Support = eNO;
            }
         }
         break;

      case (char)TELOPT_NAWS:
         if ( pProtocol->bNAWS )
         {
            /* Store the new width. */
            pProtocol->ScreenWidth = (unsigned char)apData[0];
            pProtocol->ScreenWidth <<= 8;
            pProtocol->ScreenWidth += (unsigned char)apData[1];

            /* Store the new height. */
            pProtocol->ScreenHeight = (unsigned char)apData[2];
            pProtocol->ScreenHeight <<= 8;
            pProtocol->ScreenHeight += (unsigned char)apData[3];
         }
         break;

      case (char)TELOPT_CHARSET:
         if ( pProtocol->bCHARSET )
         {
            /* Because we're only asking about UTF-8, we can just check the 
             * first character.  If you ask for more than one CHARSET you'll 
             * need to read through the results to see which are accepted.
             *
             * Note that the user must also use a unicode font!
             */
            if ( apData[0] == ACCEPTED )
               pProtocol->pVariables[eMSDP_UTF_8]->ValueInt = 1;
         }
         break;

      case (char)TELOPT_MSDP:
         ParseMSDP( apDescriptor, apData );
         break;

      case (char)TELOPT_ATCP:
         ParseATCP( apDescriptor, apData );
         break;

      default: /* Unknown subnegotiation, so we simply ignore it. */
         break;
   }
}

/******************************************************************************
 Local MSDP functions.
 ******************************************************************************/

static void ParseMSDP( descriptor_t *apDescriptor, const char *apData )
{
   char Variable[MSDP_VAL][MAX_MSDP_SIZE+1] = { {'\0'}, {'\0'} };
   char *pPos = NULL, *pStart = NULL;

   while ( *apData )
   {
      switch ( *apData )
      {
         case MSDP_VAR: case MSDP_VAL:
            pPos = pStart = Variable[*apData++-1];
            break;
         default: /* Anything else */
            if ( pPos && pPos-pStart < MAX_MSDP_SIZE )
            {
               *pPos++ = *apData;
               *pPos = '\0';
            }

            if ( *++apData )
               continue;
      }

      ExecuteMSDPPair( apDescriptor, Variable[MSDP_VAR-1], Variable[MSDP_VAL-1] );
      Variable[MSDP_VAL-1][0] = '\0';
   }
}

static void ExecuteMSDPPair( descriptor_t *apDescriptor, const char *apVariable, const char *apValue )
{
   if ( apVariable[0] != '\0' && apValue[0] != '\0' )
   {
      if ( MatchString(apVariable, "SEND") )
      {
         bool_t bDone = false;
         int i; /* Loop counter */
         for ( i = eMSDP_NONE+1; i < eMSDP_MAX && !bDone; ++i )
         {
            if ( MatchString(apValue, VariableNameTable[i].pName) )
            {
               MSDPSend( apDescriptor, (variable_t)i );
               bDone = true;
            }
         }
      }
      else if ( MatchString(apVariable, "REPORT") )
      {
         bool_t bDone = false;
         int i; /* Loop counter */
         for ( i = eMSDP_NONE+1; i < eMSDP_MAX && !bDone; ++i )
         {
            if ( MatchString(apValue, VariableNameTable[i].pName) )
            {
               apDescriptor->pProtocol->pVariables[i]->bReport = true;
               apDescriptor->pProtocol->pVariables[i]->bDirty = true;
               bDone = true;
            }
         }
      }
      else if ( MatchString(apVariable, "RESET") )
      {
         if ( MatchString(apValue, "REPORTABLE_VARIABLES") || 
            MatchString(apValue, "REPORTED_VARIABLES") )
         {
            int i; /* Loop counter */
            for ( i = eMSDP_NONE+1; i < eMSDP_MAX; ++i )
            {
               if ( apDescriptor->pProtocol->pVariables[i]->bReport )
               {
                  apDescriptor->pProtocol->pVariables[i]->bReport = false;
                  apDescriptor->pProtocol->pVariables[i]->bDirty = false;
               }
            }
         }
      }
      else if ( MatchString(apVariable, "UNREPORT") )
      {
         bool_t bDone = false;
         int i; /* Loop counter */
         for ( i = eMSDP_NONE+1; i < eMSDP_MAX && !bDone; ++i )
         {
            if ( MatchString(apValue, VariableNameTable[i].pName) )
            {
               apDescriptor->pProtocol->pVariables[i]->bReport = false;
               apDescriptor->pProtocol->pVariables[i]->bDirty = false;
               bDone = true;
            }
         }
      }
      else if ( MatchString(apVariable, "LIST") )
      {
         if ( MatchString(apValue, "COMMANDS") )
         {
            const char MSDPCommands[] = "LIST REPORT RESET SEND UNREPORT";
            MSDPSendList( apDescriptor, "COMMANDS", MSDPCommands );
         }
         else if ( MatchString(apValue, "LISTS") )
         {
            const char MSDPCommands[] = "COMMANDS LISTS CONFIGURABLE_VARIABLES REPORTABLE_VARIABLES REPORTED_VARIABLES SENDABLE_VARIABLES GUI_VARIABLES";
            MSDPSendList( apDescriptor, "LISTS", MSDPCommands );
         }
         /* Split this into two if some variables aren't REPORTABLE */
         else if ( MatchString(apValue, "SENDABLE_VARIABLES") || 
            MatchString(apValue, "REPORTABLE_VARIABLES") )
         {
            char MSDPCommands[MAX_OUTPUT_BUFFER] = { '\0' };
            int i; /* Loop counter */

            for ( i = eMSDP_NONE+1; i < eMSDP_MAX; ++i )
            {
               if ( !VariableNameTable[i].bGUI )
               {
                  /* Add the separator between variables */
                  strcat( MSDPCommands, " " );

                  /* Add the variable to the list */
                  strcat( MSDPCommands, VariableNameTable[i].pName );
               }
            }

            MSDPSendList( apDescriptor, apValue, MSDPCommands );
         }
         else if ( MatchString(apValue, "REPORTED_VARIABLES") )
         {
            char MSDPCommands[MAX_OUTPUT_BUFFER] = { '\0' };
            int i; /* Loop counter */

            for ( i = eMSDP_NONE+1; i < eMSDP_MAX; ++i )
            {
               if ( apDescriptor->pProtocol->pVariables[i]->bReport )
               {
                  /* Add the separator between variables */
                  if ( MSDPCommands[0] != '\0' )
                     strcat( MSDPCommands, " " );

                  /* Add the variable to the list */
                  strcat( MSDPCommands, VariableNameTable[i].pName );
               }
            }

            MSDPSendList( apDescriptor, apValue, MSDPCommands );
         }
         else if ( MatchString(apValue, "CONFIGURABLE_VARIABLES") )
         {
            char MSDPCommands[MAX_OUTPUT_BUFFER] = { '\0' };
            int i; /* Loop counter */

            for ( i = eMSDP_NONE+1; i < eMSDP_MAX; ++i )
            {
               if ( VariableNameTable[i].bConfigurable )
               {
                  /* Add the separator between variables */
                  if ( MSDPCommands[0] != '\0' )
                     strcat( MSDPCommands, " " );

                  /* Add the variable to the list */
                  strcat( MSDPCommands, VariableNameTable[i].pName );
               }
            }

            MSDPSendList( apDescriptor, "CONFIGURABLE_VARIABLES", MSDPCommands );
         }
         else if ( MatchString(apValue, "GUI_VARIABLES") )
         {
            char MSDPCommands[MAX_OUTPUT_BUFFER] = { '\0' };
            int i; /* Loop counter */

            for ( i = eMSDP_NONE+1; i < eMSDP_MAX; ++i )
            {
               if ( VariableNameTable[i].bGUI )
               {
                  /* Add the separator between variables */
                  if ( MSDPCommands[0] != '\0' )
                     strcat( MSDPCommands, " " );

                  /* Add the variable to the list */
                  strcat( MSDPCommands, VariableNameTable[i].pName );
               }
            }

            MSDPSendList( apDescriptor, apValue, MSDPCommands );
         }
      }
      else /* Set any configurable variables */
      {
         int i; /* Loop counter */

         for ( i = eMSDP_NONE+1; i < eMSDP_MAX; ++i )
         {
            if ( VariableNameTable[i].bConfigurable )
            {
               if ( MatchString(apVariable, VariableNameTable[i].pName) )
               {
                  if ( VariableNameTable[i].bString )
                  {
                     /* A write-once variable can only be set if the value 
                      * is "Unknown".  This is for things like client name, 
                      * where we don't really want the player overwriting a 
                      * proper client name with junk - but on the other hand, 
                      * its possible a client may choose to use MSDP to 
                      * identify itself.
                      */
                     if ( !VariableNameTable[i].bWriteOnce || 
                        !strcmp(apDescriptor->pProtocol->pVariables[i]->pValueString, "Unknown") )
                     {
                        /* Store the new value if it's valid */
                        char *pBuffer = alloca(VariableNameTable[i].Max+1);
                        int j; /* Loop counter */

                        for ( j = 0; j < VariableNameTable[i].Max && *apValue != '\0'; ++apValue )
                        {
                           if ( isprint(*apValue) )
                              pBuffer[j++] = *apValue;
                        }
                        pBuffer[j++] = '\0';

                        if ( j >= VariableNameTable[i].Min )
                        {
                           free(apDescriptor->pProtocol->pVariables[i]->pValueString);
                           apDescriptor->pProtocol->pVariables[i]->pValueString = AllocString(pBuffer);
                        }
                     }
                  }
                  else /* This variable only accepts numeric values */
                  {
                     /* Strip any leading spaces */
                     while ( *apValue == ' ' )
                        ++apValue;

                     if ( *apValue != '\0' && IsNumber(apValue) )
                     {
                        int Value = atoi(apValue);
                        if ( Value >= VariableNameTable[i].Min && 
                           Value <= VariableNameTable[i].Max )
                        {
                           apDescriptor->pProtocol->pVariables[i]->ValueInt = Value;
                        }
                     }
                  }
               }
            }
         }
      }
   }
}

/******************************************************************************
 Local ATCP functions.
 ******************************************************************************/

static void ParseATCP( descriptor_t *apDescriptor, const char *apData )
{
   char Variable[MSDP_VAL][MAX_MSDP_SIZE+1] = { {'\0'}, {'\0'} };
   char *pPos = NULL, *pStart = NULL;

   while ( *apData )
   {
      switch ( *apData )
      {
         case '@': 
            pPos = pStart = Variable[0];
            apData++;
            break;
         case ' ': 
            pPos = pStart = Variable[1];
            apData++;
            break;
         default: /* Anything else */
            if ( pPos && pPos-pStart < MAX_MSDP_SIZE )
            {
               *pPos++ = *apData;
               *pPos = '\0';
            }

            if ( *++apData )
               continue;
      }

      ExecuteMSDPPair( apDescriptor, Variable[MSDP_VAR-1], Variable[MSDP_VAL-1] );
      Variable[MSDP_VAL-1][0] = '\0';
   }
}

#ifdef MUDLET_PACKAGE
static void SendATCP( descriptor_t *apDescriptor, const char *apVariable, const char *apValue )
{
   char ATCPBuffer[MAX_VARIABLE_LENGTH+1] = { '\0' };

   if ( apVariable != NULL && apValue != NULL )
   {
      protocol_t *pProtocol = apDescriptor ? apDescriptor->pProtocol : NULL;

      /* Should really be replaced with a dynamic buffer */
      int RequiredBuffer = strlen(apVariable) + strlen(apValue) + 12;

      if ( RequiredBuffer >= MAX_VARIABLE_LENGTH )
      {
         if ( RequiredBuffer - strlen(apValue) < MAX_VARIABLE_LENGTH )
         {
            sprintf( ATCPBuffer, 
               "SendATCP: %s %d bytes (exceeds MAX_VARIABLE_LENGTH of %d).\n", 
               apVariable, RequiredBuffer, MAX_VARIABLE_LENGTH );
         }
         else /* The variable name itself is too long */
         {
            sprintf( ATCPBuffer, 
               "SendATCP: Variable name has a length of %d bytes (exceeds MAX_VARIABLE_LENGTH of %d).\n", 
               RequiredBuffer, MAX_VARIABLE_LENGTH );
         }

         ReportBug( ATCPBuffer );
         ATCPBuffer[0] = '\0';
      }
      else if ( pProtocol->bATCP )
      {
         sprintf( ATCPBuffer, "%c%c%c%s %s%c%c", 
            IAC, SB, TELOPT_ATCP, apVariable, apValue, IAC, SE );
      }

      /* Just in case someone calls this function without checking ATCP */
      if ( ATCPBuffer[0] != '\0' )
         Write( apDescriptor, ATCPBuffer );
   }
}
#endif /* MUDLET_PACKAGE */

/******************************************************************************
 Local MSSP functions.
 ******************************************************************************/

static const char *GetMSSP_Players()
{
   static char Buffer[32];
   sprintf( Buffer, "%d", s_Players );
   return Buffer;
}

static const char *GetMSSP_Uptime()
{
   static char Buffer[32];
   sprintf( Buffer, "%d", (int)s_Uptime );
   return Buffer;
}

/* Macro for readability, but you can remove it if you don't like it */
#define FUNCTION_CALL(f) "", f

static void SendMSSP( descriptor_t *apDescriptor )
{
   char MSSPBuffer[MAX_MSSP_BUFFER];
   char MSSPPair[128];
   int SizeBuffer = 3; /* IAC SB MSSP */
   int i; /* Loop counter */

   /* Before updating the following table, please read the MSSP specification:
    *
    * http://tintin.sourceforge.net/mssp/
    *
    * It's important that you use the correct format and spelling for the MSSP 
    * variables, otherwise crawlers may reject the data as invalid.
    */
   static MSSP_t MSSPTable[] =
   {
      /* Required */
      { "NAME",               MUD_NAME, NULL },   /* Change this in protocol.h */
      { "PLAYERS",            FUNCTION_CALL( GetMSSP_Players ) },
      { "UPTIME" ,            FUNCTION_CALL( GetMSSP_Uptime ) }, 

      /* Generic */
      { "CRAWL DELAY",        "-1", NULL },
/*
      { "HOSTNAME",           "" },
      { "PORT",               "" },
      { "CODEBASE",           "" },
      { "CONTACT",            "" },
      { "CREATED",            "" },
      { "ICON",               "" },
      { "IP",                 "" },
      { "LANGUAGE",           "" },
      { "LOCATION",           "" },
      { "MINIMUM AGE",        "" },
      { "WEBSITE",            "" },
*/
      /* Categorisation */
/*
      { "FAMILY",             "" },
      { "GENRE",              "" },
      { "GAMEPLAY",           "" },
      { "STATUS",             "" },
      { "GAMESYSTEM",         "" },
      { "INTERMUD",           "" },
      { "SUBGENRE",           "" },
*/
      /* World */
/*
      { "AREAS",              "0" },
      { "HELPFILES",          "0" },
      { "MOBILES",            "0" },
      { "OBJECTS",            "0" },
      { "ROOMS",              "0" },
      { "CLASSES",            "0" },
      { "LEVELS",             "0" },
      { "RACES",              "0" },
      { "SKILLS",             "0" },
*/
      /* Protocols */
/*
      { "ANSI",               "1" },
      { "GMCP",               "0" },
#ifdef USING_MCCP
      { "MCCP",               "1" },
#else
      { "MCCP",               "0" },
#endif // USING_MCCP
      { "MCP",                "0" },
      { "MSDP",               "1" },
      { "MSP",                "1" },
      { "MXP",                "1" },
      { "PUEBLO",             "0" },
      { "UTF-8",              "1" },
      { "VT100",              "0" },
      { "XTERM 256 COLORS",   "1" },
*/
      /* Commercial */
/*
      { "PAY TO PLAY",        "0" },
      { "PAY FOR PERKS",      "0" },
*/
      /* Hiring */
/*
      { "HIRING BUILDERS",    "0" },
      { "HIRING CODERS",      "0" },
*/
      /* Extended variables */

      /* World */
/*
      { "DBSIZE",             "0" },
      { "EXITS",              "0" },
      { "EXTRA DESCRIPTIONS", "0" },
      { "MUDPROGS",           "0" },
      { "MUDTRIGS",           "0" },
      { "RESETS",             "0" },
*/
      /* Game */
/*
      { "ADULT MATERIAL",     "0" },
      { "MULTICLASSING",      "0" },
      { "NEWBIE FRIENDLY",    "0" },
      { "PLAYER CITIES",      "0" },
      { "PLAYER CLANS",       "0" },
      { "PLAYER CRAFTING",    "0" },
      { "PLAYER GUILDS",      "0" },
      { "EQUIPMENT SYSTEM",   "" },
      { "MULTIPLAYING",       "" },
      { "PLAYERKILLING",      "" },
      { "QUEST SYSTEM",       "" },
      { "ROLEPLAYING",        "" },
      { "TRAINING SYSTEM",    "" },
      { "WORLD ORIGINALITY",  "" },
*/
      /* Protocols */
/*
      { "ATCP",               "1" },
      { "SSL",                "0" },
      { "ZMP",                "0" },
*/
      { NULL, NULL, NULL } /* This must always be last. */
   };

   /* Begin the subnegotiation sequence */
   sprintf( MSSPBuffer, "%c%c%c", IAC, SB, TELOPT_MSSP );

   for ( i = 0; MSSPTable[i].pName != NULL; ++i )
   {
      int SizePair;

      /* Retrieve the next MSSP variable/value pair */
      sprintf( MSSPPair, "%c%s%c%s", MSSP_VAR, MSSPTable[i].pName, MSSP_VAL, 
         MSSPTable[i].pFunction ? (*MSSPTable[i].pFunction)() : 
         MSSPTable[i].pValue );

      /* Make sure we don't overflow the buffer */
      SizePair = strlen(MSSPPair);
      if ( SizePair+SizeBuffer < MAX_MSSP_BUFFER-4 )
      {
         strcat( MSSPBuffer, MSSPPair );
         SizeBuffer += SizePair;
      }
   }

   /* End the subnegotiation sequence */
   sprintf( MSSPPair, "%c%c", IAC, SE );
   strcat( MSSPBuffer, MSSPPair );

   /* Send the sequence */
   Write( apDescriptor, MSSPBuffer );
}

/******************************************************************************
 Local MXP functions.
 ******************************************************************************/

static char *GetMxpTag( const char *apTag, const char *apText )
{
   static char MXPBuffer [64];
   const char *pStartPos = strstr(apText, apTag);

   if ( pStartPos != NULL )
   {
      const char *pEndPos = apText+strlen(apText);

      pStartPos += strlen(apTag); /* Add length of the tag */

      if ( pStartPos < pEndPos )
      {
         int Index = 0;

         /* Some clients use quotes...and some don't. */
         if ( *pStartPos == '\"' )
            pStartPos++;

         for ( ; pStartPos < pEndPos && Index < 60; ++pStartPos )
         {
            char Letter = *pStartPos;
            if ( Letter == '.' || isdigit(Letter) || isalpha(Letter) )
            {
               MXPBuffer[Index++] = Letter;
            }
            else /* Return the result */
            {
               MXPBuffer[Index] = '\0';
               return MXPBuffer;
            }
         }
      }
   }

   /* Return NULL to indicate no tag was found. */
   return NULL;
}

/******************************************************************************
 Local colour functions.
 ******************************************************************************/

static const char *GetAnsiColour( bool_t abBackground, int aRed, int aGreen, int aBlue )
{
   if ( aRed == aGreen && aRed == aBlue && aRed < 2)
      return abBackground ? s_BackBlack : aRed >= 1 ? s_BoldBlack : s_DarkBlack;
   else if ( aRed == aGreen && aRed == aBlue )
      return abBackground ? s_BackWhite : aRed >= 4 ? s_BoldWhite : s_DarkWhite;
   else if ( aRed > aGreen && aRed > aBlue )
      return abBackground ? s_BackRed : aRed > 3 ? s_BoldRed : s_DarkRed;
   else if ( aRed == aGreen && aRed > aBlue )
      return abBackground ? s_BackYellow : aRed > 3 ? s_BoldYellow : s_DarkYellow;
   else if ( aRed == aBlue && aRed > aGreen )
      return abBackground ? s_BackMagenta : aRed >= 3 ? s_BoldMagenta : s_DarkMagenta;
   else if ( aGreen > aBlue )
      return abBackground ? s_BackGreen : aGreen >= 3 ? s_BoldGreen : s_DarkGreen;
   else if ( aGreen == aBlue )
      return abBackground ? s_BackCyan : aGreen >= 3 ? s_BoldCyan : s_DarkCyan;
   else /* aBlue is the highest */
      return abBackground ? s_BackBlue : aBlue >= 3 ? s_BoldBlue : s_DarkBlue;
}

static const char *GetRGBColour( bool_t abBackground, int aRed, int aGreen, int aBlue )
{
   static char Result[16];
   int ColVal = 16 + (aRed * 36) + (aGreen * 6) + aBlue;
   sprintf( Result, "\033[%c8;5;%c%c%cm", 
      '3'+abBackground,      /* Background */
      '0'+(ColVal/100),      /* Red        */
      '0'+((ColVal%100)/10), /* Green      */
      '0'+(ColVal%10) );     /* Blue       */
   return Result;
}

static bool_t IsValidColour( const char *apArgument )
{
   int i; /* Loop counter */

   /* The sequence is 4 bytes, but we can ignore anything after it. */
   if ( apArgument == NULL || strlen(apArgument) < 4 )
      return false;

   /* The first byte indicates foreground/background. */
   if ( tolower(apArgument[0]) != 'f' && tolower(apArgument[0]) != 'b' )
      return false;

   /* The remaining three bytes must each be in the range '0' to '5'. */
   for ( i = 1; i <= 3; ++i )
   {
      if ( apArgument[i] < '0' || apArgument[i] > '5' )
         return false;
   }

   /* It's a valid foreground or background colour */
   return true;
}

/******************************************************************************
 Other local functions.
 ******************************************************************************/

static bool_t MatchString( const char *apFirst, const char *apSecond )
{
   while ( *apFirst && tolower(*apFirst) == tolower(*apSecond) ) {
      ++apFirst;
      ++apSecond;
   }
   return ( !*apFirst && !*apSecond );
}

static bool_t PrefixString( const char *apPart, const char *apWhole )
{
   while ( *apPart && tolower(*apPart) == tolower(*apWhole) ) {
      ++apPart;
      ++apWhole;
   }
   return ( !*apPart );
}

static bool_t IsNumber( const char *apString )
{
   while ( *apString && isdigit(*apString) )
      ++apString;
   return ( !*apString );
}

static char *AllocString( const char *apString )
{
   char *pResult = NULL;

   if ( apString != NULL )
   {
      int Size = strlen(apString);
      pResult = (char *) malloc(Size+1);
      if ( pResult != NULL )
         strcpy( pResult, apString );
   }

   return pResult;
}
