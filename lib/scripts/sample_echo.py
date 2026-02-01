# Room echo script example (attach to a room trigger)

def on_trigger(event):
    # Room entity the script is attached to.
    room = event["self"]

    while True:
        # Pick between two messages using a dice roll.
        if mud.roll("1d2") == 1:
            mud.echo_room(room, "someone spills a drink at the bar")
        else:
            mud.echo_room(room, "a game of cards gets rowdy as a dwarf slams a fist on a table")
        # Wait before the next echo.
        mud.sleep(60)
