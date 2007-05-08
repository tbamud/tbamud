#1200
Calculator By Mordecai~
2 d 100
*~
* By Mordecai
if %actor.is_pc%
  Return 0
  Eval sum %speech%
  Eval test1 "%speech%"
  Eval test %test1.strlen%
  Eval che %sum%/1
  If %che% == %sum%
    %echo% @WComputing results...@n
    if (%speech%/===)
      if (%sum%==1)
        set sum Yes
      elseif (%sum%==0)
        set sum No
      end
    end
    Eval st 2+%test%
    Eval o .
    Eval sumslen "%sum%
    Eval len %st% - (%sumslen.strlen%-2)
    If %len% > 0
      Eval dif (%len%/2)
      While %y.strlen% < %st%
        Eval o .%o%
        Eval y %o%
        Eval m ?%m%
        Eval p %m%
        If %dif% == %y.strlen%
          Eval wid1 %p%
        end
      done
    end
    eval opt1 8 + %test%
    eval opt2 (2*%wid1.strlen%)+%sumslen.strlen%+5
    %echo% @WWizzzzzzzzzz....@n
    if (%opt1%-2) == (%opt2%)
      %echo% @c...%y%...@n
      %echo% @c:@C..%y%..@c:@n
      %echo% @c:@C:@G   %speech% @C  :@c:@n
      %echo% @c:@C:.%y%.:@c:@n
      %echo% @c:@C: %wid1%@G %sum% @C%wid1% :@c:@n
      %echo% @c:@C:.%y%.:@c:@n
      %echo% @c:..%y%..:@n
    else
      %echo% @r....%y%...@n
      %echo% @r:@R...%y%..@r:@n
      %echo% @r:@R:@G    %speech% @R  :@r:@n
      %echo% @r:@R:..%y%.:@r:@n
      %echo% @r:@R: %wid1%@G %sum% @R%wid1% :@r:@n
      %echo% @r:@R:..%y%.:@r:@n
      %echo% @r:...%y%..:@n
    end
  end
end
~
#1201
No Recall~
1 c 100
recall~
*This trigger prevents people from recalling.
%send% %actor% Divine forces prevent you from doing that.
~
#1202
Justice sword~
1 j 100
~
* test trigger
%echo% actor : %actor%
wait 10
if %actor.level% < 34
   %send% %actor% The sword whispers: I will not serve you!
   wait 2
   %echoaround% %actor% The sword exclaims: 'I will not serve those without honor.'
   %damage% %actor% 100
   %purge% self
else
   %send% %actor% The sword whispers: I was made to serve, great one!
   wait 2
   %echoaround% %actor% The sword exclaims: 'I will serve you honorable one..'
end
~
#1203
Mynah Bird~
0 d 100
*~
if %actor.is_pc%
  if %c%<20
    eval c 20
    global c
  end
  if !(%speech%/=!)
    eval c (%c%)+1
    global c
    set %c% %speech%
    global %c%
    eval 1 Hello
    eval 2 Yes
    eval 3 Killed him
    eval 4 Food?
    eval 5 Dig dig dig.
    eval 6 Freddy says hi.
    eval 7 Inconceivable.
    eval 8 Stop mimicking me.
    eval 9 I love you.
    eval 10 Do you like me?
    eval 11 Freak.
    eval 12 You are not funny.
    eval 13 Don't you ever shut up?.
    eval 14 Eat my shorts.
    eval 15 I'm pretty. Pretty bird.
    eval 16 Let's tango.
    eval 17 Doh.
    eval 18 Who rang that bell??
    eval 19 Beam me up.
    eval 20 Shut up and get me a drink.
    eval count %%random.%c%%%
    eval ans %%%count%%%
    wait %random.5%
    say %ans%
  endif
endif 
~
#1204
Portal-Main Chamber~
1 c 100
en~
if %cmd.mudcommand% == enter && portal /= %arg%
  wait 1
  %send% %actor% A whirl of white light falls into your eyes, you fall into a huge water fall.
  %echoaround% %actor% A whirl of white light falls into %actor.name% eyes, and %actor.heshe% falls into a huge water fall that appears under %actor.hisher% feet. 
  %teleport% %actor% 3001
  wait 1
  %force% %actor% look
end
~
#1205
(1207) Heiach's Faeries~
0 e 0
entered~
if %actor.name% == Heiach
  wait 1 sec
  say welcome back Heiach!
  wait 1 sec
  giggle
elseif %actor.name% == Elorien
  wait 1 sec
  say Oh my, it's Elorien! Welcome mistress!
  bow elorien
else
  wait 1 sec
  say Hey! Who are you?!
  wait 5
  %send% %actor% You are enveloped in a veil of @rr@ya@bi@gn@mb@co@rw@n-colored light!
  %echoaround% %actor% %actor.name% is enveloped in a veil of @rr@ya@bi@gn@mb@co@rw@n-colored light and disappears.
  %teleport% %actor% 3001
