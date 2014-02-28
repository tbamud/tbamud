#20101
Seabreeze/Landbreeze - All Rooms~
2 b 75
~
if ( %time.hour% >=7 && %time.hour% <=19)
  %echo% 	WA seabreeze arrives from the south, bringing in the smell of salt.	n
  return 0
else
  %echo% 	WA landbreeze arrives swiftly from the north.	n
  return 0
end
~
#20102
Seawaves for the Seashore~
2 b 75
~
%echo% 	CThe waves roll onto the beach and breaks gently...	n
return 0
~
#20103
Seabreeze only - all~
2 b 100
~
if ( %time.hour% >=7 && %time.hour% <=19)
  %echo% 	WA seabreeze arrives from the south, bringing in the smell of salt.	n
  return 0
end
~
#20104
water swelling - shallow waters~
2 b 50
~
%echo% 	CThe waves crawl inland, swelling occasionally as an incipient wind arrives from the open ocean.	n
return 0
~
#20105
deep wave~
2 b 50
~
%echo% 	WA wind howls and blows forcefully, forming froth at the surface of the dark waters.	n
return 0
~
#20106
tree shake - 20105~
1 c 100
shake~
if %arg% ==  tree
  %send% %actor% You shake the tree, and a coconut falls out!
  %echoaround% %actor% %actor.name% shakes a coconut tree and a coconut falls out!
  %load% obj 20104
else
  %force% %actor% shake %arg%
end
~
#20107
waves + wind~
2 b 75
~
eval line %random.2%
switch %line%
  case 1
    %echo% 	WHarsh, cold wind tears at you in all directions.	n       
  break
  default
    %echo% 	BThe waves rush in as the ocean body shifts...	n
  break
done
~
#20108
wind+pounding~
2 b 80
~
eval line %random.2%
switch %line%
  case 1
    %echo% 	WHarsh, cold wind tears at you in all directions.	n       
    return 0
  break
  default
    %echo% 	BThe incessant waves pounds the base of the cliff...	n
    return 0
  break
done
~
#20109
seagull eat the crabs~
0 d 100
test~
%echo% FINDOBJ:    There are %findobj.1233(1300)% objects of vnum 1300 in roo
m 1233.
%echo%             There is %findobj.1233(1332664)% object of ID 1332605 in r
oom 1233.
%echo%             There are %findobj.1233(app)% objects of name app in room 
1233.
%echo%             There are %findobj.1233(apprehension)% objects of name app
in room 1233.
~
#20110
the crabs sleep - 20101~
0 b 100
~
if ( %time.hour% <=7 || %time.hour% >=19)
  eval line %random.5%
  switch %line%
    case 1
      sleep
    break
    case 2
    break
    default
    break
  done
else 
  eval line %random.10%
  switch %line%
    case 1
      wake
      rest
    break
    case 2
      %echo% A crab burrows itself into the sand and disappears beneath the grains.
      %purge% %self%
      wait 10 secs
    break
    default
      wake
      stand
    break
  done
end
~
#20111
seagull sleeps/wake - 20103~
0 b 100
~
if ( %time.hour% <=5 || %time.hour% >=18)
  south
  %teleport% %self% 20110
  wait 2 secs
  sleep
else
  %echo% The seagull ruffles its wings and starts preening itself.
  wake
  stand
end
~
#20112
Siren sings to all around her - 20104~
2 g 100
~
  if (%actor.sex% == male)
    %send% %actor% You hear someone sing, the voice overpowering your senses and strangely luring...
    wait 2 secs
    %send% %actor% You feel dizzy, as your feet takes you towards the voice...
    %echoaround% %actor% %actor.name% has a blank look on his face as he wanders off...
    %force% %actor% south
    %force% %actor% look
  end
~
#20113
Siren confuses the male - 20104~
0 g 100
~
%echo% A siren smile sweetly as you approach her.
if (%actor.name%==Elixias)
  return 0
else
  if (%actor.sex%==male)
    %send% %actor% A siren slides her body against yours seductively.
    wait 5 sec
    %echo% The siren starts singing, and walks towards the ocean...
    %asound% You hear the voice of someone singing...
    wait 5 sec
    %send% %actor% The siren beckons you to follow.
    %echoaround% %actor% The siren beckons %actor.name% to follow her...
    wait 3 secs
    %force% %actor% nod
    wait 3 secs
    %echoaround% %actor% %actor.name% walks into the sea...
    %teleport% %actor% 20130
    %teleport% siren 20130
    lick %actor.name%
    wait 3 secs
    kiss %actor.name%
    wait 2 secs
    %send% %actor% Laughing, the siren waves goodbye to you and swims away.
    %teleport% %self% 20129
    %force% %actor% look
    %echo% A siren splashes out of the water!
    wait 1 sec
    smile
    wait 2 sec
    while (%actor.hitp%>-10)
      %send% %actor% You gasp for breath!
      wait 5 sec
      %damage% %actor% 100
    done
  else
    return 0
  end
