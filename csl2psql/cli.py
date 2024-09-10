import csl2psql.json2psql.obj2row
import orjson
import csl2psql.cslminitools.csl
import argparse
import psycopg
from psycopg.sql import SQL, Identifier


usage = """
    %(prog)s csljson conninfo -t TABLE [--temp TEMP] [-r COLUMN]
    cat file.json | %(prog)s - conninfo -t TABLE [--temp TEMP] [-r COLUMN]
"""
desc = (
    "makes a postgresql table from a bibliography in csl-json format."
)
epilog = "(every entry in the csl-json becomes a row in the table)"
help_csljson = (
    "the bibliography in a csl-json file (or from stdin, using -)."
)
help_conninfo = "postgresql connection info string (dbname=DBNAME ...)"
help_table = "a table name (the table must exists and have a column id (datatype text).)"
help_temp = "a temp table name (only used during the process)."

parser = argparse.ArgumentParser(usage=usage, description=desc)
parser.add_argument(
    "csljson", type=argparse.FileType("r"), help=help_csljson
)
parser.add_argument("conninfo", type=str, help=help_conninfo)
parser.add_argument(
    "-t", "--table", type=str, help=help_table, required=True
)
parser.add_argument(
    "-T",
    "--temp",
    type=str,
    help=help_temp,
    required=False,
)
parser.add_argument(
    "-r",
    "--returning",
    type=str,
    required=False,
    help="a column name to be returned ('... returning id').",
)


def get_ids(cur, table) -> set[str]:
    """get all entry ids in the database.

    args:
        cur (psycopg.Cursor)
        table (psycopg.sql.Identifier)

    returns (set[str]):  the ids.
    """

    stmt = "select id from {}"
    stmt = SQL(stmt)
    stmt = stmt.format(table)
    return set(map(lambda i: i[0], cur.execute(stmt)))


def main():
    # parse command line arguments
    args = parser.parse_args()

    # connect to the database
    conn = psycopg.connect(args.conninfo)
    cur = conn.cursor()

    # table and temp table
    table = args.table
    temp = args.temp
    if not temp:
        temp = f"__{table}"
    table = Identifier(table)
    temp = Identifier(temp)

    # get all ids
    ids = get_ids(cur, table)

    # read and parse csl-json bibliography
    csl = args.csljson
    csl = csl.read()
    csl = orjson.loads(csl)

    # update ids and annotes for every entry
    csl2psql.cslminitools.csl.update_csl(csl=csl, ids=ids)

    # copy all objects (entries) to the database, as rows in the table
    r = csl2psql.json2psql.obj2row.array2table(
        csl,
        conn,
        table,
        temp,
        args.returning,
    )

    # end the transaction and end the connection
    conn.commit()
    conn.close()

    if r:
        print("\n".join(r))


if __name__ == "__main__":
    main()
