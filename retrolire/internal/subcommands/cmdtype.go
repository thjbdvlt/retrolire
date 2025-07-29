package subcommands

import (
	"strings"
	"fmt"
	"os"

	"retrolire/internal/state"
	"retrolire/internal/filters"
	"retrolire/internal/pick"
	"retrolire/internal/opts"
)

// Obj - The object type that a command operates on
type Obj int

// Object types
const (
	None     Obj = iota // For commands that take no object
	Entry               // E.g. for edit, invoke, delete...
	TextObj             // Quote, Concept, Idea
	Person              // (For later use)
	Relation            // (For later use)
	Tag                 // (For later use)
)

// Command - A subcommand definition
type Command interface {
	Fn(*state.State)    // Command-specific function called on result
	Stmt() string       // The base SELECT statement
	Clauses() []string  // Base SELECT clauses, e.g. textobj classes
	Params() []any      // Base SELECT parameters
	NQueryArgs() int    // Number of parameters required by the command-defined query
	NFnArgs() int       // Number of arguments required by the command main function
	NoPick() bool       // Don't pick something with fzf
	NoConn() bool       // Dont' connect to the database
	IsDynamicSQL() bool // This command dynamically build SQL
	OrderBy() string    // ORDER BY clause
	GroupBy() string    // GROUP BY clause
	IsTextObj() bool    // This command operates on concept/quote/idea
}

// BuildSQL - Build SQL
func BuildSQL(c Command, t *state.State) (string, []any) {
	stmt := []string{c.Stmt()}
	params := append(c.Params(), t.Args.Query...)
	clauses := c.Clauses()
	if c.IsDynamicSQL() && len(t.Args.Filters) > 0 {
		filterClauses, filterParams := filters.Parse(t.Args.Filters, t.Opts)
		clauses = append(clauses, "AND", "(")
		clauses = append(clauses, filterClauses...)
		clauses = append(clauses, ")")
		params = append(params, filterParams...)
	}
	if len(clauses) > 0 {
		clauses[0] = "WHERE"
		stmt = append(stmt, clauses...)
	}
	stmt = append(stmt, c.GroupBy(), c.OrderBy())
	s := strings.Join(stmt, " ")
	return s, params
}

// Call - Parse arguments and call command
func Call(args []string) {
	var err error
	var c Command
	t := &state.State{}
	// Parse flags
	args, t.Opts = opts.Parse(args)
	// Get the command
	args, c = GetCommandFromArgs(args, t.Opts)
	// Split the positional arguments in three slices:
	// 1. Function arguments (e.g. "update title")
	// 2. Automatic query arguments (e.g. "open" automatic filters entry with URLs)
	// 3. User-Defined filters (e.g. "cite author:antin,quintane")
	nFn, nQuery := c.NFnArgs(), c.NQueryArgs()
	nTotal := nFn + nQuery
	if len(args) < nTotal {
		fmt.Fprintf(os.Stderr, "Requires %d arguments, got %d.\n", nTotal, len(args))
		os.Exit(1)
	}
	t.Args = state.Args{Fn: args[:nFn], Filters: args[nTotal:]}
	for _, a := range args[nFn:nTotal] {
		t.Args.Query = append(t.Args.Query, a)
	}
	// Some commands don't need to parse filters
	if c.NoConn() {
		c.Fn(t)
		return
	}
	// Build the SQL query, even dynamically or not
	// stmt, params := BuildSQL(c, t.Args.Query, filterClauses, filterParams)
	stmt, params := BuildSQL(c, t)
	// FROM HERE, IT'S ACTUALLY MORE THAN PARSING ARGUMENTS
	// Connect to the database and send the Query
	db := t.Conn()
	t.Rows, err = db.Query(stmt, params...)
	// Close the database and check everything is going as we want
	check(db.Close())
	check(err)
	check(t.Rows.Err())
	// Pick something if the function requires to
	if c.NoPick() {
		c.Fn(t)
		return
	}
	picked := pick.OneID(t)
	if !picked {
		os.Exit(pick.NothingHasBeenPicked)
	}
	c.Fn(t)
}
