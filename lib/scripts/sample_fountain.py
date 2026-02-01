# Fountain refill script example (attach to an object trigger)

def on_trigger(event):
    # Object entity the script is attached to.
    fountain = event["self"]
    # Minimum refill amount per tick.
    refill_amount = 1

    while True:
        # Ensure the fountain has object values to read.
        if fountain and fountain.oval:
            # oval[0] is capacity, oval[1] is current contents.
            capacity = fountain.oval[0]
            contains = fountain.oval[1]
            if capacity > 0 and contains < capacity:
                # Calculate the new fill amount without exceeding capacity.
                new_amount = contains + refill_amount
                if new_amount > capacity:
                    new_amount = capacity
                # Update the fountain contents via osetval.
                mud.do(f"osetval 1 {new_amount}", actor=fountain)
                # Echo a drip message to the room when refilling.
                if fountain.room:
                    mud.echo_room(fountain.room, f"water drips from the ceiling into {fountain.name}")
        # Wait before attempting another refill.
        mud.sleep(300)
