package state

import (
	"strings"
)

// ID - Identifies an specific location in the bibliography
type ID struct {
	Entry string
	Line  string
	Page  string
}

// IDsep - ID field separator for representation as string
const IDsep = ","

// IDFromString - Make an ID from a string like "perec1985,84,41"
func IDFromString(s string) ID {
	x := [3]string{"", "", ""}
	for i, v := range strings.Split(s, IDsep) {
		x[i] = v
	}
	return ID{Entry: x[0], Line: x[1], Page: x[2]}
}

// IDToString - Convert an Id to a string
func IDToString(id ID) string {
	return strings.Join([]string{id.Entry, id.Line, id.Page}, IDsep)
}
