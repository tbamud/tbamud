#11800
(01) shopwoman greets~
0 gh 100
~
set gender one
if %actor.sex% == male
set gender lad
elseif %actor.sex% == female
set gender lady
end
wait 2 s
emote looks up and peers at you keenly as she smiles in greeting.
wait 2 s
say Why hello little %gender%.
wait 2 s
say Have a look around if it pleases you.
wait 2 s
say I hope you find what you are looking for.
wait 2 s
emote smiles strangely at you as she turns back to her work.
~
#11801
(01) shopwoman comments on looking~
0 c 100
l~
return 0
wait 1 s
if %cmd.mudcommand% == look && dollhouse /= %arg% && %arg%
tell %actor.name% You like that do you?
wait 1 s
tell %actor.name% A piece worth examining closer if you want my opinion.
end
~
#11802
(06) shopwoman comments on examination~
0 c 100
exa~
return 0
if %cmd.mudcommand% == examine && dollhouse /= %arg% && %arg%
wait 1 s
tell %actor.name% Aye, its a magnificent little toy that.
wait 1 s
tell %actor.name% Shame about that little doll that came with it.
end
~
#11803
(09) doll teleports when taken~
1 g 100
~
if %actor.room.vnum% == 11802
%echoaround% %actor% %actor.name% takes something from the dollhouse and seems to fade mysteriously away, leaving no trace.
%teleport% %actor% 11801
wait 2 s
%send% %actor% You feel a little strange as everything starts to blur, shifting and changing shape until you are completely disorientated.
wait 2 s
%force% %actor% look
%force% %actor% xx
end
~
#11804
(00) closed-eye girl greets~
0 c 100
xx~
wait 15 s
set gender shadow-creature
if %actor.sex% == male
  set gender shadow-man
elseif %actor.sex% == female
  set gender shadow-woman
end
if %actor.varexists(zn118_gravedone)%
  say Why %gender%, I believe you have passed through here before.
  wait 3 s
  say Did you really want to return?
  wait 3 s
  say Be aware that if you do, the cycle will begin again.
  wait 2 s
  say The words of the journal will be wiped clean.
  wait 3 s
else
  say What brings you to this private place %gender%?
  wait 3 s
  say There are answers here... but few care for the questions that invoke them.
  wait 3 s
end
%load% obj 11805
give eye %actor.name%
drop eye
wait 2 s
say The choice is yours...
wait 2 s
say Close it, and I shall send you back to your world...
wait 2 s
say Open it, and I shall send you to mine.
~
#11805
(05) opening eye teleports in~
1 c 3
o~
if %cmd.mudcommand% == open && eye /= %arg% && %arg%
  if %actor.varexists(zn118_knifestart)%
    rdelete zn118_knifestart %actor.id%
  end
  if %actor.varexists(zn118_gravedone)%
    rdelete zn118_gravedone %actor.id%
  end
  %teleport% %actor% 11822
  %send% %actor% You open the eye.
  wait 1 s
  %send% %actor% A hazy mist swirls up and around you, blurring your vision for a second before clearing away.
  wait 1 s
  %force% %actor% look
  wait 1 s
  %send% %actor% The voice of a young girl whispers: Please recover what I left there.
  wait 1 s
  %load% obj 11807
  %send% %actor% A leather-strapped journal suddenly materializes on the ground.
  wait 1 s
  %send% %actor% The voice of a young girl whispers: Give the journal to the ones you meet there, they will know what to do with it.
  %purge% self
else
  return 0
end
~
#11806
(05) closing eye teleports out~
1 c 3
c~
if %cmd.mudcommand% == close && eye /= %arg% && %arg%
%teleport% %actor% 11802
%echoaround% %actor% %actor.name% suddenly appears in a haze of mist.
%send% %actor% You close the eye.
wait 1 s
%send% %actor% A hazy mist swirls up and around you, blurring your vision for a second before clearing away. 
wait 1 s
%force% %actor% look
%purge% %self%
else
return 0
end
~
#11807
(04) 4-yr-old gives different messages~
0 c 100
xx118xx~
eval room %self.room%
if (%room.vnum% == 11804)
  if !(%actor.varexists(zn118_a)%)
    emote walks into the room and peers at you.
    wait 2 s
    say You cant talk to the shadow-ones, they're not real...
    wait 2 s
    say at least not any more.
    wait 2 s
    say The others here aren't real either...
    wait 2 s
    say but then they never were.
    wait 1 s
    emote giggles as she runs off.
    set zn118_a 1
    remote zn118_a %actor.id%
  end
end
drop journal
%purge% %self%
~
#11808
4-yr-old loads on entry~
2 g 100
~
wait 1 s
%load% mob 11804
%force% %actor% xx118xx
~
#11809
(13) bird appears when taken~
1 g 100
~
set zn118_birdquest 1
remote zn118_birdquest %actor.id%
%load% mob 11805
%send% %actor% The little bird squirms free from your hand and flies joyfully to the higher parts of the room.
%purge% %self%
~
#11810
crunch~
1 s 100
~
if %self.vnum% == 11816
%echo% GLUG GLUG GLUG...
elseif %self.vnum% == 11817
%echo% CRUNCH CRUNCH CRUNCH...
elseif %self.vnum% == 11818
%echo% CRUNCH CRUNCH CRUNCH...
end
~
#11811
(39) grave quest~
0 j 100
~
if %object.vnum% == 11807
  wait 1 s
  set num 1
  if %actor.varexists(zn118_crayondone)%
    eval num %num% + 1
    if %actor.varexists(zn118_birddone)%
      eval num %num% + 1
      if %actor.varexists(zn118_ridleydone)% 
        eval num %num% + 1
        if %actor.varexists(zn118_knifedone)%
          eval num %num% + 1
          if %actor.varexists(zn118_tunneldone)%
            eval num %num% + 1 
            if %actor.varexists(zn118_ruthdone)%
              eval num %num% + 1
              if %actor.varexists(zn118_thindone)%
                eval num %num% + 1
                if %actor.varexists(zn118_mutedone)%
                  eval num %num% + 1
                  if %actor.varexists(zn118_runningdone)%
                    eval num %num% + 1
                    if %actor.varexists(zn118_angrydone)%
                      eval num %num% + 1
                      if %actor.varexists(zn118_blinddone)%
                        eval num %num% + 1
                        if %actor.varexists(zn118_weepingdone)%
                          eval num %num% + 1
                          if %actor.varexists(zn118_scarreddone)%
                            eval num %num% + 1
                          end
                        end
                      end
                    end
                  end
                end
              end
            end
          end
        end
      end
    end
  end
  if %num% == 14
    say Well done %actor.name%, all the words of this place have been captured.
    wait 1 s
    say And so the speakers need linger no more.
    wait 2 s
    say A terrible time... and a terrible story...
    wait 1 s
    emote brushes her fingers over the pages of the journal.
    wait 2 s
    say And now it passes into your keeping, as it has passed out of mine.
    wait 2 s
    say My time is over %actor.name%, now I must be destroyed, and my bones buried here... in this ruined garden.
    wait 3 s
    say The task falls upon you to finish this, bury me here, and I shall seal this quest.
    set zn118_gravequest 1
    remote zn118_gravequest %actor.id%
    rdelete zn118_crayondone %actor.id%
    rdelete zn118_birddone %actor.id%
    rdelete zn118_ridleydone %actor.id%
    rdelete zn118_knifedone %actor.id%
    rdelete zn118_tunneldone %actor.id%
    rdelete zn118_ruthdone %actor.id%
    rdelete zn118_thindone %actor.id%
    rdelete zn118_mutedone %actor.id%
    rdelete zn118_runningdone %actor.id%
    rdelete zn118_angrydone %actor.id%
    rdelete zn118_blinddone %actor.id%
    rdelete zn118_weepingdone %actor.id%
    rdelete zn118_scarreddone %actor.id%
    give journal %actor.name%
    drop journal
  elseif %actor.varexists(zn118_gravequest)%
    wait 1 s
    say Only one thing remains %actor.name%.
    wait 1 s
    say Kill me, and bury my corpse here in this garden.
    wait 1 s
    give journal %actor.name%
    drop journal
  else
    wait 1 s
    say I have been waiting for one such as you, %actor.name%.
    wait 2 s
    say But I am the last of all you must see here.
    wait 1 s
    say Return to me when the others have told their tales.
    wait 1 s
    give journal %actor.name%
    drop journal
  end
else
  say I do not think this is meant for me.
  give %object.name.car% %actor.name%
  drop %object.name.car%
end
~
#11812
pinball - shoot~
1 c 7
shoot~
* No Script
~
#11813
random messages~
2 b 40
~
switch %random.8%
  case 0
    %echo% Birds chirp serenly from the swaying branches of surrounding trees.
  break
  case 1
  %echo% A sudden chill breathes harshly down your neck.
  break
  case 2
%echo% A soft glow lights the place as a large spark flares and dies.
  break
  case 3
 %echo% The sound of gentle laughter echoes from nearby mountains.
  break
  case 4
%echo% An anguished scream pierces the air.
  break
  case 5
%echo% The leaves shiver uneasily as something invisible stirs the air.
  break
  case 6
 %echo% The warm scent of fresh baking bread wafts on the air.
  break
case 7
%echo% The soothing sound of a mother's voice can be heard singing gently.
break
case 8
%echo% The air grows suddenly still as though this place was holding its breath.
break
  default
%echo% A soft glow lights the place as a large spark flares and dies.
  break
done
~
#11814
(02) colouring message~
0 gh 100
~
if %actor.is_pc%
  if !%actor.varexists(zn118_b)%
    wait 1 s
    emote looks up and smiles at you.
    wait 1 s
    say I'm drawing monsters...
    wait 1 s
    say but I need my red crayon...
    wait 1 s
    say they took it away.
    wait 1 s
    emote sighs and goes back to her drawing.
    set zn118_b 1
    remote zn118_b %actor.id%
  end
end
~
#11815
shadow mobs won't take journal~
0 j 100
~
  return 0
  %send% %actor% %self.name% is not even aware of you.
~
#11816
certain mobs won't take journal~
0 j 100
~
if %object.vnum% == 11807
  %send% %actor% %self.name% can't take %object.shortdesc%.
  return 0
