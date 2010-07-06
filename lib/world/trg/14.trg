#1400
free~
0 g 100
~
* we don't want him to tell this to mobs.
if %actor.is_pc%
  * only greet players coming from the south.
  if %direction% == south
    *wait 1 second, always give the player time before you start sending text.
    wait 1 sec
    say Can you help me, %actor.name%?
    wait 1 sec
    say An apprehensive ogre has something of mine.
    wait 1 sec
    say If you slay him I'll give you all the coins I can spare.
    wait 1 sec
    say Please, bring me the apprehension he has stolen.
    wait 2 sec
    emote looks to the east.
  end
end
~
#1401
free~
0 f 100
~
say you got the best of me %actor.name%.
* load some apprehension
%load% obj 1300
* reload the mob for the next questor
%load% mob 1311
~
#1402
free~
0 j 100
~
* check if this was indeed the right object
if %object.vnum% == 1300
  wait 1 sec
  say Thank you, %actor.name%
  %send% %actor% %self.name% gives you a gold piece.
  %echoaround% %actor% %actor.name% is rewarded for his valor.
  nop %actor.gold(1)%
  wait 5 sec
  %purge% %object%
else
  * this wasn't the right object - don't accept it
  say I don't want that - bring me back my apprehension.
  return 0
end
~
#1403
free~
0 n 100
~
wait 3 sec
say I'm back. Did you really think you could kill me so easily?
~
#1404
free~
0 g 100
~
if %direction% == south
  wait 1 sec
  emote snaps to attention as you approach.
  wait 1 sec
  say Admittance to the city is 10 coins.
end
~
#1405
free~
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
  unlock gate 
  wait 1 sec
  open gate 
  wait 10 sec 
  close gate 
  wait 1 sec
  lock gate 
  * else they gave too few! be nice and refund them
else
  say only %amount% coins, I require 10.
  give %amount% coin %actor.name%
end
~
#1406
free~
0 m 1
~
* No Script
~
#1407
free~
0 e 0
The gate is opened from~
wait 5 sec 
close gate 
wait 1 sec
lock gate 
~
#1408
free~
0 e 0
leaves north.~
wait 1 sec
close gate 
wait 1 sec
lock gate 
~
#1409
free~
0 g 100
~
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
#1410
free~
0 d 1
yes~
if %actor.varexists(solved_example_quest_zone_0)%
  halt
else
  wait 1 sec
  say Perfect, %actor.name%. I'll make this easy. It is to the east.
  wait 3 sec
  say I'd go get it myself, but I'm lazy and you need the exercise.
  wait 1 sec
end
~
#1411
free~
0 j 100
~
wait 1 sec
if (%object.vnum% == 47) && !%actor.varexists(solved_example_quest_zone_0)%
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
#1412
free~
2 g 100
~
wait 2 sec
%echoaround% %actor% A magic eight ball drops from the sky striking %actor.name% on the head.
%send% %actor% A magic eight ball drops from the sky striking you on the head.
%load% obj 47
%damage% %actor% %random.5%
~
#1413
Obj Command Restorative Comfy Bed Sleep - 1401~
1 c 4
sl~
if %cmd.mudcommand% == sleep && bed /= %arg%
  %force% %actor% sleep
  set laying_in_comfy_bed_14 1
  remote laying_in_comfy_bed_14 %actor.id%
  %send% %actor% The bed is extremely comfortable.
else
  return 0
end
~
#1414
Obj Random Restorative Comfy Bed - 1401~
1 b 100
~
if %random.char%
  set actor %random.char%
  if %actor.varexists(laying_in_comfy_bed_14)%
    %damage% %actor% -10
    %echoaround% %actor% %actor.name% seems refreshed from sleeping in the comfy bed.
    %send% %actor% You dream peacefully and seem magically refreshed.
  end
end
~
#1415
Obj Command Restorative Comfy Bed Wake - 1401~
1 c 4
wa~
if %cmd.mudcommand% == wake && %actor.varexists(laying_in_comfy_bed_14)%
  %force% %actor% wake
  rdelete laying_in_comfy_bed_14 %actor.id%
  %send% %actor% You sit on the edge of the bed feeling much better.
  %echoaround% %actor% %actor.name% sits up on the edge of the bed looking refreshed.
else
  return 0
