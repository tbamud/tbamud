/**
* @file py_scripts.c
* 
* This set of code was not originally part of the circlemud distribution.
*/

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "conf.h"
#include "sysdep.h"
#include <fcntl.h>
#include "structs.h"
#include "py_triggers.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "handler.h"
#include "py_event.h"
#include "class.h"
#include "species.h"
#include "py_scripts.h"
#include "graph.h"

#define SCRIPT_NAME_MAX 256

struct py_script_cache {
  char *path;
  time_t mtime;
  PyObject *globals;
  struct py_script_cache *next;
};

struct dsl_script_cache {
  char *path;
  time_t mtime;
  PyObject *globals;
  PyObject *func;
  struct dsl_script_cache *next;
};

struct dsl_state {
  char *path;
  int self_type;
  long self_uid;
  time_t mtime;
  PyObject *generator;
  int running;
  struct dsl_state *next;
};

struct py_trigger_context {
  void *go;
  int type;
  struct trig_data *trig;
};

struct py_call_data {
  char *script;
  char *func;
  int self_type;
  long self_uid;
  PyObject *args;
  PyObject *kwargs;
};

struct dsl_call_data {
  struct dsl_state *state;
};

static struct py_script_cache *script_cache = NULL;
static struct dsl_script_cache *dsl_cache = NULL;
static struct dsl_state *dsl_states = NULL;
static struct py_trigger_context *current_context = NULL;
static FILE *script_log_fp = NULL;
static char script_log_path[PATH_MAX];

static PyObject *mud_module = NULL;
static PyObject *mud_sleep_exc = NULL;
static const char *mud_dir_names[] = {
  "n", "s", "e", "w",
  "ne", "nw", "se", "sw",
  "u", "d",
  "north", "south", "east", "west",
  "northeast", "northwest", "southeast", "southwest",
  "up", "down",
  NULL
};

static int python_audit_hook(const char *event, PyObject *args, void *userData);
static int python_path_is_script_log(const char *path);
static void python_log_exception(const char *context);
static void python_log_message(const char *message);
static void python_rotate_script_log(void);
static struct py_script_cache *python_load_script(const char *path);
static int python_is_dsl_script(const char *path);
static struct dsl_script_cache *dsl_load_script(const char *path);
static struct dsl_state *dsl_get_state(const char *path, int type, long uid);
static int dsl_trigger_run(void *go, struct trig_data *trig, int type, const char *path);
static char *dsl_build_source(const char *path);
static int dsl_run_state(struct dsl_state *state, PyObject *func, PyObject *npc, PyObject *room,
                          PyObject *pc, PyObject *object);
static PyObject *python_build_event(struct trig_data *trig, void *go, int type);
static PyObject *python_entity_from_uid(long uid, int kind);
static int python_uid_kind(long uid);
static long python_entity_uid_from_obj(PyObject *obj, int *kind);
static EVENTFUNC(python_call_event);
static EVENTFUNC(dsl_call_event);
static obj_data *python_find_obj(long uid);
static room_data *python_find_room(long uid);
static PyObject *MudEntity_getattro(PyObject *obj, PyObject *nameobj);
static PyObject *MudEntity_richcompare(PyObject *a, PyObject *b, int op);
static PyTypeObject MudEntityType;

/* Mud entity type */
typedef struct {
  PyObject_HEAD
  int kind;
  long uid;
} MudEntity;

static PyObject *MudEntity_repr(MudEntity *self)
{
  char buf[128];
  const char *kind = "unknown";

  if (self->kind == MOB_TRIGGER)
    kind = "mob";
  else if (self->kind == OBJ_TRIGGER)
    kind = "obj";
  else if (self->kind == WLD_TRIGGER)
    kind = "room";

  snprintf(buf, sizeof(buf), "<mud.%s uid=%ld>", kind, self->uid);
  return PyUnicode_FromString(buf);
}

static char_data *MudEntity_get_char(MudEntity *self)
{
  if (self->kind != MOB_TRIGGER)
    return NULL;
  return find_char(self->uid);
}

static obj_data *MudEntity_get_obj(MudEntity *self)
{
  if (self->kind != OBJ_TRIGGER)
    return NULL;
  return python_find_obj(self->uid);
}

static room_data *MudEntity_get_room(MudEntity *self)
{
  if (self->kind != WLD_TRIGGER)
    return NULL;
  return python_find_room(self->uid);
}

static PyObject *MudEntity_get_kind(MudEntity *self, void *closure)
{
  return PyLong_FromLong(self->kind);
}

static PyObject *MudEntity_get_uid(MudEntity *self, void *closure)
{
  return PyLong_FromLong(self->uid);
}

static PyObject *MudEntity_get_vnum(MudEntity *self, void *closure)
{
  if (self->kind == MOB_TRIGGER) {
    char_data *ch = MudEntity_get_char(self);
    if (!ch)
      Py_RETURN_NONE;
    if (!IS_NPC(ch))
      return PyLong_FromLong(-1);
    return PyLong_FromLong(GET_MOB_VNUM(ch));
  }

  if (self->kind == OBJ_TRIGGER) {
    obj_data *obj = MudEntity_get_obj(self);
    if (!obj)
      Py_RETURN_NONE;
    return PyLong_FromLong(GET_OBJ_VNUM(obj));
  }

  if (self->kind == WLD_TRIGGER) {
    room_data *room = MudEntity_get_room(self);
    if (!room)
      Py_RETURN_NONE;
    return PyLong_FromLong(room->number);
  }

  Py_RETURN_NONE;
}

static PyObject *MudEntity_get_name(MudEntity *self, void *closure)
{
  if (self->kind == MOB_TRIGGER) {
    char_data *ch = MudEntity_get_char(self);
    if (!ch)
      Py_RETURN_NONE;
    return PyUnicode_FromString(GET_NAME(ch));
  }

  if (self->kind == OBJ_TRIGGER) {
    obj_data *obj = MudEntity_get_obj(self);
    const char *name;
    if (!obj)
      Py_RETURN_NONE;
    name = obj->short_description ? obj->short_description : obj->name;
    return PyUnicode_FromString(name ? name : "");
  }

  if (self->kind == WLD_TRIGGER) {
    room_data *room = MudEntity_get_room(self);
    if (!room)
      Py_RETURN_NONE;
    return PyUnicode_FromString(room->name ? room->name : "");
  }

  Py_RETURN_NONE;
}

static PyObject *MudEntity_build_room_contents(room_data *room)
{
  PyObject *list;
  obj_data *obj;

  if (!room)
    Py_RETURN_NONE;

  list = PyList_New(0);
  if (!list)
    return NULL;

  for (obj = room->contents; obj; obj = obj->next_content) {
    PyObject *ent = python_entity_from_uid(obj_script_id(obj), OBJ_TRIGGER);
    if (!ent)
      continue;
    PyList_Append(list, ent);
    Py_DECREF(ent);
  }

  return list;
}

static PyObject *MudEntity_build_room_people(room_data *room)
{
  PyObject *list;
  char_data *ch;

  if (!room)
    Py_RETURN_NONE;

  list = PyList_New(0);
  if (!list)
    return NULL;

  for (ch = room->people; ch; ch = ch->next_in_room) {
    PyObject *ent = python_entity_from_uid(char_script_id(ch), MOB_TRIGGER);
    if (!ent)
      continue;
    PyList_Append(list, ent);
    Py_DECREF(ent);
  }

  return list;
}

static PyObject *MudEntity_build_obj_oval(obj_data *obj)
{
  PyObject *list;
  int i;

  if (!obj)
    Py_RETURN_NONE;

  list = PyList_New(NUM_OBJ_VAL_POSITIONS);
  if (!list)
    return NULL;

  for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++) {
    PyObject *val = PyLong_FromLong(GET_OBJ_VAL(obj, i));
    if (!val) {
      Py_DECREF(list);
      return NULL;
    }
    PyList_SetItem(list, i, val);
  }

  return list;
}

