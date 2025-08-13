// Package cli - Command line usage
package cli

import (
	"bytes"
	"fmt"
	"os"
	"os/exec"
	"path"
	"strings"

	"otlet/internal/actions"
	"otlet/internal/bibtex"
	"otlet/internal/cli/opts"
	"otlet/internal/cli/pick"
	"otlet/internal/config"
	"otlet/internal/edit"
	"otlet/internal/files"
	"otlet/internal/nlp"
	"otlet/internal/nlp/word2vec"
	"otlet/internal/note"
	"otlet/internal/obj"
	"otlet/internal/sqlmaker"
	"otlet/internal/state"
	"otlet/internal/tui"
)

type cmd = cliCommand

var commands = map[string]cmd{
	"annots":       cmd{fn: pdfAnnots, pick: true, stmt: sqlmaker.EntryStmt, nArgs: 1},
	"vectors":      cmd{fn: initVectors},
	"stopwords":    cmd{fn: editStopWords},
	"tui":          cmd{fn: initTUI},
	"cite":         cmd{fn: cite, stmt: sqlmaker.EntryStmt, pick: true},
	"edit":         cmd{fn: editEntry, stmt: sqlmaker.EntryStmt, pick: true},
	"update":       cmd{fn: update, stmt: sqlmaker.EntryStmt, nArgs: 1, pick: true},
	"tag":          cmd{fn: tag, stmt: sqlmaker.EntryStmt, pick: true},
	"tag-pick":     cmd{fn: tagPick, stmt: sqlmaker.EntryStmt, pick: true},
	"delete":       cmd{fn: del, stmt: sqlmaker.EntryStmt, pick: true},
	"open":         cmd{fn: open, stmt: sqlmaker.OpenEntryStmt, pick: true},
	"textobj-edit": cmd{fn: editEntry, stmt: sqlmaker.TextobjStmt, pick: true},
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
		fmt.Fprintln(os.Stderr, "unknown command (config):", config.DefaultCmd)
		os.Exit(1)
	}
	// If a "textobj-" command have been used through -c/-q/-i flag, add textobj class
	// So "otlet cite -q" is just like "otlet textobj-cite 2"
	if options.TextObjName != "" {
		textobjID := obj.FromName(options.TextObjName)
		args = append(append([]string{}, fmt.Sprintf("%d", textobjID)), args...)
	}
	return args, c
}

func getByName(name string, options *opts.Opts) (cmd, bool) {
	if options.TextObj {
		name = "textobj-" + name
	} else if strings.HasPrefix(name, "textobj-") {
		options.TextObj = true
	}
	c, ok := commands[name]
	return c, ok
}

func cite(t *CliState) {
	_, _ = fmt.Fprint(os.Stdout, t.ID.Entry)
	if t.ID.Page != "" {
		_, _ = fmt.Fprint(os.Stdout, t.ID.Page)
	}
}

func editEntry(t *CliState) {
	if id, line := t.ID.Entry, t.ID.Line; id != "" {
		state.NoErr(t, actions.EditEntryLine(t, id, line), "Failed to edit file.")
	}
}

func del(t *CliState) {
	fmt.Println(actions.Head(t, t.ID.Entry)) // Show selected entry
	if confirmUser("Confirm deletion?") {
		state.NoErr(t, actions.DeleteEntry(t, t.ID.Entry), "Deletion failed.")
		fmt.Println("Deleted:", t.ID.Entry)
	}
}

func update(t *CliState) {
	state.NoErr(t, actions.UpdateEntryField(t, t.ID.Entry, t.Args[0]), "Update failed.")
}

func tag(t *CliState) {
	state.NoErr(t, actions.EditEntryTags(t, t.ID.Entry), "Update failed..")
}

func tagPick(t *CliState) {
	var err error
	db := t.DB()
	rows, err := db.Query(`WITH x AS (
  SELECT tag, count(DISTINCT entry) AS count
  FROM tag
  GROUP BY tag
)
SELECT x.tag
FROM x
ORDER BY x.count DESC`)
	state.NoErr(t, err, "Couldn't get tags from the database.")
	state.NoErr(t, rows.Err(), "Couldn't get tags from the database.")
	head, err := actions.Head(t, t.ID.Entry)
	state.NoErr(t, err, "This entry is not in the database:", t.ID.Entry)
	tags, _ := pick.Pick(t, rows, []string{"--multi", "--header", head})
	state.NoErr(t, actions.UpdateEntryTags(t, t.ID.Entry, tags, false), "Update failed.")
}

