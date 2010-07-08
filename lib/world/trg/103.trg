#10302
Room Command - Teleporter to Earth - 10302~
1 c 100
enter~
if %cmd.mudcommand% == enter && teleporter /= %arg% && %arg%
%send% %actor% You step into the teleporter.
%echoaround% %actor% %actor.name% steps into the teleporter and vanishes.
%teleport% %actor% 10304
%force% %actor% look
%echoaround% %actor% %actor.name% appears in a flash of light.
%force% %actor% xx
else
%send% %actor% enter what?
end
~
#10303
Obj Command - Teleport to Namek - 10303~
2 c 100
enter~
if %cmd.mudcommand% == enter && teleporter /= %arg% && %arg%
  %send% %actor% You step into the teleporter.
  %echoaround% %actor% %actor.name% steps into the teleporter and vanishes.
  %teleport% %actor% 10305
  %force% %actor% look
  %echoaround% %actor% %actor.name% appear in a flash of light.
  %force% %actor% xx
else
  %send% %actor% enter what?
end
~
#10304
Kami's Welcome - 10304~
0 ci 100
xx~
wait 1 sec
say Hello, welcome to my lookout.
wait 2 sec
say I am Kami, and this is Mr. Popo.
~
#10305
Welcome To Namek - 10305~
0 c 100
xx~
wait 1 sec
say Welcome to planet Namek, please feel free to look around.
wait 2 sec
say I am Nameks Priest, I hope you have a good time on our planet, if you have
time you should visit the Elder.
~
#10306
Nimbus Cloud - 10304~
2 c 100
board~
if board /= %cmd% && cloud /= %arg% && %arg%
  %send% %actor% You jump onto the cloud and take a seat.
  %echoaround% %actor% %actor.name% jumps up onto nimbus.
  wait 3 sec
  %send% %actor% The nimbus cloud takes off down the tower.
  %echoaround% %actor% The nimbus cloud takes off down the tower.
  %teleport% %actor% 10306
  wait 10 sec
  %echoaround% %actor% The cloud lands and %actor.name% steps off.
  %echoaround% %actor% The cloud flies off to the top of the tower.
  %send% %actor% The cloud lands and you jump off landing safely on the ground.
  wait 2 sec
  %force% %actor% look
else
  %send% %actor% board what?
end
~
#10307
Kill Wolf - 10307~
0 f 100
~
%echoaround% %actor% @RThe wolf lets out an ear splitting howl as it dies.@n
%send% %actor% @RThe wolf lets out an ear splitting howl has it dies.@n
%send% %actor% @rBlood from the wolf splatters over your armor.@n
%load% mob 10307
~
#10308
Transport - 10306~
1 c 1
goto~
%echoaround% %actor% %actor.name% smiles and a bright flame emits from
%actor.hisher% neckless and engulfs %actor.himher%, and in a instant you are
blinded by a flash from the fire.
%send% %actor% You smile as a flame emits from your neckless and it engulfs
you, you
%send% %actor% dissappear in a flash of light.
wait 3 sec
%echoaround% %actor% The light clears, but it takes a bit for your eyes to
adjust, when they do you notice %actor.name% is gone.
%teleport% %actor% %arg%
%send% %actor% When the flames subside, you notice you are somewhere else.
%echoaround% %actor% In a large flash of light you are blinded, and when the
light subsides, and your eyes readjust, %actor.name% is in the room with you.
wait 2 sec
%force% %actor% look
~
#10309
Test~
0 c 100
greet~
if %mudcommand.cmd% greet && woman /= %arg%
  wait 1 sec
  say Hello.
  wait 2 sec
  say Will you help me open this gate?
end
~
#10310
Tell~
1 c 2
tell~
%send% %arg% @G%arg%@n
~
#10314
Matildas Welcome - 10316~
0 g 100
~
if %direction% == north
wait 1 sec
say Hello, my name is Matilda.
wait 2 sec
say Master Vegeta is in his atire.
wait 1 sec
say And there is a fighter here to help you train.
end
~
#10315
Newbie Fighter Sacrifice - 10318~
2 c 100
sac~
if sacrifice /= %cmd% && fighter /= %arg%
  %send% %actor% You sacrifice the corpse of A Newbie Fighter.
  %echoaround% %actor% Dabura gives %actor.name% one zeni for his sacrifice.
  %send% %actor% Dabura gives you one zeni for your sacrifice.
  %purge% %self%
else
  %send% %actor% Sacrifice what?
end
~
#10316
Sac Vegeta - 10319~
0 c 100
sac~
if sacrifice /= %cmd% && vegeta /= %arg%
  %send% %actor% You sacrifice the corpse of vegeta.
  %echoaround% %actor% Dabura gives %actor.name% one zeni for his sacrifice.
  %send% %actor% Dabura gives you one zeni for your sacrifice.
  %purge% %self%
else
  %send% %actor% Sacrifice what?
