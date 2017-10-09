/**
 * Copyright 2012 Joseph Arnusch
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 **/

/* Externals */
void load_messages(void);
void free_messages(void);
void save_messages_to_disk(void);
void free_message_list(struct message_list * mlist);

/* Defines */
#define MSGEDIT_MAIN_MENU     1
#define MSGEDIT_CONFIRM_SAVE  2
#define MSGEDIT_TYPE          3
#define MSGEDIT_DEATH_CHAR    4
#define MSGEDIT_DEATH_VICT    5
#define MSGEDIT_DEATH_ROOM    6
#define MSGEDIT_MISS_CHAR     7
#define MSGEDIT_MISS_VICT     8
#define MSGEDIT_MISS_ROOM     9
#define MSGEDIT_HIT_CHAR      10
#define MSGEDIT_HIT_VICT      11
#define MSGEDIT_HIT_ROOM      12
#define MSGEDIT_GOD_CHAR      13
#define MSGEDIT_GOD_VICT      14
#define MSGEDIT_GOD_ROOM      15
