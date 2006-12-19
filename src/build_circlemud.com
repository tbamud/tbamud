$!
$! BUILD_CIRCLEMUD.COM
$! Written By:  Robert Alan Byer
$!              byer@mail.ourservers.net
$!
$! This script checks the file names and then compiles and links CircleMUD for
$! OpenVMS using DEC C and the DEC C TCP/IP socket routines.
$!
$! The script accepts the following parameters.
$!
$!	P1	ALL		Build Everything.
$!		CIRCLE		Just Build [-.BIN]CIRCLE.EXE.
$!		UTILS		Just Build The CircleMUD Utilities.
$!
$!	P2	DEBUG		Build With Debugger Information.
$!		NODEBUG		Build Withoug Debugger Information.
$!
$! The default is "ALL" and "NODEBUG".
$!
$! Check To Make Sure We Have Valid Command Line Parameters.
$!
$ GOSUB CHECK_OPTIONS
$!
$! Check To See If We Are On An AXP Machine.
$!
$ IF (F$GETSYI("CPU").LT.128)
$ THEN
$!
$!  We Are On A VAX Machine So Tell The User.
$!
$   WRITE SYS$OUTPUT "Compiling On A VAX Machine."
$!
$! Else, We Are On An AXP Machine.
$!
$ ELSE
$!
$!  We Are On A AXP Machine So Tell The User.
$!
$   WRITE SYS$OUTPUT "Compiling On A AXP Machine."
$!
$! End Of The Machine Check.
$!
$ ENDIF
$!
$! Check The CONF.H File.
$!
$ GOSUB CHECK_CONF_FILE
$!
$! Check Filenames.
$!
$ GOSUB CHECK_FILE_NAMES
$!
$! Check To See What We Are To Do.
$!
$ IF (BUILDALL.EQS."TRUE")
$ THEN
$!
$!  Since Nothing Special Was Specified, Build Everything.
$!
$   GOSUB BUILD_CIRCLE
$   GOSUB BUILD_UTILS
$!
$! Else...
$!
$ ELSE
$!
$!  Build Just What The User Wants Us To Build.
$!
$   GOSUB BUILD_'BUILDALL'
$!
$! Time To End The Build Check.
$!
$ ENDIF
$!
$! Time To EXIT.
$!
$ EXIT
$!
$! Build [-.BIN]CIRCLE.EXE
$!
$ BUILD_CIRCLE:
$!
$! Tell The User What We Are Doing.
$!
$ WRITE SYS$OUTPUT ""
$ WRITE SYS$OUTPUT "Compiling [-.BIN]CIRCLE.EXE"
$ WRITE SYS$OUTPUT ""
$!
$! Define The CIRCLE.EXE Files That Are Necessary.
$!
$ CIRCLE_FILES = "ACT_COMM,ACT_INFORMATIVE,ACT_ITEM,ACT_MOVEMENT," + -
                 "ACT_OFFENSIVE,ACT_OTHER,ACT_SOCIAL,ACT_WIZARD," + -
                 "ALIAS,BAN,BOARDS,CASTLE,CLASS,COMM,CONFIG," + -
                 "CONSTANTS,DB,FIGHT,GRAPH,HANDLER,HOUSE," + -
                 "INTERPRETER,LIMITS,MAGIC,MAIL,MOBACT,MODIFY," + -
                 "OBJSAVE,OLC,PLAYERS,RANDOM,SHOP,SPEC_ASSIGN," + -
                 "SPEC_PROCS,SPELLS,SPELL_PARSER,UTILS,WEATHER"
$!
$!  Define A File Counter And Set It To "0".
$!
$ CIRCLE_FILE_COUNTER = 0
$!
$! Top Of The File Loop.
$!
$ NEXT_CIRCLE_FILE:
$!
$! O.K, Extract The File Name From The File List.
$!
$ CIRCLE_FILE_NAME = F$ELEMENT(CIRCLE_FILE_COUNTER,",",CIRCLE_FILES)
$!
$! Check To See If We Are At The End Of The File List.
$!
$ IF (CIRCLE_FILE_NAME.EQS.",") THEN GOTO CIRCLE_FILE_DONE
$!
$! Increment The Counter.
$!
$ CIRCLE_FILE_COUNTER = CIRCLE_FILE_COUNTER + 1
$!
$! Create The Source File Name.
$!
$ CIRCLE_SOURCE_FILE = "SYS$DISK:[]" + CIRCLE_FILE_NAME + ".C"
$!
$! Create The Object File Name.
$!
$ CIRCLE_OBJECT_FILE = "SYS$DISK:[]" + CIRCLE_FILE_NAME + ".OBJ"
$!
$! Check To See If The File We Want To Compile Actually Exists.
$!
$ IF (F$SEARCH(CIRCLE_SOURCE_FILE).EQS."")
$ THEN
$!
$!  Tell The User That The File Dosen't Exist.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "The File ",CIRCLE_SOURCE_FILE," Dosen't Exist."
$   WRITE SYS$OUTPUT ""
$!
$!  Exit The Build.
$!
$   EXIT
$!
$! End The File Check.
$!
$ ENDIF
$!
$! Tell The User What We Are Compiling.
$!
$  WRITE SYS$OUTPUT "	",CIRCLE_SOURCE_FILE
$!
$! Compile The File.
$!
$ CC/PREFIX=ALL/'OPTIMIZE'/'DEBUGGER'/DEFINE=("DECC=1") -
    /OBJECT='CIRCLE_OBJECT_FILE' 'CIRCLE_SOURCE_FILE'
