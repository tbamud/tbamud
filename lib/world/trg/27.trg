#2700
(2741) Room only allows shackle-wearers~
2 g 100
~
eval rwrist %actor.eq(rwrist)%
eval lwrist %actor.eq(lwrist)%
if (%rwrist.vnum% == 2708)
%send% %actor% A magical force seems to ebb, allowing you to pass.
 
elseif (%lwrist.vnum% == 2708)
%send% %actor% A magical force seems to ebb, allowing you to pass.
 
elseif (%actor.varexists(Zn27_shacklepass)%)
%send% %actor% A magical force seems to ebb, allowing you to pass.
 
else
%send% %actor% A powerful force seems to repel you, preventing you from entering.
%force% %actor% sit
set Zn27_shacklereject 1
remote Zn27_shacklereject %actor.id%
return 0
end
~
#2701
testmemory~
0 o 100
~
wait 4 s
poke %actor.name%
say I've seen you before, %actor.name%.
mforget %actor.name%
~
#2702
(2701) Guard offers quest~
0 h 100
~
if (%actor.varexists(zn27_greeted)%)
  wait 1 s
  %send% %actor% The guard whispers to you 'Please do not tell me you are giving up!'
else
  wait 1 s
  tell %actor.name%  Halt! You cannot pass here.
  wait 2 s
  msend %actor% The guard seems to think for a minute, looking you up and down.
  wait 2 s
  msend %actor% The guard takes a deep breath and glances nervously over his shoulder, before turning back to you.
  wait 3 s
  %send% %actor% The guard whispers to you 'You look like a person of great might. Perhaps you could help us...'
  wait 2 s
  msend %actor% The guard glances quickly over his shoulder again.
  wait 2 s
  %send% %actor% The guard whispers to you 'An evil sorceress has taken over our caves, and is forcing my people to build her a palace in the mountainside.'
  wait 3 s
  %send% %actor% The guard whispers to you 'She is most cruel, and if you can rid us of her we would be eternally grateful.'
  wait 2 s
  %send% %actor% The guard whispers to you 'I will let you pass, though it may cost my life.' 
  wait 2 s
  %send% %actor% The guard whispers to you 'Please do not let us down!'
  set Zn27_greeted 1
  remote Zn27_greeted %actor.id%
  wait 2 s
  msend %actor% The guard whispers to you: Here, take this and hold it if your eyes are not accustomed to the dark.
  %load% obj 2703
  give mushroom %actor.name%
  drop mushroom
end
~
#2703
(2704) Mob hums randomly~
0 b 25
~
%echo% 	CA tiny wish	n hums quietly as it gives off a fresh breeze of air.
~
#2704
(2700) Mobs load and wear shackle~
0 n 100
~
mload obj 2708
wait 1
wear shackle
~
#2705
(2705) Guard keeps gates closed and locked~
0 bn 100
~
close glassygates
wait 1
lock glassygate
~
#2706
(2705) Guard warns you before attacking~
0 g 100
~
wait 1
tell %actor.name% You are not allowed to be here!
~
#2707
(2714) dying memlin greets~
0 gh 100
~
wait 1 s
%send% %actor% A dying memlin gasps: Please help me...
wait 2 s
%send% %actor% A dying memlin gasps: My death cannot be stopped, please end it now.
wait 3 s
emote groans with pain as you hear a loud creaking sound.
~
#2708
(2708) makes shackles loosen and disintegrate~
0 gh 100
~
wait 1 s
eval rwrist %actor.eq(rwrist)%
eval lwrist %actor.eq(lwrist)%
if (%rwrist.vnum% == 2708)
say No creature should be enslaved.
wait 2 s
%force% %actor% rem all.goethite
set Zn27_shacklepass 1
      remote Zn27_shacklepass %actor.id%
elseif (%lwrist.vnum% == 2708)
say No creature should be enslaved.
wait 2 s
%force% %actor% rem all.goethite
set Zn27_shacklepass 1
      remote Zn27_shacklepass %actor.id%
end
~
#2709
(2713) toothy creature greets~
0 gh 100
~
 wait 1 s
emote creaks as it turns to look at you.
wait 2 s
emote uses 	Ca suspended orb of glowing water	n, filling the room with a wave of light.
wait 2 s
say Ah, it does not touch you... and here I thought you were the Sorceress in disguise.
wait 1 s
emote chuckles to himself.
wait 1 s
say Well young one, if you've come to kill me know only that you will seal your own fate in doing so.
wait 3 s
say The path east will destroy you, there is only one way to get out of here alive.
wait 2 s
grin
wait 1 s
say Kneel before me.
~
#2710
(2708) only comes off in one room~
1 l 100
~
eval room %actor.room%
if (%room.vnum% == 2743)
%send% %actor% The shackle grows cold and loosens.
wait 1
%send% %actor% The shackle darkens and crumbles suddenly to dust.
%purge% %self%
else
%send% %actor% You try to remove %self.shortdesc%, but it refuses to open.
return (0)
end
~
#2711
(2718) Slip in Room after delay~
2 g 100
~
if %actor.is_pc%
  wait 5 sec
  eval room %actor.room%
  if %room.vnum% == 2718
    %send% %actor% Oops! You slip and hurt yourself... perhaps lingering here is not a good idea.
    %damage% %actor% 10
    if %actor.is_pc%
      wait 5 s
      eval room %actor.room%
      if %room.vnum% == 2718
        %send% %actor% Oops! You slip and hurt yourself... perhaps lingering here is not a good idea.
        %damage% %actor% 10
        wait 5 s
        if %actor.is_pc%
          eval room %actor.room%
          if %room.vnum% == 2718
            %send% %actor% Oops! You slip and hurt yourself... perhaps lingering here is not a good idea.
            %damage% %actor% 10
            wait 5 s
            if %actor.is_pc%
              eval room %actor.room%
              if (%room.vnum% == 2718)
                %send% %actor% Oops! You slip and hurt yourself... perhaps lingering here is not a good idea.
                %damage% %actor% 10
                wait 5 s
                eval room %actor.room%
                if (%room.vnum% == 2718)
                  %send% %actor% You slip even harder, landing flat on your back and skidding down the tunnel. That really HURT!
                  %damage% %actor% 70
                  %force% %actor% n
                end
              end
            end
          end
        end
      end
    end
  end
