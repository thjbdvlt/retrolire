package subcommands

import (
	"strings"

	"retrolire/internal/config"
	"retrolire/internal/opts"
	"retrolire/internal/statements"
	"retrolire/internal/util"
)

var commands = map[string]Command{
	"cite":         citeCmd{},
	"edit":         editCmd{},
	"open":         openCmd{},
	"update":       updateCmd{},
	"tag":          tagCmd{},
	"tag-pick":     tagPickCmd{},
	"delete":       deleteCmd{},
	"textobj-edit": editTextObjCmd{},
	"textobj-cite": citeTextObjCmd{},
	"list":         listCmd{},
	"parse":        parseCmd{},
	"add":          addCmd{},
	"init":         initCmd{},
	"json":         printQueryCmd{stmt: statements.JSON, sep: "\n"},
	"_output":      printQueryCmd{stmt: statements.Entry, sep: "\000"},
	"_fzfopts":     printFzfOptsCmd{},
	"_filepath":    printFilepathCmd{},
	"_tag":         cmpCmd{stmt: "select distinct '.' || tag from tag"},
	"_author":      cmpCmd{stmt: "select distinct '@' || lower(author) from entry"},
	"_field":       cmpCmd{stmt: "select distinct x.key from entry, json_each(csl) as x"},
}

// GetCommandFromArgs - Get command from command line arguments
func GetCommandFromArgs(args []string, options *opts.Opts) ([]string, Command) {
	var c Command
	var ok bool
	var name string
	if len(args) > 0 {
		name = args[0]
	}
	// Get the command by name, or use default one
	if c, ok = getByName(name, options); ok {
		args = args[1:]
	} else if c, ok = getByName(config.DefaultCmd, options); !ok {
		util.Abort("Unknown command (config):", config.DefaultCmd)
	}
	// If a "textobj-" command have been used through -c/-q/-i flag, add textobj class
	// So "retrolire cite -q" is just like "retrolire textobj-cite quote"
	if options.TextObjName != "" {
		args = append(append([]string{}, options.TextObjName), args...)
	}
	return args, c
}

func getByName(name string, options *opts.Opts) (Command, bool) {
	aliasedName, ok := config.AliasesCommand[name]
	if ok {
		name = aliasedName
	}
	if options.TextObj {
		name = "textobj-" + name
	} else if strings.HasPrefix(name, "textobj-") {
		options.TextObj = true
	}
	c, ok := commands[name]
	return c, ok
}
