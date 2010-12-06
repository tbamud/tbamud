/***************************************************************************
 * @file mail.h                                                            *
 * Public procs, macro defs, subcommand defines mudmail system.            *
 *                                                                         *
 * Part of the core tbaMUD source code distribution, which is a derivative *
 * of, and continuation of, CircleMUD.                                     *
 *                                                                         *
 * All rights reserved.  See license for complete information.             *
 * Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University  *
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.                *
 *  By Jeremy Elson.                                                       *
 **************************************************************************/

/* A handy 'flag' define - probably better in utils.h? */
#define FLAG(n) (1 << (n))

/* Maximum size of mail in bytes (arbitrary)	*/
#define MAX_MAIL_SIZE 8192
#define MAX_MAIL_ID   2000000000

/* For mail anywhere to work, we need to know which room to find the immortal mailman in */
#define MAIL_IMMORTAL_ROOM 1205

/* Mail index flags - mostly essential info that should be known before reading mails */
#define MINDEX_DELETED   FLAG(0)  /* Mail has been marked for deletion     */
#define MINDEX_URGENT    FLAG(1)  /* Mail is flagged as urgent by sender   */
#define MINDEX_HAS_OBJ   FLAG(2)  /* Mail has an attached object           */
#define MINDEX_HAS_GOLD  FLAG(3)  /* Mail contains some gold coins         */
#define MINDEX_IS_COD    FLAG(4)  /* Mail requires some gold coins         */
#define MINDEX_FROM_MOB  FLAG(5)  /* Mail has been sent by using scripts   */
#define MINDEX_READ      FLAG(6)  /* Mail has been viewed but not received */
#define MINDEX_DRAFT     FLAG(7)  /* Mail is an unsent draft copy          */

/* Mail Flags */
#define MAIL_DELETED  1  /* Marks mail for deletion with next purge */
#define MAIL_URGENT   2  /* This mail is flagged as urgent?         */
#define MAIL_COD      3  /* Means coins amount is required payment  */
#define MAIL_FROM_MOB 4  /* Specifies that 'sender' is a mob vnum   */
#define MAIL_READ     5  /* Mail has been read by recipient         */
#define MAIL_DRAFT    6  /* Mail is a draft (not yet sent)          */

#define IS_MAIL_DELETED(i) (IS_SET(mail_table[(i)].flags, MINDEX_DELETED))
#define IS_MAIL_URGENT(i)  (IS_SET(mail_table[(i)].flags, MINDEX_URGENT))
#define IS_MAIL_DRAFT(i)   (IS_SET(mail_table[(i)].flags, MINDEX_DRAFT))
#define IS_MAIL_READ(i)    (IS_SET(mail_table[(i)].flags, MINDEX_READ))
#define IS_MAIL_COD(i)     (IS_SET(mail_table[(i)].flags, MINDEX_IS_COD))
#define MAIL_HAS_OBJECT(i) (IS_SET(mail_table[(i)].flags, MINDEX_HAS_OBJ))
#define MAIL_HAS_GOLD(i)   (IS_SET(mail_table[(i)].flags, MINDEX_HAS_GOLD))

#define MAIL_URGENT_COLOR(i) (IS_MAIL_URGENT(i) ? QBRED : QNRM)

/* Mail editor submodes */
#define MAILEDIT_INBOX        0
#define MAILEDIT_OUTBOX       1
#define MAILEDIT_MAILEDIT     2
#define MAILEDIT_VIEW         3
#define MAILEDIT_REPLY        4
#define MAILEDIT_FORWARD      5
#define MAILEDIT_RECEIVE      6
#define MAILEDIT_DELETE       7
#define MAILEDIT_BACK_TO_MENU 8
#define MAILEDIT_ATTACHMENTS  9
#define MAILEDIT_ASK_DRAFT    10
#define MAILEDIT_ASK_QUIT     11
#define MAILEDIT_OUTEDIT      12
#define MAILEDIT_OUTVIEW      13
#define MAILEDIT_OUTDELETE    14
#define MAILEDIT_RECIP_MENU   15
#define MAILEDIT_ADD_RECIP    16
#define MAILEDIT_DEL_RECIP    17
#define MAILEDIT_CLR_RECIP    18
#define MAILEDIT_ATTACH_MENU  19
#define MAILEDIT_ADD_ATTACH   20
#define MAILEDIT_DEL_ATTACH   21
#define MAILEDIT_CLR_ATTACH   22
#define MAILEDIT_GET_SUBJECT  23
#define MAILEDIT_GET_GOLD     24
#define MAILEDIT_GET_BODYTEXT 25
#define MAILEDIT_PURGE_N_QUIT 26