end
~
#2712
(2706) Child starts quest on entry~
0 gh 100
~
if (%actor.varexists(zn27_childquest)%)
wait 1 s
smile %actor.name%
wait 1 s
say I'm not lost anymore, thank you!
else
eval room %actor.room%
if (%room.vnum% == 2719)
wait 1 s
cry
wait 1 s
hug %actor.name%
wait 1 s
%send% %actor% A blind child tells you 'I'm lost!'
wait 2 s
%send% %actor% A blind child tells you 'Please help me find my room.'
wait 2 s
%send% %actor% A blind child tells you 'Just beckon and I will follow.'
else
wait 1 s
%send% %actor% A blind child tells you 'Have you found my room?'
wait 2 s
%send% %actor% A blind child tells you 'Just beckon when you want me to follow'
end
end
~
#2713
(2706) Child follows at beckon~
0 c 100
beckon~
wait 1 s
%send% %actor% A blind child tells you 'You know where my room is?!'
wait 1 s
clap
wait 1 s
%send% %actor% A blind child feels for your arm and grips it. 
fol %actor.name%
wait 1 s
%send% %actor% A blind child tells you 'Please pat me when you want me to stop following.'
~
#2714
(2706) Child stops following~
0 c 100
pat~
wait 1 s
eval room %actor.room%
if (%room.vnum% == 2724)
wait 1 s
follow self
%send% %actor% A blind child lets go of your arm and feels her way around the room.
wait 1 s
say This is my room!
wait 1 s
hug %actor.name%
wait 1 s
say Thank you so much!
wait 1 s
say I'm sure I can find my way around now.
wait 2 s
say I only went to hide something for my dad...
wait 2 s
say I think if he were here he would want you to have it though.
wait 2 s
say I hid it in that cupboard where you found me... in the floorboards.
 
set Zn27_childquest 1
remote Zn27_childquest %actor.id%
 
 
else
wait 1 s
follow %self%
%send% %actor% A blind child lets go of your arm and feels her way around the room.
wait 1 s
say This isn't my room!
wait 1 s
sniff
wait 1 s
say Just beckon when you know the way.
end
~
#2715
(2706) Child runs away after time~
0 b 100
~
* No Script
eval room %self.room%
if (%room.vnum% == 2719)
look
else
wait 100 s
say I am bored!
wait 1 s
%echo% A small child suddenly runs away.
%teleport% %self% 2719
end
~
#2716
(2703) Shopkeeper comments on shackle pass~
0 c 100
st~
if (%actor.varexists(Zn27_shacklereject)%)
%send% %actor% A weary memlin tells you: Beyond that door is only for those who are seeking their freedom.
wait 1 s
%force% %actor% stand
wait 2 s
%send% %actor% A weary memlin tells you: You are already free, are you not?
wait 2 s
%send% %actor% The memlin rattles the shackle on his wrist.
wait 2 s
%send% %actor% A weary memlin tells you: Present yourself a slave, as all my kind is, and then that place will welcome you.
rdelete Zn27_shacklereject %actor.id%
else
return 0
end 
~
#2717
(2707) Room resets shacklepass~
2 g 100
~
rdelete Zn27_shacklepass %actor.id%
~
#2718
(2763) circle teleports when kneel~
2 c 100
kneel~
if %arg%==creature
%send% %actor% You kneel before the skeletal creature.
wait 1 s
%send% %actor% You suddenly feel very strange as the creature gestures and an eerie mist surrounds you.
wait 3 s
      %teleport% %actor% 2712
%force% %actor% look
elseif %arg%==skeletal
%send% %actor% You kneel before the skeletal creature.
wait 1 s
%send% %actor% You suddenly feel very strange as the creature gestures and an eerie mist surrounds you.
wait 3 s
      %teleport% %actor% 2712
%force% %actor% look
elseif %arg%==circle
%send% %actor% You kneel inside the circle.
wait 1 s
%send% %actor% You suddenly feel very strange as an eerie mist surrounds you.
wait 3 s
 %teleport% %actor% 2712
%force% %actor% look
else
%send% %actor% Kneel to who?
end
~
#2719
(2729) Wall of fire prevents fleeing/leaving~
2 q 100
~
%send% %actor% 	RThe wall of fire burns you as you attempt unsuccessfully to pass through it.	n
%damage% %actor% 10
return 0
~
#2720
(2729) Wall of fire randomly flares up and decays~
0 k 30
~
eval here %self.room%
attach 2719 %here.id%
%echo% 	RThe sorceress raises her arms, creating a massive wall of flame around you.	n
wait 5 s
use staff
wait 8 s
%echo% 	RThe flames around you flicker and die, leaving a circle of ash.	n
detach all %here.id%
wait 3 s
~
#2721
TESTCOMBAT 1~
0 c 100
*~
if %cmd%==duck
  %echo% You quickly duck low.
  rdelete side %actor.id%
elseif %cmd%==parry
  %echo% You quickly parry.
  rdelete raise %actor.id%
elseif %cmd%==block
  %echo% You quickly block.
  rdelete stab %actor.id%
else
  return 0
end
~
#2722
testgun~
1 c 7
spirit~
if %cmd%==spirit
if %arg%==gun
if %actor.fighting%
%damage% %actor.fighting% 100
%send% %actor% A mighty blast of fire issues forth from your bracelet, enveloping your opponent in flames.
%echoaround% %actor% The fire bracelet sends out a stream of fire, enveloping %actor.name%'s opponent in flame.
end
end
end
~
#2723
(2741-56) Random spooky messages in temple~
2 b 40
~
eval person %self.people%
if (%person% > 0)
switch %random.15%
  case 0
    %echo% You hear the sound of footsteps creeping softly through the room.
  break
  case 1
  %echo% A sudden shriek pierces through the air and suddenly dies away.
  break
  case 2
%echo% A child's laughter echoes mysteriously through the room.
  break
  case 3
 %echo% A voice whispers: Trust nothing, illusion is the master of this place.
  break
  case 4
