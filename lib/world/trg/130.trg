#13000
Maze Trigger~
2 g 100
~
* Random
* Mist Maze
* Using random directions maze a "maze" useing loaded and purged exits
* and load the mobs that will exist in this room
*
* first remove any exits that may already exist
eval thisRoom %self.vnum%
* if %self.people% == 0
  %door% %thisRoom% north purge
  %door% %thisRoom% south purge
  %door% %thisRoom% east purge
  %door% %thisRoom% west purge
  wait 1 sec
  set bottomRoom 13000
  set upperRoom 13078
  * bottomRoom is the lowest room number in the maze
  * upperRoom is the highest room number in the maze
  set numExits 0
  eval totalRooms %upperRoom% - %bottomRoom%
  * 1:4 chance for any direction to show up, but more than one may show
  while !%numExits%
    eval exitN %random.4%
    eval exitS %random.4%
    eval exitW %random.4%
    eval exitE %random.4%
    if %exitN% == 1
      eval temp %%random.%totalRooms%%%
      eval targetRoom %temp% + %bottomRoom%
      %door% %thisRoom% north room %targetRoom%
      eval numExits %numExits% + 1
    else
       return 0
    end
    if %exitS% == 1
      eval temp %%random.%totalRooms%%%
      eval targetRoom %temp% + %bottomRoom%
      %door% %thisRoom% south room %targetRoom%
      eval numExits %numExits% + 1
    else
       return 0
    end
    if %exitW% == 1
      eval temp %%random.%totalRooms%%%
      eval targetRoom %temp% + %bottomRoom%
      %door% %thisRoom% west room %targetRoom%
      eval numExits %numExits% + 1
    else
       return 0
    end
    if %exitE% == 1
      eval temp %%random.%totalRooms%%%
      eval targetRoom %temp% + %bottomRoom%
      %door% %thisRoom% east room %targetRoom%
      eval numExits %numExits% + 1
    else
       return 0
    end
  done
  %send% %actor% The mists swirl at your entrance revealing a new path.
  %echoaround% %actor% %actor.name% causes the mists to swirl revealing a path.
  *
  * Now load the mobs
  eval totalMob %random.3%
  eval totalMob %totalMob% -1
  set howMany 0
  while %howMany% < %totalMob%
    switch %random.4%
    case 1
      %load% mob 13000
      %force% feeling kill %actor.name%
      eval howMany %howMany% + 1
    break
    case 2
      %load% mob 13001
      %force% feeling kill %actor.name%
      eval howMany %howMany% + 1
    break
    case 3
      %load% mob 13002
      %force% feeling kill %actor.name%
      eval howMany %howMany% + 1
    break
    case 4
      %load% mob 13003
      %force% feeling kill %actor.name%
      eval howMany %howMany% + 1
    break
    default
      %echo%  This trigger is broken, please report to an immortal.
    break
    done
  done
  *
  * Check to see if the players "hear" anything in the background
  eval doHear %random.5%
  if %doHear% < 3
    wait 20 sec
    switch %random.3%
    case 1
      %echo% Voices in the distance can be heard complaining about wasting arrows shooting in the distance, since they don't have a crown to peek ahead.
    break
    case 2
      %load% obj 13008
      %echo% A torn piece of paper floats in on a soft breeze.
    break
    case 3
      %echo% A voice from behind you cusses at a tombstone, then cusses when he pokes himself with an arrow.
      wait 2 sec
      %echo% An arrow flies past your face and strikes the ground nearby.
      %load% obj 13003
    break
    default
      %echo%  This trigger is broken, please report to an immortal.
    break
    done
  end
* end
~
#13001
Mist Maze exit trigger~
2 q 100
~
* Random
* Swirling mists maze clean up trigger
* Removes all the room exits when a player leaves the room
wait 1 sec
eval thisRoom %self.vnum%
if !%self.people%
  %door% %thisRoom% north purge
  %door% %thisRoom% south purge
  %door% %thisRoom% east purge
  %door% %thisRoom% west purge
