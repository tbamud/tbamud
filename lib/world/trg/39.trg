#3900
Gatekeeper Welcome - 3997~
0 g 33
~
emote says in a bored tone, 'Welcome to Haven, stranger.  Enjoy your stay.'
~
#3901
Mad Prisoner - 3978~
0 c 100
listen~
say Diamo... Diamo, I can't reach the pile Diamo!  Help me, please!
%force% %actor% look dungeon
~
#3902
Scream - 3975~
0 g 100
~
emote screams, 'Get out of this room RIGHT NOW!'
~
#3903
Shop A - 3973~
0 g 100
~
emote says eagerly, 'How may I help you?  Hm... You look familiar.  Perhaps I have heard of you.  Would you be %actor.name%?'
~
#3904
Shop B - 3973~
0 d 100
yes~
say Are you really now?
peer %actor.name%
say You could be... You look about like your friend described.
say Well in that case... they wanted me to give this to you.
give flagon %actor.name%
say He bought it from Gilles, at the bar.  Drink well.
wink %actor.name%
~
#3905
Tia A - 3956~
0 g 100
~
say A little bit more of...  Oh!  A customer!  How may I provide thee with service?
~
#3906
Mithroq A - 3955~
0 g 100
~
emote growls, 'What is it you want, stranger?'
~
#3908
Gilles A - 3971~
0 g 100
~
say Would you like a drink?  They are good for the thirst.
~
#3909
Milo A - 3957~
0 g 25
~
say 200789... 200790... 200791... 
emote gets a dreamy look in his eyes as he stares at his pile of glittering gold.
emote snaps to attention, finally noticing that there is a person in the room.
say Well, if it isn't %actor.name%.  Your reputation preceeds you.  Welcome to my humble bank.
bow %actor.name%
~
#3910
Yelling Woman - Not Attached~
0 gn 100
~
load obj 3952
give yelling platter
load obj 3951
give yelling pitcher
load 3912
give yelling spoon
~
#3911
Near Death Trap on the Rocks - 3975~
2 g 100
~
* Near Death Trap stuns actor
wait 6 sec
set stunned %actor.hitp% 
%damage% %actor% %stunned%
wait 2 sec
%send% %actor% You lay among the jagged rocks.
~
$~
