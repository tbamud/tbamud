#1300
Mob TBA Welcome - 1300~
0 e 1
entered reconnected~
if %actor.is_pc% && %actor.level% == 1
  wait 1 sec
  if %actor.varexists(mortal_greeting_for_TBA)%
    say Welcome back %actor.name%. Tell someone level 32 or above when you complete the application.
  else
    say Welcome to The Builder Academy %actor.name%. 
    wait 2 sec
    say If you are interested in learning how to build, or want to teach others, then you have come to the right place.
    wait 2 sec
    say Please fill out the application at: geocities.com/buildersacademy/
    set mortal_greeting_for_TBA 1
    remote mortal_greeting_for_TBA %actor.id%
  end
end
~
#1301
Give Suggestions - 1302~
0 g 100
~
wait 2 sec
say The best advice for new builders is under 	RHELP SUGGESTIONS	n.
~
#1302
Finish Redit Hallway - 1304~
0 g 100
~
wait 2 sec
say Good Job. That's the first form of OLC (on-line-creation) everyone learns.
wait 2 sec
say Now I would suggest that you practice what you have learned.
wait 2 sec
say Check your title under 	RWHO	n. A vnum should be listed there, if not mudmail Rumble for one.
wait 2 sec
say Just type 	RGOTO VNUM	n and type redit to modify your room.
wait 2 sec
say Once you think you have it complete go ahead and move on to the next hallway.
wait 3 sec
beam %actor.name%
~
#1303
CAPITAL LETTERS (1301)~
0 c 0
CAPITAL~
%send% %actor% 	RSocrates tells you, 'Good job, that is correct. Be on the lookout for more of those.'	n
~
#1304
Sanct check on enter~
2 g 100
~
if %actor.affect(SANCT)%
  %echo% checked sanct
else
  %echo% sanct check failed
end
~
#1305
TBA Greeting - 1301~
0 e 1
arrives entered appears~
* By Rumble of The Builder Academy    tbamud.com 9091
* TBA start room greeting for anyone that enters, poofs, or is transferred.
* Simple remote to remember if this is a newbie or someone returning.
* Only fire on players.
if %actor.is_pc%
  wait 1 sec
  * If they have already been greeted welcome them back.
  if %actor.varexists(TBA_greeting)%
    say Welcome back %actor.name%. Read through these rooms whenever you need a refresher.
    * First greeting, give them instructions.
  else
    say Welcome to The Builder Academy %actor.name%. 
    wait 1 sec
    say Within this zone you can learn all the immortal commands and how to build. 
    wait 2 sec
    say This zone is like a newbie zone, except for gods. All you have to do is walk through the zone and read every room description.
    wait 3 sec
    say Be sure to do everything the room descriptions tell you to do. You should read and comprehend everything contained within these walls.
    * Now create a variable to remember they have been greeted.
    set TBA_greeting 1
    * Save it to the player file.
    remote TBA_greeting %actor.id%
    * Give them a trial vnum assigner if they don't have one yet.
    if !%actor.has_item(1332)%
      %load% obj 1332 %actor% inv
    end
  end
end
~
#1306
Trial Vnum Assigner - 1332~
1 c 2
tbalim~
* By Rumble of The Builder Academy    tbamud.com 9091
* Trial vnum assigner. For STAFF only! Make sure player has nohassle off.
* Make sure name matches a player, purge mobs or use 0.name if you have 
* troubles. They are given an assigner in the mortal start room.
* Usage: tbalim player vnum | purge
if !%actor.is_pc% || %actor.level% < 30
  %send% %actor% Only human staff can use this limiter.
else  
  set victim %arg.car%
  if %victim.is_pc%
    if purge /= %arg.cdr% && %victim.has_item(1332)%
      %send% %actor% %arg.car%'s assigner has been %arg.cdr%'d.
      eval TBA_trial_vnum %victim.TBA_trial_vnum% - (2 * %victim.TBA_trial_vnum%)
      remote TBA_trial_vnum %victim.id%
      %purge% %victim.inventory(1332)%
    else
      set TBA_trial_vnum %arg.cdr%
      remote TBA_trial_vnum %victim.id%
      %send% %actor% %arg.car%'s trial vnum set to %arg.cdr%.
    end
  else
    %send% %actor% Make sure they have nohassle off. Don't use it on mobs. Use 0.name!
    return 0
  end
end
~
#1307
Annoying Verizone Wireless Guy - 1314~
0 abg 100
~
say can you hear me now?
~
#1308
inventory and container purge~
2 g 100
~
while %actor.inventory%
  eval item %actor.inventory%
  *   checks inside containers
  if %item.type% == CONTAINER
    while %item.contents%
      eval stolen %item.contents.vnum%
      %echo% purging %item.contents.shortdesc% in container.
      %purge% %item.contents% 
      eval num %random.99% + 100
      %at% %num% %load% obj %stolen%
    done
  end
  eval item_to_purge %%actor.inventory(%item.vnum%)%%
  eval stolen %item.vnum%
  %echo% purging %item.shortdesc%.
  %purge% %item_to_purge% 
  eval num %random.99% + 100
  %at% %num% %load% obj %stolen%
done
~
#1309
Eat, look, read, candy~
1 c 2
*~
if look /= %cmd.mudcommand% && heart /= %arg% && %arg% || read /= %cmd.mudcommand% && heart /= %arg% && %arg% || read /= %cmd.mudcommand% && candy /= %arg% && %arg% || look /= %cmd.mudcommand% && candy /= %arg% && %arg%
*
eval color %random.3%
set col[1] \	W
set col[2] \	R
set col[3] \	M
*	n
eval colors %%col[%color%]%%
*
eval heart %random.15%
set love[1] Be Mine
set love[2] I Love You
set love[3] I Wuv You
set love[4] Kiss Me
set love[5] Hug Me
set love[6] Be My Valentine
set love[7] Will You Be Mine
set love[8] You Are The Only One For Me
set love[9] I'm yours
set love[10] You're Special To Me
set love[11] I Really Like You
set love[12] I Luv U
set love[13] U R Mine
set love[14] With All My Heart
set love[15] Cutie Pie
eval candy %%love[%heart%]%%
*
%send% %actor% Written on the candy is:
%send% %actor% %colors% %candy% \	n
*
elseif eat /= %cmd.mudcommand% && heart /= %arg% && %arg% ||  eat /= %cmd.mudcommand% && candy /= %arg% && %arg%
*
eval message %random.10%
set word[1] It warms the cockles of your heart!
set word[2] It tastes wonderful!
set word[3] It melts in your mouth.
set word[4] It slowly disolves in your mouth.
set word[5] It tastes sweet and a little chalky.
set word[6] Mmmmm!
set word[7] You can feel the love!
set word[8] You feel warmer already!
set word[9] Mmm hearty goodness!
set word[10] Sweet like candy!
eval msg %%word[%message%]%%
*
%send% %actor% You eat %self.shortdesc%.
%echoaround% %actor% %actor.name% eats %self.shortdesc%.
%send% %actor% %msg%
%purge% self
*
else
return 0
end
~
#1310
Multiple Choice Answer A~
2 c 100
A~
%echo% That's it!
~
#1311
Multiple Choice Answer B~
2 c 100
B~
%echo% That's it!
~
#1312
Multiple Choice Answer C~
2 c 100
C~
      %echo% That's it!
~
#1313
Multiple Choice Answer D~
2 c 100
D~
      %echo% That's it!
~
#1314
Room Door Example~
2 c 100
move~
*%door% <vnum> < direction> < field> [value]
*Used for adding, deleting, and modifying doors in room #vnum.
*Direction determines which door is being changed, and can be north, south, east, west, up, or down. 
*If the door does not exist first, a new door will be created. 
*Field is what property of the door is being changed. Valid fields are:
*
*purge - remove the exit in the direction specified - no value needed
*description -  value will become the new exit description
*flags - value will be the new door flags bitvector as defined here:
*  a - Exit is a door that can be opened and closed. 
*  b - The door is closed. 
*  c - The door is locked. 
*  d - The door is pick-proof. 
*key - value is the vnum of the key to the door in this direction
*name - value is the name of the door in the specified direction
*room - value is the vnum of the room this direction leads to
 
*Example by Falstar for room 14520
*The following trigger is designed to reveal a trapdoor leading down when Player types 'Move Chest'
*
*The following ARGument determines what can be MOVEd ('move' Command inserted in Arg List of trigger)
if ("%arg%" == "chest")
*
*Send text to player to show effect of command
%send% %actor% You move the chest across the floor, revealing a trapdoor underneath!!
*
*Send text to other players in room to inform them of Player's discovery
%echoaround% %actor% %actor.name% has discovered a hidden trapdoor under a chest!
*
* Set door flags to 'ab' - Exit is a door that can be opened and closed, then close door
%door% 14520 down flags ab
* Change door name to 'trapdoor' (used in door commands, eg open trapdoor, lock trapdoor)
%door% 14520 down name trapdoor
* Set room that trapdoor exits leads to (Room 14521 in this case)
%door% 14520 down room 14521
*Set Exit desc for the wary adventure who decides to 'look down'
%door% 14520 down description A wooden ladder leads down into the darkness.
*Set the Vnum of the KEY OBJECT which will lock/unlock door
%door% 14520 down key 14500
*
* Now for Continuity, door commands must be set for reverse direction FROM 14521
%door% 14521 up flags ab
%door% 14521 up name trapdoor
%door% 14521 up room 14520
%door% 14521 up description A wooden ladder leads up into a storeroom.
%door% 14521 up key 14500
*
* IMPORTANT: Note reverse of direction in both the commands and extra descriptions and room numbers
*            it can be very easy to get lost here and probably get your adventurer lost too.
*
*Finally set up what happens when adventurer tries to move anything else and end the trigger
*
else
%send% %actor% Move what ?!
end
~
#1315
Room Enter Damage Example~
2 g 100
~
* First put a wait in there so the echo waits for the player to get into the room.
wait 1
%echo% As %actor.name% enters the room a bolt of lightning smites %actor.himher%.
%damage% %actor% 10
wait 5 sec
%echo% The Gods feel sorry for %actor.name% and restore him to full health.
%damage% %actor% -10
~
#1316
Room Command Example~
2 c 100
l~
if (%cmd.mudcommand% == look) && (%arg% /= test)
  %echo% works
  return 1
else
  * Don't capture the look command..
  return 0
end
~
#1317
Room At Example~
2 d 100
test~
*In progress
%at% 1208 %echo% wat'd
%echo% trigger fired.
%at% 1208 %purge% statue
~
#1318
Room Enter Block Mobs Not Following~
2 g 100
~
   if !%actor.is_pc%
  if !%actor.master%
    return 0
  end
end
~
#1319
Room Random Test For Variables~
2 b 100
~
%echo% my ID is: %self.ID%
%echo% my VNUM is: %self.vnum%
set actor %random.char%
%echo% Hello, %actor.name%
~
#1320
Room Enter Echo Example~
2 g 100
~
* put a wait in here so it doesn't fire before the player enters the room
wait 1
switch %random.3%
  case 1
    * only the person entering the room will see this.
    %send% %actor% You trip over a root as you walk into the room. 
    * everyone in the room except the actor will see this.
    %echoaround% %actor% %actor.name% trips on a root walking into the room.
    * everyone in the room will see this.
    %echo% The root suddenly springs to life and attacks!
    * let everyone in the zone hear about this.
    %zoneecho% %self.vnum% %actor.name% is a clutz.
  break
  case 2
    %send% %actor% You strut into the room. 
    %echoaround% %actor% %actor.name% Seems to have a big head..
    %echo% A strong breeze kicks some leaves up into the air.
  break
  case 3
    %echo% A light breeze picks up, causing the leaves to rustle quietly.
  break
  default
    * this should be here, even if it's never reached
    %echo% This is the default case in case I missed something
  break
