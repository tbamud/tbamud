#1100
Weather messages for zone~
2 b 15
~
switch %random.8%
  case 1
    %echo% A fiery streak of lightning pierces the stormclouds with a crash.
  break
  case 2
    %echo% The black sky swirls ominously, wind wailing as it stirs the air.
  break
  case 3
    %echo% A smattering of hail sprinkles the ground with tiny crystals of ice.
  break
  case 4
    %echo% The land is drenched with a sudden fleeting downpour of rain.
  break
  case 5
    %echo% The clouds rumble uneasily.
  break
  case 6
    %echo% A crack of thunder booms threateningly overhead.
  break
  case 7
    %echo% The wind shrieks, whipping over the land with a sudden frenzied speed.
  break
  case 8
    %echo% The land flickers like a silver flame, illuminated by flashes of lightning.
  break
  default
    %echo% Something is broken, please report.
  break
done
~
#1101
(18/09/08) Acts seen throughout hall~
0 e 1
*~
eval act %arg%
if %arg.car% != In
  if %arg.car% != At
    if %self.room.vnum% == 1108
      %at% 1118 %echo% At the eastern end - %act%
      %at% 1109 %echo% At the eastern end - %act%
    elseif %self.room.vnum% == 1109
      %at% 1118 %echo% In the central hall - %act%
      %at% 1108 %echo% In the central hall - %act%
    elseif %self.room.vnum% == 1118
      %at% 1108 %echo% At the western end - %act%
      %at% 1109 %echo% At the western end - %act%
    end
  end
end
~
#1102
(18/09/08) Speech heard throughout hall~
2 d 1
*~
if %self.vnum% == 1108
  %at% 1118 %echo% At the eastern end - %actor.name% says, '%speech%'
  %at% 1109 %echo% At the eastern end - %actor.name% says, '%speech%'
elseif %self.vnum% == 1109
  %at% 1118 %echo% In the central hall - %actor.name% says, '%speech%'
  %at% 1108 %echo% In the central hall - %actor.name% says, '%speech%'
elseif %self.vnum% == 1118
  %at% 1108 %echo% At the western end - %actor.name% says, '%speech%'
  %at% 1109 %echo% At the western end - %actor.name% says, '%speech%'
end
~
#1103
test for Jim~
2 c 100
test~
* Dex gives percent
* 18       -        0
* 17       -        10
* 16       -        20
* 15       -        30
* 14       -        40
* 13       -        50
* 12       -        60
* 11       -        70
* 10       -        80
* 9       -        90
* 8       -        100
*
%echo% Dex is %actor.dex()%
eval num 100 - (((%actor.dex()% + 2) - 10) * 10)
%echo% chance is %num% percent of getting hit
~
#1104
(00) Wraith reincarnates self, no corpse~
0 f 100
~
%load% mob 1100
wait 2 s
%purge% self
~
#1105
(08/09/18) can't interact with wraiths~
0 c 100
*~
if %arg% /= wr || %arg% /= wra || %arg% /= wrai || %arg% /= wrait || %arg% /= wraith
  %send% %actor% %self.name% flickers out of existance briefly when you try to interact with it.
else
  return 0
end
~
#1106
new trigger~
1 c 1
shoot~
eval actor %self.worn_by%
eval origin %actor.room.vnum%
eval tar %arg%
eval place %tar.room.vnum%
eval originn %origin% + 10
eval origins %origin% - 10
if %place% >= %originn%
  switch n
  elseif %place% <= %origins%
    switch s
    elseif %place% > %origin%
      switch e
      elseif %place% < %origin%
        switch w
        end
        case n
          %echo% The missile fires north.
          eval next %origin% + 10
          while %next% < %place%
            %at% %next% %echo% A missile whizzes past.
            eval next %next% + 10
          done
          %at% %place% %echo% A missile hits its target.
        break
        case e
          %echo% The missile fires east.
          eval next %origin% + 1
          while %next% < %place%
            %at% %next% %echo% A missile whizzes past.
            eval next %next% + 1
          done
          %at% %place% %echo% A missile hits its target.
        break
        case s
          %echo% The missile fires south.
          eval next %origin% - 10
          while %next% > %place%
            %at% %next% %echo% A missile whizzes past.
            eval next %next% - 10
          done
          %at% %place% %echo% A missile hits its target.
        break
        case w
          %echo% The missile fires west.
          eval next %origin% - 1
          while %next% > %place%
            %at% %next% %echo% A missile whizzes past.
            eval next %next% - 1
          done
          %at% %place% %echo% A missile hits its target.
        break
        default
          %echo% Something is broken.
        break
      done
    done
  done
