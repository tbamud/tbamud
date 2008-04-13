#7900
Near Death Trap Fall- 7920~
2 g 100
~
* Near Death Trap stuns actor
wait 1 sec
set stunned %actor.hitp% 
%damage% %actor% %stunned%
wait 5 sec
%send% %actor% Watch your step next time.
~
#7901
Cityguard - 7900~
0 b 50
~
if !%self.fighting%
  set actor %random.char%
  if %actor%
    if %actor.is_killer%
      emote screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'
      kill %actor.name%
    elseif %actor.is_thief%
      emote screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'
      kill %actor.name%
    elseif %actor.cha% < 6
      %send% %actor% %self.name% spits in your face.
      %echoaround% %actor% %self.name% spits in %actor.name%'s face.
    end
    if %actor.fighting%
      eval victim %actor.fighting%
      if %actor.align% < %victim.align% && %victim.align% >= 0
        emote screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'
        kill %actor.name%
      end
    end
  end
end
~
$~