static PyObject *MudEntity_getattro(PyObject *obj, PyObject *nameobj)
{
  MudEntity *self = (MudEntity *)obj;
  const char *name;

  if (!PyUnicode_Check(nameobj))
    return PyObject_GenericGetAttr(obj, nameobj);

  name = PyUnicode_AsUTF8(nameobj);
  if (!name)
    return NULL;

  if (self->kind == MOB_TRIGGER) {
    char_data *ch = MudEntity_get_char(self);
    if (!ch)
      Py_RETURN_NONE;
    if (!strcmp(name, "health"))
      return PyLong_FromLong(GET_HIT(ch));
    if (!strcmp(name, "mana"))
      return PyLong_FromLong(GET_MANA(ch));
    if (!strcmp(name, "stamina") || !strcmp(name, "move"))
      return PyLong_FromLong(GET_STAMINA(ch));
    if (!strcmp(name, "max_health"))
      return PyLong_FromLong(GET_MAX_HIT(ch));
    if (!strcmp(name, "max_mana"))
      return PyLong_FromLong(GET_MAX_MANA(ch));
    if (!strcmp(name, "max_stamina") || !strcmp(name, "max_move"))
      return PyLong_FromLong(GET_MAX_STAMINA(ch));
    if (!strcmp(name, "class"))
      return PyUnicode_FromString(CLASS_NAME(ch));
    if (!strcmp(name, "class_id"))
      return PyLong_FromLong(GET_CLASS(ch));
    if (!strcmp(name, "species"))
      return PyUnicode_FromString(get_species_name(GET_SPECIES(ch)));
    if (!strcmp(name, "species_id"))
      return PyLong_FromLong(GET_SPECIES(ch));
    if (!strcmp(name, "is_pc"))
      return PyBool_FromLong(!IS_NPC(ch));
    if (!strcmp(name, "is_npc"))
      return PyBool_FromLong(IS_NPC(ch));
    if (!strcmp(name, "keyword")) {
      const char *kw = IS_NPC(ch) ? GET_KEYWORDS(ch) : GET_NAME(ch);
      return PyUnicode_FromString(kw ? kw : "");
    }
    if (!strcmp(name, "room")) {
      if (IN_ROOM(ch) != NOWHERE) {
        room_data *room = &world[IN_ROOM(ch)];
        return python_entity_from_uid(room_script_id(room), WLD_TRIGGER);
      }
      Py_RETURN_NONE;
    }
  }

  if (self->kind == OBJ_TRIGGER) {
    if (!strcmp(name, "keyword")) {
      obj_data *objp = MudEntity_get_obj(self);
      return PyUnicode_FromString(objp && objp->name ? objp->name : "");
    }
    if (!strcmp(name, "oval"))
      return MudEntity_build_obj_oval(MudEntity_get_obj(self));
    if (!strcmp(name, "room")) {
      obj_data *objp = MudEntity_get_obj(self);
      room_rnum rnum;
      if (!objp)
        Py_RETURN_NONE;
      rnum = objp->in_room;
      if (rnum != NOWHERE) {
        room_data *room = &world[rnum];
        return python_entity_from_uid(room_script_id(room), WLD_TRIGGER);
      }
      if (objp->carried_by && IN_ROOM(objp->carried_by) != NOWHERE) {
        room_data *room = &world[IN_ROOM(objp->carried_by)];
        return python_entity_from_uid(room_script_id(room), WLD_TRIGGER);
      }
      if (objp->worn_by && IN_ROOM(objp->worn_by) != NOWHERE) {
        room_data *room = &world[IN_ROOM(objp->worn_by)];
        return python_entity_from_uid(room_script_id(room), WLD_TRIGGER);
      }
      Py_RETURN_NONE;
    }
  }

  if (self->kind == WLD_TRIGGER) {
    if (!strcmp(name, "contents"))
      return MudEntity_build_room_contents(MudEntity_get_room(self));
    if (!strcmp(name, "people"))
      return MudEntity_build_room_people(MudEntity_get_room(self));
  }

  return PyObject_GenericGetAttr(obj, nameobj);
}

static int MudEntity_name_matches(MudEntity *ent, const char *needle)
{
  if (!ent || !needle || !*needle)
    return 0;

  if (ent->kind == MOB_TRIGGER) {
    char_data *ch = MudEntity_get_char(ent);
    if (!ch)
      return 0;
    return isname(needle, IS_NPC(ch) ? GET_KEYWORDS(ch) : GET_NAME(ch));
  }

  if (ent->kind == OBJ_TRIGGER) {
    obj_data *obj = MudEntity_get_obj(ent);
    if (!obj || !obj->name)
      return 0;
    return isname(needle, obj->name);
  }

  return 0;
}

static PyObject *MudEntity_richcompare(PyObject *a, PyObject *b, int op)
{
  MudEntity *left = NULL;
  MudEntity *right = NULL;
  int match = 0;

  if (op != Py_EQ && op != Py_NE)
    Py_RETURN_NOTIMPLEMENTED;

  if (PyObject_TypeCheck(a, &MudEntityType))
    left = (MudEntity *)a;
  if (PyObject_TypeCheck(b, &MudEntityType))
    right = (MudEntity *)b;

  if (left && right) {
    match = (left->kind == right->kind && left->uid == right->uid);
  } else if (left && PyUnicode_Check(b)) {
    const char *needle = PyUnicode_AsUTF8(b);
    if (!needle)
      return NULL;
    match = MudEntity_name_matches(left, needle);
  } else if (right && PyUnicode_Check(a)) {
    const char *needle = PyUnicode_AsUTF8(a);
    if (!needle)
      return NULL;
    match = MudEntity_name_matches(right, needle);
  } else if (left && PyLong_Check(b)) {
    long uid = PyLong_AsLong(b);
    match = (left->uid == uid);
  } else if (right && PyLong_Check(a)) {
    long uid = PyLong_AsLong(a);
    match = (right->uid == uid);
  } else {
    Py_RETURN_NOTIMPLEMENTED;
  }

  if (op == Py_NE)
    match = !match;

  return PyBool_FromLong(match);
}

static PyGetSetDef MudEntity_getset[] = {
  {"kind", (getter)MudEntity_get_kind, NULL, "entity kind", NULL},
  {"uid", (getter)MudEntity_get_uid, NULL, "entity uid", NULL},
  {"vnum", (getter)MudEntity_get_vnum, NULL, "entity vnum", NULL},
  {"name", (getter)MudEntity_get_name, NULL, "entity name", NULL},
  {NULL}
};

static PyTypeObject MudEntityType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "mud.Entity",
  .tp_basicsize = sizeof(MudEntity),
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_repr = (reprfunc)MudEntity_repr,
  .tp_getattro = MudEntity_getattro,
  .tp_richcompare = MudEntity_richcompare,
  .tp_getset = MudEntity_getset,
};

static PyObject *mud_do(PyObject *self, PyObject *args, PyObject *kwargs)
{
  const char *command = NULL;
  PyObject *actor_obj = Py_None;
  static char *kwlist[] = {"command", "actor", NULL};
  int kind = -1;
  long uid = 0;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|O:do", kwlist, &command, &actor_obj))
    return NULL;

  if (actor_obj != Py_None) {
    uid = python_entity_uid_from_obj(actor_obj, &kind);
  } else if (current_context && current_context->trig) {
    if (current_context->type == MOB_TRIGGER) {
      char_data *ch = (char_data *)current_context->go;
      if (ch)
        uid = char_script_id(ch);
      kind = MOB_TRIGGER;
    } else if (current_context->type == OBJ_TRIGGER) {
      obj_data *obj = (obj_data *)current_context->go;
      if (obj)
        uid = obj_script_id(obj);
      kind = OBJ_TRIGGER;
    } else if (current_context->type == WLD_TRIGGER) {
      room_data *room = (room_data *)current_context->go;
      if (room)
        uid = room_script_id(room);
      kind = WLD_TRIGGER;
    }
  }

  if (kind == MOB_TRIGGER) {
    char_data *ch = find_char(uid);
    if (!ch)
      Py_RETURN_FALSE;
    command_interpreter(ch, (char *)command);
    Py_RETURN_TRUE;
  }

  if (kind == OBJ_TRIGGER) {
    obj_data *obj = python_find_obj(uid);
    if (!obj)
      Py_RETURN_FALSE;
    obj_command_interpreter(obj, (char *)command);
    Py_RETURN_TRUE;
  }

  if (kind == WLD_TRIGGER) {
    room_data *room = python_find_room(uid);
    if (!room)
      Py_RETURN_FALSE;
    wld_command_interpreter(room, (char *)command);
    Py_RETURN_TRUE;
  }

  Py_RETURN_FALSE;
}

static PyObject *mud_emote(PyObject *self, PyObject *args, PyObject *kwargs)
{
  const char *message = NULL;
  PyObject *actor_obj = Py_None;
  static char *kwlist[] = {"message", "actor", NULL};
  int kind = -1;
  long uid = 0;
  char buf[MAX_STRING_LENGTH];

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|O:emote", kwlist, &message, &actor_obj))
    return NULL;

  if (actor_obj != Py_None) {
    uid = python_entity_uid_from_obj(actor_obj, &kind);
  } else if (current_context && current_context->trig) {
    if (current_context->type == MOB_TRIGGER) {
      char_data *ch = (char_data *)current_context->go;
      if (ch)
        uid = char_script_id(ch);
      kind = MOB_TRIGGER;
    }
  }

  if (kind == MOB_TRIGGER) {
    char_data *ch = find_char(uid);
    if (!ch)
      Py_RETURN_FALSE;
    snprintf(buf, sizeof(buf), "emote %s", message);
    command_interpreter(ch, buf);
    Py_RETURN_TRUE;
  }

  Py_RETURN_FALSE;
}

static PyObject *mud_move(PyObject *self, PyObject *args, PyObject *kwargs)
{
  PyObject *direction_obj = NULL;
  const char *direction = NULL;
  long dest_vnum = -1;
  PyObject *actor_obj = Py_None;
  static char *kwlist[] = {"direction", "actor", NULL};
  int kind = -1;
  long uid = 0;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:move", kwlist, &direction_obj, &actor_obj))
    return NULL;

  if (PyLong_Check(direction_obj)) {
    dest_vnum = PyLong_AsLong(direction_obj);
  } else if (PyUnicode_Check(direction_obj)) {
    direction = PyUnicode_AsUTF8(direction_obj);
    if (direction && is_number(direction))
      dest_vnum = atol(direction);
  } else {
    PyErr_SetString(PyExc_TypeError, "move expects a direction string or room vnum");
    return NULL;
  }

  if (actor_obj != Py_None) {
    uid = python_entity_uid_from_obj(actor_obj, &kind);
  } else if (current_context && current_context->trig) {
    if (current_context->type == MOB_TRIGGER) {
      char_data *ch = (char_data *)current_context->go;
      if (ch)
        uid = char_script_id(ch);
      kind = MOB_TRIGGER;
    }
  }

  if (kind == MOB_TRIGGER) {
    char_data *ch = find_char(uid);
    if (!ch)
      Py_RETURN_FALSE;
    if (dest_vnum > 0) {
      room_rnum src = IN_ROOM(ch);
      room_rnum target = real_room((room_vnum)dest_vnum);
      int dir;

      if (src == NOWHERE || target == NOWHERE)
        Py_RETURN_FALSE;

      dir = find_first_step_no_doors(src, target);
      if (dir == BFS_ALREADY_THERE)
        Py_RETURN_TRUE;
      if (dir < 0)
        Py_RETURN_FALSE;

      perform_move(ch, dir, 0);
      Py_RETURN_TRUE;
    }

    if (!direction)
      Py_RETURN_FALSE;
    command_interpreter(ch, (char *)direction);
    Py_RETURN_TRUE;
  }

  Py_RETURN_FALSE;
}

