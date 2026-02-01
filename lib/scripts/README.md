Scripts In lib/scripts
======================

This directory contains Python and DSL-style scripts used by the game. Scripts
are attached to rooms, npc's, and objects via builder commands (e.g. `rset add script`,
`mset add script`, `oset add script`). The script file itself is the data format.

Two Script Styles
-----------------

1) DSL (no function definitions)
   - If the file does NOT contain `def on_trigger`, it is treated as the DSL.
   - The DSL supports Python-style `if/elif/else`, `for`, and `while` blocks.
   - Actions are written as simple verbs or function-style calls:
     - `move("n")` or `move("north")`
     - `move(1200)` (room vnum path step, no closed doors)
     - `emote("looks around warily")`
     - `sleep(3)`
     - `log("text with {npc.vnum}")`
     - `do("say hello")`
   - Command-style is also supported:
     - `move "n"`
     - `sleep 3`
     - `log "hello {npc.vnum}"`
   - NOTE: The DSL does not support custom variable assignments yet. Use the
     full Python style if you need variables.

2) Full Python (with `def on_trigger(event):`)
   - If the file contains `def on_trigger`, it runs as normal Python.
   - Use `mud.sleep(seconds)` to pause and resume later.
   - You can use local variables, helpers, and full Python expressions.

Event Context (Full Python)
---------------------------

`on_trigger(event)` receives a dict with:
- `event["self"]`: the entity this trigger is attached to (mob/object/room)
- `event["trigger"]`: info about the trigger (vnum, name, type, narg, etc.)
- `event["vars"]`: trigger variables (if any)

Entity Data Model (DSL and Python)
----------------------------------

Every entity has these common properties:
- `entity.kind`    -> kind id (mob/obj/room)
- `entity.uid`     -> unique runtime id
- `entity.vnum`    -> vnum (mob/obj/room)
- `entity.name`    -> name/short for display

Mob (NPC or PC) properties:
- `npc.health` / `npc.max_health`
- `npc.mana` / `npc.max_mana`
- `npc.stamina` / `npc.max_stamina`
- `npc.move` / `npc.max_move` (alias for stamina)
- `npc.class` / `npc.class_id`
- `npc.species` / `npc.species_id`
- `npc.is_pc` / `npc.is_npc`
- `npc.keyword` (NPC keywords; for PCs, name)
- `npc.room` -> room entity where the mob is located
  - `npc.room.vnum` -> room vnum
  - `npc.room.name` -> room name/short

Object properties:
- `object.keyword` -> object keywords
- `object.oval`    -> list of object values (oval[0] .. oval[NUM_OBJ_VAL_POSITIONS-1])
- `object.room`    -> room entity where the object is located
  - `object.room.vnum` -> room vnum
  - `object.room.name` -> room name/short

Room properties:
- `room.contents`  -> list of objects in the room
- `room.people`    -> list of characters in the room
- `room.vnum`      -> room vnum
- `room.name`      -> room name/short

Entity comparisons:
- `entity == "rat"` checks keyword match for mobs/objects.
- `entity == 123456` checks entity uid.
- `entity_a == entity_b` matches if kind+uid are equal.

Available Methods
-----------------

These are available in both DSL and Python (via the `mud` module).

Core methods:
- `mud.do(command, actor=None)`
  Execute a game command. If `actor` is a mob, it runs as that mob. If `actor`
  is a room, it runs in that room context.
- `mud.emote(message, actor=None)`
  Emote as a mob (actor is optional if the trigger is on a mob).
- `mud.move(direction, actor=None)`
  Move a mob in a direction (e.g. "n", "south", "ne").
  You can also pass a room vnum to step toward a destination, but if it is blocked (eg. with a door), this will not work.
- `mud.sleep(seconds)`
  Pause the script and resume later.
- `mud.roll("1d6")`
  Roll dice. Supported: d4/d6/d8/d10/d12/d20/d100.
- `mud.log(message)`
  Write to `log/script.log`.
- `mud.echo_room(room, message)`
  Echo a message to a specific room.
- `mud.send_char(character, message)`
  Send a message to a character.
- `mud.call_later(seconds, func, *args, **kwargs)`
  Call a function later (Python scripts only).

Convenience names in full Python scripts:
- `log("...")` is available as a shortcut for `mud.log(...)`.
- Direction strings are available as `mud.n`, `mud.s`, `mud.e`, `mud.w`, etc.

Formatting / Expressions
------------------------

DSL supports Python-style blocks:
```
while True:
    if npc.stamina < 20:
        rest
    else:
        stand
```

DSL logging supports f-string style with braces:
```
log "rat {npc.vnum} in room {npc.room.vnum}"
```

In full Python scripts, use normal f-strings:
```
log(f"rat {event['self'].vnum} in room {event['self'].room.vnum}")
```

Variables (Full Python)
-----------------------

Use normal Python variables:
```
def on_trigger(event):
    room = event["self"]
    count = 0
    while True:
        count += 1
        mud.echo_room(room, f"tick {count}")
        mud.sleep(30)
```

The DSL currently does not support custom variable assignment. If you need
variables, use full Python with `def on_trigger(event):`.

Best Practices
--------------
- Keep scripts small and focused.
- Use `log(...)` during development and remove or reduce noise later.
- Avoid infinite tight loops without `sleep`.
- Use comments generously (`# ...`).
- Prefer readable direction strings: `"north"`, `"west"`, etc.

Sample: Room Echo Script
------------------------

See `sample_echo.py` in this directory for a ready-to-attach room script.