done
~
#1107
(04) play piano~
1 c 100
play~
if %arg% == piano
  eval here %self.room.vnum%
  %send% %actor% Your fingers dance along the keys of %self.shortdesc%.
  %echoaround% %actor% %actor.name%'s fingers dance along the keys of %self.shortdesc%.
  %zoneecho% %here% A delicate tinkling melody drifts forlornly through the air.
else
  %send% %actor% Play what?
end
~
#1108
test for mick~
2 c 100
test~
if %cmd% == test
  %echo% trigger fires
  if (%arg% == this is a test)
    %echo% second part works
  else
    %echo% second part doesn't work
  end
end
~
#1109
(08) bracelet gives moves~
1 c 1
draw~
if %arg% == bracelet
  %send% %actor%  @CYou attempt to draw on the power of %self.shortdesc%.@n
  %echoaround% %actor%  @C%actor.name% attempts to draw on the power of %self.shortdesc%.@n
  wait 1 s
  if %self.timer% == 0
    eval give %actor.maxmove% - %actor.move%
    eval percent (%give% * 8) / 10
eval power %actor.move(%percent%)%
    set %power%
    %send% %actor%  @CYou glow with energy as a powerful wind erupts from %self.shortdesc% and envelops you. @n
    %echoaround% %actor%  @C%actor.name% glows with energy as a powerful wind erupts from %self.shortdesc% and envelops %actor.himher%. @n
    otimer 20
  else
    %send% %actor% @cAlas, the power of %self.shortdesc% has not yet recovered.@n
  end
end
return 0
~
#1110
test for string combos~
2 c 100
test~
set X 1
set Y 2
set Z 3
eval string %X% %Y% %Z%
eval string2 %X%%Y%%Z%
%echo% string is %string%
%echo% string2 is %string2%
~
#1111
test for remoting~
0 c 100
test~
%echo% %self.name%
set test 1
remote test %self.id%
%echo% self test is %self.test%
~
#1112
new trigger~
1 i 100
~
%echo% %victim.vnum%
%echo% fired
%send% detta test
~
#1113
All mobs put to sleep~
2 c 100
test~
set target %self.people%
while %target%
  set next_target %target.next_in_room%
  if %target.is_pc%
    %send% detta targetted pc
    %send% detta %target%
  else
    dg_affect %target% sleep on 20
    %send% detta put mob to sleep
    %send% detta %target%
  end
  set target %next_target%
done
~
#1114
mobs put selves to sleep~
0 n 100
~
dg_affect %self%  blind on 1
dg_affect %self% sleep on 2
%echo% sleeping now
~
#1115
can't be interacted with~
0 c 100
*~
if %self.alias% /= %arg% || %arg% /= %self.alias%
  %send% %actor% %self.name% is frozen by some unnatural magic.
else
  return 0
end
~
#1116
Rooms don't allow casting~
2 p 100
~
%send% %actor% The powerful magic of this place absorbs your spell.
~
#1117
Mobs get out of fight~
0 k 100
~
%echo% %self.name% is reclaimed by the magic of this land.
%load% mob %self.vnum%
%purge% self
~
#1118
doors close themselves~
2 g 100
~
wait 2s
eval room %self.vnum%
if %room% == 1145
  if %direction% == west
    %door% %self.vnum% west flags ab
    %echo% The door clicks quietly shut.
  elseif %direction% == east
    %door% %self.vnum% east flags ab
    %echo% The door clicks quietly shut.
  end
elseif %room% == 1144
  if %direction% == east
    %door% %self.vnum% east flags ab
    %echo% The door clicks quietly shut.
  end
elseif %room% ==1146
  if %direction% == west
    %door% %self.vnum% west flags ab
    %echo% The door clicks quietly shut.
  end
