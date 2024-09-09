from csl2psql.json2psql import obj2row
import orjson
import psycopg
import argparse

desc = """makes a postgresql table from a json array of objects."""
epilog = "(every object in the array becomes a row in the table)"
usage = """
    %(prog)s INFILE -d DBNAME -t TABLE [--temp TEMP] [-r COLUMN]
    cat array.json | %(prog)s - -d DBNAME -t TABLE [--temp TEMP] [-r COLUMN]
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
    "-d",
    "--dbname",
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

    # dbname, table name, temp table name
    dbname = args.dbname
    table = args.table
    temp = args.temp

    # if no temp table name: make one with suffix "__".
    if not temp:
        temp = f"__{table}"

    # connect to database
    conn = psycopg.connect(dbname=dbname)

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
