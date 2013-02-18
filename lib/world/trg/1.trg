#100
Obj Command 100 - portal to Midgaard~
1 c 7
en~
* By Rumble of The Builder Academy    tbamud.com 9091
if %cmd.mudcommand% == enter && portal /= %arg%
  %send% %actor% You enter the portal.
  %echoaround% %actor% %actor.name% bravely enters the portal.
  %teleport% %actor% 3001
  %force% %actor% look
  %echoaround% %actor% %actor.name% just stepped through a portal.
else
  %send% %actor% %cmd% what?!
end
~
#101
Room Command - portal to Midgaard~
2 c 100
en~
* By Rumble of The Builder Academy    tbamud.com 9091
if %cmd.mudcommand% == enter && portal /= %arg%
  %send% %actor% You enter the portal.
  %echoaround% %actor% %actor.name% bravely enters the portal.
  %teleport% %actor% 3001
  %force% %actor% look
  %echoaround% %actor% %actor.name% just stepped through a portal.
else
  %send% %actor% enter what?!
end
~
#102
Mob Command - portal to Midgaard~
0 c 100
en~
if %cmd.mudcommand% == enter && portal /= %arg%
  %send% %actor% You enter the portal.
  %echoaround% %actor% %actor.name% bravely enters the portal.
  %teleport% %actor% 3001
  %force% %actor% look
  %echoaround% %actor% %actor.name% just stepped through a portal.
else
  %send% %actor% enter what?!
end
~
#103
Mob Greet Newbie Guide - 196~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.is_pc%
  if %actor.level% <= 3
    wait 1 sec
    bow
    wait 1 sec
    say may I suggest you visit the newbie zones under HELP AREAS by typing TELEPORT NEWBIE.
  end
  if %actor.level% >= 30
    wait 1 sec
    bow %actor.name%
  end
end
~
#104
dg_cast fireball~
0 k 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Only cast the spell every 30 seconds.
if %already_cast%
  wait 30 s
  unset already_cast
else
  dg_cast 'fireball' %actor.name%
  set already_cast 1
  * By globalling the variable it can be accessed by other triggers or when
  * this trigger fires a second time.
  global already_cast
end
~
#105
Mob Greet Hannibal - 140~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Shopkeeper greets players based on male/female/neutral.
if %actor.is_pc%
  wait 1 sec
  if %actor.sex% == male
    say Good day sir, what would you like? 
  elseif %actor.sex% == female
    wait 1 sec
    say Good day maam, what can I get you?
  else
    say What do you want?
  end
end
~
#106
Mob Greet Carpenter - 197~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  if %actor.sex% == male
    say Can't you see the place is under repairs?
    wait 1 sec
    say Don't worry, the inn will be open again soon.
  elseif %actor.sex% == female
    wait 1 sec
    say Come to work, have you?
    wait 1 sec
    wink %actor.name%
  else
    frown %actor.name%
  end
end
~
#107
Mob Greet Shiro - 103~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  say I sell the finest weapons in all the realm. See for yourself.
end
~
#108
Mob Greet Rhian - 104~
0 g 100
~
if %actor.is_pc%
  wait 3 sec
  if %actor.sex% == male
    smile %actor.name%
  elseif %actor.sex% == female
    wait 1 sec
    frown %actor.name%
  else
    say I hate your kind.
  end
end
~
#109
Mob Greet Sarge - 109~
0 g 100
~
if %actor.is_pc%
  look %actor.name%
  wait 1 sec
  if %actor.sex% == male
    say See anything you like?
  elseif %actor.sex% == female
    wait 1
    gaze %actor.name%
    wait 1
    say What can I get you pretty lady?
  else
    say What do you want?
  end
end
~
#110
Mob Greet Logan - 110~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  grin %actor.name%
  wait 1 sec
  if %actor.sex% == male
    say Look at this fine vest.
    wait 1 sec
    emote holds up a gaudy red vest.
    wait 1 sec
    say this would wear well on you.
  elseif %actor.sex% == female
    say for you my young lady, I have a fine silk shirt.
  else
    say What do you want?
  end
end
~
#111
Mob Greet Branwen - 111~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  if %actor.sex% == male
    say if it's made from leather, I have it.
  elseif %actor.sex% == female
    wait 1 sec
    say the finest leather in the realms is what I sell.
  else
    say What do you want?
  end
end
~
#112
Mob Greet Morgan - 184~
0 g 33
~
if %actor.is_pc%
  wait 1 sec
  sigh
  wait 1 sec
  if %actor.sex% == male
    say need a drink. I sure do
    wait 1 sec
    emote downs a shot of whisky.
  elseif %actor.sex% == female
    wait 1 sec
    say can I get you a drink.
    wait 1 sec
    ogle %actor.name%
  else
    say What do you want?
  end
end
~
#113
Mob Greet Ingrid - 182~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  cackle %actor.name%
  wait 1 sec
  if %actor.sex% == male
    say ahhh, doesn't that smell good.
  elseif %actor.sex% == female
    wait 1 sec
    say what can I get you, my pretty.
  else
    say what would you like?
  end
end
~
#114
Mob Greet Corwin - 110~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  emote frowns at a large stack of mail.
end
~
#115
Mob Greet Banker - 119~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  emote puts all his money in the safe when he notices you eyeing it.
end
~
#116
Mob Greet Hazel - 109~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  chuckle
  wait 1 sec
  if %actor.sex% == male
    say thirsty?
  elseif %actor.sex% == female
    wait 1 sec
    say I sell pure water, no worry about contaminants from me.
  else
    say need some water?
  end
end
~
#117
Mob Greet Carla - 158~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  emote steps away from her sewing machine.
  wait 1 sec
  if %actor.sex% == male
    say anything I can help you with sir?
  elseif %actor.sex% == female
    wait 1 sec
    say I could make something nice for a woman like you.
  else
    say need some clothes?
  end
end
~
#118
Mob Greet Ian - 101~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  if %actor.sex% == male
    say I make furs for the common man, because that is what I am.
  elseif %actor.sex% == female
    wait 1 sec
    say a fine fur coat would suit you well.
  else
    say need some fur?
  end
end
~
#119
Mob Greet Liam - 119~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  if %actor.sex% == male
    say Take your time, i've got all day.
  elseif %actor.sex% == female
    wait 1 sec
    say you aren't really an adventurer are you?
    wait 1 sec
    say who woulda thought a woman adventuring.
  else
    say need some supplies?
  end
end
~
#120
Mob Greet Baker - 187~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  say best bread in town.
  wait 1 sec      
  if %actor.sex% == male
    emote slams some dough down onto the counter.
  elseif %actor.sex% == female
    wait 1 sec
    say I'm hiring if you can cook.
  else
    say need some food?
  end
end
~
#121
Mob Greet Butcher - 199~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  grin
  wait 1 sec
  say I can butcher anything.
  wait 1 sec
  if %actor.sex% == male
    emote splatters some blood on you as he hacks at some meat.
  elseif %actor.sex% == female
    wait 1 sec
    say sorry about the mess mam.
  else
    emote rubs his bloody hands on his apron.
  end
end
~
#122
Mob Greet Rowan - 111~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  smile
  wait 1 sec
  say see anything you would like.
  wait 1 sec
  if %actor.sex% == male
    emote watches you carefully.
  elseif %actor.sex% == female
    wait 1 sec
    say isn't this diamond beautiful.
  else
    emote points you to the display cases.
  end
end
~
#123
Mob Greet Fiona - 124~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  glare
  wait 1 sec
  if %actor.sex% == male
    say much better food here than the baker or butcher sells.
    wait 1 sec
    whisper %actor.name% I hear the butcher's meat is tainted.
  elseif %actor.sex% == female
    wait 1 sec
    say hungry?
  else
    say hungry?
  end
end
~
#124
Mob Greet Lugdach - 106~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  if %actor.sex% == male
    say Aye! What can I getcha!
  elseif %actor.sex% == female
    wait 1 sec
    say What's a fine lass like you doing here?
  else
    emote need a boat?
  end
end
~
#125
Mob Greet Healer - 186~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  smile %actor.name%
  wait 1 sec
  if %actor.sex% == male
    say What aid do you need?
  elseif %actor.sex% == female
    wait 1 sec
    say Can I help you miss?
  else
    say What do you want?
  end
end
~
#126
Mob Greet Sareth - 185~
0 g 100
~
if %actor.is_pc%
  wait 1 sec
  bow
  wait 1 sec
  if %actor.sex% == male
    say Are you in need of a scroll?
  elseif %actor.sex% == female
    wait 1 sec
    say I have many types of scrolls
  else
    say What do you want?
  end
end
~
#127
Mob Act - 156 speaker greet~
0 e 0
has entered the game.~
* By Rumble of The Builder Academy    tbamud.com 9091
* Num Arg 0 means the argument has to match exactly. So trig will only fire off:
* "has entered game." and not "has" or "entered" etc. (that would be num arg 1).
* Figure out what vnum the mob is in so we can use zoneecho.
*NOTE: We now have a room-login trig. HELP TRIGEDIT-ROOM-LOGIN
eval inroom %self.room%
%zoneecho% %inroom.vnum% %self.name% shouts, 'Welcome, %actor.name%!'
~
#128
Mob Act - 156 speaker goodbye~
0 e 0
has left the game.~
eval inroom %self.room%
%zoneecho% %inroom.vnum% %self.name% shouts, 'Farewell, %actor.name%!'
~
#129
Mob Greet Beggar - 165~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.is_pc%
  wait 1 sec
  say Money for the poor?
end
~
#130
Mob Bribe Beggar - 165~
0 m 1
~
* By Rumble of The Builder Academy    tbamud.com 9091
wait 1 sec
if %actor.sex% == MALE
  say Thank you, kind sir.
elseif %actor.sex% == FEMALE
  say Thank you, ma'am.
else
  emote looks you over trying to determine your sex.
  say Thank you.....
end
~
#131
Room Command 365 - Jump~
2 c 100
jump~
* By Rumble of The Builder Academy    tbamud.com 9091
wait 1 sec
%send% %actor% You jump from the window ledge to certain death.
%echoaround% %actor% %actor.name% decides to test fate and takes a dive out the window.
%teleport% %actor% 292
wait 1 sec
%echoaround% %actor% %actor.name% falls from above screaming %actor.hisher% lungs out. %actor.heshe% hits the ground with a loud thump.
%force% %actor% look
%send% %actor% You strike the ground hard but somehow manage to survive the impact.
* Damage the player all of their hitpoints. They will recover.
set stunned %actor.hitp%
%damage% %actor% %stunned%
~
#132
dg_cast by level~
0 k 10
~
* By Rumble of The Builder Academy    tbamud.com 9091
switch %actor.level%
  case 1
    case 2
    case 3
    dg_cast 'magic missile' %actor%
  break
  case 5
    dg_cast 'chill touch' %actor%
  break
  case 6
    dg_cast 'burning hands' %actor%
  break
  case 7
    dg_cast 'shocking grasp' %actor%
  break
  case 8
  case 9
    dg_cast 'lightning bolt' %actor%
  break
  case 10
    dg_cast 'blindness' %actor%
  break
  case 11
    dg_cast 'color spray' %actor%
  break
  case 12
    dg_cast 'lightning bolt' %actor%
  break
  case 13
    dg_cast 'energy drain' %actor%
  break
  case 14
    dg_cast 'curse' %actor%
  break
  case 15
    dg_cast 'poison' %actor%
  break
  case 16
    if %actor.align% > 0
      dg_cast 'dispel good' %actor%
    else
      dg_cast 'dispel evil' %actor%
    end
  break
  case 17
    dg_cast 'call lightning' %actor%
  break
  case 18
    case 19
    dg_cast 'harm' %actor%
  break
  * Level 20 and above gets fireball!
  default
    dg_cast 'fireball' %actor%
  break
