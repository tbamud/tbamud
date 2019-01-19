#10400
Get Beer from the Keg - 10400~
1 c 100
pour~
if %cmd.mudcommand% == pour && beer /= %arg%
  %echoaround% %actor% %actor.name% pours himself a glass of beer.
  %load% obj 10401
  %send% %actor% You fill up a glass full of beer.
end
~
#10401
Birds in the trees - 10401~
2 b 100
~
wait 180 sec
%echo% The sound of birds echos through the forest, canaries and robins fly around in the area.
~
#10402
Little Children - 10402~
2 b 100
~
wait 30 sec
%echo% The sound of children laughing fills your head.
wait 25 sec
%echo% A small child tugs at your pants and disappears in a small puff of smoke.
wait 45 sec
%echo% Small handprints appear all over the trees, but quickly disappear.
~
#10403
Squirrels in the trees - 10403~
2 b 100
~
wait 24 sec
%echo% Squirrels run around in the trees, scratching the bark.
~
#10404
Deer Sounds - 10404~
2 b 50
~
%echo% The leaves rustle around you as something brushes up against them.
wait 420 sec
%echo% A Deer dashes across the path, but quickly disappears into the woods on the other side.
~
#10406
Welcome - (10400, 10401, 10402, & 10409)~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  say Hello, welcome to my shop.
  wait 1 sec
  say How can I be of service?
end
~
#10407
Welcome Zone 104 - 10413~
0 d 100
hello Jewel~
wait 2 sec
say Hello, and welcome the the land of Orchan.
wait 2 sec
say Please feel free to look around %actor.name%.
~
#10408
Journal Take Trigger~
1 c 2
write~
if %actor.id% == 13
  %send% %actor% %actor.name% welcome back home.
  %force% %actor% write journal
else
  %send% %actor% You do not have permission to write in this journal.
end
~
#10409
Sleep chair~
1 c 100
sl~
if %actor.id% == 4891
  %echoaround% %actor% %actor.name% falls asleep in the comfort of the chair.
  %send% %actor% You drift into a calm slumber.
  %force% %actor% sleep
else
  %force% %actor% sleep
end
~
#10410
test trig~
1 c 2
drop~
if %room.vnum% == 1209
  if %cmd.mudcommand% == drop && infra /= %arg%
    %echoaround% %actor% %actor.name% drops the infra on the ground.
    %echaround% %actor% The infra just disappears.
    %load% obj 8818
    %purge% %self%
  else
    if %cmd.mudcommand% == drop && infra /= %arg%
      %force% %actor% drop infra
    end
  end
end
~
#10411
Quest for Armor~
0 g 100
~
if %actor.is_pc%
  if %direction% == north
    if %actor.inventory(10429)%
      say Do you have the item?
    else
      if %actor.is_pc%
        if %direction% == north
          say Hello there young one.
          wait 2 sec
          say I am the village elder, the oldest and wisest of them all.
          wait 4 sec          
          say %actor.name%, if you bring me back the item from the center of the maze, I will reward you with great riches and power.
          wait 4 sec          
          say Will you accept my quest? Will you go to the center of the maze and retrieve the sacred item?
        end
      end
    end
  end