static PyObject *mud_log(PyObject *self, PyObject *args)
{
  const char *message = NULL;

  if (!PyArg_ParseTuple(args, "s:log", &message))
    return NULL;

  python_log_message(message);
  Py_RETURN_NONE;
}

static PyObject *mud_roll(PyObject *self, PyObject *args)
{
  const char *expr = NULL;
  int count = 0;
  int sides = 0;
  int total = 0;
  int i;

  if (!PyArg_ParseTuple(args, "s:roll", &expr))
    return NULL;

  if (!expr || !*expr) {
    PyErr_SetString(PyExc_ValueError, "roll requires a dice string like 1d6");
    return NULL;
  }

  if (sscanf(expr, "%dd%d", &count, &sides) != 2 || count <= 0 || sides <= 0) {
    PyErr_SetString(PyExc_ValueError, "roll requires a dice string like 1d6");
    return NULL;
  }

  if (!(sides == 4 || sides == 6 || sides == 8 || sides == 10 ||
        sides == 12 || sides == 20 || sides == 100)) {
    PyErr_SetString(PyExc_ValueError, "roll supports d4, d6, d8, d10, d12, d20, d100");
    return NULL;
  }

  for (i = 0; i < count; i++)
    total += rand_number(1, sides);

  return PyLong_FromLong(total);
}

static PyObject *mud_sleep(PyObject *self, PyObject *args)
{
  int seconds = 0;
  struct py_call_data *data;
  int kind = -1;
  long uid = 0;

  if (!PyArg_ParseTuple(args, "i:sleep", &seconds))
    return NULL;

  if (seconds < 0) {
    PyErr_SetString(PyExc_ValueError, "seconds must be non-negative");
    return NULL;
  }

  if (!current_context || !current_context->trig || !current_context->trig->script) {
    PyErr_SetString(PyExc_RuntimeError, "sleep requires an active trigger context");
    return NULL;
  }

  if (current_context->type == MOB_TRIGGER) {
    char_data *ch = (char_data *)current_context->go;
    if (ch)
      uid = char_script_id(ch);
    kind = MOB_TRIGGER;
  } else if (current_context->type == OBJ_TRIGGER) {
    obj_data *obj = (obj_data *)current_context->go;
    if (obj)
      uid = obj_script_id(obj);
    kind = OBJ_TRIGGER;
  } else if (current_context->type == WLD_TRIGGER) {
    room_data *room = (room_data *)current_context->go;
    if (room)
      uid = room_script_id(room);
    kind = WLD_TRIGGER;
  }

  data = (struct py_call_data *)malloc(sizeof(*data));
  if (!data) {
    PyErr_SetString(PyExc_MemoryError, "sleep out of memory");
    return NULL;
  }

  {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s%s", SCRIPTS_PREFIX, current_context->trig->script);
    data->script = strdup(path);
  }
  data->func = strdup("on_trigger");
  data->args = PyTuple_New(0);
  data->kwargs = PyDict_New();
  if (!data->args || !data->kwargs) {
    if (data->args)
      Py_DECREF(data->args);
    if (data->kwargs)
      Py_DECREF(data->kwargs);
    free(data->script);
    free(data->func);
    free(data);
    PyErr_SetString(PyExc_MemoryError, "sleep out of memory");
    return NULL;
  }

  data->self_type = kind;
  data->self_uid = uid;

  event_create(python_call_event, data, (long)(seconds * PASSES_PER_SEC));

  PyErr_SetString(mud_sleep_exc, "sleep");
  return NULL;
}

static PyObject *mud_echo_room(PyObject *self, PyObject *args)
{
  PyObject *room_obj = NULL;
  const char *message = NULL;
  int kind = -1;
  long uid = 0;

  if (!PyArg_ParseTuple(args, "Os:echo_room", &room_obj, &message))
    return NULL;

  uid = python_entity_uid_from_obj(room_obj, &kind);
  if (kind != WLD_TRIGGER)
    Py_RETURN_FALSE;

  if (uid) {
    room_data *room = python_find_room(uid);
    if (room) {
      room_rnum rnum = real_room(room->number);
      if (rnum != NOWHERE)
        send_to_room(rnum, "%s\r\n", message);
    }
  }

  Py_RETURN_TRUE;
}

static PyObject *mud_send_char(PyObject *self, PyObject *args)
{
  PyObject *char_obj = NULL;
  const char *message = NULL;
  int kind = -1;
  long uid = 0;

  if (!PyArg_ParseTuple(args, "Os:send_char", &char_obj, &message))
    return NULL;

  uid = python_entity_uid_from_obj(char_obj, &kind);
  if (kind != MOB_TRIGGER)
    Py_RETURN_FALSE;

  if (uid) {
    char_data *ch = find_char(uid);
    if (ch)
      send_to_char(ch, "%s\r\n", message);
  }

  Py_RETURN_TRUE;
}

static PyObject *mud_call_later(PyObject *self, PyObject *args, PyObject *kwargs)
{
  int seconds = 0;
  const char *func = NULL;
  PyObject *rest = NULL;
  PyObject *call_args = NULL;
  PyObject *call_kwargs = NULL;
  struct py_call_data *data;
  int kind = -1;
  long uid = 0;
  int nargs;

  if (!current_context || !current_context->trig || !current_context->trig->script) {
    PyErr_SetString(PyExc_RuntimeError, "call_later requires an active trigger context");
    return NULL;
  }

  nargs = (int)PyTuple_Size(args);
  if (nargs < 2) {
    PyErr_SetString(PyExc_TypeError, "call_later requires at least seconds and function name");
    return NULL;
  }

  if (!PyArg_ParseTuple(args, "is:call_later", &seconds, &func))
    return NULL;

  if (seconds < 0) {
    PyErr_SetString(PyExc_ValueError, "seconds must be non-negative");
    return NULL;
  }

  rest = PyTuple_GetSlice(args, 2, nargs);
  if (!rest)
    return NULL;

  call_args = rest;
  if (kwargs) {
    Py_INCREF(kwargs);
    call_kwargs = kwargs;
  } else {
    call_kwargs = PyDict_New();
  }
  if (!call_kwargs) {
    Py_DECREF(call_args);
    return NULL;
  }

  data = (struct py_call_data *)malloc(sizeof(*data));
  if (!data) {
    Py_DECREF(call_args);
    Py_DECREF(call_kwargs);
    PyErr_SetString(PyExc_MemoryError, "call_later out of memory");
    return NULL;
  }

  {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s%s", SCRIPTS_PREFIX, current_context->trig->script);
    data->script = strdup(path);
  }
  data->func = strdup(func);
  data->args = call_args;
  data->kwargs = call_kwargs;

  if (current_context->type == MOB_TRIGGER) {
    char_data *ch = (char_data *)current_context->go;
    if (ch)
      uid = char_script_id(ch);
    kind = MOB_TRIGGER;
  } else if (current_context->type == OBJ_TRIGGER) {
    obj_data *obj = (obj_data *)current_context->go;
    if (obj)
      uid = obj_script_id(obj);
    kind = OBJ_TRIGGER;
  } else if (current_context->type == WLD_TRIGGER) {
    room_data *room = (room_data *)current_context->go;
    if (room)
      uid = room_script_id(room);
    kind = WLD_TRIGGER;
  }

  data->self_type = kind;
  data->self_uid = uid;

  event_create(python_call_event, data, (long)(seconds * PASSES_PER_SEC));

  Py_RETURN_NONE;
}

static PyMethodDef mud_methods[] = {
  {"do", (PyCFunction)mud_do, METH_VARARGS | METH_KEYWORDS, "Execute a command."},
  {"emote", (PyCFunction)mud_emote, METH_VARARGS | METH_KEYWORDS, "Emote as a character."},
  {"move", (PyCFunction)mud_move, METH_VARARGS | METH_KEYWORDS, "Move an entity in a direction."},
  {"roll", mud_roll, METH_VARARGS, "Roll dice like 1d6."},
  {"sleep", mud_sleep, METH_VARARGS, "Pause script execution."},
  {"log", mud_log, METH_VARARGS, "Write to script.log."},
  {"echo_room", mud_echo_room, METH_VARARGS, "Echo a message to a room."},
  {"send_char", mud_send_char, METH_VARARGS, "Send a message to a character."},
  {"call_later", (PyCFunction)mud_call_later, METH_VARARGS | METH_KEYWORDS, "Schedule a callback."},
  {NULL, NULL, 0, NULL}
};

static struct PyModuleDef mud_module_def = {
  PyModuleDef_HEAD_INIT,
  "mud",
  NULL,
  -1,
  mud_methods
};

static PyObject *PyInit_mud(void)
{
  PyObject *module;

  if (PyType_Ready(&MudEntityType) < 0)
    return NULL;

  module = PyModule_Create(&mud_module_def);
  if (!module)
    return NULL;

  mud_sleep_exc = PyErr_NewException("mud.Sleep", NULL, NULL);
  if (!mud_sleep_exc)
    return NULL;
  PyModule_AddObject(module, "Sleep", mud_sleep_exc);

  for (int i = 0; mud_dir_names[i]; i++) {
    PyObject *dir = PyUnicode_FromString(mud_dir_names[i]);
    if (!dir)
      return NULL;
    PyModule_AddObject(module, mud_dir_names[i], dir);
  }

  Py_INCREF(&MudEntityType);
  PyModule_AddObject(module, "Entity", (PyObject *)&MudEntityType);
  PyModule_AddIntConstant(module, "ENTITY_MOB", MOB_TRIGGER);
  PyModule_AddIntConstant(module, "ENTITY_OBJ", OBJ_TRIGGER);
  PyModule_AddIntConstant(module, "ENTITY_ROOM", WLD_TRIGGER);

  return module;
}

