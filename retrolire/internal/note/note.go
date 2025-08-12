// Package note - Note parsing
package note

import (
	"bufio"
	"database/sql"
	"os"
	"regexp"
	"strings"

	"retrolire/internal/config"
	"retrolire/internal/fs"
	"retrolire/internal/nlp/word2vec"
	"retrolire/internal/obj"
	"retrolire/internal/state"
)

func isComment(line string) bool {
	return strings.HasPrefix(line, "(") && strings.HasSuffix(line, ")")
}

func removeCommaComment(line string) string {
	idx := strings.Index(line, ",,")
	if idx != -1 {
		line = line[:idx]
	}
	return line
}

type lineParser struct {
	class obj.Class
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

func (p parser) extractPageNumber(line string) string {
	matches := p.rePage.FindStringSubmatch(line)
	if len(matches) > 1 {
		return matches[1]
	}
	return ""
}

type parseStatements struct {
	insert    *sql.Stmt
	delete    *sql.Stmt
	insertFts *sql.Stmt
	deleteFts *sql.Stmt
	update    *sql.Stmt
}

func initStatement(s *parseStatements, tx *sql.Tx) error {
	var err error
	s.delete, err = tx.Prepare(`DELETE FROM textobj WHERE entry = ?`)
	if err != nil {
		return err
	}
	s.deleteFts, err = tx.Prepare(`DELETE FROM fts WHERE id = ? AND LINE > 0`)
	if err != nil {
		return err
	}
	s.insert, err = tx.Prepare(`INSERT INTO textobj
	(entry, class, text, least, linenr, page, vec)
	VALUES (?, ?, ?, ?, ?, ?, ?)`)
	if err != nil {
		return err
	}
	s.insertFts, err = tx.Prepare(`INSERT INTO fts
	(id, line, main)
	VALUES (?, ?, ?)`)
	if err != nil {
		return err
	}
	s.update, err = tx.Prepare(`UPDATE entry SET lastedit = ? WHERE id = ?`)
	if err != nil {
		return err
	}
	return nil
}

type parser struct {
	root       *os.Root
	reExclude  *regexp.Regexp
	rePage     *regexp.Regexp
	parsers    []lineParser
	vectorizer *word2vec.Vectorizer
	*parseStatements
}

func newParser(db *sql.DB) *parser {
	return &parser{
		rePage:     regexp.MustCompile(`\((\d+)\)[.,;:]?$`),
		reExclude:  regexp.MustCompile(`^[^a-zA-Z]*$`),
		parsers:    initLineParsers(),
		root:       fs.Root(),
		vectorizer: word2vec.NewVectorizer(db),
	}
}

func parseFile(psr parser, id string, fp string) error {
	var err error
	file, err := psr.root.OpenFile(fp, os.O_RDONLY, 0)
	if err != nil {
		return err
	}
	scanner := bufio.NewScanner(file)
	linenr := 0
	for scanner.Scan() {
		linenr++
		line := scanner.Text()
		line = strings.TrimSpace(line)
		line = removeCommaComment(line)
		if line != "" && !isComment(line) && psr.reExclude.FindStringIndex(line) == nil {
			pn := psr.extractPageNumber(line)
			for _, p := range psr.parsers {
				main, least, ok := p.fn(line)
				if ok {
					var bvec []byte
					tokens, vector := psr.vectorizer.Vectorize(main)
					if vector != nil {
						bvec, err = vector.AsBytes()
						if err != nil {
							bvec = nil
						}
					}
					_, err = psr.insert.Exec(id, p.class, main, least, linenr, pn, bvec)
					if err != nil {
						panic(err)
					}
					_, err = psr.insertFts.Exec(id, linenr, strings.Join(tokens, " "))
					if err != nil {
						panic(err)
					}
					break
				}
			}
		}
	}
	return file.Close()
}

// Parse entries note
func Parse(t state.State, ids []string, lastedits []int64) error {
	db := t.DB()
	update := make([]bool, len(ids))
	tx, err := db.Begin()
	if err != nil {
		return err
	}
	psr := newParser(db)
	psr.parseStatements = &parseStatements{}
	err = initStatement(psr.parseStatements, tx)
	if err != nil {
		return err
	}
	for i, id := range ids {
		fp := id + config.Ext
		fileInfo, err := psr.root.Stat(fp)
		if err == nil {
			modified := fileInfo.ModTime().Unix()
			if modified > lastedits[i] {
				lastedits[i] = modified
				update[i] = true
				_, err = psr.delete.Exec(id)
				if err != nil {
					return err
				}
				_, err = psr.deleteFts.Exec(id)
				if err != nil {
					return err
				}
				err = parseFile(*psr, id, fp)
				if err != nil {
					return err
				}
				_, err = psr.update.Exec(lastedits[i], id)
				if err != nil {
					return err
				}
			}
		}
	}
	err = tx.Commit()
	if err != nil {
		return err
	}
	return nil
}

// ParseAll - Parse all entries notes
func ParseAll(t state.State) error {
	db := t.DB()
	var ids []string
	var lastedits []int64
	rows, err := db.Query(`SELECT id, 0 FROM entry`)
	if err != nil {
		return err
	}
	t.Log(rows.Close())
	if err = rows.Err(); err != nil {
		return err
	}
	defer rows.Close()
	for rows.Next() {
		var id string
		var lastedit int64
		err = rows.Scan(&id, &lastedit)
		if err != nil {
			return err
		}
		ids = append(ids, id)
		lastedits = append(lastedits, lastedit)
	}
	return Parse(t, ids, lastedits)
}
