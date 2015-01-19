#21100
Test~
0 g 100
~
%echo% This trigger commandlist is not complete!
~
#21101
Load Cards~
0 d 100
*~
set zone 211
set start %self.room.vnum%
eval room %self.room.vnum% + 1
switch %speech.car%
  case shuffle
    if %self.varexists(Cards_Dealt)%
      %echo% 	n  The voice is in your mind again.
      %echo% 	c    'I'm sorry, the cards seem to be already laid out.'	n
      halt
    else
      set deck 78
      set layout 10
      global deck
      global layout
      set var %zone%01
      emote shuffles the cards.
      %echo% 	n  %self.name% seems to speak directly to your mind.
      %echo% 	c    'Keep shuffling until you feel the deck understands your question.	n
      %echo% 	c   When you're ready, say DEAL.'	n
      set Deck_Shuffled 1
      global Deck_Shuffled
      while %var% < %zone%79
        set %var% 1
        remote %var% %self.id%
        eval var %var% + 1
      done
      halt
    end
  end
  case deal
  if !%self.varexists(Deck_Shuffled)%
    %echo% 	n  The voice is in your mind again.
    %echo% 	c    'The cards don't seem to understand your question yet.  Have you 	n
    %echo% 	c   SHUFFLEd?'	n
    halt
  elseif %self.varexists(Cards_Dealt)%
    %echo% 	n  The voice is in your mind again.	n
    %echo% 	c    'I'm sorry, the cards seem to be already laid out.'	n
    halt
  else
    emote starts to lay out the cards.
    %echo% 	n  The voice seems to surround you now.
    %echo% 	c    'When you're ready, please go up to start your reading.  Once you	n  
    %echo% 	c   start, you won't be able to come back.  Of course, you can always	n 
    %echo% 	c   come back for another reading.	n 
    wait 2 sec
    %echo% 	c    At each room, LOOK CARD to see the meaning.  Reverse means	n 
    %echo% 	c   that the card laid out upsidedown which changes the meaning.	n 
    %echo% 	c   Don't worry about it.  The card will show the reversed meaning.	n 
    %echo% 	c   The room name will explain what the placement of the card means.'	n 
    wait 1 sec
    %door% %self.room.vnum% up flags a
    emote opens the door to the stairway.
    while %layout%
      set zonebase %zone%00
      eval card %random.78% + %zonebase%
      eval temp %%self.varexists(%card%)%%
      eval hascard %temp%
      if %hascard%
        mgoto %room% 
        set rand %random.2%
        if %rand% == 1
          %load% obj %zone%99
        end
        %load% obj %card%
        mgoto %start%
        rdelete %card% %self.id%
        eval deck %deck% - 1
        eval layout %layout% - 1
        eval room %room% + 1
        global deck
        global layout
        set Cards_Dealt 1
        global Cards_Dealt
      else   
      end
    done
    halt
  break
  default
  break
end
~
#21102
Look Card~
2 c 100
*~
* Parnassus' Special Anti-Freeze Formula
if %cmd.mudcommand% == nohassle
  return 0
  halt
end
*
set zone 211
if %self.vnum% > %zone%10 && %self.vnum% < %zone%24
  * if %self.vnum% > %zone%00 && %self.vnum% < %zone%24
  set cmdroom %zone%20
elseif %self.vnum% > %zone%30 && %self.vnum% < %zone%44
  set cmdroom %zone%40
elseif %self.vnum% > %zone%45 && %self.vnum% < %zone%64
  set cmdroom %zone%60
else
  return 0
  halt
end
if %cmd.mudcommand% == look || %cmd.mudcommand% == examine
  * Look Trigger Written by Fizban - June 06 2013
  * This trigger changes the meaning of the card for reverse.
  * If there is no argument, just look.
  if !%arg%
    return 0
    halt
  else
    * Check for the reverser.  If it is in the room, give
    *  one meaning.  If it is not, give the other.
    eval rev %%findobj.%self.vnum%(%zone%99)%%
    * The ~ anchors the comparison to the front of the word.
    * rd /= card but ~rd is not a part of ~card while ~c is.
    set arg ~%arg%
    if ~reverse /= %arg%
      if %rev% < 1
        %send% %actor% You do not see that here.
        halt
      else
        %send% %actor% The card is upside down.
        halt
      end
    end
    if ~card /= %arg%
      if %rev% < 1
        %force% %actor% look card
      else
        %force% %actor% look reverse
      end
    else
      return 0
    end
  end
