// Package tui - polyanthea Terminal User Interface
package tui

import (
	"polyanthea/internal/nlp/fts"
	"polyanthea/internal/nlp/word2vec"
	"polyanthea/internal/sqlmaker"
)

// The history is a Work In Progress, i.e. it doesn't work very well.

func (ui *UI) findSimilarFromText(text string) {
	db := ui.DB()
	vectorizer := word2vec.NewVectorizer(db)
	_, vector := vectorizer.Vectorize(text)
	if vector == nil {
		// This should rarely happens if words vectors are trained on polyanthea data
		return
	}
	b, err := vector.AsBytes()
	if err != nil {
		ui.Log(err)
		return
	}
	stmt, params := sqlmaker.TuiStmtCosine().BuildSelect([]any{b}, ui.filters(), ui)
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

func (ui *UI) findSimilar(t *thing) {
	if t == nil {
		return
	}
	if t.bvec == nil {
		ui.findSimilarFromText(t.main)
		return
	}
	stmt, params := sqlmaker.TuiStmtCosine().BuildSelect([]any{t.bvec}, ui.filters(), ui)
	rows, err := ui.DB().Query(stmt, params...)
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
	rows, err := fts.TextToQuery(ui, text)
	if err != nil {
		ui.Log(err)
		return
	}
	ui.display(fromRows(rows))
}