static PyObject *python_entity_from_uid(long uid, int kind)
{
  MudEntity *ent;

  if (uid <= 0)
    Py_RETURN_NONE;

  ent = PyObject_New(MudEntity, &MudEntityType);
  if (!ent)
    return NULL;

  ent->kind = kind;
  ent->uid = uid;

  return (PyObject *)ent;
}

static int python_uid_kind(long uid)
{
  if (uid >= OBJ_ID_BASE)
    return OBJ_TRIGGER;
  if (uid >= ROOM_ID_BASE)
    return WLD_TRIGGER;
  return MOB_TRIGGER;
}

static long python_entity_uid_from_obj(PyObject *obj, int *kind)
{
  if (!obj || obj == Py_None)
    return 0;

  if (PyObject_TypeCheck(obj, &MudEntityType)) {
    MudEntity *ent = (MudEntity *)obj;
    if (kind)
      *kind = ent->kind;
    return ent->uid;
  }

  if (PyLong_Check(obj)) {
    long uid = PyLong_AsLong(obj);
    if (kind)
      *kind = python_uid_kind(uid);
    return uid;
  }

  if (kind)
    *kind = -1;
  return 0;
}

static struct dsl_state *dsl_get_state(const char *path, int type, long uid)
{
  struct dsl_state *state;

  for (state = dsl_states; state; state = state->next) {
    if (state->self_uid == uid && state->self_type == type &&
        !strcmp(state->path, path))
      return state;
  }

  state = (struct dsl_state *)malloc(sizeof(*state));
  if (!state)
    return NULL;
  state->path = strdup(path);
  state->self_type = type;
  state->self_uid = uid;
  state->mtime = 0;
  state->generator = NULL;
  state->running = 0;
  state->next = dsl_states;
  dsl_states = state;
  return state;
}

static int python_is_dsl_script(const char *path)
{
  FILE *fp;
  char line[MAX_STRING_LENGTH];
  char *p;
  int has_def = 0;

  if (!path || !*path)
    return 0;

  fp = fopen(path, "r");
  if (!fp)
    return 0;

  while (fgets(line, sizeof(line), fp)) {
    p = line;
    while (*p && isspace((unsigned char)*p))
      p++;
    if (!*p || *p == '#')
      continue;
    if (!strn_cmp(p, "def on_trigger", 14)) {
      has_def = 1;
      break;
    }
  }

  fclose(fp);
  if (has_def)
    return 0;
  return 1;
}

static void dsl_buf_append(char **buf, size_t *len, size_t *cap, const char *text)
{
  size_t add = strlen(text);
  if (*len + add + 1 >= *cap) {
    size_t newcap = (*cap == 0) ? 512 : (*cap * 2);
    while (*len + add + 1 >= newcap)
      newcap *= 2;
    *buf = (char *)realloc(*buf, newcap);
    *cap = newcap;
  }
  memcpy(*buf + *len, text, add);
  *len += add;
  (*buf)[*len] = '\0';
}

static char *dsl_escape_string(const char *text)
{
  size_t len = 0;
  size_t cap = 0;
  char *out = NULL;
  const char *p = text;
  char buf[2] = {0, 0};

  while (*p) {
    if (*p == '\\' || *p == '"') {
      buf[0] = '\\';
      dsl_buf_append(&out, &len, &cap, buf);
    } else if (*p == '\n') {
      dsl_buf_append(&out, &len, &cap, "\\n");
      p++;
      continue;
    } else if (*p == '\r') {
      dsl_buf_append(&out, &len, &cap, "\\r");
      p++;
      continue;
    }
    buf[0] = *p;
    dsl_buf_append(&out, &len, &cap, buf);
    p++;
  }

  if (!out) {
    out = strdup("");
  }

  return out;
}

static void dsl_emit_action(char **buf, size_t *len, size_t *cap,
                            const char *indent, const char *verb, const char *arg)
{
  char line[MAX_STRING_LENGTH];
  char *escaped = NULL;

  if (!str_cmp(verb, "pass")) {
    snprintf(line, sizeof(line), "%spass\n", indent);
    dsl_buf_append(buf, len, cap, line);
    return;
  }

  if (!str_cmp(verb, "sleep")) {
    if (!arg || !*arg)
      snprintf(line, sizeof(line), "%syield 0\n", indent);
    else
      snprintf(line, sizeof(line), "%syield %s\n", indent, arg);
    dsl_buf_append(buf, len, cap, line);
    return;
  }

  if (!str_cmp(verb, "move")) {
    if (!arg || !*arg)
      snprintf(line, sizeof(line), "%smud.move(\"\", actor=npc)\n", indent);
    else
      snprintf(line, sizeof(line), "%smud.move(%s, actor=npc)\n", indent, arg);
    dsl_buf_append(buf, len, cap, line);
    return;
  }

  if (!str_cmp(verb, "emote")) {
    escaped = dsl_escape_string(arg ? arg : "");
    snprintf(line, sizeof(line), "%smud.emote(\"%s\", actor=npc)\n", indent, escaped);
    dsl_buf_append(buf, len, cap, line);
    free(escaped);
    return;
  }

  if (!str_cmp(verb, "log")) {
    const char *msg = arg ? arg : "";
    char trimmed[MAX_STRING_LENGTH];
    size_t msg_len;

    if (msg[0] == '"' || msg[0] == '\'') {
      msg_len = strlen(msg);
      if (msg_len >= 2 && msg[msg_len - 1] == msg[0]) {
        size_t copy_len = msg_len - 2;
        if (copy_len >= sizeof(trimmed))
          copy_len = sizeof(trimmed) - 1;
        memcpy(trimmed, msg + 1, copy_len);
        trimmed[copy_len] = '\0';
        msg = trimmed;
      }
    }

    escaped = dsl_escape_string(msg);
    snprintf(line, sizeof(line), "%smud.log(f\"%s\")\n", indent, escaped);
    dsl_buf_append(buf, len, cap, line);
    free(escaped);
    return;
  }

  if (!str_cmp(verb, "say")) {
    char tmp[MAX_STRING_LENGTH];
    snprintf(tmp, sizeof(tmp), "say %s", arg ? arg : "");
    escaped = dsl_escape_string(tmp);
    snprintf(line, sizeof(line), "%smud.do(\"%s\", actor=npc)\n", indent, escaped);
    dsl_buf_append(buf, len, cap, line);
    free(escaped);
    return;
  }

  if (!str_cmp(verb, "get")) {
    char tmp[MAX_STRING_LENGTH];
    snprintf(tmp, sizeof(tmp), "get %s", arg ? arg : "");
    escaped = dsl_escape_string(tmp);
    snprintf(line, sizeof(line), "%smud.do(\"%s\", actor=npc)\n", indent, escaped);
    dsl_buf_append(buf, len, cap, line);
    free(escaped);
    return;
  }

  if (!str_cmp(verb, "junk")) {
    char tmp[MAX_STRING_LENGTH];
    snprintf(tmp, sizeof(tmp), "junk %s", arg ? arg : "");
    escaped = dsl_escape_string(tmp);
    snprintf(line, sizeof(line), "%smud.do(\"%s\", actor=npc)\n", indent, escaped);
    dsl_buf_append(buf, len, cap, line);
    free(escaped);
    return;
  }

  if (!str_cmp(verb, "rest") || !str_cmp(verb, "stand")) {
    escaped = dsl_escape_string(verb);
    snprintf(line, sizeof(line), "%smud.do(\"%s\", actor=npc)\n", indent, escaped);
    dsl_buf_append(buf, len, cap, line);
    free(escaped);
    return;
  }

  if (!str_cmp(verb, "do")) {
    escaped = dsl_escape_string(arg ? arg : "");
    snprintf(line, sizeof(line), "%smud.do(\"%s\", actor=npc)\n", indent, escaped);
    dsl_buf_append(buf, len, cap, line);
    free(escaped);
    return;
  }

  snprintf(line, sizeof(line), "%spass\n", indent);
  dsl_buf_append(buf, len, cap, line);
  python_log_message("DSL script: unknown command");
}

