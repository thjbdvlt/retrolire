// Package obj - Define objects
package obj

import (
	"retrolire/internal/config"
)

// Class - The object type that a command operates on
type Class int

// Object types
const (
	None Class = iota
	Entry
	Tag
	Person
	Heading
	Concept
	Quote
	Idea
	Example
	Florilege             // Not implemented yet
	Compilation           // Not implemented yet
	CommentydBibliography // Not implemented yet
	Summary               // Not implemented yet
	Relation              // Not implemented yet
	Variable
	// Many things are actually object classes, like TUI elements
	ClassName
	Command
	UIElement
	N // N - Used to know how many classes are defined
)

// Names - Object class names
func Names() []string {
	names := make([]string, N)
	names[Entry] = "entry"
	names[Concept] = "concept"
	names[Quote] = "quote"
	names[Idea] = "idea"
	names[Example] = "example"
	names[Person] = "person"
	names[Heading] = "heading"
	names[Tag] = "tag"
	return names
}

// FromName - Get Obj int constant from name
func FromName(s string) Class {
	aliasedName, ok := config.AliasesClasses[s]
	if ok {
		s = aliasedName
	}
	for i, name := range Names() {
		if name == s {
			return Class(i)
		}
	}
	return None
}
