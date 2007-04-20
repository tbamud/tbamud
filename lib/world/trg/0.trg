#0
Non-attachable trigger~
0 g 100
~
* You can't attach trigger 0!
~
#1
Mob Tutorial Example Quest Offer - M14~
0 g 100
~
* we don't want him to tell this to mobs. Check if actor is a player.
if %actor.is_pc%
  * only greet players coming from the south.
  if %direction% == south
    * wait 1 second, always give the player time before you start sending text.
    wait 1 sec
    say Can you help me, %actor.name%?
    wait 1 sec
    say An ogre has something of mine.
    wait 1 sec
    say If you slay him I'll give you all the coins I can spare.
    wait 1 sec
    say Please, bring me the wings he has stolen.
  end
end
~
#2
Mob Tutorial Example Kill Ogre - 16~
0 f 100
~
say you got the best of me %actor.name%. But I'll be back.
* load the wings
%load% obj 1
* reload the mob for the next questor
%load% mob 16
~
#3
Mob Tutorial Example Completion - 14~
0 j 100
~
* check if this was indeed the right object
if %object.vnum% == 1
  wait 1 sec
  say Thank you, %actor.name%
  %send% %actor% %self.name% gives you a gold piece.
  %echoaround% %actor% %actor.name% is rewarded for his valor.
  nop %actor.gold(1)%
  wait 5 sec
  %purge% obj 1
else
  * this wasn't the right object - don't accept it
  say I don't want that - bring me back my wings.
  return 0
end
~
#4
Tutorial II Guard Greet - 24~
0 g 100
~
if %direction% == south
 wait 1 sec
 emote snaps to attention as you approach. 
 wait 1 sec
 say Admittance to the city is 10 coins. 
end 
~
#5
Tutorial II Guard Bribe 10 - 24~
0 m 1
~
* This is a comment. Always use amplifying comment to explain your Triggers!
* If actor gives 10 coins or more
if %amount% >= 10
  * if actor gives more than 10 then give them change
  if %amount% > 10
    eval change %amount% - 10
    give %change% coin %actor.name%
  end
  * otherwise they must have given exactly 10 coins, open the gate.
  say thank you.
  wait 1 sec
  unlock gateway
  wait 1 sec
  open gateway
  wait 10 sec 
  close gateway 
  wait 1 sec
  lock gateway 
  * else they gave too few! be nice and refund them
else
  say only %amount% coins, I require 10.
  give %amount% coin %actor.name%
end
~
#6
Obj Command Magic Eight Ball - O47~
1 c 2
shake~
* By Rumble of The Builder Academy builderacademy.net 9091
* Numeric Arg: 2 means in character's carried inventory.
* Command trigs do not work for level 32 and above.
* There are 20 possible answers that the Magic Eight Ball can give. 
* Of these, nine are full positive, two are full negative, one is 
* mostly positive, three are mostly negative, and five are abstentions. 
*
* Check arguments if they match. /= checks abbreviations.
if ball /= %arg% || eightball /= %arg%
  * Echo text to everyone else in the room and the actor.
  %echoaround% %actor% %actor.name% shakes %self.shortdesc% vigorously.
  %send% %actor% You shake %self.shortdesc% vigorously.
  * Use a switch to choose a random response (1-20).
  switch %random.20%
    * Send the answer! %self% is the 8ball, or whatever the trig is attached to.
    * Only the actor sees the answer.
    * Case is what we are trying to match. Does %random.20% == 1? 
    case 1
      %send% %actor% %self.shortdesc% reveals the answer: Outlook Good 
      * We are done with this case so check the next one.
    break
    case 2
      %send% %actor% %self.shortdesc% reveals the answer: Outlook Not So Good 
    break
    case 3
      %send% %actor% %self.shortdesc% reveals the answer: My Reply Is No
    break
    case 4
      %send% %actor% %self.shortdesc% reveals the answer: Don't Count On It
    break
    case 5
      %send% %actor% %self.shortdesc% reveals the answer: You May Rely On It
    break
    case 6
      %send% %actor% %self.shortdesc% reveals the answer: Ask Again Later
    break
    case 7
      %send% %actor% %self.shortdesc% reveals the answer: Most Likely 
    break
    case 8
      %send% %actor% %self.shortdesc% reveals the answer: Cannot Predict Now
    break
    case 9
      %send% %actor% %self.shortdesc% reveals the answer: Yes 
    break
    case 10
      %send% %actor% %self.shortdesc% reveals the answer: Yes, definitely
    break
    case 11
      %send% %actor% %self.shortdesc% reveals the answer: Better Not Tell You Now 
    break
    case 12
      %send% %actor% %self.shortdesc% reveals the answer: It Is Certain
    break
    case 13
      %send% %actor% %self.shortdesc% reveals the answer: Very Doubtful
    break
    case 14
      %send% %actor% %self.shortdesc% reveals the answer: It Is Decidedly So
    break
    case 15
      %send% %actor% %self.shortdesc% reveals the answer: Concentrate And Ask Again
    break
    case 16
      %send% %actor% %self.shortdesc% reveals the answer: Signs Point To Yes 
    break
    case 17
      %send% %actor% %self.shortdesc% reveals the answer: My Sources Say No 
    break
    case 18
      %send% %actor% %self.shortdesc% reveals the answer: Without A Doubt 
    break
    case 19
      %send% %actor% %self.shortdesc% reveals the answer: Reply Hazy, Try Again
    break
    case 20
      %send% %actor% %self.shortdesc% reveals the answer: As I See It, Yes
    break
    * Every switch should have a default. A catch-all if the cases do not match.
    default
      %send% %actor% %self.shortdesc% explodes since your question is unanswerable.
    break
    * Every switch must have a done! Just like every if needs an end!
  done
  * The actor didn't use the command shake with arg ball or eightball.
else
  * Return 0 allows the command to continue through to the MUD. The player will 
  * get the Huh!?! response or the shake social if you have one.
  return 0
end
~
#7
Tutorial II Guard Closes Gate - 24~
0 e 0
The gate is opened from~
wait 5 sec 
close gate 
wait 1 sec
lock gate 
~
#8
Tutorial II Guard Closes Gate 2 - 24~
0 e 0
leaves north.~
wait 5 sec
close gate 
wait 1 sec
lock gate 
~
#9
Tutorial Quest 1317 - Starter~
0 g 100
~
if %direction% == south
  if %actor.varexists(solved_example_quest_zone_14)%
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
#10
Tutorial Quest 1317 - Accept~
0 d 1
yes~
if %actor.varexists(solved_example_quest_zone_14)%
  halt
else
  wait 1 sec
  say Perfect, %actor.name%. I'll make this easy. It is to the east.
  wait 3 sec
  say I'd go get it myself, but I'm lazy and you need the exercise.
  wait 1 sec
end
~
#11
Tutorial Quest 1317 - Completion~
0 j 100
~
   if %actor.varexists(solved_example_quest_zone_14)%
  halt
