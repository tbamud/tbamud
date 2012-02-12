/**
* @file mud_event.h
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*/
#ifndef _MUD_EVENT_H_
#define _MUD_EVENT_H_

#include "dg_event.h"

#define EVENT_WORLD 0
#define EVENT_DESC  1
#define EVENT_CHAR  2

struct mud_event_data {
  struct  event * pEvent;
  int     iEvent_Type;
  void  * pStruct;
  char  * sVariables;	
}; 

/* Externals */
extern struct list_data * world_events;

/* Local Functions */
void init_events(void);
struct mud_event_data *new_mud_event(int iEvent_Type, void *pStruct, char *sVariables);
void attach_mud_event(void (*func), struct mud_event_data *pMudEvent, long time);
void free_mud_event(struct mud_event_data *pMudEvent);

/* Events */
EVENTFUNC(get_protocols);
EVENTFUNC(display_usage);

#endif /* _MUD_EVENT_H_ */