/* Mail Editor handy defines */
#define MAILEDIT_SUBJECT  ((OLC_MAIL(d)->mail)->subject)
#define MAILEDIT_BODYTEXT ((OLC_MAIL(d)->mail)->body)
#define MAILEDIT_GOLD     ((OLC_MAIL(d)->mail)->coins)
#define MAILEDIT_URGENT   (IS_SET_AR((OLC_MAIL(d)->mail)->mail_flags, MAIL_URGENT))
#define MAILEDIT_COD      (IS_SET_AR((OLC_MAIL(d)->mail)->mail_flags, MAIL_COD))

/* Copy modes for mail_copy */
#define MAIL_COPY_NORMAL  0
#define MAIL_COPY_FORWARD 1
#define MAIL_COPY_REPLY   2

/* Special-case sender/recipient ID's for get_sender_name, should be below zero (above zero = player id) */
#define MAIL_TO_NOBODY   0  /* Used internally - do not change this one!          */
#define MAIL_FROM_MAIL  -1  /* (sender)    Mail from the tbaMUD mail system       */
#define MAIL_TO_IMMS    -2  /* (recipient) Mail to all the MUD's immortals        */
#define MAIL_TO_ALL     -3  /* (recipient) Mail to all players (usually Imp-only) */
/* Note - changing the list above needs changing the list at the top of mail.c too */

/* This number should match the mail_groups list at the top of mail.c */
#define NUM_MAIL_GROUPS  2

#define ML_ARRAY_MAX 4
#define NO_MAIL      -1

struct recipient_list {
  long recipient;
  struct recipient_list *next;
};

struct mail_data {
  long mail_id;
  long recipient;
  long sender;
  time_t sent_time;
  char *subject;
  char *body;
  struct obj_data *attachment;
  int coins;
  int mail_flags[ML_ARRAY_MAX];
};

struct mail_edit_data {
  struct mail_data *mail;
  struct recipient_list *recipients;
};

struct mail_index {
  long mail_id;
  long recipient;
  long sender;
  time_t sent_time;
  char *subject;
  int flags;
};

struct mail_group {
  char name[MAX_NAME_LENGTH+1];   /* name entered by player                */
  long recipient;                 /* The special recipient ID (below zero) */
};

