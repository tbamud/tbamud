#27900
Thief Renoir - 27949~
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
#27994
Mask - 27990~
0 g 100
~
wait 2 sec
say Help me I am the real king.
wait 7 sec
say Is to late for me, but please revenge me.
wait 2 sec
shout arrrggghh!!!
%load% obj 27991
%load% obj 27992
%purge% man
~
$~
