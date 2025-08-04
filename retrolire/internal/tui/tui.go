// Package tui - Retrolire Terminal User Interface
package tui

import (
	"strings"

	tcell "github.com/gdamore/tcell/v2"
	"github.com/rivo/tview"

	"retrolire/internal/actions"
	"retrolire/internal/config"
	"retrolire/internal/obj"
	"retrolire/internal/sqlmaker"
	"retrolire/internal/state"
	"retrolire/internal/tui/style"
	"retrolire/internal/util"
)

var check = util.Check

// UI - User interface
type UI struct {
	App     *tview.Application
	Grid    *tview.Grid
	Preview *tview.TextView
	Labels  *tview.List
	styles  style.Styles
	bar
	*catalogue
	*state.State
}

func (ui *UI) displayFromFilters(text string) {
	filters := strings.Split(text, " ")
	ui.stock = getEntriesFromFilters(ui, filters)
	ui.display(ui.stock)
	ui.Filters.SetText(text)
}

func getEntriesFromFilters(t *UI, filters []string) []*thing {
	db := t.Conn()
	stmt, params := sqlmaker.TuiStmtOrderBy().BuildSelect([]any{}, filters)
	rows, err := db.Query(stmt, params...)
	check(err, rows.Err(), db.Close())
	return fromRows(rows)
}

func (ui *UI) operate() {
	item := ui.current()
	if item == nil {
		return
	}
	switch item.class {
	// Some classes are edited
	case obj.Person:
		ui.App.Suspend(func() { ui.Log(actions.EditPerson(ui.State, item.main)) })
	case obj.Entry:
		ui.App.Suspend(func() { ui.Log(actions.EditEntry(ui.State, item.id)) })
	case obj.Concept, obj.Quote, obj.Idea, obj.Heading, obj.Example:
		ui.App.Suspend(func() { ui.Log(actions.EditEntryLine(ui.State, item.id, item.line)) })
	// Some classes are not edited but added to the filters stack
	case obj.Tag:
		ui.addFilter("." + item.main)
	case obj.Class:
		ui.addFilter("=" + item.main)
	case obj.Variable:
		ui.searchByFilter(item.main + ":")
	default:
	}
}

func openCurrentItem(ui *UI) {
	item := ui.current()
	if item != nil {
		ui.App.Suspend(func() {
			ui.Log(actions.OpenEntryURL(ui.State, item.id))
		})
	}
}

func (ui *UI) addFilter(newFilter string) {
	ui.displayFromFilters(ui.Filters.GetText(false) + " " + newFilter)
}

func (ui *UI) removeLastFilter() string {
	text := strings.TrimRight(ui.Filters.GetText(false), " ")
	if index := strings.LastIndex(text, " "); index > 0 {
		return text[:index]
	}
	return ""
}

func setListNavigationKey(ui *UI) {
	ui.SetInputCapture(func(event *tcell.EventKey) *tcell.EventKey {
		idx := ui.currentItem
		switch event.Key() {
		case tcell.KeyBackspace: // TODO
		case tcell.KeyEscape: // TODO
		case tcell.KeyEnter:
			ui.operate()
		}
		maxIdx := len(ui.items) - 1
		switch event.Rune() {
		case config.KeyListSearchOnKeyStroke:
			ui.searchOnKey(false)
		case config.KeyListFilterAdd:
			ui.searchByFilter("")
		case config.KeyListFiltersNew:
			ui.Filters.SetText("")
			ui.searchByFilter("")
		case config.KeyListFilterRemoveLast:
			ui.displayFromFilters(ui.removeLastFilter())
		case config.KeyListResetSearch:
			ui.displayFromFilters("")
		case config.KeyListJumpLabel:
			ui.jumpLabel()
		case config.KeyListOpen:
			openCurrentItem(ui)
		case config.KeyListTag:
			ui.chooseTag()
		case config.KeyListPreviewScrollDown:
			ui.scrollPreview(1)
		case config.KeyListPreviewScrollUp:
			ui.scrollPreview(-1)
		case config.KeyListUp:
			idx--
		case config.KeyListDown:
			idx++
		case config.KeyListPageUp:
			idx -= config.PageStep
		case config.KeyListPageDown:
			idx += config.PageStep
		case config.KeyListTop:
			idx = 0
		case config.KeyListBottom:
			idx = maxIdx
		case config.KeyListQuit:
			ui.App.Stop()
		case config.KeyListClassPick:
			ui.chooseClass()
		case config.KeyListDelete:
			ui.confirmDelete()
		case config.KeyListEditTags:
			ui.editTagEntry()
		case config.KeyListAddVar:
			ui.chooseVar()
		case config.KeyListUpdate: // TODO
		case config.KeyListHelp: // TODO
			// TODO: Key for florilège
			// TODO: Key for compilation (if different than florilege)
			// TODO: Key for COMMENTED bibliography (i.e. collection)
			// TODO: Key for concordance
		}
		if idx < 0 {
			idx = maxIdx
		} else if idx > maxIdx {
			idx = 0
		}
		ui.currentItem = idx
		ui.previewIndex(idx, len(ui.items))
		return nil
	})
}