done
~
#133
Warrior Guildguard - 127~
0 q 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Replaces the guildguard special procedure.
* Check the direction the player must go to enter the guild.
if %direction% == up
  * Stop them if they are not the appropriate class.
  if %actor.class% != warrior
    return 0
    %send% %actor% The guard humiliates you, and blocks your way.
    %echoaround% %actor% The guard humiliates %actor.name%, and blocks %actor.hisher% way.
  end
end
~
#134
Mage Guildguard - 173~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == up
  * Stop them if they are not the appropriate class.
  if %actor.class% != magic user
    return 0
    %send% %actor% The guard humiliates you, and blocks your way.
    %echoaround% %actor% The guard humiliates %actor.name%, and blocks %actor.hisher% way.
  end
end
~
#135
Cleric Guildguard - 174~
0 q 100
~
* Check the direction the player must go to enter the guild.
if %direction% == up
  * Stop them if they are not the appropriate class.
  if %actor.class% != cleric
    return 0
    %send% %actor% The guard humiliates you, and blocks your way.
    %echoaround% %actor% The guard humiliates %actor.name%, and blocks %actor.hisher% way.
  end
end
~
#136
Thief Guildguard - 177~
0 q 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* This replaces stock guildguard spcial procedure that stops non-class members.
if %direction% == up
  * Let the guildmaster pass to pawn players items. T137.
  if %actor.vnum% == 122
    halt
  end
  * Stop them if they are not the appropriate class.
  if %actor.class% != thief
    return 0
    * Just send the block message to players.
    if %actor.is_pc%
      %send% %actor% The guard humiliates you, and blocks your way.
      %echoaround% %actor% The guard humiliates %actor.name%, and blocks %actor.hisher% way.
    end
  end
end
~
#137
Thief Guildmaster Steals - M122~
0 b 5
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Idea taken from cheesymud.com
* Thief guildmaster steals from players who idle in his guild. Then pawns the
* item in the shop downstairs so player has to buy their equipment back :-P
* Random trigs have no actors so pick one and make sure it is a player.
set actor %random.char%
if %actor.is_pc%
  * Pick the first item in the actors inventory.
  eval item %actor.inventory%
  * I'll be nice and not steal containers or mail.
  if %item.type% == CONTAINER || %item.vnum% <= 1
    halt
  end
  * If they have an item let the master thief steal it.
  eval item_to_steal %%actor.inventory(%item.vnum%)%%
  if %item_to_steal%
    * Give some hints that the guildmaster is not to be trusted.
    %echo% %self.name% examines %item.shortdesc%.
    wait 2 sec
    * Purge the actors object and load it to the master thief.
    eval stolen %item_to_steal.vnum%
    eval name %item_to_steal.name%
    %load% obj %stolen%
    %purge% %item_to_steal% 
    wait 2 sec
    * Lets go sell it to Morgan using its first keyword.
    down
    sell %name.car%
    wait 2 sec
    wink Morgan
    wait 2 sec
    up
  else
    emote grumbles unhappily.
  end
end
~
#138
Questmaster Greet - 3~
0 g 100
~
* By Rumble of The Builder Academy    builderacademy.net 9091 
* Part of a timed quest to kill a mob or find an object. Trigs 138-144. 
* A simple automated quest so only let players earn up to 50 questpoints.
if %actor.is_pc% && %actor.questpoints% < 50 
  wait 2 sec 
  * Check to see if they are not on a zone 1 quest. 
  if !%actor.varexists(on_quest_zone_1)% 
    say Welcome %actor.name%, are you interested in a simple quest? 
  else 
    *get the values from the player variable 
    extract day 1 %actor.on_quest_zone_1% 
    extract hour 2 %actor.on_quest_zone_1% 
    * Set  this value to the number of mud hours in a mud day 
    set HOURS_IN_DAY 24 
    * Compare the values to the current time 
    * (Your hours per day and days per year vary on each mud.) 
    * (You may also want to check for 'time.year') 
    eval current_ticks (%time.day%/%HOURS_IN_DAY%)+%time.hour% 
    eval start_ticks (%day%/%HOURS_IN_DAY%)+%hour% 
    *find out the time difference in ticks 
    eval ticks_passed %current_ticks%-%start_ticks% 
    *check if 10 ticks (~10 mins) have passed 
    if %ticks_passed% > 10 
      rdelete on_quest_zone_1 %actor.id% 
      say Welcome %actor.name%, are you interested in a simple quest? 
    else 
      say How is your quest going %actor.name%, do you have a quest token or the quest mobs head for me? 
    end 
  end 
end
~
#139
Questmaster Quest Assignment - 3~
0 d 1
*~
* By Rumble of The Builder Academy    builderacademy.net 9091 
* Part of a timed quest to kill a mob or find an object. Trigs 138-144. 
if %actor% == %self%
  halt
end
if %actor.varexists(on_quest_zone_1)% 
  *get the values from the player variable 
  extract day 1 %actor.on_quest_zone_1% 
  extract hour 2 %actor.on_quest_zone_1% 
  * Set  this value to the number of mud hours in a mud day 
  set HOURS_IN_DAY 24 
  * Compare the values to the current time 
  * (Your hours per day and days per year vary on each mud.) 
  * (You may also want to check for 'time.year') 
  eval current_ticks (%time.day%/%HOURS_IN_DAY%)+%time.hour% 
  eval start_ticks (%day%/%HOURS_IN_DAY%)+%hour% 
  *find out the time difference in ticks 
  eval ticks_passed %current_ticks%-%start_ticks% 
  *check if 10 ticks (~10 mins) have passed 
  if %ticks_passed% > 10 
    rdelete on_quest_zone_1 %actor.id% 
  else 
    halt 
  end 
end 
if %actor.questpoints% > 50 
  halt 
end 
* This loop goes through the entire string of words the actor says. .car is the
* word and .cdr is the remaining string. 
eval word %speech.car% 
eval rest %speech.cdr% 
while %word% 
  * Check to see if the word is yes or an abbreviation of yes. 
  if yes /= %word% 
    say Very well %actor.name%. Would you like to find an object or hunt a mobile? 
    halt 
  end 
  * Pick a room from 100 to 365. 
  eval loadroom 99 + %random.265%  
  if mobile /= %word% || hunt /= %word% 
    * Load the mob in the random room picked above. 
    %at% %loadroom% %load% m 15 
    say Go kill the quest mob and bring me its head %actor.name%. You only have 10 minutes! 
    * Load an object on the player that counts down from 10 minutes. 
    %load% obj 16 %actor% inv 
    %send% %actor% %self.name% gives you the quest timer. 
    %echoaround% %actor% %self.name% gives %actor.name% the quest timer. 
    set on_quest_zone_1 %time.day% %time.hour% 
    remote on_quest_zone_1 %actor.id% 
    halt 
  elseif object /= %word% || find /= %word% 
    say Go find the quest token and return it to me. You only have 10 minutes %actor.name%! 
    %load% o 15 
    %at% %loadroom% drop quest_token_zone_1 
    %load% obj 16 %actor% inv 
    %send% %actor% %self.name% gives you the quest timer. 
    %echoaround% %actor% %self.name% gives %actor.name% the quest timer. 
    set on_quest_zone_1 %time.day% %time.hour% 
    remote on_quest_zone_1 %actor.id% 
    halt 
  end 
  * End of the loop we need to take the next word in the string and save the 
  * remainder for the next pass. 
  set word %rest.car% 
  set rest %rest.cdr% 
done
~
#140
Quest Timer - 16~
1 c 7
l~
* By Rumble of The Builder Academy    tbamud.com 9091
* Part of a timed quest to kill a mob or find an object. Trigs 138-144.
* Let a player see how much time they have left.
if %cmd.mudcommand% == look && timer /= %arg%
  %send% %actor% You have %self.timer% minutes remaining.
  * Let the look timer command still go through so they look at it.
  return 0
else
  return 0
end
~
#141
Quest 10 min Purge - 15, 16, 17~
1 f 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Part of a timed quest to kill a mob or find an object. Trigs 138-144.
* Attached to quest objects 15-17. Purges itself 10 minutes after loading if 
* player does not finish the quest.
* Make sure timer is being carried by a player.
if %self.carried_by%
  set actor %self.carried_by%
  if %actor.is_pc%
    %send% %actor% Your quest time has run out. Try again.
    * Delete the on quest variable so they can try again.
    rdelete on_quest_zone_1 %actor.id%   
  end
end
* Purge the timer.
%purge% %self%
~
#142
Quest Timer Random - 16~
1 b 10
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Part of a timed quest to kill a mob or find an object. Trigs 138-144.
* If timer is being carried by a player, warn them every 2 minutes.
if %self.carried_by%
  set actor %self.carried_by%
  if %actor.is_pc%
    %send% %actor% You have %self.timer% minutes remaining.
  end
end
~
#143
Questmaster Receive - 3~
0 j 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Part of a timed quest to kill a mob or find an object. Trigs 138-144.
* Check if they are on the quest.
if !%actor.varexists(on_quest_zone_1)%
  say You are not even on a quest %actor.name%.
  * Stop the trig and prevent the item from being given to the mob.
  halt
end
*
wait 1 sec
* If they had the head or the token.
if %object.vnum% == 15 || %object.vnum% == 17
  rdelete on_quest_zone_1 %actor.id%   
  say Well done, %actor.name%.
  * Give them 50 gold and experience. Delete the on quest variable and purge.
  nop %actor.exp(50)%
  nop %actor.gold(50)%
  %purge% %object%
  * Reward them with 1 questpoint. Cheap I know but these quests are not hard.
  nop %actor.questpoints(1)%
  say you have earned 50 gold, 50 xp, 1 questpoint %actor.name%.
else
  say I don't want that!
  * Don't accept the object to prevent overloading the mob with junk.
  return 0.
end
~
#144
Quest Mob Loads Head - 15~
0 n 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Part of a timed quest to kill a mob or find an object. Trigs 138-144.
* This is a load instead of a death trig because I want the head to purge 10 
* minutes after loading.
%load% obj 17
~
#145
Dove - 193~
0 b 5
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Allows you to create multiple random actions.
* Numeric Arg of 5 fires about every 4 minutes. HELP RANDOM-TIMING
* %random.#% must match the highest txt# below.
eval number %random.5%
set txt1 pecks at your feet.
set txt2 coo's lightly.
set txt3 bobs its head as if to the beat of a drum.
set txt4 takes flight and lands a few feet away.
set txt5 deposits a liquidy black and white substance on your foot.
* set txt# .... add more here.
eval string %%txt%number%%%
emote %string%
~
#146
Apprentice healer - 201~
0 b 20
~
* By Rumble of The Builder Academy    tbamud.com 9091
* This is required because a random trig does not have an actor.
set actor %random.char%
* only continue if an actor is defined.
if %actor%
  * If they have lost more than half their hitpoints heal them.
  if %actor.hitp% < %actor.maxhitp% / 2 
    wait 1 sec
    tell %actor.name% You are injured, let me help.
    wait 2 sec
    %echoaround% %actor% %self.name% lays %self.hisher% hands on %actor.name%'s wounds and bows %actor.hisher% head in concentration.
    %send% %actor% %self.name% lays %self.hisher% hands on your wounds and bows %actor.hisher% head in concentration.
    dg_cast 'heal' %actor%
  end
