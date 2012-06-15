#11700
Kill Zulthan - 11704~
0 c 100
*~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if (%cmd.mudcommand% == bash || %cmd.mudcommand% == backstab || %cmd.mudcommand% == kill || %cmd.mudcommand% == hit || %cmd.mudcommand% == kick || %cmd.mudcommand% == shoot) && (zulthan /= %arg% || waiter /= %arg%)
  say Well, that was certainly a rude attempt of yours!
else
  return 0
end
~
#11701
Get ring - 11701~
2 c 100
g~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %cmd.mudcommand% == get && ring /= %arg%
  %send% %actor% You dig in the dirt and pick up the ring.
  %echoaround% %actor% %actor.name% gets a ring from the dirt.
  %load% obj 11700 %actor% inv
else
return 0
end
~
#11702
Steal cello - 11703~
2 c 100
steal~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %cmd.mudcommand% == steal && cello /= %arg%
  %send% %actor% You steal the cello from the cello player who exclaims 'Hey!'
  %echoaround% %actor% %actor.name% immaturely nabs the cello from the cello player's hands during the middle of a performance.
  %load% obj 11708 %actor% inv
end
~
#11703
Holden Greet - 11700~
0 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %actor.is_pc%
  say It is nice to see you today, %actor.name%
  if %actor.has_item(something)%
    say Hmmm, what is this? This looks like Blood from the trees! This must be Lacela's doing, and now I finally have proof! Thank you, %actor.name%!
    %echo% %self.name% drags Lacela out of her shop and throws her out of town for such a terrible act! Holden takes the key that Lacela has from her!
    %load% obj 11718
    say I don't need this, here, you have it.
    give cellar %actor.name%
    %at% 11706 %purge% lacela
    %purge% %actor.inventory(11719)%
  end
  if !%actor.has_item(something)%
  end
end
~
#11704
Stradler Greet - 11701~
0 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %actor.is_pc%
  say It is great to see that you are well today, %actor.name%
end
~
#11705
Use cello - 11708~
1 c 2
use~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %cmd.mudcommand% == use && cello /= %arg%
  %echoaround% %actor% %actor.name% plays The Journey in C minor on %actor.hisher% cello.
  %send% %actor% You beautifully play The Journey in C Minor with your cello.
else
  return 0
end
~
#11706
LacelaKillProof - 11705~
0 c 100
*~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if (%cmd.mudcommand% == bash || %cmd.mudcommand% == backstab || %cmd.mudcommand% == kill || %cmd.mudcommand% == hit || %cmd.mudcommand% == kick || %cmd.mudcommand% == shoot) && (cassandra /= %arg% || lacela /= %arg%)
  say Heyyyyyyy!!!! Whattad I ever doo ta yooo??!! *Hic*
else
  return 0
end
~
#11707
Can't attack Derresor - 11707~
0 c 100
*~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if (%cmd.mudcommand% == bash || %cmd.mudcommand% == backstab || %cmd.mudcommand% == kill || %cmd.mudcommand% == hit || %cmd.mudcommand% == kick || %cmd.mudcommand% == shoot) && (derresor /= %arg% || salesman /= %arg% || maosund /= %arg%)
  say Sorry %actor.name%, maybe at some other point.
else
  return 0
end
~
#11708
Large Boots - 11722~
1 j 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
return 0
if %actor.eq(legs)%
  %send% %actor% %self.shortdesc% cover your entire legs. You must remove  %actor.eq(legs).shortdesc% to wear them.
else
  %send% %actor% You wear %self.shortdesc% around your legs.
  %send% %actor% You wear %self.shortdesc% around your feet.
  %load% obj 11722 %actor% legs
  %load% obj 11722 %actor% feet
  %purge% %self%
