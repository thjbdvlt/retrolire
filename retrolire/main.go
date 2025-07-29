// Package main - command line bibliography manager
package main

import (
	"os"

	cmd "retrolire/internal/subcommands"
	"retrolire/internal/util"
)

// Retrolire - Main function for Retrolire
func main() {
	// Change to the retrolire directory. This avoid building paths laters.
	util.Check(os.Chdir(util.Dir()))
	cmd.Call(os.Args[1:])
}
