// Package word2vec - Word2vec reading and basic operations.
package word2vec

import (
	"bufio"
	"database/sql"
	"encoding/binary"
	"errors"
	"fmt"
	"math"
	"os"
	"strings"

	sqlite_vec "github.com/asg017/sqlite-vec-go-bindings/cgo"

	"retrolire/internal/config"
	"retrolire/internal/fs"
	"retrolire/internal/nlp/tokenizer"
	"retrolire/internal/state"
)

// Vectorizer transform texts in vectors
type Vectorizer struct {
	cache map[string]*Vector
	db    *sql.DB
	*tokenizer.Lector
}

// NewVectorizer create a new Vectorizer
func NewVectorizer(db *sql.DB) *Vectorizer {
	return &Vectorizer{
		db:     db,
		cache:  map[string]*Vector{},
		Lector: tokenizer.NewLector(db),
	}
}

// Vectorize - Transform a text into a Vector
func (vr *Vectorizer) Vectorize(text string) ([]string, *Vector) {
	var ok bool
	var vec *Vector
	var y int
	tokens := vr.Process(text)
	doc := make([]*Vector, len(tokens))
	for _, token := range tokens {
		vec, ok = vr.cache[token]
		if !ok {
			vec = WordVectorFromDB(vr.db, token)
			vr.cache[token] = vec
		}
		if vec != nil {
			doc[y] = vec
			y++
		}
	}
	if y == 0 {
		return []string{}, nil
	}
	return tokens, Avg(doc[:y])
}

// AsBytes - Transform Vector into bytes than can be insert into the database
func (v *Vector) AsBytes() ([]byte, error) {
	return sqlite_vec.SerializeFloat32(*v)
}

// MaxVectorDim - Maximum vector dimensions
const MaxVectorDim = 500

// https://stackoverflow.com/a/22492518
func f32FromBytes(bytes []byte) float32 {
	bits := binary.LittleEndian.Uint32(bytes)
	float := math.Float32frombits(bits)
	return float
}

// VectorFromBytes - Build a vector from bytes
func VectorFromBytes(bytes []byte) *Vector {
	dim := len(bytes) / 4
	vec := make(Vector, dim)
	for i := range dim {
		vec[i] = f32FromBytes(bytes[4*i:])
	}
	return &vec
}

// WordVectorFromDB returns a vector from the DB or nil if not in the DB
func WordVectorFromDB(db *sql.DB, word string) *Vector {
	var b []byte
	err := db.QueryRow(`SELECT vec_f32(vec)
	FROM vec_word
	WHERE word = ?`, word).Scan(&b)
	if err != nil {
		return nil
	}
	return VectorFromBytes(b)
}

// MostSimilarFromDB get most similar words from word vectors stored in the database
func MostSimilarFromDB(db *sql.DB, word string, n int) ([]string, error) {
	var similar []string
	rows, err := db.Query(`SELECT word
	FROM vec_word
	WHERE vec
	MATCH (SELECT vec FROM vec_word WHERE word = ?)
	AND k = ?`, word, n)
	defer rows.Close()
	if err != nil {
		return nil, err
	}
	err = rows.Err()
	if err != nil {
		return nil, err
	}
	var i int
	for rows.Next() {
		var s string
		err = rows.Scan(&s)
		if s != "" && s != word {
			similar = append(similar, s)
		}
		i++
		if err != nil {
			return nil, err
		}
	}
	// Remove the first one: it's also the word itself.
	// And resize the array so it matches the real number of similar words returns.
	// (Even if there is very little chance that we require more words than vectors number.)
	// return similar[1:i], nil
	return similar, nil
}

// TODO: Named errors

func insertVectors(db *sql.DB, vectors map[string]Vector, dim int) error {
	tx, err := db.Begin()
	if err != nil {
		return err
	}
	// Drop the previous database and create a new one. This ensure that:
	// 1. There are no duplicates words.
	// 2. There is no mismatches between vector lengths.
	for _, s := range []string{
		`DROP TABLE IF EXISTS vec_word`,
		// Unfortunately, I can't use a parameter here.
		// But SQL injection isn't possible since the variable is an `int`.
		fmt.Sprintf(`CREATE VIRTUAL TABLE vec_word
		USING vec0(
		word text PRIMARY KEY NOT NULL,
		vec float[%d]
	)`, dim),
	} {
		_, err = tx.Exec(s)
		if err != nil {
			return err
		}
	}
	stmt, err := tx.Prepare(`INSERT INTO vec_word (word, vec) VALUES (?, ?)`)
	if err != nil {
		return err
	}
	for k, v := range vectors {
		b, err := sqlite_vec.SerializeFloat32(v)
		if err != nil {
			return err
		}
		_, err = stmt.Exec(k, b)
		if err != nil {
			return err
		}
	}
	return tx.Commit()
}

// InitVectors - Initialize the vectors table in the database if it doesn't exists.
func InitVectors(t state.State, force bool) error {
	var err error
	db := t.DB()
	// Get no value from the table, just to check if there's any error
	if db.QueryRow(`SELECT vec FROM vec_word LIMIT 0`).Err() == nil && !force {
		return db.Close()
	}
	const fp = config.WordVectorBinaryFile
	if fp == "" {
		return errors.New("(config) WordVectorBinaryFile is not set")
	}
	if !fs.FileExists(fp) {
		return errors.New("(config) WordVectorBinaryFile doesn't exists")
	}
	file, err := os.Open(fp)
	if err != nil {
		return err
	}
	rd := bufio.NewReader(file)
	// Get the word2vec model and its dimensions
	model, err := fromReader(rd)
	if err != nil {
		return err
	}
	// Set an arbitrary max dimensions to avodi creating a Huge table.
	if model.Dim > MaxVectorDim {
		return errors.New("too many vectors dimensions")
	}
	err = insertVectors(db, model.Words, model.Dim)
	if err != nil {
		return err
	}
	t.Log(errors.New("word vectors database successfully created"))
	return nil
}

// VectorizeEntriesTitle - Vectories entries with NULL value as vec
func VectorizeEntriesTitle(db *sql.DB, onlyNull bool) error {
	var err error
	tx, err := db.Begin()
	if err != nil {
		return err
	}
	stmt, err := tx.Prepare(`UPDATE entry SET vec = ? WHERE id = ?`)
	if err != nil {
		return err
	}
	stmtDeleteTfs, err := tx.Prepare(`DELETE FROM fts WHERE id = ? AND line = -1`)
	if err != nil {
		return err
	}
	stmtInsertTfs, err := tx.Prepare(`INSERT INTO fts (id, line, main) VALUES (?, -1, ?)`)
	if err != nil {
		return err
	}
	selectStmt := `SELECT id, title FROM entry`
	if onlyNull {
		selectStmt += ` WHERE vec IS NULL`
	}
	rows, err := tx.Query(selectStmt)
	if err != nil {
		return err
	}
	err = rows.Err()
	if err != nil {
		return err
	}
	vectorizer := NewVectorizer(db)
	for rows.Next() {
		var id, title string
		err := rows.Scan(&id, &title)
		if err != nil {
			return err
		}
		tokens, vector := vectorizer.Vectorize(title)
		_, err = stmtDeleteTfs.Exec(id)
		if err != nil {
			return err
		}
		_, err = stmtInsertTfs.Exec(id, strings.Join(tokens, " "))
		if err != nil {
			return err
		}
		if vector != nil {
			b, err := vector.AsBytes()
			if err != nil {
				return err
			}
			_, err = stmt.Exec(b, id)
			if err != nil {
				return err
			}
		}
	}
	return tx.Commit()
}
