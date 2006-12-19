#4400
Secret behind shubbery~
2 c 100
hack~
if thicket /= %arg%
   wsend %actor% You hack through the thicket, clearing a path!
   wechoaround %actor% %actor.name% hacks down the thicket, openening the path to the east.
   wdoor 4410 east room 4415
   wdoor 4410 east name thicket
   wait 5 s
   %echo% The Thicket grows out again, magically.
   wdoor 4410 east purge 
else
   wsend %actor% Huh ?!?
end
~
#4401
orc_attack_trader~
0 g 100
~
if %actor.vnum% == 4105
  wait 1
  say YYEEHAA! I got you now, dwarf! Give me your gold and I might spare you.
  wait 1
  emote charge forward, attacking the dwarven trader.
  mkill trader
elseif %actor.is_pc%
  growl %actor.name%
end
~
#4402
orc_death_cry~
0 f 100
~
if %actor.vnum% == 4105 || %actor.vnum% == 4106
  say AAARRRGGH!!
  %echo% %self.name% screams loudly and collapses on the ground. Dead.  
end
~
$~
