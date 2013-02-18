#23839
Puke up your guts - 23842~
2 c 100
exa~
if %cmd.mudcommand% == examine && corpse /= %arg%
  %send% %actor% As you near the pile of decaying corpses the smell of death and rotting corpses hit you.
  %echoaround% %actor% %actor.name% begins to near the corpses and is stopped by the horrid smell.
  %force% %actor% puke
else
  %send% %actor% You cannot %cmd% that!
end
~
#23840
Pull torch - 23840~
2 c 100
pu~
if pull /= %cmd% && torch /= %arg%
  %send% %actor% The torch turns easily in your hand revealing a trap door.
  %echoaround% %actor% As %actor.name% pulls the torch a trap door swings open leading upwards.
  %door% 23840 up room 23841
  %door% 23840 up name trap door
  wait 60s
  %echo% The trap door slams itself closed as if by magic.
  %door% 23840 up purge
else
  %send% %actor% OK!
  %echoaround% %actor% You start to question the sanity of %actor.name% as they fumble about the room.
end
~
#23841
Monster Greets - 23841~
0 hi 100
~
if %actor.is_pc%
  wait 2s
  emote looks up and beams a smile towards %actor.name%
  wait 6s
  say Bah!! your head is way to small to add to my collection!!
  wait 2s
  emote turns his attention back to the heads that line the wall.
end
~
#23842
Still too small - 23841~
0 f 100
~
say Your head is still too small!!
emote cackles with insane glee.
~
#23843
Portal - 23843~
1 c 7
en~
if %cmd.mudcommand% == enter && box /= %arg%
  %send% %actor% You enter a mysterious box.
  %echoaround% %actor% %actor.name% steps into a mysterious box.
  %teleport% %actor% 23844
  %force% %actor% look
  %echoaround% %actor% %actor.name% steps out of a mysterious box.
else
  %send% %actor% %cmd.mudcommand% what?!
end
~
#23853
Riddle 1 - 23860~
0 g 100
~
if %actor.is_pc%
  wait 1s
  say hello stranger. I know why you are here, you wanna enter the gate don't you.
end
~
#23854
Yes command - 23860~
0 d 1
y~
if yes /= %speech%
  say Well then you gotta answer a few riddles first.  I hope you are smart and not wasting my time.
  wait 3s
  say hhhhmmmmmm which should i use first.
  wait 1s
  say ahhh i got it.
  wait 1s
  say ok here goes, how far can you run into the woods?
end
~
#23855
Fountain portal - 23855~
1 c 7
en~
if %cmd.mudcommand% == enter && fountain /= %arg%
  %send% %actor% You dive into a large fountain.
  %echoaround% %actor% %actor.name% dives into a large fountain.
  %teleport% %actor% 23856
  %force% %actor% look
  %echoaround% %actor% %actor.name% emerges from a large fountain.
else
  %send% %actor% enter what?!
end
~
#23856
Riddle 2 - 23860~
0 d 0
halfway~
  emote gives %actor.name% a round of applause
say well done.
wait 2 sec
say hhhhhmmmmm what should I use next?
wait 1 s
say ahh I got it.
wait 2 s
say ok here goes in magical language what word means fly?
~
#23857
Riddle 3 - 23860~
0 d 100
yrl~
say very good
wait 2 s
emote ponders over which question to use next.
wai 3 s
say ahh i got it
wait 1 s
say ok what has 4 legs up 4 legs down and is soft in the middle?
~
#23858
Riddle 4 - 23860~
0 d 100
bed~
say well done!!
wait 2 s
say I got one more for you then you may pass.
wait 1 s
say ok at night they come without being fetched, and by day they are lost without being stolen what is it?
~
#23859
Riddle 5 - 23860~
0 d 100
stars~
say very good but I changed my mind you got those to quickly you need to answer a few more to pass.
wait 2 s
say ok here goes
wait 1 s
say I never was but will always be, noone ever saw me, noone ever will yet I am the confidence of all that live and breath on this terrestrial ball what am I?
~
#23860
Riddle 6 - 23860~
0 d 100
tomorrow~
say well done!!
wait 1 s
emote applauses %actor.name% for a job well done.
wait 1 s
say you can now enter the gate to the north you posess the knowledge that is required.
wait 1 s
%load% obj 23860
drop all
wait 2 s
say use it well my son.
~
#23861
Green portal entrance - 23863~
1 c 100
en~
if %cmd.mudcommand% == enter && green /= %arg%
  %send% %actor% You enter a large green portal.
  %echoaround% %actor% %actor.name% steps into a large green portal.
  %teleport% %actor% 23864
  %force% %actor% look
  %echoaround% %actor% steps into a large green portal.
else
  %send% %actor% enter what?!
end
~
#23862
Blue portal entrance - 23861~
1 c 100
en~
if %cmd.mudcommand% == enter && blue /= %arg%
  %send% %actor% You enter a large blue portal.
  %echoaround% %actor% %actor.name% steps into a large blue portal.
  %teleport% %actor% 23862
  %force% %actor% look
  %echoaround% %actor% steps through a large blue portal.
else
  %send% %actor% enter what?!
end
~
#23863
Red portal entrance - 23862~
1 c 100
en~
if %cmd.mudcommand% == enter && red /= %arg%
  %send% %actor% You enter a large blue portal.
  %echoaround% %actor% %actor.name% steps into a large blue portal.
  %teleport% %actor% 23863
  %force% %actor% look
  %echoaround% %actor% steps through a large blue portal.
else
  %send% %actor% enter what?!
end
~
$~
