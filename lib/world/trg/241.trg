#24100
(24106) Teleporter~
2 c 100
energize~
%send% %actor% You press various buttons and slowly pull a black lever down.
%echoaround% %actor% %actor.name% presses various buttons and slowly pulls a black lever down.
%echo% There is a bright glow emanating from the north.
%at% 24142 %echo% A beam of bright light envelops you!
if %location% <1
  %at% 24142 %teleport% all 24141
  wait 3 sec
  %echo% Lots of warning lights go off!
  %send% %actor% Something has gone wrong! You forgot to input the coordinates!
else
  %at% 24142 %teleport% all %location%
  wait 3 sec
  %echo% The teleporter switches to stand-by. The teleportation was a success!
end
~
#24101
(24106) Input Teleporter~
2 c 100
input~
if %arg% <1
  %send% %actor% You need to input some coordinates.
  %send% %actor% 'INPUT 24100' for example.
elseif %arg% < 24100
  %send% %actor% Those coordinates are to low! (24100-24141)
elseif %arg% > 24141
  %send% %actor% Those coordinates are to high! (24100-24141)
else
  %send% %actor% You type briefly on a nearby keyboard.
  %echoaround% %actor% %actor.name% types briefly on a nearby keyboard.
  wait 1 sec
  %send% %actor% Coordinates set to '%arg%'.
  set location %arg%
  global location
end
~
#24102
(24110, 11, 32, 33) Turbolift~
2 d 100
*~
if %self.vnum% == 24110 && %speech% == deck 1
  %send% %actor% The turbolift doesn't budge an inch. You're already there!
elseif %self.vnum% == 24110 && %speech% == deck 2
  %echo% The turbolift zips upward and suddenly stops.
  %teleport% all 24111
elseif %self.vnum% == 24110 && %speech% == deck 3
  %echo% The turbolift zips upward rapidly and then comes to a sudden stop.
  %teleport% all 24132
elseif %self.vnum% == 24110 && %speech% == deck 4
  %echo% The turbolift flies upward very fast and slows down as it reaches the main bridge.
  %teleport% all 24133
elseif %self.vnum% == 24111 && %speech% == deck 1
  %echo% The turbolift descends for a short distance and then halts.
  %teleport% all 24110
elseif %self.vnum% == 24111 && %speech% == deck 2
  %send% %actor% The turbolift doesn't budge an inch. You're already there!
elseif %self.vnum% == 24111 && %speech% == deck 3
  %echo% The turbolift zips upward rapidly and then comes to a sudden stop.
  %teleport% all 24132
elseif %self.vnum% == 24111 && %speech% == deck 4
  %echo% The turbolift zips upward and suddenly stops.
  %teleport% all 24133
elseif %self.vnum% == 24132 && %speech% == deck 1
  %echo% The turbolift descends quite rapidly before stopping a moment or so later.
  %teleport% all 24110
elseif %self.vnum% == 24132 && %speech% == deck 2
  %echo% The turbolift sinks for a second or two and then ceases to move.
  %teleport% all 24111
elseif %self.vnum% == 24132 && %speech% == deck 3
  %send% %actor% The turbolift doesn't budge an inch. You're already there!
elseif %self.vnum% == 24132 && %speech% == deck 4
  %echo% The turbolift slowly rises and then peacefully stops.
  %teleport% all 24133
elseif %self.vnum% == 24133 && %speech% == deck 1
  %echo% The turbolift plummets for about 5 seconds and then suddenly stops!
  %teleport% all 24110
elseif %self.vnum% == 24133 && %speech% == deck 2
  %echo% The turbolift plummets for about 3 seconds and then comes to a stop.
  %teleport% all 24111
elseif %self.vnum% == 24133 && %speech% == deck 3
  %echo% The turbolift plummets for about a second and then ceases to move.
  %teleport% all 24132
elseif %self.vnum% == 24133 && %speech% == deck 4
  %send% %actor% The turbolift doesn't budge an inch. You're already there!