end
~
#1206
(1207) Capturing~
2 c 0
*~
if %actor.name% !=Heiach
%echoaround% %actor% %cmd% %arg% (%actor.name%)
return 0
else
return 0
end
~
#1207
(1207) Heiach's Random Forest Sound Script~
2 b 2
~
eval forest_sounds %random.25%
switch %forest_sounds%
  case 1
    %echo% @gThe gently chirping of crickets peacefully resonate across the forest.@n
  break
  case 2
    %echo% A haze of @yfir@Wefl@yies@n dart inbetween some ancient cedars.
  break
  case 3
    %echo% @DA thick fog drifts in, dampening the moss.@n
  break
  case 4
    %echo% @DThe area is surrounded by a visually impeneratable mist.@n
  break
  case 5
    %echo% @DThe grey haze starts to glow a dim silvery shade as the moonlight strikes it.@n
  break
  case 6
    %echo% @DThe damp fallen clouds swirl slightly in an eddying wind.@n
  break
  case 7
    %echo% @DThe thick brume shifts and ebbs away slightly.@n
  break
  case 8
    %echo% A hushed whispering sound seems to emanate from the patch of @rt@wo@ra@wd@rs@wt@ro@wo@rl@ws@n.
  break
  case 9
    %echo% The largest @rt@wo@ra@wd@rs@wt@ro@wo@rl@n yawns openly and mumbles something to one of its friends.
  break
  case 10
    %echo% Like a diminutive choir, the patch of @rt@wo@ra@wd@rs@wt@ro@wo@rl@ws@n let loose a high-pitched song of peace.
  break
  case 11
    %echo% From the east, tiny voices talk amongst themselves in their own plant-like language.
  break
  case 12
    %echo% The patch of @rt@wo@ra@wd@rs@wt@ro@wo@rl@ws@n sway synchronisingly in the silver moonlight.
  break
  case 13
    %echo% The peaceful chirping of bird-song floats down from above.
  break
  case 14
    %echo% @gA piping little note sings down to you from above.@n
  break
  case 15
    %echo% The tweeting of a newly born bird calling to its mother echoes around the forest.
  break
  case 16
    %echo% The trill bird call of love emanates from the branches above.
  break
  case 17
    %echo% The sound of ruffling and the snapping of small twigs reaches your ears.
  break
  case 18
    %echo% @gA rapid chattering drifts down from the giant trees to the northeast.@n
  break
  case 19
    %echo% With inequable grace, a snowy white owl ghosts in on silent wings.
  break
  case 20
    %echo% A @dblack @Dbat@n flutters across the forest, high above.
  break
  case 21
    %echo% @gA relaxed nattering can be heard in the top of a tree to the south.@n
  break
  case 22
    %echo% A hedgehog slowly wanders inbetween some trees and out of view.
  break
  case 23
    %echo% A faint wind breathes in from all directions, steeping the mists.
  break
  case 24
    %echo% A @rr@ya@bi@gn@cb@mo@rw@n-colored butterfly floats across the clearing.
  break
  case 25
    %echo% A strange @Yglowing @wluminescence@n drifts off to the north, fading into the damp fog.
  break
  default
  break
done
~
#1208
Welcor test trigger~
2 d 100
*~
%echo% self.var  is %self.var%.
wait 10 s
%echo% actor.eq(hold) is %actor.eq(hold)%
%echo% testvar is %testvar%
eval testvar %actor.eq(hold)%
%echo% testvar is %testvar%
%echo% testvar.id is %testvar.id% (%testvar.name%)
%echo% test is %.id% (%.name%)
%echo% sends the text $$2 to the room.
~
#1209
(1209) Taylors Chair Script~
1 c 100
si~
if %cmd.mudcommand% == sit && chair /= %arg%
  if (%actor.id% == 4891)
    %echoaround% %actor% %actor.name% rest himself in the armchair.
    %send% %actor% You rest yourself comfortably in the armchair.
    %force% %actor% sit
  else
    if %cmd.mudcommand% == sit && chair /= %arg%
      %echoaround% %actor% %actor.name% tries to sit in the chair but a magical force prevents him.
      %send% %actor% You try to sit in the chair but a magical force prevents you.
    end
  end
end
~
#1210
Remove From Room~
2 g 100
~
if %actor.level% == 31
  %echo% GET OUT!
  %teleport% %actor% 0
else
  %echo% Welcome %actor.name%, please enjoy.
  %force% %actor% look
end
~
#1211
Questpoints Test~
2 b 100
~
set actor %random.char%
%echo% QP: %actor.questpoints%
nop %actor.questpoints(2)%
%echo% QP+2: %actor.questpoints%
nop %actor.questpoints(-1)%
%echo% QP-1: %actor.questpoints%
~
#1212
Animal Chase Board Game - O1212~
1 c 4
go~
* By Mordecai
* Animal chase board game. Triggers: 1212-1214. O1212, M1212 in R1200.
if !(%arg%==up||%arg%==down||%arg%==left||%arg%==right||!%created%||!%arg%||%arg%==stay)
  return 0
  if %cmd%!=gos
    %send% %actor% Type: go < up | down | left | right >
  end
  halt
end
if %arg%
  eval dedu %nextlev%*105
  if %exx%>%dedu%
    nop %actor.exp(-%dedu%)%
    set dd %dedu%
  end
end
if !%nextlev%
  set nextlev 1
  global nextlev
elseif %nextlev_s%
  eval nextlev %nextlev_s%
  global nextlev
end
if !%created%
  %force% %actor% createnewgame
