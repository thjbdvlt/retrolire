// Package tui - Retrolire Terminal User Interface
package tui

import (
	"strings"
)

const maxHistory = 100

type history struct {
	searches []string
	index    int
}

func initHistory() *history {
	return &history{
		searches: make([]string, maxHistory+1),
		index:    1,
	}
}

func (h *history) get(n int) string {
	h.index += n
	if h.index < 0 {
		h.index = 0
	} else if h.index >= maxHistory {
		h.index = maxHistory - 1
	}
	return h.searches[h.index]
}

// TODO: It should scroll, to avoid that "cap" at MaxHistory.
// FIXME: Now it's always requires double keypresses
func (h *history) forward() string  { return h.get(1) }
func (h *history) backward() string { return h.get(-1) }
func (h *history) add(s string) {
	h.searches[h.index] = strings.TrimSpace(s)
	h.index++
}
