#55500
new trigger~
0 g 100
~
say My trigger commandlist is not complete!
~
#55513
TalkingHorse~
0 b 10
~
* Trigger Program By Rumble of The Builder Academy    tbamud.com 9091
if !%self.fighting%
  eval max %random.4%
  set txt[1] What's my job?  I eat hay.
  set txt[2] The answer... It was...INFINITY!
  set txt[3] Hey...  You look familiar, have we met?
  set txt[4] Do you want some advice?  Don't talk to horses!
  set  speech %%txt[%max%]%%
  eval speech %speech%
  say %speech%
~
#55514
Leaving MrSmithy~
0 q 100
~
if %actor.is_pc%
  %send% %actor% %self.name% says 'Come back and see me again, %actor.name%. Really!'
  %echo% @n
end
~
#55515
BeggarGold~
0 n 100
test~
wait 2 sec
%at% 55500 put gold in cup
%at% 55500 wear cup
~
#55528
DoorClose~
2 c 100
close~
if %arg% == shelf
  %echo% The shelf creaks closed.
  %door% 55528 north flags b
  %door% 55532 south purge
  %at% 55532 %echo% The shelves gently creak as they swing back towards the wall.
else
  return 0
end
~
#55529
DoorOpen~
2 c 100
open~
if %arg% /= shelf
  %send% %actor% You hear a soft creak as you swing the shelf open.
  %echoaround% %actor% You hear a soft creak as %actor.name% swings the shelf open.
  %at% 55532 %echo% You hear a soft creak as the shelf slowly swings away from the wall.
  %door% 55532 south flags a
  %door% 55532 south name shelf
  %door% 55532 south room 55528
  %door% 55532 south description An eerie blue light glows from behind the shelf.
  %door% 55528 north flags a
else
  return 0
end
~
#55532
Bookshelves~
2 c 100
open~
if %arg% /= shelf
  %send% %actor% You hear a soft creak as you swing the shelf open.
  %echoaround% %actor% You hear a soft creak as %actor.name% swings the shelf open.
  %door% 55532 south flags a
  %door% 55532 south name shelf
  %door% 55532 south room 55528
  %door% 55532 south description An eerie blue light glows from behind the shelf.
  %door% 55528 north flags a
else
  return 0
end
~
#55533
CloseBookshelves~
2 f 100
~
if %self.south%
  %door% 55532 south purge
  %echo% The shelf gently creaks as it swings back towards the wall.
end
~
#55534
BumpShelves~
2 c 100
s~
if %cmd.mudcommand% == south && if !%self.south%
  %send% %actor.name% The shelf seems to be closed.
else
  return 0
end
~
#55535
CloseShelf~
2 c 100
close~
if %arg% /= shelf
  %echo% The shelf creaks closed.
  %door% 55532 south purge
  %door% 55528 north flags b
else
  return 0
end
~
#55556
EndMoonGate~
0 q 100
~
if %direction% == up
  return 0
  %send% %actor% The guardian blocks your way.
  %echoaround% %actor% The guardian and blocks %actor.hisher% way.
end
~
#55561
EquipSentri~
0 n 100
~
%load% obj 55554 %self% head
%load% obj 55555 %self% arms
%load% obj 55556 %self% waist
%load% obj 55558 %self% hands
%load% obj 55559 %self% feet
%load% obj 55560 %self% legs
%load% obj 55561 %self% body
~
#55562
BatlinMantras~
0 n 100
~
%load% obj 55501
%load% obj 55503
%load% obj 55507
%load% obj 55505
%load% obj 55502
%load% obj 55506
%load% obj 55500
%load% obj 55504
~
#55569
DesertMaze69~
2 g 100
~
  wait 1 sec
  %door% 55569 north purge
  %door% 55569 south purge
  %door% 55569 east purge
  %door% 55569 west purge
  wait 1 sec
  switch %random.4%
    case 1
  %door% 55569 north room 55578
  %door% 55569 north description You see trees.
  %door% 55569 east room 55570
  %door% 55569 east description You see trees.
  %door% 55569 south room 55579
  %door% 55569 south description You see trees.
  %door% 55569 west room 55573
  %door% 55569 west description You see trees.
    break
    case 2
  %door% 55569 north room 55570
  %door% 55569 north description You see trees.
  %door% 55569 east room 55579
  %door% 55569 east description You see trees.
  %door% 55569 south room 55573
  %door% 55569 south description You see trees.
  %door% 55569 west room 55578
  %door% 55569 west description You see trees.
    break
    case 3
  %door% 55569 north room 55579
  %door% 55569 north description You see trees.
  %door% 55569 east room 55573
  %door% 55569 east description You see trees.
  %door% 55569 south room 55578
  %door% 55569 south description You see trees.
  %door% 55569 west room 55570
  %door% 55569 west description You see trees.
    break
    case 4
  %door% 55569 north room 55573
  %door% 55569 north description You see trees.
  %door% 55569 east room 55578
  %door% 55569 east description You see trees.
  %door% 55569 south room 55570
  %door% 55569 south description You see trees.
  %door% 55569 west room 55579
  %door% 55569 west description You see trees.
    break
    default
  %door% 55569 north room 55555
  %door% 55569 north description You see trees.
  %door% 55569 east room 55555
  %door% 55569 east description You see trees.
  %door% 55569 south room 55555
  %door% 55569 south description You see trees.
  %door% 55569 west room 55555
  %door% 55569 west description You see trees.
    break
  done
