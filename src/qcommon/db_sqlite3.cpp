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

void DB_Bind(int pos, mvdbType_t type, const mvdbValue_t *value) {
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
		ret = sqlite3_bind_text(sls.stmt, pos, value->text, -1, SQLITE_TRANSIENT);
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

void DB_Column(mvdbValue_t *dst, int size, mvdbType_t *type, int col) {
	mvdbType_t	tp;

	if (!sls.init) {
		Com_Error(ERR_FATAL, "Database call made without initialization");
	}

	if (!sls.row) {
		Com_Error(ERR_DROP, "DB_Column(): No row");
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
		Com_Error(ERR_DROP, "DB_Column(): %s", sqlite3_errmsg(sls.db));
	}

	if (type) {
		*type = tp;
	}
}
