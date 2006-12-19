                            Compiling CircleMUD
                   under OS/2 Warp Connect v3.0 or 2.1
                             by David Carver


To compile CircleMUD under OS/2, you must have the following:
All needed files can be found at the hobbes.nmsu.edu FTP site.


* OS/2 Warp Connect Version 3.0, or OS/2 Version 2.1 with TCP/IP installed.
  You should have at least 8 megs of memory.  (Circle runs quite well on an
  8 meg machine).

* An HPFS formatted drive.  CircleMUD needs to be uncompressed on an HPFS
  drive because it uses long filenames.

* The EMX09b runtime and compilation systems.  These are free and
  can be downloaded by anonymous FTP at hobbes.nmsu.edu in os2/unix/emx09b

* The OS/2 port of GNU's GCC compiler.  This can also be found at
  hobbes.nmsu.edu in os2/unix/emx09b.  Please make sure you have the most
  recent version of the GCC compiler for OS/2, as files needed by CircleMUD
  were not included in earlier versions of GCC for OS/2.  The current version
  is 2.7.0

* GNU's TAR and GZIP programs to decompress the necessary files.  Again 
  these can be found at hobbes.nmsu.edu in os2/unix.
  **** You only need this if you plan on getting some of the various
  **** addons for Circle that others have coded.

* A MAKE program.  Either the GNU Make, or IBM's NMAKE should work.  You 
  can obtain the NMAKE from either IBM's Developers kit or from
  hobbes.nmsu.edu in os2/16dev.


Installation:

***  IMPORTANT
***
***  You must have EMX and GCC installed and the directories in your
***  PATH and LIBPATH statements in your CONFIG.SYS.  Please read the
***  EMX installation instructions included with that package for more
***  information on how to install both EMX and GCC.

Download the ZIP archive of Circle and use your favorite UNZip utility
to extract it.

After you have uncompressed the files, switch to the directory that has
the CircleMUD files in it, and then to the SRC subdirectory.  Rename
the following files:

Rename 'conf.h.os2' to 'conf.h'.
Delete the old 'makefile', and rename 'makefile.os2' to 'makefile'.

To compile the MUD type the following at an OS/2 command line:

NMAKE /i

CircleMUD will be compiled and the executable will be put in your current
directory.  Copy the CIRCLE.EXE file to the circle30\bin directory.  Then
follow the CircleMUD instructions in README on how to start up the MUD.

NOTE: General questions about CircleMUD can be addressed to the author,
Jeremy Elson, at jelson@circlemud.org.  However, all questions which
specifically deal with the OS/2 port of Circle should go to my address,
listed below.

David Carver
dcarver@cougar.colstate.cc.oh.us
dcarver@iwaynet.net
