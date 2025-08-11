// Package tui - Retrolire Terminal User Interface
package tui

import (
	"retrolire/internal/nlp/fts"
	"retrolire/internal/nlp/word2vec"
	"retrolire/internal/sqlmaker"
)

// The history is a Work In Progress, i.e. it doesn't work very well.

func (ui *UI) findSimilar(text string) {
	db := ui.Conn()
	defer db.Close()
	vectorizer := word2vec.NewVectorizer(db)
	_, vector := vectorizer.Vectorize(text)
	if vector == nil {
		// This should rarely happens if words vectors are trained on retrolire data
		return
	}
	b, err := vector.AsBytes()
	if err != nil {
		ui.Log(err)
		return
	}
	stmt, params := sqlmaker.TuiStmtCosine().BuildSelect([]any{b}, []string{})
	rows, err := db.Query(stmt, params...)
	if err != nil {
		ui.Log(err)
		return
	}
	if rows.Err() != nil {
		ui.Log(rows.Err())
		return
	}
	ui.display(fromRows(rows))
}

func (ui *UI) findFts(text string) {
	db := ui.Conn()
	defer db.Close()
	rows, err := fts.TextToQuery(db, text)
	if err != nil {
		ui.Log(err)
		return
	}
	ui.display(fromRows(rows))
}
