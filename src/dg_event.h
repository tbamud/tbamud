/**
* @file dg_event.h
* This file contains defines for the simplified event system to allow trigedit 
* to use the "wait" command, causing a delay in the middle of a script.
* This system could easily be expanded by coders who wish to implement
* an event driven mud.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
* 
* This source code, which was not part of the CircleMUD legacy code,
* is attributed to:                                      
* $Author: Mark A. Heilpern/egreen/Welcor $                              
* $Date: 2004/10/11 12:07:00$                                            
* $Revision: 1.0.14 $                                                    
*/
#ifndef _DG_EVENT_H_
#define _DG_EVENT_H_

/** How often will heartbeat() call the 'wait' event function?
 * @deprecated Currently not used. */
#define PULSE_DG_EVENT 1

/**************************************************************************
 * Begin event structures and defines.
 **************************************************************************/
/** All Functions handled by the event system must be of this format. */
#define EVENTFUNC(name) long (name)(void *event_obj)

/** The event structure. Events get attached to the queue and are executed
 * when their turn comes up in the queue. */
struct event {
  EVENTFUNC(*func); /**< The function called when this event comes up. */
  void *event_obj;  /**< event_obj is passed to func when func is called */
  struct q_element *q_el;  /**< Where this event is located in the queue */
  bool isMudEvent;  /**< used by the memory routines */
};
/**************************************************************************
 * End event structures and defines.
 **************************************************************************/

/**************************************************************************
 * Begin priority queue structures and defines.
 **************************************************************************/
/** Number of buckets available in each queue. Reduces enqueue cost. */
#define NUM_EVENT_QUEUES    10

/** The priority queue. */
struct dg_queue {
  struct q_element *head[NUM_EVENT_QUEUES]; /**< Front of each queue bucket. */
  struct q_element *tail[NUM_EVENT_QUEUES]; /**< Rear of each queue bucket. */
};

/** Queued elements. */
struct q_element {
  void *data;  /**< The event to be handled. */
  long key;    /**< When the event should be handled. */
  struct q_element *prev, *next; /**< Points to other q_elements in line. */
};
/**************************************************************************
 * End priority queue structures and defines.
 **************************************************************************/

/* - events - function protos needed by other modules */
void event_init(void);
struct event *event_create(EVENTFUNC(*func), void *event_obj, long when);
void event_cancel(struct event *event);
void event_process(void);
long event_time(struct event *event);
void event_free_all(void);
void cleanup_event_obj(struct event *event);

/* - queues - function protos need by other modules */
struct dg_queue *queue_init(void);
struct q_element *queue_enq(struct dg_queue *q, void *data, long key);
void queue_deq(struct dg_queue *q, struct q_element *qe);
void *queue_head(struct dg_queue *q);
long queue_key(struct dg_queue *q);
long queue_elmt_key(struct q_element *qe);
void queue_free(struct dg_queue *q);
int  event_is_queued(struct event *event);

#endif /* _DG_EVENT_H_ */
