/**
* @file py_variables.c
* 
* This set of code was not originally part of the circlemud distribution.
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "py_triggers.h"
#include "db.h"

void add_var(struct trig_var_data **var_list, const char *name, const char *value, long id)
{
  struct trig_var_data *vd;

  if (!name || !*name)
    return;

  for (vd = *var_list; vd; vd = vd->next) {
    if (!str_cmp(name, vd->name)) {
      if (vd->value)
        free(vd->value);
      vd->value = strdup(value ? value : "");
      vd->context = id;
      return;
    }
  }

  CREATE(vd, struct trig_var_data, 1);
  vd->name = strdup(name);
  vd->value = strdup(value ? value : "");
  vd->context = id;
  vd->next = *var_list;
  *var_list = vd;
}