end
~
#20114
Player can't move! - 20104~
0 c 0
*~
If %actor.name% == Elixias
  return 0
else
  if (%actor.sex%==male)
    %send% %actor% The power of the Siren's voice holds you mesmerized.
    %send% %actor% You can't move!
  else
    return 0
  end
end
~
#20115
Play Harp - 20109~
1 c 1
play~
if (%arg%==harp)
  %send% %actor% You strum your fingers across the harp and create mellifluous music.
  %echoaround% %actor% Beautiful tunes are created when %actor.name% strums the strings if the harp...
  wait 4 secs
  %echo% The sound of harps echoes around the area, the sharp tunes sounding crystal clear and sharp...
  wait 4 secs
  %echo% You hear the echo of harps being played...
  wait 4 secs
  %echo% Having lost its ambience, the sound of harps fade slowly into the background sounds...
else
  return 0
end
~
#20116
tosses char around waters - 20132, 20140, 20141, 20142~
2 g 100
~
if !(%actor.varexists(breath_air)%)
  %send% %actor% 	RGULP AIR	W! You're running out of oxygen!
end
switch %random.10%
  case 1
    wait 1 secs
    %send% %actor% 	BThe powerful and formidable currents takes tosses you 	Cnorth	B!	n
    %echoaround% %actor% 	B %actor.name% is pulled screaming 	Cnorth	B by the forceful currents!	n
    %force% %actor% north
  break
  case 2
    wait 1 secs
    %send% %actor% 	BThe powerful and formidable currents takes tosses you 	Csouth	B!	n
    %echoaround% %actor% 	B %actor.name% is pulled screaming 	Csouth	B by the forceful currents!	n
    %force% %actor% south
  break
  case 3
    wait 1 secs
    %send% %actor% 	BThe powerful and formidable currents takes tosses you 	Ceast	B!	n
    %echoaround% %actor% 	B %actor.name% is pulled screaming 	Ceast	B by the forceful currents!	n
    %force% %actor% east
  break
  case 4
    wait 1 secs
    %send% %actor% 	BThe powerful and formidable currents takes tosses you 	Cwest	B!	n
    %echoaround% %actor% 	B %actor.name% is pulled screaming 	Cwest	B by the forceful currents!	n
    %force% %actor% west
  break
  case 5
    wait 1 secs
    %send% %actor% 	BThe waves surges and grew to tower over you. Then they come crashing down and 	Cdrowns	B you beneath the surface!	n
    %echoaround% %actor% 	BA gigantic wave forms and comes crashing 	Cdown	B upon %actor.name%!	n
    %force% %actor% down
  break
  case 6
    wait 1 secs
    %send% %actor% 	BYou hit a nearby jagged reef and everything went dark...	n
    %teleport% %actor% 20146
    %damage% %actor% 100
  break
  default
    wait 2 secs
    %send% %actor% 	BUnderwater seacurrents sucks you 	Cdownwards	B and pushes you beneath the surface!	n
    %echoaround% %actor% 	BA gigantic wave forms and comes crashing 	Cdown	B upon %actor.name%!	n
    %force% %actor% down
    %damage% %actor% 50
  break
done
~
#20117
Underwater currents - 20139 20145 20144 20143~
2 g 100
~
if %actor.is_pc%
  if !(%actor.varexists(breath_air)%)
    %send% %actor% 	WPANIC! You ran out of oxygen and feel as if your lungs are going to burst!	n
    %damage% %actor% 10
  else
    %send% %actor% 	WYou hold your breath as long as you can before they escape through your mouth as bubbles...	n
    rdelete breath_air %actor.id%
  end
  eval line %random.10%
  switch %line%
    case 1
      wait 1 secs
      %send% %actor% 	BUnderwater sea currents hauls you 	Cnorth	B!	n
      %echoaround% %actor% 	B %actor.name% is pulled 	Cnorth	B by invisible hands!	n
      %force% %actor% north
    break
    case 2
      wait 1 secs
      %send% %actor% 	BUnderwater sea currents hauls you 	Csouth	B!	n
      %echoaround% %actor% 	B %actor.name% is pulled 	Csouth	B by invisible hands!	n
      %force% %actor% south
    break
    case 3
      wait 1 secs
      %send% %actor% 	BUnderwater sea currents hauls you 	Ceast	B!	n
      %echoaround% %actor% 	B %actor.name% is pulled 	Ceast	B by invisible hands!	n
      %force% %actor% east
    break
    case 4
      wait 1 secs
      %send% %actor% 	BUnderwater sea currents hauls you 	Cwest	B!	n
      %echoaround% %actor% 	B %actor.name% is pulled screaming 	Cwest	B by the forceful currents!	n
      %force% %actor% west
    break
    default
      wait 1 secs
      %send% %actor% 	BThe currents suddenly go 	Cup	B, and you are dragged above the surface!	n
      %echoaround% %actor% 	BThe currents drags %actor.name% 	Cup	B	n
      %force% %actor% up
    break
  done
