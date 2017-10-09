/**
* @file comm.h
* Header file, prototypes of public communication functions.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
*/
#ifndef _COMM_H_
#define _COMM_H_

#define NUM_RESERVED_DESCS	8
#define COPYOVER_FILE "copyover.dat"

/* comm.c */
void close_socket(struct descriptor_data *d);
void game_info(const char *messg, ...) __attribute__ ((format (printf, 1, 2)));
size_t send_to_char(struct char_data *ch, const char *messg, ...) __attribute__
    ((format (printf, 2, 3)));
void send_to_all(const char *messg, ...) __attribute__ ((format (printf, 1,
    2)));
void send_to_room(room_rnum room, const char *messg, ...) __attribute__ ((format
    (printf, 2, 3)));
void send_to_outdoor(const char *messg, ...) __attribute__ ((format (printf, 1,
    2)));
void send_to_group(struct char_data *ch, struct group_data *group, const char * msg, ...) __attribute__ ((format
    (printf, 3, 4)));
void send_to_range(room_vnum start, room_vnum finish, const char *messg, ...)
    __attribute__ ((format (printf, 3, 4)));

/* Act type settings and flags */
#define TO_ROOM     1   /**< act() type: to everyone in room, except ch. */
#define TO_VICT     2   /**< act() type: to vict_obj. */
#define TO_NOTVICT  3   /**< act() type: to everyone in room, not ch or vict_obj. */
#define TO_CHAR     4   /**< act() type: to ch. */
#define TO_GMOTE    5   /**< act() type: to gemote channel (global emote) */
#define TO_SLEEP    128	/**< act() flag: to char, even if sleeping */
#define DG_NO_TRIG  256 /**< act() flag: don't check act trigger   */


/* act functions */
void perform_act(const char *orig, struct char_data *ch, struct obj_data *obj, void *vict_obj, struct char_data *to);
char * act(const char *str, int hide_invisible, struct char_data *ch, struct obj_data *obj, void *vict_obj, int type);

/* I/O functions */
void	write_to_q(const char *txt, struct txt_q *queue, int aliased);
int	write_to_descriptor(socket_t desc, const char *txt);
size_t	write_to_output(struct descriptor_data *d, const char *txt, ...) __attribute__ ((format (printf, 2, 3)));
size_t	vwrite_to_output(struct descriptor_data *d, const char *format, va_list args);

typedef RETSIGTYPE sigfunc(int);

void echo_off(struct descriptor_data *d);
void echo_on(struct descriptor_data *d);
void game_loop(socket_t mother_desc);
void heartbeat(int heart_pulse);
void copyover_recover(void);

/** webster dictionary lookup */
extern long last_webster_teller;

extern struct descriptor_data *descriptor_list;
extern int buf_largecount;
extern int buf_overflows;
extern int buf_switches;
extern int circle_shutdown;
extern int circle_reboot;
extern int no_specials;
extern int scheck;
extern FILE *logfile;
extern unsigned long pulse;
extern ush_int port;
extern socket_t mother_desc;
extern int next_tick;

#endif /* _COMM_H_ */