done
~
#1321
Room Command Mushroom Pick~
2 c 100
pick~
if %cmd.mudcommand%==pick && mushroom/=%arg% && %arg.strlen%>0
%load% obj 1300
%force% %actor% say it works
else
%echo% pick what?
end
~
#1322
Room Random Variables~
2 b 100
test~
%echo% the weather is: %room.weather%
~
#1323
Room Variables Example~
2 b 100
~
set actor %random.char%
eval room %actor.room%
%echo% ID:     %room.id%
%echo% NAME:   %room.name%
%echo% NORTH:  %room.north%
%echo% SOUTH:  %room.south%
%echo% EAST:   %room.east%
%echo% WEST:   %room.west%
%echo% UP:     %room.up%
%echo% DOWN:   %room.down%
%echo% VNUM:   %room.vnum%
%echo% PEOPLE: %room.people%
~
#1324
Mob Greet Inventory Check~
0 g 100
~
* By Jamie Nelson from the forum http://groups.yahoo.com/group/dg_scripts/
* Checks if the actor is wielding the item
if %actor.eq(wield)%
  eval wep %actor.eq(wield)%
  if %wep.vnum%==1300
    set has_it 1
  end
end
* Checks the actors inventory if not wielding it.
if !%has_it%
  eval i %actor.inventory%
  while (%i%)
    set next %i.next_in_list%
    if %i.vnum%==1300
      set has_it 1
      break
    end
    if %i.type% == CONTAINER
      %echo% is a container.
      %echo% contains: %i.contents%
      eval in_bag %i.contents%
      %echo% %in_bag.vnum%
      while %in_bag%
        set next_in_bag %in_bag.next_in_list%
        %echo% %next_in_bag.vnum%
        if %in_bag.vnum%==1300
          %echo% has it in bag    
          set has_it 1
          break
        end
        set in_bag %next_in_bag%
      done
    set i %next%
  done
end
* 
if %has_it%
  say %actor.name% has that special item.
else
  say %actor.name% doesnt have that crappy item.
end
~
#1325
Mob Command Quote~
0 d 100
quote~
* By Jamie Nelson
eval w1max %random.20%
eval w2max %random.20%
eval w3max %random.20%
eval w4max %random.20%
eval w5max %random.11%
eval w6max %random.20%
set  w1[0] phenomenal
set  w1[1] rapid
set w1[2] chilling
set  w1[3] insipid
set  w1[4] nauseating
set  w1[5] astronomical
set  w1[6] austere
set  w1[7] inevitable
set  w1[8] inescapable
set  w1[9] reckless
set  w1[10] haphazard
set  w1[11] accelerating
set  w1[12] profound
set  w1[13] awesome
set  w1[14] terrifying
set  w1[15] ubiquitous
set  w1[16] ignominious
set  w1[17] unprecedented
set  w1[18] unparalleled
set  w1[19] insidious
set  w1[20] broad
set  w2[0] growth
set  w2[1] decline
set  w2[2] prospects
set  w2[3] acceleration
set  w2[4] threat
set  w2[5] expansion
set  w2[6] oneness
set  w2[7] outgrowth
set  w2[8] madness
set  w2[9] evacuation
set  w2[10] diminishment
set  w2[11] consumption
set  w2[12] decay
set  w2[13] putrefaction
set  w2[14] vapidity
set  w2[15] downsizing
set  w2[16] degeneration
set  w2[17] litigation
set  w2[18] declivity
set  w2[19] hastening
set  w2[20] paradigm shifting
set  w3[0] the Internet
set  w3[1] urban tax dollars
set  w3[2] new technologies
set  w3[3] gender identification disorders
set  w3[4] censorship
set  w3[5] interpersonal communications
set  w3[6] modern life
set  w3[7] rampant paradigm shifts
set  w3[8] consumer spending
set  w3[9] rain forests
set  w3[10] human literacy
set  w3[11] natural resources
set  w3[12] traditional values
set  w3[13] media junk food
set  w3[14] family values
set  w3[15] corporate mentality
set  w3[16] the American justice system
set  w3[17] technological change
set  w3[18] the ozone layer
set  w3[19] human resources
set  w3[20] current epistemologies
set  w4[0] forever dissipate
set  w4[1] escalate
set  w4[2] aggrandize
set  w4[3] overhaul
set  w4[4] deteriorate
set  w4[5] revolutionize
set  w4[6] uglify
set  w4[7] put an end to
set  w4[8] enslave
set  w4[9] bankrupt
set  w4[10] truncate
set  w4[11] nullify
set  w4[12] sabotage
set  w4[13] destabilize
set  w4[14] incapacitate
set  w4[15] hasten
set  w4[16] dehumanize
set  w4[17] evaporate
set  w4[18] indenture
set  w4[19] intensify
set  w4[20] undermine
set  w5[0] today's
set  w5[1] tomorrow's
set  w5[2] the entrenchment of our
set  w5[3] worldwide
set  w5[4] our children's
set  w5[5] modern
set  w5[6] all of our
set  w5[7] our future
set  w5[8] our
set  w5[9] the demise of our
set  w5[10] our grandchildren's
set  w5[11] all hope for
set  w6[0] business models
set  w6[1] re-ruralization
set  w6[2] human condition
set  w6[3] family values
set  w6[4] self-esteem
set  w6[5] medical insights
set  w6[6] human psyche
set  w6[7] human depth
set  w6[8] egalitarianism
set  w6[9] World Wide Web
set  w6[10] future values
set  w6[11] hopes and dreams
set  w6[12] business models
set  w6[13] political climate
set  w6[14] education
set  w6[15] cultural heritage
set  w6[16] lifestyles
set  w6[17] fiduciary responsibility
set  w6[18] genetic diversity
set  w6[19] intestinal fortitude
set  w6[20] computer literacy
set w1 %%w1[%w1max%]%%
eval w1 %w1%
set msg The %w1%
set w2 %%w2[%w2max%]%%
eval w2 %w2%
set msg %msg% %w2% of
set w3 %%w3[%w3max%]%%
eval w3 %w3%
set msg %msg% %w3%
set w4 %%w4[%w4max%]%%
eval w4 %w4%
set msg %msg% will %w4%
set w5 %%w5[%w5max%]%%
eval w5 %w5%
set msg %msg% %w5%
set w6 %%w6[%w6max%]%%
eval w6 %w6%
set msg %msg% %w6%
say %msg%
~
#1326
Mob Greet Einstein Converter - 1319~
0 g 100
~
wait 1 sec
emote is distracted by your entrance is seems to lose his train of thought.
wait 1 sec
say Good day, %actor.name%. Can I help you convert something?
~
#1327
Temperature Converter for Detta - 1319~
0 d 100
*~
if %speech% > -999
  eval celsius (((%speech% - 32) * 5) / 9)
  eval fahrenheit (((%speech% * 9) / 5) + 32)
  say %speech% converted to celsius would be: %celsius%
  say %speech% converted to fahrenheit would be: %fahrenheit%
end
~
#1328
Mobile Receive Example~
0 j 100
~
*Check to see if the player gave it the right object
if %object.vnum% == 1300
   wait 1 sec
   say Thank you %actor.name%!
   wait 1 sec
   tell %actor.name% Here is a small reward for your hard work!
   %echo% %self.name% thanks %actor.name% 
   kiss %actor.name%
   %purge% app
   %load% obj 1301
   give gren %actor.name%
else
   say I don't want this!
   return 0
end
~
#1329
Mobile Act Example~
0 e 100
kisses you.~
   slap %actor.name%
say I'm not that kind of girl.
pout
~
#1330
Mobile Greet Example~
0 g 100
~
*only greet players
   if %actor.is_pc%
*coming from this direction
   if %direction% == down
say Ah, hello %actor.name% I have been waiting for you. Go learn to build
%teleport% %actor% 1300
%force% %actor% look
end
end
~
#1331
Mobile Random Example~
0 b 100
~
*DENTIST
*From the show "Little Shop Of Horrors" (1982)
*(Alan Menken / Howard Ashman)
*Steve Martin
*
%echo% %self.name% sings, When I was young and just a bad little kid, 
wait 3 sec
%echo% %self.name% sings, My momma noticed funny things I did. 
wait 3 sec
%echo% %self.name% sings, Like shootin' puppies with a BB-Gun. 
wait 3 sec
%echo% %self.name% sings, I'd poison guppies, and when I was done, 
wait 3 sec
%echo% %self.name% sings, I'd find a pussy-cat and bash in it's head. 
wait 3 sec
%echo% %self.name% sings, That's when my momma said... 
wait 3 sec
%echo% A chorus from above sings, 'What did she say?'
wait 3 sec
%echo% %self.name% sings, She said my boy I think someday 
wait 3 sec
%echo% %self.name% sings, You'll find a way 
wait 3 sec
%echo% %self.name% sings, To make your natural tendencies pay... 
wait 6 sec
%echo% %self.name% sings, You'll be a dentist. 
wait 3 sec
%echo% %self.name% sings, You have a talent for causing things pain! 
wait 3 sec
%echo% %self.name% sings, Son, be a dentist. 
wait 3 sec
%echo% %self.name% sings, People will pay you to be inhumane! 
wait 3 sec
%echo% %self.name% sings, You're temperment's wrong for the priesthood, 
wait 3 sec
%echo% %self.name% sings, And teaching would suit you still less. 
wait 3 sec
%echo% %self.name% sings, Son, be a dentist. 
wait 3 sec
%echo% %self.name% sings, You'll be a success. 
wait 6 sec
%echo% A chorus from above sings, "Here he is folks, the leader of the plaque." 
wait 3 sec
%echo% A chorus from above sings, "Watch him suck up that gas. Oh My God!" 
wait 3 sec
%echo% A chorus from above sings, "He's a dentist and he'll never ever be any good." 
wait 3 sec
%echo% A chorus from above sings, "Who wants their teeth done by the Marqui DeSade?" 
wait 6 sec
%echo% An innocent dental patient screams, "Oh, that hurts! Wait! I'm not numb!" 
%echo% %self.name% sings, "Eh, Shut Up! Open Wide! Here I Come!" 
wait 6 sec
%echo% %self.name% sings, I am your dentist. 
wait 3 sec
%echo% %self.name% sings, And I enjoy the career that I picked. 
wait 3 sec
%echo% %self.name% sings, I'm your dentist. 
wait 3 sec
%echo% %self.name% sings, And I get off on the pain I inflict! 
wait 6 sec
%echo% %self.name% sings, When I start extracting those mollars 
wait 3 sec
%echo% %self.name% sings, Girls, you'll be screaming like holy rollers 
wait 6 sec
%echo% %self.name% sings, And though it may cause my patients distress. 
wait 3 sec
%echo% %self.name% sings, Somewhere...Somewhere in heaven above me... 
wait 3 sec
%echo% %self.name% sings, I know...I know that my momma's proud of me. 
wait 3 sec
%echo% %self.name% sings, "Oh, Momma..." 
wait 6 sec
%echo% %self.name% sings, 'Cause I'm a dentist... 
wait 3 sec
%echo% %self.name% sings, And a success! 
wait 6 sec
%echo% %self.name% sings, "Say ahh..." 
wait 3 sec
%echo% %self.name% sings, "Say AHhhh..." 
wait 3 sec
%echo% %self.name% sings, "Say AAARRRHHHH!!!" 
wait 3 sec
%echo% %self.name% sings, "Now Spit!" 
%purge% %self%
~
#1332
Variables Example~
0 b 100
~
* By Rumble
set actor %random.char%
say hello %actor.name%
say your alias is %actor.alias%
say you canbeseen %actor.canbeseen%
say your cha is %actor.cha%
say your class is %actor.class%
say your con is %actor.con%
say your dex is %actor.dex%
say your experience is %actor.exp%
*
*equipment positions
*light 0
if %actor.eq(light)%
  eval light %actor.eq(light)%
  say your light is ID: %light.id%, Name: %light.name%, Shortdesc: %light.shortdesc%, Timer: %light.timer%, Type: %light.type%.
  say your light is Val0: %light.val0%, Val1: %light.val1%, Val2: %light.val2%, Val3: %light.val3%, Vnum: %light.vnum%, Weight: %light.weight%, Cost: %light.cost%.