elseif %cmd.mudcommand% == quit || %cmd.mudcommand% == afk
  %send% %actor% Because you have decided to %cmd.mudcommand%, you cannot finish the reading.
  %echoaround% %actor% %actor.name% has to leave the reading now.
  wait 1 sec
  %send% %actor% You are magically sent to the end of the reading.
  %echoaround% %actor% %actor.name% is whisked away in a puff of smoke.
  wait 1 sec
  %teleport% %actor% %cmdroom%
  wait 1 sec
  %at% %cmdroom% %force% %actor% down
  wait 1 sec
  %at% %actor% %force% %actor% %cmd% %arg%
  wait 1 sec
  halt
elseif %cmd% == return || %cmd% == recall || %cmd% == teleport || %cmd.mudcommand% == goto
  %send% %actor% Because you have decided to %cmd%, you cannot finish the reading.
  %echoaround% %actor% %actor.name% has to leave the reading now.
  wait 1 sec
  %send% %actor% You are magically sent to the end of the reading.
  %echoaround% %actor% %actor.name% is whisked away in a puff of smoke.
  wait 1 sec
  %teleport% %actor% %cmdroom%
  wait 1 sec
  %at% %cmdroom% %force% %actor% down
  wait 1 sec
  %at% %actor% %force% %actor% %cmd% %arg%
  wait 1 sec
  halt
else
  return 0
end
~
#21103
Clear the Cards - r21120, r21140, r21160~
2 q 100
~
* Clears cards from the reading and reader marker
* when player finishes the reading.
if %direction% == down
  wait 2 sec
  %purge%
  set room %self.vnum%
  eval purgeroom %room% - 10
  %door% %purgeroom% up flags abcd
  eval purgeroom %purgeroom% + 1
  while %purgeroom% < %self.vnum%
    %at% %purgeroom% %purge%
    eval purgeroom %purgeroom% + 1
  done
  eval purgeroom %purgeroom% + 5
  %at% %purgeroom% %purge%
end
~
#21104
Reset the Fortuneteller~
0 q 100
~
* Clears the fortune-teller for the next player.
* Closes the doors coming back to make it easier to see the path while
*   keeping the possibility of checking past cards.
if %direction% == up
  * set deck 78
  set layout 10
  * global deck
  global layout
  rdelete Deck_Shuffled %self.id%
  rdelete Cards_Dealt %self.id%
  rdelete tarot_reading_started %self.id%
  if %self.room.vnum% == 21110 || %self.room.vnum% == 21130 || %self.room.vnum% == 21150
    * %door% %self.room.vnum% up flags abc
    eval cardroom %self.room.vnum% + 2
    %door% %cardroom% down flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% north flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% south flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% west flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% north flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% west flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% south flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% south flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% south flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% east flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% south flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% west flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% north flags ab
  end
end
~
#21105
Dealer Greets~
0 h 100
~
wait 2 sec
%echo% 	n  The voice of %self.name% seems to fill your head.
%echo% 	c     'Ahh, you have something on your mind? Let us see what the	n
%echo% 	c   cards have to say.  Unfortunately, you cannot hold or shuffle	n
%echo% 	c   my cards, but concentrate on your question and say shuffle.	n  
%echo% 	c   When you feel that the cards know your question, say deal and	n
%echo% 	c   I shall lay out the cards for you to examine.	n
wait 3 sec
%echo% 	c     Usually I would interpret the cards for you, but that is	n 
%echo% 	c   forbidden me in this space and time.  All I am allowed is to	n 
%echo% 	c   show you the cards and you must decide their meanings in your	n 
%echo% 	c   own mind. Move from card to card.  Each space and each card	n 
%echo% 	c   will explain itself to you. 'LOOK CARD' in each room to see	n 
%echo% 	c   the explanation. These are very simplified meanings so they	n 
%echo% 	c   are very general.	n  
wait 3 sec
%echo% 	c     Remember, this is just a game and should not be taken	n
%echo% 	c   seriously any more than you would run your life by newspaper	n
%echo% 	c   horoscopes or slips of paper from fortune-cookies.	n
wait 2 sec
%echo% 	c     When you're ready, start by saying SHUFFLE.'	n
~
#21106
Receptionist juggles appointments - M21104~
0 d 100
*~
set zone 211
* set findobj 0
* Checks for available readers
* Kicks out people that are afk, etc
if %self.room.vnum% != %zone%02
  emote looks around in confusion.
  say I'm sorry.  I have to get to my office.
  emote leaves.
  eval findmob %%findmob.%zone%02(%self.vnum%)%%
  if %findmob% > 0
    %purge% %self%
  else
    mgoto %zone%02
    halt
  end
end
if %actor% == %self% 
  halt
