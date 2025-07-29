// Package pick - Pick an entry with FZF
package pick

import (
	"database/sql"
	fzf "github.com/junegunn/fzf/src"
	"log"
	"os"
	"strings"

	"retrolire/internal/state"
	"retrolire/internal/util"
	"retrolire/internal/statements"
)

const NothingHasBeenPicked = 2

// FzfGlobalOpts - Global options for FZF
func FzfGlobalOpts() []string {
	return []string{
		"--read0",
		"--no-multi",
		"--padding=0",
		"--margin=0",
		"--tabstop=4",
		"--ignore-case",
		"--tiebreak=chunk,begin,length",
		"--cycle",
		"--exit-0",
		"--wrap",
		"--wrap-sign", "\t.",
	}
}

var check = util.Check

// Pick - Pick an entry (or something else) with fzf and call callback function
func Pick(rows *sql.Rows, fzfOpts []string) ([]string, int) {
	if rows == nil {
		log.Fatal("nil rows to fzf")
	}
	fzfOpts = append(FzfGlobalOpts(), fzfOpts...)
	var err error
	inputChan := make(chan string)
	go func() {
		defer rows.Close()
		for rows.Next() {
			var s string
			check(rows.Scan(&s))
			inputChan <- s
		}
		check(rows.Err())
		close(inputChan)
	}()
	var res []string
	options, err := fzf.ParseOptions(
		true,
		fzfOpts,
	)
	check(err)
	options.Input = inputChan
	options.Output = nil
	options.Printer = func(a string) { res = append(res, a) }
	code, err := fzf.Run(options)
	check(err)
	return res, code
}

// OneID - Pick a single entry/textobj from stored rows
func OneID(t *state.State) bool {
	var code int
	// Pick something
	res, code := Pick(t.Rows, t.Opts.Fzf)
	// Ensure something has been picker
	if len(res) == 0 || code != 0 {
		t.ID = state.ID{}
		return false
	}
	var s string
	// Parse the result
	s = res[0]
	t.FzfResult = s // Raw result can be used e.g. as a header
	parts := strings.Split(res[0], statements.Sep)
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
		_, err := db.Exec("update entry set lastpick = unixepoch('now') where id = ?", entryID)
		check(err)
		check(db.Close())
	}
	return true
}
