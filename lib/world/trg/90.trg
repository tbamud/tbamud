#9000
Bow and Arrow Shoot - 9000~
1 c 1
shoot~
if %actor.fighting% && !%arg%
  set arg %actor.fighting.alias%
end
if !%arg%
  %send% %actor% Shoot Who?
  halt
end
* Set the rooms ID to a variable.
set room_var %actor.room%
* Target the first char.
set target_char %room_var.people%
* Now loop through everyone in the room
while %target_char%
  * Save the next target before this one perhaps dies.
  set tmp_target %target_char.next_in_room%
  * This is where the good/bad things are supposed to happen.
  if %target_char.alias% /= %arg% && %target_char% != %actor%
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
      eval dex_check %random.6% + 10
      if %actor.dex% > %dex_check%
        %send% %actor% You shoot %target_char.name%, bullseye.
        %echoaround% %actor% %target_char.name% shudders, at the impact of %actor.name%'s arrow.
        %purge% %actor.inventory(9004)%
        %force% %target_char% kill %actor.name%
        %load% obj 9004 %target_char% inv
        %damage% %target_char% 10
        halt
      else
        %send% %actor% You miss %target_char.name%, and %target_char.heshe% notices.
        %echoaround% %actor% %target_char.name% shudders, at the impact of %actor.name%'s arrow.
        %purge% %actor.inventory(9004)%
        %force% %target_char% kill %actor.name%
        %load% obj 9004
        halt
      end
    else
      %send% %actor% You are out of arrows.
      halt
    end
  end
  * Set the next target.
  set target_char %tmp_target%
  * Loop back.
done
%send% %actor% You can not find your target to shoot.
~
$~
