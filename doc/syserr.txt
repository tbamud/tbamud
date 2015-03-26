If you have any additions, corrections, ideas, or bug reports please stop by the
Builder Academy at telnet://tbamud.com:9091 or email rumble@tbamud.com -- Rumble

   There are many common errors that can be created by changing things in the
code or having  builders try inappropriate things. Here are a few, this list
will be continually developed. If you have any additions or corrections please 
stop by the Builder Academy at telnet://tbamud.com:9091 or email: 
rumble@tbamud.com -- Rumble

1.  Errant Rooms (show error command)
2.  SYSERR: Object # (oedit-s-desc) doesn't have drink type as last alias.
3.  SYSERR: Mob # both Aggressive and Aggressive_to_Alignment.
4.  SYSERR: Object # (oedit-s-desc) has out of range level #.
5.  SYSERR: Object # (oedit-s-desc) has negative weight (-2147483644).
    SYSERR: Object # (oedit-s-desc) has out of range level #2147483647.
6.  SYSERR: Object # (oedit-s-desc) uses 'UNDEFINED' spell #.
7.  SYSERR: Object # (oedit-s-desc) contains (5) more than maximum (3).
8.  Char is already equipped: (medit-s-desc), (oedit-s-desc)
9.  SYSERR: Attempt to assign spec to non-existant mob #
10. No associated object exists when attempting to create a board [vnum #]. 
11. SYSERR: Mob using >'((ch)-)player_specials....
12: SYSERR: Object # (keyword) is type NOTE and has extra description with
    same name (keyword)

1: Errant Rooms

(Nowhere) [ 8868] House of Elders Chamber in Silverwood City (south)

   The most common are exits to Nowhere. This happens when a builder 
modifies a room exit but does not include an exit room vnum. These errant 
rooms are listed by the command 'show error' The fix is simple, remove the 
exit by purge exit in redit exit menu or by using 'dig <direction> -1' A few
builders actually create exits to nowhere just so they can set the exit
descriptions. So only delete if there is no exit description.

2: SYSERR: Object # (oedit-s-desc) doesn't have drink type as last alias. 

   Another common error is caused by how shop code handles drink containers 
poorly. In order for shops to display a jug as "a jug of <drink type>" they 
must have the drink type as the last alias. It is good practice to put the 
drink type as the first and last word of an objects namelist (older versions 
of CircleMUD expect it first and once the object is empty it removes one of
the drink aliases).
i.e. a shot of whisky should have the namelist: whisky shot whisky
     a cup of slime mold juice namelist should be: juice cup slime juice

3: SYSERR: Mob # both Aggressive and Aggressive_to_Alignment.

   Another harmless error. If your mob is aggressive there is no need to also
make it aggressive to certain alignments since AGGR means it will attack any
player it can see. Choose to make it aggressive to an alignment or aggressive 
to all.
 
4: SYSERR: Object # (oedit-s-desc) has out of range level #.

   A simple one. Limit spell levels to the levels available, on tbaMUD that 
would be 1-34.

5: SYSERR: Object # (oedit-s-desc) has negative weight (-2147483644).
   SYSERR: Object # (oedit-s-desc) has out of range level #2147483647.

   These are really annoying. This happens on older versions of CircleMUD when 
you use numbers larger than necessary. This will actually crash many older 
versions of CircleMUD. Just do not do it. Use realistic numbers.

6: SYSERR: Object # (oedit-s-desc) uses 'UNDEFINED' spell #.

   There is no spell zero. Either select a spell or put -1 for none. 

7: SYSERR: Object # (oedit-s-desc) contains (#) more than maximum (#).

   When making a drink container you will set how much it initially contains 
on creation and the max it can hold if a player were to fill it. Common sense 
tells us that you can not create a container that initially holds more than 
the max you set. To simplify the max must always be greater than or equal to 
the initial amount.

8: Char is already equipped: (medit-s-desc), (oedit-s-desc)

   This happens when someone tries to equip a mob with one or more object in
a single location. HELP ZEDIT-EQUIP for all the possible object equipping 
locations. All you have to do to fix this is pick a different equip location
that is not used. 

9: SYSERR: Attempt to assign spec to non-existant mob #

   To get rid of this "grep # spec_assign.c" and remove this assignment.
  
10: No associated object exists when attempting to create a board [vnum #]. 

   You need to delete this board from lib/etc/boards/ and modify boards.c and 
boards.h. Again "grep #" *.[ch] to search for this vnum in all of your .c and 
.h files to remove the reference.

11: SYSERR: Mob using >'((ch)-)player_specials....

Players and mobs (NPC's) share many of the same data fields, but not all. 
So when a mob tries to access player data it gives a SYSERR like this:
SYSERR: Mob using >'((ch)-)player_specials...

The fix is actually a very easy one. All you have to do is make sure it only 
checks player data if it is a player. The way this is done in the code is with
a non-player-character check. Now non-player-character (NPC) means it is a mob.
So you need a double negative to make sure it is not a non-player-character. I 
know this is confusing, but just copy the example below.

- if (PRF_FLAGGED(ch, PRF_NOREPEAT)) // This line should be removed, hence the "-"
+ if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT)) // This line should be added without the "+"

The changed line now will not just check for a flag, instead it will check if 
it is a player (not an NPC) and it is flagged then continue.

12: SYSERR: Object # (keyword) is type NOTE and has extra description with
same name (keyword)

Object type NOTE is meant to be written on using the action-description. So if
you have an extra description with the same keyword it will never be viewable.
