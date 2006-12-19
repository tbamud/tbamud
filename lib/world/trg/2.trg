#200
Chunky Philosopher - 206~
0 b 5
~
eval max %random.10%
set  txt[1] I signed up for an exercise class and was told to wear loose-fitting clothing. Hell, if I HAD any loose-fitting clothing, I wouldn't  have signed up for the damn class in the first place!
set  txt[2] When I was young and trim we used to go "skinny dipping,"  now... I just "chunky dunkin."
set  txt[3] Remember... the early bird still has to eat worms.
set  txt[4] The worst thing about accidents in the kitchen is having to eat them anyway.
set  txt[5] Never argue with an idiot; people watching the two of you squabbling may not be able to tell who's who.
set  txt[6] Wouldn't it be nice if whenever we messed up our life we could simply press 'Delete' and then 'copy/paste' to do the really great parts again?
set  txt[7] Real stress is when you wake up screaming and then you realize  you haven't fallen asleep yet.
set  txt[8] my wife says I never listen to her. At least I think that's what she said.
set  txt[9] Just remember...if the world didn't suck, we'd all fall off.
set  txt[10] Lift your right foot off the ground and make clockwise circles. Now, while doing this, draw the number "6" in the air with your right hand and try to keep your foot moving clockwise.
set  txt[11] On any non-digital watch point the hour hand at the sun. The point between that and 12 will be due south. You can also judge sunlight remaining by counting the hand-widths the sun is above the horizon (4 fingers width).
set  speech %%txt[%max%]%%
eval speech %speech%
say %speech%
~
#201
Fountain Teleport - 251~
1 c 7
en~
if %cmd.mudcommand% == enter && %arg% /= fountain
  %send% %actor% You step into the fountain getting yourself wet. Something grabs you and pulls you under.
  %echoaround% %actor% %actor.name% steps into the middle of the fountain getting %actor.himher%self wet.
  %echoaround% %actor% %actor.name% falls into the fountain and disappears.
  %teleport% %actor% 97
  nop %actor.pos(sleeping)%
  %at% 97 %echoaround% %actor% %actor.name% appears in the middle of the room laying on the floor unconscious.
else
  %send% %actor% %cmd% what?!
end
~
#202
Free Trig~
0 b 100
~
if %mhunt_target.room% == %self.room%
  mkill %mhunt_target%
  %echo% killing %mhunt_target.name%.
else
  %echo% hunting %mhunt_target.name%
  mhunt %mhunt_target%
end
~
$~