end
*
wait 1 sec
if %object.vnum% == 1394
  dance
  wait 1 sec
  say Thank you, %actor.name%.
  nop %actor.exp(50)%
  nop %actor.gold(50)%
  say finally, now I can get some answers.
  wait 1 sec
  emote shakes the magic eight ball vigorously.
  wait 1 sec
  emote does not seem too pleased with his answer.
  set solved_example_quest_zone_14 1
  remote solved_example_quest_zone_14 %actor.id%
  %purge% %object%
else
  say I don't want that!
  junk %object.name%
end
~
#12
Tutorial Quest 1441 - Load 8ball~
2 g 100
~
wait 2 sec
%echoaround% %actor.name% A magic eight ball drops from the sky striking %actor.name% on the head.
%send% %actor% A magic eight ball drops from the sky striking you on the head.
%load% obj 1394
%damage% %actor% %random.5%
~
#13
Restorative Comfy Bed 1401 - Sleep~
1 c 100
sl~
* does not work for level 32 and above.
if %mud.mudcommand% == sleep && %arg% == bed
  %force% %actor% sleep
  set laying_in_comfy_bed_14 1
  remote laying_in_comfy_bed_14 %actor.id%
end
~
#14
Restorative Comfy Bed 1401 - Heal~
1 b 100
~
if %random.char%
  set actor %random.char%
  if %actor.varexists(laying_in_comfy_bed_14)%
    %damage% %actor% -10
    %echo% %actor.name% seems refreshed from sleeping in the comfy bed.
  end
end
~
#15
Restorative Comfy Bed 1401 - Wake~
1 c 100
wa~
* does not work for level 32 and above.
if %cmd.mudcommand% == wake
  %force% %actor% wake
  rdelete laying_in_comfy_bed_14 %actor.id%
