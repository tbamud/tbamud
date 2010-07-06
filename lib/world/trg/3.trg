#300
Camille Greet for Quest - 300~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.is_pc%
  wait 1 sec
  if %actor.varexists(3_napalm_complete)%
    say Welcome back %actor.name% did you have fun with that liquid fire?
    grin
    halt
  elseif %actor.varexists(3_napalm_search)%
    say Do you have the items?
    wait 1 sec
    if !%actor.varexists(3_naphthalene)%
      say Maybe you can look in the warriors quarter for the Naphthalene.
      halt
    end
    if !%actor.varexists(3_palmatite)%
      say Maybe you can look in the thieves quarter for the Palmatite.
      halt
    end
    if !%actor.varexists(3_napalm_jug)%
      say I believe the wellmaster should have something of clay that will work as a container.
    end
  else
    say Good day %actor.name%. 
    wait 1 sec
    emote looks around suspiciously.
    wait 1 sec
    say close that door.
    wait 4 sec
    say I've been working on something new. But, I need some help acquiring a few ingredients.
    wait 1 sec
    say people aren't very friendly towards me since the last fire.
    emote looks down into the cauldron lost in thought with a grimace on her face.
    wait 3 sec
    say But I have perfected it this time, don't listen to what they say. 
    wait 1 sec
    say Bring me some Naphthalene, Palmitite and a proper container. I will give you a sampling of my liquid fire as payment.
    set 3_napalm_search 1
    remote 3_napalm_search %actor.id%
  end
end
~
#301
Mob Memory - test trigger~
0 o 100
~
* assign this to a mob, force the mob to mremember you, then enter the
* room the mob is in while visible (not via goto)
say I remember you, %actor.name%!
~
#302
Mob Greet direction~
0 g 20
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.is_pc%
  wait 1 sec
  if %direction% == none
    * if the character popped in (word of recall) this will be hit
    say Where did YOU come from, %actor.name%?
  else
    say Hello, %actor.name%, how are things to the %direction%?
  end
end
~
#303
Obj Get Example - Good Cleric Only~
1 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
* Only allow Cleric's with a good align to get this item.
if %actor.class% != cleric || %actor.align% < 350
  return 0
  %send% %actor% You are not worthy to wield me.
  %echoaround% %actor% %actor.name% tries to pick up %self.shortdesc% and fails.
end
~
#304
Room Command - Look at Painting~
2 c 100
l~
* By Rumble of The Builder Academy    tbamud.com 9091
if %cmd.mudcommand% == look && painting /= %arg%
  %send% %actor% As you stare at the painting the figures seem to start moving and acting out the scenes they portray.
  %echoaround% %actor% %actor.name% stares at one of the paintings. A strange look coming over %actor.hisher% face.
else
  * If it doesn't match let the command continue. Without a return 0 a player
  * will not be able to "look" at anything else.
  return 0
end
~
#305
Mob Greet Clothing Check~
0 g 100
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.is_pc%
  wait 1 sec
  if %actor.eq(*)%
    eval worn_about %actor.eq(about)%
    if %worn_about.vnum% == 326
      look %actor.name%
      smile
    else
      say You always bathe in your clothes?
      eyebrow
    end
  else
    say at least get a towel, I don't want to see that.
  end
end
~
#306
Room Entry - sneak check~
2 g 25
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.is_pc%
  if %actor.skill(sneak)% > 50
    %send% %actor% You walk into the room, not waking any of the clerics.
  else
    %send% %actor% Your entry into the room wakes a few of the clerics.
    %load% mob 340
  end
end
~
#307
Obj Remove - %transform% test~
1 jl 7
test~
* test of object transformation (and remove trigger)
* test is designed for objects 3020 and 3021
* assign the trigger then wear/remove the item
* repeatedly.
%echo% Beginning object transform.
if %self.vnum% == 3020
  %transform% 3021
else
  %transform% 3020
end
%echo% Transform complete.
~
#308
Room Command - makeuid and remote testing~
2 c 100
test~
* makeuid test ---- assuming your MOBOBJ_ID_BASE is 200000,
* this will display the names of the first 10 mobs loaded on your MUD,
* if they are still around.
eval counter 0
while (%counter% < 10)
  makeuid mob 200000+%counter%
  %echo% #%counter%      %mob.id%   %mob.name%
  eval counter %counter% + 1
