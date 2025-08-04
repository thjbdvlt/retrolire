// Package pick - Pick an entry with FZF
package pick

import (
	"database/sql"
	fzf "github.com/junegunn/fzf/src"
	"log"

	"retrolire/internal/util"
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