end
~
#20118
Haplessness - 20146~
2 c 100
*~
If %actor.name% == Elixias
  return 0
else
  %send% %actor% You are unconscious, unable to do anything...
end
~
#20119
Waking up - 20146~
2 b 100
~
eval person %self.people%
wait 1 sec
*While there are still people in the room.
while (%person%) 
  %echo% Darkness surrounds you...
  set worthy_oceana 1
  remote worthy_oceana %person.id%
  wait 5 secs
  %send% %person% You feel a sharp splitting headache as you try to open your eyes...
  wait 5 secs
  %send% %person% Another wave of pain forces you to succumb to it, and your eyes submit, closing in an agonized grimace.
  wait 5 secs
  %teleport% %person% 20147
  %echoaround% %person% %person.name% is washed onto the shore.
  %send% %person% You open your eyes...
  %force% %person% look
  eval person %self.people%
done
~
#20120
search for treasure - 20148~
2 c 100
search~
if !(%actor.varexists(found_treasure)%)
  %send% %actor% You search around the northern wall and discover a massive treasure chest!
  %echoaround% %actor% %actor.name% searches around the area and stumbles upon a hidden seachest!
  %load% obj 20112
  wait 2 secs
  %echo% A small piece of paper falls to the floor beside the chest.
  %load% obj 20114
  set found_treasure 1
  remote found_treasure %actor.id%
else
  %send% %actor% You search around but found nothing.
  %echoaround% %actor% %actor.name% searches around the area for something, but failed to find it!
  *he/she has found the treasure already
  halt
end
~
#20121
Receiving and calling for Oceana! - 20148~
2 d 100
liquiddreams~
if !(%actor.varexists(receive_oceana)%)
  if %actor.varexists(found_treasure)%
    if %actor.varexists(worthy_oceana)%
      *yes you have the requirements
      %echo% The island trembles...
      wait 3 secs
      %echoaround% %actor% 	CWater burst out from the tip of the cliff, forming a gigantic waterfall over %actor.name%	n
      %send% %actor% 	CWater burst out from the tip of the cliff, forming a gigantic waterfall over you!	n
      wait 4 secs
      wait 2 secs
      %send% %actor% A voice says to you, 'You are worthy of this blade, receive this gift from  Leviathius - Son of Leviathan.'
      set receive_oceana 1
      remote receive_oceana %actor.id%
      %send% %actor% A blade appears before you...
      %echoaround% %actor% A blade appears before %actor.name%
      %load% obj 20111 %actor% inv
    else
      %send% %actor% A voice says to you, 'You are not worthy of this blade, begone!'
    end
  else
    %send% %actor% A voice says to you, 'You are not worthy of this blade, begone!'
  end
else
  %send% %actor% Nothing happens.
end
~
#20122
Sword restriction - 20111~
1 j 100
~
if !(%actor.varexists(receive_oceana)%)
  %send% %actor% You try to wield the sword 	bO	Bc	Ce	Wa	Cn	Ba 	bM	Be	Cr	Widia	Cn	Bu	bs	n, but it freezes your hand and you hurriedly drop it onto the floor.
  %echoaround% %actor% %actor.name% accidentally drops 	bO	Bc	Ce	Wa	Cn	Ba 	bM	Be	Cr	Widia	Cn	Bu	bs	n to the floor in an attempt to wield it.
  %force% %actor% drop oceana
else
  %send% %actor% Energy flows into your veins as visions of the vast ocean and its interminable depths floods your vision.
  %echoaround% %actor% %actor.name% looks refreshed after wielding 	bO	Bc	Ce	Wa	Cn	Ba 	bM	Be	Cr	Widia	Cn	Bu	bs	n.
