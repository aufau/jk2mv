// db_public.h -- Simple database API

#include "../qcommon/qcommon.h"
#include "../api/mvapi.h"

void DB_Startup(const char *path);
void DB_Shutdown();
void DB_Meminfo();

void DB_Prepare(const char *sql);
void DB_Bind(int pos, mvdbType_t type, const mvdbValue_t *value, int valueSize);
qboolean DB_Step();
int DB_Column(mvdbValue_t *value, int valueSize, mvdbType_t type, int col);