end
*rfinger 1
if %actor.eq(rfinger)%
  eval rfinger %actor.eq(rfinger)%
  say your rfinger is ID: %rfinger.id%, Name: %rfinger.name%, Shortdesc: %rfinger.shortdesc%, Timer: %rfinger.timer%, Type: %rfinger.type%.
  say your rfinger is Val0: %rfinger.val0%, Val1: %rfinger.val1%, Val2: %rfinger.val2%, Val3: %rfinger.val3%, Vnum: %rfinger.vnum%, Weight: %rfinger.weight%, Cost: %rfinger.cost%.
end
*lfinger 2
if %actor.eq(lfinger)%
  eval lfinger %actor.eq(lfinger)%
  say your lfinger is ID: %lfinger.id%, Name: %lfinger.name%, Shortdesc: %lfinger.shortdesc%, Timer: %lfinger.timer%, Type: %lfinger.type%.
  say your lfinger is Val0: %lfinger.val0%, Val1: %lfinger.val1%, Val2: %lfinger.val2%, Val3: %lfinger.val3%, Vnum: %lfinger.vnum%, Weight: %lfinger.weight%, Cost: %lfinger.cost%.
end
*neck1 3
if %actor.eq(neck1)%
  eval neck1 %actor.eq(neck1)%
  say your neck1 is ID: %neck1.id%, Name: %neck1.name%, Shortdesc: %neck1.shortdesc%, Timer: %neck1.timer%, Type: %neck1.type%.
  say your neck1 is Val0: %neck1.val0%, Val1: %neck1.val1%, Val2: %neck1.val2%, Val3: %neck1.val3%, Vnum: %neck1.vnum%, Weight: %neck1.weight%, Cost: %neck1.cost%.
end
*neck2 4
if %actor.eq(neck2)%
  eval neck2 %actor.eq(neck2)%
  say your neck2 is ID: %neck2.id%, Name: %neck2.name%, Shortdesc: %neck2.shortdesc%, Timer: %neck2.timer%, Type: %neck2.type%.
  say your neck2 is Val0: %neck2.val0%, Val1: %neck2.val1%, Val2: %neck2.val2%, Val3: %neck2.val3%, Vnum: %neck2.vnum%, Weight: %neck2.weight%, Cost: %neck2.cost%.
end
*body 5
if %actor.eq(body)%
  eval body %actor.eq(body)%
  say your body is ID: %body.id%, Name: %body.name%, Shortdesc: %body.shortdesc%, Timer: %body.timer%, Type: %body.type%.
  say your body is Val0: %body.val0%, Val1: %body.val1%, Val2: %body.val2%, Val3: %body.val3%, Vnum: %body.vnum%, Weight: %body.weight%, Cost: %head.cost%.
end
*head 6
if %actor.eq(head)%
  eval head %actor.eq(head)%
  say your head is ID: %head.id%, Name: %head.name%, Shortdesc: %head.shortdesc%, Timer: %head.timer%, Type: %head.type%.
  say your head is Val0: %head.val0%, Val1: %head.val1%, Val2: %head.val2%, Val3: %head.val3%, Vnum: %head.vnum%, Weight: %head.weight%, Cost: %head.cost%.
end
*legs 7
if %actor.eq(legs)%
  eval legs %actor.eq(legs)%
  say your legs is ID: %legs.id%, Name: %legs.name%, Shortdesc: %legs.shortdesc%, Timer: %legs.timer%, Type: %legs.type%.
  say your legs is Val0: %legs.val0%, Val1: %legs.val1%, Val2: %legs.val2%, Val3: %legs.val3%, Vnum: %legs.vnum%, Weight: %legs.weight%, Cost: %legs.cost%.
end
*feet 8
if %actor.eq(feet)%
  eval feet %actor.eq(feet)%
  say your feet is ID: %feet.id%, Name: %feet.name%, Shortdesc: %feet.shortdesc%, Timer: %feet.timer%, Type: %feet.type%.
  say your feet is Val0: %feet.val0%, Val1: %feet.val1%, Val2: %feet.val2%, Val3: %feet.val3%, Vnum: %feet.vnum%, Weight: %feet.weight%, Cost: %feet.cost%.
end
*hands 9
if %actor.eq(hands)%
  eval hands %actor.eq(hands)%
  say your hands is ID: %hands.id%, Name: %hands.name%, Shortdesc: %hands.shortdesc%, Timer: %hands.timer%, Type: %hands.type%.
  say your hands is Val0: %hands.val0%, Val1: %hands.val1%, Val2: %hands.val2%, Val3: %hands.val3%, Vnum: %hands.vnum%, Weight: %hands.weight%, Cost: %hands.cost%.
end
*arms 10
if %actor.eq(arms)%
  eval arms %actor.eq(arms)%
  say your arms is ID: %arms.id%, Name: %arms.name%, Shortdesc: %arms.shortdesc%, Timer: %arms.timer%, Type: %arms.type%.
  say your arms is Val0: %arms.val0%, Val1: %arms.val1%, Val2: %arms.val2%, Val3: %arms.val3%, Vnum: %arms.vnum%, Weight: %arms.weight%, Cost: %arms.cost%.
end
*shield 11
if %actor.eq(shield)%
  eval shield %actor.eq(shield)%
  say your shield is ID: %shield.id%, Name: %shield.name%, Shortdesc: %shield.shortdesc%, Timer: %shield.timer%, Type: %shield.type%.
  say your shield is Val0: %shield.val0%, Val1: %shield.val1%, Val2: %shield.val2%, Val3: %shield.val3%, Vnum: %shield.vnum%, Weight: %shield.weight%, Cost: %shield.cost%.
end
*about 12
if %actor.eq(about)%
  eval about %actor.eq(about)%
  say your about is ID: %about.id%, Name: %about.name%, Shortdesc: %about.shortdesc%, Timer: %about.timer%, Type: %about.type%.
  say your about is Val0: %about.val0%, Val1: %about.val1%, Val2: %about.val2%, Val3: %about.val3%, Vnum: %about.vnum%, Weight: %about.weight%, Cost: %about.cost%.
end
*waist 13
if %actor.eq(waist)%
  eval waist %actor.eq(waist)%
  say your waist is ID: %waist.id%, Name: %waist.name%, Shortdesc: %waist.shortdesc%, Timer: %waist.timer%, Type: %waist.type%.
  say your waist is Val0: %waist.val0%, Val1: %waist.val1%, Val2: %waist.val2%, Val3: %waist.val3%, Vnum: %waist.vnum%, Weight: %waist.weight%, Cost: %waist.cost%.
end
*rwrist 14
if %actor.eq(rwrist)%
  eval rwrist %actor.eq(rwrist)%
  say your rwrist is ID: %rwrist.id%, Name: %rwrist.name%, Shortdesc: %rwrist.shortdesc%, Timer: %rwrist.timer%, Type: %rwrist.type%.
  say your rwrist is Val0: %rwrist.val0%, Val1: %rwrist.val1%, Val2: %rwrist.val2%, Val3: %rwrist.val3%, Vnum: %rwrist.vnum%, Weight: %rwrist.weight%, Cost: %rwrist.cost%.
end
*lwrist 15
if %actor.eq(lwrist)%
  eval lwrist %actor.eq(lwrist)%
  say your lwrist is ID: %lwrist.id%, Name: %lwrist.name%, Shortdesc: %lwrist.shortdesc%, Timer: %lwrist.timer%, Type: %lwrist.type%.
  say your lwrist is Val0: %lwrist.val0%, Val1: %lwrist.val1%, Val2: %lwrist.val2%, Val3: %lwrist.val3%, Vnum: %lwrist.vnum%, Weight: %lwrist.weight%, Cost: %lwrist.cost%.
end
*wield 16
if %actor.eq(wield)%
  eval wield %actor.eq(wield)%
  say your wield is ID: %wield.id%, Name: %wield.name%, Shortdesc: %wield.shortdesc%, Timer: %wield.timer%, Type: %wield.type%.
  say your wield is Val0: %wield.val0%, Val1: %wield.val1%, Val2: %wield.val2%, Val3: %wield.val3%, Vnum: %wield.vnum%, Weight: %wield.weight%, Cost: %wield.cost%.
end
*hold 17
if %actor.eq(hold)%
  eval hold %actor.eq(hold)%
  say your hold is ID: %hold.id%, Name: %hold.name%, Shortdesc: %hold.shortdesc%, Timer: %hold.timer%, Type: %hold.type%.
  say your hold is Val0: %hold.val0%, Val1: %hold.val1%, Val2: %hold.val2%, Val3: %hold.val3%, Vnum: %hold.vnum%, Weight: %hold.weight%, Cost: %hold.cost%.