end
~
#24103
(24111) Play Flute~
1 c 2
play~
eval flute %random.10%
switch %flute%
  case 1
    set word beautifully
  break
  case 2
    set word wonderfully
  break
  case 3
    set word magnificently
  break
  case 4
    set word stupendously
  break
  case 5
    set word gracefully
  break
  case 6
    set word exquisitely
  break
  case 7
    set word splendidly
  break
  case 8
    set word sublimely
  break
  case 9
    set word superbly
  break
  case 10
    set word delightfully
  break
  default
  break
done
if %cmd% == play && flute /= %arg% && %arg%
  %send% %actor% You play %self.shortdesc% %word%.
  %echoaround% %actor% %actor.name% plays %actor.hisher% flute %word%.
else
  %send% %actor% What do you want to play?
end
~
#24104
(24106/5) Picard/Riker Play Their Instruments~
0 b 5
~
if %self.vnum% == 24105
  play trombone
elseif %self.vnum% == 24106
  play flute
end
~
#24105
(24106) Tap Badge~
1 c 1
tap~
if %cmd% == tap && badge /= %arg% && %arg%
  %send% %actor% You absently tap your Starfleet communication badge.
  %echoaround% %actor% %actor.name% absently taps %actor.hisher% Starfleet communication badge.
  %send% %actor% It emits a brief electronic sound of recognition.
  %echoaround% %actor% %actor.name%'s Starfleet communication badge emits a brief electronic sound of recognition.
else
  %send% %actor% What do you want to tap?
end
~
#24106
(24112) Play Trombone~
1 c 2
play~
eval flute %random.10%
switch %flute%
  case 1
    set word noisily
  break
  case 2
    set word horrifically
  break
  case 3
    set word awfully
  break
  case 4
    set word terribly
  break
  case 5
    set word lustily
  break
  case 6
    set word unpleasantly
  break
  case 7
    set word wickedly
  break
  case 8
    set word clumsily
  break
  case 9
    set word poorly
  break
  case 10
    set word dreadfully
  break
  default
  break
done
if %cmd% == play && trombone /= %arg% && %arg%
  %send% %actor% You play %self.shortdesc% %word%.
  %echoaround% %actor% %actor.name% plays %actor.hisher% trombone %word%.
else
  %send% %actor% What do you want to play?
end
~
#24107
(24139/40/41) Lost in Space~
2 g 100
~
eval breathe %random.5%
switch %breathe%
  case 1
    set gasp You gasp and struggle for breath
    set gasp2 gasps and struggles for breath
  break
  case 2
    set gasp You can't breathe
    set gasp2 can't breathe
  break
  case 3
    set gasp You are slowly turning a horrid shade of blue
    set gasp2 is slowly turning a horrid shade of blue
  break
  case 4
    set gasp Your eyes bulge grossly
    set gasp2 stares in horror
  break
  case 5
    set gasp You wheeze and cough, close to death
    set gasp2 wheezes and coughs, close to death
  break
  default
  break
done
if %actor.inventory(24126)%
  wait 1
  %send% %actor% You inhale and exhale very slowly.
else
  wait 1
  %send% %actor% %gasp%!
  %echoaround% %actor% %actor.name% %gasp2%!
  %damage% %actor% 50
end
~
#24108
(24107) Get Spacesuit~
1 c 100
get~
if get /= %cmd.mudcommand% && spacesuit /= %arg% && %arg%
  %send% %actor% You carefully lift down one of the spacesuits.
  %echoaround% %actor% %actor.name% carefully lifts down one of the spacesuits.
  %purge% %actor.inventory(24126)%  
  %load% o 24126 %actor% inv
else
  return 0
end
~
#24109
(24111) Spot the Cat~
0 g 100
~
if %actor.vnum% == 24101
  %echo% %self.name% purrs very excitedly!
  wait 3 sec
  lick data
else
  hiss %actor.name%
  wait 3 sec
  *%echo% %self.name% meows!
  meow