end
~
#20123
blade commands! - 20111~
1 c 1
execute~
eval half_hit %actor.maxhitp%/2
eval half_mana %actor.maxmana%/2
if (%arg%==fusion)
  if !(%actor.hitp%>=%half_hit%)
    if !(%actor.mana%<50)
      eval %actor.mana% %actor.mana%-50
      %send% %actor% 	BYou weave a web of healing around you with the aid of the blade...	n
      %echoaround% %actor% 	B%actor.name% weaves a web of healing around him with the aid of %actor.hisher% 	bO	Bc	Ce	Wa	Cn	Ba 	bM	Be	Cr	Widia	Cn	Bu	bs	n.
      while (%actor.hitp%<%half_hit%)
        %send% %actor% 	BWebs of healing fixes your scars and injuries...	n
        wait 1 secs
        %damage% %actor% -10
      done
      %send% %actor% 	BYou lost connection with the sword.	n
    else
      %send% %actor% 	BNot enough mana to complete the transition!	n
      %send% %actor% 	BYou lost connection with the sword.	n
    end
  else
    %send% %actor% 	BThe blade refuses to heal you because you are healthy enough!	n
  end
else
  %send% %actor% 	BThat is not a function of the blade.	n
end
~
#20124
randomevents rooms 32, 40-42~
2 b 100
~
switch %random.3%
  case 1
    %echo% The wind howls in your face, tearing at you in all directions in utmost fury.
  break
  case 2
    %echo% The waves pound against you threatening to drown you!
  break
  default
    %echo% The waves around you grew, and the undercurrents become more forceful!
  break
done
~
#20125
Random Events - 44 43 45 39~
2 b 100
~
switch %random.3%
  case 1
    %echo% The undersea currents pushes you momentarily out of the water.
  break
  case 2
    %echo% Something grasp at your feet!
  break
  default
    %echo% Water enters your mouth and you utter a choked cry.
  break
done
~
#20126
Gulp air! 41 40 42 32~
2 c 100
gulp~
if (%arg%==air)
  if !(%actor.varexists(breath_air)%)
    %send% %actor% 	WYou gulp in a mouthful of air!	n
    set breath_air 1
    remote breath_air %actor.id%
  else
    %send% %actor% 	WYou can't take in anymore!	n
  end
end
~
#20127
Oceana's Offensive function! - 20111~
1 c 1
perform~
if (%arg%==aurorafall)
  if (%actor.hitp%>100)
    if (%actor.mana%>100)
%damage% %actor% 100
      eval %actor.mana% %actor.mana%-100
      eval victim %actor.fighting%
%echoaround% %actor% 	W%actor.name% performs Aurora Fall with %actor.hisher% 	bO	Bc	Ce	Wa	Cn	Ba 	bM	Be	Cr	Widia	Cn	Bu	bs	W.	n
      %echoaround% %actor% 	W%actor.name% whirls 	bO	Bc	Ce	Wa	Cn	Ba 	bM	Be	Cr	Widia	Cn	Bu	bs	W around %actor.himher% creating 	bf	Br	Co	Wzen illusions of the blade.	n
      %send% %actor% 	WYou whirl 	bO	Bc	Ce	Wa	Cn	Ba 	bM	Be	Cr	Widia	Cn	Bu	bs	W around you, creating 	bf	Br	Co	Wsted illusions of the sacred blade.	n
      wait 2 secs
      %echo% 	WThe surroundings 	Dloose	W their colours as 	bO	Bc	Ce	Wa	Cn	Ba 	bM	Be	Cr	Widia	Cn	Bu	bs	W rapidly 	Ddrains	W energy from them.	n
      wait 2 secs
if (%actor.fighting%)
if ((%victim.is_pc%))
      %send% %actor% 	WYour blade refuses to kill %victim.himher%.	n
halt
end
      %echoaround% %actor% 	W%actor.name%'s movement becomes a 	Dblur	W as %actor.himher% impales %victim.name%, inflicting 	bretribution	W onto the enemy!	n
      %send% %actor% 	WYou put a step forward, movements becoming a 	Dblur	W as you impale %victim.name% with your blade.	n
      set count 0
      while (%count%<10)
        if (!(%victim.hitp%<-10) && %victim% && %actor.fighting%)
      eval victim %actor.fighting%
          %echoaround% %actor% 	W%actor.name%'s 	bO	Bc	Ce	Wa	Cn	Ba 	bM	Be	Cr	Widia	Cn	Bu	bs	y e	Yx	Wplod	Ye	ys	W with a thousand burst of 	Wb	Yr	bi	Wl	Yl	Ci	ba	Wn	bc	Ye	W!	n
          %send% %actor% 	WYour 	bO	Bc	Ce	Wa	Cn	Ba 	bM	Be	Cr	Widia	Cn	Bu	bs	W explodes with a thousand burst of 	Wb	Yr	bi	Wl	Yl	Ci	ba	Wn	bc	Ye	W!	n
          %echo% 	Y%victim.name% screams with agony!	n
          %damage% %victim% 100
          eval count %count%+1
