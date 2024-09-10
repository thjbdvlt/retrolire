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
    "-",
    "-d",
    (char*)connectioninfo,
    "-t",
    "entry",
    "-T",
    "_entry",
    NULL };

  pid_t pid = fork();

  // error if fork fails
  if (pid == -1) {
    perror("fork");
    return 0;
  }

  // subprocess
  if (pid == 0) {
    csl2psql[1] = filepath;
    execvp(csl2psql[0], csl2psql);
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

  char* bibtex2json[] = { "pandoc",
    "-i",
    NULL, // 2 = input filepath
    "-o",
    NULL, // 4 = output filepath (temporary)
    "-f",
    "bibtex",
    "-t",
    "csljson",
    NULL };

  bibtex2json[2] = filepath;
  bibtex2json[4] = tmp_filepath;

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
    wait(NULL);
    puts("????\n");
    puts("____\n");
    // wait for the subprocess to end
    command_add_json(tmp_filepath);
  }

  return 1;
}
