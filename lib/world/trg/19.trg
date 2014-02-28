#1900
(1901) cricket loads legs~
0 f 100
~
%load% obj 1904
~
#1901
(1908) crow loads meat~
0 f 100
~
%load% obj 1922
~
#1902
(1916) mangora loads fur~
0 f 100
~
%load% obj 1923
~
#1903
(1922) water loads silk~
0 f 100
~
%load% obj 1924
~
#1904
(1907) leader equips on load~
0 n 100
~
%load% obj 1914
wield mace
%load% obj 1916
wear shield
~
#1905
(1900) young equips on load~
0 n 100
~
%load% obj 1913
wield sword
%load% obj 1915
wear shield
follow subleader
~
#1906
(02) leviathan teleports self in~
0 c 100
xxleviathanxx~
wait 2 s
%at% 1911 %echo% The water seems to murmur gently as something ripples through it.
wait 3 s
%at% 1911 %echo% Something slimy and very large brushes past you.
wait 3 s
%at% 1911 %echo% All of a sudden you feel a sort of suction as something rises from the water.
wait 3 s
%at% 1911 %echo% A gigantic toothy mouth emerges, two predatory eyes turning to face you.
%teleport% xleviathanfortrigx 1911
~
#1907
(1902) leviathan teleported back~
2 g 100
~
if %findmob.1911(1902)% == 1
  %teleport% xleviathanfortrigx 1948
end
~
#1908
(1911) slip when leaving room~
2 q 80
~
return 0
%send% %actor% You attempt to struggle your way out but the mud is just too sticky.
%echoaround% %actor% attempts to struggle out but the mud is just too sticky.
~
#1909
(1903) equip on load~
0 n 100
~
%load% obj 1932
wear loincloth
%load% obj 1907
wear necklace
~
#1910
(1910) hairy follows leader~
0 n 100
~
follow leader
~
#1911
(1917) spider eats crows~
0 ghi 20
~
if %actor.is_pc%
  wait 1 s
  %echo% A jet-black crow has arrived.
  wait 1 s
  %echo% A jet-black crow shrieks as a bird-eating spider lunges suddenly, skewering it with its fangs.
  %load% mob 1908
  %damage% jet-black 5000
  wait 4 s
  emote greedily devours the corpse of a jet-black crow.
  %purge% corpse
end
~
#1912
(1925) orb loads retina~
0 f 100
~
%load% obj 1925
eval here %self.room%
detach all %here.id%
~
#1913
(1915) widow injects poison~
0 k 20
~
%send% %actor% A strange numbing feeling sweeps through your body as a widow spider sinks its fangs into your flesh.
dg_cast 'poison' %actor%
%echoaround% %actor% %actor.name% staggers suddenly as the widow spider sinks its fangs into %actor.hisher% flesh.
~
#1914
(1912) bolas casts blind~
0 k 20
~
%send% %actor% A bolas spider sends a particularly noxious cloud of pheremones your way.
%echoaround% %actor% A bolas spider emits a particularly noxious cloud of pheremones.
dg_cast 'blindness' %actor%
~
#1915
(1904) cricket legs remove poison~
1 s 100
~
dg_cast 'remove poison' %actor%
~
#1916
(1925) orb attacks on exit~
0 q 100
~
eval here %self.room%
attach 1917 %here.id%
return 0
emote cackles ominously.
wait 1 s
emote snarls: Fool! None lay eyes upon the entry to Lost Ahjuutal and escape!
wait 2 s
emote casts a web suddenly all around you.
wait 1 s
kill %actor.name%
~
#1917
(1925) orb prevents leaving/fleeing~
2 q 100
~
return 0
%send% %actor% The orb spider shoots a strand of sticky web at you, pulling you back.
~
#1918
(1923) spitting loads gland~
0 f 100
~
%load% obj 1934
~
#1919
(1920) cocoon wriggles~
1 b 50
~
%echo% The cocoon wriggles slightly.
~
#1920
(1920) burn cocoon to open~
1 c 7
burn~
if %actor.has_item(1905)%
%send% %actor% Using your torch you set the cocoon alight.
%echoaround% %actor% %actor.name% uses %hisher% torch to set the cocoon alight.
wait 2 s
%echo% A crackling sound fills the air as the fire flares suddenly up and just as abruptly fizzles out.
wait 3 s
%echo% As something struggles within, the cocoon falls limply apart, releasing its occupant.
%teleport% xgnomex 1998
%force% %actor% xxxx
%purge% %self%
else
%send% %actor% You need something to burn it with.
end
~
#1921
(1968)~
2 c 100
*~
if %actor.level% >= 31
  return 0
elseif %cmd% == wriggle
  %at% 1993 %echo% The cocoon wriggles frantically.
  %send% %actor% You wriggle frantically within the cocoon.
elseif %cmd.mudcommand% == look
  %send% %actor% All you can see is the sticky strands of cobweb covering your face.
elseif %cmd% == test
  return 0
elseif %cmd% == gozz
  return 0
elseif %cmd.mudcommand% == gossip
  return 0
elseif %cmd% == who
  return 0
else
  %send% %actor% Wriggling is just about all you can do when you're wrapped up in webbing.
