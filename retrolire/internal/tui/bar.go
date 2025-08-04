// Package tui - Retrolire Terminal User Interface
package tui

import (
	"strconv"
	"strings"

	// tcell "github.com/gdamore/tcell/v2"
	"github.com/rivo/tview"
	"retrolire/internal/tui/elements"
)

// bar - Status and search bar at the top of the screen
type bar struct {
	Input   *tview.InputField // Current input field
	Number  *tview.TextView   // Number of items
	Filters *tview.TextView   // Active filters
	Grid    *tview.Grid       // Bar Grid
}

func (ui *UI) setMode(name string, uid elements.UID) {
	if uid == -1 { // Shortcut for List Navigation, i.e. no special mode
		ui.Input.SetLabel("")
		ui.Input.SetLabelStyle(ui.styles.UI[elements.ModeListNavigation])
	} else {
		ui.Input.SetLabel(name)
		ui.Input.SetLabelStyle(ui.styles.UI[uid])
	}
}

func (b *bar) setNumber(index, total int) {
	var sb strings.Builder
	sb.WriteString(strconv.Itoa(index))
	sb.WriteString("/")
	sb.WriteString(strconv.Itoa(total))
	str := sb.String()
	b.Number.SetText(str)
	if b.Grid != nil {
		b.Grid.SetColumns(0, len(str))
	}
}

func (b *bar) cleanInput() *tview.InputField {
	// Better than SetText("") because it doesn't trigger a Changed function
	b.Grid.RemoveItem(b.Input)
	b.Input = tview.NewInputField()
	b.Grid.AddItem(b.Input, 0, 0, 1, 1, 0, 0, false)
	return b.Input
}
