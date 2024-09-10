#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

#include "../config.h"

#include "add_entries.h"

int
command_add_json(char* filepath, int remove_file)
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

  else {
    // wait for the subprocess to end
    wait(NULL);

    // remove the file
    if (remove_file) {
      remove(filepath);
    }
  }

  return 1;
}

int
command_add_bibtex(char* filepath, int remove_file)
{
  // create a temporary file to put the JSON
  char tmp_filepath[] = "/tmp/retrolire.bibtex.XXXXXX";
  if (mkstemp(tmp_filepath) == -1)
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
    // will put the CSL-JSON in the database (using the python
    // tool).
    wait(NULL);
    command_add_json(tmp_filepath, 1);

    // remove the file
    if (remove_file) {
      remove(filepath);
    }
  }

  return 1;
}

int
command_add_isbn(char* isbn)
{

  // create a temporary file
  char tmp_filepath[] = "/tmp/retrolire.XXXXXX.bib";
  if (mkstemps(tmp_filepath, 4) == -1) {
    fputs("error creating temporary file.\n", stderr);
    return 0;
  }

  // get the metadata using the ISBN and the list of isbn services
  // defined in config.h. write the result in the temporary file.
  char* cmd[] = { "fetchref",
    "isbn",
    isbn,
    "--services",
    isbn_services,
    "-o",
    tmp_filepath,
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
    execvp(cmd[0], cmd);
    exit(EXIT_SUCCESS);
  }

  else {
    // wait for the subprocess to end, then call the function that
    // will put the CSL-JSON in the database (using the python
    // tool).
    wait(NULL);
    command_add_bibtex(tmp_filepath, 1);
  }

  return 1;
}