else
  return 0
end
~
#13002
recall Staff~
1 c 1
staff~
* Random
* recall staff
* set of trigs to allow users to recall infinitely, and be able
* to return to where they where
* Trigger Intended Assignment: Objects
* Trigger Type: Command , Numeric Arg: 1, Arg list: staff
*
* this allows the user to return to a preset room
eval actor %self.worn_by%
* this allows the user to recall to 3001
if %arg% == recall
  if %actor.eq(17)%
    %teleport% %actor% 3001
    %force% %actor% look
  else
    %send% %actor% You must hold the recall staff to use it.
  end
*
* this allows the user to set the return to room
elseif %arg% == set
  if %actor.eq(17)%
    if !%actor.varexists(set_life_counter)%
      set set_life_counter 1
      remote set_life_counter %actor.id% 
    end
    if %actor.set_life_counter% < 14
      eval setReturn %self.room%
      eval setReturn %setReturn.vnum%
      eval return_room_staff %setReturn%    
      eval set_life_counter %actor.set_life_counter% + 1
      set set_counter 0
      remote set_life_counter %actor.id%
      global return_room_staff
      global set_counter
      %send% %actor% You will now return to here.
    else
      %send% %actor% The recall staff cannot be set anymore.
    end
  else
    %send% %actor% You must hold the recall staff to use it.
  end
*
* this allows the user to return to the preset room
elseif %arg% == return
  if %actor.eq(17)%
    %teleport% %actor% %return_room_staff%
    %force% %actor% look
    eval set_counter %set_counter% + 1
    global set_counter
    if %set_counter% > 2
      %send% %actor% The recall staff glows briefly then flickers back to normal.
      set return_room_staff 3001
      set set_counter 0
      global return_room_staff
      global set_counter
    end
  else
    %send% %actor% You must hold the recall staff to use it.
  end
else
  %send% %actor% Huh?!?  Who?!?  What?!?
end 
~
#13003
recall staff drop~
1 hi 100
~
* Random
* recall staff drop
* keeps players from getting rid of the recall staff
* Trigger Intended Assignment: Objects
* Trigger Type: Drop Give , Numeric Arg: 100, Arg list: None
%send% %actor% You cannot get rid of %self.shortdesc%.
return 0
~
#13004
Eagle Eye~
1 c 1
peek~
* Random
* Eagle eyes
* allows players to see the next room
eval room2 %self.room%
eval thisRoom %room2.vnum%
*
if %arg% == north || %arg% ==  east || %arg% == south || %arg% == west || %arg% == up || %arg% == down || %arg% == northwest || %arg% ==  northeast || %arg% == southeast || %arg% == southwest
  eval dir %arg%
  eval room1 %self.room%
  eval thatRoom %%room1.%dir%(vnum)%%
  if %thatRoom%
    %teleport% %actor% %thatRoom%
    %force% %actor% l
    %teleport% %actor% %thisRoom%
  else
    %send% %actor% There is no exit that direction to peek at.
  end
 else
  %send% %actor% Where do you want to peek?
end
~
#13005
Bow and Arrow~
1 c 1
fire~
* Detta and Random
* bow and arrow
* this trig is a hack of Detta's bow and arrow trig (from TBA)
* Trigger Intended Assignment: Objects
* Trigger Type: Command , Numeric Arg: 1, Arg list: fire
*
* Checks if there are any players/mobs in the room
* to the direction specified.
*
eval target %arg.car%
eval dir %arg.cdr%
if %dir.mudcommand% == north || %dir.mudcommand% == east || %dir.mudcommand% == south || %dir.mudcommand% == west || %dir.mudcommand% == up || %dir.mudcommand% == down
  eval dir %dir.mudcommand%
  eval room1 %self.room%
  eval direction %%room1.%dir%(vnum)%%
  eval where %%room1.%dir%(room)%%
* checks to see if there is a target
  if !%target% 
    %send% %actor% Who are you trying to shoot?
    halt
  end