~
#55571
DesertMaze71~
2 g 100
~
  wait 1 sec
  %door% 55571 north purge
  %door% 55571 south purge
  %door% 55571 east purge
  %door% 55571 west purge
  wait 1 sec
  switch %random.4%
    case 1
  %door% 55571 north room 55576
  %door% 55571 north description You see trees.
  %door% 55571 east room 55572
  %door% 55571 east description You see trees.
  %door% 55571 south room 55581
  %door% 55571 south description You see trees.
  %door% 55571 west room 55570
  %door% 55571 west description You see trees.
    break
    case 2
  %door% 55571 north room 55572
  %door% 55571 north description You see trees.
  %door% 55571 east room 55581
  %door% 55571 east description You see trees.
  %door% 55571 south room 55570
  %door% 55571 south description You see trees.
  %door% 55571 west room 55576
  %door% 55571 west description You see trees.
    break
    case 3
  %door% 55571 north room 55581
  %door% 55571 north description You see trees.
  %door% 55571 east room 55570
  %door% 55571 east description You see trees.
  %door% 55571 south room 55576
  %door% 55571 south description You see trees.
  %door% 55571 west room 55572
  %door% 55571 west description You see trees.
    break
    case 4
  %door% 55571 north room 55570
  %door% 55571 north description You see trees.
  %door% 55571 east room 55576
  %door% 55571 east description You see trees.
  %door% 55571 south room 55572
  %door% 55571 south description You see trees.
  %door% 55571 west room 55581
  %door% 55571 west description You see trees.
    break
    default
  %door% 55571 north room 55555
  %door% 55571 north description You see trees.
  %door% 55571 east room 55555
  %door% 55571 east description You see trees.
  %door% 55571 south room 55555
  %door% 55571 south description You see trees.
  %door% 55571 west room 55555
  %door% 55571 west description You see trees.
    break
  done
~
#55577
DesertMaze77~
2 g 100
~
  wait 1 sec
  %door% 55577 north purge
  %door% 55577 south purge
  %door% 55577 east purge
  %door% 55577 west purge
  wait 1 sec
  switch %random.4%
    case 1
  %door% 55577 north room 55580
  %door% 55577 north description You see trees.
  %door% 55577 east room 55576
  %door% 55577 east description You see trees.
  %door% 55577 south room 55570
  %door% 55577 south description You see trees.
  %door% 55577 west room 55578
  %door% 55577 west description You see trees.
    break
    case 2
  %door% 55577 north room 55576
  %door% 55577 north description You see trees.
  %door% 55577 east room 55570
  %door% 55577 east description You see trees.
  %door% 55577 south room 55578
  %door% 55577 south description You see trees.
  %door% 55577 west room 55580
  %door% 55577 west description You see trees.
    break
    case 3
  %door% 55577 north room 55570
  %door% 55577 north description You see trees.
  %door% 55577 east room 55578
  %door% 55577 east description You see trees.
  %door% 55577 south room 55580
  %door% 55577 south description You see trees.
  %door% 55577 west room 55576
  %door% 55577 west description You see trees.
    break
    case 4
  %door% 55577 north room 55578
  %door% 55577 north description You see trees.
  %door% 55577 east room 55580
  %door% 55577 east description You see trees.
  %door% 55577 south room 55576
  %door% 55577 south description You see trees.
  %door% 55577 west room 55570
  %door% 55577 west description You see trees.
    break
    default
  %door% 55577 north room 55555
  %door% 55577 north description You see trees.
  %door% 55577 east room 55555
  %door% 55577 east description You see trees.
  %door% 55577 south room 55555
  %door% 55577 south description You see trees.
  %door% 55577 west room 55555
  %door% 55577 west description You see trees.
    break
  done
~
$~
