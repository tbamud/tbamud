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
Flame Damage - R2228~
2 b 50
~
%echo% The pungent fumes burn your lungs!
set target_char %self.people%
while %target_char%
  set tmp_target %target_char.next_in_room%
  %damage% %target_char% 3
  set target_char %tmp_target%
done
~
#2202
Fire Damage - R2218-20~
2 g 100
~
%send% %actor% The hot flame and smoke burns you.
%echoaround% %actor% %actor.name% is burnt from the intense heat.
%damage% %actor% 10
~
#2203
Moaining - R2200-2~
2 g 25
~
if %actor.is_pc%
  %echo% The moaning and wailing of the undead unnerves you to the bone.
end
~
#2204
Princess Shout for Help - M2229~
0 b 1
~
eval inroom %self.room%
   %zoneecho% %inroom.vnum% %self.name% shouts, 'HELP!! Someone please rescue me!!'
~
$~
