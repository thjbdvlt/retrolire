#include <ctype.h>
#include <postgresql/libpq-fe.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "print.h"
#include "sizes.h"
#include "util.h"

// print the result of a query, row by row (expanded mode wrapped).
int
print_result(PGresult* res, FILE* f, int term_width)
{
  // get number of rows and columns, in order to iterate on them.
  int n_rows = PQntuples(res);
  int n_fields = PQnfields(res);
  // two arrays for fields:
  // - name
  // - length of name
  char fields_names[n_fields][100];
  int field_names_lens[n_fields];
  // get the longest field name length
  int max_f_len = 0;
  int curlen = 0;
  int i, s, j;
  for (int i = 0; i < n_fields; i++) {
    // get field name, put in array
    char* x = memccpy(fields_names[i],
      PQfname(res, i),
      '\0',
      sizeof(char*) * ((size_t)term_width));
    if (!x) {
      fputs("field name too long for terminal.\n", stderr);
      return 0;
    }
    // calculate the len of its name
    curlen = (int)strnlen(fields_names[i], FIELD_SIZE);
    if (curlen == FIELD_SIZE) {
      fputs("field name too long (max. 48 characters).\n", stderr);
      return 0;
    }
    // put this len in an array
    field_names_lens[i] = curlen;
    // look for the longest length.
    if (curlen > max_f_len) {
      max_f_len = curlen;
    }
  }
  // add padding after each field name and build a string
  // that is field + padding (spaces) + "|".
  for (i = 0; i < n_fields; i++) {
    for (s = field_names_lens[i]; s < max_f_len; s++) {
      // padding with spaces.
      fields_names[i][s] = ' ';
      // fields_names[i][s] = '.';
    }
    // separator '|' between field and value.
    fields_names[i][s] = '|';
    // space after separator.
    fields_names[i][s + 1] = ' ';
    // terminate string.
    fields_names[i][s + 2] = '\0';
  }
  // create a string for margin, to be used when a value is too
  // long.
#define size 100
  char margin[size];
  margin[0] = '\n';
  for (s = 1; s <= max_f_len; s++) {
    margin[s] = ' ';
  }
  char* x = memccpy(margin + s, "\\  ", '\0', size);
  if (!x) {
    fputs("error creating margin for printing.\n", stderr);
    return 0;
  }
#undef size
  // get the number of columns.
  char record_sep[term_width];
  // define a record-separator, on a new line.
  // the record separator is just made of "-" (hyphen).
  // it has the width of the terminal.
  term_width -= 4;
  for (i = 0; i < term_width; i++) {
    record_sep[i] = '-';
  }
  // terminate correctly by the null character to end the string.
  record_sep[i] = '\0';
  // define a limit: the maximum number of character per line for a
  // value.
  int limit = term_width - (max_f_len + 1);
  // start by a record separator (as there is also one at the end).
  fputs(record_sep, f);
  putc('\n', f);
  // iterate on the rows
  for (i = 0; i < n_rows; i++) {
    // iterate on the fields
    for (j = 0; j < n_fields; j++) {
      // if the value is NULL, do not print it.
      if (PQgetisnull(res, i, j) == 0) {
        // else, print the field name (with padding and delimiter).
        fputs(fields_names[j], f);
        // get the value
        char* data = PQgetvalue(res, i, j);
        // get the length of the value
        int len = PQgetlength(res, i, j);
        // if the length of the value is smaller than the limit,
        // i just print it.
        if (len < limit) {
          fputs(data, f);
        }
        // but if the length is bigger than the limit, then i
        // wrap the value
        else {
          // first, put the first char, so i can use modulo without
          // having issue with the first char (index 0).
          putc(data[0], f);
          // then, i iterate over the chars, until i reach the total
          // length
          for (int c = 1; c < len; ++c) {
            // if char-index modulo limit is 0: newline and margin.
            if (c % limit == 0) {
              // that part is ugly but well it works so for now i'll
              // keep this.
              if (isascii(data[c])) {
                fputs(margin, f);
              } else {
                while (1) {
                  putc(data[c], f);
                  c++;
                  if (isascii(data[c])) {
                    fputs(margin, f);
                    break;
                  }
                }
              }
            }
            // put the char.
            putc(data[c], f);
          }
        }
        putc('\n', f);
      }
    }
    // a record separator: newlines and many hyphens.
    fputs(record_sep, f);
    putc('\n', f);
  }
  return 1;
}

