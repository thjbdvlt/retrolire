/* stmt
 * ---
 *
 * dynamically generate SQL statement or Shell commnands.
 *
 * there are two struct:
 *
 * - Stmt, for SQL. concatenate strings. store a string. it's passed
 *   as query parameter to get a PGresult.
 * - ShCmd, for shell commands. stores strings in strings array, to
 * be used in the 'execpv(command, argv)' function as 'argv'.
 *
 * */

#ifndef _STMT_H
#define _STMT_H

#include <string.h>

#include "sizes.h"

/* the Stmt struct is used to concatenate strings.*/
struct Stmt
{
  size_t remain;
  size_t total;
  char* end;
  char* start;
  char name[10];
};

/* initiliase a Stmt from a string. */
void
init_stmt(struct Stmt* s, char* source, size_t total, size_t used);

/* reinitialise a Stmt with a string. */
void
reinit_stmt(struct Stmt* s);

/* append a string to a Stmt. */
int
append_stmt(struct Stmt* dest, char* source);

/* aggregate an array and append it to a Stmt. */
int
arrayagg(struct Stmt* s, char array[][VAL_SIZE], int n);

/* concatenate a string as a conditional clause (WHERE/AND). */
int
cat_cnd(struct Stmt* cnd, char* s_start, char* s_end, int npar);

/* append an ORDER clause to a Stmt. */
int
append_lastedit(struct Stmt cnd, int lastedit);

/* ShCmd are for shell commands, where arguments are stored and
 * passed in functions as an array of char. */
struct ShCmd
{
  int n_args;
  char* args[];
};

/* init a ShCmd from an array of strings. */
int
init_sh(struct ShCmd* sh, char* values[]);

/* append a value to a ShCmd. */
int
append_sh(struct ShCmd* sh, char* value);

/* some macros for the SQL generation. a BASE_STMT that defines the
 * SELECT statement (without WHERE/ORDER clauses) to be used for
 * most commands. */
#define BASE_STMT \
  "select e.id, coalesce(e.title, e.url) as title \n, " \
  "jsonb_concat_values(coalesce(e.author, e.editor, " \
  "e.translator), ' ') as someone\nfrom entry e join reading r " \
  "on r.id = e.id "
#define WHEREAND(i) i == 0 ? "\n\nwhere " : "\nand "
#define ORDER "order by lastedit"
#define SIZE_CND MAX_SIZE - (sizeof(BASE_STMT) + sizeof(ORDER))

#endif
