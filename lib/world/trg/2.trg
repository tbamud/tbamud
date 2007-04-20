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
Object Spells~
1 c 1
c~
* if command is cast and arg is fireshield (and all abbrevs of each)
if %cmd.mudcommand% == cast && fireshield /= %arg%
  switch %random.1%
    case 1
      if %actor.class% == Magic User
        %echoaround% %actor% %self.shortdesc% that %actor.name% is wearing glows brightly for a moment.
        %send% %actor% Your %self.shortdesc% glows brightly for a moment. 
        dg_cast 'armor' %actor%
      end
    break
    default
      %send% %actor% reached default case.
    break
    eval ward_major %ward_major%+1
    if %ward_major% == 2
      detach all %self.id%
    end
    global ward_major
  done
end
~
#203
Phoenix Rising - 211~
0 f 100
~
* You can't use waits in a mob death trig.
* Phoenix turns into ashes when it dies and is reborn 3 times.
* O219 is !TAKE and must always be in the room to count the deaths.
emote shrieks in pain and bursts into flames.
* Phoenix triggers 212 to count the deaths. Make it a unique command.
phoenix2
* Damage everyone in the room from the fire.
set room_var %self.room%
%echo% The phoenix is completely enveloped in flames and explodes, burning everyone in the room.
* Target the first char
set target_char %room_var.people%
* Loop through everyone in the room.
while %target_char%
  * Set the next target before this one perhaps dies.
  set tmp_target %target_char.next_in_room%
  * Dish out the damage.
  if %target_char.is_pc%
    %send% %target_char% The fire burns you.
    %damage% %target_char% 10
  end
  * Find next target
  set target_char %tmp_target%
  * Loop back
done
* Can't purge it (yet it is on the todo list) so teleport it to 0.
%teleport% %self% 0
* This prevents a death cry.
return 0
~
#204
Pirate Parott Pepeats - M212~
0 d 100
*~
* Parrot randomly repeats something said in the room.
* Pick the next number in the array and global it to be read next firing.
eval n %n%+1
global n
* Pick a random number from the array and global what was said to the mob.
eval r %%random.%n%%%
set speech[%n%] %speech%
global speech[%n%]
* Wait a little bit and then speak.
wait 1 sec
eval say %%speech[%r%]%%
eval say %say%
say %say%
~
#212
Phoenix Rising - 219~
1 c 4
phoenix2~
* Numeric Arg: 4 means obj has to be in the room.
* Does not work for level 32 and above.
* Add 1 to the total deaths
eval phoenix_deaths %phoenix_deaths% + 1
wait 3 sec
* Rebirth for only the first 3 times.
if %phoenix_deaths% <= 2
  * It comes back!
  %load% mob 211
  %echo% A baby phoenix pokes its head out of the pile of ashes.
else
  * Reward on the last kill!
  %load% obj 184
  eval phoenix_deaths 0
  %echo% Something in the pile of ashes glimmers brightly.
end  
* Remember the count for the next time this trig fires
global phoenix_deaths
~
$~
