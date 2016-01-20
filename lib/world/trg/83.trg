#8300
Zone 83 Enter~
2 cg 100
Enter~
if !%arg% && !%command%
  wait 2s
  %echo%  	g[ Welcome to Zone 83, by Meyekul.  Type '	oEnter Zone	n	g' to begin. ]	n
elseif %cmd.mudcommand% == Enter && zone /= %arg%
  wait 1s
  %echo% 	g[ Now Entering Zone 83... ]	n
  wait 1s
  %teleport% %actor% 8301
  %force% %actor% look
else
end
~
#8301
Small Fish (8301) Schooling~
0 h 100
~
if %actor.vnum% == 8301
  follow small
  detach all %self.id%
end
~
#8302
Large Fish (8302) chase Small Fish (8301)~
0 h 100
~
if %actor.vnum% == 8301
  emote begins chasing a small fish.
  %force% %actor% flee
  end
~
#8303
Self-Healing on half HP~
0 l 50
~
eval repair %random.3%
switch %repair%
  case 1
    %echo% 	cPirates scramble to repair their damaged ship.	n
    break
  case 2
    %echo% 	cPirates carry buckets of water to extinguish fires on the ship.	n
    break
  case 3
    %echo% 	cPirates rush to seal leaks in the hull of their ship.	n
    break
done
  %damage% %self% -1000
~
#8304
Take Loot to the Vault (8395)~
0 bq 100
~
get all
%at% 8395 drop all
~
#8305
Surrender to Pirate Ship (8305)~
0 cd 100
surrender~
   %echo% A pirate shouts from the ship, 'Good choice, ya salty dog, you may live to see the end of this day!'
   wait 1
   %echo% The pirates take you captive aboard their ship.
   %teleport% %actor% 8392
   wait 1s
   %force% %actor% look
~
#8306
Navigator (8306) Sings~
0 b 10
~
eval song %random.4%
switch %song%
  case 1
    emote clears his throat.
    wait 3s
    emote sings, 'Plunder, Plunder, How I wonder...'
    wait 3s
    emote sings, 'How'd ya get so doggone pretty?'
    wait 3s
    emote sings, 'Home to sailors, barbers, tailors, and Puerto Pollo, yer capital city!'
    wait 10s
    break
  case 2
    emote clears his throat.
    wait 3s
    emote sings, 'I'm hooked on you baby...'
    wait 3s
    emote sings, 'But the seas keep us apart.'
    wait 3s
    emote sings, 'And there aint no eyepatch big enough...'
    wait 3s
    emote sings, 'To cover up... my... broke... n.... hearrrrrt!'
    wait 10s
    break
  case 3
    emote clears his throat.
    wait 3s
    emote sings, 'Oh... there's a...'
    wait 3s
    emote sings, 'Monkey in my pocket...'
    wait 3s
    emote sings, 'And he's stealing all my change...'
    wait 3s
    emote sings, 'His stare is blank and glossy...'
    wait 3s
    emote sings, 'I suspect that he's deraaaaanged!'
    wait 10s
    break
  case 4
    emote clears his throat.
    wait 3s
    emote sings, 'For those cold dark shipboard nights...'
    wait 3s
    emote sings, 'We've got boxers, briefs and tights...'
    wait 3s
    emote sings, 'Made from cotton, silk or satin...'
    wait 3s
    emote sings, 'In styles anglo, dutch or latin!'
    wait 3s
    emote sings, 'When you sail don't take a chance...'
    wait 3s
    emote sings, 'Wearin' nothin neathe your pants...'
    wait 3s
    emote sings, 'Trust... Silvers.. Longjohns...'
    wait 3s
    emote sings, 'They breathe!'
    wait 10s
done
	n
	n
	n
	y****************************************************************
	y*  	cThese songs were written (as far as I know..) by            	y*
	y*  	c	oTim Schafer	n	c, Lead Designer of "	oThe Curse of Monkey Island	n	c"  	y*
	y*  	cI take no responsibility if he got the ideas elsewhere. :)  	y*
	y****************************************************************	n