end
~
#11817
incorrect mobs refuse journal~
0 j 100
~
  wait 1 s
  emote shrugs uncertainly.
  wait 1 s
  say Somehow I don't think this is meant for me.
drop %object.name.car%
~
#11818
(02) crayon child quest~
0 j 100
~
if %object.vnum% == 11807
  if %actor.varexists(zn118_crayondone)%
    wait 1 s
    say I have already written in that for you.
    give journal %actor.name%
    drop journal
  elseif %actor.varexists(zn118_crayonquest)%
    wait 1 s
    say Ah! Now, let me see...
    wait 1 s
    emote scribbles something into a leather-strapped journal.
    wait 1 s
    give journal %actor.name%
    drop journal
    nop %actor.exp(10000)%
    smile
    rdelete zn118_crayonquest %actor.id%
    set zn118_crayondone 1
    remote zn118_crayondone %actor.id%
  else
    wait 1 s
    say I would like to write something in that for you...
    wait 1 s
    say but I need my red crayon.
    wait 1 s
    give journal %actor.name%
    drop journal
  end
elseif %object.vnum% == 11808
  set zn118_crayonquest 1
  remote zn118_crayonquest %actor.id%
  wait 1 s
  smile
  wait 1 s
  hold crayon
  wait 1 s
  say Now I have my crayon I can write for you.
  wait 1 s
  smile %actor.name%
else
  wait 1 s
  say I don't think this is meant for me.
  give %object.name.car% %actor.name%
  drop %object.name.car%
end
~
#11819
(71) bloodstone amulet awaken~
1 c 1
awaken~
%send% %actor% You wrap your fingers around the bloodstone amulet and focus deeply.
%echoaround% %actor% %actor.name% wraps %actor.hisher% fingers around %actor.hisher% bloodstone amulet and closes %actor.hisher% eyes in concentration.
if %actor.varexists(zn118_focus_return)%
  eval place %actor.zn118_focus_return%
  wait 2 s
  %send% %actor% The amulet glows brightly, heating in your grasp and flickering with wisps of magic.
  %echoaround% %actor% The amulet glows brightly, flickering with wisps of magic as %actor.name% grasps it.
  wait 2 s
  %teleport% %actor% %place%
  rdelete zn118_focus_return %actor.id%
  %send% %actor% The amulet amplifies your focus, helping you re-emerge into reality.
  %echoaround% %actor% %actor.name% suddenly flickers into existance.
else
  wait 2 s
  %send% %actor% You are already fully awake to reality.
end
~
#11820
(09) bird quest~
0 j 100
~
if %object.vnum% == 11807
  if %actor.varexists(zn118_birddone)%
    wait 1 s
    say I have already written in that for you.
    give journal %actor.name%
    drop journal
  elseif %actor.varexists(zn118_birdquest)%
    wait 1 s
    say Thank you %actor.name%. Now part of me feels free too.
    wait 2 s
    emote scribbles something into a leather-strapped journal.
    wait 2 s
    nop %actor.exp(10000)%
    give journal %actor.name%
    drop journal
    wait 1 s
    say That key will also open the middle way.
    rdelete zn118_birdquest %actor.id%
    set zn118_birddone 1
    remote zn118_birddone %actor.id%
  else
    wait 1 s
    say Yes, there is something I could write in that...
    wait 1 s
    say but would you do me a favour first?
    wait 2 s
    say My father keeps a little bird locked away...
    wait 2 s
    say would you release it?
    wait 1 s
    %load% obj 11823
    say Here... Pin will help you do it.
    wait 1 s
    give key %actor.name%
    drop key
    give journal %actor.name%
    drop journal
  end
else
  wait 1 s
  say I don't think this is meant for me.
  drop %object.name.car%
end
~
#11821
(09) playing message~
0 gh 100
~
if %actor.is_pc%
  if !%actor.varexists(zn118_c)%
    wait 1 s
    emote jumps as she realizes she's not alone.
    wait 1 s
    emote laughs.
    wait 1 s
    say Oh hiya...
    wait 2 s
    smile %actor.name%
    wait 1 s
    say Do you play games?
    wait 2 s
    %load% obj 11818
    give carrot %actor.name%
    drop carrot
    set zn118_c 1
    remote zn118_c %actor.id%
  end
end
~
#11822
new trigger~
0 f 100
~
%purge% corpse
~
#11823
test~
1 c 3
read~
* No Script
~
#11824
(17) clapping game 1~
0 d 100
*~
wait 1 s
if %speech% == My mother says...
  say not to play...
elseif %speech% == with the gypsies...
  say in the wood.
elseif %speech% == If I should...
  say she would say...
elseif %speech% == Naughty little child...
  say to disobey!
  wait 1 s
  giggle
else
  return 0
end
~
#11825
(16) clapping game 2~
0 d 100
*~
wait 1 s
if %speech% == not to play...
  say with the gypsies...
elseif %speech% == in the wood.
  say If I should...
elseif %speech% == she would say...
  say Naughty little child...
elseif %speech% == to disobey!
  giggle
else
  return 0
end
~
#11826
(16) random clapping game~
0 b 5
~
say My mother says...
~
#11827
(11) when Ridley receives~
0 j 100
~
if %object.vnum%==11834
  if %actor.varexists(zn118_ridleyquest)%
    eval in_box %object.contents%
    while %in_box%
      set next_in_box %in_box.next_in_list%
      if %in_box.vnum%==11828
        set zn118_fruit 1
        remote zn118_fruit %actor.id%
      elseif %in_box.vnum%==11832
        set zn118_flower 1
        remote zn118_flower %actor.id%
      elseif %in_box.vnum%==11833
        set zn118_flying 1
        remote zn118_flying %actor.id%
      end
      set in_box %next_in_box%
    done
    if %actor.varexists(zn118_fruit)%
      if %actor.varexists(zn118_flower)%
        if %actor.varexists(zn118_flying)%
          rdelete zn118_fruit %actor.id%
          rdelete zn118_flower %actor.id%
          rdelete zn118_flying %actor.id%
          rdelete zn118_ridleyquest %actor.id%
          set zn118_ridleywrite 1
          remote zn118_ridleywrite %actor.id%
        end
      end
    end
    if %actor.varexists(zn118_ridleywrite)%
      wait 1 s
      say Thank you %actor.name%, I will restore them to their natural forms, evil though they be..
      wait 1 s
      %purge% box
      wait 1 s
      say And as for that book of yours, come let me write in it for you.
    else
      wait 1 s
      say Ah, it seems not everything is here.
      wait 1 s
      give box %actor.name%
      drop box
      wait 1 s
      say All of these I must reclaim... a flower, a fruit, and a flying thing.
      wait 1 s
      say Remember, do not kill the flying thing, only "capture" it.
    end
    if %actor.varexists(zn118_fruit)%
      rdelete zn118_fruit %actor.id%
      if %actor.varexists(zn118_flower)%
        rdelete zn118_flower %actor.id%
        if %actor.varexists(zn118_flying)%
          rdelete zn118_flying %actor.id%
        end
      end
    end
  else
    wait 1 s
    say Hmm, I am not sure why you had that.
  end
elseif %object.vnum%==11807
  if %actor.varexists(zn118_ridleywrite)%
    wait 2 s
    emote rearranges her blocks to spell W O R D S.
    wait 2 s
    say Words I promised you and words you shall have.
    wait 3 s
    emote weaves her hands over the blocks, drawing a dark shadow from them and guiding it into the journal.
    wait 3 s
    say There we are.
    rdelete zn118_ridleywrite %actor.id%
    set zn118_ridleydone 1
    remote zn118_ridleydone %actor.id%
    wait 1 s
    nop %actor.exp(10000)%
    give journal %actor.name%
    drop journal
    wait 1 s
    say I have something else for you too.
    wait 2 s
    emote rearranges her blocks to spell S W O R D.
    wait 2 s
    emote weaves her hands over the blocks, drawing a dark shadow from them and shaping it with her fingers.
    wait 3 s
    %load% obj 11835
    say Words can be a weapon...
    wait 1 s
    give sword %actor.name%
    drop sword
    wait 1 s
    say That will cut all bonds of silence. I can assure you that you will need it... though not for yourself.
  elseif  %actor.varexists(zn118_ridleyquest)%
    wait 1 s
    say Ah, first I must unmake what I have made.
    wait 1 s
    say I must have the box with the three items... the fruit, the flower, and the flying thing... remember only to 'capture' the flying thing, do not kill it.
    wait 2 s
    say If you have lost the box I gave you, I can make another... just say "another" if that be the case.
    wait 1 s
    give journal %actor.name%
    drop journal
  elseif %actor.varexists(zn118_ridleydone)%
    wait 1 s
    say I have already written in that for you %actor.name%.
    give journal %actor.name%
    drop journal
  else
    wait 1 s
    emote stops what she is doing and glances at the journal.
    wait 2 s
    emote sighs loudly.
    wait 1 s
    say Ah strange person you are, bringing a book of words to me...
    wait 2 s
    say Do you not know that I am a stealer of words?
    wait 2 s
    say So dangerous they are... and it is my job to make them safe.
    wait 2 s
    emote turns the journal over in her hands, studying it thoughtfully.
    wait 2 s
    say Perhaps it is time though...
    wait 2 s
    say Time to unmake what I have made, and let the cloak fall. I have spent long enough in deception.
    wait 4 s
    say Yes, words I will give you... if you return mine to me.
    wait 2 s
    give journal %actor.name%
    drop journal
    wait 1 s
    say Three things they are...
    wait 1 s
    say A fruit, a flower, and a flying thing...
    wait 2 s
    say Please do not kill the flying thing, only "capture" it.
    wait 1 s
    %load% obj 11834
    give box %actor.name%
    drop box
    wait 2 s
    say That box will hold them all safely, please return it to me when it contains them all.
    set zn118_ridleyquest 1
    remote zn118_ridleyquest %actor.id%
  end
else
  wait 1 s
  say I don't think this is meant for me.
  drop %object.name.car%
end
~
#11828
(11) Ridley gives box~
0 d 100
another~
%load% obj 11834
wait 1 s
emote weaves her hands in the air, drawing darkness from the room into a solid shape.
wait 1 s
say Please do not lose it again.
wait 1 s
give box %actor.name%
~
#11829
(21) insect can be captured~
0 c 100
capture~
if %arg% == insect
  %send% %actor% You make a quick grab for %self.name%, capturing it expertly in your hands.
  %echoaround% %actor% %actor.name% makes a quick grab for %self.name%, capturing it expertly in %actor.hisher% hands.
  %load% obj 11833 %actor% inv
  %purge% %self%