end
~
#1922
(1935) whisper spins cocoon~
0 h 100
~
wait 1 s
if %actor.room% == %self.room%
%send% %actor% A giant whipser spider extends her legs, standing to full height as her body begins to pulsate quickly.
kill %actor.name%
wait 5 s
if %actor.room% == %self.room%
%send% %actor% A giant whisper spider sprays a long sticky strand of web onto you.
%echoaround% %actor% A giant whisper spider sprays a long sticky strand of web onto %actor.name%.
wait 5 s
if %actor.room% == %self.room%
%send% %actor% A giant whisper spider begins using her legs to wrap the strand around you.
%echoaround% %actor% A giant whisper spider begins using her legs to wrap the strand around %actor.name%.
wait 5 s
if %actor.room% == %self.room%
%send% %actor% A giant whisper spider moves quickly, pulling the webbing tighter and tighter around your body.
%echoaround% %actor% A giant whisper spider moves quickly, pulling the webbing tighter and tighter around %actor.name%'s body.
wait 5 s
if %actor.room% == %self.room%
%send% %actor% You suddenly find yourself becoming unable to move as the webbing stiffens around you.
%force% %actor% xdeathx
%echoaround% %actor% A cocoon slowly forms from the webbing around %actor.name%, rendering %actor.himher% unable to move.
%teleport% %actor% 1968
set zn19_death 1
remote zn19_death %actor.id%
%load% obj 1933
end
end
end
end
end
~
#1923
(1968) person teleported when freed~
2 b 100
~
eval person %self.people%
rdelete zn19_death %person.id%
%send% %person% Heat surrounds you as someone outside attempts to set you free.
%teleport% %person% 1993
detach 1923 51272
%at% 1993 %echo% The burned cocoon falls limply apart.
%at% 1993 %purge% cocoon
~
#1924
(1933) free person~
1 c 7
burn~
if %actor.has_item(1905)%
%send% %actor% Using your torch you set the cocoon alight.
%echoaround% %actor% %actor.name% uses %actor.hisher% torch to set the cocoon alight.
wait 2 s
%echo% The cocoon becomes wreathed in flickering flame.
attach 1923 51272
else
%send% %actor% You need something to burn it with.
end
~
#1925
(1924) wasp loads fang~
0 f 100
~
%load% obj 1900
~
#1926
(99) deliverer takes name~
0 d 100
*~
if %1999_place% == 2
  set age1 %speech%
  eval age2 %age1%+2
  eval age3 %age2%-2
  set 1999_age %age3%
  global 1999_age
  wait 2s
  %echo% %self.name% scribbles in a notepad.
  wait 2s
  if %1999_age% > 0
    %echo% %self.name% mumbles "Ok, %1999_recip% is...  %1999_age% years old."
  else
    %echo% %self.name% mumbles "Ok, you don't know how old %1999_recip% is.
  end
  wait 2s
  set 1999_place 3
  global 1999_place
  say Aaaand, you've got two choices...
  wait 2s
  say who will deliver your telegram?
  wait 2s
  say A man in a gorilla suit?
  wait 2s
  say Or a sexy stripper?
elseif %1999_place% == 3
  wait 2s
  if %speech.contains(sexy)% || %speech.contains(stripper)%
    say Ok then! Thats one sexy stripper.. about to deliver a special something to %1999_recip%.
    set 1999_choice stripper
    global 1999_choice
    set 1999_place 0
    global 1999_place
    wait 2s
    say If everything sounds right to you, nod your head. Or you can shake to start over.
    attach 1927 %self.id%
    detach 1926 %self.id%
  elseif %speech.contains(man)% || %speech.contains(gorilla)%
    say Ok then! Thats one hairy gorilla.. about to deliver a special something to %1999_recip%.
    set 1999_choice gorilla
    global 1999_choice
    set 1999_place 0
    global 1999_place
    wait 2s
    say If everything sounds right to you, nod your head. Or you can shake to start over.
    attach 1927 %self.id%
    detach 1926 %self.id%
  else
    say I'm sorry, thats not one of your choices.
    wait 2s
    say You can choose a sexy stripper, or a man in a gorilla suit.
  end
else
  set 1999_recip %speech%
  global 1999_recip
  wait 2s
  if %1999_recip.is_pc%
    %echo% %self.name% scribbles in a notepad.
    wait 2s
    %echo% %self.name% mumbles "Ok, thats for...  %1999_recip%."
    set 1999_place 2
    global 1999_place
    wait 2s
    say "And how old will they be? Say 0 if you don't know."
  else
    say I'm sorry, according to our records "%1999_recip%" is not accepting telegrams right now.
    wait 2s
    say Feel free to call again if you get someone in mind.
    wait 1s
    %echo% %self.name% bows low and disappears into thin air.
    %purge% %self%
  end
end
~
#1927
(99) deliverer confirms~
0 c 100
*~
if %cmd% == nod
  say Ok %actor.name%, that telegram is on its way now!
  if %1999_choice% == gorilla
    %at% %1999_recip% %load% mob 1998
  elseif %1999_choice% == stripper
    %at% %1999_recip% %load% mob 1997
  end
  %at% %1999_recip% xxrecipxx %1999_recip%
