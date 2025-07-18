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
#include <sys/wait.h>

char*
edit_in_editor(char* value, char* ext, char* linenr)
{
#define FILEPATH "/tmp/retrolire.XXXXXX."
#define MAX_EXT_LEN 8
#define FNAMESIZE sizeof(FILEPATH) + MAX_EXT_LEN + 1
#define MEMCCPYERROR "ERROR 500: memccpy error.\n"

  /* the first part of the command deals with the extension of the
   * file. that extension is important because syntax highlight will
   * depends on it in $EDITOR. typically, i want markdown for the
   * notes, but i don't want markdown for the JSON fields, neither
   * for tags, because tags are one tag a line, and a tag could be
   * '_important' and with markdown highlight it would be a mess.
   *
   * (the extension cannot be longer than 8 characters.) */

  // get the size of the extension
  size_t extlen = strnlen(ext, MAX_EXT_LEN);
  if (extlen == MAX_EXT_LEN) {
    fputs("ERROR 501: file extension too long.\n", stderr);
    return NULL;
  }

  // add 1 to extension, for the dot.
  extlen++;

  // create a buffer to copy the file name and extension
  char fname[FNAMESIZE] = "";

  // copy the filepath
  char* p = memccpy(fname, FILEPATH, '\0', sizeof(FILEPATH) + 1);
  if (!p) {
    fputs(MEMCCPYERROR, stderr);
    return NULL;
  }

  // copy the extension
  p = memccpy(p - 1, ext, '\0', MAX_EXT_LEN);
  if (!p) {
    fputs(MEMCCPYERROR, stderr);
    return NULL;
  }

  // try to create a temporary file with this filename
  if (mkstemps(fname, (int)extlen) == -1) {
    fputs(MEMCCPYERROR, stderr);
    return NULL;
  }

  // open the file, write the value in it, close it.
  FILE* f;
  f = fopen(fname, "w");
  if (!f) {
    fputs("ERROR 502: error opening file.\n", stderr);
    return NULL;
  }
  fputs(value, f);
  fclose(f);

  // buffer for the whole command (EDITOR + FILEPATH)
  char cmd[sizeof(FILEPATH) + MAX_EXT_LEN + sizeof(EDITOR) + 4 +
           sizeof(LINENR_PREFIX) + 4] = "";

  char* x = &cmd[0];
  size_t csize = sizeof(cmd);

  // the command starts by the EDITOR (defined in config.h)
  x = memccpy(x, EDITOR, '\0', csize);
  if (!x) {
    fputs(MEMCCPYERROR, stderr);
    return NULL;
  }

  // add a space between the command and its argument (file)
  x[-1] = ' ';
  size_t gap = (size_t)(x - cmd);
  if (gap >= csize) {
    return NULL;
  }
  csize -= gap;

  // add the file name
  x = memccpy(x, fname, '\0', csize);
  if (!x) {
    fputs(MEMCCPYERROR, stderr);
    return NULL;
  }

  // if a prefix is defined in config.h for line number and if there
  // is a line number passed as a parameter, add the line number to
  // the command, just after the file name.
  if (linenr && LINENR_PREFIX) {
    // line number prefix
    x = memccpy(x - 1, LINENR_PREFIX, '\0', csize);
    if (!x) {
      fputs(MEMCCPYERROR, stderr);
      return NULL;
    }
    // line number
    x = memccpy(x - 1, linenr, '\0', csize);
    if (!x) {
      fputs(MEMCCPYERROR, stderr);
      return NULL;
    }
  }

  // call system with the command
  system(cmd);

  // allocate some memory for the file reading (will be increased).
  size_t size = sizeof(char*) * MAX_SIZE;
  char* s = malloc(size);
  f = fopen(fname, "r");
  if (!f) {
    fputs("ERROR 503: error opening file.\n", stderr);
    return NULL;
  }

  // read file character by character, until it reach EOF,
  // allocating memory dynamically
  size_t index = 0;
  int c;
  while (1) {
    c = fgetc(f);

    // if character is EOF, then add zero-terminating character,
    // close the file, and remove it (it's a temporary file).
    if (c == EOF) {
      s[index] = '\0';
      fclose(f);
      remove(fname);
      return s;
    }

    // if it's not EOF, then append it to the string. if index is
    // equal to size, then reallocate memory (double)
    s[index++] = (char)c;
    if (index == size) {
      size *= 2;
      char* temp = realloc(s, size);
      if (temp == NULL) {
        fputs("ERROR 504: error reallocating memory", stderr);
        free(s);
        fclose(f);
        remove(fname);
        return NULL;
      }
      s = temp;
    }
  }

  return NULL;
#undef FILEPATH
#undef MAX_EXT_LEN
#undef FNAMESIZE
#undef MEMCCPYERROR
}

/* edit the value of something, and replace the old value by the new
 * one. */
int
edit_value(char* id,
  char* stmtselect,
  char* stmtupdate,
  char* ext,
  char* linenr)
{

  CONNECT // connnect to database
    const char* params[1] = { id };

  // send query
  PGresult* res =
    PQexecParams(conn, stmtselect, 1, NULL, params, NULL, NULL, 0);

  // check the result
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr, "ERROR 505: query failed:\n %s\n", PQerrorMessage(conn));
    PQfinish(conn);
    PQclear(res);
    return 0;
  } else if (PQntuples(res) == 0) {
    fputs("(no entry matched.)\n", stderr);
    PQfinish(conn);
    PQclear(res);
    return 0;
  }

  PQfinish(conn); // end connection, for safety

  // edit the value in $EDITOR. new value goes in s
  char* s = edit_in_editor(PQgetvalue(res, 0, 0), ext, linenr);

  PQclear(res); // clear result, not needed anymore (old value)

  if (!s) // exit if empty
    return 0;

  RECONNECT // reconnect to send new SQL
    const char* params_mod[2] = { id, s }; // id and new value
  res = PQexecParams(
    conn, stmtupdate, 2, NULL, params_mod, NULL, NULL, 0);

  free(s); // free new value, not needed anymore

  int code = 1;
  int status = PQresultStatus(res); // check status
  if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
    fprintf(stderr, "ERROR 506: query failed:\n %s\n", PQerrorMessage(conn));
    code = 0;
  }

  PQclear(res); // clear query

  update_lastedit(
    conn, id); // update LASTEDIT (for --recent ordering)

  PQfinish(conn); // end connection

  return code;
}

int
edit_file(char* filepath)
{

  // fork
  pid_t pid = fork();
  if (pid == -1) {
    // error forking
    perror("fork");
    return 0;
  }

  // subprocess
  if (pid == 0) {

    // compute size needed to allocate memory to the command
    size_t len_editor = strlen(EDITOR);
    size_t len_f_out = strlen(filepath);

    // allocate memory for the command.
    // +2 because +1 for the 0-ttrminating and +1 for the space
    // between the command (editor) and the filepath.
    char* edit_cmd = malloc(len_editor + len_f_out + 2);
    if (!edit_cmd) {
      fputs("ERROR 507: error allocating memory.\n", stderr);
      return 0;
    }

    // make the shell command
    sprintf(edit_cmd, "%s %s", EDITOR, filepath);
    char* cmd[] = { "/bin/sh", "-c", edit_cmd, NULL };

    // execute the shell command
    execvp(cmd[0], cmd);

    // free memory needed by the shell command.
    free(edit_cmd);

    // end subprocess
    exit(EXIT_SUCCESS);
  }

  // main process
  else {
    // here: copy the content of the first file to the second.
    wait(NULL);
  }

  return 1;
}
