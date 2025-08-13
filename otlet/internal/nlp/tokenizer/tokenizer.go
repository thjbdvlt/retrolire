// Package tokenizer - Transform a text into a set of tokens
// This package also filter stopwords and apply basic stemming and normalization
package tokenizer

import (
	"database/sql"
	"regexp"
	"strings"

	"github.com/kljensen/snowball"

	"otlet/internal/config"
	"otlet/internal/util"
)

// Lector transform a text into a list of normalized tokens
type Lector struct {
	tokenize tokenizer
	stem     stemmer
	stop     stopper
}

// NewLector initialize a Lector
func NewLector(db *sql.DB) *Lector {
	return &Lector{
		tokenize: newTokenizer(),
		stem:     newStemmer(),
		stop:     newStopper(db),
	}
}

// func bigrams(tokens []string) []string {
// 	var x = make([]string, len(tokens))
// 	for i := range len(tokens) - 1 {
// 		x[i] = tokens[i] + tokens[i+1]
// 	}
// 	return x
// }
//
// func hardStems(tokens []string) []string {
// 	var x = make([]string, len(tokens))
// 	for i, token := range tokens {
// 		if len(token) > 6 {
// 			token = token[:6]
// 		}
// 		x[i] = token
// 	}
// 	return x
// }

// Process transform a text into a normalized list of tokens
func (l Lector) Process(text string) []string {
	tokens := l.tokenize(strings.ToLower(text))
	nonstop := make([]string, len(tokens))
	var y int
	for _, token := range tokens {
		if !l.stop(token) && token != "" {
			nonstop[y] = l.stem(token)
			y++
		}
	}
	nonstop = nonstop[:y]
	return nonstop
}

type tokenizer func(string) []string
type stopper func(string) bool
type stemmer func(string) string

func newTokenizer() tokenizer {
	re := regexp.MustCompile(`\pL+`)
	return func(s string) []string {
		return re.FindAllString(s, -1)
	}
}

func newStemmer() stemmer {
	const lang = config.LanguageStemming
	// Stem - Stem a word and lowercase it
	return func(word string) string {
		word, _ = snowball.Stem(word, lang, true)
		return word
	}
}

// newStopper - Returns a function that identifies stopwords
func newStopper(db *sql.DB) stopper {
	stopwords, err := util.GetSomethingAsSlice(db, `SELECT word FROM stopword`)
	if err != nil {
		return func(string) bool { return false }
	}
	lookup := make(map[string]bool, len(stopwords))
	for _, i := range stopwords {
		lookup[i] = true
	}
	return func(s string) bool {
		_, ok := lookup[s]
		return ok
	}
}
