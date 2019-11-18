#4600
Exit Anthill - 4600, 4627~
2 c 100
out~
wait 2
%send% %actor% You make your way out off the anthill
%echoaround% %actor% %actor.name% leaves out.
%teleport% %actor% 3229
%force% %actor% look
%echoaround% %actor% %actor.name% just crawled out of the anthill.
~
#4601
Enter Anthill - 4601~
1 c 100
en~
if %cmd.mudcommand% == enter && anthill /= %arg%
   %send% %actor% You enter the anthill.
   %echoaround% %actor% %actor.name% enters the anthill.
   %teleport% %actor% 4600
   %force% %actor% look
   %echoaround% %actor% %actor.name% just came in.
   end
else
   %send% %actor% enter what?!
end
~
#4602
Queen Cry for Help - 4633~
0 al 100
~
if %attacking_queen%
  return 0
else
  %echo% The queen cries out loudly, 'GUARDS! HELP ME! I'M UNDER ATTACK!'
  %at% 4632 %echo% The Queen is calling for help from the east.
  return 1
  set attacking_queen %actor%
  global attacking_queen
end
~
#4603
Make Guards go East - 4632~
0 ae 100
The Queen is calling for help from the east.~
wait 1
east
~
#4604
Queen say Help - 4633~
0 h 100
~
if (%actor.vnum% == 4632)
  wait 5
  say HELP! HELP! I'm being attacked.... HEEEELP!
end
~
#4605
Make Guards Attack - 4632~
0 d 0
HELP! HELP! I'm being attacked~
if (%actor.vnum% == 4633)
  emote joins the battle!
  assist queen
end
~
#4606
Reset attacking_queen - 4633~
0 af 100
~
unset %attacking_queen%
~
#4607
Guard Rescue Queen - 4632~
0 k 50
~
wait 1
rescue queen
~
$~
