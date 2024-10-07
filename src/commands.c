#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>

#include "add_entries.h"

#include "commands.h"
#include "edit.h"
#include "pgpopen2.h"
#include "print.h"
#include "underscore.h"
#include "util.h"

/* queryp2 -- concatenate statement, pipe out and get result.
 *
 * parameters
 * ----------
 *
 * slct (struct Stmt*):
 *      the SELECt statement.
 *
 * cnd (struct Stmt*):
 *      the conditional clauses to add at the end of the Stmt.
 *
 * lastedit (int):
 *      order clause.
 *
 * npar (int):
 *      the number of parameters for the SQL.
 *
 * params (const char* const):
 *      the SQL parameters.
 *
 * sh (struct ShCmd):
 *      the Shell Command.
 *
 * dest (char*):
 *      pointer to write the result of the Shell Command.
 *
 * pick (int):
 *      if the user must pick through the Shell Command or not.
 *      (if value is 0, then it's only output to stdout).
 * */
int
queryp2(struct Stmt* slct,
  struct Stmt* cnd,
  int lastedit,
  int npar,
  const char* const* params,
  struct ShCmd* sh,
  char* dest,
  int pick)
{

  /* append the order clause to the conditional clause.*/
  append_lastedit(*cnd, lastedit);

  /* append the conditional clauses to the select statement. */
  if (append_stmt(slct, cnd->start) == 0) {
    return 0;
  }

  /* connect to the database */
  CONNECT

  /* send a query. */
  PGresult* res = PQexecParams(
    conn, slct->start, npar, NULL, params, NULL, NULL, 0);

  /* check the result.*/
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr, "query failed:\n %s\n", PQerrorMessage(conn));
    PQfinish(conn);
    PQclear(res);
    return 0;
  }

  /* if there is no result, i still exit with success code, but
   * i don't pipe the command, as it's useless. */
  else if (PQntuples(res) == 0) {
    PQfinish(conn);
    PQclear(res);
    return 0;
  }

  /* end connection before pipe or popen, for safety. */
  PQfinish(conn);

  /* fzf is called here (only if parameter 'pick' is 1, else
   * the result is only printed to stdout.)*/
  if (pick == 1)
    pgpopen2(res, "\n\t", '\0', dest, 1000, sh->args[0], sh->args);
  else
    write_res(res, "\n\t", '\0', stdout);

  /* clear and exit */
  PQclear(res);
  return 1;
}

/* import -- import entries from doi/isbn/json/bibtex.
 *
 * import entries from csl-json, bibtex, doi, isbn or edit a
 * template. that function use a shell script 'protolire', because
 * it's mainly using shell commands that i do that (some programs,
 * like pandoc or isbn_meta, and some python library like orjson and
 * psycopg).
 *
 * parameters
 * ----------
 *
 * method (char*):
 *      the method to get the refereneces: DOI, ISBN, JSON, bibtex.
 *
 * identifier (char*):
 *      the identifier (DOI, ISBN) or file path (JSON, bibtex)
 * */
int
command_add(char* method, char* identifier)
{

  if (strstarts("doi", method)) {
    command_add_doi(identifier);
  }

  else if (strstarts("isbn", method)) {
    command_add_isbn(identifier);
  }

  else if (strstarts("bibtex", method)) {
    command_add_bibtex(identifier, 0);
  }

  else if (strstarts("json", method)) {
    command_add_json(identifier, 0);
  }

  else if (strstarts("template", method)) {
    command_add_template(identifier);
  }

  else {
    fprintf(stderr,
      "not an available method for 'add': %s.\n"
      "available methods are: {doi,isbn,json,bibtex,template}.\n",
      method);
  }

  return 1;
}

/* list -- list entries that matches filters criterias.
 *
 * parameters
 * ----------
 *
 * cnd (struct Stmt*):
 *      the actual conditional clause.
 *
 * npar (int):
 *      the number of parameter for the SQL.
 *
 * params (const char*):
 *      the parameters for the SQL.
 *
 * arg (char*):
 *      the first positional argument, that determine if tags are
 *      shown.
 * */
