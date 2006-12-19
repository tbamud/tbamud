#500
Bunny - 500~
0 g 100
~
if %actor.is_pc%
  say Welcome to the Newbie Farm!
  wait 1 s
  say The SAY command is used to speak. Say hello to the other animals in the Newbie Farm to learn how to play.
end
~
#501
NOT USED~
2 b 5
~
%echo% An apple falls...
wait 5
%force% reallyweirdalias say look out!
if !%self.catched%
   %echo% ...on the ground.
   %load% obj 501
end
set catched 0
remote catched %self.id%
~
#502
Mouse teach scan - 502~
0 d 100
hello hi~
say A mouse always SCANS before moving around, to avoid the creatures that can kill us.
~
#503
Catching Apple - 501~
0 b 1
~
%echo% An apple falls...
if %self.eq(wield)%
  wait 1 ses
  %echo% With a fast sword movement, %self.name% pierces the apple in mid-air.
  %load% obj 501
  wait 2
  %echo% %self.name% takes the apple from the sword.
end
~
#504
Rooster teach kill - 504~
0 d 100
hello hi~
%send% %actor% consider rooster
%force% %actor% con rooster
wait 2 s
say You think you can KILL me?
~
#505
Apple purge - 501~
1 f 100
~
%echo% The farmer grabs the apple and adds it to his collection.
%purge% %self%
~
#520
Farmer Welcome - 520~
0 g 100
~
if %actor.is_pc%
  say Welcome friend! Make yourself at home.
end
~
#526
Helen Welcome - 526~
0 g 100
~
if %actor.is_pc%
  wait 1 s
  emote smiles warmly at you.
  wait 2 s
  say Good day child!
end
~
#555
Bat Warning - 555~
0 g 100
~
if %actor.is_pc%
  wait 1 s
  emote flaps his wings nervously.
  say Your death awaits you down there.
  wait 2 s
  say Turn back while you still can.
end
~
#574
Climb Up - 574~
2 c 100
climb~
if %arg% == up
  %send% %actor% You climb up.
  %echoaround% %actor% %actor.name% starts climbing up out of the hole.
  %teleport% %actor% 573
  %force% %actor% look
  %echoaround% %actor% %actor.name% climbs out of the hole.
else
  %send% %actor% Climb where?!
end
~
#584
Arthur Welcome - 584~
0 d 100
hi hello~
say Good day neighbor!
wait 2 s
emote gestures for you to sit down.
wait 2 s
emote smiles happily.
~
$~
