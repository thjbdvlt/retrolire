// Package tui - Retrolire Terminal User Interface
package tui

import (
	"database/sql"
	"slices"
	"strings"

	tcell "github.com/gdamore/tcell/v2"
	"github.com/rivo/tview"

	"retrolire/internal/config"
	"retrolire/internal/obj"
)

type thing struct {
	class obj.Class
	line  int
	id    string
	main  string
	least string
	lower string
}

type catalogue struct {
	stock       []*thing // items received from last query
	Idx         int
	Grid        *tview.Grid
	ui          *UI
	items       []*thing // displayed items
	offset      int
	currentItem int
	done        func()
	changed     func(int)
	*tview.Box
}

// put a string on the screen
func putText(text string, screen tcell.Screen, x, y, width int, style tcell.Style) int {
	for _, r := range text {
		x++
		if x > width {
			break
		}
		screen.SetContent(x, y, r, nil, style)
	}
	return x
}

// put a thing on the screen
func (ui *UI) put(th *thing, screen tcell.Screen, x, y, width int, selected bool) int {
	if th == nil {
		return x
	}
	const separator = "   "
	texts := []string{" ", " ", th.id, separator, th.main, separator, th.least}
	styles := []tcell.Style{
		ui.styles.Least,
		ui.styles.Least,
		ui.styles.ID[th.class],
		ui.styles.Least,
		ui.styles.Main[th.class],
		ui.styles.Least,
		ui.styles.Least,
		ui.styles.Least,
	}
	if selected {
		for i, s := range styles {
			styles[i] = s.Bold(true)
		}
		styles[0] = ui.styles.Selected
		texts[0] = config.SelectedSign
	}
	for i, t := range texts {
		x = putText(t, screen, x, y, width, styles[i])
	}
	return x
}

func newCatalogue(ui *UI) *catalogue {
	c := &catalogue{
		Box: tview.NewBox(), ui: ui,
	}
	c.done = func() { c.Clear() }
	// c.changed = func(index int) {
	// 	ui.setNumber(index, len(c.items))
	// }
	return c
}

// SetCurrentItem - Set the current item
func (c *catalogue) SetCurrentItem(index int) *catalogue {
	if index < 0 {
		index = len(c.items) + index
	}
	if index >= len(c.items) {
		index = len(c.items) - 1
	}
	if index < 0 {
		index = 0
	}
	c.currentItem = index
	return c
}

// GetOffset - Get the rows offset (how many items before the first displayed).
func (c *catalogue) GetCurrentItem() int {
	return c.currentItem
}

// RemoveItem - Remove an item.
func (c *catalogue) RemoveItem(index int) *catalogue {
	if len(c.items) == 0 {
		return c
	}
	if index < 0 {
		index = len(c.items) + index
	}
	if index >= len(c.items) {
		index = len(c.items) - 1
	}
	if index < 0 {
		index = 0
	}
	c.items = append(c.items[:index], c.items[index+1:]...)
	if len(c.items) == 0 {
		return c
	}
	if c.currentItem > index || c.currentItem == len(c.items) {
		c.currentItem--
	}
	return c
}

// InsertItem - Insert an item. If index is -1, append to the end of the inventory.
func (c *catalogue) InsertItem(index int, id, main, least string, class obj.Class) *catalogue {
	item := &thing{
		id:    id,
		main:  main,
		least: least,
		class: class,
	}
	// TODO: Clean this function
	if index < 0 {
		index = len(c.items) + index + 1
	}
	if index < 0 {
		index = 0
	} else if index > len(c.items) {
		index = len(c.items)
	}
	// Shift current item.
	if c.currentItem < len(c.items) && c.currentItem >= index {
		c.currentItem++
	}
	// Insert item (make space for the new item, then shift and insert).
	c.items = append(c.items, nil)
	if index < len(c.items)-1 { // -1 because l.items has already grown by one item.
		copy(c.items[index+1:], c.items[index:])
	}
	c.items[index] = item
	return c
}

// GetItemCount returns the number of items in the list.
func (c *catalogue) GetItemCount() int { return len(c.items) }

// Clear removes all items from the list.
func (c *catalogue) Clear() *catalogue {
	c.items = nil
	c.currentItem = 0
	return c
}

// Draw draws this primitive onto the screen.
func (c *catalogue) Draw(screen tcell.Screen) {
	c.DrawForSubclass(screen, c)
	x, y, width, height := c.GetInnerRect()
	bottomLimit := y + height
	_, totalHeight := screen.Size()
	if bottomLimit > totalHeight {
		bottomLimit = totalHeight
	}
	if height == 0 {
		return
	}
	if c.currentItem < c.offset {
		c.offset = c.currentItem
	} else if c.currentItem-c.offset >= height {
		c.offset = c.currentItem + 1 - height
	}
	for index, item := range c.items {
		if index < c.offset {
			continue
		}
		if y >= bottomLimit {
			break
		}
		if y >= bottomLimit {
			break
		}
		y++
		_ = c.ui.put(item, screen, x, height-y, width, index == c.currentItem)
	}
}

func fromSlice(s []string, class obj.Class) []*thing {
	var items []*thing
	for _, i := range s {
		items = append(items, &thing{
			class: class,
			lower: strings.ToLower(i),
			main:  i,
		})
	}
	return items
}

func fromRows(rows *sql.Rows) []*thing {
	var items []*thing
	var n int
	if rows != nil {
		var b strings.Builder
		for rows.Next() {
			b.Reset()
			th := &thing{}
			check(rows.Scan(&th.id, &th.line, &th.main, &th.least, &th.class))
			b.WriteString(th.main)
			b.WriteString("\n")
			b.WriteString(th.least)
			th.lower = strings.ToLower(b.String())
			items = append(items, th)
			n++
		}
	}
	return items
}

func containsAny(s string, searches []string) bool {
	for _, i := range searches {
		if !strings.Contains(s, i) {
			return false
		}
	}
	return true
}

// Returns a copy of Items, but filter using search.
// The search is split in many searches on spaces (" ").
// To search a pattern containing space, replace spaces by underscores.
// (It's not possible to search for underscores with this syntax.)
func filterItems(items []*thing, search string) []*thing {
	searches := strings.Split(search, " ")
	for i, s := range searches {
		searches[i] = strings.ReplaceAll(strings.TrimSpace(s), "_", " ")
	}
	return slices.DeleteFunc(slices.Clone(items), func(t *thing) bool {
		return !containsAny(t.lower, searches)
	})
}

func (c *catalogue) item(index int) *thing {
	if len(c.items) == 0 || index >= len(c.items) {
		return &thing{}
	}
	return c.items[index]
}

func (c *catalogue) current() *thing {
	cur := c.GetCurrentItem()
	if cur >= len(c.items) {
		return &thing{}
	}
	return c.item(cur)
}

func (ui *UI) display(items []*thing) {
	ui.items = items
	ui.currentItem = 0
	ui.previewIndex(0, len(items))
}