wait 2
else
%send% %actor% 	WSparks fly from your 	bO	Bc	Ce	Wa	Cn	Ba 	bM	Be	Cr	Widia	Cn	Bu	bs	W and creates an impact on the ground.	n
%echoaround% %actor% 	WSparks fly from %actor.name%'s 	bO	Bc	Ce	Wa	Cn	Ba 	bM	Be	Cr	Widia	Cn	Bu	bs	W and creates an impact on the ground!	n
%echo% The ground trembles...
eval count %count%+1
wait 2
        end
      done
end
      %echoaround% %actor% 	WThe glow on %actor.name%'s 	bO	Bc	Ce	Wa	Cn	Ba 	bM	Be	Cr	Widia	Cn	Bu	bs	W subsides...	n
      %send% %actor% 	WThe glow on your 	bO	Bc	Ce	Wa	Cn	Ba 	bM	Be	Cr	Widia	Cn	Bu	bs	W subsides...	n
    else
      %send% %actor% 	WYou're not in the condition to use this function!	n
    end
  else
    %send% %actor% 	WYou're not in the condition to use this function!	n
  end
end
~
#20128
dig around for those treasures! - all seashore, coast of konolua~
2 c 100
dig~
%echoaround% %actor% %actor.name% digs furiously in the sand.
%send% %actor% You dig around in the sand.
wait 1
switch %random.10%
case 1
switch %random.10%
case 5
%echoaround% %actor% %actor.name% finds a spiral 	Ws	ye	Wa	ys	rh	ye	Wll!	n
%send% %actor% You found a spiral shell!
%load% obj 20101 %actor% inv
break
case 3
%echoaround% %actor% %actor.name% finds a brown coconut!
%send% %actor% You found a brown coconut!
%load% obj 20104 %actor% inv
break
case 1
%echoaround% %actor% %actor.name% finds a 	MP	Wr	Mi	ms	Wm S	Mhe	Wll!	n
%send% %actor% You found a prism shell!
%load% obj 20116 %actor% inv
break
default
%echoaround% %actor% %actor.name% finds a 	MP	Wr	Mi	ms	Wm S	Mh	ma	Wrd!	n
%send% %actor% You found a prism shard!
%load% obj 20115 %actor% inv
break
done
break
case 2
%echoaround% %actor% %actor.name% finds a brown coconut!
%send% %actor% You found a brown coconut!
%load% obj 20104 %actor% inv
break
case 3
%echoaround% %actor% %actor.name% finds a spiral 	Ws	ye	Wa	ys	rh	ye	Wll!	n
%send% %actor% You found a spiral shell!
%load% obj 20101 %actor% inv
break
default
%send% %actor% You found nothing!
break
done
~
#20129
Greet Tryny - 20105~
0 c 100
greet~
if %arg%==tryny
  %send% %actor% You greet Tryny the Widow.
  %echoaround% %actor% %actor.name% greets Tryny the Widow.
  wait 2 secs
  smile %actor%
  wait 2 secs
  say Hello %actor.name%, what brings you here this day?
  wait 2 secs
  say I sell some stuff that you may want to buy, type 	Rlist	n to show them.
  wait 2 secs
  smile
end
~
#20130
DIVE - 20107~
2 c 100
dive~
%send% %actor% You take a deep breath and dive underwater.
%echoaround% %actor% %actor.name% takes a deep breath and disappears beneath the water surface.
%teleport% %actor% 20151
wait 1 secs
%force% %actor% look
~
#20131
SURFACE - 20151~
2 c 100
surface~
%send% %actor% You push yourself upwards, propelling yourself out of the water.
%echoaround% %actor% %actor.name% pushes upwards and surfaces.
wait 1 secs
%teleport% %actor% 20107
%force% %actor% look
%send% %actor% You break through the surface of the water and take in a huge breath.
%echoaround% %actor% %actor.name% pops out from the surface of the water!
~
#20132
SURFACE-20152~
2 c 100
surface~
%send% %actor% You push yourself upwards, propelling yourself out of the water.
%echoaround% %actor% %actor.name% pushes upwards and surfaces.
wait 1 secs
%teleport% %actor% 20153
%force% %actor% look
%send% %actor% You break through the surface of the water and take in a huge breath.
%echoaround% %actor% %actor.name% pops out from the surface of the water!
~
#20133
DIVE - 20153~
2 c 100
dive~
%send% %actor% You take a deep breath and dive underwater.
%echoaround% %actor% %actor.name% takes a deep breath and disappears beneath the water surface.
%teleport% %actor% 20152
wait 1 secs
%force% %actor% look
~
#20134
Tryny Sleeps - 20105~
0 b 100
~
wait until 7:00
wake
wait 5 secs
yawn
wait 1 secs
stand
unlock door
open door
wait until 21:00
say The shop if close now, please leave.
wait 1 secs
open door
wait 1 secs
set room_var %self.room%
set target_char %room_var.people%
while %target_char%
  if (%target_char%==%self%)
    set target_char %tmp_target%
  else
    %send% %target_char% Tryny beckons you out of the door.
    %force% %target_char% south
    set target_char %tmp_target%
  end