end
~
#1119
testing act~
0 e 100
*~
%echo% %arg%
~
#1120
testing room dirs~
1 c 2
test~
set dest %arg%
eval start %actor.room.vnum%
eval difference %start% - %dest%
if !(%difference% > 100 || %difference% < -100)
  set dest %arg%
  set remotes 1
  remote remotes %actor.id%
  if !(%actor.varexists(roomstocheck)%)
    set roomstocheck 1
    remote roomstocheck %actor.id%
  end
  eval here %actor.room.vnum%
  set originset 1
  remote originset %actor.id%
  while %actor.originset% < 50
    if %here.north%
      eval northexit %here.north(vnum)%
      eval check %actor.roomstocheck%
      if !(%check.contains(%northexit%)%)
        if !(%actor.varexists(%northexit%)%)
          if %actor.varexists(%here%)%
            eval path %%actor.%here%%%
            eval path %path.trim%
            if !(%path% == here)
              eval %northexit% %path% n
              eval northexit %northexit.trim%
            else
              eval %northexit% n
            end
          else
            set %here% here
            remote %here% %actor.id%
            set remotes %remotes% %here%
            remote remotes %actor.id%
            set %northexit% n
          end
          remote %northexit% %actor.id%
          set remotes %remotes% %northexit%
          remote remotes %actor.id%
          if %northexit% == %dest%
            set originset 999
            remote originset %actor.id%
          end
          eval roomstocheck %actor.roomstocheck% %northexit%
          remote roomstocheck %actor.id%
        end
      end
    end
    if %here.east%
      eval eastexit %here.east(vnum)%
      eval check %actor.roomstocheck%
      if !(%check.contains(%eastexit%)%)
        if !(%actor.varexists(%eastexit%)%)
          if %actor.varexists(%here%)%
            eval path %%actor.%here%%%
            eval path %path.trim%
            if !(%path% == here)
              eval %eastexit% %path% e
              eval eastexit %eastexit.trim%
            else
              eval %eastexit% e
            end
          else
            set %here% here
            remote %here% %actor.id%
            set remotes %remotes% %here%
            remote  %remotes% %actor.id%
            set %eastexit% e
          end
          remote %eastexit% %actor.id%
          set remotes %remotes% %eastexit%
          remote remotes %actor.id%
          if %eastexit% == %dest%
            set originset 999
            remote originset %actor.id%
          end
          eval roomstocheck %actor.roomstocheck% %eastexit%
          remote roomstocheck %actor.id%
        end
      end
    end
    if %here.south%
      eval southexit %here.south(vnum)%
      eval check %actor.roomstocheck%
      if !(%check.contains(%southexit%)%)
        if !(%actor.varexists(%southexit%)%)
          if %actor.varexists(%here%)%
            eval path %%actor.%here%%%
            eval path %path.trim%
            if !(%path% == here)
              eval %southexit% %path% s
              eval southexit %southexit.trim%
            else
              eval %southexit% s
            end
          else
            set %here% here
            remote %here% %actor.id%
            set remotes %remotes% %here%
            remote remotes %actor.id%
            set %southexit% s
          end
          remote %southexit% %actor.id%
          set remotes %remotes% %southexit%
          remote remotes %actor.id%
          if %southexit% == %dest%
            set originset 999
            remote originset %actor.id%
          end
          eval roomstocheck %actor.roomstocheck% %southexit%
          remote roomstocheck %actor.id%
        end
      end
    end
    if %here.west%
      eval westexit %here.west(vnum)%
      eval check %actor.roomstocheck%
      if !(%check.contains(%westexit%)%)
        if !(%actor.varexists(%westexit%)%)
          if %actor.varexists(%here%)%
            eval path %%actor.%here%%%
            eval path %path.trim%
            if !(%path% == here)
              eval %westexit% %path% w
              eval westexit %westexit.trim%
            else
              eval %westexit% w
            end
          else
            set %here% here
            remote %here% %actor.id%
            set remotes %remotes% %here%
            remote remotes %actor.id%
            set %westexit% w
          end
          remote %westexit% %actor.id%
          set remotes %remotes% %westexit%
          remote remotes %actor.id%
          if %westexit% == %dest%
            set originset 999
            remote originset %actor.id%
          end
          eval roomstocheck %actor.roomstocheck% %westexit%
          remote roomstocheck %actor.id%
        end
      end
    end
    if %here.up%
      eval upexit %here.up(vnum)%
      eval check %actor.roomstocheck%
      if !(%check.contains(%upexit%)%)
        if !(%actor.varexists(%upexit%)%)
          if %actor.varexists(%here%)%
            eval path %%actor.%here%%%
            eval path %path.trim%
            if !(%path% == here)
              eval %upexit% %path% u
              eval upexit %upexit.trim%
            else
              eval %upexit% u
            end
          else
            set %here% here
            remote %here% %actor.id%
            set remotes %remotes% %here%
            remote remotes %actor.id%
            set %upexit% u
          end
          remote %upexit% %actor.id%
          set remotes %remotes% %upexit%
          remote remotes %actor.id%
          if %upexit% == %dest%
            set originset 999
            remote originset %actor.id%
          end
          eval roomstocheck %actor.roomstocheck% %upexit%
          remote roomstocheck %actor.id%
        end
      end
    end
    if %here.down%
      eval downexit %here.down(vnum)%
      eval check %actor.roomstocheck%
      if !(%check.contains(%downexit%)%)
        if !(%actor.varexists(%downexit%)%)
          if %actor.varexists(%here%)%
            eval path %%actor.%here%%%
            eval path %path.trim%
            if !(%path% == here)
              eval %downexit% %path% d
              eval downexit %downexit.trim%
            else
              eval %downexit% d
            end
          else
            set %here% here
            remote %here% %actor.id%
            set remotes %remotes% %here%
            remote remotes %actor.id%
            set %downexit% d
          end
          remote %downexit% %actor.id%
          set remotes %remotes% %downexit%
          remote remotes %actor.id%
          if %downexit% == %dest%
            set originset 999
            remote originset %actor.id%
          end
          eval roomstocheck %actor.roomstocheck% %downexit%
          remote roomstocheck %actor.id%
        end
      end
    end
    eval originset %actor.originset% + 1
    remote originset %actor.id%
    eval nexttar %actor.roomstocheck.cdr%
    eval nexttar %nexttar.trim%
    eval nexttar2 %nexttar.cdr%
    eval nexttar2 %nexttar2.trim%
    eval here %nexttar.car%
    eval here %here.trim%
    eval roomstocheck 1 %nexttar2%
    remote roomstocheck %actor.id%
  done
  if %originset% == 1000
    eval directions %%actor.%dest%%%
    remote directions %actor.id%
    %send% %actor% Your destination has been found.
    wait 2s
    %send% %actor% The directions are %directions%.
  else
    %send% %actor% Your destination is either too far away, or does not exist.
  end
  while %actor.remotes% > 1
    eval rem %actor.remotes%
    eval delete %rem.car%
    rdelete %delete% %actor.id%
    eval remotes %rem.cdr%
    remote remotes %actor.id%
  done
  rdelete remotes %actor.id%
  rdelete originset %actor.id%
  rdelete roomstocheck %actor.id%
