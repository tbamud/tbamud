#3700
block_mobs_not_following~
2 g 100
east~
if !%actor.is_pc%
  if !%actor.master%
    return 0
  end
end
~
$~
