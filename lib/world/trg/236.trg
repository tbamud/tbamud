#23600
Cleric Fight Spec - 23621~
0 k 35
~
* select the spell
eval spellnr %random.4%
if %spellnr%==1
  dg_cast 'harm' %actor%
elseif %spellnr%==2
  dg_cast 'curse' %actor%
elseif %spellnr%==3
  dg_cast 'call lightning' %actor%
else
  dg_cast 'blindness' %actor%
end
~
#23601
Healing Fountain - NOT ATTACHED~
0 e 100
drink~
if %actor.hitp% * 3 < %actor.maxhitp%
  %damage% %actor% -4
end
~
#23602
Room Command Secret Door - 23667~
2 c 100
pull~
if tapestries /= %arg%
  %send% %actor% You pull the tapestries down, and reveal a secret vault!
  %echoaround% %actor% As %actor.name% pulls the tapestries down a secret vault is revealed.
  %door% 23667 east flags abcd
  %door% 23667 east key 23634
  %door% 23667 east name vault
  %door% 23667 east room 23668 
else
  %send% %actor% Pull what ?!
end
~
#23603
Remove Secret Door - 23667~
2 f 100
~
if %self.east%
  %door% 23667 east purge 0
  %echo% As if by magic all the tapestries rise to their previous positions.
end
~
#23604
Equip Hammer Dwarf - 23604~
0 an 50
~
%load% obj 23637
wield hammer
~
#23605
Foreman Walkabout - 23628~
0 ab 40
~
eval inroom %self.room%
if %inroom.vnum% == 23653
  set foreman_leaving 1
  global foreman_leaving
end
if %inroom.vnum% == 23664
  set foreman_leaving 0
  global foreman_leaving
end
if %foreman_leaving%
  switch %inroom.vnum%
    * start room 23653
    case 23653
      open door
      wait 1
      east
      wait 1
      close door
    break
    case 23630
      case 23662
      case 23629
      case 23628
      case 23641
      case 23642
      case 23635
      case 23614
      case 23646
      east
    break
    case 23644
      case 23660
      case 23649
      case 23650
      case 23617
      case 23619
      case 23620
      case 23666
      case 23632
      case 23645
      case 23663
      west
    break
    case 23655
      * at the entrance to the court.
      open granite
      wait 4
      west
      wait 4
      close granite
    break
    case 23648
      open marble
      wait 4
      west
      wait 4
      close marble
    break
    case 23643
      case 23637
      case 23631
      case 23623
      case 23618
      case 23667
      case 23634
      case 23626
      north
    break
    case 23661
      case 23627
      case 23633
      case 23640
      case 23647
      case 23654
      case 23621
      case 23669
      case 23652
      south
    break
    default
      say I don't think I've ever been to this side of the city before...
      wait 6
      say I'd better go home.
      wait 6
      emote pulls a small scroll from a hidden pocket and quickly recites it.
      emote disappears in a puff of smoke.
      mgoto 23653
    break
  done
else
  switch %inroom.vnum%
    case 23661
      case 23648
      case 23649
      case 23643
      case 23618
      case 23617
      case 23619
      case 23662
      case 23629
      case 23628
      case 23641
      case 23642
      case 23635
      case 23614
      case 23646
      east
    break
    case 23660
      * leaving the court
      open granite
      wait 4
      east
      wait 4
      close granite
    break
    case 23655
      open marble
      wait 4
      east
      wait 4
      close marble
    break
    case 23631
      case 23666
      case 23632
      case 23645
      west
    break
    case 23654
      * entering tool shed
      open door
      wait 1
      west
      wait 1
      close door
    break
    case 23663
      case 23620
      case 23627
      case 23633
      case 23640
      case 23647
      case 23667
      case 23634
      case 23626
      north
    break
    case 23650
      case 23644
      case 23637
      case 23630
      case 23623
      case 23621
      case 23669
      case 23652
      south
    break
    case 23664
      say I'm pleased to say that productivity is at the maximum...
      wait 3 s
      say I'll be on my way..
      wait 3 s
      east
    break
    default
      say I don't think I've ever been to this side of the city before...
      wait 6
      say I'd better go home.
      wait 6
      emote pulls a small scroll from a hidden pocket and quickly recites it.
      emote disappears in a puff of smoke.
      mgoto 23653
    break
  done