static void dsl_emit_action_call(char **buf, size_t *len, size_t *cap,
                                 const char *indent, const char *verb, const char *arg)
{
  const char *use_arg = arg ? arg : "";

  if (!str_cmp(verb, "sleep")) {
    dsl_buf_append(buf, len, cap, indent);
    if (!use_arg[0]) {
      dsl_buf_append(buf, len, cap, "yield 0\n");
    } else {
      dsl_buf_append(buf, len, cap, "yield ");
      dsl_buf_append(buf, len, cap, use_arg);
      dsl_buf_append(buf, len, cap, "\n");
    }
    return;
  }

  if (!str_cmp(verb, "move")) {
    dsl_buf_append(buf, len, cap, indent);
    if (!use_arg[0]) {
      dsl_buf_append(buf, len, cap, "mud.move(\"\", actor=npc)\n");
    } else {
      dsl_buf_append(buf, len, cap, "mud.move(");
      dsl_buf_append(buf, len, cap, use_arg);
      dsl_buf_append(buf, len, cap, ", actor=npc)\n");
    }
    return;
  }

  if (!str_cmp(verb, "emote")) {
    dsl_buf_append(buf, len, cap, indent);
    if (!use_arg[0]) {
      dsl_buf_append(buf, len, cap, "mud.emote(\"\", actor=npc)\n");
    } else {
      dsl_buf_append(buf, len, cap, "mud.emote(");
      dsl_buf_append(buf, len, cap, use_arg);
      dsl_buf_append(buf, len, cap, ", actor=npc)\n");
    }
    return;
  }

  if (!str_cmp(verb, "say")) {
    dsl_buf_append(buf, len, cap, indent);
    if (!use_arg[0]) {
      dsl_buf_append(buf, len, cap, "mud.do(\"say\", actor=npc)\n");
    } else {
      dsl_buf_append(buf, len, cap, "mud.do(\"say \" + ");
      dsl_buf_append(buf, len, cap, use_arg);
      dsl_buf_append(buf, len, cap, ", actor=npc)\n");
    }
    return;
  }

  if (!str_cmp(verb, "get")) {
    dsl_buf_append(buf, len, cap, indent);
    if (!use_arg[0]) {
      dsl_buf_append(buf, len, cap, "mud.do(\"get\", actor=npc)\n");
    } else {
      dsl_buf_append(buf, len, cap, "mud.do(\"get \" + ");
      dsl_buf_append(buf, len, cap, use_arg);
      dsl_buf_append(buf, len, cap, ", actor=npc)\n");
    }
    return;
  }

  if (!str_cmp(verb, "junk")) {
    dsl_buf_append(buf, len, cap, indent);
    if (!use_arg[0]) {
      dsl_buf_append(buf, len, cap, "mud.do(\"junk\", actor=npc)\n");
    } else {
      dsl_buf_append(buf, len, cap, "mud.do(\"junk \" + ");
      dsl_buf_append(buf, len, cap, use_arg);
      dsl_buf_append(buf, len, cap, ", actor=npc)\n");
    }
    return;
  }

  if (!str_cmp(verb, "rest") || !str_cmp(verb, "stand")) {
    dsl_buf_append(buf, len, cap, indent);
    dsl_buf_append(buf, len, cap, "mud.do(\"");
    dsl_buf_append(buf, len, cap, verb);
    dsl_buf_append(buf, len, cap, "\", actor=npc)\n");
    return;
  }

  if (!str_cmp(verb, "do")) {
    dsl_buf_append(buf, len, cap, indent);
    if (!use_arg[0]) {
      dsl_buf_append(buf, len, cap, "mud.do(\"\", actor=npc)\n");
    } else {
      dsl_buf_append(buf, len, cap, "mud.do(");
      dsl_buf_append(buf, len, cap, use_arg);
      dsl_buf_append(buf, len, cap, ", actor=npc)\n");
    }
    return;
  }

  if (!str_cmp(verb, "log")) {
    dsl_buf_append(buf, len, cap, indent);
    if (!use_arg[0]) {
      dsl_buf_append(buf, len, cap, "mud.log(\"\")\n");
    } else {
      dsl_buf_append(buf, len, cap, "mud.log(");
      dsl_buf_append(buf, len, cap, use_arg);
      dsl_buf_append(buf, len, cap, ")\n");
    }
    return;
  }

  dsl_buf_append(buf, len, cap, indent);
  dsl_buf_append(buf, len, cap, "pass\n");
  python_log_message("DSL script: unknown command");
}

static char *dsl_normalize_bools(const char *text)
{
  size_t len = 0;
  size_t cap = 0;
  char *out = NULL;
  const char *p = text;

  while (*p) {
    if (isalpha((unsigned char)*p) || *p == '_') {
      const char *start = p;
      char word[MAX_INPUT_LENGTH];
      size_t wlen = 0;

      while (*p && (isalnum((unsigned char)*p) || *p == '_'))
        p++;
      wlen = (size_t)(p - start);
      if (wlen >= sizeof(word))
        wlen = sizeof(word) - 1;
      memcpy(word, start, wlen);
      word[wlen] = '\0';

      if (!strcmp(word, "true"))
        dsl_buf_append(&out, &len, &cap, "True");
      else if (!strcmp(word, "false"))
        dsl_buf_append(&out, &len, &cap, "False");
      else
        dsl_buf_append(&out, &len, &cap, word);
    } else {
      char ch[2] = { *p, '\0' };
      dsl_buf_append(&out, &len, &cap, ch);
      p++;
    }
  }

  if (!out)
    out = strdup("");

  return out;
}

static char *dsl_build_source(const char *path)
{
  FILE *fp;
  char line[MAX_STRING_LENGTH];
  char *buf = NULL;
  size_t len = 0;
  size_t cap = 0;
  int saw_stmt = 0;

  fp = fopen(path, "r");
  if (!fp)
    return NULL;

  dsl_buf_append(&buf, &len, &cap,
    "def __dsl__(npc, room, pc=None, object=None):\n"
    "    if False:\n"
    "        yield 0\n");

  while (fgets(line, sizeof(line), fp)) {
    char *p = line;
    char *start;
    char *end;
    char *colon;
    char verb[MAX_INPUT_LENGTH];
    char indent_buf[128];
    int indent = 0;

    while (*p == ' ' || *p == '\t') {
      if (*p == '\t')
        indent += 4;
      else
        indent += 1;
      p++;
    }

    start = p;
    while (*start && isspace((unsigned char)*start))
      start++;
    if (!*start || *start == '#')
      continue;

    if (!strn_cmp(start, "mudscript", 9))
      continue;

    end = start + strlen(start);
    while (end > start && isspace((unsigned char)end[-1]))
      end--;
    *end = '\0';

    if (!*start)
      continue;

    saw_stmt = 1;

    {
      char *open = strchr(start, '(');
      char *close = strrchr(start, ')');
      char name[MAX_INPUT_LENGTH];
      char argbuf[MAX_STRING_LENGTH];
      size_t name_len;
      size_t arg_len;
      char *name_end;
      int is_call = 0;

      if (open && close && close > open) {
        name_end = open;
        while (name_end > start && isspace((unsigned char)name_end[-1]))
          name_end--;
        name_len = (size_t)(name_end - start);
        if (name_len > 0 && name_len < sizeof(name)) {
          memcpy(name, start, name_len);
          name[name_len] = '\0';
          if (!str_cmp(name, "move") || !str_cmp(name, "emote") ||
              !str_cmp(name, "say") || !str_cmp(name, "get") ||
              !str_cmp(name, "junk") || !str_cmp(name, "rest") ||
              !str_cmp(name, "stand") || !str_cmp(name, "do") ||
              !str_cmp(name, "sleep") || !str_cmp(name, "log")) {
            is_call = 1;
          }
        }
      }

      if (is_call) {
        arg_len = (size_t)(close - open - 1);
        if (arg_len >= sizeof(argbuf))
          arg_len = sizeof(argbuf) - 1;
        memcpy(argbuf, open + 1, arg_len);
        argbuf[arg_len] = '\0';
        {
          char *arg = argbuf;
          while (*arg && isspace((unsigned char)*arg))
            arg++;
          while (arg_len > 0 && isspace((unsigned char)argbuf[arg_len - 1])) {
            argbuf[arg_len - 1] = '\0';
            arg_len--;
          }
          snprintf(indent_buf, sizeof(indent_buf), "%*s", 4 + indent, "");
          dsl_emit_action_call(&buf, &len, &cap, indent_buf, name, arg);
        }
        continue;
      }
    }

    if (!strn_cmp(start, "if ", 3) || !strn_cmp(start, "elif ", 5) ||
        !strn_cmp(start, "for ", 4) || !strn_cmp(start, "while ", 6) ||
        !strn_cmp(start, "else", 4)) {
      char *body = NULL;
      char body_stmt[MAX_STRING_LENGTH];
      int has_body = 0;
      char *normalized = NULL;

      colon = strchr(start, ':');
      if (colon) {
        *colon = '\0';
        body = colon + 1;
        while (body && isspace((unsigned char)*body))
          body++;
        if (body && *body)
          has_body = 1;
      }

      snprintf(indent_buf, sizeof(indent_buf), "%*s", 4 + indent, "");
      if (!colon)
        colon = start + strlen(start);

      normalized = dsl_normalize_bools(start);
      if (!normalized)
        normalized = strdup(start);

      if (!has_body) {
        char control_line[MAX_STRING_LENGTH];
        snprintf(control_line, sizeof(control_line), "%s%s:\n", indent_buf, normalized);
        dsl_buf_append(&buf, &len, &cap, control_line);
      } else {
        char *arg = body;
        char *q = arg;
        while (*q && !isspace((unsigned char)*q))
          q++;
        snprintf(verb, sizeof(verb), "%.*s", (int)(q - arg), arg);
        while (*q && isspace((unsigned char)*q))
          q++;
        snprintf(body_stmt, sizeof(body_stmt), "%s", q);
        snprintf(indent_buf, sizeof(indent_buf), "%*s", 4 + indent, "");
        {
          char control_line[MAX_STRING_LENGTH];
          snprintf(control_line, sizeof(control_line), "%s%s: ", indent_buf, normalized);
          dsl_buf_append(&buf, &len, &cap, control_line);
        }
        dsl_emit_action(&buf, &len, &cap, "", verb, body_stmt);
      }
      free(normalized);
      continue;
    }

    {
      char *arg;
      char *q = start;

      while (*q && !isspace((unsigned char)*q))
        q++;
      snprintf(verb, sizeof(verb), "%.*s", (int)(q - start), start);
      while (*q && isspace((unsigned char)*q))
        q++;
      arg = q;

      snprintf(indent_buf, sizeof(indent_buf), "%*s", 4 + indent, "");
      dsl_emit_action(&buf, &len, &cap, indent_buf, verb, arg);
    }
  }

  fclose(fp);

  if (!saw_stmt)
    dsl_buf_append(&buf, &len, &cap, "    pass\n");

  return buf;
}

static struct dsl_script_cache *dsl_load_script(const char *path)
{
  struct dsl_script_cache *entry;
  struct stat st;
  char *source = NULL;
  PyObject *code = NULL;
  PyObject *globals = NULL;
  PyObject *builtins = NULL;
  PyObject *result = NULL;
  PyObject *func = NULL;

