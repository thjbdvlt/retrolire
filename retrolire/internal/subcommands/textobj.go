// Package subcommands - Subcommands
package subcommands

import (
	"retrolire/internal/state"
)

type citeTextObjCmd struct{ pickTextObjCmd }
type editTextObjCmd struct{ pickTextObjCmd }

func (citeTextObjCmd) Fn(t *state.State) { cite(t) }
func (editTextObjCmd) Fn(t *state.State) { edit(t) }
