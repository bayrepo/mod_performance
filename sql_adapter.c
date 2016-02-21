/*
 * Copyright 2012 Alexey Berezhok (alexey_com@ukr.net, bayrepo.info@gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * sql_adapter.c
 *
 *  Created on: May 21, 2011
 *  Author: SKOREE
 *  E-mail: alexey_com@ukr.net
 *  Site: lexvit.dn.ua
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <dlfcn.h>
#include <time.h>

#include <apr_general.h>
#include <apr_pools.h>
#include <apr_tables.h>
#include <apr_time.h>
#include <apr_strings.h>
#include <apr_thread_mutex.h>

#include "httpd.h"
#include "http_config.h"
#include <http_log.h>
#include "http_protocol.h"
#include <pthread.h>

#include "custom_report.h"

pthread_rwlock_t db_lock_rw = PTHREAD_RWLOCK_INITIALIZER;

extern int performance_history;

char *get_host_id();

//-------------------Common section---------------------------
#define LOAD_FUNCTION(x) _##x = dlsym(lib_handle, #x); \
                if ((error = dlerror()) != NULL) {\
                        return -1; \
                } else if ( !_##x ) { \
                	return -1; \
                }



//-------------------MySQL section----------------------------
#define M_mysql_store_result void * (*_mysql_store_result)(void *)
#define M_mysql_num_rows unsigned long long (*_mysql_num_rows)(void *)
#define M_mysql_free_result void (*_mysql_free_result)(void *)
#define M_mysql_fetch_lengths unsigned long * (*_mysql_fetch_lengths)(void *)
#define M_mysql_fetch_row char ** (*_mysql_fetch_row)(void *)
#define M_my_init char (*_my_init)()
#define M_load_defaults int (*_load_defaults)(const char *, const char **, int *, char ***)
#define M_mysql_init void * (*_mysql_init)(void *)
#define M_mysql_real_connect void * (*_mysql_real_connect)( \
    void *, \
    const char *, \
    const char *, \
    const char *, \
    const char *, \
    unsigned int, \
    const char *, \
    unsigned long)
#define M_mysql_options int (*_mysql_options)(void *mysql, int mysql_option, const char *)
#define M_mysql_query int (*_mysql_query)(void *mysql, const char *)
#define M_mysql_close void (*_mysql_close)(void *)
#define M_mysql_error const char * (*_mysql_error)(void *)
#define M_mysql_real_escape_string unsigned long (*_mysql_real_escape_string)(void *mysql, char *, const char *, unsigned long)
#define M_mysql_ping int (*_mysql_ping)(void *)

M_mysql_store_result = NULL;
M_mysql_num_rows = NULL;
M_mysql_free_result = NULL;
M_mysql_fetch_lengths = NULL;
M_mysql_fetch_row = NULL;
M_my_init = NULL;
M_load_defaults = NULL;
M_mysql_init = NULL;
M_mysql_real_connect = NULL;
M_mysql_options = NULL;
M_mysql_query = NULL;
M_mysql_close = NULL;
M_mysql_error = NULL;
M_mysql_real_escape_string = NULL;
M_mysql_ping = NULL;

typedef void MYSQL;
typedef void MYSQL_RES;
typedef char **MYSQL_ROW;
typedef char my_bool;

enum mysql_option {
	MYSQL_OPT_CONNECT_TIMEOUT,
	MYSQL_OPT_COMPRESS,
	MYSQL_OPT_NAMED_PIPE,
	MYSQL_INIT_COMMAND,
	MYSQL_READ_DEFAULT_FILE,
	MYSQL_READ_DEFAULT_GROUP,
	MYSQL_SET_CHARSET_DIR,
	MYSQL_SET_CHARSET_NAME,
	MYSQL_OPT_LOCAL_INFILE,
	MYSQL_OPT_PROTOCOL,
	MYSQL_SHARED_MEMORY_BASE_NAME,
	MYSQL_OPT_READ_TIMEOUT,
	MYSQL_OPT_WRITE_TIMEOUT,
	MYSQL_OPT_USE_RESULT,
	MYSQL_OPT_USE_REMOTE_CONNECTION,
	MYSQL_OPT_USE_EMBEDDED_CONNECTION,
	MYSQL_OPT_GUESS_CONNECTION,
	MYSQL_SET_CLIENT_IP,
	MYSQL_SECURE_AUTH,
	MYSQL_REPORT_DATA_TRUNCATION,
	MYSQL_OPT_RECONNECT,
	MYSQL_OPT_SSL_VERIFY_SERVER_CERT
};

MYSQL *m_db = NULL;
MYSQL *m_db_r = NULL;
my_bool reconnect = 1;
//-------------------SQLite section---------------------------
typedef void sqlite3;
typedef void sqlite3_stmt;
typedef int (*sqlite3_callback)(void *, int, char **, char **);

//sqlite3_exec
#define M_sqlite3_exec int (*_sqlite3_exec)(sqlite3*, const char *sql, sqlite3_callback, void *, char **errmsg)
#define M_sqlite3_mprintf char * (*_sqlite3_mprintf)(const char*,...)
#define M_sqlite3_free void (*_sqlite3_free)(char *z)
#define M_sqlite3_close int (*_sqlite3_close)(sqlite3 *)
#define M_sqlite3_open int (*_sqlite3_open)(const char *filename, sqlite3 **ppDb)
#define M_sqlite3_prepare int (*_sqlite3_prepare)(sqlite3 *db, const char *zSql, int nBytes, sqlite3_stmt **ppStmt, const char **pzTail)
#define M_sqlite3_column_count int (*_sqlite3_column_count)(sqlite3_stmt *pStmt)
#define M_sqlite3_step int (*_sqlite3_step)(sqlite3_stmt*)
#define M_sqlite3_column_text const unsigned char *(*_sqlite3_column_text)(sqlite3_stmt*, int iCol)
#define M_sqlite3_reset int (*_sqlite3_reset)(sqlite3_stmt *pStmt)
#define M_sqlite3_column_int int (*_sqlite3_column_int)(sqlite3_stmt*, int iCol)
#define M_sqlite3_column_double double (*_sqlite3_column_double)(sqlite3_stmt*, int iCol)
#define M_sqlite3_errstr const char * (*_sqlite3_errstr)(int)
#define M_sqlite3_errmsg const char * (*_sqlite3_errmsg)(sqlite3 *)

M_sqlite3_exec = NULL;
M_sqlite3_mprintf = NULL;
M_sqlite3_free = NULL;
M_sqlite3_close = NULL;
M_sqlite3_open = NULL;
M_sqlite3_prepare = NULL;
M_sqlite3_column_count = NULL;
M_sqlite3_step = NULL;
M_sqlite3_column_text = NULL;
M_sqlite3_reset = NULL;
M_sqlite3_column_int = NULL;
M_sqlite3_column_double = NULL;
M_sqlite3_errstr = NULL;
M_sqlite3_errmsg = NULL;

#define SQLITE_OK           0	/* Successful result */
/* beginning-of-error-codes */
#define SQLITE_ERROR        1	/* SQL error or missing database */
#define SQLITE_INTERNAL     2	/* NOT USED. Internal logic error in SQLite */
#define SQLITE_PERM         3	/* Access permission denied */
#define SQLITE_ABORT        4	/* Callback routine requested an abort */
#define SQLITE_BUSY         5	/* The database file is locked */
#define SQLITE_LOCKED       6	/* A table in the database is locked */
#define SQLITE_NOMEM        7	/* A malloc() failed */
#define SQLITE_READONLY     8	/* Attempt to write a readonly database */
#define SQLITE_INTERRUPT    9	/* Operation terminated by sqlite3_interrupt() */
#define SQLITE_IOERR       10	/* Some kind of disk I/O error occurred */
#define SQLITE_CORRUPT     11	/* The database disk image is malformed */
#define SQLITE_NOTFOUND    12	/* NOT USED. Table or record not found */
#define SQLITE_FULL        13	/* Insertion failed because database is full */
#define SQLITE_CANTOPEN    14	/* Unable to open the database file */
#define SQLITE_PROTOCOL    15	/* Database lock protocol error */
#define SQLITE_EMPTY       16	/* Database is empty */
#define SQLITE_SCHEMA      17	/* The database schema changed */
#define SQLITE_TOOBIG      18	/* NOT USED. Too much data for one row */
#define SQLITE_CONSTRAINT  19	/* Abort due to contraint violation */
#define SQLITE_MISMATCH    20	/* Data type mismatch */
#define SQLITE_MISUSE      21	/* Library used incorrectly */
#define SQLITE_NOLFS       22	/* Uses OS features not supported on host */
#define SQLITE_AUTH        23	/* Authorization denied */
#define SQLITE_FORMAT      24	/* Auxiliary database format error */
#define SQLITE_RANGE       25	/* 2nd parameter to sqlite3_bind out of range */
#define SQLITE_NOTADB      26	/* File opened that is not a database file */
#define SQLITE_ROW         100	/* sqlite3_step() has another row ready */
#define SQLITE_DONE        101	/* sqlite3_step() has finished executing */

sqlite3 *s_db = NULL;
sqlite3 *s_db_r = NULL;
//-------------------PostgreSQL section-----------------------

#define PG_GETVALUE(x,y) ((*_PQgetisnull)( result, x, y )?"":(*_PQgetvalue)( result, x, y ))

#define PQClear(param) \
  if (param!=NULL) (*_PQclear)(param)

typedef enum {
	/*
	 * Although it is okay to add to this list, values which become unused
	 * should never be removed, nor should constants be redefined - that would
	 * break compatibility with existing code.
	 */
	CONNECTION_OK, CONNECTION_BAD,
	/* Non-blocking mode only below here */

	/*
	 * The existence of these should never be relied upon - they should only
	 * be used for user feedback or similar purposes.
	 */
	CONNECTION_STARTED, /* Waiting for connection to be made.  */
	CONNECTION_MADE, /* Connection OK; waiting to send.         */
	CONNECTION_AWAITING_RESPONSE, /* Waiting for a response from the
	 * postmaster.            */
	CONNECTION_AUTH_OK, /* Received authentication; waiting for
	 * backend startup. */
	CONNECTION_SETENV, /* Negotiating environment. */
	CONNECTION_SSL_STARTUP, /* Negotiating SSL. */
	CONNECTION_NEEDED
/* Internal state: connect() needed */
} ConnStatusType;

typedef enum {
	PGRES_EMPTY_QUERY = 0, /* empty query string was executed */
	PGRES_COMMAND_OK, /* a query command that doesn't return
	 * anything was executed properly by the
	 * backend */
	PGRES_TUPLES_OK, /* a query command that returns tuples was
	 * executed properly by the backend, PGresult
	 * contains the result tuples */
	PGRES_COPY_OUT, /* Copy Out data transfer in progress */
	PGRES_COPY_IN, /* Copy In data transfer in progress */
	PGRES_BAD_RESPONSE, /* an unexpected response was recv'd from the
	 * backend */
	PGRES_NONFATAL_ERROR, /* notice or warning message */
	PGRES_FATAL_ERROR
/* query failed */
} ExecStatusType;

typedef enum
{
   PQPING_OK,                  /* server is accepting connections */
   PQPING_REJECT,              /* server is alive but rejecting connections */
   PQPING_NO_RESPONSE,         /* could not establish connection */
   PQPING_NO_ATTEMPT           /* connection not attempted (bad params) */
} PGPing;

typedef void PGconn;
typedef void PGresult;
#define M_PQexec PGresult *(*_PQexec)(PGconn *conn, const char *query)
#define M_PQerrorMessage char *(*_PQerrorMessage)(const PGconn *conn)
#define M_PQresultStatus ExecStatusType (*_PQresultStatus)(const PGresult *res)
#define M_PQclear void (*_PQclear)(PGresult *res)
#define M_PQntuples int (*_PQntuples)(const PGresult *res)
#define M_PQgetvalue char *(*_PQgetvalue)(const PGresult *res, int tup_num, int field_num)
#define M_PQescapeString size_t (*_PQescapeString)(char *to, const char *from, size_t length)
#define M_PQnfields int (*_PQnfields)(const PGresult *res)
#define M_PQgetisnull int (*_PQgetisnull)(const PGresult *res, int tup_num, int field_num)
#define M_PQfinish void (*_PQfinish)(PGconn *conn)
#define M_PQconnectdb PGconn *(*_PQconnectdb)(const char *conninfo)
#define M_PQstatus ConnStatusType (*_PQstatus)(const PGconn *conn)
#define M_PQping PGPing (*_PQping)(const char *conninfo)

M_PQexec = NULL;
M_PQerrorMessage = NULL;
M_PQresultStatus = NULL;
M_PQclear = NULL;
M_PQntuples = NULL;
M_PQgetvalue = NULL;
M_PQescapeString = NULL;
M_PQnfields = NULL;
M_PQgetisnull = NULL;
M_PQfinish = NULL;
M_PQconnectdb = NULL;
M_PQstatus = NULL;
M_PQping = NULL;

PGconn *p_db = NULL;
PGconn *p_db_r = NULL;

//------------------------------------------------------------

static void *lib_handle = NULL;
static char *error = NULL;

#define PGSQL_CON_STRING 16384

static char *performance_username = NULL;
static char *performance_password = NULL;
static char *performance_dbname = NULL;
static char *performance_dbhost = NULL;
static char pgsql_conn_str[PGSQL_CON_STRING] = "";

