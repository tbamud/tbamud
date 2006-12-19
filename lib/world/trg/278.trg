#27800
Sea Serpent Fireball - 27801~
0 k 5
~
if %actor.is_pc%
  emote utters the words, 'hisssssss'.
  dg_cast 'fireball' %actor%
end
~
#27802
Leviathan - 27803~
0 k 5
~
if %actor.is_pc%
  switch %random.3%
    case 1
      emote utters the words, 'transvecta aqua'.
      %echo% a tidal wave smashes into %actor.name%.
      %damage% %actor% 50
    break
    case 2
      emote looks at you with the deepest sorrow.
    break
    case 3
      emote utters the words, 'transvecta talon'.
      dg_cast 'cure critic' %self%
    break
    default
    break     
  done
end
~
$~
