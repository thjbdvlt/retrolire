// Package pick - Pick an entry with FZF
package pick

import (
	"database/sql"
	fzf "github.com/junegunn/fzf/src"
	"log"

	"polyanthea/internal/state"
)

// NothingHasBeenPicked - Code returned if code run witout error but nothing has been picked
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

// Pick - Pick an entry (or something else) with fzf and call callback function
func Pick(t state.State, rows *sql.Rows, fzfOpts []string) ([]string, int) {
	if rows == nil {
		log.Fatal("nil rows to fzf")
	}
	fzfOpts = append(FzfGlobalOpts(), fzfOpts...)
	var err error
	inputChan := make(chan string)
	state.NoErr(t, rows.Err())
	go func() {
		for rows.Next() {
			var s string
			err = rows.Scan(&s)
			if err != nil {
				_ = rows.Close()
				return
			}
			inputChan <- s
		}
		close(inputChan)
	}()
	var res []string
	options, err := fzf.ParseOptions(
		true,
		fzfOpts,
	)
	state.NoErr(t, err)
	options.Input = inputChan
	options.Output = nil
	options.Printer = func(a string) { res = append(res, a) }
	code, err := fzf.Run(options)
	state.NoErr(t, err)
	return res, code
}
