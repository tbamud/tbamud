#26400
Healing Water~
1 s 100
~
set currenthunger %actor.hunger%
set currentthirst %actor.thirst%
if %currenthunger% > 20 && %currentthirst% > 0
  %send% %actor% Your stomach can't contain anymore!
else
  %send% %actor% The water seems to sparkle refreshingly as you drink.
  %echoaround% %actor% The clear water seems to perk %actor.name% up.
  %send% %actor% The clear water seems to perk %actor.name% up.
  dg_cast 'cure light' %actor%
  return 0
  if %actor.thirst% < 0
  else
    eval newhunger %currenthunger% + 1
    eval newthirst %currentthirst% + 5
    nop %actor.thirst(%newthirst%)%
    nop %actor.hunger(%newhunger%)%
    if %newthirst% > 20
      %send% %actor% You don't feel thirsty any more.
    end
  end
end
~
$~
