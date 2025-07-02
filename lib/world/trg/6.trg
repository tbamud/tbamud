#600
Mob Random Speak - 622~
0 b 5
~
switch %random.3%
  case 1
    say RAAWK!  Polly wants a cracker! Polly wants a cracker!
  break
  case 2
    say Who's a pretty boy then? Who's a pretty boy then, RAAAAWK!
  break
  case 3
    say Braaaak. I'm a pretty boy. I'm a pretty boy. Braaaak.
  break
  default
    sneeze
  break
done
~
#601
Mob Speech Polly - 622~
0 d 100
pretty~
if %actor.vnum% != %self.vnum%
  say I'm a pretty boy.
end
~
$~
