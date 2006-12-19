#define MAX_HOUSES	100
#define MAX_GUESTS	10

#define HOUSE_PRIVATE	0


struct house_control_rec {
   room_vnum vnum;		/* vnum of this house		*/
   room_vnum atrium;		/* vnum of atrium		*/
   sh_int exit_num;		/* direction of house's exit	*/
   time_t built_on;		/* date this house was built	*/
   int mode;			/* mode of ownership		*/
   long owner;			/* idnum of house's owner	*/
   int num_of_guests;		/* how many guests for house	*/
   long guests[MAX_GUESTS];	/* idnums of house's guests	*/
   time_t last_payment;		/* date of last house payment   */
   long spare0;
   long spare1;
   long spare2;
   long spare3;
   long spare4;
   long spare5;
   long spare6;
   long spare7;
};



   
#define TOROOM(room, dir) (world[room].dir_option[dir] ? \
			    world[room].dir_option[dir]->to_room : NOWHERE)

void	House_listrent(struct char_data *ch, room_vnum vnum);
void	House_boot(void);
void	House_save_all(void);
int	House_can_enter(struct char_data *ch, room_vnum house);
void	House_crashsave(room_vnum vnum);
void	House_list_guests(struct char_data *ch, int i, int quiet);
