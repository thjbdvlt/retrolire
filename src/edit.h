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
/* editor
 * ------
 *
 * edit a value in editor (defined in config.h). */

#ifndef _EDITOR_H
#define _EDITOR_H

/* edit_in_editor -- edit a string in $EDITOR.
 *
 * parameters
 * ----------
 *
 *  value:
 *      the value to edit.
 *
 *  ext:
 *      the extension for the temporary file. */
char*
edit_in_editor(char* value, char* ext, char* linenr);

/* edit_value -- edit a value and update it in the database.
 *
 * parameters
 * ----------
 *
 *  id:
 *      the ID of the entry, will be passed as the first parameter
 *      in the database ($1).
 *
 *  stmtselect:
 *      the SELECT statement to get the value.
 *
 *  stmtupdate:
 *      the UPDATE statement.
 *
 *  ext:
 *      the extension for the temporary file (typically, json or
 * md). */
int
edit_value(char* id,
  char* stmtselect,
  char* stmtupdate,
  char* ext,
  char* linenr);

/* edit_file -- edit a file in the $EDITOR
 *
 * parameters
 * ----------
 *
 *  filepath:
 *      the path to the file to be edited. */
int
edit_file(char* filepath);

#endif