* checks for the target to be in the room described
  set isHere 0
  eval tmpTarget %where.people%
  while %tmpTarget%
    if %target% == %tmpTarget.name%
      eval targetID %tmpTarget%
      set isHere 1
    end
  eval tmpTarget %tmpTarget.next_in_room%
  done
  if %isHere% == 0
    %send% %actor% That person isn't there for you to shoot!
    halt
  end
* checks for peaceful room flags in target room and actor room
*  if %where.flag% == PEACEFUL
*    %send% %actor% You cannot fire in to a peaceful room.
*    halt
*  end
*  if %actor.room.flag% == PEACEFUL
*    %send% %actor% You cannot fire from a peaceful room.
*    halt
*  end
  *
  * Checks for the first item in inventory that is one of the 
  * specified arrows and sets its vnum as the one to be used.
  *
  eval inv %actor.inventory%
  while (%inv%)
    if %inv.vnum% == 13003 || %inv.vnum% == 13004 || %inv.vnum% == 13005 || %inv.vnum% == 13006
      eval arrow %inv.vnum%
      eval weapon %inv%
      set inv 0
    else
      eval next %inv.next_in_list%
      set inv %next%
    end
  done
  *
  * Searchable array by Random, matches the chosen arrow
  * vnum with the damage stats and any affects to be used.
  *
  *           vnum dam dice bonus spell
  set type[1] 13003 3    3    2     0
  set type[2] 13004 3    5    3     burning hands
  set type[3] 13005 6    2    4     chill touch
  set type[4] 13006 4    3    3     poison
  set type[5] none * must be the last item in the array
  set i 1
  while %vnum% != none
    set temp %%type[%i%]%%
    eval temp %temp%
    eval vnum %temp.car%
    if %vnum% == %arrow%
      eval temp %temp.cdr%
      eval dam %temp.car%
      eval temp %temp.cdr%
      eval dice %temp.car%
      eval temp %temp.cdr%
      eval bonus %temp.car%
      eval temp %temp.cdr%
      eval spell %temp.car%
    end
    eval i %i% +1
  done
  *
  * Just a calculation of the arrow damage
  *
  set finaldam 0
  set i 0
  while %i% != %dice%
    eval tempDam %random.%dam%%
    eval finaldam %finaldam% + %tempDam%
    eval i %i% +1
  done
  eval finaldam %finaldam% + %bonus%
  *
  * If the actor has an arrow in inventory, and there are 
  * people in the room specified, one of three random things
  * happens - Actor shoots but misses, Actor shoots and damages,
  * Actor shoots, damages, but loses the arrow.
  *
  if %arrow%
    if %target%
*  a hack of the formula used in the code to determine if something hits
*  based on dexterity... 
    eval dex %actor.dex%
    eval odds %dex% * 4
      if %odds% > 99
        set odds 99
      end
    eval hitchance %random.100%
    if %target.vnum% == -1
      eval %odds% 0
    end
    if %odds% == 0
      %send% %actor% You cannot PK with this weapon.
    elseif %hitchance% == 1
      * critical failure, bow breaks and damages player
      %send% %actor% The string on your bow breaks snapping back and ripping in to your flesh and damaging the bow.
      %echoaround% %actor% The string on %actor.name%'s bow breaks.
      eval criticalDam %finaldam% * 2
      %damage% %actor% %criticalDam%
      if %spell%
        dg_cast '%spell%' %actor%
      end
