#23900
Dwarven Snow Runes - 23918~
2 d 0
Friend of NewHaven~
%send% %actor% You enter the runed door.
%echoaround% %actor% The Runed Door teleports %actor.name%.
%teleport% %actor% 23919
%force% %actor% look
%echoaround% %actor% %actor.name% is transported from the Runed Door.
~
#23901
Dwarven Entrance Runes - 23919~
2 d 0
Farewell King~
%send% %actor% You enter the runed door.
%echoaround% %actor% The Runed Door teleports %actor.name%.
%teleport% %actor% 23918
%force% %actor% look
%echoaround% %actor% %actor.name% is transported from the Runed Door.
~
#23902
Follow a Hidden Path - 23922~
2 c 100
fol~
if %cmd.mudcommand% == follow && path /= %arg%
  %send% %actor% You follow a hidden path.
  %echoaround% %actor%  %actor.name% follows the hidden path.
  %teleport% %actor% 23923
  %force% %actor% look
  %echoaround% %actor% %actor.name% comes from the hidden path.
end
~
#23903
Portal to Follow a Hidden Path - 23923~
2 c 100
fol~
if %cmd.mudcommand% == follow && path /= %arg%
  %send% %actor% You follow a hidden path.
  %echoaround% %actor%  %actor.name% follows the hidden path.
  %teleport% %actor% 23922
  %force% %actor% look
  %echoaround% %actor% %actor.name% comes from the hidden path.
end
~
#23904
Climb a Pondersoa Pine - 23925~
2 c 100
cli~
if climb /= %cmd% && tree /= %arg%
  %send% %actor% You climb up the ponderosa pine.
  %echoaround% %actor%  %actor.name% climbs up the trunk of the tree.
  %teleport% %actor% 23927
  %force% %actor% look
  %echoaround% %actor% %actor.name% climbs up the tree trunk.
end
~
#23905
Battle Story Greeting - 23917~
0 g 100
~
if %actor.is_pc%
  say Greetings Adventurer, I have a tale to tell, if you care to listen
end
~
#23906
Listen to a Battle Tale - 23917~
0 c 100
listen~
%force% %actor% look newhaven
~
#23907
Fighting Guard Disuasion - 23923~
0 l 50
0~
context %self.id%
if %already_fighting%
  wait 10 sec
  unset already_fighting
else
  say I fight for my honor and my Priestess.
  wait 10 sec
  say You fight in vain, I fight for Lolth.
  wait 10 sec
  say The drow god Lolth will assist me in battle.
  set already_fighting 1
  global already_fighting
end
~
#23908
Death of Drow Priestess - 23924~
0 f 100
~
%echo% %self.name% screams, 'Lolth will have you yet %actor.name%.'
~
#23909
Hermit's Area Hints - 23929~
0 g 100
~
%force% %actor% look hints
~
$~
