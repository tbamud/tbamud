#700
Panning for gold~
2 c 100
p~
* Don't let someone spam the trigger to make money Take some movement points away every round and stop when they get to 10
if %actor.move% <= 10
  %send% %actor% You are too exhausted to continue.
  halt
end
* Fire on pan gold or abbreviations of each word
if %cmd% /= pan && %arg% /= gold
  eval heldobj %actor.eq(hold)%
  * Make sure they picked up the gold pan in room 703 and are holding it
  if %heldobj.vnum% == 717
    %send% %actor% You dip your pan into the river and scoop up some of the river bed and begin swirling it in the water.
    %echoaround% %actor% %actor.name% dips a pan into the river and begins panning for gold.
    * Take 10 movement points away, wait 3 seconds and give a 1 in 10 chance of success.
    nop %actor.move(-10)%
    wait 3 sec
    if %random.10% == 1
      %send% %actor% You find a small gold nugget in the bottom of your pan.
      %echoaround% %actor% %actor.name% picks something out of %actor.hisher% pan.
      * Give them a nugget
      %load% obj 718 %actor% inv
    else
      %send% %actor% You find nothing of value.
    end
  else
    %send% %actor% You need a pan for that.
  end
end
~
$~
