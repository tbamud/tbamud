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
* By Rumble of The Builder Academy    tbamud.com 9091
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
* By Rumble of The Builder Academy    tbamud.com 9091
* Allow a magic user to cast fireshield, but only twice.
if %cmd.mudcommand% == cast && fireshield /= %arg%
  if %actor.class% == Magic User
    %echoaround% %actor% %self.shortdesc% that %actor.name% is wearing glows brightly for a moment.
    %send% %actor% Your %self.shortdesc% glows brightly for a moment. 
    dg_cast 'armor' %actor%
  end
  eval ward_major %ward_major%+1
  if %ward_major% == 2
    detach all %self.id%
  end
  global ward_major
end
~
#203
Phoenix Rising - 211~
0 f 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
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
Pirate Parrot Repeats - M212~
0 d 100
*~
* By Rumble of The Builder Academy    tbamud.com 9091
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
#205
Crystal Ball to Locate a Mob.~
1 c 7
locate~
* By Rumble of The Builder Academy    tbamud.com 9091
set find %arg% 
if !%find.is_pc%
  eval rname %find.room% 
  %send% %actor% As you gaze into the ball, it starts to glow. You see an image of %find.name% in %rname.name%. 
else 
  %send% %actor% All that you see is a blurry haze. 
end 
%echoaround% %actor% %actor.name% peers into %actor.hisher% gently glowing crystal ball. 
~
#206
Smelly Bum - M168~
0 i 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* A trig to let people smell the bum from 1 room away.
* For the first move there is no from_room so set it.
if !%from_room%
  eval from_room %self.room.vnum%
  global from_room
  halt
end
wait 1 sec
eval inroom %self.room%
*
%at% %from_room% %echo% %self.name%'s smell slowly dissipates with %self.himher%.
if %inroom.north% && %inroom.north(vnum)% != %from_room%
  %at% %inroom.north(vnum)% %echo% You notice a nasty smell to the south.
end
if %inroom.south% && %inroom.south(vnum)% != %from_room%
  %at% %inroom.south(vnum)% %echo% You notice a nasty smell to the north.
end
if %inroom.east% && %inroom.east(vnum)% != %from_room%
  %at% %inroom.east(vnum)% %echo% You notice a nasty smell to the west.
end
if %inroom.west% && %inroom.west(vnum)% != %from_room%
  %at% %inroom.west(vnum)% %echo% You notice a nasty smell to the east.
end
if %inroom.up% && %inroom.up(vnum)% != %from_room%
  %at% %inroom.up(vnum)% %echo% You notice a nasty smell below you.
end
if %inroom.down% && %inroom.down(vnum)% != %from_room%
  %at% %inroom.down(vnum)% %echo% You notice a nasty smell above you.
end
*
eval from_room %self.room.vnum%
global from_room
%echo% FROM:%from_room% TO:%inroom%
~
#207
Mob Blocks opening of chest~
2 c 100
o~
* By Rumble of The Builder Academy    tbamud.com 9091 
* Make sure the command is open, check for any abbrev of chest
if %cmd.mudcommand% == open && chest /= %arg%
  * findmob checks if the mob is in the room.
  if %findmob.230(189)%
    %echoaround% %actor% As %actor.name% tries to approach the chest the commander looks up.
    %send% %actor% As you approach the chest the commander looks up at you.
    wait 1 sec
    %echo% The commander says, 'Get away from there.'
  else
    * If the commander is not in the room allow the "open chest" command to continue to the next command trigger.
    return 0
  end
else
  * If it doesn't match let the command continue.
  return 0
end
~
#208
Yoda Using Extract to Warp Speech~
0 d 1
*~
* By Rumble of The Builder Academy    tbamud.com 9091
wait 2 sec
extract word1 1 %speech%
extract word2 2 %speech%
extract word3 3 %speech%
extract word4 4 %speech%
extract word5 5 %speech%
extract word6 6 %speech%
say %word6% %word5% %word4% %word3% %word2% %word1%?
~
#209
Open a Chest once per zone reset~
1 c 100
open~
* By Mordecai
if chest /= %arg% 
  * Verify that the player typed 'open chest' 
  * Has the player already opened this chest? 
  context %actor.id% 
  if %already_opened_chest% 
    %send% %actor% The chest has already been opened, and emptied. 
  else 
    * The first time!  OK, open the chest. 
    %send% %actor% You get a jar of naphthalene from an iron bound chest.
    %load% obj 306 %actor% inv
    * Remember that the player has already opened this chest until next reboot/copyover. This limits this item to only be found once per boot per player.
    set already_opened_chest 1 
    global already_opened_chest
    return 0 
  end 
else 
  * Not 'open chest' - pass control back to the command parser 
  return 0 
end
~
#210
Butcher Shop Opens~
2 at 5
~
* By Rumble of The Builder Academy    tbamud.com 9091
%echo% The butcher's assistant steps out of the shop to the North and turns the sign over from "closed" to "open."
%load% obj 69
if %findobj.291(70)%
  %purge% signclosed
end
~
#211
Butcher Shop Closes~
2 at 21
~
* By Rumble of The Builder Academy    tbamud.com 9091
%echo% The butcher's assistant steps out of the shop to the North and turns the sign over from "open" to "closed."
%load% obj 70
if %findobj.291(69)%
  %purge% signopened
