#1600
Pentagram Entry Half Damage~
2 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Damage the actor by 1/2 of their hitpoints on entry.
wait 1 s
eval num_hitp %actor.hitp%/2
%damage% %actor% %num_hitp% 
%send% %actor% The searing heat is causing your skin to blister and burn.
%echoaround% %actor% %actor.name% screams in pain as his skin begins to blister.
%asound% You hear someone screaming in pain close by
~
#1601
new trigger~
2 q 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Damage the actor by 1/2 of their hitpoints on exit.
eval num_hitp %actor.hitp%/2
%damage% %actor% %num_hitp% 
%send% %actor% As you pass through the pentagram the pain intensifies almost to the point of unconsciounsness. Then you are through.
%echoaround% %actor% %actor.name% screams in agony as he steps out of the pentagram.
%asound% You hear someone screaming in pain close by.
~
$~
