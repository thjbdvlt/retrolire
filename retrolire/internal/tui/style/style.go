// Package style - Defines colors for retrolire TUI
package style

import (
	tcell "github.com/gdamore/tcell/v2"

	conf "retrolire/internal/config"
	"retrolire/internal/obj"
	el "retrolire/internal/tui/elements"
)

// Styles - TUI styles
type Styles struct {
	UI       []tcell.Style
	ID       []tcell.Style
	Main     []tcell.Style
	Least    tcell.Style
	Selected tcell.Style
}

// FromConfig - Get styles from configuration
func FromConfig() Styles {
	var ds = tcell.Style{}
	var stylesID = make([]tcell.Style, obj.N)
	var stylesMain = make([]tcell.Style, obj.N)
	type styleItem struct {
		obj  obj.Class
		id   string
		main string
	}
	// Inventory styles
	for _, s := range []styleItem{
		{obj.Entry, conf.ColorEntryID, conf.ColorEntry},
		{obj.Concept, conf.ColorConceptID, conf.ColorConcept},
		{obj.Quote, conf.ColorQuoteID, conf.ColorQuote},
		{obj.Commentary, conf.ColorIdeaID, conf.ColorIdea},
		{obj.Example, conf.ColorExampleID, conf.ColorExample},
		{obj.Heading, conf.ColorHeadingID, conf.ColorHeading},
	} {
		stylesID[s.obj] = ds.Foreground(tcell.GetColor(s.id))
		stylesMain[s.obj] = ds.Foreground(tcell.GetColor(s.main))
	}
	// TUI styles
	var stylesUI = make([]tcell.Style, el.N)
	type styleUI struct {
		uid  el.UID
		bg   string
		fg   string
		bold bool
	}
	for _, s := range []styleUI{
		{el.ActiveFilters, conf.ColorActiveFilterBg, conf.ColorActiveFilterFg, false},
		{el.Background, conf.ColorBackground, conf.ColorBackground, false},
		{el.DeleteEntry, conf.ColorDeleteBg, conf.ColorDeleteFg, true},
		{el.ModeFilter, conf.ColorModeFilterBg, conf.ColorModeFilterFg, true},
		{el.ModeSearch, conf.ColorModeSearchBg, conf.ColorModeSearchFg, true},
		{el.ModeLabel, conf.ColorModeLabelBg, conf.ColorModeLabelFg, true},
		{el.ModeListNavigation, conf.ColorModeListNavigation, conf.ColorModeListNavigation, true},
		{el.NumberOfItems, conf.ColorNumberOfItemsBg, conf.ColorNumberOfItemsFg, false},
		{el.Preview, conf.ColorPreviewBg, conf.ColorPreviewFg, false},
		{el.Labels, conf.ColorLabelBg, conf.ColorLabelFg, false},
	} {
		stylesUI[s.uid] = ds.
			Background(tcell.GetColor(s.bg)).
			Foreground(tcell.GetColor(s.fg)).
			Bold(s.bold)
	}
	least := ds.
		Foreground(tcell.GetColor(conf.ColorSecondaryText)).
		Background(tcell.GetColor(conf.ColorBackground)).
		Italic(true)
	selected := ds.
		Background(tcell.GetColor(conf.ColorSelectedBg)).
		Foreground(tcell.GetColor(conf.ColorSelectedFg)).
		Bold(true)
	return Styles{
		UI:       stylesUI,
		ID:       stylesID,
		Main:     stylesMain,
		Least:    least,
		Selected: selected,
	}
}