func open(t *CliState) {
	state.NoErr(t, actions.OpenEntryURL(t, t.ID.Entry), "Opening file failed.")
}

func fromIsbnOrDoi(t *CliState, method string, identifier string) []byte {
	var bufOut bytes.Buffer
	sh := exec.Command("fetchref", method, identifier)
	sh.Stdout = &bufOut
	sh.Stderr = os.Stderr
	state.NoErr(t, sh.Run(), "Could get reference.")
	return bufOut.Bytes()
}

func readFromStdin(t *CliState) []byte {
	var buf bytes.Buffer
	_, err := buf.ReadFrom(os.Stdin)
	if err != nil {
		t.Log(err)
		fmt.Fprintf(os.Stderr, "Couldn't read from stdin. Abort.")
		os.Exit(0)
	}
	return buf.Bytes()
}

func readFromFileOrFile(t *CliState, path string) []byte {
	var err error
	if path == "-" {
		return readFromStdin(t)
	}
	if !files.FileExists(path) {
		fmt.Fprintln(os.Stderr, "File not found:", path)
		os.Exit(0)
		return nil
	}
	err = os.Chdir(t.RunDirectory())
	if err != nil {
		t.Log(err)
		fmt.Fprintln(os.Stderr, "Error opening file:", path)
		os.Exit(0)
		return nil
	}
	data, err := os.ReadFile(path)
	if err != nil {
		fmt.Fprintln(os.Stderr, "Couldn't open file:", path)
		os.Exit(0)
		return nil
	}
	files.CD()
	return data
}

func popen2(in []byte, command []string) ([]byte, error) {
	var bufIn *bytes.Buffer
	var bufOut bytes.Buffer
	bufIn = bytes.NewBuffer(in)
	sh := exec.Command(command[0], command[1:]...)
	sh.Stdin = bufIn
	sh.Stdout = &bufOut
	sh.Stderr = os.Stderr
	err := sh.Run()
	if err != nil {
		return nil, err
	}
	return bufOut.Bytes(), nil
}

func add(t *CliState) {
	var err error
	method := t.Args[0]
	data := t.Args[1]
	// Get existing IDs
	row := t.DB().QueryRow(`SELECT coalesce(group_concat(id, ' '), '') FROM entry`)
	var ids string
	state.NoErr(t, row.Scan(&ids), "Couldn't get data from the database. Aborted.")
	_ = t.DB().Close() // Close database while executing shell subprocesses
	var bdata []byte
	pandoc := []string{"pandoc", "-f", "biblatex", "-t", "csljson"}
	switch method {
	case "json":
		bdata = readFromFileOrFile(t, data)
	case "bibtex":
		bdata, err = popen2(readFromFileOrFile(t, data), pandoc)
	case "doi", "isbn":
		bdata, err = popen2(fromIsbnOrDoi(t, method, data), pandoc)
		state.NoErr(t, err)
		bdata, err = edit.Temp(bdata)
		state.NoErr(t, err)
	case "template":
		template, ok := bibtex.GetTemplate(data)
		if !ok {
			os.Exit(1)
		}
		bdata, err = edit.Temp(template)
		state.NoErr(t, err)
		bdata, err = popen2(bdata, pandoc)
		state.NoErr(t, err)
	default:
		fmt.Println("Unknown method:", method)
		os.Exit(1)
	}
	// Make unique IDs (if option --keep-id isn't set)
	if !t.Opts.KeepIDs {
		bdata, err = popen2(bdata, []string{"csljson-update", "-", ids})
		state.NoErr(t, err)
	}
	// Add entries to the database
	db := t.DB()
	_, err = db.Exec(`WITH x AS (SELECT value FROM json_each($1))
	INSERT INTO entry (id, csl, vec)
	SELECT value ->> 'id', value, NULL
	FROM x`, string(bdata))
	state.NoErr(t, err, "something failed while adding entries")
	state.NoErr(t, word2vec.VectorizeEntriesTitle(db, true), "(no vectors)")
}

