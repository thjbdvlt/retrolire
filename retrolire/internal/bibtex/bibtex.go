// Package bibtex - BibTeX templates
package bibtex

import (
	"fmt"
	"os"
	"strings"
)

var book = []byte(`@book{_,
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
}`)
var chapter = []byte(`@inbook{_,
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
}`)
var article = []byte(`@article{_,
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
}`)
var web = []byte(`@misc{_,
  author = {},
  title = {},
  publisher = {},
  url = {},
  urldate = {},
  year = {},
  origyear = {}
}`)
var inproceeding = []byte(`@inproceedings{_,
  address = {},
  author = {},
  booktitle = {},
  language = {},
  pages = {},
  publisher = {},
  title = {},
  year = {},
  origyear = {}
}`)
var software = []byte(`@software{_,
  author = {},
  title = {},
  url = {},
  version = {},
  date = {}
}`)

type template struct {
	name   string
	bibtex []byte
}

var templates = []template{
	{"article", article},
	{"book", book},
	{"web", web},
	{"chapter", chapter},
	{"software", software},
	{"inproceeding", inproceeding},
}

// GetTemplate - Get BibTex template by name
func GetTemplate(templateName string) (tpl []byte, ok bool) {
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
