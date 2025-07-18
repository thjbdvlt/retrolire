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
#include "tui.h"

/* read wide char user input, echoing while getting input. */
size_t
read_user_input(WINDOW* v, wchar_t* s)
{
  wint_t wc; // character received from input
  int i = 0;

  while (1) {

    // get a wide character
    wget_wch(v, &wc);

    // backspace: erase a character.
    if (wc == 127 || wc == KEY_BACKSPACE) {
      if (i > 0) {
        i--;
      }
    }

    else if (wc == '\e') {
      i = 0;
      s[0] = '\0';
      break;
    }

    // enter: end input.
    else if (wc == '\n') {
      break;
    }

    // do nothing with control keys
    else if (iscntrl(wc)) {
    }

    // else: add character string.
    // (wchar_t and wint_t should be the same..?)
    else {
      s[i] = (wchar_t)wc;
      i++;
    }

    // go at the beginning of the window
    wmove(v, 0, 0);

    // add terminating string
    s[i] = '\0';

    // clear window
    wclear(v);

    // print actual string
    waddwstr(v, s);
  }
  return (size_t)i;
}

/* print a PGresult to a window */
int
pg2win(PGresult* res, WINDOW* v, int win_height, int win_width)
{

  // get number of rows and columns, in order to iterate on them.
  int n_rows = PQntuples(res);
  int n_fields = PQnfields(res);

  // two arrays for fields:
  //  - name
  //  - length of name
  char fields_names[n_fields][100];
  int field_names_lens[n_fields];

  // ?
  win_height -= 2;
  win_width -= 2;

  int cur_row = 1;

  // get the longest field name length
  int max_f_len = 0;
  int curlen = 0;
  int i, s, j;

  for (int i = 0; i < n_fields; i++) {

    // get field name, put in array
    char* x = memccpy(fields_names[i],
      PQfname(res, i),
      '\0',
      sizeof(char*) * ((size_t)win_width));
    if (!x) {
      fputs("ERROR 601: field name too long for window.\n", stderr);
      return 0;
    }

    // calculate the len of its name
    curlen = (int)strnlen(fields_names[i], FIELD_SIZE);
    if (curlen == FIELD_SIZE) {
      fputs("ERROR 602: field name too long (max. 48 characters).\n", stderr);
      return 0;
    }

    // put this len in an array
    field_names_lens[i] = curlen;
    // look for the longest length.
    if (curlen > max_f_len) {
      max_f_len = curlen;
    }
  }

  // add padding after each field name and build a string
  // that is field + padding (spaces) + "|".
  for (i = 0; i < n_fields; i++) {
    for (s = field_names_lens[i]; s < max_f_len; s++) {
      // padding with spaces.
      fields_names[i][s] = ' ';
      // fields_names[i][s] = '.';
    }
    // separator '|' between field and value.
    fields_names[i][s] = '|';
    // space after separator.
    fields_names[i][s + 1] = ' ';
    // terminate string.
    fields_names[i][s + 2] = '\0';
  }

  // compute the characters limit for values
  size_t limit = (size_t)(win_width - (max_f_len + 1));

  // iterate on the rows
  for (i = 0; i < n_rows; i++) {
    // iterate on the fields
    for (j = 0; j < n_fields; j++) {
      // if the value is NULL, do not print it.
      if (PQgetisnull(res, i, j) == 0) {
        // stop when reach the end of the window.
        if (cur_row >= win_height) {
          return cur_row;
        }
        cur_row++;
        // else, print the field name (with padding and delimiter).
        // fputs(fields_names[j], f);
        waddstr(v, fields_names[j]);
        // get the value
        char* data = PQgetvalue(res, i, j);
        // get the lenght of the value.
        size_t _len = (size_t)PQgetlength(res, i, j);
        if (!_len) {
          waddch(v, L'\n');
          continue;
        }
        // compute the max lenght needed to store the wide chars.
        size_t wlen = (sizeof(wchar_t)) * _len;
        // initiate a wide char
        wchar_t _data[wlen];
        // convert bytes to wide chars and get len of second.
        size_t len = mbstowcs(_data, data, wlen);
        // if the length of the value is smaller than the limit,
        // i just print it.
        if (len < limit) {
          waddwstr(v, _data);
        }
        // but if the length is bigger than the limit, then i
        // wrap the value
        else {
          int max_lines_per_val = 5;
          int aligned_len = max_lines_per_val * (max_f_len + 2);
          int cur_newlines = 0;
          wchar_t aligned_data[aligned_len];
          aligned_data[0] = _data[0];
          size_t diff = 0;
          for (size_t nw = 1; nw < len; nw++) {
            if (nw % limit == 0) {
              cur_newlines++;
              if (cur_newlines >= max_lines_per_val) {
                break;
              }
              aligned_data[nw + diff] = '\n';
              diff++;
              for (int m = 0; m < max_f_len; m++) {
                aligned_data[nw + diff] = ' ';
                diff++;
              }
              aligned_data[nw + diff] = '\\';
              diff++;
              aligned_data[nw + diff] = ' ';
              diff++;
            }
            aligned_data[nw + diff] = _data[nw];
            aligned_data[nw + diff + 1] = '\0';
          }
          waddwstr(v, aligned_data);
        }
        waddch(v, '\n');
      }
    }
    waddch(v, '\n');
  }
  return cur_row;
}

