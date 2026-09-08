/* Exercise deletion, the shared trigger writer and save_all with injected
 * reference-writer failures. All disk writes stay in a temporary directory. */
#include "conf.h"
#include "sysdep.h"
#include "unity.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "dg_scripts.h"
#include "dg_olc.h"
#include "genolc.h"
#include "genzon.h"

struct zone_data *zone_table;
zone_rnum top_of_zone_table;
struct char_data *mob_proto;
struct index_data **trig_index;
int top_of_trigt;
struct trig_data *trigger_list;

static int failed_types;
static char original_dir[4096], scratch[] = "/tmp/tbamud-trigger-save-XXXXXX";

zone_rnum real_zone(zone_vnum vnum)
{
  return vnum == 30 ? 0 : vnum == 31 ? 1 : NOWHERE;
}

zone_rnum real_zone_by_thing(room_vnum vnum)
{
  return real_zone(vnum / 100);
}

trig_rnum real_trigger(trig_vnum vnum)
{
  int i;
  for (i = 0; i < top_of_trigt; i++)
    if (trig_index[i]->vnum == vnum)
      return i;
  return NOTHING;
}

void free_trigger(struct trig_data *trig) { free(trig); }
void extract_trigger(struct trig_data *trig) { TEST_FAIL_MESSAGE("Unexpected live trigger"); }
void extract_script(void *thing, int type) { TEST_FAIL_MESSAGE("Unexpected live script"); }
void delete_zone_command(struct zone_data *zone, int pos)
{
  do { zone->cmd[pos] = zone->cmd[pos + 1]; } while (zone->cmd[pos++].command != 'S');
}
void create_world_index(int zone, const char *type) { }

static int save_reference(zone_rnum zone, int type)
{
  if (failed_types & (1 << type))
    return FALSE;
  remove_from_save_list(zone_table[zone].number, type);
  return TRUE;
}
int save_mobiles(zone_rnum zone) { return save_reference(zone, SL_MOB); }
int save_objects(zone_rnum zone) { return save_reference(zone, SL_OBJ); }
int save_rooms(zone_rnum zone) { return save_reference(zone, SL_WLD); }
int save_zone(zone_rnum zone) { return save_reference(zone, SL_ZON); }
int save_shops(zone_rnum zone) { return save_reference(zone, SL_SHP); }
int save_quests(zone_rnum zone) { return save_reference(zone, SL_QST); }
int save_config(zone_rnum zone) { return TRUE; }

static void attach(struct trig_proto_list **list, int vnum)
{
  struct trig_proto_list *ref = calloc(1, sizeof(*ref));
  ref->vnum = vnum;
  ref->next = *list;
  *list = ref;
}

static void seed_file(void)
{
  FILE *file = fopen("world/trg/30.trg", "w");
  TEST_ASSERT_NOT_NULL(file);
  fputs("#3000\noriginal trigger\n", file);
  fclose(file);
}

static int retained_on_disk(void)
{
  char buf[4096];
  FILE *file = fopen("world/trg/30.trg", "r");
  size_t n;
  TEST_ASSERT_NOT_NULL(file);
  n = fread(buf, 1, sizeof(buf) - 1, file);
  buf[n] = '\0';
  fclose(file);
  return strstr(buf, "#3000\n") != NULL;
}

void setUp(void)
{
  int i;
  extern FILE *logfile;
  logfile = stderr;
  failed_types = 0;
  top_of_zone_table = 1;
  zone_table = calloc(2, sizeof(*zone_table));
  for (i = 0; i < 2; i++) {
    zone_table[i].number = 30 + i;
    zone_table[i].bot = 3000 + i * 100;
    zone_table[i].top = 3099 + i * 100;
    zone_table[i].cmd = calloc(2, sizeof(*zone_table[i].cmd));
    zone_table[i].cmd[0].command = 'S';
  }
  mob_proto = calloc(1, sizeof(*mob_proto));
  mob_index = calloc(1, sizeof(*mob_index));
  mob_index[0].vnum = 3100;
  obj_proto = calloc(1, sizeof(*obj_proto));
  obj_index = calloc(1, sizeof(*obj_index));
  obj_index[0].vnum = 3100;
  world = calloc(1, sizeof(*world));
  world[0].zone = 1;
  top_of_mobt = top_of_objt = top_of_world = 0;
  top_of_trigt = 2;
  trig_index = calloc(2, sizeof(*trig_index));
  for (i = 0; i < 2; i++) {
    trig_index[i] = calloc(1, sizeof(**trig_index));
    trig_index[i]->vnum = 3000 + i;
    trig_index[i]->proto = calloc(1, sizeof(struct trig_data));
    trig_index[i]->proto->nr = i;
  }
  seed_file();
}

void tearDown(void)
{
  int i;
  failed_types = 0;
  TEST_ASSERT_TRUE(save_all());
  TEST_ASSERT_NULL(save_list);
  for (i = 0; i < top_of_trigt; i++) {
    free_trigger(trig_index[i]->proto);
    free(trig_index[i]);
  }
  free(trig_index);
  free(mob_proto); free(mob_index);
  free(obj_proto); free(obj_index); free(world);
  for (i = 0; i < 2; i++) free(zone_table[i].cmd);
  free(zone_table);
}

