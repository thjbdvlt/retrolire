// Package aka - Also Known AS (i.e. aliases) type and functions
package aka

import (
	"os"

	"retrolire/internal/files"
)

// Thesaurus store aliases
type Thesaurus struct {
	Tags         *AKA
	Names        *AKA
	CSLVariables *AKA
}

// NewThesaurus - Create a new Thesaurus
func NewThesaurus() *Thesaurus {
	return &Thesaurus{
		Tags:         &AKA{},
		Names:        &AKA{},
		CSLVariables: &AKA{},
	}
}

// InitThesaurus - Initialize (or reinitialize, i.e. update) a Thesaurus
func InitThesaurus(th *Thesaurus, root *os.Root) error {
	var err error
	var file *os.File
	file, err = root.Open(files.TagFile)
	if err != nil {
		return err
	}
	th.Tags = parseTree(file)
	return file.Close()
}

func getIndent(s string) int {
	for i, v := range s {
		if v != ' ' {
			return i
		}
	}
	return 0
}