else
  %send% %actor% Try somewhere closer.
end
~
#1126
(22) raise bucket from well~
1 c 4
*~
if %cmd% == raise
  %send% %actor% You turn the crank, slowly raising the bucket from the depths.
  %load% obj 1121
elseif %cmd% == lower
  %send% %actor% The bucket is already lowered. You need to raise it.
  %load% obj 1122
else
  %load% obj 1122
  return 0
end
%purge% self
~
#1127
(21) bucket loads something~
1 c 4
*~
if %cmd% == empty
  if %118_bucket% > 1
    %send% %actor% The bucket has already been emptied.
  else
    eval choice %random.20%
    if %choice% > 15
      nop %actor.gold(1)%
      %send% %actor% You found a gold coin in the bucket.
    elseif %choice% < 15
      %load% obj 1124 %actor% inv
      %send% %actor% You found a pile of sludge in the bucket.
    else
      %load% obj 1108 %actor% inv
      %send% %actor% You found a bracelet in the bucket.
    end
    set 118_bucket 99
    global 118_bucket
  end
elseif %cmd% == lower
  %send% %actor% You lower the bucket into the well.
  set 118_bucket 1
  global 118_bucket
  %load% obj 1122
  %purge% self
elseif %cmd% == raise
  %send% %actor% The bucket has already been raised.
else
  return 0
