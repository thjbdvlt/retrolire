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
#include "postpick.h"
#include "../edit.h"
#include "../tui.h"

int
command_tag_edit(char* id, char** pos, int npos)
{
  /* the select statement to get tags is pretty easy. and instead of
   * doing some complicated statement for the update, i will just do
   * everything i can from within postgresql, just regexp_replace
   * stuff and regexp_split_to_table from a list of tags (one per
   * line). */
  char ext[sizeof("txt") + 1] = "txt";
  edit_value(id,
    "select string_agg(tag, E'\\n') from tag where entry = "
    "$1::text",
    "select string_to_tags($1::text, $2::text)",
    ext,
    NULL);
  return 1;
}

int
command_edit(char* id, char** pos, int npos)
{
  char ext[sizeof("md") + 1] = "md";
  char* linenr = NULL;
  if (npos) {
    linenr = pos[0];
  }

  /* set an environment variable with current id. */
  setenv("RETROLIRE_ENTRY_ID", id, 1);

  /* edit reading notes in editor. */
  if (!edit_value(id,
        "select notes from reading where id = $1::text",
        "update reading set notes = trim($2::text, E'\n\t ') || "
        "E'\n'"
        "\nwhere id = $1::text",
        ext,
        linenr))
    return 0;

  /* update the `tsvec` column. as it requires to load the
   * dictionaries, which can take a lot of time (a little lot of
   * times: few seconds, but it's way too much), the command is sent
   * using PQsendQueryParams and not PQexecParams, so the program 
   * doesn't wait to the command output.*/
  CONNECT;
  const char* params[2] = { TEXT_SEARCH_CONFIGURATION, id };
  PQsendQueryParams(conn,
    "update reading set tsvec = to_tsvector($1::regconfig, notes)\n"
    "where id = $2::text;",
    2,
    NULL,
    params,
    NULL,
    NULL,
    0);

  PQfinish(conn);

  return 1;
}

int
command_delete(char* id, char** pos, int npos)
{

  // deleting is destructive: ask for ask_confirmation.
  if (!ask_confirmation()) {
    exit(EXIT_SUCCESS);
  }

  CONNECT                           // connect to database
    const char* params[1] = { id }; // only id is required
  PGresult* res = PQexecParams(conn,
    "delete from entry where id = $1",
    1,
    NULL,
    params,
    NULL,
    NULL,
    0);

  // check status
  int code;
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "ERROR 301: deletion failed: %s\n", PQerrorMessage(conn));
    code = 0;
  }
  code = 1;

  // free memory and exit function
  PQclear(res);
  PQfinish(conn);

  return code;
}

int
command_file(char* id, char** pos, int npos)
{
  int code;

  char* filepath = pos[0];
  char buf[PATH_MAX];
  char* filepath_real = realpath(filepath, buf);

  const char* const params[] = { id, filepath_real, NULL };

  /* connect to database. */
  CONNECT
  PGresult* res = PQexecParams(conn,
    "insert into file (entry, filepath)\n"
    "select $1, $2",
    2,
    NULL,
    params,
    NULL,
    NULL,
    0);

  /* check status. */
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "ERROR 302: insert failed: %s\n", PQerrorMessage(conn));
    code = 0;
  }

  code = 1;

  /* free memory and exit function*/
  PQclear(res);
  PQfinish(conn);
  return code;
}

