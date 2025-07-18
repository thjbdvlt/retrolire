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
#include "add_entries.h"
#include "doc.h"
#include "edit.h"
#include "util.h"
#include <wait.h>

// all functions to add something have the same structure
struct AddMethod
{
  const char* name;
  int (*func)(const char*, const int);
};

int
command_add(const char* method, const char* identifier)
{

  // ensure first argument (method)
  if (method == NULL) {
    fputs(USAGE_ADD, stderr);
    exit(EXIT_FAILURE);
  }

  // defines all possible methods
  struct AddMethod methods[] = {
    { "json", command_add_json },
    { "doi", command_add_doi },
    { "isbn", command_add_isbn },
    { "template", command_add_template },
    { "bibtex", command_add_bibtex },
    { NULL, NULL },
  };

  // iterate over the methods to find the selected one
  for (int i = 0; methods[i].name; i++) {
    if (strstarts(methods[i].name, method)) {
      if (!identifier && methods[i].name[0] != 't')
        print_error_no_arg(method);
      else
        methods[i].func(identifier, 0);
      return 0;
    }
  }

  // if no method name has matched, error message
  fputs(USAGE_ADD, stderr);

  return 1;
}

int
command_add_json(const char* filepath, const int remove_file)
{

  // the subprocess command
  const char* csl2psql[] = { "csl2psql",
    filepath,
    CONNECTIONINFO,
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
    execvp(csl2psql[0], (char**)csl2psql);
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
command_add_bibtex(const char* filepath, const int remove_file)
{

  // a temporary filepath for the csl-json
  char tmp_filepath[] = "/tmp/retrolire.XXXXXX.json";
  if (mkstemps(tmp_filepath, 5) == -1) {
    fputs("ERROR 201: failed to create temporary file.\n", stderr);
    return 0;
  }

  // the pandoc command to convert the bibtex into a csl-json
  const char* bibtex2json[] = { "pandoc",
    "-i",
    filepath,
    "-o",
    tmp_filepath,
    "-f",
    "biblatex",
    "-t",
    "csljson",
    NULL };

  // fork
  pid_t pid = fork();

  // error if fork fails
  if (pid == -1) {

    // remove both files on error
    remove(tmp_filepath);

    // remove file according to parameter 'remove_file'
    if (remove_file)
      remove(filepath);

    perror("fork");
    return 0;
  }

  // subprocess
  if (pid == 0) {
    execvp(bibtex2json[0], (char**)bibtex2json);
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
        fputs("ERROR 203: cancelled.\n", stderr);
        remove(tmp_filepath);
        exit(EXIT_FAILURE);
      }
    }

    // edit the JSON file
    edit_file(tmp_filepath);

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
command_add_isbn(const char* isbn, const int _)
{

  // create a temporary file
  char tmp_filepath[] = "/tmp/retrolire.XXXXXX.json";
  if (mkstemps(tmp_filepath, 5) == -1) {
    fputs("ERROR 204: creating temporary file.\n", stderr);
    return 0;
  }

  // get the metadata using the ISBN and the list of isbn services
  // defined in config.h. write the result in the temporary file.
  const char* cmd[] = { "fetchref",
    "isbn",
    isbn,
    "--services",
    ISBN_SERVICES,
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
    execvp(cmd[0], (char**)cmd);
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
        fputs("ERROR 205: cancelled.\n", stderr);
        remove(tmp_filepath);
        exit(EXIT_FAILURE);
      }
    }

    // add the generated bibtex file to the database.
    command_add_bibtex(tmp_filepath, 1);
  }

  return 1;
}

int
command_add_doi(const char* doi, const int _)
{

  // create a temporary file
  char tmp_filepath[] = "/tmp/retrolire.XXXXXX.json";
  if (mkstemps(tmp_filepath, 5) == -1) {
    fputs("ERROR 206: failed to create temporary file.\n", stderr);
    return 0;
  }

  // get the metadata using the ISBN and the list of isbn services
  // defined in config.h. write the result in the temporary file.
  const char* cmd[] = {
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
    execvp(cmd[0], (char**)cmd);
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
        fputs("ERROR 207: cancelled.\n", stderr);
        remove(tmp_filepath);
        exit(EXIT_FAILURE);
      }
    }

    // add the bibtex to the database.
    command_add_bibtex(tmp_filepath, 1);
  }

  return 1;
}

int
command_add_template(const char* template_name, const int _)
{
#define FILEPATH "/tmp/retrolire.template.bib"
#define TEMPLATE_DIR "/usr/share/retrolire/templates/"

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
  fputs(TEMPLATE_DIR, p);

  // close the pipe
  pclose(p);

  // if the file doesn't exists. end function (fzf abort).
  if (access(FILEPATH, F_OK) != 0)
    return 0;

  // edit the file
  edit_file(FILEPATH);

  // add the bibtex file
  command_add_bibtex(FILEPATH, 1);

  return 1;
#undef FILEPATH
#undef TEMPLATE_DIR
}