end
~
#1128
bucket teleports self~
1 c 4
xxteleport~
%teleport% %self.name% 1188
%echo% teleporting
~
#1133
testing spiderball~
0 c 100
scout~
if %cmd% == scout
if %arg%
set cycles 0
global cycles
eval directions %arg%
global directions
%send% %actor% initial directions are %directions%
while %directions%
if %cycles% < 5
%send% %actor% directions now are %directions%
eval here %self.room.vnum%
%send% %actor% Here is %here%
eval nextdir %directions.car%
%send% %actor% nextdir is %nextdir%
switch %nextdir%
case n
set going north
set coming south
eval dest %here.north(vnum)%
break
case e
set going east
set coming west
eval dest %here.east(vnum)%
break
case s
set going south
set coming north
eval dest %here.south(vnum)%
break
case w
set going west
set coming east
eval dest %here.west(vnum)%
break
case u
set going up
set coming down
eval dest %here.up(vnum)%
break
case d
set going down
set coming up
eval dest %here.down(vnum)%
break
case ne
set going northeast
set coming southwest
eval dest %here.northeast(vnum)%
break
case se
set going southeast
set coming northwest
eval dest %here.southeast(vnum)%
break
case sw
set going southwest
set coming northeast
eval dest %here.southwest(vnum)%
break
case nw
set going northwest
set coming southeast
eval dest %here.northwest(vnum)%
break
case default
%echo% %Self.name% sparks and sends out a puff of smoke as it bumps into the wall.
%send% %actor% %Self.name% damaged itself trying to go %nextdir%.
if %damage%
eval damage %self.damage%+10
else 
set damage 10
end
global damage
break
done
if %dest%
%echo% %Self.name% clatters away into the %going%.
%send% %actor% teleporting self to %dest%
%teleport% %self% %dest%
%echo% %Self.name% clatters in from the %coming%.
else
%send% %actor% %Self.name% damaged itself trying to go %nextdir%.
%echo% %Self.name% sparks and sends out a puff of smoke as it bumps into the wall.
end
eval directions %directions.cdr%
eval cycles %cycles%+1
global cycles
done
else
unset directions
end
wait 1s
eval finaldest %self.room.vnum%
%send% %actor%  %Self.name% reports:
eval back %actor.room.vnum%
%teleport% %actor% %finaldest%
%force% %actor% look
%teleport% %actor% %back%
else
%send% %actor% You need to provide a list of directions.
%send% %actor% Example:
%send% %actor% scout n w w
end
else
return 0
end
~
#1134
MAP~
1 c 3
map~
* BEGINNING SETUP
*
set display 11
while %display% < 56
  eval x%display% .
  eval display %display% + 1
done
eval here %actor.room.vnum%
set north -10
set east 1
set west -1
set south 10
*
* STARTS READING NORTH
*
if %here.north%
  eval 23 %here.north(vnum)%
  eval temp %23.sector%
  eval x23 %temp.charat(1)%
  set directionlist north east south west
  while %directionlist%
    eval firstdir %directionlist.car%
    eval newroom 23 + %%%firstdir%%%
    eval %newroom% %%23.%firstdir%(vnum)%%
    eval vnumexist %%%newroom%%%
    if %vnumexist%
      eval temp %%%newroom%.sector%%
      eval x%newroom% %temp.charat(1)%
    end
    eval directionlist %directionlist.cdr%
  done
end
*
* START OF EAST
*
if %here.east%
  eval 34 %here.east(vnum)%
  eval temp %34.sector%
  eval x34 %temp.charat(1)%
  set directionlist north east south west
  while %directionlist%
    eval firstdir %directionlist.car%
    eval newroom 34 + %%%firstdir%%%
    eval %newroom% %%34.%firstdir%(vnum)%%
    eval vnumexist %%%newroom%%%
    if %vnumexist%
      eval temp %%%newroom%.sector%%
      eval x%newroom% %temp.charat(1)%
    end
    eval directionlist %directionlist.cdr%
  done
end
*
* START OF SOUTH
*
if %here.south%
  eval 43 %here.south(vnum)%
  eval temp %43.sector%
  eval x43 %temp.charat(1)%
  set directionlist north east south west
  while %directionlist%
    eval firstdir %directionlist.car%
    eval newroom 43 + %%%firstdir%%%
    eval %newroom% %%43.%firstdir%(vnum)%%
    eval vnumexist %%%newroom%%%
    if %vnumexist%
      eval temp %%%newroom%.sector%%
      eval x%newroom% %temp.charat(1)%
    end
    eval directionlist %directionlist.cdr%
  done
end
*
* START OF WEST
*
if %here.west%
  eval 32 %here.west(vnum)%
  eval temp %32.sector%
  eval x32 %temp.charat(1)%
  set directionlist north east south west
  while %directionlist%
    eval firstdir %directionlist.car%
    eval newroom 32 + %%%firstdir%%%
    eval %newroom% %%32.%firstdir%(vnum)%%
    eval vnumexist %%%newroom%%%
    if %vnumexist%
      eval temp %%%newroom%.sector%%
      eval x%newroom% %temp.charat(1)%
    end
    eval directionlist %directionlist.cdr%
  done
