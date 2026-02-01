# NPC patrol script example (attach to a mob trigger)

# Track per-NPC patrol state by uid.
patrol_state = {}
# Ordered list of target room vnums for the patrol route.
patrol_route = [103, 105, 153, 149, 100]

def on_trigger(event):
    # The mob this trigger is attached to.
    npc = event["self"]
    # Unique runtime id used to track this NPC's state.
    uid = npc.uid
    # Lookup or create state for this NPC.
    state = patrol_state.get(uid)
    if state is None:
        # Start at the first patrol target.
        state = {"target": 0}
        patrol_state[uid] = state

    # Current target room vnum.
    target = patrol_route[state["target"]]

    # If already at target, advance to the next.
    if npc.room.vnum == target:
        state["target"] = (state["target"] + 1) % len(patrol_route)
        target = patrol_route[state["target"]]

    # Move one step toward the target room vnum.
    if not mud.move(target, actor=npc):
        mud.log(f"sample_patrol: no path to {target}", actor=npc)

    # Optional flavor emotes at specific rooms.
    if npc.room.vnum == 153:
        mud.emote("looks around carefully", actor=npc)
    elif npc.room.vnum == 149:
        mud.emote("sniffs the air", actor=npc)

    # Sleep to schedule the next patrol tick.
    mud.sleep(3)
