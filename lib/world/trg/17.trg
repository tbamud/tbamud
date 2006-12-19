#1700
Excalibur - two handed sword - 1702~
1 j 100
~
if %actor.eq(hold)% && %actor.eq(light)%
  *
  ** too long?, I had to eval light so it would be shorter and work.
  *%send% %actor% %self.shortdesc% is a two-handed weapon. You must remove %actor.eq(light).shortdesc% and %actor.eq(hold).shortdesc% to wield it.
  *
  eval light %actor.eq(light).shortdesc%
  %send% %actor% %self.shortdesc% is a two-handed weapon. You must remove %light% and %actor.eq(hold).shortdesc% to wield it.
  return 0
elseif %actor.eq(hold)%
  %send% %actor% %self.shortdesc% is a two-handed weapon. You need to remove %actor.eq(hold).shortdesc%.
  return 0
elseif %actor.eq(light)%
  %send% %actor% %self.shortdesc% is a two-handed weapon. You need to remove %actor.eq(light).shortdesc%.
  return 0
end
~
#1701
Excalibur - two handed sword - 1702~
1 c 1
ho~
if %cmd.mudcommand% == hold 
  %echo% You will have to remove %self.shortdesc% to hold anything else.
end
~
$~