*     %load% obj ###broken bow### %actor% inv
      %purge% %self%
    elseif %odds% >= %hitchance%
      switch %random.3%
        * case 1 mob is hit and hunts the player
        case 1
          %at% %direction% %damage% %target% %finaldam%
          %force% %target% mhunt %actor%
          if %spell%
            dg_cast '%spell%' %target%
          end
          %send% %actor% You shoot %target.name% with %weapon.shortdesc%.
          %echoaround% %actor% %actor.name% shoots someone with %weapon.shortdesc%.
          eval item_to_purge %%actor.inventory(%arrow%)%%
          %purge% %item_to_purge%
        %at% %direction% %load% obj %arrow% %targetID% inv
        break
        * case 2 mob is hit but has no reaction
        case 2
          %at% %direction% %damage% %target% %finaldam%
          if %spell%
            dg_cast '%spell%' %target%
          end
          %send% %actor% You shoot %target.name% with %weapon.shortdesc%.
          %echoaround% %actor% %actor.name% shoots someone with %weapon.shortdesc%.
          eval item_to_purge %%actor.inventory(%arrow%)%%
          %purge% %item_to_purge%
        break
        * case 3 mob is hit and flees the room
        case 3
          %at% %direction% %damage% %target% %finaldam%
          %force% %target% flee
          if %spell%
            dg_cast '%spell%' %target%
          end
          %send% %actor% You shoot %target.name% with %weapon.shortdesc%.
          %echoaround% %actor% %actor.name% shoots someone with %weapon.shortdesc%.
          eval item_to_purge %%actor.inventory(%arrow%)%%
          %purge% %item_to_purge%
          %at% %direction% %load% obj %arrow% %targetID% inv
        break
        default
          %echo% If you see this message, something is wrong. Please report it.
        break
      done
      elseif %odds% == %hitchance%
        %at% %direction% %damage% %target% %finaldam%
        %force% %target% mhunt %actor%
        if %spell%
          dg_cast '%spell%' %target%
        end
        %send% %actor% You shoot %target.name% with %weapon.shortdesc%.
        %echoaround% %actor% %actor.name% shoots someone with %weapon.shortdesc%.
        eval item_to_purge %%actor.inventory(%arrow%)%%
        %purge% %item_to_purge%
        %at% %direction% %load% obj %arrow%
      done
      else
        switch %random.2%
            * clean miss*
            %send% %actor% You try to shoot %target.name% with %weapon.shortdesc% but miss.
            %echoaround% %actor% %actor.name% tries to shoot someone but appears to have missed.
            eval item_to_purge %%actor.inventory(%arrow%)%%
            %purge% %item_to_purge%
            %at% %direction% %load% obj %arrow% %targetID% inv
          break
          case 2
            * pricked finger and  miss*
            %send% %actor% You prick yourself with your arrow before firing it causing you to miss %target.name%.
            %echoaround% %actor% %actor.name% tries to shoot someone but appears to have missed.
            eval item_to_purge %%actor.inventory(%arrow%)%%
            if %spell%
              dg_cast '%spell%' %actor%
            end
            %damage% %actor% %random.3%
            %purge% %item_to_purge%
            %load% obj %arrow%
          break
          default
            %echo% If you see this message, something is wrong. Please report it.
          break
        done
      end
    else
      %send% %actor% Theres no one there.
    end
  else
    %send% %actor% You can't shoot without arrows.
  end
else
  %send% %actor% You must specify a direction - north, east, west, south, up, or down.
end
~
#13006
recall staff junked~
1 c 2
ju~
* Random
* Recall staff junked
* how to junk a recall staff
* Trigger Type: Command , Numeric Arg: 2, Arg list: jun
* Commands:
*
if %cmd.mudcommand% == junk && test /= %arg%
  set return_room_staff 3001
  set set_counter 0
  global return_room_staff
  global set_counter
  %echo% passed
  %purge% %self%
else
  return 0
  %echo% failed
