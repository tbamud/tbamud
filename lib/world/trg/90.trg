#9000
Bow and Arrow Shoot - 9000~
1 c 1
shoot~
if %actor.fighting% && !%arg%
  set arg %actor.fighting%
end
if !%arg% 
  %send% %actor% Shoot Who?
  halt
else
  %force% %arg% kill %actor.name%
end
if (%arg.room% != %actor.room%) || (%arg.id% == %actor.id%)
  %send% %actor% Shoot: Invalid Target!
  halt
end
eval i %actor.inventory%
while (%i%)
  set next %i.next_in_list%
  if %i.vnum%==9005
    set quiver 1
    break
  end
  set i %next%
done
if %quiver%
  %force% %actor% take arrow quiver
end  
if %actor.inventory(9004)%
  %echo% Arrow in inventory shoot.
  %damage% %arg% 20
  %send% %actor% You shoot an arrow at your opponent.
  %echoaround% %actor% %actor.name%'s opponent shudders, hit by an arrow.
  %purge% %actor.inventory(9004)%
  %load% obj 9004
else
  %send% %actor% You need to have arrows or a quiver.
end
~
$~
