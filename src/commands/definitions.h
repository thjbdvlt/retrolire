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
// commands/definitions.h -- commands definitions.

#ifndef _SUBCOMMANDS_H
#define _SUBCOMMANDS_H

#include "../stmt.h"

// Command -- retrolire subcommands.
enum Command
{
  // CMD_UNKNOWN is used for undefined commands.
  CMD_UNKNOWN = -1,

  // main commands
  CMD_ADD = 1,
  CMD_CITE,
  CMD_DELETE,
  CMD_EDIT,
  CMD_FILE,
  CMD_INVOKE,
  CMD_JSON,
  CMD_LIST,
  CMD_OPEN,
  CMD_PRINT,
  CMD_QUOTE,
  CMD_REFER,
  CMD_TAG_EDIT,
  CMD_TAG_PICK,
  CMD_UPDATE,

  // underscore commands
  UCMD_INPUT,
  UCMD_HEAD,
  UCMD_TAGS,
  UCMD_AUTHORS,
  UCMD_FIELDS,
  UCMD_LEMMES,
  UCMD_PREVIEW,
  UCMD_SCHEMA,
  UCMD_CACHE,
  UCMD_FZFOPTS,
  UCMD_LIST_VAR,

};

enum FzfOptions
{
  SH_UNDEFINED = -1,
  SH_ENTRY,
  SH_QUOTE,
  SH_CONCEPT,
  SH_IDEA,
};

enum HowPrinted
{
  OUT_UNDEFINED = -1,
  OUT_NO_PRINT = 0,
  OUT_PICKER,
  OUT_STDOUT,
  OUT_STDOUT_ZERO,
  OUT_PAGER,
  OUT_PAGER_EXPANDED
};

// RetroCmd -- structure for the subcommands.
struct RetroCmd
{
  char* name;                      // name of the command
  enum Command id;                 // numeric id
  int (*func)(char*, char**, int); // function called
  int oneline;
  const char* selectsql;         // SELECT SQL
  const char** shellopts;        // fzf options
  enum HowPrinted howprint;      // if result is picked or printed
  int (*checkargs)(char**, int); // function that check arguments
  int npos; // number of positional argument (narg)
};

/* parse_cmd_name -- parse a string to assing a command.
 *
 * if no function name is matched, the DEFAULT_COMMAND defined in
 * config.h is returned.
 *
 * parameters
 * ----------
 *
 *  cmdname:
 *      the string that may contains a command name.
 */
struct RetroCmd
parse_cmd_name(char* cmdname);

/* parse_cmd_name_no_default -- parse a string to assing a command.
 *
 * if no function name is matched, the CMD_UNKNOWN is returned.
 *
 * parameters
 * ----------
 *
 *  cmdname:
 *      the string that contains a command name.
 */
struct RetroCmd
parse_cmd_name_no_default(char* cmdname);

/* get_default_cmd -- get the default command. */
struct RetroCmd
get_default_cmd();

#endif
