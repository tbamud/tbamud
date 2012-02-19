/**************************************************************************
*  File: mud_event.c                                       Part of tbaMUD *
*  Usage: Handling of the mud event system                                *
*                                                                         *
*  By Vatiken. Copyright 2012 by Joseph Arnusch                           *
**************************************************************************/

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

struct mud_event_list mud_event_index[] = {
	{ "Null"         , NULL         , -1          },  /* eNULL */
	{ "Protocol"     , get_protocols, EVENT_DESC  }  /* ePROTOCOLS */
};

void init_events(void)
{
  /* Allocate Event List */
  world_events = create_list();
}

EVENTFUNC(event_countdown)
{
	struct mud_event_data * pMudEvent;
	struct char_data * ch = NULL;
	
	pMudEvent = (struct mud_event_data * ) event_obj;
	
	switch (mud_event_index[pMudEvent->iId].iEvent_Type) {
		case EVENT_CHAR:
		  ch = (struct char_data * ) pMudEvent->pStruct;
		break;
		default:
		break;
	}	
	
	switch (pMudEvent->iId) {
		default:
		break;
	}
	
	free_mud_event(pMudEvent);
	return 0;
}

void attach_mud_event(struct mud_event_data *pMudEvent, long time)
{
  struct event * pEvent;
  struct descriptor_data * d;
  struct char_data * ch;
   
  pEvent = event_create(mud_event_index[pMudEvent->iId].func, pMudEvent, time);
  pEvent->isMudEvent = TRUE;	
  pMudEvent->pEvent = pEvent;        

  switch (mud_event_index[pMudEvent->iId].iEvent_Type) {
    case EVENT_WORLD:
      add_to_list(pEvent, world_events);
      mudlog(CMP, LVL_GRGOD, TRUE, "INFO: Mud Event '%s' added to world", mud_event_index[pMudEvent->iId].event_name);
    break;
    case EVENT_DESC:
      d = (struct descriptor_data *) pMudEvent->pStruct;
      add_to_list(pEvent, d->events);
      mudlog(CMP, LVL_GRGOD, TRUE, "INFO: Mud Event '%s' added to %s", mud_event_index[pMudEvent->iId].event_name, d->host ? d->host : "descriptor");
    break;
    case EVENT_CHAR:
      ch = (struct char_data *) pMudEvent->pStruct;
      add_to_list(pEvent, ch->events);
      mudlog(CMP, LVL_GRGOD, TRUE, "INFO: Mud Event '%s' added to %s", mud_event_index[pMudEvent->iId].event_name, GET_NAME(ch));
    break;
  }
}

struct mud_event_data *new_mud_event(event_id iId, void *pStruct, char *sVariables)
{
  struct mud_event_data *pMudEvent;
  char *varString;
		
  CREATE(pMudEvent, struct mud_event_data, 1);
  varString = (sVariables != NULL) ? strdup(sVariables) : NULL;	
		
  pMudEvent->iId         = iId;
  pMudEvent->pStruct     = pStruct;
  pMudEvent->sVariables  = varString;
  pMudEvent->pEvent      = NULL;
	
  return (pMudEvent);	
}

void free_mud_event(struct mud_event_data *pMudEvent)
{
  struct descriptor_data * d;
  struct char_data * ch;

  mudlog(CMP, LVL_GRGOD, TRUE, "INFO: Freeing mud event '%s' : %d", mud_event_index[pMudEvent->iId].event_name, pMudEvent->iId);

  switch (mud_event_index[pMudEvent->iId].iEvent_Type) {
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

struct mud_event_data * char_has_mud_event(struct char_data * ch, event_id iId)
{
  struct event * pEvent;
  struct mud_event_data * pMudEvent;
  bool found = FALSE;

  simple_list(NULL);
	
  while ((pEvent = (struct event *) simple_list(ch->events)) != NULL) {
		if (!pEvent->isMudEvent)
		  continue;
		pMudEvent = (struct mud_event_data * ) pEvent->event_obj;
	  if (pMudEvent->iId == iId) {
	    found = TRUE;	
	    break;
    }
  }
  
  simple_list(NULL);
  
  if (found)
    return (pMudEvent);
  
  return NULL;
} 
