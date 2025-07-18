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
/* underscore commands
 * -------------------
 *
 * commands used by other commands, such as 'quote', or
 * functions prefixed by an underscore, such as '_quote'.
 *
 * */

#ifndef _UNDERSCORE_H
#define _UNDERSCORE_H

#include "../parseargs.h"
#include "../sizes.h"

// parse underscore arguments and execute functions accordingly
int
do_underscore(int argc, char* argv[]);

// read user input and execute function
// (called by ":" in the picker)
int
underscore_input(char* id, char** argv, int argc);

// function called by underscore_input for command that execute
// a new picker (e.g. quote, cite).
int
underscore_subpicker(const char* id, struct RetroCmd* cmd);

// undescore command have the same type as normal commands.
// clang-format off
int ucommand_head     (char* id,      char** __, int ___);
int ucommand_tags     (char* _,       char** __, int ___);
int ucommand_authors  (char* _,       char** __, int ___);
int ucommand_fields   (char* _,       char** __, int ___);
int ucommand_lemmes   (char* _,       char** __, int ___);
int ucommand_preview  (char* id,      char** __, int ___);
int ucommand_cache    (char* id,      char** __, int ___);
int ucommand_schema   (char* _,       char** __, int ___);
int ucommand_fzfopts  (char* cmdname, char** __, int ___);
int ucommand_list_var (char* var,     char** __, int ___);
// clang-format on

#endif
