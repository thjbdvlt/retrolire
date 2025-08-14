// Package main - command line bibliography manager
package main

import (
	"os"

	"polyanthea/internal/cli"
	"polyanthea/internal/state"
)

func main() {
	args := os.Args[1:]
	t := state.NewState(len(args) == 0 || args[0] != "init")
	cli.Call(t, args)
	t.CloseDB()
}
