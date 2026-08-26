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

/* simple_list() intentionally provides one non-reentrant convenience cursor. */
static struct iterator_data simpleIterator;
static bool simpleLoop = FALSE;
static struct list_data *pSimpleLastList = NULL;

/* Global lists */
struct list_data * global_lists = NULL;
struct list_data * group_list   = NULL;

static struct item_data * create_item(void);
static void destroy_list(struct list_data * pList);
static void free_removed_items(struct list_data * pList);
static void unlink_item(struct item_data * pItem, struct list_data * pList);

struct list_data * create_list(void)
{
  struct list_data *pNewList;

  CREATE(pNewList, struct list_data, 1);

  pNewList->pFirstItem    = NULL;
  pNewList->pLastItem     = NULL;
  pNewList->pRemovedItems = NULL;
  pNewList->iIterators    = 0;
  pNewList->iSize         = 0;
  pNewList->pendingFree   = FALSE;

  /* boot_db() explicitly creates global_lists before all registered lists. */
  if (global_lists != NULL)
    add_to_list(pNewList, global_lists);

  return (pNewList);
}

static struct item_data * create_item(void)
{
  struct item_data *pNewItem;

  CREATE(pNewItem, struct item_data, 1);

  pNewItem->pNextItem    = NULL;
  pNewItem->pPrevItem    = NULL;
  pNewItem->pNextRemoved = NULL;
  pNewItem->pContent     = NULL;
  pNewItem->isRemoved    = FALSE;

  return (pNewItem);
}

/* Removed nodes retain their next link until every active iterator detaches. */
static void free_removed_items(struct list_data * pList)
{
  struct item_data *pItem;
  struct item_data *pNext;

  if (pList == NULL || pList->iIterators != 0)
    return;

  pItem = pList->pRemovedItems;
  while (pItem != NULL) {
    pNext = pItem->pNextRemoved;
    free(pItem);
    pItem = pNext;
  }
  pList->pRemovedItems = NULL;
}

static void destroy_list(struct list_data * pList)
{
  struct item_data *pItem;
  struct item_data *pNext;

  if (pList == NULL || pList->iIterators != 0)
    return;

  pItem = pList->pFirstItem;
  while (pItem != NULL) {
    pNext = pItem->pNextItem;
    free(pItem);
    pItem = pNext;
  }

  pList->pFirstItem = NULL;
  pList->pLastItem = NULL;
  pList->iSize = 0;
  free_removed_items(pList);
  free(pList);
}

static void unlink_item(struct item_data * pItem, struct list_data * pList)
{
  if (pItem == NULL || pList == NULL || pItem->isRemoved)
    return;

  if (pItem == pList->pFirstItem)
    pList->pFirstItem = pItem->pNextItem;
  if (pItem == pList->pLastItem)
    pList->pLastItem = pItem->pPrevItem;
  if (pItem->pPrevItem != NULL)
    pItem->pPrevItem->pNextItem = pItem->pNextItem;
  if (pItem->pNextItem != NULL)
    pItem->pNextItem->pPrevItem = pItem->pPrevItem;

  if (pList->iSize > 0)
    pList->iSize--;
  if (pList->iSize == 0) {
    pList->pFirstItem = NULL;
    pList->pLastItem = NULL;
  }

  pItem->isRemoved = TRUE;
  if (pList->iIterators > 0) {
    pItem->pNextRemoved = pList->pRemovedItems;
    pList->pRemovedItems = pItem;
  } else {
    free(pItem);
  }
}

void free_list(struct list_data * pList)
{
  struct item_data *pRegistryItem;

  if (pList == NULL || pList->pendingFree)
    return;

  /* Do not disrupt a simple_list() traversal of an unrelated list. */
  if (simpleIterator.pList == pList)
    clear_simple_list();

  if (pList == global_lists) {
    global_lists = NULL;
  } else if (global_lists != NULL && !global_lists->pendingFree) {
    pRegistryItem = find_in_list(pList, global_lists);
    if (pRegistryItem != NULL)
      unlink_item(pRegistryItem, global_lists);
  }

  if (pList == group_list)
    group_list = NULL;

  /* An iterator may still hold a node address, so delay physical destruction. */
  pList->pendingFree = TRUE;
  if (pList->iIterators == 0)
    destroy_list(pList);
}

