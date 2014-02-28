#65302
Running the Elevator~
2 c 100
*~
* Written December 22, 2014 by Parnassus for TBAmud
* Thanks to Fizban for writing an Elevator trigger, and Rumble for posting it as
*    a Trigger of the day, which made me decide that my Housing project
*    apartment really needed an elevator.
* Thanks to Axanon for letting me look at his really nice elevator triggers.
* Be prepared for errors, use at your own risk.
* Any errors are mine, any people named in this trigger are blameless!  
*
* Parnassus' Special Anti-Freeze Formula
if %cmd.mudcommand% == nohassle
  return 0
  halt
end
*
* Variable assignments: make changes here.
* Thanks to Krell for helping me work out this format which is especially
*    good for a stand-alone trigger!
*  adjust zone number
*  set maxfl to the highest floor number. To make a penthouse, make adjustments
*    similar to the Ground floor here.  If you add more than 20 floors, switch to the
*    commented line using CURFL instead of the one using ORDINAL .
*  add additional floors along with %zone% and room number of the linking room.
* (Also add mention of button and display in the linking room.  Set up door to the
*    elevator, flagged as 2 - Pickproof door.) 
set zone 653
set maxfl 4
set totfl 5
set Elevat %zone%99
set Floor0 %zone%98
set Floor1 %zone%06
set Floor2 %zone%14
set Floor3 %zone%22
set Floor4 %zone%30
* Which side of the elevator is the door on?
set elevdoor north
* Which side of the room is the elevator on?
set roomdoor south
** Thanks to Thomas for the formula that makes this part work.
eval temp %%Elevat.%elevdoor%(vnum)%%
eval eleroom %temp%
if %eleroom% == %Floor1%
  set CurFl 1
  set Curex %Floor1%
elseif %eleroom% == %Floor2%
  set CurFl 2
  set Curex %Floor2%
elseif %eleroom% == %Floor3%
  set CurFl 3
  set Curex %Floor3%
elseif %eleroom% == %Floor4%
  set CurFl 4
  set Curex %Floor4% 
elseif %eleroom% == %Floor0%
  set CurFl 0
  set Curex %Floor0%