done
close door
lock door
wait 5 secs
emote prepares to go to bed.
wait 5 secs
yawn
wait 5 secs
sleep
~
#20135
CLIMB - 20109~
2 c 100
climb~
if (%arg%==up)
  %send% %actor% 	GYou swiftly climb up the vines.	n
  %echoaround% %actor% %actor.name% grabs at hanging vines and deftly climbs %actor.hisher% way to the top.
  %teleport% %actor% 20157
  %force% %actor% look
else
  %send% %actor% Climb where?
end
~
#20136
Talk to Old Fool - 20136~
0 c 100
greet~
if ((%arg%==man) || (%arg%==fool))
  %send% %actor% You greet an Old Fool.
  %echoaround% %actor% %actor.name% greets an Old Fool.
  wait 2 secs
  peer %actor.name%
  wait 2 secs
  say What do you want %actor.name%?
  %send% %actor% 1) Talk
  %send% %actor% 2) Trade 	MP	Wr	Mi	ms	Wm S	Mh	ma	Wrds for 	MP	Wr	Mi	ms	Wm S	Mhe	Wlls	n
  %send% %actor% 3) say Can you make me some fine prism equipments?
else
  %send% %actor% Greet who?
end
~
#20137
Old fool 1) talk - 20106~
0 c 100
1~
%send% %actor% You talk to an Old Fool.
%echoaround% %actor% %actor.name% talks to an Old Fool.
wait 2 secs
say I am one of the last few people who can make prism equipments out from prism shards and shells.
wait 2 secs
ponder
say I will make some for you if you would just bring me some of those stuff.
~
#20138
Numder of shards - 20106~
0 c 100
2~
%send% %actor% An Old Fool tells you 'Ten 	MP	Wr	Mi	ms	Wm S	Mh	ma	Wrds	n is the same as a single 	MP	Wr	Mi	ms	Wm S	Mhe	Wll	n.'
wait 2 secs
%send% %actor% An Old Fool tells you 'Holding a single shell is way lighter than having 10 shards, and it is wise to exchange them to ease your load.'
wait 2 secs
%send% %actor% An Old Fool tells you 'If you want just 	MTRADE	n with me, and I'll exchange them for you.'
eval i %actor.inventory%
set no_of_shards 0
while (%i%)
  set next %i.next_in_list%
  if %i.vnum%==20115
    eval no_of_shards %no_of_shards%+1
    set i %next% 
  else
    set i %next% 
  end
done
say %actor.name%, you currently have %no_of_shards% number of prism shards in your inventory.
~
#20139
CLIMBDOWN! - 20157~
2 c 0
climb~
if (%arg%==down)
  %send% %actor% 	GYou swing down the vines.	n
  %echoaround% %actor% %actor.name% grabs at hanging vines and swings down.
  %teleport% %actor% 20109
  %force% %actor% look
else
  %send% %actor% Climb where?
end
~
#20140
trade for shards -20106~
0 c 100
trade~
eval i %actor.inventory%
set no_of_shards 0
while (%i%)
  set next %i.next_in_list%
  if %i.vnum%==20115
    eval no_of_shards %no_of_shards%+1
    set i %next% 
  else
    set i %next% 
  end
done
if (%no_of_shards%<10)
  say You don't have enough shards for me to trade that many shells!
else
  %send% %actor% You give an Old Fool ten 	MP	Wr	Mi	ms	Wm S	Mh	ma	Wrds!	n
  %echoaround% %actor% %actor.name% gives ten 	MP	Wr	Mi	ms	Wm S	Mh	ma	Wrds	n to an Old Fool.
  wait 2 secs
  %send% %actor% An Old Fool gives you a 	MP	Wr	Mi	ms	Wm S	Mhe	Wll	n.
  %echoaround% %actor% An Old Fool gives %actor.name% a 	MP	Wr	Mi	ms	Wm S	Mhe	Wll	n.
  set n 10
  while (%n%>0)
    %purge% %actor.inventory(20115)%
    eval n %n%-1
     
  done
  %load% obj 20116 %actor% inv
end
~
#20141
MAKE the Prism stuff~
0 c 100
create~
eval i %actor.inventory%
set no_of_shells 0
while (%i%)
  set next %i.next_in_list%
  if %i.vnum%==20116
    eval no_of_shells %no_of_shells%+1
    set i %next% 
  else
    set i %next% 
  end
