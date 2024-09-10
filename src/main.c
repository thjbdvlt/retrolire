/*
 retrolire

 command line bibliography management with PostgreSQL and FZF.

 usage:
   retrolire COMMAND [OPTIONS ...]

  COMMANDS
   edit, cite, list, json, open, update, file, add
   delete, refer, quote, tag, print

  FILTER OPTIONS
   -v --var KEY=PATTERN
   -v --tag TAG
   -s --search PATTERN
   -q --quote PATTERN
   -c --concept PATTERN

  HISTORY
   -l --last
   -r --recent

  FZF OPTIONS
   -e --exact
   -p --preview

  MISC
   -o --output
   -T --showtags
   -i --id ID
 */

#include <argp.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "sizes.h"
#include "underscore.h"
#include "util.h"

const char* argp_program_version = "0.1.0";
static char args_doc[] = "COMMAND [...]";
static char doc[] =
  "\nbibliography management with postgresql and fzf.\n"
  "\navailable commands:\n"
  "  list\n"
  "  edit\n"
  "  cite\n"
  "  open\n"
  "  add METHOD {IDENTIFIER|FILE}\n"
  "  tag\n"
  "  json\n"
  "  file FILE\n"
  "  delete\n"
  "  refer\n"
  "  print\n"
  "  quote\n"
  "  update\n";

// clang-format off
static struct argp_option options[] = {
  // long -- short -- argname -- optional -- doc -- group
  { 0, 0, NULL, OPTION_DOC,  "filters:", 1},
  { "var", 'v', "field=regex", 0, "field-value search " , 0},
  { "tag", 't', "tag", 0, "filter entries with a tag", 0},
  { "search", 's', "regex", 0, "search pattern in reading notes", 0 },
  { "quote", 'q', "regex", 0, "search pattern in quotes", 0 },
  { 0, 0, NULL, OPTION_DOC,  "selection (fzf):", 2},
  { "exact", 'e', NULL, 0, "no fuzzy matching" , 0},
  { "preview", 'p', NULL, 0, "show entry infos, files and notes." , 0},
  { "showtags", 'T', NULL, 0, "list tags for in the fzf picker" , 0},
  { 0, 0, NULL, OPTION_DOC,  "history:", 3},
  { "last", 'l', NULL, 0, "select the last selected entry" , 0},
  { "recent", 'r', NULL, 0, "order entries by recent editing", 0 },
  { 0, 0, NULL, OPTION_DOC,  "misc:", 4},
  { "id", 'i', "id", 0, "specified the entry id " , 0},
  { "output", 'o', NULL, 0, "do not interactively pick an id" , 0},
  { 0 }
};
// clang-format on

// the structure for argument parsing
struct arguments
{
  // incremental int values:
  // - for parameters (SQL).
  // - for field=values.
  // - for number of conditional clauses.
  int npar, nvar, ncnd, npos;
  // - ON/OFF values
  int lastedit, showtags;
  int pick;
  char* command;     // first positional argument is the command
  char* pos[MAXPOS]; // other positional arguments (files, etc.)
  const char* params[MAXOPT]; // parameters for the SQL
  char* varvalues[MAXOPT];    // values from the --var option
  struct Stmt* cnd;           // the conditional clause
  struct ShCmd* sh;           // the Shell Command
};

// add a conditional clause and a parameter.
static void
_add_cnd(struct arguments* arguments, char* arg, char* s1, char* s2)
{
  // concatenate the s1/s2 (+placeholder) to the conditional clause.
  if (!cat_cnd(arguments->cnd, s1, s2, arguments->npar)) {
    exit(EXIT_FAILURE);
  };
  // update the values in the struct
  arguments->params[arguments->npar] = arg;
  arguments->npar++;
  arguments->ncnd++;
};

