         Compiling CircleMUD under Microsoft Windows 95 or Windows NT
                     using Microsoft Visual C++ v5.x


The following information is from Rob Baumstark's web page at
http://www.connect.ab.ca/~rbmstrk/.  You can contact Rob at
<shirak@connect.ab.ca>.

How to compile using MS Visual C++ 5:

  1. Rename conf.h.win to conf.h in the src directory

  2. Go to File|New... Create a new workspace called circle. You should
     put this in the root of the circle directory, unless you want to move
     circle into the workspaces directory later.

  3. Go to File|New... Create a new Win32 Console App called circle in
     the current workspace

  4. If you didn't create this in the circle dir, move the source to the
     directory where this project is. Default should be:
         C:\Program Files\DevStudio\MyProjects\Circle\Circle 

  5. Change to file-view.

  6. Right-click "Circle files", and click Add Files to Project... Select
     all of the .C files

  7. Right-click "Circle files", and click New Folder. Rename it to
     includes, or headers

  8. Right-click the new folder, and click Add Files to Folder... Select
     all of the .H files.  Note: You could just add the the .H files to the
     project with the .C files, but this helps keep it more organized I
     think. 

  9. Right-click "Circle files", and click settings... 

 10. Choose settings for all configurations, and move to the Link tab 

 11. Add wsock32.lib to the end of the Object/Library modules list. 

 12. Change the settings under the General and Debug tabs if you want
     to be able to use the internal debugger.

 13. SAVE THE WORKSPAVE 

 14. Choose Build|Build Circle.exe, or hit F7 to build it. 


The circle.exe file will be placed in the circle\debug directory, unless
you turned off debug mode, in which case it will be in the circle\release
directory.  By compiling in this way, instead of using the GNU Win32
thingy, or embedding the makefile that came with circlemud inside a
project, allows you to use all of MSVC++'s interesting features.