end
~
#24110
(24101) After Spot Licks Data~
0 e 0
spot licks you~
wait 1 sec
say that is a very good spot
wait 5 sec
ruffle spot
~
#24111
(24128) Replicator~
2 d 100
*~
* These are random FOOD objects through out the MUD. Replace with your own.
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
if %speech% == tea, earl grey, hot
  %echo% A light flashes inside the replicator and a cup of hot earl grey tea appears.
  %load% o 24129 %actor% inv
elseif %speech% == tea
  %echo% A light flashes inside the replicator and a cup of hot tea appears.
  %load% o 24129 %actor% inv
elseif %speech% == bread
  %echo% A light flashes inside the replicator and a loaf of warm bread appears.
  %load% o 24130 %actor% inv
elseif %speech% == coffee
  %echo% A light flashes inside the replicator and mug of black coffee appears.
  %load% o 24131 %actor% inv
elseif %speech% == beer
  %echo% A light flashes inside the replicator and a brown bottle of beer appears.
  %load% o 3921 %actor% inv
else
  %echo% A red light blinks on the replicator and something appears with a flash of light.
  %load% o %grub% %actor% inv
end
~
#24112
(24128) Picard Orders Tea~
0 b 100
~
eval location %self.room%
if %location.vnum% == 24128
  say tea, earl grey, hot
  wait 180 s
end
~
#24113
(24101/06) When Data or Picard enter a turbolift~
0 b 100
~
eval location %self.room%
if %self.vnum% == 24106 && %location.vnum% == 24133
  say deck 3
elseif %self.vnum% == 24101 && %location.vnum% == 24132
  say deck 1
elseif %self.vnum% == 24106 && %location.vnum% == 24128
  say tea, earl grey, hot
  wait 180 s
else
  eval number %random.20%
  switch %number%
    case 1
      if %self.vnum% == 24106 && %actor.inventory(24111)%
        play flute
      elseif if %self.vnum% == 24105 && %actor.inventory(24112)%
        play trombone
      end
    break
    default
    break
  done
end
~
#24114
(24110) Feed Livingston~
0 c 100
feed~
eval fish %random.5%
switch %fish%
  case 1
    set fishy Livingston the fish happily swims around in his bowl.
  break
  case 2
    set fishy Livingston the fish gently nibbles at the fish food.
  break
  case 3
    set fishy Livingston smiles, if you can call it that, he is a fish after all.
  break
  case 4
    set fishy Livingston the fish floats around in his bowl for a while.
  break
  case 5
    set fishy Livingston rises to the top of his bowl and starts to eat some of the fish food.
  break
  default
  break
done
if livingston /= %arg% && %arg%
  %send% %actor% You sprinkle a small amount of fish food into Livingston's bowl.
  %echoaround% %actor% %actor.name% sprinkles a small amount of fish food into Livingston's bowl.
  wait 3 sec
  %echo% %fishy%
elseif fish /= %arg% && %arg%
  %send% %actor% You sprinkle a small amount of fish food into Livingston's bowl.
  %echoaround% %actor% %actor.name% sprinkles a small amount of fish food into Livingston's bowl.
  wait 3 sec
  %echo% %fishy%
else
  %send% %actor% What do you want to feed?
end
~
#24115
(24110, 11, 32, 33) Turbolift greetings~
2 g 50
~
wait 1
if %self.vnum% == 24110
  %echo% A female voices announces, 'This is deck one'
elseif %self.vnum% == 24111
  %echo% A female voices announces, 'This is deck two'
elseif %self.vnum% == 24132
  %echo% A female voices announces, 'This is deck three'
elseif %self.vnum% == 24133
  %echo% A female voices announces, 'This is deck four'
end
~
#24116
(24108) Geordi's Visor Effects~
1 c 1
l~
if look /= %cmd.mudcommand%
  %send% %actor% Everything is pixelated and very hard to make out. Everything is
  %send% %actor% Displayed differently depending its current temperature and distance
  %send% %actor% From you. On the left of your vision there is a small read-out.
  %send% %actor% It details information such as height of target, heart-rate if it's
  %send% %actor% living and the amount of heat radiated.
