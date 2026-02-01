/**
* @file py_scripts.h
* 
* This set of code was not originally part of the circlemud distribution.
*/

#ifndef _PY_SCRIPTS_H_
#define _PY_SCRIPTS_H_

struct trig_data;

void python_scripts_init(void);
void python_scripts_shutdown(void);
int python_trigger_run(void *go, struct trig_data *trig, int type, int mode);
void python_debug_log(const char *message);

#endif /* _PY_SCRIPTS_H_ */