func parse(t *CliState) {
	// Parse notes matching filters
	var ids []string
	var lastedits []int64
	state.NoErr(t, t.Rows.Err(), "couldn't get data from the database")
	for t.Rows.Next() {
		var id string
		var lastedit int64
		t.Log(t.Rows.Scan(&id, &lastedit))
		ids = append(ids, id)
		if !t.Opts.Force {
			lastedits = append(lastedits, lastedit)
		} else {
			lastedits = append(lastedits, 0)
		}
	}
	state.NoErr(t, note.Parse(t, ids, lastedits), "failed to parse notes")
}

func parseStopWords(t *CliState) {
	db := t.DB()
	err := nlp.UpdateStopWords(t)
	if err != nil {
		panic(err)
	}
	err = db.Close()
	if err != nil {
		panic(err)
	}
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
	fp, line := idToPath(t.Args[0])
	_, _ = fmt.Fprint(os.Stdout, path.Join(config.Directory, fp), " ", line)
}

func list(t *CliState) {
	var data string
	var in bytes.Buffer
	var err error
	rows := t.Rows
	sh := exec.Command("less", "-r", "-")
	sh.Dir = config.Directory
	sh.Stdin = &in
	sh.Stdout = os.Stdout
	sh.Stderr = os.Stderr
	defer rows.Close()
	for rows.Next() {
		if rows.Scan(&data) != nil {
			t.Log(err)
			return
		}
		if data != "" {
			_, err = fmt.Fprint(&in, data, "\n")
			if err != nil {
				t.Log(err)
				return
			}
		}
	}
	t.Log(sh.Run())
	return
}

func toStdoutSpace(t *CliState) { toStdout(t, " ") }
func toStdoutZero(t *CliState)  { toStdout(t, "\000") }
func toStdout(t *CliState, sep string) {
	var err error
	rows := t.Rows
	if rows == nil {
		return
	}
	var data string
	for rows.Next() {
		state.NoErr(t, rows.Scan(&data), "couldn't get data")
		if data != "" {
			_, err = fmt.Fprint(os.Stdout, data, sep)
			state.NoErr(t, err, "couldn't print to stdout")
		}
	}
	state.NoErr(t, rows.Close())
}

func initTUI(t *CliState) { tui.InitApp(t.MainState) }

func initVectors(t *CliState) {
	err := word2vec.Train(t)
	if err != nil {
		t.Log(err)
		fmt.Fprintln(os.Stderr, "Couldn't train vectors.")
		os.Exit(1)
	}
	t.Log(word2vec.VectorizeEntriesTitle(t.DB(), false))
	t.Log(note.ParseAll(t))
}

func editStopWords(t *CliState) {
	files.CD()
	err := edit.File(files.StopWordFile)
	state.NoErr(t, err)
	parseStopWords(t)
}

func editTagsTree(t *CliState) {
	files.CD()
	state.NoErr(t, edit.File(files.TagFile))
}

func pdfAnnots(t *CliState) {
	_ = t.DB().Close()
	sh := exec.Command("pdfannots", "--format", "json", t.Args[0])
	sh.Dir = t.RunDirectory()
	var bufOut bytes.Buffer
	sh.Stdout = &bufOut
	sh.Stdin = os.Stdin
	sh.Stderr = os.Stderr
	err := sh.Run()
	if err != nil {
		fmt.Fprintln(os.Stderr, "Issue with pdfAnnots. Is it installed?")
		t.Log(err)
		os.Exit(1)
	}
	db := t.DB()
	_, err = db.Exec(`INSERT INTO pdf_annots
	(entry, text, color, page_label, page, class)
	SELECT
	? AS entry,
	coalesce(value ->> 'contents', value ->> 'text'),
	value ->> 'color',
	value ->> 'page_label',
	value ->> 'page',
	CASE WHEN value ->> 'type' = 'Text'  THEN ? ELSE ? END AS class
	FROM json_each(?)`, t.ID.Entry, obj.PdfAnnot, obj.Quote, bufOut.String())
	if err != nil {
		fmt.Println("Encountered some issue, sorry.")
		t.Log(err)
	}
}

func confirmUser(msg string) bool {
	fmt.Println(msg, "(y/N)")
	var input string
	_, err := fmt.Scanln(&input)
	if err != nil {
		return false
	}
	return strings.TrimSpace(input) == "y"
}