end
if %cmd% == push
  if %arg% == g || %arg% >= 1 && %arg% <= %maxfl%
    if %self.vnum% != %Elevat%
      %send% %actor% You don't see that here.
      halt
    else
      ** Thanks to Kyle for figuring out how to make this part work.
      eval text %%self.%elevdoor%(bits)%%
      if %text.contains(LOCKED)% && !%text.contains(PICKPROOF)% 
        %send% %actor% The elevator is already in motion.  Please be patient.
        halt
      end
    end
    if %arg% == g
      set %arg% 0
      set targfl 0
      set Tarex %Floor0%
    elseif %arg% == 1
      set Tarex %Floor1%
      set targfl 1
    elseif %arg% == 2
      set Tarex %Floor2%
      set targfl 2
    elseif %arg% == 3
      set Tarex %Floor3%
      set targfl 3
    elseif %arg% == 4
      set Tarex %Floor4%
      set targfl 4
    end
  elseif %arg% == button
    if %self.vnum% == %Elevat%
      %send% %actor% Which button?
      halt
    else
      eval text %%self.%roomdoor%(bits)%%
      if !%text.contains(CLOSED)%
        %send% %actor% The elevator is already here.
        halt
      elseif %text.contains(LOCKED)% && !%text.contains(PICKPROOF)% 
        %send% %actor% The elevator is already in use.  Please wait and try again.
        halt
      end
    end
  else
    %send% %actor% You don't see that here.
    halt
  end
  if %self.vnum% == %Floor0%
    set Tarex %Floor0%
    set targfl 0
  elseif %self.vnum% == %Floor1%
    set Tarex %Floor1%
    set targfl 1
  elseif %self.vnum% == %Floor2%
    set Tarex %Floor2%
    set targfl 2
  elseif %self.vnum% == %Floor3%
    set Tarex %Floor3%
    set targfl 3
  elseif %self.vnum% == %Floor4%
    set Tarex %Floor4%
    set targfl 4
  end
  if %targfl% == g
    set dir down
    eval length %Curfl%
  elseif %targfl% > %Curfl%
    set dir up
    eval length %targfl% - %Curfl%
  elseif %targfl% < %Curfl%
    set dir down
    eval length %Curfl% - %targfl%
  else 
    set dir nowhere
    set length 0
  end
  %at% %Elevat% %echo% The elevator doors slide shut.
  %at% %Curex% %echo% The elevator doors slide shut.
  wait 5
  set echofl 0
  while %echofl% < %totfl%
    eval temp %%Floor%echofl%%%
    %at% %temp% %echo% The elevator doors start to vibrate.
    %door% %temp% %roomdoor% flags abc
    eval echofl %echofl% + 1
  done
  if %length% > 0
    set origlen %length%
    %door% %Curex% %roomdoor% flags abc
    %door% %Elevat% %elevdoor% purge
    %door% %Elevat% %elevdoor% room %Elevat%
    %door% %Elevat% %elevdoor% flags abc
    %at% %Elevat% %echo% The elevator 	g*l u r c h e s*	n
    %at% %Elevat% %echo% 	n              	G*l u r c h e s*	n
    %at% %Elevat% %echo% 	n             	g*l u r c h e s*	n %dir%wards.
  end
  while %length%
    if %dir% == up
      eval Curfl %Curfl% + 1
    elseif %dir% == down
      eval Curfl %Curfl% -1
    end
    if %Curfl% == 0
      set ordinal ground
    elseif %Curfl% == 1
      set ordinal 1st
    elseif %Curfl% == 2
      set ordinal 2nd
    elseif %Curfl% == 3
      set ordinal 3rd
    elseif %Curfl% > 3 
      set ordinal %Curfl%th
    end 
    wait 15
    %at% %Elevat% %echo% 	g     *DING!*	n
    %at% %Elevat% %echo% You have reached the %ordinal% floor.
    * It's easier to use the following line if you have over 20 floors (instead of the 21th floor)
    *    %at% %Elevat% %echo% You have reached floor %Curfl%.
    wait 5
    eval length %length% - 1
    if %CurFl% > %maxfl%
      %echo%	r Something seems to be wrong with the elevator.	n
      %echo%	r Please try again and if there's still a problem,	n
      %echo%	r please report this to admin.	n
      set length 0
    end
  done
  if %origlen% > 0
    wait 2
    %at% %Elevat% %echo% 	nThe elevator 	y*s h u d d e r s*	n
    %at% %Elevat% %echo% 	n              	Y*s h u d d e r s*	n
    %at% %Elevat% %echo% 	n             	y*s h u d d e r s*	n to a halt.
  end
  wait 2
  set echofl 0
  while %echofl% < %totfl%
    eval temp %%Floor%echofl%%%
    %at% %temp% %echo% The elevator doors stop vibrating.
    wait 3
    %door% %temp% %roomdoor% flags abcd
    eval echofl %echofl% + 1
  done
  wait 1
  %at% %targfl%  %echo% 	g     *DING!*	n
  wait 3
  %door% %Tarex% %roomdoor% room %Elevat%
  %door% %Tarex% %roomdoor% flags a
  %door% %Elevat% %elevdoor% purge
  %door% %Elevat% %elevdoor% room %Tarex%
  %at% %Elevat% %echo% The doors slide open.
  %at% %Tarex% %echo% The elevator doors slide open.
  wait 3
  %door% %Elevat% %elevdoor% flags a
elseif %cmd.mudcommand% == look
  if !%arg%
    return 0
    halt
  elseif %arg% == display
    if %self.vnum% != %Elevat%
      eval text %%self.%roomdoor%(bits)%%
      if !%text.contains(CLOSED)%
        %send% %actor% The elevator is already here.
        halt
      end
    else
      eval text %%self.%elevdoor%(bits)%%
    end
    if %text.contains(LOCKED)% && !%text.contains(PICKPROOF)% 
      %send% %actor% Numbers flash across the display.  The elevator is on the move.
      halt
    end
    if %Curfl% == 0
      set displaymsg the ground floor
    else
      set displaymsg floor %Curfl%
    end
    %send% %actor% The elevator is on %displaymsg%.
    halt
    * if look is not noarg or display
  else
    return 0
  end
  * if command is anything except look or push
else
  return 0
end
~
$~
