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
#include "util.h"

#define KEY_VALUE_DELIMITER '='

const char* connectioninfo = CONNECTIONINFO;

/* get the width of the terminal (used for printing results)
 * the function tries 3 ways of getting the terminal width:
 *  - env var COLUMNS
 *  - env var FZF_PREVIEW_COLUMNS
 *  - ioctl
 * for previewing entries with FZF, the first two are best.
 * for other uses (e.g. listing entries in terminal), ioctl
 * is a good solution (because env var may not be set). */
int
get_term_width()
{

  int term_width = 0;

  /* try to get terminal width from environment variables
   * COLUMNS and FZF_PREVIEW_COLUMNS */
  char* columns = getenv("COLUMNS");
  if (!columns)
    columns = getenv("FZF_PREVIEW_COLUMNS");

  /* if at least one env var is not empty, use it as the
   * value of terminal width and returns id */
  if (columns) {
    char* endptr;
    // convert the string to an int
    term_width = (int)strtol(columns, &endptr, 10);

    if (endptr == columns)
      return 0;

    return term_width;
  }

  /* if no env var gave the term_width, get it with ioctl */
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  term_width = w.ws_col;

  return term_width;
}

int
get_single_value(char* id, char* query)
{
  CONNECT; // connection to the database

  const char* params[1] = { id }; // query parameters

  PGresult* res = // send query
    PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);

  // check result
  if (!check_res(res, conn)) {
    PQclear(res);
    PQfinish(conn);
    return 0;
  }

  // puts value
  puts(PQgetvalue(res, 0, 0)); // print the value

  PQclear(res);   // free memory of query
  PQfinish(conn); // end connection

  return 1;
}

void
check_nonull(char* arg, char* arg_type)
{
  if (!arg) {
    fprintf(stderr, "missing argument (%s).\n", arg_type);
    exit(EXIT_FAILURE);
  }
}

int
check_field(const char* field, PGconn* conn)
{

  // if NULL is passed as *conn parameter, then connect.
  int _conn = 0;
  if (!conn) {
    _conn = 1;
    RECONNECT;
  }

  const char* params[1] = { field }; // query paremeters
  PGresult* res = PQexecParams(conn, // send query
    "select field_exists($1::text)",
    1,
    NULL,
    params,
    NULL,
    NULL,
    0);

  int code = 0;
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {

    // query fails
    fprintf(stderr, "ERROR 701: query failed:\n %s\n", PQerrorMessage(conn));
    code = 0;
  } else if (PQgetvalue(res, 0, 0)[0] == 't') {

    // query success and there is a result
    code = 1;
  } else {

    // query success but there is no result
    fputs("unknown field: ", stderr);
    fputs(field, stderr);
    fputc('\n', stderr);
    code = 0;
  }

  PQclear(res); // clear query result

  // if NULL was passed as *conn parameter, then end connection.
  if (_conn)
    PQfinish(conn);

  return code;
}

int
split_v(struct FieldValue* fv, char* s)
{

  size_t len = strnlen(s, VAL_SIZE);

  if (len == VAL_SIZE) {
    fputs("option argument too long.\n", stderr);
    return 0;
  }

  // FIELD_SIZE must be lesser than VAL_SIZE (and it is).
  for (size_t i = 0; i < FIELD_SIZE; i++) {
    switch (s[i]) {

      // unauthorized characters: print error.
      case '\n':
      case '\t':
      case '\'':
      case '\"':
        fputs(
          "unauthorized character in -v arg.\n(quote, newline or "
          "tab.)\n",
          stderr);
        return 0;
        break;

      // delimiter: split the string, and return the result.
      case KEY_VALUE_DELIMITER:
        fv->field = s;
        fv->field[i] = '\0';
        fv->field_len = i;
        fv->value = s + i + 1;
        fv->value_len = len - i;
        return 1;
        break;

      // end of string: end parsing.
      case '\0':
        return 0;
        break;

      // any other character: do nothing
      default:
        break;
    }
  }

  return 0;
}

