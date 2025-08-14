// Package elements - Defines TUI elements IDs
package elements

// UID - A TUI element numerical identifier
type UID int

// TUI elements numerical identifiers
const (
	None UID = iota
	NumberOfItems
	ActiveFilters
	Background
	ModeSearch
	ModeFilter
	Preview
	ModeLabel
	Labels
	DeleteEntry
	ListFieldsSeparator
	ModeListNavigation
	N // Last one is N, used to know the number of UI elements defined
)