~
#8307
Cabin Boy (8307) Mops~
0 b 10
~
wait 2s
emote sings, 'Mop, mop, mop, all day long...'
wait 2s
emote sings, 'Mop, mop, mop, while I sing this song...'
wait 60s
~
#8308
Pirate Guard (8308) Bribery~
0 m 1
~
if (%amount% >= 200)
  set bribed 1
  global bribed
  emote takes the bribe and begins counting it.
  wait 2s
  say %amount% coins?  Wow, thanks!  I'll let you out!
  %door% 8392 n flags a
  %door% 8392 n room 8394
  %door% 8392 n key 8307
  %door% 8394 s flags a
  %door% 8394 s room 8392
  %door% 8394 s key 8307
  wait 2s
  say Here, take my rations for the day, I'm gettin' drunk!
  %load% obj 8304
  give rations %actor.name%
  wait 2s
  emote scants off before the next guard comes around.
  %purge% %self%
else
  eval bribed 1
  global bribed
  emote takes the bribe and begins counting it.
  wait 2s
  say %amount% coins, huh?  Ok, if I should happen to forget to lock the door...
  %door% 8392 n flags a
  %door% 8392 n room 8394
  %door% 8392 n key 8307
  %door% 8394 s flags a
  %door% 8394 s room 8392
  %door% 8394 s key 8307
  wait 2s
  emote scants off before the next guard comes around.
  %purge% %self%
end
~
#8309
Pirate Guard (8308) Patrol~
0 b 75
~
 while %people.8392%
   wait 5s
   %teleport% %self% 8392
   emote enters the cell.
   say You need anything?
   wait 5s
   if !%bribed%
     say Right then..
     wait 1s
     emote leaves the cell.
     %teleport% %self% 8394
     wait 10s
   else
     end
 done
~
#8310
Glumgold (8311) Talks to Polly~
0 b 25
~
say Polly wanna cracker?
~
#8311
Glumgold (8311) Taunts Players~
0 h 100
~
eval taunt %random.3%
switch %taunt%
  case 1
   wait 2
   say What? Who are you?
   wait 2
   say You'd best get out of here before I get angry!
   wait 2
   spit %actor.name%
   break
  case 2
   wait 2
   say You're not one of my boys, are ye?
   wait 2
   say You'd better get off me boat before I keel haul you!
   wait 2
   growl %actor.name%
   break
  case 3
   wait 2
   grumble
   wait 2
   say What are you doing here?  I'm not payin you to stand around, get out there and swab the deck, matey!
   wait 2
   break
done
~
#8312
Near Death Trap Davy Jones' Locker - 8386~
2 g 100
~
* Near Death Trap stuns actor
wait 1 sec
set stunned %actor.hitp% 
%damage% %actor% %stunned%
wait 5 sec
%send% %actor% The Gods pity you enough to allow you to survive.
~
#8313
Polly (8313) Learn & Repeat~
0 d 1
*~
*** Squak for the nice people, Polly
wait 1s
emote squawks loudly.
wait 1s
*** 75 percent chance of learning phrase
eval polly %random.4%
if (%polly% > 1)
%echo% Polly the Parrot says, '%speech%'
  *** Learn new phrases
  eval number (%number% + 1)
  eval phrase(%number%) %speech.trim%
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
#8314
Polly (8313) Random Speech~
0 b 10
~
wait 1s
emote squawks loudly.
wait 1s
if (%maxphrases% == 1)
  eval polly %random.10%
else
  eval polly %%random.%number%%%
end
switch %polly%
  case 1
    %echo% Polly the Parrot says, '%phrase(1)%'
  break
  case 2
    %echo% Polly the Parrot says, '%phrase(2)%'
  break
  case 3
    %echo% Polly the Parrot says, '%phrase(3)%'
  break
  case 4
    %echo% Polly the Parrot says, '%phrase(4)%'
  break
  case 5
    %echo% Polly the Parrot says, '%phrase(5)%'
  break
  case 6
    %echo% Polly the Parrot says, '%phrase(6)%'
  break
  case 7
    %echo% Polly the Parrot says, '%phrase(7)%'
  break
  case 8
    %echo% Polly the Parrot says, '%phrase(8)%'
  break
  case 9
    %echo% Polly the Parrot says, '%phrase(9)%'
  break
  case 10
    %echo% Polly the Parrot says, '%phrase(10)%'
  break
  default
    emote whistles.
  break