/* parse an argument for option -v and build the WHERE clause. */
char*
parse_key_value(PGconn* conn,
  struct Stmt* cnd,
  int npar,
  char* optarg)
{

  struct FieldValue fv; // initialize a FieldValue

  // try to split the argument into a field and a value.
  if (!split_v(&fv, optarg)) {
    fputs("failed to parse -v arg (no '", stderr);
    fputc(KEY_VALUE_DELIMITER, stderr);
    fputs("' sign?).\n", stderr);
    return 0;
  }

  if (!check_field(fv.field, conn)) // ensure that the field exists
    return NULL;

  char*
    escaped_field = // escape the field so it can be used in query
    PQescapeIdentifier(conn, fv.field, fv.field_len);

  /* concatenate the two parts of the string and the escaped field.
   * after each call of memccpy, check for the result and if the
   * returned value is 0, free memory for the escaped field and end
   * function. */
#define SIZE sizeof("regexp_like(e.::text, ") + FIELD_SIZE + 1
  size_t dsize = SIZE;
  char s_start[SIZE] = "";
  char* x = memccpy(s_start, "regexp_like(e.", '\0', SIZE);
  if (!x) {
    free(escaped_field);
    return 0;
  }
  size_t gap = (size_t)(x - s_start);
  if (gap >= dsize) {
    free(escaped_field);
    return 0;
  }
  dsize -= gap;
  x = memccpy(x - 1, escaped_field, '\0', dsize);
  free(escaped_field);
  if (!x) {
    return 0;
  }
  gap = (size_t)(x - s_start);
  if (gap >= dsize) {
    return 0;
  }
  dsize -= gap;
  x = memccpy(x - 1, "::text, ", '\0', dsize);
  if (!x) {
    return 0;
  }
  /* call the function cat_cnd and return its return value. */
  if (cat_cnd(cnd, s_start, "::text, 'i') ", npar))
    return fv.value;
  else
    return NULL;
#undef SIZE
}

int
check_file(const char* filepath)
{

  // non-null
  if (!filepath) {
    fputs("'file' requires a value (FILE).\n", stderr);
    return 0;
  }

  // get length of filename
  size_t len = strnlen(filepath, MAX_FILEPATH);

  // length has to be lower than limit
  if (len == MAX_FILEPATH) {
    fprintf(
      stderr, "file name is too long (max is %d).\n", MAX_FILEPATH);
    return 0;
  }

  // length has to be non-null
  else if (len == 0) {
    fputs("'add file' requires a value (FILE).\n", stderr);
    return 0;
  }

  // filepath must be accessible in the filesystem
  else if (access(filepath, F_OK) != 0) {
    fprintf(stderr, "file not found: %s\n", filepath);
    return 0;
  }

  return 1;
}

int
strstarts(const char* str, const char* prefix)
{
  return strncmp(str, prefix, strlen(prefix)) == 0;
}

void
checkconn(PGconn* conn)
{
  if (PQstatus(conn) != CONNECTION_OK) {
    fputs("ERROR 702: failed to connect to database:\n", stderr);
    fputs(PQerrorMessage(conn), stderr);
    fputc('\n', stderr);
    PQfinish(conn);
    exit(EXIT_FAILURE);
  }
}

int
ask_confirmation()
{
  fputs("\nconfirm [y/N]: ", stdout);
  if (getchar() != 'y') {
    fputs("cancelled.\n", stdout);
    return 0;
  }
  return 1;
}

int
update_lastedit(PGconn* conn, char* id)
{
  const char* params[] = { id, NULL };
  PGresult* res = PQexecParams(conn,
    "update reading set lastedit = now() where id = $1",
    1,
    NULL,
    params,
    NULL,
    NULL,
    0);
  PQclear(res);
  return 1; // it does not really matter if the query fails
}

char*
trim(char* s, size_t len)
{
  // move the pointer to the first non-space character.
  char* start = s;

  int i = 0;
  while (isspace(s[i++])) {
    start = &s[i];
  }

  // replace spaces by \0 at the end of the string.
  i = (int)len - 1;
  while (isspace(s[i])) {
    s[i--] = '\0';
  }

  return start;
}

void
print_error_no_arg(const char* argname)
{
  fputs("missing argument: ", stderr);
  fputs(argname, stderr);
  fputc('\n', stderr);
}

int
check_res(PGresult* res, PGconn* conn)
{
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fputs("ERROR 703: query failed:\n", stderr);
    fputs(PQerrorMessage(conn), stderr);
    fputc('\n', stderr);
    return 0;
  }
  return 1;
}