else
  return 0
end
~
#24117
(24132) Wield Batleth~
1 c 2
wi~
if %cmd.mudcommand% == wield && batleth /= %arg% && %arg% && %actor.sex% == Male
  return 0
  wait 1
  %send% %actor% You swing the batleth through the air with superior skill.
  %echoaround% %actor% %actor.name% swings the batleth through the air with superior skill.
elseif %cmd.mudcommand% == wield && batleth /= %arg% && %arg% && %actor.sex% == Female
  return 0
  wait 1
  %send% %actor% You scream your warcry and glare at any nearby males!
  %echoaround% %actor% %actor.name% screams %actor.hisher% warcry and glares at any nearby males!
elseif %cmd.mudcommand% == wield && batleth /= %arg% && %arg% && %actor.sex% == Neutral
  return 0
  wait 1
  %send% %actor% You flow through your learned batleth movements and finish in a defensive stance.
  %echoaround% %actor% %actor.name% flows through %actor.hisher% learnt batleth movements and finishes in a defensive stance.
else
  return 0
end
~
#24118
(24100/2/3/4/7/8/9/12/13) Random Speech~
0 b 2
~
if %self.vnum% == 24100
  say the dilithium crystals might overheat
elseif %self.vnum% == 24102
  %echo% %self.name% grunts and mutters, 'secure'
elseif %self.vnum% == 24103
  %echo% %self.name% taps a few buttons on her tricorder and scans you.
elseif %self.vnum% == 24104
  %echo% %self.name% seems to bear the stress of everyone onboard.
elseif %self.vnum% == 24107
  %echo% %self.name% makes up a rather alien cocktail and serves it.
elseif %self.vnum% == 24108
  %echo% %self.name% checks the warp core carefully.
elseif %self.vnum% == 24109
  say can I be of assistance?
elseif %self.vnum% == 24112
  say erm... uh... can.. I.. erm..
elseif %self.vnum% == 24113
  laugh
end
~
#24119
(24120) Security Replicator~
2 d 100
*~
%echo% A female voice announces, 'you are not a member of the security team'
%echo% A female voice announces, 'you do not have sufficient authority'
~
#24120
(24109) HoloDeck~
2 d 100
*~
*if %speech% == help
*  %echo% A female voice announces, 'The Holodeck commands are as follows:'
*  %echo% A female voice announces, 'A list of games can be found on the screen'
*  %echo% A female voice announces, 'Say the name of a program to load it'
*  %echo% A female voice announces, 'Say DOOR to locate the exit'
if %speech% == worf01
  %echo% A surreal alien world shimmers into view.
  %teleport% all 24143
elseif %speech% == riker05
  %echo% A female voice announces, 'That program is of an adult nature and cannot be'
  %echo% A female voice announces, 'viewed by you at this time'
elseif %speech% == picard07A
  %echo% A private detective's office is suddenly created before your eyes!
  %teleport% all 24144
elseif %speech% == data09
  %echo% A scientific laboratory assembles itself instantly!
  %teleport% all 24145
elseif %speech% == laforge11
  *  %echo% A pleasant English garden comes into focus.
  *  %teleport% all 35923
  %echo% A female voice announces, 'That program is temporarily unavailable' 
elseif %speech% == programs
  %echo% A female voice announces, 'The following programs exist:'
  %echo% A female voice announces, 'Worf01'
  %echo% A female voice announces, 'Riker05'
  %echo% A female voice announces, 'Picard07A'
  %echo% A female voice announces, 'Data09'
  %echo% A female voice announces, 'LaForge11'
else
  return 0
end
~
#24121
(24122) Open Shuttle Bay Doors~
2 c 100
op~
*I know this could all be 1 trigger, but it's 2 now. It doesn't matter :)
***************************
if %var% == closed && %cmd% == open && door /= %arg% && %arg%
  return 0
  %echo% The oxygen drains from this shuttle bay!
  wait 1
  %echo% You are dragged into space!
  wait 1
  %teleport% all 24139
  set var open
  global var
