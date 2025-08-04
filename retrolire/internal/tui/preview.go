// Package tui - Retrolire Terminal User Interface
package tui

func (ui *UI) previewIndex(index, total int) {
	item := ui.item(index)
	if item != nil {
		ui.Preview.SetText(item.main)
		ui.setNumber(index+1, total)
	} else {
		ui.Preview.SetText("")
	}
}

func (ui *UI) scrollPreview(n int) {
	row, _ := ui.Preview.GetScrollOffset()
	ui.Preview.ScrollTo(row+n, 0)
}