static apr_status_t sql_adapter_cleanup_libhandler(void *dummy) {
	if (lib_handle != NULL) {
		dlclose(lib_handle);
		lib_handle = NULL;
	}
	return APR_SUCCESS;
}

static apr_status_t sql_adapter_cleanup_close_mysql(void *dummy) {
	pthread_rwlock_wrlock(&db_lock_rw);
	if (m_db != NULL) {
		(*_mysql_close)(m_db);
		m_db = NULL;
	}
	if (m_db_r != NULL) {
		(*_mysql_close)(m_db_r);
		m_db_r = NULL;
	}
	pthread_rwlock_unlock(&db_lock_rw);
	return APR_SUCCESS;
}

static apr_status_t sql_adapter_cleanup_close_sqlite(void *dummy) {
	pthread_rwlock_wrlock(&db_lock_rw);
	if (s_db != NULL) {
		(*_sqlite3_close)(s_db);
		s_db = NULL;
	}
	if (s_db_r != NULL) {
		(*_sqlite3_close)(s_db_r);
		s_db_r = NULL;
	}
	pthread_rwlock_unlock(&db_lock_rw);
	return APR_SUCCESS;
}

static apr_status_t sql_adapter_cleanup_close_postgres(void *dummy) {
	pthread_rwlock_wrlock(&db_lock_rw);
	if (p_db != NULL) {
		(*_PQfinish)(p_db);
		p_db = NULL;
	}
	if (p_db_r != NULL) {
		(*_PQfinish)(p_db_r);
		p_db_r = NULL;
	}
	pthread_rwlock_unlock(&db_lock_rw);
	return APR_SUCCESS;
}

int sql_adapter_load_library(apr_pool_t * p, int db_type) {
	if (lib_handle == NULL) {
		switch (db_type) {
		case 1:
			//SqLite - libsqlite3.so
			lib_handle = dlopen("libsqlite3.so", RTLD_LAZY);
			if (!lib_handle) {
				lib_handle = dlopen("libsqlite3.so.0", RTLD_LAZY);
				if (!lib_handle) {
					return -1;
				}
			}
			LOAD_FUNCTION(sqlite3_exec);
			LOAD_FUNCTION(sqlite3_mprintf);
			LOAD_FUNCTION(sqlite3_free);
			LOAD_FUNCTION(sqlite3_close);
			LOAD_FUNCTION(sqlite3_open);
			LOAD_FUNCTION(sqlite3_prepare);
			LOAD_FUNCTION(sqlite3_column_count);
			LOAD_FUNCTION(sqlite3_step);
			LOAD_FUNCTION(sqlite3_column_text);
			LOAD_FUNCTION(sqlite3_reset);
			LOAD_FUNCTION(sqlite3_column_int);
			LOAD_FUNCTION(sqlite3_column_double);
			LOAD_FUNCTION(sqlite3_errstr);
			LOAD_FUNCTION(sqlite3_errmsg);
			break;
		case 2:
			//MySQL common
			lib_handle = dlopen("libmysqlclient_r.so", RTLD_LAZY);
			if (!lib_handle) {
				lib_handle = dlopen("libmysqlclient_r.so.18", RTLD_LAZY);
				if (!lib_handle) {
					lib_handle = dlopen("libmysqlclient_r.so.16", RTLD_LAZY);
					if (!lib_handle) {
						lib_handle = dlopen("libmysqlclient_r.so.15",
								RTLD_LAZY);
						if (!lib_handle) {
							return -1;
						}
					}
				}
			}
			LOAD_FUNCTION(mysql_store_result);
			LOAD_FUNCTION(mysql_num_rows);
			LOAD_FUNCTION(mysql_free_result);
			LOAD_FUNCTION(mysql_fetch_lengths);
			LOAD_FUNCTION(mysql_fetch_row);
			LOAD_FUNCTION(my_init);
			LOAD_FUNCTION(load_defaults);
			LOAD_FUNCTION(mysql_init);
			LOAD_FUNCTION(mysql_real_connect);
			LOAD_FUNCTION(mysql_options);
			LOAD_FUNCTION(mysql_query);
			LOAD_FUNCTION(mysql_close);
			LOAD_FUNCTION(mysql_error);
			LOAD_FUNCTION(mysql_real_escape_string);
			LOAD_FUNCTION(mysql_ping);
			break;
		case 3:
			//PgSql
			lib_handle = dlopen("libpq.so", RTLD_LAZY);
			if (!lib_handle) {
				lib_handle = dlopen("libpq.so.5", RTLD_LAZY);
				if (!lib_handle) {
					return -1;
				}
			}
			LOAD_FUNCTION(PQexec);
			LOAD_FUNCTION(PQerrorMessage);
			LOAD_FUNCTION(PQresultStatus);
			LOAD_FUNCTION(PQclear);
			LOAD_FUNCTION(PQntuples);
			LOAD_FUNCTION(PQgetvalue);
			LOAD_FUNCTION(PQescapeString);
			LOAD_FUNCTION(PQnfields);
			LOAD_FUNCTION(PQgetisnull);
			LOAD_FUNCTION(PQfinish);
			LOAD_FUNCTION(PQconnectdb);
			LOAD_FUNCTION(PQstatus);
			LOAD_FUNCTION(PQping);
			break;
		}
		apr_pool_cleanup_register(p, NULL, sql_adapter_cleanup_libhandler,
				apr_pool_cleanup_null);
	}
	return 0;
}

#define mysql_reconnect(x) if(smysql_reconnect(&x)<0) x=0

static int smysql_reconnect(MYSQL ** ptr) {
	pthread_rwlock_rdlock(&db_lock_rw);
	if (ptr && *ptr ) {
		if ((*_mysql_ping)(*ptr)) {
			pthread_rwlock_unlock(&db_lock_rw);
			pthread_rwlock_wrlock(&db_lock_rw);
			(*_mysql_options)(*ptr, MYSQL_OPT_RECONNECT, &reconnect);
			if (!(*_mysql_real_connect)(*ptr, performance_dbhost,
					performance_username, performance_password,
					performance_dbname, 0, NULL, 0)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return -1;
			}
			pthread_rwlock_unlock(&db_lock_rw);
		} else {
			pthread_rwlock_unlock(&db_lock_rw);
		}
	} else {
		pthread_rwlock_unlock(&db_lock_rw);
	}
	return 0;
}

#define pgsql_reconnect(x) if(spgsql_reconnect(&x)<0) x=0

static int spgsql_reconnect(PGconn ** ptr) {
	PGresult *result = NULL;
	pthread_rwlock_rdlock(&db_lock_rw);
	if (ptr && *ptr && pgsql_conn_str[0]) {
		if ((result = (*_PQexec)(*ptr, "")) == NULL) {
			(*_PQfinish)(*ptr);
			pthread_rwlock_unlock(&db_lock_rw);
			pthread_rwlock_wrlock(&db_lock_rw);
			*ptr = (*_PQconnectdb)(pgsql_conn_str);
			if (!*ptr) {
				(*_PQfinish)(*ptr);
				pthread_rwlock_unlock(&db_lock_rw);
				return -1;
			}
			if ((*_PQstatus)(p_db_r) != CONNECTION_OK) {
				(*_PQfinish)(*ptr);
				pthread_rwlock_unlock(&db_lock_rw);
				return -1;
			}
			pthread_rwlock_unlock(&db_lock_rw);
		} else {
			pthread_rwlock_unlock(&db_lock_rw);
		}
		PQClear(result);
	} else {
		pthread_rwlock_unlock(&db_lock_rw);
	}
	return 0;
}

static char *get_error_description(void *db, char *str, apr_pool_t * p, int db_type){
	char *new_str = str;
	switch (db_type) {
	case 1:
		//SqLite - libsqlite3.so
		new_str = apr_psprintf(p, "%s : %s", str, (*_sqlite3_errmsg)((sqlite3 *)db));
		break;
	case 2:
		//MySQL common
		new_str = apr_psprintf(p, "%s : %s", str, (*_mysql_error)(db));
		break;
	case 3:
		//PgSql
		new_str = apr_psprintf(p, "%s : %s", str, (*_PQerrorMessage)((const PGconn *)db));
		break;
	}
	return new_str;
}

int sql_adapter_connect_db(apr_pool_t * p, int db_type, char *host,
		char *username, char *password, char *dbname, char *path) {

	performance_username = username ? apr_pstrdup(p, username) : NULL;
	performance_password = password ? apr_pstrdup(p, password) : NULL;
	performance_dbname = dbname ? apr_pstrdup(p, dbname) : NULL;
	;
	performance_dbhost = host ? apr_pstrdup(p, host) : NULL;
	pthread_rwlock_wrlock(&db_lock_rw);

	switch (db_type) {
	case 1:
		//SqLite
	{
		int rc = (*_sqlite3_open)(path, &s_db);
		if (rc) {
			pthread_rwlock_unlock(&db_lock_rw);
			return -1;
		}
		rc = (*_sqlite3_open)(path, &s_db_r);
		if (rc) {
			pthread_rwlock_unlock(&db_lock_rw);
			return -1;
		}
		apr_pool_cleanup_register(p, NULL, sql_adapter_cleanup_close_sqlite,
				apr_pool_cleanup_null);
	}
		break;
	case 2:
		//MySQL
	{
		(*_my_init)();
		m_db = (*_mysql_init)(NULL);
		if (!m_db) {
			pthread_rwlock_unlock(&db_lock_rw);
			return -1;
		}
		(*_mysql_options)(m_db, MYSQL_OPT_RECONNECT, &reconnect);
		if (!(*_mysql_real_connect)(m_db, host, username, password, dbname, 0,
				NULL, 0)) {
			pthread_rwlock_unlock(&db_lock_rw);
			return -1;
		}

		m_db_r = (*_mysql_init)(NULL);
		if (!m_db_r) {
			(*_mysql_close)(m_db);
			pthread_rwlock_unlock(&db_lock_rw);
			return -1;
		}
		(*_mysql_options)(m_db_r, MYSQL_OPT_RECONNECT, &reconnect);
		if (!(*_mysql_real_connect)(m_db_r, host, username, password, dbname, 0,
				NULL, 0)) {
			(*_mysql_close)(m_db);
			pthread_rwlock_unlock(&db_lock_rw);
			return -1;
		}

		apr_pool_cleanup_register(p, NULL, sql_adapter_cleanup_close_mysql,
				apr_pool_cleanup_null);
	}
		break;
	case 3: {
		//extract port number form host
		char host_only[512], port_only[512];

		if (strstr((host?host:"localhost"), ":") && host) {
			char *pos = strstr(host, ":");
			strncpy(host_only, host, pos - host);
			host_only[pos - host] = '\0';
			strncpy(port_only, pos + 1, strlen(host) - (pos - host));
			port_only[strlen(host) - (pos - host)] = '\0';
		} else {
			strncpy(host_only, host?host:"localhost", 512);
			strncpy(port_only, "", 512);
			host_only[511] = '\0';
			port_only[511] = '\0';
		}


		//PgSql
		char *connect_string;
		if (!port_only[0])
			connect_string = apr_psprintf(p,
					"host=%s dbname=%s user=%s password=%s", host?host:"localhost", dbname,
					username, password);
		else
			connect_string = apr_psprintf(p,
					"host=%s dbname=%s user=%s password=%s port=%s", host_only,
					dbname, username, password, port_only);
		strncpy(pgsql_conn_str, connect_string, PGSQL_CON_STRING-1);
		p_db = (*_PQconnectdb)(connect_string);
		if (!p_db) {
			pthread_rwlock_unlock(&db_lock_rw);
			return -1;
		}
		if ((*_PQstatus)(p_db) != CONNECTION_OK) {
			(*_PQfinish)(p_db);
			pthread_rwlock_unlock(&db_lock_rw);
			return -1;
		}
		p_db_r = (*_PQconnectdb)(connect_string);
		if (!p_db_r) {
			(*_PQfinish)(p_db);
			pthread_rwlock_unlock(&db_lock_rw);
			return -1;
		}
		if ((*_PQstatus)(p_db_r) != CONNECTION_OK) {
			(*_PQfinish)(p_db);
			(*_PQfinish)(p_db_r);
			pthread_rwlock_unlock(&db_lock_rw);
			return -1;
		}

		apr_pool_cleanup_register(p, NULL, sql_adapter_cleanup_close_postgres,
				apr_pool_cleanup_null);
	}
		break;
	}
	pthread_rwlock_unlock(&db_lock_rw);
	return 0;
}

