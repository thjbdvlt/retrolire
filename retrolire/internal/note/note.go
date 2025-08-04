package note

import (
	"bufio"
	"database/sql"
	"os"
	"regexp"
	"strings"

	"retrolire/internal/config"
	"retrolire/internal/state"
	"retrolire/internal/fs"
	"retrolire/internal/util"
	"retrolire/internal/obj"
)

var check = util.Check

// isComment - Check if a line is a comment
func isComment(line string) bool {
	return strings.HasPrefix(line, "(") && strings.HasSuffix(line, ")")
}

// removeCommaComment - Remove double comma comments (",,")
func removeCommaComment(line string) string {
	idx := strings.Index(line, ",,")
	if idx != -1 {
		line = line[:idx]
	}
	return line
}

func extractPageNumber(re *regexp.Regexp, line string) string {
	matches := re.FindStringSubmatch(line)
	if len(matches) > 1 {
		return matches[1]
	}
	return ""
}

type lineParser struct {
	class obj.Obj
	fn    func(line string) (main string, least string, ok bool)
}

func parserPrefix(prefix string) func(string) (string, string, bool) {
	return func(line string) (string, string, bool) {
		if !strings.HasPrefix(line, prefix) {
			return "", "", false
		}
		return line[len(prefix):], "", true
	}
}

func parserRegexp(re *regexp.Regexp) func(string) (string, string, bool) {
	return func(line string) (string, string, bool) {
		submatches := re.FindStringSubmatch(line)
		if ls := len(submatches); ls > 1 {
			if ls > 2 {
				return submatches[1], submatches[2], true
			}
			return submatches[1], "", true
		}
		return "", "", false
	}
}

func initLineParsers() []lineParser {
	return []lineParser{
		// Markdown heading. No ATX-style because the note parsing is 100% line-based.
		{
			class: obj.Heading,
			fn:    parserRegexp(regexp.MustCompile(`^#+ (.*)`)),
		},
		// Markdown blockquotes
		{
			class: obj.Quote,
			fn:    parserPrefix("> "),
		},
		// Pandoc's numbered example lists
		{
			class: obj.Example,
			fn:    parserPrefix("(@) "),
		},
		// Concept definition parsing doesn't use Pandoc's style because of line-based parsing.
		{
			class: obj.Concept,
			fn:    parserRegexp(regexp.MustCompile(`^([^;:.?!"]+) = (.*)`)),
		},
		// https://typst.app/docs/reference/model/terms/
		{
			class: obj.Concept,
			fn:    parserRegexp(regexp.MustCompile(`^/([a-zA-Z][^;:.?!]*):(.*)`)),
		},
		// **Strong** (__strong__) is also parsed as concept definition. *Italic* isn't.
		{
			class: obj.Concept,
			fn:    parserRegexp(regexp.MustCompile(`__([^:;.?!()])__(.*)`)),
		},
		{
			class: obj.Concept,
			fn:    parserRegexp(regexp.MustCompile(`\*\*([^:;.?!()])\*\*(.*)`)),
		},
		// Everything that hasn't been parsed by something until the end is a generic "idea".
		{
			class: obj.Idea,
			fn:    func(line string) (string, string, bool) { return line, "", true },
		},
	}
}

func parseFile(root *os.Root, id string, fp string, stmt *sql.Stmt, reExclude *regexp.Regexp, rePage *regexp.Regexp, parsers []lineParser) {
	var err error
	file, err := root.OpenFile(fp, os.O_RDONLY, 0)
	if err != nil {
		return
	}
	scanner := bufio.NewScanner(file)
	linenr := 0
	for scanner.Scan() {
		linenr++
		line := scanner.Text()
		line = strings.TrimSpace(line)
		line = removeCommaComment(line)
		if line != "" && !isComment(line) && reExclude.FindStringIndex(line) == nil {
			pn := extractPageNumber(rePage, line)
			for _, p := range parsers {
				main, least, ok := p.fn(line)
				if ok {
					_, err = stmt.Exec(id, p.class, main, least, linenr, pn)
					check(err)
					break
				}
			}
		}
	}
	check(file.Close())
}

// Parse - Parse entries note
func Parse(ids []string, lastedits []int64, cn state.Connector) {
	// Compile Regexps only once
	var rePage = regexp.MustCompile(`\((\d+)\)[.,;:]?$`)
	var reExclude = regexp.MustCompile(`^[^a-zA-Z]*$`)
	var parsers = initLineParsers()
	// Run the whole function in Root for safety
	root := fs.Root()
	db := cn.Conn()
	update := make([]bool, len(ids))
	tx, err := db.Begin()
	check(err)
	// Delete from textobjs before re-inserting
	stmtDelete, err := tx.Prepare(`DELETE FROM textobj WHERE entry = ?`)
	check(err)
	// Parse notes
	stmtInsert, err := tx.Prepare(`INSERT INTO textobj (entry, class, text, least, linenr, page) VALUES (?, ?, ?, ?, ?, ?)`)
	check(err)
	stmtUpdate, err := tx.Prepare(`UPDATE entry SET lastedit = ? WHERE id = ?`)
	check(err)
	for i, id := range ids {
		fp := id + config.Ext
		check(err)
		fileInfo, err := root.Stat(fp)
		if err == nil {
			modified := fileInfo.ModTime().Unix()
			if modified > lastedits[i] {
				lastedits[i] = modified
				update[i] = true
				_, err = stmtDelete.Exec(id)
				check(err)
				parseFile(root, id, fp, stmtInsert, reExclude, rePage, parsers)
				_, err = stmtUpdate.Exec(lastedits[i], id)
				check(err)
			}
		}
	}
	check(tx.Commit(), db.Close())
}
