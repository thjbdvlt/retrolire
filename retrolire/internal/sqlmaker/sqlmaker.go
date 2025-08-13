// Package sqlmaker - Dynamically build SELECT statements from user-input filters
package sqlmaker

import (
	"slices"
	"strings"

	"retrolire/internal/aka"
)

// SelectStmt - SELECT statement base
type SelectStmt struct {
	stmt            string
	params          []any
	clauses         []string
	orderBy         string
	groupBy         string
	nRequiredParams int // Number of parameters needed to Build SQL
}

func (s SelectStmt) NRequiredParams() int { return s.nRequiredParams }

type filter func(string, *aka.Thesaurus) (string, []any, bool)

// Thesauruser - Whatever can return a Thesaurus
type Thesauruser interface{ Thesaurus() *aka.Thesaurus }

// BuildSelect - Build a SELECT statement
func (sl SelectStmt) BuildSelect(args []any, filtersDesc []string, t Thesauruser) (string, []any) {
	var thesaurus *aka.Thesaurus
	if t == nil {
		thesaurus = aka.NewThesaurus()
	} else {
		thesaurus = t.Thesaurus()
	}
	filtersFuncs := []filter{
		filterVar,
		filterAuthor,
		filterTag,
		filterClass,
		filterSearch,
	}
	stmt := []string{sl.stmt}
	// params := append(slices.Clone(sl.params), args...)
	params := slices.Clone(sl.params)
	clauses := slices.Clone(sl.clauses)
	if len(filtersDesc) > 0 {
		filterClauses, filterParams := Parse(filtersDesc, filtersFuncs, thesaurus)
		clauses = append(clauses, "AND", "(")
		clauses = append(clauses, filterClauses...)
		clauses = append(clauses, ")")
		params = append(params, filterParams...)
	}
	params = append(params, args...)
	if len(clauses) > 0 {
		clauses[0] = "WHERE"
		stmt = append(stmt, clauses...)
	}
	stmt = append(stmt, sl.groupBy, sl.orderBy)
	return strings.Join(stmt, " "), params
}

// Parse - Build filters from arguments
func Parse(args []string, filterers []filter, t *aka.Thesaurus) ([]string, []any) {
	var clauses []string
	var params []any
	var operators = [2]string{"AND", ""}
	for _, i := range args {
		i = strings.TrimSpace(i)
		// Logical operators
		switch i {
		case "or":
			operators[0] = "OR"
			continue
		case "and":
			operators[0] = "AND"
			continue
		case "not":
			operators[1] = "NOT"
			continue
		}
		// Iterate over filters until one returns a non-empty clause
		for _, f := range filterers {
			fClause, fParams, ok := f(i, t)
			if ok {
				clauses = append(clauses, operators[0], operators[1], fClause)
				params = append(params, fParams...)
				break
			}
		}
		// After a filter, reinit logical operators
		operators[0], operators[1] = "AND", ""
	}
	if len(clauses) > 0 {
		clauses[0] = ""
	}
	return clauses, params
}

// NullStmt - Returns an empty statement
func NullStmt() *SelectStmt {
	return &SelectStmt{}
}

// EntryStmt - SELECT statement for entries
func EntryStmt() *SelectStmt {
	return &SelectStmt{
		stmt:    `SELECT e.head FROM entry AS e`,
		orderBy: `ORDER BY lastpick DESC`,
	}
}

// OpenEntryStmt - Select entries to open URL or file
func OpenEntryStmt() *SelectStmt {
	st := EntryStmt()
	st.clauses = []string{`WHERE`, `csl ->> 'URL' IS NOT NULL`}
	return st
}

// TextobjStmt - SELECT statement for textobjs
func TextobjStmt() *SelectStmt {
	return &SelectStmt{
		stmt:            `SELECT o.head FROM entry AS e JOIN textobj AS o ON o.entry = e.id`,
		nRequiredParams: 1,
		clauses:         []string{`WHERE`, `o.class = ?`},
	}
}

// ListStmt - Statement for list pretty printing
func ListStmt() *SelectStmt {
	return &SelectStmt{
		params:  []any{"\033[35m", "\033[0m"},
		groupBy: `GROUP BY e.id`,
		stmt: `SELECT group_concat(
	? || c.key || ? || ': ' || c.value,
	char(10)
) || char(10) || '---' AS record
FROM entry AS e, json_each(json(e.csl)) AS c`,
	}
}

// CmpTagStmt - Tag completion statement
func CmpTagStmt() *SelectStmt {
	return &SelectStmt{
		stmt: `SELECT DISTINCT '.' || tag FROM tag`,
	}
}

// CmpFieldStmt - Field completion statement
func CmpFieldStmt() *SelectStmt {
	return &SelectStmt{
		stmt: `SELECT DISTINCT x.key FROM entry, json_each(csl) as x`,
	}
}

// CmpAuthorStmt - Author completion statement
func CmpAuthorStmt() *SelectStmt {
	return &SelectStmt{
		stmt: `SELECT DISTINCT '@' || lower(author) FROM entry`,
	}
}

// JSONStmt - List entries as JSON array
func JSONStmt() (s *SelectStmt) {
	return &SelectStmt{
		stmt: `SELECT json_group_array(json(csl)) FROM entry e`,
	}
}

// ParseStmt - Statement to parse notes
func ParseStmt() *SelectStmt {
	return &SelectStmt{
		stmt: `SELECT e.id, e.lastedit FROM entry AS e`,
	}
}

// TuiStmt - Statement to select many things
func TuiStmt() *SelectStmt {
	return &SelectStmt{stmt: `SELECT e.id, e.line, e.main, e.least, e.class FROM obj AS e`}
}

// TuiStmtOrderBy - Select many things, order by class
func TuiStmtOrderBy() *SelectStmt {
	st := TuiStmt()
	st.orderBy = `ORDER BY class, length(main)`
	return st
}

// TuiStmtFTS - Select statement for TFS5
func TuiStmtFTS() *SelectStmt {
	return &SelectStmt{
		// TODO: Don't match again class / line / id
		stmt: `
SELECT e.id, e.line, e.main, e.least, e.class FROM obj e
JOIN fts f ON f.id = e.id AND e.line = f.line
		`,
		nRequiredParams: 1,
		clauses:         []string{"WHERE", "fts MATCH ?"},
		orderBy:         "ORDER BY RANK",
	}
}

// TuiStmtCosine - Word vectors ranking statement
func TuiStmtCosine() *SelectStmt {
	return &SelectStmt{
		stmt:            `SELECT e.id, e.line, e.main, e.least as least, class from obj e`,
		clauses:         []string{"WHERE", "vec IS NOT NULL"},
		orderBy:         `ORDER BY vec_distance_cosine(e.vec, ?)`,
		nRequiredParams: 1,
	}
}

// TuiStmtL2 - Word vectors ranking statement
func TuiStmtL2() *SelectStmt {
	return &SelectStmt{
		// FIXME: Required param is at the end
		stmt:            `SELECT e.id, e.line, e.main, e.least as least, class from obj e`,
		clauses:         []string{"WHERE", "vec IS NOT NULL"},
		orderBy:         `ORDER BY vec_distance_L2(e.vec, ?)`,
		nRequiredParams: 1,
	}
}
