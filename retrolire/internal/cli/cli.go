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
	Opts *opts.Opts // Parameters for query
	Rows *sql.Rows  // Result from Query
	ID   state.ID   // Parsed result from FZF
	Args []string   // Positional arguments
	*state.MainState
}

type cliCommand struct {
	fn    func(*CliState)
	nArgs int
	pick  bool
	stmt  func() *sqlmaker.SelectStmt
}

func pickOneID(t *CliState) bool {
	var err error
	var code int
	// Pick something
	res, code := pick.Pick(t, t.Rows, t.Opts.Fzf)
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
		db := t.DB()
		_, err = db.Exec(`UPDATE entry
SET lastpick = unixepoch('now')
WHERE id = ?`, entryID)
		t.Log(err)
	}
	return true
}

// Call - Parse arguments and call command
func Call(mainState *state.MainState, args []string) {
	var err error
	var c cliCommand
	t := &CliState{MainState: mainState}
	args, t.Opts, err = opts.Parse(args)
	state.NoErr(t, err, "failed to parse command line options")
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
		nReq := selectStmt.NRequiredParams()
		if len(args) < nReq {
			fmt.Fprintf(os.Stderr, "Requires %d arguments", na+nReq)
			os.Exit(1)
		}
		stmtParams := make([]any, nReq)
		for i := 0; i < nReq; i++ {
			stmtParams[i] = args[i]
		}
		filterArgs := args[nReq:]
		stmt, params := selectStmt.BuildSelect(stmtParams, filterArgs, t)
		db := t.DB()
		t.Rows, err = db.Query(stmt, params...)
		state.NoErr(t, err, "couldn't get data")
		state.NoErr(t, t.Rows.Err(), "couldn't get data")
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