end
say you are fighting: %actor.fighting%
say you have %actor.gold% coins
say your ID is %actor.id%
say your int is %actor.int%
say your is_killer is %actor.is_killer%
say your is_thief is %actor.is_thief%
say your level is %actor.level%
say your hitp is %actor.hitp%
say your mana is %actor.mana%
say your move is %actor.move%
say your master is: %actor.master%
say your maxhitp is %actor.maxhitp%
say your maxmana is %actor.maxmana%
say your maxmove is %actor.maxmove%
say the next_in_room is %actor.next_in_room%
say you are in room %actor.room%
say your sex is %actor.sex%
say you are a: %actor.hisher%
say you are a: %actor.heshe%
say you are a: %actor.himher%
say you have %actor.inventory% in your inventory.
say you know the skill backstab %actor.skill(backstab)%
say your str is %actor.str%
say your stradd is %actor.stradd%
say your vnum is %actor.vnum%
say your wis is %actor.wis%
say your weight is %actor.weight%
*
eval roomv %actor.room%
say the weather is: %roomv.weather%
~
#1333
free~
0 ab 12
~
eval max %random.200%
set  text[1]   My god!  It's full of stars!
set  text[2]   How'd all those fish get up here?
set  text[3]   I'm a very female dragon.
set  text[4]   I've got a peaceful, easy feeling.
set  text[5]   Ahhh, spring is in the air.
set  text[6]   I'm one of those bad things that happen to good people.
set  text[7]   Bring out your dead, bring out your dead!
set  text[8]   If there is no God, who pops up the next kleenex in the box?
set  text[9]   Have you ever imagined a world with no hypothetical situations?
set  text[10]  Pardon me, but do you have any Grey Poupon?
set  text[11]  If nothing sticks to Teflon, how do they stick Teflon to the pan?
set  text[12]  Better be nice or I will use fireball on you!
set  text[13]  Do you think I'm going bald?
set  text[14]  This is your brain, this is MUD, this is your brain on MUD, Any questions?
set  text[15]  I'm Puff the Magic Dragon, who the hell are you?
set  text[16]  Quick!  Reverse the polarity of the neutron flow!
set  text[17]  Shh...  I'm beta testing.  I need complete silence!
set  text[18]  I'm the real implementor, you know.
set  text[19]  If love is blind, why is lingerie so popular?
set  text[20]  Despite the cost of living, have you noticed how popular it remains?
set  text[21]  Are you crazy, is that your problem?
set  text[22]  A bus station is where a bus stops. A train station is where a train stops. On my desk I have a work station. GO FIGURE!
set  text[23]  If you can't beat them, arrange to have them beaten.
set  text[24]  It takes a big man to cry, but it takes a bigger man to laugh at him.
set  text[25]  Friends come and go, but enemies accumulate.
set  text[26]  A lie has speed but truth has endurance.
set  text[27]  Do not blame the sword for the hand that wields it.
set  text[28]  The intention is not to see through each other but to see each other through.
set  text[29]  You can build a throne of bayonets, but you cannot sit on it for long.
set  text[30]  True friendship comes when silence between two people is comforting.
set  text[31]  I can picture in my mind a world without hate or anger and I can picture us attacking that world because they would never expect it.
set  text[32]  Life is like a dog sled team. If you are not the lead dog, the scenery never changes.
set  text[33]  The only man completely at peace is a man without a navel.
set  text[34]  Losers talk about how hard they tried while the winner goes home with the prom queen.
set  text[35]  Give a man a fish and he will eat for a day. Teach him how to fish, and he will sit in a boat and drink beer all day.
set  text[36]  Keep your friends close but keep your enemies closer.
set  text[37]  No, my powers can only be used for good.
set  text[38]  Who was the first person to look at a cow and say, "I think I'll squeeze these dangly things here, and drink whatever comes out"?
set  text[39]  Why do toasters always have a setting that burns the toast to a horrible crisp such that no decent human being would eat?
set  text[40]  Why is there a light in the fridge and not in the freezer?
set  text[41]  Why is it that some people appear bright until you hear them speak?
set  text[42]  Can a hearse carrying a corpse drive in the carpool lane?
set  text[43]  Why do people point to their wrist when asking for the time, but don't point to their crotch when they ask where the bathroom is?
set  text[44]  Why ARE Trix only for kids?
set  text[45]  Why is a person that handles your money called a 'Broker'?
set  text[46]  Whose cruel idea was it for the word "lisp" to have an "s" in it?
set  text[47]  If corn oil is made from corn, and vegetable oil is made from vegetables, then what is baby oil made from?
set  text[48]  If electricity comes from electrons, does morality come from morons?
set  text[49]  Is Disney World the only people trap operated by a mouse?
set  text[50]  "I am" is reportedly the shortest sentence in the English language. Could it be that "I do" is the longest sentence?
set  text[51]  Do illiterate people get the full effect of Alphabet Soup?
set  text[52]  Did you ever notice that when you blow in a dog's face, he gets mad at you, but when you take him on a car ride, he sticks his head out the window?
set  text[53]  My mind works like lightning one brilliant flash and it is gone.
set  text[54]  100,000 sperm and you were the fastest?
set  text[55]  A closed mouth gathers no foot. 
set  text[56]  Someday, we'll all look back on this, laugh nervously and change the subject.
set  text[57]  A diplomat is someone who can tell you to go to hell in such a way that you will look forward to the trip.
set  text[58]  All generalizations are false, including this one.
set  text[59]  We are born naked, wet and hungry. Then things get worse.
set  text[60]  What was the best thing BEFORE sliced bread?
set  text[61]  All stressed out and no one to choke.
set  text[62]  Before you criticize someone, you should walk a mile in their shoes. That way, when you criticize them, you're a mile away and you have their shoes.
set  text[63]  Better to understand a little than to misunderstand a lot.
set  text[64]  Bills travel through the mail at twice the speed of checks. 
set  text[65]  Do NOT start with me. You will NOT win.
set  text[66]  Don't be irreplaceable; if you can't be replaced, you can't be promoted.
set  text[67]  Don't piss me off! I'm running out of places to hide the bodies.
set  text[68]  Don't take life too seriously, you won't get out alive.
set  text[69]  Duct tape is like the force, it has a light side and a dark side and it holds the universe together.
set  text[70]  Eagles may soar, but weasels don't get sucked into jet engines.
set  text[71]  Ever stop to think, and forget to start again? 
set  text[72]  Forget world peace. Visualize using your turn signal.
set  text[73]  Give me ambiguity or give me something else.
set  text[74]  Why do people with closed minds always open their mouths?
set  text[75]  He who laughs last thinks slowest.
set  text[76]  I didn't say it was your fault, Relsqui. I said I was going to blame you.
set  text[77]  I don't suffer from insanity. I enjoy every minute of it. 
set  text[78]  I feel like I'm diagonally parked in a parallel universe.
set  text[79]  I just got lost in thought. It was unfamiliar territory. 
set  text[80]  I need someone really bad. Are you really bad?
set  text[81]  I poured Spot remover on my dog. Now he's gone.
set  text[82]  I used to be indecisive. Now I'm not sure.
set  text[83]  I used to have a handle on life, and then it broke. 
set  text[84]  If ignorance is bliss, you must be orgasmic. 
set  text[85]  Some people are alive only because it's illegal to kill them.
set  text[86]  It is far more impressive when others discover your good qualities without your help.
set  text[87]  It may be that your sole purpose in life is simply to serve as a warning to others.
set  text[88]  Never mess up an apology with an excuse.
set  text[89]  Okay, who put a stop payment on my reality check?
set  text[90]  Of course I don't look busy... I did it right the first time.
set  text[91]  Quantum mechanics: The dreams stuff is made of. 
set  text[92]  Save your breath. You'll need it to blow up your date! 
set  text[93]  Smith & Wesson: The original point and click interface.
set  text[94]  Some days you are the bug, some days you are the windshield.
set  text[95]  Some drink at the fountain of knowledge. Others just gargle.
set  text[96]  The early bird may get the worm, but the second mouse gets the cheese. 
set  text[97]  The only substitute for good manners is fast reflexes. 
set  text[98]  The problem with the gene pool is that there is no lifeguard.
set  text[99]  Remember my name - you'll be screaming it later.
set  text[100] The severity of the itch is inversely proportional to the ability to reach it.
set  text[101] Very funny Scotty, now beam down my clothes.
set  text[102] Why is abbreviation such a long word? 
set  text[103] Why isn't phonetic spelled the way it sounds?
set  text[104] You're just jealous because the voices are talking to me and not you! 
set  text[105] The proctologist called, they found your head.
set  text[106] Everyone has a photographic memory; some just don't have film.
set  text[107] Try not to let your mind wander. It is too small to be out by itself.
set  text[108] You need only two tools. WD-40 and duct tape. If it doesn't move and it should, use WD-40. If it moves and shouldn't, use the tape.
set  text[109] If you woke up breathing, congratulations! You have another chance!
set  text[110] I don't believe in miracles. I rely on them.
set  text[111] When I'm feeling down, I like to whistle. It makes the neighbor's dog that barks all the time run to the end of his chain and gag himself.
set  text[112] Why did kamikaze pilots wear helmets?
set  text[113] I'm not tense, just terribly, terribly alert.
set  text[114] How do I set a laser printer to stun?
set  text[115] Everything I need to know about life I learned by killing smart people and eating their brains.
set  text[116] I thought I wanted a career, turns out I just wanted paychecks.
set  text[117] Is it time for your medication or mine?
set  text[118] Too many freaks, not enough circuses.
set  text[119] How many times do I have to flush before you go away?
set  text[120] No word in the English language rhymes with month, orange, silver, and purple.
set  text[121] If lawyers are disbarred and clergymen defrocked, doesn't it follow that electricians can be delighted, musicians denoted, cowboys deranged, models deposed, tree surgeons debarked and dry cleaners depressed?
set  text[122] Sarcasm: just one more service we offer here.
set  text[123] This is a mean and cruel world. I want my nappy and medication right now!
set  text[124] Back off! You're standing in my aura.
set  text[125] More people are killed annually by donkeys than die in air crashes.
set  text[126] A 'jiffy' is an actual unit of time for 1/100th of a second.
set  text[127] Does your train of thought have a caboose?
set  text[128] Money isn't made out of paper, it's made out of cotton.
set  text[129] I got out of bed for this? 
set  text[130] You, you and you: panic.  The rest of you, come with me.
set  text[131] Stress is when you wake up screaming and you realize you haven't fallen asleep yet.
set  text[132] I'm not your type. I'm not inflatable.
set  text[133] If it's stupid but works, it isn't stupid.
set  text[134] If only you'd use your powers for good instead of evil...
set  text[135] The more you sweat in peace, the less you bleed in war.
set  text[136] Tracers work both ways.
set  text[137] Who cares if a laser guided 500 lb bomb is accurate to within 3 feet?
set  text[138] Ever wonder about those people who spend 2.00 dollars a piece on those little bottles of Evian water? Try spelling Evian backwards.
set  text[139] Isn't making a smoking section in a restaurant like making a peeing section in a swimming pool?
set  text[140] Why do croutons come in airtight packages? Aren't they just stale bread to begin with?
set  text[141] Why is it that rain drops but snow falls?
set  text[142] If it's true that we are here to help others, then what exactly are the others here for?
set  text[143] The light at the end of the tunnel has been turned off due to budget cuts.
set  text[144] %random.4% days without a human rights violation!
set  text[145] At least you're not being rectally probed by aliens.
set  text[146] The most powerful force in the universe is gossip.
set  text[147] You should not confuse your career with your life.
set  text[148] No matter what happens, somebody will find a way to take it too seriously.
set  text[149] When trouble arises and things look bad, there is always one individual who perceives a solution and is willing to take command. Very often, that individual is crazy.
set  text[150] There is a very fine line between "hobby" and "mental illness."
set  text[151] Take out the fortune before you eat the cookie.
set  text[152] Never under any circumstances take a sleeping pill and a laxative on the same night.
set  text[153] You should never say anything to a woman that even remotely suggests you think she's pregnant unless you can see an actual baby emerging from her at that moment.
set  text[154] A person who is nice to you, but rude to the waiter, is not a nice person.
set  text[155] When everything's coming your way, you're in the wrong lane.
set  text[156] I live in my own little world, but it's ok they know me here.
set  text[157] Show me a man with both feet firmly on the ground, and I'll show you a man who can't get his pants off.
set  text[158] I don't approve of political jokes...I've seen too many of them get elected.
set  text[159] I love being married. It's so great to find that one special person you want to annoy for the rest of your life.
set  text[160] I am a nobody, nobody is perfect, therefore I am perfect.
set  text[161] Everyday I beat my own previous record for number of consecutive days I've stayed alive.
set  text[162] If carrots are so good for the eyes, how come I see so many dead rabbits on the highway? 
set  text[163] Welcome To Shit Creek - Sorry, We're Out of Paddles!
set  text[164] How come we choose from just two people to run for president and 50 for Miss America?
set  text[165] Ever notice that people who spend money on beer, cigarettes, and lottery tickets are always complaining about being broke and not feeling well?
set  text[166] The next time you feel like complaining remember: Your garbage disposal probably eats better than thirty percent of the people in this world.
set  text[167] Snowmen fall from Heaven unassembled.
set  text[168] Every time I walk into a singles bar I can hear Mom's wise words: "Don't pick that up, you don't know where it's been."
set  text[169] Out of my mind...Back in five minutes.
set  text[170] I want to die peacefully in my sleep like my grandfather...Not screaming and yelling, like the passengers in his car.
set  text[171] Man who run in front of car get tired.
set  text[172] Man who run behind car get exhausted.
set  text[173] Man who scratches backside should not bite fingernails.
set  text[174] Man who passes wind in church sits in own pew.
set  text[175] Some days, I just don't feel like slaying dragons.
set  text[176] Thank you for not being perky.
set  text[177] Don't annoy the crazy person.
set  text[178] Which trailer park did you grow up in?
set  text[179] When the only tool you own is a hammer, every problem begins to look like a nail.
set  text[180] And your crybaby whiny ass opinion would be?
set  text[181] The longest one-syllable word in the English language is "screeched."
set  text[182] On a Canadian two dollar bill, the flag flying over the Parliament building is an American flag.
set  text[183] I didn't fight my way to the top of the food chain to be a vegetarian.
set  text[184] What am I? Flypaper for freaks?
set  text[185] Would you like fries with that?
set  text[186] Why do they lock gas station bathrooms? Are they worried someone will clean them?
set  text[187] I may not be the best looking gal here, but I'm the only one talking to you.
set  text[188] Where do forest rangers go to get away from it all?
set  text[189] Who are these kids and why are they calling me Mom?
set  text[190] Not the brightest crayon in the box now, are we?
set  text[191] Don't bother me. I'm living happily ever after.
set  text[192] I started out with nothing and still have most of it left.
set  text[193] You! Off my planet!
set  text[194] Therapy is expensive, poppin' bubble wrap is cheap! You choose.
set  text[195] Did the aliens forget to remove your anal probe?
set  text[196] It is as bad as you think and they are out to get you.
set  text[197] Isn't it scary that doctors call what they do "practice"?
set  text[198] Accept that some days you're the pigeon, and some days you're the statue.
set  text[199] I'm not crazy, I've just been in a very bad mood for years.
set  text[200] All I ask is a chance to prove money can't make me happy.
set  speech %%text[%max%]%%
eval speech %speech%
say %speech%
~
#1334
Mob Transform Example~
0 g 100
~
* %transform% test
* as a greet trigger, entering the room will cause
* the mob this is attached to, to toggle between mob 1 and 99.
say Beginning transform.
if %self.vnum%==1
  %transform% 99
else
  %transform% -1
