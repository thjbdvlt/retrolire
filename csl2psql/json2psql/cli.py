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
from csl2psql.json2psql import obj2row
import orjson
import psycopg
import argparse

desc = """makes a postgresql table from a json array of objects."""
epilog = "(every object in the array becomes a row in the table)"
usage = """
    %(prog)s INFILE CONNINFO -t TABLE [--temp TEMP] [-r COLUMN]
    cat array.json | %(prog)s - CONNINFO -t TABLE [--temp TEMP] [-r COLUMN]
"""

parser = argparse.ArgumentParser(
    description=desc, usage=usage, epilog=epilog
)
parser.add_argument(
    "infile",
    type=argparse.FileType("r"),
    help="a json file.",
)
parser.add_argument(
    "conninfo",
    type=str,
    required=True,
    help="a postgresql database name.",
)
parser.add_argument(
    "-t",
    "--table",
    type=str,
    required=True,
    help="a table name, where json object will be rows.",
)
parser.add_argument(
    "-r",
    "--returning",
    type=str,
    required=False,
    default=None,
    help="a column name to be returned ('... returning id').",
)
parser.add_argument(
    "-T",
    "--temp",
    type=str,
    default=None,
    required=False,
    help="a temp table name (only used during the process).",
)


def main():
    """command line function."""

    # parse arguments
    args = parser.parse_args()

    # get the file content, parse the JSON
    s = args.infile.read()
    array = orjson.loads(s)

    # connection info string, table name, temp table name
    table = args.table
    temp = args.temp

    # if no temp table name: make one with suffix "__".
    if not temp:
        temp = f"__{table}"

    # connect to database
    conn = psycopg.connect(args.conninfo)

    # copy the objects as rows to the table
    r = obj2row.array2table(array, conn, table, temp, args.returning)

    conn.commit()
    conn.close()
    if r:
        return r
    else:
        return None


if __name__ == "__main__":
    main()
