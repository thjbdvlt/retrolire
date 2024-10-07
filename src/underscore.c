#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "commands.h"
#include "print.h"
#include "sizes.h"
#include "underscore.h"
#include "util.h"

int
underscore_add(char* pos[VAL_SIZE])
// TODO: changer ça
{
  // if (pos[1] == NULL) {
  //   fprintf(stderr, "missing arg.\n");
  //   return 0;
  // } else if (strstarts("file", pos[1]) != 0) {
  //   return underscore_file(pos[2], pos[3]);
  // } else if (strstarts("tags", pos[1]) != 0) {
  //   return underscore_add_tags(pos[2]);
  // } else {
  //   fprintf(stderr, "invalid arg for add (%s).\n", pos[1]);
  //   return 0;
  // }

  return 1;
}

void
do_underscore(char* argv[])
{
  /* the parameters are always all the arguments of the
   * command (ACTION), and are kept in the same order.
   * it's just required to take off the first, and move
   * all others to the start (one position left). */
  char* pos[VAL_SIZE] = {};
  for (int i = 1; argv[i] != NULL; i++) {
    pos[i - 1] = argv[i];
  }
  // TODO: move positional

  switch (argv[1][1]) {

    case 'p': // _preview
      if (pos[1] != NULL) {
        if (preview(pos[1]) != 0) {
          exit(EXIT_SUCCESS);
        } else {
          exit(EXIT_FAILURE);
        }
      }
      break;

    case 'l': // _list
      switch (argv[1][2]) {
        case 't':
          list_tags();
          break;
        case 'f':
          list_fields();
        default:
          break;
      }
      break;

    case 't': // _tag
      check_nonull(pos[1], "ENTRY_ID");
      command_tag_edit(pos[1], pos, 0);
      break;

    case 'c': // _cache
      preview_cache_entry();
      break;

    case 'r': // _refer
      check_nonull(pos[1], "CONCEPT_ID");
      command_refer(pos[1], pos, 0);
      break;

    case 'e': // _edit
      check_nonull(pos[1], "ENTRY_ID");
      command_edit(pos[1], pos, 0);
      break;

    case 'q': // _quote
      check_nonull(pos[1], "QUOTE_ID");
      command_quote(pos[1], pos, 0);
      break;

    case 'u': // _update
      check_nonull(pos[2], "FIELD");
      check_nonull(pos[1], "ENTRY_ID");
      command_update(pos[1], pos, 1);
      break;

    case 'h': // _head
      check_nonull(pos[1], "ENTRY_ID");
      head_entry(pos[1]);
      break;

    case 'o': // _open
      check_nonull(pos[1], "ENTRY_ID");
      command_open(pos[1], pos, 0);
      break;

    case 'a': // add
      check_nonull(pos[1], "[TAGS|FILE]");
      underscore_add(pos);
      break;

    case 'd': // _update
      check_nonull(pos[1], "ENTRY_ID");
      command_delete(pos[1], pos, 0);
      break;

    case 's': // _schema
      system("cat /usr/share/retrolire/schema.sql 2>/dev/null "
             "|| echo 'schema not found (reinstall retrolire).'");
      break;

    default:
      exit(EXIT_FAILURE);
      break;
  }
  exit(EXIT_SUCCESS);
}