int
command_tag_pick(char* id, char** pos, int npos)
{
  /* connect to database. */
  CONNECT
  /* the first operation to do is to put the entry id in the cache
   * (so it can be previewed in fzf). */
  char* params[VAL_SIZE] = { id };
  /* placeholders for tags, to be put in the query. */
  char tags_placeholders[MAX_ADD_TAGS][VAL_SIZE] = {};
  /* the tag array. */
  char tags[MAX_ADD_TAGS][VAL_SIZE] = {};
  /* insert entry id in _cache table.
   * first, insert an empty line into _cache (a table with only
   * one row, that i just update). */
  PGresult* res = PQexec(
    conn, "insert into _cache select on conflict do nothing");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,
      "ERROR 303: setting cache value failed: %s\n",
      PQerrorMessage(conn));
    return 0;
  }
  PQclear(res);
  /* then, update the row with the tags values. */
  res = PQexecParams(conn,
    "update _cache set entry = $1",
    1,
    NULL,
    (const char**)params,
    NULL,
    NULL,
    0);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,
      "ERROR 304: setting cache value failed: %s\n",
      PQerrorMessage(conn));
    return 0;
  }
  PQclear(res);
  /* end connection before forking, for safety. */
  PQfinish(conn);
  /* open a pipe: user will chose tags with fzf among already used
   * tags. the pipe is open in READING mode, because the current
   * function send nothing to it (tags are read from retrolire
   * _tag). */
  FILE* f = popen("retrolire _tag | fzf --multi "
                  " --preview=\"retrolire _cache entry ;echo {+} "
                  "| tr ' ' '\n' \""
                  " --preview-window=right,60% ",
    "r");
  /* exit function if popen failed. */
  if (!f) {
    return 0;
  }
  /* read the pipe content and split tags. */
  int ch;
  /* count tags. */
  int npar = 1;
  int i = 0;
  while (
    /* three condition could be satisfied to stop reading:
     * 1. if the end of file is reached (EOF).
     * 2. if a line is too long (more than VAL_SIZE).
     * 3. if the number of lines (tags) is more that MAX_ADD_TAGS.
     * thus, its avoid infinite looping or something similar. */
    (ch = fgetc(f)) != EOF && i < VAL_SIZE && npar < MAX_ADD_TAGS)
    if (ch == '\n') {
      tags[npar][i] = '\0';
      npar++;
      i = 0;
    } else {
      tags[npar][i] = (char)ch;
      i++;
    }
  pclose(f);
  /* const string array for the query parameters. */
  int yy;
  for (int y = 0; y < npar; y++) {
    yy = y + 1;
    /* add tags in the params array. */
    params[yy] = tags[yy];
    /* add placeholders, to make a string with them. */
    char ph[SIZE_PLACEHOLDER] = "";
    snprintf(ph, SIZE_PLACEHOLDER, "$%d", yy + 1);
    char* x =
      memccpy(tags_placeholders[y], ph, '\0', SIZE_PLACEHOLDER);
    if (!x) {
      fputs("ERROR 305: too many tags.\n", stderr);
      return 0;
    }
  }

  /* create a Stmt struct and make the tag update statement. each
   * tag is passed as a parameter and they are aggregated from
   * within postgresql using an 'array[]' type maker, then the
   * function 'to_jsonb()' convert into a jsonb array.*/
#define base_s \
  "insert into tag (entry, tag) select $1, unnest(array_remove("
  char sql_s[MAX_STMT_LEN] = base_s;
  struct Stmt sql;
  init_stmt(&sql, sql_s, MAX_STMT_LEN, sizeof(base_s));
#undef base_s
  arrayagg(&sql, tags_placeholders, npar);
  append_stmt(&sql, ", '')) on conflict do nothing");

  /* reconnect to database.*/
  RECONNECT

  /* send query. */
  res = PQexecParams(conn,
    sql.start,
    npar + 1,
    NULL,
    (const char**)params,
    NULL,
    NULL,
    0);

  /* check sent query status. */
  int code = 1;
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,
      "ERROR 306: query failed.\n%s\n",
      PQerrorMessage(conn));
    code = 0;
  }
  PQclear(res);
  PQfinish(conn);
  return code;
}