$!
$! Go Back And Do It Again.
$!
$ GOTO NEXT_CIRCLE_FILE
$!
$! All Done Compiling.
$!
$ CIRCLE_FILE_DONE:
$!
$! Tell The User We Are Linking [-.BIN]CIRCLE.EXE
$!
$ WRITE SYS$OUTPUT ""
$ WRITE SYS$OUTPUT "Linking [-.BIN]CIRCLE.EXE"
$ WRITE SYS$OUTPUT ""
$!
$! Link [-.BIN]CIRCLE.EXE
$!
$ LINK/'DEBUGGER'/'TRACEBACK'/EXE=[-.BIN]CIRCLE.EXE -
      COMM.OBJ,ACT_COMM.OBJ,ACT_INFORMATIVE.OBJ,ACT_ITEM.OBJ, -
      ACT_MOVEMENT.OBJ,ACT_OFFENSIVE.OBJ,ACT_OTHER.OBJ, -
      ACT_SOCIAL.OBJ,ACT_WIZARD.OBJ,ALIAS.OBJ,BAN.OBJ,BOARDS.OBJ, -
      CASTLE.OBJ,CLASS.OBJ,CONFIG.OBJ,CONSTANTS.OBJ,DB.OBJ,FIGHT.OBJ, -
      GRAPH.OBJ,HANDLER.OBJ,HOUSE.OBJ,INTERPRETER.OBJ,LIMITS.OBJ,MAGIC.OBJ, -
      MAIL.OBJ,MOBACT.OBJ,MODIFY.OBJ,OBJSAVE.OBJ,PLAYERS.OBJ,OLC.OBJ, -
      RANDOM.OBJ,SHOP.OBJ,SPEC_ASSIGN.OBJ,SPEC_PROCS.OBJ,SPELLS.OBJ, -
      SPELL_PARSER.OBJ,UTILS.OBJ,WEATHER.OBJ$!
$! That's It, Time To Return From Where We Came From.
$!
$ RETURN
$!
$! Build The CircleMUD Utilities.
$!
$ BUILD_UTILS:
$!
$! Tell The User What We Are Doing.
$!
$ WRITE SYS$OUTPUT ""
$ WRITE SYS$OUTPUT "Building CircleMUD Utilities."
$ WRITE SYS$OUTPUT ""
$!
$! Define The Source Files That Are Necessary.
$!
$ UTIL_FILES = "ASCIIPASSWD,LISTRENT,PLRTOASCII,SHOPCONV,SPLIT," + -
                "WLD2HTML"
$!
$!  Define A File Counter And Set It To "0".
$!
$ UTIL_FILE_COUNTER = 0
$!
$! Top Of The File Loop.
$!
$ NEXT_UTIL_FILE:
$!
$! O.K, Extract The File Name From The File List.
$!
$ UTIL_FILE_NAME = F$ELEMENT(UTIL_FILE_COUNTER,",",UTIL_FILES)
$!
$! Check To See If We Are At The End Of The File List.
$!
$ IF (UTIL_FILE_NAME.EQS.",") THEN GOTO UTIL_FILE_DONE
$!
$! Increment The Counter.
$!
$ UTIL_FILE_COUNTER = UTIL_FILE_COUNTER + 1
$!
$! Create The Source File Name.
$!
$ UTIL_SOURCE_FILE = "SYS$DISK:[.UTIL]" + UTIL_FILE_NAME + ".C"
$!
$! Create The Object File Name.
$!
$ UTIL_OBJECT_FILE = "SYS$DISK:[.UTIL]" + UTIL_FILE_NAME + ".OBJ"
$!
$! Check To See If The File We Want To Compile Actually Exists.
$!
$ IF (F$SEARCH(UTIL_SOURCE_FILE).EQS."")
$ THEN
$!
$!  Tell The User That The File Dosen't Exist.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "The File ",UTIL_SOURCE_FILE," Dosen't Exist."
$   WRITE SYS$OUTPUT ""
$!
$!  Exit The Build.
$!
$   EXIT
$ ENDIF
$!
$! Tell The User What We Are Building.
$!
$  WRITE SYS$OUTPUT "Building SYS$DISK:[-.BIN]",UTIL_FILE_NAME,".EXE"
$!
$! Compile The File.
$!
$ CC/PREFIX=ALL/STANDARD=ANSI89/'OPTIMIZE'/'DEBUGGER'/DEFINE=("DECC=1") -
    /INCLUDE=SYS$DISK:[]/OBJECT='UTIL_OBJECT_FILE' 'UTIL_SOURCE_FILE'
