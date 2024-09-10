#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

  format_bibtex(filepath);

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

  // fork
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

int
command_add_doi(char* doi)
{

  // create a temporary file
  char tmp_filepath[] = "/tmp/retrolire.XXXXXX.bib";
  if (mkstemps(tmp_filepath, 4) == -1) {
    fputs("error creating temporary file.\n", stderr);
    return 0;
  }

  // get the metadata using the ISBN and the list of isbn services
  // defined in config.h. write the result in the temporary file.
  char* cmd[] = {
    "fetchref", "doi", doi, "-o", tmp_filepath, NULL
  };

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

int
format_bibtex(char* filepath)
{

  // create a temporary file
  char f_out[] = "/tmp/retrolire.XXXXXX.bib";
  if (mkstemps(f_out, 4) == -1) {
    fputs("error creating temporary file.\n", stderr);
    return 0;
  }

  // the pandoc command to convert the bibtex into a csl-json
  char* pandoc[] = { "pandoc",
    "-i",
    filepath,
    "-o",
    f_out,
    "-f",
    "bibtex",
    "-t",
    "bibtex",
    NULL };

  // fork
  pid_t pid = fork();
  if (pid == -1) {
    // error forking
    perror("fork");
    return 0;
  }

  // subprocess
  if (pid == 0) {
    execvp(pandoc[0], pandoc);
  }

  // main process
  else {
    // here: copy the content of the first file to the second.
    wait(NULL);
  }

  // open file to read. it's the output of pandoc.
  FILE* file_out = fopen(f_out, "r");
  if (!file_out) {
    fputs("error opening file.\n", stderr);
    return 0;
  }

  // open file to write. it's the input of pandoc.
  FILE* file_in = fopen(filepath, "w");
  if (!file_in) {
    fputs("error opening file.\n", stderr);
    return 0;
  }

  // copy, character by character, the content of the formatted file
  // to the original file.
  int c;
  while ((c = getc(file_in)) != EOF) {
    putc(c, file_out);
  }

  // close files.
  fclose(file_in);
  fclose(file_out);
  remove(f_out);

  return 1;
}

int
edit_bibtex(char* f_in)
{

  // create a temporary file
  char f_out[] = "/tmp/retrolire.XXXXXX.bib";
  if (mkstemps(f_out, 4) == -1) {
    fputs("error creating temporary file.\n", stderr);
    return 0;
  }

  // compute size needed to allocate memory to the command
  size_t len_editor = strlen(editor);
  size_t len_f_out = strlen(f_out);

  // allocate memory for the command.
  // char* edit_cmd = malloc(sizeof(char) * len_editor + len_f_out +
  // 2);
  char* edit_cmd = malloc(len_editor + len_f_out + 2);

  if (!edit_cmd) {
    fputs("error allocating memory.\n", stderr);
    return 0;
  }

  sprintf(edit_cmd, "%s %s", editor, f_out);

  puts(edit_cmd);

  // the pandoc command to convert the bibtex into a csl-json
  char* pandoc[] = { "pandoc",
    "-i",
    f_in,
    "-o",
    f_out,
    "-f",
    "bibtex",
    "-t",
    "bibtex",
    NULL };

  free(edit_cmd);

  return 1;
}
