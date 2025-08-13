// Package help - Help messages for otlet
package help

import (
	"fmt"
	"os"
)

// Usage - Show program usage
func Usage() {
	_, _ = fmt.Fprintln(os.Stdout, USAGE)
}

// USAGE - Help message
const USAGE = `otlet - command line bibliography manager

SYNOPSIS
  otlet <command> [options] [filters]

COMMANDS
  edit    Edit an entry's note
  cite    Output an entry's ID
  add     Add entries
  tag     Edit an entry's tags
  list    List entries matching criteria
  json    Output entries matching criteria in JSON
  delete  Delete an entry
  update  Update a field of an entry
  init    Initiate the database
  open    Open an entry URL
  parse   Parse notes and update database

Most commands requires no arguments, but some do:
  · add {doi|isbn|bibtex|json|template} <identifier|file>
  · update <field>

FILTERS
  All commands except "init" and "add" accept filters arguments.
  There are three types of filters, parsed in following order:
    · Key:Val1,Val2: "author:antin", "title:fabulous,fantastic".
    · Tag: ".poetry", ".philosophy", ".unread".
    · Plain-Text search in note: anything else.
  Filters are combined with logical operator "AND".
  Two keywords alter this:
    · "or" replace the logical operator "AND" by "OR".
    · "not" negates the next filter.
  In your configuration file (config.go), you can define key aliases,
  so that (e.g.) "a:" is mapped to "author:" and "t:" to "title:".

EXAMPLES
  otlet init
  otlet edit --quote author:antin .read
  otlet open .unread .fiction
  otlet list author:wittgenstein,kripke .important
  otlet cite --concept .philosophy
  otlet update title
  otlet add json - < mycsl.json

OPTIONS
  -q --quote     Cite/Edit quotes instead of entries
  -c --concept   Cite/Edit concepts instead of entries
  -i --idea      Cite/Edit ideas instead of entries
  -k --keep-id   Don't generate new uniques entries IDs (command add)
  -e --exact     No fuzzy matching in fzf
  -f --force     Force note parsing no matter of modified times (command parse)

CONFIG
  Configuration is done through config/config.go
  Thus, yo need to compile the software in order to changes to apply.`