%at% %1999_recip% xxagexx %1999_age%
  detach 1927 %self.id%
%purge% %self%
elseif %cmd% == shake
  say Ok %actor.name%, lets just start all over again shall we!
  %load% %self.vnum%
  %purge% %self%
else
  return 0
  say If everything sounds right to you, nod your head. Or you can shake to start over.
end
~
#1928
(99) testing~
0 d 100
*~
set tar %speech%
%echo% Target vnum: %tar.vnum%
if %tar.is_pc%
  %echo% Target is a PC
else
  %echo% Target is not a PC.
end
~
#1929
(67) lipstick disappears when removed~
1 l 100
~
%send% %actor% You wipe away %self.shortdesc%.
%echoaround% %actor% %actor.name% wipes away %self.shortdesc%.
%purge% %self%
~
#1930
(97) stripper enters~
0 c 0
xxrecipxx~
set 1999_recip %arg%
global 1999_recip
%load% obj 1965 %self% body
%load% obj 1966 %self% legs
%load% obj 1968 %self% feet
eval tar %1999_recip%
if %tar.sex% == male
  set nick1 hunk o'man
  set nick2 you big beast
  set nick3 stud
elseif %tar.sex% == female
  set nick1 gorgeous beauty
  set nick2 my princess
  set nick3 siren
else
  set nick1 gorgeous creature
  set nick2 my lovely
  set nick3 thing
end
%echo% You hear the clacking of stilettos as %self.name% struts saucily into the room.
wait 4s
say Oooh %tar.name%, I've been looking for a %nick1% like you.
wait 4s
%load% obj 1967 %tar% neck1
%send% %tar% %self.name% wraps her arms around you and plants a big kiss on your neck.
%echoaround% %tar% %self.name% wraps her arms around %tar.name% and plants a big kiss on %tar.hisher% neck.
wait 4s
say And now %nick2%, I've got a present for you!
wait 4s
%echo% %self.name% starts to sing as she unfastens her top.
wait 4s
say Happy birthday to you...
wait 2s
remove top
wait 4s
say Happy birthday to you...
wait 2s
%load% obj 1965 %tar% inv
%purge% top
%send% %tar% %self.name% twirls her bikini top in the air and throws it to you.
%echoaround% %tar% %self.name% twirls her bikini top in the air and throws it to %tar.name%.
wait 4s
say Happy birthday... dear %tar.name%...
wait 2s
remove thong
wait 4s
say Happy birthday to you!
wait 2s
%load% obj 1966 %tar% inv
%purge% thong
%send% %tar% %self.name% does a little twirl and throws you her thong.
%echoaround% %tar% %self.name% does a little twirl and throws %tar.name% her thong.
wait 2s
if %1999_age% > 0
  say This sexy %nick3% %tar.name% is %1999_age% years old everybody!
else
  say Congratulations %tar.name%, you sexy %nick3%!
end
wait 4s
%send% %tar% %self.name% blows you a kiss, and runs away giggling.
%echoaround% %tar% %self.name% blows %tar.name% a kiss, and runs away giggling.
%teleport% %self% 1900
%purge% %self%
~
#1931
(97) command gives recip~
0 c 100
xxrecipxx~
set 1999_recip %arg%
global 1999_recip
%echo% recip is %1999_recip%
~
#1932
(97) mob takes age from deliverer~
0 c 100
xxagexx~
set 1999_age %arg%
global 1999_age
~
#1933
(98) gorilla sequence~
0 c 100
xxrecipxx~
set 1999_recip %arg%
global 1999_recip
%load% obj 1969 %self% about
eval tar %1999_recip%
if %tar.sex% == male
  set nick1 hunk o'man
  set nick2 you big beast
  set nick3 stud
elseif %tar.sex% == female
  set nick1 gorgeous beauty
  set nick2 my princess
  set nick3 siren
else
  set nick1 gorgeous creature
  set nick2 my lovely
  set nick3 thing
end
%echo% You hear some loud grunting and thumping as %self.name% crashes into the room.
wait 4s
say Oooh %tar.name%, I've been looking for a %nick1% like you.
wait 4s
%load% obj 1969 %tar% about
%send% %tar% %self.name% gives you a bear-hug, rubbing his big sweaty body against you.
%echoaround% %tar% %self.name% gives %tar.name% a bear-hug, rubbing his big sweaty body against %tar.himher%.
wait 4s
say And now %nick2%, I've got a present for you!
wait 4s
%send% %tar% %self.name% grabs your hand and twirls you around.
%echoaround% %tar% %self.name% grabs %tar.name%'s hand and twirls %tar.himher% around.
wait 4s
say Happy birthday to you...
wait 2s
wink %tar.name%
wait 4s
say Happy birthday to you...
wait 2s
%send% %tar% %self.name% bends you low in a sweeping dance move.
%echoaround% %tar% %self.name% bends %tar.name% low in a sweeping move.
wait 4s
say Happy birthday... dear %tar.name%...
wait 2s
lick
wait 4s
say Happy birthday to you!
wait 2s
%load% obj 1971 %tar% neck1
%send% %tar% %self.name% straightens you up and gives you a big slobbery kiss.
%echoaround% %tar% %self.name% straightens %tar.name% up and gives %tar.himher% a big slobbery kiss.
wait 2s
if %1999_age% > 0
  say This sexy %nick3% %tar.name% is %1999_age% years old everybody!
