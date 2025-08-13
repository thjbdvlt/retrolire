// Package main - command line bibliography manager
package main

import (
	"os"

	"otlet/internal/cli"
	"otlet/internal/state"
)

func main() {
	args := os.Args[1:]
	t := state.NewState(len(args) == 0 || args[0] != "init")
	cli.Call(t, args)
	t.CloseDB()
}
