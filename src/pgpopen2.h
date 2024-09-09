/* pgpopen2
 * --------
 *
 * pipe out a PGresult to a subprocess (shell command) then read
 * the result.
 *
 * */

#ifndef _PGPOPEN2_H
#define _PGPOPEN2_H

#include <postgresql/libpq-fe.h>
#include <stdio.h>
#include <unistd.h>

/* pgpopen2 -- pipe out a PGresult then read the result.
 *
 * like popen2, it uses bidirectional pipe (main -> sub -> main).
 *
 * parameters
 * ----------
 *
 * res (PGresult):
 *      pointer to the PGresult.
 *
 * field_sep (char*):
 *      field separator (char*).
 *
 * record_sep (char):
 *      field separator (single char).
 *
 * dest (char*):
 *      destination for the result.
 *
 * s_dest (size_t):
 *      remaining size of destination.
 *
 * cmd (char*):
 *      command to be used (no arguments).
 *
 * argv (char* const []):
 *      arguments for the command. by convention, the
 *      first one ([0]) is the name of the command itself. last
 *      value has to be NULL.
 *
 */
int
pgpopen2(PGresult* res,
  char* field_sep,
  char record_sep,
  char* dest,
  size_t s_dest,
  char* cmd,
  char* const argv[]);

/* write_res -- write a PGresult to a file.
 *
 * parameters
 * ----------
 *
 *  res (PGresult*)
 *      the result of the query.
 *
 *  field_sep (char*)
 *      field separator
 *
 *  record_sep (char)
 *      record separator. a single char.
 *
 *  f (FILE*)
 *      the opened file.
 * */
void
write_res(PGresult* res, char* field_sep, char record_sep, FILE* f);

#endif
