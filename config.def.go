// Configuration file for otlet.
//
// All constant and variable can be changed, but NONE must be entirely removed.
// You won't be able to compile the software if any is missing, but you'll be able
// to manually restores the missing values using "config.def.go".
//
package config

// The most necessary configuration value is Directory, that defines the directory
// containing the database and notes files.
// It this value isn't set, it will lead to an error "Couldn't open directory".
const Directory = ""

const DefaultCmd = "tui"  // Command called when none is explicitly used
const Opener = "xdg-open" // Program to open files/URLs with
const (
	Editor           = "nvim" // Program to edit notes with
	EditorFlagLineNr = "-c"   // Flag before the line number, i.e. "nvim FILE -c 4".
	Ext              = ".md"  // Extension for notes
)

// Key bindings.
// All values must be runes, i.e. single character in single quotes.
// Each keys in a group (i.e. KeyList / KeyInput) must be different.
const (
	// List navigation and items operations. These are main keybindings.
	KeyListUp                = 'k'
	KeyListDown              = 'j'
	KeyListPageUp            = 'K'
	KeyListPageDown          = 'J'
	KeyListSearchOnKeyStroke = 'i'
	KeyListFilterAdd         = 'a'
	KeyListFiltersNew        = 'A'
	KeyListFilterRemoveLast  = 'h'
	KeyListPreviewScrollUp   = '['
	KeyListPreviewScrollDown = ']'
	KeyListClassPick         = '='
	KeyListEditTags          = 'T'
	KeyListAddVar            = 'v'
	KeyListTag               = 't'
	KeyListResetSearch       = 'r'
	KeyListJumpLabel         = 's'
	KeyListOpen              = 'o'
	KeyListDelete            = 'D'
	KeyListTop               = 'g'
	KeyListBottom            = 'G'
	KeyListQuit              = 'q'
	KeyListHelp              = '?' // Not implemented yet
	KeyListUpdate            = 'U' // Not implemented yet
	// Input key bindings
	KeyInputBackwardKillWord = '<'
	KeyInputKillLine         = ';'
	KeyInputBackwardWord     = '('
	KeyInputForwardWord      = ')'
)

// Colors can be set with their W3M name or as CSS description, e.g. "#ffa402"
const (
	// List items
	ColorEntryID       = "yellow"
	ColorEntry         = "black"
	ColorConceptID     = "hotpink"
	ColorConcept       = "#0044aa"
	ColorQuoteID       = "hotpink"
	ColorQuote         = "#008800"
	ColorExample       = "#888800"
	ColorExampleID     = "hotpink"
	ColorIdea          = "grey"
	ColorIdeaID        = "hotpink"
	ColorHeading       = "#880088"
	ColorHeadingID     = "hotpink"
	ColorSecondaryText = "grey"
	ColorSelectedBg    = "white"
	ColorSelectedFg    = "red"
	// Interface
	ColorBackground         = "white"
	ColorActiveFilterFg     = "grey"
	ColorActiveFilterBg     = "white"
	ColorNumberOfItemsFg    = "#cccc88"
	ColorNumberOfItemsBg    = "white"
	ColorPreviewFg          = "#888844"
	ColorPreviewBg          = "#eeeeee"
	ColorLabelFg            = "#0000ff"
	ColorLabelBg            = "white"
	ColorModeListNavigation = "white"
	ColorModeFilterFg       = "#cc00cc"
	ColorModeFilterBg       = "white"
	ColorModeSearchFg       = "#00ff00"
	ColorModeSearchBg       = "white"
	ColorModeLabelFg        = "#0000ff"
	ColorModeLabelBg        = "white"
	ColorDeleteFg           = "white"
	ColorDeleteBg           = "red"
)

const SelectedSign = ">"
const PageStep = 10
const JumpLabels = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789"

// Command aliases
var AliasesCommand = map[string]string{
	"ed": "edit",
	"ci": "cite",
	"up": "update",
	"ad": "add",
	"a":  "add",
}

// CSL variable Aliases, e.g. "author", "container-title"
var AliasesCSL = map[string]string{
	"a":  "author",
	"t":  "title",
	"ca": "container-author",
	"ct": "container-title",
}

// Object classes aliases, to be used as filters: "=q" for "=quote".
var AliasesClasses = map[string]string{
	"p":  "person",
	"e":  "entry",
	"ex": "example",
	"q":  "quote",
	"c":  "concept",
}
