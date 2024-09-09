/* print
 * -----
 *
 * preview and print informations about entries, or list things
 * about the database (tags, entries fields)
 *
 * */

#ifndef _PRINT_H
#define _PRINT_H

#include "pgpopen2.h"
#include "stmt.h"
#include <postgresql/libpq-fe.h>

/* print a PGresult to FILE. */
int
print_result(PGresult* res, FILE* f, int term_width);

/* preview an entry (fields, note, files, tags). */
int
preview(char* id);

/* minimal informations about an entry (id, title, authors). */
int
head_entry(char* id);

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