char *
sql_adapter_get_create_table(apr_pool_t * p, int db_type,
		apr_thread_mutex_t * mutex_db) {
	switch (db_type) {
	case 1:
		//SqLite
	{
		if (s_db) {
			int rv;
			char *errmsg = apr_palloc(p, 512);
			char *sql_buffer =
					apr_pstrdup(
							p,
							"CREATE TABLE IF NOT EXISTS performance(id INTEGER PRIMARY KEY AUTOINCREMENT, dateadd DATETIME, host CHAR(255), uri CHAR(512), script CHAR(512), cpu FLOAT, memory FLOAT, exc_time FLOAT, cpu_sec FLOAT, memory_mb FLOAT, bytes_read FLOAT, bytes_write FLOAT, hostnm CHAR(32))");
			apr_thread_mutex_lock(mutex_db);
			rv = (*_sqlite3_exec)(s_db, sql_buffer, 0, 0, &errmsg);
			apr_thread_mutex_unlock(mutex_db);
			if (rv != SQLITE_OK) {
				return errmsg;
			}
			return NULL;
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db);
		if (m_db) {
			char *sql_buffer =
					apr_pstrdup(
							p,
							"CREATE TABLE IF NOT EXISTS performance(id INT NOT NULL AUTO_INCREMENT, dateadd DATETIME, host VARCHAR(255), uri VARCHAR(512), script VARCHAR(512), cpu FLOAT(15,5), memory FLOAT(15,5), exc_time FLOAT(15,5), cpu_sec FLOAT(15,5), memory_mb FLOAT(15,5), bytes_read FLOAT(15,5), bytes_write FLOAT(15,5), hostnm CHAR(32), PRIMARY KEY(id))");
			pthread_rwlock_rdlock(&db_lock_rw);
			apr_thread_mutex_lock(mutex_db);
			if ((*_mysql_query)(m_db, sql_buffer)) {
				char *errmsg = apr_pstrdup(p, (*_mysql_error)(m_db));
				apr_thread_mutex_unlock(mutex_db);
				pthread_rwlock_unlock(&db_lock_rw);
				return errmsg;
			}
			apr_thread_mutex_unlock(mutex_db);
			pthread_rwlock_unlock(&db_lock_rw);
			return NULL;
		}
	}
		break;
	case 3:
		//PgSql
	{
		pgsql_reconnect(p_db);
		if (p_db) {
			PGresult *result;
			char *sql_buffer =
					apr_pstrdup(
							p,
							"select count(*) from pg_class where relname = 'performance'");
			char *sql_buffer2 =
					apr_pstrdup(
							p,
							"create table performance(id SERIAL, dateadd timestamp, host varchar(255), uri varchar(512), script varchar(512), cpu float(4), memory float(4), exc_time float(4), cpu_sec float(4), memory_mb float(4), bytes_read float(4), bytes_write float(4), hostnm char(32), PRIMARY KEY(id))");

			pthread_rwlock_rdlock(&db_lock_rw);
			apr_thread_mutex_lock(mutex_db);
			if ((result = (*_PQexec)(p_db, sql_buffer)) == NULL) {
				char *errmsg = apr_pstrdup(p, (*_PQerrorMessage)(p_db));
				apr_thread_mutex_unlock(mutex_db);
				pthread_rwlock_unlock(&db_lock_rw);
				return errmsg;
			}
			if ((*_PQresultStatus)(result) != PGRES_TUPLES_OK) {
				char *errmsg = apr_pstrdup(p, (*_PQerrorMessage)(p_db));
				PQClear(result);
				apr_thread_mutex_unlock(mutex_db);
				pthread_rwlock_unlock(&db_lock_rw);
				return errmsg;
			}
			if ((*_PQntuples)(result) != 1) {
				char *errmsg = apr_pstrdup(p, "Strange count value");
				PQClear(result);
				apr_thread_mutex_unlock(mutex_db);
				pthread_rwlock_unlock(&db_lock_rw);
				return errmsg;
			}
			int cnt = (int) apr_atoi64((*_PQgetvalue)(result, 0, 0));
			PQClear(result);
			if (!cnt) {
				if ((result = (*_PQexec)(p_db, sql_buffer2)) == NULL) {
					char *errmsg = apr_pstrdup(p, (*_PQerrorMessage)(p_db));
					apr_thread_mutex_unlock(mutex_db);
					pthread_rwlock_unlock(&db_lock_rw);
					return errmsg;
				}
				if ((*_PQresultStatus)(result) != PGRES_COMMAND_OK) {
					char *errmsg = apr_pstrdup(p, (*_PQerrorMessage)(p_db));
					PQClear(result);
					apr_thread_mutex_unlock(mutex_db);
					pthread_rwlock_unlock(&db_lock_rw);
					return errmsg;
				}
				PQClear(result);
			}
			apr_thread_mutex_unlock(mutex_db);
			pthread_rwlock_unlock(&db_lock_rw);
			return NULL;
		}
	}
		break;
	}
	return NULL;
}

char *
sql_adapter_get_host_name(apr_pool_t * p, int db_type) {
	switch (db_type) {
	case 1:
		//SqLite
	{
		if (s_db)
			return NULL;
	}
		break;
	case 2:
		//MySQL
	{
		if (m_db) {
			char *inner_host_name = apr_palloc(p,
					strlen(get_host_id()) * 2 + 1);
			(*_mysql_real_escape_string)(m_db, inner_host_name, get_host_id(),
					strlen(get_host_id()));

			return inner_host_name;
		}
	}
		break;
	case 3:
		//PgSql
	{
		if (p_db) {
			char *inner_host_name = apr_palloc(p,
					strlen(get_host_id()) * 2 + 1);
			(*_PQescapeString)(inner_host_name, get_host_id(),
					strlen(get_host_id()));

			return inner_host_name;
		}
	}
		break;
	}
	return NULL;
}

char *
sql_adapter_get_insert_table(apr_pool_t * p, int db_type, char *hostname,
		char *uri, char *script, double dtime, double dcpu, double dmemory,
		apr_thread_mutex_t * mutex_db, double dcpu_sec, double dmemory_mb,
		double dbr, double dbw) {
	char *inner_hostname = apr_pstrndup(p, hostname, 255);
	char *inner_uri = apr_pstrndup(p, uri, 512);
	char *inner_script = apr_pstrndup(p, script, 512);
	switch (db_type) {
	case 1:
		//SqLite
	{
		if (s_db) {
			int rv;
			char *errmsg = apr_palloc(p, 512);
			char *sql_buffer;
			char *inner_request_quoted =
					(*_sqlite3_mprintf)(
							"insert into performance(dateadd, host, uri, script, cpu, memory, exc_time, cpu_sec, memory_mb, bytes_read, bytes_write) values(strftime('%%Y-%%m-%%d %%H:%%M:%%S', current_timestamp, 'localtime'),'%q','%q','%q',%f,%f,%f,%f,%f,%f,%f)",
							inner_hostname, inner_uri, inner_script, dcpu,
							dmemory, dtime, dcpu_sec, dmemory_mb, dbr, dbw);
			sql_buffer = apr_pstrdup(p, inner_request_quoted);
			(*_sqlite3_free)(inner_request_quoted);
			apr_thread_mutex_lock(mutex_db);
			rv = (*_sqlite3_exec)(s_db, sql_buffer, 0, 0, &errmsg);
			apr_thread_mutex_unlock(mutex_db);
			if (rv != SQLITE_OK) {
				return errmsg;
			}
			return NULL;
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db);
		if (m_db) {
			char *sql_buffer;
			char *inner_hostname_q = apr_palloc(p,
					strlen(inner_hostname) * 2 + 1);
			char *inner_uri_q = apr_palloc(p, strlen(inner_uri) * 2 + 1);
			char *inner_script_q = apr_palloc(p, strlen(inner_script) * 2 + 1);
			char *inner_host_name = sql_adapter_get_host_name(p, db_type);
			(*_mysql_real_escape_string)(m_db, inner_hostname_q, inner_hostname,
					strlen(inner_hostname));
			(*_mysql_real_escape_string)(m_db, inner_uri_q, inner_uri,
					strlen(inner_uri));
			(*_mysql_real_escape_string)(m_db, inner_script_q, inner_script,
					strlen(inner_script));
			sql_buffer =
					apr_psprintf(
							p,
							"insert into performance(dateadd, host, uri, script, cpu, memory, exc_time, cpu_sec, memory_mb, bytes_read, bytes_write, hostnm) values(now(),'%s','%s','%s',%f,%f,%f,%f,%f,%f,%f,'%s')",
							inner_hostname_q, inner_uri_q, inner_script_q, dcpu,
							dmemory, dtime, dcpu_sec, dmemory_mb, dbr, dbw,
							inner_host_name);
			pthread_rwlock_rdlock(&db_lock_rw);
			apr_thread_mutex_lock(mutex_db);
			if ((*_mysql_query)(m_db, sql_buffer)) {
				apr_thread_mutex_unlock(mutex_db);
				char *errmsg = apr_pstrdup(p, (*_mysql_error)(m_db));
				pthread_rwlock_unlock(&db_lock_rw);
				return errmsg;
			}
			apr_thread_mutex_unlock(mutex_db);
			pthread_rwlock_unlock(&db_lock_rw);
			return NULL;
		}
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db);
		if (p_db) {
			char *sql_buffer;
			char *inner_hostname_q = apr_palloc(p,
					strlen(inner_hostname) * 2 + 1);
			char *inner_uri_q = apr_palloc(p, strlen(inner_uri) * 2 + 1);
			char *inner_script_q = apr_palloc(p, strlen(inner_script) * 2 + 1);
			char *inner_host_name = sql_adapter_get_host_name(p, db_type);
			(*_PQescapeString)(inner_hostname_q, inner_hostname,
					strlen(inner_hostname));
			(*_PQescapeString)(inner_uri_q, inner_uri, strlen(inner_uri));
			(*_PQescapeString)(inner_script_q, inner_script,
					strlen(inner_script));

			sql_buffer =
					apr_psprintf(
							p,
							"insert into performance(dateadd, host, uri, script, cpu, memory, exc_time, cpu_sec, memory_mb, bytes_read, bytes_write, hostnm) values(now(),'%s','%s','%s',%f,%f,%f,%f,%f,%f,%f,'%s')",
							inner_hostname_q, inner_uri_q, inner_script_q, dcpu,
							dmemory, dtime, dcpu_sec, dmemory_mb, dbr, dbw,
							inner_host_name);
			pthread_rwlock_rdlock(&db_lock_rw);
			apr_thread_mutex_lock(mutex_db);
			if ((result = (*_PQexec)(p_db, sql_buffer)) == NULL) {
				apr_thread_mutex_unlock(mutex_db);
				char *errmsg = apr_pstrdup(p, (*_PQerrorMessage)(p_db));
				pthread_rwlock_unlock(&db_lock_rw);
				return errmsg;
			}
			if ((*_PQresultStatus)(result) != PGRES_COMMAND_OK) {
				apr_thread_mutex_unlock(mutex_db);
				char *errmsg = apr_pstrdup(p, (*_PQerrorMessage)(p_db));
				PQClear(result);
				pthread_rwlock_unlock(&db_lock_rw);
				return errmsg;
			}
			PQClear(result);
			apr_thread_mutex_unlock(mutex_db);
			pthread_rwlock_unlock(&db_lock_rw);

			return NULL;
		}
	}
		break;
	}
	return NULL;
}

char *
sql_adapter_get_delete_table(apr_pool_t * p, int db_type, int days,
		apr_thread_mutex_t * mutex_db) {
	switch (db_type) {
	case 1:
		//SqLite
	{
		if (s_db) {
			int rv;
			char *errmsg = apr_palloc(p, 512);
			char *sql_buffer;
			char *inner_request_quoted =
					(*_sqlite3_mprintf)(
							"delete from performance where dateadd<datetime('now','start of day','-%d day')",
							days);
			sql_buffer = apr_pstrdup(p, inner_request_quoted);
			(*_sqlite3_free)(inner_request_quoted);
			apr_thread_mutex_lock(mutex_db);
			rv = (*_sqlite3_exec)(s_db, sql_buffer, 0, 0, &errmsg);
			apr_thread_mutex_unlock(mutex_db);
			if (rv != SQLITE_OK) {
				return errmsg;
			}
			return NULL;
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db);
		if (m_db) {
			char *sql_buffer;
			sql_buffer =
					apr_psprintf(
							p,
							"delete from performance where FROM_DAYS(TO_DAYS(dateadd)) < FROM_DAYS(TO_DAYS(now())-%d)",
							days);
			pthread_rwlock_rdlock(&db_lock_rw);
			apr_thread_mutex_lock(mutex_db);
			if ((*_mysql_query)(m_db, sql_buffer)) {
				apr_thread_mutex_unlock(mutex_db);
				char *errmsg = apr_pstrdup(p, (*_mysql_error)(m_db));
				pthread_rwlock_unlock(&db_lock_rw);
				return errmsg;
			}
			apr_thread_mutex_unlock(mutex_db);
			pthread_rwlock_unlock(&db_lock_rw);
			return NULL;
		}
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db);
		if (p_db) {
			char *sql_buffer;
			sql_buffer =
					apr_psprintf(
							p,
							"delete from performance where date(dateadd)<(date(now())-integer '%d')",
							days);
			pthread_rwlock_rdlock(&db_lock_rw);
			apr_thread_mutex_lock(mutex_db);
			if ((result = (*_PQexec)(p_db, sql_buffer)) == NULL) {

				char *errmsg = apr_pstrdup(p, (*_PQerrorMessage)(p_db));
				apr_thread_mutex_unlock(mutex_db);
				pthread_rwlock_unlock(&db_lock_rw);
				return errmsg;
			}
			if ((*_PQresultStatus)(result) != PGRES_COMMAND_OK) {
				char *errmsg = apr_pstrdup(p, (*_PQerrorMessage)(p_db));
				PQClear(result);
				apr_thread_mutex_unlock(mutex_db);
				pthread_rwlock_unlock(&db_lock_rw);
				return errmsg;
			}PQClear(result);
			apr_thread_mutex_unlock(mutex_db);
			pthread_rwlock_unlock(&db_lock_rw);

			return NULL;
		}
	}
		break;
	}
	return NULL;
}