else
  %send% %actor% Capture what?
end
~
#11830
(33) insect escapes after while~
1 f 100
~
eval actor %self.carried_by%
if %actor.is_pc%
  %send% %actor% %self.shortdesc% suddenly squirms free from your hands and flies away.
  %echoaround% %actor% %self.shortdesc% suddenly squirms free of %actor.name%'s grasp and flies away.
  %load% mob 11821
  %purge% self
end
~
#11831
(21) insect loads self on death~
0 f 100
~
%at% 11832 %load% mob 11821
~
#11832
(22) score of secrets killed~
0 f 100
~
if %actor.varexists(zn118_tunnelquest)%
  eval zn118_tunnelquest %actor.zn118_tunnelquest% + 1
  remote zn118_tunnelquest %actor.id%
end
eval room %random.18%
eval room %room% + 11874
%at% %room% %load% mob 11822
~
#11833
(22) tunnel-man quest~
0 j 100
~
if %object.vnum% == 11807
  if %actor.varexists(zn118_tunneldone)%
    wait 1 s
    say I have already written in that for you.
    give journal %actor.name%
    drop journal
  elseif %actor.varexists(zn118_tunnelquest)%
    eval score %actor.zn118_tunnelquest% - 1
    if %score% < 15
      wait 1 s
      say Thank you for your work %actor.name%, you have vanquished %score% secrets.
      wait 2 s
      say Though you will never defeat them all, would you please destroy some more?
      wait 2 s
      give journal %actor.name%
      drop journal
    else
      wait 1 s
      say Well done %actor.name%, you have rid us of %score% secrets.
      wait 2 s
      say As I promised, I shall write my piece.
      wait 1 s
      emote grasps a pen from thin air and writes something in a leather-strapped journal.
      rdelete zn118_tunnelquest %actor.id%
      set zn118_tunneldone 1
      remote zn118_tunneldone %actor.id%
      wait 3 s
      nop %actor.exp(10000)%
      give journal %actor.name%
      drop journal
      %load% obj 11855
      give key %actor.name%
      drop key
      wait 1 s
      say And here with the journal, a gift...
      wait 1 s
      say The way to the end is open before you.
      wait 1 s
      say Do you see?
      wait 1 s
    end
  else
    wait 1 s
    emote manoeuvres the journal, making it levitate and turn slowly in the air.
    wait 2 s
    say You are very trusting to give this to me shadow-person.
    wait 1 s
    say I have no name, though Ridley calls me Daniel... you would be wise to offer this to her if you have not done so already.
    wait 3 s
    emote turns his attention back to the twirling journal.
    wait 2 s
    say You do realise shadow-person... that this quest of yours will destroy me?
    wait 3 s
    say Indeed, not just myself, but all who dwell here.
    wait 2 s
    emote grows silent for a moment, apparently thinking.
    wait 3 s
    say Still... if our fall means purging of the evil along with us, then I would see it done.
    wait 3 s
    say I ask only this...that you prove to me your intentions.
    wait 2 s
    emote gestures toward the swirling tunnel.
    wait 2 s
    say Please enter it, destroy some of the evil you find there, though you will never kill them all.
    wait 2 s
    say Your willingness to do so will convince me.
    wait 1 s
    set zn118_tunnelquest 1
    remote zn118_tunnelquest %actor.id%
    give journal %actor.name%
    drop journal
  end
else
  wait 1 s
  say I don't think this is meant for me.
  give %object.name.car% %actor.name%
  drop %object.name.car%
end
~
#11834
(31) enter vortex teleports~
2 c 100
enter~
%send% %actor% You enter the vortex.
%echoaround% %actor% %actor.name% enters the vortex.
if %self.vnum% == 11831
  %teleport% %actor% 11874
elseif %self.vnum% -- 11874
  %teleport% %actor% 11831
end
wait 1 s
%force% %actor% look
~
#11835
(23) running girl runs~
0 ab 100
~
if %self.room.vnum% == 11876
  wait 1 s
  s
else
  %teleport% %self% 11876
  s
end
wait 1 s
s
wait 1 s
say I've got to keep going...
s
wait 1 s
s
wait 1 s
say Back and forth...
w
wait 1 s
n
wait 1 s
n
wait 1 s
say It gets harder and harder...
n
wait 1 s
n
wait 1 s
e
say But keep going...
wait 1 s
emote gasps for air.
~
#11836
(23) running girl does what told~
0 d 100
*~
if %actor.varexists(zn118_runningstart)%
  %speech%
  wait 1
  if %self.room.vnum% == 11831
    rdelete zn118_runningstart %actor.id%
    set zn118_runningquest 1
    remote zn118_runningquest %actor.id%
    wait 1 s
    tell %actor.name% Thank you for getting me out of there.
    follow self
  end
end
~
#11837
test missile~
1 c 1
shoot~
if %arg% == north
  eval north %self.room.north(vnum)%
  eval room %self.room.north(room)%
  eval people %room.people%
  if %people%
    %send% %people% You are suddenly shot by %actor.name%.
    %at% %north% %damage% %people% 20
    %send% %actor% You shoot %people.name% and damage %people.himher%.
    %echoaround% %actor% %actor.name% shoots %people.name% and damages %people.himher%
  else
    %send% %actor% Theres no one there.
  end
elseif %arg% == east
  eval east %self.room.east(vnum)%
  eval room %self.room.east(room)%
  eval people %room.people%
  if %people%
    %send% %people% You are suddenly shot by %actor.name%.
    %at% %east% %damage% %people% 20
    %send% %actor% You shoot %people.name% and damage %people.himher%.
    %echoaround% %actor% %actor.name% shoots %people.name% and damages %people.himher%
  else
    %send% %actor% Theres no one there.
  end
elseif %arg% == south
  eval south %self.room.south(vnum)%
  eval room %self.room.south(room)%
  eval people %room.people%
  if %people%
    %send% %people% You are suddenly shot by %actor.name%.
    %at% %south% %damage% %people% 20
    %send% %actor% You shoot %people.name% and damage %people.himher%.
    %echoaround% %actor% %actor.name% shoots %people.name% and damages %people.himher%
  else
    %send% %actor% Theres no one there.
  end
elseif %arg% == west
  eval west %self.room.west(vnum)%
  eval room %self.room.west(room)%
  eval people %room.people%
  if %people%
    %send% %people% You are suddenly shot by %actor.name%.
    %at% %west% %damage% %people% 20
    %send% %actor% You shoot %people.name% and damage %people.himher%.
    %echoaround% %actor% %actor.name% shoots %people.name% and damages %people.himher%
  else
    %send% %actor% Theres no one there.
  end
elseif %arg% == up
  eval up %self.room.up(vnum)%
  eval room %self.room.up(room)%
  eval people %room.people%
  if %people%
    %send% %people% You are suddenly shot by %actor.name%.
    %at% %up% %damage% %people% 20
    %send% %actor% You shoot %people.name% and damage %people.himher%.
    %echoaround% %actor% %actor.name% shoots %people.name% and damages %people.himher%
  else
    %send% %actor% Theres no one there.
  end
elseif %arg% == down
  eval down %self.room.down(vnum)%
  eval room %self.room.down(room)%
  eval people %room.people%
  if %people%
    %send% %people% You are suddenly shot by %actor.name%.
    %at% %down% %damage% %people% 20
    %send% %actor% You shoot %people.name% and damage %people.himher%.
    %echoaround% %actor% %actor.name% shoots %people.name% and damages %people.himher%
  else
    %send% %actor% Theres no one there.
  end
else
  %send% %actor% You must specify a direction - north, east, west, south, up, or
  down.
end
~
#11838
test missile condensed~
1 c 1
shoot~
if %arg% == north || %arg% ==  east || %arg% == south || %arg% == west || %arg% == up || %arg% == down
  eval dir %arg%
  eval direction %%self.room.%dir%(vnum)%%
  eval where %%self.room.%dir%(room)%%
  eval people %where.people%
  if %people%
    %send% %people% You are suddenly shot by %actor.name%.
    %at% %direction% %damage% %people% 20
    %send% %actor% You shoot %people.name% and damage %people.himher%.
    %echoaround% %actor% %actor.name% shoots %people.name% and damages %people.himher%
  else
    %send% %actor% Theres no one there.
  end
else
  %send% %actor% You must specify a direction - north, east, west, south, up, or down.
end
~
#11839
test bow/quiver~
1 c 1
shoot~
if %arg% == north || %arg% ==  east || %arg% == south || %arg% == west || %arg% == up || %arg% == down
  eval dir %arg%
  eval direction %%self.room.%dir%(vnum)%%
  eval where %%self.room.%dir%(room)%%
  eval people %where.people%
  if %actor.has_item(2731)%
    %force% %actor% take arrow quiver
  end
  if %actor.inventory(2733)%
    if %people%
      switch %random.3%
        case 1
          %send% %people% You are suddenly shot by %actor.name%.
          %at% %direction% %damage% %people% 20
          %send% %actor% You shoot %people.name% and damage %people.himher%.
          %echoaround% %actor% %actor.name% shoots %people.name% and damages %people.himher%
          %purge% %actor.inventory(2733)%
          %load% obj 2733 %people% inv
        break
        case 2
          %send% %people% %actor.name% tries to shoot you but misses.
          %send% %actor% You try to shoot %people.name% but miss.
          %echoaround% %actor% %actor.name% tries to shoot %people.name% but misses
          %purge% %actor.inventory(2733)%
          %load% obj 2733
        break
        case 3
          %send% %people% You are suddenly shot by %actor.name%.
          %send% %people% The arrow breaks.
          %at% %direction% %damage% %people% 20
          %send% %actor% You shoot %people.name% and damage %people.himher%.
          %send% %actor% The arrow breaks.
          %echoaround% %actor% %actor.name% shoots %people.name% and damages %people.himher%
          %purge% %actor.inventory(2733)%
        break
        default
          %send% %people% You are suddenly shot by %actor.name%.
          %at% %direction% %damage% %people% 20
          %send% %actor% You shoot %people.name% and damage %people.himher%.
          %echoaround% %actor% %actor.name% shoots %people.name% and damages %people.himher%
          %purge% %actor.inventory(2733)%
          %load% obj 2733 %people% inv
        break
      done
    else
      %send% %actor% Theres no one there.
    end
  else
    %send% %actor% You can't shoot without arrows.
  end
