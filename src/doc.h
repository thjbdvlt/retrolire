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
#ifndef _DOC_H
#define _DOC_H

#define VERSION "0.1.0"
#define ARG_DOC "<command> [...]"
#define DOC \
  "\nbibliography management with postgresql and fzf.\n" \
  "\navailable commands:\n" \
  "  list\n" \
  "  edit\n" \
  "  cite\n" \
  "  open\n" \
  "  add METHOD {IDENTIFIER|FILE}\n" \
  "  tag [pick]\n" \
  "  json\n" \
  "  file FILE\n" \
  "  delete\n" \
  "  refer\n" \
  "  print\n" \
  "  quote\n" \
  "  update FIELD\n"

// options definitions
#define DOC_VAR "field-value search"
#define DOC_TAG "filter entries with a tag"
#define DOC_REGEX "search pattern in reading notes"
#define DOC_TSEARCH "search in reading notes using a tsquery"
#define DOC_AUTHOR "filter by author"
#define DOC_QUOTE "search pattern in quotes"
#define DOC_CONCEPT "search pattern in concept"
#define DOC_EXACT "no fuzzy matching"
#define DOC_NOT ""
#define DOC_OR ""
#define DOC_LAST "select the last selected entry"
#define DOC_RECENT "order entries by recent editing"
#define DOC_ID "specified the entry id "
#define DOC_OUTPUT "do not interactively pick an id"
#define DOC_PAGER "see results in pager instead of picking an entry"

// options arguments
#define OA_VAR "field=regex"
#define OA_TAG "tag"
#define OA_AUTHOR "author"
#define OA_TSEARCH "tsquery"
#define OA_REGEX "regex"
#define OA_QUOTE OA_REGEX
#define OA_CONCEPT OA_REGEX
#define OA_ID "id"

// option groups
#define GR_FILTER "filters:"
#define GR_LOGICAL "logical operators:"
#define GR_FZF "selection (fzf):"
#define GR_HIST "history:"
#define GR_MISC "misc:"

// command-specific doc
#define USAGE_ADD \
  "usage:\n\tretrolire <method> <identifier|file>" \
  "\n\nmethod can be one of:" \
  "\n\t- doi" \
  "\n\t- isbn" \
  "\n\t- json" \
  "\n\t- bibtex" \
  "\n\t- template\n"

#endif