error_t
parse_opt(int key, char* arg, struct argp_state* state)
{
  struct arguments* arguments = state->input;

  switch (key) {
    case 'e': // exact
      append_sh(arguments->sh, "--exact");
      break;
    case 'p': // preview
      append_sh(arguments->sh, "--preview");
      append_sh(arguments->sh,
        "retrolire list -i {1}; retrolire _preview {1} | bat -l md "
        "-p --color=always");
      append_sh(arguments->sh, "--preview-window");
      append_sh(arguments->sh, preview_pos);
      break;
    case 'T': // showtags
      arguments->showtags = 1;
      break;
    case 'l': // last
      arguments->lastedit = 1;
      break;
    case 'r': // recent
      arguments->lastedit = 2;
      break;
    case 'o': // recent
      arguments->pick = 0;
      break;

    case 'v': // var, e.g."author=antin", are processed later
      arguments->varvalues[arguments->nvar] = arg;
      arguments->nvar++;
      break;

    case 't': // tag
      _add_cnd(arguments,
        arg,
        "exists (select 1 from tag t where t.entry = "
        "e.id and t.tag = ",
        ")");
      break;

    case 's': // search in notes
      _add_cnd(
        arguments, arg, "regexp_like(r.notes, ", "::text, 'i') ");
      break;

    case 'q': // quote
      _add_cnd(arguments,
        arg,
        "(select exists (select 1 from quote where entry = "
        "e.id "
        "and regexp_like(quote, ",
        "::text, 'i'))) ");
      break;

    case 'c': // concept
      _add_cnd(arguments,
        arg,
        "(select exists (select 1 from concept where entry "
        "= "
        "e.id "
        "and regexp_like(name, ",
        "::text, 'i'))) ");
      break;

    case 'i': // id
      _add_cnd(arguments, arg, "e.id = ", "::text");
      break;

    case ARGP_KEY_ARG: // positional argument
      // first positional argument is COMMAND
      if (state->arg_num == 0) {
        arguments->command = arg;
      }

      // there is a limit to positional arguments
      else if (state->arg_num >= MAXPOS) {
        argp_usage(state);
      }

      // add positional arguments
      else {
        unsigned int npos = (state->arg_num) - 1;
        arguments->pos[npos] = arg;
        arguments->npos++;
      }
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

// the object used to parse arguments with argp.
static struct argp
  argp = { options, parse_opt, args_doc, doc, NULL, NULL, NULL };

int
main(int argc, char** argv)
{
  // before argument parsing, check if the first character of the
  // first argument is an underscore (_). if it is, it's a special
  // command.
  if (argv[1] != NULL && argv[1][0] == '_') {
    do_underscore(argv);
    exit(EXIT_SUCCESS);
  }

  // initiate a struct for arguments.
  struct arguments a;

  // int arguments nearly all start at 0.
  // - incremental int.
  a.npar = 0;
  a.nvar = 0;
  a.ncnd = 0;
  a.npos = 0;
  // - on/off int
  a.showtags = 0;
  // - enum (0, 1, 2)
  a.lastedit = 0;

  // only exception is 'pick'. by default it's 1, and the option -o
  // turns it off to 0 if the query has not to be passed to FZF.
  a.pick = 1;

  // default command is defined in config.h
  a.command = default_command;

  // initiate NULL values for positional arguments
  for (int i = 0; i < MAXPOS; i++)
    a.pos[i] = NULL;
  char** pos = a.pos;

  // conditional clause
  struct Stmt cnd;
  char cnd_s[SIZE_CND] = "";
  init_stmt(&cnd, cnd_s, SIZE_CND, 0);
  a.cnd = &cnd;

  // the command for the Shell Command (pseudo-popen2). it's made of
  // an array of strings, so it will be passed as argv to execvp.
  char* pick_command[] = { "fzf",
    "--read0",
    "-d",
    "\\n\\t",
    "--tiebreak",
    "begin",
    "--bind",
    "enter:become(echo -n {1}),one:become(echo -n {1})",
    "-0",
    NULL,   // --exact
    NULL,   // --preview (1): --preview
    NULL,   // --preview (1b):(preview command)
    NULL,   // --preview (2): --preview-window
    NULL,   // --preview (2b): (window position)
    NULL }; // the args in execvp must be NULL-terminated
  struct ShCmd sh;
  init_sh(&sh, pick_command);
  a.sh = &sh;

  // parse arguments
  argp_parse(&argp, argc, argv, 0, 0, &a);

  CONNECT;

  // add 'field=values' to conditional clause and to params.
  // - field is escaped and concatenated in the conditional clause.
  // - value goes in params.
  for (int i = 0; i < a.nvar; i++) {
    int npar = a.npar;
    char* value =
      parse_key_value(conn, &cnd, a.npar, a.varvalues[i]);
    if (!value) {
      PQfinish(conn);
      exit(EXIT_FAILURE);
    }
    a.params[a.npar] = value;
    a.npar++;
  }
  // end connection, not needed anymore because not all commands
  // requires it (it was only usefull for field escaping).
  PQfinish(conn);

  // initiate a Stmt for the default SELECT statement.
  struct Stmt slct;
  char slct_s[MAX_SIZE] = BASE_STMT;
  init_stmt(&slct, slct_s, MAX_SIZE, sizeof(BASE_STMT));

  // a buffer to store the ID (entry ID, quote ID or concept ID).
  char id[VAL_SIZE] = "";
  char* cmd = a.command;

  // check the command name.
  if (!check_command_name(cmd)) {
    fputs("unknown command.\n", stderr);
    exit(EXIT_FAILURE);
  }

  // define a function pointer.
  int (*func)(char*, char* [MAXPOS], int) = NULL;

  /* commands
   *
   * assign the function to the function pointer 'func', if it's one
   * of the main commands which require to pick an ID. for some
   * commands (e.g. "quote"), it's needed to update the SQL
   * statement. some other commands do not needs to pick an ID,
   * these are entirely processed here. */
  switch (cmd[0]) {

    case 'p': // print
      func = command_print;
      break;

    case 'c': // cite
      func = command_cite;
      break;

    case 'd': // delete
      func = command_delete;
      break;

    case 'e': // edit
      func = command_edit;
      break;

    case 'q': // quote
      func = command_quote;
      if (!make_stmt_quote(&slct, &sh))
        exit(EXIT_FAILURE);
      break;

    case 'r': // refer
      func = command_refer;
      if (!make_stmt_refer(&slct, &sh))
        exit(EXIT_FAILURE);
      break;

    case 'f': // file
      func = command_file;
      if (!check_file(pos[0]))
        exit(EXIT_FAILURE);
      break;

    case 'o': // open
      func = command_open;
      if (!make_stmt_open(&cnd, a.npar))
        exit(EXIT_FAILURE);
      break;

    case 'u': // update
      func = command_update;
      if (!check_field(pos[0])) {
        fprintf(stderr, "unknown field '%s'.\n", pos[0]);
        exit(EXIT_FAILURE);
      }
      break;

    case 't': // tag
      /* there are two commands for tag modification, one for
       * editing the tag list in the editor, and another one to pick
       * some tags with fzf. */
      if (pos[0] && strstarts("pick", pos[0])) {
        // TODO: repair this, because ID is not set.
        if (!cache_entry(id))
          exit(EXIT_FAILURE);
        func = command_tag_pick;
      } else {
        func = command_tag_edit;
      }
      break;

    case 'l': // list
      /* the command 'list' show informations about all entris
       * that match search criterias (-v, -q, -s, -t, -i, -l).
       * the positional argument 'tags' can be used to print
       * tags altogether with other metadata. */
      append_lastedit(cnd, a.lastedit);
      if (!list(&cnd, a.npar, a.params, pos[0]))
        exit(EXIT_FAILURE);
      else
        exit(EXIT_SUCCESS);
      break;

    case 'j': // json
      /* 'json' command is like 'list' but in CSL-JSON format. */
      if (!json(&cnd, a.npar, a.params))
        exit(EXIT_FAILURE);
      else
        exit(EXIT_SUCCESS);
      break;

    case 'a': // add
      /* command 'import' redirect arguments to the bash script
       * 'protolire', which get the metadata from doi/isbn or let
       * the user edit a template, or read bibliography from file
       * and put that bibliography into the database */
      if (!import(pos[0], pos[1]))
        exit(EXIT_FAILURE);
      else
        exit(EXIT_SUCCESS);
      break;

    default:
      fputs("unknown command.\n", stderr);
      exit(EXIT_FAILURE);
      break;
  }

  /* if the function is not a NULL pointer, pick an ID and call
   * the function with the returned ID as first argument. if no ID
   * has been written to 'id' var, the function is not called. */
  if (func) {
    queryp2(
      &slct, &cnd, a.lastedit, a.npar, a.params, &sh, id, a.pick);
    if (strnlen(id, 1))
      (*func)(id, pos, a.npos);
  }

  exit(EXIT_SUCCESS);
}