%echo% A bright red bead of fluid oozes from the wall and runs down onto the floor.
  break
  case 5
%echo% The room suddenly grows very cold, silent breath chilling your neck.
  break
  case 6
    %echo% A shadowy figure suddenly steps from the wall and flickers out of existance.
  break
case 7
%echo% The sound of glass shattering echoes through the temple.
break
case 8
%echo% Your skin starts to crawl, as though covered in insects.
break
case 9
%echo% Tiny cold fingers grasp your arm.
break
case 10
%echo% A small child tugs at your hand, only to giggle and run away.
break
case 11
%echo% The delicate tune of a music-box drifts through the room.
break
case 12
%echo% The sound of deep breathing rasps harshly against your ear.
break
case 13
%echo% A voice whispers: All you will find here is yourself.
break
case 14
%echo% Little handprints blossom along the wall, only to fade away.
break
case 15
%echo% A single tear forms in the air and splashes onto the floor.
break
  default
%echo% A small child tugs at your hand, only to giggle and run away.
  break
done
end
~
#2724
(2714) child quest on death~
0 f 100
~
%echo% 	BA partially-petrified memlin gasps with his dying breath: My daughter... find her... in the Sandy... Tunn...	n
~
#2725
test for Tink~
2 b 100
~
set tar 2700
while %tar%
  if %tar% < 2720
    %at% %tar% %echo% This is a test message.
    eval tar %tar%+1
  else
    set tar 0
  end
done
~
#2726
test randomroom~
2 b 100
~
eval person %self.people%
if (%person% > 0)
%echo% test works
end
~
#2727
test lightning~
1 b 100
~
if %self.worn_by%
eval actor %self.worn_by%
    %echoaround% %actor% Lightning cackles around %actor.name%
%send% %actor% Lightning cackles around you.
else
return 0
end
~
#2728
test dg_cast~
1 b 100
~
dg_cast 'poison' %actor%
~
#2729
(2723) Doll assists, runs away after 3 times~
1 b 100
~
if %self.carried_by%
eval actor %self.carried_by%
  if %actor.fighting%
    eval victim %actor.fighting%
    %echoaround% %actor% 	R%actor.name%'s doll suddenly opens its eyes and causes %victim.name% to shudder in pain.	n
    %send% %actor% 	RYour doll suddenly opens its eyes and causes %victim.name% to shudder in pain.	n
    %damage% %victim% 100
if (%actor.varexists(zn27_twice)%)
rdelete Zn27_twice %actor.id%
wait 3 s
%send% %actor% 	RYour doll turns its head to look at you.	n
wait 1 s
%send% %actor% 	RYour doll says 'Thrice I have repaid my debt.'	n
wait 1 s
%send% %actor% 	RYour doll says 'And still I await my freedom.'	n
wait 1 s
%send% %actor% 	RThe doll suddenly transforms into a little girl and runs away.'	n
rdelete Zn27_offereddoll %actor.id%
%purge% %self%
elseif (%actor.varexists(zn27_once)%)
rdelete Zn27_once %actor.id%
set Zn27_twice 1
      remote Zn27_twice %actor.id%
else
set Zn27_once 1
      remote Zn27_once %actor.id%
end
end
end
~
#2730
test elaseth trigger~
0 c 100
get~
if %cmd%==get
if %arg%== book
say Hrm. That's not a toy, and I wish you'd put it back - for your own good.
wait 4
emote folds his arms and glowers at %actor.name%.
elseif%arg%== talabrax
say Hrm. That's not a toy, and I wish you'd put it back - for your own good.
wait 4
emote folds his arms and glowers at %actor.name%.
elseif%arg%==arcanum)
say Hrm. That's not a toy, and I wish you'd put it back - for your own good.
wait 4
emote folds his arms and glowers at %actor.name%.
else
return 0
end
end
~
#2731
(2709) Fire wrm loads spine at death~
0 f 100
~
%load% obj 2724
%echo% 	yAs the wrm collapses in death, one of its spines breaks off.	n
~
#2732
(2726) shard changes colour~
1 g 100
~
wait 1 s
if (%actor.class% == Cleric)
%echoaround% %actor% 	BThe shard suddenly glows blue in %actor.name%'s hand.	n
%send% %actor% 	BThe shard suddenly glows blue in your hand, causing you to drop it.	n
wait 2 s
%load% obj 2727
%echo% 	BThe shard whispers: Healer... heal thyself.	n
%purge% %self%
elseif (%actor.class% == Magic User)
%echoaround% %actor% 	MThe shard suddenly glows purple in %actor.name%'s hand.	n
%send% %actor% 	MThe shard suddenly glows purple in your hand, causing you to drop it.	n
wait 2 s
%load% obj 2730
%echo% 	MThe shard whispers: Seeker... seek thyself.	n
%purge% %self%
elseif (%actor.class% == Thief)
%echoaround% %actor% 	GThe shard suddenly glows green in %actor.name%'s hand.	n
%send% %actor% 	GThe shard suddenly glows green in your hand, causing you to drop it.	n
wait 2 s
%load% obj 2729
%echo% 	GThe shard whispers: Deceiver... know thyself.	n
%purge% %self%
elseif (%actor.class% == Warrior)
%echoaround% %actor% 	RThe shard suddenly glows red in %actor.name%'s hand.	n
%send% %actor% 	RThe shard suddenly glows red in your hand, causing you to drop it.	n
wait 2 s
%load% obj 2728
%echo% 	RThe shard whispers: Conqueror... conquer thyself.	n
%purge% %self%
end
~
#2733
(2742) Girl transforms to doll~
2 g 100
~
if (%actor.varexists(zn27_offereddoll)%)
wait 1 s
%echo% A chill runs down your spine.
else
wait 1 s
%echo% A little girl walks into the room and peers at you curiously.
wait 2 s
%echo% The little girl whispers 'Are you real?'
wait 2 s
%echo% Shrinking suddenly, the little girl crumples into a heap on the floor.
wait 2 s
%echo% Writhing, she seems to transform into a small doll lying on the ground.
%load% obj 2723
wait 2 s
%echo% The doll whimpers 'Take me with you... please!'
set Zn27_offereddoll 1
      remote Zn27_offereddoll %actor.id%
