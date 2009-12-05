#!/usr/bin/perl
# autorun -- maintain a Circle V3.0 mud server
# Copyright (c)1995 Vi'Rage Studios

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

# csh original by Fred C. Merkel, PERL conversion by D. Hall

# .fastboot	sleep for only 5 seconds, instead of the default 20
# .killscript	script will exit, and mud will not reboot
# .pause	pause (sleep 1 minute intervals) until .pause it removed

$port	= 4000;
$flags	= "-q";

$home	= ".";
$bin	= "$home/bin";
$lib	= "$home/lib";
$log	= "$home/log";

chdir $home;

while (1) {

  # Open SYSLOG and dup STDERR into SYSLOG
  open (SYSLOG, ">> syslog");
  open (STDERR, ">& SYSLOG");
  
  print SYSLOG "autoscript starting game ", `date`;
  open (SERVER, "bin/circle $flags $port |");
  
  while (<SERVER>) {
    print SYSLOG;
  }
  
  # First we open everything
  open (SYSLOG, "<syslog");
  open (DELETE, ">>log/delete");
  open (DTRAPS, ">>log/dtraps");
  open (DEATHS, ">>log/deaths");
  open (REBOOT, ">>log/reboots");
  open (LEVELS, ">>log/levels");
  open (NORENT, ">>log/norent");
  open (USAGE,  ">>log/usage");
  open (NEWPLR, ">>log/newplrs");
  open (SYSERR, ">>log/errors");
  open (GODCMD, ">>log/godcmds");
  open (BADPWS, ">>log/badpws");
  
  # Then we stash everything
  while (<SYSLOG>) {
    print DELETE if /self-delete/;
    print DELETE if /PCLEAN/;
    print DTRAPS if /death trap/;
    print DEATHS if /killed/;
    print REBOOT if /Running/;
    print LEVELS if /advanced/;
    print NORENT if /equipment lost/;
    print USAGE  if /usage/;
    print NEWPLR if /new player/;
    print SYSERR if /SYSERR/;
    print GODCMD if /\(GC\)/;
    print BADPWs if /Bad PW/;
  }
  close (SYSLOG);
  
  # Rotate SYSLOG files
  unlink ('log/syslog.6');
  rename ('log/syslog.5', 'log/syslog.6');
  rename ('log/syslog.4', 'log/syslog.5');
  rename ('log/syslog.3', 'log/syslog.4');
  rename ('log/syslog.2', 'log/syslog.3');
  rename ('log/syslog.1', 'log/syslog.2');
  rename ('syslog'      , 'log/syslog.1');
  
  # should we stay dead?
  if (-r '.killscript') {
    unlink '.killscript';
    open (SYSLOG, '>> log/syslog.1');
    print SYSLOG "autoscript terminated ", `date`;
    exit;
  }
  
  # or just play dead?
  while (-r '.pause') {
    sleep 60;
  }

  # or reboot as soon as possible?
  if (-r '.fastboot') {
    unlink '.fastboot';
    sleep 5;
  } else {
    sleep 20
  }
}