else
  %send% %actor% You must specify a direction - north, east, west, south, up, or down.
end
~
#11840
test improved bow~
1 c 1
shoot~
*
* As is this trigger is attached to bow 2732
* To be used with arrows - 2733, 2780, 2781, 2782.
* Can also be optionally used with quiver 2731
*
*
* Checks if there are any players/mobs in the room
* to the direction specified.
*
if %arg.mudcommand% == north || %arg.mudcommand% == east || %arg.mudcommand% == south || %arg.mudcommand% == west || %arg.mudcommand% == up || %arg.mudcommand% == down
  eval dir %arg.mudcommand%
  eval direction %%self.room.%dir%(vnum)%%
  eval where %%self.room.%dir%(room)%%
  eval people %where.people%
  *
  * These next three lines just make the actor withdraw
  * an arrow if he/she is carrying them in a quiver.
  * Is just intended to give the quiver a helpful feature.
  *
  if %actor.has_item(2731)%
    %force% %actor% take arrow quiver
  end
  *
  * Checks for the first item in inventory that is one of the 
  * specified arrows and sets its vnum as the one to be used.
  *
  eval inv %actor.inventory%
  while (%inv%)
    if %inv.vnum% == 2733 || %inv.vnum% == 2780 || %inv.vnum% == 2781 || %inv.vnum% == 2782
      eval arrow %inv.vnum%
      eval weapon %inv%
      %echo% %arrow%
      set inv 0
    else
      eval next %inv.next_in_list%
      set inv %next%
    end
  done
  *
  * Searchable array by Random, matches the chosen arrow
  * vnum with the damage stats and any affects to be used.
  *
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
  *
  * Just a calculation of the arrow stats using a
  * random number of dice between 1 and 3.
  *
  set dice %random.3%
  eval finaldam ((%dice% * %dam%) + %bonus%)
  %echo% Hits for total of %finaldam%.  
  *
  * If the actor has an arrow in inventory, and there are 
  * people in the room specified, one of three random things
  * happens - Actor shoots but misses, Actor shoots and damages,
  * Actor shoots, damages, but loses the arrow.
  *
  if %arrow%
    if %people%
      switch %random.3%
        case 1
          %send% %people% %actor.name% shoots you with %weapon.shortdesc%.
          %at% %direction% %damage% %people% %finaldam%
          %force% %people% mhunt %actor%
          if %spell%
            dg_cast '%spell%' %people%
          end
          %send% %actor% You shoot %people.name% with %weapon.shortdesc%.
          %echoaround% %actor% %actor.name% shoots %people.name% with %weapon.shortdesc%.
          %purge% %actor.inventory(%arrow%)%
          %load% obj %arrow% %people% inv
        break
        case 2
          %send% %people% %actor.name% tries to shoot you but misses.
          %send% %actor% You try to shoot %people.name% with %weapon.shortdesc% but miss.
          %echoaround% %actor% %actor.name% tries to shoot %people.name% but misses
          %purge% %actor.inventory(%arrow%)%
          %load% obj %arrow%
        break
        case 3
          %send% %people% %actor.name% shoots you with %weapon.shortdesc%.
          %send% %people% %weapon.shortdesc% breaks.
          %force% %people% mhunt %actor%
          %at% %direction% %damage% %people% %finaldam%
          if %spell%
            dg_cast '%spell%' %people%
          end
          %send% %actor% You shoot %people.name% with %weapon.shortdesc%.
          %send% %actor% %weapon.shortdesc% breaks.
          %echoaround% %actor% %actor.name% shoots %people.name% with %weapon.shortdesc%.
          %purge% %actor.inventory(%arrow%)%
        break
        default
          %echo% If you see this message, something is wrong. Please report it.
        break
      done
    else
      %send% %actor% Theres no one there.
    end
  else
    %send% %actor% You can't shoot without arrows.
  end
else
  %send% %actor% You must specify a direction - north, east, west, south, up, or down.
end
~
#11841
test asound~
1 c 7
test~
* No Script
~
#11842
(23) running quest given~
0 j 100
~
if %object.vnum% == 11807
  if %actor.varexists(zn118_runningdone)%
    wait 1 s
    say I have already written in that for you.
    give journal %actor.name%
    drop journal
  elseif %actor.varexists(zn118_runningquest)%
    wait 1 s
    emote pauses to think for a moment.
    wait 1 s
    emote scrawls something carefully into a leather-strapped journal.
    wait 1 s
    nop %actor.exp(10000)%
    give journal %actor.name%
    drop journal
    rdelete zn118_runningquest %actor.id%
    set zn118_runningdone 1
    remote zn118_runningdone %actor.id%
  else
    wait 1 s
    detach 11835 %self.id%
    emote comes to a halt.
    wait 1 s
    emote peers at you curiously.
    wait 2 s
    say There is not much that would give me hope enough to abandon this task.
    wait 3 s
    say But this is something I never dared to have the chance to write in...
    wait 3 s
    say Take me out of this tunnel-world and I will.
    wait 2 s
    say Whatever you say, I will do.
    set zn118_runningstart 1
    remote zn118_runningstart %actor.id%
    give journal %actor.name%
    drop journal
  end
else
  wait 1 s
  say I don't think this is meant for me.
  give %object.name.car% %actor.name%
  drop %object.name.car%
end
~
#11843
(73) examining walls reveals word~
2 c 100
ex~
if %cmd.mudcommand% == examine
  if walls /= %arg%
    eval max %random.21%
    set  txt[1] PATRIS
    set  txt[2] PRODITIO
    set  txt[3] CRUCIATUS
    set  txt[4] DOLOR
    set  txt[5] PROFANO
    set  txt[6] PROBRUM
    set  txt[7] COACTU
    set  txt[8] CONNUBIUM
    set  txt[9] ABSCONDO
    set  txt[10] VULNERO
    set  txt[11] CRUENTO
    set  txt[12] MINUO
    set  txt[13] FORMIDILOSUS
    set  txt[14] PAVIDUS
    set  txt[15] TIMEO
    set  txt[16] FORO
    set  txt[17] LEDO
    set  txt[18] PERFRINGO
    set  txt[19] PUPUGI
    set  txt[20] CONSTUPRO
    set  txt[21] RAPIO
    set  word %%txt[%max%]%%
    eval word %word%
    %send% %actor% Your eyes scan the glowing walls and alight upon the word @C%word%@n.
    if !%actor.varexists(zn118_blinddone)%
      set zn118_blindquest %word%
      remote zn118_blindquest %actor.id%
    end
  else
    return 0
  end
end
~
#11844
new trigger~
2 c 100
ge~
if %cmd.mudcommand% == get && %arg% == pick
  if !%taken%
    set taken 1
    global taken
  end
  if %taken% < 5
    %load% obj 2700 %actor% inv
    eval taken %taken% + 1
    global taken
    %send% %actor% You take a pick.
    %echoaround% %actor% %actor.name% gets a pick.
  else
    %send% %actor% There are no more picks.
  end
else
  %send% %actor% Get what?
end
~
#11845
test~
2 c 100
test~
set taken 1
global taken
~
#11846
(24) blind quest given~
0 j 100
~
if %object.vnum% == 11807
  if %actor.varexists(zn118_blinddone)%
    wait 1 s
    say I have already written in that for you.
    give journal %actor.name%
    drop journal
  elseif %actor.varexists(zn118_blindword)%
    wait 1 s
    say Since you have proved yourself willing to read...
    wait 1 s
    say Here, I am willing to write.
    wait 1 s
    emote feels her way along the pages, and writes carefully into the journal.
    wait 2 s
    nop %actor.exp(10000)%
    give jounal %actor.name%
    drop journal
    rdelete zn118_blindword %actor.id%
    set zn118_blinddone 1
    remote zn118_blinddone %actor.id%
  else
    wait 1 s
    emote turns the journal over in her hands.
    wait 1 s
    say For years, I have written and none have read.
    wait 2 s
    emote gestures around her.
    wait 1 s
    say Or have all my words been wiped away?
    wait 2 s
    give journal %actor.name%
    drop journal
    wait 2 s
    say Eyes from your world do not always see the truth...
    wait 2 s
    say Borrow mine, for I still have them, though I choose not to see.
    wait 3 s
    %load% obj 11851
    give eyes %actor.name%
    drop eyes
    wait 2 s
    say Wear them, and look again.
    wait 8 s
    tell %actor.name% Examine the walls with those eyes, if it is words you seek...
    wait 2 s
    tell %actor.name% Speak what it is you see, and if it is truth I may write for you.
  end
elseif %object.vnum% == 11851
  wait 1 s
  say Thank you.
else
  wait 1 s
  say I don't think this is meant for me.
  drop %object.name.car%
end
~
#11847
(24) blind girl checks matching word~
0 d 100
*~
if %actor.varexists(zn118_blindquest)%
  if %speech% /= %actor.zn118_blindquest%
    rdelete zn118_blindquest %actor.id%
    set zn118_blindword 1
    remote zn118_blindword %actor.id%
    wait 1 s
    emote smiles happily.
    wait 1 s
    say You are right %actor.name%. I believe I shall write for you after all.
  else
    wait 1 s
    emote shakes her head.
    wait 1 s
    say Try examining again %actor.name%, speak what it is you see.
  end
else
  return 1
end
~
#11848
(51) wearing eyes teleports~
1 j 100
~
%teleport% %actor% 11873
~
#11849
(51) removing eyes teleports~
1 l 100
~
%teleport% %actor% 11854
~
#11850
(24/25) cannot leave with eyes~
0 q 100
~
if %actor.has_item(11851)%
  say The eyes stay with me, %actor.name%.
  return 0