end
~
#11709
Large Boots - 11722~
1 l 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
%purge% %actor.eq(legs)%
~
#11710
Stump - 11753~
2 c 100
sit~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %cmd.mudcommand% == sit && stump /= %arg%
  %echo% You sit down on the stump and suddenly the forest in front of you opens!
  %echoaround% %actor% %actor.name% sits down on a stump and the forest that blocked the path suddenly opens up!
  %door% 11753 south flags a
else
  return 0
end
~
#11711
touch display - 11751~
2 c 100
touch~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %arg% == display
  %send% %actor% You browse around the forbidden display and lift up the large sword. A clicking noise is made from the stand it is on, and suddenly, a huge crashing sound comes from the far west!
  %echoaround% %actor% %actor.name% lifts the sword on the display and a gigantic crashing sound comes from the west!
  %at% 11704 %door% 11704 down flags a
  %at% 11758 %load% obj 11737
  %send% %actor% Salesman Derresor Maosund punches you in the face with anger for touching the display!!
  %damage% %actor% 150
else
  return 0
end
~
#11712
Bow - 11727~
1 c 1
shoot~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %actor.fighting% && !%arg%
  set arg %actor.fighting%
end
if !%arg% 
  %send% %actor% Shoot Who?
  halt
else
  %force% %arg% kill %actor.name%
end
if (%arg.room% != %actor.room%) || (%arg.id% == %actor.id%)
  %send% %actor% Shoot: Invalid Target!
  halt
end
eval i %actor.inventory%
while (%i%)
  set next %i.next_in_list%
  if %i.vnum%==11729
    set quiver 1
  break
end
set i %next%
done
if %quiver%
  %force% %actor% take arrow quiver
end  
if %actor.inventory(11728)%
  %damage% %arg% 10
  %send% %actor% You fire your arrow at your opponent.
  %purge% %actor.inventory(11728)%
  switch %random.4%
    case 1
      %echo% The arrow breaks!
      return 0
    break
    case 2
      load obj 11728
    break
    case 3
      load obj 11728
    break
    default
      load obj 11728
    break
  done
else
  %send% %actor% You need to have arrows or a quiver.
  return 0
end
if %actor.inventory(11730)%
  %damage% %arg% 30
  %send% %actor% 	WThe arrow makes a bright flash of light as it strikes your opponent!	n
  %purge% %actor.inventory(11730)%
  switch %random.4%
    case 1
      %echo% The arrow breaks!
      return 0
    break
    case 2
      %load% obj 11730
    break
    case 3
      %load% obj 11730
    break
    default
      %load% object 11730
    break
  done
else
  %send% %actor% You fire your arrow at %arg.shortdesc%.
end
~
#11713
Touch Nest - 11755~
2 c 100
touch~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %arg% == nest
  %send% %actor% You reach out for the squirrel's nest and touch it, but it falls toward the patch of sand and abruptly falls right through!
  %echoaround% %actor% %actor.name% tries to reach out and touch the nest, like the 2-year-old %actor.heshe% is. %actor.heshe% drops the nest clumsily but the nest abruptly falls through the strange patch of sand!
  %door% 11755 down flags a
else
  %send% %actor% Touch what?
  return 0
end
~
#11714
Chieftan Attacks You - 11709~
0 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
wait 2 s
say Hey! What are you doing in here!?!? Get out or you'll pay with your life!
wait 2 s
say I warned you!!
kill %actor.name%
~
#11715
Sit Stump - 11722~
2 c 100
sit~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %cmd.mudcommand% == sit && %arg% /= stump
  %echo% You sit down on the large stump and take a rest.
  %echoaround% %actor% %actor.name% sits down on a stump and takes a quick rest.
  return 0
end
~
#11716
pick flowers - 11735~
2 c 100
pick~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %arg% == flowers
  %send% %actor% You kneel beside the path, pick the flowers, and arrange them in a nice bouquet.
  %echoaround% %actor% %actor.name% picks the exotic flowers beside the path and makes a bouquet from them.
  %load% obj 11735 %actor% inv
else
  %send% %actor% Pick what??
  return 0
