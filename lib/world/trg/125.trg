#12500
Moki Feather Teleport~
1 c 1
use~
eval objectname %arg.car%
if %objectname% != feather
  return 0
  halt
end
eval targetname %arg.cdr%
if !(%targetname%)
  return 0
  halt
end
switch %self.vnum%
  case 12502
    set new_vnum 12520
    set fire 1
  break
  case 12520
    set new_vnum 12521
    set fire 1
  break
  case 12521
    set new_vnum 12522
    set fire 1
  break
  case 12522
    set new_vnum 12522
    set fire 0
  break
done
otransform %new_vnum%
if %fire%
  %targetname%'portal' %targetname%
  %echo% A portal springs to life in front of you and immediately closes.
else
  %send% %actor% The feather seems powerless.
end
~
#12501
Thief - 12504~
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
$~
