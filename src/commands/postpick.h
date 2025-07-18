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
// commands/postpick.h -- functions called after an entry is picked.

#ifndef _POST_PICK_H
#define _POST_PICK_H

#include "../print.h"
#include "../sizes.h"

/* command_tag_edit -- edit tag in editor.
 * command_edit -- edit reading notes in editor.
 * command_open -- open an URLfile attached to an entry.
 * command_refer -- quote a concept with its reference.
 * command_quote -- get a quote formatted for pandoc.
 * command_delete -- delete an entry.
 * command_file -- attach a file to an entry.
 * command_tag_pick -- add tags to an entry using fzf.
 * command_update -- update a field of an entry.
 * command_cite -- cite an entry (get its ID).
 * command_print -- print informations about an entry.
 *
 * all these commands take the same number and the same type of
 * argumnents, because they are used via a function pointer. (they
 * all use the parameter 'id', but some don't use other parameters.)
 *
 * parameters
 * ---------
 *
 *  id:
 *      the id of the entry.
 *
 *  pos:
 *      the positional arguments (char* array).
 *
 *  npos:
 *      the number of positional arguments.
 */
#define DECL_CMD(NAME) int NAME(char* id, char** pos, int npos);
DECL_CMD(command_cite)
DECL_CMD(command_invoke)
DECL_CMD(command_delete)
DECL_CMD(command_edit)
DECL_CMD(command_file)
DECL_CMD(command_open)
DECL_CMD(command_print)
DECL_CMD(command_quote)
DECL_CMD(command_refer)
DECL_CMD(command_tag_edit)
DECL_CMD(command_tag_pick)
DECL_CMD(command_update)
#undef DECL_CMD
  ;

#endif
