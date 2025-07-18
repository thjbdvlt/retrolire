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
#include "parseargs.h"

void
init_arguments(struct arguments* a,
  struct Stmt* cnd,
  struct ShCmd* sh,
  struct Stmt* search,
  char** posarg,
  size_t maxposarg)
{

  a->npar = 0;
  a->nvar = 0;
  a->ncnd = 0;
  a->npos = 0;
  a->last = 0;
  a->howprint = OUT_UNDEFINED;

  a->cmd = get_default_cmd();

  a->sh = sh;
  a->cnd = cnd;
  a->search = search;

  a->pos = posarg;
  a->maxposarg = maxposarg;
}

// add a conditional clause and a parameter.
int
_add_cnd(struct arguments* arguments, char* arg, char* s1, char* s2)
{

  // concatenate the s1/s2 (+placeholder) to the conditional clause.
  if (!cat_cnd(arguments->cnd, s1, s2, arguments->npar)) {
    return 0;
  };

  // update the values in the struct
  arguments->params[arguments->npar] = arg;
  arguments->npar++;
  arguments->ncnd++;

  return 1;
};

// add a string with placeholder, without WHERE/AND clause.
int
_add_ph(struct arguments* arguments, char* arg, char* s1, char* s2)
{
  if (!cat_ph(arguments->cnd, s1, s2, arguments->npar)) {
    return 0;
  };
  arguments->params[arguments->npar] = arg;
  arguments->npar++;
  arguments->ncnd++;
  return 1;
}

