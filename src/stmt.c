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
#include "../config.h"
#include "stmt.h"
#include <stdio.h>
#include <string.h>

void
init_stmt(struct Stmt* s, char* source, size_t total, size_t used)
{
  s->start = &source[0];
  /* if used is 0, then end must be 1 or append_stmt will fail.*/
  s->end = &source[(used == 0) ? 1 : used];
  s->total = total;
  s->remain = total - used;
  s->next_or = 0;
  s->next_not = 0;
}

void
reinit_stmt(struct Stmt* s)
{
  s->end = &s->start[1];
  s->remain = s->total;
  s->next_or = 0;
  s->next_not = 0;
}

/* append a string to a Stmt, and check if (1) it remains more than
 * zero bytes free in the Stmt destination and (2) the substring has
 * been correctly append  the condition (1) is performed before the
 * concatenation, and the condition (2) after. */
int
append_stmt(struct Stmt* dest, const char* source)
{
  char* end = dest->end;
  size_t remain = dest->remain;

  /* copy the string. it's here that the source string is traversed
   * and that its length is calculated. memccpy won't copy more that
   * what remains so it prevent buffer overflows. */
  char* x = memccpy(end - 1, source, '\0', remain);
  /* the second check is not about the value of dest->remain itself,
   * but if this value was enough for the source string: memccpy
   * returns NULL if the substring '\0' wasn't found in
   * dest->remain, i.e. if dest->remain wasn't large enough. */
  if (!x) {
    dest->end = NULL;
    fputs("ERROR 101: concatenation error (too many data.)\n", stderr);
    return 0;
  }
  /* update the properties of the statement. */
  size_t gap = (size_t)(x - end);
  if (gap >= remain) {
    return 0;
  }
  remain -= gap;
  dest->remain = remain;
  dest->end = x;
  return 1;
}

/* aggregate an array and append it to a string. */
int
arrayagg(struct Stmt* s, char array[][VAL_SIZE], int n)
{
  if (!append_stmt(s, "array["))
    return 0;
  int i;
  for (i = 0; i < n; i++) {
    if (!append_stmt(s, array[i]))
      return 0;
    ;
    s->end[-1] = ',';
    s->end++;
    s->remain--;
  }
  s->end[-2] = ']';
  s->end[-1] = '\0';
  s->remain++;
  return 1;
}

/* add a WHERE or AND conditional operator before a condional
 * clause. */
int
operate_cnd(struct Stmt* cnd, int npar)
{
  if (npar == 0)
    return append_stmt(cnd, "WHERE (");
  return append_stmt(cnd, cnd->next_or ? " OR " : ") AND (");
}

/* concatenate a conditional clause to the select statement.
 * parameters:
 *  - end_cnd: a pointer to the end of the conditional clauses
 * string. (it's the destination.)
 *  - s_start: a pointer to the beginning of a clause (source).
 *  - s_end: a pointer to the end of a clause (source).
 *  - npar: the number of parameters (to generate a placeholder).
 * the placeholder is between s_start and s_end.
 *  - dsize: the remaining size of dest. */
int
cat_cnd(struct Stmt* cnd, char* s_start, char* s_end, int npar)
{
  /* generate the placeholder from parameter 'npar', the number of
   * parameters before the function call. the integer cannot be > 20
   * because of constraints and checks before this function. */
  char ph[SIZE_PLACEHOLDER] = "";
  snprintf(ph, SIZE_PLACEHOLDER, "$%d", npar + 1);
  /* pointer to the end +1: it's needes to use a 'for loop'
   * structure, because of memccpy returning the end +1 of a string
   * (i think?) so it's needed to do memccpy(p+1) for chain
   * concatenation. */

  /* steps in the process of appending a conditional statement:
   * - append WHERE/AND depending of it's the first condition.
   * - then, if option -n is used before the current condition, add
   * the logical operator NOT.
   * - append the condition, with the placeholder for its value.
   *   */
  operate_cnd(cnd, npar);

  if (cnd->next_not) {
    if (!append_stmt(cnd, " not "))
      return 0;
    cnd->next_not = 0; // reset value
  }

  char* strings[] = { s_start, ph, s_end };
  for (long unsigned int i = 0; i < sizeof(strings) / sizeof(char*);
       i++) {
    if (!append_stmt(cnd, strings[i])) {
      return 0;
    };
  }
  return 1;
}

/* Add a placeholder to a string */
int
cat_ph(struct Stmt* cnd, char* s_start, char* s_end, int npar)
{
  char ph[SIZE_PLACEHOLDER] = "";
  snprintf(ph, SIZE_PLACEHOLDER, "$%d", npar + 1);
  char* strings[] = { s_start, ph, s_end };
  for (long unsigned int i = 0; i < sizeof(strings) / sizeof(char*);
       i++) {
    if (!append_stmt(cnd, strings[i])) {
      return 0;
    };
  }
  return 1;
}

int
append_lastedit(struct Stmt cnd, int lastedit)
{
  if (lastedit == LASTEDIT_LAST) {
    /* replace the condition clauses by a new one with only the
     * lastedit clause: order entries by lastedit and select only
     * one (the last one). */
    if (!append_stmt(&cnd, "order by r.lastedit desc limit 1")) {
      fputs(
        "ERROR 102: failed writing conditional clause (option -l).\n", stderr);
      return 0;
    };
    /* lastedit can also hold another value than 1 (select last
     * edited entry) or 0 (do nothing with lastedit field): it can
     * be 2, for ordering entries using lastedit field. it's
     * important that the appending of the ORDER clause come at the
     * end, or it will obviously produce a syntax error if there are
     * WHERE clause after it.*/
#ifdef ORDER_BY_LASTEDIT
  } else {
#else
  } else if (lastedit == LASTEDIT_RECENT) {
#endif
    if (!append_stmt(&cnd, "\norder by r.lastedit desc\n")) {
      return 0;
    };
  }
  return 1;
}

/* append a value at the end of an array. */
int
append_sh(struct ShCmd* sh, const char* value)
{
  if (sh->n_args >= sh->maxargs)
    return 0;
  sh->args[sh->n_args] = value;
  sh->n_args++;
  sh->args[sh->n_args] = NULL;
  return 1;
}

/* append many value */
int
append_sh_many(struct ShCmd* sh, const char** valuearray)
{

  const int max = sh->maxargs;
  const char* val;
  const char** args = sh->args;

  for (int i = 0, y = sh->n_args; y < max; i++, y++) {

    // get the values from the SOURCE array
    val = valuearray[i];

    // put it in the DEST array
    args[y] = val;

    // if this value is NULL, it's the end of SOURCE array
    if (!val) {

      sh->n_args = y; // update
      return 1;
    }
  }

  // if it go out of the loop, the limit has been reached, so the
  // function failed.
  return 0;
}
