#5730
Scorpion -5730~
0 h 100
~
if %actor.is_pc%
  wait 2 sec
  say Welcome to my parlor said the spider to the fly!
  wait 2 sec
  say of course, that spider is a friend of mine!
  wait 2 sec
  cackle
  wait 2 sec
  mkill %actor%
end
~
#5733
Assassin - NOT ATTACHED~
0 h 100
~
if %actor.is_pc%
  say Hello traveller!
  wait 1 sec
  say how are you?
  wait 1 sec
  say well, now that you've seen me I'll have to kill you...
  wait 1 sec
  mua
  wait 1 sec
  mkill %actor.name%
end
~
#5734
Crannasia - 5734~
0 h 100
~
if %actor.is_pc%
  wait 2 sec
  say Welcome traveller
  wait 4 sec
  say how are you faring?
  wait 4 sec
  say well, regardless of that it's time for you to die!
  wait 3 sec
  say goodbye weakling
  wait 2 sec
  mua
  wait 2 sec
  mkill %actor%
end
~
#5794
Claw - NOT ATTACHED~
1 j 100
~
wait 1 sec
say HEY! I won't serve you!
%purge% self
~
#5799
Near Death Trap Fall - 5707~
2 g 100
~
* Near Death Trap stuns actor
wait 2 sec
set stunned %actor.hitp% 
%damage% %actor% %stunned%
wait 2 sec
%send% %actor% You continue to fall... and fall... and fall.
~
$~