end
~
#10412
Accept Quest~
0 d 100
yes~
say So you do wish to venture to the center of the maze!
wait 2 sec
say You will find the maze to the east of this town.
wait 2 sec
say May the Gods of Miz'real be with you.
wait 1 sec
%echo% %self.name% waves his hands around and chants some strange language.
wait 3 sec
%echoaround% %actor% %actor.name% is engulfed in a bright light, and disappears.
%send% %actor% You are engulfed within a bright light, and when the light fades, you notice you are not where you were.
%teleport% %actor% 10413
%force% %actor% look
~
#10413
Finding the Armor~
1 g 100
~
%echoaround% %actor% As %actor.name% picks up the armor, the room is filled with a mystical 	Wwhite	n light.
%send% %actor% You pick up the armor and you are engulfed in a 	Wwhite	n light.
%teleport% %actor% 10413
%echoaround% %actor% As the light fades, you notice %actor.name% is no longer there.
%echoaround% %actor% Nor is the armor.
wait 2 sec
%send% %actor% The light fades away and you notice you are no longer in the maze.
%force% %actor% look
~
#10414
bringing armor back~
0 j 100
~
if %object.vnum% == 10429
  %force% %actor% give armor elder
  wait 2 s
  say Hmmm, this looks like the armor.
  wait 2 s
  say If you obtained this armor, you must be strong.
  wait 2 s
  say The armor is yours, take it.
  %load% obj 10434 %actor% inv
  wait 2 sec
  %send% %actor% The Village Elder hands you the armor then you watch as %self.name% rummages through a bag on his side and pulls out some coins.
  wait 2 s
  say You may also have this.
  wait 2 s
  %send% %actor% %self.name% hands you the coins, you feel a slight surge of energy.
  %echoaround% %actor% %self.name% rummages through a bag and pulls out a couple coins.
  wait 2 s 
  %echoaround% %actor% %self.name% hands the coins to %actor.name% , a small light emits from the coins.
  nop %actor.gold(150)%
  nop %actor.exp(400)%
  %send% %actor% You receive 150 gold coins and 400 experience.
  return 0
else
  wait 2 s
  say What is this? I don't want this.
  wait 1 s
  say Come back when you have the real thing!
  return 0
end
~
#10416
WeatherMan~
0 d 0
whats the weather~
eval here %self.room%
eval today %here.weather%
say The weather eh? Give me a little.
wait 1 sec
%echo% %self.name% looks up into the sky and pulls a notepad out of his pocket.
wait 3 sec
%echo% %self.name% begins to jot down something on his notepad.
wait 3 sec
%echo% %self.name% scratches his head then looks at %actor.name%.
wait 10 sec
if %today% == rainy
  %echo% %self.name% places a raincoat on, then pulls out an umbrella.
  wait 2 sec
  say It's pouring out today.
elseif %today% == sunny
  %echo% %self.name% puts on some sunglasses and places some sunscreen on his nose.
  wait 2 sec
  say I just might go to the beach today.
  wait 2 sec
  say  Well, my forecast for taday is simple, it is bright and clear.
elseif %today% == lightning
  %echo% %self.name% watches the flashing lights in the sky smiling.
  wait 2 sec
  say Pretty ain't it?
  wait 2 sec
  say Well, my forecast is simple, we have scattered lighting, with a slight chance of rain.
elseif %today% == cloudy
  %echo% %self.name% takes cover under a tree.
  wait 2 sec
  say Looks like it might rain today, thoase clouds are rather dark.
else
  say I cannot tell you the weather while Im indoors.
end
~
#10417
Peek Tool~
1 c 1
glance~
eval room0 %self.room%
eval homeroom %room0.vnum%
if %arg% == north || if %arg% == south || if %arg% == east || if %arg% == west || if %arg% == up || if %arg% == down
  eval room1 %self.room%
  eval otherroom %%room1.%arg%(vnum)%%
  if %otherroom%
    %teleport% %actor% %otherroom%
    %force% %actor% l
    %teleport% %actor% %homeroom%
  else
    %send% %actor% There is not a room in that direction.
  end
else
  %send% %actor% What direction do you wish to glance?
end
~
#10418
Run Trigger~
0 g 100
~
* No Script
~
#10419
Knock out Gas - 10457~
2 g 100
~
if %actor.is_pc%
  %echo% Gasses begin to fill the room, and the stench becomes unbearable.
  wait 2s
  %send% %actor% You begin to feel your self lose sight of the room.
  %echoaround% %actor% %actor.name%'s eyes shut and %actor.heshe% slumps over.
  nop %actor.pos(sleeping)%
  wait 3s
  %echoaround% %actor% Strange words can be heard coming from somewhere, and %actor.name%'s body floats into the air and vanishes.
  wait 1s
  %teleport% %actor% 10424
  nop %actor.wait(200)%
end
~
#10499
Test Trig~
2 q 100
~
if %direction% == west
  %door% 100 west flags abc
end
~
$~
