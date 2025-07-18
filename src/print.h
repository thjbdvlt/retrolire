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
/* print
 * -----
 *
 * preview and print informations about entries, or list things
 * about the database (tags, entries fields)
 *
 */

#ifndef _PRINT_H
#define _PRINT_H

#include "pgpopen2.h"
#include "stmt.h"
#include <postgresql/libpq-fe.h>

/* print a PGresult to FILE. */
int
write_res_expanded(PGresult* res, FILE* f, int term_width);

/* print a PGresult to FILE and use a pager to consult it. */
int
pgexpanded2pager(PGresult* res);

/* preview an entry (fields, note, files, tags). */
int
preview(char* id);

/* minimal informations about an entry (id, title, authors). */
int
head_entry(char* id);

/* list something. */
int
list_anything();

/* list all author family names. */
int
list_authors();

/* list all tags in the database. */
int
list_tags();

/* list all entries fields. */
int
list_fields();

/* preview from cache. */
int
preview_cache_entry();

#endif
