#10700
Vault Throw - 10737~
2 c 100
en~
if %cmd.mudcommand% == enter && vault /= %arg%
  %send% %actor% As you attempt to enter the vault an unseen spirit throws you from the room.
  %send% %actor% You are damaged by the fall.
  %echoaround% %actor% %actor.name% tries to enter the vault and is instead thrown out of the room, by some unseen force.
  %teleport% %actor% 10719
  %damage% %actor% 20
  %force% %actor% look
  %echoaround% %actor% %actor.name% is thrown into the room.
else
  %send% %actor% Enter what?!
end
~
#10701
Near Death Trap Sink Hole - 10773~
2 g 100
~
* Near Death Trap stuns actor
wait 3 sec
set stunned %actor.hitp% 
%damage% %actor% %stunned%
wait 5 sec
%send% %actor% The Gods pity you enough to allow you to survive.
~
$~