end
~
#147
Black Magi Spell - 144~
0 k 10
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.level% > 10
  say you are a fool %actor.name%.
  dg_cast 'color spray' %actor%
end
~
#148
Mouse Emote - 194~
0 b 5
~
* By Rumble of The Builder Academy    tbamud.com 9091
eval number %random.4%
set txt1 scurries away quickly.
set txt2 stands up on its hind legs and sniffs the air.
set txt3 chews on some trash.
set txt4 squeaks and shakes some water and rain out of its fur.
eval string %%txt%number%%%
emote %string%
~
#149
Cat Emote - 139~
0 b 10
~
* By Rumble of The Builder Academy    tbamud.com 9091
eval number %random.4%
set txt1 hisses at you.
set txt2 purrs happily as it brushes up against your leg.
set txt3 plays with something it has already killed.
set txt4 swishes its tail back and forth as it eyes some prey.
eval string %%txt%number%%%
emote %string%
~
#150
Dog Emote - 192~
0 b 10
~
* By Rumble of The Builder Academy    tbamud.com 9091
eval number %random.4%
set txt1 sniffs at you in greeting.
set txt2 whimpers for some attention.
set txt3 growls menacingly at your feet.
set txt4 watches your every movement suspiciously.
eval string %%txt%number%%%
emote %string%
~
#151
Townsman Emote - 170~
0 b 10
~
* By Rumble of The Builder Academy    tbamud.com 9091
eval number %random.4%
set txt1 mumbles something about the weather.
set txt2 looks up at the sky warily.
set txt3 seems to have forgotten where he was headed.
set txt4 acknowledges you with a nod as he passes.
eval string %%txt%number%%%
emote %string%
~
#152
Angel Receives Treats - 207~
0 j 100
~
* By Rumble of The Builder Academy    tbamud.com 9091 
* A simple receive trig if you give the dog food she will eat it. If you give 
* her dog treats she will follow you. Everything else she drops. 
if %object.type% == FOOD 
  wait 1 sec 
  emote swallows %object.shortdesc% without even chewing. 
  wait 1 sec 
  emote looks up at %actor.name%, hoping for some more. 
  if %object.vnum% == 164 
    wait 1 sec 
    mfollow %actor%  
  end 
  if %object.vnum% == 172
    halt
  end
  %purge% %object% 
else 
  wait 1 s 
  drop %object.name.car% 
end 
~
#153
Angel Follows Masters Commands - 207~
0 d 1
*~
* By Rumble of The Builder Academy    tbamud.com 9091
if %self.master% == %actor%
  wait 1 sec
  switch %speech.car%
    case sit
      sit
      wait 3 sec
      stand
    break
    case speak
      emote barks sharply.
    break
    case down
      sit
      emote lies down.  
      wait 3 sec
      stand
    break
    case shake
      emote puts a paw up to be shook.
    break
    case kill
      if %speech.cdr.id% && %self.room% == %speech.cdr.room%
        emote growls at %speech.cdr.name% menacingly.
        mkill %speech.cdr%
      else
        emote looks around for someone to attack.  
      end
    break
    case rollover
      emote drops to the ground and rolls over a few times.
    break
    case walk
      emote stands up on her hind legs and staggers around in circles
    break
    case crawl
      emote drops down to the ground and crawls towards %actor.name%.
    break
    case jump
      emote jumps up into the air.
    break
    case chase
      if %speech.cdr% == your tail
        emote looks back at %self.hisher% tail angrily and attacks it, running in tight little circles.
      end
    break
    case highfive
      emote jumps up and gives %actor.name% a highfive.
    break
    default
      * nothing is going to happen
    break
  done
end
~
#154
rubber chicken squeeze - 172~
1 c 2
sq~
if squeeze /= %cmd% && chicken /= %arg%
  %send% %actor% You squeeze on the chicken and it squeaks annoying, how fun!
  %echoaround% %actor% %actor.name% squeezes the life out of %actor.hisher% rubber chicken making a racket with its squeaking.
  %asound% A loud and annoying squeaking sound can be heard close by.
else
  %send% %actor% What would you like to squeeze?
  return 0
end
~
#155
Check for treats - 207~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* My dog is looking if people have some beggin strips.
if %actor.has_item(164)%
  wait 1 sec
  %send% %actor% %self.name% sits down and stands up on %self.hisher% hind legs, then starts whining pitifully staring at you.
  %echoaround% %actor% %self.name% sits down and stands up on %self.hisher% hind legs, then starts whining pitifully staring at %actor.name%.
  * Or if they have her rubber chicken.
elseif %actor.has_item(172)%
  wait 1 sec
  emote sniffs %actor.name%.
  wait 1 sec
  growl %actor.name%
  %send% %actor% %self.name% tries to get at something you are carrying.
  %echoaround% %actor% %self.name% tries to get at something %actor.name% is carrying.
end
~
#156
Angel plays with chicken - 207~
0 b 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %self.has_item(172)%
  growl chicken
  squeeze chicken
end
~
#157
10 sided die roll - 173~
1 c 7
roll~
* By Rumble of The Builder Academy    tbamud.com 9091
if dice /= %arg% || die /= %arg%
  %send% %actor% You throw the ten sided die on the ground.
  %echoaround% %actor% %actor.name% throws %actor.hisher% ten sided die on the ground.
  set total %random.10%
  %echo% The die came up as [%total%].
else
  return 0
end
~
#158
Bell Toll - 101~
2 c 100
pull~
* By Rumble of The Builder Academy    tbamud.com 9091
if rope /= %arg%
  %echoaround% %actor% %actor.name% struggles as %actor.heshe% pulls on the rope.
  %send% %actor% You pull the rope putting all your weight into it. It slowly gives.
  %zoneecho% %self.vnum% The bell tolls.
else
  %echo% I don't see what you want to "pull". The rope perhaps?
end
~
#159
Cancer Stick Smoking - 176~
1 c 2
light~
* By Rumble of The Builder Academy    tbamud.com 9091
* put your objects alias here, /= will match abbreviations of it.
if cigarette /= %arg% || cancer /= %arg% || stick /= %arg%
  %send% %actor% You light up %self.shortdesc%.
  %echoaround% %actor% %actor.name% lights up a %self.shortdesc%.
  * use as many puffs and as much time between puffs as you want.
  while %puffs% < 4
    wait 10 sec
    %send% %actor% You take a puff off of %self.shortdesc%.
    %echoaround% %actor% %actor.name% takes a puff of smoke off of %actor.hisher% %self.shortdesc%.
    eval puffs %puffs% + 1
  done
  %send% %actor% You take a final puff and put the %self.shortdesc% out.
  %echoaround% %actor% %actor.name% takes a final puff and puts %actor.hisher% %self.shortdesc% out.
  %purge% %self%
else
  %send% %actor% What would you like to %cmd%?
end
~
#160
Puppy plays - 191~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.vnum% == 207
  wait 1 sec 
  emote growls playfully at %actor.name%, crouching down into a mock attack position.
elseif %actor.is_pc%
  wait 1 sec
  if %actor.dex% < 8
    %send% %actor% %self.name% runs under your feet causing you to fall flat on your face.
    %echoaround% %actor% %self.name% runs under %actor.name%'s feet and trips %actor.himher%.
  elseif %actor.cha% < 8
    %send% %actor% %self.name% sniffs your leg and begins to urinate on you.
    %echoaround% %actor% %self.name% sniffs %actor.name%'s leg and proceeds to urinate on %actor.himher%.
  end
end
~
#161
Annoying Kid - 117~
0 b 5
~
eval max %random.4%
set  txt[1] I know you are, but what am I?
set  txt[2] Does your parents know you are out in public?
set  txt[3] And I thought I knew ugly.
set  txt[4] I'm going to tell my father on you.
set  speech %%txt[%max%]%%
eval speech %speech%
say %speech%
~
#162
Picking Mushrooms~
2 c 100
pi~
* By Rumble of The Builder Academy    tbamud.com 9091
* Make sure the command matches MUD command pick, check for any abbrev of mushrooms.
if %cmd.mudcommand% == pick && mushrooms /= %arg%
  %load% obj 1300 %actor% inv
  %send% %actor% You pick a mushroom off the floor.
  %echoaround% %actor% %actor.name% Picks a mushroom off the floor.
else
  * return 0, otherwise players would not be able to pick locks in the same room.
  return 0
  %send% %actor% Pick What?
end
~
#163
Room Heals - 101~
2 b 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* This is required because a random trig does not have an actor.
set actor %random.char%
* only continue if an actor is defined.
if %actor.is_pc%
  * check if they are hurt.
  if %actor.hitp% < %actor.maxhitp% 
    * heal them their level in hitpoints.
    %damage% %actor% -%actor.level%
  end
end
~
#164
Beggin Strips - 164~
1 s 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
%send% %actor% You gag in disgust at the foul taste of the dog treats.
%echoaround% %actor% %actor.name% gags in disgust at the foul taste of the dog treats.
return 0
%purge% %self%
~
#165
Thief - 129, 183~
0 b 10
~
set actor %random.char%
if %actor%
  if %actor.is_pc% && %actor.gold%
    %send% %actor% You discover that %self.name% has %self.hisher% hands in your wallet.
    %echoaround% %actor% %self.name% tries to steal gold from %actor.name%.
    eval coins %actor.gold% * %random.10% / 100
    nop %actor.gold(-%coins%)%
    nop %self.gold(%coins%)%
  end
end
~
#166
Cast spells on Greet - M135~
0 g 10
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Cast a random spell on players that enter. But, only if they are not already
* affected and only 10 percent of the time.
switch %random.12%
  case 1
    if %actor.affect(blind)%
      dg_cast 'cure blind' %actor%
    end
  break
  case 2
    if !%actor.affect(invis)%
      dg_cast 'invisibility' %actor%
    end
  break
  case 3
    if !%actor.affect(det-align)%
      dg_cast 'detect align' %actor%
    end
  break
  case 4
    if !%actor.affect(det-magic)%
      dg_cast 'detect magic' %actor%
    end
  break
  case 5
    if !%actor.affect(sense-life)%
      dg_cast 'sense life' %actor%
    end
  break
  case 6
    if !%actor.affect(watwalk)%
      dg_cast 'waterwalk' %actor%
    end
  break
  case 7
    if !%actor.affect(sanct)%
      dg_cast 'sanctuary' %actor%
    end
  break
  case 8
    if %actor.affect(curse)%
      dg_cast 'remove curse' %actor%
    end
  break
  case 9
    if !%actor.affect(infra)%
      dg_cast 'infravision' %actor%
    end
  break
  case 10
    if %actor.affect(poison)%
      dg_cast 'cure poison' %actor%
    end
  break
  case 11
    if !%actor.affect(bless)%
      dg_cast 'bless' %actor%
    end
  break
  case 12
    if !%actor.affect(prot-evil)%
      dg_cast 'protection from evil' %actor%
    end
  break
  default
    say whoops!
  break