end
~
#11851
(25) reacts to journal~
0 j 100
~
if %object.vnum% == 11807
  if %actor.varexists(zn118_blinddone)%
    wait 1 s
    say I have already written in that for you.
    give journal %actor.name%
    drop journal
  elseif %actor.varexists(zn118_blindword)%
    wait 1 s
    say Since you have proved yourself willing to read...
    wait 1 s
    say Here, I am willing to write.
    wait 1 s
    emote feels her way along the pages, and writes carefully into the journal.
    wait 2 s
    give journal %actor.name%
    drop journal
    rdelete zn118_blindword %actor.id%
    set zn118_blinddone 1
    remote zn118_blinddone %actor.id%
  else
    wait 1 s
    say Examine the walls %actor.name%, if it is words you seek.
    wait 2 s
    say Speak what it is you see, and if it be truth I shall write again for you.
    give journal %actor.name%
    drop journal
  end
else
  wait 1 s
  say I don't think this is meant for me.
  drop %object.name.car%
end
~
#11852
(29) cup and ball game~
0 d 100
*~
if %actor.varexists(zn118_knifestart)%
if %speech% == start
  eval max %random.3%
  set txt[1] right
  set txt[2] middle
  set txt[3] left
  set ball %%txt[%max%]%%
  eval ball %ball%
  emote places the ball under the %ball% cup.
  wait 1 s
  set 1 left
  set 2 middle
  set 3 right
  set tries 7
  while %tries% > 1
    wait 2 s
    switch %random.7%
      case 1
      emote switches the first and third cup around.
      eval z %1%
      eval 1 %3%
      eval 3 %z%
      
    break
    case 2
      emote swaps the third cup with the second cup.
      eval z %2%
      eval 2 %3%
      eval 3 %z%
      
    break
    case 3
      emote switches the second cup with the first cup.
      eval z %2%
      eval 2 %1%
      eval 1 %z%
      
    break
    case 4
      emote slides the first cup to third place.
      eval z %1%
      eval 1 %2%
      eval 2 %3%
      eval 3 %z%
      
    break
    case 5
      emote moves the second cup to the third position.
      eval z %2%
      eval 2 %3%
      eval 3 %z%
      
    break
    case 6
      emote moves the second cup into first place.
      eval z %2%
      eval 2 %1%
      eval 1 %z%
      
    break
    case 7
      emote moves the third cup into the first position.
      eval z %3%
      eval 3 %2%
      eval 2 %1%
      eval 1 %z%
      
    break
    default
      %echo% Something is broken, please report.
      
    break
  done
  eval tries %tries% - 1
done
  wait 1 s
  say Please say which cup you think the ball is under... 1, 2, or 3?
  %context% %actor.id%
  if %1% == %ball%
    set answer 1
  elseif %2% == %ball%
    set answer 2
  elseif %3% == %ball%
    set answer 3
  end
  global answer
elseif %speech% < 4
  if %speech% == %answer%
    say Ah you are correct!
set answer void
global answer
    nop nop %actor.exp(1000)%
    if %actor.varexists(zn118_knifestart)%
      eval zn118_knifestart %actor.zn118_knifestart% + 1
      remote zn118_knifestart %actor.id%
    end
    wait 1 s
eval score %actor.zn118_knifestart% - 1
if %score% <= 0
set score 0
end
say You have won %score% times %actor.name%.
if %score% >= 3
wait 1 s
say You have won three times! As promised, I shall write in your journal if you give it to me.
    set zn118_knifequest 1
    remote zn118_knifequest %actor.id%
end
    wait 1 s
    say Say start if you want to try again.
elseif %answer% == void
say You have already answered for this game %actor.name%.
wait 1 s
say Say start if you want to play again.
  else
    say No, its under the number %answer% cup.
    wait 1 s
    emote slashes her arm with frustration.
    wait 1 s
    %damage% %self% 30
    if %actor.varexists(zn118_knifestart)%
      eval score %actor.zn118_knifestart% - 1
if %score% <= 0
set score 0
end
    end
    say You have won %score% times %actor.name%.
    wait 1 s
    say Say start if you want to try again.
  end
end
elseif %speech% == start
emote blinks at you.
wait 1 s
say I did not ask you to play.
end
~
#11853
mob does not load if already present~
0 k 100
~
@c* This sets the vnum of the mob you are checking for, and loading/not loading@n
set mobvnum 42500
@c* This tells the trigger to keep checking for people until there are none.@n
set here %self.room.people%
while %here%
  set others %here.next_in_room%
@c* If the mob is in the room, the isthere variable is set.@n
  if %here.vnum% == %mobvnum%
set mobid %here%
    set isthere 1
  end
  set here %others%
done
@c* If the mob is not in the room (variable is not set)@n
@c* The mob will load.@n
if !%isthere%
  %load% mob %mobvnum%
end
~
#11854
test death~
0 f 100
~
wait 1
%purge% self
~
#11855
new trigger~
0 g 100
~
if !%actor.varexists(13667_greeted)%
  tell %actor.name% Hallo!
  set 13667_greeted 1
  remote 13667_greeted %actor.id%
end
~
#11856
(96) quiet room prevents player interacting~
2 c 100
*~
%teleport% %actor% 11897
return 0
wait 1
if %actor.room.vnum% == 11897
  %teleport% %actor% 11896
end
~
#11857
new trigger~
0 c 100
*~
if %self.master% == %actor%
  %echo% test
  if %cmd%==minv
    %send% %actor% your mules inv
  else
    return 0
  end
else
  return 0
end
~
#11858
while loop example~
0 c 100
test~
set var 0
while %var% < 10
  %damage% %self% -10
  %echo% %self.name% is surrounded with a healing aura.
  wait 1 s
  eval var %var% + 1
done
~
#11859
(52) mirror gives reflection~
1 c 100
*~
if %cmd.mudcommand% == look || %cmd.mudcommand% == examine
  if %self.name% /= %arg%
    return 0
    wait 1
    if %actor.cha% == 12
      set cha the confident
    elseif %actor.cha% < 12
      set cha an uncertain
    else
      set cha the charismatic
    end
    if %actor.align% == 0
      set align with a well balanced glow
    elseif %actor.align% < 0
      set align with a dark tinge of evil
    else
      set align with a purity of goodness
    end 
    if %actor.class% == Cleric
      set class healer
    elseif %actor.class% == Warrior
      set class fighter
    elseif %actor.class% == Thief
      set class sneaker
    else
      set class caster
    end
    if %actor.con% > 12
      set con hardy
    else
      set con vulnerable
    end
    if %actor.dex% > 12
      set dex agile
    else
      set dex clumsy
    end
    if %actor.int% > 12
      set int decisive
    else
      set int uncertain
    end
    if %actor.level% < 10
      set level an inexperienced
    elseif %actor.level% > 20
      set level a very experienced
    else
      set level a experienced
    end
    eval hitp %actor.maxhitp% / %actor.hitp%
    if %hitp% == 1
      set hitp Healthy
    elseif %hitp% < 3
      set hitp Injured
    else
      set hitp Badly injured
    end
    eval mana %actor.maxmana% / %actor.mana%
    if %mana% == 1
      set mana brightly
    elseif %mana% < 3
      set mana moderately
    else
      set mana faintly
    end
    eval move %actor.maxmove% / %actor.move%
    if %move% == 1
      set move energetic-looking
    elseif %move% < 3
      set move tired-looking
    else
      set move exhausted-looking
    end
    if %actor.sex% == male
      set sex man
    elseif %actor.sex% == female
      set sex woman
    else
      set sex person
    end
    if %actor.str% > 12
      set str Strong
    else
      set str Weak
    end
    if %actor.wis% > 12
      set wis wise
    else
      set wis naieve
    end
    if %actor.eq(wield)%
      eval wield %actor.eq(wield)%
      set wield is clutching %wield.shortdesc%
    else
      set wield appears weaponless
    end
    if %actor.eq(body)%
      eval body %actor.eq(body)%
      set body is clothed with %body.shortdesc% and
    else
      set body is bare-chested and
    end
    %send% %actor% You see the reflection of %level% %sex% with %cha% air of a %wis% %class% about %actor.himher%.
    %send% %actor% %str% and %con% looking, %actor.heshe% %body% %wield%, %actor.hisher% movements %int% and %dex%. %hitp% and %move%, %actor.hisher% magical aura shines %mana%, %align%.
  else
    return 0
  end
else
  return 0
end
~
#11860
(27) weeping quest given~
0 j 100
~
if %object.vnum% == 11807
  if %actor.varexists(zn118_weepingdone)%
    wait 1 s
    say I have already written in that for you.
    give journal %actor.name%
    drop journal
  elseif %actor.varexists(zn118_weepingquest)%
    rdelete zn118_weepingquest %actor.id%
    set zn118_weepingdone 1
    remote zn118_weepingdone %actor.id%
    wait 1 s
    smile %actor.name%
    wait 1 s
    emote etches something into the journal with a piece of glass.
    wait 1 s
    nop %actor.exp(10000)%
    give journal %actor.name%
    drop journal
    wait 1 s
    say Thank you %actor.name%.
  else
    wait 1 s
    say Ah I cannot even see properly to write...
    wait 1 s
    say I so miss my garden.
    wait 1 s
    cry
    wait 1 s
    say I used to love it so much...
    wait 2 s
    say But there is a monster there now.
    wait 1 s
    shiver
    wait 1 s
    say Please %actor.name%, kill it for me.. and I shall dry my tears and write.
    set zn118_weepingstart 1
    remote zn118_weepingstart %actor.id%
    give journal %actor.name%
    drop journal
  end
else
  wait 1 s
  say I don't think this is meant for me.
  give %object.name.car% %actor.name%
  drop %object.name.car%
end
~
#11861
(28) death completes quest~
0 f 100
~
if %actor.varexists(zn118_weepingstart)%
  rdelete zn118_weepingstart %actor.id%
  set zn118_weepingquest 1
  remote zn118_weepingquest %actor.id%