else
  return 0
end
~
#24122
(24122) Set Doors are closed~
2 f 100
~
set var closed
global var
~
#24123
(24122) Close Shuttle bay Doors~
2 c 100
clo~
if %var% == open && %cmd% == close && door /= %arg% && %arg%
  return 0
  set var closed
  global var
else
  return 0
end
~
#24124
(24144/45) Exit Vocal Command~
2 d 100
*~
if %self.vnum% == 24144 && %speech% == exit
  wait 1 sec
  %echo% A respectable-looking wooden door shimmers into view to the west.
  %door% 24144 w room 24108
  wait 20 s
  %echo% The large archway collapses.
  %door% 24144 w purge
elseif %self.vnum% == 24145 && %speech% == exit
  wait 1 sec
  %echo% A pair of plastic, bullet-proof doors shimmer into view to the west.
  %door% 24145 w room 24108
  wait 20 s
  %echo% The large archway collapses.
  %door% 24145 w purge
else
  return 0
end
~
#24125
(24143) Worf01 Difficulty~
2 d 100
*~
if %speech% == difficulty
  %echo% A female voice announces, 'Please state the difficulty required (Difficulty EASY)'
elseif %charges% <4 && %speech% == difficulty easy
  %echo% A slightly dangerous looking alien suddenly fades into view!
  %load% m 24114
  eval charges %charges% + 1
  global charges
elseif %charges% <4 && %speech% == difficulty medium
  %echo% A very dangerous looking alien is created from billions of pixels!
  %load% m 24115
  eval charges %charges% + 1
  global charges
elseif %charges% <4 && %speech% == difficulty hard
  %echo% A Klingon warrior roars and howls as he comes into existence!
  %load% m 24116
  eval charges %charges% + 1
  global charges
elseif %charges% >3 && %speech% == difficulty easy
  %echo% A female voice announces, 'You have run out of credit'
elseif %charges% >3 && %speech% == difficulty medium
  %echo% A female voice announces, 'You have run out of credit'
elseif %charges% >3 && %speech% == difficulty hard
  %echo% A female voice announces, 'You have run out of credit'
elseif %speech% == exit
  %echo% A large archway shimmers into view to the west.
  %door% 24143 w room 24108
  wait 20 s
  %echo% The large archway collapses.
  %door% 24143 w purge
else
  return 0
end
~
#24126
(24143) Reset Holodeck~
2 f 100
~
set charges 0
global charges
~
#24127
(24109/44/45) Holodeck Welcome~
2 g 100
~
eval mes %random.5%
switch %mes%
  case 1
    set msg There is a sudden gun-shot from outside!
  break
  case 2
    set msg There is a knock on the door.
  break
  case 3
    set msg The clock on the wall chimes.
  break
  case 4
    set msg There is a scream from outside the door!
  break
  case 5
    set msg The sound of footsteps resonate from outside.
  break
  default
  break
done
if %self.vnum% == 24109
  wait 1 sec
  %echo% A female voice announces, 'Welcome to the Holodeck'
  %echo% A female voice announces, 'Please refer to the screen for instructions'
elseif %self.vnum% == 24143
  wait 1 sec
  %echo% A harsh female voice announces, 'Welcome Worf, please select your difficulty'
elseif %self.vnum% == 24144
  wait 1 sec
  %echo% %msg%
elseif %self.vnum% == 24145
  wait 1 sec
  %echo% From inside a cylindrical tank, a robotic voice utters, 'waiting for instruction'.
end
~
#24128
(24144) Look/Read Clock~
2 c 100
*~
if look /= %cmd% && clock /= %arg% && %arg%
  %force% %actor% time
elseif %cmd% == read && clock /= %arg% && %arg%
  %force% %actor% time
else
  return 0
