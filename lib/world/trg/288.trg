#28800
Cleric Fight Function~
0 k 35
~
* Written by Fizban to imitate ROM Cleric Function
set current_hp %actor.hitp%
set rand %random.7%
* The dmg 5000 is actually non-damage, not damage.
switch %rand%
  case 1
    case 2
    case 3
    case 4
    set dmg 5000
  break
  case 5
    eval dmg (%random.2% * %random.8%) + (%self.level% / 2)
    set spellname cause serious
  break
  case 6
    eval dmg (%random.3% * %random.8%) + (%self.level% - 6)
    set spellname cause critical
  break
  case 7
    eval dmg %random.6% + %self.level%
    set spellname flamestrike
  break
done
eval new_current_hp %current_hp% - %dmg%
eval dmgpc (%dmg% * 100) / %current_hp%
if %dmgpc% == 0
  set vp misses
elseif %dmgpc% <= 4 
  set vp scratches
elseif %dmgpc% <= 8
  set vp grazes
elseif %dmgpc% <= 12
  set vp hits
elseif %dmgpc% <= 16
  set vp injures
elseif %dmgpc% <= 20
  set vp wounds
elseif %dmgpc% <= 24
  set vp mauls
elseif %dmgpc% <= 28
  set vp decimates
elseif %dmgpc% <= 32
  set vp devastates
elseif %dmgpc% <= 36
  set vp maims
elseif %dmgpc% <= 40
  set vp MUTILATES
elseif %dmgpc% <= 44
  set vp DISEMBOWELS
elseif %dmgpc% <= 48
  set vp EVISCERATES
elseif %dmgpc% <= 52
  set vp MASSACRES
elseif %dmgpc% <= 100
  set vp DEMOLISHES
else
  set vp ANNIHILATES
end
if %dmg% > 4000
  return 1
else
  %send% %actor% %self.name%'s %spellname% %vp% you!
  %echoaround% %actor% %self.name%'s %spellname% %vp% %actor.name%!
end
switch %rand%
  case 1
    dg_cast 'poison' %actor%
  break
  case 2
    dg_cast 'curse' %actor%
  break
  case 3
    dg_cast 'blind' %actor%
  break
  case 4
    dg_cast 'earthquake'
  break
  case 5
    case 6
    case 7
    %damage% %actor% %dmg%
  break
done
~
#28801
Mage Fight Function~
0 k 35
~
* Written by Fizban to imitate ROM Cleric Function
* adjusted to imitate TBA Mage Functions
set current_hp %actor.hitp%
set rand %random.5%
switch %rand%
  case 1
    eval dmg (%random.1% * %random.8%) + 1
    set spellname chill touch
  break
  case 2
    eval dmg (%random.3% * %random.8%) + 3
    set spellname burning hands
  break
  case 3
    eval dmg (%random.7% * %random.8%) + 7
    set spellname lightning bolt
    case 4
    eval dmg (%random.9% * %random.8%) + 9
    set spellname color spray
    case 5
    eval dmg (%random.11% * %random.8%) + 11
    set spellname fireball
  break
done
eval new_current_hp %current_hp% - %dmg%
eval dmgpc (%dmg% * 100) / %current_hp%
if %dmgpc% == 0
  set vp misses
elseif %dmgpc% <= 4 
  set vp scratches
elseif %dmgpc% <= 8
  set vp grazes
elseif %dmgpc% <= 12
  set vp hits
elseif %dmgpc% <= 16
  set vp injures
elseif %dmgpc% <= 20
  set vp wounds
elseif %dmgpc% <= 24
  set vp mauls
elseif %dmgpc% <= 28
  set vp decimates
elseif %dmgpc% <= 32
  set vp devastates
elseif %dmgpc% <= 36
  set vp maims
elseif %dmgpc% <= 40
  set vp MUTILATES
elseif %dmgpc% <= 44
  set vp DISEMBOWELS
elseif %dmgpc% <= 48
  set vp EVISCERATES
elseif %dmgpc% <= 52
  set vp MASSACRES
elseif %dmgpc% <= 100
  set vp DEMOLISHES
else
  set vp ANNIHILATES
end
if %dmg% > 4000
  return 1
else
  %send% %actor% %self.name%'s %spellname% %vp% you!
  %echoaround% %actor% %self.name%'s %spellname% %vp% %actor.name%!
