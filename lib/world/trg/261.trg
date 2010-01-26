#26100
Jump into pool - 26121~
2 c 100
ju~
* By Rumble of The Builder Academy    tbamud.com 9091
wait 1 sec
%send% %actor% You jump into the pool of water.
%echoaround% %actor% %actor.name% jumps into the pool of water and disappears.
%teleport% %actor% 26122
wait 1 sec
nop %actor.pos(resting)% 
%echoaround% %actor% %actor.name% falls from above screaming %actor.hisher% lungs out. %actor.heshe% hits lands in the center of the bed with a soft thump!
%send% %actor% You fall into the middle of a soft bed.
%force% %actor% look
~
#26101
Painting comes to Life~
1 c 100
l~
if %cmd.mudcommand% == look && picture /= %arg% || painting /= %arg% || portrait /= %arg%
  %send% %actor%     One particulary gruesome piece done in a backdrop of flames catches your eye.
  %send% %actor% Must be the face floating in the center of it, mouth open wide in a scream of
  %send% %actor% pain, huge yellow teeth dripping blood.
  wait 2 sec
  %send% %actor% The face seems to grow larger and larger in front of your eyes.
  wait 2 sec
  %send% %actor% Is it coming closer?  It keeps growing...
  wait 2 sec
  %send% %actor% It leaps out of the picture and attacks you!
  %echoaround% %actor% As %actor.name% looks at the picture, a face leaps out and attacks %actor.himher%!
  %load% obj 26124
  %load% mob 26110
  %force% floatingfacepainting kill %actor%
  %purge% %self%
else
  return 0
end
~
#26102
Restore the Painting~
1 n 100
~
* Check that the trigger only works in room 26114.
if %self.room.vnum% == 26114
  * Check to make sure they exist, before purging.
  * Make sure they have individual names so you don't
  *  purge a different object with the same name.
  if %findmob.26114(26110)%
    %purge% floatingfacepainting
    %echo% The face retreats back into the painting.
    if %findobj.26114(26124)%
      %purge% purgepic
    end
  end
else
end
~
$~
