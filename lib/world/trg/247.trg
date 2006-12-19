#24700
Demilich - 24712~
0 k 5
~
if %actor.is_pc%
  if %actor.level% > %self.level%
    %send% %actor% %self.name% tried to capture your soul.
    %echoaround% %actor% %self.name% gives %actor.name% an icy cold stare.
  else
    %send% %actor% %self.name% sucks you into one of his gems.
    %echoaround% %actor% %actor.name% disappears into one of %self.name%'s eyes.
    %send% %actor% Your soul is trapped within the demilich.
    %send% %actor% Slowly you feel your life-force drain away...
    %teleport% %actor% 0
    * once we can modify hit/mana/move this should be set to 1/0/0
  end
end
~
#24701
Ghoul Bite - 24705~
0 k 5
~
if %actor.is_pc%
  %send% %actor% %self.name% bites you.
  %echoaround% %actor% %self.name% bites %actor.name%.
  dg_cast 'poison' %actor%
end
~
#24702
Priest Align - 24703~
0 b 100
~
set actor %random.char%
if %actor.is_pc%
  if %actor.fighting%
    say You are commiting blasphemy!
    if %actor.align% > 300
      say You, %actor.name%, follow the True Path and are blessed.
      dg_cast 'bless' %actor%
    elseif %actor.align% > -300
      say it is not too late for you to mend your ways.
    else 
      emote grins and says, 'You, %actor.name%, are truly wretched.'
      say Blasphemy! %actor.name%, your presence will stain this temple no more!
      kill %actor.name%
    end
  end
end
~
$~
