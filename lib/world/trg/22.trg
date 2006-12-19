#2200
Not Used~
0 h 100
~
if (%direction% == east)
wait 1
emote snaps to attention as you approach.
wait 1
say admittance to Eldorado is 1000000 coins
end
~
#2201
Not Used~
0 m 0
~
wait 1
unlock gate
open gate
wait 20 s
close gate
lock gate
~
#2202
fire damage~
2 g 100
~
%send% %actor% The hot flame and smoke burns you.
%echoaround% %actor% %actor.name% is burnt from the intense heat.
%damage% %actor% 10
~
#2203
moaning~
2 g 25
~
if %actor.is_pc%
  %echo% The moaning and wailing of the undead unnerves you to the bone.
end
~
#2204
princess shout~
0 b 1
~
eval inroom %self.room%
   %zoneecho% %inroom.vnum% %self.name% shouts, 'HELP!! Someone please rescue me!!'
~
$~
