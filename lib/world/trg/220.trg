#22000
The Spaghetti Greet - attached to mob 22006~
0 g 100
~
if (%direction% == down)
  say Hey!
  say You pulled my noodle!
  kill %actor.name%
else
end
~
#22001
The Snuffins Greet - attached to mob 22001~
0 g 100
~
wait 3 sec
  say Oh, hello there. My name is Snuffins.
  say Take a look around, I have many maps to look at. I used to travel all the time...
wait 2 sec
  sigh
  say That is, until I met Oreo. That horrible cat chases me everytime I try to leave this place!
wait 2 sec
  say Can you help me? I'll give you my most prized possession if you do.
wait 1 sec
say Erm, 'dispose' of Oreo and bring me her collar!
~
#22002
Snuffins Happy - attached to mob 22001~
0 j 100
~
* check to see if its the collar
if %object.vnum%==22001
  wait 1
  say Thank you very much, stranger!
smile
give cheese %actor.name%
  wait 5
  junk collar
else
  * bastards. Don't mess with Snuffins.
  say This isn't Oreo's collar! Bring me her collar!
  return 0
end
~
#22003
The Sandwich Greet - attached to mob 22007~
0 g 100
~
wait 1 sec
mumble
say ...they tell stories of a fantastic monster...
mumble
wait 1 sec
say ...but I wouldn't know...I can't even move...
mumble
wait 1 sec
say ...they say he's somewhere really high and cold...but I wouldn't know...
mumble
~
#22004
The Heat Wave - attached to room 22022~
2 g 100
~
%send% %actor% @DThe intense @Rheat @Dscalds your skin!@n
%echoaround% %actor% @w%actor.name% is @rburnt @Dfrom the intense @Dheat.@n
%damage% %actor% 3
~
$~
