/**
* @file lists.h
* Lists Header file.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
* 
* This source code, which was not part of the CircleMUD legacy code,
* is attributed to:
* Copyright 2012 by Joseph Arnusch.                                                    
*/

#ifndef _LISTS_HEADER
#define _LISTS_HEADER

#include <stddef.h>

/* Lists own their item nodes but never the caller-provided pContent.  NULL
 * content is not supported because NULL is also the iteration terminator. */
struct item_data {
  struct item_data * pPrevItem;
  struct item_data * pNextItem;
  struct item_data * pNextRemoved;
  void             * pContent;
  unsigned char      isRemoved;
};

struct list_data {
  struct item_data * pFirstItem;
  struct item_data * pLastItem;
  struct item_data * pRemovedItems;
  size_t             iIterators;
  size_t             iSize;
  /* Destruction waits for active iterators to detach. */
  unsigned char      pendingFree;
};

struct iterator_data {
  struct list_data * pList;
  struct item_data * pItem;
};

/* Externals */
extern struct list_data * global_lists;
extern struct list_data * group_list;

/* Locals */
void add_to_list(void * pContent, struct list_data * pList);
void * random_from_list(struct list_data * pList);
struct list_data * randomize_list(struct list_data * pList);
struct list_data * create_list(void);
void * merge_iterator(struct iterator_data * pIterator, struct list_data * pList);
void remove_iterator(struct iterator_data * pIterator);
void * next_in_list(struct iterator_data * pIterator);
void remove_from_list(void * pContent, struct list_data * pList);
struct item_data * find_in_list(void * pContent, struct list_data * pList);
void * simple_list(struct list_data * pList);
void free_list(struct list_data * pList);
void clear_simple_list(void);
#endif