end
~
#24129
(24144) Office Random Script~
2 b 5
~
eval office %random.10%
switch %office%
  case 1
    %echo% The screeching of tires burning-rubber zooms off in the distance.
  break
  case 2
    %echo% The clock ticks and tocks peacefully.
  break
  case 3
    %echo% The telephone on the desk rings for a few moments.
  break
  case 4
    %echo% The window-blinds flicker in the wind.
  break
  case 5
    %echo% The voices from a far-off conversation drift into the office.
  break
  case 6
    %echo% There is a lot of rapid gunfire from out of the window.
  break
  case 7
    %echo% The sound of police sirens is deafening!
  break
  case 8
    %echo% The shrill yell of a paper-boy reaches your ears.
  break
  case 9
    %echo% There is a squeaking sound emanating from a hole in the wall.
  break
  case 10
    %echo% A small grey mouse scrambles out of her hole.
    %load% m 24117
  break
  default
  break
done
~
#24130
(24117) Small Grey Mouse Runs Away~
0 n 100
~
wait 60 s
%echo% A small grey mouse darts back into her hole.
%purge% self
~
#24131
(24118/19) Re-/De-Activate Android~
0 c 100
press~
if %self.vnum% == 24118 && press /= %cmd% && switch /= %arg% && %arg%
  %send% %actor% You gently press the switch hidden in the android's back.
  %echoaround% %actor% %actor.name% gently presses something on the android's back.
  %load% m 24119
  %purge% self
elseif %self.vnum% == 24119 && press /= %cmd% && switch /= %arg% && %arg%
  %send% %actor% You secretly press the hidden switch and de-activate the android.
  %echoaround% %actor% %actor.name% de-activates the android.
  wait 1 sec
  %echo% The lights in %self.name%'s eyes turn off.
  %load% m 24118
  %purge% self
else
  %send% %actor% What do you want to press?
end
~
#24132
(24119) Android Loads~
0 n 100
~
eval part1 %random.1000%
eval part2 %random.10%
switch %part2%
  case 1
    set var A-02
  break
  case 2
    set var B-29
  break
  case 3
    set var Beta
  break
  case 4
    set var Alpha
  break
  case 5
    set var V-006
  break
  case 6
    set var IX
  break
  case 7
    set var VII-A
  break
  case 8
    set var X-Delta
  break
  case 9
    set var B-17
  break
  case 10
    set var CI-Gamma
  break
  default
  break
done
set number %part1%-%var%
wait 1 sec
%echo% %self.name% opens its eyes and glances around the room.
wait 1 sec
%echo% %self.name% looks down at its arm and its fingers twitch slightly.
wait 1 sec
%echo% %self.name% looks straight ahead.
wait 1 sec
say Android Experiment %number% is active and ready for instruction
*******************************
set talk1 please instruct me
global talk1
set talk2 ...
global talk2
set talk3 I am awaiting your vocal commands
global talk3
set talk4 ...
global talk4
set talk5 please, I need input
global talk5
set talk6 ...
global talk6
set talk7 what is emotion?
global talk7
set talk8 ...
global talk8
set talk9 Where are my arms?
global talk9
set talk10 where am I?
global talk10
~
#24133
(24119) Android's Memory Receiver~
0 d 100
*~
eval var %random.10%
switch %var%
  case 1
    set talk1 %speech%
    global talk1
  break
  case 2
    set talk2 %speech%
    global talk2
  break
  case 3
    set talk3 %speech%
    global talk3
  break
  case 4
    set talk4 %speech%
    global talk4
  break
  case 5
    set talk5 %speech%
    global talk5
  break
  case 6
    set talk6 %speech%
    global talk6
  break
  case 7
    set talk7 %speech%
    global talk7
  break
  case 8
    set talk8 %speech%
    global talk8
  break
  case 9
    set talk9 %speech%
    global talk9
  break
  case 10
    set talk10 %speech%
    global talk10
  break
  default
  break
