// Package fts - use SQLite3 TFS5 information retrieval module
package fts

import (
	"database/sql"
	"strings"

	"retrolire/internal/nlp/tokenizer"
	"retrolire/internal/sqlmaker"
)

// TextToQuery - Make a FTS5 query from string
func TextToQuery(db *sql.DB, s string) (*sql.Rows, error) {
	l := tokenizer.NewLector(db)
	tokens := l.Process(s)
	var b strings.Builder
	for i := range len(tokens) - 1 {
		b.Reset()
		// Add bigrams
		b.WriteString("(")
		b.WriteString(tokens[i])
		b.WriteString(" + ")
		b.WriteString(tokens[i+1])
		b.WriteString(")")
		tokens = append(tokens, b.String())
	}
	query := strings.Join(tokens, " OR ")
	stmt, params := sqlmaker.TuiStmtFTS().BuildSelect([]any{query}, nil)
	return db.Query(stmt, params...)
}
