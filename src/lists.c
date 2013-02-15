/**************************************************************************
*  File: lists.c                                           Part of tbaMUD *
*  Usage: Handling of in-game lists                                       *
*                                                                         *
*  By Vatiken. Copyright 2012 by Joseph Arnusch                           *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "dg_event.h"

static struct iterator_data Iterator;
static bool loop = FALSE;
static struct list_data *pLastList = NULL;

/* Global lists */
struct list_data * global_lists = NULL;
struct list_data * group_list   = NULL;

struct list_data * create_list(void) 
{
  struct list_data *pNewList;
  static bool first_list = TRUE;
  
  CREATE(pNewList, struct list_data, 1);
  
  pNewList->pFirstItem = NULL;
  pNewList->pLastItem  = NULL;
  pNewList->iIterators = 0;
  pNewList->iSize      = 0;
  
  /* Add to global lists, primarily for debugging purposes */
  if (first_list == FALSE)
    add_to_list(pNewList, global_lists);
  else 
    first_list = FALSE;

  return (pNewList);
}

struct item_data * create_item(void)
{
  struct item_data *pNewItem;

  CREATE(pNewItem, struct item_data, 1);

  pNewItem->pNextItem = NULL;
  pNewItem->pPrevItem = NULL;
  pNewItem->pContent  = NULL;

  return (pNewItem);
}

void free_list(struct list_data * pList)
{
  void * pContent;
  
  clear_simple_list();  
    
  if (pList->iSize)
    while ((pContent = simple_list(pList)))
      remove_from_list(pContent, pList);
    
  if (pList->iSize > 0)
    mudlog(CMP, LVL_GOD, TRUE, "List being freed while not empty.");
      
  /* Global List for debugging */
  if (pList != global_lists)
    remove_from_list(pList, global_lists);  
  
  free(pList);
}

void add_to_list(void * pContent, struct list_data * pList)
{
  struct item_data * pNewItem;
  struct item_data * pLastItem;

  /* Allocate our memory */
  pNewItem = create_item();

  /* Place the contents in the item */
  pNewItem->pContent  = pContent;
  pNewItem->pNextItem = NULL;

  /* If we are the first entry in the list, mark us as such */
  if (pList->pFirstItem == NULL)
    pList->pFirstItem = pNewItem;
 
  /* Grab our last item from the list and attach it to our new item */
  if (pList->pLastItem) {
    pLastItem = pList->pLastItem;
    pLastItem->pNextItem = pNewItem;
    pNewItem->pPrevItem = pLastItem;
  }

  /* Make our new item our last item in the list */
  pList->pLastItem = pNewItem;

  pList->iSize++;
}

void remove_from_list(void * pContent, struct list_data * pList)
{
  struct item_data *pRemovedItem;

  if ((pRemovedItem = find_in_list(pContent, pList)) == NULL) {
    mudlog(CMP, LVL_GOD, TRUE, "WARNING: Attempting to remove contents that don't exist in list.");
    return;
  }

  if (pRemovedItem == pList->pFirstItem)
    pList->pFirstItem = pRemovedItem->pNextItem;  

  if (pRemovedItem == pList->pLastItem)
    pList->pLastItem = pRemovedItem->pPrevItem;  
 
  if (pRemovedItem->pPrevItem)
    pRemovedItem->pPrevItem->pNextItem = pRemovedItem->pNextItem;
 
  if (pRemovedItem->pNextItem)
    pRemovedItem->pNextItem->pPrevItem = pRemovedItem->pPrevItem;
  
  pList->iSize--;
  if (pList->iSize == 0) {
    pList->pFirstItem = NULL;
    pList->pLastItem  = NULL;
  }
  free(pRemovedItem);
}

/** Merges an iterator with a list
 * @post Don't forget to remove the iterator with remove_iterator().
 * */

void * merge_iterator(struct iterator_data * pIterator, struct list_data * pList)
{
  void * pContent;

  if (pList == NULL) {
    mudlog(NRM, LVL_GOD, TRUE, "WARNING: Attempting to merge iterator to NULL list.");
    pIterator->pList = NULL;
    pIterator->pItem = NULL;
    return NULL;
  }
  if (pList->pFirstItem == NULL) {
    mudlog(NRM, LVL_GOD, TRUE, "WARNING: Attempting to merge iterator to empty list.");
    pIterator->pList = NULL;
    pIterator->pItem = NULL;
    return NULL;
  }

  pList->iIterators++;
  pIterator->pList = pList;
  pIterator->pItem = pList->pFirstItem;

  pContent = pIterator->pItem ? pIterator->pItem->pContent : NULL;

  return (pContent);
}

