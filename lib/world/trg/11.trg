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
    eval power %%actor.move(%percent%)%
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
eval X 1
eval Y 2
eval Z 3
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
eval test 1
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
  %send% %actor.name% %self.name% is frozen by some unnatural magic.
else
  return 0
end
~
#1116
Rooms don't allow casting~
2 p 100
~
%send% %actor.name% The powerful magic of this place absorbs your spell.
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
            eval originset 999
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
            eval originset 999
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
            eval originset 999
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
            eval originset 999
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
            eval originset 999
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
            eval originset 999
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
#1121
test room dirs~
1 c 2
test2~
eval here %%actor.%here%%
%echo% here is %here%
if %here.east%
  eval eastexit %here.east(vnum)%
  eval check %actor.roomstocheck%
  if !(%check.contains(%eastexit%)%)
    if !(%actor.varexists(%eastexit%)%)
      if %actor.varexists(%here%)%
        eval path %%actor.%here%%
        if !(%path% == here)
          eval %eastexit% %path% e
        else
          eval %eastexit% e
        end
      else
        eval %here% here
        remote %here% %actor.id%
        eval %eastexit% e
      end
      remote %eastexit% %actor.id%
      eval roomstocheck %actor.roomstocheck% %eastexit%
      remote roomstocheck %actor.id%
    end
  end
end
~
#1122
test room dirs~
1 c 2
test3~
%echo% firing
eval here %actor.here%
%echo% here is %here%
~
#1123
test room dirs~
1 c 2
test4~
if %here.west%
  eval westexit %here.west(vnum)%
  eval check %actor.roomstocheck%
  if !(%check.contains(%westexit%)%)
    if !(%actor.varexists(%westexit%)%)
      if %actor.varexists(%here%)%
        eval path %%actor.%here%%
        eval path %path.trim%
        if !(%path% == here)
          eval %westexit% %path% w
          eval westexit %westexit.trim%
        else
          eval %westexit% w
        end
      else
        eval %here% here
        remote %here% %actor.id%
        eval %westexit% w
      end
      remote %westexit% %actor.id%
      eval roomstocheck %actor.roomstocheck% %westexit%
      remote roomstocheck %actor.id%
    end
  end
end
return 0
~
#1124
testing act~
0 e 100
*~
%echo% %arg%
~
#1125
testing random dir~
1 c 2
dir~
* No Script
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
#1129
test zoneecho~
2 c 100
test~
%zoneecho% 1101 testing testing
~
#1130
testing findobj~
2 c 100
test~
eval here %%findobj.%self.vnum%(xxtree)%%%
if %here% > 0
  %purge% xxtree
end
switch %time.month%
  case 1
    case 2
    case 3
    case 4
    set season winter
    %load% obj 1974
  break
  case 5
    set season season of the Grand Struggle
  break
  case 6
    case 7
    case 8
    case 9
    set season spring
    %load% obj 1975
  break
  case 10
    case 11
    case 12
    case 13
    set season summer
    %load% obj 1972
  break
  case 14
    case 15
    case 16
    case 17
    set season fall 
    %load% obj 1973
  break
done
%zoneecho% %self.vnum% The terrain changes as the %season% begins.
~
#1131
testing send~
2 c 100
test~
* No Script
~
#1132
testing purge~
2 c 100
test~
%purge% xxtree
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
eval damage 10
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
eval display 11
while %display% < 56
  eval x%display% .
  eval display %display% + 1
done
eval here %actor.room.vnum%
eval north -10
eval east 1
eval west -1
eval south 10
*
* STARTS READING NORTH
*
if %here.north%
  eval 23 %here.north(vnum)%
  eval temp %23.sector%
  eval x23 %temp.charat(1)%
  eval directionlist north east south west
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
  eval directionlist north east south west
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
  eval directionlist north east south west
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
  eval directionlist north east south west
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
$~