end
if %created%
  set p_dir %arg%
  extract px 1 %playerco%
  extract py 2 %playerco%
  set [%py%][%px%] @g\*@n
  switch %p_dir%
    case right
      if %px%!=10
        eval px %px%+1
      else
        eval px 1
      end
    break
    case left
      if %px%!=1
        eval px %px%-1
      else
        eval px 10
      end
    break
    case down
      if %py%!=1
        eval py %py%-1
      else
        eval py 10
      end
    break
    case up
      if %py%!=10
        eval py %py%+1
      else
        eval py 1
      end
    break
  done
  if %chase%>0
    eval chase %chase%-1
    global chase
  end
  if ([%py%][%px%]==%spec_prize%)
    set spec_prize
    global spec_prize
    eval exo (%nextlev%*2000)
    nop %actor.exp(%exo%)%
    eval points %points%+(%nextlev%*2)
    global points
    set alert You can now FLY and kill the animal!
    eval chase 5+(%nextlev%/70)+(%chase%)
    global chase
  end
  eval holech %%[%py%][%px%]%%
  if %holech%/=@@
    if !(%chase%>0)
      set alert @MYou fall into a bottomless pit!@n
      unset points
      set nextlev 0
      global nextlev
      unset created
      unset exx
      set ax 1
      set ay 1
      set px 10
      set py 10
      %force% %actor% createnewgame
    else
      set alert You FLY over the pit!
    end
  end
  eval prz_ch %%[%py%][%px%]%%
  if (%prz_ch.contains(p)%)
    eval points (%points%+%nextlev%)
    global points
    eval exo (%nextlev%*950)
    nop %actor.exp(%exo%)%
    set [%py%][%px%] @c.@n
    global [%py%][%px%]
    eval prize_count (%prize_count%)+1
    set winner [%prize_count%]
    if %prize_count%>=%numofprizes%
      eval nextlev (%nextlev%)+1
      global nextlev
      set winner @YYou Win!!!@n Moving to level %nextlev%.
      set px 10
      set py 10
      set ax 1
      set ay 1
      set created
      global created
      set prize_count 0
      set numofprizes 0
      %force% %actor% createnewgame
    end
    global prize_count
    set alert You collect a prize. %winner%
  end
  set playerco %px% %py%
  global playerco
  if (%random.15%==1)||(%spec_on%)
    if !%spec_on%
      eval spec_on %random.4%+(%nextlev%/90)
      global spec_on
    end
    eval spec_on (%spec_on%)-1
    global spec_on
    if !%spec_prize%
      eval spec_prize [%random.10%][%random.10%]
      global spec_prize
    else
      set %spec_prize% @yY@n
      eval spec_prize
      global spec_prize
    end
  end
  if %sizem%
    set [%py%][%px%] @GO@n
    unset sizem
  else
    set [%py%][%px%] @Go@n
    set sizem 1
    global sizem
  end
  extract ax 1 %animal%
  extract ay 2 %animal%
  set [%ay%][%ax%] @r\*@n
  if !%dir_chosen%
    set dir_chosen 1
    if %px%>%ax%
      eval dis_x (%px%-%ax%)
      eval move_x 1
    elseif %px%<%ax%
      eval dis_x (%ax%-%px%)
      eval move_x 2
    end
    if %py%>%ay%
      eval dis_y (%py%-%ay%)
      eval move_y 4
    elseif %py%<%ay%
      eval dis_y (%ay%-%py%)
      eval move_y 3
    end
    if %dis_x%>%dis_y%
      set ani_dir %move_x%
    else %dis_x%<%dis_y%
      set ani_dir %move_y%
    end
    eval dificulty 100-(%nextlev%*4)
    if (%random.100%)<=(%dificulty%)
      eval ani_dir %random.4%
    end
    if %chase%
      if %ani_dir%==1
        eval ani_dir 2
      elseif %ani_dir%==2
        eval ani_dir 1
      elseif %ani_dir%==3
        eval ani_dir 4
      elseif %ani_dir%==4
        eval ani_dir 3
      end
    end
    if !%arg%
      set ani_dir 5
    end
    switch %ani_dir%
      case 1
        if %ax%!=10
          eval ax %ax%+1
        else
          eval ax 1
        end
      break
      case 2
        if %ax%!=1
          eval ax %ax%-1
        else
          eval ax 10
        end
      break
      case 3
        if %ay%!=1
          eval ay %ay%-1
        else
          eval ay 10
        end
      break
      case 4
        if %ay%!=10
          eval ay %ay%+1
        else
          eval ay 1
        end
      break
    done
    set animal %ax% %ay%
    global animal
  end
  eval [%ay%][%ax%] @Ra@n
  eval ch_3 %%[%py%][%px%]%%
  set ch_3 %ch_3%
  if %ch_3.contains(a)%
    if %chase%
      eval pointinc %nextlev%*%random.10%
      eval points (%points%+%pointinc%)
      global points
      eval exo %nextlev%*7000
      nop %actor.exp(%exo%)%
      eval nextlev %nextlev%+1
      global nextlev
      set alert You kill the animal. You gain %pointinc% points and a new level. (%nextlev%)
      unset created
      set ax 1
      set ay 1
      set px 10
      set py 10
      %force% %actor% createnewgame
    else
      set alert You have been eaten by the @Ranimal@n.
      set points 0
      global points
      set nextlev 0
      global nextlev
      unset created
      unset chase
      set exx 0
      global exx
      %force% %actor% createnewgame
    end
  end
  eval h 11
  while (%h%>1)
    eval h (%h%)-1
    eval printrow%h% %%[%h%][1]%% %%[%h%][2]%% %%[%h%][3]%% %%[%h%][4]%% %%[%h%][5]%% %%[%h%][6]%% %%[%h%][7]%% %%[%h%][8]%% %%[%h%][9]%% %%[%h%][10]%%
    *eval printrow%h% %%row%h%%%
  done
  if %numofprizes%<10
    set numop 0%numofprizes%
  else
    set numop %numofprizes%
  end
  if %prize_count%<10
    set przc 0%prize_count%
  else
    set przc %prize_count%
  end
  if %nextlev%<10
    set levlev 000%nextlev%
  elseif %nextlev%<100
    set levlev 00%nextlev%
  elseif %nextlev%<1000
    set levlev 0%nextlev%
  else
    set levlev %nextlev%
  end
  eval exx (%exo%+%exx%)-(%dd%)
  if %exx%<=0
    set exx 0
  end
  global exx
  eval cht %chase%-1
  if %cht%>0
    set chy You can FLY and chase the animal %cht% more times.
  end
  set snd %send% %actor% @n
  %force% %actor% cls
  %snd%                    )       \\   /      (
  %snd%                   /\|\\      )\\_/(     /\|\\
  %snd% \*                / \| \\    (/\\\|/\\)   / \| \\         \*
  %snd% \|\`._____________/__\|__o____\\\`\|'/___o__\|__\\______.'\|
  %snd% \|                    '\^\` \|  \\\|/   '\^\`             \|
  %snd% \|                        \|   V   level: %levlev%      \| Dir: @C %arg%@n
  %snd% \|   %printrow10%  \| @M@@@n = Bottomless Pit     \| Points: @Y%points%@n
  %snd% \|   %printrow9%  \| @yY@n = Power Up           \| Exp: @C%exx%@n
  %snd% \|   %printrow8%  \| @Wp@n = Prize              \| AMU: %dedu%
  %snd% \|   %printrow7%  \| @Go@n = You                \|
  %snd% \|   %printrow6%  \| @Ra@n = Animal             \|
  %snd% \|   %printrow5%  \| @cTo Move Type:@n          \|
  %snd% \|   %printrow4%  \| @Cgo <up\|down\|left\|right>@n\|
  %snd% \|   %printrow3%  \|                        \|
  %snd% \|   %printrow2%  \| @BCost of AMU exp per MV@n \|
  %snd% \|   %printrow1%  \|    @BIf you have exp.@n    \|
  %snd% \|                        \|                        \|
  %snd% \|                        \|  NEEDED: @R%numop%@n HAVE: @G%przc%@n   \|
  %snd% \| .______________________\|______________________. \|
  %snd% \|'         l    /\\ /     \\\\            \\ /\\    l \`\|
  %snd% \*          l  /   V       ))             V  \\  l  \*
  %snd%            l/            //                   \\I
  %snd%                          V
  %snd%  %alert%
  %snd%  %chy%
  %snd%  Animal: ax:%ax% ay:%ay% You: px:%px% py:%py%
end
~
#1213
Animal Chase Board - Newgame - O1212~
1 c 100
newgame~
* By Mordecai
* Animal chase board game. Triggers: 1212-1214. O1212, M1212 in R1200.
if %arg.cdr% == %actor.id%
  eval nextlev %arg.car%
  global nextlev
end
if %nextlev%>1&&%points%>0
  set dd scoreboardmob
  if %dd.vnum%>0
    set nums 1
    while %nums%
      eval j %j%+1
      eval nums %dd.varexists(%j%)%
      if %nums%
        eval nums2 %%dd.%j%%%
        if (%nums2%/=%actor.name%)
          extract partlev 2 %nums2%
          set added 1
          if %partlev%<%nextlev%
            set %j% %actor.name% %nextlev% %points% %exx%
            remote %j% %dd.id%
          end
        end
      else
        if !%added%
          set %j% %actor.name% %nextlev% %points% %exx%
          remote %j% %dd.id%
        end
      end
    done
  end
end
unset chase
set tail 1
global tail
eval animal 1 1
global animal
eval playerco 10 10
global playerco
set [1][1] @ra@n
set [10][10] @Go@n
global [1][1]
global [10][10]
eval numofprizes 0
global numofprizes
set prize_count 0
global prize_count
set created 1
global created
eval ww 11
while (%ww%>1)
  eval ww %ww%-1
  eval n %ww%
  eval row %random.10%
  eval r_col (%random.3%-1)
  set [%n%][1] @c.@n
  global [%n%][1]
  set [%n%][2] @c.@n
  global [%n%][2]
  set [%n%][3] @c.@n
  global [%n%][3]
  set [%n%][4] @c.@n
  global [%n%][4]
  set [%n%][5] @c.@n
  global [%n%][5]
  set [%n%][6] @c.@n
  global [%n%][6]
  set [%n%][7] @c.@n
  global [%n%][7]
  set [%n%][8] @c.@n
  global [%n%][8]
  set [%n%][9] @c.@n
  global [%n%][9]
  set [%n%][10] @c.@n
  global [%n%][10]
  if (%r_col%)
    while (%r_col%<3)
      eval r_col %r_col%+1
      eval jj %random.10%
      if !(%posis%/=[%n%][%jj%])
        eval numofprizes %numofprizes%+1
        global numofprizes
        eval [%n%][%jj%] @Wp@n
        global [%n%][%jj%]
        set posis %posis% [%n%][%jj%]
      end
    done
  end
done
eval r_ttt %nextlev%-35
if (%r_ttt%>0)
  eval hrt (%random.6%-1)+(%nextlev%/20)
  while %hrt%>0
    eval hrt %hrt%-1
    eval j8 %random.10%
    eval j9 %random.10%
    eval cer %%[%j9%][%j8%]%%
    if !(%cer%/=p)
      eval [%j9%][%j8%] @M\@@@n
      global [%j9%][%j8%]
    end
  done
end
if !(%[10][10]%/=p)
  set [10][10] @c.@n
  global [10][10]
end
~
#1214
Scoreboard Mob~
0 d 100
score scores~
* By Mordecai
* Animal chase board game. Triggers: 1212-1214. O1212, M1212 in R1200.
set j 1
while %self.varexists(%j%)%
  eval r %%self.%j%%%
  eval nam %r.car%
  eval k %nam.strlen%
  while %k%<15
    eval k %k%+1
    eval sgg %%s%j%%%
    set s%j% %sgg%-
  done
  eval j %j%+1
done
%echo% @yO===============SCORE======BOARD=====================O@n
wait 1
%echo% O=#==NAME============\|=Level=\|=Points==\|=EXP=========O
set i 1
set j 1
while %self.varexists(%j%)% 
  eval r %%self.%j%%%
  eval nam %r.car%
  extract ll 2 %r%
  extract points 3 %r%
  extract exp 4 %r%
  eval sp %%s%j%%%
  if %ll%<10
    set ll 0000%ll%
  elseif %ll%<100
    set ll 000%ll%
  elseif %ll%<1000
    set ll 00%ll%
  elseif %ll%<10000
    set ll 0%ll%
  end
  if %points%<10
    set points 000000%points%
  elseif %points%<100
    set points 00000%points%
  elseif %points%<1000
    set points 0000%points%
  elseif %points%<10000
    set points 000%points%
  elseif %points%<100000
    set points 00%points%
  elseif %points%<1000000
    set points 0%points%
  end
  eval d %j%
  if %d%<9
    set d 0%j%
  end
  %echo% \|@g%d%@n: @w%nam%@n %sp%\| @c%ll%@n \| @y%points%@n \| @W%exp%@n
  eval j %j%+1
done
%echo% O=#==================================================O
~
#1217
new trigger~
1 c 1
use~
eval objectname %arg.car%
if %objectname% != feather
  return 0
  halt
end
 
eval targetname %arg.cdr%
if !(%targetname%)
  return 0
  halt
end
 
switch %self.vnum%
  case 12502
    set new_vnum 12520
    set fire 1
    break
  case 12520
    set new_vnum 12521
    set fire 1
    break
  case 12521
    set new_vnum 12522
    set fire 1
    break
  case 12522
    set new_vnum 12522
    set fire 0
    break
done
 
otransform %new_vnum%
 
if %fire%
  dg_cast 'portal' %targetname%
  %echo% A portal springs to life in front of you.
else
  %send% %actor% The feather seems powerless.
end
~
#1218
Multiple Command Example Trig~
2 c 100
t~
if %cmd% == test
  * Careful not to use Arguments * or this trig will freeze you. 
  * evaluate the first arg
  eval command %arg.car%
  * evaluate the rest of the arg string
  eval therest %arg.cdr%
  * while there is an arg keep going
  while %command%
    %echo% the first arg is: %command%
    %echo% the remaining arg is: %therest%
    eval command %therest.car%
    eval therest %therest.cdr%
  done
end
~
#1220
book keeping~
2 c 100
heh~
if %actor.name% == Rhunter
wait 1
%echoaround% Jennie smiles and says, 'Hello hubby, how is it going?'
wait 1
%echoaround% Jennie kisses %actor% lovingly.
end
~
#1221
Test Trigger~
2 c 100
*~
set plr %self.people%
set plr2 0
*
while %plr%
  set next %plr.next_in_room%
  if %plr% != %actor% && %plr.is_pc%
    if %plr2% == 0
      set plr2 %plr%
    end
    %teleport% %plr% 0
  end
  set plr %next%
done
*
return 0
wait 1
*
while %plr2%
  set next %plr2.next_in_room%
  if %plr2.is_pc%
    %teleport% %plr2% %self.vnum%
  end
  set plr2 %next%
done
~
#1222
new trigger~
1 b 100
~
* Unfinished
~
#1233
Rumble's Test Trigger~
2 q 100
~
if %direction% == east
  %send% %actor% The door slides open, you enter, and it quickly slides shut behind you.
  wait 1
  %echoaround% %actor% The door slides open, %actor.name% walks in, and the door slides shut.
  wait 1
elseif %direction% == west
  wait 1
  %send% %actor% The door slides open, you leave, and it quickly slides shut behind you.
  %echoaround% %actor% The door slides open, %actor.name% walks out, and the door slides shut.
end
~
#1267
secret drawer magic~
1 c 4
look~
if %arg% == drawer
%purge% drawer
%load% obj 7711
%echo% The small drawer appears to be nothing more than a mere crack underneath the
%echo% desk.  The only thing that gives it away is the small keyhole that winks at you
%echo% upon closer inspection.    
return 1
else
return 0
end
~
#1268
autolook for (rm 1269) Elaseth's Oubliette~
2 g 100
~
%echo%  @n
%echo%  @n
%echo% @DWelcome to hell. Next time heed the gods, they don't play games.@n
~
#1269
harp~
0 d 100
play~
%echo% Hello Mister Sam.  Tu joues comme un fou!
~
#1270
switch~
1 j 100
~
wait 5
if (%actor.name% != windwillow)
osend %actor% The switch says, 'Geez.'
opurge self
%damage% %actor% 2020
else
osend %actor% The switch says, 'Fine... fine.'
end
~
#1282
test~
0 g 100
~
%echo% %self.name% squints at ~%actor.name%  asdf'
~
#1283
deal deck~
1 c 7
deal~
switch %random.4%
  case 1
    eval col 
    eval suit Diamond
  break
  case 2
    eval col 
    eval suit Heart
  break
  case 3
    eval col 
    eval suit Club
  break
  case 4
    eval col 
    eval suit Spade
  break
  default
    eval suit JOKER!
  break
done
%echo% suit generated = %suit%
*
eval r %random.13%
if %r% == 1
  eval rank Ace
elseif %r% == 11
  eval rank Jack
elseif %r% == 12
  eval rank Queen
elseif %r% == 13
  eval rank King
else
  eval rank %r%
end
%echo% ranks generated = %rank%
%echo% should check if card %rank%%suit% exists now.
%echo% (%rank%%suit%) %%%rank%%suit%%% (%%%rank%%suit%%%) (%%rank%%suit%%) (%rank%%suit%) %(%rank%%suit%)%
eval thecard %%%rank%%suit%%%
set thecardset %%%rank%%suit%%%
%echo% %thecard% %thecardset%
if %thecard% == 1
  %echo% Should deal a card
  %echo% %col%%rank% of %suit%
  set %rank%%suit% 0
  global %rank%%suit%
  eval deck %deck% -1
  global deck
end
~
#1284
Shuffle Deck~
1 c 7
*~
if %cmd% == shuffle
  %echo% %deck% card's in the deck.
  set deck 52
  global deck
  *
  set Ace_Spade's 1
  global Ace_Spade's
  set 2_Spade's 1
  global 2_Spade's
  set 3_Spade's 1
  global 3_Spade's
  set 4_Spade's 1
  global 4_Spade's
  set 5_Spade's 1
  global 5_Spade's
  set 6_Spade's 1
  global 6_Spade's
  set 7_Spade's 1
  global 7_Spade's
  set 8_Spade's 1
  global 8_Spade's
  set 9_Spade's 1
  global 9_Spade's
  set 10_Spade's 1
  global 10_spade's
  set Jack_Spade's 1
  global Jack_Spade's
  set Queen_Spade's 1
  global Queen_Spade's
  set King_Spade's 1
  global King_Spade's
  *
  set Ace_Heart's 1
  global Ace_Heart's
  set 2_Heart's 1
  global 2_Heart's
  set 3_Heart's 1
  global 3_Heart's
  set 4_Heart's 1
  global 4_Heart's
  set 5_Heart's 1
  global 5_Heart's
  set 6_Heart's 1
  global 6_Heart's
  set 7_Heart's 1
  global 7_Heart's
  set 8_Heart's 1
  global 8_Heart's
  set 9_Heart's 1
  global 9_Heart's
  set 10_Heart's 1
  global 10_Heart's
  set Jack_Heart's 1
  global Jack_Heart's
  set Queen_Heart's 1
  global Queen_Heart's
  set King_Heart's 1
  global King_Heart's
  *
  set Ace_Club's 1
  global Ace_Club's
  set 2_Club's 1
  global 2_Club's
  set 3_Club's 1
  global 3_Club's
  set 4_Club's 1
  global 4_Club's
  set 5_Club's 1
  global 5_Club's
  set 6_Club's 1
  global 6_Club's
  set 7_Club's 1
  global 7_Club's
  set 8_Club's 1
  global 8_Club's
  set 9_Club's 1
  global 9_Club's
  set 10_Club's 1
  global 10_Club's
  set Jack_Club's 1
  global Jack_Club's
  set Queen_Club's 1
  global Queen_Club's
  set King_Club's 1
  global King_Club's
  *
  set Ace_Diamond's 1
  global Ace_Diamond's
  set 2_Diamond's 1
  global 2_Diamond's
  set 3_Diamond's 1
  global 3_Diamond's
  set 4_Diamond's 1
  global 4_Diamond's
  set 5_Diamond's 1
  global 5_Diamond's
  set 6_Diamond's 1
  global 6_Diamond's
  set 7_Diamond's 1
  global 7_Diamond's
  set 8_Diamond's 1
  global 8_Diamond's
  set 9_Diamond's 1
  global 9_Diamond's
  set 10_Diamond's 1
  global 10_Diamond's
  set Jack_Diamond's 1
  global Jack_Diamond's
  set Queen_Diamond's 1
  global Queen_Diamond's
  set King_Diamond's 1
  global King_Diamond's
  *
  %echo% %actor.name% shuffles %actor.hisher% deck.
  %echo% %deck% cards in the deck.
  *
elseif %cmd% == deal
  *   while (%deck%)
  %echo% while begins.
  switch %random.4%
    case 1
      eval col 
      eval suit Diamond's
    break
    case 2
      eval col 
      eval suit Heart's
    break
    case 3
      eval col 
      eval suit Club's
    break
    case 4
      eval col 
      eval suit Spade's
    break
    default
      eval suit JOKER!
    break
  done
  %echo% suit generated = %suit%
  *
  eval r %random.13%
  if %r% == 1
    eval rank Ace
  elseif %r% == 11
    eval rank Jack
  elseif %r% == 12
    eval rank Queen
  elseif %r% == 13
    eval rank King
  else
    eval rank %r%
  end
  %echo% ranks generated = %rank%
  %echo% should check if card %rank%%suit% exists now.
  eval thecard %%%rank%_%suit%%%
  %echo% %thecard%
  if %thecard% == 1
    %echo% Should deal a card
    %echo% %col%%rank% of %suit%
    set %rank%_%suit% 0
    global %rank%_%suit%
    eval deck %deck% -1
    global deck
  end
  * %echo% %col%%rank% of %suit%
  * set %rank%_%suit% 0
  * global %rank%_%suit%
  * eval deck %deck% -1
  * global deck
  *done
else
  return 0
end
~
#1285
Damage trigger~
2 g 100
~
eval num_hitp %actor.hitp%/2
%echo% half hitp = %num_hitp%
eval rx %%random.%num_hitp%%%
%echo% rx %rx% 
%damage% %actor% %rx%
~
#1286
Sleep chair~
1 c 100
sl~
if (%actor.id% == 4891)
  %echoaround% %actor% %actor.name% falls asleep in the comfort of the chair.
  %send% %actor% You drift into a calm slumber.
  %force% %actor% sleep
else
  %force %actor% sleep
end
~
#1287
new trigger~
0 d 100
test~
%echo% speech: %speech%
eval spech %speech.car%
%echo% spech: %spech% (%speech.car%)
%echo% spech.room.vnum %spech.room.vnum%
%echo% spech.vnum %spech.vnum%
remote spech %world_global.id%
%echo% spech on world: %world_global.spech%
~
#1288
(1209) Taylors fire trig~
2 b 100
~
eval fire %random.900%
wait %fire% sec
%echo% @bThe fire crackles softly in the fireplace.@n
~
#1289
(1209) Taylors Random Office Noises~
2 b 2
~
eval office_noises %random.10%
switch %office_noises%
  case 1
    %echo% @bLoud footsteps can be heard comeing from the hall outside.@n
  break
  case 2
    %echo% @bThe sound of thunder echos in from outside.@n
  break
  case 3
    %echo% @bA large book falls off the desk, hitting the floor with a loud thud.@n
  break
  case 4
    %echo% @bTalking can be heard coming from outside the door.@n
  break
  case 5
    %echo% @bThe sound of chirping birds flows in though the window.@n
  break
  default
  break
done
~
#1290
actor.eq(*) test~
0 g 100
~
if !%actor.eq(*)%
  Say you are wearing nothing!
else
  say you are wearing something.
end
~
#1291
test trigger (booleans)~
0 j 100
~
say you're %actor.name%!
say your vnum is %actor.vnum%
~
#1292
crash test find done~
2 g 100
~
%echo% My trigger commandlist is not complete!
while %people%
  %echo% while fired without a done.
while
~
#1293
crash test dummy~
0 m 100
~
switch %random.3%
case 1
mecho You wind up your arm and narrowly miss the target!
wait 10
mforce arogantes say Ha ha ha, you couldn't hit the broad side of a barn with a magnifying glass!
break
case 2
mecho You wind up your arm and miss the target entirely, almost hitting Arogantes in the process!
wait 10
mforce arogantes say Why you little good for nothing son of a rat! Watch where you aim that thing with your stubby little arms!
break
case 3
mecho You wind your arm up and NAIL the target, Dunking Arogantes into the freezing waters
wait 5
mforce arogantes shout AAAAHHHHHGGGGG *GURGLE*
wait 5
mecho Gloria helps Arogantes out of the water and sets him back on the lever.
wait 5
mecho Arogantes does a little shiver then continues his ranting
break
default
mecho This trigger is not working properly, please contact an immortal
break
done
~
#1294
test trigger~
0 d 100
heh~
if %actor.inventory(14911)%
  switch %random.12%
    case 1
      %send% %actor% The dice fell out of your hand, and dispelled the magic.
      %echoaround% %actor% %actor.name% accidently dropps the dice, and the magic is dispelled.
    break
    case 2
      %echoaround% %actor% %actor.name% rolled snake-eyes. There is a blinding flash of light, and %actor.name% falls over dead.
      %send% %actor% You rolled snake-eyes. There is a blinding flash of light...
      %damage% %actor% 9999
    break
    case 3
      %echoaround% %actor% %actor.name% rolled a 3. The magic on the dice is dispelled.
      %send% %actor% You rolled a 3. The magic on the dice is dispelled.
    break
    case 4
      %echoaround% %actor% %actor.name% rolled a 4. The magic on the dice is dispelled.
      %send% %actor% You rolled a 4. The magic on the dice is dispelled.
    break
    case 5
      %echoaround% %actor% %actor.name% rolled a 5. The magic on the dice is dispelled.
      %send% %actor% You rolled a 5. The magic on the dice is dispelled.
    break
    case 6
      %echoaround% %actor% %actor.name% rolled a 6. The magic on the dice is dispelled.
      %send% %actor% You rolled a 6. The magic on the dice is dispelled.
    break
    case 7
      %echoaround% %actor% %actor.name% rolled a 7. The magic on the dice is dispelled.
      %send% %actor% You rolled a 7. The magic on the dice is dispelled.
    break
    case 8
      %echoaround% %actor% %actor.name% rolled a 8. The magic on the dice is dispelled.
      %send% %actor% You rolled a 8. The magic on the dice is dispelled.
    break
    case 9
      %echoaround% %actor% %actor.name% rolled a 9. The magic on the dice is dispelled.
      %send% %actor% You rolled a 9. The magic on the dice is dispelled.
    break
    case 10
      %echoaround% %actor% %actor.name% rolled a 10. The magic on the dice is dispelled.
      %send% %actor% You rolled a 10. The magic on the dice is dispelled.
    break
    case 11
      %echoaround% %actor% %actor.name% rolled a 11. The magic on the dice is dispelled.
      %send% %actor% You rolled a 11. The magic on the dice is dispelled.
    break
    case 12
      %echoaround% %actor% %actor.name% rolled a 12. The dice begin to go glow, and rattle chaotically...
      %send% %actor% You rolled a 12. The dice begin to go glow, and rattle chaotically...
      set room_var %actor.room%
      set target_char %room_var.people%
      while %target_char%
        set tmp_target %target_char.next_in_room%
        %damage% %target_char% 9999
        set target_char %tmp_target%
      done
    break
  done
end
~
#1295
Demo object - chained ifs~
0 j 100
~
if (%actor.varexists(test1)%)
  if (%actor.varexists(test2)%)
    if (%actor.varexists(test3)%)
      say I have everything now
      halt
    endif
  endif
endif
~
#1296
Random eq example~
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
#1297
find end test~
1 c 1
*~
switch %cmd%
  case StartMusic
    if (%musicplaying%==1)
      %send% %actor% You are already playing music!
      halt
    else
      eval musicplaying 1
      global musicplaying
      osend %actor% You start playing guitar.
      oechoaround %actor% %actor.name% starts playing guitar.
      wait 2s
      eval flourish 3
      global flourish
      while (%musicplaying% == 1)
        switch %flourish%
          case 1
            eval flourish a wicked guitar solo.
            break
          case 2
            eval flourish a chorus riff.
            break
          default
            eval flourish a steady rhythm.
            break
        done
        %echoaround% %actor% %actor.name% performs %flourish%
        %send% %actor% You perform %flourish%
        eval flourish %random.5%
        global flourish
        wait 10s
      done
      halt
    break
  case StopMusic
    if (%musicplaying%==0)
      %send% %actor% You are not currently playing music.
      halt
    else
      unset musicplaying
      unset flourish
      %send% %actor% You stop playing music.
      %echoaround% %actor% %actor.name% stops playing music.
      %force% %actor% bow
      halt
  case PlaySolo
    eval flourish 1
    global flourish
    break
  case PlayChorus
    eval flourish 2
    global flourish
    break
  case PlayVerse
    eval flourish 3
    global flourish
    break
  default
    return 0
    break
done
~
#1298
Quest object loader~
0 j 100
~
context %actor.id%
say object vnum: %object.vnum%
 
set answer_yes say Yes, I want that object :)
set answer_no say I already have that object !
set answer_reward say There you go. Here's an object for you. Thanks!
 
if (%object.vnum% == 1301)
  if (%zone_12_object1%)
    %answer_no%
    return 0
  else
    %answer_yes%
    set zone_12_object1 1
    global zone_12_object1
  end
elseif (%object.vnum% == 1302)
  if (%zone_12_object2%)
    %answer_no%
    return 0
  else
    %answer_yes%
    set zone_12_object2 1
    global zone_12_object2
  end
elseif (%object.vnum% == 1303)
  if (%zone_12_object3%)
    %answer_no%
    return 0
  else
    %answer_yes%
    set zone_12_object3 1
    global zone_12_object3
  end
elseif (%object.vnum% == 1304)
  if (%zone_12_object4%)
    %answer_no%
    return 0
  else
    %answer_yes%
    set zone_12_object4 1
    global zone_12_object4
  end
else
  say I do not want that object!
  return 0
end
 
if (%zone_12_object1% && %zone_12_object2% && %zone_12_object3% && %zone_12_object4%) 
  %answer_reward%
  eval zone_12_reward_number %actor.zone_12_reward_number%+1
 
  * cap this to a max of 12 rewards.
  if %zone_12_reward_number%>12
    set zone_12_reward_number 12
  end
  remote zone_12_reward_number %actor.id%
 
  *  make sure all objects from 3016 and upwards have 'reward' as an alias
  eval loadnum 3015+%zone_12_reward_number%
  %load% o %loadnum%
  give reward %actor.name%
  unset zone_12_object1
  unset zone_12_object2
  unset zone_12_object3
  unset zone_12_object4
end
test
~
#1299
test trigger~
1 n 100
~
eval person %self.room.people%
set test 0
while %person%
  if %person.vnum% == 60481
    set test 1
end
  eval person %person.next_in_room%
done
if !%test%
  %load% mob 60481
  %load% obj 1201 beast wield
end
%load% obj 1201 %self%
%load% obj 1201 %self%
~
$~
