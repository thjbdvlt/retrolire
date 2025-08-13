// Package files - otlet files and directories paths
package files

import (
	"os"
	"regexp"

	"golang.org/x/text/runes"
	"golang.org/x/text/transform"
	"golang.org/x/text/unicode/norm"
	"unicode"

	"otlet/internal/config"
)

// DBNAME - otlet database name and CONNINFO
const DBNAME = ".otlet.db?mode=rwc&cache=shared"

// TagFile - Filename of file describing tags hierarchy
const TagFile = ".otlet.tags"

// PeopleDirectoryName - Directory name for notes about people
const PeopleDirectoryName = "people"

// LogFile - Log file for TUI
const LogFile = ".otlet.log"

// StopWordFile - File containing stopwords
const StopWordFile = ".otlet.stopswords"

// FileExists - Check if a file exists
func FileExists(fp string) bool {
	_, err := os.Stat(fp)
	if err == nil {
		return true
	}
	if os.IsNotExist(err) {
		return false
	}
	return true
}

// CD - Change to otlet directory
func CD() {
	os.Chdir(config.Directory)
}

var reFile = regexp.MustCompile("[^a-zA-Z0-9_-]")

// https://stackoverflow.com/questions/26722450/remove-diacritics-using-go
func removeDiacritics(text string) string {
	t := transform.Chain(norm.NFD, runes.Remove(runes.In(unicode.Mn)))
	text, _, err := transform.String(t, text)
	if err != nil {
		panic(err)
	}
	return text
}

// ToFilename - Make a filename from a string. (Not a very reliable solution, though.)
func ToFilename(text string) string {
	return reFile.ReplaceAllString(removeDiacritics(text), "_")
}

// CreateDirectory - Create directory if it doesn't exists
func CreateDirectory(root *os.Root, directory string) error {
	if FileExists(directory) {
		return nil
	}
	return root.Mkdir(directory, 0700)
}
