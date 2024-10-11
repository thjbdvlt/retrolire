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
underscore_input(char* id)
{
  // print the current selection.
  fputs("\nCURRENT SELECTION:\n>>>>> ", stdout);
  head_entry(id);
  fputs("<<<<<\n\nCOMMAND: \n", stdout);

  // read user input (command name)
  char line[256];
  char* x = fgets(line, sizeof(line), stdin);

  // remove the newline at the end
  for (int i = 0; i < 256; i++) {
    if (x[i] == '\n' || x[i] == ' ') {
      x[i] = '\0';
      break;
    }
  }

  // check that the command exists and exist if it doesn't
  if (!check_command_name(x)) {
    fputs("UNKNOWN COMMAND: ", stdout);
    fputs(line, stdout);
    fputs("\n", stdout);
    exit(EXIT_FAILURE);
  }

  // pseudo positional argument (so i can use the commands)
  int npos = 0;
  char* pos[MAXPOS] = {};
  int (*func)(char*, char* [MAXPOS], int) = NULL;

  switch (x[0]) {
    case 't':
      func = command_tag_edit;
      break;

    case 'e':
      func = command_edit;
      break;

    case 'o':
      func = command_open;
      break;

    case 'p':
      func = command_print;
      break;

    case 'd':
      // delete is destructive: it asks for confirmation.
      if (!ask_confirmation()) {
        exit(EXIT_SUCCESS);
      }
      func = command_delete;
      // TODO: update list of entries after deletion..?
      break;

      // TODO: support command 'update'

    default:
      exit(EXIT_FAILURE);
      break;
  }

  (*func)(id, pos, npos);
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

    // test: input
    case 'i':
      underscore_input(pos[1]);
      break;

    case 'p': // _preview
      if (pos[1] != NULL) {
        if (preview(pos[1]) != 0) {
          exit(EXIT_SUCCESS);
        } else {
          exit(EXIT_FAILURE);
        }
      }
      break;

    case 'T':
      list_tags(); // Tags (completion)
      break;

    case 'F': // Fields (completion)
      list_fields();
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
