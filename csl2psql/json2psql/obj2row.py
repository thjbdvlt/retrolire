import json
from psycopg.sql import SQL, Identifier


def convert_datatype(v):
    if isinstance(v, (list, dict, set, tuple)):
        return "jsonb"
    elif isinstance(v, str):
        return "text"
    elif isinstance(v, int):
        return "int"
    elif isinstance(v, float):
        return "float"
    return "jsonb"


def array2table(array, conn, table, temptable, returning=None):
    """makes a postgresql table from a json array of objects.

    the table columns are dynamically created using the objects. keys.

    args:
        array (list[dict]):  an array of JSON objects.
        conn (psycopg.Connection)
        tablename (str):  the table name.
        temptable (str):  the temp table name.
        returning (str):  a column name to be returned.
    """

    # create a cursor.
    cur = conn.cursor()

    # quote the table names.
    if isinstance(table, str):
        table = Identifier(table)
    if isinstance(temptable, str):
        temptable = Identifier(temptable)

    # create a temp table.
    stmt = SQL("create temp table {} (obj jsonb)")
    stmt = stmt.format(temptable)
    cur.execute(stmt)

    # copy all objects to the temp table.
    stmt = SQL("copy {} (obj) from stdin")
    stmt = stmt.format(temptable)
    with cur.copy(stmt) as copy:
        for obj in array:
            copy.write_row((json.dumps(obj, ensure_ascii=False),))

    # get the names of existing columns
    stmt = SQL("select * from {} limit 0")
    stmt = stmt.format(table)
    cur.execute(stmt)
    columns = (i[0] for i in cur.description)
    columns = set(columns)

    # get the names of all fields
    stmt = SQL("""select distinct jsonb_object_keys(obj) from {}""")
    stmt = stmt.format(temptable)
    cur.execute(stmt)
    fields = map(lambda i: i[0], cur.fetchall())
    fields = set(fields)

    stmt = SQL("""select distinct
        x.key, 
        x.value
        from {} e, jsonb_each(jsonb_strip_nulls(e.obj)) x
    """)
    stmt = stmt.format(temptable)

    # add new fields
    todo = fields - columns
    done = set()
    n_todo = len(todo)
    cur_send = conn.cursor()
    for field, value in cur.execute(stmt):
        if n_todo <= 0:
            break
        if field in done.union(columns):
            continue
        datatype = convert_datatype(value)
        datatype = Identifier(datatype)
        stmt = SQL("alter table {} add column {} {}")
        col = Identifier(field)
        stmt = stmt.format(table, col, datatype)
        cur_send.execute(stmt)
        done.add(field)
        n_todo += 1

    # populate the table with the objects
    stmt = SQL("""
    insert into {} 
    select 
        (jsonb_populate_record(
            null::{}, e.obj)
        ).* 
    from {} e
    """)
    stmt = stmt.format(table, table, temptable)
    if returning:
        stmt = stmt + SQL("returning {}").format(Identifier(returning))
    cur.execute(stmt)
    if returning:
        r = cur.fetchall()
        return [i[0] for i in r]
    else:
        return None
