#include "edit.h"
#include "sizes.h"
#include "util.h"
#include <postgresql/libpq-fe.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

char*
edit_in_editor(char* value, char* ext)
{
#define FILEPATH "/tmp/retrolire.XXXXXX."
#define MAX_EXT_LEN 10
  /* the first part of the command deals with the extension of the
   * file. that extension is important because syntax highlight will
   * depends on it in $EDITOR. typically, i want markdown for the
   * notes, but i don't want markdown for the JSON fields, neither
   * for tags, because tags are one tag a line, and a tag could be
   * '_important' and with markdown highlight it would be a mess.
   *
   * (the extension cannot be longer than 10 characters.) */
  size_t extlen = strnlen(ext, MAX_EXT_LEN);
  if (extlen == MAX_EXT_LEN) {
    fputs("file extension too long.\n", stderr);
    return NULL;
  }
  /* add 1 to extension, for the dot. */
  extlen++;
  /* copy the filepath (without extension) to the string. */
  char fname[sizeof(FILEPATH) + 1] = "";
  char* p = memccpy(fname, FILEPATH, '\0', sizeof(FILEPATH) + 1);
  if (!p) {
    fputs("memccpy error (edit_in_editor 1)\n", stderr);
    return NULL;
  }
  /* copy the extension to the string. */
  p = memccpy(p - 1, ext, '\0', MAX_EXT_LEN);
  if (!p) {
    fputs("memccpy error (edit_in_editor 2)\n", stderr);
    return NULL;
  }
  /* make the command. */
  char cmd[sizeof(FILEPATH) + MAX_EXT_LEN + sizeof(editor) + 4] =
    "";
  if (mkstemps(fname, (int)extlen) == -1) {
    fputs("error creating temporary file.\n", stderr);
    return NULL;
  }
  /* open the file.
   * write content of value into file.
   * close file. */
  FILE* f;
  f = fopen(fname, "w");
  if (!f) {
    fputs("error opening file.\n", stderr);
    return NULL;
  }
  fputs(value, f);
  fclose(f);
  /* create a command to be executed. */
  char* x = &cmd[0];
  // int csize = sizeof(cmd) + 1;
  size_t csize = sizeof(cmd);
  x = memccpy(x, editor, '\0', csize);
  if (!x) {
    fputs("error with memccpy (edit_in_editor).\n", stderr);
    return NULL;
  }
  x[-1] = ' ';
  size_t gap = (size_t)(x - cmd);
  if (gap >= csize) {
    return NULL;
  }
  csize -= gap;
  x = memccpy(x, fname, '\0', csize);
  if (!x) {
    fputs("error with memccpy (edit_in_editor).\n", stderr);
    return NULL;
  }
  /* call system with the command */
  system(cmd);

  /* allocate some memory for the file reading (will be increased).
   */
  size_t size = sizeof(char*) * MAX_SIZE;
  char* s = malloc(size);
  f = fopen(fname, "r");
  if (!f) {
    fputs("error opening file.\n", stderr);
    return NULL;
  }
  /* read file character by character, until it reach EOF,
   * allocating memory dynamically. */
  size_t index = 0;
  int c;
  while (1) {
    c = fgetc(f);
    /* if character is EOF, then add zero-terminating character,
     * close the file, and remove it (it's a temporary file). */
    if (c == EOF) {
      s[index] = '\0';
      fclose(f);
      return s;
    }
    /* if it's not EOF, then append it to the string. if index is
     * equal to size, then reallocate memory (double). */
    s[index++] = (char)c;
    if (index == size) {
      size *= 2;
      char* temp = realloc(s, size);
      if (temp == NULL) {
        fputs("error reallocating memory", stderr);
        free(s);
        fclose(f);
        remove(fname);
        return NULL;
      }
      s = temp;
    }
  }
#undef FILEPATH
#undef MAX_EXT_LEN
}

/* edit the value of something, and replace the old value by the new
 * one. */
int
edit_value(char* id, char* stmtselect, char* stmtupdate, char* ext)
{
  /* connect to the database. then make the query params array and
   * send the query. if the query fails, exit the function because a
   * return is required to be edited. plus if there is an error or
   * no row returned, there is probably no row to send the edited
   * value. */
  CONNECT
  const char* params[1] = { id };
  PGresult* res =
    PQexecParams(conn, stmtselect, 1, NULL, params, NULL, NULL, 0);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr, "query failed:\n %s\n", PQerrorMessage(conn));
    PQfinish(conn);
    PQclear(res);
    return 0;
  } else if (PQntuples(res) == 0) {
    fputs("(no entry matched.)\n", stderr);
    PQfinish(conn);
    PQclear(res);
    return 0;
  }
  /* end connection before using system in `edit_in_editor`, because
   * else it would not be safe. */
  PQfinish(conn);
  /* edit the value in $EDITOR. new value goes in s. */
  char* s = edit_in_editor(PQgetvalue(res, 0, 0), ext);
  /* clear query because the original value is not needed anymore as
   * i have the edited (new) value. */
  PQclear(res);
  /* if nothing returned by the previous function exit function. */
  if (s == NULL) {
    return 0;
  }
  /* restart new connection, build the parameters to the query, and
   * send the query.*/
  RECONNECT
  const char* params_mod[2] = { id, s };
  res = PQexecParams(
    conn, stmtupdate, 2, NULL, params_mod, NULL, NULL, 0);
  /* once i have send the query, the new value is not needed
   * anymore. so i free it. */
  free(s);
  /* check that the query has been successfully sent.*/
  int code = 1;
  int status = PQresultStatus(res);
  if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
    fprintf(stderr, "query failed:\n %s\n", PQerrorMessage(conn));
    code = 0;
  }
  /* clear everything and exit function.*/
  PQclear(res);
  PQfinish(conn);
  return code;
}