$!
$! Link The File.
$!
$ LINK/'DEBUGGER'/'TRACEBACK'/EXE=[-.BIN]'UTIL_FILE_NAME'.EXE -
      'UTIL_OBJECT_FILE'
$!
$! Go Back And Do It Again.
$!
$ GOTO NEXT_UTIL_FILE
$!
$! All Done Compiling.
$!
$ UTIL_FILE_DONE:
$!
$! That's It, Time To Return From Where We Came From.
$!
$ RETURN
$!
$! Check The User's Options.
$!
$ CHECK_OPTIONS:
$!
$! Check To See If We Are To "Just Build Everything".
$!
$ IF ((P1.EQS."").OR.(P1.EQS."ALL"))
$ THEN
$!
$!   P1 Is "ALL", So Build Everything.
$!
$    BUILDALL = "TRUE"
$!
$! Else....
$!
$ ELSE
$!
$!  Check To See If P1 Has A Valid Arguement.
$!
$   IF (P1.EQS."CIRCLE").OR.(P1.EQS."UTILS")
$   THEN
$!
$!    A Valid Arguement.
$!
$     BUILDALL = P1
$!
$!  Else...
$!
$   ELSE
$!
$!    Tell The User We Don't Know What They Want.
$!
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "The Option ",P1," Is Invalid.  The Valid Options Are:"
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "    ALL     :  Just Build Everything."
$     WRITE SYS$OUTPUT "    CIRCLE  :  Just Build [-.BIN]CIRCLE.EXE."
$     WRITE SYS$OUTPUT "    UTILS   :  Just Build The CircleMUD Utilities."
$     WRITE SYS$OUTPUT ""
$!
$!    Time To EXIT.
$!
$     EXIT
$   ENDIF
$ ENDIF
$!
$! Check To See If We Are To Compile Without Debugger Information.
$!
$ IF ((P2.EQS."").OR.(P2.EQS."NODEBUG"))
$ THEN
$!
$!  P2 Is Either Blank Or "NODEBUG" So Compile Without Debugger Information.
$!
$   DEBUGGER  = "NODEBUG"
$   OPTIMIZE = "OPTIMIZE"
$   TRACEBACK = "NOTRACEBACK"
$!
$!  Tell The User What They Selected.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "No Debugger Information Will Be Produced During Compile."
$   WRITE SYS$OUTPUT "Compiling With Compiler Optimization."
$ ELSE
$!
$!  Check To See If We Are To Compile With Debugger Information.
$!
$   IF (P2.EQS."DEBUG")
$   THEN
$!
$!    Compile With Debugger Information.
$!
$     DEBUGGER  = "DEBUG"
$     OPTIMIZE = "NOOPTIMIZE"
$     TRACEBACK = "TRACEBACK"
$!
$!    Tell The User What They Selected.
$!
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "Debugger Information Will Be Produced During Compile."
$     WRITE SYS$OUTPUT "Compiling Without Compiler Optimization."
$!
$!  Else...
$!
$   ELSE
$!
$!    Tell The User Entered An Invalid Option..
$!
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "The Option ",P2," Is Invalid.  The Valid Options Are:"
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "    DEBUG    :  Compile With The Debugger Information."
$     WRITE SYS$OUTPUT "    NODEBUG  :  Compile Without The Debugger Information."
$     WRITE SYS$OUTPUT ""
$!
$!    Time To EXIT.
$!
$     EXIT
$   ENDIF
$ ENDIF
$!
$! Time To Return To Where We Were.
$!
$ RETURN
$!
$! Subroutine To Check CONF.H File.
$!
$ CHECK_CONF_FILE:
$!
$! Tell The User We Are Checking CONF.H File.
$!
$ WRITE SYS$OUTPUT "Checking The CONF.H File."
$!
$! Check To See If The CONF.H File Exists.
$!
$ IF (F$SEARCH("SYS$DISK:[]CONF.H").EQS."")
$ THEN
$!
$!  The File Dosen't Exist So Check To See If The CONF.H_VMS File Exists. 
$!
$   IF (F$SEARCH("SYS$DISK:[]CONF.H_VMS").NES."")
$   THEN
$!
$!    Copy CONF.H_VMS To CONF.H.
$!
$     COPY SYS$DISK:[]CONF.H_VMS SYS$DISK:[]CONF.H
$!
$!  Else....
$!
$   ELSE
$!
$!    Check To See If The CONF_H.VMS File Exists.
$!
$     IF (F$SEARCH("SYS$DISK:[]CONF_H.VMS").NES."")
$     THEN
$!
$!      Copy CONF_H.VMS To CONF.H.
$!
$       COPY SYS$DISK:[]CONF_H.VMS SYS$DISK:[]CONF.H
$!
$!    Else...
$!
$     ELSE
$!
$!      The CONF.H_VMS And The CONF_H.VMS File Dosen't Exist, So Tell The User.
$!
$       WRITE SYS$OUTPUT ""
$       WRITE SYS$OUTPUT "The file CONF.H_VMS or CONF_H.VMS  dosen't exist. This file is"
$       WRITE SYS$OUTPUT "necessary to compile CircleMUD and is distributed"
$       WRITE SYS$OUTPUT "with the source code."
$       WRITE SYS$OUTPUT ""
$       WRITE SYS$OUTPUT "Since the CONF.H_VMS or CONF_H.VMS file is distributed with the"
$       WRITE SYS$OUTPUT "source files I recomend that you unpack the files"
$       WRITE SYS$OUTPUT "again or get a new source distribution."
$       WRITE SYS$OUTPUT ""
$!
$!      Since We Can't Compile Without This File, Just EXIT.
$!
$       EXIT
$!
$!    Time To End The CONF_H.VMS File Check.
$!
$     ENDIF
$!
$!  Time To End The CONF.H_VMS File Check.
$!
$   ENDIF
$!
$! End The CONF.H Check.
$!
$ ENDIF
$!
$! Time To Return To Where We Were.
$!
$ RETURN
$!
$! Subroutine To Check File Names.
$!
$ CHECK_FILE_NAMES:
$!
$! Tell The User We Are Checking File Names.
$!
$ WRITE SYS$OUTPUT "Checking File Names."
$!
$! Define The File Names We Need To Check On.
$!
$ CHECK_FOR = "ACT.COMM_C,ACT.INFORMATIVE_C,ACT.ITEM_C,ACT.MOVEMENT_C," + -
              "ACT.OFFENSIVE_C,ACT.OTHER_C,ACT.SOCIAL_C,ACT.WIZARD_C"