end
~
#10318
Reload Fighter - 10318~
0 f 100
~
* reload mob for next fighter
%load% mob 10318
%load% obj 10318
%teleport% %self% 0
~
#10319
Reload Vegeta - 10319~
0 f 100
~
say How can this be! I am the Ultimate Saiyan!
%load% obj 10319
%teleport% %self% 0
wait 360 sec
%load% mob 10319
~
#10320
Obj Command - Teleport to CoU - 10320~
1 c 100
enter~
if %cmd.mudcommand% == enter && teleporter /= %arg% && %arg%
  %send% %actor% You step into the teleporter.
  %echoaround% %actor% %actor.name% steps into the teleporter and vanishes.
  %teleport% %actor% 10301
  %force% %actor% look
  %echoaround% %actor% %actor.name% appears in a flash of light.
else
  %send% %actor% enter what?
end
~
#10321
Waves - 10321~
2 b 100
~
wait 180 sec
%echo% @cA gentle wave washes up onto the shore.@n
~
#10322
Fish jumping - 10322~
2 b 100
~
wait 180 sec
%echo% Beautiful fish jump upstream splashing you gently.
~
#10323
Computer Flash - 10323~
2 b 100
~
wait 180 sec
%echo% @WThe computer screen flashes filling the room with light.@n
~
#10329
Roshis Magazine Snicker - 10329~
0 b 100
~
wait 120 sec
%echo% @bMaster Roshi snickers and turns the page not even looking at you.@n
~
#10330
Master Roshis Kamehameha Wave - 10329 ~
1 c 7
kame~
if %cmd%==kamehameha && %arg%==wave && %actor.fighting%
  %damage% %actor.fighting% 99999
  %echoaround% %actor% @G%actor.name% clasps %actor.hisher% hands at his side
and begins to yell "KaMeHaMeHa!!!!"@n
  %send% %actor% @GYou clasps his hands at your side and begin to yell
"KaMeHaMeHa!!!!"@n
  %echoaround% %actor% @G%actor.name% then thrusts %actor.hisher% hands foward
sending a blue ball of ki straight at %actor.hisher% enemy.@n
  %send% %actor% @GYou then thrusts your hands foward sending a blue ball of
ki straight at your enemy.@n
end
~
#10331
give headband - 10329~
0 g 100
~
if %actor.is_pc%
  if %direction% == south
    wait 2 sec
    say So, you wish to have my kamehame wave do you?
  end
end
~
#10332
Agree - 10332~
0 d 100
yes~
wait 1 sec
say Alright, here you go.
give headband %actor.name%
wait 1 sec
say To use it just type kamehameha wave in battle.
~
#10339
Truncks - 10339~
0 b 100
~
wait 180 sec
%echo% @cTruncks growls at the game he is playing.
~
#10345
Bulma Hums - 10345~
0 b 100
~
wait 180 sec
%echo% @cBulma hums to herself smiling happily.
~
#10355
The wind on the mountain - 10355~
2 b 100
~
wait 60 sec
%echo% @bThe wind blows from all around as you fly.@n
~
#10362
Open Door - 10362~
2 c 100
touch~
if %arg% == dragon
  %echo% The statue suddenly comes to life and looks at the person whom
awakend it, its eyes glow red as it puts out one arm  to open the gate.
  %door% 10362 west flags a
  %door% 10362 west key 10363
  %door% 10362 west name Gate
  %door% 10362 west room 10305
  wait 60 sec
  %door% 10362 west flags abc
  %echo% The Gate closes.
else
  %send% %actor% Touch What ?
end
~
#10363
Unlock Gate~
0 c 100
touch~
if %arg% == dragon
  %echo% The statue suddenly comes to life and looks at the person whom
awakend it, its eyes glow red as it puts out one arm to open the gate.
  %force% %self% unlock door west
  wait 30 sec
  %force% %self% lock door west
  %send% %actor% The doors slam shut beind you.
end
~
#10364
Unlock Gate - 10305~
2 c 100
touch~
if %arg% == dragon
  %echo% The statue suddenly comes to life and looks at the person whom
awakend it, its eyes glow red as it puts out one arm  to open the gate.
  %door% 10305 east flags a
  %door% 10305 east key 10362
  %door% 10305 east name Gate
  %door% 10305 east room 10362
  wait 60 sec
  %door% 10305 east flags abc
  %echo% The gate slowly begins to shut, and takes serveral seconds to do so.
else
  %send% %actor% Touch What ?
end
~
#10376
Pulsating orb -- 10376~
2 b 100
~
wait 180 sec
%echo% @WThe orb in the center of the room pulsates letting off more light.@n
~
#10389
Obj Command -- Teleport CoU - 10389~
1 c 100
enter~
if %cmd.mudcommand% == enter && teleporter /= %arg%
  %send% %actor% You step into the teleporter.
  %echoaround% %actor% %actor.name% steps into the teleporter and vanishes.
  %teleport% %actor% 10301
  %force% %actor% look
  %echoaround% %actor% %actor.name% appears in a flash of light.
else
  %send% %actor% Enter What ?
end
~
$~
