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
#include "definitions.h"
#include "../fzf.h"
#include "../sql.h"
#include "../util.h"
#include "checkargs.h"
#include "postpick.h"
#include "underscore.h"

// clang-format off
struct RetroCmd retrolire_commands[] = {
#define entry   0, &sql_entry[0],    &fzf_entry[0],   OUT_PICKER
#define open    0, &sql_openable[0], &fzf_entry[0],   OUT_PICKER
#define quote   1, &sql_quote[0],    &fzf_quote[0],   OUT_PICKER
#define idea    1, &sql_idea[0],     &fzf_idea[0],    OUT_PICKER
#define concept 1, &sql_concept[0],  &fzf_concept[0], OUT_PICKER
#define list    0, &sql_list[0],     &fzf_idea[0],    OUT_PAGER_EXPANDED
#define json    0, &sql_json[0],     NULL,            OUT_STDOUT
#define nosql   0, NULL,             NULL,            OUT_NO_PRINT
#define underscore 0, NULL, OUT_STDOUT, NULL

{"edit",     CMD_EDIT,     command_edit,     entry,   NULL, 0, },
{"cite",     CMD_CITE,     command_cite,     idea,    NULL, 0, },
{"invoke",   CMD_INVOKE,   command_invoke,   entry,   NULL, 0, },
{"open",     CMD_OPEN,     command_open,     open,    NULL, 0, },
{"quote",    CMD_QUOTE,    command_quote,    quote,   NULL, 0, },
{"refer",    CMD_REFER,    command_refer,    concept, NULL, 0, },
{"file",     CMD_TAG_EDIT, command_file,     entry,   cmd_check_file, 1},
{"tag",      CMD_TAG_EDIT, command_tag_edit, entry,   NULL, 0, },
{"tag-pick", CMD_TAG_PICK, command_tag_pick, entry,   NULL, 0, },
{"delete",   CMD_DELETE,   command_delete,   entry,   NULL, 0, },
{"update",   CMD_UPDATE,   command_update,   entry,   cmd_check_update, 1},
{"print",    CMD_PRINT,    command_print,    entry,   NULL, 0, },
{"list",     CMD_LIST,     NULL,             list,    NULL, 0, },
{"json",     CMD_JSON,     NULL,             json,    NULL, 0, },
{"add",      CMD_ADD,      NULL,             nosql,   NULL, 2  },
{"_input",   UCMD_INPUT,   underscore_input, nosql,   NULL, 1, },
{"_head",    UCMD_HEAD,    ucommand_head,    nosql,   NULL, 1, },
{"_tags",    UCMD_TAGS,    ucommand_tags,    nosql,   NULL, 0, },
{"_authors", UCMD_AUTHORS, ucommand_authors, nosql,   NULL, 0, },
{"_fields",  UCMD_FIELDS,  ucommand_fields,  nosql,   NULL, 0, },
{"_lemmes",  UCMD_LEMMES,  ucommand_lemmes,  nosql,   NULL, 0, },
{"_preview", UCMD_PREVIEW, ucommand_preview, nosql,   NULL, 1, },
{"_schema",  UCMD_SCHEMA,  ucommand_schema,  nosql,   NULL, 0, },
{"_cache",   UCMD_CACHE,   ucommand_cache,   nosql,   NULL, 1, },
{"_fzfopts", UCMD_FZFOPTS, ucommand_fzfopts, nosql,   NULL, 1, },
{"_var",     UCMD_LIST_VAR,ucommand_list_var,nosql,   NULL, 1, },
{NULL,       CMD_UNKNOWN,  NULL,             nosql,   NULL, 0, },

#undef entry
#undef quote
#undef idea
#undef concept
#undef list
#undef json
#undef open
#undef _null
#undef nosql
#undef underscore
};
// clang-format off


struct RetroCmd
get_default_cmd()
{
  struct RetroCmd cmd;
  for (int i = 0;; i++) {
    cmd = retrolire_commands[i];
    if (cmd.id == DEFAULT_COMMAND)
      return cmd;
    else if (cmd.id == CMD_UNKNOWN)
      return retrolire_commands[0];
  }
}


struct RetroCmd
parse_cmd_name(char* cmdname)
{
  struct RetroCmd cmd, default_cmd;
  for (int i = 0;; i++) {
    cmd = retrolire_commands[i];
    if (cmd.id == CMD_UNKNOWN) {
      // if it reaches the end of the commands array without
      // a match, the command is the default command.
      cmd = default_cmd;
      break;
    } else if (strstarts(cmd.name, cmdname)) {
      // if it matches a command name, then this command is selected
      break;
    } else if (cmd.id == DEFAULT_COMMAND) {
      // assign the default command
      default_cmd = cmd;
    }
  }
  return cmd;
}


struct RetroCmd
parse_cmd_name_no_default(char* cmdname)
{
  struct RetroCmd cmd;
  for (int i = 0;; i++) {
    cmd = retrolire_commands[i];
    if (cmd.id == CMD_UNKNOWN || strstarts(cmd.name, cmdname))
      break;
  }
  return cmd;
}
