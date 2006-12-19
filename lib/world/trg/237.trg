#23700
Trader Walkabout - NOT ATTACHED~
0 ab 100
~
if (%self.fighting%)
halt
end
context %self.id%
eval inroom %self.room%
if (%inroom.vnum% == 23607)
  set trader_leaving 1
  global trader_leaving
end
 
if (%inroom.vnum% == 3036)
  unset trader_leaving
end
 
if %trader_leaving%
  switch %inroom.vnum%
* start room 23607
    case 23607
* Start room - need a speech here.
      say We'll leave at dawn.
      rest
     wait until 07:00      
      stand
      wait 1 t
     say Are you ready to find adventure, men ?
      wait 6 s
     say Are you ready to fight every hardheaded orc on the way ?
      wait 6 s
     say Don your gear, and follow  'll bring the name of Aldin to Radidan.
      wait 6 s
     say Let's go!
      wait 1 s
      south
      break
    case 23717
      say Be careful around here. I've heard of dragons east of here.
      wait 3 s
      south
      break
    case 3551
      say Those large people never learned to built a decent road.
      emote kicks up some dust.
      wait 5 s
      south
      break
    case 3501
      west
      break
    case 23602
      wait 2 s
* fall through 
    case 23601
    case 23700
    case 23701
    case 23703
    case 23704
    case 23706
    case 23709
    case 23710
    case 23711
    case 23712
    case 23718
    case 23719
    case 23720
    case 23721
    case 23722
    case 23723
    case 23724
    case 3550
    case 3549
    case 3547
    case 3546
    case 3545
    case 3544
    case 3543
    case 3526
    case 3525
    case 3524
    case 3515
    case 3513
    case 3512
      south
      break
    case 23702
    case 3542
    case 3541
    case 3540
    case 3539
    case 3533
    case 3532
    case 3531
    case 3530
    case 3528
    case 3527
    case 3523
    case 3522
    case 3521
    case 3518
    case 3517
    case 3516
    case 3511
    case 3510
    case 3508
    case 3507
    case 3506
    case 3505
    case 3504
    case 3503
    case 3502
    case 3051
    case 3050
    case 3049
    case 3048
    case 3047
    case 3039
      west
      break
    case 3538
    case 3537
    case 3535
    case 3534
    case 3529
    case 3520
    case 3519
    case 3509
      north
      break
    case 23705
    case 23713
    case 23715
    case 23716
    case 3548
    case 3536
    case 3514
      east
      break
    default
* we dont want him roaming where he might get hurt :)
      say I don't think I've ever been to this side of the city before...
      wait 6 s
      say I'd better go home.
      wait 6 s
      emote pulls a small scroll from a hidden pocket and quickly recites it.
      emote disappears in a puff of smoke.
      mgoto 23607
      break
  done
else
  switch %inroom.vnum%
    case 3036
       say The negociators told me they'd meet us here.
      wait 1 t
       twiddle
      wait 1 t
       say It doesn't look like they're coming.
      wait 1
       say What a pity. For them.
      wait 10 s
       say We'll be going home again, tomorrow morning.
      wait until 07:00
       say Let's go!
      wait 2 s
       east
      break
    case 23703
    case 3541
    case 3540
    case 3539
    case 3538
    case 3532
    case 3531
    case 3530
    case 3529
    case 3527
    case 3526
    case 3522
    case 3521
    case 3520
    case 3517
    case 3516
    case 3515
    case 3510
    case 3509
    case 3507
    case 3506
    case 3505
    case 3504
    case 3503
    case 3502
    case 3501
    case 3051
    case 3050
    case 3049
    case 3048
    case 3047
    case 3039
      east
      break
    case 3537
    case 3536
    case 3534
    case 3533
    case 3528
    case 3519
    case 3518
    case 3508
      south
      break
    case 23706
    case 23715
    case 23716
    case 23717
    case 3547
    case 3535
    case 3513
      west
      break
    case 23602
    case 23601
    case 23700
    case 23701
    case 23702
    case 23704
    case 23705
    case 23706
    case 23709
    case 23710
    case 23711
    case 23712
    case 23713
    case 23718
    case 23719
    case 23720
    case 23721
    case 23722
    case 23723
    case 23724
    case 3551
    case 3550
    case 3549
    case 3548
    case 3546
    case 3545
    case 3544
    case 3543
    case 3542
    case 3525
    case 3524
    case 3523
    case 3514
    case 3512
    case 3511
      north
      break
    default
* we dont want him roaming where he might get hurt :)
      say I don't think I've ever been to this side of the city before...
      wait 6
      say I'd better go home.
      wait 6
      emote pulls a small scroll from a hidden pocket and quickly recites it.
      emote disappears in a puff of smoke.
      mgoto 23607
      break
   done
end
~
#23701
Orc Attacks Trader - NOT ATTACHED~
0 g 100
~
if %actor.vnum% == 23705
  wait 1
  say YYEEHAA! I got you now, dwarf! Give me your gold and I might spare you.
  wait 1
  emote charge forward, attacking the dwarven trader.
  %kill% trader
else
  growl %actor.name%
end
~
#23702
Trader Attacks Orc - 23705~
0 h 100
~
if %actor.vnum%==3501 || %actor.vnum%==4401 || %actor.vnum%==4402 || %actor.vnum%==4403
  wait 1
  say DIE! Orc scum!
  wait 1
  emote charge forward, attacking the orc.
  %kill% orc
end
~
#23703
Bodyguard Yes - 23706~
0 d 0
Are you ready to find adventure, men ?~
wait 1
emote snaps to attention, and says promptly 'Yes, Sir!'
~
#23704
Bodyguard Yes - 23706~
0 d 0
Are you ready to fight every hardheaded orc on the way ?~
wait 1
say Yes, Sir!
~
#23705
Bodyguard Follow - 23706~
0 d 0
Don your gear, and follow me.~
wait 1
follow trader
~
#23706
Trader Cry for Help - 23705~
0 l 90
~
say HELP! I'm under attack!
wait 60 s
* so he doesn't say it ALL the time :)
~
#23707
Bodyguard Assist - 23706~
0 d 0
HELP! I'm under attack!~
wait 1
assist trader
~
#23708
Trader Sac Corpse - 23705~
0 d 0
AAARRRGGH!!~
wait 1
sac corpse
~
#23709
Attach Test - NOT ATTACHED~
2 g 100
~
wait 2
attach 23703 %self.id%
%echo% script attached.
~
#23710
Test - NOT ATTACHED~
2 p 100
~
%echo% actor name %actor.name% 
%echo% spell number %spell%
%echo% spell name %spellname%
%echo% arg %arg%
return 0
~
$~
