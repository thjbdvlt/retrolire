// Package main - command line bibliography manager
package main

import (
	"os"

	sqlite_vec "github.com/asg017/sqlite-vec-go-bindings/cgo"

	"retrolire/internal/cli"
	"retrolire/internal/fs"
	"retrolire/internal/state"
)

func main() {
	sqlite_vec.Auto()
	// /!\ Initialize state before changing directory
	t := state.NewState()
	// Change to the retrolire directory. This avoid building paths laters.
	fs.CD()
	cli.Call(t, os.Args[1:])
}