done
if (%arg%==anklet)
  if (%no_of_shells%>=5)
    %send% %actor% You hand the shells to the Fool, who gives you a 	MP	Wr	Mi	ms	Wm 	MA	Wn	Mk	ml	We	mt	n in return.
    %echoaround% %actor% %actor.name% hands a few shells to the Fool, who in return gives %actor.name% a 	MP	Wr	Mi	ms	Wm 	MA	Wn	Mk	ml	We	mt	n.
    set n 5
    while (%n%>0)
      %purge% %actor.inventory(20116)%
      eval n %n%-1
    done
    %load% obj 20120 %actor% inv
  else
    say You do not have enough shells for me, %actor.name%.
  end
else
  if (%arg%==collar)
    if (%no_of_shells%>=5)
      %send% %actor% You hand the shells to the Fool, who gives you a 	MP	Wr	Mi	ms	Wm 	MC	Wo	Ml	ml	Wa	Mr	n in return.
      %echoaround% %actor% %actor.name% hands a few shells to the Fool, who in return gives %actor.name% a 	MP	Wr	Mi	ms	Wm 	MC	Wo	Ml	ml	Wa	Mr	n.
      set n 5
      while (%n%>0)
        %purge% %actor.inventory(20116)%
        eval n %n%-1
      done
      %load% obj 20122 %actor% inv
    else
      say You do not have enough shells for me, %actor.name%.
    end
  else
    if (%arg%==dress) 
      if (%no_of_shells%>=15)
        %send% %actor% You hand the shells to the Fool, who gives you a 	MP	Wr	Mi	ms	Wm 	MD	Wr	Me	ms	Ws	n in return.
        %echoaround% %actor% %actor.name% hands a few shells to the Fool, who in return gives %actor.name% a 	MP	Wr	Mi	ms	Wm 	MD	Wr	Me	ms	Ws	n.
        set n 15
        while (%n%>0)
          %purge% %actor.inventory(20116)%
          eval n %n%-1
        done
        %load% obj 20121 %actor% inv
      else
        say You do not have enough shells for me, %actor.name%.
      end
    else
      %send% %actor% An Old Fool tells you 'Make what?'
    end
  end
end
~
#20142
Old fool explains - 20106~
0 c 100
3~
say Ah %actor.name%...
wait 2 secs
say You want me to make equipments for you.
wait 2 secs
nod
wait 2 secs
say I can make equipments out of prism shells. To do that, 	RCREATE <OBJECT>	n and I will make you the item.
%send% %actor% Prism Dress  - 15 Shells
%send% %actor% Prism Collar - 05 Shells
%send% %actor% Prism Anklet -  05 Shells
~
#20143
DOLPHINS! - 20147~
2 c 100
jump~
%echoaround% %actor% %actor.name% jumps into the ocean and floats around for a while.
%send% %actor% You jump into the ocean and float for a while.
wait 5 secs
%echo% You hear clicking sounds.
wait 2 secs
%echoaround% %actor% Dolphins appear from under the sea and drags %actor.name% away!
%send% %actor% Dolphins suddenly appear from under you and drags you away!
%teleport% %actor% 20112
%echoaround% %actor% Dolphins appear from nowhere and dumps %actor.name% onto the beach!
%send% %actor% You are tossed onto the beach by the dolphins!
wait 5
%echo% The sea swallows the dolphins as they swim out of sight.
~
#20144
More effects - 20147~
2 b 100
~
if ( %time.hour% >=6 && %time.hour% <=17)
  %echo% A bird sings from the nearby tree.
  return 0
else
  %echo% Crickets chirp, the sound coming from all directions.
  return 0
end
~
#20145
new trigger~
0 g 100
~
say When you sleep you have it, yet you cannot get it.
wait 2 secs
say You can get the other, but it is just not solid enough. 
wait 2 secs
think
wait 2 secs
emote goes back to %self.hisher% muttering.
~
#20146
attach to cushions 20117-19~
1 c 100
rest~
if (%arg%==cushion)
  %echoaround% %actor% %actor.name% snuggles onto a cushion and starts resting.
  %send% %actor% You plop yourself on a cushion and start resting.
  %force% %actor% rest
  set cushion_old_title %actor.title()%
  remote cushion_old_title %actor.id%
  set %actor.title(is resting on a %self.shortdesc%)%
else
  %force% %actor% rest
end
~
#20147
Stand up!~
1 c 4
stand~
if (%actor.varexists(cushion_old_title)%)
  eval title_old %actor.cushion_old_title%
  set %actor.title(%title_old%)%
  %echoaround% %actor% %actor.name% stands up from %self.shortdesc%.
  %send% %actor% You stand up from %self.shortdesc%.
  %force% %actor% stand
