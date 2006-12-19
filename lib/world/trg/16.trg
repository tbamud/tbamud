#1600
Unused portal~
2 c 100
en~
if %cmd.mudcommand% == enter && portal /= %arg%
  %send% %actor% You step through the mysterious portal.
  %echoaround% %actor% %actor.name% steps through the mysterious portal.
  %teleport% %actor% 3191
  %force% %actor% look
  %echoaround% %actor% %actor.name% enters through the portal.
else
  %send% %actor% %cmd% what?!
end
~
$~