  if (!path || !*path)
    return NULL;

  for (entry = dsl_cache; entry; entry = entry->next) {
    if (!strcmp(entry->path, path))
      break;
  }

  if (stat(path, &st) < 0) {
    script_log("DSL script not found: %s", path);
    return NULL;
  }

  if (entry && entry->mtime == st.st_mtime && entry->func)
    return entry;

  source = dsl_build_source(path);
  if (!source) {
    script_log("Unable to read DSL script: %s", path);
    return NULL;
  }

  code = Py_CompileString(source, path, Py_file_input);
  free(source);
  if (!code) {
    python_log_exception("dsl compile");
    return NULL;
  }

  globals = PyDict_New();
  if (!globals) {
    Py_DECREF(code);
    return NULL;
  }

  builtins = PyEval_GetBuiltins();
  PyDict_SetItemString(globals, "__builtins__", builtins);

  if (mud_module)
    PyDict_SetItemString(globals, "mud", mud_module);
  if (mud_module) {
    for (int i = 0; mud_dir_names[i]; i++) {
      PyObject *dir = PyObject_GetAttrString(mud_module, mud_dir_names[i]);
      if (dir) {
        PyDict_SetItemString(globals, mud_dir_names[i], dir);
        Py_DECREF(dir);
      } else {
        PyErr_Clear();
      }
    }
  }

  result = PyEval_EvalCode(code, globals, globals);
  Py_DECREF(code);

  if (!result) {
    python_log_exception("dsl exec");
    Py_DECREF(globals);
    return NULL;
  }

  Py_DECREF(result);

  func = PyDict_GetItemString(globals, "__dsl__");
  if (!func || !PyCallable_Check(func)) {
    python_log_message("DSL script missing __dsl__");
    Py_DECREF(globals);
    return NULL;
  }

  if (!entry) {
    entry = (struct dsl_script_cache *)malloc(sizeof(*entry));
    if (!entry) {
      Py_DECREF(globals);
      return NULL;
    }
    entry->path = strdup(path);
    entry->next = dsl_cache;
    dsl_cache = entry;
  } else {
    if (entry->globals)
      Py_DECREF(entry->globals);
    if (entry->func)
      Py_DECREF(entry->func);
  }

  entry->globals = globals;
  entry->func = func;
  Py_INCREF(func);
  entry->mtime = st.st_mtime;

  return entry;
}

static obj_data *python_find_obj(long uid)
{
  char name[32];

  if (uid <= 0)
    return NULL;

  snprintf(name, sizeof(name), "%c%ld", UID_CHAR, uid);
  return get_obj(name);
}

static room_data *python_find_room(long uid)
{
  char name[32];

  if (uid <= 0)
    return NULL;

  snprintf(name, sizeof(name), "%c%ld", UID_CHAR, uid);
  return get_room(name);
}

static int python_path_is_script_log(const char *path)
{
  char resolved[PATH_MAX];

  if (!path || !*path)
    return 0;

  if (realpath(path, resolved)) {
    if (!strcmp(resolved, script_log_path))
      return 1;
  }

  return 0;
}

