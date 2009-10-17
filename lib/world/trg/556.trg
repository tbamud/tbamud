#55618
WhirlpoolMaze18~
2 g 100
~
  wait 2 sec
  %door% 55618 north purge
  %door% 55618 east purge
  %door% 55618 west purge
  wait 1 sec
  switch %random.3%
    case 1
  %door% 55618 north room 55621
  %door% 55618 east room 55619
  %door% 55618 west room 55620
    break
    case 2
  %door% 55618 north room 55619
  %door% 55618 east room 55620
  %door% 55618 west room 55621
    break
    case 3
  %door% 55618 north room 55620
  %door% 55618 east room 55621
  %door% 55618 west room 55619
    break
    default
  %door% 55618 north room 55684
  %door% 55618 east room 55684
  %door% 55618 west room 55684
    break
  done
~
#55624
WhirlpoolMaze24~
2 g 100
~
  wait 2 sec
  %door% 55624 south purge
  %door% 55624 east purge
  %door% 55624 west purge
  wait 1 sec
  switch %random.3%
    case 1
  %door% 55624 south room 55621
  %door% 55624 east room 55625
  %door% 55624 west room 55626
    break
    case 2
  %door% 55624 south room 55625
  %door% 55624 east room 55626
  %door% 55624 west room 55621
    break
    case 3
  %door% 55624 south room 55626
  %door% 55624 east room 55621
  %door% 55624 west room 55625
    break
    default
  %door% 55624 south room 55684
  %door% 55624 east room 55684
  %door% 55624 west room 55684
    break
  done
~
#55668
BatlinMantras~
0 n 100
~
%load% obj 55501
%load% obj 55503
%load% obj 55507
%load% obj 55505
%load% obj 55502
%load% obj 55506
%load% obj 55500
%load% obj 55504
~
$~
