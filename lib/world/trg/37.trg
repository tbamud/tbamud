#3700
block_mobs_not_following~
2 g 100
~
if !%actor.is_pc% && !%actor.master%
  return 0
end
~
$~
