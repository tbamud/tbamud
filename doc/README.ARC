                     Compiling CircleMUD under RiscOS
               by Gareth Duncan (garethduncan@argonet.co.uk)
                             
You will need:
The CircleMUD source code.
!GCC, !UnixLib, drlink and make available from Hensa.
Acorns sockets library available form the Acorn ftp site.
A copy of !FreeNet and !FreeTerm.
                             
1) Firstly obtain a copy of !GCC, !UnixLib, drlink, make and Acorns 
   sockets library.

2) Place the directory Sockets from the sockets library inside 
   !UnixLib37.src.clib  
   
3) Unpack the CircleMUD binary and start setting up the directory
   structures in the src directory.
   
4)                               src
                                  |
                     ----------------------------------
                     |      |     |      |      |     |
                   util    act    c      h      o    conf
                            |
                     --------------- 
                     |      |      |
                     c      h      o
 
5) Place all the files in the correct directories according to their
   name remembering to remove the directory information from the 
   filename.
   e.g. ban/c goes in the directory c and is renamed to ban.
        act/item/c goes in the directory act then c and is renamed to 
        item.
   
6) Set the type of any data files in the src directories to text.

7) Copy the acorn configure file (should be conf/h/arc) into the h
   directory and rename it conf.
   
8) Create an obey file called !Compile in the src containing the 
   following lines
   
   -- begin (don't include this line)   
   WimpSlot -min 10000K -max 10000K
   dir <Obey$Dir>

   make -r
   -- end (don't include this line)   
   
   and set the wimpslot to as much memory as you can afford.

9) Place the make program in the src directory and rename the file
   Makefile/arc to Makefile removing the old file already called 
   Makefile.

10) Unpack GCC and Unixlib placing them where you want and then 
    double click on them. Then run the !Compile file. Everything 
    should run okay. Make sure that drlink is placed inside GCC in the
    bin directory. If you get any error messages check that the code 
    changes at the bottom of this file are present. If not alter the
    code as instructed.
    
11) Place the module CallASWI from !UnixLib37.src.CallASWI in the bin
    directory.
    
12) Now get a copy of the FreeNet internet stack or a recent version
    of Acorns stack and FreeTerm. Make sure the FreeUser start up 
    script has the line 
    
    ifconfig lo0 inet 127.0.0.1 up
    
    Then run the startup script, run FreeTerm and then open a task 
    window. Run the !Run file (which should be placed in the directory
    above src) from the task window by typing in its file name and 
    then press return, the Mud should load (you should be able to just
    shift drag the !Run file onto the window if you are using !Zap).
    
13) To log onto the mud type localhost and set the port to 4000 in 
    FreeTerm and then press connect
    
Please excuse the poor spelling and grammar in this and if you have
any trouble contact garethduncan@argonet.co.uk.

Bye.

-Gareth
