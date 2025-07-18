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
#ifndef _OPTS_H
#define _OPTS_H

#include "commands/definitions.h"
#include "doc.h"
#include "fzf.h"
#include "print.h"
#include "util.h"
#include <argp.h>
#include <stdlib.h>

// the structure for argument parsing
struct arguments
{
  int npar, nvar, ncnd, npos, last;
  char** varvalues;    // values from the --var option
  char** pos;          // other positional arguments (files, etc.)
  size_t maxposarg;    // max number of positional arguments
  size_t maxvalvalues; // max number of positional arguments
  size_t maxparams;    // max number of positional arguments
  const char** params; // parameters for the SQL
  struct RetroCmd cmd; // command
  struct Stmt* cnd;    // the conditional clause
  struct ShCmd* sh;    // the Shell Command
  struct Stmt* search; // the search string
  enum HowPrinted howprint; // picker/stdout/pager
};

// options
enum OPT
{
  OPT_VAR = 'v',
  OPT_TAG = 't',
  OPT_AUTHOR = 'a',
  OPT_REGEX = 'r',
  OPT_QUOTE = 'q',
  OPT_CONCEPT = 'c',
  OPT_NOT = 'n',
  OPT_OR = 'o',
  OPT_EXACT = 'e',
  OPT_LAST = 'l',
  OPT_RECENT = 'R',
  OPT_ID = 'i',
  OPT_OUTPUT = 'O',
  OPT_PAGER = 'p',
  OPT_TSEARCH = 's',
};

// initialize an argument structure

void
init_arguments(struct arguments* a,
  struct Stmt* cnd,
  struct ShCmd* sh,
  struct Stmt* search,
  char** posarg,
  size_t maxposarg);

// parse options using argp
error_t
parse_opt(int key, char* arg, struct argp_state* state);

// concatenate the statement according to arguments.
int
make_stmt(struct Stmt* slct, struct arguments* a);

// output the query result or send it to the picker
int
queryout(struct arguments* a);

// send the query result to the picker (fzf) and execute command
int
res2fzf(PGresult* res, struct arguments* a);

// concatenate the command options for fzf.
int
make_fzf(struct ShCmd* fzf, struct arguments* a);

// concatenate the SELECT statement.
int
make_stmt(struct Stmt* slct, struct arguments* a);

// shortcut to quickly add a conditional clause
int
_add_cnd(struct arguments* arguments,
  char* arg,
  char* s1,
  char* s2);

#endif
