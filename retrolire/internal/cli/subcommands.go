// Package cli - Command line usage
package cli

import (
	"bufio"
	"bytes"
	"fmt"
	"os"
	"os/exec"
	"path"
	"strings"

	"retrolire/internal/actions"
	"retrolire/internal/bibtex"
	"retrolire/internal/cli/opts"
	"retrolire/internal/cli/pick"
	"retrolire/internal/config"
	"retrolire/internal/fs"
	"retrolire/internal/nlp"
	"retrolire/internal/nlp/word2vec"
	"retrolire/internal/note"
	"retrolire/internal/obj"
	"retrolire/internal/sqlmaker"
	"retrolire/internal/state"
	"retrolire/internal/tui"
	"retrolire/internal/util"
)

type cmd = cliCommand

var commands = map[string]cmd{
	"vectors":      cmd{fn: initVectors},
	"stopwords":    cmd{fn: editStopWords},
	"tui":          cmd{fn: initTUI},
	"cite":         cmd{fn: cite, stmt: sqlmaker.EntryStmt, pick: true},
	"edit":         cmd{fn: edit, stmt: sqlmaker.EntryStmt, pick: true},
	"update":       cmd{fn: update, stmt: sqlmaker.EntryStmt, nArgs: 1, pick: true},
	"tag":          cmd{fn: tag, stmt: sqlmaker.EntryStmt, pick: true},
	"tag-pick":     cmd{fn: tagPick, stmt: sqlmaker.EntryStmt, pick: true},
	"delete":       cmd{fn: del, stmt: sqlmaker.EntryStmt, pick: true},
	"open":         cmd{fn: open, stmt: sqlmaker.OpenEntryStmt, pick: true},
	"textobj-edit": cmd{fn: edit, stmt: sqlmaker.TextobjStmt, pick: true},
	"textobj-cite": cmd{fn: cite, stmt: sqlmaker.TextobjStmt, pick: true},
	"list":         cmd{fn: list, stmt: sqlmaker.ListStmt},
	"edit-tags":    cmd{fn: editTagsTree},
	"parse":        cmd{fn: parse, stmt: sqlmaker.ParseStmt},
	"add":          cmd{fn: add, nArgs: 2},
	"init":         cmd{fn: initDB},
	"json":         cmd{fn: toStdoutSpace, stmt: sqlmaker.JSONStmt},
	"_output":      cmd{fn: toStdoutZero, stmt: sqlmaker.EntryStmt},
	"_filepath":    cmd{fn: printFilepath},
	"_fzfopts":     cmd{fn: printFzfOpts},
	"_tag":         cmd{fn: toStdoutSpace, stmt: sqlmaker.CmpTagStmt},
	"_author":      cmd{fn: toStdoutSpace, stmt: sqlmaker.CmpAuthorStmt},
	"_field":       cmd{fn: toStdoutSpace, stmt: sqlmaker.CmpFieldStmt},
}

func getCommandFromArgs(args []string, options *opts.Opts) ([]string, cliCommand) {
	var c cmd
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
	// So "retrolire cite -q" is just like "retrolire textobj-cite 2"
	if options.TextObjName != "" {
		textobjID := obj.FromName(options.TextObjName)
		args = append(append([]string{}, fmt.Sprintf("%d", textobjID)), args...)
	}
	return args, c
}

