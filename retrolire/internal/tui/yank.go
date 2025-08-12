// Package tui - Retrolire Terminal User Interface
package tui

import (
	"fmt"

	"retrolire/internal/obj"
)

func (ui *UI) citationFromItem(item *thing) string {
	var citation string
	switch item.class {
	case obj.Entry:
		citation = fmt.Sprintf("[@%s]", item.id)
	case obj.Quote, obj.Commentary, obj.Concept, obj.Heading, obj.Example:
		var locator string
		db := ui.DB()
		err := db.QueryRow(`SELECT page FROM textobj WHERE entry = ? AND line = ?`).Scan(&locator)
		if err != nil || locator == "" {
			citation = fmt.Sprintf("[@%s]", item.id)
		} else {
			citation = fmt.Sprintf("[@%s, %s]", item.id, locator)
		}
	default:
		citation = ""
	}
	return citation
}

func (ui *UI) yankCitation(index int) {
	item := ui.item(index)
	if item.id == "" {
		return
	}
	citation := ui.citationFromItem(item)
	if citation != "" {
		ui.WriteClipBoard(citation)
	}
}

func (ui *UI) yankWithText(index int) {
	item := ui.item(index)
	if item.id == "" {
		return
	}
	citation := ui.citationFromItem(item)
	text := item.main
	if citation == "" {
		ui.WriteClipBoard(text)
	} else {
		ui.WriteClipBoard(fmt.Sprintf(`"%s%s"`, text, citation))
	}
}
