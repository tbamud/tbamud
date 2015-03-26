Although you will be compiling tbaMUD, this document is still relevant for
recent versions of UNIX and LINUX.

                     Compiling CircleMUD under UNIX
                            by Jeremy Elson 
                 For help, write to help@circlemud.org

Compiling CircleMUD is easy using almost any variant of UNIX (for
example: Linux/MkLinux, Solaris, SunOS, IRIX, HP/UX, Ultrix, OSF, and
NetBSD/OpenBSD/FreeBSD/BSDI, just to name a few).  If you have any type
of UNIX system, these instructions are for you.  If not, check the main
README file to get a list of other operating systems that can be used to
compile and run CircleMUD.

Current versions of Circle use the GNU 'autoconf' package to
automatically determine most of the important characteristics of your
system, so chances are good that Circle will compile correctly on any
UNIX system -- even one that we have never seen before.  However, if you
do have problems, please drop us a note at help@circlemud.org so that we
can try to make Circle work with every UNIX variant that we can.

1)  You can also find information at the WWW site:

        http://www.circlemud.org/
        Old FTP mirror: http://www.circlemud.org/pub/CircleMUD/

    The latest version will be called something ending in .tar.gz, like
    "circle30bplXX.tar.gz". (where 'XX' is the patchlevel)


2)  Unpack the archive.  If you have the .tar.gz version, uncompress it
    using gzip (GNU unzip) and the tar archiver.  (Both of these utilities
    can be downloaded from ftp.gnu.ai.mit.edu:/pub/gnu if you don't have
    them.)  To unpack the archive on a UNIX system, type:

        gzip -dc circle30xxxx.tar.gz | tar xvf -



3)  Configure CircleMUD for your system.  Circle must be configured using
    the 'configure' program which attempts to guess correct values for
    various system-dependent variables used during compilation.  It uses
    those values to create a 'Makefile' and a header file called 'conf.h'.

    *** Macintosh OS X only:
      Open Makefile.in in the src directory and make the following change:
      
      Find the line:         MYFLAGS = @MYFLAGS@
      Change it to:          MYFLAGS = -DCIRCLE_OS_X
    ***
    
    From Circle's root directory, type

        ./configure

    If you're using 'csh' on an old version of System V, csh might try to
    execute 'configure' itself, giving you a message like "Permission denied"
    when you try to run "./configure".  If so, type "sh ./configure" instead.

    'configure' can take several minutes if you're using a slow computer.

    'configure' will attempt to use the 'gcc' compiler if it exists; if not,
    it will try 'cc'.  If you want to use a different compiler, set the
    'CC' environment variable to the name of the compiler you wish to use.

    For example, if you want to use the 'xlc' compiler, and your shell is
    csh or tcsh:

	setenv CC xlc
	./configure

    Or, if you want to use the 'xlc' compiler, and your shell is sh or bash:

	CC=xlc ./configure

    This will tell 'configure' to use the 'xlc' compiler instead of 'gcc'.


4)  Build the CircleMUD server.  This must be done from the 'src' directory.
    Type:
   
        cd src; make all

    This will build CircleMUD proper as well as its 10 or so ancillary
    utilities, which can take anywhere from 5 minutes to an hour depending
    on the speed of your computer.

    Note that in the future, when you need to recompile Circle as you make
    changes to the code, it is NOT necessary to run 'configure' again (it
    should only be run once, after the first time you unpack Circle from
    its .tar file).  If you move the source code to a different computer,
    you should reconfigure it by deleting the file 'config.cache' and
    running 'configure' again.

    The first time you try to compile Circle, you will be asked to read the
    CircleMUD license.  Please read it!


5)  Go back to Circle's root directory (by typing "cd ..") and run the
    CircleMUD server.  The easiest way to do this the first time is
    to use the 'autorun' script, which can be run in the background by
    typing:

        ./autorun &

    Make sure to do this in Circle's root directory, not the src directory
    that you used for the previous step.  A file called 'syslog' will start
    growing in the same directory that contains Circle's log messages.

    If you're using 'csh' on an old version of System V, csh might try to
    execute 'autorun' itself, giving you a message like "Permission denied"
    when you try to run "./autorun".  If so, type "sh ./autorun &" instead.

6)  Wait until the line 'No connections.  Going to sleep.' appears in the
    syslog.  This indicates that the server is ready and waiting for
    connections.  It shouldn't take more than about 30 seconds for the MUD
    to reach this state, though performance will vary depending on how fast
    your computer is.

    If a file appears called 'syslog.CRASH', the MUD has terminated
    (probably abnormally).  Check the contents of syslog.CRASH to see
    what error was encountered.


7)  Type 'telnet localhost 4000' to connect.  The first person to log in
    will be made an implementor (level 34) with all powers.


Jeremy Elson
(write to help@circlemud.org for help)