int
command_update(char* id, char** pos, int npos)
{

  // a positional argument is requirement: the field to update.
  if (!npos || !id || !pos[0])
    return 0;

  char* field = pos[0];

  CONNECT; // connect to the database.

  // initiate a Stmt for the SQL select statement
  char slct_s[MAX_STMT_LEN] = "";
  struct Stmt slct;
  init_stmt(&slct, slct_s, MAX_STMT_LEN, 0);

  // initiate a Stmt for the SQL update statement
  char slct_up_s[MAX_STMT_LEN] = "";
  struct Stmt slct_up;
  init_stmt(&slct_up, slct_up_s, MAX_STMT_LEN, 0);

  // escape field using libpq functions
  char* escaped_field =
    PQescapeIdentifier(conn, field, strlen(field));

  // exit if failed
  if (!escaped_field) {
    PQfinish(conn);
    exit(EXIT_FAILURE);
  }

  // chain concatenate the select statement
  if (!append_stmt(&slct, "select ") ||
      !append_stmt(&slct, escaped_field) ||
      !append_stmt(&slct, " from entry where id = $1"))
    return 0;

  // get the datatype of the field (and check it exists).
  const char* params[1] = { field };
  int datatype_text = 0;
  PGresult* res = PQexecParams(conn,
    "select data_type from information_schema.columns where "
    "table_name = 'entry' and column_name = $1;",
    1,
    NULL,
    params,
    NULL,
    NULL,
    0);

  if (!check_res(res, conn) || !PQntuples(res)) {
    PQfinish(conn);
    PQclear(res);
    free(escaped_field);
    return 0;
  }

  // store as an integer that said if the datatype is text or not
  datatype_text = !strcmp("text", PQgetvalue(res, 0, 0));

  PQclear(res);   // clear query result
  PQfinish(conn); // end connection

  // chain concatenate the update statemente
  if (!append_stmt(&slct_up, "update entry set ") ||
      !append_stmt(&slct_up, escaped_field) ||
      !append_stmt(&slct_up,
        (datatype_text) ? " = rtrim($2::text, '\n') where id = $1"
                        : " = $2 where id = $1"))
    return 0;

  free(escaped_field); // free memory of escaped field

  // filetype depends on the datatype
  char* ext = datatype_text ? "txt" : "json";

  // edit value
  return edit_value(id, slct_s, slct_up_s, ext, NULL);
}

int
command_cite(char* id, char** pos, int npos)
{
  // needs an id.
  if (!id)
    return 0;

  // define a basic string array, to be filled
  char* str[] = {"[@", id, " ", pos[0], "]", NULL};

  // if there is no argument (locator), close before NULL value
  if (!pos[0]) {
    str[2] = "]";
  }

  // print each value
  for (int i=0; str[i]; i++)
    fputs(str[i], stdout);

  return 1;
}

int
command_invoke(char* id, char** pos, int npos)
{
  puts(id);
  return 1;
}

int
command_refer(char* concept_id, char** pos, int npos)
{
  return get_single_value(
    concept_id, "select cite_concept($1::int)");
}

int
command_quote(char* quote_id, char** pos, int npos)
{
  return get_single_value(
    quote_id, "select quote_to_string_from_id($1::int, true)");
}

int
command_print(char* id, char** pos, int npos)
{
  show_infos(id);
  return 1;
}

int
command_open(char* id, char** pos, int npos)
{
  /* build a query and a single-element array as params. */
  const char* params[1] = { id };
  char* query = "select filepath from file where entry = $1\n"
                "union select \"URL\" from entry\n"
                "where id = $1 and \"URL\" is not null";

  /* connect to database and send query. ensure that query did not
   * failed and if it succeed, pipe out the file to the program
   * defined as $OPENER or to xdg-open. */
  CONNECT;
  PGresult* res = PQexecParams(conn,
    "select filepath from file where entry = $1\n"
    "union select \"URL\" from entry\n"
    "where id = $1 and \"URL\" is not null",
    1,
    NULL,
    params,
    NULL,
    NULL,
    0);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr, "ERROR 307: query failed:\n %s\n", PQerrorMessage(conn));
    PQclear(res);
    PQfinish(conn);
    return 0;
  } else if (PQntuples(res) == 0) {
    fputs("no file for this entry.\n", stderr);
    PQfinish(conn);
    PQclear(res);
    return 0;
  }

  update_lastedit(conn, id);

  /* end connection before fork/pipe/execvp, for safety. */
  PQfinish(conn);

  /* make the statement */
  char cmd[MAX_SIZE] = "";
  char fzf_become[VAL_SIZE] = "";
  sprintf(
    fzf_become, "become(%s {1} 2>/dev/null 1>/dev/null &)", OPENER);
  sprintf(cmd,
    "fzf -d\\n\\t --wrap --read0 --bind='enter:%s,one:%s'",
    fzf_become,
    fzf_become);

  /* write the PGresult to the file/pipe (popen). */
  FILE* f = popen(cmd, "w");
  if (!f) {
    return 0;
  }
  write_res(res, "\n\t", '\0', f);
  pclose(f);

  /* free memory and exit function. */
  PQclear(res);
  return 1;
}