/* minimal informations about an entry. */
int
head_entry(char* id)
{
  /* connect to the database. then, define an array for parameters,
   * and a string for statement.
   * */
  CONNECT
  const char* params[] = { id };
  PGresult* res = PQexecParams(conn,
    "select * from _head where id = $1",
    1,
    NULL,
    params,
    NULL,
    NULL,
    0);
  /* this function is used for previewing, so i don't want it to
   * print long and complete messages. but i still handle errors and
   * exit function in case there is a problem. */
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fputs("query failed.\n", stderr);
    PQclear(res);
    PQfinish(conn);
    return 0;
  } else if (PQntuples(res) == 0) {
    PQfinish(conn);
    PQclear(res);
    return 0;
  }
  int n_fields = PQnfields(res);
  for (int i = 0; i < n_fields; i++) {
    /* it's not needed to check for NULL because NULL values are
     * returned as empty strings: so there is no problem to just
     * 'puts' them out. */
    puts(PQgetvalue(res, 0, i));
  }
  PQclear(res);
  PQfinish(conn);
  return 0;
}

/* preview with: bat, less or stdout. */
int
preview(char* id)
{
  /* connect to the database and send a simple query to get entry
   * metadata (all fields). */
  CONNECT
  const char* params[] = { id };

  /* first part of the preview: informations about the entry. */
  PGresult* res = PQexecParams(conn,
    "select e.*, get_tags(e, '') as tags from entry e\n"
    "where e.id = $1",
    1,
    NULL,
    params,
    NULL,
    NULL,
    0);
  if (PQresultStatus(res) == PGRES_TUPLES_OK)
    print_result(res, stdout, get_term_width());
  PQclear(res);

  /* show files associated with the entry, and show tags. */
  res = PQexecParams(conn,
    "select filepath from file where entry = $1\n"
    "union select \"URL\" from entry e where e.id = $1",
    1,
    NULL,
    params,
    NULL,
    NULL,
    0);
  /* 2nd and 3rd queries are for files are for notes.
   * here, it's different from the first query. if it fails or if
   * there is no row, it doesn't matter. and it's the same for the
   * last query. */
  if (PQresultStatus(res) == PGRES_TUPLES_OK) {
    int n_rows = PQntuples(res);
    if (n_rows > 0) {
      for (int i = 0; i < n_rows; i++) {
        fputs(PQgetvalue(res, i, 0), stdout);
        fputc('\n', stdout);
      }
    }
  } else {
    fputs(PQerrorMessage(conn), stderr);
  }

  PQclear(res);
  res = PQexecParams(conn,
    "select notes from reading where id = $1",
    1,
    NULL,
    params,
    NULL,
    NULL,
    0);
  if (PQresultStatus(res) == PGRES_TUPLES_OK) {
    int n_rows = PQntuples(res);
    if (n_rows != 0) {
      char* data = PQgetvalue(res, 0, 0);
      if (data != NULL) {
        fputs("\n\n", stdout);
        fputs(data, stdout);
      }
    }
  } else {
    fputs(PQerrorMessage(conn), stderr);
  }

  putc('\n', stdout);
  PQclear(res);
  PQfinish(conn);
  return 1;
}

int
preview_cache_entry()
{
  CONNECT
  PGresult* res = PQexec(conn,
    "select e.*, get_tags(e, '') as tags from entry e\n"
    "join _cache c on c.entry = e.id\n");
  print_result(res, stdout, get_term_width());
  PQclear(res);
  PQfinish(conn);
  return 1;
}

/* get a list of all tags. */
int
list_tags()
{
  /* connect to database. */
  CONNECT;
  /* send query. */
  PGresult* res = PQexec(conn, "select distinct tag from tag");
  /* ensure result. */
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr, "query failed: %s\n", PQerrorMessage(conn));
    /* if error: free and exit function. */
    PQclear(res);
    PQfinish(conn);
    return 0;
  }
  /* print rows. */
  int n_rows = PQntuples(res);
  for (int i = 0; i < n_rows; i++) {
    puts(PQgetvalue(res, i, 0));
  }
  /* free and end. */
  PQclear(res);
  PQfinish(conn);
  return 1;
}

int
list_fields()
{
  CONNECT;
  PGresult* res = PQexec(conn, "select list_fields()");
  int code = 1;
  /* for auto completion: no error message if it fails.*/
  if (PQresultStatus(res) != PGRES_TUPLES_OK ||
      PQntuples(res) == 0) {
    code = 0;
  }
  char* data = PQgetvalue(res, 0, 0);
  puts(data);
  return code;
}
