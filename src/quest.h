/* ***********************************************************************
*    File:   quest.h                                  Part of CircleMUD  *
* Version:   2.1 (December 2005) Written for CircleMud CWG / Suntzu      *
* Purpose:   To provide special quest-related code.                      *
* Copyright: Kenneth Ray                                                 *
* Original Version Details:                                              *
* Morgaelin - quest.h                     *
* Copyright (C) 1997 MS                                                  *
*********************************************************************** */
#ifndef _QUEST_H_
#define _QUEST_H_

/* Aquest related defines ********************************************* */
#define AQ_UNDEFINED       -1   /* (R) Quest unavailable                */
#define AQ_OBJ_FIND         0   /* Player must retreive object          */
#define AQ_ROOM_FIND        1   /* Player must reach room               */
#define AQ_MOB_FIND         2   /* Player must find mob                 */
#define AQ_MOB_KILL         3   /* Player must kill mob                 */
#define AQ_MOB_SAVE         4   /* Player must save mob                 */
#define AQ_OBJ_RETURN       5   /* Player gives object to mob in val5   */
#define AQ_ROOM_CLEAR       6   /* Player must clear room of all mobs   */
#define NUM_AQ_TYPES        7   /* Used in qedit functions              */

#define MAX_QUEST_NAME     40   /* Length of quest name                 */
#define MAX_QUEST_DESC     75   /* Length of quest description          */
#define MAX_QUEST_MSG    2048   /* Length of quest message strings      */

#define SCMD_QUEST_LIST     0   /* List quests available at questmaster */
#define SCMD_QUEST_HISTORY  1   /* Show history of completed quests     */
#define SCMD_QUEST_JOIN     2   /* Join a quest at a questmaster        */
#define SCMD_QUEST_LEAVE    3   /* Leave a quest                        */
#define SCMD_QUEST_PROGRESS 4   /* Show progress of current quest       */
#define SCMD_QUEST_STATUS   5   /* Show complete details of a quest     */
/* AQ Flags (much room for expansion) ********************************* */
#define AQ_REPEATABLE (1 << 0)  /* Quest can be repeated                */
#define NUM_AQ_FLAGS        1
/* Main quest struct ************************************************** */
struct aq_data {
  qst_vnum vnum;                /* Virtual nr of the quest              */
  char     *name;               /* For qlist and the sort               */
  char     *desc;               /* Description of the quest             */
  char     *info;               /* Message displayed when accepted      */
  char     *done;               /* Message displayed when completed     */
  char     *quit;               /* Message displayed when quit quest    */
  long     flags;               /* Flags (repeatable, etc               */
  int      type;                /* Quest type                           */
  mob_vnum qm;                  /* questmaster offering quest           */
  int      target;              /* Target value                         */
  obj_vnum prereq;              /* Object required to undertake quest   */
  int      value[7];            /* Quest values                         */
  int      gold_reward;         /* Number of gold coins given as reward */
  int      exp_reward;          /* Experience points given as a reward  */
  obj_vnum obj_reward;          /* vnum of object given as a reward     */
  qst_vnum prev_quest;          /* Link to prev quest, NOTHING is open  */
  qst_vnum next_quest;          /* Link to next quest, NOTHING is end   */
  SPECIAL  (*func);             /* secondary spec_proc for the QM       */
};
#define QST_NUM(i)             (aquest_table[i].vnum)
#define QST_NAME(i)            (aquest_table[i].name)
#define QST_DESC(i)            (aquest_table[i].desc)
#define QST_INFO(i)            (aquest_table[i].info)
#define QST_DONE(i)            (aquest_table[i].done)
#define QST_QUIT(i)            (aquest_table[i].quit)
#define QST_TYPE(i)            (aquest_table[i].type)
#define QST_FLAGS(i)           (aquest_table[i].flags)
#define QST_MASTER(i)          (aquest_table[i].qm)
#define QST_TARGET(i)          (aquest_table[i].target)
#define QST_PREREQ(i)          (aquest_table[i].prereq)
#define QST_POINTS(i)          (aquest_table[i].value[0])
#define QST_PENALTY(i)         (aquest_table[i].value[1])
#define QST_MINLEVEL(i)        (aquest_table[i].value[2])
#define QST_MAXLEVEL(i)        (aquest_table[i].value[3])
#define QST_TIME(i)            (aquest_table[i].value[4])
#define QST_RETURNMOB(i)       (aquest_table[i].value[5])
#define QST_QUANTITY(i)        (aquest_table[i].value[6])
#define QST_GOLD(i)            (aquest_table[i].gold_reward)
#define QST_EXP(i)             (aquest_table[i].exp_reward)
#define QST_OBJ(i)             (aquest_table[i].obj_reward)

