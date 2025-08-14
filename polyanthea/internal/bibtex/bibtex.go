// Package bibtex - BibTeX templates
package bibtex

import (
	"fmt"
	"os"
	"strings"
)

const book = `@book{_,
	address = {},
	author = {},
	isbn = {},
	language = {},
	publisher = {},
	series = {},
	title = {},
	translator = {},
	year = {},
	origyear = {}
}`
const chapter = `@inbook{_,
  address = {},
  author = {},
  editor = {},
  booktitle = {},
  language = {},
  pages = {},
  publisher = {},
  title = {},
  year = {},
  origyear = {}
}`
const article = `@article{_,
  author = {},
  title = {},
  year = {},
  origyear = {},
  journal = {},
  pages = {},
  number = {},
  volume = {},
  url = {},
  urldate = {},
  publisher = {}
}`
const web = `@misc{_,
  author = {},
  title = {},
  publisher = {},
  url = {},
  urldate = {},
  year = {},
  origyear = {}
}`
const inproceeding = `@inproceedings{_,
  address = {},
  author = {},
  booktitle = {},
  language = {},
  pages = {},
  publisher = {},
  title = {},
  year = {},
  origyear = {}
}`
const software = `@software{_,
  author = {},
  title = {},
  url = {},
  version = {},
  date = {}
}`

// GetTemplate - Get BibTex template by name
func GetTemplate(templateName string) (tpl []byte, ok bool) {
	type template struct {
		name   string
		bibtex []byte
	}
	var templates = []template{
		{"article", []byte(article)},
		{"book", []byte(book)},
		{"web", []byte(web)},
		{"chapter", []byte(chapter)},
		{"software", []byte(software)},
		{"inproceeding", []byte(inproceeding)},
	}
	if templateName != "" {
		for _, i := range templates {
			if strings.HasPrefix(i.name, templateName) {
				return i.bibtex, true
			}
		}
	}
	fmt.Fprintln(os.Stderr, "Unknown template name:", templateName)
	fmt.Fprintln(os.Stderr, "Available templates are:")
	for _, i := range templates {
		fmt.Fprintln(os.Stderr, i.name)
	}
	return []byte{}, false
}
