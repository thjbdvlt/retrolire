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
#include "underscore.h"
#include "../tui.h"

int
ucommand_head(char* id, char** __, int ___)
{
  /* connect to the database. then, define an array for parameters,
   * and a string for statement.
   * */
  CONNECT
  const char* params[] = { id };
  PGresult* res = PQexecParams(conn,
    "select * from _head where id = $1",
    1,
    NULL,
    params,
    NULL,
    NULL,
    0);
  /* this function is used for previewing, so i don't want it to
   * print long and complete messages. but i still handle errors and
   * exit function in case there is a problem. */
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fputs("ERROR 401: query failed.\n", stderr);
    PQclear(res);
    PQfinish(conn);
    return 0;
  } else if (PQntuples(res) == 0) {
    PQfinish(conn);
    PQclear(res);
    return 0;
  }
  int n_fields = PQnfields(res);
  for (int i = 0; i < n_fields; i++) {
    /* it's not needed to check for NULL because NULL values are
     * returned as empty strings: so there is no problem to just
     * 'puts' them out. */
    puts(PQgetvalue(res, 0, i));
  }
  PQclear(res);
  PQfinish(conn);
  return 1; 
}

int
ucommand_schema(char* _, char** __, int ___)
{
  return system(
    "cat /usr/share/retrolire/schema.sql 2>/dev/null "
    "|| echo 'schema not found (reinstall retrolire).'");
}

int
ucommand_lemmes(char* _, char** __, int ___)
{
  CONNECT
  PGresult* res = PQexec(conn,
    "select distinct unnest(tsvector_to_array(tsvec)) from reading");
  /* this function is used for previewing, so i don't want it to
   * print long and complete messages. but i still handle errors and
   * exit function in case there is a problem. */
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fputs("ERROR 402: query failed.\n", stderr);
    PQclear(res);
    PQfinish(conn);
    return 0;
  } else if (PQntuples(res) == 0) {
    PQfinish(conn);
    PQclear(res);
    return 0;
  }
  int n_fields = PQnfields(res);
  int n_rows = PQntuples(res);
  for (int i = 0; i < n_rows; i++) {
    /* it's not needed to check for NULL because NULL values are
     * returned as empty strings: so there is no problem to just
     * 'puts' them out. */
    puts(PQgetvalue(res, i, 0));
  }
  PQclear(res);
  PQfinish(conn);
  return 1;
}


int
ucommand_preview(char* id, char** __, int ___)
{
  return (id && preview(id));
}

int
ucommand_tags(char* _, char** __, int ___)
{
  return list_tags();
}

int
ucommand_authors(char* _, char** __, int ___)
{
  return list_authors();
}

int
ucommand_list_var(char* var, char** __, int ___)
{
  CONNECT
  // TODO: dynamically generate 
  PGresult* res = PQexec(conn,
  "select distinct author from entry");
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fputs("ERROR 403: query failed.\n", stderr);
    PQclear(res);
    PQfinish(conn);
    return 0;
  } else if (PQntuples(res) == 0) {
    PQfinish(conn);
    PQclear(res);
    return 0;
  }
  int n_fields = PQnfields(res);
  int n_rows = PQntuples(res);
  for (int i = 0; i < n_rows; i++) {
    puts(PQgetvalue(res, i, 0));
  }
  PQclear(res);
  PQfinish(conn);
  return 1;
}

int
ucommand_fields(char* _, char** __, int ___)
{
  return list_fields();
}

int
ucommand_cache(char* id, char** __, int ___)
{
  return preview_cache_entry();
}

int
underscore_input(char* id, char** argv, int argc)
{

  if (!id)
    return 0;

#define INPUTSIZE 100
  char input[INPUTSIZE] = "";
  size_t i_len = read_command(id, input, INPUTSIZE);
#undef INPUTSIZE

  // trim the input string.
  char* x = trim(input, i_len);
  if (x[0] == '\0') {
    exit(EXIT_FAILURE);
  }

  struct RetroCmd cmd = parse_cmd_name_no_default(x);
  if (cmd.id == CMD_UNKNOWN) {
    fputs("unknown command: ", stderr);
    fputs(input, stderr);
    fputs("\n", stderr);
    exit(EXIT_FAILURE);
  }

  switch (cmd.id) {
    case CMD_EDIT:
    case CMD_OPEN:
    case CMD_TAG_PICK:
    case CMD_TAG_EDIT:
    case CMD_DELETE:
    case CMD_PRINT:
      (*cmd.func)(id, argv, argc);
      break;

    // subpicker functions
    case CMD_QUOTE:
    case CMD_CITE:
    case CMD_REFER:
      underscore_subpicker(id, &cmd);
      break;

    default:
      exit(EXIT_FAILURE);
      break;
  }
  return 1;
}

