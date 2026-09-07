/* Exercise the real savers, save list and file installation in an isolated
 * directory. Only world lookup, logging and trigger/tab output are stubbed;
 * these fixtures contain neither triggers nor tabs. */
#include "unity.h"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "genolc.h"
#include "genmob.h"
#include "genobj.h"
#include "genwld.h"
#include "dg_olc.h"
#ifdef CIRCLE_WINDOWS
#include <direct.h>
#include <windows.h>
#define test_mkdir(path) _mkdir(path)
#define test_chdir(path) _chdir(path)
#define test_rmdir(path) _rmdir(path)
#define test_getcwd(buf, size) _getcwd(buf, size)
#else
#define test_mkdir(path) mkdir(path, 0700)
#define test_chdir(path) chdir(path)
#define test_rmdir(path) rmdir(path)
#define test_getcwd(buf, size) getcwd(buf, size)
#endif

static struct zone_data zone;
struct zone_data *zone_table = &zone;
zone_rnum top_of_zone_table = 0;
static struct room_data rooms[2];
struct room_data *world = rooms;
static struct obj_data objects[2];
struct obj_data *obj_proto = objects;
obj_rnum top_of_objt = 1;
static struct char_data mobiles[2];
struct char_data *mob_proto = mobiles;
static struct index_data object_index[2], mobile_index[2];
struct index_data *obj_index = object_index, *mob_index = mobile_index;
struct config_data config_info;

room_rnum real_room(room_vnum vnum) { return vnum >= 3000 && vnum <= 3001 ? vnum - 3000 : NOWHERE; }
obj_rnum real_object(obj_vnum vnum) { return vnum >= 3000 && vnum <= 3001 ? vnum - 3000 : NOTHING; }
mob_rnum real_mobile(mob_vnum vnum) { return vnum >= 3000 && vnum <= 3001 ? vnum - 3000 : NOBODY; }
zone_rnum real_zone(zone_vnum vnum) { return vnum == 30 ? 0 : NOWHERE; }
void basic_mud_log(const char *format, ...) { (void)format; }
void mudlog(int type, int level, int file, const char *format, ...)
{ (void)type; (void)level; (void)file; (void)format; }
void script_save_to_disk(FILE *fp, void *item, int type)
{ (void)fp; (void)item; (void)type; }
char *convert_from_tabs(char *text) { return text; }

static char large[MAX_STRING_LENGTH / 2 + 1];
static char original[4096], result[4096];
static const char *paths[] = { "world/wld/30.wld", "world/obj/30.obj", "world/mob/30.mob" };
static const char *scratch[] = { "world/wld/30.new", "world/obj/30.new", "world/mob/30.new" };
static int (*savers[])(zone_rnum) = { save_rooms, save_objects, save_mobiles };
static const int types[] = { SL_WLD, SL_OBJ, SL_MOB };

void setUp(void)
{
    int i;
    memset(&zone, 0, sizeof(zone));
    zone.number = 30;
    zone.bot = 3000;
    zone.top = 3001;
    memset(rooms, 0, sizeof(rooms));
    memset(objects, 0, sizeof(objects));
    memset(mobiles, 0, sizeof(mobiles));
    memset(&config_info, 0, sizeof(config_info));
    memset(large, 'x', sizeof(large) - 1);
    large[sizeof(large) - 1] = '\0';
    for (i = 0; i < 2; ++i) {
        rooms[i].number = 3000 + i;
        rooms[i].name = "original";
        rooms[i].description = "description\n";
        objects[i].item_number = i;
        object_index[i].vnum = 3000 + i;
        objects[i].name = "original";
        objects[i].short_description = "an object";
        objects[i].description = "An object is here.";
        mobiles[i].nr = i;
        mobile_index[i].vnum = 3000 + i;
        GET_ALIAS(&mobiles[i]) = "original";
        GET_SDESC(&mobiles[i]) = "a mobile";
        GET_LDESC(&mobiles[i]) = "A mobile is here.\n";
        GET_DDESC(&mobiles[i]) = "description\n";
    }
}

void tearDown(void)
{
    int i;
    free_save_list();
    save_list = NULL;
    for (i = 0; i < 3; ++i) {
        remove(paths[i]);
        remove(scratch[i]);
    }
}

static void read_file(const char *path, char *buffer, size_t capacity)
{
    FILE *fp = fopen(path, "rb");
    size_t length;
    TEST_ASSERT_NOT_NULL(fp);
    length = fread(buffer, 1, capacity - 1, fp);
    /* Catch unexpected growth instead of silently comparing a prefix. */
    int extra = fgetc(fp);
    int error = ferror(fp);
    fclose(fp);
    TEST_ASSERT_EQUAL_INT(EOF, extra);
    TEST_ASSERT_EQUAL_INT(0, error);
    buffer[length] = '\0';
}

