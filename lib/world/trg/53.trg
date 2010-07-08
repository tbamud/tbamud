#5300
Near Death Trap Fall - 5307~
2 g 100
~
* Near Death Trap stuns actor
wait 2 sec
set stunned %actor.hitp% 
%damage% %actor% %stunned%
wait 2 sec
%send% %actor% You somehow survive the fall.
~
#5301
Snake Bite - 00, 01~
0 k 5
~
%send% %actor% %self.name% bites you!
%echoaround% %actor% %self.name% bites %actor.name%.
dg_cast 'poison' %actor%
~
#5302
Wizard - 5311~
0 k 10
~
switch %actor.level%
  case 1
  case 2
  case 3
  break
  case 4
    dg_cast 'magic missile' %actor%
  break
  case 5
    dg_cast 'chill touch' %actor%
  break
  case 6
    dg_cast 'burning hands' %actor%
  break
  case 7
  case 8
    dg_cast 'shocking grasp' %actor%
  break
  case 9
  case 10
  case 11
    dg_cast 'lightning bolt' %actor%
  break
  case 12
    dg_cast 'color spray' %actor%
  break
  case 13
    dg_cast 'energy drain' %actor%
  break
  case 14
    dg_cast 'curse' %actor%
  break
  case 15
    dg_cast 'poison' %actor%
  break
  case 16
    if %actor.align% > 0
      dg_cast 'dispel good' %actor%
    else
      dg_cast 'dispel evil' %actor%
    end
 break
  case 17
  case 18
    dg_cast 'call lightning' %actor%
  break
  case 19
  case 20
  case 21
  case 22
    dg_cast 'harm' %actor%
  break
  default
    dg_cast 'fireball' %actor%
  break
done
~
#5303
Thief - 5304, 5305~
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
#5304
Efreeti Fire Breathing - 5309~
0 k 5
~
%send% %actor% You are engulfed in fire as %self.name% breathes on you.
%echoaround% %actor% %self.name% engulfs %actor.name% with a hot breath of fire.
eval fire_damage %actor.level%
%damage% %actor% %fire_damage%
~
#5305
Healing Sphinx - 5313-16~
0 b 10
~
* This is required because a random trig does not have an actor.
set actor %random.char%
* only continue if an actor is defined.
if %actor%
  * if they have lost more than half their hitpoints heal em
  if %actor.hitp% < %actor.maxhitp% / 2 
    wait 1 sec
    say You are injured, let me help.
    wait 2 sec
    %echoaround% %actor% %self.name% lays %self.hisher% hands on %actor.name%'s wounds and bows %actor.hisher% head in concentration.
    %send% %actor% %self.name% lays %self.hisher% hands on your wounds and bows %actor.hisher% head in concentration.
    dg_cast 'heal' %actor%
  end
end
~
$~
