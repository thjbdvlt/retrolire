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
// configuration for retrolire.
#ifndef _CONFIG_H
#define _CONFIG_H
#include "src/commands/definitions.h"

// string used te the connection to database.
#define CONNECTIONINFO "dbname=retrolire"

// the command used to open files and urls.
#define OPENER "xdg-open"

// editor and line number prefix .
#define EDITOR "$EDITOR"

// line number prefix for editing. (e. g. for vim: `vim test.md +20`)
// set to NULL to deactivate.
#define LINENR_PREFIX " +"

// default command. see src/commands/definitions.h for a list.
#define DEFAULT_COMMAND CMD_EDIT

// isbn services for `retrolire add isbn`
#define ISBN_SERVICES "openl wiki goob"

#define ORDER_BY_LASTEDIT

// fzf options
// all fzf options are optional.
// (if an option is not set, fzf uses $FZF_DEFAULT_OPTS)
#define FZF_PREVIEW_POS "right,45%,hidden,border-sharp,wrap"
#define FZF_WRAP_SIGN "·"
#define FZF_TAB_STOP "4"
#define FZF_MARGIN "0"
#define FZF_PADDING "0"
// #define FZF_COLORS ""

// // uncomment to enable
// #define FZF_SET_REVERSE // --reverse
// #define FZF_SET_CYCLE // --no-mouse
// #define FZF_SET_MOUSE // default is: --no-mouse
// #define FZF_SET_SEPARATOR // default is: --no-separator
// #define FZF_SET_BOLD // default is: --no-bold

#define TEXT_SEARCH_CONFIGURATION "jusquci"

#endif
