#45000
Room Door Pass Test~
2 d 100
pass~
%echo% Test: Door command, remove exit
%door% 45000 north purge
%echo% Test: Door command, set new exit
%door% 45000 east room 45001
%echo% Test: Door command, set new description
%door% 45000 east description A new description!
%echo% Test: Move unmoveable object.
%move% object 45000
%move% 123456object 45000
~
#45001
Room Door Fail Test~
2 d 100
fail~
%echo% Test: Door command, wrong direction name
%door% 45000 eest purge
%echo% Test: Door command, from non-existant room
%door% 45002 east purge
%echo% Test: Door command, to non-existant room
%door% 45000 east room 45002
%echo% Test: Door command, wrong command name
%door% 45000 east poorge
%echo% Test: Reset
%door% 45000 east room 45001
~
$~