end
say Transform complete.
~
#1335
Mob Greet Switch Random~
0 g 100
~
* By Falstar
   switch %random.6%
   case 0
   set book 'Creative cooking with human flesh'
   break
   case 1
   set book 'Re-animating the Dead for Dummies'
   break
   case 2
   set book 'How to teach your henchman to rob graves in 7 days'
   break
   case 3
   set book 'An A-Z guide of Mage-induced maladies and mutations'
   break
   case 4
   set book '101 Easy ways to rescue a damsel in distress'
   break
   case 5
   set book 'Witch Hazel's Bumper Book of rare herbs and potions'
   break
   case 6
   set book 'Arcane Artifacts made easy'
   break
   default
   * this should be here, even if it's never reached
set book default reached
   done
*
*wait 5 s
%echo% Dr. Von Erhartz seems engrossed in reading a large leatherbound book through a battered pair
%echo% of reading glasses. The title reads: %book%.
*wait 3 s 
%echo% The doctor looks up at you, seeming to notice you for the first time.
*wait 1 s
say ah %actor.name%, I was wondering when you'd drop by.
~
#1336
Mob Random Room Switch Example~
0 b 100
~
* By Rumble
* So we don't get problems if more than one is loaded.
context %self.id%
eval room %self.room%
switch %room.vnum%
  case 1300
    say this is where I began my journey.
  break
  case 1301
    say Ah, yes, the beginning.
  break
  case 1302
    say TBA, The Builder Academy Implementation explained!
  break
  case 1303
    say Building blocks for beginners.
  break
  case 1304
    say Writing good descriptions. Very important.
  break
  case 1305
    say Learning redit.
  break
  case 1306
    say Oedit by osmosis.
  break
  case 1307
    say Medit by the numbers.
  break
  case 1308
    say Zedit confuses everyone.
  break
  case 1309
    say Sedit. We all love capitalism.
  break
  case 1310
    say Trigedit is tricky.
  break
  case 1311
    say Planning for those who lack direction.
  break
  case 1312
    say Advanced building, what an interesting topic.
  break
  case 1421
    say Storytelling, now that is a good idea.
  break
  case 1313
    say I'm finished. Finally. That Rumble is long winded.
  break
  default
    say so much reading, so little time.
  break
done
~
#1337
Mob Random Time Example~
0 b 50
~
* Convert hour from 24 hour to 12 hour clock with am/pm
if %time.hour% > 12
  eval hour %time.hour% - 12
  set ampm pm
else
  set hour %time.hour%
  set ampm am
end
*
* No 0 hour. Change it to 12.
if %time.hour% == 0
  set hour 12
end
*
* Figure out what day (1-35).
switch %time.day%
  case 1
  case 7
  case 14
  case 21
  case 28
  case 35
    set day the Day of the Moon
    break
  case 2
  case 8
  case 15
  case 22
  case 29
  set day the Day of the Bull
    break
  case 3
  case 9
  case 16
  case 23
  case 30
  set day the Day of the Deception
    break
  case 4
  case 10
  case 17
  case 24
  case 31
  set day the Day of Thunder
    break
  case 5
  case 11
  case 18
  case 25
  case 32
  set day the Day of Freedom
    break
  case 6
  case 12
  case 19
  case 26
  case 33
  set day the Day of the Great Gods
    break
  case 7
  case 13
  case 20
  case 27
  case 34
  set day the Day of the Sun
    break
  default
    set day I don't know what day it is
    break
done
*
* What suffix should we use for the number of the day.
switch %time.day%
  case 1
  case 21
  case 31
    set suf st
    break
  case 2
  case 22
  case 32
    set suf nd
    break
  case 3
  case 23
  case 33
    set suf rd
    break
  default
    set suf th
    break
done
*
* What month are we in (1-17).
    set m1 Month of Winter
    set m2 Month of the Winter Wolf
    set m3 Month of the Frost Giant
    set m4 Month of the Old Forces
    set m5 Month of the Grand Struggle
    set m6 Month of the Spring
    set m7 Month of Nature
    set m8 Month of Futility
    set m9 Month of the Dragon
    set m10 Month of the Sun
    set m11 Month of the Heat
    set m12 Month of the Battle
    set m13 Month of the Dark Shades
    set m14 Month of the Shadows
    set m15 Month of the Long Shadows
    set m16 Month of the Ancient Darkness
    set m17 Month of the Great Evil
eval months %%m%time.month%%%
*
* My test to make sure my variables are printing out what I expect them to.
* %echo% Hour: %time.hour% Day: %time.day% Month: %time.month% Year:
%time.year%
*
*Finally the output.
%echo% It is %hour% o'clock %ampm%, on %day%.
%echo% The %time.day%%suf% Day of the %months%, Year %time.year%.
~
#1338
Mob Command Copycat~
0 c 100
e~
if %cmd.mudcommand%==emote && %arg.strlen%>0
%force% %actor% emote %arg%
wait 2 sec
emote %arg%
~
#1339
Mob Receive Multiple Items~
0 j 100
~
* Example by Aeon
wait 2 sec
* Check mob's inventory (this does not include equipped items)
if (%self.inventory(16701)%) && (%self.inventory(16702)%) && (%self.inventory(16703)%) && (%self.inventory(16704)%) && (%self.inventory(16705)%)
  * Removing items from the mob
  %purge% flour
  %purge% salt
  %purge% sugar
  %purge% soda
  %purge% egg
  * For roleplay sake
  say Thank you %actor.name%! Now here is the cake.
  * Give the reward
  %load% obj 16706
  give cake %actor.name%
end
~
#1340
Mob Random Master Test~
0 b 100
~
if (%actor.master%) 
  eval master %self.master%
  if %master.fighting%
    say I will save you Master %master.name%
    wait 1 sec
    assist %master.name%
  end
end
~
#1341
Mob Random Follow Master~
0 b 100
~
set actor %random.char%
mfollow %actor%
say I follow you now %actor.name%
say self.master: %self.master%
say actor: %actor%
eval follower %self.master%
say follower: %follower%
say follower.name : %follower.name%
~
#1342
Mob Hunt Example~
0 o 100
~
%echo% Sleeping for 10 secs, give %actor.name% a head start.
sleep
%echo% Targetting %actor.name%
wait 10 s
wake
stand
open door
north
north
up
%echo% Hunting...%actor.name%
mhunt %actor.name%
mhunt %actor%
~
#1343
Mob Random Special Character's Example~
0 b 100
~
*Special Characters Example
eval mob %self.alias%
eval mob %mob.car%
%echo% %self.vnum%
%echo% %mob%
%echo% |%mob% -> name's, someone's, your
%echo% &%mob% -> it, you, he/she
%echo% *%mob% -> it, you, him/her
* object only
* %echo% '%mob% -> something, name
~
#1344
Mob Speech Variables Checker~
0 d 100
*~
* By Nemmie from the forum http://groups.yahoo.com/group/dg_scripts/
eval name %actor.car%
eval test %%name.varexists(%speech.cdr%)%%
if %test%
  eval var %%name.%speech.cdr%%%
  %echo% %name.name% has remote variable %speech.cdr% which has the 
value of '%var%'.
else
  %echo% %name.name% doesnt have the variable %speech.cdr%.
end
~
#1345
Obj Wear Straitjacket - 1330~
1 j 100
~
attach 1347 %self.id%
~
#1346
Obj Command to Remove Straitjacket - 1330~
1 c 7
untie~
%echoaround% %actor% %actor.name% unties the straitjacket.
%send% %actor% You untie the jacket, maybe now you can remove it.
detach 1347 %self.id%
~
#1347
Obj Remove Straitjacket - 1300~
1 l 7
~
%echoaround% %actor% %actor.name% struggles to get out of the straitjacket.
%send% %actor% You can't get out of it. Maybe you should untie it first.
return 0
~
#1348
delete me~
1 f 100
~
* No Script
~
#1349
free~
1 c 3
*~
* No Script
~
#1350
Drop While Example - Grenade 01301~
1 h 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* A small script to make a grenade go off 3 seconds after it is dropped.
* Set the rooms ID to a variable.
set room_var %actor.room%
wait 3 s
* Send a message when the bomb goes off.
%echo% The Grenade blasts into smithereens, striking everyone here.
* Target the first char.
set target_char %room_var.people%
* Now loop through everyone in the room and hurt them.
while %target_char%
  * Set the next target before this one perhaps dies.
  set tmp_target %target_char.next_in_room%
  * This is where the good/bad things are supposed to happen.
  %send% %target_char% The explosion hurts you.
  * Damage them... 30 hitpoints. To heal use -#.
  %damage% %target_char% 30
  * Set the next target.
  set target_char %tmp_target%
  * Loop back.
done
* After we go through everyone get rid of the grenade.
%purge% %self%
~
#1351
free~
1 g 100
~
%load% obj %self.vnum%
%echo% The pile of coins magically reappear.
~
#1352
Obj Command Gun Example~
1 c 7
shoot~
set room_var %actor.room%
set target_char %room_var.people%
%echoaround% %actor% %actor.name% Pulls the trigger on %self.shortdesc%. It explodes with a deafening boom.
%send% %actor% You lift up %self.shortdesc% and aim carefully, squeezing the trigger gently.
while %target_char%
  if %target_char% != %actor%
    %send% %target_char% The explosion hurts you.
    %damage% %target_char% 300
    %echoaround% %target_char.name% The shrapnel blasts into %target_char.name%.
  end
  set tmp_target %target_char.next_in_room%
  set target_char %tmp_target%
done
~
#1353
Obj Random Bunny~
1 b 1
~
* By Nemmie from the forum http://groups.yahoo.com/group/dg_scripts/
eval actor %self.carried_by%
if %actor.vnum% == 0
set self bunny_bag
eval actor %self.carried_by%
else
end
eval number %random.4%
switch %number%
case 1
oecho A pink bunny tries to free itself from %actor.name%, but fails.
break
case 3
oecho A pink bunny manages to slip away from %actor.name%.
%load% mob 800
set self pink_bunny
%purge% %self%
break
default
break
done
~
#1354
Mob Speech Spy~
0 d 1
*~
%at% rumble %echo% %actor.name% says, '%speech%'
~
#1355
Mob Act Spy~
0 e 1
leaves~
%at% rumble %echo% %actor.name% '%arg%'
%echo% arg: %arg%
%echo% cmd: %cmd%
~
#1356
Mob Global Random King Wander~
0 ab 100
~
* King wander example by Zemial
* So we don't get problems if more than one king is loaded
context %self.id%
eval room %self.room%
* so we know if he is going to bed or getting up
if (%room.vnum% == 3193)
  set getting_up 1
  global getting_up
end
if (%room.vnum% == 3166)
  unset getting_up
end
if (%getting_up%) 
* so we know he's going from his bedroom to his throne room
  switch %room.vnum%
    case 3193
      wait until 8:00
      wake
      wait 1
      yawn
      wait 1 s
      stand
      wait 1
      emote stretches a bit and flexes his muscles.
      wait 1
      open door s
      south
      break
    case 3190
      south
      break
    case 3188
      west
      break
    case 3177
      down
      break
    case 3165
      open door n
    case 3164
    case 3161
      north
      break
    default
      say I always enjoy going new places.
      break
  done
else
* so he wasn't getting up after all - let's put him to bed
  switch %room.vnum%
    case 3166
      wait until 22:00
      say I guess it's time for bed now.
      * just in case he's not standing
      stand
      open door s
      south
      break
    case 3165
    case 3164
      south
      break
    case 3161
      up
      break
    case 3177
      east
      break
    case 3188
      north
      break
    case 3190
      open door n
      north
      wait 1
      yawn
      wait 1 s
      emote gets ready for bed.
      wait 2 s
      sleep
      break
    default
      say I've always wanted to see new places
      break
  done
end
~
#1357
Mob Random Groundhog Day~
0 b 10
~
%echo% The groundhog sticks its head out of its hole.
wait 1 sec
%echo% The groundhog sees its shadow and scurries back inside.
wait 2 sec
%echo% 6 more weeks of winter!
~
#1358
Mob wait until Wake and Sleep~
0 ab 100
~
* By Aeon This script wakes the mob at dawn, and puts him to sleep at night.
wait until 06:00
wake
say It's morning already!
wait 1 s
yawn
wait 1 s
stand
wait until 21:00
emote looks sleepy.
yawn
wait 10 s
rest
wait 10 s
sleep
~
#1359
Mob Death Purges Equipment~
0 f 100
~
remove all
eval i %self.inventory%
while (%i%)
  set next %i.next_in_list%
  %purge% %i%
  set i %next% 
