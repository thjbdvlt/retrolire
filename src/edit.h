/* editor
 * ------
 *
 * edit a value in editor (defined in config.h).
 *
 * */

#ifndef _EDITOR_H
#define _EDITOR_H

/* edit a string. */
char*
edit_in_editor(char* value, char* ext);

/* edit a value and update it in the database. */
int
edit_value(char* id, char* stmtselect, char* stmtupdate, char* ext);

/* edit a file */
int
edit_file(char* filepath);

#endif