end
* This loop goes through the entire string of words the actor says. .car is the
* word and .cdr is the remaining string. 
eval word %speech.car%
eval rest %speech.cdr%
while %word%
  *   %echo% Word: %word%
  *   %echo% rest: %rest%
  switch %word%
    * Appointment starts the conversation.
    * Objxxx98 keeps trigger from reacting to other conversations.
    * if %actor.is_pc% && 
    case appointment
    * Check to see if someone is already trying to get an appointment.
    if %self.has_item(%zone%98)% && !%actor.varexists(Making_Tarot_Appointment_%zone%)%
      say I'm sorry, %actor.name%.  I'm speaking with someone else right now.
      halt
    end
    if !%self.has_item(%zone%98)%
      %load% o %zone%98
      set Making_Tarot_Appointment_%zone% 1
      remote Making_Tarot_Appointment_%zone% %actor.id%
      say Let me just see if any of our consultants have an opening.
      say Say restart at any time to leave or start over.
      %echo% She consults an appointment book.
      wait 2 seconds
      set available 0
      set readerno 0
      set unreaderno 0
      eval temp %%findobj.%zone%25(card)%%
      eval findobjsibyl %temp%
      if %findobjsibyl% < 1
        eval available %available% + 1
        eval readerno %readerno% + 1
        set reader%readerno% Sibyl
      else 
        eval unreaderno %unreaderno% + 1
        set unreader%unreaderno% Sibyl
      end
      eval temp %%findobj.%zone%45(card)%%
      eval findobjesmerelda %temp%
      if %findobjesmerelda% < 1
        eval available %available% + 1
        eval readerno %readerno% + 1
        set reader%readerno% Esmerelda
      else 
        eval unreaderno %unreaderno% + 1
        set unreader%unreaderno% Esmerelda
      end
      eval temp %%findobj.%zone%65(card)%%
      eval findobjjaelle %temp%
      if %findobjjaelle% < 1
        eval available %available% + 1
        eval readerno %readerno% + 1
        set reader%readerno% Jaelle
      else 
        eval unreaderno %unreaderno% + 1
        set unreader%unreaderno% Jaelle
      end
      if %available% == 0
        say Sorry, Sibyl, Esmerelda and Jaelle all seem to be with clients right now.
        say Please try again later.
        rdelete Making_Tarot_Appointment_%zone% %actor.id%
        halt
      end
      if %available% == 1
        say %unreader1% and %unreader2% are with clients but %reader1% is available.
        say Say %reader1% if you want to see her.
        set Choosing_Tarot_Reader_%zone% 1
        remote Choosing_Tarot_Reader_%zone% %actor.id%
        halt
      end
      if %available% == 2
        say %unreader1% is with a client but %reader1% and %reader2% are available.
        say Say %reader1% or %reader2% to see her.
        set Choosing_Tarot_Reader_%zone% 1
        remote Choosing_Tarot_Reader_%zone% %actor.id%
        halt
      end
      if %available% == 3
        say %reader1%, %reader2% and %reader3% are all available right now.
        say Say %reader1%, %reader2% or %reader3% to see her.
        set Choosing_Tarot_Reader_%zone% 1
        remote Choosing_Tarot_Reader_%zone% %actor.id%
        halt
      end
    end
  break
  case Sibyl
    if %actor.varexists(Choosing_Tarot_Reader_%zone%)%
      eval findobj %%findobj.%zone%25(card)%%
      if %findobj% < 1
        say Sibyl is ready to see you now.
        %door% %zone%02 north flags a
        wait 1 sec
        %force% %actor% north
        %door% %zone%02 north flags abc
        rdelete Making_Tarot_Appointment_%zone% %actor.id%
        rdelete Choosing_Tarot_Reader_%zone% %actor.id%
        mgoto %zone%99
        %purge% quill
        mgoto %zone%25
        %load% obj %zone%49
        mgoto %zone%02
      else
        Say I'm sorry.  Sibyl is with another client right now.
        say Please choose one of the available readers.
      end
    end
  break
  case Esmerelda
    if %actor.varexists(Choosing_Tarot_Reader_%zone%)%
      eval findobj %%findobj.%zone%45(card)%%
      if %findobj% < 1
        say Esmerelda is ready to see you now.
        %door% %zone%02 west flags a
        wait 1 sec
        %force% %actor% w
        %door% %zone%02 west flags abc
        rdelete Making_Tarot_Appointment_%zone% %actor.id%
        rdelete Choosing_Tarot_Reader_%zone% %actor.id%
        mgoto %zone%99
        %purge% quill
        mgoto %zone%45
        %load% obj %zone%52
        mgoto %zone%02
      else
        Say I'm sorry.  Esmerelda is with another client right now.
        say Please choose one of the available readers.
      end
    end
  break
  case Jaelle
    if %actor.varexists(Choosing_Tarot_Reader_%zone%)%
      eval findobj %%findobj.%zone%65(card)%%
      if %findobj% < 1
        say Jaelle is ready to see you now.
        %door% %zone%02 east flags a
        wait 1 sec
        %force% %actor% e
        %door% %zone%02 east flags abc
        rdelete Making_Tarot_Appointment_%zone% %actor.id%
        rdelete Choosing_Tarot_Reader_%zone% %actor.id%
        mgoto %zone%99
        %purge% quill
        mgoto %zone%65
        %load% obj %zone%50
        mgoto %zone%02
      else
        Say I'm sorry.  Jaelle is with another client right now.
        say Please choose one of the available readers.
      end
    end
  break
  case Restart
    if %actor.varexists(Making_Tarot_Appointment_%zone%)%
      rdelete Making_Tarot_Appointment_%zone% %actor.id%
      rdelete Choosing_Tarot_Reader_%zone% %actor.id%
      mgoto %zone%99
      %purge% quill
      mgoto %zone%02
      emote puts down the appointment book.
    end
  break
  default
  break
