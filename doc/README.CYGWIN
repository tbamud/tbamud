If you have any additions, corrections, ideas, or bug reports please stop by the
Builder Academy at telnet://tbamud.com:9091 or email rumble@tbamud.com -- Rumble

                            Compiling tbaMUD
                  under Microsoft Windows using CygWin
Originally by: Jeremy Elson, David Goldstein, Tony Robbins, and George Greer.

tbaMUD compiles under Cygwin without needing any special modifications.
This free pseudo-Unix environment for Windows includes the "bash" shell, gcc, 
and a full set of programs and libraries for Windows users to compile and run 
programs from source code intended for Unix. It can be downloaded from: 
http://cygwin.com/

Compiling tbaMUD under Cygwin is basically the same as it would be under
another form of Unix:

1) Download from: http://cygwin.com/setup.exe
   Select open.
   Install from Internet.
   Choose a root directory, I use C:\
   Select "just me" "Unix / binary"
   Select a local package directory, I use C:\WINDOWS\Temp
   Leave Direct Connection selected.
   Choose a download site.
   You can either click the "circular arrow" next to All to install the whole
    thing (it lags a bit, wait for it) or leave default and add the necessary 
	packages yourself by expanding devel and adding: autoconf, gcc, make, and 
	patchutils (choose the first one of each). The all option requires 
	broadband and at least an hour to download. It will automatically install 
	once complete, just follow the prompts.

2) Download the latest version of tbaMUD from http://tbamud.com
   Create an account on tbamud.com as it will be a great resource for any 
   problems that you encounter. Once you download and uncompress tbaMUD (I use 
   winRAR) read through the /doc directory.

3) Start the Cygwin "bash" shell by double clicking the Cygwin Icon. This will 
   open a telnet looking window that mimics a *nix server where you can run the
   MUD. Go to the directory where you extracted tbaMUD by using the "cd" change
   directory command and "dir" directory listing to find the correct files. For
   example "C:\tbaMUD". "cd c:/tbaMUD" then "dir" to list the files. You should
   see one called configure. DO NOT go into the SRC directory yet.

4) Run the configure script by typing "./configure".  This will automatically 
   detect what programs and library functions are available, and create the 
   files "Makefile" and "conf.h" based on the results.

   If you get complaints from bash that it will not run the configure script
   either because it cannot find it or because it claims the file is not
   an executable, you can also try "sh configure", "sh ./configure",
   "bash configure" and "bash ./configure" until one of them works.

5) NOW change to the /tbaMUD/src directory "cd src", and type "make", and watch
   tbaMUD and the additional utilities included in the tbaMUD distribution
   automatically being compiled and placed in /tbaMUD/bin.

6) Go back to /tbaMUD, and run the MUD either directly by typing "bin/circle", 
   or by using the "./autorun &" script. The & makes it run in the background.

7) Start a telnet program.  Open a connection to your own machine "localhost"
   on port 4000.  You should see the MUD's login screen welcoming you. The
   first person to login will be promoted to IMP.

If you have problems, read this document again.  Most questions are answered in
the documentation or at http://tbamud.com. If you are still having problems 
feel free to stop by The Builder Academy for assistance.

It really can not be stressed enough: READ EVERYTHING.

OPTIONAL: Make the following change to src/Makefile.in to make copyover work:

	--- Makefile.in.old 2007-02-26 05:52:19.000000000 +0100
	+++ Makefile.in     2007-03-26 17:21:56.000000000 +0200
	@@ -61,6 +61,8 @@ circle:
	        $(MAKE) $(BINDIR)/circle

	 $(BINDIR)/circle : $(OBJFILES)
	+       @-rm $(BINDIR)/circleold.exe
	+       @-mv $(BINDIR)/circle.exe $(BINDIR)/circleold.exe
	        $(CC) -o $(BINDIR)/circle $(PROFILE) $(OBJFILES) $(LIBS)

	 clean:

   The above is a "patch" file. All you need to do is add the two lines with
   the +'s to your file Makefile.in (but delete the +'s). If you are still 
   confused check out these links on how to patch: 
   http://www.circlemud.org/cdp/wtfaq/handpatch.html
   http://cwg.lazuras.org/modules.php?name=Forums&file=viewtopic&t=757