done
~
#167
Mob Questshop Example~
0 c 100
*~
* By Rumble of The Builder Academy    tbamud.com 9091
* A questshop that uses questpoints!
if %cmd.mudcommand% == list
  *
  %send% %actor%  ##   Available   Item                                Cost in Questpoints
  %send% %actor% -------------------------------------------------------------------------
  %send% %actor%   1)  Unlimited   War's Blood                                         100
  %send% %actor%   2)  Unlimited   shadow stealer                                      100
  %send% %actor%   3)  Unlimited   the staff of spellfire                              100
  *
elseif %cmd.mudcommand% == buy
  if War /= %arg% || Blood /= %arg% || %arg% == 1
    set quest_item 21
    set quest_item_cost 100
  elseif shadow /= %arg% || stealer /= %arg% || %arg% == 2
    set quest_item 22
    set quest_item_cost 100
  elseif staff /= %arg% || spellfire /= %arg% || %arg% == 3
    set quest_item 23
    set quest_item_cost 100
  else
    tell %actor.name% What would you like to buy?
    halt
  end
  *
  if %actor.questpoints% < %quest_item_cost% 
    tell %actor.name% You don't have enough questpoints for that.
  else
    %load% obj %quest_item% %actor% inv
    tell %actor.name% here you go.
    nop %actor.questpoints(-%quest_item_cost%)%
  end
elseif %cmd.mudcommand% == sell
  tell %actor.name% I don't want anything you have.
else
  return 0
end
~
#168
Questpoint Setter - 44~
1 c 1
questpoints~
* By Rumble of The Builder Academy    tbamud.com 9091
* Questpoint setter. For STAFF only! Make sure player has nohassle off.
* Make sure name matches a player, purge mobs or use 0.name if you have 
* troubles. 
* Usage: questpoints <player> <#>
if !%actor.is_pc% || %actor.level% < 32
  %send% %actor% Only human staff can use this.
else  
  set victim %arg.car%
  if %victim.is_pc%
    set questpoints %arg.cdr%
    remote questpoints %victim.id%
    %send% %actor% %arg.car%'s questpoints set to %arg.cdr%.
  else
    %send% %actor% Don't use it on mobs. Use 0.<name>!
    return 0
  end
end
~
#169
Quest Token check if player is on quest - 15~
1 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if !%actor.varexists(on_quest_zone_1)%
  %send% %actor% You are not on a quest, don't steal other peoples quest items!
  return 0
end
~
#170
Load Tapcode Paper - 97~
2 c 100
wa~
* By Rumble of The Builder Academy    tbamud.com 9091
if !%actor.has_item(83)% && %cmd.mudcommand% == wake
nop %actor.pos(sitting)%
  %send% %actor% As you slowly regain consciousness you hear a shuffling of feet outside the door. 
  wait 30 sec
  %echo% Two shadows pass in front of your cell. Glancing under the door you see a set of dirty manacled feet pause outside. A scrap of paper falls to the ground and is quickly kicked under the door. A grunt is heard as a set of booted feet soon follow pushing the captive that had stopped.
  %load% obj 83
else
  * If it doesn't match let the command continue.
  return 0
end
~
#171
Tapcode Say Stuff - 97-93~
2 b 10
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %self.vnum(97)%
  * tapcode SAY FREEDOM
  %echo% tap tap tap tap   tap tap tap
  wait 3 sec
  %echo% tap   tap
  wait 3 sec
  %echo% tap tap tap tap tap   tap tap tap tap
  wait 3 sec
  %echo% tap tap   tap
  wait 3 sec
  %echo% tap tap tap tap   tap tap
  wait 3 sec
  %echo% tap   tap tap tap tap tap
  wait 3 sec
  %echo% tap   tap tap tap tap tap
  wait 3 sec
  %echo% tap   tap tap tap tap
  wait 3 sec
  %echo% tap tap tap   tap tap tap tap
  wait 3 sec
  %echo% tap tap tap   tap tap
elseif %self.vnum(96)%
  * tapcode SAY INTEGRITY
  %echo% tap tap tap tap   tap tap tap
  wait 3 sec
  %echo% tap   tap
  wait 3 sec
  %echo% tap tap tap tap tap   tap tap tap tap
  wait 3 sec
  %echo% tap tap   tap tap tap tap
  wait 3 sec
  %echo% tap tap tap   tap tap tap
  wait 3 sec
  %echo% tap tap tap tap   tap tap tap tap
  wait 3 sec
  %echo% tap   tap tap tap tap tap
  wait 3 sec
  %echo% tap tap   tap tap
  wait 3 sec
  %echo% tap tap tap tap   tap tap
  wait 3 sec
  %echo% tap tap   tap tap tap tap
  wait 3 sec
  %echo% tap tap tap tap   tap tap tap tap
  wait 3 sec
  %echo% tap tap tap tap tap   tap tap tap tap
elseif %self.vnum(95)%
  * tapcode SAY WISDOM
  %echo% tap tap tap tap   tap tap tap
  wait 3 sec
  %echo% tap   tap
  wait 3 sec
  %echo% tap tap tap tap tap   tap tap tap tap
  wait 3 sec
  %echo% tap tap tap tap tap   tap tap
  wait 3 sec
  %echo% tap tap   tap tap tap tap
  wait 3 sec
  %echo% tap tap tap tap   tap tap tap
  wait 3 sec
  %echo% tap   tap tap tap tap
  wait 3 sec
  %echo% tap tap tap   tap tap tap tap
  wait 3 sec
  %echo% tap tap tap   tap tap
elseif %self.vnum(94)%
  * tapcode SAY PERSEVERENANCE
  %echo% tap tap tap tap   tap tap tap
  wait 3 sec
  %echo% tap   tap
  wait 3 sec
  %echo% tap tap tap tap tap   tap tap tap tap
  wait 3 sec
  %echo% tap tap tap   tap tap tap tap tap
  wait 3 sec
  %echo% tap   tap tap tap tap tap
  wait 3 sec
  %echo% tap tap tap tap   tap tap
  wait 3 sec
  %echo% tap tap tap tap   tap tap tap
  wait 3 sec
  %echo% tap   tap tap tap tap tap
  wait 3 sec
  %echo% tap tap tap tap tap   tap
  wait 3 sec
  %echo% tap   tap tap tap tap tap
  wait 3 sec
  %echo% tap tap tap tap   tap tap
  wait 3 sec
  %echo% tap   tap
  wait 3 sec
  %echo% tap tap tap   tap tap tap
  wait 3 sec
  %echo% tap   tap tap tap
  wait 3 sec
  %echo% tap   tap tap tap tap tap
end
~
#172
Prison Teleport 97-91~
2 d 1
*~
* By Rumble of The Builder Academy    tbamud.com 9091
* evaluate the first word.
eval word %speech.car%
* evaluate the rest of the speech string.
eval rest %speech.cdr%
* keep looping until there are no more words.
while %word%
  if %word% == freedom && %actor.room.vnum% == 97
    %send% %actor% You feel a strange shifting as you are teleported out of the cell, you hear a distant clanking as the shackles fall to the floor.
    %teleport% %actor% 96
  elseif %word% == integrity && %actor.room.vnum% == 96
    %teleport% %actor% 95
    %send% %actor% You feel a strange shifting as you are teleported out of the cell.
  elseif %word% == wisdom && %actor.room.vnum% == 95
    %teleport% %actor% 94
    %send% %actor% You feel a strange shifting as you are teleported out of the cell.
  elseif %word% == perseverance && %actor.room.vnum% == 94
    %teleport% %actor% 93
    %send% %actor% You feel a strange shifting as you are teleported out of the cell.
  end
  eval word %rest.car%
  eval rest %rest.cdr%
done
~
#173
Prison Commissar M31~
0 b 3
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Prison Commissar - M31 - T173
eval max %random.5%
set text[1] you Americans were nothing like the French, we could count on them to be reasonable.
set text[2] you Americans think you are so smart. We have figured out your fourth signal. Four beats or waves or taps or whatever you come up with means "wait."
set text[3] War has blighted this planet in all but 268 of the past 4000 years.
set text[4] We may not have freedom, but after 4000 years we have order, and we will settle for that.
set text[5] I do not get paid enough for such a despicable job. I don't suppose you would give me a little something in exchange for my help?
eval  speech %%text[%max%]%%
say %speech%
~
#174
Underground Spy M30 T174~
0 b 20
~
* By Rumble of The Builder Academy    tbamud.com 9091
if !%self.follower%
  eval max %random.4%
  set txt[1] Don't let the 'MAN' hold you back, join the rebellion. Follow me if you want to lead a better life.
  set txt[2] Be careful who you talk to, they may be part of the conspiracy. Follow me if you want to know the truth.
  set txt[3] Follow me if you want to join the resistance.
  set txt[4] Only through sacrifice can we know the truth. Follow me to be enlightened.
  set  speech %%txt[%max%]%%
  eval speech %speech%
  say %speech%
else
  say you have made the right choice.
  wait 3 sec
  %random.dir%
  wait 2 sec
  %random.dir%
  wait 2 sec
  %random.dir%
  wait 2 sec
  %random.dir%
  wait 2 sec
  say do you have anyone that knows you are in Sanctus or who may notice your absence?
  wait 5 sec
  %load% mob 32
  %echo% The hired muscle has arrived.
  %load% mob 32
  %echo% The hired muscle has arrived.
  %load% mob 32
  %echo% The hired muscle has arrived.
  %load% mob 32
  %echo% The hired muscle has arrived.
  wait 3 sec
  say lets get on with it. Don't bother putting up a fight.
  wait 3 sec
  %send% %self.follower% The spy and all of his hired muscle quickly circle you and wait for your exposed back before they attack. You catch movement out of the corner of your eye as you are clubbed from behind.
  %echoaround% %self.follower% %self.follower.name% is quickly surrounded by a group of thugs and clubbed unconscious. The underground spy pays the hired thugs and then drags %self.follower.name% away to the %random.dir%.
  nop %self.follower.pos(sleeping)%
  %teleport% %self.follower% 97
  %purge% %self%