wait 30 s
%purge%
end
~
#2734
(2702) rdelete offereddoll~
2 g 100
~
if (%actor.varexists(zn27_offereddoll)%)
rdelete Zn27_offereddoll %actor.id%
end
~
#2735
(2764) Deathtrap message~
2 g 100
~
%echo% You realise too late that only an empty chasm lies below. Plummeting faster and faster, you don't even have time to pray before you hit the ground!
wait 2 s
%purge%
eval person %self.people%
if (%person% > 0)
%teleport% %actor% 2700
%force% %actor% look
%send% %actor% 	RIf you weren't immortal you'd have just splatted.	n
end
~
#2736
(2765) Rubble falls, causing damage~
2 g 100
~
wait 7 s
if %actor.is_pc%
eval room %actor.room%
if (%room.vnum% == 2765)
%send% %actor% Ouch, several sharp rocks fall suddenly from the ceiling.
%damage% %actor% 40
end
end
~
#2737
test inv check~
1 c 1
shoot~
if %cmd%==shoot
  eval i %actor.inventory%
  while (%i%)
    set next %i.next_in_list%
    if %i.vnum%==2731
      set quiver 1
      break
    end
    set i %next%
  done
if %quiver%
  %force% %actor% take arrow quiver
  %send% %actor% You prepare to shoot.
  if %actor.inventory(2733)%
    %echo% About to nod
    set ready 1
    remote ready %actor.id%
    %force% %actor% steam arrow
  end
else
  %send% %actor% You need to have arrows either on you or in a quiver.
end
~
#2738
test obj get~
1 c 2
steam~
if %cmd%==steam && %arg%==arrow && %actor.varexists(ready)% && %self.carried_by%
  eval actor %self.carried_by%
  if %actor.fighting%
    eval victim %actor.fighting%
    %damage% %victim% 20
    %send% %actor% You shoot an arrow at your opponent.
    rdelete ready %actor.id%
    %echoaround% %actor% %actor.name%'s opponent shudders, hit by an arrow.
    %load% obj 2733
    %purge% %self%
  else
    %send% %actor% You have to be fighting someone.
  end
end
~
#2739
(2727) blue shard for healer~
1 j 100
~
if (%actor.class% == Magic User)
%send% %actor% 	MThe shard whispers: I am not for you, Seeker.	n
return 0
elseif (%actor.class% == Thief)
%send% %actor% 	GThe shard whispers: I am not for you, Deceiver.	n
return 0
elseif (%actor.class% == Warrior)
%send% %actor% 	RThe shard whispers: I am not for you, Conqueror.	n
return 0
end
~
#2740
(2728) red shard for conqueror~
1 j 100
~
if (%actor.class% == Magic User)
%send% %actor% 	MThe shard whispers: I am not for you, Seeker.	n
return 0
elseif (%actor.class% == Cleric)
%send% %actor% 	BThe shard whispers: I am not for you, Healer.	n
return 0
elseif (%actor.class% == Thief)
%send% %actor% 	GThe shard whispers: I am not for you, Deceiver.	n
return 0
end
~
#2741
(2729) green shard for deceiver~
1 j 100
~
if (%actor.class% == Magic User)
%send% %actor% 	MThe shard whispers: I am not for you, Seeker.	n
return 0
elseif (%actor.class% == Cleric)
%send% %actor% 	BThe shard whispers: I am not for you, Healer.	n
return 0
elseif (%actor.class% == Warrior)
%send% %actor% 	RThe shard whispers: I am not for you, Conqueror.	n
return 0
end
~
#2742
(2730) purple shard for seeker~
1 j 100
~
if (%actor.class% == Cleric)
%send% %actor% 	BThe shard whispers: I am not for you, Healer.	n
return 0
elseif (%actor.class% == Thief)
%send% %actor% 	GThe shard whispers: I am not for you, Deceiver.	n
return 0
elseif (%actor.class% == Warrior)
%send% %actor% 	RThe shard whispers: I am not for you, Conqueror.	n
return 0
end
~
#2743
(2736) fire wrm dies on entry~
2 g 100
~
if !(%actor.varexists(wrm)%)
wait 1 s
%echo% A 	Rfire wrm	n enters the room, squealing as the ice suddenly singes its skin.
%load% mob 2709
%damage% wrm 1000
set wrm 1
    remote wrm %actor.id%
end
~
#2744
test door~
2 c 100
enter~
if ("%arg%" == "space")
%send% %actor% A previously concealed space suddenly becomes apparent.
%echoaround% %actor% %actor.name% has discovered a hidden space!
%door% 2740 down flags a
%door% 2740 down room 2741
%door% 2740 down description Nothing can be seen within this dark space.
%door% 2741 up flags a
%door% 2741 up room 2740
%door% 2741 up description Darkness obscures whatever may be within.
else
%send% %actor% Enter what ?!
end
~
#2745
test remote~
1 g 100
~
set test 1
remote test %actor.id%
wait 3 s
eval test %test% + 2
remote test %actor.id%
wait 3 s
eval test %test% - 5
remote test %actor.id%
~
#2746
test detach~
0 c 100
nod~
%echo% this is the first part (next in 5 secs)
wait 5 s
%echo% this is the second part (next in 5 secs)
wait 5 s
%echo% this is the final part
~
#2747
TESTCOMBAT 2~
0 k 100
~
switch %random.3%
  case 0
    %echo% The warrior raises his sword.
set raise 1
remote raise %actor.id%
wait 4 s
if (%actor.varexists(raise)%)
damage %actor% 50
%echo% The warrior brings his sword down on your head.
else
%echo% You successfully evade the attack.
rdelete raise %actor.id%
end
  break
  case 1
 %echo% The warrior swings his sword to the side.
set side 1
remote side %actor.id%
wait 4 s
if (%actor.varexists(side)%)
damage %actor% 50
%echo% The warrior slashes his sword into your neck.
else
%echo% You successfully evade the attack.
rdelete side %actor.id%
end
  break
  case 3
  %echo% The warrior stabs his sword at you.
