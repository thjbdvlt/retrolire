// Package config - Configure Retrolire
package config

// Directory - Directory containing notes and database (as .retrolire.db)
const Directory = "~/retrolire"

// DefaultCmd - The default command when none is explicitly used
const DefaultCmd = "edit"

// Ext - The extension for note
const Ext = ".md"

// Editor - The program to edit notes with
const Editor = "nvim"

// EditorFlagLineNr - The flag before the line number in the editor command
// The generated command will looks be:
// Editor <file> EditorFlagLineNr <linenr>
// e.g.: {"nvim", "perec1985", "-c", "48"}
const EditorFlagLineNr = "-c"

// Opener - Program to open files or URLs depending on extension/name
const Opener = "xdg-open"

// AliasesCommand - Command names aliases
var AliasesCommand = map[string]string{
	"ed": "edit",
	"ci": "cite",
	"up": "update",
	"ad": "add",
	"a":  "add",
}

// AliasesCSL - Aliases to CSL variable, e.g. "author", "container-title"
var AliasesCSL = map[string]string{
	"a":  "author",
	"t":  "title",
	"ca": "container-author",
	"ct": "container-title",
}

// FilterFieldValueSep - String used to delimit FIELD/VALUE, e.g. "author:antin"
const FilterFieldValueSep = ":"

// FilterValuesSep - String used to delimits multiples values in FIELD:VALUE filter
const FilterValuesSep = ","

// FilterTagPrefix - Tag prefix for filters
const FilterTagPrefix = "."

// FilterAuthorPrefix - Prefix for author search
const FilterAuthorPrefix = "@"