end
~
#175
Chuck Norris - 34~
0 ab 12
~
* Chuck Norris Facts!
eval max %random.89%
set  text[1]   Chuck Norris does not sleep. He waits.
set  text[2]   If you can see Chuck Norris, he can see you. If you can't see Chuck Norris you may be only seconds away from death.
set  text[3]   Chuck Norris frequently donates blood to the Red Cross. Just never his own.
set  text[4]   The chief export of Chuck Norris is pain.
set  text[5]   Chuck Norris sold his soul to the devil for his rugged good looks and unparalleled martial arts ability. Shortly after the transaction was finalized, Chuck roundhouse kicked the devil in the face and took his soul back. The devil, who appreciates irony, couldn't stay mad and admitted he should have seen it coming. They now play poker every second Wednesday of the month.
set  text[6]   Chuck Norris is currently suing NBC, claiming Law and Order are trademarked names for his left and right legs.
set  text[7]   Chuck Norris doesn't read books. He stares them down until he gets the information he wants.
set  text[8]   The quickest way to a man's heart is with Chuck Norris's fist.
set  text[9]   Leading hand sanitizers claim they can kill 99.9 percent of germs. Chuck Norris can kill 100 percent of whatever the heck he wants.
set  text[10]  Chuck Norris thought up some of the funniest Chuck Norris facts ever, but he hasn't submitted them to the site because he doesn't believe in any form of submission.
set  text[11]  Chuck Norris once visited the Virgin Islands. They are now The Islands.
set  text[12]  Chuck Norris does not hunt because the word hunting implies the probability of failure. Chuck Norris goes killing.
set  text[13]  Chuck Norris' tears cure cancer. Too bad he has never cried.
set  text[14]  When the Boogeyman goes to sleep every night he checks his closet for Chuck Norris.
set  text[15]  Chuck Norris counted to infinity - twice.
set  text[16]  Chuck Norris puts the "laughter" in "manslaughter".
set  text[17]  Chuck Norris can speak braille.
set  text[18]  Chuck Norris was once on Celebrity Wheel of Fortune and was the first to spin. The next 29 minutes of the show consisted of everyone standing around awkwardly, waiting for the wheel to stop.
set  text[19]  Chuck Norris sleeps with a night light. Not because Chuck Norris is afraid of the dark, but the dark is afraid of Chuck Norris
set  text[20]  If Chuck Norris is late, time better slow the heck down.
set  text[21]  Superman owns a pair of Chuck Norris pajamas.
set  text[22]  If you try to introduce your mother to Chuck Norris, she'll introduce you to your biological father.
set  text[23]  Chuck Norris uses all seven letters in Scrabble... Every turn.
set  text[24]  When Chuck Norris sends in his taxes, he sends blank forms and includes only a picture of himself, crouched and ready to attack. Chuck Norris has not had to pay taxes ever.
set  text[25]  Chuck Norris died ten years ago, but the Grim Reaper can't get up the courage to tell him.
set  text[26]  Chuck Norris once survived a suicide bombing. He was the bomber.
set  text[27]  Chuck Norris does not know where you live, but he knows where you will die.
set  text[28]  Chuck Norris can divide by zero.
set  text[29]  Chuck Norris can slam revolving doors.
set  text[30]  Little kids enjoy lighting ants on fire with magnifying glasses. Chuck Norris enjoys lighting little kids on fire with ants. Scientists have yet to find out how this feat is achieved.
set  text[31]  If it looks like chicken, tastes like chicken, and feels like chicken but Chuck Norris says its beef, then it's beef.
set  text[32]  We all know the magic word is please. As in the sentence, "Please don't kill me." Too bad Chuck Norris doesn't believe in magic.
set  text[33]  Chuck Norris once went on Celebrity Jeopardy and answered, "Who is Chuck Norris?" to every question. It was the first and only time in Jeopardy history that a contestant answered every single question right.
set  text[34]  At birth, Chuck Norris came out feet first so he could roundhouse kick the doctor in the face. Nobody delivers Chuck Norris but Chuck Norris.
set  text[35]  Chuck Norris once ate three 72 oz. steaks in one hour. He spent the first 45 minutes having sex with his waitress.
set  text[36]  Chuck Norris owns the greatest Poker Face of all-time. It helped him win the 1983 World Series of Poker despite him holding just a Joker, a Get out of Jail Free Monopoloy card, a 2 of clubs, 7 of spades and a green #4 card from the game UNO.
set  text[37]  A Handicap parking sign does not signify that this spot is for handicapped people. It is actually in fact a warning, that the spot belongs to Chuck Norris and that you will be handicapped if you park there.
set  text[38]  Chuck Norris' sperm can penetrate 13 condoms, the birth control pill, a brick wall, and the 1975 Pittsburgh Steelers offensive line in order to impregnate a woman.
set  text[39]  Chuck Norris has to sort his laundry into three loads: darks, whites, and bloodstains.
set  text[40]  If at first you don't succeed, you are obviously not Chuck Norris.
set  text[41]  Geico saved 15%% by switching to Chuck Norris.
set  text[42]  If you see Chuck Norris crying he will grant you a wish, if your wish is dying.
set  text[43]  Pee Wee Herman got arrested for masturbating in public. The same day, Chuck Norris got an award for masturbating in public.
set  text[44]  They say that lightning never strikes the same place twice. Neither does Chuck Norris. He doesn't have to.
set  text[45]  Water boils faster when Chuck Norris watches it.
set  text[46]  Chuck Norris' cowboy boots are made from real cowboys.
set  text[47]  When Chuck Norris exercises, the machine gets stronger.
set  text[48]  Chuck Norris is allowed to talk about Fight Club.
set  text[49]  The only thing we have to fear is fear itself... The only thing fear has to fear is Chuck Norris.
set  text[50]  Chuck Norris clogs the toilet even when he pisses.
set  text[51]  A blind man once stepped on Chuck Norris' shoe. Chuck replied, "Don't you know who I am? I'm Chuck Norris!" The mere mention of his name cured the mans blindness. Sadly the first, last, and only thing this man ever saw, was a fatal roundhouse delivered by Chuck Norris.
set  text[52]  The most effective form of suicide known to man is to type "Chuck Norris" into Google and hit "I'm Feeling Lucky!".
set  text[53]  Chuck Norris never gets brain freeze. Slurpees know when to back off.
set  text[54]  Chuck Norris got in touch with his feminine side, and promptly got her pregnant.
set  text[55]  Chuck Norris refers to himself in fourth person.
set  text[56]  Whenever someone is constipated, doctors send them to Chuck Norris so he can scare the shit out of them.
set  text[57]  Chuck Norris is not hung like a horse... horses are hung like Chuck Norris
set  text[58]  Switzerland isn't really neutral. They just haven't figured out what side Chuck Norris is on yet.
set  text[59]  When Chuck Norris was in middle school, his English teacher assigned an essay: "What is Courage?" Chuck Norris received an "A+" for writing only the words "Chuck Norris" and promptly turning in the paper.
set  text[60]  Chuck Norris doesn't give Christmas presents. If you live to see Christmas, that is your Christmas present from Chuck.
set  text[61]  Chuck Norris ends every relationship with "Its not me, its you".
set  text[62]  Chuck Norris was sending an email one day, when he realized that it would be faster to run.
set  text[63]  When Chuck Norris laughs too hard while drinking milk, he accidently shits a cow.
set  text[64]  One time in an airport a guy accidently called Chuck Norris "Chick Norris". He explained it was an honest mistake and apologized profusely. Chuck accepted his apology and politely signed an autograph. Nine months later, the guy's wife gave birth to a bearded baby. The guy knew exactly what had happened, and blames nobody but himself.
set  text[65]  Chuck Norris doesn't understand why you should consult your doctor if your erection lasts for more than 4 hours. His erections have been known to last for up to 15 days.
set  text[66]  Chuck Norris has never had an alcohol problem. However, alcohol has had a Chuck Norris problem.
set  text[67]  Jesus owns and wears a bracelet that reads, "WWCND?" What would Chuck Norris Do?
set  text[68]  In order to survive a nuclear attack, you must remember to stop, drop, and be Chuck Norris.
set  text[69]  Similar to a Russian Nesting Doll, if you were to break Chuck Norris open you would find another Chuck Norris inside, only smaller and angrier.
set  text[70]  Chuck Norris' dog is trained to pick up his own poop because Chuck Norris will not take shit from anyone.
set  text[71]  Oxygen requires Chuck Norris to live.
set  text[72]  Chuck Norris doesn't have a bank account. He just tells the bank how much he needs.
set  text[73]  Chuck Norris frequently signs up for beginner karate classes, just so he can "accidentally" beat the shit out of little kids.
set  text[74]  Life is like a box of chocolates. You never know when Chuck Norris is going to kill you.
set  text[75]  Someone once tried to tell Chuck Norris that roundhouse kicks aren't the best way to kick someone. This has been recorded by historians as the worst mistake anyone has ever made.
set  text[76]  The phrase "Made by Chuck Norris" is imprinted beneath the surface of China.
set  text[77]  Microsoft has released a new Anti-virus removal tool called Chuck Norris. The tool dares the virus to enter the machine.
set  text[78]  Chuck Norris always gets blackjack. Even when he's playing poker.
set  text[79]  When the Incredible Hulk gets angry he transforms into Chuck Norris.
set  text[80]  When Chuck Norris answers the phone, he just says "Go". This is not permission for you to begin speaking, it is your cue to start running for your life.
set  text[81]  Chuck Norris only uses one chopstick.
set  text[82]  Chuck Norris does not leave messages. Chuck Norris leaves warnings.
set  text[83]  Filming on location for Walker: Texas Ranger, Chuck Norris brought a stillborn baby lamb back to life by giving it a prolonged beard rub. Shortly after the farm animal sprang back to life and a crowd had gathered, Chuck Norris roundhouse kicked the animal, breaking its neck, to remind the crew once more that Chuck giveth, and the good Chuck, he taketh away.
set  text[84]  When you open a can of whoop-ass, Chuck Norris jumps out.
set  text[85]  Chuck Norris once had a near death experience. Needless to say, Death now refuses to come near him.
set  text[86]  People say the truth hurts, but it hurts a hell of a lot more when it comes from Chuck Norris.
set  text[87]  If you come home to find Chuck Norris doing your wife, it's probably best to go fetch a glass of water and stand there in case Chuck gets thirsty. There ain't no future in any other course of action.
set  text[88]  Chuck Norris can open beer cans with his teeth. He still prefers to use other people's teeth, though.
set  text[89]  One time while sparring with Wolverine, Chuck Norris accidentally lost his left testicle. You might be familiar with it to this very day by its technical term: Jupiter.
set  speech %%text[%max%]%%
eval speech %speech%
%echo% %speech%
~
#176
Teleporter Recall - O82~
1 c 7
re~
* By Rumble of The Builder Academy    tbamud.com 9091
if %cmd% == recall
  eval teleporter_return_room %actor.room.vnum%
  remote  teleporter_return_room %actor.id%
  %send% %actor% You recall to safety.
  %echoaround% %actor% %actor.name% recalls.
  %teleport% %actor% 3001
  %force% %actor% look
  %echoaround% %actor% %actor.name% appears in the room.
elseif %cmd% == return
  %send% %actor% You return to your previous location.
  %echoaround% %actor% %actor.name% teleports out of the room.
  %teleport% %actor% %actor.teleporter_return_room%
  %force% %actor% look
  %echoaround% %actor% %actor.name% appears in the room.
else
  return 0
