/**
* @file genobj.h
* Generic OLC Library - Objects.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
* 
* This source code, which was not part of the CircleMUD legacy code,
* is attributed to:
* Copyright 1996 by Harvey Gilpin, 1997-2001 by George Greer.                                                    
*/
#ifndef _GENOBJ_H_
#define _GENOBJ_H_

void free_object_strings_proto(struct obj_data *obj);
void free_object_strings(struct obj_data *obj);
int copy_object(struct obj_data *to, struct obj_data *from);
int copy_object_preserve(struct obj_data *to, struct obj_data *from);
int save_objects(zone_rnum vznum);
obj_rnum insert_object(struct obj_data *obj, obj_vnum ovnum);
obj_rnum adjust_objects(obj_rnum refpt);
obj_rnum index_object(struct obj_data *obj, obj_vnum ovnum, obj_rnum ornum);
obj_rnum add_object(struct obj_data *, obj_vnum ovnum);
int copy_object_main(struct obj_data *to, struct obj_data *from, int free_object);
int delete_object(obj_rnum);
bool oset_alias(struct obj_data *obj, char * argument);
bool oset_apply(struct obj_data *obj, char * argument);
bool oset_short_description(struct obj_data *obj, char * argument);
bool oset_long_description(struct obj_data *obj, char * argument);

#endif /* _GENOBJ_H_ */
