#3300
Death Trap Pit - 3372~
2 g 100
~
* Near Death Trap stuns actor, then poison will almost kill them.
wait 3 sec
set stunned %actor.hitp% 
%damage% %actor% %stunned%
dg_affect %actor% poison on 1
~
$~
