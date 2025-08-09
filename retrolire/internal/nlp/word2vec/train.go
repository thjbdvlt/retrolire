// Package word2vec - Word2vec reading and basic operations.
package word2vec

// Training/Saving is done with my fork of https://github.com/ynqa/wego
// I'm using example:
// https://github.com/ynqa/wego/blob/master/examples/word2vec/main.go

import (
	"os"
	"strings"

	wv "github.com/thjbdvlt/wego/pkg/model/word2vec"

	"retrolire/internal/config"
	"retrolire/internal/nlp/tokenizer"
	"retrolire/internal/state"
)

const dim = config.WordVectorDimensions

func getText(t *state.State) ([]string, error) {
	db := t.Conn()
	defer db.Close()
	// rows, err := db.Query(`SELECT title
	// FROM entry
	// UNION ALL
	// SELECT text
	// FROM textobj`)
	rows, err := db.Query(`SELECT title
	FROM entry
	UNION ALL
	SELECT group_concat(text, ' ' order by linenr)
	FROM textobj
	GROUP BY entry`)
	if err != nil {
		panic(err)
	}
	err = rows.Err()
	if err != nil {
		panic(err)
	}
	var texts []string
	for rows.Next() {
		var s string
		err = rows.Scan(&s)
		if err != nil {
			panic(err)
		}
		texts = append(texts, s)
	}
	return texts, nil
}

func preprocess(t *state.State, texts []string) []string {
	db := t.Conn()
	defer db.Close()
	lector := tokenizer.NewLector(db)
	for i, s := range texts {
		texts[i] = strings.Join(lector.Process(s), " ")
	}
	return texts
}

// Train - Train word vectors from the notes texts and put the result in DB
func Train(t *state.State) error {
	var err error
	texts, err := getText(t)
	texts = preprocess(t, texts)
	if err != nil {
		return err
	}
	file, err := os.CreateTemp("", "retrolire.training")
	if err != nil {
		return err
	}
	_, err = file.WriteString(strings.Join(texts, "\n"))
	if err != nil {
		return err
	}
	err = file.Sync()
	if err != nil {
		return err
	}
	// TODO: Configurable options
	model, err := wv.New(
		// Options are set for small corpora
		wv.Window(10),
		wv.Iter(3),
		wv.MaxCount(-1),
		wv.MinCount(1),
		wv.Dim(dim),
		// wv.Model(wv.SkipGram),
		wv.Model(wv.Cbow),
		wv.Optimizer(wv.NegativeSampling),
		wv.NegativeSampleSize(5),
	)
	if err != nil {
		return err
	}
	if err = model.Train(file); err != nil {
		return err
	}
	if err = file.Close(); err != nil {
		return err
	}
	_ = os.Remove(file.Name())
	mp := model.AsMap()
	db := t.Conn()
	defer db.Close()
	// Convert to float32
	vectors := make(map[string]Vector, len((*mp)))
	for k, v := range *mp {
		arr := make([]float32, len(v))
		for i := range len(v) {
			arr[i] = float32(v[i])
		}
		vectors[k] = arr
	}
	return insertVectors(db, vectors, dim)
}