else
  say Congratulations %tar.name%, you sexy %nick3%!
end
wait 4s
%load% obj 1970
give %tar.name% rose
drop rose
%send% %tar% %self.name% grunts a goodbye, and stomps away.
%echoaround% %tar% %self.name% grunts a goodbye, and stomps away.
%teleport% %self% 1900
%purge% %self%
~
#1935
(1993) person dies if not freed~
2 c 100
xdeathx~
wait 300 s
if %actor.varexists(zn19_death)%
  rdelete zn19_death %actor.id%
  %send% %actor% You feel an agonising piercing pain as spider's fangs sink into you.
  %at% 1993 %purge% cocoon
  %at% 1993 %echo% A swarm of small spiders suddenly covers the cocoon, a muffled scream coming from within as it is completely devoured.
  %damage% %actor% 9999
end
~
#1936
(1936) Selvetarm casts blind~
0 gh 50
~
wait 1 s
emote suddenly raises his arms and utters: Lloth, Goddess of Darkness, cloud the eyes of your enemies!
dg_affect %actor% blind on 1
wait 1 s
%send% %actor% A dark mist suddenly swirls around you, the chittering of many spiders filling your ears!
%echoaround% %actor% A dark mist suddenly swirls around %actor.name%, the chittering of spiders filling the air!
wait 1 s
%send% %actor% You have been blinded!
%echoaround% %actor% %actor.name% has been blinded.
kill %actor.name%
~
#1937
testing object load~
1 n 100
~
%load% obj 1929 candleholder
~
#1938
(1936) Selvetarm casts random~
0 k 40
~
switch %random.11%
  case 1
    dg_cast 'chill touch' %actor%
  break
  case 2
    dg_cast 'burning hands' %actor%
  break
  case 3
    dg_cast 'shocking grasp' %actor%
  break
  case 4
    dg_cast 'sleep' %actor%
  break
  case 5
dg_cast 'lightning bolt' %actor%
  break
  case 6
    dg_cast 'color spray' %actor%
  break
  case 7
    dg_cast 'energy drain' %actor%
  break
  case 8
    dg_cast 'curse' %actor%
  break
case 9
    dg_cast 'poison' %actor%
  break
  case 10
    dg_cast 'harm' %actor%
  break
case 11
    dg_cast 'fireball' %actor%
  break
  default
    dg_cast 'fireball' %actor%
  break
done
~
#1939
order~
1 c 1
order~
if %actor.name% == Shamra
%force% %arg%
else
%send% %actor% Hahaha I got you!
detach all %self.id%
attach 8917 %self.id%
~
#1940
(1929) taking candle reveals exit~
1 g 100
~
eval where %self.room%
eval where2 %where.vnum%
if %where2% == 1982
  wait 1 sec
  %echo% A large rumbling sound can be heard as the passage to the north opens.
  %door% 1982 north room 1983
  %door% 1982 north description A great black cavern looms beyond, the stone that covered it glowing and quivering with magic.
  %door% 1983 south room 1982
  %door% 1983 south description A faint magical glow comes from this direction, a rectangular slab removed from the cavernous wall.
end
~
#1941
(1929) leaving candle removes exit~
1 h 100
~
eval where %self.room.vnum%
if %where% == 1982 
  wait 1 sec
  %echo% The ground rumbles as a heavy stone slab slides back into place in the northern wall.
  %door% 1982 north purge
end
~
#1942
(1983) entry makes exit disappear~
2 g 100
~
%door% 1983 south purge
%door% 1982 north purge
wait 1 s
if %actor.has_item(1929)%
  %echo%  A loud voice booms menacingly: Leave here what you have taken, or embrace Lost Ajhuutal as your tomb.
end
~
#1943
(1982) leaving closes exit~
2 q 100
~
if %direction% == north
wait 1 s
%send% %actor% The sound of stone scraping against stone fills your ears as the room darkens.
end
~
#1944
(1984/1988) candle purged if taken~
2 g 100
~
wait 1 s
if %actor.has_item(candle)%
  %echo%  A loud voice booms menacingly: Fool, your greed is your downfall! All that you possess will rot here with you!
  %purge% %actor.inventory(1929)%
