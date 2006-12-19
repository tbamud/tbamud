#17500
All-Exit Guardian~
0 q 100
none~
if %actor.is_pc% && %actor.level% < 30
  say Pass here %actor.name% not. Mistress see %actor.name% not! Begone!
  return 0
else
  say Mistress see %actor.name%. Pass here %actor.name% may.
  return 1
end
~
#17501
Guardian Elemental~
0 q 100
none~
if %direction% == north
  if %actor.level%<30
    say You are not permitted to leave in that direction, %actor.name%. Trilless does not wish to see you.
    return 0
  else
    say You are permitted to enter, %actor.name%. Trilless will see you.
    return 1
  end
end
~
#17502
Riddle #1~
2 d 0
e~
%door% 17519 down flags a
%door% 17517 up flags a
%echo% *CLICK*
%echo% The porthole slides open!
~
#17503
Riddle #1 Restore~
2 g 100
~
%door% 17517 u flags bcd
%door% 17519 d flags bcd
wait 1 sec
%echo% The porthole has closed!
set %porthole_opened% 0
~
#17504
Spectre Greeting~
0 g 100
~
%echo% Shadimar says, 'Greetings, %actor.name%. I have a message for you.
%echo%     
%echo% Carcophan protects himself with riddles. You must answer them to get from room to room.
%echo%     
%echo% If you have trouble with the riddle, return to this room and say 'help'. I will leave a clue here for you.
%echo%     
%echo% Farewell, %actor.name%. Until we meet again.'
%echo%   
%echo% %self.name% vanishes in a puff of smoke!
%echo%     
%purge% spectre
~
#17505
Clue #1~
2 d 0
help~
if %actor.vnum% != 17509
%echo%     You can hear Shadimar's voice echoing in the room:
%echo%     I know not the answer to Carcophan's riddles, but this advice I give you:
%echo%     Carcophan's riddles be minced and ground,
%echo%     But look to the letters, and your answer be found.'
%echo%     I hope this helps you on your way.'
end
~
#17506
No-exit room~
2 q 100
~
%send% %actor% You feel no need to travel %direction%. The desert to the %direction% looks no different to where you are now.
return 0
~
#17507
Riddle #2~
2 d 1
night~
%send% %actor% A red light glows around you for a moment, and you feel the world shift under your feet.
%echoaround% %actor% A red light glows around %actor.name%, and after a moment %actor.himher% is gone.
%teleport% %actor% 17520
~
#17508
Riddle #3~
2 d 1
vowel vowels~
%door% 17521 down flags a
%door% 17520 up flags a
%echo% *CLICK*
%echo% The grate swings open!
~
#17509
Riddle #3 Restore~
2 g 100
~
%echoaround% %actor% %actor.name% enters the chimney, and the grate closes behind %actor.himher%.
%door% 17520 up flags bcd
%door% 17521 down flags bcd
wait 1 sec
%send% %actor% The grate has closed behind you!
~
#17510
Riddle #4~
2 d 1
wind~
%send% %actor% The wind dies down for a moment, and suddenly darkness pours out from the grate above and engulfs you completely.
%echoaround% %actor% The wind dies down for a moment, and suddenly darkness pours out from the grate above and engulfs %actor.name% completely. Just as fast the darkness recedes, and %actor.heshe% is gone.
%teleport% %actor% 17523
~
#17511
Riddle #5~
0 g 100
~
%echo% %self.name% cackles with glee.
wait 1 sec
%echo% %self.name% 'Answer me this riddle, fool, and an audience I shall grant you.'
%echo% 'Cannot be seen,
%echo% Cannot be felt,
%echo% Cannot be heard,
%echo% Cannot be smelt.'
wait 1 sec
%echo% %self.name% laughs with dark delight, before absorbing itself into nonexistence.
%purge% Spectre
~
#17512
Riddle #5 Solution~
2 d 1
dark darkness~
%door% 17524 east flags a
%door% 17522 west flags a
%echo% *CLICK*
%echo% A section of darkness to the west falls away!
~
#17513
Carcophan's Flight~
0 g 100
~
if %variable% != 1
say Congratulations on finding me, but you'll take me not, mortal.
east
south
set %variable% 1
end
~
#17514
Salamander's block~
0 q 100
~
if %actor.vnum% == 17511
return 1
%echo% The Salamander bows respectfully before Carcophan, and allows him to pass unhindered.
elseif %actor.level%<40
%send% %actor% The Salamander growls at you menacingly, and blocks your path.
%echoaround% %actor% The Salamander growls at %actor.name% menacingly, and blocks %actor.himher%!
return 0
else
%send% %actor% The Salamander sneers at you, but allows you to pass on your way.
return 1
%echoaround% %actor% The Salamander sneers at %actor.name%, but allows %actor.himher% to pass.
end
~
#17515
Carcophan's Death~
0 f 100
~
%door% 17522 east flags a
%door% 17513 west flags a
%echo% You feel Shadimar's presence in the room.
%echo% 'Well done, %actor.name%,' Shadimar's voice echoes. 'Your way is now open.'
~
$~
