// Package tui - Retrolire Terminal User Interface
package tui

import (
	"slices"
	"strings"

	tcell "github.com/gdamore/tcell/v2"
	"github.com/rivo/tview"

	"retrolire/internal/actions"
	"retrolire/internal/config"
	"retrolire/internal/obj"
	"retrolire/internal/tui/elements"
	"retrolire/internal/tui/mode"
)

type inputer struct {
	uid      elements.UID
	onChange func(*UI, string)
	onDone   func(*UI, *tview.InputField)
	noKeyMap bool
	initText string
	mode     mode.Mode
}

func (ui *UI) addInput(fn inputer) {
	in := ui.cleanInput()
	in.SetText(fn.initText)
	ui.setMode(fn.mode, fn.uid)
	if !fn.noKeyMap {
		setInputKeyMaps(in)
	}
	ui.App.SetFocus(in)
	in.SetDoneFunc(func(tcell.Key) {
		if fn.onDone != nil {
			fn.onDone(ui, in)
		}
		ui.App.SetFocus(ui.catalogue)
		if ui.GetItemCount() < 0 {
			ui.SetCurrentItem(0)
		}
		ui.setMode(mode.List, -1)
		ui.cleanInput()
	})
	if fn.onChange != nil {
		in.SetChangedFunc(func(text string) {
			fn.onChange(ui, text)
		})
	}
}

func (ui *UI) searchByFilter(initText string) {
	ui.addInput(inputer{
		mode:     mode.Filter,
		initText: initText,
		uid:      elements.ModeFilter,
		onDone: func(s *UI, in *tview.InputField) {
			text := s.Filters.GetText() + " " + in.GetText()
			s.displayFromText(text)
			s.Filters.SetText(text)
		},
	},
	)
}

func (ui *UI) searchOnKey(select1 bool) {
	ui.addInput(inputer{
		mode: mode.SearchOnKey,
		uid:  elements.ModeSearch,
		onChange: func(s *UI, text string) {
			s.display(filterItems(s.stock, text))
			if select1 && len(s.items) == 1 {
				s.setMode(mode.List, elements.ModeListNavigation)
				s.cleanInput()
				s.App.SetFocus(s.catalogue)
				s.operate()
			}
		},
	})
}

func (ui *UI) jumpLabel() {
	ui.addInput(inputer{
		mode:     mode.JumpLabel,
		uid:      elements.ModeLabel,
		noKeyMap: true,
		onChange: func(u *UI, text string) {
			for i, label := range []rune(config.JumpLabels) {
				if string(label) == text {
					u.cleanInput()
					_, _, _, height := ui.GetInnerRect()
					i = (height - i) - 1
					u.currentItem = i + u.offset
					u.App.SetFocus(u.catalogue)
					u.previewIndex(u.currentItem, len(u.items))
				}
			}
			u.App.SetFocus(u.catalogue)
			u.setMode(mode.List, -1)
		},
	})
}

func (ui *UI) confirmDelete() {
	item := ui.current()
	if item == nil || item.class != obj.Entry {
		return
	}
	ui.addInput(inputer{
		mode:     mode.Delete,
		uid:      elements.DeleteEntry,
		noKeyMap: true,
		onDone: func(s *UI, in *tview.InputField) {
			if strings.ToLower(strings.TrimSpace(in.GetText())) == "y" {
				err := actions.DeleteEntry(s, item.id)
				if err == nil {
					s.RemoveItem(s.GetCurrentItem())
				} else {
					s.Log(err)
				}
			}
		},
	})
}

func (ui *UI) editTagEntry() {
	item := ui.current()
	if item != nil && item.class == obj.Entry {
		ui.App.Suspend(func() {
			ui.Log(actions.EditEntryTags(ui, item.id))
		})
	}
}

func (ui *UI) chooseTag() {
	db := ui.Conn()
	tags := actions.GetTags(db)
	ui.Log(db.Close())
	ui.stock = fromSlice(tags, obj.Tag)
	ui.display(ui.stock)
	ui.searchOnKey(true)
}

func (ui *UI) chooseClass() {
	classes := obj.Names()
	classes = slices.DeleteFunc(classes, func(s string) bool { return s == "" })
	ui.stock = fromSlice(classes, obj.ClassName)
	ui.display(ui.stock)
	ui.searchOnKey(true)
}

func (ui *UI) chooseVar() {
	db := ui.Conn()
	fields := actions.GetFields(db)
	ui.Log(db.Close())
	ui.stock = fromSlice(fields, obj.Variable)
	ui.display(ui.stock)
	ui.searchOnKey(true)
}

func setInputKeyMaps(in *tview.InputField) {
	in.SetInputCapture(func(event *tcell.EventKey) *tcell.EventKey {
		m := tcell.ModNone
		k := tcell.KeyRune
		var r rune
		switch event.Rune() {
		case config.KeyInputBackwardKillWord:
			in.SetText(backwardKillWord(in.GetText()))
			return nil
		case config.KeyInputKillLine:
			k = tcell.KeyCtrlU
		case config.KeyInputBackwardWord:
			m, r = tcell.ModAlt, 'b'
		case config.KeyInputForwardWord:
			m, r = tcell.ModAlt, 'f'
		default:
			return event
		}
		return tcell.NewEventKey(k, r, m)
	})
}

func backwardKillWord(text string) string {
	const chars = " ,:\n\r\t"
	text = strings.TrimRight(text, chars)
	if idx := strings.LastIndexAny(text, chars); idx > 0 {
		return text[:idx+1]
	}
	return ""
}
