// Package tui - Retrolire Terminal User Interface
package tui

import (
	"strings"

	"retrolire/internal/sqlmaker"
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
	db := ui.Conn()
	stmt, params := sqlmaker.TuiStmtOrderBy().BuildSelect([]any{}, filters)
	rows, err := db.Query(stmt, params...)
	check(err, rows.Err(), db.Close())
	return fromRows(rows)
}