end
~
#177
Kick Me Sign - O197~
1 b 10
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Only players can activate this trig.
if %self.worn_by%
  set actor %self.worn_by%
  set room_var %actor.room%
  wait 3 s
  set kickers %room_var.people%
  * Now loop through everyone in the room.
  while %kickers%
    * Set the next target before kicking.
    set next_kicker %kickers.next_in_room%
    if %kickers% != %actor%
      %send% %kickers% You can't resist the urge after seeing the sign on %actor.name%'s back. You kick %actor.himher%
      %send% %actor% %kickers.name% plants %kickers.hisher% firmly in your backside for no apparent reason.
      %echoaround% %actor% %kickers.name% kicks %actor.name% squarely in the behind after seeing the "Kick Me" sign %actor.heshe% is wearing.
      * Damage them... 2 hitpoints. To heal use -#.
      %damage% %actor% 2
    end  * Set the next target.
    set kickers %next_kicker%
    wait 1 s
    * Loop back.
  done
end
~
#178
Homer's Advice M33~
0 b 10
~
eval max %random.41%
set  text[1]  When someone tells you your butt is on fire, you should take them at their word.
set  text[2]  There is no such thing as a bad doughnut.
set  text[3]  Kids are like monkeys, only louder.
set  text[4]  If you want results, press the red button. The rest are useless.
set  text[5]  You should just name your third kid Baby. Trust me -- it'll save you a lot of hassle.
set  text[6]  You can have many different jobs and still be lazy.
set  text[7]  I enjoy the great taste of Duff. Yes, Duff is the only beer for me. Smooth, creamy Duff . . . zzzzzzzzzzzzz.
set  text[8]  You can get free stuff if you mention a product in a magazine interview. Like Chips Ahoy! cookies.
set  text[9]  You may think it's easier to de-ice your windshield with a flamethrower, but there are repercussions. Serious repercussions. 
set  text[10] There are some things that just aren't meant to be eaten.
set  text[11] The intelligent man wins his battles with pointed words. I'm sorry -- I meant sticks. Pointed sticks.
set  text[12] There are way too many numbers. The world would be a better place if we lost half of them -- starting with 8. I've always hated 8.
set  text[13] If I had a dollar for every time I heard "My God! He's covered in some sort of goo," I'd be a rich man.
set  text[14] Be generous in the bedroom -- share your sandwich.
set  text[15] I've climbed the highest mountains . . . fallen down the deepest valleys . . . I've been to Japan and Africa . . . and I've even gone into space. But I'd trade it all for a piece of candy right now.
set  text[16] Every creature on God's earth has a right to exist. Except for that damn ruby-throated South American warbler.
set  text[17] I don't need a surgeon telling me how to operate on myself.
set  text[18] Sometimes I think there's no reason to get out of bed . . . then I feel wet, and I realize there is.
set  text[19] Let me just say, Winnie the Pooh getting his head caught in a honey pot? It's not funny. It can really happen.
set  text[20] Even though it is awesome and powerful, I don't take no guff from the ocean.
set  text[21] I never ate an animal I didn't like.
set  text[22] A fool and his money are soon parted. I would pay anyone a lot of money to explain that to me.
set  text[23] Give a man a fish and he'll eat for a day. Teach a man to fish and he'll get a hook caught on his eyelid or something.
set  text[24] I made a deal with myself ten years ago . . . and got ripped off.
set  text[25] Never leave your car keys in a reactor core.
set  text[26] Always trust your first instinct -- unless it tells you to use your life savings to develop a Destructo Ray.
set  text[27] When you borrow something from your neighbor, always do it under the cover of darkness.
set  text[28] If a spaceship landed and aliens took me back to their planet and made me their leader, and I got to spend the rest of my life eating doughnuts and watching alien dancing girls and ruling with a swift and merciless hand? That would be sweet.
set  text[29] I may not be the richest man on earth. Or the smartest. Or the handsomest.
set  text[30] Never throw a butcher knife in anger.
set  text[31] The office is no place for off-color remarks or offensive jokes. That's why I never go there.
set  text[32] My favorite color is chocolate.
set  text[33] Always feel with your heart, although it's better with your hands.
set  text[34] The hardest thing I've had to face as a father was burying my own child. He climbed back out, but it still hurts.
set  text[35] If doctors are so right, why am I still alive?
set  text[36] I'm not afraid to say the word racism, or the words doormat and bee stinger.
set  text[37] Always have plenty of clean white shirts and blue pants.
set  text[38] When that guy turned water into wine, he obviously wasn't thinking of us Duff drinkers.
set  text[39] I love natural disasters because we're allowed to get out of work.
set  text[40] When I'm dead, I'm going to sleep. Oh, man, am I going to sleep.
set  text[41] What kind of fool would leave a pie on a windowsill, anyway?
set  speech %%text[%max%]%%
eval speech %speech%
say %speech%
~
#179
Streetsweeper Ramblings M161~
0 b 10
~
eval max %random.33%
set  text[1]  They told me I was gullible.... and I believed them.
set  text[2]  Teach a child to be polite and courteous in the home and, when he grows up, he'll never be able to edge his car onto a freeway.
set  text[3]  One nice thing about egotists... they don't talk about other people.
set  text[4]  How can there be self-help "groups"?
set  text[5]  Is Marx's tomb a communist plot?
set  text[6]  Stupid litterbugs.
set  text[7]  I had amnesia once -- maybe twice.
set  text[8]  This isn't an office. It's hell with fluorescent lighting.
set  text[9]  I pretend to work. They pretend to pay me.
set  text[10] A cubicle is just a padded cell without a door.
set  text[11] Can I trade this job for what's behind door #1?
set  text[12] How about never? Is never good for you?
set  text[13] I have plenty of talent and vision. I just don't care.
set  text[14] I'll try to be nicer if you'll try to be smarter.
set  text[15] I'm out of my mind but feel free to leave a message.
set  text[16] No problem is so large or so difficult that it can't be blamed on someone else.
set  text[17] Try a little kindness. As little as possible.
set  text[18] If at first you don't succeed try management.
set  text[19] Ahhh. I see the screw-up fairy has visited us again.
set  text[20] Not a morning person doesn't even begin to describe it.
set  text[21] 80 percent of success is just showing up.
set  text[22] It's lonely at the top but you eat better.
set  text[23] When I want your opinion, I'll beat it out of you.
set  text[24] Make it idiot proof and someone will make a better idiot.
set  text[25] The buck doesn't even slow down here.
set  text[26] Welcome to the Department of Redundancy Department.
set  text[27] Another deadline, another miracle.
set  text[28] Always keep your words soft and sweet, just in case you have to eat them.
set  text[29] Always read stuff that will make you look good if you die in the middle of it.
set  text[30] If you can't be kind, at least have the decency to be vague.
set  text[31] If you lend someone $20 and never see them again, it was probably worth it.
set  text[32] Never buy a car you can't push.
set  text[33] Since it's the early worm that gets eaten by the bird, sleep late.
set  speech %%text[%max%]%%
eval speech %speech%
say %speech%
~
#180
Shackle Lock Pick O89~
2 c 100
pi~
if %cmd.mudcommand% == pick && (shackles /= %arg% || lock /= %arg%) && %actor.has_item(85)%
  %send% %actor% You attempt to pick the simple lock with the pick and are surprised to hear a click as the lock quickly releases.
  %echoaround% %actor% %actor.name% quickly picks the lock to the shackles around %actor.hisher% ankles.
  %load% obj 86
  %purge% %self%
else
  %send% %actor% Pick What?