end
*
* FINAL REPORT
*
eval x33 X
%send% %actor% %x11% %x12% %x13% %x14% %x15%
%send% %actor% %x21% %x22% %x23% %x24% %x25%
%send% %actor% %x31% %x32% %x33% %x34% %x35%
%send% %actor% %x41% %x42% %x43% %x44% %x45%
%send% %actor% %x51% %x52% %x53% %x54% %x55%
*
* END
~
#1135
test for Fiz~
0 c 100
*~
if %cmd% == sell
if %actor.canbeseen%
if %arg%
eval item %actor.inventory%
while %item%
if %item.name% /= %arg%
eval object %item%
eval item 0
else
eval item %item.next_in_list%
end
done
if %object%
eval desc %object.shortdesc%
switch %object.type%
case LIGHT
case WAND
case STAFF
case WEAPON
case TREASURE
case ARMOR
case WORN
case CONTAINER
case LIQ CONTAINER
case DUAL
case GEM
case FISHING POLE
eval value ((%object.cost% * 4) / 10)
set %actor.gold(%value%)%
%purge% %object%
            if %value% == 1
say There you go %actor.name% I took %arg% off your hands for 1 gold coin.
else
say There you go %actor.name% I took %arg% off your hands for %value% gold coins.
end
break
default
say Sorry %actor.name% but I don't buy these types of items.
break
done
else
say Sorry %actor.name%, but you don't have a %arg% to sell.
end
else
%send% %actor% Yes, sell, but sell what?? That is the question.
end
else
say I don't buy from people I can't see.
end
else
return 0
end
~
#1136
test dg_cast~
2 g 100
~
set target %self.people%
while %target%
  set nexttarget %target.next_in_room%
  if %target.is_pc%
    %damage% %target% 5
    %send% %target% A dart whizzes through the air and pierces your skin!
    %echoaround% %target% %target.name% gets hit by a dart!
    eval chance %random.8%
    if %chance%
      dg_cast 'poison' %target%
    end
  end
  set target %nexttarget%
done
~
#1137
test class~
2 c 100
test~
%echo% firing
nop %actor.class(cleric)%
set %actor.class(cleric)%
%actor.class(cleric)%
~
#1138
test imm force~
2 c 100
test~
%force% testdet say test
%force% testdet redit
~
#1139
test echo card~
2 c 100
test~
set UL1 _
set UM1 _
set UR1 _
set UL4 |
set UL2 |
set UM4 |
set UM2 |
set UR4 |
set UR2 |
set ULn ...
set UMn ...
set URn ...
set UL3 _
set UM3 _
set UR3 _
set ML1 _
set MM1 _
set MR1 _
set ML4 |
set ML2 |
set MM4 |
set MM2 |
set MR4 |
set MR2 |
set MLn ...
set MMn ...
set MRn ...
set ML3 _
set MM3 _
set MR3 _
set LL1 _
set LM1 _
set LR1 _
set LL4 |
set LL2 |
set LM4 |
set LM2 |
set LR4 |
set LR2 |
set LLn ...
set LMn ...
set LRn ...
set LL3 _
set LM3 _
set LR3 _
global UL1
global UL2
global UL3
global UL4
global UM1
global UM2
global UM3
global UM4
global UR1
global UR2
global UR3
global UR4
global ML1
global ML2
global ML3
global ML4
global MM1
global MM2
global MM3
global MM4
global MR1
global MR2
global MR3
global MR4
global LL1
global LL2
global LL3
global LL4
global LM1
global LM2
global LM3
global LM4
global LR1
global LR2
global LR3
global LR4
set ulc w
set umc w
set urc w
set mlc w
set mmc w
set mrc w
set llc w
set lmc w
set lrc w
global ulc
global umc
global urc
global mlc
global mmc
global mrc
global llc
global lmc
global lrc
%echo% @n _______________
%echo% @n| @%ulc%_@Y%UL1%@%ulc%_  @%umc%_@Y%UM1%@%umc%_  @%urc%_@Y%UR1%@%urc%_@n |
%echo% @n|@%ulc%|   |@%umc%|   |@%urc%|   |@n|
%echo% @n|@Y%UL4%   %UL2%%UM4%   %UM2%%UR4%   %UR2%@n|
%echo% @n|@%ulc%|@u%ULn%@n@%ulc%|@%umc%|@u%UMn%@n@%umc%|@%urc%|@u%URn%@n@%urc%|@n|
%echo% @n|@%ulc%|_@Y%UL3%@%ulc%_|@%umc%|_@Y%UM3%@%umc%_|@%urc%|_@Y%UR3%@%urc%_|@n|
%echo% @n|@%mlc% _@Y%ML1%@%mlc%_  @%mmc%_@Y%MM1%@%mmc%_  @%mrc%_@Y%MR1%@%mrc%_@n |
%echo% @n|@%mlc%|   |@%mmc%|   |@%mrc%|   |@n|
%echo% @n|@Y%ML4%   %ML2%%MM4%   %MM2%%MR4%   %MR2%@n|
%echo% @n|@%mlc%|@u%MLn%@n@%mlc%|@%mmc%|@u%MMn%@n@%mmc%|@%mrc%|@u%MRn%@n@%mrc%|@n|
%echo% @n|@%mlc%|_@Y%ML3%@%mlc%_|@%mmc%|_@Y%MM3%@%mmc%_|@%mrc%|_@Y%MR3%@%mrc%_|@n|
%echo% @n|@%llc% _@Y%LL1%@%llc%_  @%lmc%_@Y%LM1%@%lmc%_  @%lrc%_@Y%LR1%@%lrc%_ @n|
%echo% @n|@%llc%|   |@%lmc%|   |@%lrc%|   |@n|
%echo% @n|@Y%LL4%   %LL2%%LM4%   %LM2%%LR4%   %LR2%@n|
%echo% @n|@%llc%|@u%LLn%@n@%llc%|@%lmc%|@u%LMn%@n@%lmc%|@%lrc%|@u%LRn%@n@%lrc%|@n|
%echo% @n|@%llc%|_@Y%LL3%@%llc%_|@%lmc%|_@Y%LM3%@%lmc%_|@%lrc%|_@Y%LR3%@%lrc%_|@n|
%echo% @n|_______________|
~
#1140
dealer say new game~
0 d 0
new game~
wait 1s
if %actor.varexists(ffplaying)%
  %send% %actor% @GYou are already playing a game.@n
  %send% %actor% @GIf you forfeit, you will lose the cards you've won.@n
  %send% %actor% @GYour opponent will keep the cards they've won.@n
  %send% %actor% @GSay forfeit to accept this and end the game.@n
  %send% %actor% @GSay continue to carry on with your current game.@n