end
~
#11862
(29) knife quest~
0 j 100
~
if %object.vnum% == 11807
  if %actor.varexists(zn118_knifedone)%
    wait 1 s
    say I have already written in that for you.
    give journal %actor.name%
    drop journal
  elseif %actor.varexists(zn118_knifequest)%
    rdelete zn118_knifequest %actor.id%
    set zn118_knifedone 1
    remote zn118_knifedone %actor.id%
    wait 1 s
    say you've played my game %actor.name%, so I'll play yours.
    wait 1 s
    emote carves something into the journal with her knife.
    wait 1 s
    nop %actor.exp(10000)%
    give journal %actor.name%
    drop journal
    wait 1 s
    say It was fun, I'll play with you anytime if you want the experience.
  elseif %actor.varexists(zn118_knifestart)%
    say Win three times %actor.name%, and ask me again.
    wait 1 s
    say Just say start if you want to play.
    give journal %actor.name%
    drop journal
  else
    wait 1 s
    say I see... it looks like you are giving me something, but in fact you want me to give to you.
    wait 1 s
    chuckle
    wait 1 s
    say I have already carved words... yes many words, but no one will see.
    wait 2 s
    say An interesting game you are playing %actor.name%...
    wait 2 s
    emote gets out three cups and a ball.
    wait 1 s
    say Play with me for a while... win three times, and I shall add to your book.
    wait 2 s
    say Just say start when you are ready.
    set zn118_knifestart 1
    remote zn118_knifestart %actor.id%
    give journal %actor.name%
    drop journal
  end
else
  wait 1 s
  say I don't think this is meant for me.
  give %object.name.car% %actor.name%
  drop %object.name.car%
end
~
#11863
(33) Ruth quest~
0 j 100
~
if %object.vnum%==11856 || %object.vnum%==11857 || %object.vnum%==11858
  if %actor.varexists(zn118_ruthquest)%
    set zn118_%object.vnum% 1
    remote zn118_%object.vnum% %actor.id%
    wait 1 s
    say Thank you %actor.name%, this is one of the creatures dead.
    %purge% corpse
    if %actor.varexists(zn118_11856)%
      if %actor.varexists(zn118_11857)%
        if %actor.varexists(zn118_11858)%
          rdelete zn118_11856 %actor.id%
          rdelete zn118_11857 %actor.id%
          rdelete zn118_11858 %actor.id%
          rdelete zn118_ruthquest %actor.id%
          set zn118_ruthsecond 1
          remote zn118_ruthsecond %actor.id%
        end
      end
    end
    if %actor.varexists(zn118_ruthsecond)%
      rdelete zn118_ruthsecond %actor.id%
      set zn118_ruthpearl 1
      remote zn118_ruthpearl %actor.id%
      wait 1 s
      say Ah, thank you so much %actor.name%. You have killed the three creatures I spoke of...
      wait 3 s
      say I am afraid I must ask for one last thing...
      wait 2 s
      say Ridley will be grateful to you for your help here, it is within your power to ask something of her.
      wait 4 s
      say She holds one of the darkest secrets of this place.
      wait 2 s
      say Ask her about it and she will likely give it to you... you see, she knows only how to hide things, not to destroy.
      wait 6 s
      say Bring it to me, and I shall rid us of its haunting.
      wait 2 s
      say Do this last thing, and I shall ask no more.
    end
  else
    wait 1 s
    say I don't think this is meant for me.
    give %object.name.car% %actor.name%
    drop %object.name.car%
  end
elseif %object.vnum%== 11859
  if %actor.varexists(zn118_ruthpearl)%
    rdelete zn118_ruthpearl %actor.id%
    set zn118_ruthwrite 1
    say Thank you %actor.name%, give me your journal and I shall write.
    %purge% pearl
    remote zn118_ruthwrite %actor.id%
  else
    wait 1 s
    say I don't think this is meant for me.
    give %object.name.car% %actor.name%
    drop %object.name.car%
  end
elseif %object.vnum% == 11807
  if  %actor.varexists(zn118_ruthwrite)%
    wait 1 s
    say I have asked a lot of you, %actor.name%. All I have in return are my words.
    wait 3 s
    emote scrawls something carefully into the journal.
    wait 2 s
    say What you are doing will shake the foundations of this place.
    wait 2 s
    nop %actor.exp(10000)%
    give journal %actor.name%
    drop journal   
    rdelete zn118_ruthwrite %actor.id%
    set zn118_ruthdone 1
    remote zn118_ruthdone %actor.id%
  elseif %actor.varexists(zn118_ruthdone)%
    wait 1 s
    say I have already written in that for you %actor.name%.
    give journal %actor.name%
    drop journal
  elseif %actor.varexists(zn118_ruthquest)%
    wait 1 s
    say I have not yet all of the things I asked for.
    wait 1 s
    if %actor.varexists(zn118_11856)%
      say I have the fish.
    else
      say I am missing the fish.
    end
    if %actor.varexists(zn118_11857)%
      say I have the bird.
    else
      say I am missing the bird.
    end
    if %actor.varexists(zn118_11858)%
      say I have the toad.
    else
      say I am missing the toad.
    end
    wait 1 s
    give journal %actor.name%
    drop journal
    give %object.name.car% %actor.name%
    drop %object.name.car%
  elseif %actor.varexists(zn118_ruthpearl)%
    wait 1 s
    say Only one last thing I need from you, %actor.name%.
    wait 1 s
    say The secret Ridley holds.
    wait 1 s
    say Ask her about it, and she will no doubt give it to you.
    give %object.name.car% %actor.name%
    drop %object.name.car%
    give journal %actor.name%
    drop journal
  elseif  %actor.varexists(zn118_ruthwrite)%
    wait 1 s
    say You have given me all I asked for...
    wait 1 s
    say Now all I need is the journal to complete my part of the bargain.
    wait 1 s
    give %object.name.car% %actor.name%
    drop %object.name.car%
  else
    wait 1 s
    emote stares wide-eyed at the journal.
    wait 1 s
    say So you are the one Ridley spoke of!
    wait 2 s
    I am the one that came after her... she conceals, and I destroy.
    wait 2 s
    say She had the power to heal us then, but now, now it is my time.
    wait 3 s
    say Bring me three of her monster's corpses, and I will add my part to this quest.
    wait 4 s
    say A bird... a white bird, a fish, and a toad they are. Lurking in each of the three parts of this house.
    wait 5 s
    say Bring me their carcasses %actor.name%, and my words are yours.
    set zn118_ruthquest 1
    remote zn118_ruthquest %actor.id%
    give journal %actor.name%
    drop journal
  end
else
  wait 1 s
  say I don't think this is meant for me.
  give %object.name.car% %actor.name%
  drop %object.name.car%
end
~
#11864
different corpses loaded~
0 f 100
~
%teleport% %self% 11800
if %self.vnum% == 11830
  %at% 11895 xx118trig118xx wload obj 11856
elseif %self.vnum% == 11831
  %at% 11838 xx118trig118xx wload obj 11857
elseif %self.vnum% == 11832
  %at% 11849 xx118trig118xx wload obj 11858
elseif %self.vnum% == 11839
  %at% 11848 xx118trig118xx wload obj 11869
end
~
#11865
obj corpses decay~
1 f 100
~
if %self.vnum% == 11870
%echo% %self.shortdesc% decays away, a few traces of dust blowing in the wind.
else
%echo% A quivering horde of maggots consumes %self.shortdesc%.
end
%purge% self
~
#11866
room obeys mob trigger~
2 c 100
xx118trig118xx~
%arg%
~
#11867
(11) Ridley responds to asking~
0 c 100
a~
if %cmd% == ask
  if %arg% /= Ridley
    if %arg% /= secret
      if %actor.varexists(zn118_ruthpearl)%
        if %actor.zn118_ruthpearl% == 1
          wait 1 s
          emote blinks at you.
          wait 1 s
          say Was it Ruth who spoke to you of this?
          wait 2 s
          sigh
          wait 1 s
          say Well perhaps she is right to send you...
          wait 1 s
          say You have done much for us... here, take my most guarded secret.
          wait 2 s
          %load% obj 11859
          set zn118_ruthpearl 2
          remote zn118_ruthpearl %actor.id%
          give pearl %actor.name%
drop pearl
        else
          tell %actor.name% I have already given you my secret.
        end
      else
        tell %actor.name% I have no desire to speak of it.
      end
    else
      return 0
    end
  else
    return 0
  end
else
  return 0
end
~
#11868
(70) Hiding girl beckons~
2 g 100
~
if %direction% == west
  wait 1 s
  %send% %actor% You hear a shuffling sound from under the bed.
  wait 1 s
  %send% %actor% A child's voice whispers: Quick, crawl in here!
end
~
#11869
(70/71) crawling takes you under bed~
2 c 100
cr~
if %self.vnum% == 11870
  %send% %actor% You drop to your knees and crawl under the bed.
  %echoaround% %actor% %actor.name% drops to %actor.hisher% knees and crawls under the bed.
  wait 1 s
  %at% 11871 %echo% %actor.name% crawls under the bed.
  %teleport% %actor% 11871
elseif %self.vnum% == 11871
  %send% %actor% You crawl awkwardly out from under the bed.
  %echoaround% %actor% %actor.name% crawls awkwardly out from under the bed.
  %at% 11870 %echo% %actor.name% crawls out from under the bed.
  wait 1 s
  %teleport% %actor% 11870
end
%force% %actor% look
~
#11870
(34) random mumblings~
0 b 10
~
emote mumbles to herself 'He took the bread and broke it, saying: this means my body.'
~
#11871
(60) hotdog damages when eaten~
1 c 2
eat~
if %cmd.mudcommand% == eat && hotdog /= %arg%
  %send% %actor% You feel an excruciating burning pain as it slides down your throat.
  eval amount %actor.maxhitp%/3
  if %amount% >= %actor.hitp%
    eval amount %actor.hitp%+3
  end
  %damage% %actor% %amount%
  %purge% self
else
  return 0
end
~
#11872
(34) girl restores from death~
0 c 100
eat~
return 0
wait 3 s
if %cmd.mudcommand% == eat && hotdog /= %arg%
  if %actor.varexists(zn118_thinquest)%
    if %actor.hitp% > -3
      say It is not enough to taste pain. You must taste death.
      %load% obj 11860
      give hotdog %actor.name%
      drop hotdog
    else
      wait 3 s
      eval %amount% %actor.maxhitp%
      %damage% %actor% -9999
      %send% %actor% Just as you feel your life force ebbing away, a healing power draws it back.
      rdelete zn118_thinquest %actor.id%
      set zn118_thinwrite 1
      remote zn118_thinwrite %actor.id%
      wait 1 s
      smile %actor.name%
      wait 1 s
      say Ah, this is all the sacrifice I need %actor.name%.
      wait 2 s
      say Not the giving of your life, but the willingness to.
      wait 2 s
      say See, here now I am also willing.
      wait 1 s
      say Give me the journal, and I shall write...
    end
  end
