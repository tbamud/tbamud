$!
$! VMS_AUTORUN.COM
$! Written By:  Robert Alan Byer
$!              Vice-President
$!              A-Com Computing, Inc.
$!              ByerRA@aol.com
$!              byer@mail.all-net.net
$!
$! This script is used to run the CircleMUD executable as a detached 
$! process.
$!
$! This ONLY runs the CircleMUD CIRCLE executable, it does NOT do anything 
$! else extra that the Unix "autorun" script does.
$!
$! To have the CIRCLE executable execute under a different UIC, change the
$! variable UIC below.
$!
$! To customize the CIRCLE startup, edit the [.BIN]VMS_CIRCLEMUD.COM file.
$!
$!
$! Define The UIC The CIRCLE.EXE Is To Execute Under.
$!
$ UIC = "SYSTEM"
$!
$! Run [.BIN]VMS_CIRCLEMUD.COM As A Detached Process.
$!
$ RUN/DETACHED	-
     /PROCESS_NAME= "CircleMUD" -
     /UIC=['UIC'] -
     /INPUT=CIRCLEMUD_BIN:VMS_CIRCLEMUD.COM -
     /OUTPUT=SYSLOG.LOG -
     /ERROR=SYSLOG.ERROR -
     SYS$SYSTEM:LOGINOUT.EXE
$!
$! Time To Exit.
$!
$ EXIT