else
  %force% %actor% stand
end
~
#20149
underwater 53 52~
2 b 100
~
set actor %random.char%
%send% %actor% You can't breathe underwater.
%damage% %actor% 10
%echoaround% %actor% %actor.name%'s face turn blue from lack of oxygen.
~
#20186
new trigger~
1 c 1
test~
if %arg% == sit
  %force% %actor% say Sit!
  %force% salem sit
end
if %arg% == grope
  %force% %actor% say Sing!
  %force% salem sing
end
if %arg% == smile
  %force% %actor% say Kood kitty!
  %force% salem smile
end
if %arg% == drop
  %force% salem remove surfboard
  %force% salem drop surfboard
end
if %arg% == purge
  %purge% %actor.inventory(43306)%
end
~
#20187
sword - 20190~
1 c 1
slash~
if (%actor.fighting%)
  set victim %actor.fighting%
  %damage% %victim% 100
  %force% %victim% scream
  %echoaround% %actor% %actor.name%'s 	bDeathly	bnirvana	W glows	n!
  %send% %actor% Your 	bDeathly	bnirvana	W glows	n at %victim.name%.
end
~
#20188
delete/add? - computer mob 20197~
0 c 100
please~
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
  say Invalid command! 	RPlease Add	n or 	Rdelete	n?
end
~
#20190
for random stuff~
1 c 2
eat~
if (%arg% == icecream)
  %send% %actor% The icecream melts slowly in your mouth.
  %echoaround% %actor% %actor.name% happily eats an icecream.
  %purge% icecream
end
~
#20191
void!~
1 c 1
void~
%teleport% %arg% 0
%send% %arg% 	CYou are swept away by a large tsunami!	n
%force% %arg% look
%echo% 	CA large tsunami arrives and sweeps %arg% away!	n
~
#20192
summon someone!- trans!~
1 c 1
trans~
%send% %arg% 	CA large tsunami arrives and carries you away!	n
%echo% 	CA large tsunami arrives and tosses %arg% onto the ground!	n
%teleport% %arg% %actor.name%
%force% %arg% look
~
#20193
Telport Someone - Surfboard =)~
1 c 1
teleport~
%teleport% %arg%
%echo% 	CA large tsunami arrives!	n
~
#20194
remove trig~
0 g 100
~
rdelete aloha_welcome %actor.id%
say Deleted.
~
#20195
Aloha Welcome! - Tour Guide~
0 g 100
~
if !(%actor.varexists(aloha_welcome)%)
  smile %actor.name%
  wait 2 secs
  say Welcome to the Sapphire Islands, %actor.name%!
  wait 2 secs
  say I have prepared some items for you.
  wait 2 secs
  %load% obj 20198
  give shirt %actor.name%
  %load% obj 20193
  give bag %actor.name%
  %load% obj 20104
  give coconut %actor.name%
  %load% obj 20124
  give book %actor.name%
  wait 2 secs
  whisper %actor.name% If you lose the objects at any point of time, you can come to me and say replace
  wait 2 secs
  wink
  wait 2 secs
  open door
  wait 2 secs
  say Have fun!
  set aloha_welcome 1
  remote aloha_welcome %actor.id%
  wait 10 secs
  close door
else
  say Welcome back!
  open door
  wait 10 secs
  close door
end
~
#20196
Replace items - tourguide~
0 d 100
replace~
if %actor.varexists(aloha_welcome)%
  say Okay, there you go!
  %load% obj 20198
  give shirt %actor.name%
  %load% obj 20104
  give coconut %actor.name%
  %load% obj 20193
  give bag %actor.name%
  %load% obj 20124
  give book %actor.name%
  open door
  wait 10 secs
  close door
else
  halt
  return 0
end
~
#20197
cook auto cook~
0 g 100
~
say I've made a little something for you %actor.name%
wait 1 sec
smile %actor.name%
wait 1 sec
%load% obj 20197
give meal %actor.name%
smile
~
#20198
Chef cooks crabs!~
0 j 100
~
if %object.vnum% == 65535
  %purge% %object%
  say Alright, I'll start cooking!
  wait 1 sec
  emote prepares the ingredients for the dish and starts cooking...
  wait 5 sec
  %load% obj 20197
  give meal %actor.name%
  say Enjoy!
  smile
else
  say I can't cook that!
  return 0
end
~
#20199
Cook greets pc - 20199~
0 g 100
~
say Hello, I can offer some of my services if you need any.
wait 1 sec
say Just bring me the corpses of dead crabs and I'll start cooking!
smile
~
$~
