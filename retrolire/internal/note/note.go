package note

import (
	"bufio"
	"database/sql"
	"os"
	"regexp"
	"strings"

	"retrolire/internal/config"
	"retrolire/internal/util"
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

func parseFile(root *os.Root, id string, fp string, stmt *sql.Stmt, reExclude *regexp.Regexp, rePage *regexp.Regexp) {
	var err error
	file, err := root.Open(fp)
	check(err)
	scanner := bufio.NewScanner(file)
	linenr := 0
	var prev string
	for scanner.Scan() {
		linenr++
		line := scanner.Text()
		line = strings.TrimSpace(line)
		line = removeCommaComment(line)
		if line != "" && !isComment(line) && reExclude.FindStringIndex(line) == nil {
			pn := extractPageNumber(rePage, line)
			eq := strings.Index(line, " = ")
			objType := "idea"
			modLineNr := 0
			switch {
			case line[0] == '>':
				objType = "quote"
			case line[0] == ':':
				objType = "concept"
				line = prev
				modLineNr = -1
			case eq > 0 && eq < strings.IndexAny(line, ".,:;!?()[]{}"):
				line = line[:eq]
				objType = "concept"
			}
			_, err = stmt.Exec(id, objType, line, linenr+modLineNr, pn)
			check(err)
			prev = line
		}
	}
	check(file.Close())
}

// Parse - Parse entries note
func Parse(ids []string, lastedits []int64, db *sql.DB) {
	// Compile Regexps only once
	var rePage = regexp.MustCompile(`\((\d+)\)[.,;:]?$`)
	var reExclude = regexp.MustCompile(`^[^a-zA-Z]*$`)
	// Run the whole function in Root for safety
	root, err := os.OpenRoot(util.Dir())
	check(err)
	update := make([]bool, len(ids))
	// Delete from textobjs before re-inserting
	stmtDelete, err := db.Prepare("delete from textobj where entry = ?")
	check(err)
	// Parse notes
	stmtInsert, err := db.Prepare("insert into textobj (entry, class, text, linenr, page) values (?, ?, ?, ?, ?)")
	check(err)
	stmtUpdate, err := db.Prepare("update entry set lastedit = ? where id = ?")
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
				parseFile(root, id, fp, stmtInsert, reExclude, rePage)
				_, err = stmtUpdate.Exec(lastedits[i], id)
				check(err)
			}
		}
	}
}
