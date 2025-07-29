package subcommands

import (
	"bytes"
	"database/sql"
	"fmt"
	"os"
	"os/exec"
	"path"
	"strings"

	"retrolire/internal/config"
	"retrolire/internal/pick"
	"retrolire/internal/state"
	"retrolire/internal/statements"
	"retrolire/internal/util"
)

// printQueryCmd - Command that doesn't pick with fzf but still build SQL
type listCmd struct{ printQueryCmd }
type printFzfOptsCmd struct{ noQueryCmd }
type printFilepathCmd struct{ noQueryCmd }

func (listCmd) Stmt() string      { return statements.List }
func (listCmd) OrderBy() string   { return statements.OrderBy }
func (listCmd) GroupBy() string   { return statements.GroupID }
func (listCmd) Params() []any     { return []any{"\033[35m", "\033[0m"} }
func (listCmd) Fn(t *state.State) { toPager(t.Rows) }

func (printFzfOptsCmd) Fn(t *state.State)  { printFzfOpts(t) }
func (printFilepathCmd) Fn(t *state.State) { printFilepath(t) }

// Output rows to LESS pager
func toPager(rows *sql.Rows) {
	var data string
	var err error
	var in bytes.Buffer
	sh := exec.Command("less", "-r", "-")
	sh.Dir = util.Dir()
	sh.Stdin = &in
	sh.Stdout = os.Stdout
	sh.Stderr = os.Stderr
	defer rows.Close()
	for rows.Next() {
		util.Check(rows.Scan(&data))
		if data != "" {
			_, err = fmt.Fprint(&in, data, "\n")
			util.Check(err)
		}
	}
	err = sh.Run()
	util.Check(err)
}

// Output rows to STDOUT
func toStdout(rows *sql.Rows, sep string) {
	if rows == nil {
		return
	}
	var data string
	var err error
	defer rows.Close()
	for rows.Next() {
		util.Check(rows.Scan(&data))
		if data != "" {
			_, err = fmt.Fprint(os.Stdout, data, sep)
			util.Check(err)
		}
	}
}

// Full path from entry ID
func idToPath(s string) (fname string, line string) {
	id := state.IDFromString(s)
	if id.Entry == "" {
		return "", ""
	}
	fname = id.Entry + config.Ext
	return fname, id.Line
}

// Print fzf options
func printFzfOpts(*state.State) {
	_, _ = fmt.Fprint(os.Stdout, strings.Join(pick.FzfGlobalOpts(), " "))
}

// Print filepath
func printFilepath(t *state.State) {
	if len(t.Args.Fn) == 0 {
		return
	}
	fp, linenr := idToPath(t.Args.Fn[0])
	_, _ = fmt.Fprint(os.Stdout, path.Join(util.Dir(), fp), " ", linenr)
}
