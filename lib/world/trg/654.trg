#65400
Keychain takes you to Housing~
1 j 1
~
* Thanks to bakarus for suggesting the return, to Vatiken
*  for helping him with it and to Rumble for adding it to
*  trigger 176 where I could copy it.
*  http://www.tbamud.com/forum/3-building/355-dg-script-question#371
wait 1 sec
* Adjust zone to proper zone number
set zone 654
set roomvnummin %zone%00
set roomvnummax %zone%99
* if person hasn't used the key before, send to Midgaard Temple
*   instead of returning.
set defaultroom 3001
* if person uses key in apartment zone, return player to last room
*   out of the zone where the key was used.
if %actor.room.vnum% >= %zone%00 && %actor.room.vnum% <= %zone%99
  if %actor.varexists(keychain_return_room)%
    %send% %actor% You return to your previous location.
    %echoaround% %actor% %actor.name% heads back out into the world.
    %teleport% %actor% %actor.keychain_return_room%
    %force% %actor% look
    %echoaround% %actor% %actor.name% appears in the room.
  else
    %send% %actor% You head back out into the world.
    %echoaround% %actor% %actor.name% heads back out into the world.
    %teleport% %actor% %defaultroom%
    %force% %actor% look
    %echoaround% %actor% %actor.name% appears in the room.
  end
else
  eval keychain_return_room %actor.room.vnum%
  remote  keychain_return_room %actor.id%
  %send% %actor% You head for home.
  %echoaround% %actor% %actor.name% heads for home.
  %teleport% %actor% %self.vnum%
  %force% %actor% look
  %echoaround% %actor% %actor.name% appears, heading for home.
end
%force% %actor% remove keychain
~
$~
