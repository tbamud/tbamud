                          Compiling CircleMUD
                under Microsoft Windows 95 or Windows NT
                           using Borland C++

		Written by Mundi King <kingmundi@yahoo.com>

Here are some instructions on compiling circlemud using Borland C++ 5.01. 
These instructions will not work using Turbo C++, or the 4.0 versions of
Borland C++ as those two products were geared twoards DOS and Windows 3.xx. 

It will most likely work with versions 5.00, 5.02, and 5.5 of the Borland
C++ compilers. 

Boot up your Windows 95 machine.

Unzip your CircleMUD package.

Goto a DOS prompt, and change to the circle \src directory. 

(Type) rename conf.h.win conf.h (Enter)

** BORLAND 5.5 **
If you are using Borland C++ 5.5, a couple of extra changes need to be
made at this time.  First you have to make sure the bin directory of the
tools is in your path.  You can add the following line to your autoexec.bat
to have it automatically added to your path or you can type it at a DOS
prompt:
    path = %path%;c:\borland\bcc55\bin

(Type) make -fmakefile.bcc55 (Enter)

** BORLAND 5.1 **
(Type) make -fmakefile.bcc (Enter)

** End Version Specifics **

  Something to note here is that these makefile 
  assume that you have installed Borland C++ 5.x
  to the C: drive.  If you have installed it to
  another drive you will have to open up the correct
  Makefile in a text editor and find and replace
  all C:\ references to the drive letter it has
  been installed to.

(Type) move circle.exe ..\ (Enter)

(Type) cd .. (Enter)

(Type) circle (Enter)

The game should start loading the zones and database.  You will no longer be
able to type in this DOS box. 

Click on START and then on RUN.

(Type) telnet localhost 4000 (Enter)

  The first one to logon becomes the Implementor.
  Also remember that you are using Windows95's
  built-in telnet program which is very basic.

Pat yourself on the back.

---
Mundi King 1998-07-03
Updated for 5.5: 2000-06-28