// parse options using argp
error_t
parse_opt(int key, char* arg, struct argp_state* state)
{
  struct arguments* arguments = state->input;

  switch (key) {
    // TODO: fix errors (ARGP_ERR_UNKNOWN used in wrong places).

    case OPT_EXACT:
      append_sh(arguments->sh, "--exact");
      break;
    case OPT_LAST:
      arguments->last = 1;
      break;
    case OPT_RECENT:
      arguments->last = 2;
      break;
    case OPT_OUTPUT:
      arguments->howprint = OUT_STDOUT_ZERO;
      break;
    case OPT_PAGER:
      arguments->howprint = OUT_PAGER_EXPANDED;
      break;
    case OPT_OR:
      arguments->cnd->next_or = 1;
      break;
    case OPT_NOT:
      arguments->cnd->next_not = 1;
      break;

    case OPT_VAR: // e.g."author=antin" (processed later)
      arguments->varvalues[arguments->nvar] = arg;
      arguments->nvar++;
      break;

    case OPT_AUTHOR: // alias for `-v author=...`
      if (!_add_cnd(arguments, arg, "has_author(e," , "::text)"))
        return ARGP_ERR_UNKNOWN;
      break;

    case OPT_TAG: // tags: `-t sociology`
      if (!_add_cnd(arguments,
            arg,
            "exists (select 1 from tag t where t.entry = "
            "e.id and t.tag = ",
            ")"))
        return ARGP_ERR_UNKNOWN;
      break;

    case OPT_TSEARCH: // full text search (with small syntax changes)
      if (!_add_cnd(arguments,
            TEXT_SEARCH_CONFIGURATION,
            "r.tsvec @@ to_tsquery(",
            "::regconfig,"))
        return ARGP_ERR_UNKNOWN;
      if (!_add_ph(arguments,
            arg,
            "replace(regexp_replace(trim(",
            /* spaces are replaced by `<->` sign (phrase matching)
             * while `+` is replaced by `&`. */
            "::text), ' *[&+] *', '&', 'g'), ' ', ' <-> '))"))
        return ARGP_ERR_UNKNOWN;
      break;

    case OPT_REGEX:
      if (!_add_cnd(arguments,
            arg,
            "regexp_like(r.notes, ",
            "::text, 'i') "))
        return ARGP_ERR_UNKNOWN;
      ;
      break;

    case OPT_QUOTE:
      if (!_add_cnd(arguments,
            arg,
            "(select exists (select 1 from quote where entry = "
            "e.id and regexp_like(s, ",
            "::text, 'i'))) "))
        return ARGP_ERR_UNKNOWN;
      ;
      break;

    case OPT_CONCEPT:
      if (!_add_cnd(arguments,
            arg,
            "(select exists (select 1 from concept where entry "
            "= "
            "e.id "
            "and regexp_like(s, ",
            "::text, 'i'))) "))
        return ARGP_ERR_UNKNOWN;
      break;

    case OPT_ID:
      if (!_add_cnd(arguments, arg, "e.id = ", "::text"))
        return ARGP_ERR_UNKNOWN;
      break;

      /* positional argument.
       *  - there is a limit to positional arguments (maxposarg);
       *  - first positional is COMMAND;
       *  - all other go to arguments->pos;
       *  - keep track of number of positionals (arguments-npos).
       */
    case ARGP_KEY_ARG:
      // max
      if (state->arg_num >= arguments->maxposarg) {
        argp_usage(state);
        break;
      }

      // first argument may define the command
      else if (state->arg_num == 0) {
        struct RetroCmd cmd;
        cmd = parse_cmd_name_no_default(arg);
        if (cmd.id != CMD_UNKNOWN) {
          arguments->cmd = cmd;
          break;
        }
      }

      // other positional arguments
      if (arguments->npos >= arguments->cmd.npos) {

        if (!append_stmt(arguments->search, arg) ||
            !append_stmt(arguments->search, "\\ "))
          return ARGP_ERR_UNKNOWN;

      } else {
        // put the argument in the positional array
        arguments->pos[arguments->npos] = arg;
        arguments->npos++;
      }
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

// make the SELECT statement with WHERE clauses and ORDER BY.
int
make_stmt(struct Stmt* slct, struct arguments* a)
{

  // complete the conditional clause
  CONNECT;

  // add 'field=values' to conditional clause and to params.
  // - field is escaped and concatenated in the conditional clause.
  // - value goes in params.
  for (int i = 0; i < a->nvar; i++) {
    int npar = a->npar;
    char* value =
      parse_key_value(conn, a->cnd, a->npar, a->varvalues[i]);
    if (!value) {
      PQfinish(conn);
      exit(EXIT_FAILURE);
    }
    a->params[a->npar] = value;
    a->npar++;
  }

  PQfinish(conn); // end connection (only used for field escaping)

  // if at least one conditional clause, add closing parenthese
  if (a->cnd->total != a->cnd->remain) {
    if (!append_stmt(a->cnd, ")"))
      return 0;
  }

  // `json` command cannot be used with -l or -R
  if (a->cmd.id != CMD_JSON) {
    if (!append_lastedit(*a->cnd, a->last))
      return 0;
  }

  // build the SQL SELECT statement
  return append_stmt(slct, a->cmd.selectsql) &&
         append_stmt(slct, a->cnd->start);
}

int
terminate_query(struct arguments* a)
{
  // add the input search to the options (fzf --query <str>)
  if (a->search && a->search->total != a->search->remain) {
    a->search->end[-3] = ' ';  // -3 because: '\\', ' ', '\0'.
    a->search->end[-2] = '\0'; // terminate the string
    return 1;
  }
  return 0;
}

int
make_fzf(struct ShCmd* fzf, struct arguments* a)
{


  // the search query (additional positional arguments)
  const char* query[] = { "--query", NULL, NULL};
  if (a->search)
    query[1] = a->search->start;

  // an array with all arrays of options (strings)
  const char** opts[] = {
    fzf_base,         // fzf command and necessary options
    a->cmd.shellopts, // command-specific options
    a->sh->args,      // input options (e.g. --exact)
    a->cmd.oneline ? fzf_oneline : fzf_multiline, // delimiter
    (a->search && terminate_query(a)) ? query : NULL, // search query (if any)
    NULL,
  };

  // add options
  for (int i = 0; opts[i]; i++) {
    if (!append_sh_many(fzf, opts[i]))
      return 0;
  }

  return 1;
}

int
res2fzf(PGresult* res, struct arguments* a)
{

  // initialize a string to store the picked id
  char id[VAL_SIZE] = "";

  // initialize a ShCmd struct to store command line arguments
  const char* fzf_args[FZF_N_OPTS] = { NULL };
  struct ShCmd fzf = { 0, FZF_N_OPTS, fzf_args };

  if (!make_fzf(&fzf, a))
    return 0;

  char* deli = (a->cmd.oneline) ? "\t" : "\n\t";

  // send query and get result
  pgpopen2(res,
    deli,
    '\0',
    id,
    VAL_SIZE,
    fzf.args[0],
    (char* const*)fzf.args);

  // if an id have been picked, execute the command
  if (strnlen(id, 1))
    (*a->cmd.func)(id, a->pos, a->npos);

  return 1;
}

int
queryout(struct arguments* a)
{

  // initialize the SELECT statement
  struct Stmt slct;
  char slct_s[MAX_SIZE] = "";
  init_stmt(&slct, slct_s, MAX_SIZE, 0);

  int rcode = 1;

  // make the statement with clauses and command-specificities
  make_stmt(&slct, a);

  CONNECT; // connect to the database

  // send query
  PGresult* res = PQexecParams(
    conn, slct.start, a->npar, NULL, a->params, NULL, NULL, 0);

  // exit if query failed or if no result
  if (!check_res(res, conn) || PQntuples(res) == 0) {
    PQfinish(conn);
    PQclear(res);
    return 0;
  }

  PQfinish(conn); // end connection

  enum HowPrinted howprint = a->cmd.howprint;
  if (a->howprint != OUT_UNDEFINED)
    howprint = a->howprint;

  switch (howprint) {

    case OUT_STDOUT:
      write_res(res, "", '\n', stdout);
      break;

    case OUT_STDOUT_ZERO:
      write_res(res, "\n\t", '\0', stdout);
      break;

    case OUT_PAGER_EXPANDED:
      rcode = pgexpanded2pager(res);
      break;

    case OUT_PICKER:
    default:
      // TODO: implement OUT_PAGER (not expanded)
      rcode = res2fzf(res, a);
      break;
  }

  PQclear(res); // clear query

  return rcode;
}
