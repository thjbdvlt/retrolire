// Package note - Note parsing
package note

import (
	"bufio"
	"database/sql"
	"encoding/json"
	"maps"
	"os"
	"regexp"
	"strings"
	"unicode"

	"otlet/internal/config"
	"otlet/internal/nlp/word2vec"
	"otlet/internal/obj"
	"otlet/internal/state"
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
		// Markdown heading. Only ATX-style because the note parsing is 100% line-based.
		{
			class: obj.Heading,
			fn:    parserRegexp(regexp.MustCompile(`^#+ (.*)`)),
		},
		// Markdown blockquotes
		{
			class: obj.Quote,
			fn:    parserPrefix("> "),
		},
		{
			class: obj.Quote,
			fn:    parserRegexp(regexp.MustCompile(`^["«]([^«"»]+)["»](.*)$`)),
		},
		// Pandoc's numbered example lists
		{
			class: obj.Example,
			fn:    parserRegexp(regexp.MustCompile(`\(@\w*\) (.*)`)),
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
			class: obj.Commentary,
			fn:    func(line string) (string, string, bool) { return line, "", true },
		},
	}
}

func (p parser) extractPageNumber(line string) string {
	var locator string
	matches := p.reLocator.FindStringSubmatch(line)
	for _, m := range matches {
		if m != "" {
			locator = m
		}
	}
	return locator
}

type parseStatements struct {
	insert         *sql.Stmt
	delete         *sql.Stmt
	insertFts      *sql.Stmt
	deleteFts      *sql.Stmt
	update         *sql.Stmt
	updateTags     *sql.Stmt // SQLite3 (SQL?) can't update multipe columns at once
	updateVecEntry *sql.Stmt
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
	(entry, class, text, least, line, locator, vec, tags)
	VALUES (?, ?, ?, ?, ?, ?, ?, ?)`)
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
	s.updateTags, err = tx.Prepare(`UPDATE entry SET tags = ? WHERE id = ?`)
	if err != nil {
		return err
	}
	s.updateVecEntry, err = tx.Prepare(`UPDATE entry set vec = ? WHERE id =?`)
	if err != nil {
		return err
	}
	return nil
}

type parser struct {
	root       *os.Root
	reLocator  *regexp.Regexp
	reTag      *regexp.Regexp
	reTagStart *regexp.Regexp
	parsers    []lineParser
	vectorizer *word2vec.Vectorizer
	*parseStatements
}

func newParser(t state.State) *parser {
	return &parser{
		reLocator:  regexp.MustCompile(`\((\d+)\)[.,;:]?$|(p\. ?\d+)|(\d+')|(\d+min\b)`),
		reTag:      regexp.MustCompile(`^,| ,`),
		reTagStart: regexp.MustCompile(`^,`),
		parsers:    initLineParsers(),
		root:       t.Root(),
		vectorizer: word2vec.NewVectorizer(t.DB()),
	}
}

const sepCompComment = config.SeparatorCompilationComment

func (p parser) collect(tags map[string]string, line string) {
	for _, tag := range p.reTag.Split(line, -1) {
		if tag == "" {
			continue
		}
		var comment string
		tag, comment, _ := strings.Cut(tag, sepCompComment)
		tag, comment = strings.TrimSpace(tag), strings.TrimSpace(comment)
		tags[tag] = comment
	}
}

func hasLetter(s string) bool {
	for _, r := range s {
		if unicode.IsLetter(r) {
			return true
		}
	}
	return false
}

type lineObject struct {
	line    int
	main    string
	least   string
	tags    map[string]string
	locator string
	bvec    []byte
	class   obj.Class
	tokens  string
}

func parseFile(psr parser, id string, fp string) error {
	var err error
	file, err := psr.root.OpenFile(fp, os.O_RDONLY, 0)
	if err != nil {
		return err
	}
	defer file.Close()
	scanner := bufio.NewScanner(file)

	var lineobjects []lineObject
	var vectors []*word2vec.Vector

	n := 0
	nLineObjects := 0
	tagsEntry := map[string]string{}
	for scanner.Scan() {
		n++
		line := scanner.Text()
		line = strings.TrimSpace(line)
		line = removeCommaComment(line)

		// Exclude comments, i.e. line entirely in parenthesis, or empty line or without text
		if line == "" || isComment(line) || !hasLetter(line) {
			continue
		}

		// Tags apply to previous row and are not inserted by themselves
		if psr.reTagStart.FindStringIndex(line) != nil {
			var m map[string]string
			if nLineObjects == 0 {
				m = tagsEntry
			} else {
				m = lineobjects[nLineObjects-1].tags
			}
			psr.collect(m, line)
			continue
		}

		nLineObjects++

		// Find locator, e.g. page number at the end.
		locator := psr.extractPageNumber(line)

		// Iterate over the line parsers to determine to which class belong the line
		for _, p := range psr.parsers {
			main, least, ok := p.fn(line)
			if ok {

				// Tokenize (for FTS5) + vectorize (for word vectors similarities)
				// There is always a set of tokens (even an empty one), but not always vectors
				var bvec []byte
				tokens, vector := psr.vectorizer.Vectorize(main)
				if vector != nil {
					vectors = append(vectors, vector)
					bvec, err = vector.AsBytes()
					if err != nil {
						bvec = nil
					}
				}

				// Build the line object, append to the array
				lo := lineObject{
					class:   p.class,
					main:    main,
					least:   least,
					line:    n,
					tags:    maps.Clone(tagsEntry), // Clone entry tags
					tokens:  strings.Join(tokens, " "),
					locator: locator,
					bvec:    bvec,
				}
				lineobjects = append(lineobjects, lo)
				break
			}
		}
	}

	for _, i := range lineobjects {
		jTags, err := json.Marshal(i.tags)
		if err != nil {
			return err
		}
		_, err = psr.insert.Exec(id, i.class, i.main, i.least, i.line, i.locator, i.bvec, jTags)
		if err != nil {
			return err
		}
		_, err = psr.insertFts.Exec(id, i.line, i.tokens)
		if err != nil {
			return err
		}
	}

	// Update entry's tags
	jTags, err := json.Marshal(tagsEntry)
	if err != nil {
		return err
	}
	_, err = psr.updateTags.Exec(jTags, id)

	// Update entry's vector (average of all textobjs)
	var bvec []byte
	if len(vectors) > 0 {
		bvec, err = word2vec.Avg(vectors).AsBytes()
		if err == nil {
			_, err = psr.updateVecEntry.Exec(bvec, id)
			if err != nil {
				return err
			}
		}
	}
	return err
}

// Parse entries note
func Parse(t state.State, ids []string, lastedits []int64) error {
	db := t.DB()
	update := make([]bool, len(ids))
	tx, err := db.Begin()
	if err != nil {
		return err
	}
	psr := newParser(t)
	psr.parseStatements = &parseStatements{}
	err = initStatement(psr.parseStatements, tx)
	if err != nil {
		return err
	}
	var b strings.Builder
	for i, id := range ids {
		b.Reset()
		b.WriteString(id)
		b.WriteString(config.Ext)
		fp := b.String()
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
				err := parseFile(*psr, id, fp)
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
	return tx.Commit()
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
