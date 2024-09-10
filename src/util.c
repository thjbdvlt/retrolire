#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "string.h"
#include "util.h"

#define KEY_VALUE_DELIMITER '.'

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
  if (columns == NULL) {
    columns = getenv("FZF_PREVIEW_COLUMNS");
  }
  /* if at least one env var is not empty, use it as the
   * value of terminal width and returns id */
  if (columns != NULL) {
    char* endptr;
    term_width = (int)strtol(columns, &endptr, 10);
    if (endptr == columns) {
      return 0;
    }
    return term_width;
  }
  /* if no env var gave the term_width, get it with ioctl */
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  term_width = w.ws_col;
  return term_width;
}

/* a function used here to get single value. */
int
get_single_value(char* id, char* query)
{
  CONNECT
  /* array for parameter for single value. */
  const char* params[1] = { id };
  /* send query */
  PGresult* res =
    PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
  /* if query fails, return false. */
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr, "query failed: %s\n", PQerrorMessage(conn));
    PQclear(res);
    PQfinish(conn);
    return 0;
  }
  puts(PQgetvalue(res, 0, 0));
  PQclear(res);
  PQfinish(conn);
  return 1;
}

/* a shortcut function to check for argument and print an error
 * message. */
void
check_nonull(char* arg, char* arg_type)
{
  if (arg == NULL) {
    fprintf(stderr, "missing argument (%s).\n", arg_type);
    exit(EXIT_FAILURE);
  }
}

/* check if a field exists, i.e. is a column of the table ENTRY
 * the function exits if query fails or if query returns false */
int
check_field(char* field)
{
  /* connection to database.*/
  CONNECT
  /* single element array for field. */
  const char* params[1] = { field };
  /* send query. */
  PGresult* res = PQexecParams(conn,
    "select field_exists($1::text)",
    1,
    NULL,
    params,
    NULL,
    NULL,
    0);
  /* end connection. */
  PQfinish(conn);
  /* check result */
  int code = 0;
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr, "query failed:\n %s\n", PQerrorMessage(conn));
    code = 0;
  } else if (PQgetvalue(res, 0, 0)[0] == 't') {
    code = 1;
  } else {
    code = 0;
  }
  PQclear(res);
  return code;
}

/* split -v arguments (key=value).
 * the function look for the delimiter (=). even if it's probably
 * unnecessary, some character are not authorized and produces
 * error. (for safety): \0 (of course), newlines, tabs and quotes.
 */
int
split_v(struct FieldValue* fv, char* s)
{
#define MSGERROR \
  "unauthorized character in -v arg.\n(quote, newline or " \
  "tab.)\n"
  size_t len = strnlen(s, VAL_SIZE);
  if (len == VAL_SIZE) {
    fputs("option argument too long.\n", stderr);
    return 0;
  }
  for (size_t i = 0; i < len && i < FIELD_SIZE; i++) {
    switch (s[i]) {
      case '\0':
        return 0;
        break;
      case '\n':
        fputs(MSGERROR, stderr);
        return 0;
        break;
      case '\t':
        fputs(MSGERROR, stderr);
        return 0;
        break;
      case '\'':
        fputs(MSGERROR, stderr);
        return 0;
        break;
      case '\"':
        fputs(MSGERROR, stderr);
        return 0;
        break;
      case KEY_VALUE_DELIMITER:
        fv->field = s;
        fv->field_len = i;
        fv->value = s + i + 1;
        fv->value_len = len - i;
        return 1;
        break;
      default:
        break;
    }
  }
  fprintf(stderr,
    "failed to parse -v arg (no %c sign?).\n",
    KEY_VALUE_DELIMITER);
  return 0;
#undef MSGERROR
}

/* parse an argument for option -v (key=value) and build the WHERE
clause. */
char*
parse_key_value(PGconn* conn,
  struct Stmt* cnd,
  int npar,
  char* optarg)
{
  /* define a macro for the size of the s_start string, which is the
   * concatenation of "regexp_like(e.", the escaped field, and
   * "::text, ". */
  /* initiate a string to perform concatenation with. */
  /* use the split_v function and check its return value. if it's 0,
   * it failed, so exit function. */

  struct FieldValue fv;
  if (!split_v(&fv, optarg)) {
    return 0;
  }
  char* escaped_field =
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

/* check that a filepath:
 *  - is not NULL.
 *  - is not an empty srting.
 *  - is not too long.
 *  - refers to a file accessible in the file system. */
int
check_file(char* filepath)
{
  /* non null. */
  if (!filepath) {
    fputs("'file' requires a value (FILE).\n", stderr);
    return 0;
  }

  /* get length of filename. */
  size_t len = strnlen(filepath, MAX_FILEPATH);

  /* length has to be lower than limit. */
  if (len == MAX_FILEPATH) {
    fprintf(
      stderr, "file name is too long (max is %d).\n", MAX_FILEPATH);
    return 0;
  }

  /* length has to be non-null. */
  else if (len == 0) {
    fputs("'add file' requires a value (FILE).\n", stderr);
    return 0;
  }

  /* filepath must be accessible in the filesystem. */
  else if (access(filepath, F_OK) != 0) {
    fprintf(stderr, "file not found: %s\n", filepath);
    return 0;
  }

  return 1;
}

/* found in an answer on stackoverflow, mentioning it's in the linux
 * kernel. URL of the topic:
 * https://stackoverflow.com/questions/4770985/how-to-check-if-a-string-starts-with-another-string-in-c
 */

int
strstarts(const char* str, const char* prefix)
{
  return strncmp(str, prefix, strlen(prefix)) == 0;
}

/* check if a connection is OK. if it's not, then end the
 * connection. and exit the program, printing an error message. */
void
checkconn(PGconn* conn)
{
  if (PQstatus(conn) != CONNECTION_OK) {
    fprintf(stderr,
      "failed to connect to database:\n %s\n",
      PQerrorMessage(conn));
    PQfinish(conn);
    exit(EXIT_FAILURE);
  }
}

/* add tags to an entry. */
int
cache_entry(char* id)
{

  /* connect to the database, and make sure the line in cache is not
   * empty by insert an empty line.  */
  CONNECT
  const char* params[1] = { id };
  PGresult* res = PQexec(
    conn, "insert into _cache select on conflict do nothing");
  /* check result. */
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,
      "error sending query to database:\n%s\n",
      PQerrorMessage(conn));
    PQclear(res);
    PQfinish(conn);
    return 0;
  }
  PQclear(res);

  /* update the value of 'entry' with the selected id, for the
   * preview. */
  res = PQexecParams(conn,
    "update _cache set entry = $1",
    1,
    NULL,
    params,
    NULL,
    NULL,
    0);

  /* check result. */
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,
      "error sending query to database:\n%s\n",
      PQerrorMessage(conn));
    PQclear(res);
    PQfinish(conn);
    return 0;
  }

  /* clear query. */
  PQclear(res);

  /* make the fzf binding and add it to the shell command. then
   * end the connection and return the pointer to the end of
   * the shell command. */
  PQfinish(conn);

  return 1;
}
