// Package tui - otlet Terminal User Interface
package tui

import (
	"strings"

	"otlet/internal/sqlmaker"
)

func (ui *UI) addFilter(newFilter string) {
	ui.displayFromFilters(ui.Filters.GetText() + " " + newFilter)
}

func (ui *UI) removeLastFilter() string {
	text := strings.TrimRight(ui.Filters.GetText(), " ")
	if index := strings.LastIndex(text, " "); index > 0 {
		return text[:index]
	}
	return ""
}

func (ui *UI) displayFromFilters(text string) {
	ui.stock = ui.getEntriesFromFilters(strings.Split(text, " "))
	ui.display(ui.stock)
	ui.Filters.SetText(text)
}

func (ui *UI) getEntriesFromFilters(filters []string) []*thing {
	db := ui.DB()
	stmt, params := sqlmaker.TuiStmtOrderBy().BuildSelect([]any{}, filters, ui)
	rows, err := db.Query(stmt, params...)
	ui.NoErr(err)
	ui.NoErr(rows.Err())
	return fromRows(rows)
}
