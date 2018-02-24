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
		Com_DPrintf(S_COLOR_YELLOW "DB_Close(): %s\n", sqlite3_errstr(ret));
		assert(0); // resource leak
	}

	sls.db = NULL;
}

static qboolean DB_Open() {
	int			ret;

	if (sls.db) {
		return qtrue;
	}

	ret = sqlite3_open(sls.path, &sls.db);

	if (ret != SQLITE_OK) {
		Com_DPrintf(S_COLOR_YELLOW "DB_Open(): %s\n", sqlite3_errstr(ret));
		DB_Close();
		return qfalse;
	}

	return qtrue;
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

qboolean DB_Prepare(const char *sql) {
	int		ret;

	if (!sls.init) {
		Com_Error(ERR_FATAL, "Database call made without initialization");
	}

	if (!DB_Open()) {
		return qfalse;
	}

	DB_Finalize();

	ret = sqlite3_prepare_v2(sls.db, sql, -1, &sls.stmt, NULL);

	if (ret != SQLITE_OK) {
		Com_DPrintf(S_COLOR_YELLOW "DB_Prepare(): %s\n", sqlite3_errstr(ret));
		return qfalse;
	}

	return qtrue;
}

qboolean DB_Step() {
	if (!sls.init) {
		Com_Error(ERR_FATAL, "Database call made without initialization");
	}

	if (!sls.stmt) {
		Com_DPrintf(S_COLOR_YELLOW "DB_Bind(): No statement\n");
		return qfalse;
	}

	switch (sqlite3_step(sls.stmt)) {
	case SQLITE_ROW:
		sls.row = qtrue;
		return qtrue;
	case SQLITE_DONE:
		sls.row = qfalse;
		return qtrue;
	default:
		sls.row = qfalse;
		Com_DPrintf(S_COLOR_YELLOW "DB_Step(): %s\n", sqlite3_errstr(ret));
		return qfalse;
	}
}

qboolean DB_Column(mvdbValue_t *dst, int size, mvdbType_t *type, int col) {
	mvdbType_t	tp;

	if (!sls.init) {
		Com_Error(ERR_FATAL, "Database call made without initialization");
	}

	if (!sls.row) {
		Com_DPrintf(S_COLOR_YELLOW "DB_Column(): No row\n");
		return qfalse;
	}

	Com_Memset(dst, 0, size);

	switch (sqlite3_column_type(sls.stmt, col)) {
	case SQLITE_INTEGER:
		tp = MVDB_INTEGER;
		dst->integer = sqlite3_column_int(sls.stmt, col);
		break;
	case SQLITE_FLOAT:
		tp = MVDB_REAL;
		dst->real = sqlite3_column_double(sls.stmt, col);
		break;
	case SQLITE_TEXT:
		tp = MVDB_TEXT;
		Q_strncpyz(dst->text, (const char *)sqlite3_column_text(sls.stmt, col), size);
		break;
	case SQLITE_BLOB:
		tp = MVDB_BLOB;
		// sqlite3_column_text will add terminating 0 to blob
		Q_strncpyz(dst->text, (const char *)sqlite3_column_text(sls.stmt, col), size);
		break;
	case SQLITE_NULL:
		tp = MVDB_NULL;
		break;
	default:
		Com_DPrintf(S_COLOR_YELLOW "DB_Column(): %s\n", sqlite3_errmsg(sls.db));
		return qfalse;
	}

	if (type) {
		*type = tp;
	}

	return qtrue;
}