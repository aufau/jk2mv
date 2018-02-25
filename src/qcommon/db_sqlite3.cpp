// db_sqlite3.cpp -- SQLite3 implementation of db_public.h

#include <sqlite3.h>

#include "db_public.h"

typedef struct {
	qboolean		init;
	char			path[MAX_OSPATH];
	sqlite3			*db;
	sqlite3_stmt	*stmt;
	qboolean		row;	// if we have a valid row after sqlite3_step()
} sqliteStatic_t;

static sqliteStatic_t sls;

static void DB_Close() {
	int		ret;

	ret = sqlite3_close(sls.db);

	if (ret != SQLITE_OK) {
		// should never happen or it's engine error (and resource leak)
		Com_Error(ERR_FATAL, "DB_Close(): %s", sqlite3_errstr(ret));
	}

	sls.db = NULL;
}

static void DB_Open() {
	int			ret;

	if (sls.db) {
		return;
	}

	ret = sqlite3_open(sls.path, &sls.db);

	if (ret != SQLITE_OK) {
		Com_Error(ERR_DROP, "DB_Open(): %s", sqlite3_errstr(ret));
	}
}

static void DB_Finalize() {
	sqlite3_finalize(sls.stmt);
	sls.stmt = NULL;
	sls.row = qfalse;
}

void DB_Startup(const char *path) {
	sls.init = qtrue;
	Q_strncpyz(sls.path, path, sizeof(sls.path));
}

void DB_Shutdown() {
	DB_Finalize();
	DB_Close();

	Com_Memset(&sls, 0, sizeof(sls));
}

void DB_Prepare(const char *sql) {
	int		ret;

	if (!sls.init) {
		Com_Error(ERR_FATAL, "Database call made without initialization");
	}

	DB_Open();
	DB_Finalize();

	ret = sqlite3_prepare_v2(sls.db, sql, -1, &sls.stmt, NULL);

	if (ret != SQLITE_OK) {
		Com_Error(ERR_DROP, "DB_Prepare(): %s", sqlite3_errstr(ret));
	}
}

void DB_Bind(int pos, mvdbType_t type, const mvdbValue_t *value, int valueSize) {
	int		ret;

	if (!sls.init) {
		Com_Error(ERR_FATAL, "Database call made without initialization");
	}

	// passing finalized statements to sqlite3_bind_* may be harmful
	if (!sls.stmt) {
		Com_Error(ERR_DROP, "DB_Bind(): No statement");
	}

	switch (type) {
	case MVDB_INTEGER:
		ret = sqlite3_bind_int(sls.stmt, pos, value->integer);
		break;
	case MVDB_REAL:
		ret = sqlite3_bind_double(sls.stmt, pos, value->real);
		break;
	case MVDB_TEXT:
		ret = sqlite3_bind_text(sls.stmt, pos, value->text, valueSize, SQLITE_TRANSIENT);
		break;
	case MVDB_BLOB:
		ret = sqlite3_bind_blob(sls.stmt, pos, value->blob, valueSize, SQLITE_TRANSIENT);
		break;
	case MVDB_NULL:
		ret = sqlite3_bind_null(sls.stmt, pos);
		break;
	default:
		Com_Error(ERR_DROP, "DB_Bind(): Invalid type");
	}

	if (ret != SQLITE_OK) {
		Com_Error(ERR_DROP, "DB_Bind(): %s", sqlite3_errstr(ret));
	}
}

qboolean DB_Step() {
	int		ret;

	if (!sls.init) {
		Com_Error(ERR_FATAL, "Database call made without initialization");
	}

	if (!sls.stmt) {
		Com_Error(ERR_DROP, "DB_Step(): No statement");
	}

	switch (ret = sqlite3_step(sls.stmt)) {
	case SQLITE_ROW:
		sls.row = qtrue;
		return qtrue;
	case SQLITE_OK:
	case SQLITE_DONE:
		sls.row = qfalse;
		return qfalse;
	case SQLITE_ERROR:
		Com_Error(ERR_DROP, "DB_Step(): %s", sqlite3_errmsg(sls.db));
	case SQLITE_BUSY:	// TODO: may happen when another process/thread locks db.
	default:
		Com_Error(ERR_DROP, "DB_Step(): %s", sqlite3_errstr(ret));
	}
}

int DB_Column(mvdbValue_t *value, int valueSize, mvdbType_t type, int col) {
	const void	*blob;
	int			size;

	if (!sls.init) {
		Com_Error(ERR_FATAL, "Database call made without initialization");
	}

	if (!sls.row) {
		Com_Error(ERR_DROP, "DB_Column(): No row");
	}

	if (sqlite3_column_type(sls.stmt, col) == SQLITE_NULL) {
		return -1;
	}

	switch (type) {
	case MVDB_INTEGER:
		value->integer = sqlite3_column_int(sls.stmt, col);
		size = sizeof(value->integer);
		break;
	case MVDB_REAL:
		value->real = sqlite3_column_double(sls.stmt, col);
		size = sizeof(value->real);
		break;
	case MVDB_TEXT:
		Q_strncpyz(value->text, (const char *)sqlite3_column_text(sls.stmt, col), valueSize);
		size = sqlite3_column_bytes(sls.stmt, col);
		break;
	case SQLITE_BLOB:
		blob = sqlite3_column_blob(sls.stmt, col);
		size = sqlite3_column_bytes(sls.stmt, col);
		Com_Memcpy(value->blob, blob, MIN(size, valueSize));
		break;
	default:
		Com_Error(ERR_DROP, "DB_Column(): Invalid type");
	}

	return size;
}

void DB_Meminfo() {
	long long int used = sqlite3_memory_used();
	long long int highwater = sqlite3_memory_highwater(0);

	Com_Printf("SQLite3 is using %lld bytes (%.2fMB)\n", used, used / 1024.0 / 1024.0);
	Com_Printf("SQLite3 peaked at %lld bytes (%.2fMB)\n", highwater, highwater / 1024.0 / 1024.0);
}