end
~
#11717
Search fireplace- 11725~
2 c 100
search~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %arg% == fireplace
  %send% %actor% You search around the fireplace, and bump into the loose tile. Suddenly, the fire goes out, and the wall behind it opens up!
  %echoaround% %actor% %actor.name% searches the fireplace, and before you know it, the wall to the north opens like a door!
  %door% 11725 north flags a
else
  %send% %actor% Search what?
  return 0
end
~
#11718
Wake Lesalie - 11706~
0 c 100
wake~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %cmd.mudcommand% == wake && arg /= government || official || lesalie
  %echo% %self.name% stands up.
  wait 1s
  %echo% %self.name% says "Oh... Where am I...? Lacela! Her blood! I must stop her!"
  %echo% %self.name% tries to get up with what little strength he has, but falls back down moaning.
  wait 1s
  %send% %actor% You calm him down and inform him that you took care of Lacela's scheme.
  %echoaround% %actor% %actor.name% calms Lesalie and informs him of what has happened.
  wait 1s
  %echo% %self.name% sighs with relief.
  %echo% %self.name% says "Thank you very much, now, if you excuse me, I must slowly make my way up to my room."
  %echo% %self.name% smiles at %actor.name%.
  wait 1s
  %echo% %self.name% says "By the way, my letter is S... I know what you're trying to do."
  %echo% %self.name% wobbles up the stairs and makes way to his room.
  %teleport% lesalie 11709
else
  return 0
end
~
#11719
Verno Guard North - 11718~
0 q 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %direction% == up
  %send% %actor% %self.name% steps in front of you, raising an eyebrow.
  %echoaround% %actor% %actor.name% tries to leave north, only to be foiled by the bodyguard standing right in front of %actor.himher%.
  return 0
end
~
#11720
Password - 11718~
0 d 0
barbatsis~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
%send% %actor% %self.name% stands aside and lets you pass.
%send% %actor% You make your way up the marvelous ivory staircase!
%teleport% %actor% 11719
~
#11721
say 'hello' -11717~
0 d 1
hello~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
%send% %actor% %self.name% looks up at you startled.
wait 1s
%send% %actor% %self.name% says: Please help me! The council is becoming more corrupt by the minute and our beautiful community will be ruined!
wait 2s
%send% %actor% You nod.
wait 2s
%send% %actor% %self.name% says: Thank you! Now, all you have to do is convince the Empress that the council is being corrupt! The only ways to get to her are either to kill her bodyguard, and believe me, youre not ready for that.
wait 2s
%send% %actor% %self.name% says: Or you can find the password to get into the Empresss Chamber. Every Official has one letter of the nine letter password memorized. You must track down the hidden officials and get the letter from them.
wait 2s
%send% %actor% %self.name% says: I know that the letters of the people still here are recorded on something that all of the councilors have in their office Try searching whatever that may be. 
wait 2s
%send% %actor% %self.name% says: The letters will also be out of order. You must find the document that says: what the word is once you get all the letters. Im sure the missing Officials will give you a subtle hint when you find them.
%send% %actor% %self.name% says: Then, speak the password in front of Verno, the guard, and you know how to take it from there! Good luck! By the way, my letter is A.
~
#11722
Search papers - 11718~
2 c 100
search~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %arg% == papers
  %send% %actor% You search the papers and find one marked B.
else
  return 0
end
~
#11723
Search Papers 11717 + more :P~
2 c 100
search~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %arg% == papers
  %send% %actor% You search the papers but find none with a letter!
else
  return 0
end
~
#11724
Seach Papers - 11713~
2 c 100
search~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %arg% == papers
  %send% %actor% You search the papers and find one marked T.
else
  return 0
end
~
#11725
Search Papers - 11710~
2 c 100
search~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %arg% == papers
  %send% %actor% You search the papers and find one marked S.
else
  return 0
end
~
#11726
Search Papers - 11708~
2 c 100
search~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %arg% == papers
  %send% %actor% You search the papers and find one marked I.
