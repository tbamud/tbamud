#10600
Armourer - 10634~
0 g 100
~
if %actor.is_pc%
  if %actor.sex% == male
    say Good day to ya sir, my friend to the east has my wares for sale, we have a partnership.
    chuckle
  elseif %actor.sex% == female
    say Good day to ya maam, my friend to the east has my wares for sale, we have a partnership.
    smirk
  end
end
~
#10601
Near Death Trap - 10650~
2 g 100
~
* Near Death Trap stuns actor
wait 1 sec
set stunned %actor.hitp% 
%damage% %actor% %stunned%
wait 5 sec
%send% %actor% The Gods pity you enough to allow you to survive.
~
$~
