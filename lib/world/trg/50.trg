#5000
Near Death Trap Rickety Rope Bridge - 5062~
2 g 100
~
* Near Death Trap stuns actor
wait 4 sec
set stunned %actor.hitp% 
%damage% %actor% %stunned%
wait 2 sec
%send% %actor% You somehow survive the fall and lay among the rocks.
%send% %actor% The Gods must favor you this day.
~
#5001
Magic User - 5004, 5010, 5014~
0 k 10
~
switch %actor.level%
  case 1
  case 2
  case 3
  break
  case 4
    dg_cast 'magic missile' %actor%
  break
  case 5
    dg_cast 'chill touch' %actor%
  break
  case 6
    dg_cast 'burning hands' %actor%
  break
  case 7
  case 8
    dg_cast 'shocking grasp' %actor%
  break
  case 9
  case 10
  case 11
    dg_cast 'lightning bolt' %actor%
  break
  case 12
    dg_cast 'color spray' %actor%
  break
  case 13
    dg_cast 'energy drain' %actor%
  break
  case 14
    dg_cast 'curse' %actor%
  break
  case 15
    dg_cast 'poison' %actor%
  break
  case 16
    if %actor.align% > 0
      dg_cast 'dispel good' %actor%
    else
      dg_cast 'dispel evil' %actor%
    end
 break
  case 17
  case 18
    dg_cast 'call lightning' %actor%
  break
  case 19
  case 20
  case 21
  case 22
    dg_cast 'harm' %actor%
  break
  default
    dg_cast 'fireball' %actor%
  break
done
~
#5002
Brass Dragon Guard - 5005~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == west
  * Stop everyone!
  return 0
  %send% %actor% The guard humiliates you, and blocks your way.
  %echoaround% %actor% The guard humiliates %actor.name%, and blocks %actor.hisher% way.
end
~
$~