/* Functions in mail.c (in the order they appear) */
bool build_mail_index(void);                                      /* Load the mudmail index file on MUD startup    */
long new_mail_id(void);                                           /* Get a unique ID for a new mudmail             */
void free_mail_index(void);                                       /* Erase the entire mudmail index from memory    */
void free_mail(struct mail_data *mail);                           /* Free memory used by one mail_data structure   */
void extract_mail(struct mail_data *mail);                        /* Free memory used by one mail_data structure   */
int  find_mail_by_id(long mail_id);                               /* Get the mail index rnum from the mail id      */
void copy_mail_index_entry(struct mail_index *to, struct mail_index *from); /* Make a copy of an index entry       */
void copy_mail(struct mail_data *to, struct mail_data *from, int copy_mode); /* Copy the mail data to a new mail   */
int  create_mail_index_entry(struct mail_data *mail);             /* Add a mail to the mudmail index list          */
bool delete_mail_index_entry(int mail_id);                        /* Delete one mudmail from the index (by id)     */
bool save_mail_index(void);                                       /* Save the mail index, returns TRUE on success  */
int  handle_mail_obj(struct obj_data *temp, struct mail_data *ml);/* Put loaded objects into the actual mail       */
bool check_mail_dir(long mail_id);                                /* Check that a mail folder exists or create it  */
bool save_as_draft(struct char_data *ch, struct mail_data *ml);   /* Save mail as a draft for sending later        */
bool save_mail(struct mail_data *ml);                             /* Save the actual mail for the mudmail file     */
bool load_mail(long mail_id, struct mail_data *ml);               /* Load mail by id - put data into ml            */
void draft_timeout(void);                                         /* Perform a timeout on old draft mails          */
bool mail_recip_ok(const char *name);                             /* Validate a player name for recipients         */
struct char_data *find_mailman(struct char_data *ch);             /* returns a mailman in the same room as player  */
struct char_data *find_immortal_mailman(void);                    /* returns a mailman in immortal mail room       */
int  show_inbox_to_char(struct char_data *ch);                    /* Displays the player's inbox                   */
int  show_outbox_to_char(struct char_data *ch);                   /* Displays the player's outbox                  */
void give_mail_attachments(struct char_data *ch, struct mail_data *ml);  /* give all mail attachments to a player  */
int  count_recipients(struct mail_edit_data *ml_list);            /* Count the number of recipients in edited mail */
void clear_mail_data(struct mail_data *ml);                       /* Set all values in struct to zero or NULL      */
int  purge_marked_mail(struct char_data *ch);                     /* purges all marked mail for one player         */
struct mail_data *create_mail(void);                              /* Create a new blank mail_data structure        */
bool add_recipient(struct mail_edit_data *ml, long player_id);    /* Add a recipient to list for mailing           */
bool remove_recipient(struct mail_edit_data *ml, long player_id); /* Remove a recipient from the list for mailing  */
void clear_recipients(struct mail_edit_data *ml);                 /* Clear a whole recipient list                  */
char *recipient_list(struct mail_edit_data *ml);                  /* Get a list of all recipients                  */
long get_mail_group_by_name(char *name);                          /* Get the recipient id from the group name      */
void list_attachments_numbered(struct obj_data *list, struct char_data *ch); /* Stacked list of attachments        */
struct obj_data *get_attachment_numbered(struct char_data *ch, struct obj_data *list, int find_num); /* Return obj by stacked list line  */
bool mail_from_player(long to, long from, char *message_pointer); /* Send a mail from a player to another player   */
bool mail_from_mobile(struct char_data *mob);                     /* Send a mail from a mob using scripts          */
void make_cod_payment_mail(struct mail_data *orig);               /* Send an auto-COD reply back to mail sender    */
bool perform_send_edited(struct char_data *ch, struct mail_edit_data *ml_list);   /* Send to multiple recipients   */
bool perform_mob_send_edited(struct char_data *mob);              /* Send a scripted mob mudmail                   */
void create_mob_mail(struct char_data *mob);                      /* Create a blank mail structure if needed       */
bool has_mail(struct char_data *ch);                              /* returns TRUE is player has mail waiting       */
int  count_deleted(struct char_data *ch);                         /* Returns the number of mails marked as deleted */
void notify_if_playing(struct char_data *from, int recipient_id); /* Inform a player of a new mail arrival         */
void notify_on_login(struct char_data *ch);                       /* Tell player how many new mails they have      */
char *get_sender_name(struct mail_data *ml);                      /* Get the name of the mail sender by mail_data  */
char *get_sender_name_by_id(long mail_id);                        /* Get the name of the mail sender by mail_id    */
char *get_recipient_name(struct mail_data *ml);                   /* Get the name of the mail recip by mail_data   */
char *get_recipient_name_by_id(long mail_id);                     /* Get the name of the mail recipient by mail_id */
int  attachment_count(struct mail_data *ml);                      /* Count the number of mail attachments          */
char *get_mail_text(struct mail_data *ml);                        /* Create the text string thats shown to players */
void show_mail_to_char(struct char_data *ch, struct mail_data *ml); /* Display full mail details to player         */
void mail_view(struct char_data *ch, long mail_id);               /* Show a mail to a player (by mail id)          */
bool mail_receive(struct char_data *ch, long mail_id);            /* Convert a mail into an actual object in inv   */
bool mail_delmark(struct char_data *ch, long mail_id);            /* Mark a mail for deletion at next purge        */
bool mail_forward(struct char_data *ch, long mail_id, long recipient); /* Forward a mail without changing any text */
long get_id_by_inbox_num(struct char_data *ch, int num);          /* Work out which mail_id player wants to use    */
long get_id_by_outbox_num(struct char_data *ch, int num);         /* Work out which mail_id player wants to use    */
void mail_view_by_num(struct char_data *ch, int num);             /* Show a mail to player by inbox entry number   */
bool mail_receive_by_num(struct char_data *ch, int num);          /* Receive a mail by inbox entry number          */
bool mail_delmark_by_num(struct char_data *ch, int num);          /* Mark as deleted by inbox entry number         */
ACMD(do_mail);

/* Functions in mailedit.c */
ACMD(do_mailedit);                                                /* The mail edit command, called from do_mail    */
void mailedit_setup(struct descriptor_data *d);                   /* Setup a blank structure ready for editing     */
void mailedit_cleanup(struct descriptor_data *d);                 /* Free up the actual OLC structure for exiting  */
void mailedit_disp_menu(struct descriptor_data *d);               /* Display inbox menu view                       */
void mailedit_disp_mail(struct descriptor_data *d);               /* Display main mail editor (for edit/reply/etc) */
void mailedit_parse(struct descriptor_data *d, char *arg);        /* The mail mailedit parser                      */