set stab 1
remote stab %actor.id%
wait 4 s
if (%actor.varexists(stab)%)
damage %actor% 50
%echo% The warrior stabs his sword through your heart.
else
%echo% You successfully evade the attack.
rdelete stab %actor.id%
end
 break
default
%echo% The warrior raises his sword.
set raise 1
remote raise %actor.id%
wait 4 s
if (%actor.varexists(raise)%)
damage %actor% 50
%echo% The warrior brings his sword down on your head.
else
%echo% You successfully evade the attack.
rdelete raise %actor.id%
end
  break
done
~
#2748
test attach~
1 c 1
nod~
%echo% This trigger commandlist is not complete!
attach 2739 %self.id%
~
#2749
new trigger~
1 c 1
noh~
%echo% testing testing
~
#2750
test attach~
1 c 1
fix~
attach %arg%
~
#2751
test act~
1 c 7
look~
%force% %actor% %at% 2755 look
~
#2752
test speech~
0 d 1
*~
%at% 2740 %echo% %actor.name%'s voice rasps through the intercom: %speech%
~
#2753
teleport~
1 c 7
move~
%teleport% %arg%
~
#2754
(2763) toothy loads on reset~
2 af 100
~
eval person %self.people%
if (%person% < 1)
%purge%
%load% mob 2713
%load% obj 2735
end
~
#2755
(2713) toothy holds orb on load~
0 n 100
~
%load% obj 2738
hold orb
~
#2756
test nop~
1 c 7
test~
set amount 100
nop %actor.gold(10)%
nop %actor.gold(%amount%)%
~
#2757
(2720) child smiles on entry~
0 gh 100
~
wait 1 s
emote smiles at you.
wait 1 s
say Hey, I learned some neat things about Dynar magic today.
wait 1 s
say Want to hear?
~
#2758
test~
2 f 100
~
%load% obj 14902
~
#2759
test door~
0 c 100
test~
eval where %self.room%
if %where.east(bits)% == DOOR CLOSED
  return 0
else
  say Impudent fool! Your insolence forces me to take action!
  %damage% %actor% 100
end
~
#2760
(2721) cui greets~
0 gh 100
~
if %actor.class% == Cleric
  set zn27_class healer
end
if %actor.class% == Warrior
  set zn27_class conqueror
end
if %actor.class% == Magic User
  set zn27_class seeker
end
if %actor.class% == Thief
  set zn27_class deceiver
end
if %actor.name% == %zn27_first%
  %send% %actor% The mighty Cui tips its head acknowledgingly in your direction.
  %echoaround% %actor% The mighty Cui tips its head toward %actor.name%.
elseif %actor.is_pc%
  wait 2 s
  say Welcome %zn27_class%, I am Nyah, one of the Cui... or at least a memory of them.
  wait 4 s
  say My existance, I'm afraid is merely an illusion. But you, my friend, are undoubtedly real.
  wait 4 s
  global zn27_class
  set zn27_greeted %actor.name%
  global zn27_greeted
  set zn27_first %actor.name%
  say Will you speak your name?
  global zn27_first
end
~
#2761
(2721) cui responds to name~
0 d 100
*~
if %actor.sex% == female
  set gender woman
elseif %actor.sex% == male
  set gender man
else
  set gender person
end
if %actor.name% == %zn27_greeted%
  if %speech.car% == %actor.name%
    wait 1 s
    say Ah, it is refreshing to meet a %gender% unafraid to reveal who %actor.heshe% truly is.
    wait 2 s
    say A pleasure to meet you %actor.name%.
    wait 2 s
    emote bows its head politely.
  else
    wait 1 s
    emote peers closely at you.
    wait 1 s
    say I know this is not your name, %zn27_class%.
    wait 1 s
    say But it is yours to give or withhold, I will press you no further.
    wait 2 s
    emote bows its head politely.
  end
end
set zn27_greeted off
global zn27_greeted
~
#2762
(2722) Ve greets~
0 gh 100
~
if %actor.class% == Cleric
  set zn27_class healer
end
if %actor.class% == Warrior
  set zn27_class conqueror
end
if %actor.class% == Magic User
  set zn27_class seeker
end
if %actor.class% == Thief
  set zn27_class deceiver
end
wait 1 s
smile %actor.name%
wait 1 s
say Welcome, %zn27_class%.
~
#2763
(2726) Revealer greets~
0 gh 100
~
if %actor.is_pc%
wait 2 s
say Welcome, visitor.
wait 2 s
say Tell me, do you seek knowledge of this place?
wait 2 s
say You need only answer yes or no.
end
~
#2764
(2726) yes answer teleports~
0 d 100
yes~
wait 1 s
say Very well, then knowledge shall be revealed.
wait 1 s
%send% %actor% The Revealer waves his hand slowly over your eyes.
%echoaround% %actor% The Revealer waves his hand slowly over %actor.name%'s eyes.
wait 1 s
%teleport% %actor% 2785
%force% %actor% look
%force% %actor% xxmemlinxx
return 0
~
#2765
(2726) no ends conversation~
0 d 100
no~
wait 1 s
say Then knowledge shall not be found.
wait 1 s
emote tips his head politely.
~
#2766
(2728) memlin initially greets~
0 c 100
xxmemlinxx~
 wait 1 s
bow %actor.name%
wait 1 s
say Ah! Welcome!
wait 1 s
say If you would like me to tell you more about this place, just beckon and I will follow!
~
#2767
(2728) memlin follows when beckoned~
0 c 100
bec~
%send% %actor% You beckon a young memlin.
%echoaround% %actor% %actor.name% beckons a young memlin.
smile
wait 1 s
fol %actor.name%
wait 1 s
say If you wish me to stop following at any time, just say stop.
wait 1 s
say In any of these rooms, if you would like to hear what I have to say, just say explain.
eval where %self.room%
if %where.vnum% == 2785
  wait 1 s
  say The way down will return you to the temple.