void add_to_list(void * pContent, struct list_data * pList)
{
  struct item_data *pNewItem;
  struct item_data *pLastItem;

  if (pList == NULL) {
    mudlog(CMP, LVL_GOD, TRUE, "SYSERR: add_to_list() called with NULL list pointer.");
    return;
  }
  if (pList->pendingFree) {
    mudlog(CMP, LVL_GOD, TRUE, "SYSERR: add_to_list() called for a list pending destruction.");
    return;
  }
  if (pContent == NULL) {
    mudlog(CMP, LVL_GOD, TRUE, "SYSERR: add_to_list() called with NULL content.");
    return;
  }

  pNewItem = create_item();
  pNewItem->pContent = pContent;

  if (pList->pFirstItem == NULL)
    pList->pFirstItem = pNewItem;

  if (pList->pLastItem != NULL) {
    pLastItem = pList->pLastItem;
    pLastItem->pNextItem = pNewItem;
    pNewItem->pPrevItem = pLastItem;
  }

  pList->pLastItem = pNewItem;
  pList->iSize++;
}

void remove_from_list(void * pContent, struct list_data * pList)
{
  struct item_data *pRemovedItem;

  if (pList == NULL) {
    mudlog(CMP, LVL_GOD, TRUE, "SYSERR: remove_from_list() called with NULL list pointer.");
    return;
  }
  if (pList->pendingFree) {
    mudlog(CMP, LVL_GOD, TRUE,
           "SYSERR: remove_from_list() called for a list pending destruction.");
    return;
  }

  pRemovedItem = find_in_list(pContent, pList);
  if (pRemovedItem == NULL) {
    mudlog(CMP, LVL_GOD, TRUE,
           "WARNING: Attempting to remove contents that don't exist in list.");
    return;
  }

  unlink_item(pRemovedItem, pList);
}

/** Merges an iterator with a list
 * @post remove_iterator() may be called after traversal or an early exit.
 */
void * merge_iterator(struct iterator_data * pIterator, struct list_data * pList)
{
  if (pIterator == NULL) {
    mudlog(CMP, LVL_GOD, TRUE, "SYSERR: merge_iterator() called with NULL iterator.");
    return NULL;
  }
  if (pList == NULL) {
    mudlog(NRM, LVL_GOD, TRUE, "WARNING: Attempting to merge iterator to NULL list.");
    pIterator->pList = NULL;
    pIterator->pItem = NULL;
    return NULL;
  }
  if (pList->pendingFree) {
    mudlog(CMP, LVL_GOD, TRUE,
           "SYSERR: merge_iterator() called for a list pending destruction.");
    pIterator->pList = NULL;
    pIterator->pItem = NULL;
    return NULL;
  }
  if (pList->pFirstItem == NULL) {
    pIterator->pList = NULL;
    pIterator->pItem = NULL;
    return NULL;
  }

  pIterator->pItem = pList->pFirstItem;
  while (pIterator->pItem != NULL &&
         (pIterator->pItem->isRemoved || pIterator->pItem->pContent == NULL))
    pIterator->pItem = pIterator->pItem->pNextItem;

  if (pIterator->pItem == NULL) {
    pIterator->pList = NULL;
    return NULL;
  }

  pList->iIterators++;
  pIterator->pList = pList;
  return (pIterator->pItem->pContent);
}

void remove_iterator(struct iterator_data * pIterator)
{
  struct list_data *pList;

  if (pIterator == NULL) {
    mudlog(CMP, LVL_GOD, TRUE, "SYSERR: remove_iterator() called with NULL iterator.");
    return;
  }
  if (pIterator->pList == NULL)
    return;

  pList = pIterator->pList;
  pIterator->pList = NULL;
  pIterator->pItem = NULL;

  if (pList->iIterators == 0) {
    mudlog(CMP, LVL_GOD, TRUE,
           "SYSERR: remove_iterator() found an invalid zero iterator count.");
    return;
  }

  pList->iIterators--;
  if (pList->iIterators == 0) {
    free_removed_items(pList);
    if (pList->pendingFree)
      destroy_list(pList);
  }
}