done
~
#1360
Room Random While Teleport~
2 b 100
~
*Determine number of people in the room.
eval person %self.people%
%echo% There are %person% people here.
wait 1 sec
*While there are still people in the room.
while (%person%) 
  %send% %person% You are next!
  %echo% I am targetting %person.name%.
  %echoaround% %person% %person.name% is struck by a bolt of lightning. Leaving only a pile of ash.
  %teleport% %person% 1300
  %force% %person% look
  eval person %self.people%
done
~
#1361
Free~
1 b 100
~
* No Script
~
#1362
Deodorant Bottle - 1391~
1 c 7
spray~
if %arg% 
  %echoaround% %actor% %actor.name% soaks %arg% with the deodorant spray.
  %send% %actor% You soak %arg% with the deodorant spray.
else
  %echoaround% %actor% %actor.name% sprays deodorant about the room.
  %send% %actor% You spray deodorant about the room.
end
~
#1363
Thanksgiving Turkey - 1322~
0 b 10
~
switch %random.7%
  case 1
    emote gobbles uncontrollably.
  break
  case 2
    emote is looking for the President to pardon %self.himher%.
  break
  case 3
    emote bobs %self.hisher% head up and down to some rhythm.
  break
  case 4
    emote prunes %self.hisher% feathers.
  break
  case 5
    emote puffs out %self.hisher% chest.
  break
  case 6
    emote pecks at your feet.
  break
  case 7
    emote cocks %self.hisher% head to the side looking up at you.
  break
  default
    emote is staying far away from the chopping block.
  break
done
~
#1364
Return Example~
2 c 100
w~
if %cmd.mudcommand% == west
  if %actor.sex% == male
    %send% %actor% The door vanishes as if it was never there, and you step through.
    %echoaround% %actor% As %actor.name% steps through the doorway, the door disappears for an instant.
    %teleport% %actor% 1300
    %echoaround% %actor% %actor.name% has arrived.
    %force% %actor% look
    * This part of the trigger returns 1 automatically.
  else
    * The trigger returns 0 so next trigger is checked and the player can not pass.
    return 0
  end
end
~
#1365
Trial Vnum Assigner - 1332~
1 c 2
*~
* By Rumble of The Builder Academy    tbamud.com 9091
* Player must have nohassle off! To junk assigner use tbalim purge player.
if %actor.level% > 30 && %actor.varexists(TBA_trial_vnum)%
  set actor %self.carried_by%
  * We set completed trial vnums to -#. So if negative abort.
  if %actor.TBA_trial_vnum% < 0
    return 0
  end
  if (%cmd.mudcommand% == redit && ((%arg% && %arg% != %actor.TBA_trial_vnum%) || (%actor.room.vnum% != %actor.TBA_trial_vnum%)))
    %send% %actor% GOTO %actor.TBA_trial_vnum% to edit your room.
  elseif %cmd.mudcommand% == oedit && %arg% != %actor.TBA_trial_vnum%
    %send% %actor% Use OEDIT %actor.TBA_trial_vnum% to modify your object.
  elseif %cmd.mudcommand% == medit && %arg% != %actor.TBA_trial_vnum%
    %send% %actor% Use MEDIT %actor.TBA_trial_vnum% to modify your mobile.
  elseif (%cmd.mudcommand% == zedit && ((%arg% && %arg% != %actor.TBA_trial_vnum%) || (%actor.room.vnum% != %actor.TBA_trial_vnum%)))
    %send% %actor% GOTO %actor.TBA_trial_vnum% to edit your trial vnums zone information.
  elseif %cmd.mudcommand% == purge && ((%arg% && %arg% != %actor.TBA_trial_vnum%) || (%actor.room.vnum% != %actor.TBA_trial_vnum%)))
    %send% %actor% GOTO %actor.TBA_trial_vnum% to purge your room.
  elseif %cmd.mudcommand% == nohassle || (%cmd.mudcommand% == toggle && nohassle /= %arg.car%)
    %send% %actor% You cannot enable nohassle until you finish your trial vnum.
  elseif %cmd.mudcommand% == buildwalk || (%cmd.mudcommand% == toggle && buildwalk /= %arg.car%)
    %send% %actor% You cannot enable buildwalk until you finish your trial vnum.
  elseif %cmd.mudcommand% == sedit || %cmd.mudcommand% == qedit || %cmd.mudcommand% == trigedit || %cmd.mudcommand% == dig || %cmd.mudcommand% == rclone || %cmd.mudcommand% == attach || %cmd.mudcommand% == detach || %cmd.mudcommand% == vdelete 
    %send% %actor% Sedit, Trigedit, Qedit, Dig, Rclone, Attach, Detach, and Vdelete are not required for your trial vnum.
  elseif %cmd.mudcommand% == zpurge
    %send% %actor% Zpurge is not required for your trial vnum. Use 'purge' or 'purge item.'
  elseif %cmd.mudcommand% == sacrifice
    %send% %actor% Sacrifice is disabled until your trial room is completed.
  else
    return 0
  end
else
  return 0
end
~
#1366
!DROP Assigner - 1332~
1 his 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.level% < 32
  %send% %actor% You can't get rid of %self.shortdesc%.
  return 0
end
~
#1367
Rumble's Diner~
2 d 100
food~
* These are random FOOD objects throught the MUD. Replace with your own.
eval max %random.259% -1
set  food[0] 5
set  food[1] 6
set  food[2] 7
set  food[3] 8
set  food[4] 9
set  food[5] 10
set  food[6] 14
set  food[7] 109
set  food[8] 110 
set  food[9] 111
set  food[10] 112
set  food[11] 114
set  food[12] 164
set  food[13] 180
set  food[14] 218
set  food[15] 309
set  food[16] 310
set  food[17] 311
set  food[18] 312
set  food[19] 313
set  food[20] 314
set  food[21] 315
set  food[22] 447
set  food[23] 501
set  food[24] 502
set  food[25] 521
set  food[26] 537
set  food[27] 383
set  food[28] 622 
set  food[29] 635
set  food[30] 637
set  food[31] 638
set  food[32] 639
set  food[33] 640
set  food[34] 1304
set  food[35] 1339
set  food[36] 1398
set  food[37] 1901
set  food[38] 1904
set  food[39] 1917
set  food[40] 1922
set  food[41] 1926
set  food[42] 1927
set  food[43] 1956
set  food[44] 2504
set  food[45] 2505
set  food[46] 2506
set  food[47] 2546
set  food[48] 2710
set  food[49] 2806
set  food[50] 3009
set  food[51] 3010
set  food[52] 3011
set  food[53] 3012
set  food[54] 3013
set  food[55] 3014
set  food[56] 3015
set  food[57] 3300
set  food[58] 3301
set  food[59] 3308
set  food[60] 3309
set  food[61] 3310
set  food[62] 3701
set  food[63] 3702
set  food[64] 3805
set  food[65] 3920
set  food[66] 3943
set  food[67] 3972
set  food[68] 4052
set  food[69] 4103
set  food[70] 4104
set  food[71] 4311
set  food[72] 4312
set  food[73] 4313
set  food[74] 4316
set  food[75] 4520
set  food[76] 4524
set  food[77] 4609
set  food[78] 5219
set  food[79] 5440
set  food[80] 5441
set  food[81] 5442
set  food[82] 5443
set  food[83] 5445
set  food[84] 5456
set  food[85] 5457
set  food[86] 5458
set  food[87] 5459
set  food[88] 5460
set  food[89] 5461
set  food[90] 5462
set  food[91] 5463
set  food[92] 5464
set  food[93] 5465
set  food[94] 5491
set  food[95] 6010
set  food[96] 6011
set  food[97] 6018
set  food[98] 6022
set  food[99] 6023
set  food[100] 6024
set  food[101] 6106
set  food[102] 6107
set  food[103] 6420
set  food[104] 7403
set  food[105] 7507
set  food[106] 7508
set  food[107] 7509
set  food[108] 7510
set  food[109] 7513
set  food[110] 7562
set  food[111] 7803
set  food[112] 7804
set  food[113] 7805
set  food[114] 7806
set  food[115] 7807
set  food[116] 7908
set  food[117] 8304
set  food[118] 8319
set  food[119] 8398
set  food[120] 9601
set  food[121] 10006
set  food[122] 10102
set  food[123] 10103
set  food[124] 10628
set  food[125] 10629
set  food[126] 10630
set  food[127] 12006
set  food[128] 12007
set  food[129] 12010
set  food[130] 12503
set  food[131] 12508
set  food[132] 12509
set  food[133] 12510
set  food[134] 12562
set  food[135] 12567
set  food[136] 24011
set  food[137] 24012
set  food[138] 24013
set  food[139] 24130
set  food[140] 24222
set  food[141] 24292
set  food[142] 24293
set  food[143] 24294
set  food[144] 24307
set  food[145] 24308
set  food[146] 24309
set  food[147] 24819
set  food[148] 24820
set  food[149] 25409
set  food[150] 25413
set  food[151] 25600
set  food[152] 25601
set  food[153] 25608
set  food[154] 25609
set  food[155] 25610
set  food[156] 25621
set  food[157] 25709
set  food[158] 25710
set  food[159] 25711
set  food[160] 25712
set  food[161] 25714
set  food[162] 25791
set  food[163] 26901
set  food[164] 27113
set  food[165] 27123
set  food[166] 27128
set  food[167] 27129
set  food[168] 27130
set  food[169] 27131
set  food[170] 27132
set  food[171] 27133
set  food[172] 27175
set  food[173] 27176
set  food[174] 27178
set  food[175] 27190
set  food[176] 27199
set  food[177] 27200
set  food[178] 27217
set  food[179] 27219
set  food[180] 27401
set  food[181] 27516
set  food[182] 27520
set  food[183] 27521
set  food[184] 27522
set  food[185] 27523
set  food[186] 27527
set  food[187] 27703
set  food[188] 27710
set  food[189] 28116
set  food[190] 28117
set  food[191] 28326
set  food[192] 28621
set  food[193] 28622
set  food[194] 28643
set  food[195] 28713
set  food[196] 28720
set  food[197] 28721
set  food[198] 28722
set  food[199] 28789
set  food[200] 28914
set  food[201] 28915
set  food[202] 28916
set  food[203] 29012
set  food[204] 29205
set  food[205] 29207
set  food[206] 29214
set  food[207] 29240
set  food[208] 29241
set  food[209] 29242
set  food[210] 29412
set  food[211] 29413
set  food[212] 29414
set  food[213] 29504
set  food[214] 29602
set  food[215] 29603
set  food[216] 30105
set  food[217] 30106
set  food[218] 30107
set  food[219] 30108
set  food[220] 30109
set  food[221] 30136
set  food[222] 30141
set  food[223] 30818
set  food[224] 30819
set  food[225] 30909
set  food[226] 31303
set  food[227] 31511
set  food[228] 31514
set  food[229] 31515
set  food[230] 31531
set  food[231] 31560
set  food[232] 31561
set  food[233] 31581
set  food[234] 31582
set  food[235] 31583
set  food[236] 31584
set  food[237] 31587
set  food[238] 31588
set  food[239] 31589
set  food[240] 31600
set  food[241] 31723
set  food[242] 31724
set  food[243] 31725
set  food[244] 31726
set  food[245] 31727
set  food[246] 31728
set  food[247] 31908
set  food[248] 32207
set  food[249] 32308
set  food[250] 32343
set  food[251] 32344
set  food[252] 32407
set  food[253] 32429
set  food[254] 32430
set  food[255] 32506
set  food[256] 32525
set  food[257] 32527
set  food[258] 32528
set  grub %%food[%max%]%%
eval grub %grub%
  %load% o %grub% %actor% inv
~
#1368
wait until test~
0 b 100
~
if %time.hour% > 4 && %time.hour% < 21
  wait until 21:00
  say I hereby declare Kortaal closed!
  wait 5s
  close gate
  lock gate
