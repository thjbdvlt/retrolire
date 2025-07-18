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

#include "sizes.h"
#include <string.h>

/* the Stmt struct is used to concatenate strings.*/
struct Stmt
{
  size_t remain;
  size_t total;
  char* end;
  char* start;
  int next_not;
  int next_or;
};

/* initiliase a Stmt from a string. */
void
init_stmt(struct Stmt* s, char* source, size_t total, size_t used);

/* reinitialise a Stmt with a string. */
void
reinit_stmt(struct Stmt* s);

/* append a string to a Stmt. */
int
append_stmt(struct Stmt* dest, const char* source);

/* aggregate an array and append it to a Stmt. */
int
arrayagg(struct Stmt* s, char array[][VAL_SIZE], int n);

/* add an operator before a conditional statement (WHERE/AND) */
int
operate_cnd(struct Stmt* cnd, int npar);

/* concatenate a string as a conditional clause (WHERE/AND). */
int
cat_cnd(struct Stmt* cnd, char* s_start, char* s_end, int npar);
int

/* concatenate a string with placeholder (withouht WHERE/AND). */
cat_ph(struct Stmt* cnd, char* s_start, char* s_end, int npar);

/* append an ORDER clause to a Stmt. */
int
append_lastedit(struct Stmt cnd, int lastedit);

/* ShCmd are for shell commands, where arguments are stored and
 * passed in functions as an array of char. */
struct ShCmd
{
  int n_args;
  const int maxargs;
  const char** args;
};

/* append a value to a ShCmd. */
int
append_sh(struct ShCmd* sh, const char* value);
int
append_sh_many(struct ShCmd* sh, const char** valuearray);

#endif
