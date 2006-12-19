#4501
player entering room -> tell story~
0 g 100
~
if %actor.is_pc%
  wait 1
  say Can you help us, %actor.name%?
  wait 1
  say An evil ogre holds us all hostage and blocks the road to help.
  say If you slay it I'll give you all the coins we can spare.
  wait 1 s
  say Bring me the ears as proof.
end
~
#4502
player kills ogre -> load ears~
0 f 100
~
* load the ears
%load% obj 4512 
~
#4503
player gives ears -> reward player~
0 j 100
~
* check if this was indeed the ears
if %object.vnum%==4512
wait 1
say Thank you, %actor.name%
%send% %actor% %self.name% gives you 30 gold pieces.
%echoaround% %actor% %actor.name% is rewarded for his valor.
nop %actor.gold(30)%
wait 5
junk ears
else
* this wasn't the ears - don't accept it
say I don't want that - bring me the ears!
return 0
end
~
$~