$!
$! Define What The File Names Need To Be.
$!
$ SHOULD_BE = "ACT_COMM.C,ACT_INFORMATIVE.C,ACT_ITEM.C,ACT_MOVEMENT.C," + -
              "ACT_OFFENSIVE.C,ACT_OTHER.C,ACT_SOCIAL.C,ACT_WIZARD.C"
$!
$!  Define A File Counter And Set It To "0".
$!
$ FILE_COUNTER = 0
$!
$! Top Of The File Loop.
$!
$ CHECK_NEXT_FILE:
$!
$! O.K, Extract The File Name We Are Looking For From The List.
$!
$ LOOKING_FOR = F$ELEMENT(FILE_COUNTER,",",CHECK_FOR)
$!
$! Extract The File Name It SHOULD Be From The List.
$!
$ RENAME_TO = F$ELEMENT(FILE_COUNTER,",",SHOULD_BE)
$!
$! Check To See If We Are At The End Of The File List.
$!
$ IF (LOOKING_FOR.EQS.",") THEN GOTO CHECK_FILES_DONE
$!
$! Increment The Counter.
$!
$ FILE_COUNTER = FILE_COUNTER + 1
$!
$! Check To See If The File We Are Checking For Exists.
$!
$ IF (F$SEARCH(LOOKING_FOR).EQS."")
$ THEN
$!
$!  The File Dosen't Exist, Check For The Next File.
$!
$   GOTO CHECK_NEXT_FILE
$!
$! Else...
$!
$ ELSE
$!
$!  The File Exists And Needs To Be Fixed.
$!
$   RENAME 'LOOKING_FOR' 'RENAME_TO'
$!
$! End The File Check.
$!
$ ENDIF
$!
$! Go Back And Do It Again.
$!
$ GOTO CHECK_NEXT_FILE
$!
$! All Done With Checking File Names.
$!
$ CHECK_FILES_DONE:
$!
$! Time To Return To Where We Were.
$!
$ RETURN
