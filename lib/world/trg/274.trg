#27400
Kay~
0 dg 100
~
wait 5
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
#27401
Barney - 27404~
0 g 100
~
if %actor.is_pc%
  wait 2 sec
  switch %random.5%
    case 1
      say Yea, we're so happy!
    break
    case 2
      say Will you be my friend?
    break
    case 3
      say C'mon kids sing along...
    break
    case 4
      say Happy, happy, happy, aren't we all SO happy!
    break
    case 5
      say I love you, you love me, ...
    break
    default
      sing
    break
  done
end
~
#27402
Hanz~
0 g 100
~
say Hi I'm Hanz....
wait 10
say lets get yawu puhmped up!
strut
clap
flex
~
#27403
Franz~
0 g 100
~
say hi I'm Franz...
wait 10
say lets get yawu puhmped up!
strut
clap
flex
~
#27404
Greetings St Bigid mob~
0 g 25
~
if %actor.is_pc%
   say Welcome to our village, %actor.name%!
else
 say Blessings upon you this day.
end
~
#27405
Broderick~
0 g 100
~
if %actor.is_pc%
Say Welcome to Saint Brigid, the hand of the Lady be upon thee.
wait 10
nod
say To view my wares type 'List' to show everything i have in stock.
say to buy, you correspond by name of item, Buy 'Item' or buy #
say to sell, we accept what the shop designates i'm afraid.
nod
nod
wait 5
say So What'll it be?
else
  Say Blessings upon you this Day!
end
~
#27406
Child mob st brigid~
0 g 50
~
* By Whiteknight of The Builder Academy    tbamud.com 9091
* With random triggers ACTOR is NOT defined. So set it.
set actor %random.char%
switch %random.4%
    case 1
      Say MommY!!! Daday!!!
      if %actor.sex% = man
       say its a mean old man!
      else
       say its a mean witch!
       done
      done
      %echo% the child starts crying!
    break
    case 2
    %echo% the child starts crying!
    break
    case 3
    %echo% the child starts freaking out
    %echo% the woman starts to scream!
    break
    default
      say Greetings and welcome to our home, %Actor.name%!
      hug %actor.name%
      say Please be welcome here!
    break
end
~
#27407
Std greeting shop mobs~
0 g 100
~
Say Welcome to Saint Brigid, the hand of the Lady be upon thee.
wait 10
nod
say To view my wares type 'List' to show everything i have in stock.
say to buy, you correspond by name of item, Buy 'Item' or buy #
say to sell, we accept what the shop designates i'm afraid.
nod
nod
wait 5
say So What'll it be?
~
#27408
Hookers~
0 g 35
~
* By Rumble of The Builder Academy    tbamud.com 9091
if %actor.is_pc%
   if %actor.sex% == male
    say Hey baby, how bout a little action and some
    say small bit of fun~ I am all that.
  elseif %actor.sex% == female
    SAY Get lost you bitch... I am not into Women!
    say what a whacko pervert!
  else
Say Sotrry I'm not into Neuterless ones! 
  end
else
  say Take off you Mob Swine!
end
~
$~
