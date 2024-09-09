#include "stmt.h"
#include "../config.h"
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
}

void
reinit_stmt(struct Stmt* s)
{
  s->end = &s->start[1];
  s->remain = s->total;
}

/* append a string to a Stmt, and check if (1) it remains more than
 * zero bytes free in the Stmt destination and (2) the substring has
 * been correctly append  the condition (1) is performed before the
 * concatenation, and the condition (2) after. */
int
append_stmt(struct Stmt* dest, char* source)
{
  /* check that remain is more than 0. i check that because if a
   * negative string is passed as last argument of memccpy, it is
   * not used like a zero, so memccpy(dest, "a", '\0', -10) won't
   * return NULL. */
  if (dest->remain <= 0) {
    dest->end = NULL;
    fputs("concatenation error (too many data.)\n", stderr);
    return 0;
  }
  char* x = NULL;
  /* copy the string. it's here that the source string is traversed
   * and that its length is calculated. memccpy won't copy more that
   * what remains so it prevent buffer overflows. */
  x = memccpy(dest->end - 1, source, '\0', dest->remain);
  /* the second check is not about the value of dest->remain itself,
   * but if this value was enough for the source string: memccpy
   * returns NULL if the substring '\0' wasn't found in
   * dest->remain, i.e. if dest->remain wasn't large enough. */
  if (!x) {
    dest->end = NULL;
    fputs("concatenation error (too many data.)\n", stderr);
    return 0;
  }
  /* update the properties of the statement. */
  size_t gap = (size_t)(x - dest->end);
  if (gap >= dest->remain) {
    return 0;
  }
  dest->remain -= gap;
  dest->end = x;
  return 1;
}

/* aggregate an array and append it to a string. */
int
arrayagg(struct Stmt* s, char array[][VAL_SIZE], int n)
{
  if (append_stmt(s, "array[") == 0) {
    return 0;
  }
  int i;
  for (i = 0; i < n; i++) {
    if (append_stmt(s, array[i]) == 0) {
      return 0;
    };
    s->end[-1] = ',';
    s->end++;
    s->remain--;
  }
  s->end[-2] = ']';
  s->end[-1] = '\0';
  s->remain++;
  return 1;
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
  char ph[PH] = "";
  snprintf(ph, PH, "$%d", npar + 1);
  /* pointer to the end +1: it's needes to use a 'for loop'
   * structure, because of memccpy returning the end +1 of a string
   * (i think?) so it's needed to do memccpy(p+1) for chain
   * concatenation. */
  char* strings[] = { WHEREAND(npar), s_start, ph, s_end };
  for (long unsigned int i = 0; i < sizeof(strings) / sizeof(char*);
       i++) {
    if (append_stmt(cnd, strings[i]) == 0) {
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
    if (append_stmt(&cnd, "order by r.lastedit desc limit 1") ==
        0) {
      fputs(
        "failed writing conditional clause (option -l).\n", stderr);
      return 0;
    };
    /* lastedit can also hold another value than 1 (select last
     * edited entry) or 0 (do nothing with lastedit field): it can
     * be 2, for ordering entries using lastedit field. it's
     * important that the appending of the ORDER clause come at the
     * end, or it will obviously produce a syntax error if there are
     * WHERE clause after it.*/
  } else if (lastedit == LASTEDIT_RECENT) {
    if (append_stmt(&cnd, "\norder by r.lastedit desc\n") == 0) {
      return 0;
    };
  }
  return 1;
}

/* append a value at the end of an array. */
int
append_sh(struct ShCmd* sh, char* value)
{
  sh->args[sh->n_args] = value;
  sh->n_args++;
  sh->args[sh->n_args] = NULL;
  return 1;
}

/* init a ShCmd. */
int
init_sh(struct ShCmd* sh, char* values[])
{
  int n_args = 0;
  int i;
  for (i = 0; values[i] != NULL; i++) {
    sh->args[i] = values[i];
  }
  sh->n_args = i;
  return 1;
}
