/* subcommands
 * -----------
 *
 * main subcommands
 *
 * */

#ifndef _SUBCOMMANDS_H
#define _SUBCOMMANDS_H

#include "stmt.h"

/* main commands. all these commands take the same number and the
 * same type of argumnents, because they are used via a function
 * pointer. (they all use the parameter 'id', but some don't use
 * other parameters.) */
#define COMMAND(NAME) \
  int NAME(char* id, char* pos[MAXPOS], int npos);
COMMAND(command_cite)
COMMAND(command_delete)
COMMAND(command_edit)
COMMAND(command_file)
COMMAND(command_open)
COMMAND(command_print)
COMMAND(command_quote)
COMMAND(command_refer)
COMMAND(command_tag_edit)
COMMAND(command_tag_pick)
COMMAND(command_update)
#undef COMMAND
  ;

/* command_add (for entries importing) is a bit different. */
int
command_add(char* method, char* identifier);


/* check that a string is an available command name. */
int
check_command_name(char* s);

/* send a query (SELECT) to the database, pipe out the result (all
 * values and rows) to a shell subprocess (fork + execvp), then read
 * the result of that subprocess and store it into 'dest'.
 * */
int
queryp2(struct Stmt* slct,
  struct Stmt* cnd,
  int lastedit,
  int npar,
  const char* const* params,
  struct ShCmd* sh,
  char* dest,
  int pick);

/* cite quote. */
int
make_stmt_quote(struct Stmt* slct, struct ShCmd* sh);
int
make_stmt_refer(struct Stmt* slct, struct ShCmd* sh);
int
make_stmt_open(struct Stmt* cnd, int npar);

/* list entries matching criterias. */
int
list(struct Stmt* cnd,
  int npar,
  const char* params[MAXOPT],
  char* arg);

/* output entries matching criterias in JSON format. */
int
json(struct Stmt* cnd, int npar, const char* params[MAXOPT]);

#endif
