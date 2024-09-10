#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

#include "../config.h"

#include "add_entries.h"

int
command_add_json(char* filepath)
{

  // the subprocess command
  char* csl2psql[] = { "csl2psql",
    filepath,
    (char*)connectioninfo,
    "-t",
    "entry",
    "-T",
    "_entry",
    NULL };

  // fork
  pid_t pid = fork();

  // error if fork fails
  if (pid == -1) {
    perror("fork");
    return 0;
  }

  // subprocess
  if (pid == 0) {
    execvp(csl2psql[0], csl2psql);
    exit(EXIT_SUCCESS);
  }

  // wait for the subprocess to end
  if (pid == 1){
      wait(NULL);
  }

  return 1;
}

int
command_add_bibtex(char* filepath)
{
  // create a temporary file to put the JSON
  char tmp_filepath[] = "/tmp/retrolire.bibtex.XXXXXX";
  if (!mkstemp(tmp_filepath))
    return 0;

  // the pandoc command to convert the bibtex into a csl-json
  char* bibtex2json[] = { "pandoc",
    "-i",
    filepath,
    "-o",
    tmp_filepath,
    "-f",
    "bibtex",
    "-t",
    "csljson",
    NULL };

  pid_t pid = fork();

  // error if fork fails
  if (pid == -1) {
    perror("fork");
    return 0;
  }

  // subprocess
  if (pid == 0) {
    execvp(bibtex2json[0], bibtex2json);
    exit(EXIT_SUCCESS);
  }

  else {
    // wait for the subprocess to end, then call the function that
    // will put the CSL-JSON in the database (using the python tool).
    wait(NULL);
    command_add_json(tmp_filepath);
  }

  return 1;
}

// arf relou de pas pouvoir écrire vers un fichier

// int
// command_add_doi(char* doi)
// {
//     char* doi2tex[] = {"isbn_doi2tex", doi};
//
//     // fork
//     pid_t pid = fork();
//
//     // error if fork fails
//     if (pid == -1) {
//         perror("fork");
//         return 0;
//     }
//
//     // subprocess
//     if (pid == 0) {
//         execvp(doi2tex[0], doi2tex);
//         exit(EXIT_SUCCESS);
//     }
//
//     else {
//         // wait for the subprocess to end, then call the function that
//         // will put the CSL-JSON in the database (using the python tool).
//         wait(NULL);
//         command_add_json(doi2tex);
//     }
//
//     return 1;
//
// }