end
~
#11873
(34) emaciated quest~
0 j 100
~
if %object.vnum% == 11807
  if %actor.varexists(zn118_thindone)%
    wait 1 s
    say I have already written in that for you.
    give journal %actor.name%
    drop journal
  elseif %actor.varexists(zn118_thinwrite)%
    wait 2 s
    emote dips a finger in her glass of wine, dragging it over the pages of the journal.
    wait 2 s
    wait 1 s
    %load% obj 11862
    say A gift...
    wait 1 s
    give egg %actor.name%
    drop egg
    wait 1 s
    say Look after it... my only child.
    wait 1 s
    say It is done.
    nop %actor.exp(10000)%
    give journal %actor.name%
    drop journal
    rdelete zn118_thinwrite %actor.id%
    set zn118_thindone 1
    remote zn118_thindone %actor.id%
  else
    wait 1 s
    say Ah, another who would seek something of me.
    wait 2 s
    say I have already lost more than I ever chose to give.
    wait 2 s
    say All but my life has departed from me... the final unmade sacrifice.
    wait 3 s
    say Make this sacrifice %actor.name%?
    wait 2 s
    say Give up your life for the truth?
    wait 1 s
    %load% obj 11860
    say Taste of the same poison as I have, and it shall be done.
    wait 1 s
    give hotdog %actor.name%
drop hotdog
    wait 1 s
    give journal %actor.name%
    drop journal
    set zn118_thinquest 1
    remote zn118_thinquest %actor.id%
  end
else
  wait 1 s
  say I don't think this is meant for me.
  drop %object.name.car%
end
~
#11874
(35) mute quest~
0 j 100
~
if %object.vnum% == 11807
  if %actor.varexists(zn118_mutedone)%
    wait 1 s
    say I have already written in that for you.
    give journal %actor.name%
    drop journal
  elseif %actor.varexists(zn118_mutewrite)%
    rdelete zn118_mutewrite %actor.id%
    set zn118_mutedone 1
    remote zn118_mutedone %actor.id%
    wait 1 s
    wait 1 s
    emote opens her mouth, words spilling out like tongues of fire into the pages of the journal.
    wait 3 s
    give journal %actor.name%
    drop journal
  else
    wait 1 s
    emote looks sadly at the journal as she runs her finger down the pages.
    wait 2 s
    emote looks back at you, gesturing toward her mouth as if there is something about her she wants you to see.
    wait 3 s
    emote closes the journal, looking hopefully into your eyes.
    wait 1 s
    give journal %actor.name%
    drop journal
  end
else
  wait 1 s
  say I don't think this is meant for me.
  drop %object.name.car%
end
~
#11875
(35) cut command allows speaking~
0 c 100
cut~
if %actor.has_item(11835)%
  %send% %actor% Using the sword of words you slice at the metal stitching in the silent girl's mouth.
  wait 4 s
  %send% %actor% The sword does not cut the stitching, but they begin to glow a bright blue.
  wait 3 s
  smile %actor.name%
  wait 1 s
  say Thank you so much.
  wait 2 s
  say I will never be free of these bonds, but you have loosed me from their evil.
  wait 3 s
  say Give me the journal and I shall add my voice to it.
  set zn118_mutewrite 1
  remote zn118_mutewrite %actor.id%
else
  %send% %actor% You have nothing powerful enough to cut through this magic.
end
~
#11876
(36) angry girl talks while fighting~
0 j 100
~
if %object.vnum% == 11807
  if %actor.varexists(zn118_angrydone)%
    wait 1 s
    say I already wrote in that for you.
    give journal %actor.name%
    drop journal
  elseif %actor.varexists(zn118_angryquest)%
    wait 1 s
    say Please %actor.name%, I only want a hug... just a hug.
    give journal %actor.name%
    drop journal
  elseif %actor.varexists(zn118_angrywrite)%
    wait 1 s
    say I'm sorry I attacked you, let me make it up with words.
    wait 2 s
    emote scrawls something carefully into the journal.
    wait 1 s
    rdelete zn118_angrywrite %actor.id%
    set zn118_angrydone 1
    remote zn118_angrydone %actor.id%
    give journal %actor.name%
    drop journal
  else
    wait 1 s
    give journal %actor.name%
    drop journal
    emote screams: I don't want anything from you!
    wait 1 s
    say What are you doing here?!
    wait 8 s
    if %actor.fighting% == %self%
      say I hate you!
      wait 8 s
      if %actor.fighting% == %self%
        emote begins to look tired.
        wait 8 s
        if %actor.fighting% == %self%
          emote starts to cry.
          wait 8 s
          if %actor.fighting% == %self%
            emote begins hitting you more weakly as she wears herself out.
            wait 8 s
            if %actor.fighting% == %self%
              emote dissolves into tears as her attack lessens.
              wait 8 s
              if %actor.fighting% == %self%
                say I can't do this any more!
                wait 2 s
                if %actor.fighting% == %self%
                  say I'm sorry... lets make it up...
                  wait 1 s
                  if %actor.fighting% == %self%
                    say Please give me a hug?
                    set zn118_angryquest 1
                    remote zn118_angryquest %actor.id%
                    %load% mob 11836
                    %purge% %self%
                  end
                end
              end
            end
          end
        end
      end
    end
  end
else
  wait 1 s
  say I don't want that.
  drop %object.name.car%
end
~
#11877
(36) angry girl attacks~
0 gh 100
~
if !%actor.varexists(zn118_angrydone)%
  if !%actor.varexists(zn118_angryquest)%
    wait 1 s
    emote screams loudly!
    wait 1 s
    kill %actor.name%
  end
end
~
#11878
(36) hug for angry girl~
0 c 100
hu~
return 0
if %self.alias% /= %arg%
  if %actor.varexists(zn118_angryquest)%
    rdelete zn118_angryquest %actor.id%
    set zn118_angrywrite 1
    remote zn118_angrywrite %actor.id%
    wait 1 s
    smile %actor.name%
    wait 1 s
    say You are not as bad as I thought you were.
    wait 2 s
    say Let me see that journal?
  elseif %actor.varexists(zn118_angrywrite)%
    wait 1 s
    say Thank you %actor.name%, let me see your journal?
  else
    emote screams loudly.
  end
end
~
#11879
(37) serpent loads tongue on death~
0 f 100
~
emote writhes in anguish as it collapses... a spiral of steam hissing from its crumpled body.
%load% obj 11863
~
#11880
(62) egg hatches into serpent~
1 f 100
~
%echo% %self.shortdesc% wriggles suddenly.
wait 5 s
%echo% %self.shortdesc% cracks loudly into two pieces and dissolves... releasing a serpent onto the ground.
%load% mob 11837
%purge% self
~
#11881
(65) pines burn into page~
1 f 100
~
%echo% %self.shortdesc% suddenly crackles and disintegrates into a dark page.
%load% obj 11866
%purge% self
~
#11882
(65) pines send burning messages~
1 b 50
~
%echo% %self.shortdesc% crackles and burns, slowly growing blacker.
~
#11883
(61) pines can be burned with candle~
1 c 7
burn~
if %cmd% == burn
  if %self.name% /= %arg%
    if %actor.has_item(11867)%
      %send% %actor% You set fire to the pines with the candle.
      %echoaround% %actor% %actor.name% sets fire to the pines with %actor.hisher% candle.
      %load% obj 11865
      %purge% self
    else
      %send% %actor% You need something to burn it with.
    end
  else
    return 0
  end
else
  return 0
end
~
#11884
(38) scarred quest~
0 j 100
~
if %object.vnum%==11866
  if %actor.varexists(zn118_scarredquest)%
    eval number %actor.zn118_scarredquest% + 1
    set zn118_scarredquest %number%
    remote zn118_scarredquest %actor.id%
    wait 1 s
    if %actor.zn118_scarredquest% >= 6
      say Thank you %actor.name%, this is the last of the pages.
      wait 2 s
      say I can finally make my book.
      %purge% page
      wait 1 s
      say Now come, let me write in yours...
      rdelete zn118_scarredquest %actor.id%
      set zn118_scarredwrite 1
      remote zn118_scarredwrite %actor.id%
    else
      say Thank you %actor.name%, this is one of the pages.. though more yet exist.
      %purge% page
    end
  else
wait 1 s
    say I don't think I asked you for this.
    give page %actor.name%
    drop page
  end
elseif %object.vnum% == 11807
  if  %actor.varexists(zn118_scarredwrite)%
    wait 1 s
    emote sinks deep into thought.
    wait 3 s
    emote writes a few words in the journal.
    wait 2 s
    say My book tells only my story, yours tells all of ours... when the story is told, the voices need not remain.
    wait 2 s
    nop %actor.exp(10000)%
    give journal %actor.name%
    drop journal   
    rdelete zn118_scarredwrite %actor.id%
    set zn118_scarreddone 1
    remote zn118_scarreddone %actor.id%
    wait 1 s
    %load% obj 11868
    say Do me one more favour?
    wait 1 s
    give book %actor.name%
    drop book
    wait 1 s
    say Put my book away somewhere? I cannot enter the house... but it belongs there.
    wait 2 s
    smile %actor.name%
  elseif %actor.varexists(zn118_scarreddone)%
    wait 1 s
    say I have already written in that for you %actor.name%.
    give journal %actor.name%
    drop journal
  elseif %actor.varexists(zn118_scarredquest)%
    wait 1 s
    say I do not have all the pages of my book yet.
    wait 1 s
    give journal %actor.name%
    drop journal
    give %object.name.car% %actor.name%
    drop %object.name.car%
  else
    wait 1 s
    emote blinks at the journal.
    wait 1 s
    say Ah you are writing a story? Our story?
    wait 1 s
    emote smiles wistfully.
    wait 1 s
    say I had a story... a secret one, but the pines came and choked it away.
    wait 3 s
    say I do not want them in my garden!
    wait 1 s
    say Please %actor.name%, burn them all down!
    wait 1 s
    %load% obj 11867
    give candle %actor.name%
    drop candle
    wait 1 s
    say You will find the pages amongst their ashes.
    wait 1 s
    say I must have my own story back before I can add to this.
    give journal %actor.name%
    drop journal
    set zn118_scarredquest 1
    remote zn118_scarredquest %actor.id%
  end
