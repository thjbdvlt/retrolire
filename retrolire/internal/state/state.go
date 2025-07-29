// Package state - Store results, options and program state
package state

import (
	"database/sql"

	"retrolire/internal/opts"
	"retrolire/internal/util"
)

// State - Store options and results
type State struct {
	dbChecked bool       // DB has already been checked
	Opts      *opts.Opts // Parameters for query
	Rows      *sql.Rows  // Result from Query
	ID        ID         // Parsed result from FZF
	FzfResult string     // Raw result from fzf
	Args      Args       // Positional arguments
}

// Args - Positional arguments
type Args struct {
	Filters []string // Generic filters arguments
	Fn      []string // Arguments processed by the post-query function
	Query   []any    // Command-specific arguments passed as parameters to the query
}

// Conn - Connect to the database
func (t *State) Conn() *sql.DB {
	path := util.DbPath()
	db, err := sql.Open("sqlite3", path)
	if err != nil {
		util.Abort("Error opening database", path)
	}
	// Fix slow note parsing and 'database is locked'
	_, err = db.Exec("PRAGMA synchronous = OFF")
	util.Check(err)
	// See https://github.com/mattn/go-sqlite3/issues/569
	_, err = db.Exec("PRAGMA journal_mode = WAL")
	util.Check(err)
	if !t.dbChecked {
		t.dbChecked = true
		// Check that the table ENTRY exists.
		// If it doesn't, it's likely that the database doesn't exist at all.
		_, err := db.Exec("select id, csl, lastedit from entry limit 0")
		if err != nil {
			util.Abort(
				"Looks like this database doesn't exists: ", path,
				"\nYou should call `retrolire init` to create the database.",
			)
		}
	}
	return db
}
