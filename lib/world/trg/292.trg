#29200
Cleric Guildguard - 29226~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == west
  * Stop them if they are not the appropriate class.
  if %actor.class% != cleric
    return 0
    %send% %actor% The guard humiliates you, and blocks your way.
    %echoaround% %actor% The guard humiliates %actor.name%, and blocks %actor.hisher% way.
  end
end
~
#29201
Thief Guildguard - 29203~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == west
  * Stop them if they are not the appropriate class.
  if %actor.class% != thief
    return 0
    %send% %actor% The guard humiliates you, and blocks your way.
    %echoaround% %actor% The guard humiliates %actor.name%, and blocks %actor.hisher% way.
  end
end
~
#29202
Warrior Guildguard - 29228~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == east
  * Stop them if they are not the appropriate class.
  if %actor.class% != warrior
    return 0
    %send% %actor% The guard humiliates you, and blocks your way.
    %echoaround% %actor% The guard humiliates %actor.name%, and blocks %actor.hisher% way.
  end
end
~
#29204
Near Death Trigger - 29267~
2 g 100
~
* Near Death Trap stuns actor
wait 3 sec
set stunned %actor.hitp% 
%damage% %actor% %stunned%
~
$~
