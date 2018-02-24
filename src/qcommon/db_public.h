// db_public.h -- Simple database API

#include "../qcommon/qcommon.h"
#include "../api/mvapi.h"

void DB_Startup(const char *path);
void DB_Shutdown();

qboolean DB_Prepare(const char *sql);
qboolean DB_Step();
qboolean DB_Column(mvdbValue_t *dst, int size, mvdbType_t *type, int col);
