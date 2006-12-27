/**************************************************************************
*  File: dg_event.c                                                       *
*                                                                         *
*  Usage: This file contains a simplified event system to allow           *
*  DG Script triggers to use the "wait" command, causing a delay in the   *
*  middle of a script.                                                    *
*                                                                         *
*  By: Mark A. Heilpern (Sammy @ eQuoria MUD   equoria.com:4000)          *
*                                                                         *
*  As of dg scripts pl 8 this includes the 'FULL' DG event package.       *                                                                       *
*  This file includes the file queue.c, which handles the priority queues.*                                                                       *
*  Thomas Arp - Welcor - 2002                                             *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/
/*
 * dg_event.c: This file contains a simplified event system to allow
 * DG Script triggers to use the "wait" command, causing a delay in the
 * middle of a script.
 *
 * By: Mark A. Heilpern (Sammy @ eQuoria MUD   equoria.com:4000)
 *
 * As of dg scripts pl 8 this includes the 'FULL' DG event package.
 * This file includes the file queue.c, which handles the priority queues.
 * Thomas Arp - Welcor - 2002
 *
 */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "db.h"
#include "utils.h"
#include "dg_event.h"
#include "constants.h"

struct queue *event_q;          /* the event queue */

extern long pulse;

/* initializes the event queue */
void event_init(void) 
{
  event_q = queue_init();
}


/*
** Add an event to the current list
*/
/* creates an event and returns it */
struct event *event_create(EVENTFUNC(*func), void *event_obj, long when)
{
  struct event *new_event;

  if (when < 1) /* make sure its in the future */
    when = 1;
 
  CREATE(new_event, struct event, 1);
  new_event->func = func;
  new_event->event_obj = event_obj;
  new_event->q_el = queue_enq(event_q, new_event, when + pulse);

  return new_event;
}


/* removes the event from the system */
void event_cancel(struct event *event)
{
  if (!event) {
    log("SYSERR:  Attempted to cancel a NULL event");
    return;
    }

  if (!event->q_el) {
    log("SYSERR:  Attempted to cancel a non-NULL unqueued event, freeing anyway");
  } else
    queue_deq(event_q, event->q_el);

  if (event->event_obj)
    free(event->event_obj);
  free(event);
}


/* Process any events whose time has come. */
void event_process(void)
{
  struct event *the_event;
  long new_time;

  while ((long) pulse >= queue_key(event_q)) {
    if (!(the_event = (struct event *) queue_head(event_q))) {
      log("SYSERR: Attempt to get a NULL event");
      return;
    }

    /*
    ** Set the_event->q_el to NULL so that any functions called beneath
    ** event_process can tell if they're being called beneath the actual
    ** event function.
    */
    the_event->q_el = NULL;
 
    /* call event func, reenqueue event if retval > 0 */
    if ((new_time = (the_event->func)(the_event->event_obj)) > 0)
      the_event->q_el = queue_enq(event_q, the_event, new_time + pulse);
    else
      free(the_event);
  }
}


/* returns the time remaining before the event */
long event_time(struct event *event)
{
  long when;

  when = queue_elmt_key(event->q_el);

  return (when - pulse);
}


/* frees all events in the queue */
void event_free_all(void)
{
  struct event *the_event;

  while ((the_event = (struct event *) queue_head(event_q))) {
    if (the_event->event_obj)
      free(the_event->event_obj);
    free(the_event);
  }

  queue_free(event_q);
}

/* boolean function to tell whether an event is queued or not */
int event_is_queued(struct event *event)
{
   if (event->q_el)
     return 1;
   else
     return 0;
}

/* ************************************************************************
*  File: queue.c                                                          *
*                                                                         *
*  Usage: generic queue functions for building and using a priority queue *
*                                                                         *
************************************************************************ */

/* returns a new, initialized queue */
struct queue *queue_init(void)
{
  struct queue *q;

  CREATE(q, struct queue, 1);

  return q;
}


/* add data into the priority queue q with key */
struct q_element *queue_enq(struct queue *q, void *data, long key)
{
  struct q_element *qe, *i;
  int bucket;

  CREATE(qe, struct q_element, 1);
  qe->data = data;
  qe->key = key;

  bucket = key % NUM_EVENT_QUEUES;   /* which queue does this go in */

  if (!q->head[bucket]) { /* queue is empty */
    q->head[bucket] = qe;
    q->tail[bucket] = qe;
  }

  else {
    for (i = q->tail[bucket]; i; i = i->prev) {

      if (i->key < key) { /* found insertion point */
	if (i == q->tail[bucket])
	  q->tail[bucket] = qe;
	else {
	  qe->next = i->next;
	  i->next->prev = qe;
	}

	qe->prev = i;
	i->next = qe;
	break;
      }
    }

    if (i == NULL) { /* insertion point is front of list */
      qe->next = q->head[bucket];
      q->head[bucket] = qe;
      qe->next->prev = qe;
    }
  }
 
  return qe;
}


/* remove queue element qe from the priority queue q */
void queue_deq(struct queue *q, struct q_element *qe)
{
  int i;

  assert(qe);

  i = qe->key % NUM_EVENT_QUEUES;

  if (qe->prev == NULL)
    q->head[i] = qe->next;
  else
    qe->prev->next = qe->next;

  if (qe->next == NULL)
    q->tail[i] = qe->prev;
  else
    qe->next->prev = qe->prev;
    
  free(qe);
}


/*
 * removes and returns the data of the
 * first element of the priority queue q
 */
void *queue_head(struct queue *q)
{
  void *dg_data;
  int i;

  i = pulse % NUM_EVENT_QUEUES;

  if (!q->head[i])
    return NULL;

  dg_data = q->head[i]->data;
  queue_deq(q, q->head[i]);
  return dg_data;
}


/*
 * returns the key of the head element of the priority queue
 * if q is NULL, then return the largest unsigned number
 */
long queue_key(struct queue *q)
{
  int i;

  i = pulse % NUM_EVENT_QUEUES;

  if (q->head[i])
    return q->head[i]->key;
  else
    return LONG_MAX;
}


/* returns the key of queue element qe */
long queue_elmt_key(struct q_element *qe)
{
  return qe->key;
}


/* free q and contents */
void queue_free(struct queue *q)
{
  int i;
  struct q_element *qe, *next_qe;

  for (i = 0; i < NUM_EVENT_QUEUES; i++)
    for (qe = q->head[i]; qe; qe = next_qe) {
      next_qe = qe->next;
      free(qe);
    }

  free(q);
 }
 
 
