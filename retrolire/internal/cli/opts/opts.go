// Package opts - Parse command line options
package opts

import (
	"errors"
	flag "github.com/spf13/pflag"

	"retrolire/internal/cli/help"
)

// Opts - Command line options
type Opts struct {
	KeepIDs     bool     // Don't change entry IDs (command "add")
	NoPager     bool     // Deactivate pager (command "list")
	TextObj     bool     // Either Quote or Concept or Line is set to true
	TextObjName string   // "quote", "concept", "idea"
	Fzf         []string // Addition options for FZF
	Force       bool     // Force parsing
}

// initOpts - Initialize the flags parser
func initOpts() (*flag.FlagSet, *Opts) {
	var o Opts
	// Flag parser
	fs := flag.NewFlagSet("retrolire", flag.ExitOnError)
	flag.ErrHelp = errors.New("")
	fs.Usage = help.Usage
	fs.SortFlags = false
	// Options --quote, --concept and --idea share same logic
	for _, i := range []string{"quote", "concept", "idea"} {
		fs.BoolFuncP(i, i[:1], "Cite/Edit "+i, func(string) error {
			o.TextObj = true
			o.TextObjName = i
			o.Fzf = append(o.Fzf, "--wrap", "--wrap-sign", " .")
			return nil
		})
	}
	fs.BoolVarP(&o.KeepIDs, "keep-id", "k", false, "Don't generate new uniques entries IDs (command add)")
	fs.BoolFuncP("exact", "e", "No fuzzy matching (fzf)", func(string) error {
		o.Fzf = append(o.Fzf, "--exact")
		return nil
	})
	fs.BoolVarP(&o.Force, "force", "f", false, "Parse notes even if modified time is older that last parsing (command parse)")
	return fs, &o
}

// Parse - Parse command line options. Returns positional arguments and options
func Parse(args []string) ([]string, *Opts, error) {
	fs, opts := initOpts()
	err := fs.Parse(args)
	if err != nil {
		return nil, nil, err
	}
	args = fs.Args()
	return args, opts, nil
}
