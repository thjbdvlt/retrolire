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
#ifndef _ADD_H
#define _ADD_H

/* command_add -- add entries from doi/isbn/json/bibtex.
 *
 * add entries to the database, using a file (BibTeX or CSL-JSON),
 * an universal identifier (DOI or ISBN), or a template (BibTeX).
 *
 * parameters
 * ----------
 *
 * method (char*):
 *      the method to get the refereneces: DOI, ISBN, JSON, bibtex.
 *
 * identifier (char*):
 *      the identifier (DOI, ISBN) or file path (JSON, bibtex)
 * */
int
command_add(const char* method, const char* identifier);

int
command_add_json(const char* filepath, const int remove_file);

int
command_add_bibtex(const char* filepath, const int remove_file);

int
command_add_doi(const char* doi, const int _);

int
command_add_isbn(const char* isbn, const int _);

int
command_add_template(const char* template_name, const int _);

#endif
