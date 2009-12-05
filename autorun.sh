#!/bin/sh
#
# CircleMUD 3.0 autorun script
# Contributions by Fred Merkel, Stuart Lamble, and Jeremy Elson
# Copyright (c) 1996 The Trustees of The Johns Hopkins University
# All Rights Reserved
# See license.doc for more information
#
#############################################################################
#
# This script can be used to run CircleMUD over and over again (i.e., have it
# automatically reboot if it crashes).  It will run the game, and copy some
# of the more useful information from the system logs to the 'log' directory
# for safe keeping.
#
# You can control the operation of this script by creating and deleting files
# in Circle's root directory, either manually or by using the 'shutdown'
# command from within the MUD.
#
# Creating a file called .fastboot makes the script wait only 5 seconds
# between reboot attempts instead of the usual 60.  If you want a quick
# reboot, use the "shutdown reboot" command from within the MUD.
#
# Creating a file called .killscript makes the script terminate (i.e., stop
# rebooting the MUD).  If you want to shut down the MUD and make it stay
# shut down, use the "shutdown die" command from within the MUD.
#
# Finally, if a file called pause exists, the script will not reboot the MUD
# again until pause is removed.  This is useful if you want to turn the MUD
# off for a couple of minutes and then bring it back up without killing the
# script.  Type "shutdown pause" from within the MUD to activate this feature.
#
ulimit -c unlimited

# The port on which to run the MUD
PORT=4000

# Default flags to pass to the MUD server (see admin.txt for a description
# of all flags).
FLAGS='-q'

#############################################################################

while ( : ) do

  DATE=`date`
  echo "autorun starting game $DATE" >> syslog
  echo "running bin/circle $FLAGS $PORT" >> syslog

  bin/circle $FLAGS $PORT >> syslog 2>&1

  tail -30 syslog > syslog.CRASH

  fgrep "self-delete" syslog >> log/delete
  fgrep "PCLEAN" syslog >> log/delete
  fgrep "death trap" syslog >> log/dts
  fgrep "killed" syslog >> log/rip
  fgrep "Running" syslog >> log/restarts
  fgrep "advanced" syslog >> log/levels
  fgrep "equipment lost" syslog >> log/rentgone
  fgrep "usage" syslog >> log/usage
  fgrep "new player" syslog >> log/newplayers
  fgrep "SYSERR" syslog >> log/errors
  fgrep "(GC)" syslog >> log/godcmds
  fgrep "Bad PW" syslog >> log/badpws

  rm log/syslog.1
  mv log/syslog.2 log/syslog.1
  mv log/syslog.3 log/syslog.2
  mv log/syslog.4 log/syslog.3
  mv log/syslog.5 log/syslog.4
  mv log/syslog.6 log/syslog.5
  mv syslog       log/syslog.6
  touch syslog

  if [ -r .killscript ]; then
    DATE=`date`;
    echo "autoscript terminated $DATE"  >> syslog
    rm .killscript
    exit
  fi

  if [ ! -r .fastboot ]; then
    sleep 60
  else
    rm .fastboot
    sleep 5
  fi

  while [ -r pause ]; do
    sleep 60
  done

  if [ -s lib/core ]; then
    gdb bin/circle lib/core -command gdb.tmp >lib/backtrace.$(date +%d.%m.%Y.%T)
  fi
done
