                          Compiling CircleMUD
                under Microsoft Windows 95 or Windows NT
                           using Watcom v.11


The following information is from Joe Osburn <joeos19@idt.net>.

Circle apparently compiles under 95/NT using Watcom's compiler with
the following changes:

1- Copy conf.h.win to conf.h

2- Rename all the act.* files to other names; the IDE in Watcom apparently
   doesn't like files that start with act.*

3- In Watcom make a new project that is a Windows 95 character mode
   executable; add all of Circle's .c files to it.

4- Remove the line that says "#define chdir _chdir" from sysdep.h


If you have any further information, patches, or more detailed instructions,
please mail them to us at bugs@circlemud.org.
