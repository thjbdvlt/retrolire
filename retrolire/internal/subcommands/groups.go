// Package subcommands
// This file defines groups of commands
package subcommands

import (
	"retrolire/internal/state"
	"retrolire/internal/statements"
)

// noQueryCmd - Command that does nothing and does not query the database.
type noQueryCmd struct{}

func (noQueryCmd) Stmt() string       { return "" }
func (noQueryCmd) Clauses() []string  { return []string{} }
func (noQueryCmd) Params() []any      { return []any{} }
func (noQueryCmd) NFnArgs() int       { return 0 }
func (noQueryCmd) NQueryArgs() int    { return 0 }
func (noQueryCmd) NoPick() bool       { return true }
func (noQueryCmd) NoConn() bool       { return true }
func (noQueryCmd) IsDynamicSQL() bool { return false }
func (noQueryCmd) Obj() Obj           { return None }
func (noQueryCmd) IsTextObj() bool    { return false }
func (noQueryCmd) OrderBy() string    { return "" }
func (noQueryCmd) GroupBy() string    { return "" }

// pickEntryCmd - Command that pick an entry and do something on it
type pickEntryCmd struct{ noQueryCmd }

func (pickEntryCmd) NoConn() bool       { return false }
func (pickEntryCmd) IsDynamicSQL() bool { return true }
func (pickEntryCmd) Stmt() string       { return statements.Entry }
func (pickEntryCmd) Obj() Obj           { return Entry }
func (pickEntryCmd) NoPick() bool       { return false }
func (pickEntryCmd) OrderBy() string    { return statements.OrderBy }
func (pickEntryCmd) GroupBy() string    { return statements.GroupID }

// pickTextObjCmd - Command that operates on textobj instead of entries
type pickTextObjCmd struct{ pickEntryCmd }

func (pickTextObjCmd) Stmt() string {
	return `select o.asline from textobj o join entry e on e.id = o.entry`
}
func (pickTextObjCmd) Obj() Obj          { return TextObj }
func (pickTextObjCmd) IsTextObj() bool   { return true }
func (pickTextObjCmd) Clauses() []string { return []string{"WHERE", "o.class = ?"} }
func (pickTextObjCmd) OrderBy() string   { return "" }
func (pickTextObjCmd) GroupBy() string   { return "" }
func (pickTextObjCmd) NQueryArgs() int   { return 1 }

// printQueryCmd - Command that query the database and print the result to Stdout
type printQueryCmd struct {
	stmt string
	sep  string
	noQueryCmd
}

func (printQueryCmd) NoConn() bool        { return false }
func (printQueryCmd) IsDynamicSQL() bool  { return true }
func (c printQueryCmd) Stmt() string      { return c.stmt }
func (c printQueryCmd) Fn(t *state.State) { toStdout(t.Rows, c.sep) }

// compCmd - Command that query without dynamic SQL generation and print to stdout
type cmpCmd struct {
	stmt string
	noQueryCmd
}

func (cmpCmd) NoConn() bool        { return false }
func (cmpCmd) IsDynamicSQL() bool  { return false }
func (c cmpCmd) Stmt() string      { return c.stmt }
func (c cmpCmd) Fn(t *state.State) { toStdout(t.Rows, " ") }