end
~
#181
Epictetus M29~
0 b 5
~
* Epictetus - M29 - T181 By Rumble
eval max %random.26%
set txt[1] the judge will do some things to you which are thought to be terrifying, but how can he stop you from taking the punishment he threatened?
set txt[2] what are the benefits of a stoic life? It is an ancient and honorable package of advice on how to stay out of the clutches of those who are trying to get you on the hook, trying to give you a feeling of obligation, trying to get moral leverage.
set txt[3] There can be no such thing as being the "victim" of another. You can only be a "victim" of yourself.
set txt[4] Show me a man who though sick is happy, who though in danger is happy, who though in prison is happy, and I'll show you a Stoic.
set txt[5] remember you are an actor in a drama of such sort as the Author chooses - if short, then in a short one. If long, then in a long one. If it be his pleasure that you should enact a poor man, or a cripple, or a ruler, see that you act it well. 
set txt[6] Things that are not within our own power, not without our Will, can by no means be either good or evil.
set txt[7] For it is better to die of hunger, exempt from fear and guilt, than to live in affluence with perturbation.
set txt[8] Lameness is an impediment to the leg, but not to the Will. Say this to yourself with regard to everything that happens. For you will find such things to be an impediment to something else, but not truly to yourself.
set txt[9] Look not for any greater harm than this: destroying the trustworthy, self-respecting well-behaved man within you.
set txt[10] Fear was not something that came out of the shadows of the night and enveloped you. He charged you with the total responsibility of starting it, stopping it, controlling it.
set txt[11] For it is within you, that both your destruction and deliverance lie.
set txt[12] Tranquility, fearlessness, and freedom. You can have these only if you are honest and take responsibility for your own actions. You've got to get it straight! You are in charge of you.
set txt[13] nobody can harm you without your permission. There can be no such thing as a victim, you can only be a victim of yourself.
set txt[14] Where, then, does the great evil and the great good lie in man? In the attitude of his will, and if that element stands firm and neither his self respect, nor his faithfulness, nor his intelligence be destroyed, then the man is also preserved.
set txt[15] When a man who has set his will neither on dying nor upon living at any cost, comes into the presence of the tyrant, what is there to prevent him from being without fear? Nothing.
set txt[16] it is better that man not deserve anything he does not control. Otherwise, he will go after what is not his, and this is the start of crime, wars, you name it.
set txt[17] every man bears the exclusive responsibility for his own good and evil. This makes it impossible for one person to do the wrong and make another, the innocent, suffer.
set txt[18] No one comes to his fall because of another's deed. No one is evil without loss or damage. No man can do wrong with impunity.
set txt[19] There can be no such thing as a victim, you can only be a victim of yourself.
set txt[20] A man's master is he who is able to confer or remove what that man seeks or shuns. Whoever then would be free, let him wish nothing, let him decline nothing, which depends on others, else he must necessarily be a slave.
set txt[21] You must acquire a constancy of character that will make it impossible for another to do you wrong.
set txt[22] Who is the invincible man? He who cannot be dismayed by any happening beyond his control.
set txt[23] To have your heart set on something you do not control is to invite slavery, for he who does control it, knowing of your hunger for it, can make you perform like a monkey on a string.
set txt[24] Men are disturbed not by things, but by the view that they take of them. Do not be concerned with things which are beyond your power. Demand not that events should happen as you wish, but wish them to happen as they do happen and you will get on well.
set txt[25] Lameness is an impediment to the body but not to the will.
set txt[26] We have devised a series of operating signals. The third means "repeat," just make three taps, waves, or whatever you can do.
set  speech %%txt[%max%]%%
eval speech %speech%
say %speech%
~
#182
Clausewitz M28~
0 b 5
~
* Clausewitz - M28 - T182 By Rumble
eval max %random.15%
set txt[1] it is not only in the loss of men, horses, and guns but in order, courage, confidence, cohesion and plan which come into consideration whether the engagement can still be continued or not. It is principally the moral forces which decide here.
set txt[2] failures of management and engineering are tactical shortcomings that can be fixed but that failures of leadership's nerve and character are terminal, catastrophic.
set txt[3] War is a special profession, however general its relation may be and even if all the male population of a country capable of bearing arms were able to practice it, war would still continue to be different and separate from any other activity.
set txt[4] The purpose of power is to permit moral ideas to take root.
set txt[5] Rights incur obligations.
set txt[6] In war, the moral is to the physical as three is to one.
set txt[7] Education is what's left over after you've forgotten all the facts you learned.
set txt[8] We have devised a series of operating signals. The first one says "no," "danger," "stop," or any connotation of the negative. For this use any one signal - a single thump, a single noise, a single flash, a single wave.
set txt[9] There are advantages to a commander when his troops know he cannot be contacted. They cannot ask for relief.
set txt[10] Those who expect to reap the blessing of freedom must, like men, undergo the fatigue of supporting it.
set txt[11] Sentiment rules the world, and he who fails to take that into account can never hope to lead.
set txt[12] The nation that will insist on drawing a broad line of demarcation between the fighting man and the thinking man is liable to find its fighting done by fools and its thinking done by cowards.
set txt[13] Don't just sit there and pray; get out and do something to better the World.
set txt[14] The educated man, particularly the educated leader, copes with the fact that life is not fair. The problem for education is not to teach people how to deal with success but how to deal with failure.
set txt[15] The name of the game in war is to break the enemy's will.
set  speech %%txt[%max%]%%
eval speech %speech%
say %speech%
~
#183
Socrates - 17~
0 b 1
~
* Socrates - M17 - T183 By Rumble
eval max %random.14%
set txt[1] Let him that would move the world, first move himself. 
set txt[2] Employ your time in improving yourself by other men's writings, so that you shall gain easily what others have labored hard for. 
set txt[3] No evil can befall a good man
set txt[4] You alone are in possession of the fundamental freedom of shaping your own attitude about what is going on.
set txt[5] He is richest who is content with the least, for content is the wealth of nature. 
set txt[6] And in knowing that you know nothing, that makes you the smartest of all. 
set txt[7] To find yourself, think for yourself 
set txt[8] My belief is that to have no wants is divine 
set txt[9] I know nothing except the fact of my ignorance 
set txt[10] By all means get married, If you get a good wife you'll become happy; If you get a bad one, you'll become a philosopher 
set txt[11] As for me, all I know is that I know nothing. 
set txt[12] We have devised a series of operating signals. The second one says "yes," "go," "concur," "execute," "good." For this we use any two of anything - the most efficient signal except for the single beat "no" signal.
set txt[13] Honor is often what remains after faith, love, and hope are lost.
set txt[14] It is the wise leader who comes to the conclusion that he can't be had if he can't be made to feel guilty.
set  speech %%txt[%max%]%%
eval speech %speech%
say %speech%
~
#184
Plato - 21~
0 b 3
~
* Plato - M21 - T184 By Rumble
eval max %random.22%
set txt[1] Wise men speak because they have something to say; Fools because they have to say something
set txt[2] Those who are too smart to engage in politics are punished by being governed by those who are dumber. 
set txt[3] This I know - that I know nothing. 
set txt[4] They do certainly give very strange, and newfangled, names to diseases. 
set txt[5] The measure of a man is what he does with power. 
set txt[6] The greatest wealth is to live content with little. 
set txt[7] The man who makes everything that leads to happiness depend upon himself, and not upon other men, has adopted the very best plan for living happily. 
set txt[8] For a man to conquer himself is the first and noblest of all victories.
set txt[9] And now I depart hence condemned by you to suffer the penalty of death, and they, too, go their ways condemned by the truth to suffer the penalty of villainy and wrong; and I must abide by my award - let them abide by theirs. 
set txt[10] Be kind, for everyone you meet is fighting a harder battle. 
set txt[11] Necessity, who is the mother of invention. 
set txt[12] You can discover more about a person in an hour of play than in a year of conversation. 
set txt[13] When men speak ill of you, live so as nobody may believe them. 
set txt[14] Good people do not need laws to tell them to act responsibly, while bad people will find a way around the laws. 
set txt[15] One of the penalties for refusing to participate in politics is that you end up being governed by your inferiors.
set txt[16] The life which is unexamined is not worth living.
set txt[17] Death is not the worst than can happen to men.
set txt[18] Never discourage anyone...who continually makes progress, no matter how slow.
set txt[19] Ignorance, the root and the stem of every evil. 
set txt[20] No human thing is of serious importance. 
set txt[21] Courage is simply endurance of the soul. 
set txt[22] Courage must be exercised in the presence of fear. 
set  speech %%txt[%max%]%%
eval speech %speech%
say %speech%
~
#185
Aristotle - 22~
0 b 10
~
* Aristotle - M22 - T185 By Rumble
eval max %random.26%
set txt[1] education is an ornament in prosperity and a refuge in adversity.
set txt[2] a good man with a flaw who had come to an unjustified bad end.
set txt[3] All paid jobs absorb and degrade the mind.
set txt[4] Education is the best provision for the journey to old age.
set txt[5] To perceive is to suffer.
set txt[6] The gods too are fond of a joke.
set txt[7] We are what we repeatedly do.
set txt[8] Misfortune shows those who are not really friends.
set txt[9] Hope is a waking dream.
set txt[10] Dignity consists not in possessing honors, but in the consciousness that we deserve them.
set txt[11] It is the mark of an educated mind to be able to entertain a thought without accepting it.
set txt[12] Man perfected by society is the best of all animals; he is the most terrible of all when he lives without law, and without justice.
set txt[13] I have gained this by philosophy: that I do without being commanded what others do only from fear of the law.
set txt[14] It is possible to fail in many ways...while to succeed is possible only in one way.
set txt[15] In poverty and other misfortunes of life, true friends are a sure refuge. The young they keep out of mischief; to the old they are a comfort and aid in their weakness, and those in the prime of life they incite to noble deeds. 
set txt[16] Happiness is the meaning and the purpose of life, the whole aim and end of human existence. 
set txt[17] He who is to be a good ruler must have first been ruled. 
set txt[18] There was never a genius without a tincture of madness. 
set txt[19] Those who educate children well are more to be honored than parents, for these only gave life, those the art of living well. 
set txt[20] Those that know, do. Those that understand, teach. 
set txt[21] I count him braver who overcomes his desires than him who conquers his enemies; for the hardest victory is over self. 
set txt[22] The unfortunate need people who will be kind to them; the prosperous need people to be kind to. 
set txt[23] Youth is easily deceived, because it is quick to hope. 
set txt[24] Teaching is the highest form of understanding. 
set txt[25] Excellence is an art won by training and habituation. We do not act rightly because we have virtue or excellence, but we rather have those because we have acted rightly. We are what we repeatedly do. Excellence, then, is not an act but a habit
set txt[26] Courage is a man's ability to handle fear. 
set  speech %%txt[%max%]%%
eval speech %speech%
say %speech%
~
#186
Confucius - 23~
0 b 10
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Confucius - M23 - T186 By Rumble
set max %random.27%
set txt[1] Before you embark on a journey of revenge, dig two graves. 
set txt[2] Everything has its beauty but not everyone sees it. 
set txt[3] Forget injuries, never forget kindnesses. 
set txt[4] It does not matter how slowly you go so long as you do not stop. 
set txt[5] Respect yourself and others will respect you. 
set txt[6] Study the past if you would define the future. 
set txt[7] The superior man, when resting in safety, does not forget that danger may come. When in a state of security he does not forget the possibility of ruin. When all is orderly, he does not forget that disorder may come.
set txt[8] What the superior man seeks is in himself; what the small man seeks is in others. 
set txt[9] Wheresoever you go, go with all your heart. 
set txt[10] By nature, men are nearly alike; by practice, they get to be wide apart. 
set txt[11] A man who has committed a mistake and doesn't correct it, is committing another mistake. 
set txt[12] If you know, to recognize that you know, If you don't know, to realize that you don't know: That is knowledge. 
set txt[13] Never hesitate to ask a lesser person. 
set txt[14] People with virtue must speak out; People who speak are not all virtuous. 
set txt[15] What you do not wish upon yourself, extend not to others. 
set txt[16] We take great pains to persuade other that we are happy than in endeavoring to think so ourselves. 
set txt[17] Better a diamond with a flaw than a pebble without. 
set txt[18] The people may be made to follow a path of action, but they may not be made to understand it. 
set txt[19] The superior man is modest in his speech, but exceeds in his actions. 
set txt[20] The superior man...does not set his mind either for anything, or against anything; what is right he will follow. 
set txt[21] To go beyond is as wrong as to fall short. 
set txt[22] Virtue is not left to stand alone. He who practices it will have neighbors. 
set txt[23] What the superior man seeks is in himself. What the mean man seeks is in others. 
set txt[24] What you do not want done to yourself, do not do to others. 
set txt[25] When you know a thing, to hold that you know it; and when you do not know a thing, to allow that you do not know it - this is knowledge. 
set txt[26] In any prisoner situation when you are communicating with a fellow prisoner be sure to agree about a danger signal first. Second make a cover story in case you are caught, and third, you need to decide on a backup communication system.
set txt[27] The superior man understands what is right; the inferior man understands what is accepted by a majority. 
eval  speech %%txt[%max%]%%
say %speech%
~
#187
James Stockdale - 27~
0 b 10
~
* James Stockdale - M27 - T187 By Rumble
eval max %random.43%
set txt[1] Francis Bacon once said, "Adversity doth best induce virtue... while luxury doth best induce vice."
set txt[2] to quote Glenn Gray, "Numberless soldiers have died, more or less willingly, not for country or honor or religious faith or for any other abstract good, but because they realized that by fleeing their posts and rescuing themselves, they would expose their companions to greater danger. Such loyalty to the group is the essence of fighting morale."
set txt[3] Glenn Gray said, "Nothing is clearer than that men can act contrary to the alleged basic instinct of self-preservation and against all motives of self-interest and egoism. Were it not so, the history of our civilization would be completely different than what it has been."
set txt[4] At its best, citizenship finds an equilibrium between two essential ingredients-that of rights and that of duties. When the idea of citizenship is losing its grip, one or the other of these elements becomes eroded. Either freedom is on the losing end, or the sense of duty, of obligation, goes down the drain.
set txt[5] Ariel and Will Durant said it best, "Freedom and equality are sworn enemies, when one prevails the other dies."
set txt[6] As a people we have become soft. Stress isn't a bad thing. It is actually good for you, particularly the kind that comes from physical exercise.
set txt[7] The Durants also said, "What determines whether the challenge of history will or will not be met depends upon the presence or absence of creative individuals with a clarity of mind and energy of will, capable of effective responses to new situations.
set txt[8] once one learns to accomodate the shocks of a stressful existence, his adrenalin, will power, and imagination are going to start churning to provide the maximum performance of the human mind.
set txt[9] would a good soldier fighting a tough battle stop to say to himself how unhappy he is?
set txt[10] Remember Herman Melville? " In time of peril, like the needle to the lodestone, obedience, irrespective of rank, generally flies to him who is best fitted to command.
set txt[11] There is no evidence that the way of the World assures the punishment of evil or the reward of virtue.
set txt[12] Humans seem to have an inborn need to believe that virtue will be rewarded and evil punished. Often, when they come face to face with the fact that this is not always so, they are crushed.
set txt[13] from the Book of Ecclesiastes, "I returned and saw that the race is not always to the swift nor the battle to the strong, neither yet bread to the wise nor riches to men of understanding, nor favors to men of skill, but time and chance happeneth to them all."
set txt[14] the key to our future leaders' merit may not be "hanging in there" when the light at the end of the tunnel is expected. It will be their performance when it looks like the light will never show up."
set txt[15] Mark Van Doren was right, "Being an educated person means that given the necessity [after doomsday, so to speak], you could refound your own civilization."
set txt[16] Captain McWhirr said "to go skimming over the years of existence to sink gently into a placid grave, ignorant of life to the last, without ever having been made to see all it may contain of perfidy, of violence, of terror."
set txt[17] Harry Truman said, "Men make history, and not the other way around."
set txt[18] Sir Arthur Conan Doyle wrote, "When one tries to rise above nature, one is liable to fall below it. The highest type of man may revert to the animal if he leaves the straight road of destiny. Consider, that the materialists, the sensualists, the worldy would all proplong their worthless lives. The spiritual would now avoid the call to something higher. It would be the survival of the least fit. What sort of cesspool may not our poor world become?
set txt[19] Remember when you are strung up by the ropes that they can get out of you only what they know you know.
set txt[20] a hero is a man who will not accept the status quo if it does not meet his standards.
set txt[21] any officer worth his salt, on detached duty and out of communication, has to have the right to sensibly modify the law, issue rules that fit the conditions, and be prepared to defend himself in court.
set txt[22] my experience is a perfect example of the hermetic - the alchemical transformation that may occur when a human being is subjected to intense pressure within a crucible of suffering or confinement.
set txt[23] Dostoyevsky was a wise man, "You see, gentlemen, reason is an excellent thing, there's no disputing that, but reason is nothing but reason and satisfies only the rational side of man's nature, while will is a manifestation of the whole life, that is, of the whole human life, including reason and all the impulses."
set txt[24] I once read from Nichmachean Ethics: "There are some instances in which such actions elicit forgiveness rather than praise, for example, when a man acts improperly under a strain greater than human nature can bear, and which no one could endure. Yet there are, perhaps, also acts which no man can possibly be compelled to do, but rather than do them he should accept the most terrible sufferings and death."
set txt[25] Wittgenstein is know for, "If it comes easy, it ain't worth a damn."
set txt[26] Fr. Marius recalls someone remarking, "the important thing is not what they've made of you, but what you made of what they've made of you."
set txt[27] It was only when I lay there on the rotting prison straw that I sensed within myself the first stirrings of good. Gradually it was disclosed to me that the line separating good and evil passes not between states nor between classes nor between political parties, but right through every human heart, through all human hearts. And that is why I turn back to the years of my imprisonment and say, "Bless you, prison, for having been a part of my life."
set txt[28] my rules are: BACK US: Don't Bow in public, stay off the Air, admit no Crimes, never Kiss them goodbye, Unity over Self. Never negotiate for yourself but only for us all. Not less than significant pain should cause you to submit.
set txt[29] The lecture-room of the philosopher is a hospital, students ought not to walk out of it in pleasure, but in pain. 
set txt[30] I knew I could contain material - so long as they didn't know I knew it.
set txt[31] Opportunities don't roll in as a saturating fog bank. They come as incidents. And you need spontaneity and instinct to capture them.
set txt[32] Society's main objective is to instill virtue in its citizenry and that specific laws are a secondary concern.
set txt[33] in prison I have concluded that the pincers of fear and guilt are the destroyers of men. Nothing else.
set txt[34] The challenge of education is not to prepare people for success but to prepare them for failure. I think that it's in hardship and failure that the heroes and the bums really get sorted out.
set txt[35] Solzhenitsyn once commented, "If only there were evil people somewhere insidiously committing evil deeds, and it were necessary only to separate them from the rest of us and destroy them. But the line dividing good and evil cuts through the heart of every human being, and who is willing to destroy a piece of his own heart?"
set txt[36] Edward Hidalgo once said, "Avoiding failure is not success." How right he was.
set txt[37] Clemenceau believed that, "War is too important to be left to the general's."
set txt[38] I once asked Will Durant what he thought of American foreign policy. He told me, "I think we're all mixed up. We seem to be working on the assumption that if we're nice to other people they'll be nice to us. I can tell you that in the last 4000 years there's practically no evidence to support that view."
set txt[39] Alfred Thayer Mahan believed that, "the purpose of power is to permit moral ideas to take root."
set txt[39] The Durants said that "culture is a thin and fragile veneer that superimposes itself on mankind."
set txt[40] The ones where are in trouble in here are the high school graduates who had enough sense to pick up the innuendo, and yet not enough education to accommodate it properly. A little knowledge is a dangerous thing.
set txt[41] Prof Hans Morgethau once postulated, "in the world of the intellectual, ideas meet with ideas, and anything goes that is presented cleverly and with assurance. But in the practical world, ideas meet with facts, facts which make mincemeat of wrong ideas and throw the pieces in the ashcan of history."
set txt[42] The greatest weapon our interrogators have against us is our own shame. They know how to manipulate it, our only protection is a clear conscience.
set txt[42] We seem to have burdened ourselves with a national self-consciousness that has become a self-inflicted wound of guilt, a national shame at being strong.
set txt[43] No one had to remind our founding fathers of the cost of freedom. 56 of them knowingly laid their lives, liberty and honor on the line when they signed that Declaration of Independence. And they paid their dues. In the ensuing war, nine were killed in action, five died as prisoners of war, twelve had their homes burned, several lost sons, one moan's wife died in prison, and seventeen (including Thomas Jefferson) went broke.
set speech %%txt[%max%]%%
eval speech %speech%
say %speech%
~
#188
TBA Magic Eight Ball Check - 26~
0 q 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Check to see if the person is not carrying the magic eight ball
if !%actor.has_item(47)%
  * They are not carrying it. So stop them and give them one.
  * Return 0 ignores their command to go west. They stay in the room.
  return 0
  wait 1 sec
  say You forgot your magic eight ball. Take this one.
  * Loads the object to the actors inventory.
  %load% obj 47 %actor%
  %send% %actor% %self.name% gives you the magic eight ball.
