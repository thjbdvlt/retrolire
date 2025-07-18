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
#ifndef _SIZES_H
#define _SIZES_H

#include <limits.h>

// strings
#define FIELD_SIZE 48
#define VAL_SIZE 128
#define MAX_SEARCH 256
#define MAX_STMT_LEN 256
#define MAX_SIZE 1024
#define MAX_FILEPATH 1024
#define SIZE_PLACEHOLDER 13
#define MAX_V VAL_SIZE + FIELD_SIZE

// string arrays
#define FZF_N_OPTS 50 // OK it's a lot but...
#define MAXPOS 10
#define MAXOPT 20
#define MAX_ADD_TAGS 20

// option --last and --recent
#define LASTEDIT_LAST 1
#define LASTEDIT_RECENT 2

#endif
