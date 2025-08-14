// Package mode - Defines polyanthea TUI modes
package mode

// Mode - A TUI mode
type Mode int

// Defines modes
const (
	List = iota
	Collect
	SearchOnKey
	Filter
	JumpLabel
	Delete
	Update
	Vectors
)

// labels - Mode labels to be printed on the input line
var labels = map[Mode]string{
	List:        "",
	SearchOnKey: "SEARCH",
	Filter:      "FILTER",
	Collect:     "COLLECT",
	JumpLabel:   "LABEL",
	Vectors:     "VECTORS",
	Delete:      "DELETE",
	Update:      "UPDATE",
}

// Label - Get label from mode ID
func Label(m Mode) string {
	name, ok := labels[m]
	if !ok {
		return ""
	}
	return name
}