end
~
#189
Mob Quest Example Load 8ball - 22~
2 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Check if the 8ball is already in the room. If not load it.
* I know this only checks the first item in the room, I don't really care. KISS
if %self.contents.vnum% != 47
  %load% obj 47
end
wait 2 sec
%echoaround% %actor% A magic eight ball drops from the sky striking %actor.name% on the head.
%send% %actor% A magic eight ball drops from the sky striking you on the head.
%damage% %actor% %random.5%
~
#190
Mob Quest Tutorial Example Quest completion - 25~
0 j 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %object.vnum% == 47 && !%actor.varexists(solved_example_quest_zone_0)%
  dance
  wait 1 sec
  say Thank you, %actor.name%. Here is a few experience points and some gold.
  nop %actor.exp(50)%
  nop %actor.gold(50)%
  say Finally, now I can get some answers.
  wait 1 sec
  emote shakes the magic eight ball vigorously.
  wait 1 sec
  emote does not seem too pleased with his answer.
  set solved_example_quest_zone_0 1
  remote solved_example_quest_zone_0 %actor.id%
  %purge% %object%
elseif %object.vnum% == 47
  say you already solved this quest, keep it.
  return 0
else
  say I don't want that!
  junk %object.name%
end
~
#191
Mob Quest Tutorial Example Quest accepted - 25~
0 d 1
yes~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.varexists(solved_example_quest_zone_0)% || !%actor.is_pc%
  halt
else
  wait 1 sec
  say Perfect, %actor.name%. I'll make this easy. It is to the east.
  wait 3 sec
  say I'd go get it myself, but I'm lazy and you need the exercise.
  wait 1 sec
end
~
#192
Mob Quest Tutorial Example Quest starter - 25~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %direction% == south
  if %actor.varexists(solved_example_quest_zone_0)%
    wait 1 sec
    say you have already completed this quest.
    halt
  else
    wait 1 sec
    say Hello, %actor.name%. Could you find me the magic eight ball?
    say Please say yes, %actor.name%.
  end
end
~
#193
TBA Object Get Coins Reload - 45~
1 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
%load% obj %self.vnum%
%echo% The pile of coins magically reappear.
~
#194
TBA Greeting - 17~
0 e 1
arrives entered appears~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.is_pc%
  wait 1 sec
  if %actor.varexists(TBA_greeting)%
    say Welcome back %actor.name%. Read through these rooms whenever you need a refresher.
  else
    say Welcome to The Builder Academy %actor.name%. 
    wait 1 sec
    say Within this zone you can learn all the immortal commands and how to build. 
    wait 2 sec
    say This zone is like a newbie zone, except for gods. All you have to do is walk through the zone and read every room description.
    wait 3 sec
    say Be sure to do everything the room descriptions tell you to do. You should read and comprehend everything contained within these walls.
    set TBA_greeting 1
    remote TBA_greeting %actor.id%
    if !%actor.has_item(1332)%
      %load% obj 1332 %actor% inv
    end
  end
end
~
#195
Stayalive idleout bracelet - 88~
1 b 4
~
* By Rumble of The Builder Academy    tbamud.com 9091
eval actor %self.worn_by%
if %actor%
  %send% %actor% 	n
end
~
#196
TBA Capital Letters Test - 17~
0 c 0
ca~
* By Rumble of The Builder Academy    tbamud.com 9091
* A basic command trigger. If a player types capitals the mob replies.
if capitals /= %cmd%
  tell %actor.name% Good job, that is correct. Be on the lookout for more of those.
end
~
#197
TBA Hallway - 22~
0 g 100
~
wait 2 sec
say Good Job, you made it.
wait 2 sec
say Now I would suggest that you practice what you have learned.
wait 2 sec
say Check your title under 	RWHO	n. A vnum should be listed there, if not mudmail Rumble for one.
wait 2 sec
say Just type 	RGOTO VNUM	n and type redit to modify your room.
wait 2 sec
say Once you complete your room come back to these hallways by typing 	RGOTO 3	n.
wait 3 sec
beam %actor.name%
~
#198
TBA Give Suggestions - 21~
0 g 100
~
wait 2 sec
say The best advice for new builders is under 	RHELP SUGGESTIONS	n.
~
#199
TBA Welcome - 18~
2 s 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* TBA mortal greet to fill out the application.
wait 1 sec
if %actor.level% <= 1
  wait 3 sec
  if %actor.varexists(TBA_mortal_greeting)%
    %echo% Friedrich Nietzsche says, 'Welcome back %actor.name%. Tell a God, Great God, or Implementor when you complete the application.'
  else
    %echo% Friedrich Nietzsche says, 'Welcome to The Builder Academy %actor.name%.'
    wait 2 sec
    %echo% Friedrich Nietzsche says, 'If you are interested in learning how to build, or want to teach others, then you have come to the right place.'
    wait 2 sec
    %echo% Friedrich Nietzsche says, 'Please fill out the application at: http://tbamud.com'
    nop %actor.thirst(-1)%
    nop %actor.hunger(-1)%
    set TBA_mortal_greeting 1
    remote TBA_mortal_greeting %actor.id%
    if !%actor.has_item(1332)%
      %load% obj 1332 %actor% inv
    end
  end
end
~
$~
