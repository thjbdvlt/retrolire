/* util
 * ----
 *
 * some functions used at some places.
 *
 * */

#ifndef _UTIL_H
#define _UTIL_H

#include <postgresql/libpq-fe.h>

#include "../config.h"
#include "stmt.h"

/* get a single value from the database. */
int
get_single_value(char* id, char* query);

/* check for non-nullness. */
void
check_nonull(char* arg, char* arg_type);

/* check that a field exist (for update). */
int
check_field(char* field);

/* check filepath (and that file exists). */
int
check_file(char* filepath);

// a struc for key=value parsing
struct FieldValue
{
  char* s;
  char* field;
  char* value;
  size_t field_len;
  size_t value_len;
};

int
split_v(struct FieldValue* fv, char* s);

/* parse a key-value string (e.g. author=becker). */
char*
parse_key_value(PGconn* conn,
  struct Stmt* cnd,
  int n_cnd,
  char* optarg);

/* test if a string starts with a prefix. */
int
strstarts(const char* str, const char* prefix);

/* add tags to an entry. */
int
cache_entry(char* id);

/* check that a connection is successfully. */
void
checkconn(PGconn* conn);

/* get the terminal width. */
int
get_term_width();

int
ask_confirmation();

/* two macros to connect or reconnect to database, because
 * connection is everywhere so it's easier have a macro (for
 * consistency). */
#define CONNECT \
  PGconn* conn = PQconnectdb(connectioninfo); \
  checkconn(conn);

#define RECONNECT \
  conn = PQconnectdb(connectioninfo); \
  checkconn(conn);

#endif