else
  return 0
end
~
#11727
Give Ring - 11723~
0 j 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
If %object.vnum% == 11700
  %purge% %object%
  say Oh, thank you, now I can finally go back!
  %echo% %self.name% Runs out of the tower screaming with joy!
  %echo% Suddenly, the pew moves over to the lleft, revealing a hole!
  wait 2s
  %echo% The Government Official Johanes steps out from under the pew!
  %echo% Johanes says: Ah, thank you. I would have never gotten out from under there if that stupid man had stayed. Oh, I know what youre trying to do, I can tell. My letter is B.
  %echo% Joohanes says: Oh, and I know that the password is somewhere near thewarmest place in the city.
  %echo% Johanes leaves the tower without telling you where he is going!
  %purge% %self%
else
  return 0
end
~
#11728
Comfort Man 11703(cheater trigger :P)~
2 c 100
comfort~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %cmd.mudcommand% == comfort && man =/ %arg%
  %send% %sctor% The man says: Oh dear, can you help me? I've lost a ring, my wedding ring! Please, find it for me!
else
  return 0
end
~
#11729
Wake Visconti - 11710~
0 c 100
wake~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %cmd.mudcommand% == wake && arg /= government || official || visconti
  %echo% %self.name% sits up.
  wait 1s
  %echo% %self.name% says "Thank you for saving me I must admit I didnt think I would have made it out alive.
  %echo% %self.name% stands up.
  wait 1s
  %echo% %self.name% says "I am in your debt. All I can reward you with is that my letter is R. Plus, I only know that the password order is located far away from the Empress."
  %echo% %self.name% smiles at %actor.name%.
  wait 1s
  %echo% %self.name% escapes and heads towards his office.
  %teleport% visconti 11717
else
  return 0
end
~
#11730
Greeting - 11712~
0 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %actor.is_pc%
  say Oh, yay, yay, yay! Thank you! Thank you! My letter is A, Yay! Cozy chairs! Yay!!!
  %echo% %self.name% runs out the hole and up to her office.
  %teleport% %self% 11712
end
~
#11731
Quest Finale - 11719~
0 g 100
~
* This trigger has been exported 'as is'. This means that vnums
* in this file are not changed, and will have to be edited by hand.
* This zone was number 117 on The Builder Academy, so you
* should be looking for 117xx, where xx is 00-99.
if %actor.is_pc%
  say hello.
  wait 1s
  say have you something to say to me?
  wait 2s
  %send% %actor% You tell Empress Leraillia what the council has been doing, and you want it to stop.
  wait 1s
  say I see
  wait 1s
  say well, since you went to the trouble to find my council, I will give you this reward.
  %send% %actor% %self.name% gives you a small stone.
  %load% obj 11730 %actor% inv
  say Regrettably, your request must be denied.
  Wait 1s
  %echo% Suddenly, the doors slam shut behing you!
  %door% 11720 south flags d
  wait 1s
  %echo% %self.name% presses a button on her desk, and suddenly, screams of agony come from the council chambers!
  %at% 11716 %purge%
  %at% 11717 %purge%
  %at% 11712 %purge%
  %at% 11709 %purge%
  wait 2s
  say It seems to me That my council has failed to serve me right
  Wait 1s
  Say This city is now mine. Those who oppose me will be vanquished!
  Wait 2s
  %echo% %self.name% pulls out a gigantic claw that she kept hidden before.
  Wait 2s
  Say You will be the first. Good bye, %actor.name%!
  %echo% %self.name% lunges at you, aiming to slice you in two!
  Wait 1s
  %echo% suddenly, the door opens, and a bright flash of light consumes the entire room, blinding both you and %self.name%!
  Wait 3s
  %send% %actor% You wake up at the entrance to Los Torres, and everything seems normal for some reason. What the heck happened??
  %send% %actor% 	g+	n	bQUEST COMPLETE	n	g+	n
  %teleport% %actor% 11701
end
~
$~