int
list(struct Stmt* cnd,
  int npar,
  const char* params[MAXOPT],
  char* arg) // TODO: list tag 0/1 instead
{
#define FILEPATH "/tmp/retrolire.XXXXXX"
#define CMD "bat -p "
  char slct_s[MAX_SIZE] = "";
  ;
  struct Stmt slct;
  init_stmt(&slct, slct_s, MAX_SIZE, 0);
  if (append_stmt(&slct,
        (((arg != NULL) && (strstarts("tags", arg) != 0)))
          ? "select e.*, get_tags(e, '') as tags from entry e\n"
            "join reading r on r.id = e.id\n"
          : "select e.* from entry e\n"
            "join reading r on r.id = e.id\n") == 0) {
    return 0;
  };
  if (append_stmt(&slct, cnd->start) == 0) {
    return 0;
  };
  /* connect to the database and send the SELECT query.
   * if the status is failing, end function. */
  CONNECT;
  PGresult* res = PQexecParams(
    conn, slct.start, npar, NULL, params, NULL, NULL, 0);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr, "query failed:\n %s\n", PQerrorMessage(conn));
    PQclear(res);
    PQfinish(conn);
    return 0;
  }
  PQfinish(conn);
  /* open a pipe to the pager LESS. if the pipe openning succeed,
   * then use it to print entris. else, just use stdout.
   * then clear the result and close the connection. */
  int term_width = get_term_width();
  /* create a temporary file name, open the file and write the
   * result of the query. then close it. if the file cannot be
   * created or cannot be opened, exit function. */
  char fname[sizeof(FILEPATH) + 1] = FILEPATH;
  int fd = mkstemp(fname);
  if (fd == -1) {
    fputs("error creating temporary file.\n", stderr);
    return 0;
  }
  FILE* f = fdopen(fd, "w");
  if (!f) {
    fputs("error opening file.\n", stderr);
    close(fd);
    return 0;
  }
  print_result(res, f, term_width);
  /* free memory of query (not usefull anymore) and close the
   * file.*/
  PQclear(res);
  /* close file and file descriptor. */
  fclose(f);
  close(fd);
  /* generate the command `less /tmp.retrolire.XXXXXX` */
  char cmd[sizeof(FILEPATH) + sizeof(CMD) + 2] = "";
  char* x = &cmd[0];
  size_t csize = sizeof(cmd);
  x = memccpy(x, CMD, '\0', csize);
  if (!x) {
    fputs("error with memccpy (edit_in_editor).\n", stderr);
    remove(fname);
    return 0;
  }
  /* check that there is still enough space */
  size_t gap = (size_t)(x - cmd);
  if (gap >= csize) {
    remove(fname);
    return 0;
  }
  csize -= gap;
  /* copy the filename */
  x = memccpy(x - 1, fname, '\0', csize);
  if (!x) {
    fputs("error (list).\n", stderr);
    remove(fname);
    return 0;
  }
  /* call system with the less command. */
  system(cmd);
  /* remove the temporary file. */
  remove(fname);
  return 1;
#undef FILEPATH
#undef CMD
}

/* json -- list entries in JSON entries.
 *
 * parameters
 * ----------
 *
 * cnd (struct Stmt*):
 *      the actual conditional clause.
 *
 * npar (int):
 *      the number of parameter for the SQL.
 *
 * params (const char*):
 *      the parameters for the SQL.
 * */
int
json(struct Stmt* cnd, int npar, const char* params[MAXOPT])
{
  CONNECT;
  char slct_s[MAX_SIZE] = "";
  struct Stmt slct;
  init_stmt(&slct, slct_s, MAX_SIZE, 0);
  if (append_stmt(&slct,
        "select jsonb_pretty(jsonb_agg(to_csl(e)))"
        "from entry e join reading r on r.id = e.id\n") == 0) {
    return 0;
  };
  if (append_stmt(&slct, cnd->start) == 0) {
    return 0;
  };
  PGresult* res = PQexecParams(
    conn, slct.start, npar, NULL, params, NULL, NULL, 0);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr, "query failed:\n %s\n", PQerrorMessage(conn));
    PQclear(res);
    PQfinish(conn);
    return 0;
  }
  PQfinish(conn);
  if (PQntuples(res) == 0) {
    PQclear(res);
    return 0;
  }
  puts(PQgetvalue(res, 0, 0));
  PQclear(res);
  return 1;
}