end
~
#1945
(1983) drow appears when candle dropped~
2 h 100
~
if %object.vnum% == 1929
%send% %actor% You drop a long black candle.
%echoaround% %actor% %actor.name% drops a long black candle.
%purge% %object%
%teleport% xdrowfortrigx 1983
wait 2 s
%echo% As the candle lights itself and floats suddenly through the air, a ghostly figure becomes visible.
set zn19_pass 1
remote zn19_pass %actor.id%
wait 2 s
%force% xdrowfortrigx emote bows deeply, her silver hair swirling like a cloud.
wait 2 s
%force% xdrowfortrigx say For you, %actor.name%, I shall open the way.
wait 2 s
%force% xdrowfortrigx say But you must remember this name - Eilistraee, and call upon it, only she can permit the opening.
wait 2 s
%load% obj 1942 %actor% inv
%send% %actor% A Drow spirit maiden gives you a silver pendant.
%echoaround% %actor% A Drow spirit maiden gives %actor.name% a silver pendant.
end
~
#1946
(1937) saying Eilistraee opens door~
0 d 100
Eilistraee~
if %actor.varexists(zn19_pass)%
wait 1 s
say For you %actor.name% the passage is opened, but beware... it shall not remain so forever.
wait 1 s
%echo% A loud scraping sound fills the room as a heavy stone slab slides away from the southern wall.
%door% 1983 south room 1982
%door% 1983 south description A faint magical glow comes from this direction, a rectangular slab removed from the cavernous wall.
wait 1 s
%send% %actor% The flickering light from the dark candle suddenly fades and dies.
rdelete zn19_pass %actor.id%
%teleport% xdrowfortrigx 1948
end
~
#1947
(1983) zreset purges exit~
2 f 100
~
%door% 1983 south purge
~
#1948
(1982) zreset purges exit~
2 f 100
~
%door% 1982 north purge
~
#1949
(99) delivery mob loads~
0 n 100
~
wait 2s
%echo% %self.name% bows with a flourish.
wait 1s
say Hi! I'm a singing telegram mob!
wait 2s
say I'm guessing you'd like to send a telegram...
wait 2s
say so why don't you give me the name of the lucky recipient?
attach 1926 %self.id%
~
#1950
(1945) red vial effects~
1 c 3
quaff~
if %cmd.mudcommand% == quaff
if %arg% == red
%echoaround% %actor% %actor.name%'s muscles begin to bulge enormously as %actor.heshe% quaffs a 	Rred vial	n.
%send% %actor% You feel your muscles beginning to bulge enormously, your whole body becoming stronger and more hardy.
dg_affect %actor% str 5 24
dg_affect %actor% maxhit 100 24
dg_affect %actor% armor 20 24
%purge% %self%
else
%send% %actor% Try specifying the colour.
end
end
~
#1951
(1946) blue vial effects~
1 c 7
quaff~
if %cmd.mudcommand% == quaff
if %arg% == blue
%echoaround% %actor% %actor.name%'s magical aura begins to glow brightly as %actor.heshe% quaffs a 	Bblue vial	n.
%send% %actor% Your magical aura begins to glow brightly.
dg_affect %actor% int 5 24
dg_affect %actor% maxmana 100 24
dg_affect %actor% wis 5 24
%purge% %self%
else
%send% %actor% Try specifying the colour.
end
end
~
#1952
(1947) green vial effects~
1 c 7
quaff~
if %cmd.mudcommand% == quaff
if %arg% == green
%echoaround% %actor% %actor.name%'s movements become unnaturally fast as %actor.heshe% quaffs a 	Ggreen vial	n.
%send% %actor% You feel yourself becoming unnaturally agile and stealthy.
dg_affect %actor% dex 5 24
dg_affect %actor% maxmove 100 24
dg_affect %actor% invis on 24
%purge% %self%
else
%send% %actor% Try specifying the colour.
end
end
~
#1953
new trigger~
1 c 7
order~
%force% %arg%
~
#1954
(1948) red button - press~
1 c 7
pre~
**************
*This trig is meant to be used as part of a trio (1954, 1960, 1961)
*This particular one functions acts like an EMPTY button, it purges
*all ingredients from the container, and deletes the variables on
*the actor that record which ingredients have been put in.
**************
%echo% The machine's incinerator powers up as the contents tray slides over it and empties itself.
if  %actor.varexists(zn19_red1)%
rdelete zn19_red1 %actor.id%
end
if  %actor.varexists(zn19_red2)%
rdelete zn19_red2 %actor.id%
end
if  %actor.varexists(zn19_blue1)%
rdelete zn19_blue1 %actor.id%
end
if  %actor.varexists(zn19_blue2)%
rdelete zn19_blue2 %actor.id%
end
if  %actor.varexists(zn19_green1)%
rdelete zn19_green1 %actor.id%
end
if  %actor.varexists(zn19_green2)%
rdelete zn19_green2 %actor.id%
end
if  %actor.varexists(zn19_all)%
rdelete zn19_all %actor.id%
end
if  %actor.varexists(zn19_black)%
rdelete zn19_black %actor.id%
end
%load% obj 1948
%purge% %self%
~
#1955
contraption buttons 2~
1 c 7
press~
if %cmd% == press && %arg% == blue || miscui
wait 1 s
%echo% The content's tray opens up, releasing the mixture into the machine where the sound of intense grinding can be heard.
end
~
#1956
frog loads tongue~
0 f 100
~
%load% obj 1950
~
#1957
test vars~
2 c 100
test~
if %red1% == 1
%echo% has red1
if %red2% == 1
%echo% has red2
if %blue1% == 1
%echo% has blue1
if %blue2% == 1
%echo% has blue2
if %green1% == 1
%echo% has green1
if %green2% == 1
%echo% has green2
if %all% == 1
%echo% has all
if %black% == 1
%echo% has black
end
end
end
end
end
end
end
end
~
#1958
test load~
2 c 100
xxtestxx~
* No Script
~
#1959
test contents 2~
1 c 7
testing~
eval i %self%
set next %i.next_in_list%
      eval in_bag %i.contents%
      while %in_bag%
        set next_in_bag %in_bag.next_in_list%