size_t
read_command(char* id, char* cmd, size_t size_cmd)
{
  // initialize ncurses
  initscr();
  noecho();

  // get number of rows and columns
  int term_rows, term_cols;
  getmaxyx(stdscr, term_rows, term_cols);

  // first window: the entry informations.
  int _rows = 8;
  int _cols = 80;

  // reduce the number of columns/rows if the terminal
  // is smaller that preset dimensions. and center horizontaly.
  int _x;
  int _y;

  if (term_rows < (_rows + 1)) {
    _rows = (term_rows - 1);
    _y = 0;
  } else {
    _y = (((term_rows) - (_rows + 1)) / 2);
  }

  if (term_cols < _cols) {
    _cols = term_cols;
    _x = 0;
  } else {
    _x = (term_cols - _cols) / 2;
  }

  int rows = _rows - 2;
  int cols = _cols - 2;
  int y = _y + 1;
  int x = _x + 1;

  // create the window that contains the border and create the
  // border.
  WINDOW* v_info_border = newwin(_rows, _cols, _y, _x);
  wborder(v_info_border, '|', '|', '-', '-', '-', '-', '-', '-');

  // create the subwindow, the one that contains the informations.
  WINDOW* v_info_content = subwin(v_info_border, rows, cols, y, x);

  // get the informations about the selected entry
  const char* params[1] = { id };
  CONNECT;
  char* stmt = "select e.* from _head e where e.id = $1";
  PGresult* res =
    PQexecParams(conn, stmt, 1, NULL, params, NULL, NULL, 0);

  // check that the query succeed.
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr, "ERROR 603: query failed:\n %s\n", PQerrorMessage(conn));
    PQclear(res);
    PQfinish(conn);
    return 0;
  }

  // end connection: not usefull anymore
  PQfinish(conn);

  // put informations into the subwindow
  // (i don't know why '- 2' but it's necessary.)
  pg2win(res, v_info_content, rows, cols);

  // clear postgresql result.
  PQclear(res);

  // refresh every thing to show content.
  refresh();
  wrefresh(v_info_border);
  wrefresh(v_info_content);

  // the window containing the "enter a command" message.
  int y_msg = _y + _rows;
  int x_msg = _x;
  int cols_msg = _cols;
  int rows_msg = 1;
  WINDOW* v_msg = newwin(rows_msg, cols_msg, y_msg, x_msg);

  // print the message itself
  char msg[] = "enter a command:";
  int msg_len = (int)strlen(msg);
  waddstr(v_msg, msg);
  wrefresh(v_msg);

  // the window for the user input (command name).
  int cols_cmd = cols_msg - msg_len;
  int rows_cmd = rows_msg;
  int y_cmd = y_msg;
  int x_cmd = x_msg + msg_len + 1;
  WINDOW* v_cmd = newwin(rows_cmd, cols_cmd, y_cmd, x_cmd);

  wchar_t s[40] = L"";
  size_t len_s = read_user_input(v_cmd, s);

  // delete windows and end curses
  delwin(v_cmd);
  delwin(v_msg);
  delwin(v_info_border);
  delwin(v_info_content);
  endwin();

  // write the result to the char array in parameter
  size_t wlen = wcstombs(cmd, s, size_cmd);

  return wlen;
}

size_t
show_infos(char* id)
{
  // initialize ncurses
  initscr();
  noecho();

  // get number of rows and columns
  int term_rows, term_cols;
  getmaxyx(stdscr, term_rows, term_cols);

  // first window: the entry informations.
  int _rows = 20;
  int _cols = 80;
  int _x = 0;
  int _y = 0;

  // reduce the number of columns/rows if the terminal
  // is smaller that preset dimensions. and center horizontaly.
  if (term_rows < (_rows + 1)) // rows
    _rows = (term_rows - 1);
  else
    _y = (((term_rows) - (_rows + 1)) / 2);
  if (term_cols < _cols) // cols
    _cols = term_cols;
  else
    _x = (term_cols - _cols) / 2;

  int rows = _rows - 2;
  int cols = _cols - 2;
  int y = _y + 1;
  int x = _x + 1;

  // create the window that contains the border and create the
  // border.
  WINDOW* v_info_border = newwin(_rows, _cols, _y, _x);
  wborder(v_info_border, '|', '|', '-', '-', '-', '-', '-', '-');

  // create the subwindow, the one that contains the informations.
  WINDOW* v_info_content = subwin(v_info_border, rows, cols, y, x);

  // get the informations about the selected entry
  const char* params[1] = { id };
  CONNECT;
  char* stmt = "select e.* from entry e where e.id = $1";
  PGresult* res =
    PQexecParams(conn, stmt, 1, NULL, params, NULL, NULL, 0);

  // check that the query succeed.
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr, "ERROR 604: query failed:\n %s\n", PQerrorMessage(conn));
    PQclear(res);
    PQfinish(conn);
    return 0;
  }

  // end connection: not usefull anymore
  PQfinish(conn);

  // put informations into the subwindow
  // (i don't know why '- 2' but it's necessary.)
  pg2win(res, v_info_content, rows, cols);

  // clear postgresql result.
  PQclear(res);

  // refresh every thing to show content.
  refresh();
  wrefresh(v_info_border);
  wrefresh(v_info_content);

  getch();

  delwin(v_info_border);
  delwin(v_info_content);
  endwin();

  return 1;
}