end
~
#13007
Graphical Tombstone~
1 c 4
l~
* Random
* Graphical tombstone
* a tombstone where the epitaph and the name vary
* Trigger Intended Assignment: Objects
* Trigger Type: Command , Numeric Arg: 4, Arg list: l
*
if %arg% == tombstone || %arg% == tombston || %arg% == tombsto || %arg% == tombst || %arg% == tombs || %arg% == tomb || %arg% == tom || %arg% == to || %arg% == t
* if %cmd.mudcommand% == look && %arg% /= tombstone
set length[1] \@d\@0.\@n
set length[2] \@d\@0..\@n
set length[3] \@d\@0...\@n
set length[4] \@d\@0....\@n
set length[5] \@d\@0.....\@n
set length[6] \@d\@0......\@n
set length[7] \@d\@0.......\@n
set length[8] \@d\@0........\@n
*
eval target %random.char%
eval name %target.name%
eval length %name.strlen%
eval space 17 - %length%
eval space %space% / 2
set whiteSpace %%length[%space%]%%
eval whiteSpace %whiteSpace%
eval nameLine %whiteSpace%\@w%name%%whiteSpace%\@n
if %nameLine.strlen% == 40
  eval spaceII %space% + 1
  set whiteSpaceII %%length[%spaceII%]%%
  eval whiteSpaceII %whiteSpaceII%
  eval nameLine %whiteSpace%\@w%name%%whiteSpaceII%\@n
end
*
switch %random.2%
  case 1
    %send% %actor% \@w\@0               o o o\@n
    %send% %actor% \@w\@0                \\\|/\@n
    %send% %actor% \@w\@0                ooo\@n
  break
  case 2
    %send% %actor% \@w\@0                 o@n
    %send% %actor% \@w\@0               _\| \|_@n
    %send% %actor% \@w\@0              \|_ + _\|@n
    %send% %actor% \@w\@0                \| \|@n
    %send% %actor% \@w\@0                \| \|@n
  break
  default
    %send% %actor% I am broken, please tell an Immortal
  break