/** Advances an iterator and returns the next list content. */
void * next_in_list(struct iterator_data * pIterator)
{
  struct item_data *pNextItem;

  if (pIterator == NULL) {
    mudlog(CMP, LVL_GOD, TRUE, "SYSERR: next_in_list() called with NULL iterator.");
    return NULL;
  }
  if (pIterator->pList == NULL) {
    mudlog(NRM, LVL_GOD, TRUE,
           "WARNING: Attempting to get content from iterator with NULL list.");
    return NULL;
  }
  if (pIterator->pList->pendingFree) {
    remove_iterator(pIterator);
    return NULL;
  }
  if (pIterator->pItem == NULL) {
    remove_iterator(pIterator);
    return NULL;
  }

  pNextItem = pIterator->pItem->pNextItem;
  while (pNextItem != NULL && (pNextItem->isRemoved || pNextItem->pContent == NULL))
    pNextItem = pNextItem->pNextItem;
  pIterator->pItem = pNextItem;

  if (pIterator->pItem == NULL) {
    remove_iterator(pIterator);
    return NULL;
  }

  return (pIterator->pItem->pContent);
}

/** Finds the node containing pContent by pointer identity. */
struct item_data * find_in_list(void * pContent, struct list_data * pList)
{
  struct item_data *pItem;

  if (pList == NULL) {
    mudlog(CMP, LVL_GOD, TRUE, "SYSERR: find_in_list() called with NULL list pointer.");
    return NULL;
  }
  if (pContent == NULL || pList->pendingFree)
    return NULL;

  for (pItem = pList->pFirstItem; pItem != NULL; pItem = pItem->pNextItem)
    if (!pItem->isRemoved && pItem->pContent == pContent)
      return (pItem);

  return NULL;
}

void clear_simple_list(void)
{
  if (simpleIterator.pList != NULL)
    remove_iterator(&simpleIterator);

  simpleIterator.pList = NULL;
  simpleIterator.pItem = NULL;
  simpleLoop = FALSE;
  pSimpleLastList = NULL;
}

/**
 * Convenience iteration for a single, non-nested traversal.
 * Explicit iterators must be used for nested traversals.
 */
void * simple_list(struct list_data * pList)
{
  void *pContent;

  if (pList == NULL) {
    clear_simple_list();
    return NULL;
  }

  if (!simpleLoop || pSimpleLastList != pList) {
    if (simpleLoop && pSimpleLastList != pList)
      mudlog(CMP, LVL_GRGOD, TRUE, "SYSERR: simple_list() forced to reset itself.");

    clear_simple_list();
    pContent = merge_iterator(&simpleIterator, pList);
    if (pContent == NULL)
      return NULL;

    pSimpleLastList = pList;
    simpleLoop = TRUE;
    return (pContent);
  }

  pContent = next_in_list(&simpleIterator);
  if (pContent == NULL) {
    simpleLoop = FALSE;
    pSimpleLastList = NULL;
  }

  return (pContent);
}

void * random_from_list(struct list_data * pList)
{
  struct item_data *pItem;
  size_t number;

  if (pList == NULL) {
    mudlog(CMP, LVL_GOD, TRUE, "SYSERR: random_from_list() called with NULL list pointer.");
    return NULL;
  }
  if (pList->pendingFree) {
    mudlog(CMP, LVL_GOD, TRUE,
           "SYSERR: random_from_list() called for a list pending destruction.");
    return NULL;
  }
  if (pList->iSize == 0)
    return NULL;

  number = (size_t) circle_random() % pList->iSize;
  for (pItem = pList->pFirstItem; pItem != NULL && number > 0; pItem = pItem->pNextItem)
    number--;

  if (pItem == NULL || pItem->isRemoved || pItem->pContent == NULL) {
    mudlog(CMP, LVL_GOD, TRUE,
           "SYSERR: random_from_list() found inconsistent list size or content.");
    return NULL;
  }

  return (pItem->pContent);
}

struct list_data * randomize_list(struct list_data * pList)
{
  struct list_data *newList;
  void *pContent;

  if (pList == NULL) {
    mudlog(CMP, LVL_GOD, TRUE, "SYSERR: randomize_list() called with NULL list pointer.");
    return NULL;
  }
  if (pList->pendingFree) {
    mudlog(CMP, LVL_GOD, TRUE,
           "SYSERR: randomize_list() called for a list pending destruction.");
    return NULL;
  }
  if (pList->iSize == 0) {
    free_list(pList);
    return NULL;
  }

  newList = create_list();
  while (pList->iSize > 0) {
    pContent = random_from_list(pList);
    if (pContent == NULL) {
      free_list(newList);
      return NULL;
    }
    remove_from_list(pContent, pList);
    add_to_list(pContent, newList);
  }

  free_list(pList);
  return (newList);
}