void remove_iterator(struct iterator_data * pIterator)
{
  if (pIterator->pList == NULL) {
    mudlog(NRM, LVL_GOD, TRUE, "WARNING: Attempting to remove iterator from NULL list.");
    return;
  }

  pIterator->pList->iIterators--;
  pIterator->pList = NULL;
  pIterator->pItem = NULL;
}

/** Spits out an item and cycles down the list  
 * @return Returns the content of the list
 * */

void * next_in_list(struct iterator_data * pIterator)
{
  void * pContent;
  struct item_data * pTempItem;

  if (pIterator->pList == NULL) {
    mudlog(NRM, LVL_GOD, TRUE, "WARNING: Attempting to get content from iterator with NULL list.");
    return NULL;
  }

  /* Cycle down the list */
  pTempItem = pIterator->pItem->pNextItem;
  pIterator->pItem = pTempItem;

  /* Grab the content */
  pContent = pIterator->pItem ? pIterator->pItem->pContent : NULL;

  return (pContent);
}

/** Searches through the a list and returns the item block that holds pContent
 * @return Returns the actual item block and not the pContent itself, since
 * it is assumed you already have the pContent.
 * */

struct item_data * find_in_list(void * pContent, struct list_data * pList)
{
  struct iterator_data Iterator;
  void * pFoundItem;
  struct item_data *pItem = NULL;
  bool found;

  pFoundItem = merge_iterator(&Iterator, pList);

  for (found = FALSE; pFoundItem != NULL; pFoundItem = next_in_list(&Iterator)) {
    if (pFoundItem == pContent) {
      found = TRUE;
      break;
    }
  }

  if (found)
    pItem = Iterator.pItem;

  remove_iterator(&Iterator);

  if (found)
    return (pItem);
  else
    return NULL;
}

void clear_simple_list(void)
{
  loop = FALSE;
  pLastList = NULL;  
}

/** This is the "For Dummies" function, as although it's not as flexible,
 * it is even easier applied for list searches then using your own iterators
 * and next_in_list()
 * @usage Common usage would be as follows:
 * 
 * while ((var = (struct XXX_data *) simple_list(XXX_list))) {
 *   blah blah....
 * }
 * @return Will return the next list content until it hits the end, in which
 * will detach itself from the list.
 * */

void * simple_list(struct list_data * pList)
{
  void * pContent;

  /* Reset List */
  if (pList == NULL) {
    clear_simple_list();
    return NULL;
  }

  if (!loop || pLastList != pList) {
    if (loop && pLastList != pList)
      mudlog(CMP, LVL_GRGOD, TRUE, "SYSERR: simple_list() forced to reset itself.");
  
    pContent = merge_iterator(&Iterator, pList);
    if (pContent != NULL) {
      pLastList = pList;    
      loop = TRUE;
      return (pContent);
    } else
      return NULL;
  }
   
  if ((pContent = next_in_list(&Iterator)) != NULL)
    return (pContent);

  remove_iterator(&Iterator);  
  loop = FALSE;
  return NULL;
}

void * random_from_list(struct list_data * pList)
{
  struct iterator_data Iterator;
  void * pFoundItem;
  bool found;
  int number;
  int count = 1;

  if (pList->iSize <= 0)
    return NULL;
  else
    number = rand_number(1, pList->iSize);

  pFoundItem = merge_iterator(&Iterator, pList);

  for (found = FALSE; pFoundItem != NULL; pFoundItem = next_in_list(&Iterator), count++) {
    if (count == number) {
      found = TRUE;
      break;
    }
  }

  remove_iterator(&Iterator);

  if (found)
    return (pFoundItem);
  else
    return NULL;
}

struct list_data * randomize_list(struct list_data * pList)
{
  struct list_data * newList;
  void * pContent;
  
  if (pList->iSize == 0)
    return NULL;
    
  newList = create_list();
  
  while ((pContent = random_from_list(pList)) != NULL) {
    remove_from_list(pContent, pList);
    add_to_list(pContent, newList);
  }
  
  free_list(pList);
  
  return (newList);
}