elseif %playing%
  %echo% Sorry, I'm already playing a game.
else
  %send% %actor% Starting new game!
  set UL1 _
  set UM1 _
  set UR1 _
  set UL4 |
  set UL2 |
  set UM4 |
  set UM2 |
  set UR4 |
  set UR2 |
  set ULn ...
  set UMn ...
  set URn ...
  set UL3 _
  set UM3 _
  set UR3 _
  set ML1 _
  set MM1 _
  set MR1 _
  set ML4 |
  set ML2 |
  set MM4 |
  set MM2 |
  set MR4 |
  set MR2 |
  set MLn ...
  set MMn ...
  set MRn ...
  set ML3 _
  set MM3 _
  set MR3 _
  set LL1 _
  set LM1 _
  set LR1 _
  set LL4 |
  set LL2 |
  set LM4 |
  set LM2 |
  set LR4 |
  set LR2 |
  set LLn ...
  set LMn ...
  set LRn ...
  set LL3 _
  set LM3 _
  set LR3 _
  global lrn
  global lmn
  global lln
  global mrn
  global mmn
  global mln
  global umn
  global urn
  global uln
  global UL1
  global UL2
  global UL3
  global UL4
  global UM1
  global UM2
  global UM3
  global UM4
  global UR1
  global UR2
  global UR3
  global UR4
  global ML1
  global ML2
  global ML3
  global ML4
  global MM1
  global MM2
  global MM3
  global MM4
  global MR1
  global MR2
  global MR3
  global MR4
  global LL1
  global LL2
  global LL3
  global LL4
  global LM1
  global LM2
  global LM3
  global LM4
  global LR1
  global LR2
  global LR3
  global LR4
  set ulc w
  set umc w
  set urc w
  set mlc w
  set mmc w
  set mrc w
  set llc w
  set lmc w
  set lrc w
  global ulc
  global umc
  global urc
  global mlc
  global mmc
  global mrc
  global llc
  global lmc
  global lrc
  set ffplaying 1
  remote ffplaying %actor.id%
  set playing 1
  global playing
  wait 1s
  %echo% Heads you start, tails I start.
  wait 1s
  %echo% flips a coin...
  wait 2s
  switch 2
    case 1
      %echo% The coin comes up heads.
      wait 1s
      %echo% Ok, you go first, choose your card and position.
    break
    case 2
      %echo% The coin comes up tails.
      wait 1s
      %echo% Ok, I'll go first.
      %force% %actor% xxxchoose
    break
    default
      %echo% The coin comes up heads.
      wait 1s
      %echo% Ok, you go first, choose your card and position.
    break
  done