end
~
#2768
(2728) memlin stops following when asked~
0 d 100
stop~
wait 1 s
say Very well, though just beckon if you should want me again.
wait 1 s
fol self
wave %actor.name%
%echo% A young memlin returns to his place.
%teleport% %self% 2785
~
#2769
(2728) memlin greets~
0 gh 100
~
wait 1 s
say Greetings! Just beckon me if you would learn more of this place.
~
#2770
(2728) entry into 2743 not permitted~
0 i 100
~
eval where %self.room%
if %where.vnum% == 2743
  fol self
  wait 1 s
  say Ah, this is the way out, I can go no further.
  wait 1 s
  bow
  %echo% A young memlin leaves the room.
  %teleport% %self% 2785
elseif %where.vnum% == 2791
  wait 1 s
  shiver tor
elseif %where.vnum% == 2796
  wait 1 s
  pet rabbit
elseif %where.vnum% == 2797
  wait 1 s
  hug little
elseif %where.vnum% == 2795
  wait 1 s
  smile dynar
elseif %where.vnum% == 2794
  wait 1 s
  smile khan'li
end
~
#2771
(2728) different info for rooms~
0 d 100
explain~
emote clears his throat.
wait 1 s
eval room %self.room%
if %room.vnum% == 2785
  say This is the beginning of all things. A delicate balance of darkness and light, neither conquering the other.
  wait 3 s
  say The cosmos must be in motion for there to be life. If it becomes still, it will stagnate and die.
elseif %room.vnum% == 2787
  say This is the first of the two forms of existance...
  wait 1 s
  say Miru, the inertia, the solidity that builds up and crumbles down.
  wait 1 s
  pat sculpture
  wait 1 s
  say This is Miru, stone and gems and dust... all Miru. You and I also.
elseif %room.vnum% == 2786
  say This is the second of the two forms of existance...
  wait 1 s
  say Lamen, the breeze, the energy that flows and shifts and never dies.
  wait 1 s
  caress flame
  wait 1 s
  say This is Lamen, and the forked light that appears in the sky.
elseif %room.vnum% == 2788
  say The Imari stir the universe... keep it from dying.
  wait 1 s
  say The scales must be always in motion, light and dark can never stop dancing.
  wait 2 s
  say Their will can always be felt... like a pull, the force of Navi.
elseif %room.vnum% == 2789
  say Imari have the ability to draw upon their own life energy, scattering it in an immense burst of lamen to seed new forms of weaker life.
  wait 3 s
  say The Imari must sacrifice themselves for the birth of their creation. The waves of energy that ripple through the universe on this self-scattering are called Natul.
  wait 2 s
elseif %room.vnum% == 2790
  say The first wave of Natul always produces the most powerful beings, a union of lamen and miru in a form that perceives most keenly the pull of the Imari and the ebbing and flowing of the universe.
  wait 4 s
  say These are the Cui, the Dragons, the mighty beasts whose bones litter every seeded world like a stamp of creation.
  wait 3 s
  say Their bodies are mostly miru shells, with the majority of their lamen existance concentrating in their eyes, giving them powers of perception unlike anything known to miru life.
  wait 4 s
  say The Cui also have the ability to procreate, taking the form of Denuo and like the Imar who made them, trickling their lamen into the miru population.
  wait 3 s
  say Slowly becoming more and more miru themselves until mortality takes them and they crumble into the substance of the world.
elseif %room.vnum% == 2792
  say The direct descendants of a Cui and a Denuo joining are known as Ve.
  wait 1 s
  say They inherit characteristics of both their parents, usually turning to prophesying and magic arts, their skills and powers outdoing even the strongest of their Denuo brothers.
  wait 3 s
  say Ve also have extra powers of perception and foresight, though at times they are uncertain and unclear.
elseif %room.vnum% == 2791
  say The sole restriction on Cui life and an inherent knowledge manifested strongly in their minds is that procreation must be only with Denuo life.
wait 4 s
  say The union of Cui with Cui concentrates, rather than dilutes the portion of lamen in the resulting life, creating an unstable orb of energy called a Tor that distorts the universe around it like a bubble.
wait 7 s
  say A Tor is a sphere of Navi-sensitve lamen that acts as containment for the unnatural offspring, enclosing it and holding it in a state of stasis with the very will of the Imari.
wait 5 s
  say Tors have strange and unstable influences on the existance around them, often leading them to be perceived as cursed.
wait 4 s
  say Very rarely, a Tor will suddenly fail whenever balance shifts extremely in the universe. These occasional shifts demand redirection of Imari focus, causing the power of the Tor to weaken and allowing the lifeform to awaken and emerge.
wait 7 s
  say The lifeform that emerges from a Tor is called a Nevim, a potent and unnatural concentration of lamen life, closest in power to the Imari themselves. 
wait 4 s
  say Nevim do not feel the pull of Navi and do not perceive the greater state of the cosmos, using their powers according to their own will and wreaking havoc upon the universal balance. 
wait 5 s
  say Shunned through fear and isolated from every other form of life, Nevim become bitter and angry, forces of destruction and causing much pain in an attempt to exorcise their own.
  wait 1 s
elseif %room.vnum% == 2793
  say The second wave brings forth the Denuo, the lesser beings, the second-born who live and die without changing form, colouring the lamen that flows through them but being entirely of miru. 
  wait 3 s
  say The Khan'li, the Dynar, and myself of Memlin kind, are all examples of Denuo life.
elseif %room.vnum% == 2794
  say Khan'li embrace the darkness of night and the heat of summer, inheriting through their forebearer Cui a kinship with fire, which they are not harmed by, though simple water acts like acid on their skin.
  wait 4 s
  say Black and red are their colours and they enjoy sharp points, reflective surfaces for their inclination to repel light and singular works of beauty, believing themselves superior as the firstborn.
  wait 4 s
  say They live mainly underground or in caves, seeking to escape the occasional rains and sculpting their showy palaces into mountains. 
  wait 3 s
  say They believe in domination of the strong over the weak and have hardly any sense of guilt or compassion, taking no pleasure in cruelty but exercising it without hesitation for the slightest benefit.
  wait 4 s
  say It is said that the Khan'li cannot love, but they will endure all manner of suffering for a person that has invoked the Lyra or fire-spirit in them (a kind of blazing obsession), even holding them captive if they are unwilling or untrustworthy.
  wait 4 s
  say They are passionate, ambitious, powerful, proud, single-minded and fearless.