#define QST_FUNC(i)            (aquest_table[i].func)
#define QST_PREV(i)            (aquest_table[i].prev_quest)
#define QST_NEXT(i)            (aquest_table[i].next_quest)
/* Quest Functions **************************************************** */
/* Implemented in quest.c */
void destroy_quests(void);
void assign_the_quests(void);
void parse_quest(FILE *quest_f, int nr);
int  count_quests(qst_vnum low, qst_vnum high);
void list_quests(struct char_data *ch, zone_rnum zone, qst_vnum vmin, qst_vnum vmax);
void set_quest(struct char_data *ch, qst_rnum rnum);
void clear_quest(struct char_data *ch);
void generic_complete_quest(struct char_data *ch);
void autoquest_trigger_check(struct char_data *ch, struct char_data *vict, struct obj_data *object, int type);
qst_rnum real_quest(qst_vnum vnum);
int is_complete(struct char_data *ch, qst_vnum vnum);
qst_vnum find_quest_by_qmnum(struct char_data *ch, mob_rnum qm, int num);
void add_completed_quest(struct char_data *ch, qst_vnum vnum);
void remove_completed_quest(struct char_data *ch, qst_vnum vnum);
void quest_timeout(struct char_data *ch);
void check_timed_quests(void);
SPECIAL(questmaster);
ACMD(do_quest);
/* Implemented in qedit.c  */
void qedit_parse(struct descriptor_data *d, char *arg);
void qedit_string_cleanup(struct descriptor_data *d, int terminator);
/* Implemented in genqst.c */
int copy_quest_strings(struct aq_data *from, struct aq_data *to);
int copy_quest(struct aq_data *from, struct aq_data *to, int free_old_strings);
void free_quest_strings(struct aq_data *quest);
void free_quest(struct aq_data *quest);
int add_quest(struct aq_data *nqst);
int delete_quest(qst_rnum rnum);
int save_quests(zone_rnum zone_num);

/* Qedit Connectedness ************************************************ */
#define QEDIT_MAIN_MENU                 0
#define QEDIT_CONFIRM_SAVESTRING        1
#define QEDIT_NAME                      2
#define QEDIT_DESC                      3
#define QEDIT_INFO                      4
#define QEDIT_COMPLETE                  5
#define QEDIT_ABANDON                   6
#define QEDIT_QUESTMASTER               7
#define QEDIT_TYPES                     8
#define QEDIT_FLAGS                     9
#define QEDIT_TARGET                   10
#define QEDIT_QUANTITY                 11
#define QEDIT_POINTSCOMP               12
#define QEDIT_POINTSQUIT               13
#define QEDIT_LEVELMIN                 14
#define QEDIT_LEVELMAX                 15
#define QEDIT_PREREQ                   16
#define QEDIT_TIMELIMIT                17
#define QEDIT_RETURNMOB                18
#define QEDIT_NEXTQUEST                19
#define QEDIT_PREVQUEST                20
#define QEDIT_CONFIRM_DELETE           21
#define QEDIT_GOLD                     22
#define QEDIT_EXP                      23
#define QEDIT_OBJ                      24
/* ******************************************************************** */

/* AQ Global Variables ************************************************ */
#ifndef __QUEST_C__
extern const char *aq_flags[];       /* names for quest flags (quest.c) */
extern const char *quest_types[];    /* named for quest types (quest.c) */
#endif /* __QUEST_C__ */

#endif /* _QUEST_H_ */