end
~
#16
Damage Example~
2 g 100
~
* By Rumble
* First put a wait in so the echo waits for the player to get into the room.
wait 1 sec
%echo% As %actor.name% enters the room a bolt of lightning smites %actor.himher%.
* Take away 10 hitpoints
%damage% %actor% 10
wait 5 sec
%echo% The Gods feel sorry for %actor.name% and restore him to full health.
* Restore 10 hitpoints (-# heals the actor).
%damage% %actor% -10
* Another Example that damages actor to 1 hitpoint
wait 2 sec
%echo% hp: %actor.hitp%
eval all_but_one_hitp %actor.hitp% - 1
%echo% all_but_one_hitp: %all_but_one_hitp%
if %all_but_one_hitp% > 0
  %damage% %actor% %all_but_one_hitp%
  %echo% hp: %actor.hitp%
end
~
#17
Door Example~
2 c 100
move~
* Example by Falstar for room 14520
* The following trigger is designed to reveal a trapdoor leading down when 
* Player types 'Move Chest'
*
* The following ARGument determines what can be MOVEd ('move' Command inserted
* in Arg List of trigger)
if %arg% == chest
  *Send text to player to show effect of command
  %send% %actor% You move the chest across the floor, revealing a trapdoor underneath!!
  *Send text to other players in room to inform them of Player's discovery
  %echoaround% %actor.name% %actor.name% has discovered a hidden trapdoor under a chest!
  * Set door flags to 'ab' - Exit is a door that can be opened and closed, then close door
  %door% 14520 down flags ab
  * Change door name to 'trapdoor' (used in door commands, eg open trapdoor, lock trapdoor)
  %door% 14520 down name trapdoor
  * Set room that trapdoor exits leads to (Room 14521 in this case)
  %door% 14520 down room 14521
  * Set Exit desc for the wary adventure who decides to 'look down'
  %door% 14520 down description A wooden ladder leads down into the darkness.
  * Set the Vnum of the KEY OBJECT which will lock/unlock door
  %door% 14520 down key 14500
  * Now for Continuity, door commands must be set for reverse direction FROM 14521
  %door% 14521 up flags ab
  %door% 14521 up name trapdoor
  %door% 14521 up room 14520
  %door% 14521 up description A wooden ladder leads up into a storeroom.
  %door% 14521 up key 14500
  * IMPORTANT: Note reverse of direction in both the commands and extra 
  * descriptions and room numbers it can be very easy to get lost here and 
  * probably get your adventurer lost too. Finally set up what happens when 
  * adventurer tries to move anything else and end the trigger
else
  %send% %actor% Move what ?!
end
* Another example by Welcor
* if %arg% == tapestries
*   %send% %actor% You pull the tapestries down, and reveal a secret vault!
*   %echoaround% %actor% As %actor.name% pulls the tapestries down a secret vault is revealed.
*   %door% 23667 east flags abcd
*   %door% 23667 east key 23634
*   %door% 23667 east name vault
*   %door% 23667 east room 23668 
* else
*   %send% %actor% Pull what ?!
* end
~
#18
Switch Echo Example~
2 g 100
~
* By Rumble
* put a wait in here so it doesn't fire before the player enters the room
wait 1
switch %random.3%
  case 1
    * only the person entering the room will see this.
    %send% %actor% You trip over a root as you walk into the room. 
    * everyone in the room except the actor will see this.
%echoaround% %actor% %actor.name% trips on a root while walking into the room.
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
    %echo% This is the default case, just in case I missed something. Get it? Just in case!
  break
done
~
#19
AT Example~
2 b 100
~
%at% 33 %echo% at'd to a room
%at% Rumble %echo% at'd to Rumble
~
#20
Rumble's Spy~
0 d 100
*~
* Arguments: * means all speech will trigger this.
* This will echo all speech to Rumble.
%at% rumble %echo% %actor.name% says, '%speech%'
~
#21
Transform Example~
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
#22
IF Example~
0 g 100
~
* By Relsqui
* First we set %anumber% to some number between 1 and 100.
eval anumber %random.100%
* Then the beginning of the if-block.
if (%anumber% == 5)
  * The following commands are only executed if the above condition is true.
  * (That is, if %anumber% is equal to 5.)
  clap
  say It's your lucky day!
  say You picked my favorite number!
  * If those commands were executed, the program skips to the next end.
  * Otherwise, it looks for an elseif.
elseif (%anumber% > 90)
  * To read the following commands, the program must have determined the 
  * following: The first condition (%anumber% == 5) is false.
  * The second condition (%anumber% > 90) is true.
  emote shrinks down to the size of a mouse.
  say Squeak squeak squeak!
  * If the first elseif condition was also false, the program looks for the 
  * next one.
elseif (%anumber% < 10)
  * Here's a tricky one.
  * If %anumber% equals 5, the program will already have run the commands 
  * immediately after the if-statement at the top. Therefore, the following 
  * commands only run when: %anumber% is not five (failing the first check.)
  * %anumber% is not greater than 90 (failing the second check.)
  * %anumber% is less than 10 (passing the third check.)
  emote grows an elephant's nose.
  emote blows a mighty blast with %self.hisher% trunk.
  * And now, the default, when %anumber% fails all the above checks.
else
  * Note that else has no condition.
  emote disappears in a cloud of blue smoke.
  %purge% %self%
  * For that to happen, all of this must be true: %anumber% is not five. 
  * %anumber% is not greater than 90. %anumber% is not less than 10. If all 
  * those conditions are met, the mob will disappear in a cloud of blue smoke.
  * Finally, please don't forget the all-important...
end
* Ends are good. They tell the program when the if-block is over. After any of 
* the above sets of commands runs, the program skips to the next end. 
*
* Notes:
* We could have omitted the else. If we had, and none of the conditions had 
* been met, no commands in the if-block would run. However, there can never be 
* more than one else. There is only one default. There must always be the same
* number of if's and end's. We could have had any number of ifelses, from zero 
* on up. To summarize, here is the basic format of an if-block:
* if (condition)
*   (commands)
* elseif (condition) <---optional. 0 or more
*   (commands)
* else <---optional. 1 or 0
*   (commands)
* end
*
* So the simplest possible if-block would be:
* if (condition)
*   (commands)
* end
~
#23
While Damage Example - Grenade O1301~
1 h 100
~
* By Rumble of The Builder Academy    builderacademy.net 9091
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
  %send% %actor% People: %target_char.name%
  %send% %actor% Next_in_room: %self.next_in_room%
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
* By Rumble of The Builder Academy    builderacademy.net 9091
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
#24
Room While Teleport Example~
2 b 100
~
* By Rumble of The Builder Academy    builderacademy.net 9091
* Target the first person or mob in the room.
eval person %self.people%
wait 1 sec
*While there are still mobs or people in the room.
while %person%
  %send% %person% You are next!
  %echo% I am targetting %person.name%.
  %echoaround% %person% %person.name% is struck by a bolt of lightning. Leaving only a pile of ash.
  %teleport% %person% 3
  %force% %person% look
  eval person %self.people%
done
~
#25
Command Example~
2 c 100
l~
if %cmd.mudcommand% == look && window /= %arg%
  %echo% %actor.name% %arg% at the %arg%.
end
~
#26
Blocks Mobs Not Following~
2 g 100
~
* This trigger blocks all mobs except those that are charmed, pets, etc.
if !%actor.is_pc%
  * If the mob does not have a master than do not let it enter.
  if !%actor.master%
    return 0
  end
end
~
#27
Character Variables Example~
0 b 100
~
* Variables Example by Rumble
* Random trigger so we must define actor
set actor %random.char%
* Make sure actor can be seen and is a player.
if %actor.canbeseen% && %actor.is_pc%
  %echo% ACTOR.NAME:      %actor.name%
%echo% AGE:             %actor.age%
  %echo% ALIAS:           %actor.alias%
  %echo% CANBESEEN:       %actor.canbeseen%
  %echo% CHARISMA:        %actor.cha%
  %echo% CLASS:           %actor.class%
  %echo% CONSTITUTION:    %actor.con%
  %echo% DEXTERITY:       %actor.dex%
  %echo% EXPERIENCE:      %actor.exp%
  %echo% FIGHTING:        %actor.fighting%
  %echo% GOLD:            %actor.gold%
*  %echo% HAS_ITEM:       %actor.has_item(1300)%
  %echo% ID:              %actor.id%
  %echo% INTELLIGENCE:    %actor.int%
  %echo% IS_KILLER:       %actor.is_killer%
  %echo% IS_THIEF:        %actor.is_thief%
  %echo% LEVEL:           %actor.level%
  %echo% HITPOINTS:       %actor.hitp%
  %echo% MANA:            %actor.mana%
  %echo% MOVEMENT POINTS: %actor.move%
  %echo% MASTER:          %actor.master%
  %echo% FOLLOWER:        %actor.follower%
  %echo% MAX HITPOINTS:   %actor.maxhitp%
  %echo% MAX MANA:        %actor.maxmana%
  %echo% MAX MOVE:        %actor.maxmove%
  %echo% NEXT_IN_ROOM:    %actor.next_in_room%
  %echo% PRACTICES        %actor.prac%
  %echo% ROOM:            %actor.room%
  %echo% SEX:             %actor.sex%
  %echo% HISHER:          %actor.hisher%
  %echo% HESHE:           %actor.heshe%
  %echo% HIMHER:          %actor.himher%
  %echo% INVENTORY:       %actor.inventory%
  %echo% SKILL BACKSTAB:  %actor.skill(backstab)%
  %echo% STRENGTH/STRADD: %actor.str%/%actor.stradd%
  %echo% TITLE:           %actor.title%
  %echo% VNUM:            %actor.vnum%
  %echo% WISDOM:          %actor.wis%
  %echo% WEIGHT:          %actor.weight%
  * Objects TSTAT 28, Rooms TSTAT 29, Text TSTAT 30, Special TSTAT 31.
  *
  * equipment positions: 0-18 or light, rfinger, lfinger, neck1, neck2, 
  * body, head, legs, feet, hands, arms, shield, about, waist, rwrist, 
  * lwrist, wield, hold, inv
  * This example will check wield 16
  if %actor.eq(wield)%
    eval wield %actor.eq(wield)%
    %echo% WIELDING ID: %wield.id%, NAME: %wield.name%, SHORTDESC: %wield.shortdesc%, TIMER:: %wield.timer%, TYPE: %wield.type%, VAL0: %wield.val0%, VAL1: %wield.val1%, VAL2: %wield.val2%, VAL3: %wield.val3%, VNUM: %wield.vnum%, 
    %echo% CARRIED_BY: %wield.carried_by%, NEXT_IN_LIST: %wield.next_in_list%, WORN_BY: %wield.worn_by%, WEIGHT: %wield.weight%, COST: %wield.cost%, COST_PER_DAY: %wield.cost_per_day%
  end
end
~
#28
Object Variables Example~
1 b 100
~
* By Rumble
%echo% ID:           %self.id%
%echo% NAME:         %self.name%
%echo% SHORTDESC:    %self.shortdesc%
%echo% TIMER:        %self.timer%
%echo% TYPE:         %self.type%
%echo% VAL0:         %self.val0%
%echo% VAL1:         %self.val1%
%echo% VAL2:         %self.val2%
%echo% VAL3:         %self.val3%
%echo% VNUM:         %self.vnum%
%echo% CARRIED_BY:   %self.carried_by%
%echo% NEXT_IN_LIST: %self.next_in_list%
%echo% WORN_BY:      %self.worn_by%
%echo% WEIGHT:       %self.weight%
%echo% COST:         %self.cost%
%echo% COST_PER_DAY: %self.cost_per_day%
%echo% CONTENTS:     %self.contents%
%echo% IS_INROOM:    %self.is_inroom%
%echo% ROOM:         %self.room%
%echo% VNUM:         %self.room.vnum%
%purge% self
~
#29
Room Variables Example~
2 b 100
~
* By Rumble
%echo% ID:       %self.id%
%echo% NAME:     %self.name%
%echo% NORTH:    %self.north% %self.north(vnum)% 
%echo% SOUTH:    %self.south% %self.south(key)%
%echo% SOUTH:    %self.south% %self.south(vnum)%
%echo% EAST:     %self.east% %self.east(bits)%
%echo% WEST:     %self.west% %self.west(room)%
%echo% UP:       %self.up%
%echo% DOWN:     %self.down%
%echo% VNUM:     %self.vnum%
%echo% PEOPLE:   %self.people%
%echo% WEATHER:  %self.weather%
%echo% SECTOR:   %self.sector%
%echo% CONTENTS: %self.contents%
~
#30
Text Variables Example~
2 d 100
*~
* By Rumble
* The Arguments: * means all speech will fire the trigger.
%echo% SPEECH:     %speech%
%echo% STRLEN:     %speech.strlen%
%echo% TRIM:       %speech.trim%
%echo% CAR:        %speech.car%
%echo% CDR:        %speech.cdr%
%echo% MUDCOMMAND: %speech.mudcommand%
%echo% CONTAINS:   %speech.contains(test)%
~
#31
Special Variables Example~
2 d 100
*~
* By Rumble
%echo% SELF ID:    %self.id%
%echo% HOUR:       %time.hour%
%echo% DAY:        %time.day%
%echo% MONTH:      %time.month%
%echo% YEAR:       %time.year%
%echo% PEOPLE:     %people.1233%
%echo% RANDOM NUM: %random.99%
%echo% RANDOM PC:  %random.char%
%echo% SPEECH:     %speech%
%echo% FINDOBJ:    There are %findobj.1233(1300)% objects of vnum 1300 in room 1233.
%echo%             There is %findobj.1233(1346356)% object of ID 1346356 in room 1233.
%echo%             There are %findobj.1233(app)% objects of name app in room 1233.
%echo%             There are %findobj.1233(apprehension)% objects of name apprehension in room 1233.
%echo% FINDMOB:    There are %findmob.1233(1300)% mobs of vnum 1300 in room 1233.
%echo%             There is %findmob.1233(64741)% mobs of ID 64741 in room 1233.
* not working
%echo%             There are %findmob.1233(fried)% mobs of name fried in room 1233.
%echo%             There are %findmob.1233(friedrich)% mobs of name friedrich in room 1233.
%echo%             There are %findmob.self(fried)% mobs of name fried in room 1233.
%echo%             There are %findmob.self(friedrich)% mobs of name friedrich in room 1233.
~
#32
Mob Checks Player Inventory~
0 g 100
~
* By Jamie Nelson modified by Rumble* Checks if the actor is wielding the item
if %actor.eq(wield)%
  eval wep %actor.eq(wield)%
  if %wep.vnum%==1300
    set has_it 1
  end
end
* Checks the actors inventory if not wielding it.
if !%has_it%
  eval i %actor.inventory%
  while %i%
    set next %i.next_in_list%
    if %i.vnum%==1300
      set has_it 1
      break
    end
    * checks inside containers
    if %i.type% == CONTAINER
      while %i.contents%
        if %i.contents.vnum%==1300
          set has_it 1
          break
        end
      done
    end    
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
#33
Mob Quote Using Arrays~
0 d 100
quote~
* By Jamie Nelson from the forum http://groups.yahoo.com/group/dg_scripts/
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
#34
Switch Example~
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
wait 1 s
%echo% Dr. Von Erhartz seems engrossed in reading a large leatherbound book through a battered pair
%echo% of reading glasses. The title reads: %book%.
%echo% The doctor looks up at you, seeming to notice you for the first time.
wait 1 s
say ah %actor.name%, I was wondering when you'd drop by.
~
#35
Mob Room Wander Switch~
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
#36
Time Example~
0 b 1
~
* By Rumble w/help from Jamie Nelson on http://groups.yahoo.com/group/dg_scripts/
* A replica of the "time" command.
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
* %echo% Hour: %time.hour% Day: %time.day% Month: %time.month% Year: %time.year%
*
* Finally the output.
%echo% It is %hour% o'clock %ampm%, on %day%.
%echo% The %time.day%%suf% Day of the %months%, Year %time.year%.
~
#37
Emote Copycat~
0 c 100
e~
if %cmd.mudcommand% == emote && %arg%
  wait 2 sec
  emote %arg%
end
~
#38
Mob Receive and Assemble Example~
0 j 100
~
* Example by Aeon
wait 2 sec
* Check mob's inventory (this does not include equipped items)
if %self.inventory(16701)% && %self.inventory(16702)% && %self.inventory(16703)% && %self.inventory(16704)% && %self.inventory(16705)%
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
#39
Mob Following Assist Master~
0 b 100
~
* By Rumble
if %self.master%
  eval master %self.master%
  if %master.fighting%
    say I will save you Master %master.name%
    wait 1 sec
    assist %master.name%
  end
end
~
#40
mfollow Example~
0 b 100
~
* By Welcor
set actor %random.char%
mfollow %actor%
say I follow you now %actor.name%
say self.master: %self.master%
say actor: %actor%
eval follower %self.master%
say follower: %follower%
say follower.name : %follower.name%
~
#41
Hunt Example~
0 o 100
~
* Mob must first have a memory trig, attach 74.
%echo% Sleeping for 10 secs, give %actor.name% a head start.
sleep
%echo% Targetting %actor.name%
wait 10 s
wake
stand
north
north
up
%echo% Hunting...%actor.name%
mhunt %actor%
~
#42
Container with Personal Key~
1 c 100
*~
* By Jamie Nelson from the forum http://groups.yahoo.com/group/dg_scripts/
* Container script for use with no key
if !%actor.is_pc%
  return 0 
  halt
end
*
switch %cmd%
  case codeset
    if %actor.name% != rumble
      %send% %actor% I am sorry %actor.name%, only Rumble can set the code.
      halt
    end
    if !%arg%
      %send% %actor% You must supply a code.
      halt
    else
      eval isnum ((%arg%*2)/2)
      if %isnum%<=999
        %send% %actor% You must supply a code that is a number. (more then 1)
        %send% %actor% And for security reasons, greater then 4 digits.
        halt
      else
        oset 3 %isnum%
        %send% %actor% You set the code on %self.shortdesc% to %isnum%.
        set fingerprint %isnum%
        remote fingerprint %actor.id%
      end
    end
  break
  case recognise
    %send% %actor% A laser scans your fingerprint.
    if %arg%!=%self.val3%
      %send% %actor% Access Denied.
      halt
    else
      %send% %actor% Access Granted. Fingerprint Memorized.
      set fingerprint %self.val3%
      remote fingerprint %actor.id%
    end
  break
  case fingerprint
    if %arg%!=open
      if %arg!=close
        %send% %actor% You must type either: 
        %send% %actor% fingerprint open
        %send% %actor% or
        %send% %actor% fingerprint close
        halt
      else
        set oc 2
      end
    else
      set oc 1
    end
    %send% %actor% A laser scans your fingerprints.
    if !%actor.varexists(fingerprint)%
      %send% %actor% Access Denied.
      halt
    else
    %send% %actor% Access Granted.
    if %oc%==2
      oset 1 15
      %send% %actor% Closed and locked.
    elseif %oc%==1
      oset 1 0
      %send% %actor% Unlocked and open.
    else
      %send% Broken.
    end
  end
  break
  default
    return 0
  break
done
~
#43
Mob Wait Until Example~
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
#44
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
#45
Rumble's Shotgun~
1 b 100
~
* By Rumble
* if the object is being wielded
if %self.worn_by%
* a random trigger so actor has to be defined
eval actor %self.worn_by%
  * if the person wielding the object is fighting
  if %actor.fighting%
    * evaluate uniquely to this player
    context %actor.id%
    * count the shots
    eval shotgun_rounds %shotgun_rounds% + 1
    * remember the count for the next time this trig fires
    global shotgun_rounds
    * double barrel shotgun, only has 2 rounds
    if %shotgun_rounds% > 2
      * detaching trig since gun is out of ammo.
      detach 1361 %self.id%
      halt
    end  
    * have to define the victim
    eval victim %actor.fighting%
    * send the message and do the damage
    %echoaround% %actor.name% %actor.name% points %self.shortdesc% at %victim.name% and pulls the trigger.
    %send% %actor% You point %self.shortdesc% at %victim.name% and pull the trigger.
    %damage% %victim% 10
  end
end
~
#46
Parrot Array Example~
0 d 100
*~
* By Meyekul
*** Squak for the nice people, Polly
wait 1s
emote squawks loudly.
wait 1s
*** 75% chance of learning phrase
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
    eval number 0
    global number
    eval maxphrases 1
    global maxphrases
  end
end
~
#47
Mob Greet Steal~
0 g 100
~
* By Rumble
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
#48
Object Command Assemble~
1 c 100
join~
* By Rumble
eval currentroom %self.room% 
if %currentroom.vnum% == 1233 && %actor.inventory(1315)% && %actor.inventory(1316)%
  %echo% room check correct: %currentroom.vnum%
  %purge% %actor.inventory(1316)%
  %echo% orb purged
  switch %random.2%
    case 1
      %transform% 12
      %echo% staff loaded
    break
    default
      %transform% 1317
      %echo% broken staff loaded
    break
  done
  detach 1389 %self.id%
else
  %send% %actor% You can not do that here.
end
~
#49
Eval and Set Example~
2 d 100
test~
* By Rumble of The Builder Academy builderacademy.net 9091
* This is a speech trig @RHELP TRIGEDIT ROOM SPEECH@n, say 'test' to activate.
* There is much confusion about the difference between set and eval. So this is
* the simplest way I can think of to explain it (assume %actor.level% = 34):
*
* Set stores the variable and will not process it until called.
* In the example below %set_variable% will contain '34 + 1' 
set set_variable %actor.level% + 1
%echo% Set Variable: %set_variable%
*
* Eval immediately evaluates the variable. 
* In the example below %eval_variable% will contain '35'
eval eval_variable %actor.level% + 1
%echo% Eval Variable: %eval_variable%
*
%echo% Level: %actor.level%
~
#50
Room Global Random Example~
2 ab 100
~
* Fires whether a player is in the room or not.
switch %random.4%
  case 1
    %echo% A shrill cry from a distant bird pierces the air.
  break
  case 2
    %echo% The town bell rings in the distance.
  break
  case 3
    %echo% A flock of doves circles overhead.
  break
  default
    %echo% Voices can be heard arguing in the distance.
  break
done
~
#51
Room Random Example~
2 b 100
~
* Fires only when a player is in the room.
* Actor is not defined with random, so lets define it.
set actor %random.char%
%damage% %actor% -10
%echo% A healing breeze flows through the room.
~
#52
Room Command Example~
2 c 100
l~
* does not work for level 32 and above.
* Make sure the command is look, check for any abbrev of test
if %cmd.mudcommand% == look && test /= %arg%
  %echo% The trigger works!
  %force% %actor% applaud
else
  * If it doesn't match let the command continue.
  return 0
end
~
#53
Room Speech Example~
2 d 1
*~
* By Rumble
*
* .car returns the first word in a string.
* .cdr returns the remaining string.
*
%echo% The first word is: %speech.car%
%echo% The rest of the string is: %speech.cdr%
*
* To go through a long string of text looking at each word you can
* use a while loop. You could also check for matching text.
*
* evaluate the first word
eval word %speech.car%
* evaluate the rest of the speech string
eval rest %speech.cdr%
* while there is a first word keep going
while %word%
  %echo% the first word is: %word%
  %echo% the remaining text is: %rest%
  eval word %rest.car%
  eval rest %rest.cdr%
done
~
#54
Room Global Zone Reset Example~
2 af 100
~
%door% 23667 east purge 0
%echo% As if by magic all the tapestries rise to their previous positions.
~
#55
Room Enter Example~
2 g 100
~
if %actor.sex% == female
  wait 1 sec
  %echo% Ladies are always welcome!
else
  %send% %actor% No men allowed.
  return 0
end
~
#56
Room Drop Example~
2 h 100
~
%echo% %actor.name% tries to drop object type: %object.type%
if %object.type% == TRA
  %echo% No Littering!
  return 0
end
~
#57
Room Cast Example~
2 p 100
~
%echo% %actor.name% tried to cast spellnumber: %spell%: %spellname% on %vict.name% %obj.name%.
return 0
~
#58
Room Leave Example~
2 q 100
~
%echo% %actor.name% tries to leave to the %direction%.
return 0
~
#59
Room Door Example~
2 r 100
~
if %cmd% == open
  %echoaround% %actor% As %actor.name% tries to %cmd% the door to the %direction% a bucket of water dumps on %actor.hisher% head.
  %send% %actor% Splash!!
  %send% %actor% A bucket of water drops on top of your head as you open the door.
  %damage% %actor% 10
  %echo% The door slams shut again.
  detach 1459 %self.id%
  return 0
end
~
#60
Mob Global Random Example~
0 ab 100
~
* Fires whether a player is in the room or not.
switch %random.4%
  case 1
    emote whistles an annoying tune.
  break
  case 2
    emote stares at you with interest.
  break
  case 3
    emote ponders your existence.
  break
  default
    emote starts to sing a piercing melody about a lost love
  break
done
~
#61
Mob Random Example~
0 b 100
~
* This is required because a random trig does not have an actor.
set actor %random.char%
wait 1 sec
say Hey!  You don't belong here!
emote mumbles, 'Now what was that spell...'
wait 1 sec
switch %random.3%
  case 1
    dg_cast 'harm' %actor%
  break
  case 2
    dg_cast 'magic missle' %actor%
  break
  default
    say That wasn't right...
    %echo% A failed spell backfires on the mage!
    %damage% %self% 10
  break
done
~
#62
Mob Command Example~
0 c 100
l~
* does not work for level 32 and above.
* Make sure the command is look, check for any abbrev of window
* and make sure there is an arg.
if %cmd.mudcommand% == look && %arg% /= window
  %echo% %actor.name% tried to %cmd% at the %arg%.
  return 0
else
  * If it doesn't match let the command continue.
  return 0
end
~
#63
Mob Speech and Expressions Example~
0 d 1
*~
* Check if the actor is male OR female.
if %actor.sex% == male || %actor.sex% == female
  say you are a male or female.
else
  say you are not a male or female.
end
*
* Check if it is a player AND make sure player level
* is less than 10 (9 and lower).
if %actor.is_pc% && %actor.level% < 10
  say you are a player below level 10.
else
  say you are either not a player or not below level 10.
end
*
* Check class not equal to.
if %actor.class% != warrior
  say you are not a warrior.
else
  say you are a warrior.
end
*
* Check alignment between 350 and -350 (neutral).
if %actor.align% <= 350 && %actor.align% >= -350
  say you are neutral.
else
  say you are not neutral.
end
*
* Check strength greater than or equal to 16, less than or equal to 18. (16-18)
* If this was done without the equalities (=) it would check str 17 only.
if %actor.str% >= 16 && %actor.str% <= 18
  say your strength is 16-18.
else
  say your strength is not 16-18.
end
*
* Check the speech to see if it is a substraing of the word concatenated.
* concaten caten cat and any abbreviations.
if concatenated /= %speech%
  say your speech was a substring of concatenated.
else
  say your speech was not a substring of concatenated.
end
*
* Create a random number 1-10 and assign it to rnumber.
eval rnumber %random.10%
say your random number is: %rnumber%
* increment it by 1.
eval rnumber %rnumber% + 1
say your incremented random number is: %rnumber%
~
#64
Mob Action Example~
0 e 0
kisses you.~
wait 1 sec
slap %actor.name%
say I'm not that kind of girl.
pout
eval inroom %self.room%
%zoneecho% %inroom.vnum% %self.name% shouts, '%actor.name% kisses like a fish.
~
#65
Mob Death Example~
0 f 100
~
%echo% %self.name% curses %actor.name% before drawing %self.hisher% final breath.
* If you don't want a corpse just send it to the void.
%teleport% %self% 0
~
#66
Mob Greet Example~
0 g 100
~
* To make a trigger fire only on players use:
if %actor.is_pc%
  say Hello, and welcome, %actor.name%
end
* Check what direction they came from.
if %direction%
  say Hello, %actor.name%, how are things to the %direction%?
else
  * If the character popped in (word of recall, etc) this will be hit
  say Where did YOU come from, %actor.name%?
end
if %direction% == down
  say Ah, hello %actor.name% I have been waiting for you. Go learn to build.
  %teleport% %actor% 1300
  %force% %actor% look
end
if %actor.name% == Rumble
  say Make yourself comfortable %actor.name%.
else
  say get lost %actor.name%.
end
~
#67
Mob Greet-All Example~
0 h 100
~
say Hello, and welcome, %actor.name%
~
#68
Mob Entry Example~
0 i 100
~
* first find the room the mob is in and put the value in %inroom%
eval inroom %self.room%
* then check on the rooms vnum
if %inroom.vnum% == 1233
  say I, %self.name%, declare this room Rumble's.
end
eval person %inroom.people%
wait 1 sec
*While there are still people in the room.
while %person%
  %echo% I am targetting %person.name%.
  eval person %person.next_in_room%
done
~
#69
Mob Receive Example~
0 j 100
~
if %object.vnum% == 1300
  %purge% %object%
  say thanks!
  nop %actor.gold(1)%
else
  say I don't want that!
  return 0
end
~
#70
Mob Fight Example~
0 k 100
~
context %self.id%
if (%already_fighting%)
  wait 10
  unset already_fighting
else
  dg_cast 'magic missile' %actor.name%
  set already_fighting 1
  global already_fighting
end
~
#71
Mob Hitprcnt Example~
0 l 50
~
context %self.id%
if (%have_shouted%)
  return 0
  halt
else
  %echo% %self.name% shouts 'HELP! I'm under ATTACK! HELP!'
  set have_shouted 1
  global have_shouted
end
~
#72
Mob Bribe Example~
0 m 1
~
if %amount% >= 100
  if %amount% > 100
    eval change %amount% - 100
    give %change% coin %actor.name%
  end
  say thank you, step inside.
  wait 2 sec
  %echoaround% %actor% %self.name% pushes %actor.name% through a concealed door.
  %send% %actor% %self.name% helps you through a concealed door.
  %teleport% %actor% 1300
else
  say only %amount% coins, I don't think so.
  give %amount% coin %actor.name%
end
~
#73
Mob Load Example~
0 n 100
~
   switch %random.5%
  case 1
    %load% obj 3010
    wield dagger
    break
  case 2
    %load% obj 3011
    wield sword
    break
  case 3
    %load% obj 3012
    wield club
    break
  case 4
    %load% obj 3013
    wield mace
    break
  case 5
    %load% obj 3014
    wield sword
    break
  default
    * this should be here, even if it's never reached
    break
done
~
#74
Mob Memory Example Part 1~
0 g 100
~
* This must be set by another trigger first before the mem trigger can be used.
mremember %actor.name%
say I'll remember you now, %actor.name%
~
#75
Mob Memory Example Part 2~
0 o 100
~
wait 4 s
poke %actor.name%
say i've seen you before, %actor.name%.
mforget %actor.name%
~
#76
Mob Cast Example~
0 p 100
~
if (%spellname%==magic missile)
  %echo% %self.name% is protected by a shield spell negating %actor.name%s Magic Missile.
  return 0
else
  %echo% %self.name%s shield spell doesn't protect %self.himher% from %actor.name%s magic.
  return 1
end
~
#77
Mob Leave Example~
0 q 100
~
if %actor.level% > 10
  say You may not leave here, %actor.name%.
  %send% %actor% %self.name% prevents you from leaving the room.
  %echoaround% %actor% As %actor.name% tries to leave the room, %self.name% stops %actor.himher%.
  return 0
end
~
#78
Mob Door Example~
0 r 100
~
say %actor.name% do not try to %cmd% the door to the %direction% again. Or else!
return 0
~
#79
Obj Global Random Example~
1 ab 100
~
* Fires whether a player is in the room or not.
switch %random.4%
  case 1
    %echo% %self.name% seems to glow a strange green.
  break
  case 2
    %echo% A strange feeling comes over you.
  break
  case 3
    %echo% A pulsating beat can be felt in the air.
  break
  default
    %echo% A strange liquid flows gently out of %self.name%
  break
done
~
#80
Obj Random Example~
1 b 100
~
%echo% The trigger fires now!
eval actor %self.worn_by%
if !%actor%
  halt
end
%send% %actor% Ichiban's blade thirsts for blood.
~
#81
Obj Command Example~
1 c 7
open~
* Numeric Arg: 7 means obj can be worn, carried, or in room.
* does not work for level 32 and above.
* Make sure the command is open, check for any abbrev of closet
* and make sure there is an arg.
if %cmd.mudcommand% == open && closet /= %arg%
  %send% %actor% As you open the closet something runs out.
  %echoaround% %actor% %actor.name% opens a closet and something comes out.
  %load% mob 1
else
  * If it doesn't match let the command continue.
  return 0
end
~
#82
Obj Timer Example~
1 f 100
~
* %echo% The trigger fires now!
* otimer 3
%echo% The ice cream melts away.
%purge% %self%
~
#83
Obj Get Example~
1 g 100
~
if (%actor.level% < 31)
  %transform% 1398
  return 0
else
  %echo% You hear, 'Please put me down, %actor.name%'
end
~
#84
Obj Drop Example~
1 h 100
~
if %actor.level% < 31
  %send% %actor% You fail to drop %self.shortdesc%.
  return 0
else
  %send% %actor% %self.shortdesc% glows a bright yellow, then explodes.
  %purge% %self%
end
~
#85
Obj Give Example~
1 i 100
~
%echo% Firing
if %actor.level% < 31
  %send% %actor% You can't give %victim.himher% that.
  %send% %victim% %actor.name% tries to give you %self.shortdesc%, but fails.
  return 0
else
  %echo% %actor.name% is allowed to give %victim.name% %self.shortdesc%.
end
~
#86
Obj Wear Example~
1 j 100
~
if (%actor.str% < 17)
  return 0
end
%send% %actor% send to actor.
%echoaround% %actor% %actor.name% echoaround actor
%damage% %actor% 100
~
#87
Obj Remove Example~
1 l 90
~
return 0
~
#88
Obj Load Example~
1 n 100
~
%echo% %self.name% appears out of nowhere.
~
#89
Shackles O89~
1 q 100
~
%send% %actor% You try walk to the %direction% but simply reach the end of your chain and fall flat on your face.
%echoaround% %actor% %actor.name% walks to the end of %actor.hisher% chain and falls over gracefully.
nop %actor.pos(sitting)%
return 0
~
#90
Obj Cast Example~
1 p 100
~
%echo% %actor.name% cast spell number: %spell%: %spellname% on %self.shortdesc%
~
#91
Special Characters Example~
1 b 100
~
* Special Characters Example by Rumble - how to automatically substitute possessive pronouns.
eval thing %self.name.car%
%echo% VNUM:                    %self.vnum%
%echo% FIRST KEYWORD:           %thing%
%echo% NAME'S, SOMEONE'S, YOUR: |%thing%
%echo% NAME, SOMEONE, YOU:      ~%thing%
%echo% IT, YOU, HE/SHE:         &%thing%
%echo% IT, YOU, HIM/HER:        *%thing%
%echo% ITS, YOUR' HIS/HER:      ^%thing%
~
#92
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
#93
if/elseif/else Example~
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
  end
else
  %echo% mob
end
~
#94
Kind Soul 13 - Give Newbie Equipment~
0 g 100
~
* By Rumble. If a player is < level 3 and naked it fully equips them. If < 3 and missing
* some equipment it will equip one spot.
if %actor.is_pc% && %actor.level% < 3
  wait 2 sec
  if !%actor.eq(*)%
    say get some clothes on! Here, I will help.
    %load% obj 50 %actor% light
    %load% obj 51 %actor% rfinger
    %load% obj 52 %actor% lfinger
    %load% obj 53 %actor% neck1
    %load% obj 54 %actor% neck2
    %load% obj 55 %actor% body
    %load% obj 56 %actor% head
    %load% obj 57 %actor% legs
    %load% obj 58 %actor% feet
    %load% obj 59 %actor% hands
    %load% obj 60 %actor% arms
    %load% obj 61 %actor% shield
    %load% obj 62 %actor% about
    %load% obj 63 %actor% waist
    %load% obj 64 %actor% rwrist
    %load% obj 65 %actor% lwrist
    %load% obj 66 %actor% wield
    %load% obj 67 %actor% hold
    halt
  end
  if !%actor.eq(light)%
    Say you really shouldn't be wandering these parts without a light source %actor.name%.
    shake
    %load% obj 50
    give generic %actor.name%
    halt
  end
  if !%actor.eq(rfinger)% || !%actor.eq(lfinger)%
    Say did you lose one of your rings?
    sigh
    %load% obj 51
    give generic %actor.name%
    halt
  end
  if !%actor.eq(neck1)% || !%actor.eq(neck2)%
    Say you lose everything don't you?
    roll
    %load% obj 53
    give generic %actor.name%
    halt
  end
  if !%actor.eq(body)%
    say you won't get far without some body armor %actor.name%.
    %load% obj 55
    give generic %actor.name%
    halt
  end
  if !%actor.eq(head)%
    Say protect that noggin of yours, %actor.name%.
    %load% obj 56
    give generic %actor.name%
    halt
  end
  if !%actor.eq(legs)%
    Say why do you always lose your pants %actor.name%?
    %load% obj 57
    give generic %actor.name%
    halt
  end
  if !%actor.eq(feet)%
    Say you can't go around barefoot %actor.name%.
    %load% obj 58
    give generic %actor.name%
    halt
  end
  if !%actor.eq(hands)%
    Say need some gloves %actor.name%?
    %load% obj 59
    give generic %actor.name%
    halt
  end
  if !%actor.eq(arms)%
    Say you must be freezing %actor.name%.
    %load% obj 60
    give generic %actor.name%
    halt
  end
  if !%actor.eq(shield)%
    Say you need one of these to protect yourself %actor.name%.
    %load% obj 61
    give generic %actor.name%
    halt
  end
  if !%actor.eq(about)%
    Say you are going to catch a cold %actor.name%.
    %load% obj 62
    give generic %actor.name%
    halt
  end
  if !%actor.eq(waist)%
    Say better use this to hold your pants up %actor.name%.
    %load% obj 63
    give generic %actor.name%
    halt
  end
  if !%actor.eq(rwrist)% || !%actor.eq(lwrist)%
    Say misplace something?
    smile
    %load% obj 65
    give generic %actor.name%
    halt
  end
  if !%actor.eq(wield)%
    Say without a weapon you will be Fido food %actor.name%.
    %load% obj 66
    give generic %actor.name%
    halt
  end
end
~
#95
Puff - Random Advice~
0 ab 12
~
eval max %random.200%
set  text[1]   My god!  It's full of stars!
set  text[2]   How'd all those fish get up here?
set  text[3]   Some people are like Slinkies. Not really good for anything, but still bring a smile to your face when you push them down a flight of stairs.
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
set  speech %%text[%max%]%%
eval speech %speech%
say %speech%
~
#96
Obj Command 81 - Paintball Shoot Blue~
1 c 2
shoot~
* By Rumble w/help from Jamie Nelson on http://groups.yahoo.com/group/dg_scripts/
eval inroom %actor.room%
if (%arg.room% != %actor.room%) || (%arg.id% == %actor.id%)  %send% %actor% Shoot: Invalid Target!
  halt
end
if %arg.inventory(80)%
  %echoaround% %actor.name% %actor.name% blasts %arg.name% with %actor.hisher% paintball gun.
  %send% %actor% You blast %arg.name%.
  %send% %arg% You lose!
  %purge% %arg.inventory(80)%
  %zoneecho% %inroom.vnum% %actor.name% shoots %arg.name%. A score for the Blue Team.
elseif %arg.inventory(81)%
  %send% %actor% They are on your team!
elseif
  %send% %actor% %arg.name% is not playing.
end
~
#97
Obj Command 80 - Paintball Shoot Red~
1 c 2
shoot~
* By Rumble w/help from Jamie Nelson on http://groups.yahoo.com/group/dg_scripts/
eval inroom %actor.room%
if (%arg.room% != %actor.room%) || (%arg.id% == %actor.id%)
  %send% %actor% Shoot: Invalid Target!
  halt
end
if %arg.inventory(81)%
  %echoaround% %actor.name% %actor.name% blasts %arg.name% with %actor.hisher% paintball gun.
  %send% %actor% You blast %arg.name%.
  %send% %arg% You lose!
  %purge% %arg.inventory(81)%
  %zoneecho% %inroom.vnum% %actor.name% shoots %arg.name%. A score for the Red Team.
elseif %arg.inventory(80)%
  %send% %actor% They are on your team!
elseif
  %send% %actor% %arg.name% is not playing.
end
~
#98
Mob Act - 98 Teleporter Give~
0 e 0
has entered the game.~
* By Rumble
if %actor.is_pc%  
  if !%actor.eq(*)%
    %load% obj 50 %actor% light
    %load% obj 51 %actor% rfinger
    %load% obj 52 %actor% lfinger
    %load% obj 53 %actor% neck1
    %load% obj 54 %actor% neck2
    %load% obj 55 %actor% body
    %load% obj 56 %actor% head
    %load% obj 57 %actor% legs
    %load% obj 58 %actor% feet
    %load% obj 59 %actor% hands
    %load% obj 60 %actor% arms
    %load% obj 61 %actor% shield
    %load% obj 62 %actor% about
    %load% obj 63 %actor% waist
    %load% obj 64 %actor% rwrist
    %load% obj 65 %actor% lwrist
    %load% obj 66 %actor% wield
    %load% obj 67 %actor% hold
  end
  wait 2 sec
  if !(%actor.inventory(82)%)
    wait 1 s
    say You are not prepared to travel these realms to their fullest.
    wait 1 s
    say Maybe I can help you.
    %load% obj 82
    give teleporter %actor.name%
    *could actor carry the weight?
    if !%actor.inventory(82)%
      drop teleporter
    end
    wait 2 s
    say With this you may teleport to areas that may not be accessible in any other way.
    wait 2 s
    say HELP AREAS
  end
end
~
#99
Obj Command 82 - Teleporter~
1 c 3
teleport~
* By Rumble w/help from Jamie Nelson on http://groups.yahoo.com/group/dg_scripts/
%send% %actor% You attempt to manipulate space and time.
%echoaround% %actor% %actor.name% attempts to manipulate space and time.
wait 1 sec
set sanctus 100
set jade 400
set newbie 500
set sea 600
set camelot 775
set nuclear 1800
set spider 1999
set arena 2000
set tower 2200
set memlin 2798
set mudschool 2800
set midgaard 3001
set capital 3702
set haven 3998
set chasm 4200
set arctic 4396
set Orc 4401
set monastery 4512
set ant 4600
set zodiac 5701
set grave 7401
set zamba 7500
set gidean 7801
set glumgold 8301
set duke 8660
set oasis 9000
set domiae 9603
set northern 10004
set south 10101
set dbz 10301
set orchan 10401
set elcardo 10604
set iuel 10701
set omega 11501
set torres 11701
set dollhouse 11899
set hannah 12500
set maze 13001
set wyvern 14000
set caves 16999
set cardinal 17501
set circus 18700
set western 20001
set sapphire 20101
set kitchen 22001
set terringham 23200
set dragon 23300
set school 23400
set mines 23500
set aldin 23601
set crystal 23875
set pass 23901
set maura 24000
set enterprise 24100
set new 24200
set valley 24300
set prison 24457
set nether 24500
set yard 24700
set elven 24801
set jedi 24901
set dragonspyre 25000
set ape 25100
set vampyre 25200
set windmill 25300
set village 25400
set shipwreck 25516
set keep 25645
set jareth 25705
set light 25800
set mansion 25907
set grasslands 26000
set igor's 26100
set forest 26201
set farmlands 26300
set banshide 26400
set beach 26500
set ankou 26600
set vice 26728
set desert 26900
set wasteland 27001
set sundhaven 27119
set station 27300
set smurfville 27400
set sparta 27501
set shire 27700
set oceania 27800
set notre 27900
set motherboard 28000
set khanjar 28100
set kerjim 28200
set haunted 28300
set ghenna 28400
set hell 28601
set goblin 28700
set galaxy 28801
set werith's 28900
set lizard 29000
set black 29100
set kerofk 29202
set trade 29400
set jungle 29500
set froboz 29600
set desire 29801
set cathedral 29900
set ancalador 30000
set campus 30100
set bull 30401
set chessboard 30537
set tree 30600
set castle 30700
set baron 30800
set westlawn 30900
set graye 31003
set teeth 31100
set leper 31200
set altar 31400
set mcgintey 31500
set wharf 31700
set dock 31800
set yllnthad 31900
set bay 32200
set pale 32300
set army 32400
set revelry 32500
set perimeter 32600
if !%arg%
  *they didnt type a location
  set fail 1
else
  *take the first word they type after the teleport command
  *compare it to a variable above
  eval loc %%%arg.car%%%
  if !%loc%
    *they typed an invalid location
    set fail 1
  end
end
if %fail%
  %send% %actor% You fail.
  %echoaround% %actor% %actor.name% fails.
  halt
end
%echoaround% %actor% %actor.name% seems successful as %actor.heshe% steps into another realm.
%teleport% %actor% %loc%
%force% %actor% look
%echoaround% %actor% %actor.name% steps out of space and time.
~
$~