done
~
#8315
Bucket O' Tar (8315) Cools after 10 Minutes~
1 g 100
~
wait 300s
%send% %actor% The bucket of tar begins to cool.
wait 300s
%send% %actor% The bucket has cooled and the tar has hardened.
%load% obj 8316
%purge% %self%
end
~
#8316
Bucket O' Tar (8315) Spills when Manipulated~
1 ghjl 50
~
%send% %actor% You spill a clump of tar onto the ground.
%echoaround% %actor% %actor.name% spills a clump of tar onto the ground.
%load% obj 8317
~
#8317
Bucket O' Tar - Purge Clump (8317) After 5 Minutes~
1 n 100
~
wait 300s
%purge% %self%
~
#8319
Load Meat when Crab (8319) Dies~
0 f 100
~
%load% obj 8319
~
#8320
Life Boat (8320) Dispenser~
1 g 100
~
return 0
%load% obj 8301
%force% %actor% get boat
~
#8375
Load/Purge Exits after Leave (8375)~
2 q 100
~
if (%direction% == down)
  %door% 8373 up room 8375
  %door% 8373 up description You can still reach the ladder if you wish to board the ship again.
  set loop 0
  set person 1
  while (%loop% < 60)&&(%people.8373%||%people.8375%)
   wait 1s
   eval loop (%loop% + 1)
  done
  %at% 8373 %echo% It is no longer possible to board the ship from here.
  %door% 8373 up purge
end
~
#8380
Vermin Flees on Entrance (8380)~
2 g 50
~
  wait 1s
  eval vermin %random.4%
  switch %vermin%
    case 1
      %echo% A big, fat, disgusting rat waddles away as you approach.
      break
    case 2
      %echo% A few small mice squeek and scurry away as you enter the room.
      break
    default
      %echo% Several large cockroaches flee the room as you enter.
      break
  done
~
#8385
Jump from Crow's Nest (8385)~
2 c 100
jump~
 if ((%arg% == down)||(%arg% == deck)||(%arg% == off))
   %send% %actor% You climb onto the edge of the crow's nest and dive off.
   %echoaround% %actor% %actor.name% climbs to the edge and jumps off.
   %teleport% %actor% 8376
   wait 1s
   %send% %actor% 	oDown..	n
   wait 1s
   %send% %actor% 	oDown...	n
   wait 1s
   %send% %actor% 	oDown you go...	n
   wait 1s
   eval halfhitp ((%actor.hitp% / 2) + 10)
   %send% %actor% You slam 	oHARD	n into the deck!
   %send% %actor% You take 	r%halfhitp%	n points of damage.
   %echoaround% %actor% %actor.name% slams 	oHARD	n into the deck!
   %damage% %actor% %halfhitp%
 else
   %send% %actor% Jump where?
 end
