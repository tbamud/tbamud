/* REXX */
/* Note: This is an example Autorun REXX Script for use with OS/2 and
   CircleMUD 3.0.  You may use it as is, or as the basis for your own
   script.
   April 18, 1996 - David A. Carver */
   
/* CircleMUD 3.0 autorun script
 * Originally by Fred C. Merkel
 * Copyright (c) 1993 The Trustees of The Johns Hopkins University
 * All Rights Reserved
 * See license.doc for more information
 *
 * If .fastboot exists, the script will sleep for only 5 seconds between
 * reboot attempts.  If .killscript exists, the script commit suicide (and
 * remove .killscript).  If pause exists, the script will repeatedly sleep for
 *  60 seconds and will not restart the mud until pause is removed.
 */

'echo off'

PORT=4000
FLAGS=''

call RxFuncAdd "SysLoadFuncs", "RexxUtil", "SysLoadFuncs"
call SysLoadFuncs

Do forever
  call SysCls
  say center('CrapWeasel MUD',79)
  say center('AutoRun Procedure',79)
  'del syslog'  
  Say "AutoRun starting game " || DATE()
  "set EMXOPT=-h150"

  "bin\circle " FLAGS PORT || " >> syslog"
  

  say 'Extracting little log information'
  'del log\*.* /n'   
  'fgrep -w "self-delete" syslog >> log/delete'
  'fgrep -w "PCLEAN" syslog >> log/delete'
  'fgrep -w "death trap" syslog >> log/dts'
  'fgrep -w "killed" syslog >> log/rip'
  'fgrep -w "Running" syslog >> log/restarts'
  'fgrep -w "advanced" syslog >> log/levels'
  'fgrep -w "equipment lost" syslog >> log/rentgone'
  'fgrep -w "usage" syslog >> log/usage'
  'fgrep -w "olc" syslog >> log/olc'
  'fgrep -w "new player" syslog >> log/newplayers'
  'fgrep -w "SYSERR" syslog >> log/errors'
  'fgrep -w "(GC)" syslog >> log/godcmds'
  'fgrep -w "Bad PW" syslog >> log/badpws'
  'fgrep -w "has connected" syslog >> log/whocon'
    
  Do while stream("pause","c","query exists")<>""
    Say "Pausing..."
    Call SysSleep(10)
  end

  if (stream("fastboot","c","query exists")="") then do
    Say "Waiting 40 seconds to reboot"
    Call SysSleep(40)
  end
  else do
    "del fastboot"
    Say "Waiting 5 seconds to reboot"
    Call SysSleep(5)
  end

  if (stream("killscr","c","query exists")<>"") then do
    Say "Exiting autorun"
    "echo autoscript terminated "DATE() ">> syslog"
    "del killscr"
    exit
  end
end
