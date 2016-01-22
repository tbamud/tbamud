#34300
Barney script~
0 b 100
~
* By whiteknight
eval number %random.16%
switch %number%
  case 0
    Say I love you......
    wait 2
    say you love me.....
    wait 2
    say We're best friends as friends should be.
    wait 2
  break
  case 1
     hug %actor.name%
     wait 5
     kiss %actor.name%
     wait 5
     say And remember, I love you!
     laugh
  break
  case 2
     say Let us be best-tus buddies in all the world!
     wait 5
     dance
  break
  case 3
      say Hello again to all my friends I'm
      say glad you came to play. Our fun
      say and learning never ends. Here's
      say what we did today.
  break
  case 4
      say If I lived under the sea-sea-sea,
      say I think it would be neat as it
      say could be-be-be. I could visit all
      say the fish, anytime I wish, if I
      say lived under the sea.
  break
  case 5
      say I like to eat...eat..eat...apples and bananas
  break
  case 6
      say What a super-deeeee-duper idea!
  break
  case 7
      say "Why, that's a stuuuuuuuu-pendous idea!"
  break
  case 8
      say With a great big hug and a kiss from me to you..
      wait 2
      kiss %actor.name%
      wait 2
      say won't you say you love me too?
  break
  case 9
      say I Love you. %actor.name%
      say won't you be my friend?
  break
  case 10
      say I love you...
      wait 2
      say you love me....
      wait 2
      say We're best friends...
      wait 2
      say as friends should be...
  break
  case 11
      say I love you.... You love meeee...
      say we'e a happy family!
  break
  case 12
      say My blankey lies over the ocean,
      say my blankey lies over the sea,
      say my blankey lies over the ocean,
      say so bring back my blankey to me.
  break
  case 13
      say ... And Remember now kids. I luv yew!!!!
      say ... and remember I luv yew!
  break
  case 15
      say Shimboree shimborah. Shimboree shimborah!
  break
  case 16
      say Oh boy!
      wait 2
      say Just look at all the good things to eat.
      wait 2
      say I think it would take a dinosaur to eat all of that.
  break
  default
    * this should be here, even if it's never reached
      say Come and play... Come and sing with me!
      sing
      say Come and dance with me, %actor.name%!
      dance %actor.name%
break
done
*
~
#34301
Joe~
0 hi 100
~
wait 15
say Hiya %actor.name% How can I assist you?
sneer %actor.name%
wait 5
nod %actor.name%
say type 'List' and I'll show you what is on the Menu at the Coffee Bar...
snort
wait 5
nod %actor.name%
say also remember to get a drink the glasses are IN the cabinet!
wait 5
picknose
say Bottles of Ale, Firebreather, Whiskey, and Local
say Specialty to Midguard also available on shelf.
nod %actor.name%
smile
drool
snigger
~
#34302
Drunk spirit text~
0 b 100
~
* By whiteknight
eval number %random.6%
switch %number%
case 0
Say S-set me up Kenny... 
wait 2
say A-an-another rwound here... 
wait 2
emote hics
wait 2
break
case 1
hug %actor.name%
wait 5
kiss %actor.name%
wait 5
say You're so cute!
laugh
break
case 2
say Let us be best-tus buddies in all the world!
wait 5
dance
break
case 3
say 99 bottles of beer on the wall....
say 99 bottles of beer... 
say take one down, pass it around....
say 98 bottles of beer on the wall..,
break
case 4
say Oh i think i am going to be sick...
emote turns green
wait 2
swoon
wait 2
barf
say ugh ahhhhhhhhhh.....
break
case 5
say Gimmie a bottle of anything and a glazed donut!
break
case 6
~
#34303
Imm Check~
0 gi 100
~
if %actor.level% > 30
say GREETINGS HOLINESS!
bow
else
say =====================================================
say Hrmmmph. This area mortal is not off-limits, but please go 
say to your destination quickly. Once completed, DO NOT LINGER!
say ------------------------------------------------------
nod %actor.name%
grimace %actor.name%
say -----------------------------------------------------
say Return to the Social Gathering Room to return to Midguaard
say We have our eye on you!
say =====================================================
end
~
#34399
Heal Script~
0 g 100
~
say Blessings upon you this day, %actor.name%. The hand of God
say be upon you always. Here let me heal and help you.
dg_cast 'heal' %actor.name%
wait 5
dg_cast 'sanc' %actor.name%
wait 5
dg_cast 'fly' %actor.name%
wait 5
say Go with God.
~
$~
