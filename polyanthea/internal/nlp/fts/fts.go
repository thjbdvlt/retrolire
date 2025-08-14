// Package fts - use SQLite3 TFS5 information retrieval module
package fts

import (
	"database/sql"
	"strings"

	"polyanthea/internal/config"
	"polyanthea/internal/nlp/tokenizer"
	"polyanthea/internal/nlp/word2vec"
	"polyanthea/internal/sqlmaker"
	"polyanthea/internal/state"
)

// TextToQuery - Make a FTS5 query from string
func TextToQuery(t state.State, s string) (*sql.Rows, error) {
	db := t.DB()
	l := tokenizer.NewLector(db)
	tokens := l.Process(s)

	var similar []string
	// Get the similar words before building bigrams, but only append after.
	// So similar words are not included in bigrams
	const nSimilar = config.SimilarWordsInFlorilege
	if nSimilar > 0 {
		for _, token := range tokens {
			// Add +1 to nSimilar because the word itself is returned as a similar word.
			// This is actually a good thing as it gives it more weight than others.
			sim, err := word2vec.MostSimilarFromDB(db, token, nSimilar+1)
			if err == nil {
				similar = append(similar, sim...)
			}
		}
	}

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

	tokens = append(tokens, similar...) // Append similar words after bigrams

	query := strings.Join(tokens, " OR ")
	stmt, params := sqlmaker.TuiStmtFTS().BuildSelect([]any{query}, nil, t)
	// TODO: Properly escape input tokens.
	// Because unlike most other parameters, this one can lead to a syntax error.
	return db.Query(stmt, params...)
}