done
%echoaround% %actor% %actor.name% cannot see this line.
*
*
* this will also serve as a test of getting a remote mob's globals.
* we know that puff, when initially loaded, is id 200000. We'll use remote
* to give her a global, then %mob.globalname% to read it.
makeuid mob 200000
set globalname 12345
remote globalname %mob.id%
%echo% %mob.name%'s "globalname" value is %mob.globalname%
~
#309
Mob Greet - %transform% test~
0 g 100
~
* %transform% test
* as a greet trigger, entering the room will cause
* the mob this is attached to, to toggle between mob 1 and 99.
%echo% Beginning transform.
if %self.vnum%==1
  %transform% -99
else
  %transform% -1
end
%echo% Transform complete.
~
#310
Mob Speech - attach test~
0 d 100
attach~
attach 9 %self.id%
~
#311
Mob Speech - detach test~
0 d 100
detach~
detach 9 %self.id%
~
#312
Mob Command - spellcasting test~
0 c 100
kill~
* This command trigger will disallow anyone from trying to
* use the kill command, and will toss a magic missile at them
* for trying.
dg_cast 'magic missile' %actor%
return 0
~
#313
Obj Wear 12 - Staff of Sanctum~
1 j 100
~
if %actor.level% < 30
  return 0
  %send% %actor% The Staff of Sanctum whispers: I will not serve you!
  %echoaround% %actor% The Staff of Sanctum exclaims: 'I will not serve
those without honour.'
  %purge% self
else
  wait 1s
  %send% %actor% The Staff of Sanctum whispers: I was made to serve,
great one!
  %echoaround% %actor% The Staff of Sanctus exclaims: 'I will serve you
honourable one.'
end
~
#314
Room Command - Anti-quit~
2 c 100
quit~
   %send% %actor% Powerful forces keep you here. 
~
#315
Obj Command - No quit~
1 c 3
q~
if %cmd.mudcommand% == quit
  %send% %actor% Divine forces prevent you from doing that.
else
  return 0
end
~
#316
Mob Fight - generic poison~
0 k 10
~
dg_cast 'poison' %actor%
~
#317
Mob Fight - generic chill touch~
0 k 10
~
   dg_cast 'chill touch' %actor%
~
#318
Mob Fight - generic heal self~
0 k 10
~
dg_cast 'heal'
~
#319
Mob Fight - generic dispel evil~
0 k 10
~
dg_cast 'dispel evil' %actor%
~
#320
Mob Fight - generic disarm~
0 k 10
~
disarm %actor%
~
#321
Mob Fight - generic lightning bolt~
0 k 10
~
dg_cast 'lightning' %actor%
~
#322
Mob Fight - generic kick~
0 k 30
~
* By Fizban of The Builder Academy    tbamud.com 9091
* Mimics the kick skill.
eval percent ((10 - (%actor.armor% / 10)) * 2) + %random.101%
if %percent% > %actor.skill(kick)%
  nop %actor.pos(sitting)%
  eval dam %self.level% / 2
  %damage% %actor% %dam%
  %send% %self% Your boots need polishing again -- blood all over them...
  %send% %actor% %self.name% wipes %self.hisher% boots in your face!
  %echoaround% %actor% %self.name% wipes %self.hisher% boots in the face of %actor.name%!
end
~
#323
Mob Fight - generic bash~
0 k 10
~
bash %actor%
~
#324
Mob Fight - generic curse~
0 k 10
~
dg_cast 'curse' %actor%
~
#325
Mob Fight - generic dispel good~
0 k 10
~
dg_cast 'dispel good' %actor%
~
#326
Mob Fight - generic burning hands~
0 k 10
~
dg_cast 'burning hands' %actor%
~
#327
Mob Fight - generic call lightning~
0 k 10
~
dg_cast 'call lightning' %actor%
~
#328
Mob Fight - generic earthquake~
0 k 10
~
dg_cast 'earthquake'
~
#329
Mob Fight - generic silence~
0 k 10
~
dg_cast 'silence' %actor%
~
#330
Mob Speech - Hunt Example~
0 d 100
hunt~
* By Welcor
if %speech.car% != hunt
  return 0
  halt
