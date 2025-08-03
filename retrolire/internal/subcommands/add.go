// Package subcommands - Retrolire subcommands.
// This file describe subcommands that add data to the database or create it.
package subcommands

import (
	"bytes"
	"database/sql"
	"fmt"
	"os"
	"os/exec"

	"retrolire/internal/bibtex"
	"retrolire/internal/note"
	"retrolire/internal/state"
	"retrolire/internal/statements"
	"retrolire/internal/util"
)

// Commands "parse" and "add" add data to the database.
type parseCmd struct{ printQueryCmd }
type addCmd struct{ noQueryCmd }
type initCmd struct{ noQueryCmd }

func (parseCmd) Fn(t *state.State) { parse(t) }
func (addCmd) Fn(t *state.State)   { add(t) }
func (initCmd) Fn(t *state.State)  { initDB(t) }
func (parseCmd) Stmt() string      { return "select e.id, e.lastedit from entry e" }
func (addCmd) NFnArgs() int        { return 2 }

func initDB(*state.State) {
	var err error
	var db *sql.DB
	dbPath := util.DbPath()
	db, err = sql.Open("sqlite3", dbPath)
	check(err)
	tx, err := db.Begin()
	check(err)
	stmts := []string{
		statements.CreateEntry,
		statements.CreateTextObj,
		statements.CreateTag,
		`create index entry_id on entry(id)`,
		`create index textobj_entry on textobj(entry)`,
		`create index textobj_class on textobj(class)`,
		`create index tag_entry on tag(entry)`,
		`create index tag_tag on tag(tag)`,
	}
	for _, i := range stmts {
		_, err = tx.Exec(i)
		check(err)
	}
	check(tx.Commit())
	check(db.Close())
}

func fromIsbnOrDoi(method string, identifier string) []byte {
	var bufOut bytes.Buffer
	sh := exec.Command("fetchref", method, identifier)
	sh.Stdout = &bufOut
	sh.Stderr = os.Stderr
	err := sh.Run()
	if err != nil {
		os.Exit(1)
	}
	return bufOut.Bytes()
}

func add(t *state.State) {
	popen2 := util.Popen2
	var err error
	var buf bytes.Buffer
	// Data is read from stdin if "-"
	// TODO: Bibtex / JSON from file (or error)
	data := t.Args.Fn[1]
	if data == "-" {
		_, err = buf.ReadFrom(os.Stdin)
		check(err)
		data = buf.String()
	}
	bdata := []byte(data)
	method := t.Args.Fn[0]
	pandoc := []string{"pandoc", "-f", "biblatex", "-t", "csljson"}
	switch method {
	case "json":
	case "bibtex":
		bdata = popen2(bdata, pandoc)
	case "doi", "isbn":
		bdata = popen2(fromIsbnOrDoi(method, data), pandoc)
		bdata = util.EditTemp(bdata)
	case "template":
		template, ok := bibtex.GetTemplate(data)
		if !ok {
			os.Exit(1)
		}
		bdata = util.EditTemp(template)
		bdata = popen2(bdata, pandoc)
	default:
		fmt.Println("Unknown method:", method)
		os.Exit(1)
	}
	db := t.Conn()
	// Get IDs
	row := db.QueryRow("select coalesce(group_concat(id, ' '), '') from entry")
	var ids string
	check(row.Scan(&ids))
	check(db.Close())
	// Make unique IDs (if option --keep-id isn't set)
	if !t.Opts.KeepIDs {
		bdata = popen2(bdata, []string{"csljson-update", "-", ids})
	}
	// Add entries to the database
	db = t.Conn()
	_, err = db.Exec(`with x as (
  select value from json_each($1)
)
insert into entry (id, csl)
select value ->> 'id', value
from x`, string(bdata))
	check(err)
	check(db.Close())
}

func parse(t *state.State) {
	var ids []string
	var lastedits []int64
	check(t.Rows.Err())
	for t.Rows.Next() {
		var id string
		var lastedit int64
		check(t.Rows.Scan(&id, &lastedit))
		ids = append(ids, id)
		lastedits = append(lastedits, lastedit)
	}
	check(t.Rows.Close())
	db := t.Conn()
	note.Parse(ids, lastedits, db)
	check(db.Close())
}