func getByName(name string, options *opts.Opts) (cmd, bool) {
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

var check = util.Check
var check2 = util.Check2

func cite(t *CliState) {
	_, _ = fmt.Fprint(os.Stdout, t.ID.Entry)
	if t.ID.Page != "" {
		_, _ = fmt.Fprint(os.Stdout, t.ID.Page)
	}
}

func edit(t *CliState) {
	if id, line := t.ID.Entry, t.ID.Line; id != "" {
		check(actions.EditEntryLine(t, id, line))
	}
}

func del(t *CliState) {
	fmt.Println(actions.Head(t, t.ID.Entry)) // Show selected entry
	if util.ConfirmUser("Confirm deletion?") {
		check(actions.DeleteEntry(t, t.ID.Entry))
		fmt.Println("Deleted:", t.ID.Entry)
	}
}

func update(t *CliState) {
	check(actions.UpdateEntryField(t, t.ID.Entry, t.Args[0]))
}

func tag(t *CliState) {
	check(actions.EditEntryTags(t, t.ID.Entry))
}

func tagPick(t *CliState) {
	var err error
	db := t.Conn()
	rows, err := db.Query(`WITH x AS (
  SELECT tag, count(DISTINCT entry) AS count
  FROM tag
  GROUP BY tag
)
SELECT x.tag
FROM x
ORDER BY x.count DESC`)
	check(err, rows.Err(), db.Close())
	head := actions.Head(t, t.ID.Entry)
	tags, _ := pick.Pick(rows, []string{"--multi", "--header", head})
	check(actions.UpdateEntryTags(t, t.ID.Entry, tags, false))
}

func open(t *CliState) {
	check(actions.OpenEntryURL(t, t.ID.Entry))
}

func fromIsbnOrDoi(method string, identifier string) []byte {
	var bufOut bytes.Buffer
	sh := exec.Command("fetchref", method, identifier)
	sh.Stdout = &bufOut
	sh.Stderr = os.Stderr
	err := sh.Run()
	if err != nil {
		os.Exit(1)
	}
	return bufOut.Bytes()
}

func add(t *CliState) {
	popen2 := util.Popen2
	var buf bytes.Buffer
	// Data is read from stdin if "-"
	// TODO: Bibtex / JSON from file (or error)
	data := t.Args[1]
	if data == "-" {
		check2(buf.ReadFrom(os.Stdin))
		data = buf.String()
	}
	bdata := []byte(data)
	method := t.Args[0]
	pandoc := []string{"pandoc", "-f", "biblatex", "-t", "csljson"}
	switch method {
	case "json":
	case "bibtex":
		bdata = popen2(bdata, pandoc)
	case "doi", "isbn":
		bdata = popen2(fromIsbnOrDoi(method, data), pandoc)
		bdata = util.EditTemp(bdata)
	case "template":
		template, ok := bibtex.GetTemplate(data)
		if !ok {
			os.Exit(1)
		}
		bdata = util.EditTemp(template)
		bdata = popen2(bdata, pandoc)
	default:
		fmt.Println("Unknown method:", method)
		os.Exit(1)
	}
	db := t.Conn()
	// Get IDs
	row := db.QueryRow(`SELECT coalesce(group_concat(id, ' '), '') FROM entry`)
	var ids string
	check(row.Scan(&ids))
	check(db.Close())
	// Make unique IDs (if option --keep-id isn't set)
	if !t.Opts.KeepIDs {
		bdata = popen2(bdata, []string{"csljson-update", "-", ids})
	}
	// Add entries to the database
	db = t.Conn()
	check2(db.Exec(`WITH x AS (SELECT value FROM json_each($1))
INSERT INTO entry (id, csl, vec)
SELECT value ->> 'id', value, NULL
FROM x`, string(bdata)))
	check(word2vec.VectorizeEntriesTitle(db, true), db.Close())
}

func parse(t *CliState) {
	// Parse notes matching filters
	var ids []string
	var lastedits []int64
	check(t.Rows.Err())
	for t.Rows.Next() {
		var id string
		var lastedit int64
		check(t.Rows.Scan(&id, &lastedit))
		ids = append(ids, id)
		if !t.Opts.Force {
			lastedits = append(lastedits, lastedit)
		} else {
			lastedits = append(lastedits, 0)
		}
	}
	check(t.Rows.Close())
	db := t.Conn()
	check(note.Parse(ids, lastedits, t), db.Close())
}

func getIndent(b string) int {
	for i, v := range b {
		if v != ' ' {
			return i
		}
	}
	return 0
}

func parseStopWords(t *CliState) {
	db := t.Conn()
	err := nlp.UpdateStopWords(db)
	if err != nil {
		panic(err)
	}
	err = db.Close()
	if err != nil {
		panic(err)
	}
}

func parseTags(t *CliState) {
	// Open the tag file
	root := fs.Root()
	file, err := root.Open(fs.TagFile)
	check(err, root.Close())
	// Two slices to store line contents and indent levels
	lines := []string{}
	indents := []int{}
	// Iterate over the tag file lines. Each line defines a tag
	scanner := bufio.NewScanner(file)
	var maxIndent int
	n := 0
	for scanner.Scan() {
		line := scanner.Text()
		indent := getIndent(line) // Indent = hierarchy depth
		// Increase maxIndent, needed to build the tree slice
		if indent > maxIndent {
			maxIndent = indent
		}
		lines = append(lines, strings.TrimSpace(line))
		indents = append(indents, indent)
		n++
	}
	tree := make([]string, maxIndent+1) // Store tag hierarchy
	// Delete the tagDef table that describe the tag hierarchy
	db := t.Conn()
	tx, err := db.Begin()
	check(err)
	check2(tx.Exec(`DELETE FROM tagDef`))
	check(err)
	// Re-Create the hierarchy
	stmt, err := tx.Prepare(`INSERT INTO tagDef (tag, isA) VALUES (?, ?)`)
	check(err)
	for i, line := range lines {
		// Aliases
		if strings.Contains(line, "=") {
			s := strings.Split(line, "=")
			if len(s) > 1 {
				name := strings.TrimSpace(s[0])
				for _, alias := range s[1:] {
					check2(stmt.Exec(name, strings.TrimSpace(alias)))
				}
				line = name // Remove aliases for hierarchy
			}
		}
		// Hierarchy
		indent := indents[i]
		tree[indent] = line
		for y := 0; y < indent; y++ {
			check2(stmt.Exec(line, tree[y]))
		}
	}
	check2(tx.Exec(`DELETE FROM tag WHERE implicit = true`))
	check2(tx.Exec(`INSERT INTO tag (entry, tag, implicit)
SELECT distinct t.entry, td.isA, true
FROM tag t
JOIN tagDef td ON t.tag = td.tag`))
	// 2025-08-06: I changed the database structure for tags to a JSON-based tag system.
	// Queries are WAY faster now. This function is a workaround before I update all code.
	// But it takes a lot of times, so it's not ideal at all.
	check2(tx.Exec(`WITH x AS (
  SELECT entry, json_group_object(tag, 1) AS tags
  FROM tag GROUP BY ENTRY
)
UPDATE entry AS e
SET tags = coalesce((SELECT x.tags FROM x WHERE x.entry = e.id), '{}')`))
	check(tx.Commit(), db.Close())
}

func idToPath(s string) (fname string, line string) {
	id := state.IDFromString(s)
	if id.Entry == "" {
		return "", ""
	}
	fname = id.Entry + config.Ext
	return fname, id.Line
}

func printFzfOpts(*CliState) {
	_, _ = fmt.Fprint(os.Stdout, strings.Join(pick.FzfGlobalOpts(), " "))
}

func printFilepath(t *CliState) {
	if len(t.Args) == 0 {
		return
	}
	fp, linenr := idToPath(t.Args[0])
	_, _ = fmt.Fprint(os.Stdout, path.Join(config.Directory, fp), " ", linenr)
}

func list(t *CliState) {
	rows := t.Rows
	var data string
	var in bytes.Buffer
	sh := exec.Command("less", "-r", "-")
	sh.Dir = config.Directory
	sh.Stdin = &in
	sh.Stdout = os.Stdout
	sh.Stderr = os.Stderr
	for rows.Next() {
		check(rows.Scan(&data))
		if data != "" {
			check2(fmt.Fprint(&in, data, "\n"))
		}
	}
	check(rows.Close(), sh.Run())
}

func toStdoutSpace(t *CliState) { toStdout(t, " ") }
func toStdoutZero(t *CliState)  { toStdout(t, "\000") }
func toStdout(t *CliState, sep string) {
	rows := t.Rows
	if rows == nil {
		return
	}
	var data string
	for rows.Next() {
		check(rows.Scan(&data))
		if data != "" {
			check2(fmt.Fprint(os.Stdout, data, sep))
		}
	}
	check(rows.Close())
}

func initTUI(t *CliState) { tui.InitApp(&t.State) }

func initVectors(t *CliState) {
	err := word2vec.Train(&t.State)
	if err != nil {
		panic(err)
	}
	db := t.Conn()
	check(word2vec.VectorizeEntriesTitle(db, false))
	check(db.Close())
	check(note.ParseAll(&t.State))
}

func editStopWords(t *CliState) {
	fs.CD()
	util.EditFile(fs.StopWordFile)
	parseStopWords(t)
}

func editTagsTree(t *CliState) {
	fs.CD()
	util.EditFile(fs.TagFile)
	parseTags(t)
}
