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
#include "checkargs.h"
#include "../util.h"

int
cmd_check_file(char** argv, int argc)
{
  if (!argc) {
    fputs("missing argument: <file>\n", stderr);
    return 0;
  }
  return check_file(argv[0]);
}

int
cmd_check_update(char** argv, int argc)
{
  if (!argc) {
    fputs("missing argument: <field>\n", stderr);
    return 0;
  }
  return check_field(argv[0], NULL);
}
