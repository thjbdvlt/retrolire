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
#include "sql.h"

// this is the main SELECT statement used
const char sql_entry[] = "SELECT\n"
                         "coalesce(e.title, e.url) AS title,\n"
                         "jsonb_concat_values(coalesce("
                         "e.author, e.editor, e.translator"
                         "), ' ') AS someone,\n"
                         "e.id\n"
                         "FROM entry e\n"
                         "JOIN reading r ON r.id = e.id ";

// SELECT statements for ideas, quotes and concepts select
// differents things.
const char sql_quote[] = "SELECT\n"
                         "q.s, q.id, q.linenr, q.entry\n"
                         "FROM quote q\n"
                         "JOIN entry e ON e.id = q.entry\n"
                         "JOIN reading r ON r.id = e.id\n";

const char sql_idea[] = "SELECT\n"
                        "i.s, i.page, i.linenr, i.entry\n"
                        "FROM idea i\n"
                        "JOIN entry e ON e.id = i.entry\n"
                        "JOIN reading r ON r.id = e.id\n";

const char sql_concept[] =
  "SELECT\n"
  "c.s, c.definition, c.id, c.linenr, e.id\n"
  "FROM concept c\n"
  "JOIN entry e ON e.id = c.entry\n"
  "JOIN reading r ON r.id = e.id\n";

// statement for <list> and <json> select all entry fields.
const char sql_list[] = "SELECT e.* FROM entry AS e\n"
                        "JOIN reading r on r.id = e.id\n";

// and <json> create the JSON inside the SQL.
const char sql_json[] = "SELECT\n"
                        "jsonb_pretty(jsonb_agg(to_csl(e)))\n"
                        "FROM entry e\n"
                        "JOIN reading r ON r.id = e.id\n";

// the SELECT statement used for <open> filter entries: only the
// ones that have a file or an URL are filtered. this filtering is
// done in a CTE so it's not needed to put conditional clauses.
const char sql_openable[] = "WITH openable AS (\n"
                            "SELECT e.*\n"
                            "FROM entry e\n"
                            "WHERE (SELECT EXISTS (\n"
                            "SELECT 1 FROM file\n"
                            "WHERE entry = e.id)\n"
                            "OR \"URL\" IS NOT null)\n"
                            ")\n"
                            "SELECT\n"
                            "coalesce(e.title, e.url) AS title, "
                            "jsonb_concat_values(coalesce("
                            "e.author, e.editor, e.translator"
                            "), ' ') AS someone\n,"
                            "e.id\n"
                            "FROM openable e\n"
                            "JOIN reading r ON r.id = e.id ";