done
* End of the loop we need to take the next word in the string
* and save the remainder for the next pass.
eval word %rest.car%
eval rest %rest.cdr%
done
~
#21107
Tarot Receptionist greets - M21104~
0 h 100
*~
if %direction% == south
  welcome %actor.name%
  %send% %actor% Ana says, 'Would you like to make an appointment with one of our readers?'
  %send% %actor% Ana says, 'Before we start, make sure you have enough time to finish your reading.'
  %send% %actor% Ana says, 'Please do not go afk or leave the game before you finish the reading.'
  %send% %actor% Ana says, 'If you are sure, just say appointment.'
else if %direction% == up
  smile %actor.name%
  %send% %actor% Ana says, 'I hope you enjoyed your reading.  Please, come again soon.'
  %send% %actor% Ana says, 'Of course, if you want another appointment now, say appointment.'
end
~
#21108
Leaving Tarot~
0 c 100
*~
* For mobs to clear reading from players blocking by starting and leaving.
* Should be adjusted to your muds commands.
* Parnassus' Special Anti-Freeze Formula
if %cmd.mudcommand% == nohassle
  return 0
  halt
end
*
set zone 211
if %cmd.mudcommand% == quit || %cmd.mudcommand% == afk
  if %self.vnum% == %zone%04 && %actor.varexists(Making_Tarot_Appointment_%zone%)%
    say I'm sorry but I won't be able to give you an appointment right now.
    say Please come back when you have more time available.
    rdelete Making_Tarot_Appointment_%zone% %actor.id%
    rdelete Choosing_Tarot_Reader_%zone% %actor.id%
    mgoto %zone%99
    %purge% quill
    mgoto %zone%02
    wait 1 sec
    emote puts down the appointment book.
    wait 1 sec
    %force% %actor% %cmd.mudcommand%
    wait 1 sec
    halt
  elseif %self.vnum% == %zone%01 || %self.vnum% == %zone%02 || %self.vnum% == %zone%03
    set office %self.room.vnum%
    eval endroom %office% + 10
    %echo% 	n    %self.name%'s voice sounds reproachfully in your head.
    %echo% 	c       'You don't seem to have time for this right now.	n
    %echo% 	c     Please come back when you have more time.'	n
    wait 1 sec
    %echo%    %self.name% waves her hand and you find yourself outside.
    wait 1 sec
    %teleport% %actor% %zone%01
    mgoto %endroom%
    down
    mgoto %office%
    wait 1 sec
    %force% %actor% look
    %force% %actor% %cmd.mudcommand%
    rdelete Deck_Shuffled %self.id%
    rdelete Cards_Dealt %self.id%
    rdelete tarot_reading_started %self.id%
    halt
  else
    return 0
    halt
  end
