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
#include "pgpopen2.h"

#define READ 0
#define WRITE 1

int
pgpopen2(PGresult* res,
  const char* field_sep,
  const char record_sep,
  char* dest,
  size_t s_dest,
  const char* cmd,
  char* const argv[])
{

  int p_in[2];
  int p_out[2];
  pid_t cpid;

  /* create pipes and return from function on error. */
  if ((pipe(p_in) == -1) || (pipe(p_out) == -1)) {
    perror("pipe");
    return 0;
  }

  /* fork a subprocess. (return from function if fork failed.) */
  cpid = fork();
  if (cpid == -1) {
    perror("fork");
    return 0;
  }

  else if (cpid == 0) {
    /* child process */

    /* close file descriptors used by parent process.*/
    close(p_in[1]);
    close(p_out[0]);

    /* redirect STDIN / STDOUT */
    dup2(p_in[0], STDIN_FILENO);
    dup2(p_out[1], STDOUT_FILENO);

    /* execute command */
    execvp(cmd, argv); // tcc: bad address..?
    perror("execvp");

    /* close file descriptors */
    close(p_out[1]);
    close(p_in[0]);

    _exit(EXIT_SUCCESS); // unnecessary?
  }

  else {
    /* parent process */

    /* close file descriptors used by child process.*/
    close(p_out[1]);
    close(p_in[0]);

    /* open the file from file descriptor (pipe). */
    FILE* f = fdopen(p_in[1], "w");

    /* exit function if the file descriptor opening fails. */
    if (!f) {
      fputs("pipe failed.\n", stderr);
      return 0;
    }

    write_res(res, field_sep, record_sep, f);

    /* close file (pipe). */
    fclose(f);

    /* close file descriptor.
     * /!\: before read(...) */
    close(p_in[1]);

    /* read to dest */
    read(p_out[0], dest, s_dest);

    /* close file descriptors.*/
    close(p_out[0]);
  }

  return 1;
}

void
write_res(PGresult* res,
  const char* field_sep,
  const char record_sep,
  FILE* f)
{
  /* get number of rows and columns, in order to iterate on them.
   * i substract 1 to n_fields because i don't want a delimiter
   * after last field (so it requires another processing after the
   * loop). */
  int n_rows = PQntuples(res);
  int n_fields = PQnfields(res) - 1;
  int i = 0, j = 0;
  /* iterate over the rows and columns and pipe everything to
   * stdout (popen). there is no need to check that a value is not
   * NULL because NULL values are just returned as empty strings
   * by libpq.*/
  for (i = 0; i < n_rows; i++) {
    for (j = 0; j < n_fields; j++) {
      char* data = PQgetvalue(res, i, j);
      fputs(data, f);
      fputs(field_sep, f);
    }
    char* data = PQgetvalue(res, i, j);
    fputs(data, f);
    fputc(record_sep, f);
  }
}