static int performance_get_period(char *period) {
	int i_period = apr_atoi64(period ? period : "1");
	if (i_period > performance_history)
		i_period = performance_history;
	if (i_period < 1)
		i_period = 1;
	return i_period;
}

static char *
parseDateIntoCorrectFormat(apr_pool_t * p, char *inputDate) {

	struct tm s_tm;
	char *cp;

	cp = strptime(inputDate, "%Y-%m-%d %H:%M:%S", &s_tm);
	if (!cp) {
		strptime(inputDate, "%Y/%m/%d %H:%M:%S", &s_tm);
	}
	cp = apr_palloc(p, 512);
	strftime(cp, 512, "%Y-%m-%d %H:%M:%S", &s_tm);
	return cp;
}

char *
sql_adapter_get_date_period(apr_pool_t * p, char *period_begin,
		char *period_end, char *period, int db_type, char *alias) {
	int i_period = performance_get_period(period);

	char *field_name =
			(!alias) ?
					apr_pstrdup(p, "dateadd") :
					apr_pstrcat(p, alias, ".dateadd", NULL);

	char *date_period = apr_pstrdup(p, "");
	if (period_begin && period_end) {
		period_begin = parseDateIntoCorrectFormat(p, period_begin);
		period_end = parseDateIntoCorrectFormat(p, period_end);
		date_period = apr_psprintf(p, "%s between '%s' and '%s'", field_name,
				period_begin, period_end);
	} else {
		switch (db_type) {
		case 1:
			//SqLite
			date_period = apr_psprintf(p,
					"%s>=datetime('now','start of day','-%d day')", field_name,
					i_period);
			break;
		case 2:
			//MySQL
			date_period = apr_psprintf(p,
					"FROM_DAYS(TO_DAYS(%s)) >= FROM_DAYS(TO_DAYS(now())-%d)",
					field_name, i_period);
			break;
		case 3:
			//PgSql
			date_period = apr_psprintf(p,
					"date(%s) >= (date(now())-integer '%d')", field_name,
					i_period);
			break;
		}

	}
	return date_period;
}

char *
sql_adapter_get_filter(apr_pool_t * p, char *host, char *script, char *uri,
		int db_type, char *alias) {

	char *host_name =
			(!alias) ?
					apr_pstrdup(p, "host") :
					apr_pstrcat(p, alias, ".host", NULL);
	char *script_name =
			(!alias) ?
					apr_pstrdup(p, "script") :
					apr_pstrcat(p, alias, ".script", NULL);
	char *uri_name =
			(!alias) ?
					apr_pstrdup(p, "uri") : apr_pstrcat(p, alias, ".uri", NULL);
	char *addfilter = apr_pstrdup(p, "");
	char *tmp_buf = NULL;
	switch (db_type) {
	case 1:
		//SqLite
	{
		if (host) {
			char *hhost = apr_pstrndup(p, host, 255);
			tmp_buf = apr_psprintf(p, " and %s like '%%q' ", host_name);
			char *shost = (*_sqlite3_mprintf)(tmp_buf, hhost);
			hhost = apr_pstrdup(p, shost);
			(*_sqlite3_free)(shost);
			addfilter = apr_pstrcat(p, addfilter, hhost, NULL);
		}
		if (script) {
			char *hscript = apr_pstrndup(p, script, 512);
			tmp_buf = apr_psprintf(p, " and %s like '%%q' ", script_name);
			char *sscript = (*_sqlite3_mprintf)(tmp_buf, hscript);
			hscript = apr_pstrdup(p, sscript);
			(*_sqlite3_free)(sscript);
			addfilter = apr_pstrcat(p, addfilter, hscript, NULL);
		}
		if (uri) {
			char *huri = apr_pstrndup(p, uri, 512);
			tmp_buf = apr_psprintf(p, " and %s like '%%q' ", uri_name);
			char *suri = (*_sqlite3_mprintf)(tmp_buf, huri);
			huri = apr_pstrdup(p, suri);
			(*_sqlite3_free)(suri);
			addfilter = apr_pstrcat(p, addfilter, huri, NULL);
		}
	}
		break;
	case 2:
		//MySQL
		if (host) {
			char *hhost = apr_pstrndup(p, host, 255);
			char *hhost_q = apr_palloc(p, strlen(hhost) * 2 + 1);
			(*_mysql_real_escape_string)(m_db, hhost_q, hhost, strlen(hhost));
			char *shost = apr_psprintf(p, " and %s like '%s' ", host_name,
					hhost_q);
			addfilter = apr_pstrcat(p, addfilter, shost, NULL);
		}
		if (script) {
			char *hscript = apr_pstrndup(p, script, 512);
			char *hscript_q = apr_palloc(p, strlen(hscript) * 2 + 1);
			(*_mysql_real_escape_string)(m_db, hscript_q, hscript,
					strlen(hscript));
			char *shscript = apr_psprintf(p, " and %s like '%s' ", script_name,
					hscript_q);
			addfilter = apr_pstrcat(p, addfilter, shscript, NULL);
		}
		if (uri) {
			char *huri = apr_pstrndup(p, uri, 512);
			char *huri_q = apr_palloc(p, strlen(huri) * 2 + 1);
			(*_mysql_real_escape_string)(m_db, huri_q, huri, strlen(huri));
			char *suri = apr_psprintf(p, " and %s like '%s' ", uri_name,
					huri_q);
			addfilter = apr_pstrcat(p, addfilter, suri, NULL);
		}
		break;
	case 3:
		//PgSql
		if (host) {
			char *hhost = apr_pstrndup(p, host, 255);
			char *hhost_q = apr_palloc(p, strlen(hhost) * 2 + 1);
			(*_PQescapeString)(hhost_q, hhost, strlen(hhost));
			char *shost = apr_psprintf(p, " and %s like '%s' ", host_name,
					hhost_q);
			addfilter = apr_pstrcat(p, addfilter, shost, NULL);
		}
		if (script) {
			char *hscript = apr_pstrndup(p, script, 512);
			char *hscript_q = apr_palloc(p, strlen(hscript) * 2 + 1);
			(*_PQescapeString)(hscript_q, hscript, strlen(hscript));
			char *shscript = apr_psprintf(p, " and %s like '%s' ", script_name,
					hscript_q);
			addfilter = apr_pstrcat(p, addfilter, shscript, NULL);
		}
		if (uri) {
			char *huri = apr_pstrndup(p, uri, 512);
			char *huri_q = apr_palloc(p, strlen(huri) * 2 + 1);
			(*_PQescapeString)(huri_q, huri, strlen(huri));
			char *suri = apr_psprintf(p, " and %s like '%s' ", uri_name,
					huri_q);
			addfilter = apr_pstrcat(p, addfilter, suri, NULL);
		}
		break;
	}
	if (apr_strnatcasecmp(get_host_id(), "localhost")) {
		char *hst = apr_psprintf(p, " and hostnm = '%s' ",
				sql_adapter_get_host_name(p, db_type));
		addfilter = apr_pstrcat(p, addfilter, hst, NULL);
	}
	return addfilter;
}

char *
sql_adapter_get_limit(apr_pool_t * p, char *page_number, int per_page,
		int db_type) {

	char *addlimit_fmt = apr_pstrdup(p, "");
	char *addlimit = apr_pstrdup(p, "");

	int pg_num = 0;
	if (page_number) {
		pg_num = (int) apr_atoi64(page_number) - 1;
		if (pg_num < 0)
			pg_num = 0;
	}

	switch (db_type) {
	case 1:
		//SqLite
		addlimit_fmt = apr_pstrdup(p, "limit %d offset %d");
		addlimit = apr_psprintf(p, addlimit_fmt, per_page, pg_num * per_page);
		break;
	case 2:
		//MySQL
		addlimit_fmt = apr_pstrdup(p, "limit %d,%d");
		addlimit = apr_psprintf(p, addlimit_fmt, pg_num * per_page, per_page);
		break;
	case 3:
		//PgSql
		addlimit_fmt = apr_pstrdup(p, "limit %d offset %d");
		addlimit = apr_psprintf(p, addlimit_fmt, per_page, pg_num * per_page);
		break;
	}

	return addlimit;

}

