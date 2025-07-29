# retrolire -- commande line bibliography manager.
# Copyright (C) 2024,2025  thjbdvlt
#
# retrolire is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# retrolire is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with retrolire.  If not, see <https://www.gnu.org/licenses/>.
import orjson
import csl2psql.cslminitools.csl
import argparse


desc = """update "id" and "annote" fields of every entries in a CSL-JSON bibliography."""
csl_desc = "a CSL-JSON bibliography."
ids_desc = "a list of IDs (one per line)"
usage = """
    %(prog)s csljson ids
    %(prog)s - ids < csljson
"""

parser = argparse.ArgumentParser(description=desc, usage=usage)
parser.add_argument("csljson", type=argparse.FileType("r"), help=csl_desc)
parser.add_argument("ids", type=str, help=ids_desc)


def main():
    """command line function to update a csl (using a list of ids)."""

    args = parser.parse_args()

    # get csl
    csl = args.csljson
    csl = csl.read()
    csl = orjson.loads(csl)

    # get ids
    ids = args.ids.split()
    ids = map(lambda i: i.strip(), ids)
    ids = set(ids)

    # update every entries
    csl2psql.cslminitools.csl.update_csl(csl=csl, ids=ids)

    # dump the new csl
    csl = orjson.dumps(csl)
    csl = csl.decode()

    # print it
    print(csl)
    exit(0)


if __name__ == "__main__":
    main()