done
wait 2 sec
~
#24134
(24119) Android Randomly Talks From Its Memory~
0 b 51
~
eval talk %random.10%
switch %talk%
  case 1
    say %talk1%
  break
  case 2
    say %talk2%
  break
  case 3
    say %talk3%
  break
  case 4
    say %talk4%
  break
  case 5
    say %talk5%
  break
  case 6
    say %talk6%
  break
  case 7
    say %talk7%
  break
  case 8
    say %talk8%
  break
  case 9
    say %talk9%
  break
  case 10
    say %talk10%
  break
  default
  break
done
~
#24135
Blank~
2 c 100
*~
eval food %random.20%
switch %food%
  case 1
    set grub 27132
  break
  case 2
    set grub 27128
  break
  case 3
    set grub 27199
  break
  case 4
    set grub 27200
  break
  case 5
    set grub 27219
  break
  case 6
    set grub 16706
  break
  case 7
    set grub 31562
  break
  case 8
    set grub 21301
  break
  case 9
    set grub 5463
  break
  case 10
    set grub 24292
  break
  case 11
    set grub 5462
  break
  case 12
    set grub 5461
  break
  case 13
    set grub 2806
  break
  case 14
    set grub 22402
  break
  case 15
    set grub 31724
  break
  case 16
    set grub 13848
  break
  case 17
    set grub 30140
  break
  case 18
    set grub 12383
  break
  case 19
    set grub 23202
  break
  case 20
    set grub 2736
  break
  default
  break
done
if %speech% == tea, earl grey, hot
  %echo% A light flashes inside the replicator and a cup of hot earl grey tea appears.
  %load% o 24129 %actor% inv
elseif %speech% == tea
  %echo% A light flashes inside the replicator and a cup of hot tea appears.
  %load% o 24129 %actor% inv
elseif %speech% == bread
  %echo% A light flashes inside the replicator and a loaf of warm bread appears.
  %load% o 24130 %actor% inv
elseif %speech% == coffee
  %echo% A light flashes inside the replicator and mug of black coffee appears.
  %load% o 24131 %actor% inv
elseif %speech% == beer
  %echo% A light flashes inside the replicator and a brown bottle of beer appears.
  %load% o 3921 %actor% inv
else
  %echo% A red light blinks on the replicator and something appears with a flash of light.
  %load% o %grub% %actor% inv
end
~
#24136
(24109) Tricorder Scan~
1 c 3
scan~
if %self.worn_by%
elseif %self.carried_by%
  %send% %actor% You should hold the tricorder to be able to scan with it.
  halt
end
if %arg%
else
  %send% %actor% You scan the surroundings, but learn nothing.
  halt
end
if %arg.room% == %actor.room%
  %send% %actor% You carefully scan %arg.name% with your tricorder.
  %echoaround% %actor% %actor.name% quickly scans something with %actor.hisher% tricorder.
  wait 1
  %send% %actor% %arg.name% has %arg.hitp%/%arg.maxhitp% hitpoints.
else
  %send% %actor% You cannot see them here to scan.
end
~
#24137
Blank~
2 b 100
~
* No Script
~
#24138
Blank~
2 d 100
*~
if %speech% == begin && %started% == 0
  set started 1
  global started
  %send% %actor% The game begins!
  attach 24137 %self.id%
  wait 60 s
  %send% %actor% Your time is up!
  %send% %actor% Your final score is %gscore%.
  %force% %actor% zz
  detach 24137 %self.id%
  set started 0
  global started
elseif %speech% == begin && %started% == 1
  set gscore 0
  global gscore
  %send% %actor% A game is already in session.
elseif %speech% == end && %started% == 1
  set started 0
  global started
  %send% %actor% Game canceled.
  detach 24137 %self.id%
  set gscore 0
  global gscore
elseif %speech% == end && %started% == 0
  %send% %actor% There isn't a game in session.
end
~
#24139
Blank~
0 g 100
~
* No Script
~
#24146
Blank~
0 g 100
~
* No Script
~
$~
