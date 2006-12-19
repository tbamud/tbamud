#!perl

##############################################################################
# Portions of this script are Copyright 1998 by dean takemori.
#
# This is a MacPerl script which attempts to impliment autorun functionality
# for MacOS.  It's a bit of a bastardisation of MacPerl and "standard" unix
# style perl.  I won't claim that it's pretty.  Parts of it come from the
# Autorun perl script for unix that is distributed with CircleMUD and it is
# therefore subject to the following.

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

# .fastboot     sleep for only 5 seconds, instead of the default 20
# .killscript   script will exit, and mud will not reboot
# .pause        pause (sleep 1 minute intervals) until .pause it removed
##############################################################################

use 5.004;
use Mac::MoreFiles;
use Mac::Processes;

# launchContinue() must be set or MacPerl will quit after LaunchApplication().
# The creator code "dtCM" is registered with Apple Computer for CircleMUD.
# However, if you have multiple copies of Circle on disk (a production MUD
# and a testing MUD for example), you may wish to specify by FULL PATH,
# which application to run.

$spec = $Application{'dtCM'};
#$spec = "volume:apps:myapp";

my($app);
$app = new LaunchParam(
                       'launchControlFlags' => launchContinue(),
                       'launchAppSpec'      => $spec
                      ) or die ($^E);

my($psn, $spec_found);
while (1)
  {
    LaunchApplication($app) or die($^E);
    do
      {
        # Now, every 60 seconds, scan the running processes for one whose
        # AppSpec matches the one we set up earlier.
        sleep(60);
        $spec_found = 0;
        foreach $psn (keys(%Process))
          {
            if ( $spec eq $Process{$psn}->processAppSpec() )
              {
                $spec_found = 1;
                break; # out of the foreach
              }
          }
      } while ($spec_found == 1);

    # If we're here, then the App is no longer running.
    # First we open everything
    open (SYSLOG, ":syslog") or die($^E);
    open (DELETE, ">>:log:delete");
    open (DTRAPS, ">>:log:dtraps");
    open (DEATHS, ">>:log:deaths");
    open (REBOOT, ">>:log:reboots");
    open (LEVELS, ">>:log:levels");
    open (NORENT, ">>:log:norent");
    open (USAGE,  ">>:log:usage");
    open (NEWPLR, ">>:log:newplrs");
    open (SYSERR, ">>:log:errors");
    open (GODCMD, ">>:log:godcmds");
    open (BADPWS, ">>:log:badpws");

    # Then we stash everything
    while (<SYSLOG>)
      {
        print DELETE if /self-delete/;
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
    if (-e ":log:syslog.6") { unlink (":log:syslog.6"); }
    if (-e ":log:syslog.6") { rename (":log:syslog.5", ":log:syslog.6"); }
    if (-e ":log:syslog.6") { rename (":log:syslog.4", ":log:syslog.5"); }
    if (-e ":log:syslog.6") { rename (":log:syslog.3", ":log:syslog.4"); }
    if (-e ":log:syslog.6") { rename (":log:syslog.2", ":log:syslog.3"); }
    if (-e ":log:syslog.6") { rename (":log:syslog.1", ":log:syslog.2"); }
    rename (":syslog" , ":log:syslog.1");

    # should we stay dead?
    if (-r ".killscript")
      {
        unlink(".killscript");
        open (SYSLOG, ">>:log:syslog.1");
        print (SYSLOG "autoscript killed ", localtime());  #seconds since epoch
        exit;
      }

    # or just play dead?
    while (-r ".pause")
    { sleep(60); }

    # or reboot as soon as possible?
    if (-r ".fastboot")
      {
        unlink(".fastboot");
        sleep(5);
      }
    else
      { sleep(20); }
  }
