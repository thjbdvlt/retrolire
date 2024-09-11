#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>

#include "../config.h"
#include "edit.h"

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
    if (remove_file) {
      remove(filepath);
    }
    return 0;
  }

  // subprocess
  if (pid == 0) {
    execvp(csl2psql[0], csl2psql);
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
  char tmp_filepath[] = "/tmp/retrolire.XXXXXX";
  if (mkstemp(tmp_filepath) == -1) {
    if (remove_file) {
      remove(filepath);
    }
    return 0;
  }

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
    // remove both files on error
    remove(tmp_filepath);
    if (remove_file) {
      remove(filepath);
    }
    perror("fork");
    return 0;
  }

  // subprocess
  if (pid == 0) {
    execvp(bibtex2json[0], bibtex2json);
    exit(EXIT_SUCCESS);
  }

  else {

    // wait for the subprocess to check its status
    int status;
    if (waitpid(pid, &status, 0) < 0) {
      remove(tmp_filepath);
      perror("wait");
      exit(254);
    }

    // check the exit status from the previous command.
    if (WIFEXITED(status)) {
      // get the status of the subprocess.
      int sub_status = WEXITSTATUS(status);
      if (sub_status == 1) {
        // if the exit status is 1 (fail), exit the program.
        fputs("error: cancelled.\n", stderr);
        remove(tmp_filepath);
        exit(EXIT_FAILURE);
      }
    }

    // add the CSL-JSON file in the database, using csl2psql
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

    // wait for the subprocess to check its status
    int status;
    if (waitpid(pid, &status, 0) < 0) {
      remove(tmp_filepath);
      perror("wait");
      exit(254);
    }

    // check the exit status from the previous command.
    if (WIFEXITED(status)) {
      // get the status of the subprocess.
      int sub_status = WEXITSTATUS(status);
      if (sub_status == 1) {
        // if the exit status is 1 (fail), exit the program.
        fputs("error: cancelled.\n", stderr);
        remove(tmp_filepath);
        exit(EXIT_FAILURE);
      }
    }

    // format the bibtex file.
    format_bibtex(tmp_filepath);

    // edit the formatted filepath
    edit_file(tmp_filepath);

    // add the generated bibtex file to the database.
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
    remove(tmp_filepath);
    perror("fork");
    return 0;
  }

  // subprocess
  if (pid == 0) {
    execvp(cmd[0], cmd);
  }

  else {

    // wait for the subprocess to check its status
    int status;
    if (waitpid(pid, &status, 0) < 0) {
      remove(tmp_filepath);
      perror("wait");
      exit(254);
    }

    // check the exit status from the previous command.
    if (WIFEXITED(status)) {
      // get the status of the subprocess.
      int sub_status = WEXITSTATUS(status);
      if (sub_status == 1) {
        // if the exit status is 1 (fail), exit the program.
        fputs("error: cancelled.\n", stderr);
        remove(tmp_filepath);
        exit(EXIT_FAILURE);
      }
    }

    // format the bibtex file.
    format_bibtex(tmp_filepath);

    // edit the formatted filepath
    edit_file(tmp_filepath);

    // add the bibtex to the database.
    command_add_bibtex(tmp_filepath, 1);
  }

  return 1;
}

int
format_bibtex(char* filepath)
{

  // create a temporary file
  char tmp_filepath[] = "/tmp/retrolire.XXXXXX.bib";
  if (mkstemps(tmp_filepath, 4) == -1) {
    fputs("error creating temporary file.\n", stderr);
    return 0;
  }

  // the pandoc command to convert the bibtex into a csl-json
  char* pandoc[] = { "pandoc",
    "-i",
    filepath,
    "-o",
    tmp_filepath,
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
    remove(tmp_filepath);
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
  FILE* f_tmp = fopen(tmp_filepath, "r");
  if (!f_tmp) {
    remove(tmp_filepath);
    fputs("error opening file.\n", stderr);
    return 0;
  }

  // open file to write. it's the input of pandoc.
  FILE* f = fopen(filepath, "w");
  if (!f) {
    remove(tmp_filepath);
    fputs("error opening file.\n", stderr);
    return 0;
  }

  // copy, character by character, the content of the formatted file
  // to the original file.
  int c;
  while ((c = getc(f_tmp)) != EOF) {
    putc(c, f);
  }

  // close files.
  fclose(f);
  fclose(f_tmp);

  // remove temporary file
  remove(tmp_filepath);

  return 1;
}

// TODO: add templates
int
command_add_template(char* template_name)
{
#define FILEPATH "/tmp/retrolire.template.bib"

  // pick a template file matching pattern.
  FILE* p = popen(
    // xargs is used so i don't have to concatenate strings.
    "xargs -0 fdfind |"
    // fzf is used to pick a file. only the filename is used.
    " fzf -d/ --with-nth=-1 --nth=-1 -0"
    // bind enter to copy selected file
    " --bind='enter:become(cp {} " FILEPATH ")'"
    // if there is only one file matching pattern, it's
    // automatically picked.
    " --bind='one:become(cp {} " FILEPATH ")'",
    // write mode.
    "w");

  // exit function if popen failed.
  if (!p)
    return 0;

  // put the file name if one is used as parameter. if none, an
  // empty strings (delimited by single quotes) id passed.
  if (template_name)
    fputs(template_name, p);

  // 0-delimiter fields in xargs input
  putc('\0', p);

  // the directory containing templates
  fputs("/usr/share/retrolire/templates/", p);

  // close the pipe
  pclose(p);

  // edit the file
  edit_file(FILEPATH);

  // add the bibtex file
  command_add_bibtex(FILEPATH, 1);

  return 1;
#undef FILEPATH
}