/* make_stmt_quote -- make the SQL for quotes.
 *
 * parameters
 * ----------
 *
 * slct (struct Stmt*):
 *      the actual SQL, to be reset.
 *
 * sh (struct ShCmd*):
 *      the Shell Command.
 * */
int
make_stmt_quote(struct Stmt* slct, struct ShCmd* sh)
{
  /* the 'quote' command works with a different statement. */
#define STMT_QUOTE \
  "select q.id, e.id, q.quote from quote q " \
  "\njoin entry e on e.id = q.entry " \
  "\njoin reading r on r.id = e.id "

  /* reinitialise the Stmt structure with a new value. */
  reinit_stmt(slct);
  append_stmt(slct, STMT_QUOTE);

  /* append the preview command specific to quotes.*/
  append_sh(sh, "--preview-window");
  append_sh(sh, "bottom,4");
  append_sh(sh, "--wrap");
  append_sh(sh, "--preview");
  append_sh(sh, "retrolire _head {2}");

  /* if an 'id' has been written (chosen with subprocess),
   * use the command to print quotes. */
  return 1;
#undef STMT_QUOTE
}

/* make_stmt_refer - make the SQL for refer (concept).
 *
 * parameters
 * ----------
 *
 * slct (struct Stmt*):
 *      the actual SQL, to be reset.
 *
 * sh (struct ShCmd*):
 *      the Shell Command.
 * */
int
make_stmt_refer(struct Stmt* slct, struct ShCmd* sh)
{
  /* the quote action use another select statement, because it's not
   * entries that are listed but quotes. */
#define STMT_CONCEPT \
  "select c.id, c.name, c.definition, e.id from concept c " \
  "\njoin entry e on e.id = c.entry " \
  "\njoin reading r on r.id = e.id "

  /* copy the SELECT statement to the beginning of the struct Stmt.
   */
  reinit_stmt(slct);
  append_stmt(slct, STMT_CONCEPT);

  /* append the preview command specific to quotes.*/
  append_sh(sh, "--tiebreak");
  append_sh(sh, "begin");
  append_sh(sh, "--preview-window");
  append_sh(sh, "bottom,4");
  append_sh(sh, "--preview");
  append_sh(sh, "retrolire _head {4}");
  return 1;
#undef STMT_CONCEPT
}

/* make_stmt_open -- add a conditional clause for the 'open'
 * command.
 *
 * parameters
 * ----------
 *
 * cnd (struct Stmt*):
 *      the conditional clauses.
 *
 * npar (int):
 *      the number of parameters. used to determine if WHERE
 *      or AND have to be used to append the clause.
 * */
int
make_stmt_open(struct Stmt* cnd, int npar)
{
  if (!append_stmt(cnd, WHEREAND(npar))) {
    return 0;
  };
  if (!append_stmt(cnd,
        "(select exists (select 1 from file "
        "where entry = e.id) or \"URL\" is not null)\n")) {
    return 0;
  };
  return 1;
}

/* command_tag_edit -- edit tag in editor.
 *
 * parameters
 * ----------
 *
 *  id (char*):
 *      the entry id.
 *
 *  pos (char*[MAXPOS]):
 *      positional arguments.
 *
 *  npos (int):
 *      number of positional arguments.
 * */
int
command_tag_edit(char* id, char* pos[MAXPOS], int npos)
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
    ext);
  return 1;
}

/* command_edit -- edit reading notes in editor.
 *
 * parameters
 * ----------
 *
 *  id (char*):
 *      the entry id.
 *
 *  pos (char*[MAXPOS]):
 *      positional arguments.
 *
 *  npos (int):
 *      number of positional arguments.
 * */
int
command_edit(char* id, char* pos[MAXPOS], int npos)
{
  char ext[sizeof("md") + 1] = "md";
  return edit_value(id,
    "select notes from reading where id = $1::text",
    "update reading set notes = trim($2::text, E'\n\t ') || E'\n'"
    "\nwhere id = $1::text",
    ext);
}

/* command_open -- open an URL/file attached to an entry.
 *
 * parameters
 * ----------
 *
 *  id (char*):
 *      the entry id.
 *
 *  pos (char*[MAXPOS]):
 *      positional arguments.
 *
 *  npos (int):
 *      number of positional arguments.
 * */
