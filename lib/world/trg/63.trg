#6300
Near Death Trap Mahatma - 6318~
0 g 100
~
* Near Death Trap stuns actor and takes all of their equipment.
wait 3 sec
say Here, have a quick trip to the Temple of Midgaard.
wait 2 sec
%send% %actor% Mahatma plunges a black dagger into your back and takes all your equipment.
%echoaround% %actor% Mahatma plunges a black dagger into %actor.name%'s back and takes all of %actor.hisher% equipment.
* damage the actor to 0 hitpoints so they lay there stunned, unable to move.
* they will recover.
eval stunned %actor.hitp% 
%damage% %actor% %stunned%
* steal all their inventory.
while %actor.inventory%
  eval item %actor.inventory%
  eval item_to_purge %%actor.inventory(%item.vnum%)%%
  eval stolen %item.vnum%
  %purge% %item_to_purge% 
  %load% obj %stolen%
done
eval i 0
* steal all their equipped items.
while %i% < 18
  eval item %%actor.eq(%i%)%%
  if %item%
    eval stolen %item.vnum%
    eval item_to_purge %%actor.eq(%i%)%%
    %purge% %item_to_purge% 
    %load% obj %stolen%
  end
  eval i %i%+1 
done
~
#6301
Magic User - 6302, 6309, 6312, 6314, 6315~
0 k 10
~
switch %actor.level%
  case 1
  case 2
  case 3
  break
  case 4
    dg_cast 'magic missile' %actor%
  break
  case 5
    dg_cast 'chill touch' %actor%
  break
  case 6
    dg_cast 'burning hands' %actor%
  break
  case 7
  case 8
    dg_cast 'shocking grasp' %actor%
  break
  case 9
  case 10
  case 11
    dg_cast 'lightning bolt' %actor%
  break
  case 12
    dg_cast 'color spray' %actor%
  break
  case 13
    dg_cast 'energy drain' %actor%
  break
  case 14
    dg_cast 'curse' %actor%
  break
  case 15
    dg_cast 'poison' %actor%
  break
  case 16
    if %actor.align% > 0
      dg_cast 'dispel good' %actor%
    else
      dg_cast 'dispel evil' %actor%
    end
 break
  case 17
  case 18
    dg_cast 'call lightning' %actor%
  break
  case 19
  case 20
  case 21
  case 22
    dg_cast 'harm' %actor%
  break
  default
    dg_cast 'fireball' %actor%
  break
done
~
$~
