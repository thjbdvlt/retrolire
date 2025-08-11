// Package cli - Command line usage
package cli

import (
	"database/sql"
	"fmt"
	"log"
	"os"
	"strings"

	"retrolire/internal/cli/opts"
	"retrolire/internal/cli/pick"
	"retrolire/internal/sqlmaker"
	"retrolire/internal/state"
)

// CliState - Store options and results
type CliState struct {
	Opts        *opts.Opts // Parameters for query
	Rows        *sql.Rows  // Result from Query
	ID          state.ID   // Parsed result from FZF
	Args        []string   // Positional arguments
	state.State            // /!\ Not a pointer
}

type cliCommand struct {
	fn    func(*CliState)
	nArgs int
	pick  bool
	stmt  func() *sqlmaker.SelectStmt
}

func pickOneID(t *CliState) bool {
	var code int
	// Pick something
	res, code := pick.Pick(t.Rows, t.Opts.Fzf)
	// Ensure something has been picker
	if len(res) == 0 || code != 0 {
		t.ID = state.ID{}
		return false
	}
	var s string
	// Parse the result
	parts := strings.Split(res[0], `  @`)
	// Parsing failed
	if len(parts) == 0 {
		log.Fatal("Parsing fzf result failed")
		return false
	}
	s = parts[len(parts)-1]
	t.ID = state.IDFromString(s) // Extract and Parse ID
	entryID := t.ID.Entry
	if entryID == "" || code != 0 {
		os.Exit(code)
	}
	// Update last pick if any (and if it's an entry)
	if t.Opts.TextObj && entryID != "" {
		db := t.Conn()
		check2(db.Exec(`UPDATE entry
SET lastpick = unixepoch('now')
WHERE id = ?`, entryID))
		check(db.Close())
	}
	return true
}

// Call - Parse arguments and call command
func Call(st *state.State, args []string) {
	var err error
	var c cliCommand
	t := &CliState{State: *st}
	args, t.Opts = opts.Parse(args)
	args, c = getCommandFromArgs(args, t.Opts)
	na := c.nArgs
	if len(args) < na {
		fmt.Fprintf(os.Stderr, "Requires %d arguments, got %d.\n", na, len(args))
		os.Exit(1)
	}
	t.Args = args[:na]
	args = args[na:]
	if c.stmt != nil {
		selectStmt := c.stmt()
		nReq := selectStmt.NRequiredParams
		if len(args) < nReq {
			fmt.Fprintf(os.Stderr, "Requires %d arguments", na+nReq)
			os.Exit(1)
		}
		stmtParams := make([]any, nReq)
		for i := 0; i < nReq; i++ {
			stmtParams[i] = args[i]
		}
		filterArgs := args[nReq:]
		stmt, params := selectStmt.BuildSelect(stmtParams, filterArgs)
		db := t.Conn()
		t.Rows, err = db.Query(stmt, params...)
		check(db.Close())
		check(err)
		check(t.Rows.Err())
		if c.pick {
			picked := pickOneID(t)
			if !picked {
				os.Exit(pick.NothingHasBeenPicked)
			}
		}
	}
	if c.fn != nil {
		c.fn(t)
	}
}
