
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "dg_event.h"
#include "constants.h"
#include "comm.h"  /* For access to the game pulse */
#include "mud_event.h"

/* Global List */
struct list_data * world_events = NULL;

void init_events(void)
{
  /* Allocate Event List */
  world_events = create_list();
}

void attach_mud_event(void (*func), struct mud_event_data *pMudEvent, long time)
{
  struct event * pEvent;
  struct descriptor_data * d;
  struct char_data * ch;
  
  pEvent = event_create(func, pMudEvent, time);
  pEvent->isMudEvent = TRUE;	
  pMudEvent->pEvent = pEvent;        

  switch (pMudEvent->iEvent_Type) {
    case EVENT_WORLD:
      add_to_list(pEvent, world_events);
    break;
    case EVENT_DESC:
      d = (struct descriptor_data *) pMudEvent->pStruct;
      add_to_list(pEvent, d->events);
    break;
    case EVENT_CHAR:
      ch = (struct char_data *) pMudEvent->pStruct;
      add_to_list(pEvent, ch->events);
    break;
  }
}

struct mud_event_data *new_mud_event(int iEvent_Type, void *pStruct, char *sVariables)
{
  struct mud_event_data *pMudEvent;
  char *varString;
		
  CREATE(pMudEvent, struct mud_event_data, 1);
  varString = (sVariables != NULL) ? strdup(sVariables) : NULL;	
		
  pMudEvent->iEvent_Type = iEvent_Type;
  pMudEvent->pStruct     = pStruct;
  pMudEvent->sVariables  = varString;
  pMudEvent->pEvent      = NULL;
	
  return (pMudEvent);	
}

void free_mud_event(struct mud_event_data *pMudEvent)
{
  struct descriptor_data * d;
  struct char_data * ch;

  switch (pMudEvent->iEvent_Type) {
    case EVENT_WORLD:
      remove_from_list(pMudEvent->pEvent, world_events);
    break;
    case EVENT_DESC:
      d = (struct descriptor_data *) pMudEvent->pStruct;
      remove_from_list(pMudEvent->pEvent, d->events);
    break;
    case EVENT_CHAR:
      ch = (struct char_data *) pMudEvent->pStruct;
      remove_from_list(pMudEvent->pEvent, ch->events);
    break;
  }

  if (pMudEvent->sVariables != NULL)
    free(pMudEvent->sVariables);
	  
  pMudEvent->pEvent->event_obj = NULL;
  free(pMudEvent);
}