else
  wait until 4:00
  say I hereby declare Kortaal open!
  wait 5s
  unlock gate
  open gate
end
~
#1369
Exit checker~
2 b 100
~
%echo% NORTH:    %self.north% %self.north(bits)%
%echo% SOUTH:    %self.south% %self.south(bits)%
%echo% EAST:     %self.east% %self.east(bits)%
%echo% WEST:     %self.west% %self.west(bits)%
%echo% UP:       %self.up% %self.up(bits)%
%echo% DOWN:     %self.down% %self.down(bits)%
~
#1370
Mob Transform Test~
0 l 50
~
%echo% %self.name% screams in pain as its skin begins to melt and slide away.
wait 1 sec
%transform% 1308
%echo% The skin finally melts away in chunks of steaming goo revealing %self.name%.
detach all %self.id%
~
#1371
Mob can't be attacked~
0 c 100
*~
* Room would have to be NO_MAGIC since cast could bypass this.
if (%cmd.mudcommand% == bash || %cmd.mudcommand% == backstab || %cmd.mudcommand% == kill || %cmd.mudcommand% == hit || %cmd.mudcommand% == kick) && (alias1 /= %arg% || alias2 /= %arg%)
    say You can't kill me.
  else
    return 0
  end
~
#1372
Rumble's Poofs~
0 e 0
has entered the game.~
* By Rumble of The Builder Academy    tbamud.com 9091
* To generate random poofs at login just set your loadroom to wherever this
* mob is. 
eval maxpoofin %random.24%
set  poofins[1] appears with a strange wooshing sound and climbs out of a pneumatic air 
tube like they use at the bank.
set  poofins[2] thinks himself into existence.
set  poofins[3] soars into the room like a bird, and THWAP! right into a window.
set  poofins[4] crawls out of the ground gasping for air.
set  poofins[5] appears in a flash of blinding nothingness!
set  poofins[6] falls from the sky above, screaming until he hits the ground. SPLAT! like a 
bug on a windshield.
set  poofins[7] appears with a dulcet bang.
set  poofins[8] appears with a sonic boom.
set  poofins[9] wanders into the room while practicing omphaloskepsis.
set  poofins[10] somersaults into the room.
set  poofins[11] stumbles into the room, tripping over his own feet and falling flat on his face.
set  poofins[12] dives into the room doing a two and a half tuck gainer, right into the dirt.
set  poofins[13] runs into the room screaming and looking over his shoulder.
set  poofins[14] steps out of your shadow.
set  poofins[15] forms out of the very essence of your shadow to hang in the air before you.
set  poofins[16] climbs out of your left nostril.
set  poofins[17] has abandoned his search for truth and is now looking for a good fantasy.
set  poofins[18] wishes he was a donut specialist.
set  poofins[19] can resist everything but temptation.
set  poofins[20] is searching for a near life experience.
set  poofins[21] walks into the room fashionably early.
set  poofins[22] hanglides into the room.
set  poofins[23] parachutes into the room performing a perfect parachute landing fall, 
except for the fact that he landed backside first.
set  poofins[24] does a cannonball into room, injuring himself on the hard ground.
eval  poofin %%poofins[%maxpoofin%]%%
%force% %actor% set self poofin %poofin%
*
eval maxpoofout %random.20%
set  poofouts[1] is chased out of the room by a barrel of rabid monkeys.
set  poofouts[2] creates a pneumatic air tube, like they use at the banks, and steps in. He is sucked out of sight.
set  poofouts[3] thinks himself out of existence.
set  poofouts[4] walks out saying 'time to make the donuts.'
set  poofouts[5] goes super critical and has a meltdown. Nothing remains but a pile of steaming radioactive mush.
set  poofouts[6] disappears in a flash of blinding nothingness!
set  poofouts[7] merges with his surroundings and vanishes.
set  poofouts[8] morphs into millions of ants, which run off in all directions.
set  poofouts[9] senses that everything is well, so he floats away.
set  poofouts[10] goes to hell in a handbasket.
set  poofouts[11] does somersaults out of the room.
set  poofouts[12] dives out of the room doing three rotations in the jackknife position.
set  poofouts[13] stumbles out of the room with a look of confusion on his face, must have forgotten where he parked.
set  poofouts[14] steps into your shadow and disappears.
set  poofouts[15] points behind you with a look of horror. While you turn away he disappears chuckling 'made you look'.
set  poofouts[16] doesn't like saying goodbye, so he didn't.
set  poofouts[17] completes an intricate spell of chantings and gestures that creates an inter-dimensional portal of space and time. Before he steps through it and disappears you notice a 'made in china' sticker on its bottom.
set  poofouts[18] creates a huge rubber band, straps himself into the middle of it and stretches it back like a slingshot. With a wave he releases it and is hurtled out of sight.
set  poofouts[19] straps an ACME rocket to his back and asks, 'got a light?' right before it explodes and sends him soaring.
set  poofouts[20] puts on a helmet and climbs into the barrel of an ACME cannon. It explodes sending pieces of Rumble off into the distance.
eval  poofout %%poofouts[%maxpoofout%]%%
%force% %actor% set self poofout %poofout%
~
#1373
Present Unwrapping~
1 c 7
unwrap~
eval present %random.326% * 100 + %random.99%
%echo% present: %present% id %present.id.name% vnum %present.vnum% name %present.name%
%force% %actor% vstat obj %present%
if %present% == %present.id.name%
%echo% fails
else
%echo% works
end
~
#1374
Consume Example~
1 s 100
~
%send% %actor% You %command% %self.name%.
%echoaround% %actor% %actor.name% %command%s %self.name%.
return 0
%purge% %self%
~
#1375
Random Mob Purge~
2 b 100
~
* This script checks if anyone is in the room. If so each mob has a 50 
* percent chance of being purged 5 percent of the time.
eval target %self.people%
while %target%
  eval tmp_target %target.next_in_room%
  if %target.vnum% != -1  && %random.2% != 1
    %echo% The gods destroy %target.name%
    %purge% %target%
  end
  eval target %tmp_target%
done
~
#1376
Obj Contents test~
1 c 7
testing~
%echo% firing
eval in_bag %self.contents%
while %in_bag%
  set next_in_bag %in_bag.next_in_list%
  %echo% contains: %in_bag.vnum%
  set in_bag %next_in_bag%
done
~
#1377
door test~
2 g 100
~
%door% 1233 west flags abcd
%door% 1233 west key 1233
%door% 1233 west name steel door
%door% 1233 west room 1233
~
#1378
Racing Bet~
2 c 100
bet~
set 1 Salya
set 2 Fluffy
set 3 Angela
set 4 Malicious
set 5 Arden
set 6 Calista
set 7 Balderdash
set 8 Hessa
set Salya 99
set Fluffy 50
set Angela 50
set Malicious 35
set Arden 33
set Calista 28
set Balderdash 25
set Hessa 20
set racer 1
set racing 1
while %racing%
  eval name %racer%
  %echo% Racer %racer%: %%name%%
  eval racer %racer%+1
  if %racer% == 9
    set racing 0
  else
  end
done
~
#1379
Command test~
2 c 100
l~
* Numeric Arg: 7 means obj can be worn, carried, or in room.
* Make sure the command is look, check for any abbrev of closet
* and make sure there is an arg.
if %cmd.mudcommand% == look && closet /= %arg%
  %send% %actor% As you peer into the closet you see movement.
  %echoaround% %actor% %actor.name% looks into a closet and something comes out.
  %load% mob 1
else
  * If it doesn't match let the command continue.
  return 0
end
* An example for sitting in a chair. Arg: s
* if %cmd.mudcommand% == sit && chair /= %arg%
*   %echoaround% %actor% %actor.name% sits in a chair.
*   %send% %actor% You sit in a chair.
* else
*   return 0
* end
~
#1380
Command Test~
2 c 100
l~
if %cmd.mudcommand% == look && rodent /= %arg%
  return 0  
  wait 2 sec
  %send% %actor% A soft, pleasant voice calls 'Welcome, do come inside.'
else
  return 0
end
~
#1381
Voodoo Doll~
1 c 2
pin~
* By Heiach
if !%arg%
  %send% %actor% Stab a pin into a voodoo doll of who?
else
  eval dmg %arg.hitp% * 100 / %arg.maxhitp%
  if (%dmg% > 25)
    eval pain %random.15%
    switch %pain%
      case 1
        set hurt back
      break
      case 2
        set hurt head
      break
      case 3
        set hurt stomach
      break
      case 4
        set hurt left eye
      break
      case 5
        set hurt chest
      break
      case 6
        set hurt right eye
      break
      case 7
        set hurt left arm
      break
      case 8
        set hurt right arm
      break
      case 9
        set hurt left leg
      break
      case 10
        set hurt right leg
      break
      case 11
        set hurt groin
      break
      case 12
        set hurt left foot
      break
      case 13
        set hurt right foot
      break
      case 14
        set hurt throat
      break
      case 15
        set hurt heart
      break
      default
      break
    done
    %send% %actor% You slowly push a pin into the voodoo doll of %arg.name%'s %hurt%.
    %echoaround% %actor% %actor.name% slowly pushes a pin into a voodoo doll.
    %send% %arg% You suddenly feel a sharp stabbing pain in your %hurt%!
    %echoaround% %arg% %arg.name% suddenly screams with pain, clenching %arg.hisher% %hurt%!
    %damage% %arg% 10
    wait 20 s
  elseif %dmg% < 26
    set msg %random.5%
    switch %msg%
      case 1
        %send% %actor% %arg.name% has suffered enough!
      break
      case 2
        %send% %actor% %arg.name% can't take it anymore!
      break
      case 3
        %send% %actor% %arg.name% is too weak!
      break
      case 4
        %send% %actor% %arg.name% is still writhing in pain!
      break
      case 5
        %send% %actor% %arg.name% won't survive another pin!
      break
      default
      break
    done
  end
end
~
#1382
dg_affect test~
0 g 100
~
dg_affect %actor% str 1 1
dg_affect %actor% dex 1 1
dg_affect %actor% int 1 1
dg_affect %actor% wis 1 1
dg_affect %actor% con 1 1
dg_affect %actor% cha 1 1
dg_affect %actor% age 1 1
dg_affect %actor% char_weight 1 1
dg_affect %actor% char_height 1 1
dg_affect %actor% maxmana 1 1
dg_affect %actor% maxhit 1 1
dg_affect %actor% maxmove 1 1
dg_affect %actor% armor 1 1
dg_affect %actor% hitroll 1 1
dg_affect %actor% damroll 1 1
dg_affect %actor% saving_para 1 1
dg_affect %actor% saving_rod 1 1
dg_affect %actor% saving_petri 1 1
dg_affect %actor% saving_breath 1 1
dg_affect %actor% saving_spell 1 1
dg_affect %actor% blind on 1
dg_affect %actor% invis on 1
dg_affect %actor% det-align on 1
dg_affect %actor% det-invis on 1
dg_affect %actor% det-magic on 1
dg_affect %actor% sense-life on 1
dg_affect %actor% watwalk on 1
dg_affect %actor% sanct on 1
dg_affect %actor% curse on 1
dg_affect %actor% infra on 1
dg_affect %actor% poison on 1
dg_affect %actor% prot-evil on 1
dg_affect %actor% prot-good on 1
dg_affect %actor% sleep on 1
dg_affect %actor% no_track on 1
dg_affect %actor% sneak on 1
dg_affect %actor% hide on 1
dg_affect %actor% charm on 1
~
#1383
%load% test~
0 g 100
~
%load% obj 200 %actor% light
%load% obj 201 %actor% rfinger
%load% obj 202 %actor% lfinger
%load% obj 203 %actor% neck1
%load% obj 204 %actor% neck2
%load% obj 205 %actor% body
%load% obj 206 %actor% head
%load% obj 207 %actor% legs
%load% obj 208 %actor% feet
%load% obj 209 %actor% hands
%load% obj 210 %actor% arms
%load% obj 211 %actor% shield
%load% obj 212 %actor% about
%load% obj 213 %actor% waist
%load% obj 214 %actor% rwrist
%load% obj 215 %actor% lwrist
%load% obj 216 %actor% wield
%load% obj 217 %actor% hold
%load% obj 218 %actor% inv
%echo% You have 10 seconds to remove and junk all before we test 0-17
wait 10 sec
%load% obj 200 %actor% 0
%load% obj 201 %actor% 1
%load% obj 202 %actor% 2
%load% obj 203 %actor% 3
%load% obj 204 %actor% 4
%load% obj 205 %actor% 5
%load% obj 206 %actor% 6
%load% obj 207 %actor% 7
%load% obj 208 %actor% 8
%load% obj 209 %actor% 9
%load% obj 210 %actor% 10
%load% obj 211 %actor% 11
%load% obj 212 %actor% 12
%load% obj 213 %actor% 13
%load% obj 214 %actor% 14
%load% obj 215 %actor% 15
%load% obj 216 %actor% 16
%load% obj 217 %actor% 17
%load% obj 218 %actor% 18
~
#1384
Mob Speech Parrot~
0 d 100
*~
*** Squak for the nice people, Polly
      wait 1s
      emote squawks loudly.
      wait 1s
