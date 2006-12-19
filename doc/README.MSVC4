            Compiling CircleMUD using Microsoft Visual C++ v4.x
                            by Jeremy Elson
                  For help, write to help@circlemud.org

CircleMUD compiles relatively easily under Windows 95 and NT using
the Microsoft Visual C++ compiler version 4.x.  These instructions won't
work for any compiler except for MSVC++ 4.0; if you have a different
compiler, take a look at the main README.WIN file for instructions.

Note MSVC++ 4.x is a commercial product and must be bought from your local
software store.  It can't be downloaded from any (legal) FTP sites, and I
will not send you a copy, so please don't ask.  Use the FREE GNU-Win32
package mentioned in the README.WIN file if you don't want to buy MSVC.


1) Download the latest version of CircleMUD.  You can always find the latest
   version at the following anonymous FTP sites:

       ftp.circlemud.org:/pub/CircleMUD
       ftp2.circlemud.org:/pub/CircleMUD

   You can also find information at the WWW site:

       http://www.circlemud.org/

   The latest version will be called something ending in .zip, like
   "circle30bplXX.zip".  (where 'XX' is the patchlevel)

2) When you unzip the .zip archive, MAKE SURE to use an unzip program that
   can handle long filenames.  Old versions of pkunzip (e.g. 2.x) do NOT
   handle long filenames.  WinZip (http://www.winzip.com) can.

3) Open a window with an MS-DOS prompt.  Note, this does not mean you are
   "compiling under DOS" -- the MS-DOS prompt is just a command-line
   interface to Windows 95.  This step can be done by going to the Start
   menu, going to the Programs submenu, and selecting "MS-DOS prompt".  All
   the following commands are performed at the MS-DOS prompt. 

4) Use the CD command to switch to the main CircleMUD directory.  For
   example, type "CD \circle30bplXX", where 'XX' is the patchlevel of the
   version of Circle you downloaded.  Also note that the full path will
   depend on where you decided to uncompress it.

5) Go to the src directory and rename conf.h.win to conf.h, and replace
   the Makefile with Makefile.msvc.  This can be accomplished with the
   following commands:

	cd src
	copy conf.h.win conf.h
	del Makefile
	copy Makefile.msvc Makefile
	
6) If you have MSVC++ 4.x installed in C:\MSDEV, skip to the next step.
   Otherwise, bring up the Makefile in your favorite text editor (for
   example, to use the DOS EDIT command, type "EDIT MAKEFILE".)  Find the
   two lines that start with "CLFAGS =" and "LIB=", respectively.  On BOTH
   lines, change the part that says "C:\MSDEV\" to reflect where your copy
   of MSVC++ 4.x is installed.  Then, save the Makefile and exit the
   editor.  You should still be in the "src" directory.

7) Make sure that MSVC++ binary directory (i.e., the directory where
   the actual programs are kept, such as NMAKE.EXE) is in your PATH.  You
   can see what your path is by typing PATH.  Your MSVC++ binary directory
   should be listed (for example, C:\MSDEV\BIN).  Add MSVC's binary
   directory to your path if it's not already there.  If you do not know
   how to change your path, contact someone who knows how to use the DOS
   command prompt for help, or check the manual to learn how to use the
   PATH command.

8) To compile Circle, stay in the src directory and type: 

        NMAKE

   This will invoke Microsoft's make program and should build the entire
   CircleMUD server and create a file called 'circle.exe'.  If you see the
   error message "Bad command or filename", then MSVC++'s binary directory
   is not in your path, so your computer can't find MS's NMAKE program.
   Go back to step 7.

9) Make sure your TCP/IP stack is installed, correctly configured, and
   running.  If you are already using TCP/IP applications from your
   Windows machine such as Netscape or telnet, then no changes should be
   necessary; otherwise go to the Control Panel's "Network" settings,
   select "Add Protocol", and add Microsoft's TCP/IP.  Consult the
   documentation for Windows 95 (do not write me mail) if you have any
   additional questions about how to set up TCP/IP under Windows 95. 

   YOU MUST INSTALL AND CONFIGURE YOUR TCP/IP STACK, EVEN IF YOU ARE NOT 
   CONNECTED TO THE INTERNET.

10) Go back to Circle's main directory (like in Step 4), and run the server
   by typing "src\circle".  You should see boot messages appearing on the
   screen.  Wait until the line "No connections.  Going to sleep." appears
   at the bottom of the screen -- this means Circle is ready to accept
   connections.  Go on to step 11 if you see this.

   If you see "Winsock Error #10047", your TCP/IP stack is not correctly
   configured; go back to Step 9.

   If you see "Fatal error changing to data directory: No such file
   or directory", that means you are trying to run Circle from the
   "src" directory.  Your current directory must be Circle's top-level
   directory -- the same directory that you were in during Step 4.

11) Start a telnet program (SEE NOTE BELOW).  Open a connection to your
   own machine ("localhost", or whatever the name of your machine happens
   to be) on port 4000.  You should see the MUD's login screen welcoming
   you and asking for your name.

   VERY IMPORTANT NOTE:  The standard telnet program that comes free with
   Windows 95 and NT does *not* work correctly for connecting to any MUD
   because it does not support telnet's line-mode interface (so you can't
   see what you are typing).  Note that simply turning on the "local echo"
   option does not fix the problem; this prevents echo from being turned
   off while you're typing your password, and screws up the display if you
   try to hit the backspace key too many times.

   Do not use Microsoft's telnet applet -- instead, use EWAN, CRT, zMUD, or
   any other Winsock telnet application.  EWAN and CRT can be downloaded
   from any number of sites (for example, www.windows95.com).  zMUD is an
   excellent MUD client; for more information, see the official home page
   at http://www.zuggsoft.com/zmud/zmudinfo.htm .


If you have problems, read this document again.  Most of the questions
I receive in email are answered in this README file.  If you're still
having problems and you're *sure* that this document doesn't answer
your question, try reading the CircleMUD FAQ at
ftp://ftp.circlemud.org/pub/CircleMUD/FAQ.  If all else fails, you can
get help by sending mail to help@circlemud.org.  Note, however, that
if you ask a question that is answered in this document, all I'll do
is mail it to you.

Have fun!

Jeremy Elson
(To get help, write to help@circlemud.org)
