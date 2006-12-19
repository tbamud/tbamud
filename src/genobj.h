/************************************************************************
 * Generic OLC Library - Objects / genobj.h			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

void copy_object_strings(struct obj_data *to, struct obj_data *from);
void free_object_strings(struct obj_data *obj);
void free_object_strings_proto(struct obj_data *obj);
int copy_object(struct obj_data *to, struct obj_data *from);
int copy_object_preserve(struct obj_data *to, struct obj_data *from);
int save_objects(zone_rnum vznum);
obj_rnum insert_object(struct obj_data *obj, obj_vnum ovnum);
obj_rnum adjust_objects(obj_rnum refpt);
obj_rnum index_object(struct obj_data *obj, obj_vnum ovnum, obj_rnum ornum);
int update_objects(struct obj_data *refobj);
obj_rnum add_object(struct obj_data *, obj_vnum ovnum);
int copy_object_main(struct obj_data *to, struct obj_data *from, int free_object);
int delete_object(obj_rnum); 