char *
sql_adapter_get_full_text_info(
		apr_pool_t * p,
		int db_type,
		void *r,
		char *period,
		char *host,
		char *script,
		char *uri,
		char *period_begin,
		char *period_end,
		int sort,
		int tp,
		char *page_number,
		int per_page,
		void(*call_back_function1)(void *, char *, char *, char *, char *,
				char *, char *, char *, char *, char *, char *, char *, char *)) {
	if ((sort < 1) || (sort > 12))
		sort = 1;
	char *srt = apr_psprintf(p, "%d", sort);
	char *ttp = apr_psprintf(p, "%s", (tp ? "asc" : "desc"));
	char *sql =
			apr_pstrcat(
					p,
					"select id,dateadd,host,uri,script,cpu,memory,exc_time,cpu_sec, memory_mb, bytes_read, bytes_write from performance where %s %s order by ",
					srt, " ", ttp, " %s", NULL);

	char *string_sql = apr_psprintf(
			p,
			sql,
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL),
			sql_adapter_get_limit(p, page_number, per_page, db_type));

	switch (db_type) {
	case 1:
		//SqLite
	{
		int rv;
		sqlite3_stmt *stmt;
		rv = (*_sqlite3_prepare)(s_db_r, string_sql, strlen(string_sql), &stmt,
				0);
		if (rv) {
			return get_error_description(s_db_r, string_sql, p, db_type);
		} else {
			while (1) {
				rv = (*_sqlite3_step)(stmt);
				if (rv == SQLITE_ROW) {
					(*call_back_function1)(r,
							(char *) (*_sqlite3_column_text)(stmt, 0),
							(char *) (*_sqlite3_column_text)(stmt, 1),
							(char *) (*_sqlite3_column_text)(stmt, 2),
							(char *) (*_sqlite3_column_text)(stmt, 3),
							(char *) (*_sqlite3_column_text)(stmt, 4),
							(char *) (*_sqlite3_column_text)(stmt, 5),
							(char *) (*_sqlite3_column_text)(stmt, 6),
							(char *) (*_sqlite3_column_text)(stmt, 7),
							(char *) (*_sqlite3_column_text)(stmt, 8),
							(char *) (*_sqlite3_column_text)(stmt, 9),
							(char *) (*_sqlite3_column_text)(stmt, 10),
							(char *) (*_sqlite3_column_text)(stmt, 11));
				} else if (rv == SQLITE_DONE) {
					break;
				} else {
					(*_sqlite3_reset)(stmt);
					return apr_pstrdup(p, "Error while request processing");
					break;
				}
			}
			(*_sqlite3_reset)(stmt);
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db_r) {
			if ((*_mysql_query)(m_db_r, string_sql)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return get_error_description(m_db_r, string_sql, p, db_type);
			}
			MYSQL_RES *result = (*_mysql_store_result)(m_db_r);
			MYSQL_ROW row;
			while ((row = (*_mysql_fetch_row)(result))) {
				(*call_back_function1)(r, row[0], row[1], row[2], row[3],
						row[4], row[5], row[6], row[7], row[8], row[9], row[10],
						row[11]);
			}
			(*_mysql_free_result)(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if(p_db_r){
		if ((result = (*_PQexec)(p_db_r, string_sql)) == NULL) {
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);;
		}
		if ((*_PQresultStatus)(result) == PGRES_TUPLES_OK) {
			int i;
			for (i = 0; i < (*_PQntuples)(result); i++) {

				(*call_back_function1)(r, PG_GETVALUE (i, 0),
						PG_GETVALUE (i, 1), PG_GETVALUE (i,
								2), PG_GETVALUE (i, 3), PG_GETVALUE (i,
								4), PG_GETVALUE (i, 5), PG_GETVALUE (i,
								6), PG_GETVALUE (i, 7), PG_GETVALUE (i,
								8), PG_GETVALUE (i, 9), PG_GETVALUE (i,
								10), PG_GETVALUE (i, 11));
			}
		} else {
			PQClear(result);
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);;
		}
		PQClear(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;
}

char *
sql_adapter_get_full_text_info_count(apr_pool_t * p, int db_type, void *r,
		char *period, char *host, char *script, char *uri, char *period_begin,
		char *period_end, char *page_number, int per_page,
		void(*call_back_function2)(void *, int, char *)) {
	char *sql = apr_pstrdup(p, "select count(*) from performance where %s %s");

	char *string_sql = apr_psprintf(
			p,
			sql,
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL));
	switch (db_type) {
	case 1:
		//SqLite
	{
		int rv;
		sqlite3_stmt *stmt;
		rv = (*_sqlite3_prepare)(s_db_r, string_sql, strlen(string_sql), &stmt,
				0);
		if (rv) {
			return get_error_description(s_db_r, string_sql, p, db_type);
		} else {
			while (1) {
				rv = (*_sqlite3_step)(stmt);
				if (rv == SQLITE_ROW) {
					(*call_back_function2)(r,
							(int) (*_sqlite3_column_int)(stmt, 0), page_number);
				} else if (rv == SQLITE_DONE) {
					break;
				} else {
					(*_sqlite3_reset)(stmt);
					return apr_pstrdup(p, "Error while request processing");
					break;
				}
			}
			(*_sqlite3_reset)(stmt);
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db_r) {
			if ((*_mysql_query)(m_db_r, string_sql)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return get_error_description(m_db_r, string_sql, p, db_type);;
			}
			MYSQL_RES *result = (*_mysql_store_result)(m_db_r);
			MYSQL_ROW row;
			while ((row = (*_mysql_fetch_row)(result))) {
				(*call_back_function2)(r, (int) apr_atoi64(row[0]),
						page_number);
			}
			(*_mysql_free_result)(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if(p_db_r){
		if ((result = (*_PQexec)(p_db_r, string_sql)) == NULL) {
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);;
		}
		if ((*_PQresultStatus)(result) == PGRES_TUPLES_OK) {
			int i;
			for (i = 0; i < (*_PQntuples)(result); i++) {
				(*call_back_function2)(r, (int) apr_atoi64(PG_GETVALUE (i, 0)),
						page_number);
			}
		} else {
			PQClear(result);
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);;
		}
		PQClear(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;
}

char *
sql_adapter_get_full_text_info_picture(
		apr_pool_t * p,
		int db_type,
		void *r,
		char *period,
		char *host,
		char *script,
		char *uri,
		char *period_begin,
		char *period_end,
		char *page_number,
		int per_page,
		void(*call_back_function3)(void *, int, double, double, double, double,
				void *, int), void *ptr, int number) {
	char *sql = apr_pstrdup(p, "");
	switch (db_type) {
	case 1:
		//SqLite
		sql =
				apr_pstrdup(
						p,
						"select strftime('%%s',dateadd) as dt,max(cpu),max(memory), max(bytes_read), max(bytes_write) from performance where %s %s group by dt order by dt");
		break;
	case 2:
		//MySQL
		sql =
				apr_pstrdup(
						p,
						"select UNIX_TIMESTAMP(dateadd) as dt,max(cpu),max(memory), max(bytes_read), max(bytes_write) from performance where %s %s group by dt order by dt");
		break;
	case 3:
		//PgSql
		sql =
				apr_pstrdup(
						p,
						"select EXTRACT(EPOCH FROM dateadd at time zone 'UTC')::int as dt,max(cpu),max(memory), max(bytes_read), max(bytes_write) from performance where %s %s group by dt order by dt");

		break;
	}

	char *string_sql = apr_psprintf(
			p,
			sql,
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL));
	switch (db_type) {
	case 1:
		//SqLite
	{
		int rv;
		sqlite3_stmt *stmt;
		rv = (*_sqlite3_prepare)(s_db_r, string_sql, strlen(string_sql), &stmt,
				0);
		if (rv) {
			return get_error_description(s_db_r, string_sql, p, db_type);
		} else {
			while (1) {
				rv = (*_sqlite3_step)(stmt);
				if (rv == SQLITE_ROW) {
					(*call_back_function3)(r,
							(int) (*_sqlite3_column_int)(stmt, 0),
							(double) (*_sqlite3_column_double)(stmt, 1),
							(double) (*_sqlite3_column_double)(stmt, 2),
							(double) (*_sqlite3_column_double)(stmt, 3),
							(double) (*_sqlite3_column_double)(stmt, 4), ptr,
							number);
				} else if (rv == SQLITE_DONE) {
					break;
				} else {
					(*_sqlite3_reset)(stmt);
					return apr_pstrdup(p, "Error while request processing");
					break;
				}
			}
			(*_sqlite3_reset)(stmt);
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db_r) {
			if ((*_mysql_query)(m_db_r, string_sql)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return get_error_description(m_db_r, string_sql, p, db_type);;
			}
			MYSQL_RES *result = (*_mysql_store_result)(m_db_r);
			MYSQL_ROW row;
			while ((row = (*_mysql_fetch_row)(result))) {
				(*call_back_function3)(r, (int) apr_atoi64(row[0]),
						atof(row[1]), atof(row[2]), atof(row[3]), atof(row[4]),
						ptr, number);
			}
			(*_mysql_free_result)(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if(p_db_r){
		if ((result = (*_PQexec)(p_db_r, string_sql)) == NULL) {
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);;
		}
		if ((*_PQresultStatus)(result) == PGRES_TUPLES_OK) {
			int i;
			for (i = 0; i < (*_PQntuples)(result); i++) {

				(*call_back_function3)(r, (int) apr_atoi64(PG_GETVALUE (i, 0)),
						atof(PG_GETVALUE (i, 1)), atof(PG_GETVALUE (i, 2)),
						atof(PG_GETVALUE (i, 3)), atof(PG_GETVALUE (i, 4)), ptr,
						number);

			}
		} else {
			PQClear(result);
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);;
		}
		PQClear(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;
}

//------------------------------------Reports-------------------------------------

char *
sql_adapter_get_cpu_max_text_info(
		apr_pool_t * p,
		int db_type,
		void *r,
		char *period,
		char *host,
		char *script,
		char *uri,
		char *period_begin,
		char *period_end,
		char *page_number,
		int per_page,
		void(*call_back_function1)(void *, char *, char *, char *, char *,
				char *, char *, char *, char *, char *, char *, char *, char *)) {
	char *sql =
			apr_pstrdup(
					p,
					"select ttt2.* from (select tt1.id as id, tt1.host as host, count(*) as num from (select t2.id as id, t2.host as host from (select host,max(cpu) as mx from performance where %s %s group by host) as t1 join performance as t2 on t1.host=t2.host and t1.mx=t2.cpu and %s %s) as tt1 join (select t2.id as id, t2.host as host from (select host,max(cpu) as mx from performance where %s %s group by host) as t1 join performance as t2 on t1.host=t2.host and t1.mx=t2.cpu and %s %s) as tt2 on tt1.host = tt2.host and tt1.id >= tt2.id group by tt1.host, tt1.id having count(*)<=1) as ttt1 join performance as ttt2 on ttt1.id=ttt2.id");
	//"select id,dateadd,host,uri,script,cpu,memory,exc_time,cpu_sec, memory_mb, bytes_read, bytes_write  from performance where %s and cpu=(select max(cpu) from performance where %s %s) %s order by id desc");

	char *string_sql = apr_psprintf(
			p,
			sql,
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL),
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, "t2"),
			sql_adapter_get_filter(p, host, script, uri, db_type, "t2"),
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL),
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, "t2"),
			sql_adapter_get_filter(p, host, script, uri, db_type, "t2"));

	switch (db_type) {
	case 1:
		//SqLite
	{
		int rv;
		sqlite3_stmt *stmt;
		rv = (*_sqlite3_prepare)(s_db_r, string_sql, strlen(string_sql), &stmt,
				0);
		if (rv) {
			return get_error_description(s_db_r, string_sql, p, db_type);
		} else {
			while (1) {
				rv = (*_sqlite3_step)(stmt);
				if (rv == SQLITE_ROW) {
					(*call_back_function1)(r,
							(char *) (*_sqlite3_column_text)(stmt, 0),
							(char *) (*_sqlite3_column_text)(stmt, 1),
							(char *) (*_sqlite3_column_text)(stmt, 2),
							(char *) (*_sqlite3_column_text)(stmt, 3),
							(char *) (*_sqlite3_column_text)(stmt, 4),
							(char *) (*_sqlite3_column_text)(stmt, 5),
							(char *) (*_sqlite3_column_text)(stmt, 6),
							(char *) (*_sqlite3_column_text)(stmt, 7),
							(char *) (*_sqlite3_column_text)(stmt, 8),
							(char *) (*_sqlite3_column_text)(stmt, 9),
							(char *) (*_sqlite3_column_text)(stmt, 10),
							(char *) (*_sqlite3_column_text)(stmt, 11));
				} else if (rv == SQLITE_DONE) {
					break;
				} else {
					(*_sqlite3_reset)(stmt);
					return apr_pstrdup(p, "Error while request processing");
					break;
				}
			}
			(*_sqlite3_reset)(stmt);
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db_r) {
			if ((*_mysql_query)(m_db_r, string_sql)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return get_error_description(m_db_r, string_sql, p, db_type);;
			}
			MYSQL_RES *result = (*_mysql_store_result)(m_db_r);
			MYSQL_ROW row;
			while ((row = (*_mysql_fetch_row)(result))) {
				(*call_back_function1)(r, row[0], row[1], row[2], row[3],
						row[4], row[5], row[6], row[7], row[8], row[9], row[10],
						row[11]);
			}
			(*_mysql_free_result)(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if(p_db_r){
		if ((result = (*_PQexec)(p_db_r, string_sql)) == NULL) {
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);;
		}
		if ((*_PQresultStatus)(result) == PGRES_TUPLES_OK) {
			int i;
			for (i = 0; i < (*_PQntuples)(result); i++) {

				(*call_back_function1)(r, PG_GETVALUE (i, 0),
						PG_GETVALUE (i, 1), PG_GETVALUE (i,
								2), PG_GETVALUE (i, 3), PG_GETVALUE (i,
								4), PG_GETVALUE (i, 5), PG_GETVALUE (i,
								6), PG_GETVALUE (i, 7), PG_GETVALUE (i,
								8), PG_GETVALUE (i, 9), PG_GETVALUE (i,
								10), PG_GETVALUE (i, 11));
			}
		} else {
			PQClear(result);
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);;
		}
		PQClear(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;
}

char *
sql_adapter_get_cpu_max_text_info_no_hard(
		apr_pool_t * p,
		int db_type,
		void *r,
		char *period,
		char *host,
		char *script,
		char *uri,
		char *period_begin,
		char *period_end,
		char *page_number,
		int per_page,
		void(*call_back_function1)(void *, char *, char *, char *, char *,
				char *, char *, char *, char *, char *, char *, char *, char *)) {
	char *sql =
			apr_pstrdup(
					p,
					"select id,dateadd,host,uri,script,cpu,memory,exc_time,cpu_sec, memory_mb, bytes_read, bytes_write  from performance where %s and cpu=(select max(cpu) from performance where %s %s) %s order by id desc");

	char *string_sql = apr_psprintf(
			p,
			sql,
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL));

	switch (db_type) {
	case 1:
		//SqLite
	{
		int rv;
		sqlite3_stmt *stmt;
		rv = (*_sqlite3_prepare)(s_db_r, string_sql, strlen(string_sql), &stmt,
				0);
		if (rv) {
			return get_error_description(s_db_r, string_sql, p, db_type);
		} else {
			while (1) {
				rv = (*_sqlite3_step)(stmt);
				if (rv == SQLITE_ROW) {
					(*call_back_function1)(r,
							(char *) (*_sqlite3_column_text)(stmt, 0),
							(char *) (*_sqlite3_column_text)(stmt, 1),
							(char *) (*_sqlite3_column_text)(stmt, 2),
							(char *) (*_sqlite3_column_text)(stmt, 3),
							(char *) (*_sqlite3_column_text)(stmt, 4),
							(char *) (*_sqlite3_column_text)(stmt, 5),
							(char *) (*_sqlite3_column_text)(stmt, 6),
							(char *) (*_sqlite3_column_text)(stmt, 7),
							(char *) (*_sqlite3_column_text)(stmt, 8),
							(char *) (*_sqlite3_column_text)(stmt, 9),
							(char *) (*_sqlite3_column_text)(stmt, 10),
							(char *) (*_sqlite3_column_text)(stmt, 11));
				} else if (rv == SQLITE_DONE) {
					break;
				} else {
					(*_sqlite3_reset)(stmt);
					return apr_pstrdup(p, "Error while request processing");
					break;
				}
			}
			(*_sqlite3_reset)(stmt);
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db_r) {
			if ((*_mysql_query)(m_db_r, string_sql)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return get_error_description(m_db_r, string_sql, p, db_type);;
			}
			MYSQL_RES *result = (*_mysql_store_result)(m_db_r);
			MYSQL_ROW row;
			while ((row = (*_mysql_fetch_row)(result))) {
				(*call_back_function1)(r, row[0], row[1], row[2], row[3],
						row[4], row[5], row[6], row[7], row[8], row[9], row[10],
						row[11]);
			}
			(*_mysql_free_result)(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if(p_db_r){
		if ((result = (*_PQexec)(p_db_r, string_sql)) == NULL) {
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);;
		}
		if ((*_PQresultStatus)(result) == PGRES_TUPLES_OK) {
			int i;
			for (i = 0; i < (*_PQntuples)(result); i++) {

				(*call_back_function1)(r, PG_GETVALUE (i, 0),
						PG_GETVALUE (i, 1), PG_GETVALUE (i,
								2), PG_GETVALUE (i, 3), PG_GETVALUE (i,
								4), PG_GETVALUE (i, 5), PG_GETVALUE (i,
								6), PG_GETVALUE (i, 7), PG_GETVALUE (i,
								8), PG_GETVALUE (i, 9), PG_GETVALUE (i,
								10), PG_GETVALUE (i, 11));
			}
		} else {
			PQClear(result);
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);;
		}
		PQClear(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;
}

char *
sql_adapter_get_mem_max_text_info(
		apr_pool_t * p,
		int db_type,
		void *r,
		char *period,
		char *host,
		char *script,
		char *uri,
		char *period_begin,
		char *period_end,
		char *page_number,
		int per_page,
		void(*call_back_function1)(void *, char *, char *, char *, char *,
				char *, char *, char *, char *, char *, char *, char *, char *)) {
	char *sql =
			apr_pstrdup(
					p,
					"select ttt2.* from (select tt1.id as id, tt1.host as host, count(*) as num from (select t2.id as id, t2.host as host from (select host,max(memory) as mx from performance where %s %s group by host) as t1 join performance as t2 on t1.host=t2.host and t1.mx=t2.memory and %s %s) as tt1 join (select t2.id as id, t2.host as host from (select host,max(memory) as mx from performance where %s %s group by host) as t1 join performance as t2 on t1.host=t2.host and t1.mx=t2.memory and %s %s) as tt2 on tt1.host = tt2.host and tt1.id >= tt2.id group by tt1.host, tt1.id having count(*)<=1) as ttt1 join performance as ttt2 on ttt1.id=ttt2.id");
	//"select id,dateadd,host,uri,script,cpu,memory,exc_time,cpu_sec, memory_mb, bytes_read, bytes_write  from performance where %s and memory=(select max(memory) from performance where %s %s) %s order by id desc");

	char *string_sql = apr_psprintf(
			p,
			sql,
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL),
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, "t2"),
			sql_adapter_get_filter(p, host, script, uri, db_type, "t2"),
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL),
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, "t2"),
			sql_adapter_get_filter(p, host, script, uri, db_type, "t2"));

	switch (db_type) {
	case 1:
		//SqLite
	{
		int rv;
		sqlite3_stmt *stmt;
		rv = (*_sqlite3_prepare)(s_db_r, string_sql, strlen(string_sql), &stmt,
				0);
		if (rv) {
			return get_error_description(s_db_r, string_sql, p, db_type);
		} else {
			while (1) {
				rv = (*_sqlite3_step)(stmt);
				if (rv == SQLITE_ROW) {
					(*call_back_function1)(r,
							(char *) (*_sqlite3_column_text)(stmt, 0),
							(char *) (*_sqlite3_column_text)(stmt, 1),
							(char *) (*_sqlite3_column_text)(stmt, 2),
							(char *) (*_sqlite3_column_text)(stmt, 3),
							(char *) (*_sqlite3_column_text)(stmt, 4),
							(char *) (*_sqlite3_column_text)(stmt, 5),
							(char *) (*_sqlite3_column_text)(stmt, 6),
							(char *) (*_sqlite3_column_text)(stmt, 7),
							(char *) (*_sqlite3_column_text)(stmt, 8),
							(char *) (*_sqlite3_column_text)(stmt, 9),
							(char *) (*_sqlite3_column_text)(stmt, 10),
							(char *) (*_sqlite3_column_text)(stmt, 11));
				} else if (rv == SQLITE_DONE) {
					break;
				} else {
					(*_sqlite3_reset)(stmt);
					return apr_pstrdup(p, "Error while request processing");
					break;
				}
			}
			(*_sqlite3_reset)(stmt);
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db_r) {
			if ((*_mysql_query)(m_db_r, string_sql)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return get_error_description(m_db_r, string_sql, p, db_type);
			}
			MYSQL_RES *result = (*_mysql_store_result)(m_db_r);
			MYSQL_ROW row;
			while ((row = (*_mysql_fetch_row)(result))) {
				(*call_back_function1)(r, row[0], row[1], row[2], row[3],
						row[4], row[5], row[6], row[7], row[8], row[9], row[10],
						row[11]);
			}
			(*_mysql_free_result)(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db_r);
		pthread_rwlock_unlock(&db_lock_rw);
		if(p_db_r){
		if ((result = (*_PQexec)(p_db_r, string_sql)) == NULL) {
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		if ((*_PQresultStatus)(result) == PGRES_TUPLES_OK) {
			int i;
			for (i = 0; i < (*_PQntuples)(result); i++) {

				(*call_back_function1)(r, PG_GETVALUE (i, 0),
						PG_GETVALUE (i, 1), PG_GETVALUE (i,
								2), PG_GETVALUE (i, 3), PG_GETVALUE (i,
								4), PG_GETVALUE (i, 5), PG_GETVALUE (i,
								6), PG_GETVALUE (i, 7), PG_GETVALUE (i,
								8), PG_GETVALUE (i, 9), PG_GETVALUE (i,
								10), PG_GETVALUE (i, 11));
			}
		} else {
			PQClear(result);
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		PQClear(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;
}

char *
sql_adapter_get_mem_max_text_info_no_hard(
		apr_pool_t * p,
		int db_type,
		void *r,
		char *period,
		char *host,
		char *script,
		char *uri,
		char *period_begin,
		char *period_end,
		char *page_number,
		int per_page,
		void(*call_back_function1)(void *, char *, char *, char *, char *,
				char *, char *, char *, char *, char *, char *, char *, char *)) {
	char *sql =
			apr_pstrdup(
					p,
					"select id,dateadd,host,uri,script,cpu,memory,exc_time,cpu_sec, memory_mb, bytes_read, bytes_write  from performance where %s and memory=(select max(memory) from performance where %s %s) %s order by id desc");

	char *string_sql = apr_psprintf(
			p,
			sql,
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL));

	switch (db_type) {
	case 1:
		//SqLite
	{
		int rv;
		sqlite3_stmt *stmt;
		rv = (*_sqlite3_prepare)(s_db_r, string_sql, strlen(string_sql), &stmt,
				0);
		if (rv) {
			return get_error_description(s_db_r, string_sql, p, db_type);
		} else {
			while (1) {
				rv = (*_sqlite3_step)(stmt);
				if (rv == SQLITE_ROW) {
					(*call_back_function1)(r,
							(char *) (*_sqlite3_column_text)(stmt, 0),
							(char *) (*_sqlite3_column_text)(stmt, 1),
							(char *) (*_sqlite3_column_text)(stmt, 2),
							(char *) (*_sqlite3_column_text)(stmt, 3),
							(char *) (*_sqlite3_column_text)(stmt, 4),
							(char *) (*_sqlite3_column_text)(stmt, 5),
							(char *) (*_sqlite3_column_text)(stmt, 6),
							(char *) (*_sqlite3_column_text)(stmt, 7),
							(char *) (*_sqlite3_column_text)(stmt, 8),
							(char *) (*_sqlite3_column_text)(stmt, 9),
							(char *) (*_sqlite3_column_text)(stmt, 10),
							(char *) (*_sqlite3_column_text)(stmt, 11));
				} else if (rv == SQLITE_DONE) {
					break;
				} else {
					(*_sqlite3_reset)(stmt);
					return apr_pstrdup(p, "Error while request processing");
					break;
				}
			}
			(*_sqlite3_reset)(stmt);
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db_r) {
			if ((*_mysql_query)(m_db_r, string_sql)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return get_error_description(m_db_r, string_sql, p, db_type);
			}
			MYSQL_RES *result = (*_mysql_store_result)(m_db_r);
			MYSQL_ROW row;
			while ((row = (*_mysql_fetch_row)(result))) {
				(*call_back_function1)(r, row[0], row[1], row[2], row[3],
						row[4], row[5], row[6], row[7], row[8], row[9], row[10],
						row[11]);
			}
			(*_mysql_free_result)(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if(p_db_r){
		if ((result = (*_PQexec)(p_db_r, string_sql)) == NULL) {
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		if ((*_PQresultStatus)(result) == PGRES_TUPLES_OK) {
			int i;
			for (i = 0; i < (*_PQntuples)(result); i++) {

				(*call_back_function1)(r, PG_GETVALUE (i, 0),
						PG_GETVALUE (i, 1), PG_GETVALUE (i,
								2), PG_GETVALUE (i, 3), PG_GETVALUE (i,
								4), PG_GETVALUE (i, 5), PG_GETVALUE (i,
								6), PG_GETVALUE (i, 7), PG_GETVALUE (i,
								8), PG_GETVALUE (i, 9), PG_GETVALUE (i,
								10), PG_GETVALUE (i, 11));
			}
		} else {
			PQClear(result);
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		PQClear(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;
}

char *
sql_adapter_get_time_max_text_info(
		apr_pool_t * p,
		int db_type,
		void *r,
		char *period,
		char *host,
		char *script,
		char *uri,
		char *period_begin,
		char *period_end,
		char *page_number,
		int per_page,
		void(*call_back_function1)(void *, char *, char *, char *, char *,
				char *, char *, char *, char *, char *, char *, char *, char *)) {
	char *sql =
			apr_pstrdup(
					p,
					"select ttt2.* from (select tt1.id as id, tt1.host as host, count(*) as num from (select t2.id as id, t2.host as host from (select host,max(exc_time) as mx from performance where %s %s group by host) as t1 join performance as t2 on t1.host=t2.host and t1.mx=t2.exc_time and %s %s) as tt1 join (select t2.id as id, t2.host as host from (select host,max(exc_time) as mx from performance where %s %s group by host) as t1 join performance as t2 on t1.host=t2.host and t1.mx=t2.exc_time and %s %s) as tt2 on tt1.host = tt2.host and tt1.id >= tt2.id group by tt1.host, tt1.id having count(*)<=1) as ttt1 join performance as ttt2 on ttt1.id=ttt2.id");
	//"select id,dateadd,host,uri,script,cpu,memory,exc_time,cpu_sec, memory_mb, bytes_read, bytes_write from performance where %s and exc_time=(select max(exc_time) from performance where %s %s) %s order by id desc");

	char *string_sql = apr_psprintf(
			p,
			sql,
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL),
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, "t2"),
			sql_adapter_get_filter(p, host, script, uri, db_type, "t2"),
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL),
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, "t2"),
			sql_adapter_get_filter(p, host, script, uri, db_type, "t2"));

	switch (db_type) {
	case 1:
		//SqLite
	{
		int rv;
		sqlite3_stmt *stmt;
		rv = (*_sqlite3_prepare)(s_db_r, string_sql, strlen(string_sql), &stmt,
				0);
		if (rv) {
			return get_error_description(s_db_r, string_sql, p, db_type);
		} else {
			while (1) {
				rv = (*_sqlite3_step)(stmt);
				if (rv == SQLITE_ROW) {
					(*call_back_function1)(r,
							(char *) (*_sqlite3_column_text)(stmt, 0),
							(char *) (*_sqlite3_column_text)(stmt, 1),
							(char *) (*_sqlite3_column_text)(stmt, 2),
							(char *) (*_sqlite3_column_text)(stmt, 3),
							(char *) (*_sqlite3_column_text)(stmt, 4),
							(char *) (*_sqlite3_column_text)(stmt, 5),
							(char *) (*_sqlite3_column_text)(stmt, 6),
							(char *) (*_sqlite3_column_text)(stmt, 7),
							(char *) (*_sqlite3_column_text)(stmt, 8),
							(char *) (*_sqlite3_column_text)(stmt, 9),
							(char *) (*_sqlite3_column_text)(stmt, 10),
							(char *) (*_sqlite3_column_text)(stmt, 11));
				} else if (rv == SQLITE_DONE) {
					break;
				} else {
					(*_sqlite3_reset)(stmt);
					return apr_pstrdup(p, "Error while request processing");
					break;
				}
			}
			(*_sqlite3_reset)(stmt);
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db_r) {
			if ((*_mysql_query)(m_db_r, string_sql)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return get_error_description(m_db_r, string_sql, p, db_type);
			}
			MYSQL_RES *result = (*_mysql_store_result)(m_db_r);
			MYSQL_ROW row;
			while ((row = (*_mysql_fetch_row)(result))) {
				(*call_back_function1)(r, row[0], row[1], row[2], row[3],
						row[4], row[5], row[6], row[7], row[8], row[9], row[10],
						row[11]);
			}
			(*_mysql_free_result)(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if(p_db_r){
		if ((result = (*_PQexec)(p_db_r, string_sql)) == NULL) {
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		if ((*_PQresultStatus)(result) == PGRES_TUPLES_OK) {
			int i;
			for (i = 0; i < (*_PQntuples)(result); i++) {

				(*call_back_function1)(r, PG_GETVALUE (i, 0),
						PG_GETVALUE (i, 1), PG_GETVALUE (i,
								2), PG_GETVALUE (i, 3), PG_GETVALUE (i,
								4), PG_GETVALUE (i, 5), PG_GETVALUE (i,
								6), PG_GETVALUE (i, 7), PG_GETVALUE (i,
								8), PG_GETVALUE (i, 9), PG_GETVALUE (i,
								10), PG_GETVALUE (i, 11));
			}
		} else {
			PQClear(result);
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		PQClear(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;
}

char *
sql_adapter_get_time_max_text_info_no_hard(
		apr_pool_t * p,
		int db_type,
		void *r,
		char *period,
		char *host,
		char *script,
		char *uri,
		char *period_begin,
		char *period_end,
		char *page_number,
		int per_page,
		void(*call_back_function1)(void *, char *, char *, char *, char *,
				char *, char *, char *, char *, char *, char *, char *, char *)) {
	char *sql =
			apr_pstrdup(
					p,
					"select id,dateadd,host,uri,script,cpu,memory,exc_time,cpu_sec, memory_mb, bytes_read, bytes_write from performance where %s and exc_time=(select max(exc_time) from performance where %s %s) %s order by id desc");

	char *string_sql = apr_psprintf(
			p,
			sql,
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL));

	switch (db_type) {
	case 1:
		//SqLite
	{
		int rv;
		sqlite3_stmt *stmt;
		rv = (*_sqlite3_prepare)(s_db_r, string_sql, strlen(string_sql), &stmt,
				0);
		if (rv) {
			return get_error_description(s_db_r, string_sql, p, db_type);
		} else {
			while (1) {
				rv = (*_sqlite3_step)(stmt);
				if (rv == SQLITE_ROW) {
					(*call_back_function1)(r,
							(char *) (*_sqlite3_column_text)(stmt, 0),
							(char *) (*_sqlite3_column_text)(stmt, 1),
							(char *) (*_sqlite3_column_text)(stmt, 2),
							(char *) (*_sqlite3_column_text)(stmt, 3),
							(char *) (*_sqlite3_column_text)(stmt, 4),
							(char *) (*_sqlite3_column_text)(stmt, 5),
							(char *) (*_sqlite3_column_text)(stmt, 6),
							(char *) (*_sqlite3_column_text)(stmt, 7),
							(char *) (*_sqlite3_column_text)(stmt, 8),
							(char *) (*_sqlite3_column_text)(stmt, 9),
							(char *) (*_sqlite3_column_text)(stmt, 10),
							(char *) (*_sqlite3_column_text)(stmt, 11));
				} else if (rv == SQLITE_DONE) {
					break;
				} else {
					(*_sqlite3_reset)(stmt);
					return apr_pstrdup(p, "Error while request processing");
					break;
				}
			}
			(*_sqlite3_reset)(stmt);
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db_r) {
			if ((*_mysql_query)(m_db_r, string_sql)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return get_error_description(m_db_r, string_sql, p, db_type);
			}
			MYSQL_RES *result = (*_mysql_store_result)(m_db_r);
			MYSQL_ROW row;
			while ((row = (*_mysql_fetch_row)(result))) {
				(*call_back_function1)(r, row[0], row[1], row[2], row[3],
						row[4], row[5], row[6], row[7], row[8], row[9], row[10],
						row[11]);
			}
			(*_mysql_free_result)(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if(p_db_r){
		if ((result = (*_PQexec)(p_db_r, string_sql)) == NULL) {
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		if ((*_PQresultStatus)(result) == PGRES_TUPLES_OK) {
			int i;
			for (i = 0; i < (*_PQntuples)(result); i++) {

				(*call_back_function1)(r, PG_GETVALUE (i, 0),
						PG_GETVALUE (i, 1), PG_GETVALUE (i,
								2), PG_GETVALUE (i, 3), PG_GETVALUE (i,
								4), PG_GETVALUE (i, 5), PG_GETVALUE (i,
								6), PG_GETVALUE (i, 7), PG_GETVALUE (i,
								8), PG_GETVALUE (i, 9), PG_GETVALUE (i,
								10), PG_GETVALUE (i, 11));
			}
		} else {
			PQClear(result);
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		PQClear(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;
}

char *
sql_adapter_get_host_text_info(apr_pool_t * p, int db_type, void *r,
		char *period, char *host, char *script, char *uri, char *period_begin,
		char *period_end, int sort, int tp, char *page_number, int per_page,
		void(*call_back_function4)(void *, char *, char *, char *)) {

	if ((tp < 0) || (tp > 1))
		sort = 0;
	char *srt = apr_psprintf(p, "%s", (tp ? "asc" : "desc"));
	char *sql =
			apr_pstrcat(
					p,
					"select host, count(*)*100.0/(select count(*) from performance where %s %s) as prc, count(*) from performance where %s %s group by host order by 2 ",
					srt, NULL);

	char *string_sql = apr_psprintf(
			p,
			sql,
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL),
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL));

	switch (db_type) {
	case 1:
		//SqLite
	{
		int rv;
		sqlite3_stmt *stmt;
		rv = (*_sqlite3_prepare)(s_db_r, string_sql, strlen(string_sql), &stmt,
				0);
		if (rv) {
			return get_error_description(s_db_r, string_sql, p, db_type);
		} else {
			while (1) {
				rv = (*_sqlite3_step)(stmt);
				if (rv == SQLITE_ROW) {
					(*call_back_function4)(r,
							(char *) (*_sqlite3_column_text)(stmt, 0),
							(char *) (*_sqlite3_column_text)(stmt, 1),
							(char *) (*_sqlite3_column_text)(stmt, 2));
				} else if (rv == SQLITE_DONE) {
					break;
				} else {
					(*_sqlite3_reset)(stmt);
					return apr_pstrdup(p, "Error while request processing");
					break;
				}
			}
			(*_sqlite3_reset)(stmt);
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db_r) {
			if ((*_mysql_query)(m_db_r, string_sql)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return get_error_description(m_db_r, string_sql, p, db_type);
			}
			MYSQL_RES *result = (*_mysql_store_result)(m_db_r);
			MYSQL_ROW row;
			while ((row = (*_mysql_fetch_row)(result))) {
				(*call_back_function4)(r, row[0], row[1], row[2]);
			}
			(*_mysql_free_result)(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if(p_db_r){
		if ((result = (*_PQexec)(p_db_r, string_sql)) == NULL) {
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		if ((*_PQresultStatus)(result) == PGRES_TUPLES_OK) {
			int i;
			for (i = 0; i < (*_PQntuples)(result); i++) {
				(*call_back_function4)(r, PG_GETVALUE (i, 0),
						PG_GETVALUE (i, 1), PG_GETVALUE (i,
								2));

			}
		} else {
			PQClear(result);
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		PQClear(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;
}

char *
sql_adapter_get_host_text_info_picture(apr_pool_t * p, int db_type, void *r,
		char *period, char *host, char *script, char *uri, char *period_begin,
		char *period_end, int sort, char *page_number, int per_page,
		void(*call_back_function4_data)(void *, char *, double, void *),
		void *ptr) {

	if ((sort < 1) || (sort > 3))
		sort = 1;
	char *srt = apr_psprintf(p, "%d", sort);
	char *sql =
			apr_pstrcat(
					p,
					"select host, count(*)*100.0/(select count(*) from performance where %s %s) as prc from performance where %s %s group by host order by ",
					srt, " desc", NULL);

	char *string_sql = apr_psprintf(
			p,
			sql,
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL),
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL));

	switch (db_type) {
	case 1:
		//SqLite
	{
		int rv;
		sqlite3_stmt *stmt;
		rv = (*_sqlite3_prepare)(s_db_r, string_sql, strlen(string_sql), &stmt,
				0);
		if (rv) {
			return get_error_description(s_db_r, string_sql, p, db_type);
		} else {
			while (1) {
				rv = (*_sqlite3_step)(stmt);
				if (rv == SQLITE_ROW) {
					(*call_back_function4_data)(r,
							(char *) (*_sqlite3_column_text)(stmt, 0),
							(double) (*_sqlite3_column_double)(stmt, 1), ptr);
				} else if (rv == SQLITE_DONE) {
					break;
				} else {
					(*_sqlite3_reset)(stmt);
					return apr_pstrdup(p, "Error while request processing");
					break;
				}
			}
			(*_sqlite3_reset)(stmt);
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db_r) {
			if ((*_mysql_query)(m_db_r, string_sql)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return get_error_description(m_db_r, string_sql, p, db_type);
			}
			MYSQL_RES *result = (*_mysql_store_result)(m_db_r);
			MYSQL_ROW row;
			while ((row = (*_mysql_fetch_row)(result))) {
				(*call_back_function4_data)(r, row[0], atof(row[1]), ptr);
			}
			(*_mysql_free_result)(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if(p_db_r){
		if ((result = (*_PQexec)(p_db_r, string_sql)) == NULL) {
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		if ((*_PQresultStatus)(result) == PGRES_TUPLES_OK) {
			int i;
			for (i = 0; i < (*_PQntuples)(result); i++) {
				(*call_back_function4_data)(r, PG_GETVALUE (i, 0),
						atof(PG_GETVALUE (i, 1)), ptr);

			}
		} else {
			PQClear(result);
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		PQClear(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;
}

char *
sql_adapter_get_avg_text_info(
		apr_pool_t * p,
		int db_type,
		void *r,
		char *period,
		char *host,
		char *script,
		char *uri,
		char *period_begin,
		char *period_end,
		int sort,
		int tp,
		char *page_number,
		int per_page,
		void(*call_back_function5)(void *, char *, char *, char *, char *,
				char *, char *)) {
	if ((sort < 1) || (sort > 6))
		sort = 1;
	char *srt = apr_psprintf(p, "%d", sort);
	char *ttp = apr_psprintf(p, "%s", (tp ? "asc" : "desc"));
	char *sql =
			apr_pstrcat(
					p,
					"select host,sum(cpu)/count(*),sum(memory)/count(*),sum(exc_time)/count(*), sum(bytes_read)/count(*), sum(bytes_write)/count(*) from performance where %s %s group by host order by ",
					srt, " ", ttp, NULL);

	char *string_sql = apr_psprintf(
			p,
			sql,
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL));
	switch (db_type) {
	case 1:
		//SqLite
	{
		int rv;
		sqlite3_stmt *stmt;
		rv = (*_sqlite3_prepare)(s_db_r, string_sql, strlen(string_sql), &stmt,
				0);
		if (rv) {
			return get_error_description(s_db_r, string_sql, p, db_type);
		} else {
			while (1) {
				rv = (*_sqlite3_step)(stmt);
				if (rv == SQLITE_ROW) {
					(*call_back_function5)(r,
							(char *) (*_sqlite3_column_text)(stmt, 0),
							(char *) (*_sqlite3_column_text)(stmt, 1),
							(char *) (*_sqlite3_column_text)(stmt, 2),
							(char *) (*_sqlite3_column_text)(stmt, 3),
							(char *) (*_sqlite3_column_text)(stmt, 4),
							(char *) (*_sqlite3_column_text)(stmt, 5));
				} else if (rv == SQLITE_DONE) {
					break;
				} else {
					(*_sqlite3_reset)(stmt);
					return apr_pstrdup(p, "Error while request processing");
					break;
				}
			}
			(*_sqlite3_reset)(stmt);
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db_r) {
			if ((*_mysql_query)(m_db_r, string_sql)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return get_error_description(m_db_r, string_sql, p, db_type);
			}
			MYSQL_RES *result = (*_mysql_store_result)(m_db_r);
			MYSQL_ROW row;
			while ((row = (*_mysql_fetch_row)(result))) {
				(*call_back_function5)(r, row[0], row[1], row[2], row[3],
						row[4], row[5]);
			}
			(*_mysql_free_result)(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if(p_db_r){
		if ((result = (*_PQexec)(p_db_r, string_sql)) == NULL) {
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		if ((*_PQresultStatus)(result) == PGRES_TUPLES_OK) {
			int i;
			for (i = 0; i < (*_PQntuples)(result); i++) {

				(*call_back_function5)(r, PG_GETVALUE (i, 0),
						PG_GETVALUE (i, 1), PG_GETVALUE (i,
								2), PG_GETVALUE (i, 3), PG_GETVALUE (i,
								4), PG_GETVALUE (i, 5));
			}
		} else {
			PQClear(result);
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		PQClear(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;
}

char *
sql_adapter_get_avg_text_info_picture(
		apr_pool_t * p,
		int db_type,
		void *r,
		char *period,
		char *host,
		char *script,
		char *uri,
		char *period_begin,
		char *period_end,
		int sort,
		char *page_number,
		int per_page,
		void(*call_back_function5_1)(void *, char *, double, double, double,
				double, double, void *, int), void *ptr, int number) {
	if ((sort < 1) || (sort > 6))
		sort = 1;
	char *srt = apr_psprintf(p, "%d", sort);
	char *sql =
			apr_pstrcat(
					p,
					"select host,sum(cpu)/count(*),sum(memory)/count(*),sum(exc_time)/count(*), sum(bytes_read)/count(*), sum(bytes_write)/count(*) from performance where %s %s group by host order by ",
					srt, NULL);

	char *string_sql = apr_psprintf(
			p,
			sql,
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL));
	switch (db_type) {
	case 1:
		//SqLite
	{
		int rv;
		sqlite3_stmt *stmt;
		rv = (*_sqlite3_prepare)(s_db_r, string_sql, strlen(string_sql), &stmt,
				0);
		if (rv) {
			return get_error_description(s_db_r, string_sql, p, db_type);
		} else {
			while (1) {
				rv = (*_sqlite3_step)(stmt);
				if (rv == SQLITE_ROW) {
					(*call_back_function5_1)(r,
							(char *) (*_sqlite3_column_text)(stmt, 0),
							(double) (*_sqlite3_column_double)(stmt, 1),
							(double) (*_sqlite3_column_double)(stmt, 2),
							(double) (*_sqlite3_column_double)(stmt, 3),
							(double) (*_sqlite3_column_double)(stmt, 4),
							(double) (*_sqlite3_column_double)(stmt, 5), ptr,
							number);
				} else if (rv == SQLITE_DONE) {
					break;
				} else {
					(*_sqlite3_reset)(stmt);
					return apr_pstrdup(p, "Error while request processing");
					break;
				}
			}
			(*_sqlite3_reset)(stmt);
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db_r) {
			if ((*_mysql_query)(m_db_r, string_sql)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return get_error_description(m_db_r, string_sql, p, db_type);
			}
			MYSQL_RES *result = (*_mysql_store_result)(m_db_r);
			MYSQL_ROW row;
			while ((row = (*_mysql_fetch_row)(result))) {
				(*call_back_function5_1)(r, row[0], atof(row[1]), atof(row[2]),
						atof(row[3]), atof(row[4]), atof(row[5]), ptr, number);
			}
			(*_mysql_free_result)(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if(p_db_r){
		if ((result = (*_PQexec)(p_db_r, string_sql)) == NULL) {
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		if ((*_PQresultStatus)(result) == PGRES_TUPLES_OK) {
			int i;
			for (i = 0; i < (*_PQntuples)(result); i++) {

				(*call_back_function5_1)(r, PG_GETVALUE (i, 0),
						atof(PG_GETVALUE (i, 1)), atof(PG_GETVALUE (i, 2)),
						atof(PG_GETVALUE (i, 3)), atof(PG_GETVALUE (i, 4)),
						atof(PG_GETVALUE (i, 5)), ptr, number);
			}
		} else {
			PQClear(result);
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		PQClear(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;
}

char *
sql_adapter_optimize_table(apr_pool_t * p, int db_type,
		apr_thread_mutex_t * mutex_db) {
	switch (db_type) {
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db) {
			char *sql_buffer;
			sql_buffer = apr_psprintf(p, "OPTIMIZE TABLE performance");
			apr_thread_mutex_lock(mutex_db);
			if ((*_mysql_query)(m_db, sql_buffer)) {
				char *errmsg = apr_pstrdup(p, (*_mysql_error)(m_db));
				apr_thread_mutex_unlock(mutex_db);
				pthread_rwlock_unlock(&db_lock_rw);
				return errmsg;
			}
			apr_thread_mutex_unlock(mutex_db);
			pthread_rwlock_unlock(&db_lock_rw);
			return NULL;
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;
}

char *
sql_adapter_get_exec_tm(
		apr_pool_t * p,
		int db_type,
		void *r,
		char *period,
		char *host,
		char *script,
		char *uri,
		char *period_begin,
		char *period_end,
		char *page_number,
		int per_page,
		void(*call_back_function6)(void *, char *, char *, char *, char *,
				char *, char *, char *, char *, char *)) {
	char *sql =
			apr_pstrdup(
					p,
					"select host,case when exc_time = 0 then '0' when exc_time > 0 and exc_time <= 0.1 then '0-0.1' when exc_time > 0.1 and exc_time <= 0.3 then '0.1-0.3' when exc_time > 0.3 and exc_time <= 0.6 then '0.3-0.6'  when exc_time > 0.6 and exc_time <= 1 then '0.6-1' when exc_time > 1 and exc_time <= 3 then '1-3' when exc_time > 3 and exc_time <= 10 then '3-10' when exc_time > 10 then '>10' end as b1, count(*), min(cpu), max(cpu), sum(cpu)/count(*), min(memory), max(memory), sum(memory)/count(*) from performance where %s %s group by b1, host union select host,host as b1,NULL, NULL, NULL, NULL, NULL, NULL, NULL from performance where %s %s order by host, b1 desc");

	char *string_sql = apr_psprintf(
			p,
			sql,
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL),
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL));
	switch (db_type) {
	case 1:
		//SqLite
	{
		int rv;
		sqlite3_stmt *stmt;
		rv = (*_sqlite3_prepare)(s_db_r, string_sql, strlen(string_sql), &stmt,
				0);
		if (rv) {
			return get_error_description(s_db_r, string_sql, p, db_type);
		} else {
			while (1) {
				rv = (*_sqlite3_step)(stmt);
				if (rv == SQLITE_ROW) {
					(*call_back_function6)(r,
							(char *) (*_sqlite3_column_text)(stmt, 0),
							(char *) (*_sqlite3_column_text)(stmt, 1),
							(char *) (*_sqlite3_column_text)(stmt, 2),
							(char *) (*_sqlite3_column_text)(stmt, 3),
							(char *) (*_sqlite3_column_text)(stmt, 4),
							(char *) (*_sqlite3_column_text)(stmt, 5),
							(char *) (*_sqlite3_column_text)(stmt, 6),
							(char *) (*_sqlite3_column_text)(stmt, 7),
							(char *) (*_sqlite3_column_text)(stmt, 8));
				} else if (rv == SQLITE_DONE) {
					break;
				} else {
					(*_sqlite3_reset)(stmt);
					return apr_pstrdup(p, "Error while request processing");
					break;
				}
			}
			(*_sqlite3_reset)(stmt);
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db_r) {
			if ((*_mysql_query)(m_db_r, string_sql)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return get_error_description(m_db_r, string_sql, p, db_type);
			}
			MYSQL_RES *result = (*_mysql_store_result)(m_db_r);
			MYSQL_ROW row;
			while ((row = (*_mysql_fetch_row)(result))) {
				(*call_back_function6)(r, row[0], row[1], row[2], row[3],
						row[4], row[5], row[6], row[7], row[8]);
			}
			(*_mysql_free_result)(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if(p_db_r){
		if ((result = (*_PQexec)(p_db_r, string_sql)) == NULL) {
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		if ((*_PQresultStatus)(result) == PGRES_TUPLES_OK) {
			int i;
			for (i = 0; i < (*_PQntuples)(result); i++) {

				(*call_back_function6)(r, PG_GETVALUE (i, 0),
						PG_GETVALUE (i, 1), PG_GETVALUE (i,
								2), PG_GETVALUE (i, 3), PG_GETVALUE (i,
								4), PG_GETVALUE (i, 5), PG_GETVALUE (i,
								6), PG_GETVALUE (i, 7), PG_GETVALUE (i,
								8));
			}
		} else {
			PQClear(result);
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		PQClear(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;

}

char *
sql_adapter_get_exec_tm_common(
		apr_pool_t * p,
		int db_type,
		void *r,
		char *period,
		char *host,
		char *script,
		char *uri,
		char *period_begin,
		char *period_end,
		char *page_number,
		int per_page,
		void(*call_back_function6)(void *, char *, char *, char *, char *,
				char *, char *, char *, char *, char *)) {
	char *sql =
			apr_pstrdup(
					p,
					"select NULL, case when exc_time = 0 then '0' when exc_time > 0 and exc_time <= 0.1 then '0-0.1' when exc_time > 0.1 and exc_time <= 0.3 then '0.1-0.3' when exc_time > 0.3 and exc_time <= 0.6 then '0.3-0.6'  when exc_time > 0.6 and exc_time <= 1 then '0.6-1' when exc_time > 1 and exc_time <= 3 then '1-3' when exc_time > 3 and exc_time <= 10 then '3-10' when exc_time > 10 then '>10' end as b1, count(*), min(cpu), max(cpu), sum(cpu)/count(*), min(memory), max(memory), sum(memory)/count(*) from performance where %s %s group by b1 order by b1");

	char *string_sql = apr_psprintf(
			p,
			sql,
			sql_adapter_get_date_period(p, period_begin, period_end, period,
					db_type, NULL),
			sql_adapter_get_filter(p, host, script, uri, db_type, NULL));

	switch (db_type) {
	case 1:
		//SqLite
	{
		int rv;
		sqlite3_stmt *stmt;
		rv = (*_sqlite3_prepare)(s_db_r, string_sql, strlen(string_sql), &stmt,
				0);
		if (rv) {
			return get_error_description(s_db_r, string_sql, p, db_type);
		} else {
			while (1) {
				rv = (*_sqlite3_step)(stmt);
				if (rv == SQLITE_ROW) {
					(*call_back_function6)(r,
							(char *) (*_sqlite3_column_text)(stmt, 0),
							(char *) (*_sqlite3_column_text)(stmt, 1),
							(char *) (*_sqlite3_column_text)(stmt, 2),
							(char *) (*_sqlite3_column_text)(stmt, 3),
							(char *) (*_sqlite3_column_text)(stmt, 4),
							(char *) (*_sqlite3_column_text)(stmt, 5),
							(char *) (*_sqlite3_column_text)(stmt, 6),
							(char *) (*_sqlite3_column_text)(stmt, 7),
							(char *) (*_sqlite3_column_text)(stmt, 8));
				} else if (rv == SQLITE_DONE) {
					break;
				} else {
					(*_sqlite3_reset)(stmt);
					return apr_pstrdup(p, "Error while request processing");
					break;
				}
			}
			(*_sqlite3_reset)(stmt);
		}
	}
		break;
	case 2:
		//MySQL
	{
		mysql_reconnect(m_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db_r) {
			if ((*_mysql_query)(m_db_r, string_sql)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return get_error_description(m_db_r, string_sql, p, db_type);
			}
			MYSQL_RES *result = (*_mysql_store_result)(m_db_r);
			MYSQL_ROW row;
			while ((row = (*_mysql_fetch_row)(result))) {
				(*call_back_function6)(r, row[0], row[1], row[2], row[3],
						row[4], row[5], row[6], row[7], row[8]);
			}
			(*_mysql_free_result)(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	case 3:
		//PgSql
	{
		PGresult *result;
		pgsql_reconnect(p_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if(p_db_r){
		if ((result = (*_PQexec)(p_db_r, string_sql)) == NULL) {
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		if ((*_PQresultStatus)(result) == PGRES_TUPLES_OK) {
			int i;
			for (i = 0; i < (*_PQntuples)(result); i++) {

				(*call_back_function6)(r, PG_GETVALUE (i, 0),
						PG_GETVALUE (i, 1), PG_GETVALUE (i,
								2), PG_GETVALUE (i, 3), PG_GETVALUE (i,
								4), PG_GETVALUE (i, 5), PG_GETVALUE (i,
								6), PG_GETVALUE (i, 7), PG_GETVALUE (i,
								8));
			}
		} else {
			PQClear(result);
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		PQClear(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;

}

char *
sql_adapter_get_custom_text_info(apr_pool_t * p, int db_type, void *r,
		char *period, char *host, char *script, char *uri, char *period_begin,
		char *period_end, int sort, int tp, char *page_number, int per_page,
		void * itm) {
	custom_report_item *item = (custom_report_item*) itm;

	switch (db_type) {
	case 1:
		//SqLite
	{
		if (!item->ssql)
			return NULL;
		char *string_sql = custom_report_pasre_sql_filter(
				item->ssql,
				p,
				sql_adapter_get_date_period(p, period_begin, period_end, period,
						db_type, NULL),
						apr_pstrcat(p," 1=1 ", sql_adapter_get_filter(p, host, script, uri, db_type, NULL),NULL));
		int rv;
		sqlite3_stmt *stmt;
		rv = (*_sqlite3_prepare)(s_db_r, string_sql, strlen(string_sql), &stmt,
				0);
		if (rv) {
			return get_error_description(s_db_r, string_sql, p, db_type);
		} else {
			while (1) {
				rv = (*_sqlite3_step)(stmt);
				if (rv == SQLITE_ROW) {
					ap_rputs("<tr>", r);
					int i = 0;
					for (i = 0; i < item->fields_list->nelts; i++) {
						ap_rvputs(r, "<td>",
								(char *) (*_sqlite3_column_text)(stmt, i),
								"</td>", NULL);
					}
					ap_rputs("</tr>\n", r);

				} else if (rv == SQLITE_DONE) {
					break;
				} else {
					(*_sqlite3_reset)(stmt);
					return apr_pstrdup(p, "Error while request processing");
					break;
				}
			}
			(*_sqlite3_reset)(stmt);
		}
	}
		break;
	case 2:
		//MySQL
	{
		if (!item->msql)
			return NULL;
		char *string_sql = custom_report_pasre_sql_filter(
				item->msql,
				p,
				sql_adapter_get_date_period(p, period_begin, period_end, period,
						db_type, NULL),
						apr_pstrcat(p," 1=1 ", sql_adapter_get_filter(p, host, script, uri, db_type, NULL),NULL));
		mysql_reconnect(m_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if (m_db_r) {
			if ((*_mysql_query)(m_db_r, string_sql)) {
				pthread_rwlock_unlock(&db_lock_rw);
				return get_error_description(m_db_r, string_sql, p, db_type);
			}
			MYSQL_RES *result = (*_mysql_store_result)(m_db_r);
			MYSQL_ROW row;
			while ((row = (*_mysql_fetch_row)(result))) {
				ap_rputs("<tr>", r);
				int i = 0;
				for (i = 0; i < item->fields_list->nelts; i++) {
					ap_rvputs(r, "<td>", row[i], "</td>", NULL);
				}
				ap_rputs("</tr>\n", r);
			}
			(*_mysql_free_result)(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	case 3:
		//PgSql
	{
		if (!item->psql)
			return NULL;
		char *string_sql = custom_report_pasre_sql_filter(
				item->psql,
				p,
				sql_adapter_get_date_period(p, period_begin, period_end, period,
						db_type, NULL),
						apr_pstrcat(p," 1=1 ", sql_adapter_get_filter(p, host, script, uri, db_type, NULL),NULL));
		PGresult *result;
		pgsql_reconnect(p_db_r);
		pthread_rwlock_rdlock(&db_lock_rw);
		if(p_db_r){
		if ((result = (*_PQexec)(p_db_r, string_sql)) == NULL) {
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		if ((*_PQresultStatus)(result) == PGRES_TUPLES_OK) {
			int i;
			for (i = 0; i < (*_PQntuples)(result); i++) {
				ap_rputs("<tr>", r);
				int j = 0;
				for (j = 0; j < item->fields_list->nelts; j++) {
					ap_rvputs(r, "<td>", PG_GETVALUE (i, j), "</td>", NULL);
				}
				ap_rputs("</tr>\n"
						, r);
			}
		} else {
			PQClear(result);
			pthread_rwlock_unlock(&db_lock_rw);
			return get_error_description(p_db_r, string_sql, p, db_type);
		}
		PQClear(result);
		}
		pthread_rwlock_unlock(&db_lock_rw);
	}
		break;
	}
	return NULL;
}

/*
 switch (db_type)
 {
 case 1:
 //SqLite
 break;
 case 2:
 //MySQL

 break;
 case 3:
 //PgSql
 break;
 }
 */
