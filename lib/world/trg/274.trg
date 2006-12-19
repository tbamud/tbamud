#27400
Happy Smurf - 27415~
0 g 100
~
if %actor.is_pc%
  wait 2 sec
  switch %random.5%
    case 1
      say Tra, La, la, La, la, la.
    break
    case 2
      say Will you be my friend?
    break
    case 3
      say It's a very smurfy day in Smurfville!
    break
    case 4
      say Everything is Smurfy-OK!
    break
    case 5
      say Yea, we're so happy!
    break
    default
      sing
    break
  done
end
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
Papa dg_cast by level - 27400~
0 k 50
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
