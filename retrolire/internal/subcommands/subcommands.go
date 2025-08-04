package subcommands

import (
	"bytes"
	"fmt"
	"os"
	"os/exec"
	"strings"

	"retrolire/internal/config"
	"retrolire/internal/note"
	"retrolire/internal/pick"
	"retrolire/internal/state"
	"retrolire/internal/statements"
	"retrolire/internal/util"
)

var check = util.Check

// State - Contains results, options and program state
type State = state.State

// Main subcommands operate on entries and require to pick one
type editCmd struct{ pickEntryCmd }
type citeCmd struct{ pickEntryCmd }
type updateCmd struct{ pickEntryCmd }
type openCmd struct{ pickEntryCmd }
type tagCmd struct{ pickEntryCmd }
type tagPickCmd struct{ pickEntryCmd }
type deleteCmd struct{ pickEntryCmd }

func (editCmd) Fn(t *state.State)    { edit(t) }
func (citeCmd) Fn(t *state.State)    { cite(t) }
func (updateCmd) Fn(t *state.State)  { update(t) }
func (openCmd) Fn(t *state.State)    { open(t) }
func (tagCmd) Fn(t *state.State)     { tag(t) }
func (tagPickCmd) Fn(t *state.State) { tagPick(t) }
func (deleteCmd) Fn(t *state.State)  { del(t) }

func getEntryValue(t *State, field string) string {
	db := t.Conn()
	row := db.QueryRow(
		"select coalesce(csl ->> ?, '') from entry where id = ?",
		field,
		t.ID.Entry,
	)
	check(db.Close())
	var data string
	check(row.Err())
	check(row.Scan(&data))
	return data
}

func cite(t *state.State) {
	_, _ = fmt.Fprint(os.Stdout, t.ID.Entry)
	if t.ID.Page != "" {
		_, _ = fmt.Fprint(os.Stdout, t.ID.Page)
	}
}

func edit(t *state.State) {
	db := t.Conn()
	id, line := t.ID.Entry, t.ID.Line
	if id == "" {
		return
	}
	if line == "" {
		line = "1"
	}
	fname := id + config.Ext
	util.EditFileLine(fname, line)
	note.Parse([]string{id}, []int64{0}, db)
	check(db.Close())
}

func del(t *state.State) {
	fmt.Println(t.FzfResult) // Show selected entry
	if !util.ConfirmUser("Confirm deletion?") {
		return
	}
	db := t.Conn()
	stmts := []string{
		"delete from entry where id = ?",
		"delete from tag where entry = ?",
		"delete from textobj where entry = ?",
	}
	for _, i := range stmts {
		_, err := db.Exec(i, t.ID.Entry)
		check(err)
	}
	check(db.Close())
	fmt.Println("Deleted:", t.ID.Entry)
}

func update(t *state.State) {
	id := t.ID.Entry
	args := t.Args.Fn
	db := t.Conn()
	field := args[0]
	path := "$." + field
	row := db.QueryRow("select json_type(csl -> ?) = 'text' from entry where id = ?", path, id)
	var isText bool
	check(row.Err())
	check(row.Scan(&isText))
	var stmtFrom, stmtTo string
	if isText {
		stmtFrom = "select csl ->> ? from entry where id = ?"
		stmtTo = "update entry set csl = json_set(csl, ?, json_quote(?)) where id = ?"
	} else {
		stmtFrom = "select csl -> ? from entry where id = ?"
		stmtTo = "update entry set csl = json_set(csl, ?, json(?)) where id = ?"
	}
	row = db.QueryRow(stmtFrom, field, id)
	check(row.Err())
	var value []byte
	check(row.Scan(&value))
	value = bytes.TrimSpace(value)
	value = util.EditTemp(value)
	_, err := db.Exec(stmtTo, path, strings.TrimSpace(string(value)), id)
	check(err)
	check(db.Close())
}

func tag(t *state.State) {
	id := t.ID.Entry
	db := t.Conn()
	var tags []byte
	var err error
	row := db.QueryRow("select coalesce(group_concat(tag, char(10)), '') from tag where entry = $1", id)
	check(row.Err())
	check(row.Scan(&tags))
	check(db.Close())
	tags = util.EditTemp(tags)
	db = t.Conn()
	tx, err := db.Begin()
	check(err)
	_, err = tx.Exec("delete from tag where entry = $1", id)
	check(err)
	stmt, err := tx.Prepare("insert into tag (entry, tag) values (?, ?)")
	check(err)
	for _, i := range bytes.Split(tags, []byte{'\n'}) {
		i = bytes.TrimSpace(i)
		if len(i) > 0 {
			_, err = stmt.Exec(id, string(i)) // It's important to convert to string!
			check(err)
		}
	}
	check(tx.Commit())
	check(db.Close())
}

func tagPick(t *state.State) {
	var err error
	db := t.Conn()
	rows, err := db.Query(statements.SelectTagOrderByUse) // Get tags
	check(err)
	check(rows.Err())
	check(db.Close()) // Close database while selecting with fzf
	tags, _ := pick.Pick(rows, []string{"--multi", "--header", t.FzfResult})
	db = t.Conn() // Reconnect to the database to insert tags
	stmt, err := db.Prepare("insert into tag (entry, tag) values (?, ?) on conflict do nothing")
	check(err)
	for _, tag := range tags {
		_, err = stmt.Exec(t.ID.Entry, tag)
		check(err)
	}
	check(db.Close())
}

func open(t *state.State) {
	url := getEntryValue(t, "URL")
	if url != "" {
		sh := exec.Command(config.Opener, url)
		check(sh.Run())
		return
	}
	fmt.Println("This entry has no URL.")
}