elseif %cmd% == return || %cmd% == recall || %cmd% == teleport || %cmd.mudcommand% == goto
  if %self.vnum% == %zone%04 && %actor.varexists(Making_Tarot_Appointment_%zone%)%
    say I'm sorry but I won't be able to give you an appointment right now.
    say Please come back when you have more time available.
    rdelete Making_Tarot_Appointment_%zone% %actor.id%
    rdelete Choosing_Tarot_Reader_%zone% %actor.id%
    mgoto %zone%99
    %purge% quill
    mgoto %zone%02
    %send% %actor%  	n
    emote puts down the appointment book.
    %send% %actor%  	n
    return 0
    halt
  elseif %self.vnum% == %zone%01 || %self.vnum% == %zone%02 || %self.vnum% == %zone%03
    set office %self.room.vnum%
    eval endroom %office% + 10
    %echo% 	n    %self.name%'s voice sounds reproachfully in your head.
    %echo% 	c       'You don't seem to have time for this right now.	n
    %echo% 	c     Please come back when you have more time.'	n
    %send% %actor%  	n
    %teleport% %actor% %zone%01
    return 0
    %send% %actor%  	n
    mgoto %endroom%
    down
    mgoto %office%
    rdelete Deck_Shuffled %self.id%
    rdelete Cards_Dealt %self.id%
    rdelete tarot_reading_started %self.id%
    halt
  else
    return 0
    halt
  end
elseif %cmd.mudcommand% == south
  if %self.vnum% == %zone%04 && %actor.varexists(Making_Tarot_Appointment_%zone%)%
    say I'm sorry but I won't be able to give you an appointment right now.
    say Please come back when you have more time available.
    rdelete Making_Tarot_Appointment_%zone% %actor.id%
    rdelete Choosing_Tarot_Reader_%zone% %actor.id%
    mgoto %zone%99
    %purge% quill
    mgoto %zone%02
    wait 1 sec
    emote puts down the appointment book.
    wait 1 sec
    %force% %actor% %cmd%
    halt
  else
    return 0
    halt
  end
else
  return 0
end
~
#21109
Timer for obj 21198~
1 f 100
~
* Timer on obj 21198 is set to 10 minutes.  This is adjustable.
* Since 21198 stops any appointments while talking to one person
*  this keeps any person from blocking the zone until reboot.
set zone 211
set actor %self.carried_by%
if %actor.vnum% == %zone%04
  %echo% %actor.name% says, 'I've been waiting too long for this appointment.'
  %echo% %actor.name% puts down the appointment book.
  %purge% %self%
else
  set actor %self.carried_by.name%
  %force% %actor% say I seem to have stolen someone's pen.
  %echoaround% %actor% The nib of the pen pokes %actor%.
  %send% %actor% The nib of the pen pokes you.
  %damage% %actor% 5
  %echoaround% %actor% %actor.name% shakes %actor.hisher% hand in pain and drops a pen which rolls away.
  %send% %actor% You drop a pen which rolls away somewhere.
  %purge% %self%
end
~
#21110
Reload glass and bread - obj 21180 and 21182~
1 c 100
*~
* This trigger is to keep the waiting room supplied with food and drink.
* Because of the auto-regenerative qualities, it also cancels out any
*  sac benefits to prevent spam-saccing for gold or exp.
* Parnassus' Special Anti-Freeze Formula
if %cmd.mudcommand% == nohassle
  return 0
  halt
end
*
set zone 211
if get == %cmd.mudcommand% || sacrifice == %cmd.mudcommand%
  if %self.room.vnum% == %zone%02
    set testernumber 2
  else
    set testernumber 1
  end 
  set arg _%arg%
  eval inroom %self.room%
  eval obj %inroom.contents%
  * find the first object in the room
  while %obj%
    set next_obj %obj.next_in_list%
    set objlist %obj.name%
    set keywordlist _%obj.name.car%    
    set keywordrest _%obj.name.cdr%
    while %keywordlist%
      * while an object is in the room
      if %keywordlist.contains(%arg%)%
        if %obj.id% == %self.id%
          if get == %cmd.mudcommand%
            %force% %actor% %cmd.mudcommand% %obj.name.car%
          elseif sacrifice == %cmd.mudcommand%
            %send% %actor% You carefully dispose of %obj.shortdesc%.
            %echoaround% %actor% %actor.name% carefully disposes of %obj.shortdesc%.
            set me %self.vnum%
            eval temp %%findobj.%zone%02(%me%)%%
            eval tester %temp%
            if %self.room.vnum% == %zone%02
              set testernumber 2
            else
              set testernumber 1
            end 
            if %tester% < %testernumber%
              %at% %zone%02 %load% obj %self.vnum%
            end
            %purge% %self%
          end
          set me %self.vnum%
          eval temp %%findobj.%zone%02(%me%)%%
          eval tester %temp%
          if %tester% < %testernumber%
            %at% %zone%02 %load% obj %self.vnum%
          end
          halt
        end
      end
      set keywordlist %keywordrest.car%
      set keywordrest %keywordrest.cdr%
    done
    * find the next object for the while to loop
    set obj %next_obj%
  done
  return 0
  halt
else
  return 0
  halt
end
~
$~
