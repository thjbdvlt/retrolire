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

const rowPreview = 2
const rowInput = 1
const rowFilters = 3
const rowCatalogue = 0

// InitApp - Initialize the TUI application
func InitApp(t *state.MainState) {
	tview.Styles = tview.Theme{
		PrimitiveBackgroundColor: tcell.ColorWhite,
		PrimaryTextColor:         tcell.ColorBlack,
	}
	ui := &UI{
		App:       tview.NewApplication(),
		MainState: t,
		styles:    style.FromConfig(),
	}
	ui.App.SetTitle("retrolire")
	ui.Input = tview.NewInputField()
	ui.Filters = tview.NewInputField().
		SetFieldStyle(ui.styles.UI[elements.ActiveFilters]).
		SetLabelStyle(ui.styles.UI[elements.NumberOfItems])
	ui.Preview = tview.NewTextView().
		SetDynamicColors(false).
		SetTextStyle(ui.styles.UI[elements.Preview])
	initCatalogue(ui)
	buildGrids(ui)
	if err := ui.App.SetRoot(ui.Grid, true).SetFocus(ui.catalogue).Run(); err != nil {
		panic(err)
	}
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
	ui.Grid.SetColumns(0).SetRows(0, 1, 6, 1).SetBorders(false)
	ui.catalogue.Grid = tview.NewGrid()
	ui.catalogue.Grid.SetColumns(1, 0).SetBorder(false)
	ui.catalogue.Grid.AddItem(ui.catalogue, 0, 1, 1, 3, 0, 0, false)
	ui.catalogue.Grid.AddItem(ui.Labels, 0, 0, 1, 1, 0, 0, false)
	ui.Grid.AddItem(ui.Filters, rowFilters, 0, 1, 1, 0, 0, false)
	ui.Grid.AddItem(ui.catalogue.Grid, rowCatalogue, 0, 1, 1, 0, 0, false)
	ui.Grid.AddItem(ui.Preview, rowPreview, 0, 1, 1, 0, 0, false)
}