end
~
#1416
free~
0 q 100
~
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
#1417
Attach Example~
2 b 100
~
%echo% This trigger commandlist is not complete!
%at% 1233 %echo% %self.vnum% %self.id%
%echo% %self.vnum% %self.id%
~
#1418
Command Draw~
1 c 100
draw~
* this trigger is meant for a card table or something similiar
set CARDOBJ 700
eval inroom %self.room%
* find what room the object is in.
eval obj %inroom.contents%
* find the first object in the room
while %obj%
  * while an object is in the room
  if %obj.vnum% == %CARDOBJ%
    set CARDISHERE 1
    * if the object is here, remember!
  end
  if %CARDISHERE%
    %send% %actor%  There is already a face up Adventure card here!
    halt
    * if its here stop the trig and let the player know
  else
    %send% %actor% You reach into the Adventure deck and select the top card...
    %echoaround% %actor% %actor.name% reaches into the Adventure deck and selects the top card...
    %load% obj %CARDOBJ%
    * if it isn't here load one
  end
  set next_obj %obj.next_in_list%
  * find the next object for the while to loop
  set obj %next_obj%
done
~
#1425
Random Content Generation~
2 d 100
showmethereview~
eval start 1
while %start% < 12
set opening1 Upon opening the pack a strong vanilla note hits me,
set opening2 A slight hint of vanilla hits me as I open the pack,
set opening3 There was a slight hint of vanilla when I opened the pack,
set opening4 Opening the pack I was treated to a lovely vanilla scented 
tobacco,
set opening5 This particular pouch smells rather sweet, possibly vanilla, 
set opening6 First let me say that this pouch was really sweet smelling,
set opening7 This sweet smelling pouch made me wonder about the quality of the 
tobacco, 
set opening8 I had a bit of trouble with the tin, 
set look1 but inside was an excellent well rubbed tobacco.
set look2 and inside I was treated to a nice well rubbed tobacco.
set look3 but I found a nice rubbed tobacco inside.
set look4 but it produced a nice pinch of reddish tobacco.
set look5 yet it was filled with beautiful flakes of tobacco.
set look6 but the tobacco was too lightly packed. It was uneven and packed 
funny into the pipe.
set look7 and I couldn't help noticing how great this would be for the room 
note.
set look8 and yet I found an excellent nugget of tobacco waiting inside.
set moisture1 It was a bit wet at first,
set moisture2 It was pretty damp out of the pack,
set moisture3 The pack had far too much dampness to it,
set moisture4 It was deliciously moist,
set moisture5 It was a little wet for my tastes,
set moisture6 There was no way this would light in my pipe,
set drying1 but I let it sit on a napkin for %random.3% hours.
set drying2 but I air-dried it and it turned out fine.
set drying3 so I put it in a napkin for a bit to make it smokeable.
set drying4 so I wrapped it in a napkin and forgot about it for a few hours.
set drying5 so being me, I tried lighting it as it was. Big mistake. My next 
batch I let sit out before lighting up.
set drying6 but this was easily fixed by letting it dry out on my desk.
set drying7 but I like my tobacco a bit on the wet side, so I only aired it 
for 20 minutes.
set drying8 and it took a while to dry, but it lit beautifully afterwards.
set mix1 A well honed mix of Virginia and Burley, this mix stood the test of 
time.
set mix2 This mix suited my briar quite well as I'd smoked plenty of 
Virginia/Burley in the past in it.
set mix3 It was worth it, this is an excellent mix of Virginia and Burley.
set mix4 I quite enjoyed the Virginia notes inside of the Burley base of this 
tobacco.
set mix5 Your standard Virginia and Burley mixed, this particular mix stands
set mix6 A pleasant Virginia/Burley, although nothing out of the ordinary.
set mix7 I could taste a strong note of Burley on this particular specimen, 
but not sure what else.
set mix8 As always, this excellent blend worked well for my morning pipe.
set burn1 The burn on this batch was rather mild flavored, but not any more 
than you'd normally suspect.
set burn2 Rather mild, but quite aromatic. I quite liked this pouch.
set burn3 Nice and mild, but with a slight kick to it. I was enjoying it so 
much I went a bit too fast.
set burn4 Rather bland flavor to be honest, but I like my tobacco strong and 
my coffee black.
set burn5 A bit on the softer side as far as burn goes, but a rich, smooth 
flavor.
set burn6 Great mild taste, but pretty well beginner only.
set burn7 The tobacco itself had a mild taste, but would make an excellent 
pipe for after dessert.
set burn8 I didn't pack my pipe properly, so I didn't get to enjoy this as 
much as I'd have liked to.
set price1 Definitely worth the price.
set price2 Pretty cheap tobacco, but for good reason.
set price3 I think given the flavor and quality of the tobacco the price 
matches up.
set price4 The price is pretty reasonable for what you're getting. It's an 
excellent beginners pouch.
set price5 A nice starter-oriented price. This tobacco won't break the bank 
and is mild enough to start with, but any true conniseur will move on soon.
set price6 With today's gas prices, I don't know why I spend so much on 
tobacco. Must be the addiction.
set price7 Excellently priced, as usual.
set price8 This bowl really took me back to my college days when I first tried 
this mix.
set openingamt 8
set lookamt 8 
set moistureamt 6
set dryamt 8
set mixamt 8
set burnamt 8
set priceamt 8
set opening %opening^random.8^%
set look %look^random.8^%
set moisture %moisture^random.6^%
set drying %drying^random.8^%
set mix %mix^random.8^%
set burn %burn^random.8^%
set price %price^random.8^%
eval which %random.6%
if %which% == 1
%echo% %opening% %look% %moisture% %drying% %mix% %burn% %price%
end
if %which% == 2
%echo% %opening% %look% %mix% %burn% %price%
end
if %which% == 3
%echo% %opening% %look% %moisture% %drying% %burn% %price%
end
if %which% == 4
%echo% %opening% %look% %moisture% %drying% %mix% %burn%
end
if %which% == 5
%echo% %opening% %look% %mix% %burn%
end
if %which% == 6
%echo% %opening% %look% %mix% %burn% %price%
end
%echo% 
eval start %start%+1
done
~
#1450
Room Global Random Example~
2 ab 100
~
* Fires whether a player is in the room or not.
%echo% The trigger fires now!
~
#1451
Room Random Example~
2 b 100
~
* Fires only when a player is in the room.
%echo% The trigger fires now!
* Example by Snowlock
* %echo% The pungent fumes burn your lungs!
* set target_char %self.people%
* while %target_char%
*   set tmp_target %target_char.next_in_room%
*   %damage% %target_char% 3
*   set target_char %tmp_target%
* done
~
#1452
Room Command Example~
2 c 100
l~
if %cmd.mudcommand% == look && test /= %arg%
  %echo% The trigger works!
  %force% %actor% applaud