end
~
#23606
Mine Crane - 23674~
2 c 100
enter~
* funtions
set passup %echoaround% %actor% %actor.name% passes you in a basket, going up.
set passdown %echoaround% %actor% %actor.name% passes you in a basket, going down. 
set look wforce %actor% look
 
* the crane itself
if %arg% == basket
 if %self.vnum% == 23674
   %send% %actor% As you enter the basket, the crane starts moving. 
   %echoaround% %actor% As %actor.name% enters the basket it starts moving upwards.
   wait 2 s
   %teleport% %actor% 23673
   %passup%
   %look%
   %send% %actor% You lean back and relax the ride, while you are lifted out of the mine. 
   wait 2 s
   %teleport% %actor% 23672
   %passup%
   %look%
   wait 2 s
   %teleport% %actor% 23671
   %passup%
   %look%
   wait 2 s
   %teleport% %actor% 23670
   %passup%
   %look%
   wait 2 s
   %teleport% %actor% 23622
   %look%
   %echoaround% %actor% %actor.name% arrives from the hole in a large basket.
   wait 2 s
   %send% %actor% Having arrived at the top level, you quickly jump out of the basket.
   %echoaround% %actor% %actor.name% quickly jumps out of the basket.
 else if %self.vnum% == 23622
   %send% %actor% As you sit in the basket, it lowers slowly into the mine. 
   %echoaround% %actor% As %actor.name% enters the basket, it lowers slowly into the mine.
   wait 2 s
   %teleport% %actor% 23670
   %passdown%
   %look%
   %send% %actor% You lean back and relax the ride, while you are slowly descending the mine. 
   wait 2 s
   %teleport% %actor% 23671
   %passdown%
   %look%
   wait 2 s
   %teleport% %actor% 23672
   %passdown%
   %look%
   wait 2 s
   %teleport% %actor% 23673
   %passdown%
   %look%
   wait 2 s
   %teleport% %actor% 23674
   %echoaround% %actor% %actor.name% arrives from above and quickly leaves the basket.
   %look%
   wait 2 s
   %send% %actor% Having descended into the mines, you quickly jump out of the basket.
  end
else
   %send% %actor% Huh?!
end
~
#23607
Bad Stairs Trap - 23672~
2 g 100
~
* Variable definitions 
if %direction% == up
  set todir down
else
  set todir up
end
 
eval temp %random.6%
    
switch %temp%
  case 1
    set slip foot
    set hurt hit
    set part head
    eval damage %actor.hitp% / 2 
    break     
  case 2
    set slip hand
    set hurt strain
    set part arm
    eval damage %actor.hitp% / 3
    break
  case 3
    set slip hand
    set hurt hit
    set part knee
    eval damage %actor.hitp% / 4
    break
  case 4
    set slip foot
    set hurt hurt
    set part leg
    eval damage %actor.hitp% / 3
    break
  default
    set slip hand
    set hurt hit
    set part other hand
    eval damage %actor.hitp% / 8
    break
done
* the actual trap part :
wait 1    
%send% %actor% As you step %todir%, your %slip% slips and you %hurt% your %part%. OUCH!
%echoaround% %actor% As %actor.name% steps %todir%, %actor.hisher% %slip% slips and %actor.heshe% %hurt%s %actor.hisher% %part%. It looks painful.
         
wdamage %actor% %damage%
~
#23608
Singing Miners - 23627~
0 b 30
~
switch %random.6%
  case 1
    emote sings a merry song about someone named Fulbert and Beatrice.
    break
  case 2
    emote booms out loudly, 'Heigh Ho! Heigh Ho! It's home from work we go!'
    break
  case 3
    emote tries to sing falsetto, but his voice doesn't agree.
    break
  default
    sing
    break