elseif %room.vnum% == 2795
  say Dynar are smaller than the Khan'li, but just as adept at fighting, extremely fast, able to contort their bodies amazingly and putting their skills to use in the construction of elaborate weapons and studying of the world. 
  wait 4 s
  say Pale-skinned, their blood is as the milky sap of trees, having slight phosphorescent properties which causes them to glow faintly in darkness.
  wait 3 s
  say They believe in the superiority of light over darkness, often trying to 'improve' their fellow Denuo.
  wait 2 s
  say The Dynar are intensely private, fearing corruption from outside forces and are easily broken in spirit if separated from their own kind.
  wait 3 s
  say Their strengths lie in their ability to work with natural forces, having a keen friendship with animal-kind and able to speed the growth of plant life.
  wait 3 s
  say Water is their inheritance and they are able to breathe as easily in it as in air. They are gentle, thoughtful, meditative, deeply loyal, private and highly intelligent.
elseif %room.vnum% == 2797
  chuckle
  wait 1 s
  say Well I may be slightly biased here, but...
wait 3 s
  say Peace-loving and highly spiritual, the memlins are most in touch with the flow of Navi and are constantly trying to bridge the gap between the Dynar and the Khan'li, dedicating much of their art to the beauty of both races.
wait 6 s
  say Well adapted to living and digging small tunnel networks for themselves, the memlins occasionally also build habitats in deep forests.
wait 4 s
  say The memlin skills are a mystery to most as they are an unaggressive race. It is known that they do not bleed, having bodies that rise and return to earth and that neither fire nor water harm them.
wait 6 s
  say In times of great need, it is said that they can call instant death upon an enemy, though these cases are so rarely witnessed that they are often believed to be myth.
wait 5 s
  say In any case, I have never seen it.
elseif %room.vnum% == 2796
  say After the first and second wave, the Natul continues rippling.
  wait 2 s
  say Every ebbing wave thereafter produces more of this miru life, rendering it lesser and weaker until the Natul is utterly spent.
end
wait 2 s
emote has finished speaking.
~
#2772
(2700) different eq on load~
0 n 100
~
switch %random.3%
  case 1
    %load% obj 2750
    wear tunic
  break
  case 2
    %load% obj 2752
    wear shirt
  break
  case 3
    %load% obj 2753
    wear vest
  break
  default
    %load% obj 2752
    wear shirt
  break
done
switch %random.3%
  case 1
    %load% obj 2754
    wear pants
  break
  case 2
    %load% obj 2755
    wear trousers
  break
  case 3
    %load% obj 2756
    wear breeches
  break
  default
    %load% obj 2755
    wear trousers
  break
done
switch %random.3%
  case 1
    %load% obj 2757
    wear sandals
  break
  case 2
    %load% obj 2758
    wear shoes
  break
  case 3
    %load% obj 2760
    wear boots
  break
  default
    %load% obj 2758
    wear shoes
  break
done
switch %random.2%
  case 1
    %load% obj 2751
    wear cord
  break
  case 2
    %load% obj 2759
    wear belt
  break
  default
    %load% obj 2751
    wear cord
  break
done
switch %random.2%
  case 1
    %load% obj 2714
  break
  case 2
    %load% obj 2761
    wear cloth
  break
  default
    %load% obj 2761
    wear cloth
  break
done
wear all
~
#2773
(2708) Keeper gives book~
0 gh 100
~
if (%direction% == south)
  return 1
elseif (%direction% == west)
  return 1
elseif (%direction% == east)
  return 1
elseif (%direction% == north)
  return 1
else
  smile %actor.name%
  wait 1 s
  say If you are eager to learn, this may be of interest.
  wait 1 s
  %load% obj 2765
  give book %actor.name%
end
~
#2774
(2707) pheasant loads meat~
0 f 100
~
%load% obj 2766
~
#2775
(2719) memlin asks if hungry~
0 gh 100
~
if %self.has_item(2763)%
  if %actor.is_pc%
    wait 1 s
    smile %actor.name%
    wait 1 s
    say Hi sweetie, are you looking for something to eat?
  end
end
~
#2776
(2719) memlin gives food~
0 d 1
yes~
* This is just a check to see if the mob is carrying an item, put it in there
* so that only one version of the mob responds.
if %self.has_item(2763)%
  wait 1 s
  * The mob loads the item and gives it to the person saying "yes".
  %load% obj 2768
  give meat %actor.name%
  smile
end
~
#2778
(2716) rat loads meat~
0 f 100
~
%load% obj 2771
~
#2779
(2780) choke on smoke~
2 g 100
~
wait 1 s
%send% %actor% You choke as the hot smoke singes your lungs.
%echoaround% %actor% %actor.name% chokes as the hot smoke singes %actor.hisher% lungs.
%damage% %actor% 10
~
#2780
(2772) dress burns actor~
1 b 100
~
if %self.worn_by%
  eval actor %self.worn_by%
  if !%actor.has_item(2773)%
    %send% %actor% You feel a stab of pain as %self.shortdesc% burns you.
    %echoaround% %actor% %actor.name% cringes with pain as %self.shortdesc% burns %actor.himher%.
    %damage% %actor% 15
  end
end
~
#2781
(2776) ring increases maxmana~
1 c 1
use~
if %arg% == ring
  %send% %actor% 	C You attempt to draw on the power of %self.shortdesc%. 	n
  %echoaround% %actor% 	C %actor.name% attempts to draw on the power of %self.shortdesc%. 	n
  wait 1 s
  if %self.timer% == 0
    eval give %actor.maxmana% * 2
    dg_affect %actor% maxmana %give% 10
    %send% %actor% 	C You glow with energy as %self.shortdesc% infuses you with magical potential. 	n
    %echoaround% %actor% 	C %actor.name% glows with energy as %self.shortdesc% infuses %actor.himher% with magical potential. 	n
    otimer 20
  else
    %echo% 	c Alas, the power of %self.shortdesc% has not yet recovered. 	n
  end