else
  %send% %actor% Look at what?
end
* Portal example with arguments: enter
* if portal /= %arg%
* %send% %actor% You enter the portal.
* %echoaround% %actor.name% %actor.name% bravely steps into the portal.
* %teleport% %actor% 3001
* %force% %actor% look
* %echoaround% %actor% %actor.name% steps through a portal.
* else 
* %send% %actor% Enter what?!
* end
~
#1453
Room Speech Example~
2 d 100
test~
%echo% The trigger fires now!
~
#1454
Room Zone Reset Example~
2 f 100
~
%echo% The trigger fires now!
~
#1455
Room Enter Example~
2 g 100
~
%echo% The trigger fires now!
~
#1456
Room Drop Example~
2 h 100
~
%echo% %actor.name% tries to drop object type: %object.type%
if %object.type% == TRASH
  %echo% No Littering!
  return 0
end
~
#1457
Room Cast Example~
2 p 100
~
%echo% %actor.name% tried to cast spellnumber: %spell%: %spellname% on %vict.name% %obj.name%.
return 0
~
#1458
Room Leave Example~
2 q 100
~
%echo% %actor.name% tries to leave to the %direction%.
return 0
~
#1459
Room Door Example~
2 r 100
~
if %cmd% == open
  %echoaround% %actor% As %actor.name% tries to %cmd% the door to the %direction% a bucket of water dumps on his head.
  %send% %actor% Splash!!
  %send% %actor% A bucket of water drops on top of your head as you open the door.
  %damage% %actor% 10
  %echo% The door slams shut again.
  detach 1459 %self.id%
  return 0
end
~
#1460
Mob Global Random Example~
0 ab 100
~
say The trigger fires now!
~
#1461
Mob Random Example~
0 b 100
~
* No Script
* This I just threw in because it is a random trig and does not normally have an actor.
set actor %random.char%
*
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
    mecho A failed spell backfires on the mage!
    mdamage %self% 10
  break
done
~
#1462
Mob Command Example~
0 c 100
test~
say The trigger fires now!
say triggered by %actor.name%
~
#1463
Mob Speech Example~
0 d 0
test~
say speech: %speech%
say car: %speech.car%
say cdr: %speech.cdr%
eval text %speech.car%
say %text%
~
#1464
Mob Action Example~
0 e 0
has entered the game.~
eval inroom %self.room%
%zoneecho% %inroom.vnum% %self.name% shouts, 'HELP!! Someone please rescue me!!'
~
#1465
Mob Death Example~
0 f 100
~
%echo% %self.name% curses %actor.name% before drawing %self.hisher% final breath.
~
#1466
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
~
#1467
Mob Greet-All Example~
0 h 100
~
say Hello, and welcome, %actor.name%
~
#1468
Mob Entry Example~
0 i 100
~
* first find the room the mob is in and put the value in %inroom%
eval inroom %self.room%
* then check on the rooms vnum
if (%inroom.vnum% == 1233)
  say I, %self.name%, declare this room Rumble's.
end
~
#1469
Mob Receive Example~
0 j 100
~
if (%object.vnum% == 1300)
  %purge% %object%
  say thanks!
  nop %actor.gold(1)%