static int python_audit_hook(const char *event, PyObject *args, void *userData)
{
  (void)userData;

  if (!event)
    return 0;

  if (!strcmp(event, "open")) {
    PyObject *path_obj = NULL;
    PyObject *mode_obj = NULL;
    const char *path = NULL;
    const char *mode = NULL;

    if (PyTuple_Size(args) >= 2) {
      path_obj = PyTuple_GetItem(args, 0);
      mode_obj = PyTuple_GetItem(args, 1);
    }

    if (path_obj && PyUnicode_Check(path_obj))
      path = PyUnicode_AsUTF8(path_obj);
    else if (path_obj && PyBytes_Check(path_obj))
      path = PyBytes_AsString(path_obj);
    if (mode_obj && PyUnicode_Check(mode_obj))
      mode = PyUnicode_AsUTF8(mode_obj);

    if (mode && (strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+') || strchr(mode, 'x'))) {
      if (!path || !python_path_is_script_log(path)) {
        PyErr_SetString(PyExc_PermissionError, "Python scripts may only write to script.log");
        return -1;
      }
    }
  }

  if (!strcmp(event, "os.open") || !strcmp(event, "os.openat")) {
    PyObject *path_obj = NULL;
    PyObject *flags_obj = NULL;
    const char *path = NULL;
    long flags = 0;

    if (PyTuple_Size(args) >= 2) {
      path_obj = PyTuple_GetItem(args, 0);
      flags_obj = PyTuple_GetItem(args, 1);
    }

    if (path_obj && PyUnicode_Check(path_obj))
      path = PyUnicode_AsUTF8(path_obj);
    else if (path_obj && PyBytes_Check(path_obj))
      path = PyBytes_AsString(path_obj);
    if (flags_obj && PyLong_Check(flags_obj))
      flags = PyLong_AsLong(flags_obj);

    if (flags & (O_WRONLY | O_RDWR | O_CREAT | O_TRUNC | O_APPEND)) {
      if (!path || !python_path_is_script_log(path)) {
        PyErr_SetString(PyExc_PermissionError, "Python scripts may only write to script.log");
        return -1;
      }
    }
  }

  if (!strcmp(event, "socket.connect") || !strcmp(event, "socket.bind")) {
    PyObject *addr_obj = NULL;

    if (PyTuple_Size(args) >= 1)
      addr_obj = PyTuple_GetItem(args, 0);

    if (addr_obj) {
      if (PyTuple_Check(addr_obj) && PyTuple_Size(addr_obj) >= 1) {
        PyObject *host_obj = PyTuple_GetItem(addr_obj, 0);
        const char *host = NULL;

        if (host_obj && PyUnicode_Check(host_obj))
          host = PyUnicode_AsUTF8(host_obj);

        if (!host || !*host || !strcmp(host, "0.0.0.0") || !strcmp(host, "::") ||
            (strcmp(host, "127.0.0.1") && strcmp(host, "localhost") && strcmp(host, "::1"))) {
          PyErr_SetString(PyExc_PermissionError, "Outbound network connections are blocked");
          return -1;
        }
      }
    }
  }

  return 0;
}

static void python_log_exception(const char *context)
{
  PyObject *ptype = NULL;
  PyObject *pvalue = NULL;
  PyObject *ptrace = NULL;
  PyObject *pstr = NULL;
  const char *msg = NULL;
  char buf[MAX_STRING_LENGTH];

  PyErr_Fetch(&ptype, &pvalue, &ptrace);
  PyErr_NormalizeException(&ptype, &pvalue, &ptrace);

  if (pvalue)
    pstr = PyObject_Str(pvalue);

  if (pstr)
    msg = PyUnicode_AsUTF8(pstr);

  if (!msg)
    msg = "(unknown python error)";

  snprintf(buf, sizeof(buf), "Python error in %s: %s", context ? context : "script", msg);
  python_log_message(buf);
  script_log("%s", buf);

  Py_XDECREF(pstr);
  Py_XDECREF(ptype);
  Py_XDECREF(pvalue);
  Py_XDECREF(ptrace);
}

static void python_log_message(const char *message)
{
  time_t ct = time(0);
  char timestr[21];
  int i;

  if (!message || !*message)
    return;

  if (!script_log_fp) {
    script_log_fp = fopen(SCRIPT_LOGFILE, "a");
    if (!script_log_fp)
      return;
  }

  for (i = 0; i < 21; i++)
    timestr[i] = 0;
  strftime(timestr, sizeof(timestr), "%b %d %H:%M:%S %Y", localtime(&ct));

  fprintf(script_log_fp, "%-20.20s :: %s\n", timestr, message);
  fflush(script_log_fp);
}

static void python_rotate_script_log(void)
{
  struct stat st;
  char path[PATH_MAX];
  char next[PATH_MAX];
  int i;

  if (stat(SCRIPT_LOGFILE, &st) != 0)
    return;

  for (i = 1; ; i++) {
    if (snprintf(path, sizeof(path), "%s.%d", SCRIPT_LOGFILE, i) >= (int)sizeof(path))
      return;
    if (stat(path, &st) != 0)
      break;
  }

  for (i = i - 1; i >= 1; i--) {
    if (snprintf(path, sizeof(path), "%s.%d", SCRIPT_LOGFILE, i) >= (int)sizeof(path))
      return;
    if (snprintf(next, sizeof(next), "%s.%d", SCRIPT_LOGFILE, i + 1) >= (int)sizeof(next))
      return;
    rename(path, next);
  }

  if (snprintf(next, sizeof(next), "%s.1", SCRIPT_LOGFILE) >= (int)sizeof(next))
    return;
  rename(SCRIPT_LOGFILE, next);
}

void python_debug_log(const char *message)
{
  python_log_message(message);
}

static struct py_script_cache *python_load_script(const char *path)
{
  struct py_script_cache *entry;
  struct stat st;
  char *source = NULL;
  FILE *fp = NULL;
  long size = 0;
  PyObject *code = NULL;
  PyObject *globals = NULL;
  PyObject *builtins = NULL;
  PyObject *result = NULL;
  PyObject *file_str = NULL;

  if (!path || !*path)
    return NULL;

  for (entry = script_cache; entry; entry = entry->next) {
    if (!strcmp(entry->path, path))
      break;
  }

  if (stat(path, &st) < 0) {
    script_log("Python script not found: %s", path);
    return NULL;
  }

  if (entry && entry->mtime == st.st_mtime && entry->globals)
    return entry;

  fp = fopen(path, "rb");
  if (!fp) {
    script_log("Unable to open python script: %s", path);
    return NULL;
  }

  if (fseek(fp, 0, SEEK_END) == 0)
    size = ftell(fp);
  if (size < 0)
    size = 0;
  rewind(fp);

  source = (char *)malloc((size_t)size + 1);
  if (!source) {
    fclose(fp);
    script_log("Out of memory reading python script: %s", path);
    return NULL;
  }

  if (size > 0 && fread(source, 1, (size_t)size, fp) != (size_t)size) {
    free(source);
    fclose(fp);
    script_log("Failed to read python script: %s", path);
    return NULL;
  }
  source[size] = '\0';
  fclose(fp);

  code = Py_CompileString(source, path, Py_file_input);
  free(source);
  if (!code) {
    python_log_exception("compile");
    return NULL;
  }

  globals = PyDict_New();
  if (!globals) {
    Py_DECREF(code);
    return NULL;
  }

  builtins = PyEval_GetBuiltins();
  PyDict_SetItemString(globals, "__builtins__", builtins);

  file_str = PyUnicode_FromString(path);
  if (file_str) {
    PyDict_SetItemString(globals, "__file__", file_str);
    Py_DECREF(file_str);
  }

  if (mud_module)
    PyDict_SetItemString(globals, "mud", mud_module);
  if (mud_sleep_exc)
    PyDict_SetItemString(globals, "MudSleep", mud_sleep_exc);
  if (mud_module) {
    PyObject *log_func = PyObject_GetAttrString(mud_module, "log");
    if (log_func) {
      PyDict_SetItemString(globals, "log", log_func);
      Py_DECREF(log_func);
    } else {
      PyErr_Clear();
    }
  }
  if (mud_module) {
    for (int i = 0; mud_dir_names[i]; i++) {
      PyObject *dir = PyObject_GetAttrString(mud_module, mud_dir_names[i]);
      if (dir) {
        PyDict_SetItemString(globals, mud_dir_names[i], dir);
        Py_DECREF(dir);
      } else {
        PyErr_Clear();
      }
    }
  }

  result = PyEval_EvalCode(code, globals, globals);
  Py_DECREF(code);

  if (!result) {
    python_log_exception("exec");
    Py_DECREF(globals);
    return NULL;
  }

  Py_DECREF(result);

  if (!entry) {
    entry = (struct py_script_cache *)malloc(sizeof(*entry));
    if (!entry) {
      Py_DECREF(globals);
      return NULL;
    }
    entry->path = strdup(path);
    entry->next = script_cache;
    script_cache = entry;
  } else if (entry->globals) {
    Py_DECREF(entry->globals);
  }

  entry->globals = globals;
  entry->mtime = st.st_mtime;

  return entry;
}

static int dsl_run_state(struct dsl_state *state, PyObject *func, PyObject *npc,
                         PyObject *room, PyObject *pc, PyObject *object)
{
  PyObject *value = NULL;
  double seconds = 0.0;
  long passes = 0;

  if (!state || !func)
    return 1;

  if (!state->generator) {
    state->generator = PyObject_CallFunctionObjArgs(func, npc, room, pc, object, NULL);
    if (!state->generator) {
      python_log_exception("dsl start");
      return 1;
    }
  }

  state->running = 1;
  value = PyIter_Next(state->generator);
  state->running = 0;

  if (!value) {
    if (PyErr_Occurred())
      python_log_exception("dsl run");
    PyErr_Clear();
    Py_DECREF(state->generator);
    state->generator = NULL;
    return 1;
  }

  if (PyLong_Check(value))
    seconds = (double)PyLong_AsLong(value);
  else if (PyFloat_Check(value))
    seconds = PyFloat_AsDouble(value);
  else {
    python_log_message("DSL script yield must be a number");
    seconds = 0.0;
  }

  Py_DECREF(value);

  if (seconds < 0.0)
    seconds = 0.0;

  passes = (long)(seconds * PASSES_PER_SEC);
  if (passes < 0)
    passes = 0;

  {
    struct dsl_call_data *data = (struct dsl_call_data *)malloc(sizeof(*data));
    if (!data) {
      script_log("DSL script out of memory scheduling sleep");
      return 1;
    }
    data->state = state;
    event_create(dsl_call_event, data, passes);
  }

  return 1;
}

static int dsl_trigger_run(void *go, struct trig_data *trig, int type, const char *path)
{
  PyGILState_STATE gstate;
  struct dsl_script_cache *entry;
  struct dsl_state *state;
  char_data *ch = NULL;
  obj_data *obj = NULL;
  room_data *room = NULL;
  long uid = 0;
  PyObject *npc = NULL;
  PyObject *room_obj = NULL;
  PyObject *pc = NULL;
  PyObject *object = NULL;

  if (!path || !*path)
    return 1;

  gstate = PyGILState_Ensure();

  entry = dsl_load_script(path);
  if (!entry || !entry->func) {
    PyGILState_Release(gstate);
    return 1;
  }

  if (type == MOB_TRIGGER) {
    ch = (char_data *)go;
    if (ch)
      uid = char_script_id(ch);
    if (ch && IN_ROOM(ch) != NOWHERE)
      room = &world[IN_ROOM(ch)];
  } else if (type == OBJ_TRIGGER) {
    obj = (obj_data *)go;
    if (obj)
      uid = obj_script_id(obj);
    if (obj) {
      if (obj->in_room != NOWHERE)
        room = &world[obj->in_room];
      else if (obj->carried_by && IN_ROOM(obj->carried_by) != NOWHERE)
        room = &world[IN_ROOM(obj->carried_by)];
      else if (obj->worn_by && IN_ROOM(obj->worn_by) != NOWHERE)
        room = &world[IN_ROOM(obj->worn_by)];
    }
  } else if (type == WLD_TRIGGER) {
    room = (room_data *)go;
    if (room)
      uid = room_script_id(room);
  }

  if (!uid) {
    PyGILState_Release(gstate);
    return 1;
  }

  state = dsl_get_state(path, type, uid);
  if (!state) {
    PyGILState_Release(gstate);
    return 1;
  }

  if (state->running) {
    PyGILState_Release(gstate);
    return 1;
  }

  if (state->mtime != entry->mtime) {
    if (state->generator) {
      Py_DECREF(state->generator);
      state->generator = NULL;
    }
    state->mtime = entry->mtime;
  }

  if (type == MOB_TRIGGER && ch)
    npc = python_entity_from_uid(uid, MOB_TRIGGER);
  if (room)
    room_obj = python_entity_from_uid(room_script_id(room), WLD_TRIGGER);
  if (type == OBJ_TRIGGER && obj)
    object = python_entity_from_uid(uid, OBJ_TRIGGER);

  if (room) {
    char_data *tch;
    for (tch = room->people; tch; tch = tch->next_in_room) {
      if (!IS_NPC(tch)) {
        pc = python_entity_from_uid(char_script_id(tch), MOB_TRIGGER);
        break;
      }
    }
  }

  if (!npc && PyErr_Occurred())
    PyErr_Clear();
  if (!room_obj && PyErr_Occurred())
    PyErr_Clear();
  if (!pc && PyErr_Occurred())
    PyErr_Clear();
  if (!object && PyErr_Occurred())
    PyErr_Clear();

  if (!npc) {
    npc = Py_None;
    Py_INCREF(npc);
  }
  if (!room_obj) {
    room_obj = Py_None;
    Py_INCREF(room_obj);
  }
  if (!pc) {
    pc = Py_None;
    Py_INCREF(pc);
  }
  if (!object) {
    object = Py_None;
    Py_INCREF(object);
  }

  {
    struct py_trigger_context ctx;
    ctx.go = go;
    ctx.type = type;
    ctx.trig = trig;
    current_context = &ctx;
    dsl_run_state(state, entry->func, npc, room_obj, pc, object);
    current_context = NULL;
  }

  Py_DECREF(npc);
  Py_DECREF(room_obj);
  Py_DECREF(pc);
  Py_DECREF(object);

  PyGILState_Release(gstate);
  return 1;
}

static EVENTFUNC(dsl_call_event)
{
  struct dsl_call_data *data = (struct dsl_call_data *)event_obj;
  struct dsl_state *state;
  void *go = NULL;

  if (!data)
    return 0;

  state = data->state;
  free(data);

  if (!state)
    return 0;

  if (state->self_type == MOB_TRIGGER)
    go = find_char(state->self_uid);
  else if (state->self_type == OBJ_TRIGGER)
    go = python_find_obj(state->self_uid);
  else if (state->self_type == WLD_TRIGGER)
    go = python_find_room(state->self_uid);

  if (!go) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    if (state->generator) {
      Py_DECREF(state->generator);
      state->generator = NULL;
    }
    PyGILState_Release(gstate);
    return 0;
  }

  dsl_trigger_run(go, NULL, state->self_type, state->path);
  return 0;
}