~
#8393
99 Bottles of Beer (Drunken Pirate: 8309)~
0 b 10
~
set beers 99
emote clears his throat.
wait 3s
while %beers%
  eval beertens %beers% / 10
  eval beerones %beers% - ( %beertens% * 10 )
  switch %beerones%
    case 0
      unset alphabeers
      break
    case 1
      set alphabeers One
      break
    case 2
      set alphabeers Two
      break
    case 3
      set alphabeers Three
      break
    case 4
      set alphabeers Four
      break
    case 5
      set alphabeers Five
      break
    case 6
      set alphabeers Six
      break
    case 7
      set alphabeers Seven
      break
    case 8
      set alphabeers Eight
      break
    case 9
      set alphabeers Nine
      break
  done
  switch %beertens%
    case 0
      break
    case 1
      switch %beerones%
        case 0
          set alphabeers Ten
          break
        case 1
          set alphabeers Eleven
          break
        case 2
          set alphabeers Twelve
          break
        case 3
          set alphabeers Thirteen
          break
        case 4
          set alphabeers Fourteen
          break
        case 5
          set alphabeers Fifteen
          break
        case 6
          set alphabeers Sixteen
          break
        case 7
          set alphabeers Seventeen
          break
        case 8
          set alphabeers Eighteen
          break
        case 9
          set alphabeers Nineteen
          break
      done
      break
    case 2
      set alphabeers Twenty %alphabeers%
      break
    case 3
      set alphabeers Thirty %alphabeers%
      break
    case 4
      set alphabeers Fourty %alphabeers%
      break
    case 5
      set alphabeers Fifty %alphabeers%
      break
    case 6
      set alphabeers Sixty %alphabeers%
      break
    case 7
      set alphabeers Seventy %alphabeers%
      break
    case 8
      set alphabeers Eighty %alphabeers%
      break
    case 9
      set alphabeers Ninety %alphabeers%
      break
  done
  if %beers% == 99
    eval alphabeers2 %alphabeers%
  else
    emote sings, '%alphabeers2.trim% bottles of beer on the wall...'
    wait 3s
    emote sings, '%alphabeers2.trim% bottles of beer...'
    wait 5s
    emote sings, 'Take one down, pass it around...'
    wait 3s
    emote sings, '%alphabeers.trim% bottles of beer on the wall.'
    eval alphabeers2 %alphabeers%
    wait 5s
  end
    eval beers %beers% - 1
done
wait 10s
~
#8397
Chinchirorin Dice (8397)~
1 h 100
~
*** Roll them Bones ***
   wait 1s
   eval die1 %random.6%
   eval die2 %random.6%
   eval die3 %random.6%
   eval roll %die1%%die2%%die3%
   %send% %actor% The dice land on  	o	g%die1%	n, 	o	c%die2%	n, 	o	r%die3%	n.
   oechoaround %actor% %actor.name% rolls a 	o	g%die1%	n, 	o	c%die2%	n, 	o	r%die3%	n.
*** Check For 3 of a Kind ***
    if (%roll% == 111)
     oechoaround %actor% It's a 	o	g1-1-1	n!  %actor.name% pays triple the bet!
     %send% %actor% It's a 	o	g1-1-1	n!  You pay triple the bet!
     halt
    elseif ((%die1% == %die2%) && (%die2% == %die3%))
     oechoaround %actor% 	o	g%die1%	c%die2%	r%die3%	n Three of a kind! %actor.name% wins triple the bet!
     %send% %actor% 	o	g%die1%	c%die2%	r%die3%	n Three of a kind! You win triple the bet!
     halt
*** Check for Storms ***
    elseif (%roll% == 123 || %roll% == 132 || %roll% == 213 || %roll% == 321 || %roll% == 312)
     oechoaround %actor% It's a storm!  	o	g1-2-3	n!  %actor.name% pays double the bet!
     %send% %actor% It's a storm!  	o	g1-2-3	n!  You pay double the bet!
     halt
    elseif (%roll% == 456 || %roll% == 465 || %roll% == 546 || %roll% == 654 || %roll% == 645)
     oechoaround %actor% It's a storm!  	o	g4-5-6	n!  %actor.name% wins double the bet!
     %send% %actor% It's a storm!  	o	g4-5-6	n!  You win double the bet!
     halt
*** Otherwise, Compute the Score ***
    elseif (%die1%==%die2%)
     oechoaround %actor% %actor.name% scores a 	o	g%die3%	n.
     %send% %actor% You score a 	o	g%die3%	n.
     halt
    elseif (%die1%==%die3%)
     oechoaround %actor% %actor.name% scores a 	o	g%die2%	n.
     %send% %actor% You score a 	o	g%die2%	n.
     halt
    elseif (%die2%==%die3%)
     oechoaround %actor% %actor.name% scores a 	o	g%die1%	n.
     %send% %actor% You score a 	o	g%die1%	n.
     halt
    else
     oecho 	o	gNo score!	n
   end
*** Please Do not Edit This Section ***
* Written by Meyekul (meyekul@@hotmail.com) for Anywhere But Home (anywhere.wolfpaw.net:5555).
* The name Chinchirorin and the idea for this script came from the Playstation game Suikoden.
* Feel free to use this script in your own MUD, but please leave this section intact.
*** End of File ***
~
$~