end
eval target %speech.cdr%
if !%target% || %target% == hunt
  return 0
  halt
end
if %actor.gold% < 4000
  tell %actor.name% You cannot afford my services - go away!
  halt
end
nop %actor.gold(-4000)%
mhunt %target%
tell %actor.name% I'll charge you 4000 gold for that. I'll hunt %target% as long as I can.
~
#331
Obj Timer - Spoil meat~
1 f 100
~
%transform% 131
~
#332
Room Drop - Dg_cast for dropping objects by cost~
2 h 100
~
%%send%% %actor% As you drop the %object.shortdesc%, a loud humming starts to come from the walls.
wait 1
eval worth %object.cost% / 100
switch %worth%
  case 0
    %send% %actor% Your offering was NOT sufficient.
    dg_cast 'magic missile' %actor%
    break
  case 1
    %send% %actor% Your offering was just sufficient.
    dg_cast 'cure light' %actor%
    dg_cast 'clot minor' %actor%
    %purge% %object%
    break
  case 2
    %send% %actor% Your offering was sufficient.
    dg_cast 'refresh' %actor%
    %purge% %object%
    break
  default
    %send% %actor% Your offering was as it must be.
    dg_cast 'invisibility' %actor%
    %purge% %object%
    break
done
~
#333
Obj Command - Cast spell when eating.~
1 c 2
eat~
return 0
wait 2
if %self.carried_by% == 0
  switch %self.vnum%
    case 201
      %damage% %actor% -10
      break 
    case 202
      %damage% %actor% 10
      %send% %actor% Ouch, that hurt a little!
      break
  done
end
~
#334
Room Enter actor.level test for bug~
2 g 100
~
%echo% %actor.name%'s level is %actor.level%
if %actor.level% > 30
%echo% > 30
else
  %echo% < 30
end
~
#335
Camille Napalm Assembly Quest - 300~
0 j 100
~
if %actor.is_pc%
  wait 1 sec
  * only let players do it once.
  if %actor.varexists(3_napalm_complete)%
    say What would I want that for? Leave me alone, I have work to do.
    return 0
    halt
  end
  if %actor.varexists(3_napalm_search)%
    if %object.vnum% == 306
      set 3_naphthalene 1
      remote 3_naphthalene %actor.id%
      say Thank you! Napthalene at last!
      %purge% %object%
    elseif %object.vnum% == 307
      set 3_palmatite 1
      remote 3_palmatite %actor.id%
      say Outstanding, Palmatite! Just what I need!
      %purge% %object%
    elseif %object.vnum% == 308
      set 3_napalm_bomb 1
      remote 3_napalm_bomb %actor.id%
      say this will work well.
      %purge% %object%
    else
      say what would I want that for?
      return 0
    end
    if %actor.varexists(3_napalm_search)% && %actor.varexists(3_naphthalene)% && %actor.varexists(3_palmatite)% && %actor.varexists(3_napalm_jug)%
      say Thank you so much, here is your reward!
      %load% obj 317
      give napalm %actor.name%
      rdelete 3_napalm_search %actor.id%
      rdelete 3_naphthalene %actor.id%
      rdelete 3_palmatite %actor.id%
      rdelete 3_napalm_jug %actor.id%
      set 3_napalm_done 1
      remote 3_napalm_done %actor.id%
    end
  end
end
~
#336
Room Contents Test~
2 b 100
~
%echo% Contents: %self.contents%
~
#337
Napalm bomb - 317~
1 c 2
nap~
if napalm /= %cmd%
  if %actor.fighting% && !%arg%
    set arg %actor.fighting%
  end
  if !%arg% 
    %send% %actor% Throw it at Who?
    halt
  end
  if %arg.room% != %actor.room%
    %send% %actor% Throw it at who?
    halt
  end
  %send% %actor% You throw the napalm at %arg.name%, it strikes %arg.himher% and shatters, exploding into a ball of fire consuming %arg.himher% completely.
  %echoaround% %actor% %actor.name% throws the napalm at %arg.name%. It shatters and explodes into a ball of fire consuming %arg.himher%. 
  %asound% A large explosion is heard close by.
  set stunned %arg.hitp% 
  %damage% %arg% %stunned%
  wait 5 sec
  %echoaround% %arg% %arg.name% collapses to the ground as the flames die down. %arg.heshe% seems to still be alive, but barely. 
