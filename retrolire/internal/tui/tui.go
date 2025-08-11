// Package tui - Retrolire Terminal User Interface
package tui

import (
	"strconv"
	"strings"

	tcell "github.com/gdamore/tcell/v2"
	"github.com/rivo/tview"

	"retrolire/internal/actions"
	"retrolire/internal/config"
	"retrolire/internal/obj"
	"retrolire/internal/state"
	"retrolire/internal/tui/elements"
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
	Filters *tview.InputField
	styles  style.Styles
	Input   *tview.InputField
	*catalogue
	*state.State
}

const magicPrefixCosine = '*'
const magicPrefixTFS = '&'

func (ui *UI) displayFromText(text string) {
	text = strings.TrimSpace(text)
	if len(text) == 0 {
		return
	}
	switch text[0] {
	case magicPrefixTFS:
		ui.findFts(text[1:])
	case magicPrefixCosine:
		ui.findSimilar(text[1:])
	default:
		ui.displayFromFilters(text)
	}
}

// TODO: It could depends of previous command? E.g. "update".
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
	case obj.ClassName:
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

func (ui *UI) setMode(name string, uid elements.UID) {
	if uid == -1 { // Shortcut for List Navigation, i.e. no special mode
		ui.Input.SetLabel("")
		ui.Input.SetLabelStyle(ui.styles.UI[elements.ModeListNavigation])
	} else {
		ui.Input.SetLabel(name)
		ui.Input.SetLabelStyle(ui.styles.UI[uid])
	}
}

func (ui *UI) setNumber(index, total int) {
	var sb strings.Builder
	sb.WriteString(strconv.Itoa(index))
	sb.WriteString("/")
	sb.WriteString(strconv.Itoa(total))
	str := sb.String()
	ui.Filters.SetLabel(str)
}

func (ui *UI) cleanInput() *tview.InputField {
	ui.Grid.RemoveItem(ui.Input) // Better than SetText: doesn't trigger a Changed function
	ui.Input = tview.NewInputField()
	ui.Grid.AddItem(ui.Input, rowInput, 0, 1, 1, 0, 0, false)
	return ui.Input
}

func setListNavigationKey(ui *UI) {
	ui.SetInputCapture(func(event *tcell.EventKey) *tcell.EventKey {
		idx := ui.currentItem
		switch event.Key() {
		case tcell.KeyEnter:
			ui.operate()
		}
		maxIdx := len(ui.items) - 1
		switch event.Rune() {
		case config.KeyListSimilar:
			ui.findSimilar(ui.current().main)
			idx = 0
		case config.KeyListFlorilegeFind:
			ui.findFts(ui.current().main)
			idx = 0
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
			idx++
		case config.KeyListDown:
			idx--
		case config.KeyListPageUp:
			idx += config.PageStep
		case config.KeyListPageDown:
			idx -= config.PageStep
		case config.KeyListTop:
			idx = maxIdx
		case config.KeyListBottom:
			idx = 0
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
			// TODO: Key for Commented bibliography (i.e. collection)
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
