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
// retrolire -- command line bibliography management

#include "add_entries.h"
#include "commands/underscore.h"
#include "doc.h"
#include "parseargs.h"
#include <locale.h>

const char* argp_program_version = VERSION;
static char args_doc[] = ARG_DOC;
static char doc[] = DOC;

// clang-format off
static struct argp_option options[] = {
// long, short, argname, optional, doc, group
{ 0,         0,           NULL,       OPTION_DOC, GR_FILTER,   1 },
{ "var",     OPT_VAR,     OA_VAR,     0,          DOC_VAR,     0 },
{ "tag",     OPT_TAG,     OA_TAG,     0,          DOC_TAG,     0 },
{ "regex",   OPT_REGEX,   OA_REGEX,   0,          DOC_REGEX ,  0 },
{ "quote",   OPT_QUOTE,   OA_QUOTE,   0,          DOC_QUOTE,   0 },
{ "concept", OPT_CONCEPT, OA_CONCEPT, 0,          DOC_CONCEPT, 0 },
{ "search",  OPT_TSEARCH, OA_TSEARCH, 0,          DOC_TSEARCH, 0 },
{ "author",  OPT_AUTHOR,  OA_AUTHOR, 0,           DOC_AUTHOR,  0 },
{ 0,         0,           NULL,       OPTION_DOC, GR_LOGICAL,  2 },
{ "not",     OPT_NOT,     NULL,       0,          DOC_NOT,     0 },
{ "or",      OPT_OR,      NULL,       0,          DOC_OR,      0 },
{ 0,         0,           NULL,       OPTION_DOC, GR_FZF,      3 },
{ "exact",   OPT_EXACT,   NULL,       0,          DOC_EXACT,   0 },
{ 0,         0,           NULL,       OPTION_DOC, GR_HIST,     4 },
{ "last",    OPT_LAST,    NULL,       0,          DOC_LAST,    0 },
{ "recent",  OPT_RECENT,  NULL,       0,          DOC_RECENT,  0 },
{ 0,         0,           NULL,       OPTION_DOC, GR_MISC,     5 },
{ "id",      OPT_ID,      OA_ID,      0,          DOC_ID,      0 },
{ "output",  OPT_OUTPUT,  NULL,       0,          DOC_OUTPUT,  0 },
{ "pager",   OPT_PAGER,   NULL,       0,          DOC_PAGER,  0 },
{ 0 }
};
// clang-format on

static struct argp // argument parsing
  argp = { options, parse_opt, args_doc, doc, NULL, NULL, NULL };

int
main(int argc, char** argv)
{
  setlocale(LC_CTYPE, ""); // wide char support

  // no argument parsing for underscore commands
  if (argv[1] && argv[1][0] == '_')
    exit(do_underscore(argc, argv));

  // shell options
  const char* user_sh_opts[MAXPOS] = { NULL };
  struct ShCmd sh = {
    .n_args = 0,
    .maxargs = MAXPOS,
    .args = user_sh_opts,
  };

  // initialize structs needed for argument parsing

  // conditional clauses
  char cnd_s[MAX_SIZE] = "";
  struct Stmt cnd = {
    .start = cnd_s,
    .end = &cnd_s[1],
    .total = MAX_SEARCH,
    .remain = MAX_SEARCH,
    .next_or = 0,
    .next_not = 0,
  };

  // search string
  char search_s[MAX_SEARCH] = "'";
  struct Stmt search = {
    .start = search_s,
    .end = &search_s[2],
    .total = MAX_SEARCH,
    .remain = MAX_SEARCH - 1,
    .next_or = 0,
    .next_not = 0,
  };

  // initialize the argument structure
  char* pos[MAXPOS] = {};
  char* varval[MAXOPT] = {};
  const char* params[MAXOPT] = {};
  struct arguments a = {
    .npar = 0,
    .nvar = 0,
    .ncnd = 0,
    .npos = 0,
    .last = 0,
    .howprint = OUT_UNDEFINED,
    .cmd = get_default_cmd(),
    .params = params,
    .varvalues = varval,
    .sh = &sh,
    .cnd = &cnd,
    .search = &search,
    .pos = pos,
    .maxposarg = MAXPOS,
    .maxparams = MAXOPT,
    .maxvalvalues = MAXOPT,
  };

  // parse command line arguments
  argp_parse(&argp, argc, argv, 0, 0, &a);

  // the command 'add' is special: no SELECT SQL.
  if (a.cmd.id == CMD_ADD)
    exit(command_add(a.pos[0], a.pos[1]) ? 0 : 1);

  // check arguments specific for the chosen command
  if (a.cmd.checkargs && !a.cmd.checkargs(a.pos, a.npos))
    exit(0);

  // send the query out: to fzf, to stdout or to the pager
  queryout(&a);

  // end program
  exit(0);
}