end
return 0
~
#2782
test otimer~
1 f 100
~
otimer 1
%echo% The ice cream melts away.
~
#2783
(2776) mana ring recharges~
1 f 100
~
eval actor %self.worn_by%
%send% %actor% 	C Your icy mana ring glows faintly blue, renewed with magical force. 	n 	n 	n 	n
%echoaround% %actor% 	C %actor.name%'s icy mana ring glows faintly blue, renewed with magical force. 	n
~
#2784
test corpse purge (use with 2785)~
1 c 7
xx27testxx~
wait 1
%purge% %self.room.contents%
%echo% fires
%purge% self
~
#2785
test corpse purge (use with 2784)~
0 f 100
~
%load% obj 2778 %actor% inv
%force% %actor% xx27testxx
~
#2786
(31) test mob loads test obj~
0 n 100
~
%load% obj 2778
~
#2787
(79) otimer set on staff~
1 n 100
~
otimer 5
~
#2788
test time~
0 c 100
test~
%purge% %self%
~
#2789
(2777) staff burns if take~
1 g 100
~
%send% %actor% You try to take %self.shortdesc% but its fire elemental hisses and burns you.
%echoaround% %actor% %actor.name% tries to take %self.shortdesc% but its fire elemental hisses and burns %actor.himher%.
%damage% %actor% 10
return 0
~
#2790
(2729) sorceress equips on load~
0 n 100
~
%purge%
%load% obj 2777
%load% obj 2776
%load% obj 2773
%load% obj 2772
wear all
hold staff
~
#2791
(2729) sorceress' temporary death~
0 l 50
~
eval here %self.room%
detach all %here.id%
attach 2792 %here.id%
%echo% %self.name% shrieks with rage.
wait 1 s
%echo% An enormous surge of fire seems to explode from within %self.name%, engulfing the room in a strange flame.
xxtestxx
%purge% %self%
~
#2792
test~
2 c 100
xxhealxx~
if %actor.level% > 30
  return 0
  halt
else
  if %cmd% == xxhealxx
    set room_var %actor.room%
    set target_char %room_var.people%
    while %target_char%
      set tmp_target %target_char.next_in_room%
      %send% %target_char% You feel healed.
      %damage% %target_char% -100
      set target_char %tmp_target%
    done
  else
    return 0
  end
end
~
#2793
(2779) staff reincarnates sorceress~
1 f 100
~
eval here %self.room%
%echo% %self.shortdesc%'s fire elemental makes some magical movements.
wait 1 s
%echo% A burst of flame erupts from %self.shortdesc%, reincarnating the Sorceress.
%load% mob 2729
%zoneecho% %here.vnum% The voice of the Sorceress shrieks: Fools! I cannot be destroyed so easily!
%purge% %self%
~
#2794
(2738) orb can be used~
1 c 3
use~
if %arg% == orb
  %echo% You draw upon the power of %self.shortdesc%.
  %echoaround% %actor% %actor.name% draws upon the power of %self.shortdesc%.
  %echo% The room glows briefly with a wave of light.
  eval here %self.room%
  if %here.vnum% == 2784
    %force% %actor% xxorbxx
  end
else
  return 0
end
~
#2795
(2729) sorceress stops orb~
0 c 100
xxorbxx~
%echo% 	RA fire elemental shrieks in terror at the sudden blast, but the Sorceress quickly raises a magical wall of fire.	n
wait 2 s
%echo% 	RThe fire absorbs the blue light from the orb.	n
wait 1 s
emote snarls: Where did you get that?!
~
#2796
(2779) orb destroys staff~
1 c 100
xxorbxx~
%echo% 	CA fire elemental shrieks and sizzles as a blue wave of light hits it.	n
wait 2 s
%echo% The death cry of the Sorceress can be heard as her form materializes and slumps to the ground, dissolving into smoke.
%load% obj 2773
%load% obj 2776
%load% obj 2772
wait 2 s
%echo% 	CA fire elemental finally withers, shrivelling into a tiny black skeleton.	n
%purge% %self%
~
#2797
(2720) memlin explains Dynar magic~
0 d 100
yes~
emote smiles happily.
wait 1 s
say Its like this, see... 
wait 1 s
emote stands up and traces a circle in the sand.
wait 2 s
say They make these magic circles... that glow like moonlight.
wait 2 s
say And if you kneel inside one it will take you away to a safe place.
wait 3 s
say I've never seen one.. but my dad has.. he's the one who told me.
wait 2 s
emote smiles and goes back to daydreaming.
~
#2798
test remote~
2 c 100
test~
if %actor.varexists(zn118_a)%
  eval zn118_a %actor.zn118_a% - 1
  remote zn118_a %actor.id%
else
  set zn118_a 1
  remote zn118_a %actor.id%
end
%echo% %actor.zn118_a%
~
#2799
name-generator~
2 c 100
test~
eval 1choice %random.12%
set name[1] b
set name[2] d
set name[3] f
set name[4] g
set name[5] h
set name[6] k
set name[7] p
set name[8] s
set name[9] v
set name[10] br
set name[11] gr
set name[12] sp
eval first %%name[%1choice%]%%
eval 2choice %random.5%
set name[1] a
set name[2] e
set name[3] i
set name[4] o
set name[5] u
eval second %%name[%2choice%]%%
eval 3choice %random.11%
set name[1] b
set name[2] d
set name[3] g
set name[4] p
set name[5] t
set name[6] mble
set name[7] ddle
set name[8] ngle
set name[9] ggle
set name[10] pple
set name[11] ndle
eval third %%name[%3choice%]%%
eval 4choice %random.5%
set name[1] a
set name[2] e
set name[3] i
set name[4] o
set name[5] u
eval fourth %%name[%4choice%]%%
eval 5choice %random.7%
set name[1] b
set name[2] d
set name[3] g
set name[4] p
set name[5] t
set name[6] rt
set name[7] ld
eval fifth %%name[%5choice%]%%
eval 6choice %random.8%
set name[1] f
set name[2] h
set name[3] s
set name[4] m
set name[5] n
set name[6] p
set name[7] y
set name[8] w
eval sixth %%name[%6choice%]%%
eval name %first%%second%%third%%sixth%%fourth%%fifth%
%echo% %name%
~
$~