static PyObject *python_build_event(struct trig_data *trig, void *go, int type)
{
  PyObject *event;
  PyObject *vars;
  PyObject *trigger;
  PyObject *temp;

  event = PyDict_New();
  if (!event)
    return NULL;

  vars = PyDict_New();
  if (!vars) {
    Py_DECREF(event);
    return NULL;
  }

  trigger = PyDict_New();
  if (!trigger) {
    Py_DECREF(vars);
    Py_DECREF(event);
    return NULL;
  }

  temp = PyLong_FromLong(GET_TRIG_VNUM(trig));
  if (temp) {
    PyDict_SetItemString(trigger, "vnum", temp);
    Py_DECREF(temp);
  }
  temp = PyUnicode_FromString(GET_TRIG_NAME(trig) ? GET_TRIG_NAME(trig) : "");
  if (temp) {
    PyDict_SetItemString(trigger, "name", temp);
    Py_DECREF(temp);
  }
  temp = PyLong_FromLong(GET_TRIG_TYPE(trig));
  if (temp) {
    PyDict_SetItemString(trigger, "type", temp);
    Py_DECREF(temp);
  }
  temp = PyLong_FromLong(trig->attach_type);
  if (temp) {
    PyDict_SetItemString(trigger, "attach_type", temp);
    Py_DECREF(temp);
  }
  temp = PyLong_FromLong(GET_TRIG_NARG(trig));
  if (temp) {
    PyDict_SetItemString(trigger, "narg", temp);
    Py_DECREF(temp);
  }
  temp = PyUnicode_FromString(GET_TRIG_ARG(trig) ? GET_TRIG_ARG(trig) : "");
  if (temp) {
    PyDict_SetItemString(trigger, "arglist", temp);
    Py_DECREF(temp);
  }
  temp = PyUnicode_FromString(trig->script ? trig->script : "");
  if (temp) {
    PyDict_SetItemString(trigger, "script", temp);
    Py_DECREF(temp);
  }

  if (type == MOB_TRIGGER && go) {
    char_data *ch = (char_data *)go;
    temp = python_entity_from_uid(char_script_id(ch), MOB_TRIGGER);
    if (temp) {
      PyDict_SetItemString(event, "self", temp);
      Py_DECREF(temp);
    }
  } else if (type == OBJ_TRIGGER && go) {
    obj_data *obj = (obj_data *)go;
    temp = python_entity_from_uid(obj_script_id(obj), OBJ_TRIGGER);
    if (temp) {
      PyDict_SetItemString(event, "self", temp);
      Py_DECREF(temp);
    }
  } else if (type == WLD_TRIGGER && go) {
    room_data *room = (room_data *)go;
    temp = python_entity_from_uid(room_script_id(room), WLD_TRIGGER);
    if (temp) {
      PyDict_SetItemString(event, "self", temp);
      Py_DECREF(temp);
    }
  }

  for (struct trig_var_data *vd = GET_TRIG_VARS(trig); vd; vd = vd->next) {
    PyObject *value_obj = NULL;

    if (!vd->name || !vd->value)
      continue;

    if (*(vd->value) == UID_CHAR) {
      long uid = atol(vd->value + 1);
      int kind = python_uid_kind(uid);
      value_obj = python_entity_from_uid(uid, kind);
    } else {
      value_obj = PyUnicode_FromString(vd->value);
    }

    if (!value_obj)
      continue;

    PyDict_SetItemString(vars, vd->name, value_obj);

    if (!PyDict_GetItemString(event, vd->name))
      PyDict_SetItemString(event, vd->name, value_obj);

    Py_DECREF(value_obj);
  }

  PyDict_SetItemString(event, "trigger", trigger);
  PyDict_SetItemString(event, "vars", vars);

  Py_DECREF(vars);
  Py_DECREF(trigger);

  return event;
}

static EVENTFUNC(python_call_event)
{
  struct py_call_data *data = (struct py_call_data *)event_obj;
  PyGILState_STATE gstate;
  struct py_script_cache *entry;
  PyObject *func = NULL;
  PyObject *event = NULL;
  PyObject *result = NULL;
  struct py_trigger_context ctx;
  struct trig_data temp_trig;
  const char *script_name = NULL;

  if (!data)
    return 0;

  gstate = PyGILState_Ensure();

  entry = python_load_script(data->script);
  if (entry && entry->globals) {
    func = PyDict_GetItemString(entry->globals, data->func);
    if (func && PyCallable_Check(func)) {
      event = PyDict_New();
      if (event) {
        PyObject *self_obj = python_entity_from_uid(data->self_uid, data->self_type);

        if (self_obj) {
          PyDict_SetItemString(event, "self", self_obj);
          Py_DECREF(self_obj);
        } else {
          PyErr_Clear();
        }
        PyDict_SetItemString(event, "scheduled", Py_True);
      }
      if (event) {
        script_name = data->script;
        if (script_name && !strncmp(script_name, SCRIPTS_PREFIX, strlen(SCRIPTS_PREFIX)))
          script_name += strlen(SCRIPTS_PREFIX);
        temp_trig.script = (char *)script_name;
        ctx.type = data->self_type;
        ctx.trig = &temp_trig;
        ctx.go = NULL;
        if (data->self_type == MOB_TRIGGER)
          ctx.go = find_char(data->self_uid);
        else if (data->self_type == OBJ_TRIGGER)
          ctx.go = python_find_obj(data->self_uid);
        else if (data->self_type == WLD_TRIGGER)
          ctx.go = python_find_room(data->self_uid);
        current_context = &ctx;
        PyObject *call_args = PyTuple_Pack(1, event);
        PyObject *full_args = PySequence_Concat(call_args, data->args);
        Py_DECREF(call_args);
        Py_DECREF(event);
        if (full_args) {
          result = PyObject_Call(func, full_args, data->kwargs);
          Py_DECREF(full_args);
        }
        current_context = NULL;
      }
      if (!result) {
        if (mud_sleep_exc && PyErr_ExceptionMatches(mud_sleep_exc)) {
          PyErr_Clear();
        } else {
          python_log_exception("call_later");
        }
      }
      Py_XDECREF(result);
    } else {
      python_log_message("call_later: function not found");
    }
  }

  PyGILState_Release(gstate);

  if (data->args)
    Py_DECREF(data->args);
  if (data->kwargs)
    Py_DECREF(data->kwargs);
  free(data->script);
  free(data->func);
  free(data);

  return 0;
}

void python_scripts_init(void)
{
  PyObject *sys_path = NULL;
  PyObject *scripts_path = NULL;

  if (Py_IsInitialized())
    return;

  if (PyImport_AppendInittab("mud", PyInit_mud) == -1)
    return;

  Py_Initialize();

  if (PySys_AddAuditHook(python_audit_hook, NULL) < 0)
    PyErr_Clear();

  sys_path = PySys_GetObject("path");
  if (sys_path && PyList_Check(sys_path)) {
    scripts_path = PyUnicode_FromString("scripts");
    if (scripts_path) {
      PyList_Insert(sys_path, 0, scripts_path);
      Py_DECREF(scripts_path);
    }
  }

  mud_module = PyImport_ImportModule("mud");

  python_rotate_script_log();

  if (script_log_fp)
    fclose(script_log_fp);
  script_log_fp = fopen(SCRIPT_LOGFILE, "a");
  if (script_log_fp)
    fclose(script_log_fp);
  script_log_fp = NULL;

  if (!realpath(SCRIPT_LOGFILE, script_log_path))
    strlcpy(script_log_path, SCRIPT_LOGFILE, sizeof(script_log_path));
}

void python_scripts_shutdown(void)
{
  struct py_script_cache *entry = script_cache;
  struct dsl_script_cache *dentry = dsl_cache;
  struct dsl_state *state = dsl_states;

  if (!Py_IsInitialized())
    return;

  while (entry) {
    struct py_script_cache *next = entry->next;
    if (entry->globals)
      Py_DECREF(entry->globals);
    free(entry->path);
    free(entry);
    entry = next;
  }
  script_cache = NULL;

  while (dentry) {
    struct dsl_script_cache *next = dentry->next;
    if (dentry->globals)
      Py_DECREF(dentry->globals);
    if (dentry->func)
      Py_DECREF(dentry->func);
    free(dentry->path);
    free(dentry);
    dentry = next;
  }
  dsl_cache = NULL;

  while (state) {
    struct dsl_state *next = state->next;
    if (state->generator)
      Py_DECREF(state->generator);
    free(state->path);
    free(state);
    state = next;
  }
  dsl_states = NULL;

  if (mud_module) {
    Py_DECREF(mud_module);
    mud_module = NULL;
  }
  mud_sleep_exc = NULL;

  Py_Finalize();

  if (script_log_fp) {
    fclose(script_log_fp);
    script_log_fp = NULL;
  }
}

int python_trigger_run(void *go, struct trig_data *trig, int type, int mode)
{
  PyGILState_STATE gstate;
  struct py_script_cache *entry;
  PyObject *event = NULL;
  PyObject *func = NULL;
  PyObject *result = NULL;
  int ret = 1;
  char path[PATH_MAX];

  if (!trig || !trig->script || !*trig->script)
    return 1;

  snprintf(path, sizeof(path), "%s%s", SCRIPTS_PREFIX, trig->script);
  {
    char logbuf[MAX_STRING_LENGTH];
    snprintf(logbuf, sizeof(logbuf), "python_trigger_run: vnum=%d script=%s", GET_TRIG_VNUM(trig), path);
    python_log_message(logbuf);
  }

  if (python_is_dsl_script(path)) {
    (void)mode;
    return dsl_trigger_run(go, trig, type, path);
  }

  gstate = PyGILState_Ensure();

  entry = python_load_script(path);
  if (!entry || !entry->globals) {
    PyGILState_Release(gstate);
    return 1;
  }

  func = PyDict_GetItemString(entry->globals, "on_trigger");
  if (!func || !PyCallable_Check(func)) {
    python_log_message("Python script missing on_trigger(event)");
    PyGILState_Release(gstate);
    return 1;
  }

  event = python_build_event(trig, go, type);
  if (!event) {
    PyGILState_Release(gstate);
    return 1;
  }

  {
    struct py_trigger_context ctx;
    ctx.go = go;
    ctx.type = type;
    ctx.trig = trig;
    current_context = &ctx;
    result = PyObject_CallFunctionObjArgs(func, event, NULL);
    current_context = NULL;
  }

  Py_DECREF(event);

  if (!result) {
    if (mud_sleep_exc && PyErr_ExceptionMatches(mud_sleep_exc)) {
      PyErr_Clear();
      PyGILState_Release(gstate);
      return 1;
    }
    python_log_exception("on_trigger");
    PyGILState_Release(gstate);
    return 1;
  }

  if (result == Py_False)
    ret = 0;

  Py_DECREF(result);
  PyGILState_Release(gstate);

  (void)mode;

  return ret;
}