static void exercise_save(int kind, int debug)
{
    FILE *fp;
    CONFIG_DEBUG_MODE = debug;
    TEST_ASSERT_TRUE(savers[kind](0));
    read_file(paths[kind], original, sizeof(original));
    TEST_ASSERT_NOT_NULL(strstr(original, "#3000"));
    TEST_ASSERT_NOT_NULL(strstr(original, "#3001"));

    /* A valid edited record precedes the oversized record, so failure must
     * discard a partially written zone and retain the builder's pending edit.
     * Every individual string remains below the loader's per-string limit. */
    rooms[0].name = objects[0].name = GET_ALIAS(&mobiles[0]) = "edited";
    rooms[1].name = rooms[1].description = large;
    objects[1].name = objects[1].description = large;
    GET_ALIAS(&mobiles[1]) = GET_DDESC(&mobiles[1]) = large;
    TEST_ASSERT_TRUE(add_to_save_list(30, types[kind]));
    TEST_ASSERT_FALSE(savers[kind](0));
    read_file(paths[kind], result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING(original, result);
    TEST_ASSERT_TRUE(in_save_list(30, types[kind]));
    fp = fopen(scratch[kind], "rb");
    if (fp) fclose(fp);
    TEST_ASSERT_EQUAL_INT(debug, fp != NULL);
    if (debug) {
        read_file(scratch[kind], result, sizeof(result));
        TEST_ASSERT_NOT_NULL(strstr(result, "edited"));
        TEST_ASSERT_NULL(strstr(result, "#3001"));
    }

    /* Repair and retry: the edit must finally reach disk, both records must
     * survive, and only this successful save may clear the pending entry. */
    rooms[1].name = objects[1].name = GET_ALIAS(&mobiles[1]) = "repaired";
    rooms[1].description = objects[1].description = GET_DDESC(&mobiles[1]) = "description\n";
    TEST_ASSERT_TRUE(savers[kind](0));
    TEST_ASSERT_FALSE(in_save_list(30, types[kind]));
    read_file(paths[kind], result, sizeof(result));
    TEST_ASSERT_NOT_NULL(strstr(result, "#3000"));
    TEST_ASSERT_NOT_NULL(strstr(result, "#3001"));
    TEST_ASSERT_NOT_NULL(strstr(result, "edited"));
    TEST_ASSERT_NOT_NULL(strstr(result, "repaired"));
    TEST_ASSERT_NOT_NULL(strstr(result, kind == 2 ? "$" : "$~"));
    fp = fopen(scratch[kind], "rb");
    if (fp) fclose(fp);
    TEST_ASSERT_NULL(fp);
}

static void test_rooms_oversize(void) { exercise_save(0, 0); }
static void test_objects_oversize(void) { exercise_save(1, 0); }
static void test_mobiles_oversize(void) { exercise_save(2, 0); }
static void test_rooms_debug_scratch(void) { exercise_save(0, 1); }
static void test_objects_debug_scratch(void) { exercise_save(1, 1); }
static void test_mobiles_debug_scratch(void) { exercise_save(2, 1); }

int main(void)
{
    char cwd[4096], temp[4096];
    int status;
    if (!test_getcwd(cwd, sizeof(cwd))) return 1;
#ifdef CIRCLE_WINDOWS
    {
        char root[MAX_PATH];
        if (!GetTempPathA(sizeof(root), root) ||
            !GetTempFileNameA(root, "olc", 0, temp)) return 1;
        if (!DeleteFileA(temp) || test_mkdir(temp)) return 1;
    }
#else
    strcpy(temp, "/tmp/tbamud-savers-XXXXXX");
    if (!mkdtemp(temp)) return 1;
#endif
    if (test_chdir(temp) || test_mkdir("world") ||
        test_mkdir("world/wld") || test_mkdir("world/obj") ||
        test_mkdir("world/mob")) return 1;
    UNITY_BEGIN();
    RUN_TEST(test_rooms_oversize);
    RUN_TEST(test_objects_oversize);
    RUN_TEST(test_mobiles_oversize);
    RUN_TEST(test_rooms_debug_scratch);
    RUN_TEST(test_objects_debug_scratch);
    RUN_TEST(test_mobiles_debug_scratch);
    status = UNITY_END();
    test_rmdir("world/wld");
    test_rmdir("world/obj");
    test_rmdir("world/mob");
    test_rmdir("world");
    if (test_chdir(cwd) || test_rmdir(temp)) return 1;
    return status;
}