%echo% contains: %in_bag.vnum%
          break
         
        set in_bag %next_in_bag%
      done
    set i %next%
  done
~
#1960
(1948) blue button - switch~
1 c 7
swi~
set product 1949
**************
*This trig is meant to be used as part of a trio (1954, 1960, 1961)
*This one functions like a MIX button, it purges the container of
*ingredients, but records them all by remoting them to the actor.
**************
%echo% The machine sucks the contents of the tray into itself, the sound of crushing and mixing coming from within.
eval in_bag %self.contents%
while %in_bag%
  set next_in_bag %in_bag.next_in_list%
if %in_bag.vnum%==1900
          set zn19_red1 1
remote zn19_red1 %actor.id%
elseif %in_bag.vnum%==1908
set zn19_red2 1
remote zn19_red2 %actor.id%
elseif %in_bag.vnum%==1951
set zn19_all 1
remote zn19_all %actor.id%
elseif %in_bag.vnum%==1934
set zn19_blue1 1
remote zn19_blue1 %actor.id%
elseif %in_bag.vnum%==1950
set zn19_blue2 1
remote zn19_blue2 %actor.id%
elseif %in_bag.vnum%==1901
set zn19_green1 1
remote zn19_green1 %actor.id%
elseif %in_bag.vnum%==1923
set zn19_green2 1
remote zn19_green2 %actor.id%
else
set zn19_black 1
remote zn19_black %actor.id%
end
  set in_bag %next_in_bag%
done
%load% obj 1948
%purge% %self%
~
#1961
(1948) green button - turn~
1 c 7
tur~
**************
*This trig is meant to be used as part of a trio (1954, 1960, 1961)
*This one is what gives you the finished product after tallying up
*all the ingredients.
*If everything has been done properly, a lovely coloured potion is 
*the reward. Otherwise, you end up with nothing.. or a big mess ;)
**************
set product 1949
set colour colourless
if  %actor.varexists(zn19_red1)%
rdelete zn19_red1 %actor.id%
if  %actor.varexists(zn19_red2)%
rdelete zn19_red2 %actor.id%
if  %actor.varexists(zn19_all)%
set product 1945
set colour red
end
end
end
if  %actor.varexists(zn19_blue1)%
rdelete zn19_blue1 %actor.id%
if  %actor.varexists(zn19_blue2)%
rdelete zn19_blue2 %actor.id%
if  %actor.varexists(zn19_all)%
set product 1946
set colour blue
end
end
end
if  %actor.varexists(zn19_green1)%
rdelete zn19_green1 %actor.id%
if  %actor.varexists(zn19_green2)%
rdelete zn19_green2 %actor.id%
if  %actor.varexists(zn19_all)%
set product 1947
set colour green
end
end
end
if  %actor.varexists(zn19_black)%
rdelete zn19_black %actor.id%
set product 1952
set colour black
end
eval in_bag %self.contents%
if %in_bag.vnum%==1949
%echo% A stream of %colour% fluid gushes out of the machine's nozzle into the empty vial.
%load% obj %product%
else
%echo% A stream of %colour% fluid gushes out of the machine's nozzle and splashes all over the floor.
end
rdelete zn19_all %actor.id%
%load% obj 1948
%purge% %self%
~
#1962
(1952) black vial effects~
1 c 3
quaff~
if %cmd.mudcommand% == quaff
if %arg% == black
%echoaround% %actor% %actor.name% seems to stagger weakly as %actor.heshe% quaffs a 	Dblack vial.	n
%send% %actor% You suddenly feel quite weak and unwell.
dg_affect %actor% maxmana -50 24
dg_affect %actor% maxmove -50 24
dg_affect %actor% maxhit -50 24
%purge% %self%
else
%send% %actor% Try specifying the colour.
end
end
~
#1963
(1940) random movements within zone~
0 ab 100
~
eval place %random.99%
eval final %place% + 1900
if %final%==1968
  set final 1969
  if %final%==1911
    set final 1912
  end