void test_failed_delete_blocks_other_trigger_save(void)
{
  int stale;
  attach(&mob_proto[0].proto_script, 3000);
  failed_types = 1 << SL_MOB;
  TEST_ASSERT_TRUE(delete_trigger(0, &stale));
  TEST_ASSERT_EQUAL_INT(1, stale);
  TEST_ASSERT_EQUAL_INT(NOTHING, real_trigger(3000));
  TEST_ASSERT_TRUE(in_save_list(30, SL_TRG));
  /* Same writer used when trigedit saves the surviving trigger #3001. */
  TEST_ASSERT_FALSE(save_triggers(0));
  TEST_ASSERT_TRUE(retained_on_disk());
  TEST_ASSERT_FALSE(save_all());
  TEST_ASSERT_TRUE(retained_on_disk());
}

void test_saveall_finishes_delete_after_reference_repair(void)
{
  int stale;
  attach(&mob_proto[0].proto_script, 3000);
  failed_types = 1 << SL_MOB;
  TEST_ASSERT_TRUE(delete_trigger(0, &stale));
  TEST_ASSERT_EQUAL_INT(SL_TRG, save_list->type);
  failed_types = 0;
  TEST_ASSERT_TRUE(save_all());
  TEST_ASSERT_FALSE(retained_on_disk());
  TEST_ASSERT_NULL(save_list);
}

void test_all_reference_types_must_save(void)
{
  int stale;
  attach(&mob_proto[0].proto_script, 3000);
  attach(&obj_proto[0].proto_script, 3000);
  attach(&world[0].proto_script, 3000);
  zone_table[1].cmd[0].command = 'T';
  zone_table[1].cmd[0].arg2 = 0;
  zone_table[1].cmd[1].command = 'S';
  failed_types = (1 << SL_MOB) | (1 << SL_OBJ) | (1 << SL_WLD) | (1 << SL_ZON);
  TEST_ASSERT_TRUE(delete_trigger(0, &stale));
  TEST_ASSERT_EQUAL_INT(4, stale);
  failed_types = 1 << SL_ZON;
  TEST_ASSERT_FALSE(save_all());
  TEST_ASSERT_TRUE(retained_on_disk());
  failed_types = 0;
  TEST_ASSERT_TRUE(save_all());
  TEST_ASSERT_FALSE(retained_on_disk());
}

void test_second_delete_cannot_bypass_first_dependency(void)
{
  int stale;
  attach(&mob_proto[0].proto_script, 3000);
  failed_types = 1 << SL_MOB;
  TEST_ASSERT_TRUE(delete_trigger(0, &stale));
  TEST_ASSERT_TRUE(delete_trigger(0, &stale));
  TEST_ASSERT_EQUAL_INT(0, stale);
  TEST_ASSERT_FALSE(save_triggers(0));
  TEST_ASSERT_TRUE(retained_on_disk());
}

void test_trigger_io_failure_stays_queued(void)
{
  int stale;
  TEST_ASSERT_TRUE(delete_trigger(0, &stale));
  TEST_ASSERT_EQUAL_INT(0, stale);
  TEST_ASSERT_EQUAL_INT(0, rename("world/trg", "world/trg-held"));
  TEST_ASSERT_FALSE(save_all());
  TEST_ASSERT_TRUE(in_save_list(30, SL_TRG));
  TEST_ASSERT_EQUAL_INT(0, rename("world/trg-held", "world/trg"));
  TEST_ASSERT_TRUE(save_all());
  TEST_ASSERT_FALSE(retained_on_disk());
}

void test_direct_reference_save_unblocks_trigger_file(void)
{
  int stale;
  attach(&mob_proto[0].proto_script, 3000);
  failed_types = 1 << SL_MOB;
  TEST_ASSERT_TRUE(delete_trigger(0, &stale));
  failed_types = 0;
  TEST_ASSERT_TRUE(save_mobiles(1));
  TEST_ASSERT_TRUE(save_triggers(0));
  TEST_ASSERT_FALSE(retained_on_disk());
  TEST_ASSERT_NULL(save_list);
}

int main(void)
{
  int result;
  if (!getcwd(original_dir, sizeof(original_dir)) || !mkdtemp(scratch) || chdir(scratch))
    return 1;
  if (mkdir("world", 0700) || mkdir("world/trg", 0700))
    return 1;
  UNITY_BEGIN();
  RUN_TEST(test_failed_delete_blocks_other_trigger_save);
  RUN_TEST(test_saveall_finishes_delete_after_reference_repair);
  RUN_TEST(test_all_reference_types_must_save);
  RUN_TEST(test_second_delete_cannot_bypass_first_dependency);
  RUN_TEST(test_trigger_io_failure_stays_queued);
  RUN_TEST(test_direct_reference_save_unblocks_trigger_file);
  result = UNITY_END();
  remove("world/trg/30.trg");
  rmdir("world/trg"); rmdir("world");
  if (chdir(original_dir)) return 1;
  rmdir(scratch);
  return result;
}