else
  say I don't want that!
  return 0
end
~
#1470
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
#1471
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
#1472
Mob Bribe Example~
0 m 1
~
say thank you, step inside.
wait 2 sec
%echoaround% %actor% %self.name% pushes %actor.name% through a concealed door.
%send% %actor% %self.name% helps you through a concealed door.
%teleport% %actor% 1300
~
#1473
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
#1474
Mob Memory Example Part 1~
0 g 100
~
* This must be set by another trigger first before the mem trigger can be used.
mremember %actor.name%
say I'll remember you now, %actor.name%
~
#1475
Mob Memory Example Part 2~
0 o 100
~
wait 4 s
poke %actor.name%
say i've seen you before, %actor.name%.
mforget %actor.name%
~
#1476
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
#1477
Mob Leave Example~
0 q 100
~
if (%actor.level% > 10)
  say You may not leave here, %actor.name%.
  %send% %actor% %self.name% prevents you from leaving the room.
  %echoaround% %actor% As %actor.name% tries to leave the room, %self.name% stops %actor.himher%.
  return 0
end
~
#1478
Mob Door Example~
0 r 100
~
say %actor.name% do not try to %cmd% the door to the %direction% again. Or else!
return 0
~
#1479
Obj Global Random Example~
1 ab 100
~
%echo% The trigger fires now!
~
#1480
Obj Random Example~
1 b 100
~
%echo% The trigger fires now!
eval actor %self.worn_by%
if !%actor%
  halt
endif
%send% %actor% Ichiban's blade thirsts for blood.
~
#1481
Obj Command Example~
1 c 7
open~
* Numeric Arg: 7 means obj can be worn, carried, or in room.
if ("%arg%" == "closet")
%load% mob 1307
else
%send% %actor% Open What?
end
~
#1482
Obj Timer Example~
1 f 100
~
* %echo% The trigger fires now!
* otimer 3
%echo% The ice cream melts away.
%purge% %self%
~
#1483
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
#1484
Obj Drop Example~
1 h 100
~
if (%actor.level% < 31)
  return 0
end
~
#1485
Obj Give Example~
1 0 100
~
if (%actor.level% < 31)
  return 0
end
~
#1486
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
#1487
Obj Remove Example~
1 l 90
~
return 0
~
#1488
Obj Load Example~
1 n 100
~
%echo% %self.name% appears out of nowhere.
~
#1489
Obj Leave Example~
1 q 100
~
%echo% My trigger commandlist is not complete!
~
#1490
Nested If Example~
0 q 100
~
* In this nested if example anyone leaving north will be checked for passage.
if %direction% == NORTH
  * If it is a male over 18 let them pass.
  if %actor.sex% == MALE
    if %actor.age% > 18
      say Welcome.
    else
      say let me see your ID.
      return 0
    end
    * If a female over 18 or less than 18 but charisma above 16 let them pass.
  elseif %actor.sex% == FEMALE
    if %actor.age% > 18
      say welcome.
    else
      if %actor.cha% > 16
        say don't tell anyone I let you in.
      else
        say let me see your ID.
        return 0     
      end
    end
    * Don't let nuetrals pass
  elseif %actor.sex% == NUETRAL
    say what the heck are you?
    return 0
  end
end
~
#1491
Room Global Example~
2 d 100
*~
set global_example_actor %actor.name%
global global_example_actor
set global_example_speech %speech%
global global_example_speech
%echo% saving globals
~
#1492
Room Global Example II~
2 b 100
~
%echo% %global_example_actor% said: %global_example_speech%
~
#1493
mob load test~
0 c 100
load~
%load% mob 1300
~
#1498
Object Command Parser~
1 c 3
*~
*Ideally we need an object speech trig. This is the workaround.
if %cmd% == say || %cmd% == gossip
  * evaluate the first word
  eval word %arg.car%
  * evaluate the rest of the string
  eval rest %arg.cdr%
  * while there is a first word keep going
  while %word%
    %echo% the first word is: %word%
    %echo% the remaining text is: %rest%
    eval word %rest.car%
    eval rest %rest.cdr%
  done
else
  return 0
end
~
#1499
new trigger~
0 g 100
~
if %arg% == delete
  say Deleted all quest flags!
  rdelete found_treasure %actor.id%
  rdelete worthy_oceana %actor.id%
  rdelete receive_oceana %actor.id%
  rdelete aloha_welcome %actor.id%
elseif %arg% == add
  say Added quest flags!
  set receive_oceana 1
  remote receive_oceana %actor.id%
  set found_treasure 1
  remote found_treasure %actor.id%
  set worthy_oceana 1
  remote worthy_oceana %actor.id%
else
  say Invalid command! Please Add or delete?
end
~
$~
