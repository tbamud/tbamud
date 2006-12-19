#9600
Windblow - 1, 4-8, 10-22~
2 ab 23
~
%echo% A light breeze picks up, causing the leaves to rustle quietly.
wait 200
%echo% Nearby, a bird chirps its greetings to the world.
wait 200
%echo% A rabbit hops across the path, pursued by a wily red fox.
~
#9603
Wizard - 9603~
0 g 100
~
if %actor.is_pc%
  emote sighs deeply.
  say I suppose...that you're...here to seek the....treasure also?
  wait 1 s
  %echo% The wizard's face brightens considerably.
  say Or...Are you here to shop? I have a nice variety of potions...
end
~
#9604
Wizard tell - 9603~
0 d 1
treasure~
say Fool! But...it is...your...death!
emote hums a word of arcane power, and your vision blurs momentarily.
%teleport% all 9633
close door
~
#9609
Swamp Eat - 9609~
2 g 30
~
wait 1
%echo% A rabbit emerges from the underbrush, chased by a fox.
wait 1 s
%echo% When the rabbit reaches the swampy mess, he slows to a stop, and starts sinking.
wait 1 s
%echo% The fox waits for a moment at the edge of the pit, then disappears back into the forest.
~
#9623
Walking Squelch - 9623~
2 g 100
~
wait 10
%echo% *squelch, squelch, squelch* Grrrrrrr...
~
#9647
Knight Death - 9647~
0 f 100
~
%load% obj 9666
drop head
~
#9648
Fish Key - 9653~
2 d 1
fish~
%echo% A childish voice booms, "Well done! Here's the key..."
%load% obj 9652
~
#9649
Choose Brain Riddle - 9653~
2 d 1
brain brains~
%echo% A childish voice booms, "Riddle me this, riddle me that...
%echo% Alive without breath
%echo% Cold as death
%echo% Never thirsty, ever drinking
%echo% All in mail, never clinking."
~
#9650
Key Load - 9651~
0 n 100
~
%load% obj 9652
~
#9651
Choose Brawn - 9653~
2 d 1
Brawn~
%echo% A voice booms, "So, you have chosen brawn! Good luck, for my minions aren't easily bested!"
%load% mob 9651
wait 1
%echo% A gremlin appears.
~
#9652
King Challenge - 9653~
2 g 100
~
if %actor.is_pc%
  wait 1 s
  %echo% A voice booms, "Welcome traveller! You have come far to seek my Sword.
  %echo% I admire your perseverance. 
  %echo% However, you must prove yourself one more time. 
  %echo% You have proven your brawn, now show me your wit! 
  %echo% Unless you are all brawn, in that case you may fight again. 
  %echo% So, which shall it be? Brains or brawn?"
end
~
#9654
King Quest - 9654~
0 g 100
~
if %actor.is_pc%
  wait 1 s
  say Welcome, I have waited long for one to claim my sword.
  say But, although you've proven yourself many times before, I must ask you to complete one more test.
  %load% obj 9646
  give key %actor.name%
  wait 2 sec
  say Use this key to get into a holding room downstairs. They're directly across from the stairways.
  say Bring me back proof of your victory there and I shall give you my sword.
  wait 1 s
  say Now go!
end
~
#9666
Head Give - 9654~
0 j 100
~
if %object.vnum% != 9666
  say What is this? I don't want this filth!
else
  wait 1 s
  say Good work. Here's my sword.
  %load% obj 9602
  give sword %actor.name%
  say Goodbye...My work is done.
  emote begins aging rapidly, and is soon nothing but a pile of ash.
  %purge% head
  %purge% king
  %echo% You find yourself at the entrance of Domiae.
  %teleport% all 9619
end
~
$~
