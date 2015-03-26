Updated: Apr 2007
                 Compiling CircleMUD under Microsoft Windows XP
                     using Microsoft Visual C++ 2005 (8.0)


The following information is from by Jason Pullara. You can contact him at
<webmaster@geekstreak.com>. [1][2]

Compiling with MS Visual C++ 8.0:

1.  In the src directory, rename conf.h.win to conf.h.

2.  Go to File|New Project... Create a new workspace called circle.
    Put the root directory path into the location field.

3.  Select "Win32 Console Application." Click OK.

4.  On the next screen Select "Application Settings" and check "Empty
    Project" under the "Additional Options" heading. Click Finish.

5.  In the solution explorer, right click on the "header" folder, and select
    Add->Existing Item...

6.  Select all of the .h files in the src directory and click Add.

7.  In the solution explorer, right click on the "source" folder, and select
    Add->Existing Item...

8.  Select all of the .c files in the src directory and click Add.

9.  In the solution explorer, right click on the workspace name ("circle").
    Select properties.

10. In the solution properties dialog box, under Configuration Properties,
    select "General"

11. Change "Character Set" to "Use Multi-Byte Character Set"*

12. In the solution properties dialog box, expand "Linker" and select
    "Input"

13. Under the "Additional Dependencies" add wsock32.lib and click OK.

14. Save all.

15. In the menu click Build->Build Solution (or press Ctrl-Shft-B).
    CircleMUD should now build.

16. Move the circle.exe file from the circle/Debug directory to the root
    directory.

17. You're done! =)


* Yes, you have to change it to multi-byte character set, otherwise MSVC8
  will throw a hissey-fit about being unable to convert char to wchar_t.


=========

[1] - This appears (by 'diff') to be based on the README.MSVC5 document by Rob
Baumstark from http://www.connect.ab.ca/~rbmstrk/.  You can contact Rob at his
<shirak@connect.ab.ca> e-mail address.

[2] - This is based on the README.MSVC6 document by Michael Robinson. You can
contact Michael at his <chevy67ss@geocities.com> e-mail address.

George Greer
greerga@circlemud.org

Jason Pullara
webmaster@geekstreak.com

To eliminate warnings like these:
  warning C4996: 'strcpy': This function or variable may be unsafe.
  warning C4996: 'getch': The POSIX name for this item is deprecated.
Do this:
  In Solution Explorer
  Right click the 'project'
  Select 'Properties' which brings up the Property Pages dialog
  Expand 'Configuration Properties'
  Expand 'C/C++'
  Click on Preprocessor
  At the end of the 'Preprocessor Definitions' add:
  ;_CRT_SECURE_NO_DEPRECATE;_CRT_NONSTDC_NO_DEPRECATE 
