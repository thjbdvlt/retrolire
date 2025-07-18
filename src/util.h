/* retrolire -- commande line bibliography manager.
 * Copyright (C) 2024,2025  thjbdvlt
 *
 * retrolire is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * retrolire is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with retrolire.  If not, see <https://www.gnu.org/licenses/>.
 */
/* util
 * ----
 *
 * some functions used at some places.
 */

#ifndef _UTIL_H
#define _UTIL_H

#include "../config.h"
#include "stmt.h"
#include <ctype.h>
#include <postgresql/libpq-fe.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

// a function used here to get single value in the database.
int
get_single_value(char* id, char* query);

// check that the argument is non null, or exit with error message
void
check_nonull(char* arg, char* arg_type);

// check that a field exist, or print an error message.
int
check_field(const char* field, PGconn* conn);

// check status of a result, and exit if query failed.
int
check_res(PGresult* res, PGconn* conn);

/* check that a filepath:
 *  - is not NULL.
 *  - is not an empty srting.
 *  - is not too long.
 *  - refers to a file accessible in the file system. */
int
check_file(const char* filepath);

// a struct for key=value parsing
struct FieldValue
{
  char* s;
  char* field;
  char* value;
  size_t field_len;
  size_t value_len;
};

/* split -v arguments (key=value).
 * the function look for the delimiter (=). even if it's probably
 * unnecessary, some character are not authorized and produces
 * error. (for safety): \0 (of course), newlines, tabs and quotes.
 */
int
split_v(struct FieldValue* fv, char* s);

// parse a key-value string (e.g. author=becker).
char*
parse_key_value(PGconn* conn,
  struct Stmt* cnd,
  int n_cnd,
  char* optarg);

// source: linux kernel (linus torvald)
int
strstarts(const char* str, const char* prefix);

// remove space at the start and end of a string
char*
trim(char* s, size_t len);

// check if connection is OK. else, exit program with error message
void
checkconn(PGconn* conn);

// get terminal width
int
get_term_width();

// ask for confirmation
int
ask_confirmation();

// print error ("missing arg: [...]")
void
print_error_no_arg(const char* argname);

// update the column 'lastedit' used to order entries.
int
update_lastedit(PGconn* conn, char* id);

/* two macros to connect or reconnect to database, because
 * connection is everywhere so it's easier have a macro (for
 * consistency).
 *
 * as it's everywhere, it's better to use the same string instead
 * of the macro CONNECTIONINFO (config.h), in order to produce a
 * smaller program. */
extern const char* connectioninfo;
#define CONNECT \
  PGconn* conn = PQconnectdb(connectioninfo); \
  checkconn(conn);
#define RECONNECT \
  conn = PQconnectdb(connectioninfo); \
  checkconn(conn);

#endif