*** 75 percent chance of learning phrase
      eval polly %random.4%
      if (%polly% > 1)
        say %speech%
%echo% %speech%
%echo% %phrase(1)%
%echo% %phrase(2)%
%echo% %phrase(3)%
*** Ignore if already known
        switch %speech%
          case test
          case test 2
          case %phrase(3)%
          case %phrase(4)%
          case %phrase(5)%
          case %phrase(6)%
          case %phrase(7)%
          case %phrase(8)%
          case %phrase(9)%
          case %phrase(10)%
            emote looks at you curiously.
            break
          default
            break
        done
*** Learn new phrases
        eval number (%number% + 1)
        eval phrase(%number%) %speech%
        global number
        global phrase(%number%)
*** Reset array after 10 phrases
          if (%number% == 10)
            set number 0
            global number
            set maxphrases 1
            global maxphrases
            end
      else
       end
~
#1385
Room Enter if/elseif/else Combo crash test~
2 g 100
~
if %actor.is_pc%
  wait 1 sec
  if %actor.sex% == male
    %echo% male
  elseif %actor.sex% == female
    %echo% elseif female
  else
    %echo% else nuetral
else
%echo% mob
~
#1386
Room Command Combination Lock~
2 c 100
push~
*
* combinationlock, based on script by Rumble
* small modifications made by Welcor
*
* To allow multiple scripts based on this script we set a context:
  context %self.id%
* clear old var, if set.
  unset reset_buttons
* check argument
  if %arg% == red
    %send% %actor% You push the red button.
    set pushed_red 1
    global pushed_red
  elseif %arg% == yellow
    %send% %actor% You push the yellow button.
    if %pushed_red%
      set pushed_yellow 1
      global pushed_yellow
    else 
      set reset_buttons 1
    end
  elseif %arg% == green
    %send% %actor% You push the green button.
    if %pushed_red% && %pushed_yellow%
      set pushed_green 1
      global pushed_green
    else
      set reset_buttons 1
    end
  elseif %arg% == blue
    %send% %actor% You push the blue button.
    if %pushed_red% && %pushed_yellow% && %pushed_green%
      set pushed_blue 1
      global pushed_blue
    else
      set reset_buttons 1
    end
  elseif %arg% == purple
    %send% %actor% You push the purple button.
    if %pushed_red% && %pushed_yellow% && %pushed_green% && %pushed_blue%
      %send% %actor% You hear a loud click as you push the final button.
      set reset_buttons 1
    else
      set reset_buttons 1
      end
  elseif %arg% == white
    %send% %actor% You push the white reset button.
    set reset_buttons 1
  else
    %send% %actor% Push what ?!
    halt
  end
* reset the buttons if necessary
  if %reset_buttons%
    unset reset_buttons
    unset pushed_red
    unset pushed_yellow
    unset pushed_green
    unset pushed_blue
    %send% %actor% The buttons all suddenly pop back out.
  end
* send the state of the buttons
  %send% %actor% red:%pushed_red% yellow:%pushed_yellow% green:%pushed_green% blue:%pushed_blue% purple:%pushed_purple%
~
#1387
free~
2 g 100
~
eval item %actor.inventory%
eval item_to_purge %%actor.inventory(%item.vnum%)%%
if %item_to_purge%
  %echo% purging %item.shortdesc% with vnum %item.vnum% in %actor.name%'s inventory.
  %purge% %item_to_purge% 
else
%echo% I cant find %item.shortdesc% with vnum %item.vnum% in %actor.name%'s inventory.
  %echo% I cant find an item in %actor.name%'s inventory.
end
~
#1388
Room Command Detach Example~
2 c 100
detach~
Detach 1388 %self.id%
%echo% detached
~
#1389
FREE~
1 c 7
join~
eval currentroom %self.room% 
if ((%currentroom.vnum% == 1233) && (%actor.inventory(1315)%) && (%actor.inventory(1316)%))
  %echo% room check correct: %currentroom.vnum%
  %purge% %actor.inventory(1316)%
  %echo% orb purged
  switch %random.2%
    case 1
      otransform 12
      %echo% staff loaded
    break
    default
      otransform 1317
      %echo% broken staff loaded
    break
  done
  detach 1389 %self.id%
else
  %send% %actor% You can not do that here.
end
~
#1390
Obj Random Special Character's Example~
1 b 100
~
*Special Characters Example
eval obj %self.name%
eval obj %obj.car%
%echo% %self.vnum%
%echo% %obj%
%echo% |%obj% -> name's, someone's, your
%echo% %obj% -> name, someone, you
%echo% &%obj% -> it, you, he/she
%echo% *%obj% -> it, you, him/her
* object only
%echo% '%obj% -> something, name
~
#1391
Obj Wear Ruby Slippers~
1 j 100
~
wait 1 sec
dg_cast 'word of recall' %actor%
~
#1392
free~
1 c 2
shake~
* Numeric Arg: 2 means in character's carried inventory
* There are 20 possible answers that the Magic Eight Ball can give. 
* Of these, nine are full positive, two are full negative, one is 
* mostly positive, three are mostly negative, and five are abstentions. 
*
if ball /= %arg% || eightball /= %arg%
  %echoaround% %actor% %actor.name% shakes the magic eight ball vigorously.
  %send% %actor% You shake the magic eight ball vigorously.
    switch %random.20%
      case 1
        %send% %actor% The magic eight ball reveals the answer: Outlook Good 
      break
      case 2
        %send% %actor% The magic eight ball reveals the answer: Outlook Not So Good 
      break
      case 3
        %send% %actor% The magic eight ball reveals the answer: My Reply Is No
      break
      case 4
        %send% %actor% The magic eight ball reveals the answer: Don't Count On It
      break
      case 5
        %send% %actor% The magic eight ball reveals the answer: You May Rely On It
      break
      case 6
        %send% %actor% The magic eight ball reveals the answer: Ask Again Later
      break
      case 7
        %send% %actor% The magic eight ball reveals the answer: Most Likely 
      break
      case 8
        %send% %actor% The magic eight ball reveals the answer: Cannot Predict Now
      break
      case 9
        %send% %actor% The magic eight ball reveals the answer: Yes 
      break
      case 10
        %send% %actor% The magic eight ball reveals the answer: Yes, definitely
      break
      case 11
        %send% %actor% The magic eight ball reveals the answer: Better Not Tell You Now 
      break
      case 12
        %send% %actor% The magic eight ball reveals the answer: It Is Certain
      break
      case 13
        %send% %actor% The magic eight ball reveals the answer: Very Doubtful
      break
      case 14
        %send% %actor% The magic eight ball reveals the answer: It Is Decidedly So
      break
      case 15
        %send% %actor% The magic eight ball reveals the answer: Concentrate And Ask Again
      break
      case 16
        %send% %actor% The magic eight ball reveals the answer: Signs Point To Yes 
      break
      case 17
        %send% %actor% The magic eight ball reveals the answer: My Sources Say No 
      break
      case 18
        %send% %actor% The magic eight ball reveals the answer: Without A Doubt 
      break
      case 19
        %send% %actor% The magic eight ball reveals the answer: Reply Hazy, Try Again
      break
      case 20
        %send% %actor% The magic eight ball reveals the answer: As I See It, Yes
      break
      default
        %send% %actor% The magic eight ball explodes since your question is unanswerable.
      break
    done
else
%send% %actor% shake What?
end
~
#1393
Obj Command quarter flip example~
1 c 2
flip~
* Numeric Arg: 2 means in character's carried inventory
if coin /= %arg% || quarter /= %arg%
  %echoaround% %actor%  %actor.name% flips a coin high up into the air.
  %send% %actor% You flip the coin up into the air.
    switch %random.2%
      case 1
        %echo% The coin falls to the ground, bounces, rolls, and ends up showing heads!
      break
      case 2
        %echo% The coin falls to the ground, bounces, rolls, and ends up showing tails!
      break
      default
        %echo% The coin falls on its edge and balances perfectly.
      break
    done
else
  %send% %actor% flip What?
end
~
#1394
Timing trigger~
2 ab 100
~
wait until 7:00
%echo% loading
%load% obj 1200
wait until 11:00
%purge% list
~
#1395
Mob Random Black Cat~
0 b 20
~
switch %random.7%
  case 1
    emote crosses your path.
  break
  case 2
    emote sneezes.
  break
  case 3
    emote rubs up against your leg.
  break
  case 4
    emote starts cleaning itself.
  break
  case 5
    emote howls wickedly.
  break
  case 6
    emote hisses at you spitefully!
  break
  case 7
    emote purrs warmly.
  break
  default
    emote runs in terror.
  break
done
~
#1396
Room Speech Actor.eq Example~
2 d 100
test~
set i 0
while %i%<32
eval item %%actor.eq(%i%)%%
if %item%
%echo% In slot %i% you are wearing %item.shortdesc%
end
eval i %i%+1
done
~
#1397
Mob Random Wait Until Example~
0 ab 100
~
eval time1 %time.hour%
if %time1% != %time.hour%
  say %time1%
end
wait until 1:00
say 1
wait until 2:00
say 2
wait until 3:00
say 3
wait until 4:00
say 4
wait until 5:00
say 5
wait until 6:00
say 6
wait until 7:00
say 7
wait until 8:00
say 8
wait until 9:00
say 9
wait until 10:00
say 10
wait until 11:00
say 11
wait until 12:00
say 12
wait until 13:00
say 13
wait until 14:00
say 14
wait until 15:00
say 15
wait until 16:00
say 16
wait until 1700
say 17
wait until 18:00
say 18
wait until 19:00
say 19
wait until 20:00
say 20
wait until 21:00
say 21
wait until 22:00
say 22
wait until 23:00
say 23
wait until 24:00
say 24
~
#1398
Random Rabbit Decapitates Mobs - M1307~
0 b 100
none~
* By Rumble of The Builder Academy    tbamud.com 9091
* This is for any Monty Python Fans.
* First figure out what room you are in.
eval room_var %self.room%
* Target the first character.
set target %room_var.people%
* Make a loop so everyone in the room is targeted.
while %target%
  * Create the next target before the bunny kills them.
  set tmp_target %target.next_in_room%
  * Don't let the bunny kill players or itself.
  if ((%target.vnum% != -1) && (%target.name% != %self.name%))
    * Do the deed with a little pause in between.
    emote hops towards %target.name% and looks up innocently. 
    wait 2 sec
    emote strikes with lightning speed, decapitating %target.name%.
    * bye bye.
    %purge% %target%
    wait 5 sec
    * End the if statement.
  end
  * Target to the temp target you created above.
  set target %tmp_target%
  * Loop back to the next target.
done
* Remove the bunny, I don't want people leaving him lying in waiting.
%purge% %self%
~
#1399
Chair Sit~
1 c 7
si~
* Trigger fires off the command sit chair.
if %cmd.mudcommand% == sit && %arg% /= chair
  %echoaround% %actor% %actor.name% sits in a chair.
  %send% %actor% You sit in a chair.
  * Set the actors position as sitting (they will have to stand).
  nop %actor.pos(sitting)%
else
  * Make sure to let them sit, even if they don't choose the chair.
  return 0
end
~
$~
