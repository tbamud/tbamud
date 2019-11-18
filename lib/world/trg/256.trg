#25600
Firetender msg~
0 b 5
~
emote pokes at the coals in the fire with an iron rod.
~
#25601
Firetender msg II~
0 b 5
~
emote grabs a few pieces of wood from a pile in the corner of the room and adds it to the fire.
~
#25602
Firetender Greet Female~
0 g 100
~
if (%actor.sex% == MALE)
  wait 1 s
  say Oh my! But you cannot be in here! Ladies only, m'lord!
else
  wait 1 s
  say A Bath, m'lady?
end
~
#25603
Firetender Greet Male~
0 g 100
~
if (%actor.sex% == FEMALE)
  wait 1 s
  emote stares at you with round eyes, aghast.
  wait 1 s
  say M'lady! Thou canst not enter this chamber for fear of spoiling thy virtue!
  wait 1 s
  say I beg thee, m'lady - please avert thine eyes and leave!
else if (%actor.sex% == MALE)
  wait 1 s
  say A bath for m'lord?
end
~
#25604
Maid Greet~
0 g 15
~
wait 1 s
emote lowers her eyes and offers a quick curtsy as she busies herself with her work.
~
#25605
Servant Secret~
0 g 10
~
if %actor.is_pc%
  wait 1 s
  emote looks both ways before turning to you quickly.
  wait 1 s
  say Shh... the Lord does not tolerate dawdling service. At least not since the Lady Penelope was taken for ransom by Mordecai.
end
~
#25606
Lord's Guest Goose~
0 b 5
~
mecho You feel a sudden pain, a sharp pinch on your butt-cheek.
wait 1 s
mecho You look around the room, but the only other person you see is the Lord's guest
mecho and he's facing the away from you, studying a portrait on the wall.
wait 1 s
mecho What in the heck could that have been?
~
#25607
Lord's Guest Leave Room~
0 q 7
~
if (%actor.sex% == MALE)
emote gives you a sly wink as he leaves the room.
end
~
#25608
Kitchen Hustle-Bustle I~
0 b 5
~
mecho A servant hurries in, grabs up a platter of vegetables and hurries back out.
~
#25609
Kitchen Hustle-Bustle II~
0 b 5
~
mecho The cook grabs a pan from a ceiling hook and begins filling it with water.
~
#25610
Kitchen Hustle-Bustle III~
0 b 5
~
mecho A servant shuffles in quickly, grabs a small hunk of cheese for himself, then leaves just as quickly as he came.
~
#25611
Kitchen Hustle-Bustle IV~
0 b 5
~
mecho One of the cooks throws a chunk of wood on the fire.
~
#25612
Kitchen Hustle-Bustle V~
0 b 5
~
emote whistles a merry tune while slicing a carrot.
~
$~
