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
// functions for TUI, using ncurses.
#ifndef _TUI_H
#define _TUI_H

// for wide char functions
#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif

#include "util.h"
#include <ctype.h>
#include <curses.h>
#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

/* read_user_input -- read user input (wide char)
 *
 * the input is echoed, without ^? or ^H.
 *
 * parameters
 * ----------
 *
 * v (WINDOW*):
 *      the window where the user will write input.
 *
 * s (wchar*):
 *      the string where the input will be stored.
 *
 */
size_t
read_user_input(WINDOW* v, wchar_t* s);

/* pg2win -- print the result of a pg query into a ncurses window.
 *
 * parameters
 * ----------
 *
 *  res (PQresult*):
 *      pointer to the result of a query.
 *
 *  v (WINDOW*):
 *      a ncurses window.
 *
 *  win_height (int):
 *      maximum number of rows to write in the window.
 *
 *  win_width (int):
 *      maximum number of columns to write in the window.
 */
int
pg2win(PGresult* res, WINDOW* v, int win_height, int win_width);

/* read_command -- read the name of a command from the user
 *
 * parameters
 * ----------
 *
 *  id (char*):
 *      the id of an entry.
 *
 *  cmd (char*):
 *      the string where the command will be stored.
 *
 *  size_cmd (size_t):
 *      the size of the string that will store the cmd.
 */
size_t
read_command(char* id, char* cmd, size_t size_cmd);

/* read_infos -- show informations about an entry (all fields).
 *
 * parameters
 * ----------
 *
 *  id (char*):
 *      the id of an entry. */
size_t
show_infos(char* id);

#endif