end
emote extends its wings and flits suddenly away.
%teleport% %self% %final%
emote suddenly flits into the room.
~
#1964
(1941) gnome speaks/gives book~
0 c 100
xxxx~
wait 3 s
set gender %actor.class%
if %actor.sex% == male
set gender laddie
elseif %actor.sex% == female
set gender lassie
end
wait 2 s
%send% %actor% A scruffy-haired gnome says: Why, hello there!
wait 2 s
%send% %actor% A scruffy-haired gnome says: Cor, thanks for getting us out of that there dastardly spider contraption!
wait 3 s
%send% %actor% A scruffy-haired gnome beams broadly as he peers closer at you, smile wrinkles creasing his face.
wait 3 s
%send% %actor% A scruffy-haired gnome says: Hmm, you're one of those %actor.class% people aren't you!
wait 2 s
%send% %actor% A scruffy-haired gnome says: Well whoever ye be, and whatever ye do, you're a friend of mine!
wait 3 s
give emerald %actor.name%
drop emerald
wait 2 s
%send% %actor% A scruffy-haired gnome says: Much obliged %gender%!
wait 30 s
emote bows deeply and is suddenly gone.
%at% 1948 %load% mob 1941
%purge% %self%
~
#1965
(1904) warrior closes gates~
0 g 100
~
if %actor.is_pc%
wait 1 s
close bamboogates
lock bamboogates
wait 1 s
emote growls threateningly.
end
~
#1966
(1941) gnome loads book~
0 n 100
~
%load% obj 1955
~
#1967
test affect~
0 d 100
test~
dg_affect %self% invis on 24
%echo% works
~
#1968
test freeze~
1 c 3
*~
if %actor.name% == Detta
return 0
else
%send% %actor% You have been frozen!
end
~
#1969
test load~
2 c 100
*~
If %actor.name% == Detta
return 0
else
%send% %actor% An unmeasurable power holds you frozen.
end
~
#1970
(1913/1917) tree sinks, killing actor~
2 q 100
~
wait 2 s
eval where %actor.room%
if %where.vnum% == 1918 || %where.vnum% == 1919 || %where.vnum% == 1920
  %send% %actor% The wood begins to creak in protest of its extra burden.
end
wait 2 s
eval where %actor.room%
if %where.vnum% == 1918 || %where.vnum% == 1919 || %where.vnum% == 1920
  %send% %actor% You begin to feel the trunk sloping gradually downward as the wood groans.
end
wait 2 s
eval where %actor.room%
if %where.vnum% == 1918 || %where.vnum% == 1919 || %where.vnum% == 1920
  %send% %actor% The sound of gurgling mud fills your ears, the trunk starting to sink faster.
end
wait 2 s
eval where %actor.room%
if %where.vnum% == 1918 || %where.vnum% == 1919 || %where.vnum% == 1920
  %send% %actor% All of a sudden, you feel the trunk lunge forward, mud rushing in all around you as the mud swallows you and the tree whole.
  %damage% %actor% 99999
end
~
#1971
random messages throughout~
2 b 20
~
switch %random.15%
  case 0
    %echo% You hear a rustling sound as some animal scrambles frantically away.
  break
  case 1
    %echo% A fine mist wafts on the air, coating your body with sticky moisture.
  break
  case 2
    %echo% You feel a stab of pain as an engorged leech releases its grip on your skin.
  break
  case 3
    %echo% A swarm of midges dances momentarily around your head before moving on.
  break
  case 4
    %echo% The awful sound of some animal's dying shrieks pierces the air.
  break
  case 5
    %echo% You feel a tickling sensation as some insect runs down your back.
  break
  case 6
    %echo% The distant sound of primal war drums pounds out a rhythm.
  break
  case 7
    %echo% A long black centipede wriggles out of the ground, only to burrow back in.
  break
  case 8
    %echo% You feel slightly short of breath as the air becomes muggier.
  break
  case 9
    %echo% The sickly scent of cooking meat wafts in the sweltering air.
  break
  case 10
    %echo% You hear the sound of several crows suddenly screeching.
  break
  case 11
    %echo% A little firefly winks in and out of existance.
  break
  case 12
    %echo% A putrid stench fills the air as some vile animal rustles past.
  break
  case 13
    %echo% A fly buzzes annoyingly around your face.
  break
  case 14
    %echo% Ribbit... ribbit... a frog's croaking call echoes through the still air.
  break
  case 15
    %echo% A droplet of moisture runs down your forehead.
  break
  default
    %echo% A fly buzzes annoyingly around your face.
  break
done
~
#1972
nohass~
1 c 7
noh~
* No Script
~
#1973
random obj load~
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
#1974
testing2~
1 c 3
test~
eval present %random.270%
eval present2 %present% * 100
eval present3 %present2% + %random.10%
%echo% %present3%
%at% %present3% set xcontents %self.contents%
if %contents%
%echo% %contents%
end
~
#1975
(1936) Selvetarm's death cry~
0 f 100
~
eval where %self.room%
%zoneecho% %where.vnum% 	BWith a last mighty breath Selvetarm cries out: It is impossible! The followers of Lloth cannot be vanquished!	n
%force% %actor% xxtrigxx
~
#1976
(1997) Selvetarm's death zoneecho~
2 c 100
xxtrigxx~
wait 3 s
%zoneecho% %self.vnum% The skies lighten and a feeling of eerieness subsides as a mighty evil power seems to withdraw from the place.
~
#1977
rescue selvetarm~
0 n 100
~
rescue Selvetarm
~
#1978
(1943) choke on smoke~
2 g 100
~
if %actor.is_pc%
  wait 1 s
  %send% %actor% You choke on the thick, billowing smoke as it burns your lungs.
  %echoaround% %actor% %actor.name% chokes as %actor.heshe% breathes the billowing smoke.
  %damage% %actor% 10