int
command_open(char* id, char* pos[MAXPOS], int npos)
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
    fprintf(stderr, "query failed:\n %s\n", PQerrorMessage(conn));
    PQclear(res);
    PQfinish(conn);
    return 0;
  } else if (PQntuples(res) == 0) {
    fputs("no file for this entry.\n", stderr);
    PQfinish(conn);
    PQclear(res);
    return 0;
  }

  /* end connection before fork/pipe/execvp, for safety. */
  PQfinish(conn);

  /* make the statement */
  char cmd[MAX_SIZE] = "";
  char fzf_become[VAL_SIZE] = "";
  sprintf(
    fzf_become, "become(%s {1} 2>/dev/null 1>/dev/null &)", opener);
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

/* command_refer -- quote a concept with its reference.
 *
 * parameters
 * ----------
 *
 *  id (char*):
 *      the entry id.
 *
 *  pos (char*[MAXPOS]):
 *      positional arguments.
 *
 *  npos (int):
 *      number of positional arguments.
 * */
int
command_refer(char* concept_id, char* pos[MAXPOS], int npos)
{
  return get_single_value(
    concept_id, "select cite_concept($1::int)");
}

/* command_quote -- get a quote formatted for pandoc.
 *
 * parameters
 * ----------
 *
 *  id (char*):
 *      the entry id.
 *
 *  pos (char*[MAXPOS]):
 *      positional arguments.
 *
 *  npos (int):
 *      number of positional arguments.
 * */
int
command_quote(char* quote_id, char* pos[MAXPOS], int npos)
{
  return get_single_value(
    quote_id, "select quote_to_string_from_id($1::int)");
}

/* command_delete -- delete an entry.
 *
 * parameters
 * ----------
 *
 *  id (char*):
 *      the entry id.
 *
 *  pos (char*[MAXPOS]):
 *      positional arguments.
 *
 *  npos (int):
 *      number of positional arguments.
 * */
int
command_delete(char* id, char* pos[MAXPOS], int npos)
{
  /* connect to database. */
  CONNECT
  /* send query*/
  const char* params[1] = { id };
  PGresult* res = PQexecParams(conn,
    "delete from entry where id = $1",
    1,
    NULL,
    params,
    NULL,
    NULL,
    0);
  /* check status. */
  int code;
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "deletion failed: %s\n", PQerrorMessage(conn));
    code = 0;
  }
  code = 1;
  /* free memory and exit function*/
  PQclear(res);
  PQfinish(conn);
  return code;
}

/* command_file -- attach a file to an entry.
 *
 * parameters
 * ----------
 *
 *  id (char*):
 *      the entry id.
 *
 *  pos (char*[MAXPOS]):
 *      positional arguments.
 *
 *  npos (int):
 *      number of positional arguments.
 * */
int
command_file(char* id, char* pos[MAXPOS], int npos)
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
    fprintf(stderr, "insert failed: %s\n", PQerrorMessage(conn));
    code = 0;
  }

  code = 1;

  /* free memory and exit function*/
  PQclear(res);
  PQfinish(conn);
  return code;
}

/* command_tag_pick -- add tags to an entry using fzf.
 *
 * parameters
 * ----------
 *
 *  id (char*):
 *      the entry id.
 *
 *  pos (char*[MAXPOS]):
 *      positional arguments.
 *
 *  npos (int):
 *      number of positional arguments.
 * */
int
command_tag_pick(char* id, char* pos[MAXPOS], int npos)
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
      "setting cache value failed: %s\n",
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
      "setting cache value failed: %s\n",
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
  FILE* f = popen("retrolire _ltags | fzf --multi "
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
    char ph[PH] = "";
    snprintf(ph, PH, "$%d", yy + 1);
    char* x = memccpy(tags_placeholders[y], ph, '\0', PH);
    if (!x) {
      fputs("too many tags.\n", stderr);
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
      "error sending query to database:\n%s\n",
      PQerrorMessage(conn));
    code = 0;
  }
  PQclear(res);
  PQfinish(conn);
  return code;
}

/* command_update -- update a field of an entry.
 *
 * parameters
 * ----------
 *
 *  id (char*):
 *      the entry id.
 *
 *  pos (char*[MAXPOS]):
 *      positional arguments.
 *
 *  npos (int):
 *      number of positional arguments.
 * */
