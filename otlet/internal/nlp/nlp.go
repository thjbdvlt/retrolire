// Package nlp - NLP functions, e.g. tokenization and word vectors
package nlp

import (
	"bufio"
	"strings"

	"otlet/internal/files"
	"otlet/internal/state"
)

// UpdateStopWords stop word file and update database
func UpdateStopWords(t state.State) error {
	root := t.Root()
	if !files.FileExists(files.StopWordFile) {
		return nil // stopword file is optional
	}
	file, err := root.Open(files.StopWordFile)
	if err != nil {
		return err
	}
	err = root.Close()
	if err != nil {
		return err
	}
	stopwords := []string{}
	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		stopwords = append(stopwords, strings.TrimSpace(scanner.Text()))
	}
	tx, err := t.DB().Begin()
	if err != nil {
		return err
	}
	_, err = tx.Exec(`DROP TABLE IF EXISTS stopword`)
	if err != nil {
		return err
	}
	_, err = tx.Exec(`CREATE TABLE stopword (word text)`)
	if err != nil {
		return err
	}
	stmt, err := tx.Prepare(`INSERT INTO stopword (word) VALUES (?)`)
	if err != nil {
		return err
	}
	for _, i := range stopwords {
		if i != "" {
			_, err = stmt.Exec(i)
			if err != nil {
				return err
			}
		}
	}
	if err != nil {
		return err
	}
	return tx.Commit()
}
