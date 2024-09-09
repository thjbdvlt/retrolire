#!/bin/bash

# create the schema for the retrolire database.

# THE DATABASE HAS TO BE CREATED BY USER,
# BEFORE THEM USE THIS SCRIPT.

set -e

dbname="${RETROLIRE_DBNAME:=retrolire}"
data_dir="/usr/share/retrolire"

psql -d "$dbname" -c "select 1" || {
    echo ""
    echo "error. please check that:"
    echo "- environnment variable \$RETROLIRE_DBNAME is set."
    echo "- its value points to an existing and empty database."
    exit 1
}

for i in tables functions indexes quotes triggers constraints
do
    psql -d "$dbname" < "${data_dir}/schema/${i}.sql" \
        -v ON_ERROR_STOP=true || {
        echo "error initializing the database."
        echo "- maybe the database has already been initialized (do nothing)."
        echo "- maybe files are missings (try to reinstall retrolire)."
        exit 1
    }
done
exit 0
