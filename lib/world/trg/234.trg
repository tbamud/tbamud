#23400
Open Door - 23441~
2 c 100
push~
if %arg% == button
  %send% %actor% You push the button, and reveal a secret door!
  %echoaround% %actor% As %actor.name% pushes a button, a secret door is revealed.
  %door% 23441 west flags a
  %door% 23441 west name door
  %door% 23441 west room 23471 
else
  %send% %actor% Push what ?!
end
~
#23401
Open Door - 23471~
2 c 100
push~
if %arg% == button
  %send% %actor% You push the button, and reveal a secret door!
  %echoaround% %actor% As %actor.name% pushes a button, a secret door is revealed.
  %door% 23471 east room 23441
else
  %send% %actor% Push what ?!
end
~
#23402
Close Door - 23471~
2 g 100
~
wait 2 sec
%door% 23471 east purge 0
%door% 23441 west purge 0
%echo% A wall slams down shut!
~
$~