int
underscore_subpicker(const char* id, struct RetroCmd* cmd)
{

  // shell options
  const char* user_sh_opts[MAXPOS] = { NULL };

  // conditional clauses
  char cnd_s[] = "where e.id = $1::text";
  const char* params[] = {id};

  // fzf options
  struct ShCmd sh = {
    0,
    MAXPOS,
    user_sh_opts,
  };

  struct Stmt cnd = {
    .total = MAX_SIZE,
    .remain = MAX_SIZE,
    .start = &cnd_s[0],
    .end = &cnd_s[1],
  };

  struct arguments a = {
    // no --var options
    .nvar = 0,
    .varvalues = NULL,
    .maxvalvalues = 1,

    // entry id
    .npar = 1,
    .params = params, // one: entry id
    .ncnd = 1,

    // no positional argument
    .npos = 0,
    .pos = NULL,
    .maxposarg = 1,

    // no --last or --recent
    .last = 0,

    // command specific (cmd is func parameter)
    .howprint = OUT_UNDEFINED,
    .cmd = *cmd,
    .sh = &sh,
    .cnd = &cnd,

    // no search
    .search = NULL,
    .maxparams = MAXOPT,

  };

  return queryout(&a);
}

void
putarg(const char*s, FILE* f)
{

  int i = 0;
  char c = s[i];

  // flag (--option, -o) needs no escaping
  if (c == '-') {
    fputs(s, f);
    fputc(' ', f);
    return;
  }

  // option argument must be quoted.
  fputc('"', f);

  while (c != '\0') {
    c = s[i];
    switch (c) {

      // no newlines, no tabs
      case '\n':
        fputs("\\n", f);
        break;
      case '\t':
        fputs("\\t", f);
        break;
      case '\r':
        fputs("\\r", f);
        break;

      // escape quotes (because it's quoted)
      case '\"':
        fputs("\\\"", f);
        break;

      // escape backslash
      case '\\':
        fputs("\\\\", f);
        break;

      case '\0':
        break;

      // put anything else just as it is
      default:
        fputc(c, f);
    }

    i++;
  }

  // add a space before
  fputs("\" ", f);
}

int ucommand_fzfopts (char* cmdname, char** __, int ___)
{

  const char** arrayarray[] = {
    &fzf_base[1], // skip first argument, which is "fzf".
    NULL, // for command-specific options
    NULL, // for multiline/singleline options
    NULL, // NULL terminates the array
  };
  const char** array;
  const char* value;

  // if there a subcommand is submitted as first argument, add
  // the options relatives to this subcommand
  if (cmdname) {
    struct RetroCmd cmd = parse_cmd_name_no_default(cmdname);
    if (cmd.id != CMD_UNKNOWN && cmd.shellopts) {
      arrayarray[1] = cmd.shellopts;
      arrayarray[2] = cmd.oneline ? fzf_oneline : fzf_multiline;
    }
  }

  // nested loop, to iterate over all values in all arrays.
  for (int i=0;; i++) {
    array = arrayarray[i];
    if (!array)
      break;
    for (int y=0;; y++) {
      value = array[y];
      if (!value)
        break;
      putarg(value, stdout);
    }
  }

  fputc('\n', stdout);

  return 1;
}

int
do_underscore(int argc, char* argv[])
{
  char* cmdargv[VAL_SIZE] = {};
  int i = 0;
  int cmdargc = 0;
  struct RetroCmd cmd;

  // there are two types of underscore commands:
  //
  // single underscore prefix:
  //    : commands for preview / completion
  //
  //    `retrolire _tags`
  //    `retrolire _fields`
  //    `retrolire _preview antin2008a`
  //
  // double underscore prefix:
  //    : normal commands (editor integration)
  //
  //    `retrolire __edit antin2008a`
  //    `retrolire __quote quintane2020`
  int idx = 0;
  if (argv[1][1] == '_')
    idx = 2;

  // needs at least one character, that must not be '_' or it will
  // match any underscore command name.
  if (argv[1][2] == '\0' || argv[1][2] == '_')
    return 0;

  // parse the command name.
  cmd = parse_cmd_name_no_default(&argv[1][idx]);

  // exit if unknown command or a command that does not have a
  // function
  if (cmd.id == CMD_UNKNOWN || !cmd.func)
    return 0;

  // create the string array for command arguments (and count argc)
  char* id = argv[2];

  // move the arguments
  for (int i = 3, cmdargc = 0; i < argc; i++, cmdargc++)
    cmdargv[cmdargc] = argv[i];

  // execute command
  cmd.func(id, cmdargv, cmdargc + 1);

  return EXIT_SUCCESS;
}