end
~
#338
Horse Petshop - 203~
0 c 100
*~
if %cmd.mudcommand% == list
  *
  %send% %actor%
  %send% %actor%  ##   Pet                                                 Cost
  %send% %actor% --------------------------------------------------------------
  %send% %actor%   1)  a horse                                              1500
  %send% %actor%   2)  a fine mare                                          1800
  %send% %actor%   3)  a stallion                                           3000
  *
elseif %cmd.mudcommand% == buy
  if %actor.gold% < 1500
    tell %actor.name% You have no money, go beg somewhere else.
    halt
    * lets not allow them to have more than one pet to keep the game balanced.
  elseif %actor.follower%
    tell %actor.name% You already have someone following you.
    halt
  end
  if horse /= %arg% || %arg% == 1
    set pet_name horse
    set pet_vnum 202
    set pet_cost 1500
  elseif fine /= %arg% || mare /= %arg% || %arg% == 2
    set pet_name mare
    set pet_vnum 204
    set pet_cost 1800
  elseif stallion /= %arg% || %arg% == 3
    set pet_name stallion
    set pet_vnum 205
    set pet_cost 3000
  else
    tell %actor.name% What? I don't have that.
    halt
  end
  *
  if %actor.gold% < %pet_cost% 
    tell %actor.name% You don't have enough gold for that.
  else
    * Need to load the mob, have it follow the player AND set the affect
    * CHARM so the mob will follow the masters orders.
    %load% mob %pet_vnum%
    %force% %pet_name% mfollow %actor%
    dg_affect %pet_name% charm on 999
    emote opens the stable door and returns leading your horse by its reins.
    tell %actor.name% here you go. Treat'em well.
    nop %actor.gold(-%pet_cost%)%
  end
elseif %cmd.mudcommand% == sell
  tell %actor.name% Does it look like I buy things?
else
  return 0
end
~
#380
Obj Command 82 - Teleporter Recall~
1 c 7
recall~
%send% %actor% You recall to safety.
%echoaround% %actor% %actor.name% recalls.
%teleport% %actor% 3001
%force% %actor% look
~
#394
Mob Greet - Kind Soul - 13~
0 g 100
~
if %actor.is_pc%   
  wait 2 sec
  if !%actor.eq(light)%
    Say you really shouldn't be wondering these parts without a light source %actor.name%.
    shake
    %load% obj 200
    give light %actor.name%
    halt
  end
  if !%actor.eq(rfinger)% || !%actor.eq(lfinger)%
    Say did you lose one of your rings?
    sigh
    %load% obj 201
    give ring %actor.name%
    halt
  end
  if !%actor.eq(neck1)% || !%actor.eq(neck2)%
    Say you lose everything don't you?
    roll
    %load% obj 203
    give necklace %actor.name%
    halt
  end
  if !%actor.eq(body)%
    say you won't get far without some body armor %actor.name%.
    %load% obj 205
    give body %actor.name%
    halt
  end
  if !%actor.eq(head)%
    Say protect that noggin of yours, %actor.name%.
    %load% obj 206
    give helm %actor.name%
    halt
  end
  if !%actor.eq(legs)%
    Say why do you always lose your pants %actor.name%?
    %load% obj 207
    give leggings %actor.name%
    halt
  end
  if !%actor.eq(feet)%
    Say you can't go around barefoot %actor.name%.
    %load% obj 208
    give boots %actor.name%
    halt
  end
  if !%actor.eq(hands)%
    Say need some gloves %actor.name%?
    %load% obj 209
    give gloves %actor.name%
    halt
  end
  if !%actor.eq(arms)%
    Say you must be freezing %actor.name%.
    %load% obj 210
    give sleeves %actor.name%
    halt
  end
  if !%actor.eq(shield)%
    Say you need one of these to protect yourself %actor.name%.
    %load% obj 211
    give shield %actor.name%
    halt
  end
  if !%actor.eq(about)%
    Say you are going to catch a cold %actor.name%.
    %load% obj 212
    give cape %actor.name%
    halt
  end
  if !%actor.eq(waist)%
    Say better use this to hold your pants up %actor.name%.
    %load% obj 213
    give belt %actor.name%
    halt
  end
  if !%actor.eq(rwrist)% || !%actor.eq(lwrist)%
    Say misplace something?
    smile
    %load% obj 215
    give wristguard %actor.name%
    halt
  end
  if !%actor.eq(wield)%
    Say without a weapon you will be fido food %actor.name%.
    %load% obj 216
    give weapon %actor.name%
    halt
  end
  if !%actor.eq(hold)%
    Say this might help you %actor.name%.
    %load% obj 217
    give staff %actor.name%
    halt
  end