else
  wait 1 s
  say I don't think this is meant for me.
  give %object.name.car% %actor.name%
  drop %object.name.car%
end
~
#11891
(07) journal entries~
1 c 3
read~
if %self.name% /= %arg%
  if %actor.varexists(zn118_crayondone)% || %actor.varexists(zn118_gravedone)% || %actor.varexists(zn118_gravequest)%
    %send% %actor% .  I drew you the pictures, if only you'd see,
    %send% %actor% .  crayons instead of a voice.
  else
    %send% %actor% .  - - - - - - - - - - - - - - - - - - - - - - -
  end
  if %actor.varexists(zn118_birddone)% || %actor.varexists(zn118_gravedone)% || %actor.varexists(zn118_gravequest)%
    %send% %actor% .  I showed you with games I should never have played,
    %send% %actor% .  obedience rather than choice.
  else
    %send% %actor% .  - - - - - - - - - - - - - - - - - - - - - - -
  end
  %send% %actor% .    
  if %actor.varexists(zn118_ridleydone)% || %actor.varexists(zn118_gravedone)% || %actor.varexists(zn118_gravequest)%
    %send% %actor% .  Riddles to speak what couldn't be said
    %send% %actor% .  uncover the truth within
  else
    %send% %actor% .  - - - - - - - - - - - - - - - - - - - - - - -
  end
  if %actor.varexists(zn118_knifedone)% || %actor.varexists(zn118_gravedone)% || %actor.varexists(zn118_gravequest)%
    %send% %actor% .  I painted you answers enough for the whys -
    %send% %actor% .  scarlet, on a canvas of skin.
  else
    %send% %actor% .  - - - - - - - - - - - - - - - - - - - - - - -
  end
  %send% %actor% .    
  if %actor.varexists(zn118_tunneldone)% || %actor.varexists(zn118_gravedone)% || %actor.varexists(zn118_gravequest)%
    %send% %actor% .  The mind offers the chance for desperate escapes,
    %send% %actor% .  any place other than here.
  else
    %send% %actor% .  - - - - - - - - - - - - - - - - - - - - - - -
  end
  if %actor.varexists(zn118_ruthdone)% || %actor.varexists(zn118_gravedone)% || %actor.varexists(zn118_gravequest)%
    %send% %actor% .  Dark places become my mother's arms,
    %send% %actor% .  secret comfort in times of fear.
  else
    %send% %actor% .  - - - - - - - - - - - - - - - - - - - - - - -
  end
  %send% %actor% .    
  if %actor.varexists(zn118_thindone)% || %actor.varexists(zn118_gravedone)% || %actor.varexists(zn118_gravequest)%
    %send% %actor% .  Surrender means giving up all that you are,
    %send% %actor% .  forfeit if you cannot pay.
  else
    %send% %actor% .  - - - - - - - - - - - - - - - - - - - - - - -
  end  
  if %actor.varexists(zn118_mutedone)% || %actor.varexists(zn118_gravedone)% || %actor.varexists(zn118_gravequest)%
    %send% %actor% .  Silence is screaming its rage to the void,
    %send% %actor% .  run inside if you can't run away.
  else
    %send% %actor% .  - - - - - - - - - - - - - - - - - - - - - - -
  end
  %send% %actor% .    
  if %actor.varexists(zn118_runningdone)% || %actor.varexists(zn118_gravedone)% || %actor.varexists(zn118_gravequest)%
    %send% %actor% .  I showed you with actions,
    %send% %actor% .  and with words between words.
  else
    %send% %actor% .  - - - - - - - - - - - - - - - - - - - - - - -
  end 
  if %actor.varexists(zn118_angrydone)% || %actor.varexists(zn118_gravedone)% || %actor.varexists(zn118_gravequest)%
    %send% %actor% .  I lashed out and fought you,
    %send% %actor% .  as a creature that hurts.
  else
    %send% %actor% .  - - - - - - - - - - - - - - - - - - - - - - -
  end
  %send% %actor% .   
  if %actor.varexists(zn118_blinddone)% || %actor.varexists(zn118_gravedone)% || %actor.varexists(zn118_gravequest)%
    %send% %actor% .  How to show you a monster where you saw a man?
    %send% %actor% .  Tell you I hated the one that you loved?
  else
    %send% %actor% .  - - - - - - - - - - - - - - - - - - - - - - -
  end   
  if %actor.varexists(zn118_weepingdone)% || %actor.varexists(zn118_gravedone)% || %actor.varexists(zn118_gravequest)%
    %send% %actor% .  How to deny whose eyes I see in the mirror?
    %send% %actor% .  Disown what I'm bound to with blood.
  else
    %send% %actor% .  - - - - - - - - - - - - - - - - - - - - - - -
  end
  %send% %actor% .    
  if %actor.varexists(zn118_scarreddone)% || %actor.varexists(zn118_gravedone)% || %actor.varexists(zn118_gravequest)%
    %send% %actor% .  Ah time, the great healer, will pale the scars,
    %send% %actor% .  my book sealed and returned to its shelf.
  else
    %send% %actor% .  - - - - - - - - - - - - - - - - - - - - - - -
  end 
if %actor.varexists(zn118_gravedone)%
    %send% %actor% .  The ghosts shall be buried, I choose to forget,
    %send% %actor% .  and learn to forgive myself.
  else
    %send% %actor% .  - - - - - - - - - - - - - - - - - - - - - - -
  end
  %send% %actor% .    
else
  return 0
end
~
#11892
(69) corpse can be buried~
1 c 7
bury~
if %arg% == corpse
  %send% %actor% You carefully pile a mound over %self.shortdesc%.
  %echoaround% %actor% %actor.name% carefully piles a mound over %self.shortdesc%
  if %actor.varexists(zn118_gravequest)%
    rdelete zn118_gravequest %actor.id%
    set zn118_gravedone 1
    remote zn118_gravedone %actor.id%
    wait 2 s
    %send% %actor% You feel a slight chill against your shoulder as the air stirs.
    wait 3 s
    %send% %actor% The journal seems to open itself, the pages crinkling as they turn.
    wait 3 s
    %send% %actor% A cool breath wafts over the last page, condensing into frosty writing.
    wait 3 s
    %send% %actor% The wind whispers against your ear: It is done.
    wait 3 s
    %send% %actor% Suddenly you feel the surroundings blur around you... and fade away.
    %at% 11802 %echo% A young woman wanders into the shop.
    %at% 11802 %load% mob 11840
    %teleport% %actor% 11802
    wait 1 s
    %force% %actor% look
    %echoaround% %actor% %actor.name% emerges from a shadowy place in the shop.
    %force% %actor% xxtrigxx
    %purge% self
  end
else
  %send% %actor% Bury what?
end
~
#11893
(40) final quest~
0 c 100
xxtrigxx~
wait 9 s
smile %actor.name%
wait 3 s
if %actor.has_item(11807)%
  tell %actor.name% Ah! I have been looking for a journal...
  wait 3 s
  tell %actor.name% In fact, the very journal you are carrying now.
  wait 3 s
  %send% %actor% %self.name% removes an amulet from around her neck.
  wait 3 s
  tell %actor.name% This amulet can take you to a place of safety... in times of need.
  wait 4 s
  tell %actor.name% My use for it is over. I will pass it to you gladly, if you will return to me my journal.
end
~
#11894
(71) bloodstone amulet focus~
1 c 1
focus~
%send% %actor% You wrap your fingers around the bloodstone amulet and focus deeply.
%echoaround% %actor% %actor.name% wraps %actor.hisher% fingers around %actor.hisher% bloodstone amulet and closes %actor.hisher% eyes in concentration.
if %self.timer% == 0
  eval zn118_focus_return %self.room.vnum%
  remote zn118_focus_return %actor.id%
  wait 2 s
  %send% %actor% The amulet glows brightly, heating in your grasp and flickering with wisps of magic.
  %echoaround% %actor% The amulet glows brightly, flickering with wisps of magic as %actor.name% grasps it.
  wait 2 s
  %send% %actor% The amulet amplifies your focus, letting you retreat deep inside your mind.
  %echoaround% %actor% %actor.name% starts to fade, %actor.hisher% physical body flickering away.
  %teleport% %actor% 11896
  %force% %actor% xxfocushealxx
  otimer 20
else
  wait 2 s
  %send% %actor% Alas, the bloodstone stays cold, not yet fully recharged.
end
~
#11895
(40) woman exchanges amulet for journal~
0 j 100
~
if %object.vnum% == 11807
  %purge% journal
  wait 1 s
  smile %actor.name%
  wait 2 s
  tell %actor.name% Thank you so much.
  wait 2 s
  %load% obj 11871 %actor% inv
  %send% %actor% %self.name% gives you a fiery bloodstone amulet.
  wait 3 s
  tell %actor.name% You need only 'focus' to call on its powers, and 'awaken' whenever you wish to return.
  wait 4 s
  tell %actor.name% Using it will zap it of strength for a while, but its powers will return.
  wait 4 s
  tell %actor.name% Thank you... for everything.
  wait 2 s
  %echo% %self.name% leaves west.
  %teleport% %self% 11800
  rem all
  drop all
  %purge%
  %purge% %self%
else
  wait 1 s
  say I don't think this is meant for me.
  give %object.name.car% %actor.name%
  drop %object.name.car%
end
~
#11896
(05) humming bird purges after while~
0 n 100
~
wait 20 s
%echo% %self.name% finds an open window and flies away.
%purge% %self%
~
#11897
(33/62/65) sets timer on load~
1 n 100
~
wait 2 s
if %self.vnum% == 11862
  otimer 3
elseif %self.vnum% == 11865
  otimer 1
elseif %self.vnum% == 11833
  otimer 1
end
~
#11898
(56,57,58,69,70) otimer set~
1 n 100
~
otimer 3
~
#11899
(96) focus room heals~
2 c 100
xxfocushealxx~
if %cmd% == xxfocushealxx
  eval amount %actor.maxhitp%
  %damage% %actor% -%amount%
  dg_cast 'cure blind' %actor%
  dg_cast 'remove curse' %actor%
  dg_cast 'remove poison' %actor%
  %send% %actor% Your soul feels cleansed and refreshed.
else
  return 0
end
~
$~