end
~
#212
Phoenix Rising - 219~
1 c 4
phoenix2~
* By Rumble of The Builder Academy    tbamud.com 9091
* Numeric Arg: 4 means obj has to be in the room.
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
  set phoenix_deaths 0
  %echo% Something in the pile of ashes glimmers brightly.
end  
* Remember the count for the next time this trig fires
global phoenix_deaths
~
#213
player damage~
0 b 100
test~
dg_cast 'armor' %self%
%damage% %self% 10
~
#214
bug test~
0 b 100
~
detach 214 %self.id%
say test crash!
~
#215
Christmas Greet~
0 h 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.is_pc%
  wait 1 sec
  if !%actor.has_item(222)%
    say Merry Christmas!
    %load% obj 222 %actor% inv
    %echoaround% %actor% Santa gives %actor.name% a wrapped present.
    %send% %actor% Santa gives you a wrapped present.
  end
end
~
#218
Holiday Decorations by Interior Design - M218~
0 d 100
decorate~
* By Rumble of The Builder Academy    tbamud.com 9091
* A trigger to make decorating for holidays easier. Just say "decorate <holiday>"
if %actor.level% > 31
  switch %speech.cdr%
    case christmas
      say decorating for Christmas, how fun
      %teleport% %self% 1204
      %load% obj 1299
      %load% obj 1318
      %load% obj 1319
      %load% obj 1336
      %load% obj 1337
      %load% obj 1338
      %load% obj 1339
      %load% obj 1340
      %load% obj 1341
      %load% obj 1397
      %load% mob 1308
      drop all
      %teleport% %self% 2
      %load% obj 1299
      %load% obj 1318
      %load% obj 1319
      %load% obj 1336
      %load% obj 1337
      %load% obj 1338
      %load% obj 1339
      %load% obj 1340
      %load% obj 1341
      %load% obj 1397
      %load% mob 1308
      drop all
      %teleport% %self% %actor.name%
      wait 1 sec
      say I'm all done
    break
    case new years
      say hmm, bringing in the new year.
      %teleport% %self% 1204
      %load% obj 1298
      %load% obj 1963
      drop all
      %teleport% %self% 2
      %load% obj 1298
      %load% obj 1963
      drop all
      %teleport% %self% %actor.name%
      wait 1 sec
      say I'm all done
    break
    case valentines
      say how sweet it is.
      %teleport% %self% 1204
      %load% obj 1304
      %load% obj 1342
      drop all
      %teleport% %self% 2
      %load% obj 1304
      %load% obj 1342
      drop all
      %teleport% %self% %actor.name%
      wait 1 sec
      say I'm all done
    break
    case Easter
      say I'll have to think about it
    break
    case fourth
      say ah, US independence day.
      %teleport% %self% 1204
      %load% obj 1298
      %load% obj 1963
      drop all
      %teleport% %self% 2
      %load% obj 1298
      %load% obj 1963
      drop all
      %teleport% %self% %actor.name%
      wait 1 sec
      say I'm all done
    break
    case Halloween
      say Hallow's Eve it is.
      %teleport% %self% 1204
      %load% mob 1313
      %load% obj 11712
      %load% obj 11713
      drop all
      %teleport% %self% 2
      %load% mob 1313
      %load% obj 11712
      %load% obj 11713
      drop all
      %teleport% %self% %actor.name%
      wait 1 sec
      say I'm all done
    break
    case thanksgiving
      say Turkey Day, ode to triptophan.
      %teleport% %self% 1204
      %load% mob 1322
      %load% obj 1331
      drop all
      %teleport% %self% 2
      %load% mob 1322
      %load% obj 1331
      drop all
      %teleport% %self% %actor.name%
      wait 1 sec
      say I'm all done
    break
    default
      * nothing is going to happen
    break
  done
else
  say I only work for the Gods.
end
~
#219
Modify AC on wear~
1 j 100
~
osetval 0 9
wait 1 sec
%send% %actor% As you wear %self.shortdesc% you feel more protected.
~
#220
Birthday Present Unwrap~
1 c 3
unwrap~
eval present %random.27%
eval present2 %present% * 1000
eval present3 %present2% + %random.4%
%send% %actor% You begin unwrapping the present.
%echoaround% %actor% %actor.name% begins unwrapping %actor.hisher% present.
wait 1 s
%load% obj %present3% %actor% inv
eval inv %actor.inventory%
%echo% As the wrapping falls apart, it reveals... %inv.shortdesc%.
%purge% %self%
~
#221
Open Sesame~
2 c 100
s~
if %cmd.mudcommand% == say && avaa ovi /= %arg%
  %echo% open door.
else
  %echo% don't open door.
end
~
#222
Mob Death - weapon name~
0 f 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.eq(wield)%
  set item   %actor.eq(wield)%
  say I can not believe I fell at the hands of one such as you.
  say I curse you %actor.name% and I curse %item.shortdesc% that you use.
else
  say I can not believe I fell at the hands of one such as you.
  say I curse you %actor.name%.
end
~
$~