int
command_update(char* id, char* pos[MAXPOS], int npos)
{
  /* a positional argument is requirement: the field to update. */
  if (npos == 0) {
    return 0;
  }
  char* field = pos[0];

  /* connect to the database. */
  CONNECT;

  /* initiate a Stmt for the SQL select statement. */
  char slct_s[MAX_STMT_LEN] = "";
  struct Stmt slct;
  init_stmt(&slct, slct_s, MAX_STMT_LEN, 0);

  /* initiate a Stmt for the SQL update statement. */
  char slct_up_s[MAX_STMT_LEN] = "";
  struct Stmt slct_up;
  init_stmt(&slct_up, slct_up_s, MAX_STMT_LEN, 0);

  /* escape field using libpq functions. */
  char* escaped_field =
    PQescapeIdentifier(conn, field, strlen(field));
  if (escaped_field == NULL) {
    PQfinish(conn);
    exit(EXIT_FAILURE);
  }

  /* chain concatenate the select statement. */
  append_stmt(&slct, "select ");
  append_stmt(&slct, escaped_field);
  append_stmt(&slct, " from entry where id = $1");

  /* check that field exists. and if it exists, look if its datatype
   * is text or something else, because if it's text, i remove
   * trailing newline. */
  const char* params[1] = { field };
  int datatype_test = 0;
  PGresult* res = PQexecParams(conn,
    "select data_type from information_schema.columns where "
    "table_name = 'entry' and column_name = $1;",
    1,
    NULL,
    params,
    NULL,
    NULL,
    0);
  if (PQresultStatus(res) != PGRES_TUPLES_OK ||
      PQntuples(res) == 0) {
    PQfinish(conn);
    PQclear(res);
    free(escaped_field);
    fputs("invalid field.\n", stderr);
    exit(EXIT_FAILURE);
  }

  /* store as an integer that said if the datatype is text or not.
   */
  datatype_test = strcmp("text", PQgetvalue(res, 0, 0));

  /* clear query and end connection. */
  PQclear(res);
  PQfinish(conn);

  /* chain concatenate the update statemente. */
  append_stmt(&slct_up, "update entry set ");
  append_stmt(&slct_up, escaped_field);
  append_stmt(&slct_up,
    (datatype_test == 0) ? " = rtrim($2::text, '\n') where id = $1"
                         : " = $2 where id = $1");

  /* free the memory for the escaped_field. */
  free(escaped_field);

  /* edit the value. */
  char ext[sizeof(".txt") + 1] = ".txt";
  return edit_value(id, slct_s, slct_up_s, ext);
}

/* command_cite -- cite an entry (get its ID).
 *
 * parameters
 * ----------
 *
 *  id (char*):
 *      the entry id.
 *
 *  pos (char*[MAXPOS]):
 *      positional arguments.
 *
 *  npos (int):
 *      number of positional arguments.
 * */
int
command_cite(char* id, char* pos[MAXPOS], int npos)
{
  puts(id);
  return 1;
}

/* command_print -- print informations about an entry.
 *
 * parameters
 * ----------
 *
 *  id (char*):
 *      the entry id.
 *
 *  pos (char*[MAXPOS]):
 *      positional arguments.
 *
 *  npos (int):
 *      number of positional arguments.
 * */
int
command_print(char* id, char* pos[MAXPOS], int npos)
{
  return preview(id);
}

/* check_command_name -- check that the command is a register
 * command.
 *
 * parameters
 * ----------
 *
 * cmd (char*):
 *      check that a string is a registered commnand.
 * */
int
check_command_name(char* cmd)
{
  // list of all available commands
  const char* available_cmds[] = { "list",
    "add",
    "json",
    "cite",
    "delete",
    "refer",
    "print",
    "edit",
    "quote",
    "update",
    "open",
    "file",
    "tag",
    NULL };
  // iterate over the commands names. if the command passed as
  // argument starts with the same letter than a command, check that
  // is a part of that command.
  for (int i = 0;; i++) {
    if (available_cmds[i] == NULL) {
      return 0;
    }
    const char* s = available_cmds[i];
    if (s[0] == cmd[0]) {
      if (!strstarts(s, cmd)) {
        return 0;
      }
      return 1;
    }
  }
  return 1;
}
