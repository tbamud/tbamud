#34500
get key accept mob 34500~
0 d 100
yes~
say Good, please return it to me.
~
#34501
get key start mob 34500~
0 h 100
~
wait 1 s
if %direction% == West
  say Can you help me find my key, %actor.alias%
  say I bet its somewhere in the cemetery.  
  say Oh what a confusing place that is.
else
  emote looks for his key nearby.
end
~
#34502
recieve key~
0 j 100
~
return 0
if %object.vnum(34500)%
  wait 2
  say Thank you.
  say You know, I don't go in there much any more.
  say perhaps you may have a better use for it.
else
  wait 2
  say Thats not my key!
end
~
#34504
start quest 2~
0 g 100
~
wait 1 s
if %actor.varexists(accepted_quest_two)%
  say Have you returned with the head, %actor.alias%? 
Else
  say Can you help me?, %actor.alias%
  emote rings his hands nerviously, looking around. 
end
~
#34505
Quest 2 accept (mob 34502)~
0 d 100
yes help~
say Oh good.
emote looks around nervously
wait 15
say A colleague and I where..ah...operating
say on a patient down in the basement when 
say he broke his restraints.
wait 30
say I was able to escape up to here, but 
say the other doctor didn't make it.  
say I looked back just as the patient ripped
say his head off.
wait 30
say I need you to go retrieve the head for me
say I have some information that you might find useful
say if you could do that for me.
set accepted_quest_two 1
remote accepted_quest_two %actor.id%
~
#34506
Quest 2 Char. Return (mob 34502)~
0 g 100
~
if %actor.varexists(accepted_quest_two)%
  say Have you returned with the head?
else
  emote looks around nervously
end
~
#34507
Quest 2 give obj. (mob 34502)~
0 j 100
~
if %object.vnum( 34502 )%
  wait 1 s
  say Thank you.
  say Now for your information
  wait 3 s
  say In the room next to this one there
  say is a secret door that leads to a room
  say with an artifact of some power.
  wait 5 s
  say I don't know how to find the door or use it
  say All I know is that it's there.
  say Perhaps you should go over and look around some
  %purge% %object%
  rdelete accepted_quest_two %actor.id%
Else
  return 0
  wait 3
  say Thats not a head!
  
End
~
#34508
atmosphere for rooms (Fog)~
2 b 10
~
%echo% An unnatural fog swirls at your feet.
~
#34509
Secret room enter (room 34537)~
2 d 0
Edgar Allen Poe~
wait 2 s
%echo% The ground starts to shake slightly
%echo% as if some tremendous pressure was building up
wait 4 s
%echo% Suddenly a part of the east wall begins to move
%echo% Reveling a hidden room!
wait 4 s
%send% %actor% Do to some unknown force you begin to enter.
wait 2 s
%teleport% %actor% 34552
%force% %actor% look
~
#34510
atmosphere for rooms (Something Brushes up against you)~
2 b 10
~
%echo% Something brushes up against you.
~
#34511
fear~
2 b 10
~
%echo% Something brushes across your mind.
~
$~
