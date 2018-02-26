// db_public.h -- Simple database API

#include "../qcommon/qcommon.h"
#include "../api/mvapi.h"

void DB_Startup(const char *path);
void DB_Shutdown();
void DB_Meminfo();

mvstmtHandle_t DB_Prepare(const char *sql);
void DB_Bind(mvstmtHandle_t h, int pos, mvdbType_t type, const mvdbValue_t *value, int valueSize);
mvdbResult_t DB_Step(mvstmtHandle_t h);
int DB_Column(mvstmtHandle_t h, mvdbValue_t *value, int valueSize, mvdbType_t type, int col);
void DB_Reset(mvstmtHandle_t h);
void DB_Finalize(mvstmtHandle_t h);
