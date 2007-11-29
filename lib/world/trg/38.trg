#3800
Rat Hiss~
0 ab 20
~
%echo% The rat chews on an iron object.
wait 5 s
%echo% The rat stops and starts hissing.
wait 5 s
%echo% The rat continues to chew.
~
#3801
unlimited fountain~
1 j 100
~
otransform 3801
%echo% The ring refills itself!
~
#3802
Backstabber~
1 j 100
~
%echo% You feel like you could walk right up beind a Cityguard without him noticing!
~
#3804
Slaive Attack~
0 g 100
~
wait 10s
%echoaround% %actor% The carcass rises up and plants his dagger in %actor.name%'s back!
%send% %actor% The corpse of Slaive rises up and plants his dagger in your back!
%damage% %actor% 100
mkill %actor%
~
$~
