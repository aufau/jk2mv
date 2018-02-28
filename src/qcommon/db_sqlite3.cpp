// db_sqlite3.cpp -- SQLite3 implementation of db_public.h

#include <sqlite3.h>

#include "db_public.h"

#define MAX_STATEMENT_HANDLES 32

typedef struct {
	sqlite3_stmt	*p;
	qboolean		row;	// if we have a valid row after sqlite3_step()
} sqliteStmt_t;

typedef struct {
	qboolean		init;
	char			path[MAX_OSPATH];
	sqlite3			*db;
	sqliteStmt_t	statements[MAX_STATEMENT_HANDLES];	// 0 is reserved
} sqliteStatic_t;

static sqliteStatic_t sls;

static mvstmtHandle_t DB_HandleForStatement() {
	for (int i = 1; i < MAX_STATEMENT_HANDLES; i++) {
		if (sls.statements[i].p == NULL) {
			return i;
		}
	}

	Com_Error(ERR_DROP, "DB_HandleForStatement(): None free");
}

static sqliteStmt_t *DB_StatementForHandle(mvstmtHandle_t h) {
	if (h < 1 || MAX_STATEMENT_HANDLES <= h) {
		Com_Error(ERR_DROP, "DB_StatementForHandle(): Out of range");
	}
	if (sls.statements[h].p == NULL) {
		Com_Error(ERR_DROP, "DB_StatementForHandle(): NULL");
	}

	return &sls.statements[h];
}

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

void DB_Startup(const char *path) {
	sls.init = qtrue;
	Q_strncpyz(sls.path, path, sizeof(sls.path));
}

void DB_Shutdown() {
	for (int i = 0; i < MAX_STATEMENT_HANDLES; i++) {
		sqlite3_finalize(sls.statements[i].p);
	}

	DB_Close();

	Com_Memset(&sls, 0, sizeof(sls));
}

mvstmtHandle_t DB_Prepare(const char *sql) {
	mvstmtHandle_t	h;
	int				ret;

	if (!sls.init) {
		Com_Error(ERR_FATAL, "Database call made without initialization");
	}

	DB_Open();

	h = DB_HandleForStatement();

	ret = sqlite3_prepare_v2(sls.db, sql, -1, &sls.statements[h].p, NULL);

	if (ret != SQLITE_OK) {
		Com_Error(ERR_DROP, "DB_Prepare(): %s", sqlite3_errstr(ret));
	}

	return h;
}

void DB_Bind(mvstmtHandle_t h, int pos, mvdbType_t type, const mvdbValue_t *value, int valueSize) {
	sqliteStmt_t	*statement;
	int				ret;

	if (!sls.init) {
		Com_Error(ERR_FATAL, "Database call made without initialization");
	}

	statement = DB_StatementForHandle(h);

	switch (type) {
	case MVDB_INTEGER:
		ret = sqlite3_bind_int(statement->p, pos, value->integer);
		break;
	case MVDB_REAL:
		ret = sqlite3_bind_double(statement->p, pos, value->real);
		break;
	case MVDB_TEXT:
		ret = sqlite3_bind_text(statement->p, pos, value->text, valueSize, SQLITE_TRANSIENT);
		break;
	case MVDB_BLOB:
		ret = sqlite3_bind_blob(statement->p, pos, value->blob, valueSize, SQLITE_TRANSIENT);
		break;
	case MVDB_NULL:
		ret = sqlite3_bind_null(statement->p, pos);
		break;
	default:
		Com_Error(ERR_DROP, "DB_Bind(): Invalid type");
	}

	if (ret != SQLITE_OK) {
		Com_Error(ERR_DROP, "DB_Bind(): %s", sqlite3_errstr(ret));
	}
}

mvdbResult_t DB_Step(mvstmtHandle_t h) {
	sqliteStmt_t	*statement;
	int				ret;

	if (!sls.init) {
		Com_Error(ERR_FATAL, "Database call made without initialization");
	}

	statement = DB_StatementForHandle(h);
	statement->row = qfalse;

	switch (ret = sqlite3_step(statement->p)) {
	case SQLITE_ROW:
		statement->row = qtrue;
		return MVDB_ROW;
	case SQLITE_OK:
	case SQLITE_DONE:
		return MVDB_DONE;
	case SQLITE_BUSY:
		Com_DPrintf( S_COLOR_YELLOW "DB_Step(): %s", sqlite3_errstr(ret));
		return MVDB_BUSY;
	case SQLITE_CONSTRAINT:
		Com_DPrintf( S_COLOR_YELLOW "DB_Step(): %s", sqlite3_errstr(ret));
		return MVDB_CONSTRAINT;
	case SQLITE_ERROR:
		Com_Error(ERR_DROP, "DB_Step(): %s", sqlite3_errmsg(sls.db));
	default:
		Com_Error(ERR_DROP, "DB_Step(): %s", sqlite3_errstr(ret));
	}
}

int DB_Column(mvstmtHandle_t h, mvdbValue_t *value, int valueSize, mvdbType_t type, int col) {
	sqliteStmt_t	*statement;
	const void		*blob;
	int				size;

	if (!sls.init) {
		Com_Error(ERR_FATAL, "Database call made without initialization");
	}

	statement = DB_StatementForHandle(h);

	if (!statement->row) {
		Com_Error(ERR_DROP, "DB_Column(): No row");
	}

	if (sqlite3_column_type(statement->p, col) == SQLITE_NULL) {
		return -1;
	}

	switch (type) {
	case MVDB_INTEGER:
		value->integer = sqlite3_column_int(statement->p, col);
		size = sizeof(value->integer);
		break;
	case MVDB_REAL:
		value->real = sqlite3_column_double(statement->p, col);
		size = sizeof(value->real);
		break;
	case MVDB_TEXT:
		Q_strncpyz(value->text, (const char *)sqlite3_column_text(statement->p, col), valueSize);
		size = sqlite3_column_bytes(statement->p, col);
		break;
	case SQLITE_BLOB:
		blob = sqlite3_column_blob(statement->p, col);
		size = sqlite3_column_bytes(statement->p, col);
		Com_Memcpy(value->blob, blob, MIN(size, valueSize));
		break;
	default:
		Com_Error(ERR_DROP, "DB_Column(): Invalid type");
	}

	return size;
}

void DB_Reset(mvstmtHandle_t h) {
	sqliteStmt_t	*statement;

	if (!sls.init) {
		Com_Error(ERR_FATAL, "Database call made without initialization");
	}

	statement = DB_StatementForHandle(h);

	sqlite3_reset(statement->p);
	statement->row = qfalse;
}

void DB_Finalize(mvstmtHandle_t h) {
	sqliteStmt_t	*statement;

	if (!sls.init) {
		Com_Error(ERR_FATAL, "Database call made without initialization");
	}

	statement = DB_StatementForHandle(h);

	sqlite3_finalize(statement->p);
	Com_Memset(statement, 0, sizeof(*statement));
}

void DB_Meminfo() {
	long long int used = sqlite3_memory_used();
	long long int highwater = sqlite3_memory_highwater(0);

	Com_Printf("SQLite3 is using %lld bytes (%.2fMB)\n", used, used / 1024.0 / 1024.0);
	Com_Printf("SQLite3 peaked at %lld bytes (%.2fMB)\n", highwater, highwater / 1024.0 / 1024.0);
}