end
~
#1141
mob xxxchoose~
2 c 100
xxxchoose~
switch %random.5%
  case 1
    set cardchoice 001
  break
  case 2
    set cardchoice 002
  break
  case 3
    set cardchoice 003
  break
  case 4
    set cardchoice 004
  break
  case 5
    set cardchoice 005
  break
  default
    set cardchoice 005
  break
done
global cardchoice
switch %random.9%
  case 1
    set cardpos UL
  break
  case 2
    set cardpos UM
  break
  case 3
    set cardpos UR
  break
  case 4
    set cardpos ML
  break
  case 5
    set cardpos MM
  break
  case 6
    set cardpos MR
  break
  case 7
    set cardpos LL
  break
  case 8
    set cardpos LM
  break
  case 9
    set cardpos LR
  break
done
global cardpos
%force% %actor% xxxcardchoice %cardchoice%
~
#1142
actor xxxcardchoice~
2 c 100
xxxcardchoice~
%echo% I'm going to put card %cardchoice% in the %cardpos% position.
switch %arg%
  case 001
    set %cardpos%1 2
    set %cardpos%2 1
    set %cardpos%3 2
    set %cardpos%4 1
  break
  case 002
    set %cardpos%1 1
    set %cardpos%2 2
    set %cardpos%3 1
    set %cardpos%4 2
  break
  case 003
    set %cardpos%1 2
    set %cardpos%2 2
    set %cardpos%3 2
    set %cardpos%4 1
  break
  case 004
    set %cardpos%1 2
    set %cardpos%2 1
    set %cardpos%3 2
    set %cardpos%4 2
  break
  case 005
    set %cardpos%1 2
    set %cardpos%2 2
    set %cardpos%3 2
    set %cardpos%4 2
  break
done
global %cardpos%1
global %cardpos%2
global %cardpos%3
global %cardpos%4
~
#1143
show board~
2 c 100
show~
%echo% @n _______________
%echo% @n| @%ulc%_@Y%UL1%@%ulc%_  @%umc%_@Y%UM1%@%umc%_  @%urc%_@Y%UR1%@%urc%_@n |
%echo% @n|@%ulc%|   |@%umc%|   |@%urc%|   |@n|
%echo% @n|@Y%UL4%   %UL2%%UM4%   %UM2%%UR4%   %UR2%@n|
%echo% @n|@%ulc%|@u%ULn%@n@%ulc%|@%umc%|@u%UMn%@n@%umc%|@%urc%|@u%URn%@n@%urc%|@n|
%echo% @n|@%ulc%|_@Y%UL3%@%ulc%_|@%umc%|_@Y%UM3%@%umc%_|@%urc%|_@Y%UR3%@%urc%_|@n|
%echo% @n|@%mlc% _@Y%ML1%@%mlc%_  @%mmc%_@Y%MM1%@%mmc%_  @%mrc%_@Y%MR1%@%mrc%_@n |
%echo% @n|@%mlc%|   |@%mmc%|   |@%mrc%|   |@n|
%echo% @n|@Y%ML4%   %ML2%%MM4%   %MM2%%MR4%   %MR2%@n|
%echo% @n|@%mlc%|@u%MLn%@n@%mlc%|@%mmc%|@u%MMn%@n@%mmc%|@%mrc%|@u%MRn%@n@%mrc%|@n|
%echo% @n|@%mlc%|_@Y%ML3%@%mlc%_|@%mmc%|_@Y%MM3%@%mmc%_|@%mrc%|_@Y%MR3%@%mrc%_|@n|
%echo% @n|@%llc% _@Y%LL1%@%llc%_  @%lmc%_@Y%LM1%@%lmc%_  @%lrc%_@Y%LR1%@%lrc%_ @n|
%echo% @n|@%llc%|   |@%lmc%|   |@%lrc%|   |@n|
%echo% @n|@Y%LL4%   %LL2%%LM4%   %LM2%%LR4%   %LR2%@n|
%echo% @n|@%llc%|@u%LLn%@n@%llc%|@%lmc%|@u%LMn%@n@%lmc%|@%lrc%|@u%LRn%@n@%lrc%|@n|
%echo% @n|@%llc%|_@Y%LL3%@%llc%_|@%lmc%|_@Y%LM3%@%lmc%_|@%lrc%|_@Y%LR3%@%lrc%_|@n|
%echo% @n|_______________|
~
#1144
new trigger~
2 c 100
test~
%echo% var is %var%
%echo% var2 is %var2%
%echo% coloured var is @Y%var%@n
set col Y
%echo% var coloured var is @%col%%var%@n
~
$~