end
~
#395
Mob Greet 9 - Eating its own stool~
0 g 100
~
if %actor.is_pc%
  wait 2 sec
  %echo% %self.name% sniffs a pile of steaming stool.
  wait 2 sec
  take stool
  wait 2 sec
  %echo% %self.name% devours the steaming pile of stool.
  wait 3 sec
  %echo% %self.name% walks around in a circle, stops, then squats.
  wait 2 sec
  drop stool
end
~
#396
Obj Command 81 - Paintball Shoot Blue~
1 c 2
shoot~
eval inroom %actor.room%
if (%arg.room% != %actor.room%) || (%arg.id% == %actor.id%)
  %send% %actor% Shoot: Invalid Target!
  halt
end
if %arg.inventory(80)%
  %echoaround% %actor% %actor.name% blasts %arg.name% with %actor.hisher% paintball gun.
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
#397
Obj Command 80 - Paintball Shoot Red~
1 c 2
shoot~
eval inroom %actor.room%
if (%arg.room% != %actor.room%) || (%arg.id% == %actor.id%)
  %send% %actor% Shoot: Invalid Target!
  halt
end
if %arg.inventory(81)%
  %echoaround% %actor% %actor.name% blasts %arg.name% with %actor.hisher% paintball gun.
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
#398
Mob Act - 98 Teleporter Give~
0 e 0
has entered the game.~
if !(%actor.inventory(82)%)
  wait 1s
  say You are not prepared to travel these realms to their fullest.
  wait 1s
  say Maybe I can help you.
  %load% obj 82
  give teleporter %actor.name%
*could actor carry the weight?
  if !%actor.inventory(82)%
  drop teleporter
  end
  wait 2s
  say With this you may teleport to areas that may not be accessible in
any other way.
  wait 2 sec
  say HELP AREAS
end
~
#399
Obj Command 82 - Teleporter~
1 c 3
teleport~
%send% %actor% You attempt to manipulate space and time.
%echoaround% %actor% %actor.name% attempts to manipulate space and time.
wait 1 sec
set Sanctus 100
set jade 400
set newbie 500
set sea 600
set camelot 775
set nuclear 1800
set arena 2000
set tower 2200
set Orc 4401
set monastery 4512
set ant 4600
set zodiac 5701
set graveyard 7401
set zamba 7500
set oasis 9000
set northern 10004
set south 10101
set elcardo 10604
set iuel 10701
set omega 11501
set hannah 12500
set wyvern 14000
set cardinal 17501
set circus 18700
set wester 20001
set terringham 23200
set dragon 23300
set school 23400
set mines 23500
set aldin 23600
set crystal 23800
set pass 23901
set maura 24000
set enterprise 24100
set midgaard 24200
set valley 24303
set prison 24450
set nether 24500
set yard 24700
set elven 24801
set jedi 24901
set dragonspyre 25000
set ape 25100
set Vampyre 25200
set windmill 25300
set village 25400
set shipwreck 25516
set keep 25645
set jareth 25705
set mansion 25900
set igor's 26100
set forest 26201
set banshide 26400
set ankou 26600
set vice 26728
set desert 26900
set wasteland 27000
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
set froboz 29600
set enclave 29700
set desire 29801
set ancalador 30000
set campus 30100
set bull 30401
set chessboard 30537
set castle 30700
set baron 30800
set westlawn 30900
set graye 31003
set teeth 31100
set leper 31200
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
