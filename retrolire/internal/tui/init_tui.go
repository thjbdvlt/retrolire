// Package tui - Retrolire Terminal User Interface
package tui

import (
	tcell "github.com/gdamore/tcell/v2"
	"github.com/rivo/tview"

	"retrolire/internal/config"
	"retrolire/internal/state"
	"retrolire/internal/tui/elements"
	"retrolire/internal/tui/style"
)

// We use a main grid and 2 subgrids:
//
//   Mode | Search | Infos   <= Subgrid 1
//   ---------------------
//   Active filters
//   ---------------------
//   Label | List item
//   Label | List item       <= Subgrid 2
//   Label | List
//   ...   | ...
//   ---------------------
//   Preview
//

// InitApp - Initialize the TUI application
func InitApp(t *state.State) {
	tview.Styles = tview.Theme{
		PrimitiveBackgroundColor: tcell.ColorWhite,
		PrimaryTextColor:         tcell.ColorBlack,
	}
	ui := &UI{
		App:    tview.NewApplication(),
		State:  t,
		styles: style.FromConfig(),
	}
	ui.App.SetTitle("retrolire")
	initStatusBar(ui)
	initPreview(ui)
	initCatalogue(ui)
	buildGrids(ui)
	if err := ui.App.SetRoot(ui.Grid, true).SetFocus(ui.catalogue).Run(); err != nil {
		panic(err)
	}
}

func initPreview(ui *UI) {
	ui.Preview = tview.NewTextView().
		SetDynamicColors(false).
		SetTextStyle(ui.styles.UI[elements.Preview])
}

func initStatusBar(ui *UI) {
	ui.Input = tview.NewInputField()
	ui.Number = tview.NewTextView().
		SetDynamicColors(false).
		SetTextStyle(ui.styles.UI[elements.NumberOfItems]).
		SetTextAlign(tview.AlignRight)
	ui.Filters = tview.NewTextView().
		SetTextStyle(ui.styles.UI[elements.ActiveFilters])
}

func initLabels(ui *UI) {
	ui.Labels = tview.NewList()
	ui.Labels.ShowSecondaryText(false).
		SetDoneFunc(func() {}).
		SetMainTextStyle(ui.styles.UI[elements.Labels]).
		SetSelectedStyle(ui.styles.UI[elements.Labels])
	for index, label := range []rune(config.JumpLabels) {
		ui.Labels.InsertItem(index, string(label), "", 0, nil)
	}
}

func initCatalogue(ui *UI) {
	ui.catalogue = newCatalogue(ui)
	ui.displayFromFilters("")
	if ui.GetItemCount() > 0 {
		ui.currentItem = 0
	}
	setListNavigationKey(ui)
	initLabels(ui)
}

func buildGrids(ui *UI) {
	ui.Grid = tview.NewGrid()
	ui.Grid.SetColumns(0).SetRows(1, 1, 0, 4).SetBorders(false)
	ui.bar.Grid = tview.NewGrid().SetColumns(0, 0).SetBorders(false)
	ui.catalogue.Grid = tview.NewGrid()
	ui.catalogue.Grid.SetColumns(1, 0).SetBorder(false)
	ui.catalogue.Grid.AddItem(ui.catalogue, 0, 1, 1, 3, 0, 0, false)
	ui.catalogue.Grid.AddItem(ui.Labels, 0, 0, 1, 1, 0, 0, false)
	ui.bar.Grid.AddItem(ui.Number, 0, 1, 1, 1, 0, 0, false)
	ui.Grid.AddItem(ui.bar.Grid, 0, 0, 1, 1, 0, 0, false)
	ui.Grid.AddItem(ui.Filters, 1, 0, 1, 1, 0, 0, false)
	ui.Grid.AddItem(ui.catalogue.Grid, 2, 0, 1, 1, 0, 0, false)
	ui.Grid.AddItem(ui.Preview, 3, 0, 1, 1, 0, 0, false)
}
