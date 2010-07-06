#26700
Harem Master - 26706~
0 b 5
~
emote looks at his minions and growls, 'get to work, bitches!'
~
#26701
Slaves Bow - 26701-26704~
0 e 0
A fat harem master looks at his minions and growls, 'get to work, bitches!'~
wait 1 sec
bow master
~
#26702
Master Needs Assist - 26706~
0 k 10
~
emote screams loudly, 'HELP ME, MY SLAVES!'
~
#26703
Slaves Assist Master - 26701-26704~
0 e 0
A fat harem master screams loudly, 'HELP ME, MY SLAVES!'~
wait %random.6% sec
assist master
~
#26705
Fear Spell - 26710~
0 k 5
~
emote utters the words, 'pabrow'.
%force% %actor% flee
~
#26706
Butcher - 26713~
0 b 10
~
emote begins sharpening %self.hisher% knife.
~
#26707
Tar Baby Follow - 26708~
0 b 10
~
set actor %random.char%
if %actor.is_pc%
  mfollow %actor%
  say Ho ho, hee hee, %actor.name% you are sooo funneeee!
  %send% %actor% %self.name% looks at you with the cutest expression.
  %echoaround% %actor% %self.name% looks at %actor.name% with the cutest expression.
end
~
#26708
Grand Inquisitor - 26719~
0 k 100
5~
switch %random.2%
  case 1
    emote utters the words, 'ordalaba'.
    dg_cast 'energy drain' %actor%
  break
  case 2
    emote evilly grins and snaps his hands.
    dg_cast 'animate dead' corpse
    order followers assist
  break
  default
  break
done
~
#26709
High Priest of Terror - 26714~
0 k 5
~
switch %random.2%
  case 1
    emote utters the words, 'ordalaba'.
    dg_cast 'charm' %actor%
  break
  case 2
    emote waves $s hands in a swirling motion.
    dg_cast 'earthquake'
  break
  default
  break
done
~
#26710
Near Death Trap - 26744~
2 g 100
~
* Near Death Trap stuns actor
wait 3 sec
set stunned %actor.hitp% 
%damage% %actor% %stunned%
wait 3 sec
%send% %actor% The Gods allow your puny existence to continue.
~
$~