done    
~
#23609
Poisonous Spider - NOT ATTACHED~
0 k 100
~
dg_cast 'poison' %actor.name%
~
#23610
Bell - 23638~
1 g 100
~
if %self.vnum% == 23638
  return 0
  %echoaround% %actor% As %actor.name% touches the pile of titanium, the bell rings.
  %send% %actor% As you touch the pile of titanium, the bell rings.
    
  wait 1
  
  %echo% Some miners heard the bell, and come running as fast as they possibly can.
  wait 1 
  %echo% A miner has arrived.
  %load% mob 23626 
  wait 2
  %echo% A miner has arrived.
  %load% mob 23626
  wait 2
  %echo% A miner has arrived.
  %load% mob 23626
  wait 2 
   
  %echo% The bell falls to the ground. 
  otransform 23639
else
  return 1
end
~
#23611
Mad Miners - NOT ATTACHED~
0 n 100
~
mkill %bad_guy%
~
#23612
Lieutenant Door Bribe - M23608~
0 m 1
~
* By Welcor of The Builder Academy    tbamud.com 9091
wait 1
* The price is 400 coins to pass. Player must 'give 400 coin leader.'
if %amount% < 400
  say Did you really think I was that cheap, %actor.name%.
  snarl 
else
  * Context saves the global with the players ID so multiple players can bribe.
  context %actor.id%
  * Set the variable to a value, 1 for YES.
  set has_bribed_guard 1
  * Global it! You can now 'stat leader' and see it listed.
  global has_bribed_guard
  whisper %actor.name% Enter when you're ready. I'll lock the door behind you.
  unlock door
end
~
#23613
Lieutenant Door Bribe 2 - M23608~
0 r 100
~
* By Welcor of The Builder Academy    tbamud.com 9091
* Allows more than one instance of this trigger to run.
context %actor.id%
* Checks a global variable to see if this mob has been bribed. TSTAT 23612.
if %has_bribed_guard%
  * Let the player through, he's paid.
  return 1
  * Don't bother continuing the trig, just stop it.
  halt
end
* If the player tries to pick the lock catch him!
if %cmd% == pick
  * Stop them! Return 0 prevents the command from going through.
  return 0
  wait 1
  say No way, you don't fool me, %actor.name%.
  * If mob can see the player, kill him!
  if %actor.canbeseen%
    mkill %actor%
  end
end
~
#23614
Lieutenant Leave - 23608~
0 q 100
~
* By Welcor of The Builder Academy    tbamud.com 9091
* If the player is trying to leave to the East.
if %direction% == east
  context %actor.id%
  * If they have bribed let them through and forget them.
  if %has_bribed_guard%
    unset has_bribed_guard
    * Let the command go through. Halt the trig, its over.
    return 1
    halt
  end
  * They haven't paid, stop them.
  %send% %actor% You try to leave, but %self.name% stops you.
  return 0
end
~
#23615
FREE~
2 q 100
~
if %direction% == east
return 0
  %send% %actor% The door vanishes as if it was never there, and you step through.
  %echoaround% %actor% As %actor.name% steps through the doorway, the door disappears for an instant.
  %teleport% %actor% 23611
  %echoaround% %actor% %actor.name% has arrived.
  %force% %actor% look
end
~
#23616
Check for Dwarf the other way - 23611~
2 c 100
w~
if %cmd.mudcommand% == west
  if %actor.race%==dwarf
    %send% %actor% The door vanishes as if it was never there, and you step through.
    %echoaround% %actor% As %actor.name% steps through the doorway, the door disappears for an instant.
    %teleport% %actor% 23610
    %echoaround% %actor% %actor.name% has arrived.
    %force% %actor% look
    return 1
  else
    return 0
  end
end
~
#23617
Near Death Trap Fall - 12684~
2 g 100
~
* Near Death Trap stuns actor
wait 5 sec
set stunned %actor.hitp% 
%damage% %actor% %stunned%
wait 5 sec
%send% %actor% The Gods allow your puny existence to continue.
~
$~