end
~
#28802
Stock Thief~
0 b 10
~
set actor %random.char%
if %actor%
  if %actor.is_pc% && %actor.gold%
    %send% %actor% You discover that %self.name% has %self.hisher% hands in your wallet.
    %echoaround% %actor% %self.name% tries to steal gold from %actor.name%.
    eval coins %actor.gold% * %random.10% / 100
    nop %actor.gold(-%coins%)%
    nop %self.gold(%coins%)%
  end
end
~
#28803
Fire Breath Function~
0 k 100
~
set current_hp %actor.hitp%
eval low (%self.hitp% / 9)
eval high %self.hitp% / 5
eval range %high% - %low%
eval dmg %%random.%range%%% + %low%
eval dmgpc (%dmg% * 100) / %current_hp%
set spellname fire breath
if %dmgpc% == 0
  set vp misses
elseif %dmgpc% <= 4 
  set vp scratches
elseif %dmgpc% <= 8
  set vp grazes
elseif %dmgpc% <= 12
  set vp hits
elseif %dmgpc% <= 16
  set vp injures
elseif %dmgpc% <= 20
  set vp wounds
elseif %dmgpc% <= 24
  set vp mauls
elseif %dmgpc% <= 28
  set vp decimates
elseif %dmgpc% <= 32
  set vp devastates
elseif %dmgpc% <= 36
  set vp maims
elseif %dmgpc% <= 40
  set vp MUTILATES
elseif %dmgpc% <= 44
  set vp DISEMBOWELS
elseif %dmgpc% <= 48
  set vp EVISCERATES
elseif %dmgpc% <= 52
  set vp MASSACRES
elseif %dmgpc% <= 100
  set vp DEMOLISHES
else
  set vp ANNIHILATES
end
%echoaround% %actor% %self.name% breathes forth a cone of fire.
%send% %actor% %self.name% breathes a cone of hot fire over you!
%send% %actor% %self.name%'s %spellname% %vp% you!
%echoaround% %actor% %self.name%'s %spellname% %vp% %actor.name%!
%damage% %actor% %dmg%
~
#28804
Acid Breath Function~
0 k 100
~
set current_hp %actor.hitp%
eval low (%self.hitp% / 11)
eval high %self.hitp% / 6
eval range %high% - %low%
eval dice_dam %self.level% * 16
eval hp_dam %%random.%range%%% + %low%
eval dmg (%hp_dam% + %dice_dam%) / 10
eval dmgpc (%dmg% * 100) / %current_hp%
set spellname acid breath
if %dmgpc% == 0
  set vp misses
elseif %dmgpc% <= 4 
  set vp scratches
elseif %dmgpc% <= 8
  set vp grazes
elseif %dmgpc% <= 12
  set vp hits
elseif %dmgpc% <= 16
  set vp injures
elseif %dmgpc% <= 20
  set vp wounds
elseif %dmgpc% <= 24
  set vp mauls
elseif %dmgpc% <= 28
  set vp decimates
elseif %dmgpc% <= 32
  set vp devastates
elseif %dmgpc% <= 36
  set vp maims
elseif %dmgpc% <= 40
  set vp MUTILATES
elseif %dmgpc% <= 44
  set vp DISEMBOWELS
elseif %dmgpc% <= 48
  set vp EVISCERATES
elseif %dmgpc% <= 52
  set vp MASSACRES
elseif %dmgpc% <= 100
  set vp DEMOLISHES
else
  set vp ANNIHILATES
end
%echoaround% %actor% %self.name% spits acid at %actor.name%.
%send% %actor% %self.name% spits a stream of corrosive acid at you.
%send% %actor% %self.name%'s %spellname% %vp% you!
%echoaround% %actor% %self.name%'s %spellname% %vp% %actor.name%!
%damage% %actor% %dmg%
~
#28820
Supernova DeathTrap~
2 g 100
~
* Taken from a trigger By Rumble of The Builder Academy
* Near Death Trap stuns actor
wait 1 sec
%send% %actor% There must be a price to pay to witness this stunning sight!
wait 2 sec
set stunned %actor.hitp% - 2
%send% %actor% That really HURTS!  And indeed...
%damage% %actor% %stunned%
%echo% @n
%force% %actor% look
~
#28830
Enter Crystal Ball~
1 c 100
l~
if %cmd.mudcommand% == look && %arg% /= crystal || %arg% /= ball
  %send% %actor% You feel drawn towards the crystal ball.  Your hand reaches out...
  %echoaround% %actor% %actor.name% is drawn into the crystal ball!
  %teleport% %actor% 28802
  wait 2 sec
  %at% 28802 %echoaround% %actor% %actor.name% appears out of nowhere!
  %force% %actor% look
else
  return 0
end
~
$~