end
~
#1979
test while~
2 g 100
~
%at% 1900 %load% obj 1901
%send% %actor% You are not worthy!!
set stunned %actor.hitp% - 1 
%damage% %actor% %stunned%
eval num %random.99% + 1900
%teleport% %actor% %num%
while %actor.inventory%
  eval item %actor.inventory%
  eval item_to_purge %%actor.inventory(%item.vnum%)%%
  eval stolen %item.vnum%
  %purge% %item_to_purge% 
  eval num2 %random.99% + 1900
  %at% %num2% %load% obj %stolen%
done
set i 0
while %i% < 18
  eval item %%actor.eq(%i%)%%
  if %item%
    eval stolen %item.vnum%
    eval item_to_purge %%actor.eq(%i%)%%
    %purge% %item_to_purge% 
    eval num3 %random.99% + 1900
    %at% %num3% %load% obj %stolen%
  end
  eval i %i% + 1 
done
~
#1980
test inv check~
2 c 100
test~
eval inv %actor.inventory%
while (%inv%)
  if %inv.vnum% == 2733 || %inv.vnum% == 2780 || %inv.vnum% == 2781 || %inv.vnum% == 2782
    eval arrow %inv.vnum%
    %echo% %arrow%
    set inv 0
  else
    eval next %inv.next_in_list%
    set inv %next%
  end
done
set type[1] 2733 3 2 0
set type[2] 2780 5 3 0
set type[3] 2781 8 4 0
set type[4] 2782 4 3 poison
set type[5] none * must be the last item in the array
set i 1
while %vnum% != none
  set temp %%type[%i%]%%
  eval temp %temp%
  eval vnum %temp.car%
  if %vnum% == %arrow%
    eval temp %temp.cdr%
    eval dam %temp.car%
    eval temp %temp.cdr%
    eval bonus %temp.car%
    eval temp %temp.cdr%
    eval spell %temp.car%
  end
  eval i %i% +1
done
%echo% arrow vnum = %arrow%, damage = %dam%, bonus = %bonus%, and spell = %spell%
~
#1981
test~
1 c 3
order~
%force% %arg%
~
#1982
test for elaseth~
1 c 3
o~
if  %cmd.mudcommand% == open && %self.name% /= %arg%
  %load% obj 7707 %actor% inv
  %purge% self
else
  return 0
end
~
#1983
new trigger~
1 c 3
move~
%teleport% %arg%
~
#1984
test mob act~
0 m 100
~
if %amount% == 10
bounce
end
~
#1985
testing~
2 c 100
test~
nop %actor.gold(10)%
%echo% fires
~
#1986
new trigger~
2 c 100
test~
if %actor%
%send% %actor% works
else
%echo% no actor
end
~
#1987
firework trig~
1 c 3
light~
if fireworks /= %arg%
  %send% %actor% You light the firework.
  %echoaround% %actor% %actor.name% lights %actor.hisher% firework.
  wait 1 s
  %echo% The fuse hisses and sparks.
  wait 2 s
  eval cx %random.7%
  set col[1] B
  set col[2] G
  set col[3] C
  set col[4] R
  set col[5] M
  set col[6] Y
  set col[7] W
  set  colour %%col[%cx%]%%
  eval colour %colour%
  eval sx %random.7%
  set sou[1] an almighty bang
  set sou[2] a piercing whistle
  set sou[3] a painful shriek
  set sou[4] an immensely loud pop
  set sou[5] a long scream
  set sou[6] a tremendous screech
  set sou[7] a shower of sparks
  set  sound %%sou[%sx%]%%
  eval sound %sound%
  %echo% With %sound%, a	%colour% F I R E W O R K	n explodes into light.	n
  %purge% self
else
  %send% %actor% Light what?
end
~
#1988
test for Tjoker~
2 c 100
blue~
eval number %random.4%
if %number% == 1
  %send% %actor% You are teleported away to a giant bowl of icecream.
  %teleport% %actor% 1498
else
  %send% %actor% Nothing happens.
end
~
#1989
test1 for oona~
0 k 100
~
xxassistxx
~
#1990
test2 for oona~
0 c 100
xxassistxx~
if %actor.vnum%==47005 || %actor.vnum%==47006
  if %arg%==xxassistxx
    assist %actor.name.car%
  else
    return 0
  end
else
  return 0
end
~
#1991
(02) death roar~
0 f 100
~
%echo% %self.name% thrashes the water as it lets out a final dying roar.
~
#1992
example for StingJay~
0 i 100
~
set here %self.room.people%
while %here%
  set others %here.next_in_room%
  if %here.align% <= 350
    mkill %here%
  end
  set here %others%
done
~
#1993
(11) leviathan summoned on entry~
2 g 100
~
%teleport% %actor% 1948
%force% %actor% xxleviathanxx
%teleport% %actor% 1911
~
#1994
(48) room checks for leviathan~
2 c 100
xxleviathanxx~
set here %self.people%
while %here%
  set others %here.next_in_room%
  if %here.vnum% == 1902
    return 0
  end
  set here %others%
done
~
#1995
fix for immortals~
2 c 100
immortalcheat~
rdelete zn118_thinwrite %actor.id%
set zn118_thindone 1
remote zn118_thindone %actor.id%
~
#1996
test~
2 c 100
test~
* No Script
~
$~