done
%send% %actor% \@w\@0             .-'''''-.\@n
%send% %actor% \@w\@0          .-' = oOo = '-.\@n
%send% %actor% \@w\@0       .-'  :::::_:::::  '-.\@n
%send% %actor% \@w\@0   ___/ ==:...:::-:::...:== \\___\@n
%send% %actor% \@w\@0  /_____________________________\\\@n
%send% %actor% \@w\@0  '-._________________________.-'\@n
%send% %actor% \@w\@0     \\ .-------------------. /\@n
%send% %actor% \@w\@0     \| \|\*                 \*\| \|\@n
%send% %actor% \@w\@0     \| \|'    Here Lies    '\| \|\@n
%send% %actor% \@w\@0     \|=\|'%nameLine%\@w\@0'\|=\|\@n
%send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
*
* the following line is a template to be used to make new
* cases for epitaphs
* %echo% \@w\@0     \|=\|'                 '\|=\|\@n
*
switch %random.8%
  case 1
    * Only The Good Die Young (23)
    %send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'  Only The Good  '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'    Die Young    '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
  break
  case 2
    * A friendly young widow seeks comfort (36)
    %send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'    A Friendly   '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'   Young Widow   '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'  Seeks Comfort  '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
  break
  case 3
    * R.I.P.  I told you this was dangerous (37)
    %send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|' I told you this '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'  was dangerous  '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
  break
  case 4
    * Once I wasn't  Then I was  Now I ain't again (44)   
    %send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'  Once I wasn't  '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'   Then I was    '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'Now I ain't again'\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
  break
  case 5
    * Feces Occurs (12)
    %send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'   Feces Occurs  '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
  break
  case 6
    * This space for rent (19)
    %send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'   This space    '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'    for rent     '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
  break
  case 7
    * This way to the Egress (22)
    %send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|' This way to the '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'      EGRESS     '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'     \<------\>    '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
  break
  case 8
    * I would turn back if I where you (32)
    %send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'I would turn back'\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'  if I were you  '\|=\|\@n
    %send% %actor% \@w\@0     \|=\|'                 '\|=\|\@n
  break
  default
    %send% %actor% I am broken, please tell an Immortal
  break
done
*
%send% %actor% \@w\@0     \|=\|\*                 \*\|=\|\@n
%send% %actor% \@w\@0     \| '-------------------' \|\@n
%send% %actor% \@w\@0     \|_______________________\|\@n
*
switch %random.3%
  case 1
    %send% %actor% \@w\@0    /_________________\@R@@@G}---\@w\@0___\\\@n
  break
  case 2
    %send% %actor% \@w\@0    /___\@G\\\|/\@w___\@G\\\|/\@w____\@G\\\|/\@w__\@G\\\|/\@w_\\\@n
  break
  case 3
    %send% %actor% \@w\@0    /_________________________\\\@n
  break
  default
    %send% %actor% I am broken, please tell an Immortal
  break
done
%send% %actor% \@n
%echoaround% %actor% %actor.name% looks at an old tombstone.
else
  return 0
end
~
#13008
Mist mob death~
0 f 100
~
* Random
* Mist maze mob's death
* Randomly loads objs on the mobs upon death
*
switch %random.5%
  case 1
    * load a portal
    %load% obj 13009
    drop mist
  break
  case 2
    * load the torn note
    %load% obj 13009
    drop note
  break
  case 3
    * load some arrows
    eval numArrows %random.5%
    set counter 0
    while %counter% < %numArrows%
      eval temp %random.4%
      eval arrowNum %temp% + 13002
      %load% obj %arrowNum%
      eval counter %counter% + 1
    done
    drop all.arrow
  break
  case 4
    * load nothing
  break
  case 5
    * load nothing
  break
done
%echo% The feeling fades away.
return 0
%teleport% %self% 13099
~
#13009
Mist portal~
1 c 7
en~
if %cmd.mudcommand% == enter && %arg% /= mist
  %send% %actor% You step in to the swirling mist.
  %echoaround% %actor% %actor.name% steps in to the swirling mist.
  switch %random.5%
    case 1
      %teleport% %actor% 13096
    break
    case 2
      %teleport% %actor% 13092
    break
    case 3
      %teleport% %actor% 13088
    break
    case 4
      %teleport% %actor% 13084
    break
    case 5
      %teleport% %actor% 13080
    break
  done
  %force% %actor% look
  %echoaround% %actor% %actor.name% steps out of the swirling mist.
else
  %send% %actor% %cmd% Huh?!
end
~
#13010
Mist portal back in to maze~
1 c 7
en~
if %cmd.mudcommand% == enter && %arg% /= mist
  %send% %actor% You step in to the swirling mist.
  %echoaround% %actor% %actor.name% steps in to the swirling mist.
  %teleport% %actor% 13001
  %force% %actor% look
  %echoaround% %actor% %actor.name% causes the mists to swirl and change.
else
  %send% %actor% %cmd% Huh?!
end
~
#13011
mist maze - load mobs only~
2 g 100
~
* Random
* Mist Maze - load mobs with no exits
* Load the mobs that will exist in this room
*
* Now load the mobs
eval totalMob %random.3%
eval totalMob %totalMob% -1
set howMany 0
while %howMany% < %totalMob%
  switch %random.4%
  case 1
    %load% mob 13000
    %force% feeling kill %actor.name%
    eval howMany %howMany% + 1
  break
  case 2
    %load% mob 13001
    %force% feeling kill %actor.name%
    eval howMany %howMany% + 1
  break
  case 3
    %load% mob 13002
    %force% feeling kill %actor.name%
    eval howMany %howMany% + 1
  break
  case 4
    %load% mob 13003
    %force% feeling kill %actor.name%
    eval howMany %howMany% + 1
  break
  default
    %echo%  This trigger is broken, please report to an immortal.
  break
  done
done
~
#13012
mist and suicide spells~
0 k 25
~
* Random
* Mist's Special Attacks
* allows the mist mob to have various special attacks
*
switch %random.7%
  case 1
    say Your time has come to an end!
    wait 3 sec
    dg_cast 'poison' %actor%
  break
  case 2
    %send% %actor% The mist slams to the ground sending a shockwave toward you.
    %echoaround% %actor% The mist slams to the ground sending a shockwave toward %actor.name%.
    wait 1 sec
    %force% %actor% remove all
    %force% %actor% drop all
  break
  case 3
    say Let's see how well you do if I pluck out your eyes!
    wait 1 sec
    dg_cast 'blind' %actor%
  break
  case 4
    say I said to 'shut up'!!
    wait 1 sec
    dg_cast 'curse' %actor%
  break
  case 5
    say Your time has come to an end!
    wait 3 sec
    dg_cast 'poison' %actor%
  break
  case 6
    say I said to 'shut up'!!
    wait 1 sec
    dg_cast 'curse' %actor%
  break
  case 7
    say Your time has come to an end!
    wait 3 sec
    dg_cast 'poison' %actor%
  break
  default
    %echo% You got lucky this time.  I'm broken, please tell an immortal.
  break
done
~
#13013
Mist mob dies~
0 f 100
~
* Random
* Mist mob's death
* 
*
%load% obj 13010
drop mist
%teleport% %self% 13099
~
#13014
loads a mob called mist to room~
2 g 100
~
%load% mob 13004
%force% mist kill %actor.name%
wait 10 sec
~
#13015
load a mob called mist and one called suicide to room~
2 g 100
~
%load% mob 13004
%force% mist kill %actor.name%
%load% mob 13005
%force% suicide kill %actor.name%
wait 10 sec
~
#13016
suicide mob death~
0 f 100
~
switch %random.3%
  case 1
    %load% o 13001
  break
  case 2
    %load% o 13002
  break
  case 3
    %load% o 13007
  break
  default
    %echo% I'm broken, tell an immortal!!
  break
done
~
#13017
Falling Fruit~
2 b 51
~
* Random
* Falling fruity affects
* Fruit of varius descriptions falls from the tree and as it strikes
*  players it cast's various affects on them
eval numOne %random.8%
eval numTwo %random.6%
eval numThr %random.16%
*
set fruit1 apple
set fruit2 banana
set fruit3 pear
set fruit4 orange
set fruit5 kiwi
set fruit6 starfruit
set fruit7 peach
set fruit8 strawberry
set color1 @Rred@n
set color2 @ybrown@n
set color3 @Yyellow@n
set color4 @Ggreen@n
set color5 @Bblue@n
set color6 @Mpurple@n
set spell1 cure light
set spell2 heal
set spell3 poison
set spell4 blind
set spell5 bestow curse
set spell6 remove blind
set spell7 inflict light
set spell8 bless
set spell9 cure light
set spell10 invisibility
set spell11 bull strength
set spell12 sense life
set spell13 bless
set spell14 sleep
set spell15 sanc
set spell16 chill touch
*
set type %%fruit%numOne%%%
set desc %%color%numTwo%%%
set affect %%spell%numThr%%%
*
eval type %type%
eval desc %desc%
eval affect %affect%
eval target %random.char%
*
eval thisRoom %self.vnum%
eval total %%people.%thisRoom%%%
if %total% != 0
  %send% %target% A %desc% %type% falls from the tree and hits you.
  %echoaround% %target% A %desc% %type% falls from the tree and hits %target.name%.
  dg_cast '%affect%' %target%
  wait 1 sec
  %echo% The %type% falls to the ground and vanishes.
end
~
#13018
ray of light portal back to midgaard~
1 c 7
en~
if %cmd.mudcommand% == enter && %arg% /= light
  %send% %actor% You step in to the light.
  %echoaround% %actor% %actor.name% steps in to the light.
  %teleport% %actor% 3151
  %force% %actor% look
else
  %send% %actor% %cmd% Huh?!
end
~
#13099
portal to enter the mist maze~
1 c 7
en~
if %cmd.mudcommand% == enter && %arg% /= mist
  %send% %actor% You step in to the swirling mist.
  %echoaround% %actor% %actor.name% steps in to the swirling mist.
  %teleport% %actor% 13001
  %force% %actor% look
  %echoaround% %actor% %actor.name% causes the mists to swirl and change.
else
  %send% %actor% %cmd% Huh?!
end
~
$~
